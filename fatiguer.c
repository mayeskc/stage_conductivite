#include "ad5940.h"
#include <stdio.h>
#include "string.h"
#include "math.h"
#include "fatiguer.h"
//include the printf.h from the used arduino printf library as it forwards all printf statements to the Serial object!
//see https://www.avrfreaks.net/forum/how-use-printf-statements-external-c-file-arduino
#include "../extras/printf/printf.h"

/* Default LPDAC resolution(2.5V internal reference). */
#define DAC12BITVOLT_1LSB   (2200.0f/4095)  //mV
#define DAC6BITVOLT_1LSB    (DAC12BITVOLT_1LSB*64)  //mV



/**
 * @brief The ramp application paramters.
 * @details Do not modify following default parameters. Use the function in AD5940Main.c to change it.
 *
 * */
AppIMPCfg_Type AppIMPCfg = 
{
  .bParaChanged = bTRUE,
  .SeqStartAddr = 0, // Initialaztion sequence start address in SRAM of AD5940
  .MaxSeqLen = 0, //Limit the maximum sequence.
  
  .SeqStartAddrCal = 0, // Measurement sequence start address in SRAM of AD5940
  .MaxSeqLenCal = 0,    // Limit the maximum sequence.

  .ImpODR = 20.0,           /* 20.0 Hz*/ 
  .NumOfData = -1,  //
  .SysClkFreq = 16000000.0,    // 16MHz
  .WuptClkFreq = 32000.0, // 32kHz
  .AdcClkFreq = 16000000.0,// 16MHz
  .RcalVal = 10000.0,     // 10kOhm 

   // Switch matrix configuration
  .DswitchSel = SWD_CE0,
  .PswitchSel = SWP_AIN2, 
  .NswitchSel = SWN_AIN3,
  .TswitchSel = SWT_AIN1,

  .PwrMod = AFEPWR_HP, //   Power mode

  .HstiaRtiaSel = HSTIARTIA_5K, // RTIA : transforme le courant en tension mesurable
  .ExcitBufGain = EXCITBUFGAIN_2, //  gain du buffer d'excitation
  .HsDacGain = HSDACGAIN_1, //  gain du DAC de haute vitesse
  .HsDacUpdateRate = 7,   //  update rate of DAC, 7 means 1/8 of system clock
  .DacVoltPP = 800.0, //  DAC output voltage in mV peak to peak. Maximum value is 800mVpp. Peak to peak voltage
  .BiasVolt = -0.0f, //  The excitation signal is DC+AC. This parameter decides the DC value in mV unit. 0.0mV means no DC bias.

  .SinFreq = 600000.0, /* 1000Hz */

  .DftNum = DFTNUM_16384,     // NUMERO DE TRANSFORMEE DE FOURIER
  .DftSrc = DFTSRC_SINC3,   // DFT Source a la sortie du filtre sinc3
  .HanWinEn = bTRUE,    // reduire effets de leakage

  .AdcPgaGain = ADCPGA_1, // Amplifie la tension d'entrée de 1x
  .ADCSinc3Osr = ADCSINC3OSR_2,
  .ADCSinc2Osr = ADCSINC2OSR_22,

  .ADCAvgNum = ADCAVGNUM_16,

  .SweepCfg.SweepEn = bTRUE, // balayage de frequence
  .SweepCfg.SweepStart = 1000, // frequence de debut du balayage
  .SweepCfg.SweepStop = 100000.0,  // frequence de fin du balayage
  .SweepCfg.SweepPoints = 3, // nombre de points de mesure
  .SweepCfg.SweepLog = bFALSE, // 0: Linear, 1: Logarithmic
  .SweepCfg.SweepIndex = 0, // Current position du balayage

  .FifoThresh = 4,
  .IMPInited = bFALSE,
  .StopRequired = bFALSE,
};

/**
 * @todo add paramater check.
 * SampleDelay will limited by wakeup timer, check WUPT register value calculation equation below for reference.
 * SampleDelay > 1.0ms is acceptable.
 * RampDuration/StepNumber > 2.0ms
 * ...
 * */


