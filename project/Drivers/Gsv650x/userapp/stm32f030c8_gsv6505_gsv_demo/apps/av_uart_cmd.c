#include "av_config.h"
#include "hal.h"
#include "kapi.h"
#include "av_event_handler.h"
#include "av_uart_cmd.h"
#include "av_key_cmd.h"

#define CommandNameIs(data) (!AvStrcmp((const char*) cCommand, data))
#define UartCommandLength 8

#if AvEnableUartInput /* Enable UART */
void ProcessChar(AvPort *port);
void DoUartCommand(AvPort *port);
void ReturnIdle(void);

typedef enum
{
  P_IDLE=0,
  P_CMD,
  P_ARG1,
  P_ARG2,
  P_ARG3
}ParseState;

extern uint8  AvEdidCeaParamForce;
extern uint16 AvEdidSupportFreq;
#if AvEnableCecFeature /* CEC Related */
extern uchar  AudioStatus;
extern uchar  DevicePowerStatus;
extern uint8  ARCFeatureSupport;
#endif
static ParseState eParser = P_IDLE;
static uint8 cCommand[UartCommandLength];
static uint8 uCmdIndex;
static uint8 cArg1[4];
static uint8 uArg1Index;
static uint8 cArg2[4];
static uint8 uArg2Index;
static uint8 cArg3[4];
static uint8 uArg3Index;
static uint32 uArg1;
static uint32 uArg2;
static uint32 uArg3;

static uint8 RegTx1From = 2;
static uint8 RegPause   = 0;
static AvFrlType LoopMode = AV_FRL_NONE;
static uint8 LoopCount  = 0;
static uint32 UartDelay = 0;

#endif /* Enable UART */

void ProcessChar(AvPort *port)
{
#if AvEnableUartInput
    uint8 uChar = 0x00;
    uint8 uReturn = 0x0a;
    /* Get Uart Data, if no data, return */
    if(AvHalUartGetByte(&uChar) == AvError)
    {
        if(eParser != P_IDLE)
        {
            UartDelay = UartDelay + 1;
            if((UartDelay > 5000) && (RegPause == 0))
                eParser = P_IDLE;
        }
        return;
    }
    UartDelay = 0;

    AvHalUartSendByte(&uChar, 1);
    if(uChar == 0x0d)
        AvHalUartSendByte(&uReturn, 1);

    switch (eParser)
    {
        case P_IDLE:
            //AvKapiOutputDebugMessage("\n\rKeyIn List:\n\r's'-Current Connection");
            //AvKapiOutputDebugMessage("'c * *'-Connect Rx* to Tx*, e.g. 'c 4 1'");
            uChar = IsLetter(uChar);
            if (uChar)
            {
                cCommand[uCmdIndex] = uChar;
                uCmdIndex++;
                eParser = P_CMD;
            }
            break;

        case P_CMD:
            if (IsCR(uChar)) //receive end char 0x0D
            {
                cCommand[uCmdIndex] = 0;
                uCmdIndex++;
                DoUartCommand(port); //process data
                ReturnIdle();
            }
            else if (IsSpace(uChar)) //receive space key
            {
                cCommand[uCmdIndex] = 0;
                uCmdIndex++;
                eParser = P_ARG1;
            }
            else if (IsBackSpace(uChar))
            {
                if (uCmdIndex>0)
                    uCmdIndex--;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cCommand[uCmdIndex] = uChar;
                    uCmdIndex++;
                }
            }
            break;

        case P_ARG1:
            if (IsCR(uChar))
            {
                cArg1[uArg1Index] = 0;
                uArg1 = AsciiToNumber(cArg1, uArg1Index);
                DoUartCommand(port);
                ReturnIdle();
            }
            else if (IsSpace(uChar))
            {
                cArg1[uArg1Index] = 0;
                uArg1 = AsciiToNumber(cArg1, uArg1Index);
                eParser = P_ARG2;
            }
            else if (IsBackSpace(uChar))
            {
                if (uArg1Index>0) uArg1Index--;
                else eParser = P_CMD;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cArg1[uArg1Index] = uChar;
                    uArg1Index++;
                }
            }
            break;

        case P_ARG2:
            if (IsCR(uChar))
            {
                cArg2[uArg2Index] = 0;
                uArg2 = AsciiToNumber(cArg2, uArg2Index);
                DoUartCommand(port);
                ReturnIdle();
            }
            else if (IsSpace(uChar))
            {
                cArg2[uArg2Index] = 0;
                uArg2 = AsciiToNumber(cArg2, uArg2Index);
                eParser = P_ARG3;
            }
            else if (IsBackSpace(uChar))
            {
                if (uArg2Index>0) uArg2Index--;
                else eParser = P_ARG1;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cArg2[uArg2Index] = uChar;
                    uArg2Index++;
                }
            }
            break;

        case P_ARG3:
            if (IsCR(uChar) || IsSpace(uChar))
            {
                cArg3[uArg3Index] = 0;
                uArg3 = AsciiToNumber(cArg3, uArg3Index);
                DoUartCommand(port);
                ReturnIdle();
            }
            else if (IsBackSpace(uChar))
            {
                if (uArg3Index>0) uArg3Index--;
                else eParser = P_ARG2;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cArg3[uArg3Index] = uChar;
                    uArg3Index++;
                }
            }
            break;
    }
