/**
 * @file kernel_fsm_update.h
 *
 * @brief functions for update FSMs in the kernel layer
 */

#include "kernel_status_update.h"
#include "kapi.h"
#include "uapi_function_mapper.h"
#include "uapi.h"


/* PART 1 */
/* Receiver FSM */

/* check RX FSM status */
void KfunCheckRxState(pin AvPort *port)
{
    /* if no new information, return state */
    switch (*port->content.is_ReceiverFsm)
    {
        case AvFsmRxDefault:
            break;
        case AvFsmRxDetect:
            {
                KfunPollingRxStatus(port);
                break;
            }
        case AvFsmRxFreerun:
            {
                KfunPollingRxStatus(port);
                break;
            }
        case AvFsmRxReceiving:
            {
                KfunPollingRxStatus(port);
                break;
            }
        case AvFsmRxReset:
            {
                KfunPollingRxStatus(port);
                break;
            }
        default:
            break;
    }

    return;
}

/* PART 2 */
/* HDCP FSM */
void KfunSimpleHdcpManage(AvPort *port)
{
    /* 1. Firstly Find whether source is encrypted or not */
    KfunHdcpDetectSource(port);
    /* 2. No matter source is encrypted or not, sink should also get encrypt info */
    KfunHdcpDetectSink(port);
    /* 3. Sync them */
    KfunSimpleHdcpSync(port);
    return;
}

/* PART 3 */
/* Rx Routing FSM */

/* PART 4 */
/* Tx Routing FSM */

/* PART 5 */
/* Plug RX FSM */

void KfunCheckPrState( AvPort *port)
{
    /* State detection based on FSM state */
    /* TMDS clock detection, not interrupt */
    /* HdmiInts.TmdsClkChg */
    /* cable detection, not interrupt */
    /* HdmiInts.CableDetect */
    /* Internal Mute detection*/
    /* HdmiInts.InternMute */
    /* Video PLL Lock */
    /* HdmiInts.VidPllChg */
    /* Check DE regen lock */
    /* HdmiInts.DeRegenLck */
    /* Vsync lock */
    /* HdmiInts.VsyncLck */
    /* Encryption state */
    /* HdmiInts.Encrypted */

    /* TMDS Frequency Change int*/
    /* HdmiInts.NewTmdsFreq */

    /* check routing, if not sink is connected, reset this port */
#if (AvAllowHpdLowEdidRead == 1)
    /* force to Reset = 0 */
    port->content.rx->Reset = 0;
#else
    if(port->core.HdmiCore != -1)
        port->content.rx->Reset = 0;
    else
        port->content.rx->Reset = 1;
#endif

    switch (*port->content.is_PlugRxFsm)
    {
        case AvFsmPlugRxDefault:
            break;
        case AvFsmPlugRxDetect:
        {
            AvUapiRxGetStatus(port);
            break;
        }
        case AvFsmPlugRxInfoUpdate:
            break;
        case AvFsmPlugRxInputLock:
        {
            AvUapiRxGetStatus(port);
            break;
        }
        case AvFsmPlugRxPlugged:
        {
            AvUapiRxGetStatus(port);
            break;
        }
        case AvFsmPlugRxPullDownHpd:
        {
            AvUapiRxGetStatus(port);
            break;
        }
        case AvFsmPlugRxReadTiming:
            break;
        case AvFsmPlugRxReset:
        {
            AvUapiRxGetStatus(port);
            break;
        }
        case AvFsmPlugRxStable:
        {
            AvUapiRxGetStatus(port);
            break;
        }
        default:
            break;
    }
}

/* PART 6 */
/* Plug TX FSM */

