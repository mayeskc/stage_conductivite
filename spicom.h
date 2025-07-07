#ifndef SPICOM_H
#define SPICOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AD5940_ReadWriteNBytes(unsigned char *pSendBuffer, unsigned char *pRecvBuff, unsigned long length);

void AD5940_CsClr(void);
void AD5940_CsSet(void);

void AD5940_RstSet(void);
void AD5940_RstClr(void);

void AD5940_Delay10us(uint32_t time);

uint32_t AD5940_GetMCUIntFlag(void);
uint32_t AD5940_ClrMCUIntFlag(void);

uint32_t AD5940_MCUResourceInit(void *pCfg);

void Ext_Int0_Handler(void);

#ifdef __cplusplus
}
#endif

#endif // SPICOM_H