/**
 * @brief This function is provided for upper controllers that want to change
 *        application parameters specially for user defined parameters.
 * @param pCfg: The pointer used to store application configuration structure pointer.
 * @return none.
*/
int32_t AppIMPGetCfg(void *pCfg)
{
  if(pCfg)
  {
    *(AppIMPCfg_Type**)pCfg = &AppIMPCfg;
    return AD5940ERR_OK;
  }
  return AD5940ERR_PARA;
}

int32_t AppIMPCtrl(uint32_t Command, void *pPara)
{
  
  switch (Command)
  {
    case IMPCTRL_START:
    {
      WUPTCfg_Type wupt_cfg;

      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      if(AppIMPCfg.IMPInited == bFALSE)
        return AD5940ERR_APPERROR;
      /* Start it */
      wupt_cfg.WuptEn = bTRUE;
      wupt_cfg.WuptEndSeq = WUPTENDSEQ_A;
      wupt_cfg.WuptOrder[0] = SEQID_0;
      wupt_cfg.SeqxSleepTime[SEQID_0] = 4;
      wupt_cfg.SeqxWakeupTime[SEQID_0] = (uint32_t)(AppIMPCfg.WuptClkFreq/AppIMPCfg.ImpODR)-4;
      AD5940_WUPTCfg(&wupt_cfg);
      
      AppIMPCfg.FifoDataCount = 0;  /* restart */
      break;
    }
    case IMPCTRL_STOPNOW:
    {
      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      /* Start Wupt right now */
      AD5940_WUPTCtrl(bFALSE);
      /* There is chance this operation will fail because sequencer could put AFE back 
        to hibernate mode just after waking up. Use STOPSYNC is better. */
      AD5940_WUPTCtrl(bFALSE);
      break;
    }
    case IMPCTRL_STOPSYNC:
    {
      AppIMPCfg.StopRequired = bTRUE;
      break;
    }
    case IMPCTRL_GETFREQ:
      {
        if(pPara == 0)
          return AD5940ERR_PARA;
        if(AppIMPCfg.SweepCfg.SweepEn == bTRUE)
          *(float*)pPara = AppIMPCfg.FreqofData;
        else
          *(float*)pPara = AppIMPCfg.SinFreq;
      }
    break;
    case IMPCTRL_SHUTDOWN:
    {
      AppIMPCtrl(IMPCTRL_STOPNOW, 0);  /* Stop the measurement if it's running. */
      /* Turn off LPloop related blocks which are not controlled automatically by hibernate operation */
      AFERefCfg_Type aferef_cfg;
      LPLoopCfg_Type lp_loop;
      memset(&aferef_cfg, 0, sizeof(aferef_cfg));
      AD5940_REFCfgS(&aferef_cfg);
      memset(&lp_loop, 0, sizeof(lp_loop));
      AD5940_LPLoopCfgS(&lp_loop);
      AD5940_EnterSleepS();  /* Enter Hibernate */
    }
    break;
    default:
    break;
  }
  return AD5940ERR_OK;
}

/* generated code snnipet */
float AppIMPGetCurrFreq(void)
{
  if(AppIMPCfg.SweepCfg.SweepEn == bTRUE)
    return AppIMPCfg.FreqofData;
  else
    return AppIMPCfg.SinFreq;
}

