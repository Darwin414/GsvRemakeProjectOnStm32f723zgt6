/**
 * @file av_event_handler.c
 *
 * @brief av event handler to handle the events for customers use
 */
#include "av_event_handler.h"

#if AvEnableCecFeature /* CEC Related */
extern uchar  DevicePowerStatus;
extern char   DeviceName[20];
extern uchar  AudioStatus;
extern CecAudioStatus CecTxAudioStatus;
extern uint8  ARCFeatureSupport;
uint16 AvCecFindRequiredInput (AvPort *port, uint16 SrcPhys);
#endif

/* Edid Related Declare Start */
extern AvEdidReg DevEdidReg;
extern uint16    AvEdidSupportFreq;
extern uint16    AvEdidVesaParamForce;
extern uint8     AvEdidCeaParamForce;
extern uint16    AvEdidCeabParamForce;
extern uint16    AvEdidVesaParamRemove;
extern uint8     AvEdidCeaParamRemove;
extern uint16    AvEdidCeabParamRemove;
extern uint8     AvEdidSupportColorDepth;
extern uint16    AvEdidSupportFrameRate;
uint8 InEdid[AvEdidMaxSize];
uint8 SinkEdid[AvEdidMaxSize];
uint8 OutEdid[AvEdidMaxSize];
AvRet AvEdidPortManage(AvPort *RxPort);
AvRet AvEdidPortAnalysis(AvPort *port);
void AvEdidMaxFrlFilter(uint8 *EdidContent, uint8 Max);
void AvEdidDscFilter(uint8 *EdidContent);
void AvEdidFreeSyncFilter(uint8 *EdidContent);
/* Edid Related Declare End */

/* Earc CapData Related Declare Start */
extern AvEarcReg DevEarcReg;
extern uint8  AvEarcParamForce;
extern uint8  AvEarcParamRemove;
extern uint8  AvEarcStreamLayout;
AvRet AvEarcCapDataManage(AvPort *port);
/* Earc CapData Related Declare End */
/**
 * @brief  process audio/video events
 * @return none
 */
AvRet AvHandleEvent(AvPort *port, AvEvent event, uint8 *wparam, uint8 *pparam)
{
    AvRet ret = AvOk;
    AvPort *TempPort;
#if AvEnableCecFeature /* CEC Related */
    uint8 *RxContent = port->content.cec->RxContent;
    uint8 *TxContent = port->content.cec->TxContent;
    uint8 MsgLenValue = 2;
    uint8 *Msg_Len = &MsgLenValue;
    uint16 ActiveSource = 0;
    uint16 InpIndex;
    uint16 Recv_Phy_Addr;
    uint8  DeviceType;
    uint8  CecAbortReason;
    uint8  Len;
    uint8  Spa[2];
#endif

    switch(event)
    {
        case AvEventRxSetHdcpStyle:
            /* Rx Hdcp Selection:
            AV_HDCP_RX_NOT_SUPPORT = 0,
            AV_HDCP_RX_AUTO        = 1,
            AV_HDCP_RX_FOLLOW_SINK = 2,
            AV_HDCP_RX_1P4_ONLY    = 3,
            AV_HDCP_RX_2P2_ONLY    = 4,
             */
            port->content.hdcp->HdcpNeeded = AV_HDCP_RX_AUTO;
            break;

        case AvEventTxSetHdcpStyle:
            /* Tx Hdcp Selection:
            AV_HDCP_TX_ILLEGAL_NO_HDCP = 0,
            AV_HDCP_TX_AUTO            = 1,
            AV_HDCP_TX_AUTO_FAIL_OUT   = 2,
            AV_HDCP_TX_1P4_ONLY        = 3,
            AV_HDCP_TX_1P4_FAIL_OUT    = 4,
            AV_HDCP_TX_2P2_ONLY        = 5,
            AV_HDCP_TX_2P2_FAIL_OUT    = 6
               Tx Dvi Selection:
            AV_HDCP_TX_DVI_STRICT      = 0,
            AV_HDCP_TX_DVI_LOOSE       = 1
            */
            port->content.hdcptx->HdmiStyle = AV_HDCP_TX_AUTO;
            port->content.hdcptx->DviStyle  = AV_HDCP_TX_DVI_LOOSE;
            break;

        case AvEventRxPortInit:
            /* HDMI Rx FRL Mode Selection:
               AV_FRL_RATE_DOWNGRADE   = 0,
               AV_FRL_RATE_FIXED       = 2,
             */
            port->content.rx->H2p1FrlMode = AV_FRL_RATE_FIXED;
#if AvEnableRxSingleLaneMode
            /* single lane configuration: Lane0(0x01), 10Gbps */
            port->content.rx->H2p1FrlManual = (0x08<<16) + (10*1000);
#endif
            break;

        case AvEventTxPortInit:
#if AvEnableEArcRxFeature
            if(port->content.earx != NULL)
            {
                if(((port->content.earx->Enable == AV_EARCRX_EN_ENABLED) ||
                    (port->content.earx->Enable == AV_EARCRX_EN_TO_ENABLE)) &&
                   (port->content.earx->CapStat == AV_EARCRX_CAP_RESET))
                {
                    AvEarcCapDataManage(port);
                }
            }
#endif
#if AvEnableTxSingleLaneMode
            /* single lane configuration: Lane0(0x01), 10Gbps */
            port->content.tx->H2p1FrlManual = (0x01<<16) + (10*1000);
            port->content.tx->IgnoreEdidError = 1;
#endif
            /* Enable HDMI 5V output after preparation */
            TxOut5Venable(1, 1);
            break;

        case AvEventPortAudioInfoChanged:
            break;

        case AvEventPortVideoInfoChanged:
            break;

        case AvEventUpStreamConnectNewDownStream:
#if AvNoLinkageMode
#else
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
                TempPort->content.rx->EdidStatus = AV_EDID_RESEND;
#endif
            break;

        case AvEventPortDownStreamDisconnected:
#if AvNoLinkageMode
#else
    #if AvStrictEdidRule
            port->content.tx->EdidCks[0] = 0xFF;
            port->content.tx->EdidCks[1] = 0xFF;
    #endif
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
                TempPort->content.rx->EdidStatus = AV_EDID_SINKLOST;
#endif
            TxOutLedOut(port->index-4, 0);
            TxOut5Venable(1, 1);
            break;

        case AvEventPortDownStreamConnected:
            TxOutLedOut(port->index-4, 1);
            TxOut5Venable(1, 1);
            break;

        case AvEventPortDownStreamSending:
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
            {
                if(TempPort->type == HdmiRx)
                {
                    /* 1. Set HDMI mode */
                    /* HDMI Mode Input Judge */
                    if(TempPort->content.rx->IsInputStable == 1)
                    {
                        if((port->content.tx->IgnoreEdidError == 1) || /* sink is default as HDMI */
                           (port->content.tx->PhyAddr != 0x0000))      /* sink is HDMI */
                            port->content.tx->HdmiMode = TempPort->content.rx->HdmiMode;
                        else
                            port->content.tx->HdmiMode = 0; /* sink is DVI as only option */
                    }
                    else
                        port->content.tx->HdmiMode = 1; /* sink is default to be HDMI */
                    KfunTxSetHdmiModeSupport(port);
                    /* 2. Set FRL Rate if necessary */
                    if(port->content.tx->H2p1FrlManual == 0)
                    {
                        TempPort = (AvPort*)(port->content.RouteVideoFromPort);
                        if(TempPort->type == VideoScaler)
                            port->content.tx->H2p1FrlType = AV_FRL_NONE;
#if (AvEnableDualFrlOutRate == 0)
                        if((TempPort->type == HdmiRx) && (port->core.HdmiCore == -1))
                            port->content.tx->H2p1FrlType = TempPort->content.rx->H2p1FrlType;
#endif
#if ((AvEnableTxSingleLaneMode == 0) && (Gsv6k5MuxMode == 1))
                        port->content.tx->H2p1FrlType = TempPort->content.rx->H2p1FrlType;
#endif
                     }
                }
            }
            break;

        case AvEventPortUpStreamDisconnected:
            RxInLedOut(0);
            break;

        case AvEventPortUpStreamConnected:
            RxInLedOut(1);
            break;

        case AvEventPortDownStreamPowerOff:
            /* Enable HDMI 5V output after preparation */
            TxOut5Venable(1, 0);
            AvKapiOutputDebugMessage("TxPort: 5V Off");
            break;

        case AvEventPortDownStreamPowerOn:
            TxOut5Venable(1, 1);
            AvKapiOutputDebugMessage("TxPort: 5V On");
            break;

        case AvEventRxPrepareEdid:
            if(port->content.rx->EdidStatus != AV_EDID_UPDATED)
            {
                AvEdidPortManage(port);
                port->content.rx->EdidStatus = AV_EDID_UPDATED;
            }
            break;

        case AvEventPortEdidReady:
            AvEdidPortAnalysis(port);
            /* Resend Edid */
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
            {
                if((TempPort->type == HdmiRx) || (TempPort->type == DpRx))
                    TempPort->content.rx->EdidStatus = AV_EDID_RESEND;
#if AvEnableTTLCompressAudio
                /* 0 = Copy EDID, 1 = Force EDID */
                AvEdidCeaParamForce = 1;
#endif
            }
            break;

        case AvEventPortEdidReadFail:
            break;

        case AvEventTxDefaultEdid:
            port->content.tx->EdidSupportFeature = 0xffffffff;
            port->content.tx->H2p1SupportFeature = 0x000001fe;
            break;

        case AvEventPortUpStreamEncrypted:
            /* Upstream input is encrypted */
            break;

        case AvEventPortUpStreamDecrypted:
            /* Upstream input is not encrypted */
            break;

#if AvEnableCecFeature /* CEC Related */

        case AvEventCecRxMessage:
            break;

        case AvEventCecTxDone:
            break;

        case AvEventCecTxTimeout:
            AvHandleEvent(port, AvEventCecTxError, 0, NULL);
            break;

        case AvEventCecTxArbLost:
            AvHandleEvent(port, AvEventCecTxError, 0, NULL);
            break;

            /* when kernel layer finds a valid LogAddr, it tells */
        case AvEventCecLogAddrAlloc:
            /* user can modify the LogAddr by themselves, in this example,
               LogAddr will be allocated to the 1st none-zero value, all the
               unallocated address will be poped here */
            if(*wparam == AvCecLogicAddress)
            {
                port->content.cec->LogAddr = *wparam;
                AvKapiCecSetLogicalAddr(port);
                AvKapiOutputDebugMessage("CEC Port%d: Set Logical Address = %d",
                                         port->index, port->content.cec->LogAddr);
                /* Manual Stop Logic Address Search */
                port->content.cec->AddrIndex = 15;
            }
            break;

        case AvEventCecTxError:
            /* to be inserted */
            break;

        case AvEventCecRxMessageRespond:
            ret = AvOk;
            break;

        case AvEventCecDefaultManage:
            /* 1. ARC enable after eARC session expires */
#if (AvEnableEArcRxFeature == 1) && (AvEnableCecCts == 0)
            if((AudioStatus == 1) && (port->content.earx->LinkStat == AV_EARCRX_ST_LINK_FAIL))
            {
                port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
            }
#endif
            /* 2. ModetoTV enable */
            if(port->content.cec->AmplifierDelayExpire != 0)
            {
                port->content.cec->AmplifierDelayExpire = port->content.cec->AmplifierDelayExpire - 1;
                /* AvKapiOutputDebugMessage("%d",port->content.cec->AmplifierDelayExpire); */
            }
            if(port->content.cec->AmplifierDelayExpire == 1)
            {
                if(AudioStatus == 1)
                {
                    if(port->content.cec->EnableAudioAmplifier != AV_CEC_AMP_ENABLED)
                        port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                    if(port->content.cec->EnableARC != AV_CEC_ARC_INITIATED)
                        port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
                }
                else
                {
                    if(port->content.cec->EnableAudioAmplifier != AV_CEC_AMP_DISABLED)
                        port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
                }
            }
            break;

        case AvEventCecArcManage:
            if(port->content.cec->TxSendFlag == AV_CEC_TX_SEND_SUCCESS)
            {
                if(port->content.cec->ARCTryCount < 2)
                {
                    port->content.cec->ARCTryCount = port->content.cec->ARCTryCount + 1;
                    if(port->content.cec->EnableARC == AV_CEC_ARC_TO_INITIATE)
                        AvHandleEvent(port, AvEventCecSendInitiateARC, NULL, NULL);
                    else if(port->content.cec->EnableARC == AV_CEC_ARC_TO_TERMINATE)
                        AvHandleEvent(port, AvEventCecSendTerminateARC, NULL, NULL);
                }
            }
#if AvEnableEArcRxFeature
            if(port->content.earx->LinkStat == AV_EARCRX_ST_LINK_SUCCESS)
            {
                AudioStatus = 1;
                port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
            }
#endif
            break;
#endif

#if AvCecDataSendingOutEvent

        case AvEventCecSendRoutingChange:
            /* 0x80, init of 2.2 routing control */
            /* pparam = NewPort, wparam = OldPort */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 6;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_ROUTE_CHANGE);
            AV_CEC_SET_PHYS_ADDR1(TxContent, ((wparam[0]<<8)+wparam[1]));
            AV_CEC_SET_PHYS_ADDR2(TxContent, ((pparam[0]<<8)+pparam[1]));
            AvKapiOutputDebugMessage("CEC: Sending routing change. From Inp=%04x to Inp=%04x",
                                     ((wparam[0]<<8)+wparam[1]), ((pparam[0]<<8)+pparam[1]));
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendRoutingInformation:
            /* 0x81, response for 2.2 routing control */
            /* after EDID allocation of upper level chain, enable this feature */
            Spa[0] = (port->content.tx->PhyAddr)>>8;
            Spa[1] = (port->content.tx->PhyAddr)&0xff;
            KfunGenerateSourceSpa(port, Spa, (port->content.cec->InputCount>>4));
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_ROUTE_INFO);
            AV_CEC_SET_PHYS_ADDR1(TxContent, ((Spa[0]<<8)+Spa[1]));
            AvKapiOutputDebugMessage("CEC: Sending routing information. Inp=%04x", ((Spa[0]<<8)+Spa[1]));
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportPhysAddress:
            /* 0x84, report physical address */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=5;
            switch(port->content.cec->LogAddr)
            {
                case AV_CEC_PLAYBACK1_LOG_ADDRESS:
                case AV_CEC_PLAYBACK2_LOG_ADDRESS:
                case AV_CEC_PLAYBACK3_LOG_ADDRESS:
                   DeviceType = AV_CEC_PLAYBACK1_LOG_ADDRESS;
                   break;
                case AV_CEC_RECORD1_LOG_ADDRESS:
                case AV_CEC_RECORD2_LOG_ADDRESS:
                   DeviceType = AV_CEC_RECORD1_LOG_ADDRESS;
                   break;
                case AV_CEC_TUNER1_LOG_ADDRESS:
                case AV_CEC_TUNER2_LOG_ADDRESS:
                case AV_CEC_TUNER3_LOG_ADDRESS:
                case AV_CEC_TUNER4_LOG_ADDRESS:
                   DeviceType = AV_CEC_TUNER1_LOG_ADDRESS;
                   break;
                default:
                   DeviceType = port->content.cec->LogAddr;
                   break;
            }
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_PHYS_ADDR);
            if(port->type == HdmiTx)
            {
                AV_CEC_SET_PHYS_ADDR1(TxContent,port->content.tx->PhyAddr);
            }
            else
            {
                AV_CEC_SET_PHYS_ADDR1(TxContent,0x0000);
            }
            TxContent[4] = DeviceType;
            AvKapiOutputDebugMessage("CEC: Sending Report phys address=%02x%02x", TxContent[2],TxContent[3]);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendActiveSource:
            /* 0x82, send active source, step 2 of 2.1 one touch play */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_ACTIVE_SRC);
            if(port->type == HdmiTx)
            {
                AV_CEC_SET_PHYS_ADDR1(TxContent,port->content.tx->PhyAddr);
            }
            else
            {
                AV_CEC_SET_PHYS_ADDR1(TxContent,0x0000);
            }
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendInactiveSource:
            /* 0x82, send active source, step 2 of 2.1 one touch play */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_INACTIVE_SOURCE);
            if(port->type == HdmiTx)
            {
                AV_CEC_SET_PHYS_ADDR1(TxContent,port->content.tx->PhyAddr);
            }
            else
            {
                AV_CEC_SET_PHYS_ADDR1(TxContent,0x0000);
            }
            TxContent[4] = DeviceType;
            AvKapiOutputDebugMessage("CEC: Sending Inactive source=%02x%02x", TxContent[2],TxContent[3]);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendFeatureAbort:
            /* 0x00, default response */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            port->content.cec->TxMsg = AV_CEC_MSG_FEATURE_ABORT;
            /* reverse logical addr */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_FEATURE_ABORT);
            TxContent[2] = AV_CEC_OPCODE(RxContent);
            TxContent[3] = AV_CEC_ABORT_REASON_REFUSED;
            AvKapiOutputDebugMessage("CEC: Sending feature abort");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendPowerStatus:
            /* 0x90 ,14.2 report power status, response of give power status */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            port->content.cec->TxMsg = AV_CEC_MSG_REPORT_PWR_STATUS;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_PWR_STATUS);
            TxContent[2] = DevicePowerStatus;
            AvKapiOutputDebugMessage("CEC: Sending report power status");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendSetOsdName:
            /* 0x47, 10.2 response for give osd name */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 16;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent)); /* Directly addressed */
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SET_OSD_NAME);
            for (Len=0; (Len<14) && DeviceName[Len]; Len++)
            {
                TxContent[Len+2] = DeviceName[Len];
            }
            AvKapiOutputDebugMessage("CEC: Sending Set OSD name");
            AvKapiCecSendMessage(port);
            break;
        case AvEventCecSendDeviceVendorID:
            /* 0x87, 9.2 vendor id broadcast */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 5;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr); /* Broadcast */
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_DEVICE_VENDOR_ID);
            TxContent[2] = AvCecGsvVendorIdByte1;
            TxContent[3] = AvCecGsvVendorIdByte2;
            TxContent[4] = AvCecGsvVendorIdByte3;
            AvKapiOutputDebugMessage("CEC: Sending Vendor ID");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendCecVersion:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_CEC_VERSION);
            TxContent[2] = 0x05; /* only support CEC 1.4 */
            AvKapiOutputDebugMessage("CEC: Sending CEC Version 1.4");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendFeatureAbortReason:
            /* Abort Reason = wparam */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            port->content.cec->TxMsg = AV_CEC_MSG_FEATURE_ABORT;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_FEATURE_ABORT);
            TxContent[2] = AV_CEC_OPCODE(RxContent);
            TxContent[3] = (uint8)*wparam;
            AvKapiOutputDebugMessage("CEC: Sending feature abort. Abort Reason=%02x", *wparam);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReqActiveDevice:
            /* TV sends request active source */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=2;
            AV_CEC_SET_HDR_BC(TxContent, port->content.tx->PhyAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REQ_ACTIVE_SRC);
            AvKapiOutputDebugMessage("CEC MSG: Sending active source request");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendSetSystemAudioModeToTv:
            /* Audio 1.2: response to system audio mode request */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=3;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_TV_LOG_ADDRESS, AvCecLogicAddress);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE);
            TxContent[2] = AudioStatus;
            AvKapiCecSendMessage(port);
            CecTxAudioStatus.AudioMode = AudioStatus;
            break;

        case AvEventCecSendSetSystemAudioModeToAll:
