/**
 * @file kapi.c
 *
 * @brief kernel api functions of this audio/video software package \n
 *        this file will also be only header file of this software package when user do porting
 */
#include "kapi.h"
#include "uapi.h"
#include "kernel_status_update.h"

static AvPort *FirstPort = NULL;
static AvPort *PreviousPort = NULL;
AvFpKeyCommand   AvHookKeyCmd;
AvFpUartCommand  AvHookUartCmd;
AvFpIrdaCommand  AvHookIrdaCmd;
void ClearVideoFromPort(pin AvPort *port);
void ClearAudioFromPort(pin AvPort *port);

/**
 * @brief  hookup user layer access functions
 * @return AvOk - success
 */
kapi AvRet AvKapiHookUserFunctions(pin AvFpKeyCommand keyCmd, pin AvFpUartCommand uartCmd,pin AvFpIrdaCommand IrdaCmd)
{
    AvRet ret = AvOk;
    AvHookKeyCmd = keyCmd;
    AvHookUartCmd = uartCmd;
    AvHookIrdaCmd = IrdaCmd;
    return ret;
}

/**
 * @brief  init software
 * @return none
 * @note
 */
kapi AvRet AvApiInit(void)
{
    AvRet ret = AvOk;
    ret = AvUapiInit();
    return ret;
}

/**
 * @brief  add device
 * @return none
 * @note customer is allowed to add customized device to the loop
 * during initialization, the device's detail should be fed
 */
kapi AvRet AvApiAddDevice(AvDevice *device, AvDeviceType type, uint8 index,
                           void *specific, void *port,  void *extension)
{
    device->type      = type;
    device->index     = index;
    device->specific  = specific;
    device->port      = port;
    device->extension = extension;

    return AvOk;

}

/**
 * @brief  add audio/video port
 * @return none
 * @note customer is allowed to add customized port to the loop
 * during initialzation, the port's AvPort and AvPortType
 * must be set, a dedicated ID will be given to this port
 */
kapi AvRet AvApiAddPort(AvDevice *device, pin AvPort *port, uint16 index, AvPortType type)
{
    if(!FirstPort)
        FirstPort = port;

    port->device = device;
    port->index  = index;
    port->type   = type;
    AvApiPortMemAllocate(port);

    if(PreviousPort)
        PreviousPort->next = (struct AvPort*)port;
    PreviousPort = port;
    port->next = NULL;

    return AvOk;

}

/**
 * @brief  init device
 * @return none
 * @note device initialization must be done after AddDevice and AddPort
 * Or else, the port structure will crash.
 */
kapi AvRet AvApiInitDevice(AvDevice *device)
{
    AvUapiInitDevice(device);
    return AvOk;
}

/**
 * @brief  init fsm
 * @return none
 * @note device initialization must be done after initdevice
 * Or else, the port structure will crash.
 */
kapi AvRet AvApiInitFsm(pin AvPort *port)
{
    switch(port->type)
    {
        case HdmiRx:
        case DpRx:
        {
            AvKapiFsmEnterFunPlugRxFsm(port);
            AvKapiFsmEnterFunReceiverFsm(port);
#if AvEnableCecFeature /* CEC Related */
            if(port->index == AvCecPortIndex)
                AvKapiFsmEnterFunCecFsm(port);
#endif
            break;
        }
        case LogicVideoRx:
        {
            AvKapiFsmEnterFunReceiverFsm(port);
            break;
        }
        case LogicAudioRx:
        {
            break;
        }
        case HdmiTx:
        {
            AvKapiFsmEnterFunPlugTxFsm(port);
#if AvEnableCecFeature /* CEC Related */
            if(port->index == AvCecPortIndex)
                AvKapiFsmEnterFunCecFsm(port);
#endif
            break;
        }
        default:
            break;
    }
    return AvOk;
}

/**
 * @brief  update all ports status
 * @return none
 * @note
 */