/* Application initialization */
static AD5940Err AppIMPSeqCfgGen(void)
{
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  AFERefCfg_Type aferef_cfg;
  HSLoopCfg_Type HsLoopCfg;
  DSPCfg_Type dsp_cfg;
  float sin_freq;

  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);
  
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);  /* Init all to disable state */

  aferef_cfg.HpBandgapEn = bTRUE;
  aferef_cfg.Hp1V1BuffEn = bTRUE;
  aferef_cfg.Hp1V8BuffEn = bTRUE;
  aferef_cfg.Disc1V1Cap = bFALSE;
  aferef_cfg.Disc1V8Cap = bFALSE;
  aferef_cfg.Hp1V8ThemBuff = bFALSE;
  aferef_cfg.Hp1V8Ilimit = bFALSE;
  aferef_cfg.Lp1V1BuffEn = bFALSE;
  aferef_cfg.Lp1V8BuffEn = bFALSE;
  /* LP reference control - turn off them to save power*/
  if(AppIMPCfg.BiasVolt != 0.0f)    /* With bias voltage */
  {
    aferef_cfg.LpBandgapEn = bTRUE;
    aferef_cfg.LpRefBufEn = bTRUE;
  }
  else
  {
    aferef_cfg.LpBandgapEn = bFALSE;
    aferef_cfg.LpRefBufEn = bFALSE;
  }
  aferef_cfg.LpRefBoostEn = bFALSE;
  AD5940_REFCfgS(&aferef_cfg);	
  HsLoopCfg.HsDacCfg.ExcitBufGain = AppIMPCfg.ExcitBufGain;
  HsLoopCfg.HsDacCfg.HsDacGain = AppIMPCfg.HsDacGain;
  HsLoopCfg.HsDacCfg.HsDacUpdateRate = AppIMPCfg.HsDacUpdateRate;

  HsLoopCfg.HsTiaCfg.DiodeClose = bFALSE;
	if(AppIMPCfg.BiasVolt != 0.0f)    /* With bias voltage */
		HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_VZERO0;
	else
		HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;
  HsLoopCfg.HsTiaCfg.HstiaCtia = 31; /* 31pF + 2pF */
  HsLoopCfg.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  HsLoopCfg.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_OPEN;
  HsLoopCfg.HsTiaCfg.HstiaRtiaSel = AppIMPCfg.HstiaRtiaSel;

  HsLoopCfg.SWMatCfg.Dswitch = AppIMPCfg.DswitchSel;
  HsLoopCfg.SWMatCfg.Pswitch = AppIMPCfg.PswitchSel;
  HsLoopCfg.SWMatCfg.Nswitch = AppIMPCfg.NswitchSel;
  HsLoopCfg.SWMatCfg.Tswitch = SWT_TRTIA|AppIMPCfg.TswitchSel;

  HsLoopCfg.WgCfg.WgType = WGTYPE_SIN;
  HsLoopCfg.WgCfg.GainCalEn = bTRUE;
  HsLoopCfg.WgCfg.OffsetCalEn = bTRUE;
  if(AppIMPCfg.SweepCfg.SweepEn == bTRUE)
  {
    AppIMPCfg.FreqofData = AppIMPCfg.SweepCfg.SweepStart;
    AppIMPCfg.SweepCurrFreq = AppIMPCfg.SweepCfg.SweepStart;
    AD5940_SweepNext(&AppIMPCfg.SweepCfg, &AppIMPCfg.SweepNextFreq);
    sin_freq = AppIMPCfg.SweepCurrFreq;
  }
  else
  {
    sin_freq = AppIMPCfg.SinFreq;
    AppIMPCfg.FreqofData = sin_freq;
  }
  HsLoopCfg.WgCfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(sin_freq, AppIMPCfg.SysClkFreq);
  HsLoopCfg.WgCfg.SinCfg.SinAmplitudeWord = (uint32_t)(AppIMPCfg.DacVoltPP/800.0f*2047 + 0.5f);
  HsLoopCfg.WgCfg.SinCfg.SinOffsetWord = 0;
  HsLoopCfg.WgCfg.SinCfg.SinPhaseWord = 0;
  AD5940_HSLoopCfgS(&HsLoopCfg);
  if(AppIMPCfg.BiasVolt != 0.0f)    /* With bias voltage */
  {
    LPDACCfg_Type lpdac_cfg;
    
    lpdac_cfg.LpdacSel = LPDAC0;
    lpdac_cfg.LpDacVbiasMux = LPDACVBIAS_12BIT; /* Use Vbias to tuning BiasVolt. */
    lpdac_cfg.LpDacVzeroMux = LPDACVZERO_6BIT;  /* Vbias-Vzero = BiasVolt */
    lpdac_cfg.DacData6Bit = 0x40>>1;            /* Set Vzero to middle scale. */
    if(AppIMPCfg.BiasVolt<-1100.0f) AppIMPCfg.BiasVolt = -1100.0f + DAC12BITVOLT_1LSB;
    if(AppIMPCfg.BiasVolt> 1100.0f) AppIMPCfg.BiasVolt = 1100.0f - DAC12BITVOLT_1LSB;
    lpdac_cfg.DacData12Bit = (uint32_t)((AppIMPCfg.BiasVolt + 1100.0f)/DAC12BITVOLT_1LSB);
    lpdac_cfg.DataRst = bFALSE;      /* Do not reset data register */
    lpdac_cfg.LpDacSW = LPDACSW_VBIAS2LPPA|LPDACSW_VBIAS2PIN|LPDACSW_VZERO2LPTIA|LPDACSW_VZERO2PIN|LPDACSW_VZERO2HSTIA;
    lpdac_cfg.LpDacRef = LPDACREF_2P5;
    lpdac_cfg.LpDacSrc = LPDACSRC_MMR;      /* Use MMR data, we use LPDAC to generate bias voltage for LPTIA - the Vzero */
    lpdac_cfg.PowerEn = bTRUE;              /* Power up LPDAC */
    AD5940_LPDACCfgS(&lpdac_cfg);
  }
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_HSTIA_N;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_HSTIA_P;
  dsp_cfg.ADCBaseCfg.ADCPga = AppIMPCfg.AdcPgaGain;
  
  memset(&dsp_cfg.ADCDigCompCfg, 0, sizeof(dsp_cfg.ADCDigCompCfg));
  
  dsp_cfg.ADCFilterCfg.ADCAvgNum = AppIMPCfg.ADCAvgNum;
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;	/* Tell filter block clock rate of ADC*/
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = AppIMPCfg.ADCSinc2Osr;
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = AppIMPCfg.ADCSinc3Osr;
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE;
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE;
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bTRUE;
  dsp_cfg.DftCfg.DftNum = AppIMPCfg.DftNum;
  dsp_cfg.DftCfg.DftSrc = AppIMPCfg.DftSrc;
  dsp_cfg.DftCfg.HanWinEn = AppIMPCfg.HanWinEn;
  
  memset(&dsp_cfg.StatCfg, 0, sizeof(dsp_cfg.StatCfg));
  AD5940_DSPCfgS(&dsp_cfg);
    
  /* Enable all of them. They are automatically turned off during hibernate mode to save power */
  if(AppIMPCfg.BiasVolt == 0.0f)
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bTRUE);
  else
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH|AFECTRL_DCBUFPWR, bTRUE);
    /* Sequence end. */
  AD5940_SEQGenInsert(SEQ_STOP()); /* Add one extra command to disable sequencer for initialization sequence because we only want it to run one time. */

  /* Stop here */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  AD5940_SEQGenCtrl(bFALSE); /* Stop sequencer generator */
  if(error == AD5940ERR_OK)
  {
    AppIMPCfg.InitSeqInfo.SeqId = SEQID_1;
    AppIMPCfg.InitSeqInfo.SeqRamAddr = AppIMPCfg.SeqStartAddr;
    AppIMPCfg.InitSeqInfo.pSeqCmd = pSeqCmd;
    AppIMPCfg.InitSeqInfo.SeqLen = SeqLen;
    /* Write command to SRAM */
    AD5940_SEQCmdWrite(AppIMPCfg.InitSeqInfo.SeqRamAddr, pSeqCmd, SeqLen);
  }
  else
    return error; /* Error */
  return AD5940ERR_OK;
}


