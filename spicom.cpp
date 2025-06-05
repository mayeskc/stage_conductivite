#include "ad5940.h"
#include <SPI.h>

#define SPI_CS_AD5940_Pin A2
#define AD5940_ResetPin A3
//*******************************************************
//declarations/definitions
volatile static uint32_t ucInterrupted = 0;       /* Flag to indicate interrupt occurred */
void Ext_Int0_Handler(void);

/**
	@brief Using SPI to transmit N bytes and return the received bytes. This function targets to
         provide a more efficient way to transmit/receive data.
	@param pSendBuffer :{0 - 0xFFFFFFFF}
      - Pointer to the data to be sent.
	@param pRecvBuff :{0 - 0xFFFFFFFF}
      - Pointer to the buffer used to store received data.
	@param length :{0 - 0xFFFFFFFF}
      - Data length in SendBuffer.
	@return None.
**/
void AD5940_ReadWriteNBytes(unsigned char *pSendBuffer, unsigned char *pRecvBuff, unsigned long length)
{
  //set SPI settings for the following transaction
  //speedMaximum: 12MHz found to be max for Adafruit Feather M0, AD5940 rated for max 16MHz clock frequency
  //dataOrder: MSB first
  //dataMode: SCLK idles low/ data clocked on SCLK falling edge --> mode 0
  SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0));

  for (int i = 0; i < length; i++)
  {
    *pRecvBuff++ = SPI.transfer(*pSendBuffer++);  //do a transfer
  }

  SPI.endTransaction(); //transaction over
}

void AD5940_CsClr(void)
{
  digitalWrite(SPI_CS_AD5940_Pin, LOW);
}

void AD5940_CsSet(void)
{
  digitalWrite(SPI_CS_AD5940_Pin, HIGH);
}

void AD5940_RstSet(void)
{
  digitalWrite(AD5940_ResetPin, HIGH);
}

void AD5940_RstClr(void)
{
  digitalWrite(AD5940_ResetPin, LOW);
}

void AD5940_Delay10us(uint32_t time)
{
  //Warning: micros() only has 4us (for 16MHz boards) or 8us (for 8MHz boards) resolution - use a timer instead?
  unsigned long time_last = micros();
  while (micros() - time_last < time * 10) // subtraction handles the roll over of micros()
  {
    //wait
  }
}

//declare the following function in the ad5940.h file if you use it in other .c files (that include ad5940.h):
//unsigned long AD5940_GetMicros(void);
//used for time tests:
// unsigned long AD5940_GetMicros()
// {
//   return micros();
// }

uint32_t AD5940_GetMCUIntFlag(void)
{
  return ucInterrupted;
}

uint32_t AD5940_ClrMCUIntFlag(void)
{
  ucInterrupted = 0;
  return 1;
}

/* Functions that used to initialize MCU platform */

uint32_t AD5940_MCUResourceInit(void *pCfg)
{
  /* Step1, initialize SPI peripheral and its GPIOs for CS/RST */
  //start the SPI library (setup SCK, MOSI, and MISO pins)
  SPI.begin();
  //initalize SPI chip select pin
  pinMode(SPI_CS_AD5940_Pin, OUTPUT);
  //initalize Reset pin
  pinMode(AD5940_ResetPin, OUTPUT);


  //chip select high to de-select AD5940 initially
  AD5940_CsSet();
  AD5940_RstSet();
  return 0;
}

/* MCU related external line interrupt service routine */
//The interrupt handler handles the interrupt to the MCU
//when the AD5940 INTC pin generates an interrupt to alert the MCU that data is ready
void Ext_Int0_Handler()
{
  ucInterrupted = 1;
  /* This example just set the flag and deal with interrupt in AD5940Main function. It's your choice to choose how to process interrupt. */
}