kapi AvRet AvApiUpdate(void)
{
    AvPort* TempPort = FirstPort;
    uint8 OldState;

    while(TempPort)
    {
        /* Make Key Response and Uart Response faster */
        AvUserUartCmd(FirstPort);
        AvUserKeyCmd(FirstPort);
        AvUserIrdaCmd(FirstPort);
//      if(TempPort->device->index != 0)
      {
        switch(TempPort->type)
        {
            case HdmiRx:
            case DpRx:
                OldState = *TempPort->content.is_PlugRxFsm;
                KfunCheckPrState(TempPort);
                AvKapiFsmFunPlugRxFsm(TempPort);
                ReportPlugRxFsm(TempPort, OldState);
                OldState = *TempPort->content.is_ReceiverFsm;
                KfunCheckRxState(TempPort);
                AvKapiFsmFunReceiverFsm(TempPort);
                ReportReceiverFsm(TempPort, OldState);
                KfunCheckHdcpState(TempPort);
#if AvEnableCecFeature /* CEC Related */
                if(TempPort->index == AvCecPortIndex)
                {
                    OldState = *TempPort->content.is_CecFsm;
                    KfunCheckCecState(TempPort);
                    AvKapiFsmFunCecFsm(TempPort);
                    ReportCecFsm(TempPort, OldState);
                }
#endif
                break;
            case LogicVideoRx:
                KfunCheckLogicVideoRx(TempPort);
                break;
            case LogicAudioRx:
#if AvEnableAudioTTLInput
                KfunCheckLogicAudioRx(TempPort);
#endif
                break;
            case HdmiTx:
                OldState = *TempPort->content.is_PlugTxFsm;
                KfunCheckPtState(TempPort);
                AvKapiFsmFunPlugTxFsm(TempPort);
                ReportPlugTxFsm(TempPort, OldState);
#if AvEnableCecFeature /* CEC Related */
                if(TempPort->index == AvCecPortIndex)
                {
                    OldState = *TempPort->content.is_CecFsm;
                    KfunCheckCecState(TempPort);
                    AvKapiFsmFunCecFsm(TempPort);
                    ReportCecFsm(TempPort, OldState);
                }
#endif
#if AvEnableEArcRxFeature
                KfunCheckEarcRx(TempPort);
#endif
                break;
            case LogicVideoTx:
                KfunCheckLogicVideoTx(TempPort);
                break;
            case LogicAudioTx:
                KfunCheckLogicAudioTx(TempPort);
                break;
            case VideoScaler:
                KfunCheckVideoScaler(TempPort);
                break;
            case VideoColor:
                KfunCheckVideoColor(TempPort);
                break;
#if AvEnableInternalVideoGen
            case VideoGen:
                KfunCheckVideoGen(TempPort);
                break;
#endif
#if AvEnableInternalAudioGen
            case AudioGen:
                KfunCheckAudioGen(TempPort);
                break;
#endif
#if AvEnableInternalClockGen
            case ClockGen:
                KfunCheckClockGen(TempPort);
                break;
#endif
            }
          }
        if(!TempPort->next)
            break;
        else
            TempPort = (AvPort*)TempPort->next;
    }

    return AvOk;
}

/**
 * @brief  Change the routing of ports
 * @return none
 * @note
 */