#if AvEnableEArcRxFeature
            /* Switch Audio Status to ON when eARC Rx succeeds */
            if(port->content.earx->LinkStat == AV_EARCRX_ST_LINK_SUCCESS)
            {
                AudioStatus = 1;
                /* eARC synchronize on ARC state */
                /* no CEC ARC any more */
                if(port->content.cec->EnableARC == AV_CEC_ARC_TO_INITIATE)
                    port->content.cec->EnableARC = AV_CEC_ARC_INITIATED;
                if(port->content.cec->EnableARC == AV_CEC_ARC_TO_TERMINATE)
                    port->content.cec->EnableARC = AV_CEC_ARC_TERMINATED;
            }
#endif
            /* Audio 1.2: response to system audio mode request */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=3;
            AV_CEC_SET_HDR_BC(TxContent, AvCecLogicAddress);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE);
            TxContent[2] = AudioStatus;
            AvKapiCecSendMessage(port);
            CecTxAudioStatus.AudioMode = AudioStatus;
            break;

        case AvEventCecSendReportAudioStatus:
            /* response of AvEventCecMsgGiveAudioStatus */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_AUDIO_STATUS);
            /* AV_CEC_AUDIO_MUTE_SHIFTER = bit 7 */
            TxContent[2] = CecTxAudioStatus.Mute << 7;
            /* CEC_AUDIO_MASK_MUTE_BIT = 0x7f */
            TxContent[2] = TxContent[2] | (CecTxAudioStatus.Volume & 0x7f);
            AvKapiOutputDebugMessage("CEC: Sending audio status");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportSystemAudioModeStatus:
            /* response of AvEventCecMsgGiveSystemAudioModeStatus */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS);
            TxContent[2] = CecTxAudioStatus.AudioMode;        /* audio mode status      */
            AvKapiOutputDebugMessage("CEC: Sending audio mode status");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportShortAudioDecriptor:
            /* response of AvEventCecMsgRequestShortAudioDescriptor */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 5;

            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR);

            TxContent[2] = (CecTxAudioStatus.MaxNumberOfChannels-1) | ( (CecTxAudioStatus.AudioFormatCode) << 3);
            TxContent[3] = CecTxAudioStatus.AudioSampleRate;          /* audio  capability 2      */
            TxContent[4] = CecTxAudioStatus.AudioBitLen;
            AvKapiOutputDebugMessage("CEC: Sending audio capability");
            AvKapiCecSendMessage(port);
            break;

        case AvEventMsgRequestCurrentLatency:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=2;
            AV_CEC_SET_HDR_BC(TxContent, port->content.tx->PhyAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REQUEST_CURRENT_LATENCY);
            /* fixed TV as target for response */
            AV_CEC_SET_PHYS_ADDR1(TxContent, 0x0000);
            AvKapiOutputDebugMessage("CEC MSG: sending Request Current Latency");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendGivePhyAddr:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_GIVE_PHYS_ADDR);
            AvKapiOutputDebugMessage("CEC: Sending Give Physical Address");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendGiveVendorId:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_GET_VENDOR_ID);
            AvKapiOutputDebugMessage("CEC: Sending Give Vendor ID");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendGiveOsdName:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_GIVE_OSD_NAME);
            AvKapiOutputDebugMessage("CEC: Sending Give Physical Address");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendGiveSystemAudioModeStatus:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_GIVE_SYSTEM_AUDIO_MODE_STATUS);
            AvKapiOutputDebugMessage("CEC: Sending Give System Audio Mode Status");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendRequestArcInitiation:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REQUEST_ARC_INITIATION);
            AvKapiOutputDebugMessage("CEC: Sending Request Arc Initiation");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendRequestArcTermination:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REQUEST_ARC_TERMINATION);
            AvKapiOutputDebugMessage("CEC: Sending Request Arc Termination");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendSystemAudioModeRequest:
            /* Directly addressed */
            port->content.cec->TxLen = 2;
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SYSTEM_AUDIO_MODE_REQUEST);
            if((port->content.cec->EnableAudioAmplifier == AV_CEC_AMP_TO_ENABLE) ||
               (port->content.cec->EnableAudioAmplifier == AV_CEC_AMP_ENABLED))
            {
                port->content.cec->TxLen = 4;
                TxContent[2] = (uint8)(CecTxAudioStatus.ActiveSource>>8);
                TxContent[3] = (uint8)(CecTxAudioStatus.ActiveSource>>0);
            }
            AvKapiOutputDebugMessage("CEC: Sending Audio Mode Request %d bytes",port->content.cec->TxLen);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendRequestShortAudioDescriptor:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR);
            TxContent[2] = 0x0A;
            AvKapiOutputDebugMessage("CEC: Sending Audio Mode Request");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportArcInitiated:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_ARC_INITIATED);
            AvKapiOutputDebugMessage("CEC: Sending Report Arc Initiated");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportArcTerminated:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_ARC_TERMINATED);
            AvKapiOutputDebugMessage("CEC: Sending Report Arc Terminated");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendGiveAudioStatus:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, port->content.cec->DstAddr, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_GIVE_AUDIO_STATUS);
            AvKapiOutputDebugMessage("CEC: Sending Give Audio Status");
            AvKapiCecSendMessage(port);
            break;

