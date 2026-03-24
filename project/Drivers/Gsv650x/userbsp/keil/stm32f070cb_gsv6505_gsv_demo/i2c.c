/* Includes ------------------------------------------------------------------*/
#define MANI2CFREQ     8
#define GPIO_NUMBER           (16U)

#if defined(STM32F070xB)
  #define SET_SCL0_1     I2C0_SCL_GPIO_Port->BSRR = I2C0_SCL_Pin // Pin I2C0_SCL
  #define SET_SDA0_1     I2C0_SDA_GPIO_Port->BSRR = I2C0_SDA_Pin // Pin I2C0_SDA

  #define SET_SCL0_0     I2C0_SCL_GPIO_Port->BRR  = I2C0_SCL_Pin // Pin I2C0_SCL
  #define SET_SDA0_0     I2C0_SDA_GPIO_Port->BRR  = I2C0_SDA_Pin // Pin I2C0_SDA

  #define GET_SDA0       ((I2C0_SDA_GPIO_Port->IDR & I2C0_SDA_Pin) != 0) 

  #define SCL0_IN        I2C0_SCL_INPUT_PULLUP
  #define SCL0_OUT       I2C0_SCL_OUTPUT_PUSHPULL
  #define SDA0_IN        I2C0_SDA_INPUT_PULLUP
  #define SDA0_OUT       I2C0_SDA_OUTPUT_PUSHPULL

  #define SET_SCL1_1     I2C1_SCL_GPIO_Port->BSRR = I2C1_SCL_Pin // Pin I2C1_SCL
  #define SET_SDA1_1     I2C1_SDA_GPIO_Port->BSRR = I2C1_SDA_Pin // Pin I2C1_SDA

  #define SET_SCL1_0     I2C1_SCL_GPIO_Port->BRR  = I2C1_SCL_Pin // Pin I2C1_SCL
  #define SET_SDA1_0     I2C1_SDA_GPIO_Port->BRR  = I2C1_SDA_Pin // Pin I2C1_SDA

  #define GET_SDA1       ((I2C1_SDA_GPIO_Port->IDR & I2C1_SDA_Pin) != 0) 

  #define SCL1_IN        I2C1_SCL_INPUT_PULLUP
  #define SCL1_OUT       I2C1_SCL_OUTPUT_PUSHPULL
  #define SDA1_IN        I2C1_SDA_INPUT_PULLUP
  #define SDA1_OUT       I2C1_SDA_OUTPUT_PUSHPULL
#endif
#if defined(STM32H723xx)
#define SET_SCL0_1     I2C0_SCL_GPIO_Port->BSRR = I2C0_SCL_Pin // Pin I2C0_SCL
#define SET_SDA0_1     I2C0_SDA_GPIO_Port->BSRR = I2C0_SDA_Pin // Pin I2C0_SDA

#define SET_SCL0_0     I2C0_SCL_GPIO_Port->BSRR  = (uint32)I2C0_SCL_Pin<<GPIO_NUMBER // Pin I2C0_SCL
#define SET_SDA0_0     I2C0_SDA_GPIO_Port->BSRR  = (uint32)I2C0_SDA_Pin<<GPIO_NUMBER // Pin I2C0_SDA

#define GET_SDA0       ((I2C0_SDA_GPIO_Port->IDR & I2C0_SDA_Pin) != 0) 

#define SCL0_IN        I2C0_SCL_INPUT_PULLUP
#define SCL0_OUT       I2C0_SCL_OUTPUT_PUSHPULL
#define SDA0_IN        I2C0_SDA_INPUT_PULLUP
#define SDA0_OUT       I2C0_SDA_OUTPUT_PUSHPULL

#define SET_SCL1_1     I2C1_SCL_GPIO_Port->BSRR = I2C1_SCL_Pin // Pin I2C1_SCL
#define SET_SDA1_1     I2C1_SDA_GPIO_Port->BSRR = I2C1_SDA_Pin // Pin I2C1_SDA

#define SET_SCL1_0     I2C1_SCL_GPIO_Port->BSRR  = (uint32)I2C1_SCL_Pin<<GPIO_NUMBER // Pin I2C1_SCL
#define SET_SDA1_0     I2C1_SDA_GPIO_Port->BSRR  = (uint32)I2C1_SDA_Pin<<GPIO_NUMBER // Pin I2C1_SDA

#define GET_SDA1       ((I2C1_SDA_GPIO_Port->IDR & I2C1_SDA_Pin) != 0) 

#define SCL1_IN        I2C1_SCL_INPUT_PULLUP
#define SCL1_OUT       I2C1_SCL_OUTPUT_PUSHPULL
#define SDA1_IN        I2C1_SDA_INPUT_PULLUP
#define SDA1_OUT       I2C1_SDA_OUTPUT_PUSHPULL
#endif /* STM32H723xx */
void I2CDelay(uint16 delay)
{
    uint16 j = 0;
    for(j=0;j<delay;j++);
    return;
}

