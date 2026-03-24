#ifndef __gsv6k5_h
#define __gsv6k5_h

#include "av_config.h"

#define Gsv6k5ResourceSize  (1040+128+(AvEnableDetailTiming*144)+(AvEnableInternalVideoGen*40)+(AvEnableInternalAudioGen*24)+(AvEnableInternalClockGen*16)+(AvEnableCecFeature*180))

#include "gsv6k5_device.h"

/* supported uapi */
#define Gsv6k5_AvUapiInitDevice
#define Gsv6k5_AvUapiEnablePort
#define Gsv6k5_AvUapiResetPort
#define Gsv6k5_AvUapiRxPortInit
#define Gsv6k5_AvUapiRxEnableFreeRun
#define Gsv6k5_AvUapiRxGetStatus
#define Gsv6k5_AvUapiRxGet5VStatus
#define Gsv6k5_AvUapiRxSetHpdDown
#define Gsv6k5_AvUapiRxSetHpdUp
#define Gsv6k5_AvUapiRxGetVideoPacketStatus
#define Gsv6k5_AvUapiRxGetAudioPacketStatus
#define Gsv6k5_AvUapiRxGetHdcpStatus
#define Gsv6k5_AvUapiRxClearFlags
#define Gsv6k5_AvUapiTxEnableCore
#define Gsv6k5_AvUapiTxDisableCore
#define Gsv6k5_AvUapiTxPortInit
#define Gsv6k5_AvUapiTxGetStatus
#define Gsv6k5_AvUapiTxSendAksv
#define Gsv6k5_AvUapiTxVideoManage
#define Gsv6k5_AvUapiTxAudioManage
#define Gsv6k5_AvUapiRxVideoManage
#define Gsv6k5_AvUapiRxAudioManage
#define Gsv6k5_AvUapiTxSetAudioPackets
#define Gsv6k5_AvUapiRxGetPacketContent
#define Gsv6k5_AvUapiTxSetPacketContent
#define Gsv6k5_AvUapiTxEnableInfoFrames
#define Gsv6k5_AvUapiRxGetHdmiAcrInfo
#define Gsv6k5_AvUapiConnectPort
#define Gsv6k5_AvUapiDisconnectPort
#define Gsv6k5_AvUapiRxClearAudioInterrupt
#define Gsv6k5_AvUapiRxClearVideoInterrupt
#define Gsv6k5_AvUapiRxClearHdcpInterrupt
#define Gsv6k5_AvUapiTxMuteTmds
#define Gsv6k5_AvUapiRxGetAvMute
#define Gsv6k5_AvUapiTxSetAvMute
#define Gsv6k5_AvUapiRxGetHdmiModeSupport
#define Gsv6k5_AvUapiTxSetHdmiModeSupport
#define Gsv6k5_AvUapiTxSetFeatureSupport
#define Gsv6k5_AvUapiRxGetVideoLock
#define Gsv6k5_AvUapiRxGetVideoTiming
#define Gsv6k5_AvUapiTxSetVideoTiming
#define Gsv6k5_AvUapiRxGetHdmiDeepColor
#define Gsv6k5_AvUapiTxSetHdmiDeepColor
#define Gsv6k5_AvUapiRxGetAudioInternalMute
#define Gsv6k5_AvUapiRxSetAudioInternalMute
#define Gsv6k5_AvUapiTxSetAudNValue
#define Gsv6k5_AvUapiRxGetPacketType
#define Gsv6k5_AvUapiTxSetAviInfoFrame
#define Gsv6k5_AvUapiTxReadBksv
#define Gsv6k5_AvUapiRxAddBksv
#define Gsv6k5_AvUapiTxGetBksvTotal
#define Gsv6k5_AvUapiTxSetBlackMute
#define Gsv6k5_AvUapiTxEncryptSink
#define Gsv6k5_AvUapiTxDecryptSink
#define Gsv6k5_AvUapiTxGetHdcpStatus
#define Gsv6k5_AvUapiTxGetBksvReady
#define Gsv6k5_AvUapiRxSetHdcpEnable
#define Gsv6k5_AvUapiRxSetBksvListReady
#define Gsv6k5_AvUapiRxSetHdcpMode
#define Gsv6k5_AvUapiHdcp2p2Mode
#define Gsv6k5_AvUapiTxClearBksvReady
#define Gsv6k5_AvUapiTxClearRxidReady
#define Gsv6k5_AvUapiTxClearHdcpError
#define Gsv6k5_AvUapiRxCheckBksvExisted
#define Gsv6k5_AvUapiTxGetSinkHdcpCapability
#define Gsv6k5_AvUapiTxGetSinkHdcpCapability
#define Gsv6k5_AvUapiCecSendMessage
#define Gsv6k5_AvUapiGetNackCount
#define Gsv6k5_AvUapiTxCecInit
#define Gsv6k5_AvUapiCecRxGetStatus
#define Gsv6k5_AvUapiCecTxGetStatus
#define Gsv6k5_AvUapiTxCecSetPhysicalAddr
#define Gsv6k5_AvUapiTxCecSetLogicalAddr
#define Gsv6k5_AvUapiRxReadEdid
#define Gsv6k5_AvUapiRxWriteEdid
#define Gsv6k5_AvUapiRxSetSpa
#define Gsv6k5_AvUapiTxReadEdid
#define Gsv6k5_AvUapiRxEnableInternalEdid
#define Gsv6k5_AvUapiTxArcEnable
#define Gsv6k5_AvUapiRxHdcp2p2Manage
#define Gsv6k5_AvUapiTxHdcp2p2Manage
#define Gsv6k5_AvUapiCheckLogicAudioTx
#define Gsv6k5_AvUapiCheckLogicAudioRx
#define Gsv6k5_AvUapiCheckVideoScaler
#define Gsv6k5_AvUapiCheckVideoColor
#define Gsv6k5_AvUapiCheckVideoGen
#define Gsv6k5_AvUapiCheckAudioGen
#define Gsv6k5_AvUapiCheckClockGen
#define Gsv6k5_AvUapiEarcRxWriteCapData
#define Gsv6k5_AvUapiCheckEarcRx
#define Gsv6k5_AvUapiEarcRxGetAudioPacketStatus
#define Gsv6k5_AvUapiEarcSetAudioInterface

#ifdef Gsv6k5_AvUapiTxSetAudioPackets
void Gsv6k5_TxEnableInfoFrames (AvPort* port, uint16 InfoFrames, bool Enable);
void Gsv6k5_TxSetAudioInterface (AvPort* port);
void Gsv6k5_TxWriteAudIfPacket (AvPort* port, uint8 *Packet);
void Gsv6k5_TxSetAudChStatSampFreq (AvPort* port);
void Gsv6k5_TxSetAudMCLK (AvPort* port);
AvRet Gsv6k5_TxAudInputEnable (AvPort *port, TxAudioInterface Interface, bool Enable);
AvRet Gsv6k5_TxSendAVInfoFrame (AvPort *port, uint8 *AviPkt, uint8 PktEn);
AvRet Gsv6k5_TxSetManualPixelRepeat(AvPort *port);
AvRet Gsv6k5_TxSetCSC (AvPort *port);
uint8 Gsv6k5_RxGetPixRepValue (AvPort *port);
#endif

#endif