#endif

#if AvCecDataReadingInEvent

        case AvEventCecReceiveSystemAudioModeStatus:
            /* reg setting from AvEventCecMsgSystemAudioModeStatus */
            CecTxAudioStatus.AudioMode = RxContent[2];
            break;

        case AvEventCecReceiveAudioLatency:
            /* HDMI 2.1 Spec Table 10-29 */
            CecTxAudioStatus.VideoLatency = RxContent[4];
            CecTxAudioStatus.LatencyFlags = RxContent[5];
            CecTxAudioStatus.AudioOutputDelay = RxContent[6];
            break;

        case AvEventCecReceiveSetSystemAudioMode:
            /* reg setting from AvEventCecMsgSetSystemAudioMode */
            CecTxAudioStatus.Mute = AV_CEC_AUDIO_MUTE_ON;
            if (RxContent[2])
            {
                CecTxAudioStatus.Mute = AV_CEC_AUDIO_MUTE_OFF;
            }
            break;

        case AvEventCecReceiveAudioStatus:
            /* reg setting from AvEventCecMsgReportAudioStatus */
            /* AV_CEC_AUDIO_MUTE_SHIFTER = bit 7 */
            CecTxAudioStatus.Mute = RxContent[2] >> 7;
            /* CEC_AUDIO_MASK_MUTE_BIT = 0x7f */
            CecTxAudioStatus.Volume = RxContent[2] & 0x7f;
            break;

        case AvEventCecReceiveSetAudioRate:
            /* reg setting from AvEventCecMsgSetAudioRate */
            CecTxAudioStatus.AudioRate = RxContent[2];
            break;

        case AvEventCecReceiveShortAudioDescriptor:
            /* reg setting from AvEventCecMsgReportShortAudioDescriptor */
            /* AV_CEC_AUDIO_FORMAT_ID_SHIFTER = 3 */
            CecTxAudioStatus.AudioFormatCode = RxContent[2] >> 3;
            /* CEC_AUDIO_MASK_AUDIOFORMATID_BIT = 0x07 */
            CecTxAudioStatus.MaxNumberOfChannels = RxContent[2] & 0x07;
            (CecTxAudioStatus.MaxNumberOfChannels)++;
            /* CEC_AUDIO_ENABLE_AUDIOSAMPLERATE_BIT = 0x7f */
            CecTxAudioStatus.AudioSampleRate = RxContent[3] & 0x7f;
            if ( CecTxAudioStatus.AudioFormatCode == AV_AUD_FORMAT_LINEAR_PCM)
            {
                CecTxAudioStatus.AudioBitLen  = RxContent[4];
            }
            else if ( (CecTxAudioStatus.AudioFormatCode >= AV_AUD_FORMAT_AC3)
                     && (CecTxAudioStatus.AudioFormatCode <= AV_AUD_FORMAT_AC3))
            {
                CecTxAudioStatus.MaxBitRate  = ((uint16)RxContent[4]) << 3;
            }
            break;

        case AvEventCecSendActiveSourceToAudio:
            /* reg setting from AvEventCecMsgReportPhyAddr */
            CecTxAudioStatus.ActiveSource = *wparam;
            break;

        case AvEventCecSendInitiateARC:
            /* called from AvEventCecArcManage */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=2;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_TV_LOG_ADDRESS, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_INITIATE_ARC);
            AvKapiOutputDebugMessage("CEC: Sending Initiate ARC");
            AvKapiCecSendMessage(port);
            break;
        case AvEventCecSendTerminateARC:
            /* called from AvEventCecArcManage */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_TV_LOG_ADDRESS, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_TERMINATE_ARC);
            AvKapiOutputDebugMessage("CEC: Sending Terminate ARC");
            AvKapiCecSendMessage(port);
            break;
        case AvEventCecSendMenuStatus:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_MENU_STATUS);
            if(RxContent[2] == 0x00) /* query = deactivate */
                TxContent[2] = 0x00;
            else
                TxContent[2] = 0x01;
            AvKapiOutputDebugMessage("CEC: Sending Menu Status");
            AvKapiCecSendMessage(port);
            break;

#endif

