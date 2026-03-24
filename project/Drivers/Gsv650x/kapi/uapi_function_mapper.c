/**
 * @file uapi_function_mapper.c
 *
 * @brief low level ports related universal api functions
 */

#include "av_config.h"
#include "kapi.h"
#include "uapi.h"
#include "hal.h"
#include "av_event_handler.h"
#include "uapi_function_mapper.h"
#include "kernel_status_update.h"

void KfunRxVicFilter(pin AvPort *port);
#if AvEnableCecFeature /* CEC Related */
extern const uint8 FlexOpCodes[];
extern const uint8 CecOpCodes[];
#endif

/* PART 1 */
/* Receiver FSM */

/**
 * @brief
 * function to Initialize Rx port parameters, just like following script configuration
 * @return AvOk - success
 * @note
 */
void KfunRxVarInit(pin AvPort *port)
{
    AvUapiRxPortInit(port);
    AvMemset(port->content.video, 0, sizeof(AvVideo));
    AvMemset(port->content.audio, 0, sizeof(AvAudio));
    KfunHdcpReset(port);
    KfunHdcpVarInit(port);
    return;
}

/**
 * @brief
 * function to Initialize Rx port, just like following script configuration
 * @return AvOk - success
 * @note
 */
void KfunRxInit(pin AvPort *port)
{
    AvHandleEvent(port, AvEventRxPortInit, NULL, NULL);
    return;
}

void KfunTxfromRxPacketContent(AvPort *RxPort, AvPort *TxPort, uint8 *PktContent, uint16 AvBit,
                               PacketType AvPacket, AvPacketType type, AvRxUpdateType update)
{
    if(type == AvVideoPacketType)
    {
        if(((RxPort->content.video->AvailableVideoPackets & AvBit) != 0) || (update == AvRxForce))
        {
            if((update == AvRxUpdate) || (update == AvRxForce))
            {
                AvUapiRxGetPacketContent(RxPort, AvPacket, PktContent);
                KfunUpdatePortFromPacket(RxPort, AvPacket, PktContent);
            }
            KfunCheckVspPortContent(TxPort, AvPacket, PktContent);
            KfunUpdatePortFromPacket(TxPort, AvPacket, PktContent);
            AvUapiTxSetPacketContent(TxPort, AvPacket, PktContent, 1);
            TxPort->content.video->AvailableVideoPackets =
                TxPort->content.video->AvailableVideoPackets | AvBit;
        }
        else
        {
            AvUapiTxEnableInfoFrames(TxPort, AvBit, 0);
            TxPort->content.video->AvailableVideoPackets =
                TxPort->content.video->AvailableVideoPackets & (~AvBit);
        }
    }
    else if(type == AvAudioPacketType)
    {
        if(((RxPort->content.audio->AvailableAudioPackets & AvBit) != 0) || (update == AvRxForce))
        {
            if((update == AvRxUpdate) || (update == AvRxForce))
            {
                AvUapiRxGetPacketContent(RxPort, AvPacket, PktContent);
                KfunUpdatePortFromPacket(RxPort, AvPacket, PktContent);
            }
            KfunCheckVspPortContent(TxPort, AvPacket, PktContent);
            KfunUpdatePortFromPacket(TxPort, AvPacket, PktContent);
            AvUapiTxSetPacketContent(TxPort, AvPacket, PktContent, 1);
            TxPort->content.audio->AvailableAudioPackets =
                TxPort->content.audio->AvailableAudioPackets | AvBit;
        }
        else
        {
            AvUapiTxEnableInfoFrames(TxPort, AvBit, 0);
            TxPort->content.audio->AvailableAudioPackets =
                TxPort->content.audio->AvailableAudioPackets & (~AvBit);
        }
    }
}

void KfunRxtoTxPacketContent(AvPort *port, uint8 *PktContent, uint16 AvBit,
                             PacketType AvPacket, AvPacketType type)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;

    if(type == AvVideoPacketType)
    {
        if((port->type == HdmiRx) || (port->type == DpRx))
        {
            port->content.rx->ChangedVideoPackets = port->content.rx->ChangedVideoPackets & (~AvBit);
        }
        if((port->content.video->AvailableVideoPackets & AvBit) != 0)
        {
            AvUapiRxGetPacketContent(port, AvPacket, PktContent);
            KfunUpdatePortFromPacket(port, AvPacket, PktContent);
        }
        while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
        {
            if(CurrentPort->type == HdmiTx)
                KfunTxfromRxPacketContent(port, CurrentPort, PktContent, AvBit, AvPacket, type, AvRxUpdate);
            PrevPort = CurrentPort;
            CurrentPort = NULL;
        }
    }
    else if(type == AvAudioPacketType)
    {
        if((port->type == HdmiRx) || (port->type == DpRx))
        {
            port->content.rx->ChangedAudioPackets = port->content.rx->ChangedAudioPackets & (~AvBit);
        }
        if((port->content.audio->AvailableAudioPackets & AvBit) != 0)
        {
            AvUapiRxGetPacketContent(port, AvPacket, PktContent);
            KfunUpdatePortFromPacket(port, AvPacket, PktContent);
        }
        while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
        {
            if(CurrentPort->type == HdmiTx)
                KfunTxfromRxPacketContent(port, CurrentPort, PktContent, AvBit, AvPacket, type, AvRxUpdate);
            PrevPort = CurrentPort;
            CurrentPort = NULL;
        }
    }
}

#if AvEnableDisplayPortRx

void DpRxVideoManage(AvPort *port)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint8 PktContent[40];
    bool gcp = 0;

    /* GCP: always, polling VBID no_video flag */
    /* AVI INFO: always */
    /* AMD SPD: free sync */
    /* HDR: static hdr */
    /* VSIF: dynamic hdr */

    if(port->core.HdmiCore != 1)
        return;

    if(port->content.dprx->DpPhyLinkSt == PHYLINK_READY)
    {
        /* trigger once when link ready */
        if((port->content.dprx->DpRxSdpsChanged & (0x01 << SDP_AVI_INFO)) != 0)
        {
            port->content.dprx->DpRxSdpsChanged &= ~(0x01 << SDP_AVI_INFO);
            port->content.video->AvailableVideoPackets &= ~AV_BIT_GC_PACKET;
            /* mapping to hdmi */
            port->content.video->AvailableVideoPackets |= AV_BIT_AV_INFO_FRAME;
            port->content.rx->ChangedVideoPackets |= AV_BIT_AV_INFO_FRAME;
            /* AVI INFO: always */
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_AV_INFO_FRAME,
                                    AV_PKT_AV_INFO_FRAME, AvVideoPacketType);
            AvUapiOutputDebugMessage("AVI_INFO-->");
        }
        /* GCP: always, polling VBID no_video flag */
        if(((port->content.video->AvailableVideoPackets & AV_BIT_AV_INFO_FRAME) != 0) &&
           ((port->content.video->AvailableVideoPackets & AV_BIT_GC_PACKET) == 0))
        {
            port->content.video->AvailableVideoPackets |= AV_BIT_GC_PACKET;
            port->content.rx->ChangedVideoPackets |= AV_BIT_GC_PACKET;
            gcp = 1;
        }
        if(gcp)
        {
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                if(CurrentPort->type == HdmiTx)
                {
                    KfunTxSetColorDepth(port, CurrentPort);
                    AvUapiTxSetHdmiDeepColor(CurrentPort);
                    CurrentPort->content.video->Mute.AvMute = port->content.video->Mute.AvMute;
                    AvUapiOutputDebugMessage("GCP-->");
                    AvUapiTxSetAvMute(CurrentPort);
                    CurrentPort->content.video->AvailableVideoPackets |= AV_BIT_GC_PACKET;
                    AvUapiTxSetPacketContent(CurrentPort, AV_PKT_GC_PACKET, PktContent, 1);
                }
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
#if AvDisableFreeSync == 0
        /* AMD SPD: free sync */
        if((GET_DP_SDP_INTR(port->content.dprx->DpRxSdpIntr, SDP_SPD)) ||
            (port->content.dprx->DpRxSdpsChanged & (0x01 << SDP_SPD)))
        {
            port->content.dprx->DpRxSdpsChanged &= ~(0x01 << SDP_SPD);
            /* mapping to hdmi */
            if(GET_DP_SDP_PKT_FLAG(SDP_SPD))
                port->content.video->AvailableVideoPackets |= AV_BIT_SPD_PACKET;
            else
                port->content.video->AvailableVideoPackets &= ~AV_BIT_SPD_PACKET;
            port->content.rx->ChangedVideoPackets |= AV_BIT_SPD_PACKET;
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_SPD_PACKET,
                                    AV_PKT_SPD_PACKET, AvVideoPacketType);
            AvUapiOutputDebugMessage("AMD_SPD-->");
        }
#endif
        /* HDR: static hdr */
        if((GET_DP_SDP_INTR(port->content.dprx->DpRxSdpIntr, SDP_HDR_STATIC)) ||
            (GET_DP_SDP_CHG_INTR(port->content.dprx->DpRxSdpIntr, SDP_HDR_STATIC)) ||
            (port->content.dprx->DpRxSdpsChanged & (0x01 << SDP_HDR_STATIC)))
        {
            port->content.dprx->DpRxSdpsChanged &= ~(0x01 << SDP_HDR_STATIC);
            /* mapping to hdmi */
            if(GET_DP_SDP_PKT_FLAG(SDP_HDR_STATIC))
                port->content.video->AvailableVideoPackets |= AV_BIT_HDR_PACKET;
            else
                port->content.video->AvailableVideoPackets &= ~AV_BIT_HDR_PACKET;
            port->content.rx->ChangedVideoPackets |= AV_BIT_HDR_PACKET;
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_HDR_PACKET,
                                    AV_PKT_HDR_PACKET, AvVideoPacketType);
            AvUapiOutputDebugMessage("STATIC_HDR-->");
        }
        /* VDSI: dynamic hdr */
        if((GET_DP_SDP_INTR(port->content.dprx->DpRxSdpIntr, SDP_VDSI)) ||
            (GET_DP_SDP_CHG_INTR(port->content.dprx->DpRxSdpIntr, SDP_VDSI)) ||
            (port->content.dprx->DpRxSdpsChanged & (0x01 << SDP_VDSI)))
        {
            port->content.dprx->DpRxSdpsChanged &= ~(0x01 << SDP_VDSI);
            /* mapping to hdmi */
            if(GET_DP_SDP_PKT_FLAG(SDP_VDSI))
                port->content.video->AvailableVideoPackets |= AV_BIT_VS_PACKET;
            else
                port->content.video->AvailableVideoPackets &= ~AV_BIT_VS_PACKET;
            port->content.rx->ChangedVideoPackets |= AV_BIT_VS_PACKET;
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_VS_PACKET,
                                    AV_PKT_VS_PACKET, AvVideoPacketType);
            AvUapiOutputDebugMessage("VDSI(10+)-->");
        }
    }
}

void DpRxAudioManage(AvPort *port)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint8 PktContent[40];
    uint8 EnableAudio = 1;

    /* ACR: timstamp */

    if(port->core.HdmiCore != 1)
        return;

    if(port->content.dprx->DpPhyLinkSt == PHYLINK_READY)
    {
        /* AUDIO INFO: default */
        if(GET_DP_SDP_PKT_FLAG(SDP_AUDIO_INFO))
            port->content.audio->AvailableAudioPackets |= AV_BIT_AUDIO_INFO_FRAME;
        else
        {
            port->content.audio->AvailableAudioPackets &= (~AV_BIT_AUDIO_INFO_FRAME);
            /* Comment out to enable audio output without Audio Infoframe */
            EnableAudio = 0;
        }
        if((GET_DP_SDP_INTR(port->content.dprx->DpRxSdpIntr, SDP_AUDIO_INFO)) ||
            (GET_DP_SDP_CHG_INTR(port->content.dprx->DpRxSdpIntr, SDP_AUDIO_INFO)) ||
            (port->content.dprx->DpRxSdpsChanged & (0x01 << SDP_AUDIO_INFO)))
        {
            port->content.dprx->DpRxSdpsChanged &= ~(0x01 << SDP_AUDIO_INFO);
            if(GET_DP_SDP_PKT_FLAG(SDP_AUDIO_INFO))
            {
                /* mapping to hdmi */
                port->content.rx->ChangedAudioPackets |= AV_BIT_AUDIO_INFO_FRAME;

                KfunRxtoTxPacketContent(port, PktContent, AV_BIT_AUDIO_INFO_FRAME,
                                        AV_PKT_AUDIO_INFO_FRAME, AvAudioPacketType);
                AvUapiOutputDebugMessage("SDP_AUDIO_INFO-->");
            }
        }
        /* FS CHANGE */
        if(GET_DP_MISC_INTR(port->content.dprx->DpMiscIntr, MISC_INTR_FS_CHG))
        {
            CLR_DP_MISC_INTR(port->content.dprx->DpMiscIntr, MISC_INTR_FS_CHG);
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                CurrentPort->content.audio->NValue = port->content.audio->NValue;
                AvUapiTxSetAudNValue(CurrentPort);
                AvUapiOutputDebugMessage("FS_CHANGE_N-->");
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
        /* AUDIO CHANNEL STATUS */
        if(GET_DP_MISC_INTR(port->content.dprx->DpMiscIntr, MISC_INTR_CS_VALID))
        {
            CLR_DP_MISC_INTR(port->content.dprx->DpMiscIntr, MISC_INTR_CS_VALID);
            if(GET_DP_SDP_PKT_FLAG(SDP_AUD_CHN_ST))
            {
                /* mapping to hdmi */
                port->content.audio->AvailableAudioPackets |= AV_BIT_AUDIO_CHANNEL_STATUS;
                port->content.rx->ChangedAudioPackets |= AV_BIT_AUDIO_CHANNEL_STATUS;

                KfunRxtoTxPacketContent(port, PktContent, AV_BIT_AUDIO_CHANNEL_STATUS,
                                        AV_PKT_AUDIO_CHANNEL_STATUS, AvAudioPacketType);
                 /* for dprx, channel st is last step of audio process, should inform hdmi tx reset */
                /* because of dprx link issue whitch cann't read pkt correct from buffer, have to */
                PrevPort = NULL;
                CurrentPort = NULL;
                while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
                {
                    CurrentPort->content.audio->AvailableAudioPackets = 0;
                    PrevPort = CurrentPort;
                    CurrentPort = NULL;
                }
                AvUapiOutputDebugMessage("SDP_AUD_CHN_ST-->");
            }
        }
        /* AUDIO MUTE: */
        if(GET_DP_MISC_INTR(port->content.dprx->DpMiscIntr, MISC_INTR_AUD_MUTE))
        {
            /* inform tx mute unmute */
            port->content.audio->AudioMute = 1;
        }
        else
        {
            port->content.audio->AudioMute = 0;
        }
        /* Audio Stream & Audio Timstamp */
        if((GET_DP_SDP_PKT_FLAG(SDP_AUDIO_STREAM)) && (EnableAudio == 1))
        {
            port->content.audio->AvailableAudioPackets |= AV_BIT_ACR_PACKET;
        }
        else
        {
            port->content.audio->AvailableAudioPackets &= ~AV_BIT_ACR_PACKET;
        }
        //SDP_AUDIO_TIMESTAMP
        if((GET_DP_SDP_PKT_FLAG(SDP_AUDIO_TIMESTAMP)) && (EnableAudio == 1))
        {
            port->content.audio->AvailableAudioPackets |= AV_BIT_AUDIO_SAMPLE_PACKET;
        }
        else
        {
            port->content.audio->AvailableAudioPackets &= ~AV_BIT_AUDIO_SAMPLE_PACKET;
        }
    }
}

#endif

/**
 * @brief
 * function to Manage HDMI Rx Video Interrupts
 * @return AvOk - success
 * @note
 */
