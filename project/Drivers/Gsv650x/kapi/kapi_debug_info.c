#include "kernel_fsm.h"
#include "kapi.h"
#include "uapi.h"

void PrintPlugTxFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_PlugTxFsm != OldState)
    {
        switch (*Port->content.is_PlugTxFsm)
        {
          case AvFsmPlugTxEdidError:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxEdidError", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxDefault:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxDefault", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxDefaultEdid:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxDefaultEdid", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxEdid:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxEdid", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxEdidManage:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxEdidManage", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxEnableTxCore:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxEnableTxCore", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxHdcp:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxHdcp", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxHpdAntiDither:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxHpdAntiDither", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxReset:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxReset", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxStable:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxStable", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxTransmitVideo:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxTransmitVideo", Port->device->index, Port->index);
            break;
          case AvFsmPlugTxVideoUnlocked:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugTxVideoUnlocked", Port->device->index, Port->index);
            break;
        }
    }
}

void PrintPlugRxFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_PlugRxFsm != OldState)
    {
        switch (*Port->content.is_PlugRxFsm)
        {
          case AvFsmPlugRxDefault:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxDefault", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxDetect:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxDetect", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxInfoUpdate:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxInfoUpdate", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxInputLock:
              AvHandleEvent(Port, AvEventPortUpStreamConnected, NULL, NULL);
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxInputLock", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxPlugged:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxPlugged", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxPullDownHpd:
              if(Port->content.rx->Input5V == 0)
                  AvHandleEvent(Port, AvEventPortUpStreamDisconnected, NULL, NULL);
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxPullDownHpd", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxReadTiming:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxReadTiming", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxReset:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxReset", Port->device->index, Port->index);
            break;
          case AvFsmPlugRxStable:
              AvUapiOutputDebugFsm("Port[%d][%d]:PlugRxStable", Port->device->index, Port->index);
            break;
        }
    }
}

void PrintReceiverFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_ReceiverFsm != OldState)
    {
        switch (*Port->content.is_ReceiverFsm)
        {
          case AvFsmRxDefault:
            AvUapiOutputDebugFsm("Port[%d][%d]:RxDefault", Port->device->index, Port->index);
            break;
          case AvFsmRxDetect:
            AvUapiOutputDebugFsm("Port[%d][%d]:RxDetect", Port->device->index, Port->index);
            break;
          case AvFsmRxFreerun:
            AvUapiOutputDebugFsm("Port[%d][%d]:RxFreerun", Port->device->index, Port->index);
            break;
          case AvFsmRxReceiving:
            AvUapiOutputDebugFsm("Port[%d][%d]:RxReceiving", Port->device->index, Port->index);
            break;
          case AvFsmRxReset:
            AvUapiOutputDebugFsm("Port[%d][%d]:RxReset", Port->device->index, Port->index);
            break;
        }
    }
}

#if AvEnableCecFeature /* CEC Related */
void PrintCecFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_CecFsm != OldState)
    {
        switch (*Port->content.is_CecFsm)
        {
#if (AvCecLogicAddress != 0)
          case AvFsmCecDefault:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecDefault", Port->device->index, Port->index);
            break;
          case AvFsmCecIdle:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecIdle", Port->device->index, Port->index);
            break;
          case AvFsmCecNotConnected:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecNotConnected", Port->device->index, Port->index);
            break;
          case AvFsmCecReset:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecReset", Port->device->index, Port->index);
            break;
          case AvFsmCecTxLogAddr:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecTxLogAddr", Port->device->index, Port->index);
            break;
          case AvFsmCecAudioControl:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecAudioControl", Port->device->index, Port->index);
            break;
          case AvFsmCecAudioFormat:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecAudioFormat", Port->device->index, Port->index);
            break;
          case AvFsmCecAudioManage:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecAudioManage", Port->device->index, Port->index);
            break;
          case AvFsmCecCmdARC:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecCmdARC", Port->device->index, Port->index);
            break;
          case AvFsmCecCmdAudioARC:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecCmdAudioARC", Port->device->index, Port->index);
            break;
          case AvFsmCecCmdSystemAudioModetoAll:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecCmdSystemAudioModetoAll", Port->device->index, Port->index);
            break;
          case AvFsmCecFunctionalDefault:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecFunctionalDefault", Port->device->index, Port->index);
            break;
          case AvFsmCecCmdActiveSource:
            AvUapiOutputDebugFsm("Port[%d][%d]:CecCmdActiveSource", Port->device->index, Port->index);
            break;
#else
          case AvFsmTvCmdGivePhysicalAddress:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvCmdGivePhysicalAddress", Port->device->index, Port->index);
            break;
          case AvFsmTvFunctionalDefault:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvFunctionalDefault", Port->device->index, Port->index);
            break;
          case AvFsmTvAudioModeStatus:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvAudioModeStatus", Port->device->index, Port->index);
            break;
          case AvFsmTvCmdARCActive:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvCmdARCActive", Port->device->index, Port->index);
            break;
          case AvFsmTvCmdARCInactive:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvCmdARCInactive", Port->device->index, Port->index);
            break;
          case AvFsmTvCmdVendorID:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvCmdVendorID", Port->device->index, Port->index);
            break;
          case AvFsmTvDefault:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvDefault", Port->device->index, Port->index);
            break;
          case AvFsmTvIdle:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvIdle", Port->device->index, Port->index);
            break;
          case AvFsmTvNotConnected:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvNotConnected", Port->device->index, Port->index);
            break;
          case AvFsmTvReset:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvReset", Port->device->index, Port->index);
            break;
          case AvFsmTvTxLogAddr:
            AvUapiOutputDebugFsm("Port[%d][%d]:TvTxLogAddr", Port->device->index, Port->index);
            break;
#endif
        }
    }
}
#endif /* CEC Related */