#if AvCecMessageEvent
        case AvEventCecMsgRouteChange:
            /* from RxMsg AV_CEC_MSG_ROUTE_CHANGE */
            if ((AV_CEC_BROADCAST(RxContent)) &&
                (AV_CEC_PHYS_ADDR2(RxContent) == port->content.tx->PhyAddr))
                /* New address is me */
            {
                Spa[0] = port->content.tx->PhyAddr>>8;
                Spa[1] = port->content.tx->PhyAddr & 0xff;
                AvHandleEvent(port, AvEventCecSendRoutingInformation, Spa, NULL);
                AvKapiOutputDebugMessage("CEC: Routing change. Old PA=%04x  New PA=%04x",
                                         AV_CEC_PHYS_ADDR1(RxContent),
                                         AV_CEC_PHYS_ADDR2(RxContent));
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore RouteChange");
            }
            break;

        case AvEventCecMsgRouteInfo:
            /* from RxMsg AV_CEC_MSG_ROUTE_INFO */
            if (AV_CEC_BROADCAST(RxContent))
            {
                Spa[0] = port->content.tx->PhyAddr>>8;
                Spa[1] = port->content.tx->PhyAddr & 0xff;
                AvHandleEvent(port, AvEventCecSendRoutingInformation, Spa, NULL);
                AvKapiOutputDebugMessage("CEC: Routing Info. Active Route=%04x",
                                         AV_CEC_PHYS_ADDR1(RxContent));
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore RouteInfo");
            }
            break;

        case AvEventCecMsgActiveSource:
            /* from RxMsg AV_CEC_MSG_ACTIVE_SRC */
            /* when getting ACTIVE_SRC, power up at first */
            if (AV_CEC_BROADCAST(RxContent))
            {
                Recv_Phy_Addr  = AV_CEC_PHYS_ADDR1(RxContent);
                InpIndex = AvCecFindRequiredInput (port, Recv_Phy_Addr);
                /* Low 4 bits = Input Total, High 4 bits = Current Active Input */
                /* InpIndex is within InputCount Range, and not invalid value 0 */
                if((InpIndex <= ((port->content.cec->InputCount)&0xf)) &&
                   (InpIndex != 0))
                {
                    AvKapiOutputDebugMessage("************Active Source Request From %04x (Port %d)",
                                             Recv_Phy_Addr, InpIndex);
#if AvCecLinkageMode
                    /* Active Input needs to be switched */
                    if(InpIndex != (port->content.cec->InputCount>>4))
                    {
                        AvKapiOutputDebugMessage("CEC Source try to connect Rx%d - Tx[%d][%d]", InpIndex, port->device->index, port->index);
                        /* Find Device's first port (Rx Port) */
                        TempPort = (AvPort*)port->device->port;
                        port->content.cec->InputCount = (InpIndex<<4) | (port->content.cec->InputCount & 0xf);
                        AvApiConnectPort(&TempPort[InpIndex-1], &TempPort[2], AvConnectAV);
                    }
                    /* Current Active Input is already streaming  */
                    else
                    {
                        AvKapiOutputDebugMessage("Current Input %d is already streaming", InpIndex);
                    }
#endif
                }
                else
                {
                    AvKapiOutputDebugMessage("CEC: ignore ActiveSource");
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgGivePhyAddr:
            /* from RxMsg AV_CEC_MSG_GIVE_PHYS_ADDR */
            /* only report phyaddr of self */
            if (AV_CEC_DST(RxContent) == port->content.cec->LogAddr)
            {
                AvKapiOutputDebugMessage("CEC: Give physical address");
                AvHandleEvent(port, AvEventCecSendReportPhysAddress, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore GivePhyAddr");
            }
            break;

        case AvEventCecMsgSetStreamPath:
            /* from RxMsg AV_CEC_MSG_SET_STRM_PATH */
            if (AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("************Set Stream Path Request to Source=%04x", AV_CEC_PHYS_ADDR1(RxContent));
                if (AV_CEC_PHYS_ADDR1(RxContent)==port->content.tx->PhyAddr)
                {
                    AvHandleEvent(port, AvEventCecSendActiveSource, 0, NULL);
                }
                else
                {
#if AvCecLinkageMode
                    /* Switch to appropriate input */
                    InpIndex = AvCecFindRequiredInput (port, AV_CEC_PHYS_ADDR1(RxContent));
                    if (InpIndex <= port->content.cec->InputCount)
                    {
                        AvHandleEvent(port, AvEventCecSendActiveSource, 0, NULL);
                        /* Set Stream requires a new Source */
                        if(InpIndex != (port->content.cec->InputCount>>4))
                        {
                            /* Find Device's first port (Rx Port) */
                            AvKapiOutputDebugMessage("CEC TV try to connect Rx%d - Tx[%d][%d]", InpIndex, port->device->index, port->index);
                            TempPort = (AvPort*)port->device->port;
                            port->content.cec->InputCount = (InpIndex<<4) | (port->content.cec->InputCount & 0xf);
                            AvApiConnectPort(&TempPort[InpIndex-1], &TempPort[2], AvConnectAV);
                        }
                    }
#endif
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore SetStreamPath");
            }
            break;

        case AvEventCecMsgStandby:
            /* from RxMsg AV_CEC_MSG_STANDBY */
            port->content.cec->RxMsg = AV_CEC_MSG_STANDBY;
            port->content.cec->RxLen = 0;
            ret = AvHandleEvent(port, AvEventCecRxMessageRespond,
                                    Msg_Len,
                                    NULL);
            if ( ret == AvOk )
            {
                AvKapiOutputDebugMessage("CEC: To be in Standby mode");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: is already in Standby mode, ignore");
            }
            break;

        case AvEventCecMsgAbort:
            /* from RxMsg AV_CEC_MSG_ABORT */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: 0x%x Abort message",RxContent[2]);
                CecAbortReason = 0;
                AvHandleEvent(port, AvEventCecSendFeatureAbort, &CecAbortReason, NULL);
            }
            else
            {
                 AvKapiOutputDebugMessage("CEC: ignore Abort");
            }
            break;

        case AvEventCecMsgFeatureAbort:
            /* from RxMsg AV_CEC_MSG_FEATURE_ABORT */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Feature 0x%x abort message",RxContent[2]);
                if ( RxContent[2] == 0x72 )
                {
                    port->content.cec->AmplifierDelayExpire = 0;
                }
            }
            else
            {
                 AvKapiOutputDebugMessage("CEC: ignore FeatureAbort");
            }
            break;

        case AvEventCecMsgGivePowerStatus:
            /* from RxMsg AV_CEC_MSG_GIVE_PWR_STATUS */
            /* when getting GIVE_PWR_STATUS, first to power up, then to REPORT_PWR_STATUS */
            AvHandleEvent(port, AvEventCecRxMessageRespond,Msg_Len, NULL);
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Give power status message");
                AvHandleEvent(port, AvEventCecSendPowerStatus, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore GivePowerStatus");
            }
            break;

        case AvEventCecMsgReportPowerStatus:
            /* from RxMsg AV_CEC_MSG_REPORT_PWR_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: get reporting power status message");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ReportPowerStatus");
            }
            break;

        case AvEventCecMsgGiveOsdName:
            /* from RxMsg AV_CEC_MSG_GIVE_OSD_NAME */
            if ((!AV_CEC_BROADCAST(RxContent)) &&
                (AV_CEC_SRC(RxContent) != AV_CEC_BROADCAST_LOG_ADDRESS ))
            {
                AvKapiOutputDebugMessage("CEC: Give OSD name message");
                AvHandleEvent(port, AvEventCecSendSetOsdName, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore GiveOsdName");
            }
            break;

        case AvEventCecMsgGetVendorId:
            /* from RxMsg AV_CEC_MSG_GET_VENDOR_ID */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Get Vendor ID");
                AvHandleEvent(port, AvEventCecSendDeviceVendorID, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore GetVendorId");
            }
            break;

       case AvEventCecMsgGetCecVersion:
           /* from RxMsg AV_CEC_MSG_GET_CEC_VERSION */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Get Cec Version");
                AvHandleEvent(port, AvEventCecSendCecVersion, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore GetCecVersion");
            }
            break;

        case AvEventCecMsgRequestArcInitiation:
            /* from RxMsg AV_CEC_MSG_REQUEST_ARC_INITIATION */
            /* ARC is directly from TV */
            if ((AV_CEC_SRC(RxContent) == AV_CEC_TV_LOG_ADDRESS) && (!AV_CEC_BROADCAST(RxContent)) &&
                ((port->content.tx->PhyAddr & 0x0FFF) == 0) && (ARCFeatureSupport == 1))
            {
                AvKapiOutputDebugMessage("CEC: Request ARC Initiation message");
                ret = AvHandleEvent(port, AvEventCecSendInitiateARC, NULL, NULL);
                AudioStatus = 1;
                port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;

                if( ret != AvOk )
                {
                    CecAbortReason = 0;
                    AvHandleEvent(port, AvEventCecSendFeatureAbortReason, &CecAbortReason, NULL);
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore RequestArcInitiation");
            }
            break;

        case AvEventCecMsgReportArcInitiated:
            /* from RxMsg AV_CEC_MSG_REPORT_ARC_INITIATED */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Report ARC Initiated message");
                port->content.cec->EnableARC = AV_CEC_ARC_INITIATED;
                if(AudioStatus == 0)
                {
                    port->content.cec->EnableARC = AV_CEC_ARC_TO_TERMINATE;
                    port->content.cec->ARCTryCount = 0;
                }
#if (AvEnableCecCts == 0)
                else
                    AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
#endif
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ReportArcInitiated");
            }
            break;

        case AvEventCecMsgRequestArcTermination:
            /* from RxMsg AV_CEC_MSG_REQUEST_ARC_TERMINATION */
            if ( (AV_CEC_SRC(RxContent) == AV_CEC_TV_LOG_ADDRESS) &&
                 (!AV_CEC_BROADCAST(RxContent)) )
            {
                AvKapiOutputDebugMessage("CEC: Request ARC Termination message");
#if AvEnableEArcRxFeature
                if(port->content.earx->LinkStat == AV_EARCRX_ST_LINK_SUCCESS)
                {
                    AudioStatus = 1;
                    AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
                    //CecAbortReason = 1;
                    //AvHandleEvent(port, AvEventCecSendFeatureAbortReason, &CecAbortReason, NULL);
                    break;
                }
#endif
                ret = AvHandleEvent(port, AvEventCecSendTerminateARC, NULL, NULL);
                port->content.cec->EnableARC = AV_CEC_ARC_TO_TERMINATE;
                if( ret != AvOk )
                {
                    CecAbortReason = 0;
                    AvHandleEvent(port, AvEventCecSendFeatureAbortReason, &CecAbortReason, NULL);
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore RequestArcTermination");
            }
            break;

        case AvEventCecMsgReportArcTerminated:
            /* from RxMsg AV_CEC_MSG_REPORT_ARC_TERMINATED */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Report ARC Terminated message");
                if(AudioStatus == 1)
                {
                    port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                    port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
                    port->content.cec->ARCTryCount = 0;
                }
                else
                    AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ReportArcTerminated");
            }
            break;

        case AvEventCecMsgUserControlPressed:
            /* from RxMsg AV_CEC_MSG_USER_CONTROL_PRESSED */
            /* when getting USER_CONTROL_PRESSED, UI Command is POWER ON, then to power up */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: User Control Pressed with UI Command = 0x%x", RxContent[2]);
                switch(RxContent[2])
                {
                    case 0x41: /* Volume Up */
                        if(CecTxAudioStatus.Volume <= 100)
                            CecTxAudioStatus.Volume = CecTxAudioStatus.Volume + 10;
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    case 0x42: /* Volume Down */
                        if(CecTxAudioStatus.Volume >= 10)
                            CecTxAudioStatus.Volume = CecTxAudioStatus.Volume - 10;
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore UserControlPressed");
            }
            break;

        case AvEventCecMsgUserControlReleased:
            /* from RxMsg AV_CEC_MSG_USER_CONTROL_RELEASED */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: User Control Released with UI Command = 0x%x", RxContent[2]);
                switch(RxContent[2])
                {
                    case 0x41: /* Volume Up */
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    case 0x42: /* Volume Down */
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore UserControlReleased");
            }
            break;

        case AvEventCecMsgImageViewOn:
        case AvEventCecMsgTextViewOn:
            /* when getting IMAGE_VIEW_ON or TEXT_VIEW_ON to power up */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Image/Text View On");
                AvHandleEvent(port, AvEventCecSendReqActiveDevice, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ViewOn");
            }
            break;

        case AvEventCecMsgSystemAudioModeRequest:
            //AvKapiOutputDebugMessage("CEC: Info%d - 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x", port->content.cec->RxLen,
            //                         RxContent[0],RxContent[1],RxContent[2],RxContent[3],RxContent[4],RxContent[5]);
            if (!AV_CEC_BROADCAST(RxContent))
            {
                if ( port->content.cec->RxLen == 2 )
                {
                    if(AudioStatus != 0)
                    {
                        AudioStatus = 0;
                        port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
                    }
                }
                else if ( port->content.cec->RxLen == 4)
                {
                    InpIndex = AvCecFindRequiredInput (port, AV_CEC_PHYS_ADDR1(RxContent));
                    AvKapiOutputDebugMessage("CEC: Active Source is %04x (Port%d)",
                                             AV_CEC_PHYS_ADDR1(RxContent), InpIndex);
                    if((InpIndex != 0xFF) || (AV_CEC_PHYS_ADDR1(RxContent) == 0x0000))
                    {
                        if(AV_CEC_SRC(RxContent) != 0)
                        {
                            AudioStatus = 1;
                            AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
                            port->content.cec->AmplifierDelayExpire = 200;
                        }
                        else
                        {
                            if(AudioStatus != 1)
                            {
                                AudioStatus = 1;
                                port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                                port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
                            }
                            AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToAll, NULL, NULL);
                        }
                    }
                }
                else
                {
                    AvKapiOutputDebugMessage("CEC: parameters number is wrong");
                    AvKapiOutputDebugMessage("CEC: ignore SystemAudioModeRequest");
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore SystemAudioModeRequest");

            }
            break;

        case AvEventCecMsgDeckControl:
            /* from RxMsg AV_CEC_MSG_DECK_CONTROL */
            /*================================================
             * when getting DECK_CONTROL
             * to power up
             * no action to [Deck Control Mode], and
             * not to response by <Deck Status> [Deck Info]
             *================================================*/
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: DECK CONTROL message");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore DeckControl");
            }
            break;

        case AvEventCecMsgPlay:
            /* from RxMsg AV_CEC_MSG_PLAY */
            /*===============================================
             * when getting PLAY
             * to power up
             * no action to [Play Mode], and
             * not to response by <Deck Status> [Deck Info]
             *===============================================*/
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: PLAY message");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore Play");

            }
            break;

        case AvEventCecMsgGiveAudioStatus:
            /* from RxMsg AV_CEC_MSG_GIVE_AUDIO_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: give audio status");
                AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                /* SendReportAudioStatus(RxContent); */
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore GiveAudioStatus");
            }
            break;

        case AvEventCecMsgGiveSystemAudioModeStatus:
            /* from RxMsg AV_CEC_MSG_GIVE_SYSTEM_AUDIO_MODE_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: give system audio mode status");
                AvHandleEvent(port, AvEventCecSendReportSystemAudioModeStatus, 0, NULL);
                /* SendReportSystemAudioModeStatus(RxContent); */
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore GiveSystemAudioModeStatus");
            }
            break;

        case AvEventCecMsgRequestShortAudioDescriptor:
            /* from RxMsg AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                if(((RxContent[2] == 0x01) && (port->content.cec->RxLen > 2)) ||
                   ((RxContent[3] == 0x01) && (port->content.cec->RxLen > 3)) ||
                   ((RxContent[4] == 0x01) && (port->content.cec->RxLen > 4)) ||
                   ((RxContent[5] == 0x01) && (port->content.cec->RxLen > 5)))
                {
                    AvKapiOutputDebugMessage("CEC: give short audio descriptor");
                    AvHandleEvent(port, AvEventCecSendReportShortAudioDecriptor, 0, NULL);
                    /* SendReportShortAudioDecriptor(RxContent); */
                }
                else
                {
                    CecAbortReason = 3;
                    AvHandleEvent(port, AvEventCecSendFeatureAbortReason, &CecAbortReason, NULL);
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore RequestShortAudioDescriptor");
            }
            break;

        case AvEventCecMsgReportCurrentLatency:
            /* from RxMsg AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvHandleEvent(port, AvEventCecReceiveAudioLatency, 0, NULL);
                AvKapiOutputDebugMessage("CEC: receiving message about current latency");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ReportCurrentLatency");
            }
            break;

        case AvEventCecMsgSystemAudioModeStatus:
            /* from RxMsg AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvHandleEvent(port, AvEventCecReceiveSystemAudioModeStatus, 0, NULL);
                /* ReceiveSystemAudioModeStatus(RxContent); */
                AvKapiOutputDebugMessage("CEC: receiving message about system audio mode status");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore SystemAudioModeStatus");
            }
            break;

        case AvEventCecMsgSetSystemAudioMode:
            /* from RxMsg AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE */
            if(port->content.cec->LogAddr == AV_CEC_TV_LOG_ADDRESS)
            {
                if((AudioStatus == 1) && (RxContent[2] == 0x00))
                {
                    AudioStatus = 0;
                    if(port->content.cec->EnableAudioAmplifier != AV_CEC_AMP_DISABLED)
                        port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
                }
                else if((AudioStatus == 0) && (RxContent[2] == 0x01))
                {
                    AudioStatus = 1;
                    if(port->content.cec->EnableAudioAmplifier != AV_CEC_AMP_ENABLED)
                        port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                    if(port->content.cec->EnableARC != AV_CEC_ARC_INITIATED)
                        port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
                }
            }
            if (!AV_CEC_BROADCAST(RxContent))
            {
                ret = AvHandleEvent(port, AvEventCecReceiveSetSystemAudioMode, 0, NULL);
                AvKapiOutputDebugMessage("CEC receiving message about setting system audio mode");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore SetSystemAudioMode");
            }
            break;

        case AvEventCecMsgReportAudioStatus:
            /* from RxMsg AV_CEC_MSG_REPORT_AUDIO_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvHandleEvent(port, AvEventCecReceiveAudioStatus, 0, NULL);
                /* ReceiveAudioStatus(RxContent); */
                AvKapiOutputDebugMessage("CEC: receiving message about audio status");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ReportAudioStatus");
            }
            break;

        case AvEventCecMsgSetAudioRate:
            /* from RxMsg AV_CEC_MSG_SET_AUDIO_RATE */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvHandleEvent(port, AvEventCecReceiveSetAudioRate, 0, NULL);
                /* ReceiveSetAudioRate(RxContent); */
                AvKapiOutputDebugMessage("CEC: receiving message about audio rate");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore SetAudioRate");
            }
            break;

        case AvEventCecMsgReportShortAudioDescriptor:
            /* from RxMsg AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: receiving message about short audio descriptor");
                AvHandleEvent(port, AvEventCecReceiveShortAudioDescriptor, 0, NULL);
                /* ReceiveShortAudioDescriptor(RxContent); */
             }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ReportShortAudioDescriptor");
            }
            break;

        case AvEventCecMsgReportPhyAddr:
            /* from RxMsg AV_CEC_MSG_REPORT_PHYS_ADDR */
            if (AV_CEC_BROADCAST(RxContent))
            {
                ActiveSource = (((uint16)(RxContent[2])<<8)+
                                (uint16)(RxContent[3]));
                if(AV_CEC_SRC(RxContent) == AV_CEC_AUDIO_LOG_ADDRESS)
                    CecTxAudioStatus.ActiveSource = ActiveSource;
                /*
                param = (uint32)ActiveSource;
                AvHandleEvent(port, AvEventCecSendActiveSourceToAudio, &param, NULL);
                */
                /* SendActiveSourceToAudio(ActiveSource); */
                AvKapiOutputDebugMessage("CEC: source %x reports phy addr",ActiveSource);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore ReportPhyAddr");
            }
            break;

        case AvEventCecMsgMenuRequest:
            /* from RxMsg AV_CEC_MSG_MENU_REQUEST */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Receive menu request %x", RxContent[2]);
                AvHandleEvent(port, AvEventCecSendMenuStatus, NULL, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore MenuRequest");
            }
            break;

        case AvEventCecMsgRequestActiveSource:
            /* from RxMsg AV_CEC_MSG_REQ_ACTIVE_SRC */
            if((AV_CEC_BROADCAST(RxContent)) &&
               (port->content.cec->LogAddr != AV_CEC_TV_LOG_ADDRESS))
            {
                /* Response only when CEC switch is applicable */
                if(port->device->type == Gsv6k5)
                    AvHandleEvent(port, AvEventCecSendActiveSource, NULL, NULL);
            }
            else
            {
                 AvKapiOutputDebugMessage("CEC: ignore RequestActiveSource");
            }
            break;

        case AvEventCecMsgSetMenuLanguage:
            /* from RxMsg AV_CEC_MSG_SET_MENU_LANGUAGE */
            if(AV_CEC_BROADCAST(RxContent)&&
               (port->content.cec->LogAddr == AV_CEC_TV_LOG_ADDRESS))
            {
                AvKapiOutputDebugMessage("CEC: Set menu language");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore SetMenuLanguage");
            }
            break;

        case AvEventCecMsgDeviceVendorId:
            AvKapiOutputDebugMessage("CEC: ignore DeviceVendorId");
            break;

