#include "conductivity.h"
#include <LibPrintf.h>
#include "ad5940.h"
#include "rcal.h"

// === Paramètres généraux ===
#define APPBUFF_SIZE 512
#define DUREE_MESURE_S 10

// === CONSTANTE DE CELLULE ===
#define CELL_CONSTANT 1.42  // cm^-1 (à calibrer selon la sonde !)

uint32_t AppBuff[APPBUFF_SIZE];
unsigned long chronoStart = 0;
bool mesureActive = true;

// === Affichage propre des résultats ===
int32_t ImpedanceShowResult(uint32_t *pData, uint32_t DataCount) {
  static int headerPrinted = 0;
  float freq;
  fImpPol_Type *pImp = (fImpPol_Type*)pData;
  AppIMPCtrl(IMPCTRL_GETFREQ, &freq);

  float secondes = (millis() - chronoStart) / 1000.0;
  if (!headerPrinted) {
    printf("  Temps(s)    Freq(Hz)   Rz(Mag)     Rz(Phase °)  Conductivite (uS/cm)\n");
    printf("--------------------------------------------------------------------------\n");
    headerPrinted = 1;
  }
  for (int i = 0; i < DataCount; i++) {
    float R = pImp[i].Magnitude; // |Z| en ohms
    float conductivite = 0.0f;
    if(R > 0.0f) {
      conductivite = CELL_CONSTANT / R;      // en S/cm
      conductivite = conductivite * 1e6f;    // en µS/cm
    }
    printf("%9.2f  %9.2f  %10.3f  %12.3f  %15.2f\n",
      secondes,
      freq,
      R,
      pImp[i].Phase * 180.0f / MATH_PI,
      conductivite
    );
  }
  return 0;
}

// === Calibration ===
void Calibrate_HSDAC_and_ADC(void) {
  HSDACCal_Type hsdac_cal;
  ADCPGACal_Type adcpga_cal;

  adcpga_cal.AdcClkFreq = 4000000;
  adcpga_cal.ADCPga = ADCPGA_1;
  adcpga_cal.ADCSinc2Osr = ADCSINC2OSR_1333;
  adcpga_cal.ADCSinc3Osr = ADCSINC3OSR_2;
  adcpga_cal.PGACalType = PGACALTYPE_OFFSET;
  adcpga_cal.TimeOut10us = 1000;
  adcpga_cal.VRef1p11 = 1.11;
  adcpga_cal.VRef1p82 = 1.82;
  AD5940_ADCPGACal(&adcpga_cal);

  hsdac_cal.ExcitBufGain = EXCITBUFGAIN_2;
  hsdac_cal.HsDacGain = HSDACGAIN_0P2;
  hsdac_cal.AfePwrMode = AFEPWR_HP;
  AD5940_HSDACCal(&hsdac_cal);
}

// === Configuration matérielle AD5940 ===
static int32_t AD5940PlatformCfg(void) {
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;

  AD5940_HWReset();
  AD5940_Initialize();
  AD5940_CsSet();
  AD5940_RstSet();

  // Vérification communication SPI (affichage minimal)
  uint16_t chipid = AD5940_ReadReg(0x00000404);
  printf("CHIPID = 0x%04X\n", chipid);

  // Horloge
  clk_cfg.ADCClkDiv = ADCCLKDIV_1;
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC;
  clk_cfg.SysClkDiv = SYSCLKDIV_1;
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC;
  clk_cfg.HfOSC32MHzMode = bFALSE;
  clk_cfg.HFOSCEn = bTRUE;
  clk_cfg.HFXTALEn = bFALSE;
  clk_cfg.LFOSCEn = bTRUE;
  AD5940_CLKCfg(&clk_cfg);

  // FIFO
  fifo_cfg.FIFOEn = bFALSE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  fifo_cfg.FIFOThresh = 4;
  AD5940_FIFOCfg(&fifo_cfg);
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);

  // INT
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

  return 0;
}

// === Configuration structure impédance ===
extern AppIMPCfg_Type AppIMPCfg;
void AD5940ImpedanceStructInit(void) {
  AppIMPCfg_Type *pImpedanceCfg;
  AppIMPGetCfg(&pImpedanceCfg);

  pImpedanceCfg->SeqStartAddr = 0;
  pImpedanceCfg->MaxSeqLen = 512;
  pImpedanceCfg->RcalVal = 10000.0f;
  pImpedanceCfg->SinFreq = 10000.0;
  pImpedanceCfg->FifoThresh = 4;
  pImpedanceCfg->DswitchSel = SWD_CE0;
  pImpedanceCfg->PswitchSel = SWP_AIN2;
  pImpedanceCfg->NswitchSel = SWN_AIN3;
  pImpedanceCfg->TswitchSel = SWT_AIN1;
  pImpedanceCfg->HstiaRtiaSel = HSTIARTIA_10K;
  pImpedanceCfg->SweepCfg.SweepEn = bFALSE;
  pImpedanceCfg->SweepCfg.SweepStart = 1000.0f;
  pImpedanceCfg->SweepCfg.SweepStop = 10000.0f;
  pImpedanceCfg->SweepCfg.SweepPoints = 101;
  pImpedanceCfg->SweepCfg.SweepLog = bTRUE;
  pImpedanceCfg->PwrMod = AFEPWR_HP;
  pImpedanceCfg->ADCSinc3Osr = ADCSINC3OSR_2;
  pImpedanceCfg->DftNum = DFTNUM_16384;
  pImpedanceCfg->DftSrc = DFTSRC_SINC3;
}

// === Setup principal ===
void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  select_r_cal(3);

  AD5940_MCUResourceInit(NULL);
  AD5940PlatformCfg();
  Calibrate_HSDAC_and_ADC();
  AD5940ImpedanceStructInit();

  // Réveil puce
  int tries = AD5940_WakeUp(10);
  if (tries > 10) {
    printf("Erreur réveil AD5940: SPI\n");
  }

  // Init App
  int ret = AppIMPInit(AppBuff, APPBUFF_SIZE);
  if (ret != 0) {
    printf("AppIMPInit echec !\n");
  }

  // Démarrage acquisition
  ret = AppIMPCtrl(IMPCTRL_START, 0);
  if (ret != 0) {
    printf("AppIMPCtrl(START) echec !\n");
  }

  chronoStart = millis();
  mesureActive = true;
}

// === Boucle principale ===
void loop() {
  if (mesureActive) {
    uint32_t temp = APPBUFF_SIZE;
    AppIMPISR(AppBuff, &temp);
    if (temp) {
      ImpedanceShowResult(AppBuff, temp);
    }
  
  }
}