static AD5940Err AppIMPSeqMeasureGen(void)
{
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  
  uint32_t WaitClks;
  SWMatrixCfg_Type sw_cfg;
  ClksCalInfo_Type clks_cal;

  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = AppIMPCfg.DftSrc;
  clks_cal.DataCount = 1L<<(AppIMPCfg.DftNum+2); /* 2^(DFTNUMBER+2) */
  clks_cal.ADCSinc2Osr = AppIMPCfg.ADCSinc2Osr;
  clks_cal.ADCSinc3Osr = AppIMPCfg.ADCSinc3Osr;
  clks_cal.ADCAvgNum = AppIMPCfg.ADCAvgNum;
  clks_cal.RatioSys2AdcClk = AppIMPCfg.SysClkFreq/AppIMPCfg.AdcClkFreq;
  AD5940_ClksCalculate(&clks_cal, &WaitClks);

  AD5940_SEQGenCtrl(bTRUE);
  AD5940_SEQGpioCtrlS(AGPIO_Pin2); /* Set GPIO1, clear others that under control */
  AD5940_SEQGenInsert(SEQ_WAIT(16*250));  /* @todo wait 250us? */
  sw_cfg.Dswitch = SWD_RCAL0;
  sw_cfg.Pswitch = SWP_RCAL0;
  sw_cfg.Nswitch = SWN_RCAL1;
  sw_cfg.Tswitch = SWT_RCAL1|SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
	AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bTRUE);
  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);  /* Enable Waveform generator */
  //delay for signal settling DFT_WAIT
  AD5940_SEQGenInsert(SEQ_WAIT(16*10));
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);  /* Start ADC convert and DFT */
  AD5940_SEQGenInsert(SEQ_WAIT(WaitClks));
  //wait for first data ready
  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG, bFALSE);  /* Stop ADC convert and DFT */

  /* Configure matrix for external Rz */
  sw_cfg.Dswitch = AppIMPCfg.DswitchSel;
  sw_cfg.Pswitch = AppIMPCfg.PswitchSel;
  sw_cfg.Nswitch = AppIMPCfg.NswitchSel;
  sw_cfg.Tswitch = SWT_TRTIA|AppIMPCfg.TswitchSel;
  AD5940_SWMatrixCfgS(&sw_cfg);
  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_WG, bTRUE);  /* Enable Waveform generator */
  AD5940_SEQGenInsert(SEQ_WAIT(16*10));  //delay for signal settling DFT_WAIT
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);  /* Start ADC convert and DFT */
  AD5940_SEQGenInsert(SEQ_WAIT(WaitClks));  /* wait for first data ready */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG|AFECTRL_ADCPWR, bFALSE);  /* Stop ADC convert and DFT */
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bFALSE);
  AD5940_SEQGpioCtrlS(0); /* Clr GPIO1 */

  AD5940_EnterSleepS();/* Goto hibernate */

  /* Sequence end. */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  AD5940_SEQGenCtrl(bFALSE); /* Stop sequencer generator */

  if(error == AD5940ERR_OK)
  {
    AppIMPCfg.MeasureSeqInfo.SeqId = SEQID_0;
    AppIMPCfg.MeasureSeqInfo.SeqRamAddr = AppIMPCfg.InitSeqInfo.SeqRamAddr + AppIMPCfg.InitSeqInfo.SeqLen ;
    AppIMPCfg.MeasureSeqInfo.pSeqCmd = pSeqCmd;
    AppIMPCfg.MeasureSeqInfo.SeqLen = SeqLen;
    /* Write command to SRAM */
    AD5940_SEQCmdWrite(AppIMPCfg.MeasureSeqInfo.SeqRamAddr, pSeqCmd, SeqLen);
  }
  else
    return error; /* Error */
  return AD5940ERR_OK;
}