#endif
}

void ReturnIdle(void)
{
#if AvEnableUartInput
    uint8 i;
    eParser = P_IDLE;
    uCmdIndex = 0;
    uArg1Index = 0;
    uArg2Index = 0;
    uArg3Index = 0;
    uArg1 = 0;
    uArg2 = 0;
    uArg3 = 0;
    for(i=0;i<4;i++)
        cArg1[i]=0;
    for(i=0;i<4;i++)
        cArg2[i]=0;
    for(i=0;i<4;i++)
        cArg3[i]=0;

#endif
}

void DoUartCommand(AvPort *port)
{
    uint8 value = 0;
    uint32 i = 0;
    uint8  j = 0;
    uint8 num = 0;
    uint8 uReturn = 0x0a;

#if AvEnableUartInput
    if CommandNameIs("A") /* ARC */
    {
    #if AvEnableCecFeature
        if(uArg1 == 0)
        {
            if(AudioStatus == 0)
            {
                AudioStatus = 1;
                AvApiConnectPort(&port[2], &port[3], AvConnectAudio);
                AvKapiOutputDebugMessage("Software try to enable ARC to TV");
            }
            else
            {
                AudioStatus = 0;
                AvKapiOutputDebugMessage("Software try to disable ARC to TV");
            }
            /* Set ModeToTv first for CEC CTS */
            AvHandleEvent(&port[2], AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
            (&port[2])->content.cec->AmplifierDelayExpire = 200;
        }
        else if(uArg1 == 2)
        {
            AudioStatus = 1;
            AvHandleEvent(&port[2], AvEventCecSendInitiateARC, NULL, NULL);
        }
        else if(uArg1 == 3)
        {
            AudioStatus = 0;
            AvHandleEvent(&port[2], AvEventCecSendTerminateARC, NULL, NULL);
        }
        else if(uArg1 == 4)
        {
            ARCFeatureSupport = 0;
        }
        else if(uArg1 == 5)
        {
            ARCFeatureSupport = 1;
        }
        else
        {
            if(AudioStatus == 0)
            {
                AudioStatus = 1;
                if((&port[2])->content.cec->EnableAudioAmplifier != AV_CEC_AMP_ENABLED)
                    (&port[2])->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                if((&port[2])->content.cec->EnableARC != AV_CEC_ARC_INITIATED)
                    (&port[2])->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
                AvApiConnectPort(&port[2], &port[3], AvConnectAudio);
                AvKapiOutputDebugMessage("Software try to enable ARC to All");
            }
            else
            {
                AudioStatus = 0;
                if((&port[2])->content.cec->EnableAudioAmplifier != AV_CEC_AMP_DISABLED)
                    (&port[2])->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
                AvKapiOutputDebugMessage("Software try to disable ARC to All");
            }
        }
    #endif
        return;
    }
    if CommandNameIs("B") /* StandBy */
    {
        #if AvEnableCecFeature
        if(uArg1 == 1)
        {
            DevicePowerStatus = 1;
            AvKapiOutputDebugMessage("Stand By");
        }
        else
        {
            DevicePowerStatus = 0;
            AvKapiOutputDebugMessage("Power Up");
        }
        #endif
    }
    if CommandNameIs("S") /* STATUS */
    {
        AvKapiOutputDebugMessage("Current Routing Rx%x=>Tx1", RegTx1From);
        return;
    }
    else if CommandNameIs("P") /* PAUSE */
    {
        RegPause = 1;
        AvKapiOutputDebugMessage("Software Paused");
        return;
    }
    else if CommandNameIs("G") /* GO ON */
    {
        RegPause = 0;
        AvKapiOutputDebugMessage("Software Continued");
        return;
    }
    else if CommandNameIs("C") /* CONNECT HDMI PORT */
    {
        if((uArg1 > 5) || (uArg1 <1))
            AvKapiOutputDebugMessage("Invalid Rx! Use 1~4 for Rx, 5 for VideoGen, ReTry!");
        else if((uArg2 != 0) || (uArg3 != 0))
            AvKapiOutputDebugMessage("Don't input 2nd/3rd argument!");
        else
        {
            RegTx1From = uArg1;
            if(uArg1 == 5)
            {
                AvApiConnectPort(&port[4], &port[2], AvConnectVideo);
                AvKapiOutputDebugMessage("Current Routing VideoGen=>Tx1");
            }
            else
            {
                AvApiConnectPort(&port[uArg1-1], &port[2], AvConnectVideo);
                AvKapiOutputDebugMessage("Current Routing Rx%x=>Tx1", RegTx1From);
            }
        }
    }
    else if CommandNameIs("F") /* FRL Stat change */
    {
        if(uArg1 > 7)
            AvKapiOutputDebugMessage("Invalid FRL mode!");
        else if((uArg2 > 0) && (uArg2 < 7) && (uArg1 != 0))
        {
            i = uArg2<<16;
            switch(uArg1)
            {
                case 1:
                    i = i + 3000;
                    break;
                case 2:
                case 3:
                    i = i + 6000;
                    break;
                case 4:
                    i = i + 8000;
                    break;
                case 5:
                    i = i + 10000;
                    break;
                case 6:
                    i = i + 12000;
                    break;
                default:
                    i = i + 6000;
                    break;
            }
            (&port[1])->content.tx->H2p1FrlManual = i;
            (&port[1])->content.tx->Hpd = AV_HPD_FORCE_LOW;
            (&port[1])->content.tx->IgnoreEdidError = 1;
            AvKapiOutputDebugMessage("New Config FRL Manual = %x",i);
        }
        else if((uArg2 > 9) && (uArg2 < 14))
        {
            value = uArg2 - 9;
            (&port[value])->content.tx->H2p1FrlType = (AvFrlType)uArg1;
            (&port[value])->content.tx->Hpd = AV_HPD_FORCE_LOW;
            AvKapiOutputDebugMessage("Tx%d FRL Mode = %d",value,uArg1);
        }
        else
        {
            (&port[1])->content.tx->H2p1FrlType = (AvFrlType)uArg1;
            (&port[1])->content.tx->Hpd = AV_HPD_FORCE_LOW;
            AvKapiOutputDebugMessage("TxA New Config FRL Mode = %d",uArg1);
        }
    }
    else if CommandNameIs("E") /* EDID change */
    {
        if(uArg1 > 8)
            AvKapiOutputDebugMessage("Invalid FRL mode!");
        else if(uArg2 != 0)
            AvKapiOutputDebugMessage("Don't input 2nd argument!");
        else if(uArg1 == 7)
        {
            AvEdidCeaParamForce = 0;
            (&port[0])->content.rx->EdidStatus = AV_EDID_NEEDUPDATE;
            AvKapiOutputDebugMessage("Copy Downstream EDID!");
        }
        else
        {
            AvEdidSupportFreq = AvEdidBitFreq6G    |
                                AvEdidBitFreq4P5G  |
                                AvEdidBitFreq3P75G |
                                AvEdidBitFreq3G    |
                                AvEdidBitFreq2P25G |
                                AvEdidBitFreq1P5G  |
                                AvEdidBitFreq750M  |
                                AvEdidBitFreq270M  |
                                AvEdidBitFreq135M;
            if(uArg1 > 0)
                AvEdidSupportFreq = AvEdidSupportFreq | AvEdidBitFreq3G3Lane;
            if(uArg1 > 1)
                AvEdidSupportFreq = AvEdidSupportFreq | AvEdidBitFreq6G3Lane;
            if(uArg1 > 2)
                AvEdidSupportFreq = AvEdidSupportFreq | AvEdidBitFreq6G4Lane;
            if(uArg1 > 3)
                AvEdidSupportFreq = AvEdidSupportFreq | AvEdidBitFreq8G4Lane;
            if(uArg1 > 4)
                AvEdidSupportFreq = AvEdidSupportFreq | AvEdidBitFreq10G4Lane;
            if(uArg1 > 5)
                AvEdidSupportFreq = AvEdidSupportFreq | AvEdidBitFreq12G4Lane;
            (&port[0])->content.rx->EdidStatus = AV_EDID_NEEDUPDATE;
            //(&port[1])->content.rx->EdidStatus = AV_EDID_NEEDUPDATE;
            /* Force FRL Frequency */
            AvEdidCeaParamForce = AvEdidBitCeaSVD | AvEdidBitCeaVSDBHF;
            AvKapiOutputDebugMessage("EDID Config FRL Mode = %d",uArg1);
        }
    }
    else if CommandNameIs("V") /* VideoGen */
    {
        if((uArg1 > 5) || (uArg1 <1))
            AvKapiOutputDebugMessage("Invalid Timing Format! 1=480p,2=1080p,3=4k60,4=4k120,5=8k30");
        else if((uArg2 != 0) || (uArg3 != 0))
            AvKapiOutputDebugMessage("Don't input 2nd/3rd argument!");
        else
        {
            switch(uArg1)
            {
                case 1:
                    (&port[4])->content.video->timing.Vic = 0x02;
                    break;
                case 2:
                    (&port[4])->content.video->timing.Vic = 0x10;
                    break;
                case 3:
                    (&port[4])->content.video->timing.Vic = 0x61;
                    break;
                case 4:
                    (&port[4])->content.video->timing.Vic = 0x76;
                    break;
                case 5:
                    (&port[4])->content.video->timing.Vic = 0xc4;
                    break;
            }
            (&port[2])->content.tx->Hpd = AV_HPD_FORCE_LOW;
        }
    }
    else if CommandNameIs("M") /* Measure input eye */
    {
        switch(uArg1)
        {
            case 1:
                KfunRxReadInfo((&port[0]));
                break;
            case 2:
                KfunRxReadInfo((&port[1]));
                break;
        }
    }
    if CommandNameIs("L") /* ARC */
    {
        switch(LoopMode)
        {
            case AV_FRL_10G4L:
                LoopMode = AV_FRL_3G3L;
                break;
            case AV_FRL_8G4L:
                LoopMode = AV_FRL_10G4L;
                break;
            case AV_FRL_6G4L:
                LoopMode = AV_FRL_8G4L;
                break;
            case AV_FRL_6G3L:
                LoopMode = AV_FRL_6G4L;
                break;
            case AV_FRL_3G3L:
                LoopMode = AV_FRL_6G3L;
                break;
            default:
                LoopMode = AV_FRL_3G3L;
                break;
        }
        if(LoopMode == AV_FRL_3G3L)
            LoopCount = LoopCount + 1;
        (&port[2])->content.tx->H2p1FrlType = LoopMode;
        (&port[2])->content.tx->Hpd = AV_HPD_FORCE_LOW;
        AvKapiOutputDebugMessage("Loop FRL Mode = %d, Round %d",(uint8)LoopMode, LoopCount);
    }

    if CommandNameIs("H")
    {
      /* Sets the HDCP type for a port.
       * The first arg is the port number (1-6, where 1-4 are INPUTS and where 5-6 are OUTPUTS)
       * The second arg is the HDCP mode : 1 --> HDCP 1.4, 2 --> HDCP 2.2, 3 --> AUTO, other --> No encryption */

      if(port[uArg1-1].type == HdmiRx)
      {
          if(uArg2 == 1)
          {
              port[uArg1-1].content.hdcp->HdcpNeeded = AV_HDCP_RX_1P4_ONLY;
          }
          else if(uArg2 == 2)
          {
              port[uArg1-1].content.hdcp->HdcpNeeded = AV_HDCP_RX_2P2_ONLY;
          }
          else if(uArg2 == 3)
          {
              port[uArg1-1].content.hdcp->HdcpNeeded = AV_HDCP_RX_AUTO;
          }
          else if(uArg2 == 4)
          {
              port[uArg1-1].content.hdcp->HdcpNeeded = AV_HDCP_RX_FOLLOW_SINK;
          }
          else
          {
              port[uArg1-1].content.hdcp->HdcpNeeded = AV_HDCP_RX_NOT_SUPPORT;
          }
          /* Force Rx DDC Rerun */
          port[uArg1-1].content.rx->Hpd = AV_HPD_TOGGLE;
      }
      else if(port[uArg1-1].type == HdmiTx)
      {
          if(uArg2 == 1)
          {
              port[uArg1-1].content.hdcptx->HdmiStyle = AV_HDCP_TX_1P4_ONLY;
          }
          else if(uArg2 == 2)
          {
              port[uArg1-1].content.hdcptx->HdmiStyle = AV_HDCP_TX_2P2_ONLY;
          }
          else if(uArg2 == 3)
          {
              port[uArg1-1].content.hdcptx->HdmiStyle = AV_HDCP_TX_AUTO;
          }
          else
          {
              port[uArg1-1].content.hdcptx->HdmiStyle = AV_HDCP_TX_ILLEGAL_NO_HDCP;
          }
          /* Force Tx HPD rerun */
          port[uArg1-1].content.hdcptx->HdcpModeUpdate = 1;
      }
      else
      {
        AvKapiOutputDebugMessage("Physical port #%u does not exist!", uArg1);
      }
    }
    else if CommandNameIs("T") /* Temperature */
    {
        i = KfunDevMeasureTj(port->device);
        AvKapiOutputDebugMessage("Chip Temerapture = %d", i);
    }

#if AvEnableIntegrityCheck
    else if CommandNameIs("R") /* Report Integrity */
    {
        IntegrityReport();
    }
#endif

    if CommandNameIs("X")
    {
        /* Step 1. PHY bypass enable */
        AvHalI2cWriteField8(0x01B000,0x03,0xFF,0,0x0F);
        AvHalI2cWriteField8(0x01B060,0xF4,0xFF,0,0x01);
        AvHalI2cWriteField8(0x01B001,0x07,0xFF,0,0x18);
        AvHalI2cWriteField8(0x01B000,0x12,0xFF,0,0x0C);
        AvHalI2cWriteField8(0x01B060,0xF1,0xFF,0,0x00);
        /* HDCP logic disable */
        //AvHalI2cWriteField8(0x01B000,0x93,0xFF,0,0x00);
        //AvHalI2cWriteField8(0x01B000,0xBE,0xFF,0,0x00);
        while(1)
            AvHalI2cReadField8(0x01B001,0x07,0xFF,0,&value);
    }
    if CommandNameIs("Y")
    {
        /* Step 1. PHY bypass enable */
        AvHalI2cWriteField8(0x01B000,0x03,0xFF,0,0x0F);
        AvHalI2cWriteField8(0x01B060,0xF4,0xFF,0,0x01);
        AvHalI2cWriteField8(0x01B001,0x07,0xFF,0,0x18);
        AvHalI2cWriteField8(0x01B000,0x12,0xFF,0,0x0C);
        AvHalI2cWriteField8(0x01B060,0xF1,0xFF,0,0x00);
        /* HDCP logic disable */
        //AvHalI2cWriteField8(0x01B000,0x93,0xFF,0,0x00);
        //AvHalI2cWriteField8(0x01B000,0xBE,0xFF,0,0x00);
        for(j=0;j<2;j++)
        {
            for(i=0;i<0xffffff;i++);
            num = 48+j;
            AvHalUartSendByte(&num, 1);
            AvHalUartSendByte(&uReturn, 1);
        }
        while(1)
        {
            AvHalI2cReadField8(0x01B001,0x07,0xFF,0,&value);
            if(value != 0x18)
            {
                while(1)
                {
                    for(i=0;i<0xffff;i++)
                        RxInLedOut(0);
                    for(i=0;i<0xffff;i++)
                        RxInLedOut(1);
                    if(AvHalUartGetByte(&value) != AvError)
                        break;
                }
            }
        }
    }
    if CommandNameIs("Z")
    {
        /* Disable 1.4 FSM engine */
        while(1)
        {
            AvHalI2cReadField8(0x01B062,0x07,0xFF,0,&value);
            if(value != 0x4A)
            {
                AvKapiOutputDebugMessage("\n\rE-0x%x",value);
                AvHalI2cReadField8(0x01B0FF,0xFF,0xFF,0,&value);
                AvKapiOutputDebugMessage("\n\rR-0x%x",value);
                while(1)
                {
                    for(i=0;i<0xffff;i++)
                        RxInLedOut(0);
                    for(i=0;i<0xffff;i++)
                        RxInLedOut(1);
                    if(AvHalUartGetByte(&value) != AvError)
                        break;
                }
            }
        }
    }
    if CommandNameIs("U")
    {
        AvHalI2cWriteField8(0x01B023,0xDE,0xFF,0,0xF9);
        AvHalI2cWriteField8(0x01B023,0xDF,0xFF,0,0x28);
        AvHalI2cWriteField8(0x01B023,0xE0,0xFF,0,0x6E);
        AvHalI2cWriteField8(0x01B023,0xE1,0xFF,0,0x58);

        while(1)
        {
            AvHalI2cReadField8(0x0101B062,0x07,0xFF,0,&value);
            if(value != 0x4A)
            {
                AvKapiOutputDebugMessage("\n\rE-0x%x",value);
                AvHalI2cReadField8(0x01B0FF,0xFF,0xFF,0,&value);
                AvKapiOutputDebugMessage("\n\rR-0x%x",value);
                while(1)
                {
                    for(i=0;i<0xffff;i++)
                        RxInLedOut(0);
                    for(i=0;i<0xffff;i++)
                        RxInLedOut(1);
                    if(AvHalUartGetByte(&value) != AvError)
                        break;
                }
            }
        }
    }


#endif
}

void ListenToUartCommand(AvPort *port)
{
#if AvEnableUartInput
    ProcessChar(port);
    while(RegPause == 1)
        ProcessChar(port);

#endif
}