void KfunRxVideoManage(pin AvPort *port)
{
    /* receive and manage the video packet content */
    AvRet ret;
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint8 PktContent[40];
    VideoInterrupt Intpt;

    /* Don't output unless fully Stable and RxReadInfo is done */
    if(port->content.rx->IsFreeRun == 0)
        return;

#if AvEnableDisplayPortRx
    if(port->type == DpRx)
    {
        DpRxVideoManage(port);
        return;
    }
#endif

    /* Timing ReRead in case of Error */
#if AvEnableDetailTiming
    uint32 HTotal =
        port->content.video->timing.HActive +
        port->content.video->timing.HFront +
        port->content.video->timing.HSync +
        port->content.video->timing.HBack;
    uint32 VTotal =
        (port->content.video->timing.VActive>>port->content.video->timing.Interlaced) +
        port->content.video->timing.VFront +
        port->content.video->timing.VSync +
        port->content.video->timing.VBack;

    if((port->content.video->DscStream == 0) &&
       (port->content.video->VrrStream == 0) &&
       ((HTotal != port->content.video->timing.HTotal) ||
        (VTotal != port->content.video->timing.VTotal) ||
        (port->content.video->timing.HActive == 0) ||
        (port->content.video->timing.VActive == 0) ||
        (port->content.video->timing.HTotal  == 0) ||
        (port->content.video->timing.VTotal  == 0) ||
        (port->content.video->timing.VFront  == 0) ||
        (port->content.video->timing.VSync   == 0) ||
        (port->content.video->timing.VBack   == 0) ||
        (port->content.video->timing.HFront  == 0) ||
        (port->content.video->timing.HSync   == 0) ||
        (port->content.video->timing.HBack   == 0)))
    {
        AvUapiRxGetVideoTiming(port);
    }
#endif

    /* Step 1. Get the Packet Status */
    ret = AvUapiRxGetVideoPacketStatus(port, &Intpt);
    /* Step 1.1 Clear Interrupt */
    AvUapiRxClearVideoInterrupt(port, &Intpt);

    /* Step 2. Process it if existed */
    if((ret == AvOk) && (Intpt.DeRegenLck == 0) && (Intpt.NewTmds == 0) && (Intpt.BadTmdsClk == 0))
    {
        if(Intpt.AvMute)
        {
            AvUapiRxGetAvMute(port);
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                if(CurrentPort->type == HdmiTx)
                {
                    CurrentPort->content.video->Mute.AvMute = port->content.video->Mute.AvMute;
                    AvUapiOutputDebugMessage("from Intpt.AvMute-->");
                    AvUapiTxSetAvMute(CurrentPort);
                    //CurrentPort->content.video->AvailableVideoPackets =
                    //    CurrentPort->content.video->AvailableVideoPackets | AV_BIT_GC_PACKET;
                }
                PktContent[0] = 1;
                AvUapiTxSetPacketContent(CurrentPort, AV_PKT_GC_PACKET, PktContent, 1);
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
        if(Intpt.HdmiModeChg)
        {
            AvUapiRxGetHdmiModeSupport(port);
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                if(CurrentPort->type == HdmiTx)
                {
                    if((CurrentPort->content.tx->InfoReady > TxVideoManageThreshold) &&
                       (CurrentPort->core.HdmiCore != -1))
                    {
                        CurrentPort->content.tx->Hpd = AV_HPD_FORCE_LOW;
                    }
                    else
                    {
                        CurrentPort->content.tx->HdmiMode = port->content.rx->HdmiMode;
                        AvUapiTxSetHdmiModeSupport(CurrentPort);
                    }
                }
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
        if(Intpt.DeRegenLck)
        {
        }
        if(Intpt.VsyncLck)
        {
        }
        if(Intpt.Vid3dDet)
        {
            AvUapiRxGetVideoTiming(port);
        }
        if(Intpt.NewTmds)
        {
        }
        if(Intpt.BadTmdsClk)
        {
        }
        if(Intpt.DeepClrChg)
        {
            AvUapiRxGetHdmiDeepColor(port);
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                if(CurrentPort->type == HdmiTx)
                {
                    KfunTxSetColorDepth(port, CurrentPort);
                    AvUapiTxSetHdmiDeepColor(CurrentPort);
                }
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
            if(port->content.hdcp->Hdcp2p2RxRunning == 1)
                port->content.hdcp->HdcpError = port->content.hdcp->HdcpError + 1;
        }
        if(Intpt.PktErr)
        {
        }
        if((Intpt.AvIfValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_AV_INFO_FRAME) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_AV_INFO_FRAME,
                                    AV_PKT_AV_INFO_FRAME, AvVideoPacketType);
        }
        if((Intpt.SpdValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_SPD_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_SPD_PACKET,
                                    AV_PKT_SPD_PACKET, AvVideoPacketType);
        }
        if((Intpt.HdrValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_HDR_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_HDR_PACKET,
                                    AV_PKT_HDR_PACKET, AvVideoPacketType);
        }
        if((Intpt.EmpValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_EMP_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_EMP_PACKET,
                                    AV_PKT_EMP_PACKET, AvVideoPacketType);
        }
        if((Intpt.MsValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_MPEG_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_MPEG_PACKET,
                                    AV_PKT_MPEG_PACKET, AvVideoPacketType);
        }
        if((Intpt.VsValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_VS_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_VS_PACKET,
                                    AV_PKT_VS_PACKET, AvVideoPacketType);
        }
        if((Intpt.Isrc1Valid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_ISRC1_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_ISRC1_PACKET,
                                    AV_PKT_ISRC1_PACKET, AvVideoPacketType);
        }
        if((Intpt.Isrc2Valid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_ISRC2_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_ISRC2_PACKET,
                                    AV_PKT_ISRC2_PACKET, AvVideoPacketType);
        }
        if((Intpt.GamutValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_GMD_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_GMD_PACKET,
                                    AV_PKT_GMD_PACKET, AvVideoPacketType);
        }
        if((Intpt.GcValid) ||
           ((port->content.rx->ChangedVideoPackets & AV_BIT_GC_PACKET) != 0))
        {
            port->content.rx->ChangedVideoPackets =
                port->content.rx->ChangedVideoPackets & (~AV_BIT_GC_PACKET);
            if((port->content.video->AvailableVideoPackets & AV_BIT_GC_PACKET) != 0)
            {
                AvUapiRxGetAvMute(port);
                AvUapiRxGetHdmiDeepColor(port);
            }
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                if(CurrentPort->type == HdmiTx)
                {
                    if((port->content.video->AvailableVideoPackets & AV_BIT_GC_PACKET) != 0)
                    {
                        KfunTxSetColorDepth(port, CurrentPort);
                        AvUapiTxSetHdmiDeepColor(CurrentPort);
                        CurrentPort->content.video->Mute.AvMute = port->content.video->Mute.AvMute;
                        AvUapiOutputDebugMessage("from Intpt.GcValid-->");
                        AvUapiTxSetAvMute(CurrentPort);
                        CurrentPort->content.video->AvailableVideoPackets =
                            CurrentPort->content.video->AvailableVideoPackets | AV_BIT_GC_PACKET;
                        PktContent[0] = 1;
                        AvUapiTxSetPacketContent(CurrentPort, AV_PKT_GC_PACKET, PktContent, 1);
                    }
                    else
                    {
                        CurrentPort->content.video->AvailableVideoPackets =
                            CurrentPort->content.video->AvailableVideoPackets & (~AV_BIT_GC_PACKET);
                        PktContent[0] = 0;
                        AvUapiTxSetPacketContent(CurrentPort, AV_PKT_GC_PACKET, PktContent, 1);
                    }
                }
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
    }

    /* Step 3. Call Layer */
    AvUapiRxVideoManage(port, &Intpt);

}

/**
 * @brief
 * function to Manage HDMI Rx Audio Interrupts
 * @return AvOk - success
 * @note
 */
void KfunRxAudioManage(pin AvPort *port)
{
    AvRet ret;
    /* receive and manage the audio packet content */
    uint8 PktContent[40];
    AvPort* PrevPort = NULL;
    AvPort* CurrentPort = NULL;
    AudioInterrupt Intpt;
    uint8 TxClearAudioContentFlag = 0;

    /* Don't output unless fully Stable and RxReadInfo is done */
    if((port->content.rx->IsFreeRun == 0) || (port->content.rx->Lock.AudioLock == 0))
        return;

#if AvEnableDisplayPortRx
    if(port->type == DpRx)
    {
        DpRxAudioManage(port);
        return;
    }
#endif
    /* Step 1. Get the Packet Status */
    ret = AvUapiRxGetAudioPacketStatus(port, &Intpt);
    /* Step 1.1 Clear Interrupt */
    AvUapiRxClearAudioInterrupt(port, &Intpt);

    /* Step 2. Process it if existed */
    if(ret == AvOk)
    {
        if(Intpt.AudChanMode)
        {
            Intpt.AudIfValid = 1;
            Intpt.CtsThresh = 1;
            Intpt.CsDataValid = 1;
        }
        if(Intpt.InternMute)
        {
            AvUapiRxGetAudioInternalMute(port); /* only detect AV MUTE and Audio Mute */
            AvUapiRxSetAudioInternalMute(port);
        }
        if(Intpt.CtsThresh)
        {
            AvUapiRxGetHdmiAcrInfo(port);
        }
        if(Intpt.AudFifoOv)
        {
        }
        if(Intpt.AudFifoUn)
        {
        }
        if(Intpt.AudFifoNrOv)
        {
        }
        if(Intpt.AudioPktErr)
        {
        }
        if(Intpt.AudModeChg)
        {
            /* when audio mode is changed, always resend audio */
            TxClearAudioContentFlag = 1;
            AvUapiRxGetPacketType(port);
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                CurrentPort->content.audio->AudType = port->content.audio->AudType;
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
        if(Intpt.AudFifoNrUn)
        {
        }
        if(Intpt.AudFlatLine)
        {
            AvUapiRxGetAudioInternalMute(port);
            AvUapiRxSetAudioInternalMute(port);
        }
        if(Intpt.AudSampChg)
        {
            /* when audio mode is changed, always resend audio */
            TxClearAudioContentFlag = 1;
            AvUapiRxGetHdmiAcrInfo(port);
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                CurrentPort->content.audio->NValue = port->content.audio->NValue;
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
        if(Intpt.AudPrtyErr)
        {
        }
        if((Intpt.AcpValid) ||
           ((port->content.rx->ChangedAudioPackets & AV_BIT_ACP_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_ACP_PACKET, AV_PKT_ACP_PACKET,
                                    AvAudioPacketType);
        }
        if((Intpt.AmdValid) ||
           ((port->content.rx->ChangedAudioPackets & AV_BIT_AMD_PACKET) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_AMD_PACKET, AV_PKT_AMD_PACKET,
                                    AvAudioPacketType);
        }
        if((Intpt.CsDataValid) ||
           ((port->content.rx->ChangedAudioPackets & AV_BIT_AUDIO_CHANNEL_STATUS) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_AUDIO_CHANNEL_STATUS,
                                    AV_PKT_AUDIO_CHANNEL_STATUS, AvAudioPacketType);
        }
        if((Intpt.AudIfValid) ||
           ((port->content.rx->ChangedAudioPackets & AV_BIT_AUDIO_INFO_FRAME) != 0))
        {
            KfunRxtoTxPacketContent(port, PktContent, AV_BIT_AUDIO_INFO_FRAME,
                                    AV_PKT_AUDIO_INFO_FRAME, AvAudioPacketType);
        }
        if(Intpt.NChange)
        {
            AvUapiRxGetHdmiAcrInfo(port);
            PrevPort = NULL;
            CurrentPort = NULL;
            while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                CurrentPort->content.audio->NValue = port->content.audio->NValue;
                AvUapiTxSetAudNValue(CurrentPort);
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
        }
    }

    /* Step 3. Rx Audio has important change, update Tx again */
    if(TxClearAudioContentFlag == 1)
    {
        PrevPort = NULL;
        CurrentPort = NULL;
        while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
        {
            if(CurrentPort->type == HdmiTx)
                AvUapiTxEnableInfoFrames(CurrentPort, (AV_BIT_ACR_PACKET | AV_BIT_AUDIO_SAMPLE_PACKET), 0);
            CurrentPort->content.audio->AvailableAudioPackets = 0;
            PrevPort = CurrentPort;
            CurrentPort = NULL;
        }
    }

    /* Step 4. Call Silicon Layer */
    AvUapiRxAudioManage(port, &Intpt);

}

/**
 * @brief
 * function to Enable HDMI Rx Freerun
 * @return AvOk - success
 * @note
 */
void KfunRxEnableFrun(pin AvPort *port)
{
    if((port->content.rx->IsFreeRun == 1) &&
       (port->content.rx->IsInputStable == 0) &&
       (port->content.rx->H2p1FrlType != AV_FRL_NONE))
    {
        KfunRxReadInfo(port);
    }
    else
    {
        AvUapiRxEnableFreeRun(port, true);
    }
    return;
}

/**
 * @brief
 * function to Disable HDMI Rx Freerun
 * @return AvOk - success
 * @note
 */
void KfunRxDisableFrun(pin AvPort *port)
{
    AvUapiRxEnableFreeRun(port, false);
    return;
}

/**
 * @brief
 * function to Poll Rx Status to update Rx Port vars
 * @return AvOk - success
 * @note
 */
void KfunPollingRxStatus(pout AvPort *port)
{
    /* to update the current port status */
    /* AvUapiRxGetStatus(port); */
    return;
}

/**
 * @brief
 * function to Clear HDMI Rx Flags in Port Vars
 * @return AvOk - success
 * @note
 */
void KfunRxClearFlags(pin AvPort *port)
{
    AvUapiRxClearFlags(port);
    KfunRxReadInfo(port);
    return;
}

/* PART 2 */
/* HDCP FSM */
/**
 * @brief
 * function to Initialize HDCP Variables in Port structure
 * @return AvOk - success
 * @note
 */
void KfunHdcpVarInit(pin AvPort *port)
{
    AvHandleEvent(port, AvEventRxSetHdcpStyle, NULL, NULL);
    AvUapiRxSetHdcpEnable(port);
    return;
}

/**
 * @brief
 * function to Reset HDCP Variables in Port structure
 * @return AvOk - success
 * @note
 */
void KfunHdcpReset(pin AvPort *port)
{
    port->content.hdcp->SinkNumber = 0;
    port->content.hdcp->SinkTotal = 0;
    port->content.hdcp->BksvPtr = 0;
    port->content.hdcp->AksvInterrupt = 0;
    AvUapiRxSetHdcpMode(port);
    AvUapiHdcp2p2Mode(port);
    AvUapiOutputDebugMessage("Port[%d][%d]:HDCP Reset to REC Mode", port->device->index, port->index);
    return;
}

/**
 * @brief
 * function to Detect Source in HDCP Connection Chain in Port structure
 * @return AvOk - success
 * @note
 */
void KfunHdcpDetectSource(pin AvPort *port)
{
    AvRet ret;
    HdcpInterrupt Intpt;
    AvPort* FrontEnd = (AvPort *)port->content.HdcpSource;
    AvMemset((void*)(&Intpt), 0, sizeof(HdcpInterrupt));

    /* Front End processing for HdmiRx*/
    if(port->type == HdmiRx)
    {
        /* If HdcpNeeded = 0, HDCP is not needed and supported */
        if(port->content.hdcp->HdcpNeeded != AV_HDCP_RX_NOT_SUPPORT)
        {
            /* front end */
            /* or not connected, you don't need to consider hdcp */
            if(!FrontEnd)
            {
                /* 1. Get Rx Status */
                ret = AvUapiRxGetHdcpStatus(port, &Intpt);

                /* 2. Interpret interrupt information */
                if(ret == AvOk)
                {
                    if(Intpt.AksvUpdate)
                    {
                    }
                    if(Intpt.Encrypted)
                    {
                        if(port->content.rx->VideoEncrypted)
                            AvHandleEvent(port, AvEventPortUpStreamEncrypted, NULL, NULL);
                        else
                            AvHandleEvent(port, AvEventPortUpStreamDecrypted, NULL, NULL);
                    }
                }

                /* 3. Clear Interrupt */
                AvUapiRxClearHdcpInterrupt(port, &Intpt);

                /* 4. Hdcp Rx Error Manage */
                if((port->type == HdmiRx) &&
                   (port->content.rx->VideoEncrypted == 1) &&
                   (port->content.rx->Lock.EqLock == 1) &&
                   (port->content.rx->H2p1FrlType == AV_FRL_NONE) &&
                   (port->content.rx->IsInputStable == 0))
                    port->content.hdcp->HdcpError = port->content.hdcp->HdcpError + 1;
                if(port->content.hdcp->HdcpError >= AvHdcpRxErrorThreshold)
                {
                    port->content.rx->Hpd = AV_HPD_TOGGLE;
                    port->content.hdcp->HdcpError = 0;
                }
            }
        }
    }
}

/**
 * @brief
 * function to Detect Sinks in HDCP Connection Chain in Port structure
 * @return AvOk - success
 * @note
 */
void KfunHdcpDetectSink(pin AvPort *port)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    port->content.hdcp->SinkTotal = 0;

    /* Summarize all the Sinks that support Hdcp */

    while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
    {
        /* Check whether there are missing sinks or added sinks */
        if(CurrentPort->type == HdmiTx)
        {
            /* Get Tx Hdcp Status */
            AvUapiTxGetHdcpStatus(CurrentPort);
            /* Support HDCP */
            if((CurrentPort->content.hdcptx->HdcpSupport == 1) || (CurrentPort->content.hdcptx->Hdcp2p2SinkSupport == 1))
            {
                /* Increase Sink Total Number */
                port->content.hdcp->SinkTotal++;
            }
        }

        PrevPort = CurrentPort;
        CurrentPort = NULL;
    }
}

void KfunSimpleHdcpSync(pin AvPort *port)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;

    port->content.hdcp->SinkTotal = 0;

    /* Summarize all the Sinks that support Hdcp */
    while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
    {
        /* Check whether there are missing sinks or added sinks */
        if(CurrentPort->type == HdmiTx)
        {
            /* enable sink hdcp */
            if((port->content.rx->VideoEncrypted == 1) &&
               (*(port->content.is_PlugRxFsm)        == AvFsmPlugRxStable) &&
               (*(CurrentPort->content.is_PlugTxFsm) == AvFsmPlugTxStable) &&
               (CurrentPort->content.tx->InfoReady   >= TxHdcpManageThreshold))
            {
                AvUapiTxEncryptSink(CurrentPort);
            }
            /* disable sink hdcp */
            if(port->content.rx->VideoEncrypted == 0)
            {
                if(CurrentPort->content.tx->HdmiMode == 0)
                {
                    if(CurrentPort->content.video->Mute.BlkMute == 1)
                    {
                        CurrentPort->content.video->Mute.BlkMute = 0;
                        AvUapiTxSetBlackMute(CurrentPort);
                    }
                }
                else if(CurrentPort->content.hdcptx->HdcpEnabled == 1)
                {
                    AvUapiTxDecryptSink(CurrentPort);
                }
            }
        }

        PrevPort = CurrentPort;
        CurrentPort = NULL;
    }
}

void KfunTxUnlockProtection(pio AvPort *port)
{
    port->content.video->AvailableVideoPackets = 0;
    port->content.audio->AvailableAudioPackets = 0;
    port->content.tx->InfoReady = 0;
    AvUapiTxDisableCore(port);
    AvUapiTxEnableCore(port);

    return;
}

/**
 * @brief
 * function to Read Bksv in All HDCP Sinks of HDCP Chain Connection
 * @return AvOk - success
 * @note
 */
void KfunReadBksv(pin AvPort *port)
{
    return;
}

/**
 * @brief
 * function to look up HDCP Source in the HDCP connection
 * @return AvOk - success
 * @note
 */
AvPort *KfunLookupHdcpSource(pin AvPort *port)
{
    return (AvPort*)(port->content.HdcpSource);
}

/**
 * @brief
 * function to upload Sink information to Source
 * @return AvOk - success
 * @note
 */
void KfunUploadSinkInfo(pin AvPort *port)
{
    /* Find front end device and upload it */
    return;
}

/**
 * @brief
 * function to copy Bksv to Rx Bksv Pool
 * @return AvOk - success
 * @note
 */
AvRet KfunCopyBksv(pout AvPort *RxPort, pin AvPort *TxPort)
{
    return AvOk;
}

/**
 * @brief
 * function to check HDCP source encryption status
 * @return AvOk - success
 * @note
 */
void KfunCheckEncryption(pin AvPort *port)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;

    while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
    {
        /* Check whether there are missing sinks or added sinks */
        if(CurrentPort->type == HdmiTx)
        {
            if(port->content.video->Mute.AvMute != CurrentPort->content.video->Mute.AvMute)
            {
                CurrentPort->content.video->Mute.AvMute = port->content.video->Mute.AvMute;
                AvUapiTxSetAvMute(CurrentPort);
            }
        }
        PrevPort = CurrentPort;
        CurrentPort = NULL;
    }
    return;
}

/**
 * @brief
 * function to check HDCP sink encryption
 * @return AvOk - success
 * @note
 */
void KfunEncryptSink(pin AvPort *port)
{
    return;
}

/**
 * @brief
 * function to decrypt HDCP sink encryption
 * @return AvOk - success
 * @note
 */
void KfunDecryptSink(pin AvPort *port)
{
    return;
}

/**
 * @brief
 * function to decrypt new HDCP sink encryption
 * @return AvOk - success
 * @note
 */
void KfunDecryptNewSink(pin AvPort *port)
{
    return;
}

/**
 * @brief
 * function to remove All AvMute in HDCP Txs of HDCP Connection
 * @return AvOk - success
 * @note
 */
void KfunHdcpUnMuteAv(pio AvPort *port)
{
    return;
}

/**
 * @brief
 * function to set All AvMute in HDCP Tx of HDCP Connection
 * @return AvOk - success
 * @note
 */
void KfunTxSetMuteAv(pio AvPort *port)
{
    /* AvMute */
    if(port->content.tx->HdmiMode)
    {
        port->content.video->Mute.AvMute = 1;
#if AvEnableDebugHdcp
        AvUapiOutputDebugMessage("from Kfun TX SetMuteAv-->");
#endif
        AvUapiTxSetAvMute(port);
        /* BlackMute */
        port->content.video->Mute.BlkMute = 1;
        AvUapiTxSetBlackMute(port);
    }
    return;
}

/**
 * @brief
 * function to clear All AvMute in HDCP Tx of HDCP Connection
 * @return AvOk - success
 * @note
 */
void KfunTxClearMuteAv(pio AvPort *port)
{
    /* AvMute */
    if(port->content.tx->HdmiMode)
    {
        port->content.video->Mute.AvMute = 0;
#if AvEnableDebugHdcp
        AvUapiOutputDebugMessage("from Kfun TX ClearMuteAv-->");
#endif
        AvUapiTxSetAvMute(port);
        /* BlackMute */
        port->content.video->Mute.BlkMute = 0;
        AvUapiTxSetBlackMute(port);
    }
    return;
}

/**
 * @brief
 * function to mute Av in HDCP sinks of connection chain
 * @return AvOk - success
 * @note
 */
void KfunHdcpMuteAv(pio AvPort *port)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;

    while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
    {
        /* Check whether there are missing sinks or added sinks */
        if(CurrentPort->type == HdmiTx)
        {
            /* Support HDCP */
            if(((CurrentPort->content.hdcptx->HdcpSupport == 1) || (CurrentPort->content.hdcptx->Hdcp2p2SinkSupport == 1)) &&
               (CurrentPort->content.hdcptx->Authenticated == 0))
            {
                KfunTxSetMuteAv(CurrentPort);
            }
        }
        PrevPort = CurrentPort;
        CurrentPort = NULL;
    }
}

/**
 * @brief
 * function to manage HDCP status of HDMI Tx
 * @return AvOk - success
 * @note
 */
void KfunTxHdcpManage(pio AvPort *port)
{
    if(port->content.hdcptx->HdcpEnabled)
    {
        if(port->content.hdcptx->BksvReady == 1)
        {
            AvUapiTxClearBksvReady(port);
        }
#if AvEnableHdcp2p2Feature
        AvUapiTxHdcp2p2Manage(port);
#endif

    }
    return;
}

/* PART 3 */
/* Transmitter FSM */

/**
 * @brief
 * function to Mute Tx TMDS
 * @return AvOk - success
 * @note
 */
void KfunTxTmdsMute(pout AvPort *port)
{
    AvUapiTxMuteTmds(port, true);
    return;
}

/**
 * @brief
 * function to UnMute Tx TMDS
 * @return AvOk - success
 * @note
 */
void KfunTxTmdsUnmute(pout AvPort *port)
{
    AvUapiTxMuteTmds(port, false);
    return;
}

/**
 * @brief
 * function to Detect Hdmi Tx Status
 * @return AvOk - success
 * @note
 */
void KfunPollingTxStatus(pout AvPort *port)
{
    AvUapiTxDetectMode(port);
    return;
}

/* PART 4 */
/* Tx Routing FSM */

/**
 * @brief
 * function to Initialize Tx Routing Fsm Vars
 * @return AvOk - success
 * @note
 */
void KfunTxRoutingVarInit(pout AvPort *port)
{
    return;
}

/**
 * @brief
 * function to Initialize Tx Routing Fsm Vars
 * @return AvOk - success
 * @note
 */
void KfunRxConnection(pio AvPort *port)
{
    return;
    /* if(port != NULL) */
        /* update all members Hdcp from hardware (rx port only) */
        /* AvUapiRxGetHdcpStatus(port); */
}

/**
 * @brief
 * function to check Tx Connection status
 * @return AvOk - success
 * @note
 */
void KfunTxConnection(pio AvPort *port)
{
    if(port)
    {
        /* HDCP Detect Sink Number will only be done for HdmiRx Port */
        if(port->type == HdmiRx)
        {
            /* Input parameter is still Rx Port */
            /* Find the first TxPort */
            AvPort* TxPort = (AvPort*)(port->content.HdcpNextSinkPort);
            port->content.hdcp->SinkTotal = 0;
            port->content.hdcp->SinkNumber = 0;

            /* Step 1. Update SinkTotal and SinkNumber */
            while(TxPort)
            {
                AvUapiTxGetHdcpStatus(TxPort);
                /* find out whether this is a HDCP needed Tx */
                if((TxPort->content.hdcptx->HdcpSupport == 1) || (TxPort->content.hdcptx->Hdcp2p2SinkSupport == 1))
                    port->content.hdcp->SinkTotal++;
                /* Step 2. Look for the next TxPort */
                TxPort = (AvPort*)(TxPort->content.HdcpNextSinkPort);
            }
        }
    }

}

AvRet KfunTxSinkLost(AvPort *port)
{
    AvHandleEvent(port, AvEventPortDownStreamDisconnected, NULL, NULL);
    return AvOk;
}

/* PART 5 */
/* Plug RX FSM */

/**
 * @brief
 * function to Initialize Plug Rx Vars
 * @return AvOk - success
 * @note
 */
void KfunPrVarInit(pin AvPort *port)
{
    port->content.rx->Reset = 1;
    port->content.rx->EnableFreeRun = 1;
    port->content.rx->ForceFreeRun = 0;
    port->content.rx->IsFreeRun = 0;
    port->content.rx->IsInputStable = 0;
    port->content.rx->VideoEncrypted = 0;
    port->content.rx->EdidStatus = AV_EDID_RESET;
    port->content.rx->Hpd = AV_HPD_LOW;
    port->content.rx->HpdDelayExpire = 0;
    port->content.rx->Lock.AudioLock = 0;
    port->content.rx->Lock.DeRegenLock = 0;
    port->content.rx->Lock.PllLock = 0;
    port->content.rx->Lock.VSyncLock = 0;
    port->content.rx->HdmiMode = 0;
#if AvEnableDisplayPortRx
    if(port->type == DpRx)
    {
        port->content.dprx->DpPhyLinkSt = PHYLINK_IDLE;
        port->content.dprx->DpNoLtSt = DP_NO_LT_OFF;
    }
#endif
    return;
}

/**
 * @brief
 * function to Reset Plug Rx Port
 * @return AvOk - success
 * @note
 */
void KfunPrResetPort(pin AvPort *port)
{
    AvUapiResetPort(port);
    return;
}

/**
 * @brief
 * function to Plug Rx Hpd Down
 * @return AvOk - success
 * @note
 */
void KfunPrHpdDown(pin AvPort *port)
{
    AvUapiRxSetHpdDown(port);
    if(port->content.rx->Hpd != AV_HPD_LOW)
        port->content.rx->HpdDelayExpire = 0;
    else
        port->content.rx->HpdDelayExpire = RxHpdDelayExpireThreshold;
    port->content.rx->Hpd = AV_HPD_LOW;
    port->content.hdcp->HdcpError = 0;
    return;
}

/**
 * @brief
 * function to Plug Rx Hpd Up
 * @return AvOk - success
 * @note
 */
void KfunPrHpdUp(pin AvPort *port)
{
    AvUapiRxSetHpdUp(port);
    port->content.rx->Hpd = AV_HPD_HIGH;
    port->content.rx->HpdDelayExpire = 0;
    return;
}

void KfunManageHpa(pin AvPort *port)
{
    AvPort *PrevPort = NULL;
    AvPort *CurrentPort = NULL;
    uint8 ValidSinkFlag = 0;

#if (AvAllowHpdLowEdidRead == 1)
    CurrentPort = port->device->port; /* Get 1st Port of the device */
    while((CurrentPort != NULL) && (CurrentPort->device->index == port->device->index))
    {
        if((port->core.HdmiCore != -1) &&
           (CurrentPort->type == HdmiTx) &&
           (CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED))
        {
            ValidSinkFlag = 1;
        }
        CurrentPort = (AvPort*)(CurrentPort->next);
    }
    CurrentPort = NULL;
#endif

#if AvForceDefaultEdid
    if(port->content.rx->EdidStatus == AV_EDID_NEEDUPDATE)
        port->content.rx->EdidStatus = AV_EDID_UPDATED;
#endif

    /* Toggle will cause HPD to Drive Low */
    if(port->content.rx->Hpd == AV_HPD_TOGGLE)
    {
        AvUapiOutputDebugMessage("Port[%d][%d]:RxHPD Toggled", port->device->index, port->index);
        KfunPrHpdDown(port);
    }

    /* EDID Related HPA */
    /* wait until EDID is updated to assert HPA */
    if((port->content.rx->EdidStatus == AV_EDID_RESEND) ||
        (port->content.rx->EdidStatus == AV_EDID_UPDATED))
    {
        if(port->content.rx->Hpd == AV_HPD_LOW)
        {
            if(port->content.rx->HpdDelayExpire >= RxHpdDelayExpireThreshold)
            {
                /* Only Pull UP HPA when Valid Sink is available */
                CurrentPort = NULL;
                PrevPort = NULL;
                while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
                {
                    if((CurrentPort->type == HdmiTx) || (CurrentPort->type == DviTx))
                    {
                        if(CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED)
                            ValidSinkFlag = 1;
                    }
                    PrevPort = CurrentPort;
                    CurrentPort = NULL;
                }
                /* Check Audio Output */
#if AvEnableTTLCompressAudio
                PrevPort = NULL;
                CurrentPort = NULL;
                while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
                {
                    if(CurrentPort->type == LogicAudioTx)
                        ValidSinkFlag = 1;
                    PrevPort = CurrentPort;
                    CurrentPort = NULL;
                }
#endif
                /* Force Downstream Output */
#if (AvEnableTxSingleLaneMode || AvEnableRxSingleLaneMode)
                ValidSinkFlag = 1;
#endif
                /* Pull Up Action */
                if(ValidSinkFlag == 1)
                {
                    KfunPrHpdUp(port);
                    port->content.rx->EdidStatus = AV_EDID_UPDATED;
                }
            }
        }
    }
    else
    {
        if((port->content.rx->Hpd == AV_HPD_HIGH) ||
           (port->content.rx->Hpd == AV_HPD_FORCE_LOW))
        {
            AvUapiOutputDebugMessage("Port[%d][%d]:RxHPD Forced LOW",port->device->index,port->index);
            KfunPrHpdDown(port);
        }
        else if(port->content.rx->Hpd == AV_HPD_FORCE_HIGH)
            KfunPrHpdUp(port);
    }
}

/**
 * @brief
 * function to Prepare Edid
 * @return AvOk - success
 * @note
 */
void KfunPrepareEdid(pin AvPort *port)
{
    if(port->content.rx->EdidStatus != AV_EDID_UPDATED)
        AvHandleEvent(port, AvEventRxPrepareEdid, NULL, NULL);
}

/**
 * @brief
 * function to Read Stdi Information
 * @return AvOk - success
 * @note
 */
void KfunStdiReadStatus(pin AvPort *port)
{
    AvUapiRxReadStdi(port);
    return;
}

/**
 * @brief
 * function to Read Rx Information
 * @return AvOk - success
 * @note
 */
void KfunRxReadInfo(pin AvPort *port)
{
    AvPort *PrevPort;
    AvPort *CurrentPort;
    AvUapiRxReadInfo(port);
    AvUapiRxGetHdmiDeepColor(port);
    AvUapiRxGetVideoTiming(port);
    KfunRxVicFilter(port);
    PrevPort = NULL;
    CurrentPort = NULL;
    while(KfunFindVideoNextNode(port, &PrevPort, &CurrentPort) == AvOk)
    {
        if(CurrentPort->type == HdmiTx)
        {
            AvMemcpy(&(CurrentPort->content.video->timing),
                     &(port->content.video->timing),
                     sizeof(VideoTiming));
            KfunTxSetColorDepth(port, CurrentPort);
            AvUapiTxSetVideoTiming(CurrentPort);
        }
        PrevPort = CurrentPort;
    }
    return;
}

/* PART 6 */
/* Plug TX FSM */

/**
 * @brief
 * function to Plug Tx Fsm Var Initialization
 * @return AvOk - success
 * @note
 */
void KfunPtVarInit(pout AvPort *port)
{
    port->content.hdcptx->HdcpModeUpdate = 0;
    port->content.hdcptx->HdcpError = 0;
    port->content.hdcptx->BksvReady = 0;
    port->content.hdcptx->Authenticated = 0;
    port->content.hdcptx->HdcpSupport = 0;
    port->content.hdcptx->Hdcp2p2SinkSupport = 0;
    port->content.hdcptx->HdcpEnabled = 0;
    port->content.tx->Lock.PllLock = 0;
    port->content.tx->Lock.AudioLock = 0;
    port->content.tx->Lock.DeRegenLock = 0;
    port->content.tx->Lock.VSyncLock = 0;
    port->content.tx->InfoReady = 0;
    port->content.tx->H2p1FrlExtraDelay = 0;
    port->content.tx->EdidReadFail = 0;
    port->content.tx->EdidReadSuccess = AV_EDID_RESET;
    port->content.tx->IgnoreEdidError = 0; /* for test without EDID fun */
    port->content.tx->Hpd = AV_HPD_LOW; /* RELEASE_LOW */
    port->content.tx->HpdDelayExpire = 0;
    port->content.video->AvailableVideoPackets = 0;
    port->content.audio->AvailableAudioPackets = 0;
    AvHandleEvent(port, AvEventTxSetHdcpStyle, NULL, NULL);
    return;
}

/**
 * @brief
 * function to Plug Tx Port Initialization
 * @return AvOk - success
 * @note
 */
void KfunPtPortInit(pout AvPort *port)
{
    AvUapiTxPortInit(port);
    return;
}

/**
 * @brief
 * function to Reset Port for HDMI Tx Port
 * @return AvOk - success
 * @note
 */
void KfunPtResetPort(pout AvPort *port)
{
    /* no Tx Audio detection, enable it directly */
    port->content.tx->Lock.AudioLock = 1;
    port->content.tx->EdidReadFail = 0;
    port->content.tx->EdidReadSuccess = AV_EDID_RESET;
    port->content.tx->Hpd = AV_HPD_LOW; /* RELEASE_LOW */
    port->content.tx->InfoReady = 0;
    port->content.tx->H2p1FrlExtraDelay = 0;
    AvUapiResetPort(port);
#if AvEnableHdcp2p2Feature
    port->content.hdcptx->Hdcp2p2TxRunning = 0;
#endif
    AvHandleEvent(port, AvEventTxPortInit, NULL, NULL);
    return;
}

/**
 * @brief
 * function to Enable HDMI Tx Core
 * @return AvOk - success
 * @note
 */
void KfunPtEnableTxCore(pout AvPort *port)
{
    /* Low Freq Protection against Tx HPD */
    AvPort *SourcePort;
    AvUapiTxEnableCore(port);
    if(KfunFindVideoRxFront(port, &SourcePort) == AvOk)
    {
        if(SourcePort->content.rx->IsInputStable == 1)
            AvUapiTxSetVideoTiming(port);
    }
    return;
}

/**
 * @brief
 * function to do Anti Dither FSM in Plug Tx Fsm
 * @return AvOk - success
 * @note
 */
void KfunPtAntiDither(pout AvPort *port)
{
    /* Temp fix, should call event or setting timer to decide the delay */
    port->content.tx->HpdDelayExpire = port->content.tx->HpdDelayExpire + 1;
    return;
}

/**
 * @brief
 * function to Manage Video in HDMI Tx
 * @return AvOk - success
 * @note
 */
AvRet KfunTxVideoManage(pout AvPort *port)
{
    AvRet ret;
    uint16 DifferentVideoPackets = 0;
    uint16 VspGenVideoPackets = 0;
    AvPort *SourcePort = NULL;
    AvPort *UpperPort = NULL;
    uint8 PktContent[40];
    uint8 UpdateSlot = 0;

    /* 1. Get Source information */
    /* if source does not exist, no need to update information at all */
    /* 2. if source exists, follow Rx's status, compare difference */
    /* Firstly carry bulk info from Front Port, then update the port */
    /* 3. if source port is vsp port, update information accordingly */
    if(KfunFindVideoRxFront(port, &SourcePort) == AvOk)
    {
        UpperPort = (AvPort*)port->content.RouteVideoFromPort;
        ret = AvOk;
        /* InfoReady Delay Implement */
        if((port->content.tx->InfoReady <= TxHdcpManageThreshold) ||
           (port->content.tx->InfoReady <= TxScdcManageThreshold) ||
           (port->content.tx->InfoReady <= TxCecManageThreshold))
        {
            port->content.tx->InfoReady = port->content.tx->InfoReady + 1;
            //AvUapiOutputDebugMessage("Port[%d][%d]:InfoReady = %d", port->device->index, port->index, port->content.tx->InfoReady);
        }

        if(port->core.HdmiCore == -1)
            return AvOk;

        /* FRL Stream Link Protection */
        if((SourcePort != NULL) && (SourcePort->type == HdmiRx) &&
           (SourcePort->content.rx->IsFreeRun == 0))
            return AvOk;

        DifferentVideoPackets = SourcePort->content.video->AvailableVideoPackets ^
                                port->content.video->AvailableVideoPackets;
        if(SourcePort != UpperPort)
        {
            /* Check Hdmi Core Update Status */
            switch(port->core.HdmiCore)
            {
                case 0:
                    UpdateSlot = 0x01;
                    break;
                case 1:
                    UpdateSlot = 0x02;
                    break;
            }
            switch(UpperPort->type)
            {
                case VideoScaler:
                    if((UpperPort->content.scaler->Update & UpdateSlot) != 0)
                    {
                        UpperPort->content.scaler->Update = (UpperPort->content.scaler->Update & (~UpdateSlot));
                        VspGenVideoPackets = VspGenVideoPackets | AV_BIT_AV_INFO_FRAME;
                    }
                    break;
                case VideoColor:
                    if((UpperPort->content.color->Update & UpdateSlot) != 0)
                    {
                        UpperPort->content.color->Update = (UpperPort->content.color->Update & (~UpdateSlot));
                        VspGenVideoPackets = VspGenVideoPackets | AV_BIT_AV_INFO_FRAME;
                    }
                    break;
            }
        }
        /* Direct Connect, Keep Color Always Correct */
        else if(((SourcePort->content.video->AvailableVideoPackets & AV_BIT_AV_INFO_FRAME) != 0) &&
                (SourcePort->content.video->Y != port->content.video->Y))
        {
            port->content.video->Y     = SourcePort->content.video->Y;
            port->content.video->InCs  = SourcePort->content.video->InCs;
            DifferentVideoPackets      = DifferentVideoPackets | AV_BIT_AV_INFO_FRAME;
        }
        if((DifferentVideoPackets & AV_BIT_AV_INFO_FRAME) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_AV_INFO_FRAME,
                                      AV_PKT_AV_INFO_FRAME, AvVideoPacketType, AvRxUpdate);
        }
        else if((VspGenVideoPackets & AV_BIT_AV_INFO_FRAME) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_AV_INFO_FRAME,
                                      AV_PKT_AV_INFO_FRAME, AvVideoPacketType, AvRxForce);
        }
        if((DifferentVideoPackets & AV_BIT_SPD_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_SPD_PACKET,
                                      AV_PKT_SPD_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        if((DifferentVideoPackets & AV_BIT_HDR_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_HDR_PACKET,
                                      AV_PKT_HDR_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        if((DifferentVideoPackets & AV_BIT_EMP_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_EMP_PACKET,
                                      AV_PKT_EMP_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        if((DifferentVideoPackets & AV_BIT_MPEG_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_MPEG_PACKET,
                                      AV_PKT_MPEG_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        if((DifferentVideoPackets & AV_BIT_VS_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_VS_PACKET,
                                      AV_PKT_VS_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        else if((VspGenVideoPackets & AV_BIT_VS_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_VS_PACKET,
                                      AV_PKT_VS_PACKET, AvVideoPacketType, AvRxForce);
        }
        if((DifferentVideoPackets & AV_BIT_ISRC1_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_ISRC1_PACKET,
                                      AV_PKT_ISRC1_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        if((DifferentVideoPackets & AV_BIT_ISRC2_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_ISRC2_PACKET,
                                      AV_PKT_ISRC2_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        if((DifferentVideoPackets & AV_BIT_GMD_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_GMD_PACKET,
                                      AV_PKT_GMD_PACKET, AvVideoPacketType, AvRxUpdate);
        }
        if((DifferentVideoPackets & AV_BIT_GC_PACKET) != 0)
        {
            if((SourcePort->content.video->AvailableVideoPackets & AV_BIT_GC_PACKET) != 0)
            {
                AvUapiRxGetHdmiDeepColor(SourcePort);
                KfunTxSetColorDepth(SourcePort, port);
                AvUapiTxSetHdmiDeepColor(port);
                port->content.video->AvailableVideoPackets =
                    port->content.video->AvailableVideoPackets | AV_BIT_GC_PACKET;
                PktContent[0] = 1;
                AvUapiTxSetPacketContent(port, AV_PKT_GC_PACKET, PktContent, 1);
            }
            else
            {
                port->content.video->AvailableVideoPackets =
                    port->content.video->AvailableVideoPackets & (~AV_BIT_GC_PACKET);
                PktContent[0] = 0;
                AvUapiTxSetPacketContent(port, AV_PKT_GC_PACKET, PktContent, 1);
            }
        }
        if((SourcePort->content.rx->HdmiMode == 0) && (SourcePort->type == HdmiRx))
        {
            switch(UpperPort->type)
            {
                case VideoColor:
                    UpperPort->content.color->ColorInSpace = AV_Y2Y1Y0_RGB;
                    UpperPort->content.color->ColorInCs    = AV_CS_RGB;
                    port->content.video->Y     = AV_Y2Y1Y0_YCBCR_420;
                    port->content.video->InCs  = AV_CS_YUV_709;
                    break;
                case VideoScaler:
                    UpperPort->content.scaler->ColorSpace = AV_Y2Y1Y0_RGB;
                    port->content.video->Y     = AV_Y2Y1Y0_RGB;
                    port->content.video->InCs  = AV_CS_RGB;
                    break;
                default:
                    port->content.video->Y     = AV_Y2Y1Y0_RGB;
                    port->content.video->InCs  = AV_CS_RGB;
                    break;
            }
        }
        /* Call Silicon Layer */
        AvUapiTxVideoManage(port);
    }
    else
    {
        ret = AvNotAvailable;
    }
    return ret;
}

/**
 * @brief
 * function to Manage Audio in HDMI Tx
 * @return AvOk - success
 * @note
 */
AvRet KfunTxAudioManage(pout AvPort *port)
{
    AvRet ret = AvOk;
    uint16 DifferentAudioPackets = 0;
    AvPort *SourcePort = NULL;
    uint8 PktContent[40];
    uint8 TxAudioMute = 0;

    /* Don't check audio for DVI Tx Port */
    if((port->content.tx->HdmiMode == 0) || (port->core.HdmiCore == -1))
    {
        port->content.audio->AvailableAudioPackets = 0;
        return AvOk;
    }

    /* 1. Get Source information */
    /* if source does not exist, no need to update information at all */
    /* 2. if source exists, follow Rx's status, compare difference */
    /* Firstly carry bulk info from Front Port, then update the port */
    if(KfunFindAudioRxFront(port, &SourcePort) == AvOk)
    {
        ret = AvOk;
        if(SourcePort->type == LogicAudioRx)
        {
            AvUapiTxAudioManage(port);
            return AvOk;
        }
        /* FRL Stream Link Protection */
        if((SourcePort != NULL) && (SourcePort->type == HdmiRx) &&
           (SourcePort->content.rx->IsFreeRun == 0))
            return AvOk;

        DifferentAudioPackets = SourcePort->content.audio->AvailableAudioPackets ^
                                port->content.audio->AvailableAudioPackets;
        if(SourcePort->content.audio->Layout != port->content.audio->Layout)
            DifferentAudioPackets |= AV_BIT_AUDIO_CHANNEL_STATUS;
        if((DifferentAudioPackets & AV_BIT_AUDIO_CHANNEL_STATUS) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_AUDIO_CHANNEL_STATUS,
                                      AV_PKT_AUDIO_CHANNEL_STATUS, AvAudioPacketType, AvRxUpdate);
        }
        if((DifferentAudioPackets & AV_BIT_AUDIO_INFO_FRAME) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_AUDIO_INFO_FRAME,
                                      AV_PKT_AUDIO_INFO_FRAME, AvAudioPacketType, AvRxUpdate);
        }
        if((DifferentAudioPackets & AV_BIT_ACP_PACKET) != 0)
        {
            KfunTxfromRxPacketContent(SourcePort, port, PktContent, AV_BIT_ACP_PACKET,
                                      AV_PKT_ACP_PACKET, AvAudioPacketType, AvRxUpdate);
        }
        if((DifferentAudioPackets != 0) && (port->core.HdmiCore != -1))
        {
            TxAudioMute = port->content.audio->AudioMute;
            AvMemcpy(port->content.audio, SourcePort->content.audio, sizeof(AvAudio));
            port->content.audio->AudioMute = TxAudioMute;
            port->content.tx->Lock.AudioLock = SourcePort->content.rx->Lock.AudioLock;
            AvUapiTxSetAudioPackets(port);
        }
        /* Call Silicon Layer */
        AvUapiTxAudioManage(port);

    }
    else
    {
        ret = AvNotAvailable;
    }
    return ret;
}

/* lookup the connection to find which is the port before */
/**
 * @brief
 * function to Look Up From Port in the connection chain
 * @return AvOk - success
 * @note
 */
AvPort* KfunLookupFromPort(AvPort *port)
{
    if((port->type == LogicAudioRx) || (port->type == LogicAudioTx))
    {
        /* lookup audio port */
        return (AvPort*)(port->content.RouteAudioFromPort);
    }
    else
    {
        /* lookup video port */
        return (AvPort*)(port->content.RouteVideoFromPort);
    }
}

/**
 * @brief
 * function to Initialize Rx Routing Related Vars
 * @return AvOk - success
 * @note
 */
void KfunRxRoutingVarInit(pout AvPort *port)
{
    return;
}

void KfunRxRoutingClearEndInfo(pin AvPort *port)
{
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;

    while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
    {
        /* Check whether there are missing sinks or added sinks */
        if(CurrentPort->type == HdmiTx)
        {
            CurrentPort->content.video->AvailableVideoPackets = 0x0;
            CurrentPort->content.audio->AvailableAudioPackets = 0x0;
            CurrentPort->content.tx->ChangedVideoPackets = 0xffff;
            CurrentPort->content.tx->ChangedAudioPackets = 0xffff;
        }
        PrevPort = CurrentPort;
        CurrentPort = NULL;
    }

    return;
}

/**
 * @brief  Find next Tx End Audio Node in the signal chain
 * @return AvOk: Found CompareNode
 *         AvNotAvailable: Not Found CompareNode
 */
AvRet KfunFindAudioNextTxEnd(AvPort* CurrentNode, AvPort* *CompareNode, AvPort* *FoundNode)
{
    AvRet ret = AvNotAvailable;
    AvPort* TempNode = (AvPort*)CurrentNode->content.RouteAudioToPort;

    /* Loop through all nodes in the level */
    while(TempNode)
    {
        /* 1. First Node Contains Next Level */
        /* This TempNode will never be CompareNode */
        if(TempNode->content.RouteAudioToPort)
        {
            /* CompareNode is Found in the next level */
            if(KfunFindAudioNextTxEnd(TempNode, CompareNode, FoundNode) == AvOk)
            {
                /* Next Node is Found */
                if(*FoundNode)
                    return AvOk;
                /* CompareNode is Found, but NextNode is not Found */
                /* set CompareNode to be NULL, and return any End Port other than NULL in the next loop */
                else
                {
                    *CompareNode = NULL;
                    ret = AvOk;
                    TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                    continue;
                }
            }
            /* CompareNode is not Found, go to the next same level node */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                continue;
            }
        }

        /* 2. First Port Does Not Contain Next Level */
        else
        {
            /* 2.1 If CompareNode Found, Next Port is The Result */
            if(TempNode == *CompareNode)
            {
                *CompareNode = NULL;
                TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                if(!TempNode)
                    return AvNotAvailable;
                else
                {
                    ret = AvOk;
                    continue;
                }
            }
            /* 2.2 No Next Level Nodes available, the Node itself is an End */
            /* Return This Node as the wanted Node */
            else if((!(*CompareNode)) && (TempNode))
            {
                *FoundNode = TempNode;
                return AvOk;
            }
            /* 2.3 Both CompareNode and TempNode are not Found */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                continue;
            }
        }
    }
    /* If the port in this level is totally searched */
    return ret;
}

/**
 * @brief  Find all connected Audio Node in the signal chain
 * @return AvOk: Found CompareNode
 *         AvNotAvailable: Not Found CompareNode
 */
AvRet KfunFindAudioNextNode(AvPort* CurrentNode, AvPort* *CompareNode, AvPort* *FoundNode)
{
    AvPort* TempNode = (AvPort*)CurrentNode->content.RouteAudioToPort;

    /* Loop through all nodes in the level */
    while(TempNode)
    {
        /* 1. Feed the Current Node */
        /* Return This Node as the wanted Node */
        if((!(*CompareNode)) && (TempNode))
        {
            *FoundNode = TempNode;
            return AvOk;
        }
        /* 2. First Port Does Not Contain Next Level */
        else if(!(TempNode->content.RouteAudioToPort))
        {
            /* 2.1 If CompareNode Found, Next Port is The Result */
            if(TempNode == *CompareNode)
            {
                *CompareNode = NULL; /* indicate CompareNode is found */
                TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                continue;
            }
            /* 2.2 Both CompareNode and TempNode are not Found */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                continue;
            }
        }
        /* 3. First Node Contains Next Level */
        /* This TempNode will never be CompareNode */
        else
        {
            /* 3.1 The Parent Node is the Compare Node */
            if(TempNode == *CompareNode)
            {
                *CompareNode = NULL; /* indicate CompareNode is found */
            }
            /* 3.2 CompareNode is Found in the next level */
            if(KfunFindAudioNextNode(TempNode, CompareNode, FoundNode) == AvOk)
            {
                /* Next Node is Found */
                if(*FoundNode)
                    return AvOk;
                /* CompareNode is Found, but NextNode is not Found */
                /* set CompareNode to be NULL, and return any End Port other than NULL in the next loop */
                else
                {
                    *CompareNode = NULL;
                    TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                    continue;
                }
            }
            /* CompareNode is not Found, go to the next same level node */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteAudioNextSameLevelPort;
                continue;
            }
        }

    }
    /* If the port in this level is totally searched */
    return AvNotAvailable;
}

/**
 * @brief  Find Rx Front Audio Node in the signal chain
 * @return AvOk: Found CompareNode
 *         AvNotAvailable: Not Found CompareNode
 */
AvRet KfunFindAudioRxFront(AvPort* CurrentNode, AvPort* *FoundNode)
{
    AvRet ret = AvNotAvailable;
    AvPort* TempNode = (AvPort*)CurrentNode->content.RouteAudioFromPort;

    /* 1. No Front at all */
    if(!TempNode)
        return ret;

    /* 2. Loop through all nodes in the level */
    while(TempNode)
    {
        *FoundNode = TempNode;
        TempNode = (AvPort*)TempNode->content.RouteAudioFromPort;
        ret = AvOk;
    }

    return ret;
}

/**
 * @brief  Find next Tx End Video Node in the signal chain
 * @return AvOk: Found CompareNode
 *         AvNotAvailable: Not Found CompareNode
 */
AvRet KfunFindVideoNextTxEnd(AvPort* CurrentNode, AvPort* *CompareNode, AvPort* *FoundNode)
{
    AvRet ret = AvNotAvailable;
    AvPort* TempNode = (AvPort*)CurrentNode->content.RouteVideoToPort;

    /* Loop through all nodes in the level */
    while(TempNode)
    {
        /* 1. First Node Contains Next Level */
        /* This TempNode will never be CompareNode */
        if(TempNode->content.RouteVideoToPort)
        {
            /* CompareNode is Found in the next level */
            if(KfunFindVideoNextTxEnd(TempNode, CompareNode, FoundNode) == AvOk)
            {
                /* Next Node is Found */
                if(*FoundNode)
                    return AvOk;
                /* CompareNode is Found, but NextNode is not Found */
                /* set CompareNode to be NULL, and return any End Port other than NULL in the next loop */
                else
                {
                    *CompareNode = NULL;
                    ret = AvOk;
                    TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                    continue;
                }
            }
            /* CompareNode is not Found, go to the next same level node */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                continue;
            }
        }

        /* 2. First Port Does Not Contain Next Level */
        else
        {
            /* 2.1 If CompareNode Found, Next Port is The Result */
            if(TempNode == *CompareNode)
            {
                *CompareNode = NULL;
                TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                if(!TempNode)
                    return AvNotAvailable;
                else
                {
                    ret = AvOk;
                    continue;
                }
            }
            /* 2.2 No Next Level Nodes available, the Node itself is an End */
            /* Return This Node as the wanted Node */
            else if((!(*CompareNode)) && (TempNode))
            {
                *FoundNode = TempNode;
                return AvOk;
            }
            /* 2.3 Both CompareNode and TempNode are not Found */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                continue;
            }
        }
    }
    /* If the port in this level is totally searched */
    return ret;
}

/**
 * @brief  Find all connected Video Node in the signal chain
 * @return AvOk: Found CompareNode
 *         AvNotAvailable: Not Found CompareNode
 */
AvRet KfunFindVideoNextNode(AvPort* CurrentNode, AvPort* *CompareNode, AvPort* *FoundNode)
{
    AvPort* TempNode = (AvPort*)CurrentNode->content.RouteVideoToPort;

    /* Loop through all nodes in the level */
    while(TempNode)
    {
        /* 1. Feed the Current Node */
        /* Return This Node as the wanted Node */
        if((!(*CompareNode)) && (TempNode))
        {
            *FoundNode = TempNode;
            return AvOk;
        }
        /* 2. First Port Does Not Contain Next Level */
        else if(!(TempNode->content.RouteVideoToPort))
        {
            /* 2.1 If CompareNode Found, Next Port is The Result */
            if(TempNode == *CompareNode)
            {
                *CompareNode = NULL; /* indicate CompareNode is found */
                TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                continue;
            }
            /* 2.2 Both CompareNode and TempNode are not Found */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                continue;
            }
        }
        /* 3. First Node Contains Next Level */
        /* This TempNode will never be CompareNode */
        else
        {
            /* 3.1 The Parent Node is the Compare Node */
            if(TempNode == *CompareNode)
            {
                *CompareNode = NULL; /* indicate CompareNode is found */
            }
            /* 3.2 CompareNode is Found in the next level */
            if(KfunFindVideoNextNode(TempNode, CompareNode, FoundNode) == AvOk)
            {
                /* Next Node is Found */
                if(*FoundNode)
                    return AvOk;
                /* CompareNode is Found, but NextNode is not Found */
                /* set CompareNode to be NULL, and return any End Port other than NULL in the next loop */
                else
                {
                    *CompareNode = NULL;
                    TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                    continue;
                }
            }
            /* CompareNode is not Found, go to the next same level node */
            else
            {
                TempNode = (AvPort*)TempNode->content.RouteVideoNextSameLevelPort;
                continue;
            }
        }

    }
    /* If the port in this level is totally searched */
    return AvNotAvailable;
}

/**
 * @brief  Find Rx Front Video Node in the signal chain
 * @return AvOk: Found CompareNode
 *         AvNotAvailable: Not Found CompareNode
 */
AvRet KfunFindVideoRxFront(AvPort* CurrentNode, AvPort** FoundNode)
{
    AvRet ret = AvNotAvailable;
    AvPort* TempNode = (AvPort*)CurrentNode->content.RouteVideoFromPort;

    /* 1. No Front at all */
    if(!TempNode)
        return ret;

    /* 2. Loop through all nodes in the level */
    while(TempNode)
    {
        *FoundNode = TempNode;
        TempNode = (AvPort*)TempNode->content.RouteVideoFromPort;
        ret = AvOk;
    }

    return ret;
}

AvRet KfunCheckVspPortContent(pin AvPort *port, PacketType Pkt, uint8 *Content)
{
    AvRet ret = AvOk;
    AvPort *SourcePort;
    uint8 VicUpdateFlag = 0;

    SourcePort = (AvPort*)port->content.RouteVideoFromPort;
    while((SourcePort != NULL) &&
          ((SourcePort->type == VideoScaler) ||
           (SourcePort->type == VideoColor)))
    {
        switch(Pkt)
        {
            case AV_PKT_AV_INFO_FRAME:
            {
                switch(SourcePort->type)
                {
                    case VideoScaler:
                        KfunCheckVideoScaler(SourcePort);
                        if(VicUpdateFlag == 0)
                        {
                            if(SourcePort->content.scaler->ScalerOutVic != 0)
                            {
                                SET_AVIF_VIC(Content,SourcePort->content.scaler->ScalerOutVic);
                                port->content.video->timing.Vic = SourcePort->content.scaler->ScalerOutVic;
                                /* Deinterlacer */
                                if((port->content.video->timing.Vic == 2) || (port->content.video->timing.Vic == 17))
                                {
                                    SET_AVIF_PR(Content, 0);
                                }
                                VicUpdateFlag = 1;
                            }
                            else
                                SET_AVIF_VIC(Content,0);
                        }
                        if(SourcePort->content.scaler->ColorSpace == AV_Y2Y1Y0_YCBCR_420)
                        {
                            SourcePort->content.scaler->ScalerOutSpace = AV_Y2Y1Y0_YCBCR_444;
                        }
                        SET_AVIF_Y(Content,SourcePort->content.scaler->ScalerOutSpace);
                        SET_AVIF_C(Content,(SourcePort->content.scaler->ScalerOutCs & 0x03));
                        SET_AVIF_EC(Content,((SourcePort->content.scaler->ScalerOutCs>>2) & 0x07));
                        SET_AVIF_Q(Content,((SourcePort->content.scaler->ScalerOutCs>>6) & 0x03));
                        port->content.video->Y = SourcePort->content.scaler->ScalerOutSpace;
                        port->content.video->InCs = SourcePort->content.scaler->ScalerOutCs;
                        break;
                    case VideoColor:
                        KfunCheckVideoColor(SourcePort);
                        if(VicUpdateFlag == 0)
                        {
                            if(SourcePort->content.color->ColorInVic != 0)
                            {
                                SET_AVIF_VIC(Content,SourcePort->content.color->ColorInVic);
                                port->content.video->timing.Vic = SourcePort->content.color->ColorInVic;
                                VicUpdateFlag = 1;
                            }
                        }
                        SET_AVIF_Y(Content,SourcePort->content.color->ColorOutSpace);
                        SET_AVIF_C(Content,(SourcePort->content.color->ColorOutCs & 0x03));
                        SET_AVIF_EC(Content,((SourcePort->content.color->ColorOutCs>>2) & 0x07));
                        SET_AVIF_Q(Content,((SourcePort->content.color->ColorOutCs>>6) & 0x03));
                        port->content.video->Y = SourcePort->content.color->ColorOutSpace;
                        port->content.video->InCs = SourcePort->content.color->ColorOutCs;
                        break;
                }
                break;
            }// AV_PKT_AV_INFO_FRAME
            case AV_PKT_VS_PACKET:
            {
                break;
            }// AV_PKT_VS_PACKET
        }

        /* Find upper port in the chain list*/
        SourcePort = (AvPort*)SourcePort->content.RouteVideoFromPort;
    }
    return ret;
}

/**
 * @brief
 * function to update Port Structure from Video Packet
 * @return AvOk - success
 * @note
 */
AvRet KfunUpdatePortFromPacket(pin AvPort *port, PacketType Pkt, uint8 *Content)
{
    AvRet ret = AvOk;
    uint8 Value = 0;
    uint8 i = 0;
    uint8 AviC = 0, AviY = 0, AviQ = 0, AviEC = 0;
    uint8 ChanAlloc = 0;
    AvPort *FromPort = (AvPort*)(port->content.RouteVideoFromPort);

    switch (Pkt)
    {
        case AV_PKT_VS_PACKET:
            if((Content[4] == 0x46) && (Content[5] == 0xD0) && (Content[6] == 0x00))
                AvUapiOutputDebugMessage("Port[%d][%d]:DolbyVision Detected ", port->device->index, port->index);
            else if((Content[4] == 0x8b) && (Content[5] == 0x84) && (Content[6] == 0x90))
                AvUapiOutputDebugMessage("Port[%d][%d]:HDR10+ Detected ", port->device->index, port->index);
            else if((Content[4] == 0x03) && (Content[5] == 0x0C) && (Content[6] == 0x00))
            {
                /* 3D related update */
                port->content.video->info.Detect3D = GET_3D_STATUS(Content);
                port->content.video->info.Format3D = (Vid3DFormat)GET_3D_FORMAT(Content);
                if(port->content.video->info.Detect3D == 0)
                {
                    if((port->type == HdmiRx) && (port->content.rx->H2p1FrlStat == AV_FRL_TRAIN_IDLE) &&
                       ((port->content.video->info.TmdsFreq > 310) || (port->content.video->info.TmdsFreq < 280)))
                    {
                        SET_4K_FORMAT(Content, 0);
                        break;
                    }
                    Value = GET_4K_STATUS(Content);
                    if(Value == 1)
                    {
                        Value = GET_4K_FORMAT(Content);
                        switch(Value)
                        {
                            case 1: /* 4K30 */
                                port->content.video->timing.Vic = 95;
                                break;
                            case 2: /* 4K25 */
                                port->content.video->timing.Vic = 94;
                                break;
                            case 3: /* 4K24 */
                                port->content.video->timing.Vic = 93;
                                break;
                            case 4: /* 4K24 SMPTE */
                                port->content.video->timing.Vic = 98;
                                break;
                        }
                    }
                }
            }
            break;
        case AV_PKT_AV_INFO_FRAME:
            /* VIC */
            Value = GET_AVIF_VIC(Content);
            if(Value != 0)
            {
                port->content.video->timing.Vic = Value;
                AvUapiOutputDebugMessage("Port[%d][%d]:Get Vic = [%x]", port->device->index, port->index, Value);
            }
            if(port->type == HdmiRx)
                KfunRxVicFilter(port);
            /* Color Space */
            AviY  = GET_AVIF_Y(Content);
            AviEC = GET_AVIF_EC(Content);
            AviC  = GET_AVIF_C(Content);
            if(AviY == 0)
            {
                AviQ = GET_AVIF_Q(Content);
                if(AviC == 3)
                    AviQ = AviQ>>0;
                else
                    AviC = 0x00;
            }
            else
                AviQ  = GET_AVIF_YQ(Content);
            port->content.video->Y = (AvVideoY)AviY;
            /* Protect Cs Error From Y Information */
            if((port->content.video->Y != AV_Y2Y1Y0_RGB) && (AviC == 0x00))
                AviC = AviC | 0x02;
            port->content.video->InCs = (AvVideoCs)((AviQ<<6)+AviC);
            if(AviC == 3)
                port->content.video->InCs = (AvVideoCs)(port->content.video->InCs + (AviEC<<2));
            if(port->content.video->InCs == 0x00)
            {
                if(port->content.video->Y != AV_Y2Y1Y0_RGB)
                    port->content.video->InCs = AV_CS_YUV_709;
            }
            if(port->content.video->Y == AV_Y2Y1Y0_RGB)
            {
                if((port->content.video->InCs != AV_CS_DEFAULT_RGB) &&
                   (port->content.video->InCs != AV_CS_RGB) &&
                   (port->content.video->InCs != AV_CS_RGB) &&
                   (port->content.video->InCs != AV_CS_LIM_RGB) &&
                   (port->content.video->InCs != AV_CS_BT2020_YCC) &&
                   (port->content.video->InCs != AV_CS_BT2020_DFT_RGB) &&
                   (port->content.video->InCs != AV_CS_BT2020_RGB) &&
                   (port->content.video->InCs != AV_CS_AUTO))
                {
                    port->content.video->InCs = AV_CS_DEFAULT_RGB;
                }
            }
            if(FromPort != NULL)
            {
                if(FromPort->type != LogicVideoRx)
                    port->content.video->OutCs = port->content.video->InCs;
            }
            switch (GET_AVIF_M(Content))
            {
                case 1:
                    port->content.video->AspectRatio = AV_AR_4_3;
                    break;
                case 2:
                    port->content.video->AspectRatio = AV_AR_16_9;
                    break;
                default:
                    if (port->content.video->timing.Vic <= 107)
                        port->content.video->AspectRatio = ARTable[port->content.video->timing.Vic];
                    else
                        port->content.video->AspectRatio = AV_AR_NOT_INDICATED;
                    break;
            }
            /* Pixel Repetition */
            if(port->type == HdmiRx)
                port->content.video->PixelRepeatValue = GET_AVIF_PR(Content);
            break;
        case AV_PKT_SPD_PACKET:
            break;
        case AV_PKT_HDR_PACKET:
            break;
        case AV_PKT_AUDIO_INFO_FRAME:
            /*==================================================
             * Adjust CC field according to CA since TX device
             * uses CC to decide on number of channels to send
             *=================================================*/
            if (GET_AUDIF_CC(Content) == 0)
            {
                /* Get Input Audio Packet Type */
                if (port->content.audio->AudFormat == AV_AUD_IF_AUTO)
                {
                    switch (port->content.audio->AudType)
                    {
                        case AV_AUD_TYPE_HBR:
                            port->content.audio->AudFormat = AV_AUD_I2S;
                            break;
                        case AV_AUD_TYPE_DSD:
                            port->content.audio->AudFormat = AV_AUD_DSD_NORM;
                            break;
                        case AV_AUD_TYPE_DST:
                            port->content.audio->AudFormat = AV_AUD_DSD_DST;
                            break;
                        case AV_AUD_TYPE_ASP:
                        default:
                            port->content.audio->AudFormat = AV_AUD_I2S;
                            break;
                    }
                }

                /* Get Audio Channel Number */
                ChanAlloc = GET_AUDIF_CA(Content);
                if (ChanAlloc < 32)
                {
                    port->content.audio->ChanNum = ChanCount[ChanAlloc]+1;
                    SET_AUDIF_CC(Content, port->content.audio->ChanNum-1);
                }
            }
            else
            {
                port->content.audio->ChanNum = GET_AUDIF_CC(Content) + 1;
            }
            /* Get Audio Coding Type */
            Value = GET_AUDIF_CT(Content);
            if((Value == 0) && ((port->content.audio->AvailableAudioPackets & AV_BIT_AUDIO_CHANNEL_STATUS) == 0))
                Value = 1;
            if((Value != 0) && ((port->content.audio->Consumer & 0x02) != 0))
                port->content.audio->AudCoding = (AvAudioCoding)Value;
            break;
        case AV_PKT_MPEG_PACKET:
            break;
        case AV_PKT_ACP_PACKET:
            break;
        case AV_PKT_ISRC1_PACKET:
            break;
        case AV_PKT_ISRC2_PACKET:
            break;
        case AV_PKT_GMD_PACKET:
            break;
        case AV_PKT_AMD_PACKET:
            break;
        case AV_PKT_AUDIO_CHANNEL_STATUS:
            port->content.audio->Consumer   = GET_CHST_PROFF_APP(Content);
            port->content.audio->Copyright  = GET_CHST_COPYRIGHT(Content);
            port->content.audio->Emphasis   = GET_CHST_EMPHASIS(Content);
            port->content.audio->CatCode    = GET_CHST_CATG_CODE(Content);
            port->content.audio->SrcNum     = GET_CHST_SRC_NUM(Content);
            port->content.audio->ClkAccur   = GET_CHST_CLK_ACCUR(Content);
            port->content.audio->WordLen    = GET_CHST_4BIT_WORD_LEN(Content);
            port->content.audio->Layout     = Content[5];

            if((port->content.audio->Consumer & 0x2) == 0)
                port->content.audio->AudCoding = AV_AUD_FORMAT_LINEAR_PCM;
            else if(((Content[6] == 0x00) && (Content[7] == 0x01)) || /* Dolby AC-3 */
                    ((Content[6] == 0x00) && (Content[7] == 0x15)))   /* Dolby EAC-3 */
                port->content.audio->AudCoding = AV_AUD_FORMAT_AC3;
            else if((Content[6] == 0x00) && (Content[7] == 0x0B))
                port->content.audio->AudCoding = AV_AUD_FORMAT_DTS;
            else if((Content[6] == 0x02) && (Content[7] == 0x11))
                port->content.audio->AudCoding = AV_AUD_FORMAT_DTS_HD;
            else if((Content[6] == 0x00) && (Content[7] == 0x16))
                port->content.audio->AudCoding = AV_AUD_FORMAT_DOLBY_DIGITAL;
            else
                port->content.audio->AudCoding = AV_AUD_FORMAT_COMPRESSED_AUDIO;
            if(port->content.audio->ChanNum < 2)
                port->content.audio->ChanNum = 2;
            Value = GET_CHST_SRC_NUM(Content);
            if(Value != 0)
            {
                if((port->content.audio->ChanNum != Value) && (port->type == DpRx))
                    port->content.rx->ChangedAudioPackets |= AV_BIT_AUDIO_INFO_FRAME;
            }

            Value = GET_CHST_SAMP_FREQ(Content);
            i = LookupValue8((uchar *)ChannelStatusSfTable+1,(uchar)Value,0xff,3);
            if(port->content.audio->SampFreq != (AvAudioSampleFreq)ChannelStatusSfTable[i])
            {
                port->content.audio->SampFreq = (AvAudioSampleFreq)ChannelStatusSfTable[i];
                switch(port->content.audio->SampFreq)
                {
                    case AV_AUD_FS_32KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:32KHz Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_44KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:44KHz Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_48KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:48KHz Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_88KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:88KHz Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_96KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:96KHz Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_176KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:176KHz Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_192KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:192KHz Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_HBR:
                        AvUapiOutputDebugMessage("Port[%d][%d]:HBR Detected", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_FROM_STRM:
                        AvUapiOutputDebugMessage("Port[%d][%d]:Audio From Stream Detected", port->device->index, port->index);
                        break;
                }
            }
            break;
        default:
            ret = AvNotSupport;
            break;
    }

    return ret;
}

#if AvEnableCecFeature /* CEC Related */

/**
 * @brief
 * function to Validize Message Size
 * @return AvOk - success
 * @note
 */
AvRet ValidMsgSize (CecMessage Opcode, uint8 MsgSize)
{
    uint8 i;
    AvRet ret = AvOk;
    uint8 FixedLen = 0;

    switch(Opcode)
    {
        case AV_CEC_MSG_VENDOR_COMMAND:
            if ( MsgSize <= 2 )
            {
                ret = AvInvalidParameter;
            }
            break;
        case AV_CEC_MSG_VENDOR_REMOTE_BTN_DOWN:
            if ( MsgSize <= 2 )
            {
                ret = AvInvalidParameter;
            }
            break;
        case AV_CEC_MSG_SET_OSD_NAME:
            if ( MsgSize <= 2 )
            {
                ret = AvInvalidParameter;
            }
            break;
        case AV_CEC_MSG_SET_OSD_STRING:
            if (  MsgSize <= 3 )
            {
                ret = AvInvalidParameter;
            }
            break;
        case AV_CEC_MSG_TUNER_DEV_STATUS:
            ret = AvInvalidParameter;
            if ( ( MsgSize == 7) || (  MsgSize == 10))
            {
                ret = AvOk;
            }
            break;
        case AV_CEC_MSG_RECORD_ON:
            if (  MsgSize <= 3 )
            {
                ret = AvInvalidParameter;
            }
            break;
        case AV_CEC_MSG_SET_TIMER_PROG_TITLE:
            if ( MsgSize <= 2)
            {
                ret = AvInvalidParameter;
            }
            break;
        case AV_CEC_MSG_TIMER_STATUS:
            ret = AvInvalidParameter;
            if ( ( MsgSize == 3) || (  MsgSize == 5))
            {
                ret = AvOk;
            }
            break;
        case AV_CEC_MSG_VENDOR_CMD_WITH_ID:
            if (  MsgSize <= 5)
            {
                ret = AvInvalidParameter;
            }
            break;
        default:
            FixedLen = 1;
            for(i = 0; i < AV_FLEX_OP_CODES; i+=4 )
            {
                if (Opcode == FlexOpCodes[i])
                {
                    if (MsgSize < FlexOpCodes[i+3])
                    {
                        ret = AvInvalidParameter;
                    }
                    FixedLen = 0;
                }
            }
            break;
    }

    if ( FixedLen )
    {
        for (i=0; (i+1) < AV_CEC_OP_CODES; i+=2)
        {
            if (Opcode == CecOpCodes[i])
            {
                if (MsgSize < CecOpCodes[i+1])
                {
                    ret = AvInvalidParameter;
                }
                break;
            }
        }
    }
    else
    {
        AvUapiOutputDebugMessage("CEC: Msg size = %d", MsgSize);
        if(ret == AvInvalidParameter)
        {
            AvUapiOutputDebugMessage("CEC: Invalid msg size is %d,Opcode 0x%x.", MsgSize, Opcode);
            AvUapiOutputDebugMessage("CEC: take no action");
        }
    }
    return ret;
}

/**
 * @brief
 * function to Process Rx CEC Message
 * @return AvOk - success
 * @note
 */
AvRet KfunCecRxMsgProcess(AvPort *port)
{
    AvRet ret = AvNotAvailable;
    uint8* Msg = port->content.cec->RxContent;
    port->content.cec->RxMsg = (CecMessage)(AV_CEC_OPCODE(Msg));

    /* if(AvUapiGetCecStatus(port) != AvOk) */
    if(port->content.cec->RxGetFlag == 0)
        return AvNotAvailable;

    AvUapiOutputDebugMessage("CEC: InOp=0x%x,%d->%d,Data=%x",
                             port->content.cec->RxMsg, AV_CEC_SRC(Msg), AV_CEC_DST(Msg), Msg[2]);

    if (ValidMsgSize(port->content.cec->RxMsg, port->content.cec->RxLen) == AvOk)
    {
        switch (AV_CEC_OPCODE(Msg))
        {
            case AV_CEC_MSG_ROUTE_CHANGE:
                AvHandleEvent(port, AvEventCecMsgRouteChange, Msg, NULL);
                break;

            case AV_CEC_MSG_ROUTE_INFO:
                AvHandleEvent(port, AvEventCecMsgRouteInfo, Msg, NULL);
                break;

            case AV_CEC_MSG_ACTIVE_SRC:
                AvHandleEvent(port, AvEventCecMsgActiveSource, Msg, NULL);
                break;


            case AV_CEC_MSG_GIVE_PHYS_ADDR:
                AvHandleEvent(port, AvEventCecMsgGivePhyAddr, Msg, NULL);
                break;

            case AV_CEC_MSG_SET_STRM_PATH:
                AvHandleEvent(port, AvEventCecMsgSetStreamPath, Msg, NULL);
                break;

            case AV_CEC_MSG_STANDBY:
                AvHandleEvent(port, AvEventCecMsgStandby, Msg, NULL);
                break;

            case AV_CEC_MSG_ABORT:
                AvHandleEvent(port, AvEventCecMsgAbort, Msg, NULL);
                break;

            case AV_CEC_MSG_FEATURE_ABORT:
                AvHandleEvent(port, AvEventCecMsgFeatureAbort, Msg, NULL);
                break;

            case AV_CEC_MSG_GIVE_PWR_STATUS:
                AvHandleEvent(port, AvEventCecMsgGivePowerStatus, Msg, NULL);
                break;

            case AV_CEC_MSG_REPORT_PWR_STATUS:
                AvHandleEvent(port, AvEventCecMsgReportPowerStatus, Msg, NULL);
                break;

            case AV_CEC_MSG_GIVE_OSD_NAME:
                AvHandleEvent(port, AvEventCecMsgGiveOsdName, Msg, NULL);
                break;

            case AV_CEC_MSG_GET_VENDOR_ID:
                AvHandleEvent(port, AvEventCecMsgGetVendorId, Msg, NULL);
                break;

            case AV_CEC_MSG_GET_CEC_VERSION:
                AvHandleEvent(port, AvEventCecMsgGetCecVersion, Msg, NULL);
                break;

            case AV_CEC_MSG_REQUEST_ARC_INITIATION:
                AvKapiArcEnable(port, 1);
                /* port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE; */
                AvHandleEvent(port, AvEventCecMsgRequestArcInitiation, Msg, NULL);
                break;

            case AV_CEC_MSG_INITIATE_ARC:
                AvKapiArcEnable(port, 1);
                port->content.cec->EnableARC = AV_CEC_ARC_INITIATED;
                AvHandleEvent(port, AvEventCecSendReportArcInitiated, Msg, NULL);
                break;

            case AV_CEC_MSG_TERMINATE_ARC:
                AvKapiArcEnable(port, 0);
                port->content.cec->EnableARC = AV_CEC_ARC_TERMINATED;
                AvHandleEvent(port, AvEventCecSendReportArcTerminated, Msg, NULL);
                break;

            case AV_CEC_MSG_REPORT_ARC_INITIATED:
                AvKapiArcEnable(port, 1);
                port->content.cec->EnableARC = AV_CEC_ARC_INITIATED;
                AvHandleEvent(port, AvEventCecMsgReportArcInitiated, Msg, NULL);
                break;

            case AV_CEC_MSG_REQUEST_ARC_TERMINATION:
                AvKapiArcEnable(port, 0);
                /* port->content.cec->EnableARC = AV_CEC_ARC_TO_TERMINATE; */
                AvHandleEvent(port, AvEventCecMsgRequestArcTermination, Msg, NULL);
                break;

            case AV_CEC_MSG_REPORT_ARC_TERMINATED:
                AvKapiArcEnable(port, 0);
                port->content.cec->EnableARC = AV_CEC_ARC_TERMINATED;
                AvHandleEvent(port, AvEventCecMsgReportArcTerminated, Msg, NULL);
                break;

            case AV_CEC_MSG_REPORT_CURRENT_LATENCY:
                AvHandleEvent(port, AvEventCecMsgReportCurrentLatency, Msg, NULL);
                break;

            case AV_CEC_MSG_USER_CONTROL_PRESSED:
                AvHandleEvent(port, AvEventCecMsgUserControlPressed, Msg, NULL);
                break;

            case AV_CEC_MSG_USER_CONTROL_RELEASED:
                AvHandleEvent(port, AvEventCecMsgUserControlReleased, Msg, NULL);
                break;
            case AV_CEC_MSG_IMAGE_VIEW_ON:
                AvHandleEvent(port, AvEventCecMsgImageViewOn, Msg, NULL);
                break;

            case AV_CEC_MSG_TEXT_VIEW_ON:
                AvHandleEvent(port, AvEventCecMsgTextViewOn, Msg, NULL);
                break;

            case AV_CEC_MSG_SYSTEM_AUDIO_MODE_REQUEST:
                AvHandleEvent(port, AvEventCecMsgSystemAudioModeRequest, Msg, NULL);
                break;

            case AV_CEC_MSG_DECK_CONTROL:
                AvHandleEvent(port, AvEventCecMsgDeckControl, Msg, NULL);
                break;

            case AV_CEC_MSG_PLAY:
                AvHandleEvent(port, AvEventCecMsgPlay, Msg, NULL);
                break;

            case AV_CEC_MSG_GIVE_AUDIO_STATUS:
                AvHandleEvent(port, AvEventCecMsgGiveAudioStatus, Msg, NULL);
                break;

            case AV_CEC_MSG_GIVE_SYSTEM_AUDIO_MODE_STATUS:
                AvHandleEvent(port, AvEventCecMsgGiveSystemAudioModeStatus, Msg, NULL);
                break;

            case AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR:
                AvHandleEvent(port, AvEventCecMsgRequestShortAudioDescriptor, Msg, NULL);
                break;

            case AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS:
                AvHandleEvent(port, AvEventCecMsgSystemAudioModeStatus, Msg, NULL);
                break;

            case AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE:
                AvHandleEvent(port, AvEventCecMsgSetSystemAudioMode, Msg, NULL);
                break;

            case AV_CEC_MSG_REPORT_AUDIO_STATUS:
                AvHandleEvent(port, AvEventCecMsgReportAudioStatus, Msg, NULL);
                break;

            case AV_CEC_MSG_SET_AUDIO_RATE:
                AvHandleEvent(port, AvEventCecMsgSetAudioRate, Msg, NULL);
                break;

            case AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR:
                AvHandleEvent(port, AvEventCecMsgReportShortAudioDescriptor, Msg, NULL);
                break;

            case AV_CEC_MSG_REPORT_PHYS_ADDR:
                AvHandleEvent(port, AvEventCecMsgReportPhyAddr, Msg, NULL);
                break;

            case AV_CEC_MSG_MENU_REQUEST:
                AvHandleEvent(port, AvEventCecMsgMenuRequest, Msg, NULL);
                break;

            case AV_CEC_MSG_REQ_ACTIVE_SRC:
                AvHandleEvent(port, AvEventCecMsgRequestActiveSource, Msg, NULL);
                break;

            case AV_CEC_MSG_SET_MENU_LANGUAGE:
                AvHandleEvent(port, AvEventCecMsgSetMenuLanguage, Msg, NULL);
                break;

            case AV_CEC_MSG_DEVICE_VENDOR_ID:
                AvHandleEvent(port, AvEventCecMsgDeviceVendorId, Msg, NULL);
                break;

            default:
                if ((!AV_CEC_BROADCAST(Msg)) &&
                   (AV_CEC_OPCODE(Msg) != AV_CEC_MSG_CDC_MESSAGE) )
                {
                    AvHandleEvent(port, AvEventCecSendFeatureAbort, Msg, NULL);
                    /* SendFeatureAbort(Msg); */
                }
                else
                {
                    AvUapiOutputDebugMessage("CEC: Take no action");
                }
                break;
        }

    }
    else
    {
        switch (AV_CEC_OPCODE(Msg))
        {
            case AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR:
            if ( (!AV_CEC_BROADCAST(Msg)) && (port->content.cec->RxLen > 2 ))
                {
                    AvHandleEvent(port, AvEventCecSendFeatureAbortReason, Msg, NULL);
                    /* SendFeatureAbortReason(Msg,0); */
                }
                else
                {
                     AvUapiOutputDebugMessage("CEC: Take no action");
                }
                break;
            default:
                AvUapiOutputDebugMessage("CEC: Take no action");
                break;

        }

    }
    return ret;
}

/**
 * @brief
 * function to Send CEC Message
 * @return AvOk - success
 * @note
 */
AvRet KfunCecSendMessage(AvPort *port)
{
    AvRet ret = AvOk;
    port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
    AvUapiCecSendMessage(port);
    return ret;
}

/**
 * @brief
 * function to Set System Audio Mode
 * @return AvOk - success
 * @note
 */
void SendSetSystemAudioMode (AvPort* port, uint8 status)
{
    port->content.cec->TxLen=3;
    port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
    AV_CEC_SET_HDR_DA(port->content.cec->TxContent,
                   0x00,
                   port->content.cec->LogAddr);
    AV_CEC_SET_OPCODE(port->content.cec->TxContent,
                   AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE);
    port->content.cec->TxContent[2] = status;
    AvUapiCecSendMessage(port);
}

/**
 * @brief
 * function to perform logical address allocation
 * @return AvOk - success
 * @note
 */
AvRet CecAllocateLogAddr(AvPort* port)
{
    AvRet ret;

#if (AvCecLogicAddress == 0)
    if(*port->content.is_CecFsm != AvFsmTvTxLogAddr)
        return AvNotAvailable;
#else
    if(*port->content.is_CecFsm != AvFsmCecTxLogAddr)
        return AvNotAvailable;
#endif

    /*=========================================
     * Copy message parameters to local vars
     *========================================*/
    port->content.cec->TxLen = 1;
    port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;

    /*=========================================
     * Kick-off transmission
     *========================================*/
    port->content.cec->TxContent[0] = (port->content.cec->AddrIndex & 0xf) |
                                     ((port->content.cec->AddrIndex & 0xf) << 4);
    ret = AvUapiCecSendMessage(port);
    return ret;
}

/**
 * @brief
 * function to List all active logical addresses in the system
 * @return AvOk - success
 * @note
 */
AvRet KfunPrepareLogAddr (AvPort* port)
{
    if(port->content.cec->CecDelayExpire < TxCecManageThreshold)
    {
#if AvEnableEArcRxFeature
        if(port->content.earx->LinkStat == AV_EARCRX_ST_LINK_IDLE)
            return AvNotAvailable;
#endif
        port->content.cec->CecDelayExpire = port->content.cec->CecDelayExpire + 1;
        return AvNotAvailable;
    }

#if (AvCecLogicAddress == 0)
    if(port->content.cec->TxSendFlag == AV_CEC_TX_TO_SEND)
    {
        AvUapiOutputDebugMessage("CEC: Found TxSendFlag = 1");
        return AvOk;
    }
    else if((port->content.cec->TxSendFlag == AV_CEC_TX_WAIT_RESPONSE) ||
            (port->content.cec->TxSendFlag == AV_CEC_TX_SEND_SUCCESS) ||
            (port->content.cec->TxSendFlag == AV_CEC_TX_SEND_FAIL))
    {
        /* when there are CEC feedback information, process it */
        if(port->content.cec->TxReady)
        {
            port->content.cec->LogicAddrList |= (1<<(port->content.cec->AddrIndex));
            AvUapiOutputDebugMessage("CEC Port[%d][%d]: %d Exists",port->device->index,
                                     port->index, port->content.cec->AddrIndex);
        }
        else if((port->content.cec->ArbLost) || (port->content.cec->Timeout))
        {
            if(port->content.cec->RetryCount > 100) /* CEC_RETRY_COUNT = 100 */
            {
                port->content.cec->RetryCount = 0;
                CecAllocateLogAddr(port);
                AvUapiOutputDebugMessage("CEC Port[%d][%d]:Retry Address %d",port->device->index,
                                         port->index, port->content.cec->AddrIndex);
            }
            else
                port->content.cec->RetryCount++;
        }
    }
    else
    {
        if(CecAllocateLogAddr(port) == AvOk)
        {
            port->content.cec->RetryCount = 0;
            port->content.cec->NackCount  = 0;
        }
    }

#elif (AvCecLogicAddress == 5)
    uint8 ErrCode = 0;
    uint8 FoundLogAddr = 0;
    /* AvUapiOutputDebugMessage("CEC: TxSendFlag = %d.", port->content.cec->TxSendFlag); */

    if(port->content.cec->TxSendFlag == AV_CEC_TX_TO_SEND)
    {
        AvUapiOutputDebugMessage("CEC: Found TxSendFlag = 1");
        return AvOk;
    }
    else if((port->content.cec->TxSendFlag == AV_CEC_TX_WAIT_RESPONSE) ||
            (port->content.cec->TxSendFlag == AV_CEC_TX_SEND_SUCCESS) ||
            (port->content.cec->TxSendFlag == AV_CEC_TX_SEND_FAIL))
    {
        /* when there are CEC feedback information, process it */
        if(port->content.cec->TxReady ||
            port->content.cec->ArbLost ||
            port->content.cec->Timeout)
        {
            /* Clear the Send Flag */
            port->content.cec->TxSendFlag = AV_CEC_TX_IDLE;
            /*======================================
             * Acknowledged, Log address is taken
             *=====================================*/
            if (port->content.cec->TxReady)
            {
                port->content.cec->LogicAddrList |= (1<<(port->content.cec->AddrIndex));
                AvUapiOutputDebugMessage("CEC Port[%d][%d]:%d Exists", port->device->index,
                                         port->index, port->content.cec->AddrIndex);
                /* force 0x05 for ARC */
#if (AvCecLogicAddress == 5)
                if(port->content.cec->AddrIndex == 5)
                {
                    FoundLogAddr = port->content.cec->AddrIndex;
                    AvHandleEvent(port, AvEventCecLogAddrAlloc, &FoundLogAddr, NULL);
                }
#endif
                if(port->content.cec->AddrIndex < 15)
                    port->content.cec->AddrIndex++;
            }
            else if(port->content.cec->Timeout)
            {
                AvUapiGetNackCount(port);
                /*=============================================
                 * No response, logical addr available, exit
                 *============================================*/
                if (port->content.cec->NackCount == (3 + 1)) /* CEC_RETRY_COUNT = 3 */
                {
                    /* find the avaialbe addr */
                    FoundLogAddr = port->content.cec->AddrIndex;
                    AvHandleEvent(port, AvEventCecLogAddrAlloc, &FoundLogAddr, NULL);
                    AvUapiOutputDebugMessage("CEC Port[%d][%d]:%d Available",port->device->index,
                                             port->index, port->content.cec->AddrIndex);
                    if(port->content.cec->AddrIndex < 15)
                        port->content.cec->AddrIndex++;
                }
                else
                {
                    ErrCode = 0x10;                 /* Try same logical addr again */
                }
            }
            else if (port->content.cec->ArbLost)
            {
                ErrCode = 0x11;                     /* Try same logical addr again */
            }

            /*====================================
             * If need to resend
             *===================================*/
            if (ErrCode)
            {
                port->content.cec->RetryCount++;
                /* resend previous index again */
                if (port->content.cec->RetryCount > 3) /* CEC_RETRY_COUNT = 3 */
                {
                    port->content.cec->RetryCount = 0;
                    FoundLogAddr = port->content.cec->AddrIndex;
                    AvHandleEvent(port, AvEventCecLogAddrAlloc, &FoundLogAddr, NULL);
                    if(port->content.cec->AddrIndex < 15)
                        port->content.cec->AddrIndex++;
                    AvUapiOutputDebugMessage("CEC Port[%d][%d]:%d Available",port->device->index,port->index, port->content.cec->AddrIndex);
                }
                else
                {
                    CecAllocateLogAddr(port);
                }
            }
            else
            {
                port->content.cec->RetryCount = 0;
            }
        }
    }
    else
    {
        if(CecAllocateLogAddr(port) == AvOk)
        {
            port->content.cec->RetryCount = 0;
            port->content.cec->NackCount  = 0;
        }
    }
#endif

    return AvOk;
}

/**
 * @brief
 * function to Initialize CEC FSM
 * @return AvOk - success
 * @note
 */
AvRet KfunCecInit (AvPort* port)
{
    AvUapiTxCecInit(port);
    port->content.cec->CecDelayExpire = 0;
#if (AvEnableCecCts == 0)
    if(port->content.cec->EnableAudioAmplifier == AV_CEC_AMP_ENABLED)
        port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
    if(port->content.cec->EnableAudioAmplifier == AV_CEC_AMP_DISABLED)
        port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
    if(port->content.cec->EnableARC == AV_CEC_ARC_INITIATED)
        port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
#endif
    return AvOk;
}

/**
 * @brief
 * function to Set Physical Address
 * @return AvOk - success
 * @note
 */
AvRet KfunCecSetPhysicalAddr(pin AvPort *port)
{
    AvUapiTxCecSetPhysicalAddr(port);
    return AvOk;
}

/**
 * @brief
 * function to Set Logical Address
 * @return AvOk - success
 * @note
 */
AvRet KfunCecSetLogicalAddr(pin AvPort *port)
{
    AvUapiTxCecSetLogicalAddr(port);
    return AvOk;
}

AvRet KfunCecManage(AvPort *port)
{
    AvRet ret = AvOk;
    uint8 ActionFlag = 0;
    uint8 CurrentInput = port->content.cec->InputCount>>4;
    AvPort *CurrentRx = NULL;
    uint8 NewSpa[2];
    uint8 OldSpa[2];

    /* 1. Check CEC line is free */
    if((port->content.cec->TxSendFlag == AV_CEC_TX_WAIT_RESPONSE) ||
       (port->content.cec->TxSendFlag == AV_CEC_TX_TO_SEND))
        return AvNotAvailable;
    /* check tv setting */
    if(port->content.cec->LogAddr == AV_CEC_TV_LOG_ADDRESS)
    {
        AvHandleEvent(port, AvEventCecDefaultManage, NULL, NULL);
        return ret;
    }

    /* 2. front port exists */
    if(KfunFindVideoRxFront(port, &CurrentRx) == AvOk)
    {
        if(port->type == HdmiTx)
        {
            NewSpa[0] = (port->content.tx->PhyAddr)>>8;
            NewSpa[1] = (port->content.tx->PhyAddr)&0xff;
            OldSpa[0] = NewSpa[0];
            OldSpa[1] = NewSpa[1];
        }
        /* only act when input is stable */
        if(CurrentRx->content.rx->IsInputStable == 1)
        {
            /* 2.1 generate the phy for source */
            KfunGenerateSourceSpa(CurrentRx, NewSpa, 0);
            /* 2.2.1 source not indicated */
            if(CurrentInput == 0)
            {
                port->content.cec->InputCount = ((CurrentRx->index + 1)<<4) | port->content.cec->InputCount;
#if AvCecLinkageMode
                AvHandleEvent(port, AvEventCecSendRoutingInformation, NewSpa, NULL);
#endif
                ActionFlag = 1;
            }
            /* 2.2.2 new source */
            else if(CurrentInput != (CurrentRx->index + 1))
            {
                KfunGenerateSourceSpa(CurrentRx, OldSpa, CurrentInput);
#if AvCecLinkageMode
                AvHandleEvent(port, AvEventCecSendRoutingChange, OldSpa, NewSpa);
#endif
                port->content.cec->InputCount = ((CurrentRx->index + 1)<<4) | (port->content.cec->InputCount & 0x0f);
                ActionFlag = 1;
            }
        }
    }
    /* no rx */
    else
    {
        /* but already assigned CurrentInput */
        if(CurrentInput != 0)
        {
#if AvCecLinkageMode
            AvHandleEvent(port, AvEventCecSendInactiveSource, NULL, NULL);
#endif
            port->content.cec->InputCount = port->content.cec->InputCount & 0x0f;
            ActionFlag = 1;
        }
    }

    /* 3. AvEvent Processing */
    if(ActionFlag == 0)
    {
        AvHandleEvent(port, AvEventCecDefaultManage, NULL, NULL);
    }

    return ret;
}

#endif /* CEC Related */

AvRet KfunCecUpdateFromEdid(AvPort *port)
{
    uint8 Edid[AvEdidMaxSize];
    uint8 Spa[2] = {0x00,0x00};
    uint8 SpaLocation = 0x00;
    AvPort *SourcePort = NULL;

    /* Edid Already Read, return */
    if(port->content.tx->EdidReadSuccess == AV_EDID_UPDATED)
        return AvOk;

    if(port->type == HdmiTx)
    {
        AvUapiTxReadEdid(port, Edid, AvEdidMaxSize);
        /* Incorrect Edid, Return */
        if((Edid[6] != 0xFF) || (Edid[7] != 0x00))
        {
            AvUapiOutputDebugFsm("Port[%d][%d]:Edid Checksum Fail, Abort",port->device->index,port->index);
#if AvForceDefaultEdid
            port->content.tx->IgnoreEdidError = 1;
            KfunTxUseDefaultEdid(port);
#else
            port->content.tx->Hpd = AV_HPD_FORCE_LOW;
#endif
            return AvError;
        }
        SpaLocation = KfunFindCecSPAFromEdid(Edid, Spa);
        if((SpaLocation > 0x82) && (SpaLocation < 0xFE))
        {
            if((Spa[0] == 0) && (Spa[1] == 0))
                port->content.tx->PhyAddr = 0xFFFF;
            else
                port->content.tx->PhyAddr = (Spa[0]<<8) | Spa[1];
        }
        else
            port->content.tx->PhyAddr = 0x0000;
    #if AvEnableCecFeature /* CEC Related */
        AvKapiCecSetPhysicalAddr(port);
    #endif

        if(//((Spa[0] == 0) && (Spa[1] == 0)) ||
           (SpaLocation < 0x82) || (SpaLocation > 0xFE))
            port->content.tx->HdmiMode = 0;
        else
            port->content.tx->HdmiMode = 1;
    #if AvEdidSameNoAcknowledge
        if((Edid[127] != port->content.tx->EdidCks[0]) ||
           (Edid[255] != port->content.tx->EdidCks[1]))
        {
            port->content.tx->EdidCks[0] = Edid[127];
            port->content.tx->EdidCks[1] = Edid[255];
            AvHandleEvent(port, AvEventPortEdidReady, Edid, Spa);
        }
        else
        {
            if(KfunFindVideoRxFront(port, &SourcePort) == AvOk)
            {
                if((SourcePort->type == HdmiRx) &&
                   (SourcePort->content.rx->EdidStatus  != AV_EDID_UPDATED))
                    AvHandleEvent(port, AvEventPortEdidReady, Edid, Spa);
            }
        }
    #else
        AvHandleEvent(port, AvEventPortEdidReady, Edid, Spa);
    #endif
    }
    AvUapiTxSetHdmiModeSupport(port);
    AvUapiTxSetFeatureSupport(port);

    return AvOk;
}

/**
 * @brief
 * function to use Default EDID
 * @return AvOk - success
*/
AvRet KfunTxUseDefaultEdid(pin AvPort *port)
{
    /* Allow user to customize the code */
    AvHandleEvent(port, AvEventTxDefaultEdid, NULL, NULL);
    /* Force Tx to output HDMI mode when using default Edid */
    port->content.tx->HdmiMode = 1;
    AvUapiTxSetHdmiModeSupport(port);
    return AvOk;
}

AvRet KfunTxEdidError(pin AvPort *port)
{
    /* Allow user to customize the code */
    AvHandleEvent(port, AvEventPortEdidReadFail, NULL, NULL);
    return AvOk;
}

void KfunTxReadEdid(AvPort *port, uint8 *ReadData)
{
    AvUapiTxReadEdid(port, ReadData, AvEdidMaxSize);
}

void KfunRxWriteEdid(pio AvPort *port, uint8 *MergedEdid, uint8 SpaLocation, uint8 *SpaValue)
{
    uint16 len = 256;
    if(MergedEdid[0x7E] == 0x02)
        len = 384;
    else if(MergedEdid[0x7E] == 0x03)
        len = 512;
    /* HF-EEODB */
    if((MergedEdid[0x84] == 0xE2) && (MergedEdid[0x85] == 0x78))
    {
        if(MergedEdid[0x86] == 0x00)
            len = 256;
        else if(MergedEdid[0x86] == 0x01)
            len = 384;
        else
            len = 512;
    }
    AvUapiRxWriteEdid(port, MergedEdid, len);
    AvUapiRxSetSpa(port, SpaLocation, SpaValue);
    AvUapiRxEnableInternalEdid(port);
}

void KfunHdcp2p2Manage(pio AvPort *port)
{
#if AvEnableHdcp2p2Feature
    /* report some information about HDCP 2.2 although it is automatic */
    AvUapiRxHdcp2p2Manage(port);
#endif
}

uint8 KfunFindCecSPAFromEdid(uint8 *currptr, uint8 *spa)
{
    uint8 i = 0;
    uint8 offset = 0;
    uint8 DTDstart = 0;
    uint8 data1, data2, data3;
    uint32 vsdbid;
    uint8 ret = 0x01;

    /* Step 0. clear 2 bytes of SPA */
    spa[0] = 0xFF;
    spa[1] = 0xFF;
    /* Step 1. Check if the 128~255 bytes exist, return if not exist */
    if(currptr[0x7E] == 0)
        return ret;
    /* Step 2. check if 128~255 is a CEA block, only support version 3 */
    if((currptr[0x80] != 0x02) || (currptr[0x81] != 0x03))
        return ret;
    /* Step 3. find the DTD starter, SPA exists before DTD */
    if(currptr[0x82] >= 4)
        DTDstart = 0x80 + currptr[0x82];
    else
        DTDstart = 0xff;
    /* Step 4. set the offset of first data block */
    offset = 0x84;

    /* loop to find SPA */
    while(offset < DTDstart)
    {
        /* VSDB block found */
        if((currptr[offset]&0xe0) == 0x60)
        {
            data1 = currptr[offset+1];
            data2 = currptr[offset+2];
            data3 = currptr[offset+3];
            vsdbid = (data1) | (data2 << 8) | (data3 << 16);
            /* 24-bit IEEE Registeration */
            if (vsdbid != TAG_VSDB_IDENTIFIER)
            {
                i = currptr[offset]&0x1f;
                offset = offset + i + 1;
            }
            else
            {
                spa[0] = currptr[offset+4];
                spa[1] = currptr[offset+5];
                AvUapiOutputDebugMessage("SPA Found =%02x,%02x", spa[0], spa[1]);
                return (offset+4);
            }
        }
        /* other blocks or invalid blocks found */
        else
        {
            i = currptr[offset]&0x1f;
            offset = offset + i + 1;
        }
    }

    return ret;
}

void KfunGenerateSourceSpa(AvPort *port, uint8 *Spa, uint8 DedicateAddr)
{
    AvPort *DefaultPort = (AvPort*)port->device->port;
    uint8 NewAddr;
    /* 1. set inserted value: rule - index number + 1 */
    if(DedicateAddr == 0)
        NewAddr = 1 + (port->index&0x03) - (DefaultPort->index&0x03);
    else
        NewAddr = DedicateAddr;
    /* 2. find existed shift */
    /* 1.1 fully occupied */
    if((Spa[1] & 0x0f) != 0x00)
        return;
    /* 1.2 other cases */
    else if(Spa[1] & 0xf0)
        Spa[1] = Spa[1] | NewAddr;
    else if(Spa[0] & 0x0f)
        Spa[1] = NewAddr<<4;
    else if(Spa[0] & 0xf0)
        Spa[0] = Spa[0] | NewAddr;
    else
        Spa[0] = NewAddr<<4;

    return;
}

void KfunTxSetColorDepth(AvPort *RxPort, AvPort *TxPort)
{
    /* 1. Set Default Color Depth */
    TxPort->content.video->Cd = RxPort->content.video->Cd;
    /* 2. 12-bit process */
    if(RxPort->content.video->Cd == AV_CD_36)
    {
        /* 2.1 normal deep color 12-bit */
        if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_12B_DEEP_COLOR) == 0)
            TxPort->content.video->Cd = AV_CD_24;
        /* 2.2 clock divider required, 450M */
        else if(RxPort->content.video->info.TmdsFreq > 275)
        {
            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4G5) == 0)
                TxPort->content.video->Cd = AV_CD_24;
            else if((RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420) &&
                    ((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_36B420) == 0))
                TxPort->content.video->Cd = AV_CD_24;
        }
    }
    /* 3. 10-bit process */
    if(RxPort->content.video->Cd == AV_CD_30)
    {
        /* 3.1 normal deep color 10-bit */
        if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_10B_DEEP_COLOR) == 0)
            TxPort->content.video->Cd = AV_CD_24;
        /* 3.2 clock divider required, 375M */
        else if(RxPort->content.video->info.TmdsFreq > 275)
        {
            if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_3G75) == 0)
                TxPort->content.video->Cd = AV_CD_24;
            else if((RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420) &&
                    ((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_30B420) == 0))
                TxPort->content.video->Cd = AV_CD_24;
        }
    }
    AvUapiOutputDebugMessage("Port[%d][%d]:Color Depth = %d", TxPort->device->index, TxPort->index, TxPort->content.video->Cd);
}