/* This function provide application initialize. It can also enable Wupt that will automatically trigger sequence. Or it can configure  */
int32_t AppIMPInit(uint32_t *pBuffer, uint32_t BufferSize)
{
  AD5940Err error = AD5940ERR_OK;  
  SEQCfg_Type seq_cfg;
  FIFOCfg_Type fifo_cfg;

  printf("AppIMPInit: Debut\n");

  if(AD5940_WakeUp(10) > 10) {
    printf("AppIMPInit: WakeUp FAILED\n");
    return AD5940ERR_WAKEUP;
  }

  // Configure sequencer and stop it
  seq_cfg.SeqMemSize = SEQMEMSIZE_2KB;  // 2kB SRAM for sequencer, rest for FIFO
  seq_cfg.SeqBreakEn = bFALSE;
  seq_cfg.SeqIgnoreEn = bTRUE;
  seq_cfg.SeqCntCRCClr = bTRUE;
  seq_cfg.SeqEnable = bFALSE;
  seq_cfg.SeqWrTimer = 0;
  AD5940_SEQCfg(&seq_cfg);
  printf("AppIMPInit: SEQCfg OK\n");
  
  // Reconfigure FIFO
  AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE); // Disable FIFO first
  fifo_cfg.FIFOEn = bTRUE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;      // 4kB for FIFO, rest for sequencer
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  fifo_cfg.FIFOThresh = AppIMPCfg.FifoThresh;
  AD5940_FIFOCfg(&fifo_cfg);
  printf("AppIMPInit: FIFO OK\n");
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

  // Start sequence generator
  // Initialize sequencer generator
  if((AppIMPCfg.IMPInited == bFALSE) || (AppIMPCfg.bParaChanged == bTRUE))
  {
    printf("AppIMPInit: Bloc initialisation sequence\n");

    if(pBuffer == 0) {
      printf("AppIMPInit: ERREUR pBuffer == 0\n");
      return AD5940ERR_PARA;
    }
    if(BufferSize == 0) {
      printf("AppIMPInit: ERREUR BufferSize == 0\n");
      return AD5940ERR_PARA;
    }

    AD5940_SEQGenInit(pBuffer, BufferSize);
    printf("AppIMPInit: SEQGenInit OK\n");

    printf("AppIMPInit: Avant AppIMPSeqCfgGen()\n");
    error = AppIMPSeqCfgGen();
    printf("AppIMPInit: AppIMPSeqCfgGen() return = %ld\n", error);
    if(error != AD5940ERR_OK) return error;

    printf("AppIMPInit: Avant AppIMPSeqMeasureGen()\n");
    error = AppIMPSeqMeasureGen();
    printf("AppIMPInit: AppIMPSeqMeasureGen() return = %ld\n", error);
    if(error != AD5940ERR_OK) return error;

    AppIMPCfg.bParaChanged = bFALSE;
    printf("AppIMPInit: bParaChanged clear\n");
  }

  // Initialization sequencer
  AppIMPCfg.InitSeqInfo.WriteSRAM = bFALSE;
  AD5940_SEQInfoCfg(&AppIMPCfg.InitSeqInfo);
  seq_cfg.SeqEnable = bTRUE;
  AD5940_SEQCfg(&seq_cfg);  // Enable sequencer
  AD5940_SEQMmrTrig(AppIMPCfg.InitSeqInfo.SeqId);
  printf("AppIMPInit: SEQ MmrTrig\n");
  while(AD5940_INTCTestFlag(AFEINTC_1, AFEINTSRC_ENDSEQ) == bFALSE);

  // Measurement sequence
  AppIMPCfg.MeasureSeqInfo.WriteSRAM = bFALSE;
  AD5940_SEQInfoCfg(&AppIMPCfg.MeasureSeqInfo);

  seq_cfg.SeqEnable = bTRUE;
  AD5940_SEQCfg(&seq_cfg);  // Enable sequencer, and wait for trigger
  AD5940_ClrMCUIntFlag();   // Clear interrupt flag generated before

  AD5940_AFEPwrBW(AppIMPCfg.PwrMod, AFEBW_250KHZ);

  AppIMPCfg.IMPInited = bTRUE;  // IMP application has been initialized.
  printf("AppIMPInit: FIN OK\n");
  return AD5940ERR_OK;
}