kapi AvRet AvApiConnectPort(pin AvPort *FromPort, pout AvPort *ToPort, AvConnectType type)
{
    AvPort *TempPort = NULL;
    /* Step 1. Assign Video Relation */
    if((type == AvConnectVideo) || (type == AvConnectAV))
    {
        switch(FromPort->type)
        {
            case HdmiRx:
            case DpRx:
            case AnalogRx:
            case HdmiTx:
            case AnalogTx:
            case VideoScaler:
            case VideoColor:
            case VideoGen:
            case DviRx:
            case LogicVideoRx:
            case DviTx:
            case LogicVideoTx:
                ClearVideoFromPort(ToPort);
                /* To Port has already been assigned, the new added one is placed to be front */
                if((FromPort->content.RouteVideoToPort) &&
                   (FromPort->content.RouteVideoToPort != (struct AvPort*)ToPort))
                    ToPort->content.RouteVideoNextSameLevelPort = FromPort->content.RouteVideoToPort;
                FromPort->content.RouteVideoToPort = (struct AvPort*)(ToPort);
                ToPort->content.RouteVideoFromPort = (struct AvPort*)FromPort;
                if(ToPort->type == HdmiTx)
                {
                    ToPort->content.video->AvailableVideoPackets = 0;
                    /* ToPort->content.tx->Hpd = AV_HPD_FORCE_LOW; */
                    AvHandleEvent(FromPort, AvEventUpStreamConnectNewDownStream, NULL, NULL);
                }
                break;
            default:
                break;
        }
    }
    /* Step 2. Assign Audio Relation */
    if((type == AvConnectAudio) || (type == AvConnectAV))
    {
        switch(FromPort->type)
        {
            case HdmiRx:
            case DpRx:
            case HdmiTx:
            case LogicAudioRx:
            case LogicAudioTx:
            case AnalogTx:
            case AnalogRx:
#if AvEnableInternalAudioGen
            case AudioGen:
#endif
                ClearAudioFromPort(ToPort);
                /* To Port has already been assigned, the new added one is placed to be front */
                if((FromPort->content.RouteAudioToPort) &&
                   (FromPort->content.RouteAudioToPort != (struct AvPort*)ToPort))
                    ToPort->content.RouteAudioNextSameLevelPort = FromPort->content.RouteAudioToPort;
                FromPort->content.RouteAudioToPort = (struct AvPort*)ToPort;
                ToPort->content.RouteAudioFromPort = (struct AvPort*)FromPort;
                if(ToPort->type == HdmiTx)
                {
                    ToPort->content.audio->AvailableAudioPackets = 0;
                }
                break;
            default:
                break;
        }
    }
    /* Step 3. Hdcp Update Chain */
    if((type == AvConnectVideo) || (type == AvConnectAV))
    {
        switch(FromPort->type)
        {
            case HdmiRx:
            case DpRx:
            case HdmiTx:
            case AnalogTx:
            case AnalogRx:
            case VideoScaler:
            case VideoColor:
            case VideoGen:
            case DviTx:
            case LogicVideoTx:
            case DviRx:
            case LogicVideoRx:
                /* by default, the source port should be connected, right after the port  */
                /* is connected, its HdcpSource will be assigned. But if the sink port    */
                /* is placed in advance, its HdcpSource is null, just look into the chain */
                /* to find it. */
                if(FromPort->content.HdcpSource)
                {
                    TempPort = (AvPort*)(FromPort->content.HdcpSource);
                    while(TempPort->content.HdcpSource)
                    {
                        TempPort = (AvPort*)(TempPort->content.HdcpSource);
                    }
                    ToPort->content.HdcpSource = (struct AvPort*)TempPort;
                }
                else
                   ToPort->content.HdcpSource = FromPort->content.HdcpSource;
                break;
            case LogicAudioRx:
            case LogicAudioTx:
                FromPort->content.HdcpSource = NULL;
                break;
        }
    }

    /* Step 4. Uapi Connect */
    AvUapiConnectPort(FromPort, ToPort, type);

    return AvOk;
}

kapi AvRet AvApiPortMemAllocate(pin AvPort *port)
{
    /* Layer 0. Port Structure */
    port->core.HdmiCore = -1;
#ifndef COMPILER_C51_MODE
    port->core.DpllCore = -1;
    port->core.OsdCore  = -1;
    port->core.VspCore  = -1;
    /* Layer 1. AvPort */
    port->content.ID = NULL;
    port->content.is_active_PlugRxFsm = NULL;
    port->content.is_active_ReceiverFsm = NULL;
    port->content.is_active_PlugTxFsm = NULL;
    port->content.is_active_TxRoutingFsm = NULL;
#endif
    port->content.is_PlugRxFsm = NULL;
    port->content.is_ReceiverFsm = NULL;
    port->content.is_PlugTxFsm = NULL;
    port->content.is_TxRoutingFsm = NULL;
#if AvEnableCecFeature /* CEC Related */
    port->content.is_CecFsm = NULL;
    port->content.is_active_CecFsm = NULL;
    port->content.cec = NULL;
#endif /* CEC Related */
    port->content.rx = NULL; /* Rx FSM vars */
    port->content.tx = NULL; /* Tx FSM vars */
#if AvEnableDisplayPortRx
    port->content.dprx = NULL; /* DpRx vars */
#endif
    port->content.hdcp = NULL; /* Hdcp vars */
    port->content.hdcptx = NULL; /* Hdcp vars */
    port->content.video = NULL;
    port->content.audio = NULL;
    port->content.scaler = NULL;
    port->content.color = NULL;
#if AvEnableInternalVideoGen
    port->content.vg = NULL;
#endif
#if AvEnableInternalAudioGen
    port->content.ag = NULL;
#endif
#if AvEnableInternalClockGen
    port->content.cg = NULL;
#endif
#if AvEnableEArcRxFeature
    port->content.earx = NULL;
#endif
    port->content.HdcpSource = NULL;
    port->content.HdcpNextSinkPort = NULL;
    port->content.RouteVideoFromPort = NULL;
    port->content.RouteVideoNextSameLevelPort = NULL;
    port->content.RouteVideoToPort = NULL;
    port->content.RouteAudioFromPort = NULL;
    port->content.RouteAudioNextSameLevelPort = NULL;
    port->content.RouteAudioToPort = NULL;

    /* Layer 2. AvContent Details */
#ifndef COMPILER_C51_MODE
    AvUapiAllocateMemory(sizeof(uint16),    (void **)&(port->content.ID));
#endif
    switch(port->type)
    {
        case HdmiRx:
        case DpRx:
        {
#ifndef COMPILER_C51_MODE
            AvUapiAllocateMemory(sizeof(uint8),     (void **)&(port->content.is_active_PlugRxFsm));
            AvUapiAllocateMemory(sizeof(uint8),     (void **)&(port->content.is_active_ReceiverFsm));
#endif
            AvUapiAllocateMemory(sizeof(uint8),     (void **)&(port->content.is_PlugRxFsm));
            AvUapiAllocateMemory(sizeof(uint8),     (void **)&(port->content.is_ReceiverFsm));
            AvUapiAllocateMemory(sizeof(RxVars),    (void **)&(port->content.rx));
#if AvEnableDisplayPortRx
            if(port->type == DpRx)
            {
                AvUapiAllocateMemory(sizeof(DpRxVars),    (void **)&(port->content.dprx));
            }
#endif
            AvUapiAllocateMemory(sizeof(HdcpRx),    (void **)&(port->content.hdcp));
            AvUapiAllocateMemory(sizeof(AvVideo),   (void **)&(port->content.video));
            AvUapiAllocateMemory(sizeof(AvAudio),   (void **)&(port->content.audio));
#if AvEnableCecFeature /* CEC Related */
            if(port->index == AvCecPortIndex)
            {
                AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_CecFsm));
                AvUapiAllocateMemory(sizeof(AvCec),    (void **)&(port->content.cec));
            }
#endif /* CEC Related */
            break;
        }
        case LogicAudioRx:
        {
            AvUapiAllocateMemory(sizeof(AvAudio),  (void **)&(port->content.audio));
            break;
        }
        case HdmiTx:
        {
#ifndef COMPILER_C51_MODE
            AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_active_PlugTxFsm));
            AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_active_TxRoutingFsm));
