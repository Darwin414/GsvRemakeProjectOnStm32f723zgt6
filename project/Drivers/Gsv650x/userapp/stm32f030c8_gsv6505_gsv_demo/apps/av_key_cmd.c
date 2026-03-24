#include "av_key_cmd.h"
#include "av_config.h"
#include "hal.h"
#include "kapi.h"

extern uint8  EdidHdmi2p0;

#if AvEnableKeyInput /* Enable UART */

extern uchar AudioStatus;

void ProcessKey(AvPort *port);
#define FoundKeyPress(p) uChar&p

#endif /* Enable UART */

void ProcessKey(AvPort *port)
{
#if AvEnableKeyInput
    uint8 value = 0x00;
    uint8 uChar = 0x00;
    if(AvHalGetKey(&uChar) == AvNotAvailable)
        return;

    /* Hdmi 2.0 EDID */
    value = (uChar>>1) & 0x01;
    if(value != 0x00)
    {
        EdidHdmi2p0 = 1 - EdidHdmi2p0;
        port->content.rx->EdidStatus = AV_EDID_NEEDUPDATE;
    }
    /* Edid 2.0 LED */
    Edid2p0LedOut(EdidHdmi2p0);

    /* Audio Change */
    /*
    AvPort *RxPort = port->device->port;
    AvPort *TempPort = NULL;
    value = (uChar>>0) & 0x01;
    if(value != 0x00)
    {
        TempPort = (AvPort*)((&RxPort[1])->content.RouteAudioFromPort);
        if(TempPort->type == HdmiRx)
            AvApiConnectPort(&RxPort[9], &RxPort[1], AvConnectAudio);
        else
            AvApiConnectPort(&RxPort[0], &RxPort[1], AvConnectAudio);
    }
    */

#if AvEnableCecFeature /* CEC Related */
    AvPort *RxPort = port->device->port;
    AvPort *TempPort = NULL;
    value = (uChar>>0) & 0x01;
    if(value != 0x00)
    {
        TempPort = (AvPort*)((&RxPort[3])->content.RouteAudioFromPort);
        if(TempPort->type == HdmiTx)
        {
            AudioStatus = 0;
            if((&RxPort[1])->content.cec->EnableAudioAmplifier != AV_CEC_AMP_DISABLED)
                (&RxPort[1])->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
            AvApiConnectPort(&RxPort[0], &RxPort[3], AvConnectAudio);
        }
        else if(TempPort->type == HdmiRx)
        {
            AudioStatus = 1;
            if((&RxPort[1])->content.cec->EnableAudioAmplifier != AV_CEC_AMP_ENABLED)
                (&RxPort[1])->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
            if((&RxPort[1])->content.cec->EnableARC != AV_CEC_ARC_INITIATED)
                (&RxPort[1])->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
            AvApiConnectPort(&RxPort[1], &RxPort[3], AvConnectAudio);
        }
    }
#endif

#endif
}

void ListenToKeyCommand(AvPort *port)
{
#if AvEnableKeyInput
    ProcessKey(port);
#endif
}

void RxInLedOut(uint8 enable)
{
//    if(enable == 1)
//        HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_RESET);
//    else
//        HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_SET);
}

void TxOutLedOut(uint8 index, uint8 enable)
{
    GPIO_PinState PinSet;
    if(enable == 1)
        PinSet = GPIO_PIN_RESET;
    else
        PinSet = GPIO_PIN_SET;

//    HAL_GPIO_WritePin(GPIOA, LED2_Pin, PinSet);
}

void Edid2p0LedOut(uint8 enable)
{
}

void TxOut5Venable(uint8 index, uint8 enable)
{
    GPIO_PinState PinSet;
    if(enable == 1)
        PinSet = GPIO_PIN_SET;
    else
        PinSet = GPIO_PIN_RESET;
//    HAL_GPIO_WritePin(GPIOA, EN_5V_Pin, PinSet);
}