/* Depending on the data type, do appropriate data pre-process before return back to controller */
int32_t AppIMPDataProcess(int32_t * const pData, uint32_t *pDataCount)
{
  uint32_t DataCount = *pDataCount;
  uint32_t ImpResCount = DataCount/4;

  fImpPol_Type * const pOut = (fImpPol_Type*)pData;
  iImpCar_Type * pSrcData = (iImpCar_Type*)pData;

  *pDataCount = 0;

  DataCount = (DataCount/4)*4;/* We expect RCAL data together with Rz data. One DFT result has two data in FIFO, real part and imaginary part.  */

  /* Convert DFT result to int32_t type */
  for(uint32_t i=0; i<DataCount; i++)
  {
    pData[i] &= 0x3ffff; /* @todo option to check ECC */
    if(pData[i]&(1L<<17)) /* Bit17 is sign bit */
    {
      pData[i] |= 0xfffc0000; /* Data is 18bit in two's complement, bit17 is the sign bit */
    }
  }
  for(uint32_t i=0; i<ImpResCount; i++)
  {
    iImpCar_Type *pDftRcal, *pDftRz;

    pDftRcal = pSrcData++;
    pDftRz = pSrcData++;
    float RzMag,RzPhase;
    float RcalMag, RcalPhase;
    
    RcalMag = sqrt((float)pDftRcal->Real*pDftRcal->Real+(float)pDftRcal->Image*pDftRcal->Image);
    RcalPhase = atan2(-pDftRcal->Image,pDftRcal->Real);
    RzMag = sqrt((float)pDftRz->Real*pDftRz->Real+(float)pDftRz->Image*pDftRz->Image);
    RzPhase = atan2(-pDftRz->Image,pDftRz->Real);

    RzMag = RcalMag/RzMag*AppIMPCfg.RcalVal;
    RzPhase = RcalPhase - RzPhase;
    printf("V:%d,%d,I:%d,%d ",pDftRcal->Real,pDftRcal->Image, pDftRz->Real, pDftRz->Image);
    printf("RCAL: Re=%ld Im=%ld | DUT: Re=%ld Im=%ld\n", 
  pDftRcal->Real, pDftRcal->Image, 
  pDftRz->Real, pDftRz->Image);
    pOut[i].Magnitude = RzMag;
    pOut[i].Phase = RzPhase;
  }
  *pDataCount = ImpResCount; 
  AppIMPCfg.FreqofData = AppIMPCfg.SweepCurrFreq;
  /* Calculate next frequency point */
  if(AppIMPCfg.SweepCfg.SweepEn == bTRUE)
  {
    AppIMPCfg.FreqofData = AppIMPCfg.SweepCurrFreq;
    AppIMPCfg.SweepCurrFreq = AppIMPCfg.SweepNextFreq;
    AD5940_SweepNext(&AppIMPCfg.SweepCfg, &AppIMPCfg.SweepNextFreq);
    
  }

  return 0;
}

/**

*/


int32_t AppIMPISR(void *pBuff, uint32_t *pCount)
{
  uint32_t BuffCount;
  uint32_t FifoCnt;
  BuffCount = *pCount;
  
  *pCount = 0;
  
  if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
    return AD5940ERR_WAKEUP;  /* Wakeup Failed */
  AD5940_SleepKeyCtrlS(SLPKEY_LOCK);  /* Prohibit AFE to enter sleep mode. */

  if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH) == bTRUE)
  {
    /* Now there should be 4 data in FIFO */
    FifoCnt = (AD5940_FIFOGetCnt()/4)*4;
    
    if(FifoCnt > BuffCount)
    {
      ///@todo buffer is limited.
    }
    AD5940_FIFORd((uint32_t *)pBuff, FifoCnt);
    AD5940_INTCClrFlag(AFEINTSRC_DATAFIFOTHRESH);
    //AD5940_EnterSleepS(); /* Manually put AFE back to hibernate mode. This operation only takes effect when register value is ACTIVE previously */
    AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);  /* Allow AFE to enter sleep mode. */
    /* Process data */ 
    AppIMPDataProcess((int32_t*)pBuff,&FifoCnt); 
    *pCount = FifoCnt;
    return 0;
  }
  
  return 0;
} 