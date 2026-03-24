/**
 * @file kernel_fsm.c
 *
 * @brief kernel fsm functions

 *        this file is used to keep the port structure function run following the protocol
 *
 */
#include "kernel_fsm.h"
void AvFsmFunPlugTxFsm(AvPort *port)
{
    TxVars* tx = port->content.tx;
    HdcpTx* hdcp = port->content.hdcptx;
    bool guard1 = false;
    bool guard2 = false;
    bool guard3 = false;
    bool guard4 = false;
    bool guard5 = false;
    bool guard6 = false;
    guard1 = false;
    guard2 = false;
    guard3 = false;
    guard4 = false;
    guard5 = false;
    guard6 = false;
    switch (*port->content.is_PlugTxFsm)
    {
      case AvFsmPlugTxDefault:
        *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
        KfunPtVarInit(port);
        KfunPtResetPort(port);
        KfunTxSetMuteAv(port);
        break;
      case AvFsmPlugTxDefaultEdid:
        *port->content.is_PlugTxFsm = AvFsmPlugTxEdidManage;
        break;
      case AvFsmPlugTxEdid:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
            KfunPtVarInit(port);
            KfunPtResetPort(port);
            KfunTxSetMuteAv(port);
        }
        else if (port->content.tx->EdidReadSuccess == AV_EDID_FAIL)
        {
            if (tx->IgnoreEdidError == 0)
            {
                *port->content.is_PlugTxFsm = AvFsmPlugTxEdidError;
                KfunTxEdidError(port);
            }
            else if (tx->IgnoreEdidError == 1)
            {
                guard1 = true;
            }
            else
            {
                guard5 = true;
            }
        }
        else
        {
            guard5 = true;
        }
        break;
      case AvFsmPlugTxEdidError:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
            KfunPtVarInit(port);
            KfunPtResetPort(port);
            KfunTxSetMuteAv(port);
        }
        break;
      case AvFsmPlugTxEdidManage:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            KfunTxSinkLost(port);
            *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
            KfunPtVarInit(port);
            KfunPtResetPort(port);
            KfunTxSetMuteAv(port);
        }
        else if ((tx->Lock.PllLock == 1) && (tx->EdidReadSuccess ==
                  AV_EDID_UPDATED))
        {
            *port->content.is_PlugTxFsm = AvFsmPlugTxTransmitVideo;
        }
        else
        {
            KfunCecUpdateFromEdid(port);
        }
        break;
      case AvFsmPlugTxEnableTxCore:
        *port->content.is_PlugTxFsm = AvFsmPlugTxEdid;
        break;
      case AvFsmPlugTxHdcp:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            if (tx->Hpd == AV_HPD_FORCE_LOW)
            {
                tx->Hpd = AV_HPD_HIGH;
                guard2 = true;
            }
            else
            {
                KfunTxSinkLost(port);
                *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
                KfunPtVarInit(port);
                KfunPtResetPort(port);
                KfunTxSetMuteAv(port);
            }
        }
        else if ((hdcp->Authenticated != 0U) || (hdcp->Hdcp2p2TxRunning != 0U))
        {
            *port->content.is_PlugTxFsm = AvFsmPlugTxStable;
            KfunTxClearMuteAv(port);
        }
        else if (tx->Lock.PllLock == 0)
        {
            guard2 = true;
        }
        else
        {
            KfunTxHdcpManage(port);
        }
        break;
      case AvFsmPlugTxHpdAntiDither:
        if (tx->HpdDelayExpire >= TxHpdDelayExpireThreshold)
        {
            *port->content.is_PlugTxFsm = AvFsmPlugTxEnableTxCore;
            KfunPtEnableTxCore(port);
        }
        else if (tx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
            KfunPtVarInit(port);
            KfunPtResetPort(port);
            KfunTxSetMuteAv(port);
        }
        else
        {
            KfunPtAntiDither(port);
        }
        break;
      case AvFsmPlugTxReset:
        if (tx->Hpd == AV_HPD_HIGH)
        {
            *port->content.is_PlugTxFsm = AvFsmPlugTxHpdAntiDither;
        }
        else
        {
            tx->Hpd = AV_HPD_LOW;
        }
        break;
      case AvFsmPlugTxStable:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            if (tx->Hpd == AV_HPD_FORCE_LOW)
            {
                tx->Hpd = AV_HPD_HIGH;
                guard3 = true;
            }
            else
            {
                KfunTxSinkLost(port);
                *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
                KfunPtVarInit(port);
                KfunPtResetPort(port);
                KfunTxSetMuteAv(port);
            }
        }
        else if (tx->Lock.PllLock == 0)
        {
            guard3 = true;
        }
        else
        {
            KfunTxVideoManage(port);
            KfunTxAudioManage(port);
            KfunTxHdcpManage(port);
        }
        break;
      case AvFsmPlugTxTransmitVideo:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            if (tx->Hpd == AV_HPD_FORCE_LOW)
            {
                tx->Hpd = AV_HPD_HIGH;
                guard4 = true;
            }
            else
            {
                KfunTxSinkLost(port);
                *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
                KfunPtVarInit(port);
                KfunPtResetPort(port);
                KfunTxSetMuteAv(port);
            }
        }
        else if (tx->InfoReady >= TxVideoManageThreshold)
        {
            if (hdcp->HdcpEnabled == 1)
            {
                *port->content.is_PlugTxFsm = AvFsmPlugTxHdcp;
            }
            else if (hdcp->HdcpEnabled == 0)
            {
                *port->content.is_PlugTxFsm = AvFsmPlugTxStable;
                KfunTxClearMuteAv(port);
            }
            else
            {
                guard6 = true;
            }
        }
        else
        {
            guard6 = true;
        }
        break;
      default:
        if ((bool)tx->Lock.PllLock)
        {
            KfunPtEnableTxCore(port);
            *port->content.is_PlugTxFsm = AvFsmPlugTxTransmitVideo;
        }
        else
        {
            if (tx->Hpd != AV_HPD_HIGH)
            {
                if (tx->Hpd == AV_HPD_FORCE_LOW)
                {
                    tx->Hpd = AV_HPD_HIGH;
                    *port->content.is_PlugTxFsm =
                        AvFsmPlugTxVideoUnlocked;
                    KfunTxUnlockProtection(port);
                }
                else
                {
                    KfunTxSinkLost(port);
                    *port->content.is_PlugTxFsm = AvFsmPlugTxReset;
                    KfunPtVarInit(port);
                    KfunPtResetPort(port);
                    KfunTxSetMuteAv(port);
                }
            }
        }
        break;
    }
    if (guard6)
    {
        if (tx->Lock.PllLock == 0)
        {
            guard4 = true;
        }
        else
        {
            KfunTxVideoManage(port);
            KfunTxAudioManage(port);
        }
    }
    if (guard5)
    {
        if (tx->EdidReadSuccess != AV_EDID_RESET)
        {
            if (tx->IgnoreEdidError == 1)
            {
                guard1 = true;
            }
            else
            {
                if (tx->IgnoreEdidError == 0)
                {
                    *port->content.is_PlugTxFsm =
                        AvFsmPlugTxEdidManage;
                }
            }
        }
    }
    if (guard4)
    {
        *port->content.is_PlugTxFsm = AvFsmPlugTxVideoUnlocked;
        KfunTxUnlockProtection(port);
    }
    if (guard3)
    {
        *port->content.is_PlugTxFsm = AvFsmPlugTxVideoUnlocked;
        KfunTxUnlockProtection(port);
    }
    if (guard2)
    {
        *port->content.is_PlugTxFsm = AvFsmPlugTxVideoUnlocked;
        KfunTxUnlockProtection(port);
    }
    if (guard1)
    {
        *port->content.is_PlugTxFsm = AvFsmPlugTxDefaultEdid;
        KfunTxUseDefaultEdid(port);
    }
}
void AvFsmFunPlugRxFsm(AvPort *port)
{
    RxVars* rx = port->content.rx;
    bool guard1 = false;
    bool guard2 = false;
    guard1 = false;
    guard2 = false;
    switch (*port->content.is_PlugRxFsm)
    {
      case AvFsmPlugRxDefault:
        *port->content.is_PlugRxFsm = AvFsmPlugRxReset;
        KfunPrVarInit(port);
        KfunPrResetPort(port);
        KfunPrHpdDown(port);
        break;
      case AvFsmPlugRxDetect:
        if (!(rx->Input5V != 0U))
        {
            guard2 = true;
        }
        else if ((bool)rx->Reset)
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxReset;
            KfunPrVarInit(port);
            KfunPrResetPort(port);
            KfunPrHpdDown(port);
        }
        else if (rx->EdidStatus != AV_EDID_UPDATED)
        {
            guard2 = true;
        }
        else if ((rx->Hpd == AV_HPD_HIGH) && (rx->Lock.PllLock != 0U))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxPlugged;
        }
        else
        {
            KfunManageHpa(port);
        }
        break;
      case AvFsmPlugRxInfoUpdate:
        if (!(rx->Lock.PllLock != 0U))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxDetect;
            KfunManageHpa(port);
        }
        else
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxStable;
            rx->IsFreeRun = 1;
        }
        break;
      case AvFsmPlugRxInputLock:
        if ((bool)rx->Reset)
        {
            KfunRxRoutingClearEndInfo(port);
            *port->content.is_PlugRxFsm = AvFsmPlugRxReset;
            KfunPrVarInit(port);
            KfunPrResetPort(port);
            KfunPrHpdDown(port);
        }
        else if (!(rx->Lock.PllLock != 0U))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxDetect;
            KfunManageHpa(port);
        }
        else if (rx->IsInputStable == 0)
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxPlugged;
        }
        else if ((rx->EdidStatus != AV_EDID_UPDATED) || (rx->Hpd != AV_HPD_HIGH))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxPullDownHpd;
            KfunPrHpdDown(port);
        }
        else if (rx->EdidStatus == AV_EDID_UPDATED)
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxReadTiming;
            KfunStdiReadStatus(port);
        }
        else
        {
        }
        break;
      case AvFsmPlugRxPlugged:
        if (!(rx->Lock.PllLock != 0U))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxDetect;
            KfunManageHpa(port);
        }
        else if ((rx->EdidStatus != AV_EDID_UPDATED) || (rx->Hpd != AV_HPD_HIGH))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxPullDownHpd;
            KfunPrHpdDown(port);
        }
        else
        {
            if (rx->IsInputStable == 1)
            {
                *port->content.is_PlugRxFsm = AvFsmPlugRxInputLock;
            }
        }
        break;
      case AvFsmPlugRxPullDownHpd:
        if ((rx->Input5V != 0U) && (rx->EdidStatus == AV_EDID_UPDATED))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxDetect;
            KfunManageHpa(port);
        }
        else
        {
            KfunPrepareEdid(port);
        }
        break;
      case AvFsmPlugRxReadTiming:
        if (!(rx->Lock.PllLock != 0U))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxDetect;
            KfunManageHpa(port);
        }
        else
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxInfoUpdate;
            KfunRxReadInfo(port);
        }
        break;
      case AvFsmPlugRxReset:
        if (!(rx->Reset != 0U))
        {
            *port->content.is_PlugRxFsm = AvFsmPlugRxPullDownHpd;
            KfunPrHpdDown(port);
        }
        break;
      default:
        if ((bool)rx->Reset)
        {
            KfunRxRoutingClearEndInfo(port);
            rx->IsFreeRun = 0;
            *port->content.is_PlugRxFsm = AvFsmPlugRxReset;
            KfunPrVarInit(port);
            KfunPrResetPort(port);
            KfunPrHpdDown(port);
        }
        else if ((rx->EdidStatus != AV_EDID_UPDATED) || (rx->Input5V == 0))
        {
            KfunRxRoutingClearEndInfo(port);
            KfunPrResetPort(port);
            rx->IsFreeRun = 0;
            *port->content.is_PlugRxFsm = AvFsmPlugRxPullDownHpd;
            KfunPrHpdDown(port);
        }
        else if ((rx->Hpd == AV_HPD_LOW) || (rx->Hpd == AV_HPD_FORCE_LOW) ||
                 (rx->Hpd == AV_HPD_TOGGLE))
        {
            KfunRxRoutingClearEndInfo(port);
            guard1 = true;
        }
        else if (!(rx->Lock.PllLock != 0U))
        {
            guard1 = true;
        }
        else
        {
            if ((rx->Lock.EqLock == 0) && (rx->Lock.PllLock == 1))
            {
                rx->IsFreeRun = 0;
                *port->content.is_PlugRxFsm = AvFsmPlugRxInputLock;
            }
        }
        break;
    }
    if (guard2)
    {
        *port->content.is_PlugRxFsm = AvFsmPlugRxPullDownHpd;
        KfunPrHpdDown(port);
    }
    if (guard1)
    {
        rx->IsFreeRun = 0;
        *port->content.is_PlugRxFsm = AvFsmPlugRxDetect;
        KfunManageHpa(port);
    }
}
void AvFsmFunReceiverFsm(AvPort *port)
{
    RxVars* rx = port->content.rx;
    bool guard1 = false;
    bool guard2 = false;
    bool guard3 = false;
    guard1 = false;
    guard2 = false;
    guard3 = false;
    switch (*port->content.is_ReceiverFsm)
    {
      case AvFsmRxDefault:
        *port->content.is_ReceiverFsm = AvFsmRxReset;
        KfunRxVarInit(port);
        break;
      case AvFsmRxDetect:
        if (rx->Reset == 1)
        {
            *port->content.is_ReceiverFsm = AvFsmRxReset;
            KfunRxVarInit(port);
        }
        else if (((rx->IsInputStable == 0) && (rx->EnableFreeRun != 0U)) ||
                 (rx->ForceFreeRun != 0U))
        {
            *port->content.is_ReceiverFsm = AvFsmRxFreerun;
            KfunRxEnableFrun(port);
        }
        else
        {
            if (rx->IsInputStable == 1)
            {
                *port->content.is_ReceiverFsm = AvFsmRxReceiving;
                KfunRxClearFlags(port);
            }
        }
        break;
      case AvFsmRxFreerun:
        if (rx->EnableFreeRun == 0)
        {
            KfunRxDisableFrun(port);
            *port->content.is_ReceiverFsm = AvFsmRxDetect;
        }
        else if (rx->Reset == 1)
        {
            KfunRxDisableFrun(port);
            *port->content.is_ReceiverFsm = AvFsmRxReset;
            KfunRxVarInit(port);
        }
        else
        {
            if ((rx->IsInputStable != 0U) && (!(rx->ForceFreeRun != 0U)))
            {
                KfunRxDisableFrun(port);
                *port->content.is_ReceiverFsm = AvFsmRxReceiving;
                KfunRxClearFlags(port);
            }
        }
        break;
      case AvFsmRxReceiving:
        if ((bool)rx->Reset)
        {
            guard1 = true;
        }
        else if ((bool)rx->ForceFreeRun)
        {
            guard2 = true;
        }
        else if (!(rx->IsInputStable != 0U))
        {
            if ((bool)rx->EnableFreeRun)
            {
                guard2 = true;
            }
            else if (!(rx->ForceFreeRun != 0U))
            {
                guard1 = true;
            }
            else
            {
                guard3 = true;
            }
        }
        else
        {
            guard3 = true;
        }
        break;
      default:
        if (rx->Reset == 0)
        {
            *port->content.is_ReceiverFsm = AvFsmRxDetect;
        }
        break;
    }
    if (guard3)
    {
        KfunRxVideoManage(port);
        KfunRxAudioManage(port);
    }
    if (guard2)
    {
        *port->content.is_ReceiverFsm = AvFsmRxFreerun;
        KfunRxEnableFrun(port);
    }
    if (guard1)
    {
        *port->content.is_ReceiverFsm = AvFsmRxReset;
        KfunRxVarInit(port);
    }
}
void AvFsmEnterFunPlugTxFsm(AvPort *port)
{
    *port->content.is_PlugTxFsm = AvFsmPlugTxDefault;
    KfunPtPortInit(port);
}
void AvFsmFunCecFsm(AvPort *port)
{
#if AvEnableCecFeature
    #if (AvCecLogicAddress == 0)
    AvFsmFunTvFsm(port);
    return;
    #else
    AvCec* cec = port->content.cec;
    TxVars* tx = port->content.tx;
    bool guard1 = false;
    guard1 = false;
    switch (*port->content.is_CecFsm)
    {
      case AvFsmCecAudioControl:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        else
        {
            *port->content.is_CecFsm = AvFsmCecAudioManage;
        }
        break;
      case AvFsmCecAudioFormat:
        *port->content.is_CecFsm = AvFsmCecAudioManage;
        break;
      case AvFsmCecAudioManage:
        if (cec->CecEnable == 0)
        {
            *port->content.is_CecFsm = AvFsmCecDefault;
        }
        else if (tx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if ((cec->EnableAudioAmplifier == AV_CEC_AMP_TO_ENABLE) ||
                    (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_DISABLE))
            {
                *port->content.is_CecFsm = AvFsmCecCmdSystemAudioModetoAll;
            }
            else
            {
                if ((cec->RxGetFlag == 1) && (cec->RxMsg ==
                        AV_CEC_MSG_USER_CONTROL_PRESSED))
                {
                    *port->content.is_CecFsm = AvFsmCecAudioControl;
                }
                else
                {
                    if ((cec->RxGetFlag == 1) && (cec->RxMsg ==
                            AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR))
                    {
                        *port->content.is_CecFsm =
                            AvFsmCecAudioFormat;
                    }
                    else
                    {
                        if ((cec->EnableARC == AV_CEC_ARC_TO_TERMINATE) ||
                                (cec->EnableARC == AV_CEC_ARC_TO_INITIATE) ||
                                ((cec->RxGetFlag == 1) && ((cec->RxMsg ==
                                AV_CEC_MSG_REQUEST_ARC_INITIATION) ||
                                (cec->RxMsg ==
                                 AV_CEC_MSG_REQUEST_ARC_TERMINATION))))
                        {
                            *port->content.is_CecFsm =
                                AvFsmCecCmdAudioARC;
                            cec->ARCTryCount = 0;
                        }
                        else
                        {
                            KfunCecManage(port);
                        }
                    }
                }
            }
        }
        break;
      case AvFsmCecCmdARC:
        if ((tx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if ((cec->RxGetFlag == 1) && ((cec->RxMsg ==
                    AV_CEC_MSG_REPORT_ARC_INITIATED) || (cec->RxMsg ==
                    AV_CEC_MSG_REPORT_ARC_TERMINATED)))
            {
                *port->content.is_CecFsm = AvFsmCecFunctionalDefault;
            }
            else
            {
                AvHandleEvent(port, AvEventCecArcManage, NULL, NULL);
            }
        }
        break;
      case AvFsmCecCmdActiveSource:
        if ((tx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if (cec->TxSendFlag == AV_CEC_TX_SEND_SUCCESS)
            {
                *port->content.is_CecFsm = AvFsmCecCmdVendorID;
                AvHandleEvent(port, AvEventCecSendDeviceVendorID, NULL, NULL);
            }
        }
        break;
      case AvFsmCecCmdAudioARC:
        if ((cec->EnableAudioAmplifier == AV_CEC_AMP_TO_ENABLE) ||
                (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_DISABLE))
        {
            *port->content.is_CecFsm = AvFsmCecCmdSystemAudioModetoAll;
        }
        else
        {
            if ((cec->RxGetFlag == 1) && ((cec->RxMsg ==
                    AV_CEC_MSG_REPORT_ARC_INITIATED) || (cec->RxMsg ==
                    AV_CEC_MSG_REPORT_ARC_TERMINATED)))
            {
                *port->content.is_CecFsm = AvFsmCecAudioManage;
            }
            else if ((tx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
            {
                *port->content.is_CecFsm = AvFsmCecNotConnected;
                KfunCecInit(port);
            }
            else
            {
                AvHandleEvent(port, AvEventCecArcManage, NULL, NULL);
            }
        }
        break;
      case AvFsmCecCmdSystemAudioModetoAll:
        if (tx->Hpd != AV_HPD_HIGH)
        {
            AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToAll, NULL,
                          NULL);
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        else if (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_ENABLE)
        {
            cec->EnableAudioAmplifier = AV_CEC_AMP_ENABLED;
            guard1 = true;
        }
        else if (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_DISABLE)
        {
            cec->EnableAudioAmplifier = AV_CEC_AMP_DISABLED;
            guard1 = true;
        }
        else if (cec->EnableAudioAmplifier == AV_CEC_AMP_ENABLED)
        {
            guard1 = true;
        }
        else
        {
            KfunCecManage(port);
        }
        break;
      case AvFsmCecCmdVendorID:
        if ((tx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if (cec->TxSendFlag == AV_CEC_TX_SEND_SUCCESS)
            {
                *port->content.is_CecFsm = AvFsmCecFunctionalDefault;
            }
        }
        break;
      case AvFsmCecDefault:
        if (cec->CecEnable == 1)
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        break;
      case AvFsmCecFunctionalDefault:
        if (cec->CecEnable == 0)
        {
            *port->content.is_CecFsm = AvFsmCecDefault;
        }
        else if (tx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
#if AvEnableEArcRxFeature
        else if (port->content.earx->LinkStat == AV_EARCRX_ST_LINK_SUCCESS)
        {
            cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
            *port->content.is_CecFsm = AvFsmCecCmdSystemAudioModetoAll;
        }
        else if (port->content.earx->LinkStat == AV_EARCRX_ST_LINK_FAIL)
#else
        else
#endif
        {
            if (((cec->RxGetFlag == 1) && (cec->RxMsg ==
                    AV_CEC_MSG_SYSTEM_AUDIO_MODE_REQUEST)) ||
                    ((cec->EnableAudioAmplifier == AV_CEC_AMP_TO_ENABLE) ||
                     (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_DISABLE)))
            {
                *port->content.is_CecFsm = AvFsmCecCmdSystemAudioModetoAll;
            }
            else
            {
                if ((cec->EnableARC == AV_CEC_ARC_TO_TERMINATE) ||
                        (cec->EnableARC == AV_CEC_ARC_TO_INITIATE) ||
                        ((cec->RxGetFlag == 1) && ((cec->RxMsg ==
                        AV_CEC_MSG_REQUEST_ARC_INITIATION) || (cec->RxMsg ==
                        AV_CEC_MSG_REQUEST_ARC_TERMINATION))))
                {
                    *port->content.is_CecFsm = AvFsmCecCmdARC;
                    cec->ARCTryCount = 0;
                }
                else
                {
                    if (cec->EnableAudioAmplifier == AV_CEC_AMP_ENABLED)
                    {
                        AvHandleEvent(port,
                                      AvEventCecSendSetSystemAudioModeToAll,
                                      NULL, NULL);
                        *port->content.is_CecFsm =
                            AvFsmCecAudioManage;
                    }
                    else
                    {
                        KfunCecManage(port);
                    }
                }
            }
        }
        break;
      case AvFsmCecIdle:
        if (cec->CecEnable == 0)
        {
            *port->content.is_CecFsm = AvFsmCecDefault;
        }
        else if (cec->AddrIndex != 15)
        {
            *port->content.is_CecFsm = AvFsmCecReset;
        }
        else if (tx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_CecFsm = AvFsmCecNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if (tx->PhyAddr != 0)
            {
                *port->content.is_CecFsm = AvFsmCecCmdActiveSource;
                AvHandleEvent(port, AvEventCecSendReportPhysAddress, NULL, NULL);
            }
        }
        break;
      case AvFsmCecNotConnected:
        if (cec->CecEnable == 0)
        {
            cec->TxSendFlag = AV_CEC_TX_IDLE;
            cec->AddrIndex = 0;
            *port->content.is_CecFsm = AvFsmCecDefault;
        }
        else
        {
            if ((tx->Hpd == AV_HPD_HIGH) && (tx->PhyAddr != 0) &&
                    (tx->EdidReadSuccess == AV_EDID_UPDATED))
            {
                cec->TxSendFlag = AV_CEC_TX_IDLE;
                cec->AddrIndex = 0;
                *port->content.is_CecFsm = AvFsmCecReset;
            }
        }
        break;
      case AvFsmCecReset:
        if (cec->CecEnable == 0)
        {
            *port->content.is_CecFsm = AvFsmCecDefault;
        }
        else if (cec->AddrIndex != 15)
        {
            *port->content.is_CecFsm = AvFsmCecTxLogAddr;
            cec->LogicAddrList = 0;
            cec->AddrIndex = 5;
        }
        else if (cec->AddrIndex == 15)
        {
            *port->content.is_CecFsm = AvFsmCecIdle;
            cec->DstAddr = 0x00;
        }
        else
        {
            if (tx->Hpd != AV_HPD_HIGH)
            {
                *port->content.is_CecFsm = AvFsmCecNotConnected;
                KfunCecInit(port);
            }
        }
        break;
      default:
        if ((cec->AddrIndex == 15) && (cec->TxSendFlag == 0))
        {
            *port->content.is_CecFsm = AvFsmCecReset;
        }
        else
        {
            KfunPrepareLogAddr(port);
        }
        break;
    }
    if (guard1)
    {
        AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToAll, NULL, NULL);
        *port->content.is_CecFsm = AvFsmCecAudioManage;
    }
    #endif
#endif
}
void AvFsmEnterFunPlugRxFsm(AvPort *port)
{
    *port->content.is_PlugRxFsm = AvFsmPlugRxDefault;
}
void AvFsmFunTvFsm(AvPort *port)
{
#if AvEnableCecFeature
    #if (AvCecLogicAddress == 0)
    AvCec* cec = port->content.cec;
    RxVars* rx = port->content.rx;
    bool guard1 = false;
    guard1 = false;
    switch (*port->content.is_CecFsm)
    {
      case AvFsmTvAudioModeStatus:
        if (cec->TxSendFlag == AV_CEC_TX_SEND_SUCCESS)
        {
            if (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_ENABLE)
            {
                *port->content.is_CecFsm = AvFsmTvCmdARCActive;
                cec->ARCTryCount = 0;
                AvHandleEvent(port, AvEventCecSendRequestArcInitiation, NULL,
                              NULL);
            }
            else if (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_DISABLE)
            {
                *port->content.is_CecFsm = AvFsmTvCmdARCInactive;
                cec->ARCTryCount = 0;
                AvHandleEvent(port, AvEventCecSendRequestArcTermination, NULL,
                              NULL);
            }
            else
            {
                guard1 = true;
            }
        }
        else
        {
            guard1 = true;
        }
        break;
      case AvFsmTvCmdARCActive:
        if ((rx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        else if (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_DISABLE)
        {
            *port->content.is_CecFsm = AvFsmTvAudioModeStatus;
            cec->ARCTryCount = 0;
            AvHandleEvent(port, AvEventCecSendSystemAudioModeRequest, NULL, NULL);
        }
        else
        {
            KfunCecManage(port);
            AvHandleEvent(port, AvEventCecArcManage, NULL, NULL);
        }
        break;
      case AvFsmTvCmdARCInactive:
        if (cec->EnableAudioAmplifier == AV_CEC_AMP_TO_ENABLE)
        {
            *port->content.is_CecFsm = AvFsmTvAudioModeStatus;
            cec->ARCTryCount = 0;
            AvHandleEvent(port, AvEventCecSendSystemAudioModeRequest, NULL, NULL);
        }
        else if ((rx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        else
        {
            KfunCecManage(port);
            AvHandleEvent(port, AvEventCecArcManage, NULL, NULL);
        }
        break;
      case AvFsmTvCmdGivePhysicalAddress:
        if ((rx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if (((cec->RxGetFlag == 1) && (cec->RxMsg ==
                    AV_CEC_MSG_REPORT_PHYS_ADDR)) || ((cec->RxGetFlag == 1) &&
                    (cec->RxMsg == AV_CEC_MSG_FEATURE_ABORT) && (cec->RxLen >= 3)
                 && (cec->RxContent[2] == AV_CEC_MSG_GIVE_PHYS_ADDR)))
            {
                *port->content.is_CecFsm = AvFsmTvCmdVendorID;
                AvHandleEvent(port, AvEventCecSendGiveVendorId, NULL, NULL);
            }
            else
            {
            }
        }
        break;
      case AvFsmTvCmdVendorID:
        if ((rx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if (((cec->RxGetFlag == 1) && ((cec->RxMsg ==
                    AV_CEC_MSG_DEVICE_VENDOR_ID) || (cec->RxMsg ==
                    AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE) || (cec->RxMsg ==
                    AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS) || (cec->RxMsg ==
                    AV_CEC_MSG_INITIATE_ARC) || (cec->RxMsg ==
                    AV_CEC_MSG_TERMINATE_ARC))) || ((cec->RxGetFlag == 1) &&
                    (cec->RxMsg == AV_CEC_MSG_FEATURE_ABORT) && (cec->RxLen >= 3)
                 && (cec->RxContent[2] == AV_CEC_MSG_GET_VENDOR_ID)))
            {
                *port->content.is_CecFsm = AvFsmTvFunctionalDefault;
                AvHandleEvent(port, AvEventCecSendGiveSystemAudioModeStatus,
                              NULL, NULL);
            }
            else
            {
            }
        }
        break;
      case AvFsmTvDefault:
        if (cec->CecEnable == 1)
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        break;
      case AvFsmTvFunctionalDefault:
        if (cec->CecEnable == 0)
        {
            *port->content.is_CecFsm = AvFsmTvDefault;
        }
        else if (rx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        else
        {
            if ((cec->RxGetFlag == 1) && (cec->RxMsg ==
                    AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS))
            {
                *port->content.is_CecFsm = AvFsmTvAudioModeStatus;
                cec->ARCTryCount = 0;
                AvHandleEvent(port, AvEventCecSendSystemAudioModeRequest, NULL,
                              NULL);
            }
            else
            {
                KfunCecManage(port);
            }
        }
        break;
      case AvFsmTvIdle:
        if (cec->CecEnable == 0)
        {
            *port->content.is_CecFsm = AvFsmTvDefault;
        }
        else if (cec->AddrIndex != 5)
        {
            *port->content.is_CecFsm = AvFsmTvReset;
        }
        else if (rx->Hpd != AV_HPD_HIGH)
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        else
        {
            *port->content.is_CecFsm = AvFsmTvCmdGivePhysicalAddress;
            AvHandleEvent(port, AvEventCecSendGivePhyAddr, NULL, NULL);
        }
        break;
      case AvFsmTvNotConnected:
        if (cec->CecEnable == 0)
        {
            cec->TxSendFlag = AV_CEC_TX_IDLE;
            cec->AddrIndex = 0;
            cec->LogicAddrList = 0;
            cec->LogAddr = 0;
            KfunCecSetLogicalAddr(port);
            *port->content.is_CecFsm = AvFsmTvDefault;
        }
        else
        {
            if (rx->Hpd == AV_HPD_HIGH)
            {
                cec->TxSendFlag = AV_CEC_TX_IDLE;
                cec->AddrIndex = 0;
                cec->LogicAddrList = 0;
                cec->LogAddr = 0;
                KfunCecSetLogicalAddr(port);
                *port->content.is_CecFsm = AvFsmTvReset;
            }
        }
        break;
      case AvFsmTvReset:
        if (cec->CecEnable == 0)
        {
            *port->content.is_CecFsm = AvFsmTvDefault;
        }
        else if (cec->AddrIndex != 5)
        {
            *port->content.is_CecFsm = AvFsmTvTxLogAddr;
            cec->LogicAddrList = 0;
            cec->AddrIndex = 5;
            cec->RetryCount = 0;
        }
        else if (cec->AddrIndex == 5)
        {
            *port->content.is_CecFsm = AvFsmTvIdle;
            cec->DstAddr = 0x05;
        }
        else
        {
            if (rx->Hpd != AV_HPD_HIGH)
            {
                *port->content.is_CecFsm = AvFsmTvNotConnected;
                KfunCecInit(port);
            }
        }
        break;
      default:
        if (cec->LogicAddrList != 0)
        {
            cec->RetryCount = 0;
            *port->content.is_CecFsm = AvFsmTvReset;
        }
        else
        {
            KfunPrepareLogAddr(port);
        }
        break;
    }
    if (guard1)
    {
        if ((rx->Hpd != AV_HPD_HIGH) || (cec->CecEnable == 0))
        {
            *port->content.is_CecFsm = AvFsmTvNotConnected;
            KfunCecInit(port);
        }
        else
        {
            KfunCecManage(port);
        }
    }
    #endif
#endif
}
void AvFsmEnterFunReceiverFsm(AvPort *port)
{
    *port->content.is_ReceiverFsm = AvFsmRxDefault;
    KfunRxInit(port);
}
void AvFsmEnterFunCecFsm(AvPort *port)
{
#if AvEnableCecFeature
    #if (AvCecLogicAddress == 0)
    *port->content.is_CecFsm = AvFsmTvDefault;
    #else
    *port->content.is_CecFsm = AvFsmCecDefault;
    #endif
#endif
}