#endif
            AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_PlugTxFsm));
            AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_TxRoutingFsm));
#if AvEnableCecFeature /* CEC Related */
            if(port->index == AvCecPortIndex)
            {
                AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_CecFsm));
                AvUapiAllocateMemory(sizeof(AvCec),    (void **)&(port->content.cec));
            }
#endif /* CEC Related */
#if AvEnableEArcRxFeature
            AvUapiAllocateMemory(sizeof(EarcRx),   (void **)&(port->content.earx));
            AvUapiAllocateMemory(sizeof(AvAudio),  (void **)&(port->content.eaaud));
#endif
            AvUapiAllocateMemory(sizeof(TxVars),   (void **)&(port->content.tx));
            AvUapiAllocateMemory(sizeof(HdcpTx),   (void **)&(port->content.hdcptx));
            AvUapiAllocateMemory(sizeof(AvVideo),  (void **)&(port->content.video));
            AvUapiAllocateMemory(sizeof(AvAudio),  (void **)&(port->content.audio));
            break;
        }
        case VideoScaler:
        {
            AvUapiAllocateMemory(sizeof(AvScaler), (void **)&(port->content.scaler));
            break;
        }
        case VideoColor:
        {
            AvUapiAllocateMemory(sizeof(AvColor),  (void **)&(port->content.color));
            break;
        }
#if AvEnableInternalVideoGen
        case VideoGen:
        {
            AvUapiAllocateMemory(sizeof(RxVars),   (void **)&(port->content.rx));
            AvUapiAllocateMemory(sizeof(AvVideo),  (void **)&(port->content.video));
            AvUapiAllocateMemory(sizeof(AvVideoGen),(void **)&(port->content.vg));
            break;
        }
#endif
#if AvEnableInternalAudioGen
        case AudioGen:
        {
            AvUapiAllocateMemory(sizeof(RxVars),   (void **)&(port->content.rx));
            AvUapiAllocateMemory(sizeof(AvAudio),  (void **)&(port->content.audio));
            AvUapiAllocateMemory(sizeof(AvAudioGen),(void **)&(port->content.ag));
            break;
        }
#endif
#if AvEnableInternalClockGen
        case ClockGen:
        {
            AvUapiAllocateMemory(sizeof(AvClockGen),(void **)&(port->content.cg));
            break;
        }
#endif
#if AvEnableVideoLogicBus
        case LogicVideoTx:
        {
            AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_TxRoutingFsm));
            AvUapiAllocateMemory(sizeof(uint8),    (void **)&(port->content.is_active_TxRoutingFsm));
            AvUapiAllocateMemory(sizeof(AvVideo),  (void **)&(port->content.video));
            AvUapiAllocateMemory(sizeof(AvLogicVideo),(void **)&(port->content.lvtx));
            break;
        }