#endif

        default:
            ret = AvNotAvailable;
            break;
    }

    return ret;
}

#if AvEnableCecFeature /* CEC Related */
uint16 AvCecFindRequiredInput (AvPort *port, uint16 SrcPhys)
{
    /* PhyAddr order is fixed */
    uint16 i;
    uint8  shift = 12;
    uint16 Mask=0xf000, Mask2=0;

    for (i=0; i<4; i++)
    {
        if ((port->content.tx->PhyAddr & Mask) == 0)
        {
            break;
        }
        Mask2 |= Mask;
        Mask >>= 4;
        if(shift == 0)
            return (0xff);
        else
            shift = shift - 4;
    }

    if ((port->content.tx->PhyAddr & Mask2) == (SrcPhys & Mask2))
    {
        i = (SrcPhys>>shift) & 0x0f;
        return(i);
    }
    return (0xff);
}
#endif /* CEC Related */

AvRet AvEarcCapDataManage(AvPort *port)
{
#if AvEnableEArcRxFeature
    /* 1. prepare EARC Capability Data, reuse InEdid,SinkEdid,OutEdid */
    AvMemset(InEdid, 0, 256);
    AvMemset(SinkEdid, 0, 256);
    AvMemset(OutEdid, 0, 256);
    AvEarcFuncCapDataProcess(&DevEarcReg, InEdid, SinkEdid, OutEdid);
    /* 2. write EARC Capability Data */
    KfunEarcRxWriteCapData(port, OutEdid);
    port->content.earx->CapStat = AV_EARCRX_CAP_NEEDUPDATE;
#endif
    return AvOk;
}

AvRet AvEdidPortManage(AvPort *RxPort)
{
    uint8 SpaLocation = 0;
    uint8 SpaValue[2];
    uint8 CheckSum[2] = {0x00,0x00};
    uint8 SinkNumber = 0; /* default to force mode */
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;

    /* 1. Clear RAM header, save time */
    AvMemset(InEdid, 0, AvEdidMaxSize);
    AvMemset(SinkEdid, 0, AvEdidMaxSize);
    AvMemset(OutEdid, 0, AvEdidMaxSize);
    /* 1.1 Set Edid Parameter */
    /* disable AvEdidSupportFreq for manual Uart Tweak */
#if (AvEnableUartInput == 0)
    AvEdidSupportFreq = //AvEdidBitFreq12G4Lane  |
                        AvEdidBitFreq10G4Lane  |
                        AvEdidBitFreq8G4Lane  |
                        AvEdidBitFreq6G4Lane  |
                        AvEdidBitFreq6G    |
                        AvEdidBitFreq4P5G  |
                        AvEdidBitFreq3P75G |
                        AvEdidBitFreq3G    |
                        AvEdidBitFreq2P25G |
                        AvEdidBitFreq1P5G  |
                        AvEdidBitFreq750M  |
                        AvEdidBitFreq270M  |
                        AvEdidBitFreq135M;
#endif
    /* AvEdidCeaParamForce = AvEdidBitCeaSVD | AvEdidBitCeaVSDBHF to allow EDID re-configuration */
    AvEdidCeaParamForce  =  AvEdidBitCeaSVD | AvEdidBitCeaVSDBHF;
    /* AvEdidCeabParamForce = AvEdidBitCeabY420CMDB | AvEdidBitCeabY420VDB to allow EDID re-configuration */
    AvEdidCeabParamForce =  AvEdidBitCeabY420CMDB | AvEdidBitCeabY420VDB | AvEdidBitCeabHDR_STATIC;
    AvEdidCeaParamRemove =  0;

    /* 2. Look up front port */
    CurrentPort = NULL;
    PrevPort = NULL;
    /* loop to the end of the same level connections */
    /* Merge every 2 port Edids a time */
    while(KfunFindVideoNextTxEnd(RxPort, &PrevPort, &CurrentPort) == AvOk)
    {
        if(CurrentPort->type == HdmiTx)
        {
            if((CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED) &&
               (CurrentPort->content.tx->IgnoreEdidError == 0))
            {
                if(SinkNumber != 4)
                    SinkNumber = SinkNumber + 1;
                AvKapiOutputDebugMessage("Port[%d][%d] Edid Manage: Read Port[%d][%d] EDID",RxPort->device->index,RxPort->index, CurrentPort->device->index, CurrentPort->index);
                /* find the next Edid Ram Ptr */
                if(SinkNumber == 1)
                {
                    KfunTxReadEdid(CurrentPort,InEdid);
                    CheckSum[0] = InEdid[127];
                    CheckSum[1] = InEdid[255];
                }
                else
                {
                    KfunTxReadEdid(CurrentPort,SinkEdid);
                    CheckSum[0] = SinkEdid[0];
                    CheckSum[1] = SinkEdid[1];
                    /* Merge Edids */
                    if((CheckSum[0] == InEdid[127]) && (CheckSum[1] == InEdid[255]))
                        SinkNumber = 1;
                    else
                    {
                        AvEdidFuncStructInit(&DevEdidReg);
                        AvMemset(OutEdid, 0, 256);
                        AvEdidFuncVesaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
                        AvEdidFuncCeaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
                        AvMemcpy(InEdid,OutEdid,256);
                        AvMemset(SinkEdid, 0, 256);
                    }
                }
            }
            else
            {
                CurrentPort->content.tx->EdidCks[0] = 0xFF;
                CurrentPort->content.tx->EdidCks[1] = 0xFF;
            }
        }
        PrevPort = CurrentPort;
    }

    /* Merge Edids */
    if(SinkNumber == 0)
    {
        AvEdidFuncStructInit(&DevEdidReg);
        AvEdidFuncVesaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
        AvEdidFuncCeaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
    }
    else if(SinkNumber == 1)
    {
        AvMemcpy(OutEdid+0,InEdid+0,AvEdidMaxSize);
    }
#if (Gsv6k5MuxMode == 0)
    else
        AvEdidDscFilter(OutEdid);
#endif

    /* Post Processing of HDMI 2.1 content */
    AvEdidFreeSyncFilter(OutEdid);

    /* 3. Final Merge Process */
#ifdef AddrManualEdidCmd
    if(RegMailboxEdidRxAValid == 1)
        AvMemcpy(OutEdid+0,RegMailboxEdidRxA+0,AvEdidMaxSize);
#endif
    //AvMemcpy(OutEdid+0,(void*)(&AvDefaultEDID[0]),256);
    AvEdidFuncCheckSum(OutEdid);

    /* 4. Find Spa Location and generate its own SPA for source */
    SpaLocation = KfunFindCecSPAFromEdid(OutEdid, SpaValue);
    KfunGenerateSourceSpa(RxPort, SpaValue, 0);

    uint8 i=0;
    AvKapiOutputDebugMessage("RxP[%d][%d] Generated Edid Start: ",RxPort->device->index,RxPort->index);
    for(i=0;i<16;i++)
    {
        AvKapiOutputDebugMessage(" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                                 OutEdid[i*16+0],OutEdid[i*16+1],OutEdid[i*16+2],OutEdid[i*16+3],
                                 OutEdid[i*16+4],OutEdid[i*16+5],OutEdid[i*16+6],OutEdid[i*16+7],
                                 OutEdid[i*16+8],OutEdid[i*16+9],OutEdid[i*16+10],OutEdid[i*16+11],
                                 OutEdid[i*16+12],OutEdid[i*16+13],OutEdid[i*16+14],OutEdid[i*16+15]);
    }

    /* 5. write Edid, set rx->EdidStatus to AV_EDID_NEEDUPDATE */
    KfunRxWriteEdid(RxPort, OutEdid, SpaLocation, SpaValue);
    RxPort->content.rx->EdidStatus = AV_EDID_UPDATED;

    return AvOk;
}

