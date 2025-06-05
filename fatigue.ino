#include "fatiguer.h"    // AppIMPInit, AppIMPCtrl, AppIMPISR, AppIMPCfg_Type
#include <LibPrintf.h>
#include "ad5940.h"

// ==== Paramètres généraux ====
#define APPBUFF_SIZE 512
uint32_t AppBuff[APPBUFF_SIZE];

// ==== Affichage des résultats ====
int32_t ImpedanceShowResult(uint32_t *pData, uint32_t DataCount)
{
  float freq;
  fImpPol_Type *pImp = (fImpPol_Type*)pData;
  AppIMPCtrl(IMPCTRL_GETFREQ, &freq);

  printf("Freq: %.2f Hz\n", freq);
  for(int i = 0; i < DataCount; i++)
    printf("RzMag: %8.2f Ohm, RzPhase: %7.2f deg\n", pImp[i].Magnitude, pImp[i].Phase * 180.0f / MATH_PI);
  return 0;
}

// ==== Configuration matérielle AD5940 ====
static int32_t AD5940PlatformCfg(void)
{
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  AGPIOCfg_Type gpio_cfg;


  AD5940_HWReset();
  
  AD5940_Initialize();

  // Activation du bit 3 de SWMUX pour le Common Mode entre AIN2 et AIN3
  uint32_t swmux = AD5940_ReadReg(REG_AFE_SWMUX);
  Serial.print("SWMUX avant = 0x"); Serial.println(swmux, HEX);

  swmux |= BITM_AFE_SWMUX_CMMUX;
  AD5940_WriteReg(REG_AFE_SWMUX, swmux);

  swmux = AD5940_ReadReg(REG_AFE_SWMUX); // relire pour vérifier
  Serial.print("SWMUX après = 0x"); Serial.println(swmux, HEX);

  // Vérification communication SPI
  uint16_t chipid = AD5940_ReadReg(0x00000404);
  Serial.print("CHIPID = 0x"); Serial.println(chipid, HEX);

  // Configuration horloge
  clk_cfg.ADCClkDiv = ADCCLKDIV_1;
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC;
  clk_cfg.SysClkDiv = SYSCLKDIV_1;
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC;
  clk_cfg.HfOSC32MHzMode = bFALSE;
  clk_cfg.HFOSCEn = bTRUE;
  clk_cfg.HFXTALEn = bFALSE;
  clk_cfg.LFOSCEn = bTRUE;
  AD5940_CLKCfg(&clk_cfg);


  // FIFO configuration
  fifo_cfg.FIFOEn = bFALSE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  fifo_cfg.FIFOThresh = 4;
  AD5940_FIFOCfg(&fifo_cfg);
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);

  // INT configuration
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

  // GPIO (optionnel, pour debug ou sync)
  gpio_cfg.FuncSet = GP0_INT | GP1_SLEEP | GP2_SYNC | SWD_AFE3;
  gpio_cfg.InputEnSet = 0;
  gpio_cfg.OutputEnSet = AGPIO_Pin0 | AGPIO_Pin1 | AGPIO_Pin2;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;
  AD5940_AGPIOCfg(&gpio_cfg);

  return 0;
}

// ==== Configuration application impédance (4-wire) ====
extern AppIMPCfg_Type AppIMPCfg;
void AD5940ImpedanceStructInit(void)
{
  AppIMPCfg_Type *pImpedanceCfg;
  
  AppIMPGetCfg(&pImpedanceCfg);
  /* Step1: configure initialization sequence Info */
  pImpedanceCfg->SeqStartAddr = 0;
  pImpedanceCfg->MaxSeqLen = 512;

  pImpedanceCfg->RcalVal = 10000.0;
  pImpedanceCfg->SinFreq = 60000.0;
  pImpedanceCfg->FifoThresh = 4;

  /* Switch matrix 4-wire */
  pImpedanceCfg->DswitchSel = SWD_CE0;
  pImpedanceCfg->PswitchSel = SWP_AIN2;
  pImpedanceCfg->NswitchSel = SWN_AIN3;
  pImpedanceCfg->TswitchSel = SWT_AIN1;

  /* RTIA pour la gamme attendue */
  pImpedanceCfg->HstiaRtiaSel = HSTIARTIA_5K;

  /* Sweep */
  pImpedanceCfg->SweepCfg.SweepEn = bTRUE;
  pImpedanceCfg->SweepCfg.SweepStart = 100.0f;
  pImpedanceCfg->SweepCfg.SweepStop = 100000.0f;
  pImpedanceCfg->SweepCfg.SweepPoints = 101;
  pImpedanceCfg->SweepCfg.SweepLog = bTRUE;

  /* Puissance & filtres */
  pImpedanceCfg->PwrMod = AFEPWR_LP;
  pImpedanceCfg->ADCSinc3Osr = ADCSINC3OSR_2;
  pImpedanceCfg->DftNum = DFTNUM_16384;
  pImpedanceCfg->DftSrc = DFTSRC_SINC3;
}

// ==== SETUP / LOOP ====

void setup() {
  Serial.begin(115200);
  while (!Serial); // Attendre l'ouverture du port série

  AD5940_MCUResourceInit(NULL); 
  AD5940PlatformCfg();
  AD5940ImpedanceStructInit();

  int ret = AppIMPInit(AppBuff, APPBUFF_SIZE);
  Serial.print("AppIMPInit return = "); Serial.println(ret);
  if (ret != 0) {
    Serial.println("ERREUR: AppIMPInit a échoué !");
    while (1);
  }

  ret = AppIMPCtrl(IMPCTRL_START, 0);
  Serial.print("AppIMPCtrl(START) return = "); Serial.println(ret);
  if (ret != 0) {
    Serial.println("ERREUR: AppIMPCtrl(START) a échoué !");
    while (1);
  }

  float freq = 0;
  AppIMPCtrl(IMPCTRL_GETFREQ, &freq);
  Serial.print("Fréquence sweep active (après start) = "); Serial.println(freq, 2);

  Serial.println("AD5940 Impedance Test - Init complete!");
}

void loop() {
  uint32_t temp = APPBUFF_SIZE;
  AppIMPISR(AppBuff, &temp);
  if (temp) {
    ImpedanceShowResult(AppBuff, temp);
  } 
  delay(1000);
}