#endif
        case LogicAudioTx:
        {
            AvUapiAllocateMemory(sizeof(AvAudio),  (void **)&(port->content.audio));
            break;
        }
    }

    return AvOk;

}

void ClearVideoFromPort(pin AvPort *port)
{
    /* clarify the routing in AvPort structure and AvDevice structure */
    AvPort *AbandonRxPort = NULL;

    /* 1. Find abandoned rx port and tx port */
    AbandonRxPort = port;
    while(AbandonRxPort->content.RouteVideoFromPort != NULL)
    {
        AbandonRxPort = (AvPort*)AbandonRxPort->content.RouteVideoFromPort;
        /* 1.1 Remove video ToPort in FromPort */
        if(AbandonRxPort->content.RouteVideoToPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteVideoToPort = port->content.RouteVideoNextSameLevelPort;
        if(AbandonRxPort->content.RouteVideoNextSameLevelPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteVideoNextSameLevelPort = port->content.RouteVideoNextSameLevelPort;
    }
    AbandonRxPort = port->device->port; /* Get 1st Port of the device */
    while((AbandonRxPort != NULL) && (AbandonRxPort->device->index == port->device->index))
    {
        /* 1.2 Remove video ToPort in FromPort */
        if(AbandonRxPort->content.RouteVideoToPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteVideoToPort = port->content.RouteVideoNextSameLevelPort;
        if(AbandonRxPort->content.RouteVideoNextSameLevelPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteVideoNextSameLevelPort = port->content.RouteVideoNextSameLevelPort;
        AbandonRxPort = (AvPort*)(AbandonRxPort->next);
    }

    /* 2. clear tx's pointers, in case of already cleared by kapi layer, add protection */
    port->content.RouteVideoFromPort = NULL;
    port->content.RouteVideoNextSameLevelPort = NULL;
}

void ClearAudioFromPort(pin AvPort *port)
{
    /* clarify the routing in AvPort structure and AvDevice structure */
    AvPort *AbandonRxPort = NULL;

    /* 1. Find abandoned rx port and tx port */
    AbandonRxPort = port;
    while(AbandonRxPort->content.RouteAudioFromPort != NULL)
    {
        AbandonRxPort = (AvPort*)AbandonRxPort->content.RouteAudioFromPort;
        /* 1.1 Remove audio ToPort in FromPort */
        if(AbandonRxPort->content.RouteAudioToPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteAudioToPort = port->content.RouteAudioNextSameLevelPort;
        if(AbandonRxPort->content.RouteAudioNextSameLevelPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteAudioNextSameLevelPort = port->content.RouteAudioNextSameLevelPort;
    }
    AbandonRxPort = port->device->port; /* Get 1st Port of the device */
    while((AbandonRxPort != NULL) && (AbandonRxPort->device->index == port->device->index))
    {
        /* 1.2 Remove audio ToPort in FromPort */
        if(AbandonRxPort->content.RouteAudioToPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteAudioToPort = port->content.RouteAudioNextSameLevelPort;
        if(AbandonRxPort->content.RouteAudioNextSameLevelPort == (struct AvPort*)port)
            AbandonRxPort->content.RouteAudioNextSameLevelPort = port->content.RouteAudioNextSameLevelPort;
        AbandonRxPort = (AvPort*)(AbandonRxPort->next);
    }

    /* 2. clear tx's pointers, in case of already cleared by kapi layer, add protection */
    port->content.RouteAudioFromPort = NULL;
    port->content.RouteAudioNextSameLevelPort = NULL;
}

/**
 * @brief  add audio/video port
 * @return none
 * @note this function is used for initializing port FSMs.
 */
kapi AvRet AvApiPortStart(void)
{
    AvPort *TempPort = FirstPort;
    while(TempPort)
    {
        AvApiInitFsm(TempPort);
        TempPort = (AvPort*)TempPort->next;
    }
    return AvOk;
}

#if AvEnableCecFeature /* CEC Related */
kapi AvRet AvKapiCecSendMessage(AvPort *port)
{
    return KfunCecSendMessage(port);
}

kapi AvRet AvKapiCecSetPhysicalAddr(AvPort *port)
{
    return KfunCecSetPhysicalAddr(port);
}

kapi AvRet AvKapiCecSetLogicalAddr(AvPort *port)
{
    return KfunCecSetLogicalAddr(port);
}

kapi AvRet AvKapiArcEnable(AvPort *port, uint8 value)
{
    return AvUapiTxArcEnable(port, value);
}

#endif /* CEC Related */