/*
PB0-1: index 0 I2C 0:SDA, 1:SCL
PB2-3: index 1 I2C 2:SDA, 3:SCL
PB4-5: index 2 I2C 4:SDA, 5:SCL
*/

void ManReleaseScl(uint8 index)
{
    switch(index)
    {
    case 0:
        SCL0_IN;
        break;
    case 1:
        SCL1_IN;
        break;
//      case 2:
//        SCL2_IN;
//        break;
    }

}

void ManDriveScl(uint8 index)
{
    switch(index)
    {
    case 0:
        SCL0_OUT;
        break;
    case 1:
        SCL1_OUT;
        break;
//      case 2:
//        SCL2_OUT;
//        break;
    }

}

void ManReleaseSda(uint8 index)
{
    switch(index)
    {
    case 0:
        SDA0_IN;
        break;
    case 1:
        SDA1_IN;
        break;
//      case 2:
//        SDA2_IN;
//        break;
    }

}


void ManDriveSda(uint8 index)
{
    switch(index)
    {
    case 0:
        SDA0_OUT;
        break;
    case 1:
        SDA1_OUT;
        break;
//      case 2:
//        SDA2_OUT;
//        break;
    }

}

void ManSclHigh(uint8 index)
{
    switch(index)
    {
    case 0:
        SET_SCL0_1;
        break;
    case 1:
        SET_SCL1_1;
        break;
//      case 2:
//        SET_SCL2_1;
//        break;
    }
}

void ManSclLow(uint8 index)
{
    switch(index)
    {
    case 0:
        SET_SCL0_0;
        break;
    case 1:
        SET_SCL1_0;
        break;
//      case 2:
//        SET_SCL2_0;
//        break;
    }
}

void ManSdaHigh(uint8 index)
{
    switch(index)
    {
    case 0:
        SET_SDA0_1;
        break;
    case 1:
        SET_SDA1_1;
        break;
//      case 2:
//        SET_SDA2_1;
//        break;
    }
}

void ManSdaLow(uint8 index)
{
    switch(index)
    {
    case 0:
        SET_SDA0_0;
        break;
    case 1:
        SET_SDA1_0;
        break;
//      case 2:
//        SET_SDA2_0;
//        break;
    }
}

uint8 GetSdaValue(uint8 index)
{
    uint8 value = 0;
    switch(index)
    {
    case 0:
        value = GET_SDA0;
        break;
    case 1:
        value = GET_SDA1;
        break;
//      case 2:
//        value = GET_SDA2;
//        break;
    }
    return value;
}

uint8 ManGetAck(uint8 index)
{
    uint8 value = 0;

    I2CDelay(MANI2CFREQ);
    ManSclLow(index);
    I2CDelay(MANI2CFREQ);
    ManReleaseSda(index);
    I2CDelay(MANI2CFREQ);
    ManSclHigh(index);
    I2CDelay(MANI2CFREQ);
    value = GetSdaValue(index);
    I2CDelay(MANI2CFREQ);
    ManSclLow(index);
    if(value == 0)
        ManSdaLow(index);
    else
        ManSdaHigh(index);
    I2CDelay(MANI2CFREQ);
    ManDriveSda(index);
    I2CDelay(MANI2CFREQ);
    return value;
}

void ManSetAck(uint8 index, uint8 i2cdata)
{
    I2CDelay(MANI2CFREQ);
    ManSclLow(index);
    I2CDelay(MANI2CFREQ);
    if(i2cdata != 0)
        ManSdaHigh(index);
    else
        ManSdaLow(index);
    I2CDelay(MANI2CFREQ);
    ManSclHigh(index);
    I2CDelay(2*MANI2CFREQ);
    ManSclLow(index);
    I2CDelay(MANI2CFREQ);
    ManSdaLow(index);
    I2CDelay(MANI2CFREQ);
}

void ManI2cStart(uint8 index)
{
    ManDriveScl(index);
    ManDriveSda(index);
    I2CDelay(MANI2CFREQ);
    ManSclHigh(index);
    I2CDelay(MANI2CFREQ);
    ManSdaHigh(index);
    I2CDelay(MANI2CFREQ);
    ManSdaLow(index);
    I2CDelay(MANI2CFREQ);
    ManSclLow(index);
    I2CDelay(MANI2CFREQ);

}

void ManI2cStop(uint8 index)
{
    I2CDelay(MANI2CFREQ);
    ManSclHigh(index);
    I2CDelay(MANI2CFREQ);
    ManSdaHigh(index);
    I2CDelay(MANI2CFREQ);
    ManReleaseScl(index);
    ManReleaseSda(index);

}