AvRet AvEdidPortAnalysis(AvPort *port)
{
    uint8 i = 0;
    uint8 round = 16;
    KfunTxReadEdid(port,InEdid);
    AvEdidFuncStructInit(&DevEdidReg);
    AvEdidFunFullAnalysis(&DevEdidReg,InEdid);

    if(InEdid[126] == 0x02)
        round = 24;
    else if(InEdid[126] == 0x03)
        round = 32;
    AvKapiOutputDebugMessage("Tx[%d][%d] Read Edid Start: ",port->device->index,port->index);
    for(i=0;i<round;i++)
    {
        AvKapiOutputDebugMessage(" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                                 InEdid[i*16+0], InEdid[i*16+1], InEdid[i*16+2], InEdid[i*16+3],
                                 InEdid[i*16+4], InEdid[i*16+5], InEdid[i*16+6], InEdid[i*16+7],
                                 InEdid[i*16+8], InEdid[i*16+9], InEdid[i*16+10],InEdid[i*16+11],
                                 InEdid[i*16+12],InEdid[i*16+13],InEdid[i*16+14],InEdid[i*16+15]);
    }

    if(DevEdidReg.MaxCharRate == 0)
        DevEdidReg.MaxCharRate = DevEdidReg.VesaMaxClk;
    if(DevEdidReg.MaxTmdsClk == 0)
    {
        DevEdidReg.MaxTmdsClk  = DevEdidReg.VesaMaxClk;
        DevEdidReg.MaxCharRate = DevEdidReg.VesaMaxClk;
    }
    /* Feature Support */
    port->content.tx->EdidSupportFeature = AV_BIT_FEAT_1G5;
    if((DevEdidReg.VsdbHfCheck[3] == 1) || (DevEdidReg.MaxTmdsClk > 340) || (DevEdidReg.MaxCharRate > 340) || (DevEdidReg.VesaMaxClk > 340))
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_SCDC;
    if(DevEdidReg.VsdbHfCheck[8] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_RR;
    if(DevEdidReg.VsdbHfCheck[7] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_B340MSCR;
    if(DevEdidReg.VsdbHfCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_36B420;
    if(DevEdidReg.VsdbHfCheck[2] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_30B420;
    if(DevEdidReg.MaxCharRate >= 600)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_6G;
    if(DevEdidReg.MaxCharRate >= 450)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4G5;
    if(DevEdidReg.MaxCharRate >= 370)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_3G75;
    if((DevEdidReg.MaxTmdsClk >= 300) &&
       ((DevEdidReg.VsdbCheckList[4] == 1) ||
        (DevEdidReg.VsdbCheckList[5] == 1) ||
        (DevEdidReg.VsdbCheckList[6] == 1) ||
        (DevEdidReg.VsdbCheckList[7] == 1)))
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_3G;
    if(DevEdidReg.MaxTmdsClk >= 225)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_2G25;
    if((DevEdidReg.HdrStCheck[0] != 0) ||
       (DevEdidReg.HdrStCheck[1] != 0) ||
       (DevEdidReg.HdrStCheck[2] != 0) ||
       (DevEdidReg.HdrStCheck[3] != 0))
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_HDR;
    if(DevEdidReg.Y420VdbCheck[0] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4K50_420;
    if(DevEdidReg.Y420VdbCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4K60_420;
    if(DevEdidReg.Y420VdbCheck[2] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4KS50_420;
    if(DevEdidReg.Y420VdbCheck[3] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4KS60_420;
    if(DevEdidReg.CdbCheck[0] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YUV_601;
    if(DevEdidReg.CdbCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YUV_709;
    if((DevEdidReg.CdbCheck[5] == 1) || (DevEdidReg.CdbCheck[6] == 1))
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YCC_2020;
    if(DevEdidReg.CdbCheck[7] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_RGB_2020;
    if(DevEdidReg.VcdbCheck[0] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YUV_FULL_RANGE;
    if(DevEdidReg.VcdbCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_RGB_FULL_RANGE;
    if(DevEdidReg.VsdbCheckList[11] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_10B_DEEP_COLOR;
    if(DevEdidReg.VsdbCheckList[10] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_12B_DEEP_COLOR;
    if((DevEdidReg.FreeSyncCheck[0] == 0x08) && (DevEdidReg.FreeSyncCheck[1]))
    {
        port->content.tx->EdidExtendFeature = port->content.tx->EdidExtendFeature | AV_BIT_DPFT_FREESYNC_TIER1;
        port->content.tx->FreeSyncRange = (((uint16)DevEdidReg.FreeSyncCheck[2] << 8) | DevEdidReg.FreeSyncCheck[3]);
    }
    else if((DevEdidReg.FreeSyncCheck[0] == 0x0A) && (DevEdidReg.FreeSyncCheck[1]))
    {
        port->content.tx->EdidExtendFeature = port->content.tx->EdidExtendFeature | AV_BIT_DPFT_FREESYNC_PRIME;
        port->content.tx->FreeSyncRange = (((uint16)DevEdidReg.FreeSyncCheck[2] << 8) | DevEdidReg.FreeSyncCheck[3]);
    }

    for(i=1;i<14;i++)
    {
        if(DevEdidReg.EdidCeaAudioCheck[i] == 1)
        {
            port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_COMPRESS_AUDIO;
            break;
        }
    }
    /* H2p1 Support */
    port->content.tx->H2p1SupportFeature = 0;
    if(DevEdidReg.MaxFrlRate >= 6)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FRL_12G4;
    if(DevEdidReg.MaxFrlRate >= 5)
    {
    #if AvEnableFrl48GSupport
        DevEdidReg.MaxFrlRate = 6;
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FRL_12G4;
    #endif
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FRL_10G4;
    }
    if(DevEdidReg.MaxFrlRate >= 4)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FRL_8G4;
    if(DevEdidReg.MaxFrlRate >= 3)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FRL_6G4;
    if(DevEdidReg.MaxFrlRate >= 2)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FRL_6G3;
    if(DevEdidReg.MaxFrlRate >= 1)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FRL_3G3;
    if(DevEdidReg.HfVrrMax != DevEdidReg.HfVrrMin)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_VRR;
    if(DevEdidReg.VsdbHfCheck[12] == 1)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_FVA;
    if(DevEdidReg.VsdbHfCheck[15] == 1)
        port->content.tx->H2p1SupportFeature = port->content.tx->H2p1SupportFeature | AV_BIT_H2P1_DSC;
    /* FRL Type:
       AV_FRL_12G4L/AV_FRL_10G4L/AV_FRL_8G4L/AV_FRL_6G4L/AV_FRL_6G3L/AV_FRL_3G3L/AV_FRL_NONE */
    if(port->content.tx->H2p1FrlManual == 0)
    {
        #if AvEnableFrl48GSupport
        if(DevEdidReg.MaxFrlRate >= 6)
            port->content.tx->H2p1FrlType = AV_FRL_12G4L;
        else if(DevEdidReg.MaxFrlRate >= 5)
        #else
        if(DevEdidReg.MaxFrlRate >= 5)
        #endif
            port->content.tx->H2p1FrlType = AV_FRL_10G4L; /* highest FRL rate */
        else if(DevEdidReg.MaxFrlRate >= 4)
            port->content.tx->H2p1FrlType = AV_FRL_8G4L;
        else if(DevEdidReg.MaxFrlRate >= 3)
            port->content.tx->H2p1FrlType = AV_FRL_6G4L;
        else if(DevEdidReg.MaxFrlRate >= 2)
            port->content.tx->H2p1FrlType = AV_FRL_6G3L;
        else
            port->content.tx->H2p1FrlType = AV_FRL_NONE;
    }

    return AvOk;
}

void AvEdidMaxFrlFilter(uint8 *EdidContent, uint8 Max)
{
    uint16  iCeabParam = 0;
    uint8   iEdidOffset = 0;
    uint8   iEdidLen    = 0;
    uint8   value       = 0;

    iCeabParam    = AvEdidFuncFindCeabTag(EdidContent,   0x03, 1);
    iEdidOffset   = (iCeabParam>>8)&0xff;
    iEdidLen      = (iCeabParam>>0)&0xff;

    if(iEdidOffset != 0)
    {
        if(iEdidLen >= 7)
        {
            value = (EdidContent[iEdidOffset+7]>>4) & 0x0F;
            if(value > Max)
                EdidContent[iEdidOffset+7] = (Max<<4) | (EdidContent[iEdidOffset+7] & 0x0F);
        }
        if(iEdidLen >= 12)
        {
            value = (EdidContent[iEdidOffset+7]>>4) & 0x0F;
            if(value > Max)
                EdidContent[iEdidOffset+12] = (Max<<4) | (EdidContent[iEdidOffset+12] & 0x0F);
        }
    }

    return;
}

void AvEdidDscFilter(uint8 *EdidContent)
{
    uint16  iCeabParam = 0;
    uint8   iEdidOffset = 0;
    uint8   iEdidLen    = 0;

    iCeabParam    = AvEdidFuncFindCeabTag(EdidContent,   0x03, 1);
    iEdidOffset   = (iCeabParam>>8)&0xff;
    iEdidLen      = (iCeabParam>>0)&0xff;

    if(iEdidOffset != 0)
    {
        if(iEdidLen >= 11)
            EdidContent[iEdidOffset+11] = 0x00;
        if(iEdidLen >= 12)
            EdidContent[iEdidOffset+12] = 0x00;
        if(iEdidLen >= 13)
            EdidContent[iEdidOffset+13] = 0x00;
    }

    return;
}

void AvEdidFreeSyncFilter(uint8 *EdidContent)
{
    uint16  iCeabParam = 0;
    uint8   iEdidOffset = 0;
    uint8   iEdidLen    = 0;

    iCeabParam    = AvEdidFuncFindCeabTag(EdidContent,   0x03, 2);
    iEdidOffset   = (iCeabParam>>8)&0xff;
    iEdidLen      = (iCeabParam>>0)&0xff;

    if((iEdidOffset != 0) && (iEdidLen >= 9))
    {
        if(EdidContent[iEdidOffset+4] == 0x02)
            EdidContent[iEdidOffset+8] = 0x00;
    }
    return;
}

RouteStat AvPortRoutingPolicy(AvPort *RxPort, AvPort *TxPort)
{
    RouteStat TxConnectStyle = ROUTE_NO_CONNECT;
    /* 0.1 Check RxPort is valid output */
    if(RxPort->type != HdmiRx)
        return ROUTE_NO_CONNECT;
    /* 0.2 Check TxPort is valid output */
    if(TxPort->type == HdmiTx)
    {
        if((TxPort->content.tx->EdidReadSuccess != AV_EDID_UPDATED) ||
           (TxPort->content.tx->Hpd != AV_HPD_HIGH))
            return ROUTE_NO_CONNECT;
    }
    else
    {
        return ROUTE_NO_CONNECT;
    }

    /* 1. FRL Routing Management */
    /* 1.1 FRL6 48G Tx Port*/
    if(TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_12G4)
    {
        TxConnectStyle = ROUTE_1_R_T;
    }
    /* 1.2 FRL5 40G Tx Port */
    else if(TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_10G4)
    {
        /* 1.2.1 Rx is 1700 above, only FRL6 can support, 420 */
        if(RxPort->content.video->info.TmdsFreq >= 1700)
        {
            if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
                TxConnectStyle = ROUTE_3_R_S_T;
            else
                TxConnectStyle = ROUTE_2_R_C_T;
        }
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* 1.3 FRL4 32G Tx Port */
    else if(TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_8G4)
    {
        /* 1.3.1 Rx is 1400 above, only FRL5/6 can support, 420 */
        if(RxPort->content.video->info.TmdsFreq >= 1400)
        {
            if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
                TxConnectStyle = ROUTE_3_R_S_T;
            else
                TxConnectStyle = ROUTE_2_R_C_T;
        }
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* 1.4 FRL3 24G Tx Port */
    else if(TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_6G4)
    {
        /* 1.4.1 Rx is 1700 above, only FRL6 can support, downscale */
        if(RxPort->content.video->info.TmdsFreq >= 1700)
            TxConnectStyle = ROUTE_3_R_S_T;
        /* 1.4.2 Rx is 1100 above, only FRL4/5/6 can support, 420 */
        else if(RxPort->content.video->info.TmdsFreq >= 1100)
        {
            if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
                TxConnectStyle = ROUTE_3_R_S_T;
            else
                TxConnectStyle = ROUTE_2_R_C_T;
        }
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* 1.5 FRL3 18G Tx Port */
    else if(TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_6G3)
    {
        /* 1.5.1 Rx is 1400 above, only FRL5/6 can support, downscale */
        if(RxPort->content.video->info.TmdsFreq >= 1400)
            TxConnectStyle = ROUTE_3_R_S_T;
        /* 1.5.2 Rx is 1100 above, only FRL3/4/5/6 can support, 420 */
        else if(RxPort->content.video->info.TmdsFreq >= 700)
        {
            if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
                TxConnectStyle = ROUTE_3_R_S_T;
            else
                TxConnectStyle = ROUTE_2_R_C_T;
        }
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* HDMI 2.0 management */
    /* 2.1 6G Tx Port */
    else if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_SCDC) && /* support SCDC */
            (TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_6G))     /* support 6G */
    {
        /* 2.1.1 Rx is 630 above, only FRL can support, downscale */
        if(RxPort->content.video->info.TmdsFreq >= 630)
            TxConnectStyle = ROUTE_3_R_S_T;
#if AvEnableDetailTiming
        else if((RxPort->content.video->timing.VActive > 2160) ||
                ((RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420) && (RxPort->content.video->timing.HActive > 2048)) ||
                ((RxPort->content.video->Y != AV_Y2Y1Y0_YCBCR_420) && (RxPort->content.video->timing.HActive > 4096)))
            TxConnectStyle = ROUTE_3_R_S_T;
#endif
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* 2.2 3G Tx Port */
    else if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_3G)) /* support 3G */
    {
        /* 2.1.1 Rx is 630 above, only FRL can support, downscale */
        if(RxPort->content.video->info.TmdsFreq >= 630)
            TxConnectStyle = ROUTE_3_R_S_T;
        /* 2.2.2 Rx is 6G, go into vsp, if no 420, then downscale */
        else if(RxPort->content.video->info.TmdsFreq >= 580)
        {
            if(RxPort->content.video->InCs == AV_CS_LIM_BT2020_RGB)
                TxConnectStyle = ROUTE_3_R_S_T;
            else
            {
                switch(RxPort->content.video->timing.Vic)
                {
                    case 96:  /* 4K50 */
                    case 106:
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K50_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 97:  /* 4K60 */
                    case 107:
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K60_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 101: /* 4KS50 */
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS50_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 102: /* 4KS60 */
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS60_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 0:
                        TxConnectStyle = ROUTE_NO_CONNECT;
                        break;
                    default: /* down scale to output */
                        TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                }
            }
        }
        /* 2.2.3 Rx is 3G */
        else if(RxPort->content.video->info.TmdsFreq >= 290)
        {
            /* 2.2.3.1 bypass 420 if support, or else, downscale */
            if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
            {
                /* 2.2.3.1.1 only do 420 if support format */
                if((RxPort->content.video->InCs == AV_CS_YUV_601) ||
                   (RxPort->content.video->InCs == AV_CS_YUV_709) ||
                   (RxPort->content.video->InCs == AV_CS_YCC_601) ||
                   (RxPort->content.video->InCs == AV_CS_YCC_709) ||
                   (RxPort->content.video->InCs == AV_CS_SYCC_601) ||
                   (RxPort->content.video->InCs == AV_CS_ADOBE_YCC_601) ||
                   (RxPort->content.video->InCs == AV_CS_BT2020_YCC) ||
                   (RxPort->content.video->InCs == AV_CS_BT2020_RGB) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_BT2020_YCC) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_BT2020_RGB) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_YUV_601) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_YUV_709) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_YCC_601) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_YCC_709) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_SYCC_601) ||
                   (RxPort->content.video->InCs == AV_CS_LIM_ADOBE_YCC_601))
                {
                    switch(RxPort->content.video->timing.Vic)
                    {
                        case 96:  /* 4K50 */
                        case 106:
                            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K50_420) &&
                               (RxPort->content.video->Cd == AV_CD_24))
                                TxConnectStyle = ROUTE_1_R_T;
                            else
                                TxConnectStyle = ROUTE_3_R_S_T;
                            break;
                        case 97:  /* 4K60 */
                        case 107:
                            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K60_420) &&
                               (RxPort->content.video->Cd == AV_CD_24))
                                TxConnectStyle = ROUTE_1_R_T;
                            else
                                TxConnectStyle = ROUTE_3_R_S_T;
                            break;
                        case 101: /* 4KS50 */
                            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS50_420) &&
                               (RxPort->content.video->Cd == AV_CD_24))
                                TxConnectStyle = ROUTE_1_R_T;
                            else
                                TxConnectStyle = ROUTE_3_R_S_T;
                            break;
                        case 102: /* 4KS60 */
                            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS60_420) &&
                               (RxPort->content.video->Cd == AV_CD_24))
                                TxConnectStyle = ROUTE_1_R_T;
                            else
                                TxConnectStyle = ROUTE_3_R_S_T;
                            break;
                        case 0:
                            TxConnectStyle = ROUTE_NO_CONNECT;
                            break;
                        default: /* down scale to output */
                            TxConnectStyle = ROUTE_3_R_S_T;
                            break;
                    }
                }
                /* 2.2.3.1.2 still bypass it */
                else
                    TxConnectStyle = ROUTE_1_R_T;
            }
            else
                TxConnectStyle = ROUTE_1_R_T;
        }
        /* 2.2.4 Rx is below 3G */
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* 2.3 1G5 Tx Port */
    else if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_1G5)) /* support 1G5 */
    {
        /* 2.3.1 Rx is 6G, go into vsp, downscale */
        if(RxPort->content.video->info.TmdsFreq >= 290)
            TxConnectStyle = ROUTE_3_R_S_T;
        /* 2.3.2 Rx is below 3G */
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* 2.4 Don't care, direct output */
    else
        TxConnectStyle = ROUTE_1_R_T;
    /* 3. FRL Rate limit */
    if(TxConnectStyle == ROUTE_1_R_T)
    {
        if((((TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_12G4) == 0) && (RxPort->content.rx->H2p1FrlType == AV_FRL_12G4L)) ||
           (((TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_10G4) == 0) && (RxPort->content.rx->H2p1FrlType == AV_FRL_10G4L)) ||
           (((TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_8G4) == 0)  && (RxPort->content.rx->H2p1FrlType == AV_FRL_8G4L))  ||
           (((TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_6G4) == 0)  && (RxPort->content.rx->H2p1FrlType == AV_FRL_6G4L))  ||
           (((TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_6G3) == 0)  && (RxPort->content.rx->H2p1FrlType == AV_FRL_6G3L))  ||
           (((TxPort->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_3G3) == 0)  && (RxPort->content.rx->H2p1FrlType == AV_FRL_3G3L)))
            TxConnectStyle = ROUTE_3_R_S_T;
    }

    return TxConnectStyle;
}

uint32 AvPortDownscalePolicy(AvPort *RxPort, AvPort *ScalerPort, AvPort *TxPort)
{
    uint8 Vratio = 4;
    uint8 Hratio = 4;
    uint32 result = 0;

#if AvEnableDetailTiming
    uint16 HActive = RxPort->content.video->timing.HActive;
    uint16 Hmax = 4096;
    uint16 Vmax = 2160;
    uint16 value = 0;
    uint16 residue = 0;
    uint8 div = 1;
    /* 1. Judge Max Support Resolution */
    if(TxPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED)
    {
        if((RxPort->content.video->timing.FrameRate > 61) || (RxPort->content.video->VrrStream == 1))
        {
            Hmax = 2048;
            Vmax = 1080;
        }
        else if(RxPort->content.video->timing.FrameRate > 31)
        {
            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_6G) == 0)
            {
                Hmax = 2048;
                Vmax = 1080;
            }
        }
        else
        {
            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_3G) == 0)
            {
                Hmax = 2048;
                Vmax = 1080;
            }
        }
    }
    /* 1.1 HActive Alignment for 420 input */
    if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
        HActive = HActive << 1;
    /* 2. Exceptions on fixed cases */
    /* 2.1 10240x4320(10K) */
    if(HActive >= 10240)
    {
        /* 2560x1440 for HDMI 2.0 */
        if(Hmax == 4096)
        {
            Hratio = 4;
            Vratio = 3;
        }
        /* 1280x1080 for HDMI 1.4 */
        else
        {
            Hratio = 8;
            Vratio = 4;
        }
    }
    /* 2.2 7680x4320(8K) */
    else if(HActive >= 7680)
    {
        /* 3840x2160 for HDMI 2.0 */
        if(Hmax == 4096)
        {
            Hratio = 2;
            Vratio = 2;
        }
        /* 1920x1080 for HDMI 1.4 */
        else
        {
            Hratio = 4;
            Vratio = 4;
        }
    }
    /* 2.3 5120x2160(5K) */
    else if(HActive >= 5120)
    {
        /* 2560x1080 for HDMI 2.0 */
        if(Hmax == 4096)
        {
            Hratio = 2;
            Vratio = 2;
        }
        /* 1280x720 for HDMI 1.4 */
        else
        {
            Hratio = 4;
            Vratio = 3;
        }
    }
    else
    {
        /* 3.1 Calculate Horizontal Downscale Ratio */
        value = Hmax;
        div = 1;
        residue = 0;
        while((HActive > value) || (residue != 0))
        {
            if(div >= 32)
                break;
            else if(div >= 4)
                div = div <<1;
            else
                div = div + 1;
            value = Hmax*div;
            residue = HActive % div;
        }
        Hratio = div;
        /* 3.2 Calculate Vertical Downscale Ratio */
        value = Vmax;
        div = 1;
        residue = 0;
        while((RxPort->content.video->timing.VActive > value) || (residue != 0))
        {
            if(div >= 4)
                break;
            else
                div = div + 1;
            value = Vmax*div;
            residue = RxPort->content.video->timing.VActive % div;
        }
        Vratio = div;
        /* 3.3 HRatio/VRatio Alignment */
        if(Vratio > Hratio)
            Vratio = Hratio;
        else
            Hratio = Vratio;
    }
