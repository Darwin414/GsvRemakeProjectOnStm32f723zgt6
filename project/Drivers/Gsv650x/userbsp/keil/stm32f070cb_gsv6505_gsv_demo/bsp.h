/**
 * @file bsp.h
 *
 * @brief sample bsp support
 */

#ifndef __bsp_h
#define __bsp_h

#include "av_config.h"
#include "stm32h7xx.h"
#include "main.h"
/* exported functions */
#ifdef __cplusplus
extern "C" {
#endif

AvRet BspInit(void);
AvRet BspI2cRead(uint32 devAddress, uint32 regAddress, uint8 *data, uint16 count);
AvRet BspI2cWrite(uint32 devAddress, uint32 regAddress, uint8 *data, uint16 count);
AvRet BspUartSendByte(uint8 *data, uint16 size);
AvRet BspUartGetByte(uint8 *data);
AvRet BspGetMilliSecond(uint32 *ms);
AvRet BspGetKey(uint8 *data);
AvRet BspIrdaGetByte(uint8 *data);
uint32 BspGetTimerTicks(void);
uint32 BspGetElapsedTimerTicks(uint32 start_time);

#ifdef __cplusplus
}

#endif
#endif