void KfunTxSetHdmiModeSupport(pout AvPort *port)
{
    if(port->type == HdmiTx)
    {
        AvUapiTxSetHdmiModeSupport(port);
        port->content.video->AvailableVideoPackets =
            port->content.video->AvailableVideoPackets & (~AV_BIT_AV_INFO_FRAME);
    }
}

#if AvEnableEArcRxFeature /* eARC Rx Related */
void KfunEarcRxWriteCapData(pio AvPort *port, uint8 *MergedCapData)
{
    AvUapiEarcRxWriteCapData(port, MergedCapData, 256);
}

void KfunEarcRxManage(pin AvPort *port)
{
    AvRet ret;
    uint8 i = 0;
    uint8 value = 0;
    /* receive and manage the audio packet content */
    uint8 PktContent[24];
    EarcAudioInterrupt Intpt;

    if(port->content.earx->LinkStat != AV_EARCRX_ST_LINK_SUCCESS)
    {
        port->content.audio->AudLRclk = 0;
       return;
    }

    /* Step 1. Get the Packet Status */
    ret = AvUapiEarcRxGetAudioPacketStatus(port, &Intpt);
    /* Step 1.1 Clear Interrupt */
    AvUapiEarcRxClearAudioInterrupt(port, &Intpt);

    /* Step 2. Process it if existed */
    if(ret == AvOk)
    {
        if(Intpt.cBit)
        {
            AvUapiRxGetPacketContent(port, AV_PKT_EARC_CBIT, PktContent);
            KfunRxtoTxPacketContent(port, PktContent, AV_PKT_EARC_CBIT,
                                AV_PKT_EARC_CBIT, AvAudioPacketType);
            port->content.eaaud->AudioMute = (PktContent[5]>>2);
            /* 2.1 eARC audio format */
            port->content.earx->Fmt = (EarcFmt)(((PktContent[18]&0xF0)<<8) + (PktContent[23]&0x3B));
            switch(port->content.earx->Fmt)
            {
                case AV_EARC_RAW_LPCM_STEREO:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_LPCM_STEREO", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_LPCM_MULTI_CH2:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_LPCM_MULTI_CH2", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_LPCM_MULTI_CH8:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_LPCM_MULTI_CH8", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_LPCM_MULTI_CH16:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_LPCM_MULTI_CH16", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_LPCM_MULTI_CH32:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_LPCM_MULTI_CH32", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_FORAMT_DSD_CH6:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_FORAMT_DSD_CH6", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_FORAMT_DSD_CH12:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_FORAMT_DSD_CH12", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_COMPRESSED_A:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_COMPRESSED_A", port->device->index, port->index);
                    break;
                case AV_EARC_RAW_COMPRESSED_B:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx RAW_COMPRESSED_B", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_COMPRESSED_A:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_COMPRESSED_A", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_COMPRESSED_B:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_COMPRESSED_B", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_LPCM_MULTI_CH2:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_LPCM_MULTI_CH2", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_LPCM_MULTI_CH8:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_LPCM_MULTI_CH8", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_LPCM_MULTI_CH16:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_LPCM_MULTI_CH16", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_LPCM_MULTI_CH32:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_LPCM_MULTI_CH32", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_FORMAT_DSD_CH6:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_FORMAT_DSD_CH6", port->device->index, port->index);
                    break;
                case AV_EARC_ENC_FORMAT_DSD_CH12:
                    AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx ENC_FORMAT_DSD_CH12", port->device->index, port->index);
                    break;
                default:
                    break;
            }
            /* 2.2 Channel Status Management */
            value = (PktContent[20] & 0x0F);
            i = LookupValue8((uchar *)ChannelStatusSfTable+1,(uchar)value,0xff,3);
            if(port->content.eaaud->SampFreq != (AvAudioSampleFreq)ChannelStatusSfTable[i])
            {
                port->content.eaaud->SampFreq = (AvAudioSampleFreq)ChannelStatusSfTable[i];
                switch(port->content.eaaud->SampFreq)
                {
                    case AV_AUD_FS_32KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx 32KHz", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_44KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx 44KHz", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_48KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx 48KHz", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_88KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx 88KHz", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_96KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx 96KHz", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_176KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx 176KHz", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_192KHZ:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx 192KHz", port->device->index, port->index);
                        break;
                    case AV_AUD_FS_HBR:
                        AvUapiOutputDebugMessage("Port[%d][%d]:eARC Rx HBR", port->device->index, port->index);
                        break;
                }
            }
            /* 2.3 add detailed Audio Infoframe management */
            AvUapiEarcRxSetAudioInterface(port);
        }
        if(Intpt.uBit)
        {
            AvUapiRxGetPacketContent(port, AV_PKT_EARC_UBIT, PktContent);
        }
        if(Intpt.vBit)
        {
            AvUapiRxGetPacketContent(port, AV_PKT_EARC_VBIT, PktContent);
        }
    }
}