void KfunCheckPtState( AvPort *port)
{
    /* HPD Interrupt */
    /* Int.Hpd */
    /* MSEN state */
    /* Int.MonSen */
    /* EDID Ready Interrupt */
    /* Int.EdidReady*/
    /* Vsync Interrupt */
    /* Int.Vsync */

    switch (*port->content.is_PlugTxFsm)
    {
        case AvFsmPlugTxEdidError:
        {
            AvUapiTxGetStatus(port);
            break;
        }
        case AvFsmPlugTxDefault:
            break;
        case AvFsmPlugTxDefaultEdid:
            break;
        case AvFsmPlugTxEdid:
        {
            AvUapiTxGetStatus(port);
            /* AvUapiTxEdidStat(port); */
            break;
        }
        case AvFsmPlugTxEdidManage:
        {
            AvUapiTxGetStatus(port);
            /* AvUapiTxEdidStat(port); */
            break;
        }
        case AvFsmPlugTxHdcp:
        {
            AvUapiTxGetStatus(port);
            AvUapiTxGetHdcpStatus(port);
            break;
        }
        case AvFsmPlugTxHpdAntiDither:
        {
            AvUapiTxGetStatus(port);
            break;
        }
        case AvFsmPlugTxReset:
        {
            AvUapiTxGetStatus(port);
            break;
        }
        case AvFsmPlugTxStable:
        {
            AvUapiTxGetStatus(port);
            AvUapiTxGetHdcpStatus(port);
            break;
        }
        case AvFsmPlugTxTransmitVideo:
        {
            if(port->content.tx->InfoReady == 0)
            {
                if(port->content.tx->H2p1FrlExtraDelay == 0)
                    AvHandleEvent(port, AvEventPortDownStreamSending, NULL, NULL);
                /*
                else if(port->content.tx->H2p1FrlExtraDelay == TxHdcpManageThreshold)
                    AvHandleEvent(port, AvEventPortDownStreamPowerOff, NULL, NULL);
                else if(port->content.tx->H2p1FrlExtraDelay == TxVideoManageThreshold)
                    AvHandleEvent(port, AvEventPortDownStreamPowerOn, NULL, NULL);
                */
            }
            AvUapiTxGetStatus(port);
            AvUapiTxGetHdcpStatus(port);
            break;
        }
        case AvFsmPlugTxEnableTxCore:
        {
            AvHandleEvent(port, AvEventPortDownStreamConnected, NULL, NULL);
            AvUapiTxGetStatus(port);
            break;
        }
        case AvFsmPlugTxVideoUnlocked:
        {
            AvUapiTxGetStatus(port);
            break;
        }
        default:
            break;
    }
    return;
}

/* PART 7 */
/* CEC FSM state */
void KfunCheckCecState(AvPort *port)
{
#if AvEnableCecFeature /* CEC Related */
    switch (*port->content.is_CecFsm)
    {
#if (AvCecLogicAddress == 0)
        case AvFsmTvNotConnected:
            break;
#elif (AvCecLogicAddress == 5)
        case AvFsmCecNotConnected:
            break;
#endif
        default:
            AvUapiCecRxGetStatus(port);
            AvUapiCecTxGetStatus(port);
            if(port->content.cec->RxGetFlag)
                KfunCecRxMsgProcess(port);
            break;
    }
#endif
    return;
}

/* Part 8 Video Input and Output */
void KfunCheckLogicVideoTx(AvPort *port)
{
    //AvUapiCheckLogicVideoTx(port);
}

void KfunCheckLogicVideoRx(AvPort *port)
{
    //AvUapiCheckLogicVideoRx(port);
}

/* Part 9 Audio Input and Output */
void KfunCheckLogicAudioTx(AvPort *port)
{
    AvUapiCheckLogicAudioTx(port);
}

void KfunCheckLogicAudioRx(AvPort *port)
{
#if AvEnableAudioTTLInput
    AvUapiCheckLogicAudioRx(port);
#endif
}