#endif
    result = (Vratio<<8) + Hratio;
    return result;
}

RouteStat AvPortRoutingMap(AvPort *RxPort, AvPort *TxPort, AvPort *ScalerPort, AvPort *ColorPort)
{
    RouteStat TxCurrentStyle = ROUTE_NO_CONNECT;
    if(TxPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
        TxCurrentStyle = ROUTE_1_R_T;
    else if(TxPort->content.RouteVideoFromPort == (struct AvPort*)ColorPort)
    {
        if(ColorPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
            TxCurrentStyle = ROUTE_2_R_C_T;
        else
            TxCurrentStyle = ROUTE_NO_CONNECT;
    }
    else if(TxPort->content.RouteVideoFromPort == (struct AvPort*)ScalerPort)
    {
        if(ScalerPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
            TxCurrentStyle = ROUTE_3_R_S_T;
        else
            TxCurrentStyle = ROUTE_NO_CONNECT;
    }
    else
        TxCurrentStyle = ROUTE_NO_CONNECT;
    return TxCurrentStyle;
}

void AvPortSetRouting(AvPort *RxPort, AvPort *TxPort, AvPort *ColorPort, AvPort *ScalerPort, uint8 TxConnectStyle)
{
    uint8 MessageFlag = 0;
    switch(TxConnectStyle)
    {
        case ROUTE_1_R_T:
            if(TxPort->content.RouteVideoFromPort != (struct AvPort*)RxPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(RxPort,    TxPort,     AvConnectVideo);
            }
            if(MessageFlag == 1)
                AvKapiOutputDebugMessage("New Route: Rx[%d][%d]-Tx[%d][%d]",RxPort->device->index,RxPort->index, TxPort->device->index, TxPort->index);
            break;
        case ROUTE_2_R_C_T:
            if(ColorPort->content.RouteVideoFromPort != (struct AvPort*)RxPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(RxPort,    ColorPort,  AvConnectVideo);
            }
            if(TxPort->content.RouteVideoFromPort != (struct AvPort*)ColorPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(ColorPort, TxPort,     AvConnectVideo);
            }
            if(MessageFlag == 1)
                AvKapiOutputDebugMessage("New Route: Rx[%d][%d]-Color-Tx[%d][%d]",RxPort->device->index,RxPort->index, TxPort->device->index, TxPort->index);
            break;
        case ROUTE_3_R_S_T:
            if(ScalerPort->content.RouteVideoFromPort != (struct AvPort*)RxPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(RxPort,    ScalerPort, AvConnectVideo);
            }
            if(TxPort->content.RouteVideoFromPort != (struct AvPort*)ScalerPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(ScalerPort, TxPort,     AvConnectVideo);
            }
            if(MessageFlag == 1)
                AvKapiOutputDebugMessage("New Route: Rx[%d][%d]-Scaler-Tx[%d][%d]",RxPort->device->index,RxPort->index, TxPort->device->index, TxPort->index);
            break;
    }
}

AvRet AvPortConnectUpdate(AvDevice *Device)
{
    AvPort *TxAPort;
    AvPort *TxBPort;
    AvPort *TxCPort;
    AvPort *TxDPort;
    AvPort *RxPort;
    AvPort *ColorPort;
    AvPort *ScalerPort;
    /* 1-bypass, 2-color, 3-scale, 4-color-scale */
    RouteStat TxAConnectStyle = ROUTE_NO_CONNECT;
    RouteStat TxBConnectStyle = ROUTE_NO_CONNECT;
    RouteStat TxCConnectStyle = ROUTE_NO_CONNECT;
    RouteStat TxDConnectStyle = ROUTE_NO_CONNECT;
    /* 1-bypass, 2-color, 3-scale, 4-color-scale */
    RouteStat TxACurrentStyle = ROUTE_NO_CONNECT;
    RouteStat TxBCurrentStyle = ROUTE_NO_CONNECT;
    RouteStat TxCCurrentStyle = ROUTE_NO_CONNECT;
    RouteStat TxDCurrentStyle = ROUTE_NO_CONNECT;
    uint32 DownscalePlan = 0;
    uint8 DsVPlan = 0;
    uint8 DsHPlan = 0;
    uint8 DsVGoal = 0;
    uint8 DsHGoal = 0;

    /* 0. only process Gsv2k1 device */
    if(Device->type != Gsv6k5)
        return AvNotSupport;

    /* Decide the vsp management */
    /* 1. Prepare the ports */
    RxPort     = (AvPort*)Device->port;
    TxAPort    = &RxPort[1];
    TxBPort    = &RxPort[2];
    TxCPort    = &RxPort[3];
    TxDPort    = &RxPort[4];
    ColorPort  = &RxPort[6];
    ScalerPort = &RxPort[5];
    /* 1.1 Find Valid RxPort */
    /* 1.1.2 Check RxA is valid or not */
    if(RxPort->content.rx->IsInputStable != 1)
        return AvOk;
    /* 2. Find Feasible Routing Solution */
    if(TxAPort->type == HdmiTx)
        TxAConnectStyle = AvPortRoutingPolicy(RxPort, TxAPort);
    if(TxBPort->type == HdmiTx)
        TxBConnectStyle = AvPortRoutingPolicy(RxPort, TxBPort);
    if(TxCPort->type == HdmiTx)
        TxCConnectStyle = AvPortRoutingPolicy(RxPort, TxCPort);
    if(TxDPort->type == HdmiTx)
        TxDConnectStyle = AvPortRoutingPolicy(RxPort, TxDPort);

    /* 3. Find Tx Current Routing Map */
    TxACurrentStyle = AvPortRoutingMap(RxPort, TxAPort, ScalerPort, ColorPort);
    TxBCurrentStyle = AvPortRoutingMap(RxPort, TxBPort, ScalerPort, ColorPort);
    TxCCurrentStyle = AvPortRoutingMap(RxPort, TxCPort, ScalerPort, ColorPort);
    TxDCurrentStyle = AvPortRoutingMap(RxPort, TxDPort, ScalerPort, ColorPort);

    /* 4. New Plan for Connection */
    if((TxAConnectStyle == ROUTE_3_R_S_T) ||
       (TxBConnectStyle == ROUTE_3_R_S_T) ||
       (TxCConnectStyle == ROUTE_3_R_S_T) ||
       (TxDConnectStyle == ROUTE_3_R_S_T))
    {
        if(TxAConnectStyle == ROUTE_2_R_C_T)
            TxAConnectStyle = ROUTE_3_R_S_T;
        if(TxBConnectStyle == ROUTE_2_R_C_T)
            TxBConnectStyle = ROUTE_3_R_S_T;
        if(TxCConnectStyle == ROUTE_2_R_C_T)
            TxCConnectStyle = ROUTE_3_R_S_T;
        if(TxDConnectStyle == ROUTE_2_R_C_T)
            TxDConnectStyle = ROUTE_3_R_S_T;
        DsVGoal = 1;
        DsHGoal = 1;
        if(TxAConnectStyle == ROUTE_3_R_S_T)
        {
            DownscalePlan = AvPortDownscalePolicy(RxPort, ScalerPort, TxAPort);
            DsVGoal = DownscalePlan>>8;
            DsHGoal = DownscalePlan&0xFF;
        }
        if(TxBConnectStyle == ROUTE_3_R_S_T)
        {
            DownscalePlan = AvPortDownscalePolicy(RxPort, ScalerPort, TxBPort);
            DsVPlan = DownscalePlan>>8;
            DsHPlan = DownscalePlan&0xFF;
            if(DsVPlan > DsVGoal)
                DsVGoal = DsVPlan;
            if(DsHPlan > DsHGoal)
                DsHGoal = DsHPlan;
        }
        if(TxCConnectStyle == ROUTE_3_R_S_T)
        {
            DownscalePlan = AvPortDownscalePolicy(RxPort, ScalerPort, TxCPort);
            DsVPlan = DownscalePlan>>8;
            DsHPlan = DownscalePlan&0xFF;
            if(DsVPlan > DsVGoal)
                DsVGoal = DsVPlan;
            if(DsHPlan > DsHGoal)
                DsHGoal = DsHPlan;
        }
        if(TxDConnectStyle == ROUTE_3_R_S_T)
        {
            DownscalePlan = AvPortDownscalePolicy(RxPort, ScalerPort, TxDPort);
            DsVPlan = DownscalePlan>>8;
            DsHPlan = DownscalePlan&0xFF;
            if(DsVPlan > DsVGoal)
                DsVGoal = DsVPlan;
            if(DsHPlan > DsHGoal)
                DsHGoal = DsHPlan;
        }
        /*
        if((DsVGoal == 1) && (DsHGoal == 1))
        {
            DsVGoal = 2;
            DsHGoal = 2;
        }
        */
        if(ScalerPort->content.scaler->Vratio != DsVGoal)
        {
            ScalerPort->content.scaler->Vratio = DsVGoal;
            AvKapiOutputDebugMessage("Scaler:Vratio=%d",DsVGoal);
        }
        if(ScalerPort->content.scaler->Hratio != DsHGoal)
        {
            ScalerPort->content.scaler->Hratio = DsHGoal;
            AvKapiOutputDebugMessage("Scaler:Hratio=%d",DsHGoal);
        }
    }
    /* HDR to SDR Update */
    ColorPort->content.color->Enable = 1;
    if(((RxPort->content.video->AvailableVideoPackets & AV_BIT_HDR_PACKET) != 0) &&
       (ColorPort->content.color->Hdr2SdrEnable == 1))
    {
        if((TxAConnectStyle != ROUTE_3_R_S_T) && (TxBConnectStyle != ROUTE_3_R_S_T) &&
           (TxCConnectStyle != ROUTE_3_R_S_T) && (TxDConnectStyle != ROUTE_3_R_S_T))
        {
            if(((TxAPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_HDR) == 0) &&
               (TxAConnectStyle == ROUTE_1_R_T))
            {
                ColorPort->content.color->Enable = 0;
                TxAConnectStyle = ROUTE_2_R_C_T;
            }
            if(((TxBPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_HDR) == 0) &&
               (TxBConnectStyle == ROUTE_1_R_T))
            {
                ColorPort->content.color->Enable = 0;
                TxBConnectStyle = ROUTE_2_R_C_T;
            }
            if(((TxCPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_HDR) == 0) &&
               (TxCConnectStyle == ROUTE_1_R_T))
            {
                ColorPort->content.color->Enable = 0;
                TxCConnectStyle = ROUTE_2_R_C_T;
            }
            if(((TxDPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_HDR) == 0) &&
               (TxDConnectStyle == ROUTE_1_R_T))
            {
                ColorPort->content.color->Enable = 0;
                TxDConnectStyle = ROUTE_2_R_C_T;
            }
        }
    }

    /* 5. Action */
    if((TxACurrentStyle != TxAConnectStyle) && (TxAConnectStyle != ROUTE_NO_CONNECT))
        AvPortSetRouting(RxPort, TxAPort, ColorPort, ScalerPort, TxAConnectStyle);
    if((TxBCurrentStyle != TxBConnectStyle) && (TxBConnectStyle != ROUTE_NO_CONNECT))
        AvPortSetRouting(RxPort, TxBPort, ColorPort, ScalerPort, TxBConnectStyle);
    if((TxCCurrentStyle != TxCConnectStyle) && (TxCConnectStyle != ROUTE_NO_CONNECT))
        AvPortSetRouting(RxPort, TxCPort, ColorPort, ScalerPort, TxCConnectStyle);
    if((TxDCurrentStyle != TxDConnectStyle) && (TxDConnectStyle != ROUTE_NO_CONNECT))
        AvPortSetRouting(RxPort, TxDPort, ColorPort, ScalerPort, TxDConnectStyle);


    return AvOk;
}