#endif
#if AvEnableDisplayPortRx
void KfunDpDPCDInit(AvPort *port)
{
    AvUapiDpRxDPCDInit(port);
}
#endif

int32 KfunDevMeasureTj(AvDevice *device)
{
    int32 Tj;
    AvUapiDevMeasureTj(device, &Tj);
    return Tj;
}

#if AvEnableDetailTiming
/* Timing Declaration */
typedef struct
{
    uint8      Vic;
    uint8      Interlaced;
    uint16     HActive;
    uint16     VActive;
    uint16     HTotal;
    uint16     VTotal;
    uint8      VFreq;
} AvTimTables;
/************************ Cea Timing Starts Here ************************/
/*
  AvCeaTables:Vic,HActive,VActive,Interlaced,HTotal,HBlank,VTotal,VBlank,VFreq
*/
const AvTimTables CeaTimingTables[] = {
    { 20,1,1920,1080,2640,1125,50},//1080i50
    { 21,1, 720, 576, 864, 625,50},//576i50
    {  5,1,1920,1080,2200,1125,60},//1080i60
    {  6,1, 720, 480, 858, 525,60},//480i60
    { 19,0,1280, 720,1980, 750,50},//720p50
    {  4,0,1280, 720,1650, 750,60},//720p60
    { 31,0,1920,1080,2640,1125,50},//1080p50
    { 16,0,1920,1080,2200,1125,60},//1080p60
    { 93,0,3840,2160,5500,2250,24},//4k24
    { 94,0,3840,2160,5280,2250,25},//4k25
    { 95,0,3840,2160,4400,2250,30},//4k30
    { 98,0,4096,2160,5500,2250,24},//4k24 smpte
    { 96,0,3840,2160,5280,2250,50},//4k50
    {101,0,4096,2160,5280,2250,50},//4k50 smpte
    { 97,0,3840,2160,4400,2250,60},//4k60
    {102,0,4096,2160,4400,2250,60},//4k60 smpte
    {117,0,3840,2160,5280,2250,100},//4k100
    {218,0,4096,2160,5280,2250,100},//4k100 smpte
    {118,0,3840,2160,4400,2250,120},//4k120
    {219,0,4096,2160,4400,2250,120},//4k120 smpte
    {121,0,5120,2160,7500,2200,24},//5k24
    {122,0,5120,2160,7200,2200,25},//5k25
    {123,0,5120,2160,6000,2200,30},//5k30
    {125,0,5120,2160,6600,2250,50},//5k50
    {126,0,5120,2160,5500,2250,60},//5k60
    {194,0,7680,4320,11000,4500,24},//8k24
    {195,0,7680,4320,10800,4400,25},//8k25
    {196,0,7680,4320, 9000,4400,30},//8k30
    {198,0,7680,4320,10800,4400,50},//8k50
    {199,0,7680,4320, 9000,4400,60},//8k60
    {210,0,10240,4320,12500,4950,24},//10k24
    {211,0,10240,4320,13500,4400,25},//10k25
    {212,0,10240,4320,11000,4500,30},//10k30
    {214,0,10240,4320,13500,4400,50},//10k50
    {215,0,10240,4320,11000,4500,60},//10k60
    {255,0,  255, 255,    0,   0, 0}
};
/************************ Cea Timing Ends Here  ************************/