/* Part 10 Scaler */
void KfunCheckVideoScaler(AvPort *port)
{
    AvPort *SourcePort = (AvPort*)port->content.RouteVideoFromPort;
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint32 EdidSupportFeature = 0xffffffff;
    if(SourcePort == NULL)
        return;
    else if(port->content.RouteVideoToPort == NULL)
    {
        port->content.scaler->ScalerInVic = 0;
        port->content.scaler->ColorSpace  = AV_Y2Y1Y0_INVALID;
        port->content.scaler->ScalerInCs  = AV_CS_AUTO;
    }
    else if((SourcePort->type == HdmiRx) || (SourcePort->type == VideoGen))
    {
        /* Update Vic */
        if(port->content.scaler->ScalerInVic != SourcePort->content.video->timing.Vic)
        {
            port->content.scaler->Update = 0x03;
            port->content.scaler->ScalerInVic = SourcePort->content.video->timing.Vic;
        }
        if(port->content.scaler->ColorSpace != SourcePort->content.video->Y)
        {
            port->content.scaler->Update = 0x03;
            port->content.scaler->ColorSpace = SourcePort->content.video->Y;
        }
        port->content.scaler->ScalerInCs = SourcePort->content.video->InCs;
        /* Check Downstream Full Range Support */
        while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
        {
            if((CurrentPort->type == HdmiTx) &&
               (CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED))
                EdidSupportFeature = EdidSupportFeature & CurrentPort->content.tx->EdidSupportFeature;
            PrevPort = CurrentPort;
            CurrentPort = NULL;
        }
        /* HDR2SDR is required */
        if((SourcePort->content.video->AvailableVideoPackets & AV_BIT_HDR_PACKET) &&
           (port->content.scaler->Hdr2SdrEnable == 1) &&
           (((port->content.scaler->ScalerInCs&0x1F) == 0x17) || ((port->content.scaler->ScalerInCs&0x1F) == 0x1B)) &&
           ((EdidSupportFeature & AV_BIT_FEAT_HDR) == 0))
        {
            if(port->content.scaler->ColorSpace == AV_Y2Y1Y0_RGB)
                port->content.scaler->ScalerOutSpace = AV_Y2Y1Y0_YCBCR_444;
            else
                port->content.scaler->ScalerOutSpace = port->content.scaler->ColorSpace;
            if(EdidSupportFeature & AV_BIT_FEAT_YUV_FULL_RANGE)
            {
                if(EdidSupportFeature & AV_BIT_FEAT_YUV_709)
                    port->content.scaler->ScalerOutCs = AV_CS_YUV_709;
                else
                    port->content.scaler->ScalerOutCs = AV_CS_YUV_601;
            }
            else
            {
                if(EdidSupportFeature & AV_BIT_FEAT_YUV_709)
                    port->content.scaler->ScalerOutCs = AV_CS_LIM_YUV_709;
                else
                    port->content.scaler->ScalerOutCs = AV_CS_LIM_YUV_601;
            }
        }
        else
        {
            /* no CSC for HDR */
            port->content.scaler->ScalerOutCs = port->content.scaler->ScalerInCs;
            port->content.scaler->ScalerOutSpace = port->content.scaler->ColorSpace;
        }
    }
    /* Call Uapi Function */
    AvUapiCheckVideoScaler(port);
}