void ManI2cRestart(uint8 index)
{
    I2CDelay(MANI2CFREQ);
    ManSdaHigh(index);
    I2CDelay(3*MANI2CFREQ);
    ManSclHigh(index);
    I2CDelay(3*MANI2CFREQ);
    ManSdaLow(index);
    I2CDelay(3*MANI2CFREQ);
    ManSclLow(index);
    I2CDelay(MANI2CFREQ);

}

uint8 ManI2cSendByte(uint8 index, uint8 i2cdata)
{
    uint8  i;
    uint32 value;
    uint8  ack;

    /* Send byte */
    for(i=0;i<8;i++)
    {
        I2CDelay(MANI2CFREQ);
        ManSclLow(index);
        I2CDelay(MANI2CFREQ);
        value = (i2cdata>>(7-i))&0x01;
        if(value == 0)
            ManSdaLow(index);
        else
            ManSdaHigh(index);
        I2CDelay(MANI2CFREQ);
        ManSclHigh(index);
        I2CDelay(MANI2CFREQ);
    }
    I2CDelay(MANI2CFREQ);
    ManSclLow(index);
    I2CDelay(3*MANI2CFREQ);
    /* Release Sda to get ACK */
    ManReleaseSda(index);
    ack = ManGetAck(index);
    ManDriveSda(index);
    return ack;
}

uint8 ManI2cReadByte(uint8 index, uint8 FinalByte)
{
    uint8  i;
    uint32 value = 0;
    uint8  i2cdata = 0;

    ManReleaseSda(index);
    for(i=0;i<8;i++)
    {
        ManSclLow(index);
        I2CDelay(2*MANI2CFREQ);
        ManSclHigh(index);
        I2CDelay(MANI2CFREQ);
        value = GetSdaValue(index);
        if(value != 0)
            i2cdata = (i2cdata<<1)+1;
        else
            i2cdata = i2cdata<<1;
        I2CDelay(MANI2CFREQ);
    }
    ManSclLow(index);
    I2CDelay(MANI2CFREQ);
    ManDriveSda(index);
    I2CDelay(MANI2CFREQ);

    if(FinalByte == 0x0)
        ManSetAck(index,0);
    else
        ManSetAck(index,1);
    return i2cdata;
}

AvRet ManI2cWrite(uint32 devAddress, uint32 regAddress, uint8 *i2cdata, uint16 count, uint8 index, uint8 Flag16bit)
{
    uint8  value;
    uint8  ack = 1;
    uint16 i;
    ManI2cStart(index);
    value = devAddress & 0xfe;
    ack = ManI2cSendByte(index, value);
    if(Flag16bit == 1)
    {
        value = (regAddress>>8) & 0xff;
        ManI2cSendByte(index, value);
    }
    value = regAddress & 0xff;
    ManI2cSendByte(index, value);
    for(i=0;i<count;i++)
    {
        if(i==count-1)
            ManI2cSendByte(index, i2cdata[i]);
        else
            ManI2cSendByte(index, i2cdata[i]);
    }
    ManI2cStop(index);

    I2CDelay(0x3f);

    if(ack == 0)
        return AvOk;
#if AvEnableDebugMode
    else
    {
        uint32 delay = 0;
        while(1)
        {
            for(delay=0;delay<0xffff;delay++)
                HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_SET);
            for(delay=0;delay<0xffff;delay++)
                HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_RESET);
            if(HAL_UART_Receive(&huart1, &value, 1, 0) == HAL_OK)
                return AvError;
        }
    }
#else
    else
        return AvError;
#endif
}

AvRet ManI2cRead(uint32 devAddress, uint32 regAddress, uint8 *i2cdata, uint16 count, uint8 index, uint8 Flag16bit)
{
    uint8  value;
    uint8  ack = 1;
    uint16 i;
    ManI2cStart(index);
    value = devAddress & 0xfe;
    ack = ManI2cSendByte(index,value);
    if(Flag16bit == 1)
    {
        value = (regAddress>>8) & 0xff;
        ManI2cSendByte(index, value);
    }
    value = regAddress & 0xff;
    ManI2cSendByte(index,value);

    ManI2cRestart(index);

    value = (devAddress & 0xff) | 0x01;
    ManI2cSendByte(index,value);
    for(i=0;i<count;i++)
    {
        if(i==count-1)
            i2cdata[i] = ManI2cReadByte(index, 1);
        else
            i2cdata[i] = ManI2cReadByte(index, 0);
    }
    ManI2cStop(index);

    I2CDelay(0x3f);

    if(ack == 0)
        return AvOk;
#if AvEnableDebugMode
    else
    {
        uint32 delay = 0;
        while(1)
        {
            for(delay=0;delay<0xffff;delay++)
                HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_SET);
            for(delay=0;delay<0xffff;delay++)
                HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_RESET);
            if(HAL_UART_Receive(&huart1, &value, 1, 0) == HAL_OK)
                return AvError;
        }
    }
#else
    else
        return AvError;
#endif
}