/************************ Vesa Timing Starts Here ************************/
/*
  AvVesaTables: Vic,VTotal,VActive,VBlank,VFront,VSync,VBack,HTotal,HActive,HBlank,HFront,HSync,HBack
*/

const AvTimTables VesaTimingTables[] = {
    {255,0,  255, 255,    0,  0, 0}
};
/************************  Vesa Timing Ends Here  ************************/
#endif

void KfunRxVicFilter(pin AvPort *port)
{
#if AvEnableDetailTiming
    uint8 Value = 0;
    uint16 i = 0;
    /* 1. Check Vesa Timing */
    for(i=0;VesaTimingTables[i].Vic != 255;i++)
    {
        if((VesaTimingTables[i].VTotal  == port->content.video->timing.VTotal) &&
           (VesaTimingTables[i].VActive == port->content.video->timing.VActive) &&
           (((VesaTimingTables[i].HTotal  == port->content.video->timing.HTotal) &&
             (VesaTimingTables[i].HActive == port->content.video->timing.HActive)) ||
            ((VesaTimingTables[i].HTotal  == port->content.video->timing.HTotal*2) &&
             (VesaTimingTables[i].HActive == port->content.video->timing.HActive*2))))
        {
            port->content.video->timing.Vic = VesaTimingTables[i].Vic;
            Value = 1;
            break;
        }
    }
    /* 2. Check Cea Timing */
    if(Value == 0)
    {
        for(i=0;CeaTimingTables[i].Vic != 255;i++)
        {
            if((CeaTimingTables[i].VTotal  == port->content.video->timing.VTotal) &&
               (CeaTimingTables[i].VActive == port->content.video->timing.VActive) &&
               (((CeaTimingTables[i].HTotal  == port->content.video->timing.HTotal) &&
                 (CeaTimingTables[i].HActive == port->content.video->timing.HActive)) ||
                ((CeaTimingTables[i].HTotal  == port->content.video->timing.HTotal*2) &&
                 (CeaTimingTables[i].HActive == port->content.video->timing.HActive*2))) &&
               (CeaTimingTables[i].Interlaced == port->content.video->timing.Interlaced) &&
               ((CeaTimingTables[i].VFreq   == port->content.video->timing.FrameRate) ||
                (CeaTimingTables[i].VFreq   == (port->content.video->timing.FrameRate - 2)) ||
                (CeaTimingTables[i].VFreq   == (port->content.video->timing.FrameRate - 1)) ||
                (CeaTimingTables[i].VFreq   == (port->content.video->timing.FrameRate + 1))))
            {
                port->content.video->timing.Vic = CeaTimingTables[i].Vic;
                Value = 1;
                break;
            }
        }
    }
    /* 3. Correct Incorrect Vic */
    if(Value == 0)
    {
        for(i=0;VesaTimingTables[i].Vic != 255;i++)
        {
            if(port->content.video->timing.Vic == VesaTimingTables[i].Vic)
                port->content.video->timing.Vic = 0;
        }
        for(i=0;CeaTimingTables[i].Vic != 255;i++)
        {
            if(port->content.video->timing.Vic == CeaTimingTables[i].Vic)
                port->content.video->timing.Vic = 0;
        }
    }

    AvUapiOutputDebugFsm("Port[%d][%d]: Filtered Vic = %x,%d", port->device->index, port->index, port->content.video->timing.Vic,port->content.video->timing.FrameRate);
#endif
    return;
}