/* Part 11 Color */
void KfunCheckVideoColor(AvPort *port)
{
    AvPort *SourcePort = (AvPort*)port->content.RouteVideoFromPort;
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint32 EdidSupportFeature = 0xffffffff;
    AvVideoY NewColorOutSpace;
    AvVideoCs NewColorOutCs;
    if(SourcePort == NULL)
        return;
    else if(port->content.RouteVideoToPort == NULL)
    {
        port->content.color->ColorInVic    = 0;
        port->content.color->ColorInSpace  = AV_Y2Y1Y0_INVALID;
        port->content.color->ColorOutSpace = AV_Y2Y1Y0_INVALID;
        port->content.color->ColorInCs     = AV_CS_AUTO;
        port->content.color->ColorOutCs    = AV_CS_AUTO;
    }
    else if((SourcePort->type == HdmiRx) || (SourcePort->type == VideoGen))
    {
        /* Input Vic */
        if(port->content.color->ColorInVic != SourcePort->content.video->timing.Vic)
        {
            port->content.color->Update = 0x03;
            port->content.color->ColorInVic = SourcePort->content.video->timing.Vic;
        }
        /* Input ColorSpace */
        if(port->content.color->ColorInSpace != SourcePort->content.video->Y)
        {
            port->content.color->Update = 0x03;
#if AvEnableInternalVideoGen
            if(SourcePort->type == VideoGen)
                port->content.color->ColorInSpace = AV_Y2Y1Y0_RGB;
            else
#endif
            port->content.color->ColorInSpace = SourcePort->content.video->Y;
        }
        if(port->content.color->ColorInCs != SourcePort->content.video->InCs)
        {
            port->content.color->Update = 0x03;
            port->content.color->ColorInCs = SourcePort->content.video->InCs;
        }
        /* Check Downstream Full Range Support */
        while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
        {
            if((CurrentPort->type == HdmiTx) &&
               (CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED))
                EdidSupportFeature = EdidSupportFeature & CurrentPort->content.tx->EdidSupportFeature;
            PrevPort = CurrentPort;
            CurrentPort = NULL;
        }
        /* Case 1: HDR2SDR is required */
        if(((SourcePort->content.video->AvailableVideoPackets & AV_BIT_HDR_PACKET) &&
            (port->content.color->Hdr2SdrEnable == 1) &&
            (((port->content.color->ColorInCs&0x1F) == 0x17) || ((port->content.color->ColorInCs&0x1F) == 0x1B)) &&
            ((EdidSupportFeature & AV_BIT_FEAT_HDR) == 0)) ||
        /* Case 2: RGB to YCbCr conversion is required */
           (SourcePort->content.video->Y == AV_Y2Y1Y0_RGB))
        {
            if(EdidSupportFeature & AV_BIT_FEAT_YUV_FULL_RANGE)
            {
                if(EdidSupportFeature & AV_BIT_FEAT_YUV_709)
                    NewColorOutCs = AV_CS_YUV_709;
                else
                    NewColorOutCs = AV_CS_YUV_601;
            }
            else
            {
                if(EdidSupportFeature & AV_BIT_FEAT_YUV_709)
                    NewColorOutCs = AV_CS_LIM_YUV_709;
                else
                    NewColorOutCs = AV_CS_LIM_YUV_601;
            }
        }
        else
            /* no CSC for HDR */
            NewColorOutCs = port->content.color->ColorInCs;
        /* Update CS when new State */
        if(port->content.color->ColorOutCs != NewColorOutCs)
        {
            port->content.color->Update = 0x03;
            port->content.color->ColorOutCs = NewColorOutCs;
        }
        /* Output ColorSpace */
        if(port->content.color->Enable == 0)
        {
            if(port->content.color->ColorInSpace == AV_Y2Y1Y0_RGB)
                NewColorOutSpace = AV_Y2Y1Y0_YCBCR_444;
            else
                NewColorOutSpace = port->content.color->ColorInSpace;
        }
        else
        {
            if(port->content.color->ColorInSpace != AV_Y2Y1Y0_YCBCR_420)
                NewColorOutSpace = AV_Y2Y1Y0_YCBCR_420;
            else
                NewColorOutSpace = AV_Y2Y1Y0_YCBCR_444;
        }
        if(NewColorOutSpace != port->content.color->ColorOutSpace)
        {
            port->content.color->Update = 0x03;
            port->content.color->ColorOutSpace = NewColorOutSpace;
        }
    }
    /* Call Uapi Function */
    AvUapiCheckVideoColor(port);
}

/* Part 12 VideoGen */
void KfunCheckVideoGen(AvPort *port)
{
    /* Call Uapi Function */
    if(port->content.RouteVideoToPort != NULL)
        AvUapiCheckVideoGen(port);
}

/* Part 13 AudioGen */
void KfunCheckAudioGen(AvPort *port)
{
    /* Call Uapi Function */
    if(port->content.RouteAudioToPort != NULL)
        AvUapiCheckAudioGen(port);
}

/* Part 14 ClockGen */
void KfunCheckClockGen(AvPort *port)
{
    /* Call Uapi Function */
    AvUapiCheckClockGen(port);
}

/* Part 15 EarcRx */
void KfunCheckEarcRx(AvPort *port)
{
#if AvEnableEArcRxFeature
    /* Call Uapi Function */
    AvUapiCheckEarcRx(port);
    KfunEarcRxManage(port);
#endif
}
