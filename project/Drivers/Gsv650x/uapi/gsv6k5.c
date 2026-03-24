#include "gsv6k5.h"
#include "gsv6k5_lib.h"
#include "hal.h"
#include "uapi.h"
#include "uapi_function_mapper.h"
#include "gsv6k5_tables.h"

typedef struct
{
    /* The Rx Port Core1 is connected */
    AvPort *RxCore1Occupied;
    /* The Port CP Core1 is connected */
    AvPort *Cp1CoreOccupied;
    uint32 CrcError;
    uint32 LoopCount;
#if AvEdidStoredInRam
    uint8  TxAEdidRam[AvEdidMaxSize];
    uint8  TxBEdidRam[AvEdidMaxSize];
    uint8  TxCEdidRam[AvEdidMaxSize];
    uint8  TxDEdidRam[AvEdidMaxSize];
#endif
}   Resource;

static Resource InternalResource[Gsv6k5DeviceNumber];

#define FindCore1(port)        (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].RxCore1Occupied)
#define FindCp1Mode(port)      (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].Cp1CoreOccupied)
//#define FindLookCount(port)    (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount)
//#define FindCrcError(port)     (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].CrcError)
#if AvEdidStoredInRam
#define TxAEdidRam(port)       (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].TxAEdidRam)
#define TxBEdidRam(port)       (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].TxBEdidRam)
#define TxCEdidRam(port)       (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].TxCEdidRam)
#define TxDEdidRam(port)       (InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].TxDEdidRam)
#endif

#define Gsv6k5SetVideoPacketFlag(BitName) \
        if((value == 0) && ((port->content.video->AvailableVideoPackets & BitName) != 0))\
            port->content.rx->ChangedVideoPackets = port->content.rx->ChangedVideoPackets | BitName;\
        else if((value != 0) && ((port->content.video->AvailableVideoPackets & BitName) == 0))\
            port->content.rx->ChangedVideoPackets = port->content.rx->ChangedVideoPackets | BitName;\
        if(value == 0)\
            port->content.video->AvailableVideoPackets = port->content.video->AvailableVideoPackets & (~BitName);\
        else\
            port->content.video->AvailableVideoPackets = port->content.video->AvailableVideoPackets | BitName;

#define Gsv6k5SetAudioPacketFlag(BitName) \
        if((value == 0) && ((port->content.audio->AvailableAudioPackets & BitName) != 0))\
            port->content.rx->ChangedAudioPackets = port->content.rx->ChangedAudioPackets | BitName;\
        else if((value != 0) && ((port->content.audio->AvailableAudioPackets & BitName) == 0))\
            port->content.rx->ChangedAudioPackets = port->content.rx->ChangedAudioPackets | BitName;\
        if(value == 0)\
            port->content.audio->AvailableAudioPackets = port->content.audio->AvailableAudioPackets & (~BitName);\
        else\
            port->content.audio->AvailableAudioPackets = port->content.audio->AvailableAudioPackets | BitName;

#define Gsv6k5IntptSt(func, value) \
            GSV6K5_INT_get_RX1_##func##_INT_ST(port,         &value);

#define Gsv6k5IntptRaw(func) \
            GSV6K5_INT_get_RX1_##func##_RAW_ST(port,         &Value);

#define Gsv6k5IntptClr(func) \
            GSV6K5_INT_set_RX1_##func##_CLEAR(port,         1);

/* local functions */
void Gsv6k5GetRx5VStatus(pin AvPort *port);
void Gsv6k5EnableRxHpa(pin AvPort *port);
void Gsv6k5DisableRxHpa(pin AvPort *port);
void Gsv6k5ToggleTxHpd(pin AvPort *port, uint8 EdidRead);
void Gsv6k5ToggleTmdsOut(AvPort *port, uint8 Enable);
void Gsv6k5RxManualEQUpdate(AvPort *port, uint8 DefaultEQEnable);
void Gsv6k5SetTxHdcpVersion(pin AvPort *port, HdcpTxSupportType HdmiStyle, uint8 ForceUpdate);
void Gsv6k5UpdateRxCdrBandWidth(pin AvPort *port);
void Gsv6k5RxScdcMonitor(pin AvPort *port);
void Gsv6k5RxScdcCedClear(pin AvPort *port);
uint8 Gsv6k5TxScdcAction(pin AvPort *port, uint8 WriteEnable, uint8 DevAddr, uint8 RegAddr, uint8 Value);
uint8 Gsv6k5TxDDCError(pin AvPort *port);
void Gsv6k5ManualBksvRead(pin AvPort *port);
AvPort *Gsv6k5FindHdmiRxFront(pin AvPort *port);
AvVideoCs Gsv6k5ColorCsMapping(AvVideoCs InputCs);
void Gsv6k5ManualCpParameter(AvPort *port, AvPort *FromPort);
void Gsv6k5CpCscManage(pin AvPort *port, AvVideoCs VarInCs, AvVideoCs VarOutCs);
uint8 Gsv6k5Hdr2Sdr(pin AvPort *port);
void Gsv6k5Thumbnail(pin AvPort *port, uint16 InH, uint16 InV, uint16 OutH, uint16 OutV);
void Gsv6k5ScaleRatio(pin AvPort *port, uint8 hparam, uint8 hratio, uint8 vparam, uint8 vratio, uint8 force);
void Gsv6k5TxPllUnlockClear(pin AvPort *port);
uint8 Gsv6k5TweakCrystalFreq(pin AvPort *port, uint32 value32);
uint32 Gsv6k5ReadPllFreq(pin AvPort *port);
void Gsv6k5MpllProtect(pin AvPort *port);
void Gsv6k5RpllProtect(pin AvPort *port);
uint8 Gsv6k5TxPllProtect(AvPort *port, AvPort *UpperPort, AvPort *FromPort);
void Gsv6k5CpProtect(pin AvPort *port, uint8 value);
void Gsv6k5ResetTxFifo(pin AvPort *port);
AvRet Gsv6k5TxCoreInUse(pin AvPort *port);
void Gsv6k5ResetTxAudioDatapath(AvPort *port);
void Gsv6k5TxHBlankControl(AvPort *port, uint8 HdcpEnable);
void Gsv6k5TxFrlTrafficControl(AvPort *port);
uint32 Gsv6k5TuneTxAcr(AvPort *port, uint32 NValue);
void Gsv6k5ToggleDpllFreq(pin AvPort *port, uint8 index, uint8 Integer, uint8 *Fraction);
void Gsv6k5RxEyePrint(pin AvPort *port);
void Gsv6k5SetPacketBypass(AvPort *port);
void Gsv6k5SetRxH2p1Mode(AvPort *port, uint8 enable);
uint8 Gsv6k5SetTxH2p1Mode(AvPort *port);
void Gsv6k5TxFrlStart(AvPort *port);
AvRet Gsv6k5TxSchedule(pin AvPort *port);
void Gsv6k5VrrTimingAdjust(pin AvPort *port);

/**
 * @brief  device init function
 * @return AvOk if success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiInitDevice(pin AvDevice *device))
{
    //Resource *InternalResource;
    uint8 tmpVal = 0;
    uint32 i = 0;
    uint32 k = 0;
    uint32 DefaultMapAddress;
    uint32 LaneMapAddress = 0;

    AvPort *port = (AvPort *)device->port;
    Gsv6k5Device *gsv6k5Dev = (Gsv6k5Device *)device->specific;
    DefaultMapAddress = gsv6k5Dev->DeviceAddress;

    /* check internal mcu enable flag, external mcu idle if enabled */
    uint8 value;
    AvHalI2cReadField8(DefaultMapAddress | 0xFF, 0xFF, 0xFF, 0, &value);
    if(value == 0x01)
    {
        AvUapiOutputDebugFsm("External MCU halts by enabled Internal MCU");
        while(1);
    }

    /* reset device */
    GSV6K5_PRIM_set_MAIN_RST(port, 0);
    GSV6K5_PRIM_set_MAIN_RST(port, 1);
    for(i=0; i<100; i++)
        GSV6K5_PRIM_set_MAIN_RST(port, 0);

    /* set default i2c address for internal maps */
    AvUapiOutputDebugMessage("Gsv6k5%c-%c - setting i2c addresses.", 'A'+port->device->index, 'A'+device->index);

    /* Write Init Table */
    for(k=0; Gsv6k5InitTable[k]!=0xFF; k=k+3)
    {
        AvHalI2cWriteField8(DefaultMapAddress | Gsv6k5InitTable[k],Gsv6k5InitTable[k+1],0xFF,0,Gsv6k5InitTable[k+2]);
      if(k<128)
      {
        AvUapiOuputDbgMsg("    0x%02X, 0x%02X, 0x%02X", Gsv6k5InitTable[k], Gsv6k5InitTable[k+1], Gsv6k5InitTable[k+2]);
        AvHalI2cReadField8(DefaultMapAddress | Gsv6k5InitTable[k],Gsv6k5InitTable[k+1],0xFF,0,&tmpVal);
        if(tmpVal == Gsv6k5InitTable[k+2])
          AvUapiOuputDbgMsg("      %02X", tmpVal);
        else
          AvUapiOuputDbgMsg("      %02X!!!", tmpVal);
      }
    }

    // RX inital added for adaptive EQ
    /* Already Get 1st Rx Port of device */
    LaneMapAddress = DefaultMapAddress | gsv6k5Dev_RxALane0MapAddress;
    for(k=0; Gsv6k5HdmiEqTable[k]!=0xFF; k=k+2)
        AvHalI2cWriteField8(LaneMapAddress,Gsv6k5HdmiEqTable[k],0xFF,0,Gsv6k5HdmiEqTable[k+1]);
    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),0x40,0xFF,0,0x06);
    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),0x42,0xFF,0,0x00);
    for(k=0; k<=0xFF; k=k+1)
        AvHalI2cWriteField8(GSV6K5_EQRAM_MAP_ADDR(port),k,0xFF,0,Gsv6k5EqRamInitTable[k]);
    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),0x42,0xFF,0,0x01);
    for(k=0; k<=0x49; k=k+1)
        AvHalI2cWriteField8(GSV6K5_EQRAM_MAP_ADDR(port),k,0xFF,0,Gsv6k5EqRamInitTable[256+k]);
    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),0x40,0xFF,0,0x00);

    /* Alternative CEC Pin over AVMUTE for Pin 11 since U03 for xx02 */
    /* This is used for CEC CTS Item 7-15 compatibility */
    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),0x51,0xFF,0,0x80);
    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),0x51,0xFF,0,0xA0);
    AvHalI2cReadField8(GSV6K5_SEC_MAP_ADDR(port),0x0F,0x0F,0,&value);
    if(value&0x08)
    {
        AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),0x51,0xFF,0,0x10);
    }

    Gsv6k5MpllProtect(port);
    Gsv6k5SetPacketBypass(port);

    //AvUapiAllocateMemory(sizeof(Resource), (uint32 *)&InternalResource);
    device->extension = &InternalResource;
    FindCore1(port) = NULL;
    FindCp1Mode(port) = NULL;
    return AvOk;
}

/**
 * @brief  init rx port
 * @return AvOk: success
 * @note
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxPortInit(pin AvPort *port))
{
    uint8 value   = 0;
    uint8 addr    = 0;
    uint8 offset1 = 0;
    uint8 mask1   = 0;
    uint8 offset2 = 0;
    uint8 mask2   = 0;
    uint8 swap    = 0;

    /* 1. Only RxPort occupies RxCore*/
    if(port != FindCore1(port))
        port->core.HdmiCore = -1;

    /* 2. Rx Pin mapping configuration */
    if(port->content.rx->PinMapping != 0)
    {
        swap = (port->content.rx->PinMapping & 0x300)>>8;
        mask2 = 0x03;
        offset2 = 0;
        mask1 = 0x03;
        addr = 0x9C;
        offset1 = 0;
        /* 2.1 4-channel lane swap */
        /* [7:6] = Lane3/CLK from Lane0(0),Lane1(1),Lane2(2),Lane3/CLK(3) */
        /* [5:4] = Lane2     from Lane0(0),Lane1(1),Lane2(2),Lane3/CLK(3) */
        /* [3:2] = Lane1     from Lane0(0),Lane1(1),Lane2(2),Lane3/CLK(3) */
        /* [1:0] = Lane0     from Lane0(0),Lane1(1),Lane2(2),Lane3/CLK(3) */
        value = port->content.rx->PinMapping & 0xFF;
        AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port),addr,0xFF,0,value);
        /* RxFRL.0x10[4] for FRL P/N swap,RxDIG.0x1E[3] for TMDS P/N swap */
        if(port->type == HdmiRx)
        {
            value = (port->content.rx->PinMapping & 0x1000)>>12;
            AvHalI2cWriteField8(GSV6K5_RXFRL_MAP_ADDR(port),0x10,0x10,4,value);
            AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port),0x1E,0x08,3,value);
        }
        /* 2.2 ddc/aux lane swap */
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),0x72,mask1,0,(swap<<offset1));
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),0x72,mask2,0,((offset1/2)<<offset2));
        /* 2.4 HPD lane swap */
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),0x73,mask1,0,(swap<<offset1));
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),0x73,mask2,0,((offset1/2)<<offset2));
        /* 2.5 5V lane swap */
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),0x74,mask1,0,(swap<<offset1));
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),0x74,mask2,0,((offset1/2)<<offset2));
    }

    /* 3. Disable Rx HPA */
    Gsv6k5DisableRxHpa(port);
    return AvOk;
}


void AvUapiTxPortSwapRead(AvPort *port)
{
  uint8 value   = 0;

  if((port != NULL) && (port->type == HdmiTx))
  {
    AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port),0xD9,0xFF,0,&value);
    AvUapiOutputDebugMessage("  port[%d][%d] 0x%02XD9: %02X", port->device->index, port->index, GSV6K5_TXPHY_MAP_ADDR(port),value);
    AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port),0xF4,0xFF,0,&value);
    AvUapiOutputDebugMessage("  port[%d][%d] 0x%02XF4: %02X", port->device->index, port->index, GSV6K5_TXPHY_MAP_ADDR(port),value);
    // AvHalI2cReadField8(GSV6K5_TXDP2_MAP_ADDR(port),0x42,0xFF,0,&value);
    // AvUapiOutputDebugMessage("  port[%d][%d] 0x6542: %02X",value);
    AvHalI2cReadField8(GSV6K5_PRIM_MAP_ADDR(port),0x70,0xFF,0,&value);
    AvUapiOutputDebugMessage("  port[%d][%d] 0x0070: %02X", port->device->index, port->index,value);
  }
}

void Gsv6k5_PrintTxPinMappingReg(AvPort *port)
{
    uint8 value;
    AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port), 0xF4, 0xff, 0, &value);
    AvUapiOutputDebugMessage("  Port[%d][%d] TmdsLaneSwap:(%02X & 0xC0)", port->device->index, port->index, value);
    AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port), 0xF4, 0xff, 0, &value);
    AvUapiOutputDebugMessage("  Port[%d][%d] TmdsNpSwap:(%02X & 0x02)", port->device->index, port->index, value); 
    AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port), 0xD9, 0xff, 0, &value);
    AvUapiOutputDebugMessage("  Port[%d][%d] CmSterm:(%02X & 0x04)", port->device->index, port->index, value); 
    AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port), 0xD9, 0xff, 0, &value);
    AvUapiOutputDebugMessage("  Port[%d][%d] TmdsMirror:(%02X & 0x08)", port->device->index, port->index, value); 
}

/**
 * @brief  init tx port
 * @return AvOk: success
 * @note
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxPortInit(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value = 0;

    /* 1. Tx Pin mapping configuration */
    /* 0 = Lane3/CLK, Lane2, Lane1, Lane0 (default)                   */
    /* 1 = Lane3/CLK, Lane0, Lane1, Lane2 (HDMI 2.0 lane swap)        */
    /* 2 = Lane0,     Lane3, Lane2, Lane1 (GSV DP intra-connection)   */
    /* 3 = Lane2,     Lane3, Lane0, Lane1 (GSV HDMI intra-connection) */
    if((port->device->index == 0) && (port->index == 6))
      port->content.tx->PinMapping = 0x4;
    AvUapiOutputDebugMessage("  Port[%d][%d] PinMapping: %08X", port->device->index, port->index, port->content.tx->PinMapping);
    if(port->content.tx->PinMapping != 0)
    {
        value = port->content.tx->PinMapping & 0x03;
        GSV6K5_TXPHY_set_TMDS_CH_SWAP(port, value);  // 0x60F4=0x30 & 0xC0
        value = (port->content.tx->PinMapping & 0x04)>>2;
        GSV6K5_TXPHY_set_TMDS_POL_REVERT(port, value);  // 0x60F4=0x30 & 0x02
        value = 1 - ((port->content.tx->PinMapping & 0x08)>>3);
        GSV6K5_TXPHY_set_EN_CM_STERM(port, value);  // 0x60D9=0xF4 & 0x04
        value = (port->content.tx->PinMapping & 0x10)>>4;
        GSV6K5_TXPHY_set_TMDS_MIRROR(port, value);  // 0x60D9=0xF4 & 0x08
    }
    AvUapiTxPortSwapRead(port);
    Gsv6k5_PrintTxPinMappingReg(port);

#if AvEnableCecFeature /* CEC Related */
    /* Gsv6k5 has 1 input port */
    if(port->index == AvCecPortIndex)
        port->content.cec->InputCount = 1;
#endif

#if AvEnableEthTxMode
    AvHalI2cWriteField8(GSV6K5_ANAPHY_MAP_ADDR(port),0xB9,0xF0,0,0xF0);
    AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0xD5,0x0F,0,0x0F);
#endif
    return ret;
}

/**
 * @brief  enable tx port core
 * @return AvOk: success
 * @note
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxEnableCore(pin AvPort *port))
{
    AvRet ret = AvOk;

    if(port->index == 4)
        GSV6K5_PRIM_set_TXA_PWR_DN(port, 0);
    else if(port->index == 5)
        GSV6K5_PRIM_set_TXB_PWR_DN(port, 0);
    else if(port->index == 6)
        GSV6K5_PRIM_set_TXC_PWR_DN(port, 0);
    else if(port->index == 7)
        GSV6K5_PRIM_set_TXD_PWR_DN(port, 0);
    else
        ret = AvError;

    return ret;
}

/**
 * @brief  disable tx port core
 * @return AvOk: success
 * @note
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxDisableCore(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint32 MainMapAddress   = GSV6K5_TXDIG_MAP_ADDR(port);
    uint32 PhyMapAddress    = GSV6K5_TXPHY_MAP_ADDR(port);

    /* Disable Tx TMDS Clock Driver, only enable after video is fully stable */
    if(port->content.tx->EdidSupportFeature & AV_BIT_FEAT_SCDC)
    {
#if AvEnableTxCtsFrlTraining
        uint8 data[2] = {0x00,0x00};
        AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0x08, 2, data);
        if((data[0] != 0x44) || (data[1] != 0x89))
#endif
        Gsv6k5TxScdcAction(port, 1, 0xA8, 0x31, 0x00); /* Disable FRL */
    }
    /* Disable HDCP */
    AvUapiTxDecryptSink(port);
    /* Set HDCP Version Before Reset Tx */
    Gsv6k5SetTxHdcpVersion(port, port->content.hdcptx->HdmiStyle, 0);
    /* Reset Tx Phy SCDC state */
    AvUapiTxSetFeatureSupport(port);
    Gsv6k5ToggleTmdsOut(port, 0);
    port->content.tx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
    /* keep Hdmi mode */
    if((port->core.HdmiCore == 0) && (Gsv6k5TxCoreInUse(port) != AvOk))
    {
        AvUapiTxSetHdmiModeSupport(port);
        AvHalI2cWriteField8(MainMapAddress,0x53,0xFF,0,0x04);//; Tx Color Depth = 8 bits
        AvHalI2cWriteField8(MainMapAddress,0x23,0xFF,0,0x80);//; Manual Pixel Repetition x1
        AvHalI2cWriteField8(MainMapAddress,0x20,0xFF,0,0x00);//; Tx input set to default RGB
        AvHalI2cWriteField8(MainMapAddress,0x21,0xFF,0,0x00);//; Tx output set to default RGB
        AvHalI2cWriteField8(MainMapAddress,0x00,0xFF,0,0x00);//; Mclk Tx 128*fs
        AvHalI2cWriteField8(MainMapAddress,0x50,0xFF,0,0x00);//; Disable AudioInfo/AviInfo packet
        AvHalI2cWriteField8(MainMapAddress,0x51,0xFF,0,0x00);//; Disable GC/AudioSample/N_CTS packet
        AvHalI2cWriteField8(MainMapAddress,0x2B,0x80,0,0x00);//; Disable Tx CSC
        AvHalI2cWriteField8(MainMapAddress,0x1A,0xFF,0,0xB0);//; Audio Mute and default Layout 0
        GSV6K5_TXPKT_set_TX_PKT_UPDATE(port, 0);
    }
    /* FRL parameter reset to default */
    AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x23,0xFF,0,0x04);
    /* Enable Tx SCDC Feature as default */
    AvHalI2cWriteField8(PhyMapAddress,0x01,0xFF,0,0x01);
    AvHalI2cWriteField8(PhyMapAddress,0x02,0xFF,0,0x01);
    AvHalI2cWriteField8(PhyMapAddress,0x04,0xFF,0,0x00);
    /* Enable Sink SCDC and RR Analysis done */
    AvHalI2cWriteField8(PhyMapAddress,0x03,0x03,0,0x03);

#if AvEnableHdcp2p2Feature
    /* HDCP 2.2 Stop on Error */
    GSV6K5_TX2P2_set_TX_HDCP2P2_OP_WHEN_ERROR(port, 1);
    /* HDCP 2.2 revocation_check_pass=1 */
    GSV6K5_TX2P2_set_REVOCATION_CHECK_PASS(port, 1);
    /* HDCP 2.2 Stop on reset */
    GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 1);
    /* default DDC Speed */
    GSV6K5_TX2P2_set_HDCP2P2_MASTER_CLK_DIV(port, 0x84);
    /* HDCP 2.2 i2c_clear_int_rxid_ready=1 */
    Gsv6k5AvUapiTxClearRxidReady(port);
#endif

    /* CEC Recover */
#if AvEnableCecFeature /* CEC Related */
    if(port->index == AvCecPortIndex)
    {
        if(port->content.cec->CecEnable == 1)
        {
            Gsv6k5AvUapiTxCecInit(port);
            Gsv6k5AvUapiTxCecSetLogicalAddr(port);
        }
    }
#endif

#if (Gsv6k5MuxMode == 0)
    /* Packet information needs to be resent */
    port->content.video->InCs  = AV_CS_RGB;
    port->content.video->OutCs = AV_CS_RGB;
    port->content.video->Y     = AV_Y2Y1Y0_RGB;
    port->content.video->AvailableVideoPackets = 0;
    port->content.audio->AvailableAudioPackets = 0;
    port->content.audio->NValue = 0;
#endif

    return ret;
}

void Gsv6k5ResetTxFifo(pin AvPort *port)
{
    /* reset bandwidth */
    if((port->content.tx->H2p1FrlType != AV_FRL_NONE) || (port->content.tx->H2p1FrlManual != 0))
        Gsv6k5TxFrlTrafficControl(port);

#ifdef GsvInternalHdcpTxScdcFlag
    Gsv6k5TxScdcAction(port, 1, 0xA8, 0xDF, 0);
#endif
    /* reset dc fifo */
    //GSV6K5_TXDIG_set_DCFIFO_RESET(port, 1);
    //GSV6K5_TXDIG_set_DCFIFO_RESET(port, 0);
    /* reset audio fifo */
    //GSV6K5_TXDIG_set_TX_AUDIOFIFO_RESET(port, 1);
    //GSV6K5_TXDIG_set_TX_AUDIOFIFO_RESET(port, 0);
    /* Lane fifo Reset */
    GSV6K5_TXFRL_set_FRL_TX_LANE_FIFO_RST(port, 1);
    GSV6K5_TXFRL_set_FRL_TX_LANE_FIFO_RST(port, 0);
    /* reset phy */
    GSV6K5_TXPHY_set_TMDS_SRST_MAN(port, 0);
    GSV6K5_TXPHY_set_TMDS_SRST_MAN(port, 1);
}

void Gsv6k5ResetTxAudioDatapath(AvPort *port)
{
    GSV6K5_TXDIG_set_AUDIO_SYNC_FIFO_CLR(port, 1);
    GSV6K5_TXDIG_set_CLEAR_AUDIO_BYPASS_STATE(port, 1);
    GSV6K5_TXDIG_set_TX_AUDIOFIFO_RESET(port, 1);
    GSV6K5_TXDIG_set_AUDIO_SYNC_FIFO_CLR(port, 0);
    GSV6K5_TXDIG_set_CLEAR_AUDIO_BYPASS_STATE(port, 0);
    GSV6K5_TXDIG_set_TX_AUDIOFIFO_RESET(port, 0);
#if Gsv6k5AudioBypass
    /* Audio Bypass Fifo Reset */
    GSV6K5_TXDIG_set_BYPASS_PKT_STATE_CLR(port, 1);
    GSV6K5_TXDIG_set_BYPASS_PKT_SFIFO_CLR(port, 1);
    GSV6K5_TXDIG_set_BYPASS_PKT_AFIFO_CLR(port, 1);
    GSV6K5_TXDIG_set_BYPASS_PKT_STATE_CLR(port, 0);
    GSV6K5_TXDIG_set_BYPASS_PKT_SFIFO_CLR(port, 0);
    GSV6K5_TXDIG_set_BYPASS_PKT_AFIFO_CLR(port, 0);
#endif
}

void Gsv6k5TxHBlankControl(AvPort *port, uint8 HdcpEnable)
{
    AvPort *CurrentPort = port->device->port;
    AvPort *FromPort = NULL;
    uint8 flag = 0;
    uint8 value = 0;
    uint8 H1p4MuxModeMatch = 0;
    uint8 H1p4TxCoreMatch = 0;

    /* Check HDCP Encryption */
    if((HdcpEnable == 1) &&
       ((port->content.hdcptx->Hdcp2p2SinkSupport == 0) ||
        (port->content.hdcptx->HdmiStyle == AV_HDCP_TX_1P4_ONLY) ||
        (port->content.hdcptx->HdmiStyle == AV_HDCP_TX_1P4_FAIL_OUT)))
    {
        if(port->core.HdmiCore == 0)
            H1p4TxCoreMatch = 1;
        else
            H1p4MuxModeMatch = 1;
    }
    else
    {
        while((CurrentPort != NULL) && (CurrentPort->device->index == port->device->index))
        {
            if(CurrentPort->type == HdmiTx)
            {
                GSV6K5_TXPHY_get_TX_HDCP1P4_STATE_RB(CurrentPort, &flag);
                if((flag >= 2) && (flag <= 5))
                {
                    if(CurrentPort->core.HdmiCore == 0)
                        H1p4TxCoreMatch = 1;
                    else
                        H1p4MuxModeMatch = 1;
                }
            }
            CurrentPort = (AvPort*)(CurrentPort->next);
        }
    }

    /* Audio Insertion Protection */
    FromPort = (AvPort*)(port->content.RouteVideoFromPort);
    if((FromPort->type == VideoScaler) || (FromPort->type == VideoColor))
        FromPort = (AvPort*)(FromPort->content.RouteVideoFromPort);
    if((FromPort != NULL) && ((FromPort->type == HdmiRx) || (FromPort->type == VideoGen)) &&
       (FromPort->content.rx->IsFreeRun == 1))
    {
        if(((port->content.hdcptx->HdcpEnabled == 0) && (FromPort->content.video->DscStream == 1)) ||
           (port->content.tx->H2p1FrlManual != 0))
            value = 0x02;
        else
            value = 0x09;
        if(((port->core.HdmiCore == 0)  && (H1p4TxCoreMatch == 1)) ||
           ((port->core.HdmiCore == -1) && (H1p4MuxModeMatch == 1)))
            value = 0x0E; /* 0x0E for HDCP 1.4, 0x09 for HDCP 2.2 */
        if(port->core.HdmiCore == 0)
        {
            GSV6K5_TXDIG_get_BLANK_AFTER_DE_MAN(port, &flag);
            if(flag != value)
                GSV6K5_TXDIG_set_BLANK_AFTER_DE_MAN(port, value);
        }
        else
        {
            switch(port->index)
            {
                case 4:
                    GSV6K5_SEC_get_TXPORT_A_SRC_SEL(port, &flag);
                    break;
                case 5:
                    GSV6K5_SEC_get_TXPORT_B_SRC_SEL(port, &flag);
                    break;
                case 6:
                    GSV6K5_SEC_get_TXPORT_C_SRC_SEL(port, &flag);
                    break;
                case 7:
                    GSV6K5_SEC_get_TXPORT_D_SRC_SEL(port, &flag);
                    break;
            }
            if(flag == 3)
            {
                GSV6K5_SEC_get_MUX_MODE_BLANK_AFTER_DE_MAN(port, &flag);
                if(flag != value)
                    GSV6K5_SEC_set_MUX_MODE_BLANK_AFTER_DE_MAN(port, value);
            }
        }
    }
}

void Gsv6k5TxFrlTrafficControl(AvPort *port)
{
    uint8 value1 = 0x20;
    uint8 value2 = 0x03;
    uint8 value3 = 0;
    uint8 enable = 0;
    /* Traffic Control */
    if(port->content.video->info.LaneFreq < port->content.video->info.TmdsFreq)
    {
        GSV6K5_TXFRL_get_RB_FRL_VIDEO_FIFO_TH(port, &value1);
        if(value1 > 0x24)
            value1 = 0x24;
    }
    else
    {
        if(port->content.video->info.TmdsFreq < 45)
        {
            value1 = 0x04;
            value2 = 0x01;
        }
        else if(port->content.video->info.TmdsFreq < 100)
        {
            value1 = 0x08;
            value2 = 0x01;
        }
    }
#if FixedIntervalSmoothFrl
    value1 = 0x10;
    value2 = 0x01;
#endif
    if((port->content.tx->H2p1FrlType != AV_FRL_NONE) || (port->content.tx->H2p1FrlManual != 0))
        enable = 1;
    if((enable == 1) && (value2 == 0x01))
        value3 = 1;
    GSV6K5_TXFRL_set_FRL_VID_FIFO_TH_MAN_VALUE(port, value1);
    GSV6K5_TXFRL_set_FRL_BLK_FIFO_TH_MAN_VALUE(port, value2);
    GSV6K5_TXFRL_set_FRL_REPCNT_BYPASS(port, value3);
    GSV6K5_TXFRL_set_FRL_FIFO_TH_VALUE_MAN_EN(port, enable);
    GSV6K5_TXFRL_set_FRL_TIMING_DET_RESTART(port, 1);
    GSV6K5_TXFRL_set_FRL_TIMING_DET_RESTART(port, 0);
    /* Tx TMDS-FRL Fifo Reset */
    GSV6K5_TXFRL_set_FRL_TX_FIFO_MANUAL_RST(port, 1);
    if((port->content.tx->InfoReady > TxVideoManageThreshold) ||
       (port->content.tx->H2p1FrlManual != 0))
        GSV6K5_TXFRL_set_FRL_TX_FIFO_MANUAL_RST(port, 0);
}

uint32 Gsv6k5TuneTxAcr(AvPort *port, uint32 NValue)
{
    uint32 TmdsClk = 0;
    uint32 TuneN = NValue;
    uint32 Divider = 0;
    uint32 ExpectCts = 0;
    /* Step 1. Exception */
    /* Step 2. TmdsClk */
    switch(port->content.tx->H2p1FrlType)
    {
        case AV_FRL_12G4L:
            TmdsClk = 12000000/(18*256);
            break;
        case AV_FRL_10G4L:
            TmdsClk = 10000000/(18*256);
            break;
        case AV_FRL_8G4L:
            TmdsClk = 8000000/(18*256);
            break;
        case AV_FRL_6G4L:
        case AV_FRL_6G3L:
            TmdsClk = 6000000/(18*256);
            break;
        case AV_FRL_3G3L:
            TmdsClk = 3000000/(18*256);
            break;
        default:
            if(port->content.tx->H2p1FrlManual != 0)
            {
                Divider = (port->content.tx->H2p1FrlManual&0xFFFF)*1000;
                TmdsClk = Divider/(18*256);
            }
            else
                TmdsClk = (port->content.video->info.TmdsFreq*1000)/256;
            if(TmdsClk == 0)
                return TuneN;
            break;
    }
    /* Step 3. Divider */
    switch(port->content.audio->SampFreq)
    {
        case AV_AUD_FS_32KHZ:
            Divider = 32/2;
            break;
        case AV_AUD_FS_44KHZ:
            Divider = 44/2;
            break;
        case AV_AUD_FS_48KHZ:
            Divider = 48/2;
            break;
        case AV_AUD_FS_88KHZ:
            Divider = 88/2;
            break;
        case AV_AUD_FS_96KHZ:
            Divider = 96/2;
            break;
        case AV_AUD_FS_176KHZ:
            Divider = 176/2;
            break;
        default:
            Divider = 192/2;
            break;
    }
    /* Step 4. Tune */
    ExpectCts = (TuneN*TmdsClk)/Divider;
    while(ExpectCts > 0xF0000)
    {
        TuneN = TuneN >> 1;
        ExpectCts = (TuneN*TmdsClk)/Divider;
    }
    return TuneN;
}

/**
 * @brief  reset port
 * @return AvOk: success
 * @note
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiResetPort(pin AvPort *port))
{
    AvRet ret = AvOk;

    if(port->type == HdmiRx)
    {
        AvUapiOutputDebugMessage("--------------Port[%d][%d] Reset Port------------", port->device->index, port->index);
        Gsv6k5DisableRxHpa(port);
        /* EQ reset to default state */
        Gsv6k5RxManualEQUpdate(port, 1);
    }
    else if(port->type == HdmiTx)
    {
        switch(port->index)
        {
            case 4: /* tx port 0 */
            case 5: /* tx port 0 */
            case 6: /* tx port 0 */
            case 7: /* tx port 0 */
                /* Reset Tx */
                Gsv6k5AvUapiTxDisableCore(port);
                Gsv6k5AvUapiTxEnableCore(port);
                /* Edid will be reread 15 times if failed */
                GSV6K5_TXPHY_set_TX_EDID_TRY_TIMES(port,15);
                break;
            default:
                break;
        }
    }

    return ret;
}

/**
 * @brief  enable port
 * @return AvOk: success
 * @note
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiEnablePort(pin AvPort *port))
{
    return AvOk;
}

/**
 * @brief  free run rx port
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxEnableFreeRun(pin AvPort *port, bool enable))
{
    return AvOk;
}

/**
 * @brief  get +5v status of Rx
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGet5VStatus(pin AvPort *port))
{
    AvRet ret = AvOk;

    Gsv6k5GetRx5VStatus(port);

    return ret;
}

/**
 * @brief  get rx port status
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetStatus(pin AvPort *port))
{
    uint8 value = 0;
    uint8 temp = 0;
    uint32 value32 = 0;
    uint32 temp32 = 0;
    uint8 DeepColor = 0;
    AvRet ret = AvOk;

    if(port->type == HdmiRx)
        Gsv6k5GetRx5VStatus(port);

    if(port->content.rx->Input5V == 1)
    {
        if((port->content.rx->Hpd == 0) &&
           (port->content.rx->HpdDelayExpire <= RxHpdDelayExpireThreshold))
        {
            port->content.rx->HpdDelayExpire = port->content.rx->HpdDelayExpire + 1;
            if((port->content.rx->HpdDelayExpire % 10) == 0)
                AvUapiOutputDebugMessage("Port[%d][%d]:Lock Delay = %d", port->device->index, port->index, port->content.rx->HpdDelayExpire);
        }
    }
    else
    {
        if(port->content.rx->HpdDelayExpire <= RxHpdDelayExpireThreshold)
            port->content.rx->HpdDelayExpire = port->content.rx->HpdDelayExpire + 1;
        port->content.rx->IsInputStable = 0;
        port->content.video->AvailableVideoPackets = 0;
        port->content.audio->AvailableAudioPackets = 0;
    }

    if((port->core.HdmiCore != -1) && (port->content.rx->Hpd == 1))
    {
        switch(port->index)
        {
            case 0:
                GSV6K5_PLL_get_RB_RXA_PLL_VCO_FREQ(port, &value32);
                GSV6K5_PLL_get_RB_RXA_PLL_SER_DIV(port, &value);
                break;
        }
        temp = (1<<value) * 5;
        value32 = value32/temp;
        if(value32 == 0)
            value32 = Gsv6k5ReadPllFreq(port);

        if(port->content.video->info.LaneFreq != value32)
        {
            /* calculate diff */
            if(port->content.video->info.LaneFreq > value32)
                temp32 = port->content.video->info.LaneFreq - value32;
            else
                temp32 = value32 - port->content.video->info.LaneFreq;
            port->content.video->info.LaneFreq = value32;
            AvUapiOutputDebugFsm("Port[%d][%d]:Rx Lane Frequency = %d", port->device->index, port->index, port->content.video->info.LaneFreq);
            /* only reset when diff is too big */
            if((temp32 > 5) || (port->content.video->info.LaneFreq <= 10))
            {
                port->content.rx->Lock.DeRegenLock = 0;
                port->content.rx->Lock.VSyncLock = 0;
                port->content.video->EmpTotal = 0;
                port->content.rx->Lock.PllLock = 0;
                port->content.video->info.TmdsFreq = 0;
                if(port->content.rx->H2p1FrlType == AV_FRL_NONE)
                    port->content.rx->Lock.EqLock = 0;
            }
        }

        /* HDMI Rx Core Processing */
        Gsv6k5AvUapiRxGetVideoLock(port);
        /* there is a mapped core existed */
        GSV6K5_RXDIG_get_RB_RX_HDMI_MODE(port, &(port->content.rx->HdmiMode));
        GSV6K5_RXDIG_get_RB_RX_PIXEL_REPETITION(port, &(port->content.video->PixelRepeatValue));
        GSV6K5_RXDIG_get_RB_RX_DEEP_COLOR_MODE(port, &DeepColor);
        /* SCDC monitor */
        Gsv6k5RxScdcMonitor(port);

        /* Input Lock Stage */
        if((port->type == HdmiRx) && (port->content.rx->Lock.PllLock == 1))
        {
            if((port->content.rx->H2p1FrlType == AV_FRL_NONE) && (port->content.rx->H2p1FrlManual == 0))
            {
                /* remove deep color */
                if(DeepColor == 1) /* 10-bit */
                    value32 = (value32<<2)/5;
                else if(DeepColor == 2) /* 12-bit */
                    value32 = (value32<<1)/3;
                port->content.video->info.TmdsFreq = value32;
            }

            if((port->content.rx->Lock.DeRegenLock == 1) &&
               (port->content.rx->Lock.EqLock == 1) &&
               (port->content.rx->Lock.VSyncLock == 1))
            {
                if(port->content.rx->IsInputStable == 0)
                {
                    port->content.video->AvailableVideoPackets = 0;
                    port->content.video->timing.Vic = 0;
                    port->content.audio->AvailableAudioPackets = 0;
                    /* CED status update */
                    if(port->content.rx->H2p1FrlType != AV_FRL_NONE)
                    {
                        Gsv6k5RxScdcCedClear(port);
                    }
                    else
                    {
                        GSV6K5_RXDIG_set_CED_EN(port, 1);
                        GSV6K5_RXFRL_set_FRL_CED_EN(port, 1);
                    }
                    /* when EQ locked to start audio detection,
                       avoid garbage audio packets detection */
                    GSV6K5_RXAUD_set_CLEAR_AUDIO_HB_CONTENT(port, 1);
                    GSV6K5_RXAUD_set_CLEAR_AUDIO_HB_CONTENT(port, 0);
                    InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount = 0;
                }
                port->content.rx->IsInputStable = 1;
                if(InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount < 0xFFFFFFFF)
                    InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount = InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount + 1;
            }
            else
            {
                port->content.rx->IsInputStable = 0;
                if((port->content.rx->Lock.EqLock == 0) ||
                   ((port->content.rx->H2p1FrlType == AV_FRL_NONE) &&
                    (port->content.rx->H2p1FrlManual == 0) &&
                    (port->content.rx->Lock.DeRegenLock == 0) &&
                    (port->content.rx->Lock.VSyncLock == 0) &&
                    (port->content.video->info.TmdsFreq < 155)))
                    Gsv6k5RxManualEQUpdate(port, 0);
            }
        }

        /* Check Re-Training */
        if(port->content.rx->H2p1FrlStat == AV_FRL_TRAIN_SUCCESS)
        {
            GSV6K5_INT_get_RX1_FRL_TRAINING_PASS_INT_ST(port, &value);
            GSV6K5_RXSCDC_get_FLT_NO_RETRAIN(port, &temp);
            if((value == 1) && (temp == 0))
            {
                if(port->content.rx->IsFreeRun == 1)
                {
                    port->content.rx->Lock.EqLock = 0;
                    Gsv6k5RxManualEQUpdate(port, 1);
                    AvUapiOutputDebugMessage("Port[%d][%d]:Re-Training", port->device->index, port->index);
                }
                port->content.rx->IsFreeRun = 0;
                if(port->content.rx->H2p1FrlLockDelay >= (RxFrlLockExpireThreshold*2))
                {
                    port->content.rx->Hpd = AV_HPD_TOGGLE;
                    AvUapiOutputDebugMessage("Port[%d][%d]:HPD for Re-Training failure", port->device->index, port->index);
                }
                else
                {
                    port->content.rx->H2p1FrlLockDelay = port->content.rx->H2p1FrlLockDelay + 1;
                    //AvUapiOutputDebugMessage("Calc_Delay%d",port->content.rx->H2p1FrlLockDelay);
                }
            }
            if((port->content.rx->H2p1FrlType > AV_FRL_6G4L) && (temp == 1))
                temp = 1;
            else if((port->content.rx->H2p1FrlType == AV_FRL_3G3L) && (temp == 1))
            {
                Gsv6k5RxScdcCedClear(port);
                temp = 0;
            }
            else
                temp = 0;
            GSV6K5_RXFRL_get_FRL_CED_EN(port, &value);
            if(temp == value)
            {
                GSV6K5_RXDIG_set_NO_RR(port, 1);
                GSV6K5_RXFRL_set_FRL_CED_EN(port, (1-temp));
                GSV6K5_RXDIG_set_DBG_RR_REQ(port, 0);
                GSV6K5_RXDIG_set_DBG_RR_REQ(port, 1);
                AvUapiOutputDebugMessage("Port[%d][%d]:No Retrain%d", port->device->index, port->index,temp);
            }
        }

        if(port->content.rx->IsInputStable == 1)
        {
            /* CRC check start */
            if(InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount == 300)
            {
                InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].CrcError = 0;
                GSV6K5_VSP_get_RX1_CRC_READBACK(port, &temp32);
                GSV6K5_VSP_set_RX1_CRC_GOLDEN(port, temp32);
                GSV6K5_VSP_set_RX1_PRBS_CHECK_EN(port, 0);
                GSV6K5_VSP_set_RX1_PRBS_CHECK_EN(port, 1);
                /*
                uint32 ErrCnt[4];
                GSV6K5_RXSCDC_get_CED_LN0_ERR_CNT(port, &ErrCnt[0]);
                GSV6K5_RXSCDC_get_CED_LN1_ERR_CNT(port, &ErrCnt[1]);
                GSV6K5_RXSCDC_get_CED_LN2_ERR_CNT(port, &ErrCnt[2]);
                GSV6K5_RXSCDC_get_CED_LN3_ERR_CNT(port, &ErrCnt[3]);
                AvUapiOutputDebugFsm("CED:%d,%d,%d,%d,",ErrCnt[0],ErrCnt[1],ErrCnt[2],ErrCnt[3]);
                */
            }
            else if(InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount == 60)
            {
#if AvEnableDetailTiming
                /*
                GSV6K5_RXFRL_set_FRL_CED_DEBUG_MODE(port, 1);
                GSV6K5_RXFRL_set_FRL_CED_ERR_CLR(port, 1);
                GSV6K5_RXFRL_set_FRL_CED_ERR_CLR(port, 0);
                */
                /* timing print */
                uint8 RbFreq[5];
                AvHalI2cReadMultiField(GSV6K5_RXFRL_MAP_ADDR(port), 0xAD, 4, RbFreq+0);
                AvUapiOutputDebugFsm("%d:%d,%d,%d,%d,%d,%d,%d,%d, %d,%d,%d,%d,%d,%d,  0x%x,0x%x,0x%x,0x%x,0x%x",
                                         port->content.video->timing.Vic,
                                         port->content.video->timing.HPolarity,
                                         port->content.video->timing.VPolarity,
                                         port->content.video->timing.Interlaced,
                                         port->content.video->timing.HActive,
                                         port->content.video->timing.VActive,
                                         port->content.video->timing.HTotal,
                                         port->content.video->timing.VTotal,
                                         port->content.video->timing.FrameRate,
                                         port->content.video->timing.VFront,
                                         port->content.video->timing.VSync,
                                         port->content.video->timing.VBack,
                                         port->content.video->timing.HFront,
                                         port->content.video->timing.HSync,
                                         port->content.video->timing.HBack,
                                         RbFreq[0],RbFreq[1],RbFreq[2],RbFreq[3],RbFreq[4]
                    );
#endif
            }
            else if(InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].LoopCount > 300)
            {
                /* CRC check routine */
                GSV6K5_VSP_get_RX1_CRC_FRAME_ERR_CNT(port, &temp32);
                if(temp32 != InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].CrcError)
                {
                    InternalResource[port->device->index-Gsv6k5DeviceIndexOffset].CrcError = temp32;
                    //AvUapiOutputDebugMessage("Error Count: %d", temp32);
                }
            }

            GSV6K5_RXAUD_get_ACR_PKT_DET(port, &value);
            if(value == 1)
            {
                /* only set APLL lock process after N/CTS is stable */
                GSV6K5_RXDIG_get_RX_APLL_MAN_START_CALC(port, &value);
                if(value == 1)
                {
                    /* Get Audio Fs Frequency */
                    GSV6K5_RXDIG_get_RB_RX_AUD_128FS_FREQ(port, &temp32);
                    port->content.audio->AudLRclk = (1+((temp32*1000)>>15)) & 0xFFFC;
                    GSV6K5_RXDIG_set_APLL_CHK_USDW_INRANGE_DELTA(port, 0x800000);
                    value32 = 0x200;
                    if(port->content.audio->AudLRclk != 0)
                        value32 = (value32*48)/port->content.audio->AudLRclk;
                    if(port->type == HdmiRx)
                        GSV6K5_RXDIG_get_RB_RX_AUDIO_LAYOUT(port, &temp);
                    else
                        temp = 0;
                    if(temp == 1)
                        value32 = value32>>2;
                    value32 = value32<<16;
                    GSV6K5_RXDIG_set_APLL_CODE_DELTA(port, value32);
                    AvUapiOutputDebugMessage("Port[%d][%d]:Fs=%dKHz,%x", port->device->index, port->index,port->content.audio->AudLRclk,value32);
                    /* check function in DP application mode */
                    GSV6K5_RXDIG_set_N_CTS_STABLE_DET_START(port, 0);
                    GSV6K5_RXDIG_set_RX_APLL_MAN_START_CALC(port, 0);
                    GSV6K5_RXAUD_set_DISCARD_CSDATA_DIS(port, 1);
                    GSV6K5_RXAUD_set_CLEAR_AUDIO_HB_CONTENT(port, 1);
                    GSV6K5_RXAUD_set_CLEAR_AUDIO_HB_CONTENT(port, 0);
                    if(port->type == HdmiRx)
                    {
                        GSV6K5_INT_set_RX1_PKT_UNDFL_CLEAR(port,1);
                        GSV6K5_INT_set_RX1_PKT_OVFL_CLEAR(port, 1);
                    }
                    port->content.rx->Lock.AudioLock = 0;
                }
                /* Real Apll Lock Process*/
                else
                {
                    GSV6K5_RXAUD_get_RX_AUDIO_PLL_LOCKED(port, &temp);
                    /* Adjust Lock Detection if needed */
                    AvUapiRxGetHdmiAcrInfo(port);
                    port->content.rx->Lock.AudioLock = temp;
                    if(temp == 1)
                        port->content.audio->AvailableAudioPackets |= AV_BIT_AUDIO_SAMPLE_PACKET;
                    else
                        port->content.audio->AvailableAudioPackets &= (~AV_BIT_AUDIO_SAMPLE_PACKET);
                    /* Audio Process Routine */
                    if(port->content.rx->Lock.AudioLock)
                    {
                        /* Get internal actual audio mute status */
                        Gsv6k5AvUapiRxGetAudioInternalMute(port);
                        /* Possible Incorrect Garbage Audio Type Recovery */
                        AvHalI2cReadField8(GSV6K5_RXAUD_MAP_ADDR(port), 0x17, 0x1E, 0, &value);
                        if((value != 0) && (port->type == HdmiRx))
                        {
                            GSV6K5_RXAUD_set_CLEAR_AUDIO_HB_CONTENT(port, 1);
                            GSV6K5_RXAUD_set_CLEAR_AUDIO_HB_CONTENT(port, 0);
                        }
                        /* Packet Type */
                        AvUapiRxGetPacketType(port);
                        /* Set DSD Audio Mode */
                        GSV6K5_RXAUD_get_MAN_TDM_ENABLE(port, &value);
                        if((port->content.audio->AudType == AV_AUD_TYPE_DSD) ||
                           (port->content.audio->AudType == AV_AUD_TYPE_DST))
                            temp = 0;
                        else
                            temp = 1;
#ifdef AvEnableTDMAudioMode
                        GSV6K5_RXAUD_set_SYNC_POL(port, 0); /* 0 = LRCLK Positive, 1 = LRCLK Negative */
                        GSV6K5_RXAUD_set_SYNC_STYLE(port, 2); /* LRCLK 50% in TDM */
                        GSV6K5_RXAUD_set_TDM_ENABLE(port, 1);
                        GSV6K5_RXAUD_set_MAN_TDM_ENABLE(port, 1);
                        GSV6K5_RXAUD_set_MAN_CHANNEL_CNT_ENABLE(port, 1);
                        GSV6K5_RXAUD_set_CHANNEL_CNT_ENABLE(port, 8);
#else
                        if(temp != value)
                            GSV6K5_RXAUD_set_MAN_TDM_ENABLE(port, temp);
#endif
                        /* Channel Number */
                        GSV6K5_RXDIG_get_RB_RX_AUDIO_LAYOUT(port, &port->content.audio->Layout);
                        if(port->content.audio->Layout == 0)
                        {
                            port->content.audio->ChanNum = 2;
                            GSV6K5_RXAUD_get_RX_AUDIO_PIN_NUM(port, &value);
                            if((port->content.audio->AudType == AV_AUD_TYPE_DSD) ||
                               (port->content.audio->AudType == AV_AUD_TYPE_DST))
                                temp = 1;
                            else
                                temp = 0;
#ifdef AvEnableTDMAudioMode
                            GSV6K5_RXAUD_set_RX_AUDIO_PIN_NUM(port, 0);
#else
                            if(value != temp)
                                GSV6K5_RXAUD_set_RX_AUDIO_PIN_NUM(port, temp);
#endif
                        }
                        else
                        {
                            GSV6K5_RXAUD_get_AUD_IF_PKT_DET(port, &value);
                            if(value == 1)
                            {
                                AvHalI2cReadField8(GSV6K5_RXINFO_MAP_ADDR(port),0x1D,0xFF,0,&value);
                                port->content.audio->ChanNum = (value & 0x07) + 1;
                                /* Check Audio Channel Using Logic Rather than Packet */
                                GSV6K5_RXAUD_get_NUM_CH(port, &value);
                                temp = (value-1)>>1;

                                if((port->content.audio->AudType == AV_AUD_TYPE_HBR) ||
                                   (port->content.audio->AudType == AV_AUD_TYPE_DSD) ||
                                   (port->content.audio->AudType == AV_AUD_TYPE_DST))
                                    temp = 3;
#ifdef AvEnableTDMAudioMode
                                GSV6K5_RXAUD_set_RX_AUDIO_PIN_NUM(port, 0);
#else
                                GSV6K5_RXAUD_get_RX_AUDIO_PIN_NUM(port, &value);
                                if(value != temp)
                                    GSV6K5_RXAUD_set_RX_AUDIO_PIN_NUM(port, temp);
#endif
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        port->content.rx->IsFreeRun = 0;
        port->content.rx->IsInputStable = 0;
        port->content.rx->VideoEncrypted = 0;
        port->content.rx->Lock.AudioLock = 0;
        port->content.rx->Lock.DeRegenLock = 0;
        port->content.rx->Lock.PllLock = 0;
        port->content.rx->Lock.VSyncLock = 0;
        port->content.rx->Lock.EqLock = 0;
        port->content.rx->HdmiMode = 0;
        port->content.video->InCs = AV_CS_AUTO;
        port->content.video->OutCs = AV_CS_AUTO;
        port->content.video->DscStream = 0;
        port->content.video->VrrStream = 0;
        port->content.video->EmpTotal = 0;
        port->content.rx->H2p1FrlType = AV_FRL_NONE;
        port->content.rx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
    }

    if((port->content.rx->HdmiMode == 0) && (port->content.rx->IsInputStable))
    {
        port->content.video->Y = AV_Y2Y1Y0_RGB;
        port->content.video->InCs = AV_CS_RGB;
    }

    if((port->content.rx->HdmiMode) && (port->content.rx->IsInputStable) && (port->type == HdmiRx))
    {
        AvHalI2cReadField8(GSV6K5_RXAUD_MAP_ADDR(port), 0x19, 0xFF, 0, &temp);
        /* AVI_INFO_FRAME */
        value = (temp>>6) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_AV_INFO_FRAME);
        /* AUDIO_INFO */
        value = (temp>>7) & 0x01;
        Gsv6k5SetAudioPacketFlag(AV_BIT_AUDIO_INFO_FRAME);
        AvHalI2cReadField8(GSV6K5_RXAUD_MAP_ADDR(port), 0x18, 0xFF, 0, &temp);
        /* SPD_INFO */
        value = (temp>>0) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_SPD_PACKET);
        /* MS_INFO */
        value = (temp>>1) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_MPEG_PACKET);
        /* VS_INFO */
        value = (temp>>2) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_VS_PACKET);
        /* ACP_PCKT */
        value = (temp>>3) & 0x01;
        Gsv6k5SetAudioPacketFlag(AV_BIT_ACP_PACKET);
        /* ISRC1_PCKT */
        value = (temp>>4) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_ISRC1_PACKET);
        /* ISRC2_PCKT */
        value = (temp>>5) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_ISRC2_PACKET);
        /* Gamut */
        value = (temp>>6) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_GMD_PACKET);
        /* ACR Packet */
        value = (temp>>7) & 0x01;
        Gsv6k5SetAudioPacketFlag(AV_BIT_ACR_PACKET);
        AvHalI2cReadField8(GSV6K5_RXAUD_MAP_ADDR(port), 0x17, 0xFF, 0, &temp);
        /* GC */
        value = (temp>>0) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_GC_PACKET);
        /* HDR_INFO */
        value = (temp>>6) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_HDR_PACKET);
        /* AMD Packet */
        value = (temp>>5) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_AMD_PACKET);
        /* EMP Packet */
        value = (temp>>7) & 0x01;
        Gsv6k5SetVideoPacketFlag(AV_BIT_EMP_PACKET);
        /* Channel Status */
        GSV6K5_RXAUD_get_CSDATA_VALID(port, &value);
        Gsv6k5SetAudioPacketFlag(AV_BIT_AUDIO_CHANNEL_STATUS);
        /* AvMute */
        GSV6K5_INT_get_RX1_AVMUTE_RAW_ST(port, &(port->content.video->Mute.AvMute));
        /* Hdmi Mode */
        GSV6K5_INT_get_RX1_HDMI_MODE_RAW_ST(port, &(port->content.rx->HdmiMode));
        /* Interlace */
        GSV6K5_RXDIG_get_RB_RX_INTERLACED(port, &(port->content.video->timing.Interlaced));
        /* Audio Information Protection */
        if((port->content.audio->AudType == AV_AUD_TYPE_UNKNOWN) ||
           (port->content.rx->Lock.AudioLock == 0))
            port->content.audio->AvailableAudioPackets = 0;
    }

    return ret;
}

/**
 * @brief  set HPA = high for corresponding port
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxSetHpdUp(pin AvPort *port))
{
    AvRet ret = AvOk;

    Gsv6k5EnableRxHpa(port);
    AvUapiOutputDebugMessage("---------------Port[%d][%d] Power Up------------", port->device->index, port->index);
    return ret;
}

/**
 * @brief  set HPA = low for corresponding port
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxSetHpdDown(pin AvPort *port))
{
    AvRet ret = AvOk;

    Gsv6k5DisableRxHpa(port);
    Gsv6k5SetRxH2p1Mode(port, 0);
    AvUapiOutputDebugMessage("--------------Port[%d][%d] Power Down-----------", port->device->index, port->index);

    return ret;
}

/**
 * @brief  mute tx tmds output
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxMuteTmds(pin AvPort *port, bool mute))
{
    uint8 Enable = 0;
    if(mute == 0)
        Enable = 1;
    Gsv6k5ToggleTmdsOut(port, Enable);
    AvUapiOutputDebugFsm("AvUapiTxMuteTmds");
    return AvOk;
}

/**
 * @brief  get tx port status
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxGetStatus(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    uint8 tmp = 0;
    uint8 LocalPllLock = 0;
    uint8 NewValue = 0;
    uint8 Freq3G4Plus = 0;
    uint8 ConfigFlag = 0;
    AvFrlStat TxFrlStat = AV_FRL_TRAIN_IDLE;
    uint32 value32 = 0;
    //uint32 temp32 = 0;
    uint8 Packet[31];
    AvPort *FromPort = NULL;
    AvPort *UpperPort = NULL;
    FromPort = (AvPort*)(Gsv6k5FindHdmiRxFront(port));
    UpperPort = (AvPort*)(port->content.RouteVideoFromPort);

    if((port->content.tx->Hpd != AV_HPD_FORCE_LOW) &&
       (port->content.tx->Hpd != AV_HPD_FORCE_HIGH))
    {
        GSV6K5_TXPHY_get_HPD_RAW_STATE(port, &value);
#ifndef AvDisableMsenDetection
        if((value == 1) && (port->content.tx->InfoReady != 0) &&
           ((port->content.tx->PinMapping & 0x08) == 0x00) &&
           (port->content.tx->H2p1FrlStat != AV_FRL_TRAIN_ONGOING) &&
           (port->content.tx->H2p1FrlStat != AV_FRL_TRAIN_NO_TIMEOUT))
        {
            GSV6K5_TXPHY_get_MSEN_RAW_STATE(port, &NewValue);
            /* MSEN detection */
            if(NewValue == 0xf)
            {
                if(port->content.tx->InfoReady > TxVideoManageThreshold)
                {
                    port->content.tx->Hpd = AV_HPD_FORCE_LOW;
                    port->content.tx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
                    AvUapiOutputDebugMessage("Port[%d][%d]:MSEN Triggers TMDS Disable", port->device->index, port->index);
                }
                else
                {
                    port->content.tx->H2p1FrlExtraDelay = 0;
                    port->content.tx->InfoReady = 0;
                }
            }
        }
#endif
        /* 1.1 Tx Detection Power Down */
        switch(port->index)
        {
            case 4:
                GSV6K5_INT_get_TXA_HPD_INTR_INT_ST(port, &NewValue);
                if(NewValue == 1)
                {
                    GSV6K5_INT_set_TXA_HPD_INTR_CLEAR(port, 1);
                    value = 0;
                    /* Multi-Output Audio ACR Protection */
                    GSV6K5_SEC_get_TXPORT_A_SRC_SEL(port, &Packet[0]);
                    if(Packet[0] == 0x03)
                        GSV6K5_SEC_set_TXPORT_A_SRC_SEL(port, 0x01);
                }
                /* ES2 Fix on MSENSE */
                if((value == 1) && ((port->content.tx->PinMapping & 0x08) == 0x00))
                {
                    GSV6K5_INT_get_TXA_RXSENSE_INT_ST(port, &NewValue);
                    if(NewValue == 1)
                    {
                        GSV6K5_INT_set_TXA_RXSENSE_CLEAR(port, 1);
                        if((port->content.tx->InfoReady < TxVideoManageThreshold) ||
                           (port->content.tx->InfoReady >= (TxVideoManageThreshold+2)))
                        {
                            value = 0;
                        }
                    }
                }
                break;
            case 5:
                GSV6K5_INT_get_TXB_HPD_INTR_INT_ST(port, &NewValue);
                if(NewValue == 1)
                {
                    GSV6K5_INT_set_TXB_HPD_INTR_CLEAR(port, 1);
                    value = 0;
                    /* Multi-Output Audio ACR Protection */
                    GSV6K5_SEC_get_TXPORT_B_SRC_SEL(port, &Packet[0]);
                    if(Packet[0] == 0x03)
                        GSV6K5_SEC_set_TXPORT_B_SRC_SEL(port, 0x01);
                }
                /* ES2 Fix on MSENSE */
                if(value == 1)
                {
                    GSV6K5_INT_get_TXB_RXSENSE_INT_ST(port, &NewValue);
                    if(NewValue == 1)
                    {
                        GSV6K5_INT_set_TXB_RXSENSE_CLEAR(port, 1);
                        if((port->content.tx->InfoReady < TxVideoManageThreshold) ||
                           (port->content.tx->InfoReady >= (TxVideoManageThreshold+2)))
                        {
                            value = 0;
                        }
                    }
                }
                break;
            case 6:
                GSV6K5_INT_get_TXC_HPD_INTR_INT_ST(port, &NewValue);
                if(NewValue == 1)
                {
                    GSV6K5_INT_set_TXC_HPD_INTR_CLEAR(port, 1);
                    value = 0;
                    /* Multi-Output Audio ACR Protection */
                    GSV6K5_SEC_get_TXPORT_C_SRC_SEL(port, &Packet[0]);
                    if(Packet[0] == 0x03)
                        GSV6K5_SEC_set_TXPORT_C_SRC_SEL(port, 0x01);
                }
                /* ES2 Fix on MSENSE */
                if(value == 1)
                {
                    GSV6K5_INT_get_TXC_RXSENSE_INT_ST(port, &NewValue);
                    if(NewValue == 1)
                    {
                        GSV6K5_INT_set_TXC_RXSENSE_CLEAR(port, 1);
                        if((port->content.tx->InfoReady < TxVideoManageThreshold) ||
                           (port->content.tx->InfoReady >= (TxVideoManageThreshold+2)))
                        {
                            value = 0;
                        }
                    }
                }
                break;
            case 7:
                GSV6K5_INT_get_TXD_HPD_INTR_INT_ST(port, &NewValue);
                if(NewValue == 1)
                {
                    GSV6K5_INT_set_TXD_HPD_INTR_CLEAR(port, 1);
                    value = 0;
                    /* Multi-Output Audio ACR Protection */
                    GSV6K5_SEC_get_TXPORT_D_SRC_SEL(port, &Packet[0]);
                    if(Packet[0] == 0x03)
                        GSV6K5_SEC_set_TXPORT_D_SRC_SEL(port, 0x01);
                }
                /* ES2 Fix on MSENSE */
                if(value == 1)
                {
                    GSV6K5_INT_get_TXD_RXSENSE_INT_ST(port, &NewValue);
                    if(NewValue == 1)
                    {
                        GSV6K5_INT_set_TXD_RXSENSE_CLEAR(port, 1);
                        if((port->content.tx->InfoReady < TxVideoManageThreshold) ||
                           (port->content.tx->InfoReady >= (TxVideoManageThreshold+2)))
                        {
                            value = 0;
                        }
                    }
                }
                break;
        }

        if(port->content.tx->Hpd != AV_HPD_FORCE_LOW)
            port->content.tx->Hpd = (AvHpdState)value;
        /* 1.2 InfoReady Management */
        if(port->content.tx->InfoReady == 1)
        {
            if(port->content.tx->H2p1FrlExtraDelay != 0)
            {
                port->content.tx->H2p1FrlExtraDelay = port->content.tx->H2p1FrlExtraDelay - 1;
                if((port->content.tx->H2p1FrlExtraDelay%20) == 0)
                    AvUapiOutputDebugMessage("Port[%d][%d]:Extra Delay%d", port->device->index, port->index, port->content.tx->H2p1FrlExtraDelay);
                port->content.tx->InfoReady = 0;
            }
        }
        /* 1.3 Edid status reset */
        if(port->content.tx->Hpd == AV_HPD_LOW)
        {
            port->content.tx->EdidReadSuccess = AV_EDID_RESET;
            GSV6K5_TXPHY_set_TX_SCDC_EN(port,0);
        }
    }

    /* 2. Local Pll Lock Status Check, pass the test to get LocalPllLock */
    if((port->content.tx->Hpd != AV_HPD_LOW) && (FromPort != NULL))
    {
        /* 2.1 capture pll lock low */
        if(port->content.tx->InfoReady > TxVideoManageThreshold)
        {
            switch(port->index)
            {
                case 4:
                    GSV6K5_PLL_get_RB_TXA_PLL_LOCK_CAPTURED(port, &value);
                    break;
                case 5:
                    GSV6K5_PLL_get_RB_TXB_PLL_LOCK_CAPTURED(port, &value);
                    break;
                case 6:
                    GSV6K5_PLL2_get_RB_TXC_PLL_LOCK_CAPTURED(port, &value);
                    break;
                case 7:
                    GSV6K5_PLL2_get_RB_TXD_PLL_LOCK_CAPTURED(port, &value);
                    break;
            }
            if(value == 0)
                Gsv6k5TxPllUnlockClear(port);
            else
            {
                AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port), 0xF2, 0xFF, 0x0, &NewValue);
                if(NewValue == 0xF0)
                    value = 0;
                if((value == 1) && (port->core.HdmiCore != -1))
                {
                    if((port->content.tx->H2p1FrlType != AV_FRL_NONE) || (port->content.tx->H2p1FrlManual != 0))
                    {
                        GSV6K5_TXFRL_get_RB_FRL_XTAL_HBLANK_VALID(port, &NewValue);
                        if(NewValue == 0)
                        {
                            GSV6K5_TXDIG_get_TX_GC_CD(port, &NewValue);
                            if((NewValue == 5) || (NewValue == 6))
                            {
                                GSV6K5_TXDIG_set_TX_GC_CD(port, 4);
                                GSV6K5_TXDIG_set_TX_GC_CD(port, NewValue);
                                value = 0;
                                AvUapiOutputDebugMessage("Port[%d][%d]:DeepColor Protect", port->device->index, port->index);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            switch(port->index)
            {
                case 4:
                    GSV6K5_PLL_get_RB_TXA_PLL_LOCK(port, &value);
                    break;
                case 5:
                    GSV6K5_PLL_get_RB_TXB_PLL_LOCK(port, &value);
                    break;
                case 6:
                    GSV6K5_PLL2_get_RB_TXC_PLL_LOCK(port, &value);
                    break;
                case 7:
                    GSV6K5_PLL2_get_RB_TXD_PLL_LOCK(port, &value);
                    break;
            }
#if (AvEnableConsistentFrl == 1)
            if(value == 0)
#else
            if((value == 0) && ((FromPort->content.rx->IsFreeRun == 1) || (FromPort->type == VideoGen)))
#endif
            {
                GSV6K5_TXFRL_get_FRL_RATE(port, &tmp);
                NewValue = (uint8)(port->content.tx->H2p1FrlType);
                if(NewValue != tmp)
                {
                    GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 1);
                    GSV6K5_TXFRL_set_FRL_RATE(port, NewValue);
                    AvUapiOutputDebugMessage("Port[%d][%d]:Frl%d Mode Switch", port->device->index, port->index,NewValue);
                }
                Gsv6k5RpllProtect(port);
            }
        }
        if(value == 1)
        {
            if(FromPort->content.rx->IsFreeRun == 1)
            {
                /* Calculate TmdsFreq */
                ConfigFlag = FromPort->content.video->timing.Vic;
                value32 = FromPort->content.video->info.TmdsFreq;
                if(UpperPort != NULL)
                {
                    if(UpperPort->type == VideoScaler)
                    {
                        value = 1;
                        if(UpperPort->content.scaler->Hratio == 0)
                            value = value * 2;
                        else
                            value = value * UpperPort->content.scaler->Hratio;
                        if(UpperPort->content.scaler->Vratio == 0)
                            value = value * 2;
                        else
                            value = value * UpperPort->content.scaler->Vratio;
                        if(FromPort->content.video->timing.Interlaced == 1)
                            value32 = value32 << 1;
                        else
                        {
                            value32 = FromPort->content.video->info.TmdsFreq / value;
                            if(UpperPort->content.scaler->ColorSpace == AV_Y2Y1Y0_YCBCR_420)
                               value32 = value32 << 1;
                        }
                        if(FromPort->content.video->Cd == AV_CD_30)
                            value32 = (value32<<2)/5;
                        else if(FromPort->content.video->Cd == AV_CD_36)
                            value32 = (value32<<1)/3;
                        ConfigFlag = UpperPort->content.scaler->ScalerOutVic;
                    }
                    else if(UpperPort->type == VideoColor)
                    {
                        value32 = FromPort->content.video->info.TmdsFreq;
                        if((UpperPort->content.color->ColorOutSpace == AV_Y2Y1Y0_YCBCR_420) &&
                           (UpperPort->content.color->ColorInSpace  != AV_Y2Y1Y0_YCBCR_420))
                            value32 = value32 >> 1;
                        else if((UpperPort->content.color->ColorOutSpace != AV_Y2Y1Y0_YCBCR_420) &&
                                (UpperPort->content.color->ColorInSpace  == AV_Y2Y1Y0_YCBCR_420))
                            value32 = value32 << 1;;
                        if(FromPort->content.video->Cd == AV_CD_30)
                            value32 = (value32<<2)/5;
                        else if(FromPort->content.video->Cd == AV_CD_36)
                            value32 = (value32<<1)/3;
                        ConfigFlag = UpperPort->content.color->ColorInVic;
                    }
                }
                if((port->content.tx->H2p1FrlType == AV_FRL_NONE) && (port->content.tx->H2p1FrlManual == 0))
                    LocalPllLock = 1;
                else if(((port->content.video->info.TmdsFreq == (value32 + 1)) ||
                         (port->content.video->info.TmdsFreq == (value32 - 1)) ||
                         (port->content.video->info.TmdsFreq == value32)) &&
                        (port->content.video->info.DetectedVic == ConfigFlag))
                {
                    if(port->core.HdmiCore != -1)
                    {
                        /* 2.1.1 Tx Pixel Clock is stable */
                        GSV6K5_TXDIG_get_TX_GC_CD(port, &value);
                        if((value != 0) && (value != 4))
                        {
                            GSV6K5_INT_get_TX1_PLL_LOCKED_INT_ST(port, &value);
                            if(value == 1)
                            {
                                GSV6K5_INT_set_TX1_PLL_LOCKED_CLEAR(port, 1);
                                LocalPllLock = 0;
                            }
                            else
                                GSV6K5_INT_get_TX1_PLL_LOCKED_RAW_ST(port, &LocalPllLock);
                            if(LocalPllLock == 0)
                                Gsv6k5TxPllProtect(port, UpperPort, FromPort);
                        }
                        else
                            LocalPllLock = 1;
                        /* 2.1.2 Tx Timing is stable */
                        /*
                        if(LocalPllLock == 1)
                        {
                            AvHalI2cReadField8(GSV6K5_TXDIG_MAP_ADDR(port),0xFD,0x30,0,&value);
                            if(value != 0x30)
                                LocalPllLock = 0;
                        }
                        */
                    }
                    else
                        LocalPllLock = 1;
                }
                else
                {
                    port->content.video->info.TmdsFreq = value32;
                    port->content.video->info.DetectedVic = ConfigFlag;
                    port->content.video->AvailableVideoPackets = 0;
                    if(port->content.tx->H2p1FrlManual != 0)
                        AvUapiTxSetHdmiDeepColor(port);
                    AvUapiOutputDebugMessage("Port[%d][%d]:FRL Timing change", port->device->index, port->index);
                    //GSV6K5_PLL_set_TXA_PLL_ENABLE_MAN_EN(port, 1);
                    //GSV6K5_PLL_set_TXA_PLL_ENABLE_MAN_EN(port, 0);
                }
            }
        }
    }
    /* 2.2 TMDS Output Mode Switch */
    if(port->content.tx->InfoReady == 1)
    {
        LocalPllLock = Gsv6k5SetTxH2p1Mode(port);
        if(LocalPllLock == 1)
            LocalPllLock = Gsv6k5TxPllProtect(port, UpperPort, FromPort);
    }
    /* 2.3 Protect TMDS Output in Consistent FRL mode */
#if (AvEnableConsistentFrl == 1) || (AvEnableTxSingleLaneMode == 1)
    if((port->content.tx->InfoReady >= TxHdcpManageThreshold) &&
       (((port->content.tx->H2p1FrlType != AV_FRL_NONE) && (port->content.tx->H2p1FrlStat == AV_FRL_TRAIN_SUCCESS)) ||
        (port->content.tx->H2p1FrlManual != 0)))
    {
        /* 2.3.1 Trigger Tx Core Mode Configuration */
        port->content.tx->Lock.DeRegenLock = LocalPllLock;
        /* 2.3.2 Mute FRL Output Valid Data */
        if(FromPort->content.rx->IsFreeRun == 0)
            NewValue = 1;
        else
            NewValue = 1 - LocalPllLock;
        AvHalI2cReadField8(GSV6K5_TXFRL_MAP_ADDR(port),0x19,0xFF,0,&value);
        if(NewValue == 0)
        {
            if((value == 0x24) || (value == 0x00))
                NewValue = 0x00;
            else
                NewValue = 0x24;
        }
        else
        {
            if((value == 0x24) || (value == 0x34))
                NewValue = 0x34;
            else
                NewValue = 0x24;
        }
        if(value != NewValue)
        {
            if(LocalPllLock)
            {
                LocalPllLock = Gsv6k5TxPllProtect(port, UpperPort, FromPort);
            }
            GSV6K5_TXFRL_set_FRL_TX_FIFO_MANUAL_RST(port, 1);
            GSV6K5_TXFRL_set_FRL_TX_FIFO_MANUAL_RST(port, 0);
            AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x19,0xFF,0,NewValue);
            if(NewValue == 0x00)
                Gsv6k5ResetTxFifo(port);
            else if(NewValue == 0x34)
            {
                if(port->core.HdmiCore == 0)
                {
                    AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port),0x53,0xFF,0,0x04);//; Tx Color Depth = 8 bits
                    AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port),0x50,0xFF,0,0x00);//; Disable AudioInfo/AviInfo packet
                    AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port),0x51,0xFF,0,0x00);//; Disable GC/AudioSample/N_CTS packet
                    AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port),0x2B,0x80,0,0x00);//; Disable Tx CSC
                    AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port),0x1A,0xFF,0,0xB0);//; Audio Mute and default Layout 0
                    port->content.video->AvailableVideoPackets = 0;
                    port->content.audio->AvailableAudioPackets = 0;
                }
            }
        }
        /* 2.3.3 Force Lock For Further Tx FRL Routine */
        LocalPllLock = 1;
    }
    else
    {
        /* 2.3.4 Force LocalPllLock to continue output */
        LocalPllLock = 1;
    }
#else
    port->content.tx->Lock.DeRegenLock = port->content.tx->Lock.PllLock;
#endif
    /* 2.4 PLL unlock protection */
    if((port->content.tx->Lock.PllLock == 1) && (LocalPllLock == 0))
    {
        Gsv6k5ToggleTmdsOut(port, 0); /* Disable TMDS Out */
        AvUapiOutputDebugFsm("Port[%d][%d]:PllUnlock", port->device->index, port->index);
    }

    /* 3. Tx Pll Setting for unlock protection */
    if(LocalPllLock)
    {
        /* 3.1 Get Tx Pll Freq */
        if(port->index == 4)
        {
            GSV6K5_PLL_get_RB_TXA_PLL_VCO_FREQ(port, &value32);
            GSV6K5_PLL_get_RB_TXA_PLL_SER_DIV(port, &value);
        }
        else if(port->index == 5)
        {
            GSV6K5_PLL_get_RB_TXB_PLL_VCO_FREQ(port, &value32);
            GSV6K5_PLL_get_RB_TXB_PLL_SER_DIV(port, &value);
        }
        else if(port->index == 6)
        {
            GSV6K5_PLL2_get_RB_TXC_PLL_VCO_FREQ(port, &value32);
            GSV6K5_PLL2_get_RB_TXC_PLL_SER_DIV(port, &value);
        }
        else if(port->index == 7)
        {
            GSV6K5_PLL2_get_RB_TXD_PLL_VCO_FREQ(port, &value32);
            GSV6K5_PLL2_get_RB_TXD_PLL_SER_DIV(port, &value);
        }
        value32 = (value32/5)/(1<<value);
        /* 3.2 Clock Change Detected */
        if((port->content.tx->InfoReady > TxVideoManageThreshold) &&
           (port->content.video->info.LaneFreq != value32))
            LocalPllLock = 0;
        else if(value32 == 0)
        {
            LocalPllLock = Gsv6k5TxPllProtect(port, UpperPort, FromPort);
            AvUapiOutputDebugMessage("Port[%d][%d]:VCO reread", port->device->index, port->index);
        }
        /* 3.3 TMDS Frequency Update */
        port->content.video->info.LaneFreq = (uint16)value32;
        if((port->content.tx->H2p1FrlType == AV_FRL_NONE) && (port->content.tx->H2p1FrlManual == 0))
        {
            if(port->content.video->info.TmdsFreq != port->content.video->info.LaneFreq)
                port->content.video->AvailableVideoPackets = 0;
            port->content.video->info.TmdsFreq = port->content.video->info.LaneFreq;
        }
        /* 3.4 Get Tx Pll Freq */
        if(port->content.tx->H2p1FrlType == AV_FRL_NONE)
        {
            if(port->content.video->info.LaneFreq > 340)
                Freq3G4Plus = 1;
            else
                Freq3G4Plus = 0;
            /* 3.4.1 Set Phy Value Action */
            if(Freq3G4Plus == 1)
            {
                NewValue = 1;
                port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_SCDC;
            }
            else
                NewValue = 0;
            /* 3.4.2 Phy Mode Change */
            GSV6K5_TXPHY_get_TX_HDMI2_MODE_MAN(port, &value);
            if(value != NewValue)
            {
                GSV6K5_TXPHY_set_TX_HDMI2_MODE_MAN_EN(port, 1);
                GSV6K5_TXPHY_set_TX_HDMI2_MODE_MAN(port, NewValue);
            }
            /* 3.4.3 Scramble Change */
            GSV6K5_TXPHY_get_TX_SCR_EN_MAN(port, &value);
            if(value != NewValue)
            {
                GSV6K5_TXPHY_set_TX_SCR_EN_MAN(port, NewValue);
                GSV6K5_TXPHY_set_TX_SCR_EN_MAN_EN(port, 1);
            }
        }
        /* 3.5 SCDC Change */
        GSV6K5_TXPHY_get_EDID_ANALYZE_DONE(port, &value);
        if(value == 0)
            GSV6K5_TXPHY_set_EDID_ANALYZE_DONE(port, 1);
    }

    /* 4. Find Final Lock Status, only pass the test to have Tx Pll Lock */
    if(port->content.tx->InfoReady == 1)
    {
        /* 4.1 set Tx datapath */
        AvUapiTxSetHdmiModeSupport(port);
        Gsv6k5ResetTxFifo(port);
        Gsv6k5TxHBlankControl(port,0);
    }
    /* 4.2 Wait FRL Tx Timing Stable */
    if(port->content.tx->InfoReady == TxVideoManageThreshold)
    {
        if((port->content.tx->H2p1FrlType != AV_FRL_NONE) || (port->content.tx->H2p1FrlManual != 0))
        {
#if (AvEnableConsistentFrl != 1)
            AvHalI2cReadField8(GSV6K5_TXFRL_MAP_ADDR(port),0x63,0x80,0,&value);
            if(value != 0x80)
            {
                port->content.tx->InfoReady = TxVideoManageThreshold - 1;
                AvUapiOutputDebugMessage("Port[%d][%d]:Await FRL Timing", port->device->index, port->index);
            }
#endif
        }
    }
    /* 4.3 Schedule Tx Enable Order */
    Gsv6k5TxSchedule(port);
    if(port->content.tx->InfoReady == TxVideoManageThreshold)
    {
        AvUapiOutputDebugFsm("Port[%d][%d]:Tx Pixel Frequency = %d, %dbit, Lane = %d", port->device->index,
                             port->index, port->content.video->info.TmdsFreq,
                             port->content.video->Cd, port->content.video->info.LaneFreq);
        AvUapiTxPortSwapRead(port);
        Gsv6k5_PrintTxPinMappingReg(port);
        port->content.tx->InfoReady = port->content.tx->InfoReady + 1;
        /* 5.0 Tx Phy Setting */
        if(((port->content.tx->H2p1FrlType != AV_FRL_NONE) &&
            ((port->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_SUPPORT) != 0)) ||
           (port->content.tx->H2p1FrlManual != 0))
        {
            AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0xD0,0xFF,0,0xFF);
        }
        else
        {
            AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0xD0,0xFF,0,0xEE);
        }
#if AvEnableTxCtsPhySetting
        if((port->content.tx->H2p1FrlType == AV_FRL_NONE) && (port->content.tx->H2p1FrlManual == 0))
        {
            if(port->content.video->info.TmdsFreq > 450)
                value = 0x06;
            else if(port->content.video->info.TmdsFreq > 290)
                value = 0x05;
            else if(port->content.video->info.TmdsFreq > 140)
                value = 0x04;
            else
                value = 0x03;
            AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0xD5,0x0F,0,value);
        }
#endif
        /* 5.1 Protect for HDCP 2.2 Restart, disable EDID and SCDC in advance */
        GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 1);
        GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 0);
        /* 5.2 SCDC configuration routine */
        if((port->content.tx->EdidSupportFeature & AV_BIT_FEAT_SCDC) != 0x0)
        {
            /* 5.2.1 Protect FRL mode setting and impact on PLL */
            if((port->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_SUPPORT) != 0)
            {
                /* Get Downstream FRL state */
                value = Gsv6k5TxScdcAction(port, 0, 0xA8, 0x31, 0x00);
                NewValue = (uint8)(port->content.tx->H2p1FrlType);
                if(((value != NewValue) || (port->content.tx->H2p1FrlStat == AV_FRL_TRAIN_IDLE)) &&
                   (NewValue != 0))
                {
                    Gsv6k5ToggleTmdsOut(port, 1); /* Enable TMDS Out */
                    Gsv6k5TxScdcAction(port, 1, 0xA8, 0x10, 0x73); /* clear interrupts */
                    /* 5.2.1.1 new FRL initial setup, auto training */
                    GSV6K5_TXPHY_get_SCDC_TMDS_CFGED(port, &ConfigFlag);
                    if(ConfigFlag == 0)
                    {
                        AvUapiOutputDebugMessage("Port[%d][%d]:new FRL%d Auto Config", port->device->index, port->index, NewValue);
                    }
                    /* 5.2.1.2 new FRL training manual trigger */
                    else
                    {
                        AvUapiOutputDebugMessage("Port[%d][%d]:new FRL%d Manual Trigger", port->device->index, port->index, NewValue);
                    }
                    Gsv6k5TxFrlStart(port);
                }
                /* 5.2.1.3 FRL Manual configuration */
#if AvEnableTxSingleLaneMode
                else if(port->content.tx->H2p1FrlManual != 0)
                {
                    GSV6K5_TXFRL_set_FRL_TRAIN_FSM_DISABLE(port, 1);
                    Gsv6k5ToggleTmdsOut(port, 1); /* Enable TMDS Out */
                    GSV6K5_TXFRL_set_FRL_TRAIN_FSM_DISABLE(port, 0);
                    AvUapiOutputDebugMessage("Port[%d][%d]:Manual FRL enable", port->device->index, port->index);
                }
#endif
                else
                {
                    /* 5.2.1.4 From FRL to TMDS mode configuration */
                    if(NewValue == 0)
                    {
                        Gsv6k5ToggleTmdsOut(port, 0); /* Disable TMDS Out */
                        Gsv6k5TxScdcAction(port, 1, 0xA8, 0x20, 0x00); /* Disable Scrmable */
                        Gsv6k5TxScdcAction(port, 1, 0xA8, 0x31, 0x00); /* Disable FRL */
                        GSV6K5_TXPHY_get_SCDC_TMDS_CFGED(port, &value);
                        if(value == 1)
                            GSV6K5_TXPHY_set_SCDC_TMDS_CFGED(port, 1);
                        Gsv6k5ToggleTmdsOut(port, 1); /* Enable TMDS Out */
                        AvUapiOutputDebugMessage("Port[%d][%d]:Normal TMDS setup", port->device->index, port->index, NewValue);
                    }
                    /* 5.2.1.5 Enable Link Training Process if not enabled */
                    else
                    {
                        Gsv6k5ToggleTmdsOut(port, 1); /* Enable TMDS Out */
                        ConfigFlag = Gsv6k5TxScdcAction(port, 0, 0xA8, 0x10, 0x00);
                        if((ConfigFlag & 0x10) == 0x00) /* FRL_start is not set, do link training */
                        {
                            Gsv6k5TxScdcAction(port, 1, 0xA8, 0x10, 0x43); /* clear interrupts */
                            Gsv6k5TxFrlStart(port);
                            AvUapiOutputDebugMessage("Port[%d][%d]:new FRL%d Re-Training", port->device->index, port->index, NewValue);
                        }
                        else
                        {
                            GSV6K5_TXFRL_set_FRL_MAN_TX_SEND_GAP_EN(port, 0);
                            GSV6K5_TXFRL_set_FRL_MAN_TX_SEND_VID_EN(port, 0);
                            AvUapiOutputDebugMessage("Port[%d][%d]:new FRL%d Trained, Ignored", port->device->index, port->index, NewValue);
                        }
                    }
                }
            }
            /* 5.2.2 TMDS mode enable, Manual Write SCDC Scramble and Clock Divider */
            else
            {
                Gsv6k5ToggleTmdsOut(port, 0); /* Disable TMDS Out */
                GSV6K5_TXPHY_set_SCDC_TMDS_CFGED(port, 1);
                Gsv6k5ToggleTmdsOut(port, 1); /* Enable TMDS Out */
            }
        }
        else
        {
            GSV6K5_TXPHY_get_TX_TMDS_CLK_DRIVER_ENABLE(port, &value);
            if(value == 0)
                Gsv6k5ToggleTmdsOut(port, 1);
        }
        /* 5.3 Clear PLL Lock Status when entering stable */
        Gsv6k5TxPllUnlockClear(port);
    }

    /* 6. FRLink Status */
    if((port->content.tx->Hpd == AV_HPD_LOW) || (port->content.tx->InfoReady < TxVideoManageThreshold))
    {
        port->content.tx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
        port->content.tx->H2p1FrlPoll = AV_FRL_POLL_IDLE;
        port->content.tx->H2p1FrlCedRb = 0;
        port->content.tx->H2p1FrlExtraDelay = 0;
        /* No DDC FRL Auto Training after HPD pull up */
        GSV6K5_TXFRL_get_FRL_SKIP_ALL_DDC(port, &value);
        if(value != 1)
            GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 1);
    }
    else if((port->content.tx->H2p1FrlType != AV_FRL_NONE) &&
            ((port->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_SUPPORT) != 0))
    {
#if AvEnableTxCtsFrlTraining
        /* 6.1 no timeout */
        /* Force Manual FRL Test Pattern */
        /* TxFrlStat = AV_FRL_TRAIN_NO_TIMEOUT; */
        if((port->content.tx->H2p1FrlStat != AV_FRL_TRAIN_NO_TIMEOUT) &&
           (port->content.tx->H2p1FrlStat != AV_FRL_TRAIN_SUCCESS))
        {
            ConfigFlag = Gsv6k5TxScdcAction(port, 0, 0xA8, 0x35, 0x00);
            if(((ConfigFlag & 0x20) != 0x00) && (ConfigFlag != 0xFF))
            {
                TxFrlStat = AV_FRL_TRAIN_NO_TIMEOUT;
                AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x19,0xFF,0,0x79);
                AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x10,0xFF,0,0x57);
                GSV6K5_TXFRL_set_FRL_MAN_LN0_LTP_REQ(port,0x05);
                GSV6K5_TXFRL_set_FRL_MAN_LN1_LTP_REQ(port,0x06);
                GSV6K5_TXFRL_set_FRL_MAN_LN2_LTP_REQ(port,0x07);
                GSV6K5_TXFRL_set_FRL_MAN_LN3_LTP_REQ(port,0x08);
                AvUapiOutputDebugFsm("Port[%d][%d]:FRL%d CTS LTP Out", port->device->index, port->index, port->content.tx->H2p1FrlType);
            }
        }
#endif
        /* 6.2 status */
        if(TxFrlStat != AV_FRL_TRAIN_NO_TIMEOUT)
        {
            AvHalI2cReadField8(GSV6K5_TXFRL_MAP_ADDR(port),0x0C,0xFF,0,&value);
            if(port->content.tx->H2p1FrlStat == AV_FRL_TRAIN_IDLE)
                TxFrlStat = AV_FRL_TRAIN_IDLE;
            else if((value & 0xC0) != 0)
            {
                TxFrlStat = AV_FRL_TRAIN_FAIL;
#if AvEnableTxCtsFrlTraining
                AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0x08, 2, Packet+0);
                if((value == 0x80) && (Packet[0] == 0x04) && (Packet[1] == 0x6F))
                {
                    /* Force Manual FRL Test Pattern */
                    if(port->content.tx->H2p1FrlStat != AV_FRL_TRAIN_NO_TIMEOUT)
                    {
                        AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x19,0xFF,0,0x79);
                        AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x10,0xFF,0,0x57);
                        GSV6K5_TXFRL_set_FRL_MAN_LN0_LTP_REQ(port,0x05);
                        GSV6K5_TXFRL_set_FRL_MAN_LN1_LTP_REQ(port,0x06);
                        GSV6K5_TXFRL_set_FRL_MAN_LN2_LTP_REQ(port,0x07);
                        GSV6K5_TXFRL_set_FRL_MAN_LN3_LTP_REQ(port,0x08);
                        AvUapiOutputDebugFsm("Port[%d][%d]:FRL%d CTS LTP Out", port->device->index, port->index, port->content.tx->H2p1FrlType);
                    }
                    TxFrlStat = AV_FRL_TRAIN_NO_TIMEOUT;
                }
#endif
            }
            else if((value & 0x20) != 0)
            {
                GSV6K5_TXPHY_get_SCDC_TMDS_CFGED(port, &ConfigFlag);
                if(ConfigFlag)
                    TxFrlStat = AV_FRL_TRAIN_SUCCESS;
                else
                    TxFrlStat = AV_FRL_TRAIN_ONGOING;
            }
            else if(port->content.tx->H2p1FrlStat == AV_FRL_TRAIN_NO_TIMEOUT)
                TxFrlStat = AV_FRL_TRAIN_NO_TIMEOUT;
            else
            {
                AvHalI2cReadField8(GSV6K5_TXFRL_MAP_ADDR(port),0x0F,0xFF,0,&value);
                if(value == 0x01)
                    TxFrlStat = AV_FRL_TRAIN_IDLE;
                else
                    TxFrlStat = AV_FRL_TRAIN_ONGOING;
            }
        }
#if AvEnableTxCtsFrlTraining
        if(TxFrlStat == AV_FRL_TRAIN_NO_TIMEOUT)
        {
            ConfigFlag = Gsv6k5TxScdcAction(port, 0, 0xA8, 0x41, 0x00);
            AvHalI2cReadField8(GSV6K5_TXFRL_MAP_ADDR(port),0x1A,0xFF,0,&value);
            NewValue = ((value>>4)&0x0F) + ((value<<4) & 0xF0);
            if(ConfigFlag != NewValue)
            {
                value = ConfigFlag&0x0F;
                if(value <= 8)
                    GSV6K5_TXFRL_set_FRL_MAN_LN0_LTP_REQ(port, value);
                value = ConfigFlag>>4;
                if(value <= 8)
                    GSV6K5_TXFRL_set_FRL_MAN_LN1_LTP_REQ(port, value);
                AvUapiOutputDebugFsm("Port[%d][%d]:Lane0/1=LTP%02x", port->device->index, port->index, ConfigFlag);
            }
            ConfigFlag = Gsv6k5TxScdcAction(port, 0, 0xA8, 0x42, 0x00);
            AvHalI2cReadField8(GSV6K5_TXFRL_MAP_ADDR(port),0x1B,0xFF,0,&value);
            NewValue = ((value>>4)&0x0F) + ((value<<4) & 0xF0);
            if(ConfigFlag != NewValue)
            {
                value = ConfigFlag&0x0F;
                if(value <= 8)
                    GSV6K5_TXFRL_set_FRL_MAN_LN2_LTP_REQ(port, value);
                value = ConfigFlag>>4;
                if(value <= 8)
                    GSV6K5_TXFRL_set_FRL_MAN_LN3_LTP_REQ(port, value);
                AvUapiOutputDebugFsm("Port[%d][%d]:Lane2/3=LTP%02x", port->device->index, port->index, ConfigFlag);
            }
        }
#endif
        if(port->content.tx->H2p1FrlStat != TxFrlStat)
        {
            if(((TxFrlStat == AV_FRL_TRAIN_FAIL) || (TxFrlStat == AV_FRL_TRAIN_IDLE)) &&
               (port->content.tx->H2p1FrlStat == AV_FRL_TRAIN_ONGOING) &&
               (port->content.tx->InfoReady > TxVideoManageThreshold))
            {
                port->content.tx->Hpd = AV_HPD_FORCE_LOW;
                port->content.tx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
                port->content.tx->H2p1FrlExtraDelay = TxScdcManageThreshold;
#if AvEnableTxCtsFrlTraining
                value = (uint8)(port->content.tx->H2p1FrlType) - 1;
#else
                value = (uint8)(port->content.tx->H2p1FrlType);
#endif
                Gsv6k5TxScdcAction(port, 1, 0xA8, 0x31, value); /* no FRL reset */
                AvUapiOutputDebugFsm("Port[%d][%d]:FRL%d failure restart", port->device->index, port->index, port->content.tx->H2p1FrlType);
                /* Toggle For SDA Low Drive Fix, cause extra FRL training trigger */
                GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 1);
                Gsv6k5ToggleTxHpd(port, 0);
            }
            if(TxFrlStat == AV_FRL_TRAIN_FAIL)
                AvUapiOutputDebugFsm("Port[%d][%d]:FRL%d Fail", port->device->index, port->index, port->content.tx->H2p1FrlType);
            else if(TxFrlStat == AV_FRL_TRAIN_SUCCESS)
            {
#if AvEnableTxScdcCedCheck
                /* CED error reset action */
                GSV6K5_TXPHY_set_TX_CLR_UPDATE_FLAG(port, 0x03);
                AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0x07,0xFF,0,0x80);
#endif
                AvUapiOutputDebugFsm("Port[%d][%d]:FRL%d Success", port->device->index, port->index, port->content.tx->H2p1FrlType);
            }
            else if(TxFrlStat == AV_FRL_TRAIN_IDLE)
            {
                AvUapiOutputDebugFsm("Port[%d][%d]:FRL%d Idle", port->device->index, port->index, port->content.tx->H2p1FrlType);
            }
            else if(TxFrlStat == AV_FRL_TRAIN_NO_TIMEOUT)
            {
                GSV6K5_TXFRL_set_FRL_WT_FLT_UPDATE_TIMER(port, 0xffff);
                GSV6K5_TXFRL_set_FRL_WT_FRL_START_TIMER(port, 0xffff);
                GSV6K5_TXFRL_set_FRL_WT_FLT_READY_TIMER(port, 0xffff);
                Gsv6k5TxScdcAction(port, 1, 0xA8, 0x10, 0x08);
                AvUapiOutputDebugFsm("Port[%d][%d]:FRL in Test Mode", port->device->index, port->index);
            }
            else
                AvUapiOutputDebugFsm("Port[%d][%d]:FRL%d in Training", port->device->index, port->index, port->content.tx->H2p1FrlType);
            /* Update FRL status */
            port->content.tx->H2p1FrlStat = TxFrlStat;
            port->content.tx->H2p1FrlPoll = AV_FRL_POLL_IDLE;
            port->content.tx->H2p1FrlCedRb = 0;
        }
    }
#if AvEnableTxSingleLaneMode
    if((port->content.tx->InfoReady > TxVideoManageThreshold) && (port->content.tx->H2p1FrlManual != 0))
    {
        GSV6K5_TXFRL_get_RB_FRL_TRAINING_PASS(port, &value);
        if(value == 0)
        {
            GSV6K5_TXFRL_set_FRL_TRAIN_FSM_DISABLE(port, 1);
            GSV6K5_TXFRL_set_FRL_TRAIN_FSM_DISABLE(port, 0);
        }
    }
#endif

    /* 7. handle SCDC errors */
#if AvEnableTxScdcCedCheck
    if((port->content.tx->EdidSupportFeature & AV_BIT_FEAT_SCDC) != 0x0)
    {
        /*
        if(port->content.tx->InfoReady == TxScdcManageThreshold)
        {
            GSV6K5_TXPHY_set_TX_CLR_UPDATE_FLAG(port, 0x03);
            AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0x07,0xFF,0,0x80);
        }
        */
        /* 7.1 FRL mode SCDC processing */
        if((port->content.tx->H2p1FrlType != AV_FRL_NONE) && (port->content.tx->InfoReady > TxScdcManageThreshold))
        {
            if(port->content.tx->H2p1FrlStat == AV_FRL_TRAIN_SUCCESS)
            {
                /* 7.1.1 read update register */
                if(port->content.tx->H2p1FrlPoll == AV_FRL_POLL_INIT)
                {
                    GSV6K5_TXPHY_get_RX_SCDC_READ_REQUEST_DONE(port, &ConfigFlag);
                    if(ConfigFlag == 1)
                    {
                        port->content.tx->H2p1FrlPoll = AV_FRL_POLL_STAT;
                        GSV6K5_TXPHY_get_RX_SCDC_UPDATE_0(port, &NewValue);
                        /* 7.1.2 FLT update */
                        if((NewValue & 0x20) != 0x00)
                        {
                            Gsv6k5TxScdcAction(port, 1, 0xA8, 0x10, 0x63); /* clear interrupts */
#if AvEnableTxCtsFrlTraining
                            port->content.tx->H2p1FrlType = (AvFrlType)((uint8)port->content.tx->H2p1FrlType - 1);
                            GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 1);
                            GSV6K5_TXFRL_set_FRL_RATE(port, port->content.tx->H2p1FrlType);
#else
                            Gsv6k5TxFrlStart(port);
#endif
                            AvUapiOutputDebugMessage("Port[%d][%d]:Failed FRL Re-Training", port->device->index, port->index);
                        }
                        /* 7.1.3 CED update */
                        if((NewValue & 0x03) != 0x00)
                        {
                            GSV6K5_TXPHY_set_TX_CLR_UPDATE_FLAG(port, 0x00);
                            /* read back CED value */
                            AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0x07,0xFF,0,0x83);
                            port->content.tx->H2p1FrlPoll = AV_FRL_POLL_CEDR;
                            //AvUapiOutputDebugMessage("Port[%d][%d]:Readback CED %x", port->device->index, port->index, NewValue);
                        }
                    }
                }
                else if(port->content.tx->H2p1FrlPoll == AV_FRL_POLL_CEDR)
                {
                    GSV6K5_TXPHY_get_TX_CLEARED_UPDATE_FLAG(port, &ConfigFlag);
                    if(ConfigFlag == 1)
                    {
                        port->content.tx->H2p1FrlPoll = AV_FRL_POLL_CEDD;
                        AvHalI2cReadMultiField(GSV6K5_TXPHY_MAP_ADDR(port), 0x11, 8, Packet+0);
                        if((port->content.tx->H2p1FrlType == AV_FRL_3G3L) ||
                           (port->content.tx->H2p1FrlType == AV_FRL_6G3L))
                        {
                            Packet[8] = 0x00;
                            Packet[9] = 0x00;
                        }
                        else
                        {
                            AvHalI2cReadMultiField(GSV6K5_TXPHY_MAP_ADDR(port), 0x23, 2, Packet+8);
                        }
                        /* 7.1.4 CED Error triggers retraining */
                        if(((Packet[2] == 0xFF) && (Packet[3] == 0xFF)) ||
                           ((Packet[4] == 0xFF) && (Packet[5] == 0xFF)) ||
                           ((Packet[6] == 0xFF) && (Packet[7] == 0xFF)) ||
                           ((Packet[8] == 0xFF) && (Packet[9] == 0xFF)))
                        {
                            /* CED error clear action */
                            if(port->content.tx->H2p1FrlCedRb < 1)
                            {
                                GSV6K5_TXPHY_set_TX_CLR_UPDATE_FLAG(port, Packet[0]);
                                AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0x07,0xFF,0,0x80);
                                port->content.tx->H2p1FrlPoll = AV_FRL_POLL_CLEAR;
                                port->content.tx->H2p1FrlCedRb = port->content.tx->H2p1FrlCedRb + 1;
                                AvUapiOutputDebugMessage("Port[%d][%d]:CED Clears Error", port->device->index, port->index);
                            }
                            else
                            {
                                /* CED error restart action */
                                port->content.tx->Hpd = AV_HPD_FORCE_LOW;
                                port->content.tx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
                                AvUapiOutputDebugMessage("Port[%d][%d]:CED triggers FRL Re-Training", port->device->index, port->index);
                            }
                            AvUapiOutputDebugMessage("%x %x %x %x %x %x %x %x %x %x", Packet[0],Packet[1],Packet[2],
                                                     Packet[3],Packet[4],Packet[5],Packet[6],Packet[7],Packet[8],Packet[9]);
                        }
                        /* 7.1.5 CED Error triggers retraining */
                        else if((((port->content.tx->H2p1FrlType == AV_FRL_6G3L) || (port->content.tx->H2p1FrlType == AV_FRL_3G3L)) &&
                                 ((Packet[1] & 0x0E) != 0x0E)) ||
                                (((port->content.tx->H2p1FrlType == AV_FRL_6G4L) || (port->content.tx->H2p1FrlType == AV_FRL_8G4L) ||
                                  (port->content.tx->H2p1FrlType == AV_FRL_10G4L)) && ((Packet[1] & 0x1E) != 0x1E)))
                        {
                            /* CED error clear action */
                            //GSV6K5_TXPHY_set_TX_CLR_UPDATE_FLAG(port, Packet[0]);
                            //AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0x07,0xFF,0,0x80);
                            //port->content.tx->H2p1FrlPoll = AV_FRL_POLL_CLEAR;
                            //port->content.tx->H2p1FrlCedRb = port->content.tx->H2p1FrlCedRb + 1;
                            //AvUapiOutputDebugMessage("Port[%d][%d]:CED Clears Unlock", port->device->index, port->index);
                            /* CED error restart action */
                            port->content.tx->Hpd = AV_HPD_FORCE_LOW;
                            port->content.tx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
                            AvUapiOutputDebugMessage("Port[%d][%d]:UnLock triggers FRL Re-Training", port->device->index, port->index);
                            AvUapiOutputDebugMessage("%x %x %x %x %x %x %x %x %x %x", Packet[0],Packet[1],Packet[2],
                                                     Packet[3],Packet[4],Packet[5],Packet[6],Packet[7],Packet[8],Packet[9]);
                        }
                        /* 7.1.6 clear update flags */
                        else
                        {
                            GSV6K5_TXPHY_set_TX_CLR_UPDATE_FLAG(port, Packet[0]);
                            AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0x07,0xFF,0,0x80);
                            //AvUapiOutputDebugMessage("Port[%d][%d]:Clear Update %x", port->device->index, port->index, Packet[0]);
                            port->content.tx->H2p1FrlPoll = AV_FRL_POLL_CLEAR;
                        }
                    }
                }
                /* 7.1.6 read update action */
                else if((port->content.tx->H2p1FrlPoll == AV_FRL_POLL_IDLE) ||
                        (port->content.tx->H2p1FrlPoll == AV_FRL_POLL_CLEAR) ||
                        (port->content.tx->H2p1FrlPoll == AV_FRL_POLL_STAT))
                {
                    if(port->content.tx->H2p1FrlPoll == AV_FRL_POLL_CLEAR)
                    {
                        GSV6K5_TXPHY_set_CLR_TX_CLEARED_UPDATE_FLAG(port, 1);
                        //AvUapiOutputDebugMessage("Port[%d][%d]:Clear Update Stat", port->device->index, port->index);
                    }
                    else
                    {
                        GSV6K5_TXPHY_set_CLR_RX_SCDC_READ_REQUEST_DONE(port, 1);
                        //AvUapiOutputDebugMessage("Port[%d][%d]:Clear Read Request", port->device->index, port->index);
                    }
                    port->content.tx->H2p1FrlPoll = AV_FRL_POLL_READY;
                }
                else if(port->content.tx->H2p1FrlPoll == AV_FRL_POLL_READY)
                {
                    GSV6K5_TXPHY_set_TX_MAN_READ_UPDATE_FLAG(port, 1);
                    port->content.tx->H2p1FrlPoll = AV_FRL_POLL_INIT;
                    //AvUapiOutputDebugMessage("Port[%d][%d]:Read Update", port->device->index, port->index);
                }
            }
        }
        /* 7.2 TMDS mode SCDC processing */
        else if(port->content.tx->H2p1FrlManual == 0)
        {
            if(((port->content.tx->H2p1SupportFeature & AV_BIT_H2P1_FRL_SUPPORT) == 0) &&
               ((port->content.tx->EdidSupportFeature & AV_BIT_FEAT_SCDC) != 0) &&
               (port->content.tx->InfoReady > TxScdcManageThreshold) &&
               (port->content.tx->InfoReady <= TxScdcManageThreshold+10))
            {
                /* Protection Scrambler, Max 100Ms */
                ConfigFlag = Gsv6k5TxScdcAction(port, 0, 0xA8, 0x21, 0x00);
                if((((ConfigFlag & 0x01) != 0x01) && (port->content.video->info.TmdsFreq > 340)) ||
                   (((ConfigFlag & 0x01) != 0x00) && (port->content.video->info.TmdsFreq < 340)) ||
                   (ConfigFlag == 0xFF))
                {
                    Gsv6k5ToggleTxHpd(port, 0);
                    AvUapiOutputDebugMessage("Port[%d][%d]:SCDC check fail %d", port->device->index, port->index, ConfigFlag);
                }
            }
            if(port->content.tx->InfoReady == TxScdcManageThreshold)
            {
                /* SCDC status check enable */
                GSV6K5_TXPHY_set_TX_POLLING_UPDATE_FLAG_EN(port, 1);
            }
            else if(port->content.tx->InfoReady > TxScdcManageThreshold)
            {
                GSV6K5_TXPHY_get_RX_SCDC_UPDATE_0(port, &value);
                if((value & 0x7F) != 0x00)
                {
                    GSV6K5_TXPHY_set_TX_CLR_UPDATE_FLAG(port, value);
                    value = (value & 0x03) | 0x80; /* task action */
                    AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port),0x07,0xFF,0,value);
                    AvUapiOutputDebugFsm("Port[%d][%d]:Clear Interrupt = %x", port->device->index, port->index, value);
                }
            }
        }
    }
#endif //AvEnableTxScdcCedCheck

    /* 8. Update Pll Lock Status*/
    if((port->content.tx->Lock.PllLock == 0) && (LocalPllLock == 1))
    {
        /* 8.1 Tx Output SRST */
        GSV6K5_TXPHY_set_TMDS_SRST_MAN(port, 0);
        GSV6K5_TXPHY_set_TMDS_SRST_MAN(port, 1);
    }
    port->content.tx->Lock.PllLock = LocalPllLock;

    if(port->content.tx->Lock.PllLock)
    {
        /* Pixel Repetition for 480i/576i */
        value32 = port->content.video->info.TmdsFreq >> port->content.video->PixelRepeatValue;
        if((value32 < 21) && (value32 != 0))
        {
            port->content.video->ClockMultiplyFactor = 1;
            port->content.video->PixelRepeatValue = 1;
        }
        else
        {
            port->content.video->ClockMultiplyFactor = 0;
            port->content.video->PixelRepeatValue = 0;
        }
        if(port->core.HdmiCore == 0)
        {
            /* Protect DVI->HDMI Output RGB Color Space */
            if(port->content.tx->InfoReady == 1)
            {
                if((FromPort != NULL) &&
                   (FromPort->type == HdmiRx) &&
                   (FromPort->content.rx->IsFreeRun == 1) &&
                   (FromPort->content.rx->HdmiMode == 0) &&
                   (port->content.tx->HdmiMode == 1) &&
                   ((port->content.video->AvailableVideoPackets & AV_BIT_AV_INFO_FRAME) == 0))
                {
                    AvMemset(Packet, 0, PktSize[AV_PKT_AV_INFO_FRAME]);
                    Packet[1] = 0x02;
                    Packet[2] = 0x0D;
                    Gsv6k5_TxSendAVInfoFrame(port, Packet, 1);
                }
            }
            if(port->content.tx->InfoReady >= TxVideoManageThreshold)
            {
                /* Video Black Mute */
                GSV6K5_TXDIG_get_TX_BLACK_VIDEO_EN(port, &value);
                if(value != port->content.video->Mute.BlkMute)
                {
                    if((Gsv6k5TxCoreInUse(port) != AvOk) || (port->content.video->Mute.BlkMute == 0))
                    {
                        GSV6K5_TXDIG_set_TX_BLACK_VIDEO_EN(port, port->content.video->Mute.BlkMute);
                    }
                }
                /* Audio Mute */
                GSV6K5_TXDIG_get_TX_AUDIO_MUTE(port, &value);
                /* Unmute when Channel Status Available and Audio Mute is 0 */
                FromPort = (AvPort*)(port->content.RouteAudioFromPort);
                if(port->content.audio->AudioMute == 1)
                    NewValue = 1;
                else if((FromPort != NULL) && (FromPort->type == LogicAudioRx))
                    NewValue = 0;
                else if((port->content.audio->AudType == AV_AUD_TYPE_DSD) ||
                        (port->content.audio->AudType == AV_AUD_TYPE_DST) ||
                        (port->content.audio->AudType == AV_AUD_TYPE_HBR) ||
                        ((port->content.audio->AvailableAudioPackets & AV_BIT_AUDIO_CHANNEL_STATUS) != 0))
                    NewValue = 0;
                else
                    NewValue = 1;
                if(value != NewValue)
                {
                    if((Gsv6k5TxCoreInUse(port) != AvOk) || (NewValue == 0))
                    {
                        GSV6K5_TXDIG_set_TX_AUDIO_MUTE(port, NewValue);
                    }
                }
            }
        }
    }
    else
    {
        port->content.video->AvailableVideoPackets = 0;
        port->content.audio->AvailableAudioPackets = 0;
    }

    /* 9. Edid Status Update */
    GSV6K5_TXPHY_get_TX_EDID_READY_RB(port,    &value);
    if(value == 1)
    {
        GSV6K5_TXPHY_get_EDID_READ_CFGED(port, &value);
        if(value == 0)
        {
            GSV6K5_TXPHY_get_EDID_ANALYZE_DONE(port, &ConfigFlag);
            if(ConfigFlag == 0)
                GSV6K5_TXPHY_set_EDID_ANALYZE_DONE(port, 1);
            else
            {
                ConfigFlag = Gsv6k5TxDDCError(port);
                if(ConfigFlag == 1)
                    Gsv6k5ToggleTxHpd(port, 1);
            }
        }
    }
    if(port->content.tx->IgnoreEdidError != 0)
    {
        /* no need to read EDID if not required */
        port->content.tx->EdidReadSuccess = AV_EDID_UPDATED;
    }
    else if((value == 1) && (port->content.tx->EdidReadSuccess != AV_EDID_UPDATED))
    {
        /* EDID not updated yet, update it */
        if(port->content.tx->EdidReadSuccess == AV_EDID_RESET)
        {
            /* Manually set Edid Read Success, EDID will be read in Event */
            port->content.tx->EdidReadSuccess = AV_EDID_SUCCESS;
        }
#if (AvEdidStoredInRam == 1)
        /* Increment Edid Segment Ptr */
        GSV6K5_TXPHY_get_TX_DDC_READ_EDID_RAM_PART(port, &NewValue);
        if(NewValue == 0)
        {
            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0x00, 8, Packet);
            /* Check EDID Header */
            if((Packet[0] != 0x00) || (Packet[1] != 0xFF) || (Packet[2] != 0xFF) || (Packet[3] != 0xFF) ||
               (Packet[4] != 0xFF) || (Packet[5] != 0xFF) || (Packet[6] != 0xFF) || (Packet[7] != 0x00))
            {
                GSV6K5_TXPHY_set_TX_DDC_READ_EDID_RAM_PART(port, 0);
                GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_ERR(port, 1);
                GSV6K5_TXPHY_set_EDID_READ_CFGED(port, 1);
                port->content.tx->EdidReadSuccess = AV_EDID_RESET;
                AvUapiOutputDebugMessage("Port[%d][%d]:Edid Read Reset", port->device->index, port->index);
            }
            else
            {
                /* HF-EEODB */
                AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0x84, 3, Packet);
                if((Packet[0] == 0xE2) && (Packet[1] == 0x78))
                    NewValue = Packet[2];
                else
                    AvHalI2cReadField8(GSV6K5_TXEDID_MAP_ADDR(port), 0x7E, 0xFF, 0, &NewValue);
                if((NewValue > 1) && (NewValue < 0x80))
                {
                    switch(port->index)
                    {
                        case 4:
                            AvMemset(TxAEdidRam(port), 0, AvEdidMaxSize);
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxAEdidRam(port));
                            break;
                        case 5:
                            AvMemset(TxBEdidRam(port), 0, AvEdidMaxSize);
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxBEdidRam(port));
                            break;
                        case 6:
                            AvMemset(TxCEdidRam(port), 0, AvEdidMaxSize);
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxCEdidRam(port));
                            break;
                        case 7:
                            AvMemset(TxDEdidRam(port), 0, AvEdidMaxSize);
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxDEdidRam(port));
                            break;
                    }
                    AvUapiOutputDebugMessage("Port[%d][%d]:Read Segment 0-1", port->device->index, port->index);
                    GSV6K5_TXPHY_set_TX_DDC_READ_EDID_RAM_PART(port, 1);
                    GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_ERR(port, 1);
                    GSV6K5_TXPHY_set_EDID_READ_CFGED(port, 1);
                    port->content.tx->EdidReadSuccess = AV_EDID_RESET;
                }
            }
        }
        else
        {
            switch(port->index)
            {
                case 4:
                    if((TxAEdidRam(port)[0x84] == 0xE2) && (TxAEdidRam(port)[0x85] == 0x78))
                        value = TxAEdidRam(port)[0x86]/2;
                    else
                        value = TxAEdidRam(port)[0x7E]/2;
                    break;
                case 5:
                    if((TxBEdidRam(port)[0x84] == 0xE2) && (TxBEdidRam(port)[0x85] == 0x78))
                        value = TxBEdidRam(port)[0x86]/2;
                    else
                        value = TxBEdidRam(port)[0x7E]/2;
                    break;
                case 6:
                    if((TxCEdidRam(port)[0x84] == 0xE2) && (TxCEdidRam(port)[0x85] == 0x78))
                        value = TxCEdidRam(port)[0x86]/2;
                    else
                        value = TxCEdidRam(port)[0x7E]/2;
                    break;
                case 7:
                    if((TxDEdidRam(port)[0x84] == 0xE2) && (TxDEdidRam(port)[0x85] == 0x78))
                        value = TxDEdidRam(port)[0x86]/2;
                    else
                        value = TxDEdidRam(port)[0x7E]/2;
                    break;
            }
            if(NewValue <= value)
            {
                if(NewValue >= (AvEdidMaxSize/256))
                    value32 = (AvEdidMaxSize/256) - 1;
                else
                    value32 = NewValue;
                if(value32 != 0)
                {
                    switch(port->index)
                    {
                        case 4:
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxAEdidRam(port)+(256*value32));
                            break;
                        case 5:
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxBEdidRam(port)+(256*value32));
                            break;
                        case 6:
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxCEdidRam(port)+(256*value32));
                            break;
                        case 7:
                            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxDEdidRam(port)+(256*value32));
                            break;
                    }
                }
                NewValue = NewValue + 1;
                GSV6K5_TXPHY_set_TX_DDC_READ_EDID_RAM_PART(port, NewValue);
                if(NewValue <= value)
                {
                    AvUapiOutputDebugMessage("Port[%d][%d]: Read Segment %d", port->device->index, port->index, NewValue);
                    GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_ERR(port, 1);
                    GSV6K5_TXPHY_set_EDID_READ_CFGED(port, 1);
                    port->content.tx->EdidReadSuccess = AV_EDID_RESET;
                }
            }
        }
#endif
    }
    else if((port->content.tx->Hpd == AV_HPD_HIGH) &&
            (port->content.tx->EdidReadSuccess == AV_EDID_RESET))
    {
        GSV6K5_TXPHY_get_TX_HDCP1P4_STATE_RB(port, &value);
        if(value != 1)
        {
            /* DDC circuit error */
            value = Gsv6k5TxDDCError(port);
            if(value == 1)
            {
                AvUapiOutputDebugFsm("Port[%d][%d]:DDC Fail, Abort", port->device->index, port->index);
                port->content.tx->EdidReadFail    = 0;
                Gsv6k5ToggleTxHpd(port, 1);
#if AvForceDefaultEdid
                port->content.tx->EdidReadSuccess = AV_EDID_FAIL;
                port->content.tx->IgnoreEdidError = 1;
#endif
            }
        }
        /* if has already tried 15 times, that means failed */
        GSV6K5_TXPHY_get_TX_EDID_TRY_TIMES(port, &value);
        if(value == 0)
        {
            AvUapiOutputDebugFsm("Port[%d][%d]:Edid Fail, Retry", port->device->index, port->index);
            /* set flag */
            GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_ERR(port, 1);
            Gsv6k5ToggleTxHpd(port, 1);
            if(port->content.tx->EdidReadFail < 255)
                port->content.tx->EdidReadFail = port->content.tx->EdidReadFail + 1;
            if((port->content.tx->EdidReadFail > AvEdidErrorThreshold) || (port->content.tx->H2p1FrlManual != 0))
            {
                port->content.tx->EdidReadFail    = 0;
                port->content.tx->EdidReadSuccess = AV_EDID_FAIL;
#if AvForceDefaultEdid
                port->content.tx->IgnoreEdidError = 1;
#endif
            }
        }
    }

    /* 10. Manual BKSV Read Protection */
#if AvEnableHdcp1p4BksvCheck
    if((port->content.tx->EdidReadSuccess == AV_EDID_UPDATED) &&
        (port->content.hdcptx->HdcpSupport == 0))
        Gsv6k5ManualBksvRead(port);
#endif

    return ret;
}

/**
 * @brief  Tx video management
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxVideoManage(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    uint8 NewValue = 0;
    uchar *RegTable = (uchar *)CscYcc709ToRgb;
    AvPort *FromPort = (AvPort*)(port->content.RouteVideoFromPort);

    /* 0. No action for mux mode */
    if(port->core.HdmiCore != 0)
        return ret;
    /* 1. DVI mode check */
    if(port->content.tx->HdmiMode == 0)
    {
        /* 1.1 Input Color for Black Mute */
        if(port->content.video->Y == AV_Y2Y1Y0_RGB)
            NewValue = 0;
        else
            NewValue = 1;
        GSV6K5_TXDIG_get_TX_VIDEO_INPUT_CS(port, &value);
        if(value != NewValue)
            GSV6K5_TXDIG_set_TX_VIDEO_INPUT_CS(port, NewValue);
        /* 1.2 DVI color space is always RGB */
        GSV6K5_TXDIG_get_TX_VIDEO_OUTPUT_FORMAT(port, &value);
        if(value != 0)
            GSV6K5_TXDIG_set_TX_VIDEO_OUTPUT_FORMAT(port, 0);
        /* 1.3 color manage */
        /* 1.3.1 get CSC table */
        if((port->content.video->Y == AV_Y2Y1Y0_YCBCR_444) ||
           (port->content.video->Y == AV_Y2Y1Y0_YCBCR_422))
        {
            NewValue = 1;
            if((port->content.video->InCs == AV_CS_YUV_709) ||
               (port->content.video->InCs == AV_CS_YCC_709))
                RegTable = (uchar *)CscYcc709ToRgb;
            else
                RegTable = (uchar *)CscYcc601ToRgb;
        }
        else
        {
            NewValue = 0;
        }
        /* 1.3.2 check current color setting */
        /* 1.3.2.1 csc enable */
        GSV6K5_TXDIG_get_TX_VIDEO_CSC_ENABLE(port, &value);
        if(value != NewValue)
            GSV6K5_TXDIG_set_TX_VIDEO_CSC_ENABLE(port, NewValue);
        /* 1.3.2.2 csc coeffs */
        if(NewValue == 1)
        {
            AvHalI2cReadField8(GSV6K5_TXDIG_MAP_ADDR(port), 0x2D, 0xFF, 0, &value);
            if(value != RegTable[1])
            {
                AvHalI2cWriteMultiField(GSV6K5_TXDIG_MAP_ADDR(port), 0x2C, 24, RegTable);
                value = (RegTable[0]>>5)&0x03;
                GSV6K5_TXDIG_set_TX_VIDEO_CSC_SCALING_FACTOR(port, value);
            }
        }
    }
    if(((port->content.tx->H2p1FrlManual != 0) && (port->content.video->Y == AV_Y2Y1Y0_YCBCR_422)) ||
       ((port->content.tx->H2p1FrlType == AV_FRL_NONE) && (port->content.tx->H2p1FrlManual == 0) &&
#ifdef FixedIntervalSmoothFrl
        (port->content.tx->HdmiMode == 0)))
#else
        ((port->content.tx->HdmiMode == 0) || (port->content.video->info.TmdsFreq > 630))))
#endif
    {
        GSV6K5_TXDIG_get_TX_GC_CD(port, &value);
        if((value != 0) && (value != 4))
            GSV6K5_TXDIG_set_TX_GC_CD(port, 4); /* fixed to 8-bit */
    }
    /* 2. DSC Stream Protection on GCP for 4K144Hz 8-bit Samsung TV */
    if((FromPort != NULL) && (port->content.tx->H2p1FrlType != AV_FRL_NONE))
    {
        if((FromPort->type == HdmiRx) || (FromPort->type == DpRx))
        {
            GSV6K5_TXDIG_get_TX_PKT_GC_ENABLE(port, &value);
            if((FromPort->content.video->DscStream == 1) && (port->content.video->Cd == AV_CD_24))
                NewValue = 0;
            else if(port->content.video->AvailableVideoPackets & AV_BIT_GC_PACKET)
                NewValue = 1;
            else
                NewValue = 0;
            if(value != NewValue)
                GSV6K5_TXDIG_set_TX_PKT_GC_ENABLE(port, NewValue);
        }
    }

    return ret;
}

/**
 * @brief  Tx audio management
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxAudioManage(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    int16 blank = 0;
    uint32 flag = 0;
    uint8 margin = 0;
    AvPort *FromPort = NULL;

    /* No action for mux mode */
    FromPort = (AvPort*)(port->content.RouteAudioFromPort);
#if Gsv6k5AudioBypass
    if((FromPort != NULL) && (FromPort->type == HdmiRx))
        return ret;
#endif
    if(port->core.HdmiCore != 0)
        return ret;

    /* Protect Audio Type in case of mode switch,
       AV_AUD_TYPE_AUTO means incorrect configuration */
    if((FromPort != NULL) && (FromPort->type == HdmiRx) &&
       (FromPort->content.rx->IsFreeRun == 1) &&
       (FromPort->content.audio->AudType != port->content.audio->AudType))
    {
        port->content.audio->SampFreq = FromPort->content.audio->SampFreq;
        port->content.audio->AudType  = FromPort->content.audio->AudType;
        port->content.audio->NValue   = FromPort->content.audio->NValue;
        AvUapiTxSetAudioPackets(port);
    }
    /* No Audio when blank is too short */
    flag = 0;
#if AvEnableDetailTiming
    if((FromPort != NULL) && (FromPort->type == HdmiRx))
    {
        blank = port->content.video->timing.HTotal - port->content.video->timing.HActive;
        if((blank > 0) && (FromPort->content.video->DscStream == 0))
        {
            if(port->content.tx->H2p1FrlManual != 0)
            {
                if(blank < 60)
                    flag = 1;
            }
            else if(((port->content.tx->H2p1FrlType == AV_FRL_NONE) && (blank < 62)) ||
                    ((port->content.tx->H2p1FrlType != AV_FRL_NONE) && (blank < 68)))
                flag = 1;
        }
        /* DSC Stream HBlank Insertion */
        if((blank > 0) && (FromPort->content.video->DscStream == 1))
        {
            if(((blank >= 76) && (blank <= 85)) || ((blank >= 108) && (blank <= 117)) || (blank > 123))
                value = 0x02;
            else
                value = 0x00;
        }
        else
            value = 0x02;
        GSV6K5_TXDIG_get_H_BLANK_PKT_INSERT_MARGIN(port, &margin);
        if(margin != value)
            GSV6K5_TXDIG_set_H_BLANK_PKT_INSERT_MARGIN(port, value);
        Gsv6k5TxHBlankControl(port, 0);
    }
#endif
    /* No Audio Output when compressed audio is not supported */
    if((flag == 1) ||
       ((port->content.audio->AudCoding != AV_AUD_FORMAT_LINEAR_PCM) && (port->content.audio->AudCoding != 0) &&
        (port->content.tx->H2p1FrlManual == 0) &&
        ((port->content.tx->EdidSupportFeature & AV_BIT_FEAT_COMPRESS_AUDIO) == 0)))
    {
        GSV6K5_TXDIG_get_TX_PKT_AUDIO_SAMPLE_ENABLE(port, &value);
        if(value == 1)
            GSV6K5_TXDIG_set_TX_PKT_AUDIO_SAMPLE_ENABLE(port, 0);
    }
    else if((port->content.audio->AvailableAudioPackets & AV_BIT_AUDIO_SAMPLE_PACKET) != 0)
    {
        /* Audio Manage: audio might underflow or overflow, datapath reset fix */
        if(port->type == HdmiTx)
            GSV6K5_INT_get_TX1_AUDIOFIFO_FULL_RAW_ST(port, &value);
        if(value == 0)
        {
            GSV6K5_TXDIG_set_CLR_FIFO_MAXFILL(port, 1);
            GSV6K5_TXDIG_set_CLR_FIFO_MAXFILL(port, 0);
            GSV6K5_TXDIG_get_RI2C_FIFO_MAXFILL(port, &flag);
            if(flag > 0xF0)
                value = 1;
        }
        if(value == 1)
        {
            Gsv6k5ResetTxAudioDatapath(port);
            AvUapiOutputDebugMessage("Port[%d][%d]:Audio Recover From Fifo Full", port->device->index, port->index);
            if(port->type == HdmiTx)
                GSV6K5_INT_set_TX1_AUDIOFIFO_FULL_CLEAR(port, 1);
        }
    }

    return ret;
}

/**
 * @brief  Rx video management
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxVideoManage(pin AvPort *port, VideoInterrupt* Intpt))
{
#if AvEnableRxSingleLaneMode
    uint8 value = 0;
    uint8 prev = 0;
    if(port->content.rx->H2p1FrlManual != 0)
    {
        GSV6K5_RXFRL_get_FRL_RX_CG_CODE_DELTA_MAN_EN(port, &prev);
        if(port->content.video->info.TmdsFreq < 80)
            value = 1;
        if(prev != value)
            GSV6K5_RXFRL_set_FRL_RX_CG_CODE_DELTA_MAN_EN(port, value);
    }
#endif
    return AvOk;
}

/**
 * @brief  Rx audio management
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxAudioManage(pin AvPort *port, AudioInterrupt* Intpt))
{
    uint8 DsdMan=1, DsdSel=0, HbrMan=1, HbrSel=0;
    uint8 ApllRecalcFlag = 0;
    AvRet ret = AvOk;

    if(Intpt->AudChanMode)
    {
        ApllRecalcFlag = 1;
    }
    if(Intpt->InternMute)
    {
        Gsv6k5AvUapiRxSetAudioInternalMute(port);
        ret = AvOk;
    }
    if(Intpt->CsDataValid)
    {
        switch (port->content.audio->AudFormat)
        {
            case AV_AUD_I2S:
                GSV6K5_RXAUD_set_RX_DIS_I2S_ZERO_COMPR(port, 1);
                break;

            case AV_AUD_SPDIF:
                HbrSel = 1;
                break;

            case AV_AUD_DSD_NORM:
                DsdSel = 1;
                break;

            case AV_AUD_IF_AUTO:
                DsdMan = HbrMan = 0;
                break;

            default:
                break;
        };

        GSV6K5_RXAUD_set_RX_DSD_OUT_MAN_EN(port, DsdMan);
        GSV6K5_RXAUD_set_RX_DSD_OUT_MAN_SEL(port, DsdSel);
        GSV6K5_RXAUD_set_RX_HBR_OUT_MAN_EN(port, HbrMan);
        GSV6K5_RXAUD_set_RX_HBR_OUT_MAN_SEL(port, HbrSel);
        ret = AvOk;
    }
    if(Intpt->NChange)
    {
        ApllRecalcFlag = 1;
        ret = AvOk;
    }
    if(Intpt->CtsThresh)
    {
        //ApllRecalcFlag = 1;
        ret = AvOk;
    }
    if(Intpt->AudFifoOv)
    {
        ApllRecalcFlag = 1;
        ret = AvOk;
    }
    if(Intpt->AudFifoUn)
    {
        ApllRecalcFlag = 1;
        ret = AvOk;
    }
    if(Intpt->AudFifoNrOv)
        ret = AvOk;
    if(Intpt->AudioPktErr)
        ret = AvOk;
    if(Intpt->AudModeChg)
    {
        ApllRecalcFlag = 1;
        /* Switch between DSD/I2S, data rate varies triggers Audio Mute */
        GSV6K5_RXAUD_set_DISABLE_AUDIO_MUTE(port, 1);
        GSV6K5_RXAUD_set_DISABLE_AUDIO_MUTE(port, 0);
        ret = AvOk;
    }
    if(Intpt->AudFifoNrUn)
        ret = AvOk;
    /*
    if(Intpt->AudFlatLine)
        ret = AvOk;
    */
    if(Intpt->AudSampChg)
    {
        ret = AvOk;
    }
    if(Intpt->AudPrtyErr)
    {
        ret = AvOk;
    }
    if(Intpt->AudIfValid)
    {
        ret = AvOk;
    }
    if(Intpt->AcpValid)
    {
        ret = AvOk;
    }

    if(ApllRecalcFlag == 1)
    {
        GSV6K5_RXAUD_set_DISCARD_CSDATA_DIS(port, 0);
        GSV6K5_RXDIG_set_N_CTS_STABLE_DET_START(port, 1);
        GSV6K5_RXDIG_set_RX_APLL_MAN_START_CALC(port, 1);
        port->content.rx->Lock.AudioLock = 0;
    }

    return ret;
}

/**
 * @brief  connect rx and tx
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiConnectPort(pin AvPort *FromPort, pin AvPort *ToPort, AvConnectType type))
{
    AvRet ret = AvOk;
    uint8 value1 = 0;
    AvPort *TempPort = NULL;

    /* 1. Disconnet invalid port */
    if((ToPort->type == HdmiTx) && (type != AvConnectAudio))  /* Tx core with video connection */
    {
        if(ToPort->content.tx->InfoReady > TxVideoManageThreshold)
            ToPort->content.tx->Hpd = AV_HPD_FORCE_LOW;
        else
        {
            ToPort->content.tx->InfoReady = 0;
            ToPort->content.tx->H2p1FrlExtraDelay = 0;
        }
        if(ToPort->core.HdmiCore != -1)
            Gsv6k5AvUapiTxDisableCore(ToPort);
    }
    /* Step 2. Set port infrastructure content */
    if(ToPort->type == HdmiTx)
    {
    #if (Gsv6k5MuxMode == 1)
        ToPort->core.HdmiCore = -1;
    #else
        if(FromPort->type == HdmiRx)
            ToPort->core.HdmiCore = -1;
        else
            ToPort->core.HdmiCore = 0;
    #endif
        /* Force TxPort to use HdmiCore */
        //ToPort->core.HdmiCore = 0;
        /* 2.1 Disable InfoFrames in advance */
        if(type == AvConnectAudio)
        {
            ToPort->content.audio->AvailableAudioPackets = 0;
            Gsv6k5AvUapiTxEnableInfoFrames (ToPort, (AV_BIT_ACR_PACKET |
                                              AV_BIT_AUDIO_SAMPLE_PACKET), 0);
        }
        else
        {
            ToPort->content.tx->Lock.PllLock = 0;
            ToPort->content.video->AvailableVideoPackets = 0;
            ToPort->content.audio->AvailableAudioPackets = 0;
            Gsv6k5AvUapiTxEnableInfoFrames (ToPort, 0xffff, 0);
        }
    }

    /* Step 3. Switch Port Selection */
    if(type != AvConnectAudio)
    {
        if(FromPort->type == HdmiRx)
        {
            GSV6K5_PRIM_set_RX1_CORE_SEL(FromPort, FromPort->index);
        }
    }

    switch(ToPort->index)
    {
        case 4:
        case 5:
        case 6:
        case 7:
            /* To TxA */
            if(ToPort->core.HdmiCore == 0)
            {
                switch(ToPort->index)
                {
                    case 4:
                        GSV6K5_SEC_set_TXPORT_A_SRC_SEL(ToPort, 0);
                        break;
                    case 5:
                        GSV6K5_SEC_set_TXPORT_B_SRC_SEL(ToPort, 0);
                        break;
                    case 6:
                        GSV6K5_SEC_set_TXPORT_C_SRC_SEL(ToPort, 0);
                        break;
                    case 7:
                        GSV6K5_SEC_set_TXPORT_D_SRC_SEL(ToPort, 0);
                        break;
                }
                switch(FromPort->index)
                {
                    case 0:
                        /* From Rx1 Core */
                        if(type != AvConnectAudio)
                        {
                            GSV6K5_SEC_set_TX1_SRC_SEL(FromPort, 0);
                        }
                        break;
                    case 20:
                        /* From Scaler Port */
                        GSV6K5_SEC_set_TX1_SRC_SEL(FromPort, 2);
                        break;
                    case 24:
                        /* From Color Port */
                        GSV6K5_SEC_set_TX1_SRC_SEL(FromPort, 2);
                        break;
                    case 28:
                        /* From VG */
                        GSV6K5_SEC_set_TX1_SRC_SEL(FromPort, 3);
                        break;
                }
            }
            else
            {
                switch(ToPort->index)
                {
                    case 4:
                        GSV6K5_SEC_set_TXPORT_A_SRC_SEL(ToPort, 1);
                        break;
                    case 5:
                        GSV6K5_SEC_set_TXPORT_B_SRC_SEL(ToPort, 1);
                        break;
                    case 6:
                        GSV6K5_SEC_set_TXPORT_C_SRC_SEL(ToPort, 1);
                        break;
                    case 7:
                        GSV6K5_SEC_set_TXPORT_D_SRC_SEL(ToPort, 1);
                        break;
                }
#if (Gsv6k5MuxMode == 1)
                GSV6K5_PRIM_set_RX1_TMDS_BYPASS_EN(FromPort, 1);
                GSV6K5_PRIM_set_TX1_TMDS_BYPASS_EN(FromPort, 1);
#endif
            }
            /* Audio Processing */
            GSV6K5_PRIM_get_TX1_AUD_SRC_SEL(FromPort, &value1);
            switch(FromPort->index)
            {
                case 0:
                    if(type != AvConnectVideo)
                        GSV6K5_PRIM_set_TX1_AUD_SRC_SEL(FromPort, 1);
                    break;
                case 10:
                    GSV6K5_PRIM_set_AUD1_OEN(FromPort, 0xff);
#if (AvEnableI2sDualDirection == 0)
                    GSV6K5_PRIM_set_AP1_OUT_SRC_SEL(FromPort, 4);
#endif
                    GSV6K5_PRIM_set_TX1_AUD_SRC_SEL(FromPort, 5);
                    GSV6K5_AG_set_AUD_CONV_ENABLE(FromPort, 1);
                    /* Conversion source: 1=ARC, 2=AP, 3=Rx */
                    GSV6K5_AG_set_AUD_CONV_MODE_SEL(FromPort, 2);
                    /* AP1.SPDIF = 0x0, AP2.SPDIF = 0x8 */
                    GSV6K5_AG_set_AUD_CONV_AP_SEL(FromPort, 0x00);
                    break;
                case 11:
                    GSV6K5_PRIM_set_AUD2_OEN(FromPort, 0xff);
#if (AvEnableI2sDualDirection == 0)
                    GSV6K5_PRIM_set_AP2_OUT_SRC_SEL(FromPort, 4);
#endif
                    GSV6K5_PRIM_set_TX1_AUD_SRC_SEL(FromPort, 6);
                    GSV6K5_AG_set_AUD_CONV_ENABLE(FromPort, 1);
                    /* Conversion source: 1=ARC, 2=AP, 3=Rx */
                    GSV6K5_AG_set_AUD_CONV_MODE_SEL(FromPort, 2);
                    /* AP1.SPDIF = 0x0, AP2.SPDIF = 0x8 */
                    GSV6K5_AG_set_AUD_CONV_AP_SEL(FromPort, 0x08);
                    break;
                case 34:
                    GSV6K5_PRIM_set_TX1_AUD_SRC_SEL(FromPort, 0);
                    break;
                default:
                    if(type != AvConnectVideo)
                        GSV6K5_PRIM_set_TX1_AUD_SRC_SEL(FromPort, 1);
                    break;
            }
#if AvEnableI2sDualDirection
            /* I2S Dual Direction, [7:4] for Audio Input, [3:0] for Audio Output */
            if(FromPort->type == LogicAudioRx)
                GSV6K5_TXDIG_set_I2S0_AP_SEL(ToPort, 4);
            else
                GSV6K5_TXDIG_set_I2S0_AP_SEL(ToPort, 0);
#endif
            break;
        case 8:
        case 9:
            /* Mute Output */
            if(ToPort->index == 8)
                GSV6K5_PRIM_set_AUD1_OEN(FromPort, 0xff);
            else
                GSV6K5_PRIM_set_AUD2_OEN(FromPort, 0xff);
            switch(FromPort->index)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    value1 = 1;
                    break;
                case 40:
                    value1 = 2;
                case 4:
                    AvUapiTxArcEnable(FromPort, 1);
                    value1 = 3;
#if AvEnableSpdifToI2s
                    value1 = 0;
                    /* Conversion source: 1=ARC, 2=AP, 3=Rx */
                    GSV6K5_AG_set_AUD_CONV_MODE_SEL(FromPort, 1);
#endif
                    break;
                case 10:
#if AvEnableSpdifToI2s
                    value1 = 0;
                    /* Conversion source: 1=ARC, 2=AP, 3=Rx */
                    GSV6K5_AG_set_AUD_CONV_MODE_SEL(FromPort, 2);
                    /* AP1.SPDIF = 0x4, AP2.SPDIF = 0xC */
                    GSV6K5_AG_set_AUD_CONV_AP_SEL(FromPort, 0x04);
#else
                    value1 = 5;
#endif
                    break;
                case 11:
#if AvEnableSpdifToI2s
                    value1 = 0;
                    /* Conversion source: 1=ARC, 2=AP, 3=Rx */
                    GSV6K5_AG_set_AUD_CONV_MODE_SEL(FromPort, 2);
                    /* AP1.SPDIF = 0x4, AP2.SPDIF = 0xC */
                    GSV6K5_AG_set_AUD_CONV_AP_SEL(FromPort, 0x0C);
#else
                    value1 = 6;
#endif
                    break;
                case 34:
                    value1 = 0;
                    break;
                default:
                    value1 = 4;
                    break;
            }
            if(ToPort->index == 8)
                GSV6K5_PRIM_set_AP1_OUT_SRC_SEL(FromPort, value1);
            else
                GSV6K5_PRIM_set_AP2_OUT_SRC_SEL(FromPort, value1);
            break;
        case 10:
            GSV6K5_PRIM_set_AUD1_OEN(FromPort, 255);
            break;
        case 11:
            GSV6K5_PRIM_set_AUD2_OEN(FromPort, 255);
            break;
        case 20:
            GSV6K5_CP_set_CP_OUT_QUAD_EN(FromPort, 0);
        case 24:
            /* To Color/Scaler Port */
            switch(FromPort->index)
            {
                case 0:
                    /* From Rx Port */
                    GSV6K5_SEC_set_CP_SRC_SEL(FromPort, 0);
                    break;
                case 28:
                    /* From VG */
                    GSV6K5_SEC_set_CP_SRC_SEL(FromPort, 2);
                    break;
            }
            GSV6K5_VSP_set_CP_VID_IN(FromPort, 0);
            break;
    }

    /* Step 4. Set Core Occupied */
    if((FromPort->type == HdmiRx) || (FromPort->type == VideoGen))
    {
        TempPort = FromPort->device->port; /* Get 1st Rx Port of device */
        while((TempPort != NULL) && (TempPort->device->index == FromPort->device->index))
        {
            if(TempPort->device->index != FromPort->device->index)
                break;
            /* HDMI Rx occupies Rx1Core */
            else if((TempPort->type == HdmiRx) && (TempPort->core.HdmiCore == -1))
            {
                FindCore1(FromPort) = FromPort;
                TempPort->core.HdmiCore = 0;
            }
            /* VideoGen occupies Rx2Core*/
            else if((TempPort->type == VideoGen) && (TempPort->core.HdmiCore == -1))
            {
                TempPort->core.HdmiCore = 1;
            }
            /* Loop to the next RxPort */
            TempPort = (AvPort*)(TempPort->next);
        }
    }
    if((ToPort->index == 20) || (ToPort->index == 24))
    {
        if((FindCp1Mode(ToPort) != NULL) && (FindCp1Mode(ToPort) != ToPort))
            FindCp1Mode(ToPort)->content.RouteVideoFromPort = NULL;
        FindCp1Mode(ToPort) = ToPort;
    }

    /* Step 5. Set Single Lane mode */
#if AvEnableRxSingleLaneMode
    if(FromPort->core.HdmiCore == 0)
    {
        if(FromPort->content.rx->H2p1FrlManual == 0)
        {
            /* set 0 when AUTO EQ */
            GSV6K5_RXFRL_set_FRL_RX_TRAIN_DIS(FromPort, 0);
            GSV6K5_RXFRL_set_FRL_RX_FFE_REQ_MODE(FromPort, (uint8)FromPort->content.rx->H2p1FrlMode);
            GSV6K5_RXDIG_set_FRL_CTRL_MAN_EN(FromPort, 0);
            GSV6K5_RXDIG_set_FRL_MODE_MAN(FromPort, 0);
            GSV6K5_ANAPHY_set_RXA_CH_READ_CLK_SEL_MAN_EN(FromPort, 0);
            AvUapiOutputDebugMessage("Port[%d][%d]:Rx Auto FRL 0x%x", FromPort->device->index,FromPort->index,value1);
        }
        else
        {
            value1 = ((FromPort->content.rx->H2p1FrlManual)>>16)&0xf;
            AvUapiOutputDebugMessage("Port[%d][%d]:Rx Manual FRL 0x%x", FromPort->device->index,FromPort->index,value1);
            GSV6K5_RXFRL_set_FRL_RX_TRAIN_DIS(FromPort, 1);
            GSV6K5_RXFRL_set_FRL_RX_FFE_REQ_MODE(FromPort, 3);
            GSV6K5_RXDIG_set_FRL_LANE_NUM_MAN(FromPort, value1);
            GSV6K5_RXDIG_set_FRL_DATA_RATE_MAN(FromPort, ((FromPort->content.rx->H2p1FrlManual)&0xffff));
            GSV6K5_RXDIG_set_FRL_CTRL_MAN_EN(FromPort, 1);
            GSV6K5_RXDIG_set_FRL_MODE_MAN(FromPort, 1);
            if((value1 & 0x01) != 0)
                value1 = 1;
            else if((value1 & 0x02) != 0)
                value1 = 2;
            else if((value1 & 0x04) != 0)
                value1 = 3;
            else
                value1 = 0;
            GSV6K5_ANAPHY_set_ANA_RXA_CH_READ_CLK_SEL(FromPort, value1);
            GSV6K5_ANAPHY_set_RXA_CH_READ_CLK_SEL_MAN_EN(FromPort, 1);
        }
    }
#endif


#if AvEnableInternalVideoGen
    /* Step 6. VideoGen Process */
    if(FromPort->index == 28)
    {
        /* 6.1 Select VideoGen Clock Input */
        FromPort->core.HdmiCore = 1; /* use Rx2 */
        GSV6K5_AG_set_PLL_SRC_SEL_CH3(FromPort, 0);
    }
#endif

#if AvEnableInternalAudioGen
    /* Step 7. AudioGen Process */
    if((FromPort->type == AudioGen) && (ToPort->type == HdmiTx) && (type == AvConnectAudio))
    {
        GSV6K5_AG_set_PLL_SRC_SEL_CH2(FromPort, 0);
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(FromPort),0xC4,0xFF,0,0x40);
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(FromPort),0xC5,0xFF,0,0xB0);
        AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(FromPort),0x71,0xFF,0,0x00);
        AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(FromPort),0x00,0xFF,0,0x18);
        AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(FromPort),0x01,0xFF,0,0x01);
        AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(FromPort),0x03,0xFF,0,0xF8);
        AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(FromPort),0x05,0xFF,0,0x34);
        AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(FromPort),0x06,0xFF,0,0x82);
        AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(FromPort),0x07,0xFF,0,0x82);
    }
    else if((type == AvConnectAudio) || (type == AvConnectAV))
    {
        if(FromPort->type == HdmiRx)
        {
            GSV6K5_AG_set_PLL_SRC_SEL_CH2(FromPort, 3);
            AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(FromPort),0xC4,0xFF,0,0x00);
            AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(FromPort),0xC5,0xFF,0,0x00);
        }
    }
#endif

    return ret;
}

/**
 * @brief  disconnect rx and tx
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiDisconnectPort(pin AvPort *port))
{
    if(port->type == HdmiTx)
    {
        /* 1. Disconnect Tx Port */
        Gsv6k5AvUapiTxDisableCore(port);
        port->core.HdmiCore = -1;
    }
    else if(port->type == HdmiRx)
    {
        /* 1. Disconnect Rx Port */
        Gsv6k5DisableRxHpa(port);
        port->content.rx->Hpd = AV_HPD_LOW;
        port->content.rx->HpdDelayExpire = 0;
        /* 2. Clear Rx's Pointer */
        port->core.HdmiCore = -1;
    }
    return AvOk;
}

/**
 * @brief  Configure Tx Audio Packets
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetAudioPackets(pin AvPort *port))
{
    AvRet ret = AvInvalidParameter;
    AvPort *FromPort = (AvPort*)(port->content.RouteAudioFromPort);
    uint8 value = 0;

    if((port->content.tx->Hpd != AV_HPD_HIGH) ||
       (port->content.tx->Lock.PllLock == 0) ||
       (port->content.tx->Lock.AudioLock == 0))
    {
        port->content.audio->AvailableAudioPackets =
            port->content.audio->AvailableAudioPackets &
            (0xFFFF - AV_BIT_AUDIO_SAMPLE_PACKET - AV_BIT_AUDIO_INFO_FRAME - AV_BIT_ACR_PACKET);
        Gsv6k5AvUapiTxEnableInfoFrames(port, AV_BIT_AUDIO_SAMPLE_PACKET |
                                   AV_BIT_AUDIO_INFO_FRAME |
                                   AV_BIT_ACR_PACKET, 0);
        return ret;
    }

    if(port->type == HdmiTx)
    {
        /* Step 1. Select Mclk Source */
        /* value = 1 for external MCLK as default, value = 0 for internal MCLK as default */
        value = 0;
        if(port->content.audio->AudType == AV_AUD_TYPE_SPDIF)
        {
#if AvUseSpdifOutOnI2sData0
            value = 1;  /* internal MCLK no Audio Data in HDMI TX */
#else
            value = 0;
#endif
        }
        else if((FromPort != NULL) && (FromPort->type == LogicAudioRx))
            value = 0;
        else if(port->content.audio->AudType == AV_AUD_TYPE_HBR)
            value = 1;
        GSV6K5_TXDIG_set_TX_AUDIO_EXT_MCLK_ENABLE(port, value);
        /* Step 2. Select Correct Audio Input */
        if(port->content.audio->AudType == AV_AUD_TYPE_UNKNOWN)
            value = 0;
        else
            value = port->content.audio->AudType;
        GSV6K5_TXDIG_set_TX_AUDIO_MODE(port, value);
        /* Step 4. Enable Audio Interface */
        Gsv6k5_TxSetAudioInterface(port);
        /* Step 5. Enable Audio Sampling Frequency */
        Gsv6k5_TxSetAudChStatSampFreq(port);
        /* Step 6. Enable N Value */
        Gsv6k5AvUapiTxSetAudNValue(port);
        /* Finally. Enable the packet */
        Gsv6k5AvUapiTxEnableInfoFrames(port, AV_BIT_ACR_PACKET, 1);
#if Gsv6k5AudioBypass
        if((FromPort != NULL) && (FromPort->type == HdmiRx))
            return AvOk;
#endif
        Gsv6k5AvUapiTxEnableInfoFrames(port, AV_BIT_AUDIO_SAMPLE_PACKET, 1);

        ret = AvOk;
    }

    return ret;
}

/**
 * @brief  Clear Rx Flag indication
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxClearFlags(pio AvPort *port))
{
    AvRet ret = AvOk;
    if(port->core.HdmiCore != -1)
    {
        GSV6K5_RXAUD_set_RX_PKT_DET_RST(port, 1);
    }
    return ret;
}

/**
 * @brief  Read Rx Video Packet Status
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetVideoPacketStatus(pin AvPort *port, VideoInterrupt* Intpt))
{
    uint8 Value = 0;
    uint8 MapData[43];
    uint16 AvailablePacket = 0;

    AvMemset(Intpt, 0, sizeof(VideoInterrupt));

    /* 1. Read Map Data */
    if(port->core.HdmiCore == 0)
        Value = 0x30;
    else
        return AvNotAvailable;
    AvHalI2cReadMultiField(GSV6K5_INT_MAP_ADDR(port),Value,15,MapData+0);
    if(port->core.HdmiCore == 0)
        Value = 0x50;
    AvHalI2cReadMultiField(GSV6K5_INT_MAP_ADDR(port),Value,28,MapData+15);

    /* 2. Handle Video Interrupt */
    if(port->content.rx->H2p1FrlStat == AV_FRL_TRAIN_IDLE)
    {
        if(((MapData[6] & 0x10) != 0) ||
           (port->content.video->Mute.AvMute != ((MapData[6]>>5) & 0x01)))
            Intpt->AvMute = 1;
        if(((MapData[5] & 0x10) != 0) ||
           (port->content.rx->HdmiMode != ((MapData[5]>>5) & 0x01)))
            Intpt->HdmiModeChg = 1;
        if(((MapData[8] & 0x01) != 0) ||
           (port->content.rx->Lock.DeRegenLock != ((MapData[8]>>1) & 0x01)))
            Intpt->DeRegenLck = 1;
        if(((MapData[8] & 0x10) != 0) ||
           (port->content.rx->Lock.VSyncLock != ((MapData[8]>>5) & 0x01)))
            Intpt->VsyncLck = 1;
        if(((MapData[9] & 0x01) != 0) ||
           (port->content.video->info.Detect3D != ((MapData[9]>>1) & 0x01)))
            Intpt->Vid3dDet = 1;
        if((MapData[23] & 0x30) != 0)
            Intpt->NewTmds = 1;
        if((MapData[26] & 0x03) != 0)
            Intpt->BadTmdsClk = 1;
        if((MapData[26] & 0x30) != 0)
            Intpt->DeepClrChg = 1;
        if((MapData[20] & 0x03) != 0)
            Intpt->PktErr = 1;
    }

#if (Gsv6k5AviInfoBypass == 0)
    if(((MapData[0] & 0x01) != 0) || ((MapData[15] & 0x01) != 0))
        Intpt->AvIfValid = 1;
    else
    {
        if((port->content.video->AvailableVideoPackets & AV_BIT_AV_INFO_FRAME) != 0)
        {
            AvHalI2cReadField8(GSV6K5_RXINFO_MAP_ADDR(port),0x00,0xFF,0,&Value);
            if(Value != port->content.rx->Cks.AvCks)
                Intpt->AvIfValid = 1;
        }
    }
#endif
#if (Gsv6k5SpdInfoBypass == 0)
    if(((MapData[1] & 0x01) != 0) || ((MapData[16] & 0x01) != 0))
        Intpt->SpdValid = 1;
#endif
#if (Gsv6k5HdrInfoBypass == 0)
    if(((MapData[13] & 0x10) != 0) || ((MapData[41] & 0x10) != 0))
        Intpt->HdrValid = 1;
#endif
#if (Gsv6k5MpegInfoBypass == 0)
    if(((MapData[1] & 0x10) != 0) || ((MapData[16] & 0x10) != 0))
        Intpt->MsValid = 1;
#endif
#if (Gsv6k5VsInfoBypass == 0)
    if(((MapData[2] & 0x01) != 0) || ((MapData[17] & 0x01) != 0))
        Intpt->VsValid = 1;
#endif
#if (Gsv6k5IsrcPktBypass == 0)
    if(((MapData[3] & 0x01) != 0) || ((MapData[18] & 0x01) != 0))
        Intpt->Isrc1Valid = 1;
    if(((MapData[3] & 0x10) != 0) || ((MapData[18] & 0x10) != 0))
        Intpt->Isrc2Valid = 1;
#endif
#if (Gsv6k5GmdPktBypass == 0)
    if(((MapData[4] & 0x01) != 0) || ((MapData[19] & 0x01) != 0))
        Intpt->GamutValid = 1;
#endif
#if (Gsv6k5EmpPktBypass == 0)
    if(((MapData[14] & 0x10) != 0) || ((MapData[42] & 0x01) != 0))
        Intpt->EmpValid = 1;
#endif
    if((MapData[5] & 0x01) != 0)
        Intpt->GcValid = 1;
    else
    {
        Value = (MapData[5]>>1) & 0x01;
        AvailablePacket = port->content.video->AvailableVideoPackets & AV_BIT_GC_PACKET;
        if((AvailablePacket != 0) && (Value == 0))
        {
            Intpt->GcValid = 1;
        }
        else if((AvailablePacket == 0) && (Value != 0))
        {
            Intpt->GcValid = 1;
        }
    }

    /* timing read again */
    if((Intpt->HdmiModeChg) || (Intpt->NewTmds) || (Intpt->BadTmdsClk) || (Intpt->AvIfValid))
        AvUapiRxGetVideoTiming(port);

    if(Intpt->AvMute == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AvMute", port->device->index, port->index);
    if(Intpt->HdmiModeChg == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->HdmiModeChg", port->device->index, port->index);
    if(Intpt->DeRegenLck == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->DeRegenLck", port->device->index, port->index);
    if(Intpt->Vid3dDet == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->Vid3dDet", port->device->index, port->index);
    if(Intpt->NewTmds == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->NewTmds", port->device->index, port->index);
    if(Intpt->BadTmdsClk == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->BadTmdsClk", port->device->index, port->index);
    if(Intpt->DeepClrChg == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->DeepClrChg", port->device->index, port->index);
    if(Intpt->PktErr == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->PktErr", port->device->index, port->index);
    if(Intpt->AvIfValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AvIfValid", port->device->index, port->index);
    if(Intpt->SpdValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->SpdValid", port->device->index, port->index);
    if(Intpt->HdrValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->HdrValid", port->device->index, port->index);
    if(Intpt->EmpValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->EmpValid", port->device->index, port->index);
    if(Intpt->MsValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->MsValid", port->device->index, port->index);
    if(Intpt->VsValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->VsValid", port->device->index, port->index);
    if(Intpt->Isrc1Valid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->Isrc1Valid", port->device->index, port->index);
    if(Intpt->Isrc2Valid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->Isrc2Valid", port->device->index, port->index);
    if(Intpt->GamutValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->GamutValid", port->device->index, port->index);
    if(Intpt->GcValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->GcValid", port->device->index, port->index);

    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetPacketType(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    uint8 temp = 0;
    AvAudioType PrevAudType = port->content.audio->AudType;

    GSV6K5_RXAUD_get_AUDIO_SAMPLE_PKT_DET(port, &value);
    if(value == 1)
    {
        port->content.audio->AudFormat  = AV_AUD_I2S;
        port->content.audio->AudType    = AV_AUD_TYPE_ASP;
    }
    else
    {
        GSV6K5_RXAUD_get_DSD_AUDIO_PKT_DET(port, &value);
        if(value == 1)
        {
            port->content.audio->AudFormat  = AV_AUD_DSD_NORM;
            port->content.audio->AudType    = AV_AUD_TYPE_DSD;
        }
        else
        {
            GSV6K5_RXAUD_get_DST_AUDIO_PKT_DET(port, &value);
            if(value == 1)
            {
                port->content.audio->AudFormat  = AV_AUD_DSD_DST;
                port->content.audio->AudType    = AV_AUD_TYPE_DST;
            }
            else
            {
                GSV6K5_RXAUD_get_HBR_AUDIO_PKT_DET(port, &value);
                if(value == 1)
                {
                    port->content.audio->AudFormat  = AV_AUD_I2S;
                    port->content.audio->AudType    = AV_AUD_TYPE_HBR;
                }
                else
                {
                    port->content.audio->AudFormat = AV_AUD_IF_AUTO;
                    port->content.audio->AudType   = AV_AUD_TYPE_UNKNOWN;
                }
            }
        }
    }
    /* HBR Special Setting */
    GSV6K5_RXAUD_get_RX_SEL_I2S(port, &value);
    if(port->content.audio->AudType == AV_AUD_TYPE_HBR)
    {
        /* HBR will use SPDIF Out */
        temp = 0x00;
    }
    else
    {
#if AvUseSpdifOutOnI2sData0
        /* will use SPDIF Out On I2S Data0*/
        temp = 0x00;
#else
        /* LPCM will use I2S Out, this is default */
        temp = 0x01;
#endif
    }
    if(temp != value)
        GSV6K5_RXAUD_set_RX_SEL_I2S(port, temp);
    /* Audio Tuning Setting */
    GSV6K5_RXDIG_get_APLL_CG_CODE_DELTA_MAN_EN(port, &value);
#if AvEnableRxSingleLaneMode
    if(port->content.rx->H2p1FrlManual != 0)
        temp = 0x00;
    else
#endif
        temp = 0x01;
    if(value != temp)
    {
        GSV6K5_RXDIG_set_APLL_CG_CODE_DELTA_MAN_EN(port, temp);
        GSV6K5_RXDIG_set_APLL_CG_1MS_1ADDR_DELTA_MAN_EN(port, (1-temp));
    }
    /* Audio Type change Reset */
    if(PrevAudType != port->content.audio->AudType)
    {
        GSV6K5_RXAUD_set_DISCARD_CSDATA_DIS(port, 0);
        GSV6K5_RXDIG_set_N_CTS_STABLE_DET_START(port, 1);
        GSV6K5_RXDIG_set_RX_APLL_MAN_START_CALC(port, 1);
        AvUapiOutputDebugMessage("Port[%d][%d]:AudType Change", port->device->index, port->index);
    }

    return ret;
}

/**
 * @brief  Read Rx Audio Packets
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetAudioPacketStatus(pin AvPort *port, AudioInterrupt* Intpt))
{
    uint8 MapData[42];
    uint8 Value = 0;

    AvMemset(Intpt, 0, sizeof(AudioInterrupt));

    /* 1. Read Map Data */
    if(port->content.rx->Lock.AudioLock)
    {
        if(port->core.HdmiCore == 0)
            Value = 0x30;
        else
            return AvNotAvailable;
        AvHalI2cReadMultiField(GSV6K5_INT_MAP_ADDR(port),Value,15,MapData+0);
        if(port->core.HdmiCore == 0)
            Value = 0x50;
        AvHalI2cReadMultiField(GSV6K5_INT_MAP_ADDR(port),Value,27,MapData+15);
    }
    else
        return AvNotAvailable;

    /* 2. Handle Audio Interrupt */
    if((MapData[7] & 0x01) != 0)
        Intpt->InternMute = 1;
    if((MapData[6] & 0x01) != 0)
        Intpt->AudChanMode = 1;
    else
    {
        Value = (MapData[6]>>1) & 0x01;
        if(port->content.audio->AudioMute == 1)
            Intpt->AudChanMode = 0;
        else if((Value == 1) && (port->content.audio->Layout == 0))
        {
            Intpt->AudChanMode = 1;
        }
        else if((Value == 0) && (port->content.audio->Layout == 1))
        {
            Intpt->AudChanMode = 1;
        }
    }
    if(((MapData[7] & 0x10) != 0) || ((MapData[24] & 0x10) != 0))
       Intpt->CsDataValid = 1;
    if((MapData[20] & 0x30) != 0)
        Intpt->NChange = 1;
    if(((MapData[4] & 0x10) != 0) || ((MapData[21] & 0x01) != 0))
        Intpt->CtsThresh = 1;
    if((MapData[21] & 0x30) != 0)
        Intpt->AudFifoOv = 1;
    if((MapData[22] & 0x03) != 0)
        Intpt->AudFifoUn = 1;
    if((MapData[22] & 0x30) != 0)
        Intpt->AudFifoNrOv = 1;
    if((MapData[19] & 0x30) != 0)
        Intpt->AudioPktErr = 1;
    if((MapData[25] & 0x30) != 0)
        Intpt->AudModeChg = 1;
    /* Detect Packet Type */
    if(Intpt->AudModeChg == 1)
        Gsv6k5AvUapiRxGetPacketType(port);
    if((MapData[23] & 0x03) != 0)
        Intpt->AudFifoNrUn = 1;
    /*
    if((MapData[24] & 0x03) != 0)
        Intpt->AudFlatLine = 1;
    */
    if((MapData[23] & 0x30) != 0)
        Intpt->AudSampChg = 1;
    if((MapData[25] & 0x03) != 0)
        Intpt->AudPrtyErr = 1;
#if (Gsv6k5AudioInfoBypass == 0)
    if(((MapData[0] & 0x10) != 0) || ((MapData[15] & 0x10) != 0))
        Intpt->AudIfValid = 1;
#endif
#if (Gsv6k5AcpPktBypass == 0)
    if(((MapData[2] & 0x10) != 0) || ((MapData[17] & 0x10) != 0))
        Intpt->AcpValid = 1;
#endif
#if (Gsv6k5AmdPktBypass == 0)
    if(((MapData[14] & 0x01) != 0) || ((MapData[41] & 0x01) != 0))
        Intpt->AmdValid = 1;
#endif
    /* Audio Channel Status Protect: check channel status is correct or not */
    if((port->content.rx->IsInputStable == 0) || (port->content.audio->AudioMute == 1))
        Intpt->CsDataValid = 0;

    if(Intpt->AudChanMode == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudChanMode", port->device->index, port->index);
    if(Intpt->InternMute == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->InternMute", port->device->index, port->index);
    if(Intpt->CsDataValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->CsDataValid", port->device->index, port->index);
    if(Intpt->NChange == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->NChange", port->device->index, port->index);
    if(Intpt->CtsThresh == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->CtsThresh", port->device->index, port->index);
    if(Intpt->AudFifoOv == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudFifoOv", port->device->index, port->index);
    if(Intpt->AudFifoUn == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudFifoUn", port->device->index, port->index);
    /* Don't care about Near OverFlow or Near UnderFlow */
    /*
    if(Intpt->AudFifoNrOv == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudFifoNrOv", port->device->index, port->index);
    if(Intpt->AudFifoNrUn == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudFifoNrUn", port->device->index, port->index);
    */
    if(Intpt->AudioPktErr == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudioPktErr", port->device->index, port->index);
    if(Intpt->AudModeChg == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudModeChg", port->device->index, port->index);
    /*
    if(Intpt->AudFlatLine == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudFlatLine", port->device->index, port->index);
    */
    if(Intpt->AudSampChg == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudSampChg", port->device->index, port->index);
    if(Intpt->AudPrtyErr == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudPrtyErr", port->device->index, port->index);
    if(Intpt->AudIfValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AudIfValid", port->device->index, port->index);
    if(Intpt->AcpValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AcpValid", port->device->index, port->index);
    if(Intpt->AmdValid == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Intpt->AmdValid", port->device->index, port->index);

    return AvOk;
}

/**
 * @brief  Check HDCP Encryption
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetHdcpStatus(pin AvPort *port, HdcpInterrupt* Intpt))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    uint8 value1 = 0;
    uint8 Rx2p2Stat = 0;
    uint32 StreamType = 0;
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint8 PreHdcpVersion = 0;
    uint8 PreHdcpManual  = 0;
    uint8 HdcpVersion = 0;
    uint8 HdcpManual  = 0;
    uint8 NewHdcpVersion = 0;
    uint8 NewHdcpManual  = 0;

    /* Pll Lock Status */
    if((port->content.rx->Lock.PllLock == 0) || (port->core.HdmiCore == -1) ||
       (port->content.rx->Hpd == AV_HPD_LOW))
    {
        Intpt->Encrypted = 0;
        port->content.rx->VideoEncrypted = 0;
        port->content.hdcp->HdcpError = 0;
        port->content.hdcp->Hdcp2p2RxRunning = 0;
        return AvOk;
    }

    AvMemset(Intpt, 0, sizeof(HdcpInterrupt));

#ifdef GsvInternalHdcpRxScdcFlag
    GSV6K5_RXSCDC_get_REG_DF(port, &value);
    value = value & 0x01;
    if((port->content.rx->VideoEncrypted == 0) && (value == 1))
        AvUapiOutputDebugMessage("Port[%d][%d]:Rx Scdc is encrypted with HDCP", port->device->index, port->index);
    port->content.rx->VideoEncrypted = value;
#else
    /* hdcp */
    GSV6K5_INT_get_RX1_AKSV_UPDATE_INT_ST(port,       &Intpt->AksvUpdate);
    GSV6K5_INT_get_RX1_HDCP1P4_ENCRYPTED_INT_ST(port, &Intpt->Encrypted);
    GSV6K5_INT_get_RX1_AKSV_UPDATE_RAW_ST(port,       &port->content.hdcp->AksvInterrupt);
    GSV6K5_INT_get_RX1_HDCP1P4_ENCRYPTED_RAW_ST(port, &port->content.rx->VideoEncrypted);
    GSV6K5_INT_get_RX1_HDCP2P2_RX_AUTH_RAW_ST(port,   &Rx2p2Stat);
#endif
    /* FRL HDCP protection */
    if(port->content.rx->VideoEncrypted == 1)
    {
        if(port->content.rx->H2p1FrlType == AV_FRL_NONE)
        {
            if(port->content.rx->Lock.EqLock == 0)
                port->content.rx->VideoEncrypted = 0;
        }
        else if(Rx2p2Stat == 0)
        {
            port->content.rx->VideoEncrypted = 0;
        }
    }
    /*
    if((port->content.rx->H2p1FrlType != AV_FRL_NONE) && (Rx2p2Stat == 1))
        port->content.rx->VideoEncrypted = 1;
    */

    if(Intpt->Encrypted)
    {
        if(Rx2p2Stat == 0)
        {
            if(port->content.rx->VideoEncrypted == 1)
                AvUapiOutputDebugMessage("Port[%d][%d]:Rx is encrypted with HDCP", port->device->index, port->index);
        }
        else if(Rx2p2Stat == 1)
        {
            if(port->content.rx->VideoEncrypted == 1)
                AvUapiOutputDebugMessage("Port[%d][%d]:Rx is encrypted with HDCP 2.2", port->device->index, port->index);
        }
    }

    /* Check HDCP State */
    GSV6K5_PRIM_get_RX_HDCP_CFG_MAN_EN(port, &PreHdcpManual);
    GSV6K5_PRIM_get_RX_CFG_TO_HDCP2P2(port,  &PreHdcpVersion);
    HdcpManual     = PreHdcpManual  & 0x01;
    HdcpVersion    = PreHdcpVersion & 0x01;
    PreHdcpManual  = PreHdcpManual  & 0x02;
    PreHdcpVersion = PreHdcpVersion & 0x02;
    NewHdcpManual  = HdcpManual;
    NewHdcpVersion = HdcpVersion;
    switch(port->content.hdcp->HdcpNeeded)
    {
        case AV_HDCP_RX_AUTO:
            /* Auto Hdcp Mode */
            NewHdcpManual = 0;
            break;
        case AV_HDCP_RX_FOLLOW_SINK:
            /* default to Auto Rx Mode */
            NewHdcpManual  = 0;
            NewHdcpVersion = 0;
            while(KfunFindVideoNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
            {
                if(CurrentPort->type == HdmiTx)
                {
                    if(CurrentPort->content.tx->HdmiMode == 0)
                    {
                        /* only support 1.4 when Dvi sink connected */
                        if(CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED)
                        {
                            NewHdcpManual  = 1;
                            NewHdcpVersion = 0;
                        }
                    }
                    else if(CurrentPort->content.hdcptx->HdcpSupport == 1)
                    {
                        /* Found HDCP 1.4 only device, set Rx HDCP to 1.4 */
                        GSV6K5_TXPHY_get_TX_HDCP2P2_CAPABILITY_RB(CurrentPort, &value);
                        GSV6K5_TXPHY_get_TX_HDCP2P2_CHECKED(CurrentPort, &value1);
                        if((CurrentPort->content.hdcptx->Hdcp2p2SinkSupport == 0) &&
                           ((value ==0) || (value1 == 0)))
                        {
                            NewHdcpManual  = 1;
                            NewHdcpVersion = 0;
                        }
                    }
                }
                PrevPort = CurrentPort;
                CurrentPort = NULL;
            }
            break;
        case AV_HDCP_RX_1P4_ONLY:
            /* Hdcp 1.4 only Mode */
            NewHdcpManual  = 1;
            NewHdcpVersion = 0;
            break;
        case AV_HDCP_RX_2P2_ONLY:
            /* Hdcp 2.2 only Mode */
            NewHdcpManual  = 1;
            NewHdcpVersion = 1;
            break;
        default:
            break;
    }
    if((NewHdcpManual != HdcpManual) || (NewHdcpVersion != HdcpVersion))
    {
        port->content.rx->Hpd = AV_HPD_TOGGLE;
        HdcpManual  = NewHdcpManual  | PreHdcpManual;
        HdcpVersion = NewHdcpVersion | PreHdcpVersion;
        GSV6K5_PRIM_set_RX_HDCP_CFG_MAN_EN(port, HdcpManual);
        GSV6K5_PRIM_set_RX_CFG_TO_HDCP2P2(port,  HdcpVersion);
    }

    /* Update VideoEncrypted Status */
    if(Intpt->Encrypted)
    {
        /* When Found Rx has started being encrypted ,
           start to check Encryption is not stopped by AvMute */
        GSV6K5_INT_set_RX1_HDCP1P4_ENCRYPTED_CLEAR(port,1);
    }

#if AvEnableHdcp2p2Feature
    if(Rx2p2Stat == 0x01)
        value = 1;
    else
        value = 0;
    /* IO.0x46[5],0x27[5] = 1, HDCP 2.2 Rx Version */
    /* IO.0x46[4],0x27[4] = 1, HDCP 2.2 Rx is encrypted */
    if(value != port->content.hdcp->Hdcp2p2RxRunning)
    {
        port->content.hdcp->Hdcp2p2RxRunning = value;
        if(value == 1)
        {
            AvUapiOutputDebugMessage("Port[%d][%d]:Rx is Running HDCP 2.2", port->device->index, port->index);
            GSV6K5_RX2P2H_get_RX_STREAMID_TYPE(port, &StreamType);
            if(StreamType != 0x00)
                port->content.hdcp->Hdcp2p2Flag = 1;
            else
                port->content.hdcp->Hdcp2p2Flag = 0;
        }
        else
            AvUapiOutputDebugMessage("Port[%d][%d]:Rx is Stopping HDCP 2.2", port->device->index, port->index);
    }

    /* HDCP 2.2 ECC Check Judge */
    GSV6K5_RX2P2H_get_RX_DISABLE_ECC_CHECK(port, &value);
    value1 = 1 - port->content.rx->IsInputStable;
    if(value1 != value)
        GSV6K5_RX2P2H_set_RX_DISABLE_ECC_CHECK(port, value1);

#endif

    return ret;
}

/**
 * @brief  Check Tx HDCP Encryption Capability
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxGetHdcpStatus(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint8 Desire1p4 = 0;
    uint8 Desire2p2 = 0;
    uint8 Error1 = 0;
    uint8 Error2 = 0;
    uint8 Error3 = 0;
    uint8 Error4 = 0;
    uint8 value = 0;
    uint8 value1 = 0;
    uint8 value2 = 0;
    uint8 Fsm1p4State = 0;
    uint8 Fsm2p2State = 0;

    /* Set HDCP Version with 2 Steps */
    /* Step 1. Disable Data Encryption */
    if(port->content.hdcptx->HdcpModeUpdate == 1)
    {
        GSV6K5_TXPHY_set_TX_HDCP1P4_ENC_EN(port, 0);
        GSV6K5_TX2P2_set_TX_HDCP2P2_ENCRYPTION_ENABLE(port, 0);
        port->content.hdcptx->HdcpModeUpdate = port->content.hdcptx->HdcpModeUpdate + 1;
    }
    /* Step 2. Reset HDCP Tx Version */
    else if(port->content.hdcptx->HdcpModeUpdate == 2)
    {
        Gsv6k5SetTxHdcpVersion(port, port->content.hdcptx->HdmiStyle, 1);
        port->content.hdcptx->HdcpModeUpdate = 0;
    }
    /* 0. HDCP Support */
    if((port->content.hdcptx->HdcpSupport == 0) &&
       (port->content.hdcptx->Hdcp2p2SinkSupport == 0))
        return AvOk;
    GSV6K5_TXPHY_get_TX_HDCP1P4_DESIRED(port, &Desire1p4);
    GSV6K5_TX2P2_get_TX_HDCP2P2_DESIRED(port, &Desire2p2);
    if((Desire1p4 != 0) || (Desire2p2 != 0))
        port->content.hdcptx->HdcpEnabled = 1;
    else
        port->content.hdcptx->HdcpEnabled = 0;
    /* 1. Check HDCP 1.4 Errors */
    GSV6K5_TXPHY_get_TX_HDCP1P4_STATE_RB(port,   &Fsm1p4State);
    GSV6K5_TXPHY_get_TX_HDCP1P4_ERROR_FLAG(port, &Error1);
    GSV6K5_TXPHY_get_TX_HDCP1P4_BKSV_READ_FLAG(port, &port->content.hdcptx->BksvReady);
    /* 1.1 HDCP 1.4 Running check */
    if((Fsm1p4State == 4) && (port->content.hdcptx->Authenticated == 0))
        AvUapiOutputDebugMessage("Port[%d][%d]:Tx is Running HDCP 1.4", port->device->index, port->index);
    /* 1.2 HDCP 1.4 FRL protection */
    if(((Fsm1p4State == 3) || (Fsm1p4State == 4) || (Fsm1p4State == 5)) &&
       (port->content.tx->H2p1FrlType != AV_FRL_NONE))
    {
        Gsv6k5ToggleTxHpd(port, 0);
    }
    /* 2. Check HDCP 2.2 Errors */
#if AvEnableHdcp2p2Feature
    AvHalI2cReadField8(GSV6K5_TX2P2_MAP_ADDR(port), 0x0D, 0xFF, 0, &Error2);
    AvHalI2cReadField8(GSV6K5_TX2P2_MAP_ADDR(port), 0x0E, 0xFF, 0, &Error3);
    AvHalI2cReadField8(GSV6K5_TX2P2_MAP_ADDR(port), 0x14, 0xFF, 0, &Fsm2p2State);
    if(Fsm2p2State == 0x00)
    {
        AvHalI2cReadField8(GSV6K5_TX2P2_MAP_ADDR(port), 0x13, 0xFF, 0, &value);
        if(value == 0x20)
        {
            Error4 = 1;
            AvUapiOutputDebugMessage("Port[%d][%d]:Unexpected DDC Error", port->device->index, port->index);
        }
    }
    /* 2.1 HDCP 2.2 no response check */
    if((Fsm1p4State == 6) && /* HDCP 2.2 Selected */
       (Desire2p2 == 1) &&    /* HDCP 2.2 Engine Enabled */
       (Fsm2p2State == 0x02)) /* no HDCP 2.2 FSM response */
        Error4 = 1;
    else if((Error2 != 0) || (Error3 != 0))
        Error4 = 1;
    /* 2.2 HDCP 2.2 Running check */
    if(((Fsm2p2State == 0x80) && (port->content.hdcptx->Hdcp2p2TxRunning == 0)) ||
       ((Fsm2p2State != 0x80) && (port->content.hdcptx->Hdcp2p2TxRunning == 1)))
    {
        port->content.hdcptx->Hdcp2p2TxRunning = (Fsm2p2State>>7) & 0x01;
        if(port->content.hdcptx->Hdcp2p2TxRunning == 1)
            AvUapiOutputDebugMessage("Port[%d][%d]:Tx is Running HDCP 2.2", port->device->index, port->index);
        else
            AvUapiOutputDebugMessage("Port[%d][%d]:Tx is Stopping HDCP 2.2", port->device->index, port->index);
#if AvEnableConsistentFrl
        if((port->content.tx->H2p1FrlType != AV_FRL_NONE) && (port->content.hdcptx->Hdcp2p2TxRunning == 0))
            port->content.tx->Hpd = AV_HPD_FORCE_LOW;
#endif
    }
    /* 3. Check Capability */
    if(Fsm1p4State == 6)
        port->content.hdcptx->Hdcp2p2SinkSupport = 1;
#endif
    /* 4. Check Authentication */
    if(((Fsm1p4State == 6) && (Fsm2p2State == 0x80)) || (Fsm1p4State == 4))
        port->content.hdcptx->Authenticated = 1;
    else
        port->content.hdcptx->Authenticated = 0;
    /* 4.1 V' 1B-04a CTS Protection on fast HDCP disable */
    GSV6K5_TXPHY_get_TX_HDCP1P4_BKSV_COUNT(port, &value1);
    if((value1 != 0) && ((Fsm1p4State == 3) || (Fsm1p4State == 5)))
    {
        for(value1=0;value1<200;value1++)
        {
            AvHalI2cReadField8(GSV6K5_TXPHY_MAP_ADDR(port), 0xC8, 0xFF, 0, &value2);
            if(((value2 & 0xF0) != 0) || ((value2 & 0x0F) != 5))
            {
                AvUapiOutputDebugMessage("HDCP.0xC8=%x in %d loop",value2,value1);
                value2 = value2>>4;
                if(value2 == 7)
                {
                    GSV6K5_TXPHY_set_TX_HDCP1P4_DESIRED(port, 0);
                    for(value1=0x22;value1<=0x33;value1++)
                        Gsv6k5TxScdcAction(port, 0, 0x74, value1, 0x00);
                    GSV6K5_TXPHY_set_TX_HDCP1P4_ENC_EN(port, 0);
                    Gsv6k5TxScdcAction(port, 0, 0x74, 0x41, 0x00);
                    Gsv6k5TxScdcAction(port, 0, 0x74, 0x42, 0x00);
                    GSV6K5_TXPHY_set_TX_HDCP1P4_DESIRED(port, 1);
                }
                break;
            }
        }
    }
    /* 5. Error Found */
    if((Error1 != 0) || (Error4 != 0))
    {
        /* 5.1 Add Error Total */
        port->content.hdcptx->HdcpError = port->content.hdcptx->HdcpError + 1;
        /* 5.2 Clear Errors */
        /* 5.2.1 seq_num_V rollover and incorrect M' delay */
        if((Error3&0x0C) != 0x00)
        {
            GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_ERROR(port, 1);
            GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_ERROR(port, 0);
            AvUapiOutputDebugMessage("Port[%d][%d]:Incorrect M' delay", port->device->index, port->index);
        }
        else if((Error3&0x81) != 0x00)
        {
            AvUapiOutputDebugMessage("Port[%d][%d]:Get seq_num_V rollover with error %d", port->device->index,
                                     port->index, port->content.hdcptx->HdcpError);
            AvUapiTxClearHdcpError(port);
        }
        /* default hdcp error clear */
        else
        {
            AvUapiTxClearHdcpError(port);
        }
        /* 5.3 When not locked, disable HDCP */
        if(((port->content.tx->Hpd == 0) && (port->content.hdcptx->HdcpEnabled == 1)) ||
           (port->content.tx->Lock.PllLock == 0))
            AvUapiTxDecryptSink(port);
    }
    else
    {
        if(port->content.video->Mute.BlkMute == 1)
        {
            if((Fsm1p4State == 0x06) && ((Fsm2p2State & 0x81) == 0x00))
                Error4 = 1;
            if((Fsm1p4State == 0x03) || (Fsm1p4State == 0x05))
                Error1 = 1;
        }
    }
    /* 6. Disable HDCP encryption or Mute */
    switch(port->content.hdcptx->HdmiStyle)
    {
        case AV_HDCP_TX_AUTO:
            port->content.video->Mute.BlkMute = Error1 | Error4;
            port->content.audio->AudioMute    = Error1 | Error4;
            if((port->content.hdcptx->HdcpError >= AvHdcpTxErrorThreshold) ||
               (port->content.hdcptx->HdcpSupport == 0))
                Gsv6k5ToggleTmdsOut(port, 0);
            break;
        case AV_HDCP_TX_2P2_ONLY:
            port->content.video->Mute.BlkMute = Error4;
            port->content.audio->AudioMute    = Error4;
            if((port->content.hdcptx->HdcpError >= AvHdcpTxErrorThreshold) ||
               (port->content.hdcptx->Hdcp2p2SinkSupport == 0))
                Gsv6k5ToggleTmdsOut(port, 0);
            break;
        case AV_HDCP_TX_1P4_ONLY:
            port->content.video->Mute.BlkMute = Error1;
            port->content.audio->AudioMute    = Error1;
            if((port->content.hdcptx->HdcpError >= AvHdcpTxErrorThreshold) ||
               (port->content.hdcptx->HdcpSupport == 0))
                Gsv6k5ToggleTmdsOut(port, 0);
            break;
        case AV_HDCP_TX_AUTO_FAIL_OUT:
            if((port->content.hdcptx->HdcpError == 2) &&
               ((Error1 != 0) || (Error4 != 0)))
            {
                Gsv6k5SetTxHdcpVersion(port, AV_HDCP_TX_1P4_FAIL_OUT, 1);
                AvUapiOutputDebugMessage("Port[%d][%d]:Tx HDCP 2.2 DownGrade to 1.4", port->device->index, port->index);
            }
            else if((port->content.hdcptx->HdcpError >= AvHdcpTxErrorThreshold) &&
                    ((Desire1p4 == 1) || (Desire2p2 == 1)))
            {
                AvUapiTxDecryptSink(port);
                port->content.hdcptx->HdcpError = AvHdcpTxErrorThreshold;
            }
            break;
        case AV_HDCP_TX_1P4_FAIL_OUT:
        case AV_HDCP_TX_2P2_FAIL_OUT:
            if((port->content.hdcptx->HdcpError >= AvHdcpTxErrorThreshold) &&
               ((Desire1p4 == 1) || (Desire2p2 == 1)))
            {
                AvUapiTxDecryptSink(port);
                port->content.hdcptx->HdcpError = AvHdcpTxErrorThreshold;
            }
            break;
        case AV_HDCP_TX_ILLEGAL_NO_HDCP:
            if((Desire1p4 == 1) || (Desire2p2 == 1))
                AvUapiTxDecryptSink(port);
            break;
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxGetBksvReady(pin AvPort *port))
{
    AvRet ret = AvNotAvailable;
    uint8 value;
    GSV6K5_TXPHY_get_TX_HDCP1P4_BKSV_READ_FLAG(port, &port->content.hdcptx->BksvReady);
    /* Running_rb only set 1 after BKSV flag is cleared which makes it an indicator */
    GSV6K5_TXPHY_get_TX_HDCP1P4_RUNNING_RB(port, &value);
    if(port->content.hdcptx->BksvReady == 1)
        ret = AvOk;
    else if(value == 1)
    {
        /* Bksv read but cleared */
        port->content.hdcptx->BksvReady = 1;
        ret = AvInvalidParameter;
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxClearBksvReady(pin AvPort *port))
{
    AvRet ret = AvOk;
    GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_BKSV_FLAG(port, 1);
    port->content.hdcptx->BksvReady = 0;

    GSV6K5_TXPHY_set_TX_HDCP1P4_ENC_EN(port, 1);

#if AvEnableDebugHdcp
    AvUapiOutputDebugMessage("Clear Tx Bksv Ready");
#endif

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxClearRxidReady(pin AvPort *port))
{
    AvRet ret = AvOk;

    GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_RXID_LIST_READY(port, 1);
    GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_RXID_READY(port, 1);

#if AvEnableDebugHdcp
    AvUapiOutputDebugMessage("Clear Tx RxID Ready");
#endif

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxGetSinkHdcpCapability(pio AvPort *port))
{
    AvRet ret = AvOk;
    uint8 RbState = 0;
    /* 1. Manual Force HDCP 1.4 Support */
    port->content.hdcptx->HdcpSupport = 1;
    /* 2. Check HDCP 2.2 Capability */
    RbState = Gsv6k5TxScdcAction(port, 0, 0x74, 0x50, 0x00);
    if(RbState == 0x04)
    {
        port->content.hdcptx->Hdcp2p2SinkSupport = 1;
        AvUapiOutputDebugMessage("Port[%d][%d]:Tx is HDCP 2.2 Capable", port->device->index, port->index);
    }
    else
    {
        Gsv6k5ToggleTxHpd(port, 0);
        port->content.hdcptx->Hdcp2p2SinkSupport = 0;
        AvUapiOutputDebugMessage("Port[%d][%d]:Tx is HDCP 2.2 Incapable", port->device->index, port->index);
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxClearHdcpError(pin AvPort *port))
{
    AvRet ret = AvOk;
    if(port->content.hdcptx->Hdcp2p2SinkSupport)
    {
        GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 1);
        GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_ERROR(port, 1);
        GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_ERROR(port, 0);
        GSV6K5_TX2P2_set_HDCP2P2_MASTER_CLK_DIV(port, 0x14A); // lower to 30KHz DDC
        GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 0);
    }
    GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_ERR(port, 1);
    AvUapiOutputDebugMessage("Port[%d][%d]:Clear Tx Hdcp Error%d", port->device->index, port->index, port->content.hdcptx->HdcpError);

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxReadBksv(pio AvPort *port, uint8 *Value, uint8 Count))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxHdmiClearKsvFifo(pio AvPort *port))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxCheckBksvExisted(pin AvPort *port, uint8 *Bksv))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxAddBksv(pio AvPort *port, uint8 *Value, uint8 Position))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxGetBksvTotal(pio AvPort *port, uint8 *Value))
{
    return AvOk;
}

/**
 * @brief  Read Packet Content
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetPacketContent(pio AvPort *port, PacketType Pkt, uint8 *Content))
{
    AvRet ret = AvOk;
    uint8 value = 0;

#if AvEnableInternalVideoGen
    if(port->type == VideoGen)
    {
        switch(Pkt)
        {
            case AV_PKT_AV_INFO_FRAME:
                AvMemset(Content, 0, PktSize[AV_PKT_AV_INFO_FRAME]);
                Content[0] = 0x82; /* AV Info  */
                Content[1] = 0x02; /* Ver2     */
                Content[2] = 0x0D; /* Length   */
                Content[4] = ((uint8)(port->content.video->Y))<<5;
                Content[7] = (uint8)(port->content.video->timing.Vic);
                break;
        }
        return AvOk;
    }
#endif
#if AvEnableInternalAudioGen
    if(port->type == AudioGen)
    {
        switch(Pkt)
        {
            case AV_PKT_AUDIO_INFO_FRAME:
                AvMemset(Content, 0, PktSize[AV_PKT_AUDIO_INFO_FRAME]);
                Content[0] = 0x84;
                Content[1] = 0x01;
                Content[2] = 0x0A;
                Content[3] = 0x70;
                Content[4] = 0x01;
                break;
            case AV_PKT_AUDIO_CHANNEL_STATUS:
                AvMemset(Content, 0, PktSize[AV_PKT_AUDIO_CHANNEL_STATUS]);
                Content[0] = 0x04;
                Content[4] = 0x0B;
                Content[5] = 0x00;
                value = LookupValue8((uchar *)AudioSfTable, (uchar)(port->content.audio->SampFreq), 0xff, 2);
                Content[3] = AudioSfTable[value+1];
                break;
        }
        return AvOk;
    }
#endif

    switch (Pkt)
    {
        case AV_PKT_AMD_PACKET:
        case AV_PKT_EMP_PACKET:
        case AV_PKT_HDR_PACKET:
            AvHalI2cRdMultiField(GSV6K5_RXINFO2_MAP_ADDR(port), RxPktID[Pkt], 2, (Content+1));
            AvHalI2cRdMultiField(GSV6K5_RXINFO2_MAP_ADDR(port), RxPktPtr[Pkt], PktSize[Pkt]-3, (Content+3));
            if(Pkt == AV_PKT_AMD_PACKET)
                Content[0] = 0x0D;
            else if(Pkt == AV_PKT_HDR_PACKET)
                Content[0] = 0x87;
            else if(Pkt == AV_PKT_EMP_PACKET)
                Content[0] = 0x7F;
            break;
        case AV_PKT_AUDIO_CHANNEL_STATUS:
            GSV6K5_RXDIG_get_RB_RX_AUDIO_LAYOUT(port, &Content[5]);
            AvHalI2cRdMultiField(GSV6K5_RXAUD_MAP_ADDR(port), RxPktID[Pkt], 5, &Content[0]);
            /* GSV6K5_RXAUD_get_COMPRESS_AUDIO_PC_VALID(port, &value); */
            value = (Content[0]&0x02)>>1; /* check when stream is compressed audio */
            if(value == 1)
                AvHalI2cRdMultiField(GSV6K5_RXAUD_MAP_ADDR(port), 0xFD, 2, &Content[6]);
            else
            {
                Content[6] = 0x00;
                Content[7] = 0x00;
            }
            GSV6K5_INT_get_RX1_CSDATA_VALID_INT_ST(port, &value);
            if (value != 0)
                GSV6K5_INT_set_RX1_CSDATA_VALID_CLEAR(port, 1);
            break;
        case AV_PKT_VS_PACKET:
        case AV_PKT_AV_INFO_FRAME:
        case AV_PKT_SPD_PACKET:
        case AV_PKT_AUDIO_INFO_FRAME:
        case AV_PKT_MPEG_PACKET:
        case AV_PKT_ACP_PACKET:
        case AV_PKT_ISRC1_PACKET:
        case AV_PKT_ISRC2_PACKET:
        case AV_PKT_GMD_PACKET:
            AvHalI2cRdMultiField(GSV6K5_RXINFO_MAP_ADDR(port), RxPktID[Pkt], 3, Content);
            AvHalI2cRdMultiField(GSV6K5_RXINFO_MAP_ADDR(port), RxPktPtr[Pkt], PktSize[Pkt]-3, (Content+3));
            break;
        default:
            ret = AvNotSupport;
            break;
    }
    if(Pkt == AV_PKT_AV_INFO_FRAME)
    {
        port->content.rx->Cks.AvCks = Content[3];
        //AvUapiOutputDebugMessage("Port[%d][%d]:Av Info [4] = %x, [7] = %x", port->device->index, port->index, Content[4], Content[7]);
    }
    return ret;
}

/**
 * @brief  Clear Rx Audio Related Interrupt
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxClearAudioInterrupt(pio AvPort *port, AudioInterrupt* Intpt))
{
    AvRet ret = AvOk;
    uint8 value = 0;

    /* AudChanMode */
    if(Intpt->AudChanMode)
    {
        GSV6K5_INT_set_RX1_AUDIO_LAYOUT_CLEAR(port, 1);
    }
    /* InternMute */
    if(Intpt->InternMute)
    {
        GSV6K5_INT_set_RX1_HDMI_AVMUTE_CLEAR(port,1);
    }
    /* CsDataValid */
    if(Intpt->CsDataValid)
    {
        GSV6K5_RXDIG_get_RX_APLL_MAN_START_CALC(port, &value);
        if(value == 0)
        {
            GSV6K5_INT_set_RX1_CSDATA_VALID_CLEAR(port,1);
            GSV6K5_INT_set_RX1_PKT_CS_CLEAR(port,1);
            /* Channel Status continuous Interrupt protection */
            GSV6K5_INT_get_RX1_PKT_CS_INT_ST(port, &value);
            if(value == 1)
            {
                /* GSV6K5_RXAUD_set_RX_PKT_DET_RST(port, 1); */
                GSV6K5_RXAUD_set_DISCARD_CSDATA_DIS(port, 0);
                GSV6K5_RXDIG_set_N_CTS_STABLE_DET_START(port, 1);
                GSV6K5_RXDIG_set_RX_APLL_MAN_START_CALC(port, 1);
                AvUapiOutputDebugMessage("Port[%d][%d]:Cs Packet Change", port->device->index, port->index);
            }
        }
    }
    /* NChange */
    if(Intpt->NChange)
    {
        GSV6K5_RXDIG_get_RX_APLL_MAN_START_CALC(port, &value);
        if(value == 0)
            GSV6K5_INT_set_RX1_PKT_N_CLEAR(port,1);
    }
    /* CtsThresh */
    if(Intpt->CtsThresh)
    {
        GSV6K5_INT_set_RX1_PKTDET_ACR_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_CTS_CLEAR(port,1);
    }
    /* AudFifoOv */
    if(Intpt->AudFifoOv)
    {
        GSV6K5_INT_set_RX1_PKT_OVFL_CLEAR(port,1);
    }
    /* AudFifoUn */
    if(Intpt->AudFifoUn)
    {
        GSV6K5_INT_set_RX1_PKT_UNDFL_CLEAR(port,1);
    }
    /* AudFifoNrOv */
    if(Intpt->AudFifoNrOv)
    {
        GSV6K5_INT_set_RX1_PKT_ALMOST_OVFL_CLEAR(port,1);
    }
    /* AudioPktErr */
    if(Intpt->AudioPktErr)
    {
        GSV6K5_INT_set_RX1_PKT_AUDPKTERR_CLEAR(port,1);
    }
    /* AudModeChg */
    if(Intpt->AudModeChg)
    {
        GSV6K5_INT_set_RX1_CHANGE_AUDIO_MODE_CLEAR(port,1);
    }
    /* AudFifoNrUn */
    if(Intpt->AudFifoNrUn)
    {
        GSV6K5_INT_set_RX1_PKT_ALMOST_UNDFL_CLEAR(port,1);
    }
    /* AudFlatLine */
    /*
    if(Intpt->AudFlatLine)
    {
        GSV6K5_INT_set_RX1_PKT_FLATLINE_CLEAR(port,1);
    }
    */
    /* AudSampChg */
    if(Intpt->AudSampChg)
    {
        GSV6K5_INT_set_RX1_PKT_NEWFREQ_CLEAR(port,1);
    }
    /* AudPrtyErr */
    if(Intpt->AudPrtyErr)
    {
        GSV6K5_INT_set_RX1_PKT_PARITY_CLEAR(port,1);
    }
    /* AudIfValid */
    if(Intpt->AudIfValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_AUD_IF_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_AUD_IF_CLEAR(port,1);
    }
    /* AcpValid */
    if(Intpt->AcpValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_ACP_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_ACP_CLEAR(port,1);
    }
    /* AmdValid */
    if(Intpt->AmdValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_AUD_MD_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_AUD_MD_CLEAR(port,1);
    }

    return ret;
}

/**
 * @brief  Clear Rx Video Related Interrupt
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxClearVideoInterrupt(pio AvPort *port, VideoInterrupt* Intpt))
{
    AvRet ret = AvOk;

    /* AvMute */
    if(Intpt->AvMute)
    {
        GSV6K5_INT_set_RX1_AVMUTE_CLEAR(port,1);
    }
    /* HdmiModeChg */
    if(Intpt->HdmiModeChg)
    {
        GSV6K5_INT_set_RX1_HDMI_MODE_CLEAR(port,1);
    }
    /* DeRegenLck */
    if(Intpt->DeRegenLck)
    {
        GSV6K5_INT_set_RX1_HS_LOCKED_CLEAR(port, 1);
    }
    /* VsyncLck */
    if(Intpt->VsyncLck)
    {
        GSV6K5_INT_set_RX1_VS_LOCKED_CLEAR(port, 1);
    }
    /* Vid3dDet */
    if(Intpt->Vid3dDet)
    {
        GSV6K5_INT_set_RX1_VIDEO_3D_CLEAR(port,1);
    }
    /* NewTmds */
    if(Intpt->NewTmds)
    {
        GSV6K5_INT_set_RX1_PKT_NEWFREQ_CLEAR(port,1);
    }
    /* BadTmdsClk */
    if(Intpt->BadTmdsClk)
    {
        GSV6K5_INT_set_RX1_CLKCHANGEDETECTED_CLEAR(port,1);
    }
    /* DeepClrChg */
    if(Intpt->DeepClrChg)
    {
        GSV6K5_INT_set_RX1_DEEP_COLOR_CHANGE_CLEAR(port,1);
    }
    /* PktErr */
    if(Intpt->PktErr)
    {
        /* PktErr triggers HDCP error */
        if((port->content.hdcp->Hdcp2p2RxRunning == 1) || (port->content.rx->VideoEncrypted == 1))
            port->content.hdcp->HdcpError = port->content.hdcp->HdcpError + 1;
        GSV6K5_INT_set_RX1_PKT_PKTERR_CLEAR(port,1);
#if AvEnableRxSingleLaneMode
        if((port->content.rx->H2p1FrlManual != 0) && (port->content.rx->IsFreeRun == 1))
        {
            port->content.rx->H2p1FrlLockDelay = port->content.rx->H2p1FrlLockDelay + 1;
            if(port->content.rx->H2p1FrlLockDelay > AvHdcpRxErrorThreshold)
            {
                port->content.rx->H2p1FrlLockDelay = 0;
                Gsv6k5RxManualEQUpdate(port, 1);
                port->content.rx->Lock.PllLock   = 0;
                port->content.rx->Lock.EqLock    = 0;
                port->content.rx->Lock.AudioLock = 0;
                port->content.rx->IsInputStable  = 0;
                AvUapiOutputDebugMessage("Port[%d][%d]:PktErr triggers RxHPD Toggle", port->device->index, port->index);
            }
        }
#endif
    }
    /* AvIfValid */
    if(Intpt->AvIfValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_AVI_IF_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_AVI_IF_CLEAR(port,1);
    }
    /* SpdValid */
    if(Intpt->SpdValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_SPD_IF_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_SPD_IF_CLEAR(port,1);
    }
    /* HdrValid */
    if(Intpt->HdrValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_DRM_IF_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_DRM_IF_CLEAR(port,1);
    }
    /* EmpValid */
    if(Intpt->EmpValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_SPR_CLEAR(port, 1);
        GSV6K5_INT_set_RX1_PKT_NEW_SPR_PKT_CLEAR(port, 1);
    }
    /* MsValid */
    if(Intpt->MsValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_MPEG_IF_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_MPEG_IF_CLEAR(port,1);
    }
    /* VsValid */
    if(Intpt->VsValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_VS_IF_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_VS_IF_CLEAR(port,1);
    }
    /* Isrc1Valid */
    if(Intpt->Isrc1Valid)
    {
        GSV6K5_INT_set_RX1_PKTDET_ISRC1_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_ISRC1_CLEAR(port,1);
    }
    /* Isrc2Valid */
    if(Intpt->Isrc2Valid)
    {
        GSV6K5_INT_set_RX1_PKTDET_ISRC2_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_ISRC2_CLEAR(port,1);
    }
    /* GamutValid */
    if(Intpt->GamutValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_GAMUT_CLEAR(port,1);
        GSV6K5_INT_set_RX1_PKT_NEW_GAMUT_CLEAR(port,1);
    }
    /* GcValid */
    if(Intpt->GcValid)
    {
        GSV6K5_INT_set_RX1_PKTDET_GCP_CLEAR(port,1);
    }

    return ret;
}


/**
 * @brief  Clear Rx Hdcp Related Interrupt
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxClearHdcpInterrupt(pio AvPort *port, HdcpInterrupt* Intpt))
{
    AvRet ret = AvOk;

    if(Intpt->AksvUpdate)
    {
        Gsv6k5IntptClr(AKSV_UPDATE);
    }
    /* Intpt->Encrypted is cleared in AvUapiRxGetHdcpStatus() */

    return ret;
}

/**
 * @brief  Enable/Disable packets and info-frames
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxEnableInfoFrames(AvPort* port, uint32 InfoFrames, uint8 Enable))
{
    AvRet ret = AvOk;
#if ((Gsv6k5MuxMode == 0) || (AvEnableInternalVideoGen == 1))
    if((port->core.HdmiCore == -1) || (port->content.tx->Hpd == AV_HPD_LOW))
        return ret;

    uint32 CurrentShadow = 0;
    uint32 ShadowEnable  = 0;
    uint32 ShadowDisable = 0;
    uint32 ShadowFlagBit = 0;
    uint8 PktEn = 0;
    uint8 PktShadow = 0;

    /* 1. Check Shadow and Enable State */
    if(Enable == 1)
        PktEn = 1;
    else if(Enable == 2)
    {
        PktEn = 1;
        PktShadow = 1;
    }

    /* 2. Get Current Shadow State */
    GSV6K5_TXPKT_get_TX_PKT_UPDATE(port, &CurrentShadow);
    ShadowEnable  = CurrentShadow;
    ShadowDisable = CurrentShadow;

    /* 3. Set Packet Enable Regs */
    if(InfoFrames & AV_BIT_AV_INFO_FRAME)
        ShadowFlagBit = ShadowFlagBit | 0x0002;
    if(InfoFrames & AV_BIT_AUDIO_INFO_FRAME)
        ShadowFlagBit = ShadowFlagBit | 0x0008;
    if(InfoFrames & AV_BIT_ACP_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0020;
    if(InfoFrames & AV_BIT_SPD_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0004;
    if(InfoFrames & AV_BIT_GMD_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0100;
    if(InfoFrames & AV_BIT_MPEG_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0010;
    if(InfoFrames & AV_BIT_VS_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0001;
    if(InfoFrames & AV_BIT_ISRC1_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0040;
    if(InfoFrames & AV_BIT_ISRC2_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0080;
    if(InfoFrames & AV_BIT_GC_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0200;
    if(InfoFrames & AV_BIT_HDR_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0400;
    if(InfoFrames & AV_BIT_EMP_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x0800;
    if(InfoFrames & AV_BIT_SPARE3_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x1000;
    if(InfoFrames & AV_BIT_SPARE4_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x2000;
    if(InfoFrames & AV_BIT_AMD_PACKET)
        ShadowFlagBit = ShadowFlagBit | 0x4000;
    /* 4. Set Packet Shadow Regs */
    ShadowEnable  = ShadowEnable  | ShadowFlagBit;
    ShadowDisable = ShadowDisable & (~ShadowFlagBit);
    /* 4.1 Set Shadow */
    if(PktEn == 1)
        GSV6K5_TXPKT_set_TX_PKT_UPDATE(port, ShadowEnable);
    /* 4.2 Set Infoframe Enable Bit */
    if(InfoFrames & AV_BIT_AV_INFO_FRAME)
        GSV6K5_TXDIG_set_TX_PKT_AVIIF_PKT_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_AUDIO_INFO_FRAME)
        GSV6K5_TXDIG_set_TX_PKT_AUDIO_INFO_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_ACP_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_ACP_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_SPD_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_SPDIF_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_GMD_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_GM_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_MPEG_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_MPEG_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_VS_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_VSIF_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_ISRC1_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_ISRC_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_ISRC2_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_ISRC_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_GC_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_GC_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_ACR_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_N_CTS_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_AUDIO_SAMPLE_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_AUDIO_SAMPLE_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_HDR_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_USER0_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_EMP_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_USER1_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_SPARE3_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_USER2_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_SPARE4_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_USER3_ENABLE(port, PktEn);
    if(InfoFrames & AV_BIT_AMD_PACKET)
        GSV6K5_TXDIG_set_TX_PKT_USER4_ENABLE(port, PktEn);
    /* 4.3 Release shadow */
    if(PktShadow == 0)
        GSV6K5_TXPKT_set_TX_PKT_UPDATE(port, ShadowDisable);

#endif
    return ret;
}

/**
 * @brief  Configure Packet Content
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetPacketContent(pio AvPort *port, PacketType Pkt, uint8 *Content, uint8 PktEn))
{
    AvRet ret = AvOk;
    AvPort *FromPort = Gsv6k5FindHdmiRxFront(port);
#if (Gsv6k5EmpPktBypass == 0)
    uint8 value = 0;
#endif

#if ((Gsv6k5MuxMode == 0) || (AvEnableInternalVideoGen == 1))
    PacketType MapPkt = Pkt;
    /* Don't Output when HPD Low */
    if((port->core.HdmiCore == -1) || (port->content.tx->Hpd == AV_HPD_LOW))
        return ret;

    uint32 InfoFrameBit = 0;
    if(port->type == HdmiTx)
    {
        /* Map Packet Bit */
        switch(Pkt)
        {
            case AV_PKT_AV_INFO_FRAME:
                #if (Gsv6k5AviInfoBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_AV_INFO_FRAME;
                break;
            case AV_PKT_HDR_PACKET:
                #if (Gsv6k5HdrInfoBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_HDR_PACKET;
                break;
            case AV_PKT_VS_PACKET:
                #if (Gsv6k5VsInfoBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_VS_PACKET;
                break;
            case AV_PKT_SPD_PACKET:
                #if (Gsv6k5SpdInfoBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_SPD_PACKET;
                break;
            case AV_PKT_AUDIO_INFO_FRAME:
                #if (Gsv6k5AudioInfoBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_AUDIO_INFO_FRAME;
                break;
            case AV_PKT_AUDIO_CHANNEL_STATUS:
                break;
            case AV_PKT_MPEG_PACKET:
                #if (Gsv6k5MpegInfoBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_MPEG_PACKET;
                break;
            case AV_PKT_ACP_PACKET:
                #if (Gsv6k5AcpPktBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_ACP_PACKET;
                break;
            case AV_PKT_ISRC1_PACKET:
                #if (Gsv6k5IsrcPktBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_ISRC1_PACKET;
                break;
            case AV_PKT_ISRC2_PACKET:
                #if (Gsv6k5IsrcPktBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_ISRC2_PACKET;
                break;
            case AV_PKT_GMD_PACKET:
                #if (Gsv6k5GmdPktBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_GMD_PACKET;
                break;
            case AV_PKT_AMD_PACKET:
                #if (Gsv6k5AmdPktBypass == 1)
                if(FromPort->type == HdmiRx)
                    break;
                #endif
                InfoFrameBit = AV_BIT_AMD_PACKET;
                break;
            case AV_PKT_EMP_PACKET:
                #if (Gsv6k5EmpPktBypass == 0)
                /* 1. Map Packet to separate address */
                if((Content[2] == 0x00) &&           /* DSC Index 0 related */
                   ((Content[3] & 0x3F) == 0x06) &&  /* VFR related */
                   (Content[5] == 0x01) &&           /* HDMI 2.1 defined */
                   (Content[7] == 0x02))             /* CVTEM item */
                {
                    InfoFrameBit = AV_BIT_ISRC1_PACKET;
                    MapPkt = AV_PKT_ISRC1_PACKET;
                }
                else if(Content[2] == 0x01)
                {
                    InfoFrameBit = AV_BIT_ISRC2_PACKET;
                    MapPkt = AV_PKT_ISRC2_PACKET;
                }
                else if(Content[2] == 0x03)
                {
                    InfoFrameBit = AV_BIT_SPARE3_PACKET;
                    MapPkt = AV_PKT_SPARE3_PACKET;
                }
                else if(Content[2] == 0x04)
                {
                    InfoFrameBit = AV_BIT_SPARE4_PACKET;
                    MapPkt = AV_PKT_SPARE4_PACKET;
                }
                else if(Content[2] == 0x05)
                {
                    InfoFrameBit = AV_BIT_AMD_PACKET;
                    MapPkt = AV_PKT_AMD_PACKET;
                }
                else
                    InfoFrameBit = AV_BIT_EMP_PACKET;
                /* 2. EMP */
                if(FromPort->type == HdmiRx)
                {
                    AvUapiOutputDebugMessage("Port[%d][%d]:EMP%d Write", port->device->index, port->index, Content[2]);
                    value = Content[2] + 1;
                    if(value < FromPort->content.video->EmpTotal)
                    {
                        AvHalI2cWriteField8(GSV6K5_RXINFO_MAP_ADDR(FromPort),0xFE,0x0F,0,value);
                    }
                    if(FromPort->content.video->EmpTotal > 1)
                    {
                        GSV6K5_TXFRL_set_FRL_TMDS_HBLANK_MAN_EN(port, 1);
                        GSV6K5_TXFRL_set_FRL_HTOTAL_STABLE_MAN_EN(port, 1);
                    }
                }
                #endif
                break;
            case AV_PKT_GC_PACKET:
                InfoFrameBit = AV_BIT_GC_PACKET;
                break;
        }
        switch(MapPkt)
        {
            case AV_PKT_AV_INFO_FRAME:
                if((Content[1] == 0x00) || (Content[2] == 0x00))
                {
                    Content[1] = 0x02;
                    Content[2] = 0x0D;
                }
                Gsv6k5_TxSendAVInfoFrame(port, Content, PktEn);
                break;
            case AV_PKT_AUDIO_CHANNEL_STATUS:
                GSV6K5_TXDIG_set_TX_AUDIO_CS_OVERRIDE(port, 1);
                GSV6K5_TXDIG_set_TX_AUDIO_CS_FORMAT(port,port->content.audio->Consumer);
                GSV6K5_TXDIG_set_TX_AUDIO_CS_COPYRIGHT(port,        port->content.audio->Copyright);
                GSV6K5_TXDIG_set_TX_AUDIO_CS_FORMAT_EXTRA(port,        port->content.audio->Emphasis);
                GSV6K5_TXDIG_set_TX_AUDIO_CS_MODE(port,       port->content.audio->ClkAccur);
                GSV6K5_TXDIG_set_TX_AUDIO_CS_CATEGORY_CODE(port, port->content.audio->CatCode);
                GSV6K5_TXDIG_set_TX_AUDIO_CS_SOURCE_NUMBER(port, port->content.audio->SrcNum);
                GSV6K5_TXDIG_set_TX_AUDIO_CS_WORD_LENGTH(port,   port->content.audio->WordLen);
                GSV6K5_TXDIG_set_TX_AUDIO_LAYOUT_MAN_VALUE(port, port->content.audio->Layout);
                Gsv6k5_TxSetAudChStatSampFreq(port);
                /* Audio UnMute */
                GSV6K5_TXDIG_set_TX_AUDIO_MUTE(port, 0);
                break;
            case AV_PKT_HDR_PACKET:
                FromPort = (AvPort*)(port->content.RouteVideoFromPort);
                if(FromPort->type == VideoColor)
                {
                    if((FromPort->content.color->Hdr2SdrEnable == 1) &&
                       (FromPort->content.scaler->ScalerInCs != FromPort->content.scaler->ScalerOutCs))
                        break;
                }
                else if(FromPort->type == VideoScaler)
                {
                    if((FromPort->content.scaler->Hdr2SdrEnable == 1) &&
                       (FromPort->content.scaler->ScalerInCs != FromPort->content.scaler->ScalerOutCs))
                        break;
                }
            case AV_PKT_EMP_PACKET:
                Gsv6k5AvUapiTxEnableInfoFrames(port, InfoFrameBit, 2);
                //AvCheckSum(Content,PktSize[MapPkt],3); /* no checksum byte in EMP */
                AvHalI2cWrMultiField(GSV6K5_TXDIG_MAP_ADDR(port), TxPktPtr[MapPkt], PktSize[MapPkt], Content);
                Gsv6k5AvUapiTxEnableInfoFrames(port, InfoFrameBit, PktEn);
                break;
            case AV_PKT_SPARE3_PACKET:
            case AV_PKT_SPARE4_PACKET:
            case AV_PKT_AMD_PACKET:
                Gsv6k5AvUapiTxEnableInfoFrames(port, InfoFrameBit, 2);
                AvHalI2cWrMultiField(GSV6K5_TXCEC_MAP_ADDR(port), TxPktPtr[MapPkt], PktSize[MapPkt], Content);
                Gsv6k5AvUapiTxEnableInfoFrames(port, InfoFrameBit, PktEn);
                break;
            case AV_PKT_VS_PACKET:
            case AV_PKT_SPD_PACKET:
            case AV_PKT_AUDIO_INFO_FRAME:
            case AV_PKT_MPEG_PACKET:
            case AV_PKT_ACP_PACKET:
            case AV_PKT_ISRC1_PACKET:
            case AV_PKT_ISRC2_PACKET:
            case AV_PKT_GMD_PACKET:
                Gsv6k5AvUapiTxEnableInfoFrames(port, InfoFrameBit, 2);
                AvHalI2cWrMultiField(GSV6K5_TXPKT_MAP_ADDR(port), TxPktPtr[MapPkt], PktSize[MapPkt], Content);
                Gsv6k5AvUapiTxEnableInfoFrames(port, InfoFrameBit, PktEn);
                break;
            case AV_PKT_GC_PACKET:
                if(port->content.tx->HdmiMode == 0)
                    Gsv6k5AvUapiTxEnableInfoFrames(port, AV_BIT_GC_PACKET, 0);
                else
                    Gsv6k5AvUapiTxEnableInfoFrames(port, AV_BIT_GC_PACKET, PktEn);
                break;
            default:
                ret = AvNotSupport;
                break;
        }
    }
#endif
    return ret;
}

/**
 * @brief  Get N and CTS Value
 * @return AvOk: success
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetHdmiAcrInfo (AvPort *port))
{
    uint8   value;
    uint32  usdw;
    uint32  Fcode;

    GSV6K5_INT_get_RX1_PKTDET_ACR_RAW_ST(port, &value);

    if(value == 1)
    {
        /* Reading dummy CTS is for protection of reading N */
        /* directly reading N will result in 0 */
        GSV6K5_RXDIG_get_RB_RX_AUDIO_CTS(port, &port->content.audio->CtsValue);
        if(port->content.audio->CtsValue > 0xF0000)
            GSV6K5_RXDIG_set_CTS_STABLE_DELTA(port, 0x10);
        else
            GSV6K5_RXDIG_set_CTS_STABLE_DELTA(port, 0xFF0);

        /* Audio Fifo Used Words */
        GSV6K5_RXDIG_set_APLL_CG_DEBUG_SEL(port, 5);
        GSV6K5_RXDIG_set_APLL_CAP_USDW(port, 1);
        GSV6K5_RXDIG_set_APLL_CAP_USDW(port, 0);
        GSV6K5_RXDIG_get_APLL_RB_F_CODE(port, &usdw);
        GSV6K5_RXDIG_set_APLL_CG_DEBUG_SEL(port, 0);
        //AvUapiOutputDebugMessage("Port[%d][%d]:Usdw=%d", port->device->index, port->index,usdw);
        if((usdw > 340) && (usdw < 360))
        {
            /* Audio Delta Calculation */
            if(port->content.audio->AudType == AV_AUD_TYPE_HBR)
            {
                GSV6K5_RXDIG_set_APLL_CODE_DELTA(port, 0x80000);
                Fcode = 0x8000;
            }
            else
            {
                GSV6K5_RXDIG_get_APLL_RB_F_CODE(port, &Fcode);
                Fcode = (32<<12)+(Fcode>>15);
                GSV6K5_RXDIG_set_APLL_CODE_DELTA(port, Fcode);
                Fcode = Fcode>>6;
            }
            GSV6K5_RXDIG_set_APLL_CHK_USDW_INRANGE_DELTA(port, Fcode);
        }
#if AvEnableTxSingleLaneMode
        else
        {
            GSV6K5_RXDIG_get_APLL_CG_1MS_1ADDR_DELTA_MAN_EN(port, &value);
            if(value == 1)
            {
                GSV6K5_RXDIG_set_APLL_CODE_DELTA(port, 0x800000);
                GSV6K5_RXDIG_set_APLL_CHK_USDW_INRANGE_DELTA(port, 0x40000);
            }
        }
#endif
        GSV6K5_RXDIG_get_RB_RX_AUDIO_N(port, &port->content.audio->NValue);
        if (port->content.audio->NValue != 0)
        {
            GSV6K5_SEC_set_MUX_MODE_N_VALUE(port, port->content.audio->NValue);
            return AvOk;
        }
    }
    return AvError;
}

/*********************** Local Functions **************************/

void Gsv6k5DisableRxHpa(pin AvPort *port)
{
    uint8 temp;
    uint8 PreviousValue;
    GSV6K5_PRIM_get_RX_RTERM_EN(port, &temp);
    PreviousValue = temp;

#if (AvAllowHpdLowEdidRead==0)
    switch(port->index)
    {
        case 0: /* rx port A */
            GSV6K5_RXRPT_set_RXA_EDID_EN(port, 0);
            break;
        default:
            return;
    }
#endif
    if(port->core.HdmiCore == 0)
    {
      if((port->device->index != 0) && (port->index != 6))
        AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port),0xD9,0xFF,0,0x00);
        /* HF2-79 CTS Compliance */
        if(port->content.rx->H2p1FrlManual == 0)
        {
            GSV6K5_RXFRL_set_FRL_RX_RSCC_CLR(port, 1);
            GSV6K5_RXDIG_set_FRL_CTRL_MAN_EN(port, 1);
            GSV6K5_RXDIG_set_FRL_MODE_MAN(port, 1);
            GSV6K5_RXDIG_set_FRL_CTRL_MAN_EN(port, 0);
            GSV6K5_RXDIG_set_FRL_MODE_MAN(port, 0);
        }
        GSV6K5_RXDIG_set_SCDC_EN(port, 0);
        GSV6K5_RXDIG_set_CED_EN(port, 0);
        GSV6K5_RXFRL_set_FRL_CED_EN(port, 0);
#if AvEnableHdcp2p2Feature
        GSV6K5_RXDIG_set_ECC_AVMUTE_MAN_EN(port, 1);
        GSV6K5_RXDIG_set_ECC_AVMUTE_MAN_EN(port, 0);
#endif
        if(port->content.hdcp->HdcpNeeded == AV_HDCP_RX_NOT_SUPPORT)
            GSV6K5_PRIM_set_RX1_HDCP_EN(port, 0);
    }

    switch(port->index)
    {
        case 0: /* rx port 0 */
            temp = temp&0xFE;
            break;
        default:
            return;
    }
    if(temp != PreviousValue)
    {
        /* HDMI Interface Power Down */
        GSV6K5_PRIM_set_RX_HPD_MAN_VALUE(port, temp);        /* HPA=0 */
        GSV6K5_PRIM_set_RX_RTERM_EN(port, temp);             /* RTERM = 0 */
        GSV6K5_PRIM_set_RX_TMDSPLL_EN_OVERRIDE(port, temp);   /* CLK=0 */
        GSV6K5_PRIM_set_RX_CHANNEL_EN_OVERRIDE(port, temp);   /* CHANNEL=0 */
        AvUapiOutputDebugMessage("Port[%d][%d]:Disable Rx Hpa %x...", port->device->index, port->index, temp);
    }

    return;
}

void Gsv6k5EnableRxHpa(pin AvPort *port)
{
    uint8 temp;
    uint8 PreiousValue;
    GSV6K5_PRIM_get_RX_RTERM_EN(port, &temp);
    PreiousValue = temp;

#if (AvAllowHpdLowEdidRead==0)
    switch(port->index)
    {
        case 0: /* rx port 0 */
            GSV6K5_RXRPT_set_RXA_EDID_EN(port, 1);
            break;
        default:
            return;
    }
#endif

    if(port->core.HdmiCore == 0)
    {
        GSV6K5_RXDIG_set_SCDC_EN(port, 1);
        if(port->content.hdcp->HdcpNeeded != AV_HDCP_RX_NOT_SUPPORT)
        {
            GSV6K5_PRIM_set_RX1_HDCP_EN(port, 1);
        }
    }

    /* HDMI Interface Power Up */
    switch(port->index)
    {
        case 0: /* rx port A */
            temp = temp|0x01;
            break;
        default:
            return;
    }
    if(temp != PreiousValue)
    {
        GSV6K5_PRIM_set_RX_RTERM_EN(port, temp);             /* RTERM = 1 */
        GSV6K5_PRIM_set_RX_HPD_MAN_VALUE(port, temp);    /* HPA=1 */
        GSV6K5_PRIM_set_RX_TMDSPLL_EN_OVERRIDE(port, temp);  /* CLK=1 */
        GSV6K5_PRIM_set_RX_CHANNEL_EN_OVERRIDE(port, temp);  /* CHANNEL=1 */
        AvUapiOutputDebugMessage("Port[%d][%d]:Enable Rx Hpa %x...", port->device->index, port->index, temp);
    }
}

void Gsv6k5GetRx5VStatus(pin AvPort *port)
{
    uint8 value = 0;
    switch(port->index)
    {
        case 0:
            GSV6K5_INT_get_RXA_CABLE_DETECT_RAW_ST(port, &value);
            break;
    }
    if((port->content.rx->Input5V == 1) && (value == 0))
        AvUapiOutputDebugMessage("Port[%d][%d]:Rx5V Loss", port->device->index, port->index);
    port->content.rx->Input5V = value;
}

void Gsv6k5ToggleTmdsOut(AvPort *port, uint8 Enable)
{
    uint8 BulkWrite = 0xf0;
    uint8 FtcWrite  = 0x00;
    if(Enable == 1)
    {
        if(port->type == HdmiTx)
        {
            BulkWrite = 0xff;
            if((port->content.tx->H2p1FrlType != AV_FRL_NONE) || (port->content.tx->H2p1FrlManual != 0))
                FtcWrite = 0x0f;
        }
    }
    else
    {
        if(port->content.tx->H2p1FrlStat == AV_FRL_TRAIN_ONGOING)
        {
            Gsv6k5ToggleTxHpd(port, 0);
            AvUapiOutputDebugFsm("Port[%d][%d]:Stop Ongoing FRL training", port->device->index, port->index);
        }
    }
    GSV6K5_TXPHY_set_EN_FTC(port, FtcWrite);
  if((port->device->index != 0) && (port->index != 6))
    GSV6K5_TXPHY_set_EN_SRFTC(port, 0x00);
    /* TMDS CLK, Lane0, Lane1, Lane2, Ser0, Ser1, Ser2 */
    AvHalI2cWriteField8(GSV6K5_TXPHY_MAP_ADDR(port), 0xF2, 0xFF, 0x0, BulkWrite);
    if(Enable == 1)
        AvUapiOutputDebugFsm("Port[%d][%d]:Set TMDS to 1...", port->device->index, port->index);
    else
        AvUapiOutputDebugFsm("Port[%d][%d]:Set TMDS to 0...", port->device->index, port->index);
}

void Gsv6k5SetTxHdcpVersion(pin AvPort *port, HdcpTxSupportType HdmiStyle, uint8 ForceUpdate)
{
    uint8 HdcpMan = 0;
    uint8 HdcpValue = 0;
    uint8 PrevHdcpMan = 0;
    uint8 PrevHdcpValue = 0;
    /* Set Tx HDCP Version */
    switch(HdmiStyle)
    {
        case AV_HDCP_TX_1P4_ONLY:
        case AV_HDCP_TX_1P4_FAIL_OUT:
            HdcpMan   = 1;
            HdcpValue = 0;
            break;
        case AV_HDCP_TX_2P2_ONLY:
        case AV_HDCP_TX_2P2_FAIL_OUT:
            HdcpMan   = 1;
            HdcpValue = 1;
            break;
        default:
            break;
    }
    switch(port->index)
    {
        case 4:
            GSV6K5_PRIM_get_TXA_CFG_TO_HDCP2P2(port,  &PrevHdcpValue);
            GSV6K5_PRIM_get_TXA_HDCP_CFG_MAN_EN(port, &PrevHdcpMan);
            break;
        case 5:
            GSV6K5_PRIM_get_TXB_CFG_TO_HDCP2P2(port,  &PrevHdcpValue);
            GSV6K5_PRIM_get_TXB_HDCP_CFG_MAN_EN(port, &PrevHdcpMan);
            break;
        case 6:
            GSV6K5_PRIM_get_TXC_CFG_TO_HDCP2P2(port,  &PrevHdcpValue);
            GSV6K5_PRIM_get_TXC_HDCP_CFG_MAN_EN(port, &PrevHdcpMan);
            break;
        case 7:
            GSV6K5_PRIM_get_TXD_CFG_TO_HDCP2P2(port,  &PrevHdcpValue);
            GSV6K5_PRIM_get_TXD_HDCP_CFG_MAN_EN(port, &PrevHdcpMan);
            break;
    }
    if((PrevHdcpValue != HdcpValue) || (PrevHdcpMan != HdcpMan) || (ForceUpdate == 1))
    {
        GSV6K5_TXPHY_set_TX_HDCP1P4_ENC_EN(port, 0);
        GSV6K5_TX2P2_set_TX_HDCP2P2_ENCRYPTION_ENABLE(port, 0);
        GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 1);
        switch(port->index)
        {
            case 4:
                GSV6K5_PRIM_set_TXA_HDCP_EN(port, 0);
                GSV6K5_PRIM_set_TXA_CFG_TO_HDCP2P2(port,  HdcpValue);
                GSV6K5_PRIM_set_TXA_HDCP_CFG_MAN_EN(port, HdcpMan);
                GSV6K5_PRIM_set_TXA_HDCP_EN(port, 1);
                break;
            case 5:
                GSV6K5_PRIM_set_TXB_HDCP_EN(port, 0);
                GSV6K5_PRIM_set_TXB_CFG_TO_HDCP2P2(port,  HdcpValue);
                GSV6K5_PRIM_set_TXB_HDCP_CFG_MAN_EN(port, HdcpMan);
                GSV6K5_PRIM_set_TXB_HDCP_EN(port, 1);
                break;
            case 6:
                GSV6K5_PRIM_set_TXC_HDCP_EN(port, 0);
                GSV6K5_PRIM_set_TXC_CFG_TO_HDCP2P2(port,  HdcpValue);
                GSV6K5_PRIM_set_TXC_HDCP_CFG_MAN_EN(port, HdcpMan);
                GSV6K5_PRIM_set_TXC_HDCP_EN(port, 1);
                break;
            case 7:
                GSV6K5_PRIM_set_TXD_HDCP_EN(port, 0);
                GSV6K5_PRIM_set_TXD_CFG_TO_HDCP2P2(port,  HdcpValue);
                GSV6K5_PRIM_set_TXD_HDCP_CFG_MAN_EN(port, HdcpMan);
                GSV6K5_PRIM_set_TXD_HDCP_EN(port, 1);
                break;
        }
        GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 0);
        Gsv6k5ToggleTxHpd(port, 0);
    }
    if(HdmiStyle == AV_HDCP_TX_ILLEGAL_NO_HDCP)
        HdcpValue = 0;
    else
        HdcpValue = 1;
    GSV6K5_TXPHY_set_TX_HDCP1P4_ENC_EN(port, HdcpValue);
    GSV6K5_TX2P2_set_TX_HDCP2P2_ENCRYPTION_ENABLE(port, HdcpValue);
}

void Gsv6k5ToggleTxHpd(pin AvPort *port, uint8 EdidRead)
{
    if(EdidRead == 0)
    {
        GSV6K5_TXPHY_set_TX_NOT_READ_EDID_EN(port, 1);
    }
    GSV6K5_TXPHY_set_TX_HPD_SRC_SEL(port, 3); /* Always Set HPD to 1 */
    GSV6K5_TXPHY_set_HPD_FILTER_EN(port, 0);
    GSV6K5_TXPHY_set_DDC_RESCUE_EN(port, 1); /* Manual Rescue Tx DDC */
    GSV6K5_TXPHY_set_DDC_RESCUE_EN(port, 0);
    GSV6K5_TXPHY_set_TX_HPD_MAN_VALUE(port, 0); /* Toggle HPD to 0 */
    AvUapiOutputDebugFsm("Port[%d][%d]:Toggle Tx HPD...", port->device->index, port->index);
    GSV6K5_TXPHY_set_TX_HPD_MAN_VALUE(port, 1); /* Toggle HPD to 1 */
    GSV6K5_TXPHY_set_HPD_FILTER_EN(port, 1);
    GSV6K5_TXPHY_set_TX_HPD_SRC_SEL(port, 0); /* Normal HPD Setting */
    GSV6K5_TXPHY_set_EDID_ANALYZE_DONE(port, 1);
    if(EdidRead == 0)
    {
        GSV6K5_TXPHY_set_TX_EDID_TRY_TIMES(port, 0);
        GSV6K5_TXPHY_set_TX_NOT_READ_EDID_EN(port, 0);
    }
}

void Gsv6k5UpdateRxCdrBandWidth(pin AvPort *port)
{
    uint8 value;
    uint8 temp;
    uint32 value32;
    /* Recalc PLL parameter */
    GSV6K5_PLL_set_RXA_PLL_DIV_CALC_MAN_START(port, 1);
    GSV6K5_PLL_set_RXA_PLL_DIV_CALC_MAN_START(port, 0);
    GSV6K5_PLL_set_RXA_PLL_LOCK_CAP_SEL(port, 1);
    GSV6K5_PLL_get_RB_RXA_PLL_VCO_FREQ(port, &value32);
    GSV6K5_PLL_get_RB_RXA_PLL_SER_DIV(port, &value);
    temp = (1<<value) * 5;
    value32 = value32/temp;
    if(port->content.rx->H2p1FrlManual != 0)
        value = 0x40;
    else if(value32 > 806)
        value = 0x41;
    else if(value32 > 620)
        value = 0x42;
    else if(value32 > 550)
        value = 0x40;
    else if(value32 > 320)
        value = 0x53;
    else if(value32 > 200)
        value = 0x63;
    else
    {
        if(port->type == DpRx)
            value = 0x75;
        else
            value = 0x63;
    }
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0xAF, 0xFF, 0x0, value);
    if((value32 > 1020) && (value32 < 1280))
        value = 0x00;
    else
        value = 0x01;
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0x13, 0xFF, 0x0, value);
    if(value32 < 100)
    {
        value = 0x00;
        temp  = 0x00;
    }
    else
    {
        value = 0x01;
        temp  = 0x10;
    }
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0x60, 0xFF, 0x0, value);
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0x63, 0xFF, 0x0, temp);
    if(value32 > 320)
        value = 0x18;
    else
        value = 0x15;
    AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),  0x1D, 0xFF, 0, value);
    /* Reset Adaptive EQ FSM, remove when AUTO EQ */
    GSV6K5_LANE_set_ADP_EQ_FSM_RESET(port, 1);
#if (GSV2502 == 0)
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0x99, 0x80, 0x0, 0x80);
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0x99, 0x80, 0x0, 0x00);
#endif
    GSV6K5_LANE_set_ADP_EQ_FSM_RESET(port, 0);
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0xE0, 0xFF, 0x0, 0x06);
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0xE4, 0xFF, 0x0, 0x03);
    if((port->type != HdmiRx) || (value32 > 599))
        AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0xE0, 0xFF, 0x0, 0x07);
    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0xE4, 0xFF, 0x0, 0x02);
}

void Gsv6k5RxScdcMonitor(pin AvPort *port)
{
#if AvEnableTmdsRxCts
    uint8 value = 0;
    GSV6K5_RXSCDC_get_RR_ENABLE(port, &value);
    if(value == 1)
    {
        /* 1. RR Test */
        AvHalI2cReadField8(GSV6K5_RXSCDC_MAP_ADDR(port), 0xC0, 0xFF, 0, &value);
        if((value & 0x80) != 0)
        {
            GSV6K5_RXDIG_set_DBG_RR_REQ(port, 1);
            AvUapiOutputDebugMessage("Port[%d][%d]:RR", port->device->index, port->index);
        }
    }
#endif
}

void Gsv6k5RxScdcCedClear(pin AvPort *port)
{
    GSV6K5_RXFRL_set_FRL_CED_ERR_CLR(port, 1);
    GSV6K5_RXFRL_set_FRL_RX_RSCC_CLR(port, 1);
    GSV6K5_RXFRL_set_FRL_CED_ERR_CLR(port, 0);
    GSV6K5_RXFRL_set_FRL_RX_RSCC_CLR(port, 0);
}

uint8 Gsv6k5TxScdcAction(pin AvPort *port, uint8 WriteEnable, uint8 DevAddr, uint8 RegAddr, uint8 Value)
{
    uint8 round;
    uint8 PageAddr;
    uint8 DoneFlag;
    /* 1. Write Page Address and Rega Address, Value */
    if(WriteEnable)
        PageAddr = DevAddr & 0xFE;
    else
        PageAddr = DevAddr | 0x01;

    GSV6K5_TXPHY_set_RX_DDC_UDP_DEV_ADDR(port, PageAddr);
    GSV6K5_TXPHY_set_RX_DDC_UDP_SUB_ADDR(port, RegAddr);
    if(WriteEnable)
        GSV6K5_TXPHY_set_RX_DDC_UDP_WRITE_DATA(port, Value);
    GSV6K5_TXPHY_set_RX_DDC_UDP_ACCESS_REQ(port, 1);
    /* 3. Wait For Complete */
    for(round=0;round<0x18;round++)
    {
        GSV6K5_TXPHY_get_RX_DDC_UDP_DONE(port, &DoneFlag);
        if(DoneFlag == 1)
            break;
    }
    /* 4. Handle Return Value */
    if(DoneFlag == 0)
        round = 0xff;
    else
    {
        GSV6K5_TXPHY_get_RX_DDC_UDP_READ_DATA(port, &round);
    }
    return round;
}

AvRet Gsv6k5TxCoreInUse(pin AvPort *port)
{
    AvRet ret = AvError;
    AvPort *TxAFromPort = NULL;
    AvPort *TxBFromPort = NULL;
    AvPort *TxCFromPort = NULL;
    AvPort *TxDFromPort = NULL;
    AvPort *CurrentPort = port->device->port;
    uint8 PortMatchCount = 0;

    /* 1. when Tx Core is not Hdmi Tx, don't check */
    if((port->type == HdmiTx) && (CurrentPort != NULL))
    {
        /* 2. Find TxA and TxB */
        /* 2.1 Find Valid Tx in same device */
        while((CurrentPort != NULL) && (CurrentPort->device->index == port->device->index))
        {
            if(CurrentPort->type == HdmiTx)
            {
                if((CurrentPort->content.tx->InfoReady > TxVideoManageThreshold) && (CurrentPort->core.HdmiCore == 0))
                {
                    switch(CurrentPort->index)
                    {
                        case 4:
                            TxAFromPort = (AvPort*)(CurrentPort->content.RouteVideoFromPort);
                            break;
                        case 5:
                            TxBFromPort = (AvPort*)(CurrentPort->content.RouteVideoFromPort);
                            break;
                        case 6:
                            TxCFromPort = (AvPort*)(CurrentPort->content.RouteVideoFromPort);
                            break;
                        case 7:
                            TxDFromPort = (AvPort*)(CurrentPort->content.RouteVideoFromPort);
                            break;
                    }
                }
            }
            CurrentPort = (AvPort*)(CurrentPort->next);
        }
        /* 2.2 Find both Tx, check its source */
        CurrentPort = (AvPort*)(port->content.RouteVideoFromPort);
        /* 2.2.1 Txs have Same Source means match */
        if((TxAFromPort != NULL) && (CurrentPort->index == TxAFromPort->index))
            PortMatchCount = PortMatchCount + 1;
        if((TxBFromPort != NULL) && (CurrentPort->index == TxBFromPort->index))
            PortMatchCount = PortMatchCount + 1;
        if((TxCFromPort != NULL) && (CurrentPort->index == TxCFromPort->index))
            PortMatchCount = PortMatchCount + 1;
        if((TxDFromPort != NULL) && (CurrentPort->index == TxDFromPort->index))
            PortMatchCount = PortMatchCount + 1;
        /* Find More than one Match Output Port */
        if(PortMatchCount >= 1)
            ret = AvOk;
    }
    return ret;
}

AvRet Gsv6k5TxSchedule(pin AvPort *port)
{
    AvRet ret = AvError;
    AvPort *CurrentPort = port->device->port;
    uint8 PortInAction = 0;
    uint8 Protection = 0;

    /* 1. when Tx Core is not Hdmi Tx, don't check */
    if((port->type == HdmiTx) && (CurrentPort != NULL))
    {
        /* 2. Find Valid Tx in same device */
        while((CurrentPort != NULL) && (CurrentPort->device->index == port->device->index))
        {
            if(CurrentPort->type == HdmiTx)
            {
                if((CurrentPort->content.tx->InfoReady > TxVideoManageThreshold) &&
                   (CurrentPort->content.tx->InfoReady <= (TxVideoManageThreshold+10)))
                    Protection = 1;
                else if(PortInAction == 0)
                {
                    if((CurrentPort->content.tx->InfoReady == TxVideoManageThreshold) ||
                       (CurrentPort->content.tx->InfoReady == (TxVideoManageThreshold-1)))
                        PortInAction = CurrentPort->index;
                }
            }
            CurrentPort = (AvPort*)(CurrentPort->next);
        }
        /* 3. Protection */
        if((Protection == 1) || (port->index != PortInAction))
        {
            if(port->content.tx->InfoReady == TxVideoManageThreshold)
                port->content.tx->InfoReady = (TxVideoManageThreshold-1);
        }
    }
    return ret;
}

void Gsv6k5VrrTimingAdjust(pin AvPort *port)
{
#if AvEnableDetailTiming
    uint32 k = 0;

    if((port->content.video->timing.VBack  == 0)  ||
       (port->content.video->timing.VSync  == 0)  ||
       (port->content.video->timing.VBack  > 200) ||
       (port->content.video->timing.VSync  > 50)  ||
       ((((port->content.video->timing.VActive & 0x0F) != 0) && (port->content.video->timing.HActive != 1920)) ||
        (((port->content.video->timing.VActive & 0x07) != 0) && (port->content.video->timing.HActive == 1920))))
    {
        AvUapiRxGetVideoTiming(port);
    }

    for(k=0; Gsv6k5VrrAdjustTable[k]!=0xFFFF; k=k+10)
    {
        if((port->content.video->timing.Vic != 0) &&
           ((port->content.video->timing.Vic == Gsv6k5VrrAdjustTable[k+8]) ||
            (port->content.video->timing.Vic == Gsv6k5VrrAdjustTable[k+9])))
        {
            port->content.video->timing.HFront  = Gsv6k5VrrAdjustTable[k+0];
            port->content.video->timing.HSync   = Gsv6k5VrrAdjustTable[k+1];
            port->content.video->timing.HBack   = Gsv6k5VrrAdjustTable[k+2];
            port->content.video->timing.VFront  = Gsv6k5VrrAdjustTable[k+3];
            port->content.video->timing.VSync   = Gsv6k5VrrAdjustTable[k+4];
            port->content.video->timing.VBack   = Gsv6k5VrrAdjustTable[k+5];
            port->content.video->timing.HActive = Gsv6k5VrrAdjustTable[k+6];
            port->content.video->timing.VActive = Gsv6k5VrrAdjustTable[k+7];
            port->content.video->timing.VTotal  = Gsv6k5VrrAdjustTable[k+7] +
                Gsv6k5VrrAdjustTable[k+3] + Gsv6k5VrrAdjustTable[k+4] + Gsv6k5VrrAdjustTable[k+5];
            port->content.video->timing.HTotal  = Gsv6k5VrrAdjustTable[k+6] +
                Gsv6k5VrrAdjustTable[k+0] + Gsv6k5VrrAdjustTable[k+1] + Gsv6k5VrrAdjustTable[k+2];
            if(port->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
            {
                port->content.video->timing.HFront  = port->content.video->timing.HFront  >> 1;
                port->content.video->timing.HSync   = port->content.video->timing.HSync   >> 1;
                port->content.video->timing.HBack   = port->content.video->timing.HBack   >> 1;
                port->content.video->timing.HActive = port->content.video->timing.HActive >> 1;
                port->content.video->timing.HTotal  = port->content.video->timing.HTotal  >> 1;
            }
            return;
        }
    }
    for(k=0; Gsv6k5VrrAdjustTable[k]!=0xFFFF; k=k+10)
    {
        if((port->content.video->timing.VSync   == Gsv6k5VrrAdjustTable[k+4]) &&
           (port->content.video->timing.VBack   == Gsv6k5VrrAdjustTable[k+5]))
        {
            if((port->content.video->timing.HActive == Gsv6k5VrrAdjustTable[k+6]) &&
               (port->content.video->timing.HFront  == Gsv6k5VrrAdjustTable[k+0]) &&
               (port->content.video->timing.HSync   == Gsv6k5VrrAdjustTable[k+1]) &&
               (port->content.video->timing.HBack   == Gsv6k5VrrAdjustTable[k+2]))
            {
                if((port->content.video->timing.VActive != Gsv6k5VrrAdjustTable[k+7]) ||
                   (port->content.video->timing.VFront  != Gsv6k5VrrAdjustTable[k+3]))
                {
                    port->content.video->timing.VActive = Gsv6k5VrrAdjustTable[k+7];
                    port->content.video->timing.VFront  = Gsv6k5VrrAdjustTable[k+3];
                    port->content.video->timing.VTotal  = Gsv6k5VrrAdjustTable[k+7] +
                        Gsv6k5VrrAdjustTable[k+3] + Gsv6k5VrrAdjustTable[k+4] + Gsv6k5VrrAdjustTable[k+5];
                    AvUapiOutputDebugMessage("Port[%d][%d]:VRR Timing Correction %d", port->device->index, port->index,port->content.video->timing.VFront);
                    return;
                }
            }
            else if((port->content.video->timing.HActive == (Gsv6k5VrrAdjustTable[k+6]/2)) &&
                    (port->content.video->timing.HFront  == (Gsv6k5VrrAdjustTable[k+0]/2)) &&
                    (port->content.video->timing.HSync   == (Gsv6k5VrrAdjustTable[k+1]/2)) &&
                    (port->content.video->timing.HBack   == (Gsv6k5VrrAdjustTable[k+2]/2)))
            {
                if((port->content.video->timing.VActive != Gsv6k5VrrAdjustTable[k+7]) ||
                   (port->content.video->timing.VFront  != Gsv6k5VrrAdjustTable[k+3]))
                {
                    port->content.video->timing.VActive = Gsv6k5VrrAdjustTable[k+7];
                    port->content.video->timing.VFront  = Gsv6k5VrrAdjustTable[k+3];
                    port->content.video->timing.VTotal  = Gsv6k5VrrAdjustTable[k+7] +
                        Gsv6k5VrrAdjustTable[k+3] + Gsv6k5VrrAdjustTable[k+4] + Gsv6k5VrrAdjustTable[k+5];
                    AvUapiOutputDebugMessage("Port[%d][%d]:VRR Timing Correction %d", port->device->index, port->index,port->content.video->timing.VFront);
                    return;
                }
            }
        }
    }
    k = port->content.video->timing.VActive;
    if(k < 1100)
    {
        if((k & 0x02) != 0)
            k = (k & 0xFFFC) + 0x4;
        else
            k = k & 0xFFFC;
    }
    else
    {
        if((k & 0x06) != 0)
            k = (k & 0xFFF8) + 0x8;
        else
            k = k & 0xFFF8;
    }
    if(port->content.video->timing.VActive != k)
    {
        port->content.video->timing.VActive = k;
        AvUapiOutputDebugMessage("Port[%d][%d]:VRR VActive Correction %d", port->device->index, port->index,port->content.video->timing.VActive);
    }
#endif
}

uint8 Gsv6k5TxDDCError(pin AvPort *port)
{
    uint32 value32 = 0;
    uint8  value   = 0;
    uint8  i       = 0;
    GSV6K5_TXPHY_get_TX_EDID_DDC_ERROR_RB(port, &value);
    if(value == 1)
    {
        GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_ERR(port, 1);
        return 1;
    }
    else
    {
        for(i=0;i<10;i++)
        {
           GSV6K5_TXPHY_set_RD_CPU_PC_TRIGGER(port, 0);
           GSV6K5_TXPHY_set_RD_CPU_PC_TRIGGER(port, 1);
           GSV6K5_TXPHY_get_RB_CPU_PC_CAPTURED(port, &value32);
           if((value32 != 0x58B) && (value32 != 0x58C))
               break;
        }
        if(i == 10)
        {
            Gsv6k5ToggleTxHpd(port, 0);
            return 1;
        }
    }
    return 0;
}

void Gsv6k5ManualBksvRead(pin AvPort *port)
{
#if AvEnableHdcp1p4BksvCheck
    uint8 i = 0;
    uint8 j = 0;
    uint8 OneNumber;
    uint8 RdBksv[5];
    /* 0. Disable HDCP 1.4 and 2.2 FSM */
#ifdef GsvInternalHdcpTxScdcFlag
    Gsv6k5TxScdcAction(port, 1, 0xA8, 0xDF, 0x00);
#else
    GSV6K5_TXPHY_set_TX_HDCP1P4_DESIRED(port, 0);
    GSV6K5_TX2P2_set_TX_HDCP2P2_DESIRED(port, 0);
#endif
    /* 1. Read Sink Bksv */
    Gsv6k5TxScdcAction(port, 0, 0x74, 0, 0x00); /* Redundant Read */
    for(i=0;i<5;i++)
        RdBksv[i] = Gsv6k5TxScdcAction(port, 0, 0x74, i, 0x00);
    AvUapiOutputDebugMessage("Port[%d][%d]:BKSV = %x,%x,%x,%x,%x", port->device->index, port->index,
                             RdBksv[0], RdBksv[1], RdBksv[2], RdBksv[3], RdBksv[4]);
    /* 2. Check HDCP 2.2 Capability */
    j = Gsv6k5TxScdcAction(port, 0, 0x74, 0x50, 0x00);
    if(j == 0x04)
        port->content.hdcptx->Hdcp2p2SinkSupport = 1;
    else
        port->content.hdcptx->Hdcp2p2SinkSupport = 0;
    /* 3. Check Bksv validity */
    OneNumber = 0;
    for(i=0;i<5;i++)
        for(j=0;j<8;j++)
            if(((RdBksv[i]>>j) & 0x01) == 0x01)
                OneNumber++;
    /* 4. Judge Tx HDCP Support */
    if(OneNumber == 20)
    {
        port->content.hdcptx->HdcpSupport = 1;
    }
    else
    {
        port->content.hdcptx->HdcpSupport = 0;
        port->content.hdcptx->Hdcp2p2SinkSupport = 0;
    }
    /* 5. Report HDCP Capability */
    if(port->content.hdcptx->HdcpSupport == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Tx is HDCP 1.4 Capable", port->device->index, port->index);
    if(port->content.hdcptx->Hdcp2p2SinkSupport == 1)
        AvUapiOutputDebugMessage("Port[%d][%d]:Tx is HDCP 2.2 Capable", port->device->index, port->index);
#endif
}

/* Audio Part */

void Gsv6k5_TxSetAudioInterface (AvPort* port)
{
    uint8 I2SBlockEnable = 0;
    uint8 SpdifBlockEnable = 0;
    uint8 DsdBlockEnable = 0;
    uint8 EncodeStyle = 0;
#ifdef AvEnableTDMAudioMode
    uint8 NumCh = 2; /* default 2 channels */
    uint8 NumPins = 0; /* default 1 pin */
    uint8 NumSlot = 2; /* default 2 slots */
    AvPort *FromPort = (AvPort*)(port->content.RouteAudioFromPort);
    if((FromPort->type == HdmiRx) && (FromPort->core.HdmiCore != -1))
    {
        GSV6K5_RXAUD_get_NUM_CH(FromPort, &NumCh);
        NumSlot = NumCh;
    }
#else
    uint8 NumCh = 8;   /* default 8 channels */
    uint8 NumPins = 3; /* default 4 pins */
    uint8 NumSlot = 2; /* default 2 slots */
#endif
    /* Default 128Fs MCLK */
    GSV6K5_TXDIG_set_TX_AUDIO_MCLK_FS_RATIO(port, port->content.audio->AudMclkRatio);

    switch(port->content.audio->AudType)
    {
        case AV_AUD_TYPE_AUTO:
        case AV_AUD_TYPE_ASP:
            I2SBlockEnable = 0xf;
            break;
        case AV_AUD_TYPE_SPDIF:
            SpdifBlockEnable = 1;
            break;
        case AV_AUD_TYPE_HBR:
            SpdifBlockEnable = 1;
            I2SBlockEnable = 0xf;
            EncodeStyle = 0; /* Style = 0 for HBR's SPDIF input mode */
            GSV6K5_TXDIG_set_TX_AUDIO_LAYOUT_MAN_VALUE(port, 1);
            GSV6K5_TXDIG_set_TX_AUDIO_PAPB_SYNC(port, 0);
            GSV6K5_TXDIG_set_TX_AUDIO_PAPB_SYNC(port, 1);
            break;
        case AV_AUD_TYPE_DSD:
            NumCh = port->content.audio->ChanNum;
            if(port->content.audio->Layout == 1)
                NumPins = 3;
            else
                NumPins = 1;
            NumSlot = NumCh/(NumPins+1);
            switch(NumCh)
            {
                case 2:
                    DsdBlockEnable = 0x03;
                    break;
                case 4:
                    DsdBlockEnable = 0x0f;
                    break;
                case 6:
                    DsdBlockEnable = 0x3f;
                    break;
                default:
                    DsdBlockEnable = 0xff;
                    break;
            }
            break;
        case AV_AUD_TYPE_DST:
            DsdBlockEnable = 0x0;
            break;
        case AV_AUD_TYPE_UNKNOWN:
        default:
            break;
    }
    /* I2S default */
    GSV6K5_TXDIG_set_TX_AUDIO_I2S_BLOCK_ENABLE(port, I2SBlockEnable);
    GSV6K5_TXDIG_set_TX_AUDIO_I2S_FORMAT(port, 0);
    /* currently disabled, only useful when AP to Tx */
    GSV6K5_TXDIG_set_TX_AUDIO_SPDIF_ENABLE(port, SpdifBlockEnable);
    /* DSD and DST */
    GSV6K5_TXDIG_set_TDM_DESER_NUM_PINS(port, NumPins);
    GSV6K5_TXDIG_set_TDM_DESER_NUM_SLOT(port, NumSlot);
    GSV6K5_TXDIG_set_TDM_DESER_NUM_CH(port, NumCh);
    GSV6K5_TXDIG_set_DSD_EN(port, DsdBlockEnable);
    /* Encode Style */
    GSV6K5_TXDIG_set_TX_AUDIO_ENCODE_STYLE(port, EncodeStyle);

    return;
}

void Gsv6k5_TxSetAudChStatSampFreq (AvPort* port)
{
    uchar i;
    AvAudioSampleFreq SampFreq = port->content.audio->SampFreq;

    i = LookupValue8 ((uchar *)AudioSfTable, (uchar)SampFreq, 0xff, 2);
    if (AudioSfTable[i] != 0xff)
    {
        if (SampFreq != AV_AUD_FS_FROM_STRM)
        {
            GSV6K5_TXDIG_set_TX_AUDIO_I2S_MAN_SF(port, AudioSfTable[i+1]);
        }
        else
        {
            GSV6K5_TXDIG_set_TX_AUDIO_I2S_MAN_SF(port, 9);
        }
    }

}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetAudNValue(AvPort* port))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    if((port->type == HdmiTx) && (port->core.HdmiCore == 0))
    {
        port->content.audio->NValue = Gsv6k5TuneTxAcr(port, port->content.audio->NValue);
        //GSV6K5_TXDIG_set_TX_AUDIO_MAN_N(port, port->content.audio->NValue);
        value = (port->content.audio->NValue & 0x000ff) >> 0;
        AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port), 0x05, 0xFF, 0x0, value);
        value = (port->content.audio->NValue & 0x0ff00) >> 8;
        AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port), 0x06, 0xFF, 0x0, value);
        value = (port->content.audio->NValue & 0xff0000) >> 16;
        AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port), 0x07, 0x0F, 0x0, value);
        /* reset audio fifo */
        Gsv6k5ResetTxAudioDatapath(port);
    }
    return ret;
}

AvRet Gsv6k5_TxSendAVInfoFrame (AvPort *port, uint8 *AviPkt, uint8 PktEn)
{
    AvRet ret = AvOk;
    AvVideoY InY = AV_Y2Y1Y0_RGB;
    AvVideoY OutY = AV_Y2Y1Y0_RGB;
    uint8 Value;
    uint8 Packet[31];
    AvPort *FromPort = Gsv6k5FindHdmiRxFront(port);

    if((port->type == HdmiTx) && (port->core.HdmiCore == 0))
    {
        AvMemcpy(Packet, AviPkt, PktSize[AV_PKT_AV_INFO_FRAME]);
        /* Step 1. Configure Color Space */
        if(port->content.tx->HdmiMode == 0)
        {
            InY = port->content.video->Y;
            OutY = AV_Y2Y1Y0_RGB;
            if((port->content.video->InCs == AV_CS_LIM_RGB) ||
               (port->content.video->InCs == AV_CS_DEFAULT_RGB) ||
               (port->content.video->InCs == AV_CS_RGB) ||
               (port->content.video->InCs == AV_CS_AUTO))
            {
                port->content.video->OutCs = port->content.video->InCs;
            }
            else
            {
                port->content.video->OutCs = AV_CS_LIM_RGB;
            }
        }
        else
        {
            /* decide input color space */
            if((port->content.video->InCs == AV_CS_YUV_601)  ||
               (port->content.video->InCs == AV_CS_YUV_709)  ||
               (port->content.video->InCs == AV_CS_YCC_601)  ||
               (port->content.video->InCs == AV_CS_YCC_709)  ||
               (port->content.video->InCs == AV_CS_ADOBE_YCC_601) ||
               (port->content.video->InCs == AV_CS_BT2020_YCC)    ||
               (port->content.video->InCs == AV_CS_LIM_YUV_601)  ||
               (port->content.video->InCs == AV_CS_LIM_YUV_709)  ||
               (port->content.video->InCs == AV_CS_LIM_YCC_601)  ||
               (port->content.video->InCs == AV_CS_LIM_YCC_709)  ||
               (port->content.video->InCs == AV_CS_LIM_ADOBE_YCC_601) ||
               (port->content.video->InCs == AV_CS_LIM_BT2020_YCC))
            {
#if AvEnableInternalVideoGen
               if(FromPort->type == VideoGen)
               {
                   InY = AV_Y2Y1Y0_RGB;
                   port->content.video->InCs = AV_CS_RGB;
               }
               else
#endif
                   InY = AV_Y2Y1Y0_YCBCR_444;
            }
            /* decide output color space */
            OutY = port->content.video->Y;
            if(port->content.video->Y != AV_Y2Y1Y0_YCBCR_420)
            {
                if((port->content.video->OutCs == AV_CS_YUV_601)  ||
                   (port->content.video->OutCs == AV_CS_YUV_709)  ||
                   (port->content.video->OutCs == AV_CS_YCC_601)  ||
                   (port->content.video->OutCs == AV_CS_YCC_709)  ||
                   (port->content.video->OutCs == AV_CS_ADOBE_YCC_601) ||
                   (port->content.video->OutCs == AV_CS_BT2020_YCC)    ||
                   (port->content.video->OutCs == AV_CS_LIM_YUV_601)  ||
                   (port->content.video->OutCs == AV_CS_LIM_YUV_709)  ||
                   (port->content.video->OutCs == AV_CS_LIM_YCC_601)  ||
                   (port->content.video->OutCs == AV_CS_LIM_YCC_709)  ||
                   (port->content.video->OutCs == AV_CS_LIM_ADOBE_YCC_601) ||
                   (port->content.video->OutCs == AV_CS_LIM_BT2020_YCC))
                {
                    if(port->content.video->Y == AV_Y2Y1Y0_RGB)
                        OutY = AV_Y2Y1Y0_YCBCR_444;
                }
                else
                    OutY = AV_Y2Y1Y0_RGB;
                if((port->content.video->InCs == AV_CS_BT2020_RGB) ||
                   (port->content.video->InCs == AV_CS_BT2020_DFT_RGB) ||
                   (port->content.video->InCs == AV_CS_BT2020_YCC) ||
                   (port->content.video->InCs == AV_CS_LIM_BT2020_YCC) ||
                   (port->content.video->InCs == AV_CS_LIM_BT2020_RGB))
                {
                    InY = port->content.video->Y;
                    OutY = port->content.video->Y;
                }
            }
            /* Step 2. Color Range */
            /* 2.0.1 YUV Full Range */
            if(OutY != AV_Y2Y1Y0_RGB)
            {
                SET_AVIF_Q(Packet,0);
                Value = (port->content.video->OutCs)>>6;
                SET_AVIF_YQ(Packet,Value);
            }
            /* 2.0.2 RGB Full Range */
            else
            {
                SET_AVIF_YQ(Packet,0);
                Value = (port->content.video->OutCs)>>6;
                SET_AVIF_Q(Packet,Value);
            }
        }
        /* Step 2.1 For Black Mute Choose Correct Color */
        if(InY == AV_Y2Y1Y0_RGB)
            GSV6K5_TXDIG_set_TX_VIDEO_INPUT_CS(port, 0);
        else
            GSV6K5_TXDIG_set_TX_VIDEO_INPUT_CS(port, 1);
        /* Step 2.2 Scan Info Processing */
        FromPort = (AvPort*)(port->content.RouteVideoFromPort);
        if(FromPort->type == VideoScaler)
        {
            Value = GET_AVIF_SC(Packet);
            /* Remove Overscan after downscaling */
            if(Value == 0x01)
                SET_AVIF_SC(Packet,0);
        }
        /* Step 2.3 CSC Setting */
        Gsv6k5_TxSetCSC(port);
        /* Step 3. Aspect Ratio */
        if(port->content.video->timing.Vic <= 107)
        {
            Value = port->content.video->timing.Vic;
            port->content.video->AspectRatio = ARTable[Value];
        }
        switch (port->content.video->AspectRatio)
        {
            case 1:
                SET_AVIF_M(Packet, 1);
                GSV6K5_TXDIG_set_TX_VIDEO_ASPECT_RATIO(port, 0);
                break;
            case 2:
                SET_AVIF_M(Packet, 2);
                GSV6K5_TXDIG_set_TX_VIDEO_ASPECT_RATIO(port, 1);
                break;
            default:
                SET_AVIF_M(Packet, 0);
                GSV6K5_TXDIG_set_TX_VIDEO_ASPECT_RATIO(port, 0);
                break;
        }
        /* Step 4. Pixel Repeat */
        Gsv6k5_TxSetManualPixelRepeat(port);
        SET_AVIF_PR(Packet, port->content.video->PixelRepeatValue);
        /* Step 5. Set Y */
        FromPort = (AvPort*)(port->content.RouteVideoFromPort);
        if((port->content.tx->H2p1FrlType == AV_FRL_NONE) &&
           (port->content.tx->H2p1FrlManual == 0) &&
           (FromPort->type == HdmiRx) &&
           (FromPort->content.video->Y == AV_Y2Y1Y0_YCBCR_422))
            Value = 1;
        else
            Value = 0;
        GSV6K5_TXDIG_set_TX_VIDEO_INPUT_FORMAT(port, Value);
        switch (OutY)
        {
            case AV_Y2Y1Y0_RGB:
                SET_AVIF_Y(Packet, 0);
                //SET_AVIF_C(Packet, 0);
                Value = 0;
                break;
            case AV_Y2Y1Y0_YCBCR_422:
                SET_AVIF_Y(Packet, 1);
                Value = 3;
                break;
            case AV_Y2Y1Y0_YCBCR_444:
                SET_AVIF_Y(Packet, 2);
                Value = 1;
                break;
            case AV_Y2Y1Y0_YCBCR_420:
                SET_AVIF_Y(Packet, 3);
                Value = 1;
                break;
            default:
                SET_AVIF_Y(Packet, 3);
                Value = 0; /* This is an incorrect setting */
                break;
        }
        //if((port->content.video->OutCs == AV_CS_LIM_BT2020_RGB) ||
        //   (port->content.video->OutCs == AV_CS_BT2020_DFT_RGB) ||
        //   (port->content.video->OutCs == AV_CS_BT2020_RGB))
        {
            SET_AVIF_C(Packet,  (port->content.video->OutCs & 0x03));
            SET_AVIF_EC(Packet, ((port->content.video->OutCs >> 2) & 0x07));
        }
        GSV6K5_TXDIG_set_TX_VIDEO_OUTPUT_FORMAT(port, Value);
        /* Step 6. Set AVI Infoframe Content  */
        GSV6K5_TXPKT_set_AVI_IF_TYPE(port, 0x82);
        GSV6K5_TXPKT_set_AVI_IF_VER(port, Packet[1]);
        GSV6K5_TXPKT_set_AVI_IF_LEN(port, Packet[2]);
        #if Gsv6k5AviInfoBypass
        if(FromPort->type != HdmiRx)
        #endif
        {
            Gsv6k5AvUapiTxEnableInfoFrames(port, AV_BIT_AV_INFO_FRAME, 2); /* enable AVI infoframe */
            AvHalI2cWrMultiField(GSV6K5_TXPKT_MAP_ADDR(port),
                             TxPktPtr[AV_PKT_AV_INFO_FRAME],
                             PktSize[AV_PKT_AV_INFO_FRAME]-3,
                             Packet+3);
            Gsv6k5AvUapiTxEnableInfoFrames(port, AV_BIT_AV_INFO_FRAME, PktEn); /* enable AVI infoframe */
        }
        if(PktEn)
        {
            AvUapiOutputDebugFsm("Port[%d][%d]:Y1Y0 = %d,%x", port->device->index, port->index, port->content.video->Y, port->content.video->OutCs);
        }
    }

    return ret;
}

AvRet Gsv6k5_TxSetManualPixelRepeat(AvPort *port)
{
    AvRet ret = AvInvalidParameter;
    uint8 value;
    uint8 Factor = port->content.video->ClockMultiplyFactor;
    uint8 PrValue = port->content.video->PixelRepeatValue;
    /* 4x clock PrValue mapping */
    if(PrValue >= 3)
        PrValue = 2;
    if((Factor > 4) || (PrValue > 3))
    {
        return ret;
    }
    else
    {
        switch(port->index)
        {
            case 4:
                GSV6K5_SEC_get_TXPORT_A_SRC_SEL(port, &value);
                break;
            case 5:
                GSV6K5_SEC_get_TXPORT_B_SRC_SEL(port, &value);
                break;
            case 6:
                GSV6K5_SEC_get_TXPORT_C_SRC_SEL(port, &value);
                break;
            case 7:
                GSV6K5_SEC_get_TXPORT_D_SRC_SEL(port, &value);
                break;
        }
        if(value != 0)
        {
            Factor = 0;
            PrValue = 0;
        }
    }
    GSV6K5_TXDIG_set_TX_VIDEO_TMDS_CLK_RATIO_MAN(port, PrValue);
    GSV6K5_TXDIG_set_TX_VIDEO_PR_TO_RX_MAN(port, PrValue);
    GSV6K5_TXDIG_set_TX_VIDEO_PR_MODE(port, 2);

    ret = AvOk;
    return ret;
}

AvRet Gsv6k5_TxSetCSC (AvPort *port)
{
    AvRet ret = AvOk;
    uint8 i;
    AvVideoCs InCs  = Gsv6k5ColorCsMapping(port->content.video->InCs);
    AvVideoCs OutCs = Gsv6k5ColorCsMapping(port->content.video->OutCs);

    if ((InCs == OutCs) || (InCs == AV_CS_AUTO) || (OutCs == AV_CS_AUTO) ||
        (port->content.video->Y == AV_Y2Y1Y0_YCBCR_420))
    {
        GSV6K5_TXDIG_set_TX_VIDEO_CSC_ENABLE(port, 0);
        ret = AvOk;
    }
    else
    {
        for (i=0; CscTables[i].ConvTable; i++)
        {
            if ((CscTables[i].InCs  == InCs) &&
                (CscTables[i].OutCs == OutCs))
            {
                AvHalI2cWrMultiField(GSV6K5_TXDIG_MAP_ADDR(port), 0x2C, 24, (uint8 *)(CscTables[i].ConvTable));
                AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port),0x2B,0xFF,0,(CscTables[i].ConvTable[0] & 0xE0));
                ret = AvOk;
                break;
            }
        }
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetAvMute(pio AvPort *port))
{
    AvRet ret = AvOk;
    GSV6K5_RXDIG_get_RB_RX_AVMUTE_STATE(port, &(port->content.video->Mute.AvMute));
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetAvMute(pio AvPort *port))
{
    AvRet ret = AvOk;
    if((port->type == HdmiTx) && (port->core.HdmiCore == 0))
    {
        if(port->content.video->Mute.AvMute == 1)
        {
            if(Gsv6k5TxCoreInUse(port) != AvOk)
            {
                GSV6K5_TXDIG_set_TX_GC_CLEAR_AVMUTE(port, 0);
#if (AvEnableTmdsTxCts != 1)
                GSV6K5_TXDIG_set_TX_GC_SET_AVMUTE(port, 1); /* CTS 7-19 Fix */
#endif
                AvUapiOutputDebugMessage("Port[%d][%d]:AVMUTE Enable...", port->device->index, port->index);
            }
        }
        else
        {
            GSV6K5_TXDIG_set_TX_GC_SET_AVMUTE(port, 0);
#if (AvEnableTmdsTxCts != 1)
            GSV6K5_TXDIG_set_TX_GC_CLEAR_AVMUTE(port, 1); /* CTS 7-19 Fix */
#else
            GSV6K5_TXDIG_set_TX_GC_CLEAR_AVMUTE(port, 0); /* CTS 7-19 Fix */
#endif
            AvUapiOutputDebugMessage("Port[%d][%d]:AVMUTE Disable...", port->device->index, port->index);
        }
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetHdmiModeSupport(pio AvPort *port))
{
    AvRet ret = AvOk;
    GSV6K5_RXDIG_get_RB_RX_HDMI_MODE(port, &(port->content.rx->HdmiMode));
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetHdmiModeSupport(pio AvPort *port))
{
    AvRet ret = AvOk;
    AvPort *CurrentPort = port->device->port;
    uint8 PortMatchCount = 0;
    uint8 PortTotalCount = 0;

    if(port->core.HdmiCore == 0)
    {
        while((CurrentPort != NULL) && (CurrentPort->device->index == port->device->index))
        {
            if(CurrentPort->type == HdmiTx)
            {
                PortTotalCount = PortTotalCount + 1;
                if((CurrentPort->core.HdmiCore == 0) &&
                   (CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED))
                {
                    if((CurrentPort->content.tx->HdmiMode == 0) &&
                       (CurrentPort->content.tx->H2p1FrlType == AV_FRL_NONE) &&
                       (CurrentPort->content.tx->H2p1FrlManual == 0))
                        PortMatchCount = PortMatchCount + 1;
                }
                else
                    PortMatchCount = PortMatchCount + 1;
            }
            /* go to next port */
            CurrentPort = (AvPort*)(CurrentPort->next);
        }

        /* Only when all sinks are DVI connected, switch HDMI core to DVI */
        if(PortTotalCount != PortMatchCount)
        {
            GSV6K5_TXDIG_set_TX_HDMI_MODE_OVERRIDE(port, 1);
            GSV6K5_TXDIG_set_TX_HDMI_MODE_MAN(port, 1);
        }
        else
        {
            GSV6K5_TXDIG_set_TX_HDMI_MODE_OVERRIDE(port, 1);
            GSV6K5_TXDIG_set_TX_HDMI_MODE_MAN(port, 0);
        }
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetFeatureSupport(pio AvPort *port))
{
    AvRet ret = AvOk;
    /* Only Enable Tx SCDC, no RR mode */
    GSV6K5_TXPHY_set_TX_SCDC_EN(port, 1);
    GSV6K5_TXPHY_set_RX_SCDC_PRESENT(port, 1);
    /* Disable Tx RR, only use polling to check Scdc Flag */
    GSV6K5_TXPHY_set_TX_RR_CAPABLE(port, 1);
    /* Restore default TxPHY scramble/clock ratio */
    GSV6K5_TXPHY_set_TX_HDMI2_MODE_MAN(port, 0);
    GSV6K5_TXPHY_set_TX_SCR_EN_MAN(port, 0);
#if AvEnableTmdsRxCts
    /*
    if(((port->content.tx->EdidSupportFeature & AV_BIT_FEAT_SCDC) != 0) &&
       ((port->content.tx->EdidSupportFeature & AV_BIT_FEAT_RR) != 0))
    {
        GSV6K5_TXPHY_set_RX_RR_CAPABLE(port, 1);
        Gsv6k5TxScdcAction(port, 1, 0xA8, 0x30, 0x01);
    }
    else
    */
#endif
        GSV6K5_TXPHY_set_RX_RR_CAPABLE(port, 0);

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetVideoLock(pio AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value;
    uint8 temp;
    uint8 det_data[4];
    uint32 capture_data;
    uint8 tmds_pll_lock_flag = 0;
    uint8 de_regen_lock_flag = 0;
    uint8 v_sync_lock_flag = 0;
    uint8 frl_stable_flag = 0;
    uint8 pb_data[10];
    /* AvPort *ToPort = (AvPort*)port->content.RouteVideoToPort; */

    /* Step 1. Manual SCDC FRL check */
    GSV6K5_RXSCDC_get_FRL_RATE(port, &value);
    GSV6K5_INT_get_RX1_FRL_RATE_CHANGE_INT_ST(port, &temp);
    if((value != (uint8)(port->content.rx->H2p1FrlType)) || (temp == 1))
    {
        /* 1.1 VCO limit for H2p1 */
        Gsv6k5UpdateRxCdrBandWidth(port);
        /* 1.3 Update Value */
        AvUapiOutputDebugMessage("Port[%d][%d]:Rx FRL %d->%d, set %d", port->device->index, port->index,(uint8)(port->content.rx->H2p1FrlType),value, temp);
        port->content.rx->H2p1FrlType = (AvFrlType)value;
        port->content.rx->Lock.EqLock = 0;
        tmds_pll_lock_flag = 1;
        /* 1.4 clear interrupts */
        GSV6K5_INT_set_RX1_FRL_RATE_CHANGE_CLEAR(port, 1);
    }
    /* Step 2. Check the lock status */
    GSV6K5_RXDIG_get_RB_RX_TMDS_PLL_LOCKED (port, &(port->content.rx->Lock.PllLock));
    if(port->content.rx->Lock.PllLock == 0)
        Gsv6k5RpllProtect(port);
    /* Step 3. Check whether tmds_pll_lock value has changed */
    if(tmds_pll_lock_flag == 0)
    {
        GSV6K5_PLL_get_RXA_PLL_LOCK_CAP_SEL(port, &value);
        if(value == 1)
        {
            GSV6K5_PLL_set_RXA_PLL_LOCK_CAP_SEL(port, 0);
            GSV6K5_INT_set_RXA_TMDSPLL_LOCK_DET_CLEAR(port, 1);
        }
        else
            GSV6K5_INT_get_RXA_TMDSPLL_LOCK_DET_INT_ST(port, &tmds_pll_lock_flag);
    }
    if(tmds_pll_lock_flag)
    {
        /* Step 3.1 Manual Reset EQ when PLL Lock Status is lost */
        Gsv6k5RxManualEQUpdate(port, 1);
        port->content.rx->Lock.PllLock   = 0;
        port->content.rx->Lock.EqLock    = 0;
        port->content.rx->Lock.AudioLock = 0;
        port->content.rx->IsInputStable  = 0;
        /* EQ reset to default state */
        Gsv6k5RxScdcCedClear(port);
        GSV6K5_RXDIG_set_CED_EN(port, 1);
        GSV6K5_RXFRL_set_FRL_CED_EN(port, 1);
#if AvEnableTmdsRxCts
        /* HF2-18 SCDC CED Manual Control */
        GSV6K5_RXDIG_set_NO_RR(port, 1);
        GSV6K5_RXDIG_set_CED_EN(port, 0);
        GSV6K5_RXDIG_set_DBG_RR_REQ(port, 0);
        GSV6K5_RXDIG_set_DBG_RR_REQ(port, 1);
#endif
        GSV6K5_INT_set_RXA_TMDSPLL_LOCK_DET_CLEAR(port, 1);
        AvUapiOutputDebugMessage("Port[%d][%d]:TMDS PLL Unlock ST triggers EQ ReRun", port->device->index, port->index);
    }
    /* Step 4. FRL mode Rx detection routine */
    if((port->content.rx->H2p1FrlType != AV_FRL_NONE) || (port->content.rx->H2p1FrlManual != 0))
    {
        /* 4.1 Restore to default unlock state */
        port->content.rx->Lock.DeRegenLock = 0;
        port->content.rx->Lock.VSyncLock   = 0;
        frl_stable_flag = 0;
        /* 4.2 lock state detection */
        if((tmds_pll_lock_flag == 0) && (port->content.rx->Lock.PllLock == 1))
        {
            /* 4.2.1 FRL Training State */
            AvHalI2cReadField8(GSV6K5_RXFRL_MAP_ADDR(port),0x56,0xFF,0,&value);
            if(value == 0x10)
                port->content.rx->H2p1FrlStat = AV_FRL_TRAIN_SUCCESS;
            else
                port->content.rx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
            /* 4.2.2 FRL timing TMDS Frequency detection */
            GSV6K5_RXFRL_set_FRL_RX_CG_DEBUG_SEL(port, 0);
            AvHalI2cReadMultiField(GSV6K5_RXFRL_MAP_ADDR(port), 0xA6, 4, det_data);
            if(((det_data[0] != 0x00) || (det_data[1] != 0x00) || (det_data[2] != 0x00) || (det_data[3] != 0x00)) ||
               (port->content.rx->H2p1FrlManual != 0))
            {
                //AvHalI2cReadField8(GSV6K5_RXDIG_MAP_ADDR(port),0xFF,0x01,0,&value);
                capture_data = (det_data[0]<<16) + (det_data[1]<<8) + (det_data[2]<<0);
                if(port->content.video->info.TmdsFreq != capture_data)
                {
                    if(((port->content.video->info.TmdsFreq > capture_data) &&
                        ((port->content.video->info.TmdsFreq - capture_data) > 1)) ||
                       ((capture_data > port->content.video->info.TmdsFreq) &&
                        ((capture_data - port->content.video->info.TmdsFreq) > 1)))
                    {
                        port->content.rx->Lock.DeRegenLock = 0;
                        port->content.rx->Lock.VSyncLock   = 0;
                        port->content.video->EmpTotal = 0;
                        port->content.video->DscStream = 0;
                        port->content.video->VrrStream = 0;
                        port->content.video->info.TmdsFreq = (uint16)capture_data;
                        AvUapiOutputDebugMessage("Port[%d][%d]:Rx FRL TMDS frequency = %d", port->device->index, port->index, capture_data);
                        Gsv6k5RxScdcCedClear(port);
                    }
                    else if(capture_data != 0)
                    {
                        frl_stable_flag = 1;
                    }
                }
                else
                {
                    frl_stable_flag = 1;
                }
            }
            if((port->content.rx->IsInputStable == 0) && (port->content.rx->Lock.EqLock == 1))
            {
                if(port->content.rx->H2p1FrlLockDelay == RxFrlLockExpireThreshold)
                {
                    GSV6K5_RXSCDC_get_FLT_NO_RETRAIN(port, &temp);
                    if(temp == 0)
                    {
                        GSV6K5_RXFRL_set_FRL_RX_LINE_DET_DIS(port, 1);
                        GSV6K5_RXFRL_set_FRL_RX_LINE_DET_DIS(port, 0);
                        if(port->content.rx->H2p1FrlManual != 0)
                            port->content.rx->H2p1FrlLockDelay = 0;
                        AvUapiOutputDebugMessage("Port[%d][%d]:Long Delay triggers ReCalc", port->device->index, port->index);
                        Gsv6k5RxManualEQUpdate(port, 1);
                        port->content.rx->Lock.EqLock = 0;
                        port->content.rx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
                    }
                }
                else
                {
#if AvEnableConsistentFrl
                    if((port->content.rx->H2p1FrlType == AV_FRL_6G3L) || (port->content.rx->H2p1FrlType == AV_FRL_3G3L))
                        value = 0x07;
                    else if(port->content.rx->H2p1FrlManual != 0)
                        value = (port->content.rx->H2p1FrlManual>>16)&0xf;
                    else
                        value = 0x0F;
                    GSV6K5_RXFRL_get_RB_LANE_LOCKED(port, &temp);
                    if(temp == 0)
                    {
                        Gsv6k5RxManualEQUpdate(port, 1);
                        port->content.rx->Lock.EqLock = 0;
                        //AvUapiOutputDebugMessage("Port[%d][%d]:FRL Fifo Align", port->device->index, port->index);
                    }
                    else
#endif
                        port->content.rx->H2p1FrlLockDelay = port->content.rx->H2p1FrlLockDelay + 1;
                }
            }
        }
    }
    /* 4.3 FRL mode protection */
    if((frl_stable_flag == 1) && (port->content.rx->Lock.EqLock == 0))
    {
        port->content.rx->Lock.DeRegenLock = 1;
        port->content.rx->Lock.VSyncLock   = 1;
    }
    /* Step 5. TMDS mode Rx detection routine */
    if(((port->content.rx->H2p1FrlType == AV_FRL_NONE) && (port->content.rx->H2p1FrlManual == 0)) ||
       (frl_stable_flag == 1))
    {
        /* Step 5.1 Check whether is locked */
        if(frl_stable_flag == 1)
        {
            GSV6K5_INT_get_RX1_FRL_HS_LOCK_RAW_ST(port, &(port->content.rx->Lock.DeRegenLock));
            GSV6K5_INT_get_RX1_FRL_VS_LOCK_RAW_ST(port, &(port->content.rx->Lock.VSyncLock));
            GSV6K5_INT_get_RX1_FRL_HS_LOCK_INT_ST(port, &de_regen_lock_flag);
            GSV6K5_INT_get_RX1_FRL_VS_LOCK_INT_ST(port, &v_sync_lock_flag);
        }
        else
        {
            GSV6K5_RXDIG_get_RB_RX_HS_LOCKED(port, &(port->content.rx->Lock.DeRegenLock));
            GSV6K5_RXDIG_get_RB_RX_VS_LOCKED(port, &(port->content.rx->Lock.VSyncLock));
            GSV6K5_INT_get_RX1_HS_LOCKED_INT_ST(port, &de_regen_lock_flag);
            GSV6K5_INT_get_RX1_VS_LOCKED_INT_ST(port, &v_sync_lock_flag);
        }
        /* 5.1.1 DSC register reset if not enabled */
        if((port->content.rx->Lock.EqLock == 1) &&
           ((port->content.rx->Lock.VSyncLock != port->content.rx->Lock.DeRegenLock) ||
            (v_sync_lock_flag != de_regen_lock_flag)))
        {
            GSV6K5_RXAUD_get_SPR_PKT_DET(port, &value);
            if(value == 1)
            {
                AvHalI2cReadMultiField(GSV6K5_RXINFO2_MAP_ADDR(port), 0x41, 9, pb_data);
                /* 5.1.2 VRR mode detection */
                if(((pb_data[0] & 0x3F) == 0x04) &&  /* VFR related */
                   (pb_data[2] == 0x01) &&           /* HDMI 2.1 defined */
                   (pb_data[4] == 0x01))             /* VTEM item */
                {
                    if(pb_data[7] != 0x00)           /* VRR,FVA enabled */
                    {
                        v_sync_lock_flag = 0;
                        port->content.rx->Lock.VSyncLock = 1;
                        if(port->content.video->VrrStream == 0)
                        {
                            port->content.video->VrrStream = 1;
                            GSV6K5_CP_set_CP_VRR_EN(port, 1);
                            AvUapiOutputDebugMessage("Port[%d][%d]:VRR detected", port->device->index, port->index);
                        }
                    }
                    else
                        port->content.video->VrrStream = 0;
                    /* Possible Overlap of DSC and VRR */
                    if((frl_stable_flag == 1) && (port->content.rx->IsInputStable == 1))
                    {
                        GSV6K5_RXDIG_get_RB_RX_HS_LOCKED(port, &temp);
                        if((temp == 0) && (port->content.video->DscStream == 0))
                        {
                            port->content.video->DscStream = 1;
                            AvHalI2cReadField8(GSV6K5_RXINFO_MAP_ADDR(port),0xFF,0xFF,0,&temp);
                            port->content.video->EmpTotal = temp;
                            AvUapiOutputDebugMessage("Port[%d][%d]:VRR+DSC detected, %d", port->device->index, port->index, temp);
                        }
                    }
                }
                /* 5.1.3 DSC mode detection protection */
                else if(((pb_data[0] & 0x3F) == 0x06) &&  /* VFR related */
                        (pb_data[2] == 0x01) &&           /* HDMI 2.1 defined */
                        (pb_data[4] == 0x02))             /* CVTEM item */
                {
                    if(port->content.video->DscStream == 0)
                    {
                        port->content.video->DscStream = 1;
                        AvHalI2cReadField8(GSV6K5_RXINFO_MAP_ADDR(port),0xFF,0xFF,0,&temp);
                        port->content.video->EmpTotal = temp;
                        AvUapiOutputDebugMessage("Port[%d][%d]:DSC detected, %d", port->device->index, port->index, temp);
                    }
                }
                /* 5.1.4 HSync Force for DSC */
                /*
                if(port->content.video->DscStream == 1)
                {
                    de_regen_lock_flag = 0;
                    port->content.rx->Lock.DeRegenLock = 1;
                }
                */
            }
            else
            {
                port->content.video->DscStream = 0;
                GSV6K5_RXAUD_get_SPD_IF_PKT_DET(port, &value);
                if(value == 1)
                {
                    AvHalI2cReadMultiField(GSV6K5_RXINFO_MAP_ADDR(port), 0x2b, 3, pb_data);
                    if((pb_data[0] == 0x1A) && (pb_data[1] == 0x00) && (pb_data[2] == 0x00))
                    {
                        v_sync_lock_flag = 0;
                        port->content.rx->Lock.VSyncLock = 1;
                        if(port->content.video->VrrStream == 0)
                        {
                            port->content.video->VrrStream = 1;
                            GSV6K5_CP_set_CP_VRR_EN(port, 1);
                            AvUapiOutputDebugMessage("Port[%d][%d]:FreeSync Premium detected", port->device->index, port->index);
                        }
                    }
                    else
                        port->content.video->VrrStream = 0;
                }
                else
                    port->content.video->VrrStream = 0;
                if(port->content.video->VrrStream == 0)
                {
                    GSV6K5_CP_set_CP_VRR_EN(port, 0);
                }
            }
        }
        if((de_regen_lock_flag == 1) || (v_sync_lock_flag == 1))
        {
            AvUapiOutputDebugMessage("Port[%d][%d]:Lock h%d-%d,v%d-%d", port->device->index, port->index,
                                     port->content.rx->Lock.DeRegenLock, de_regen_lock_flag,
                                     port->content.rx->Lock.VSyncLock, v_sync_lock_flag);
            port->content.rx->Lock.DeRegenLock = 0;
            port->content.rx->Lock.VSyncLock = 0;
            port->content.video->DscStream = 0;
            port->content.video->VrrStream = 0;
            GSV6K5_INT_set_RX1_FRL_VS_LOCK_CLEAR(port, 1);
            GSV6K5_INT_set_RX1_FRL_HS_LOCK_CLEAR(port, 1);
            GSV6K5_INT_set_RX1_HS_LOCKED_CLEAR(port, 1);
            GSV6K5_INT_set_RX1_VS_LOCKED_CLEAR(port, 1);
            AvHalI2cWriteField8(GSV6K5_RXINFO_MAP_ADDR(port),0xFE,0x0F,0,0x00);
#if AvEnableRxSingleLaneMode
            if((port->content.rx->IsInputStable == 1) && (port->content.rx->H2p1FrlManual != 0))
            {
                port->content.rx->H2p1FrlLockDelay = port->content.rx->H2p1FrlLockDelay + 1;
                if(port->content.rx->H2p1FrlLockDelay >= RxFrlLockExpireThreshold)
                {
                    port->content.rx->Lock.PllLock = 0;
                    port->content.rx->Lock.EqLock = 0;
                    port->content.rx->IsInputStable = 0;
                    Gsv6k5UpdateRxCdrBandWidth(port);
                    AvUapiOutputDebugMessage("Port[%d][%d]:EQ reset by timing change", port->device->index, port->index);
                }
            }
#endif
        }
#if Gsv6k5MuxMode
        if((port->content.video->info.TmdsFreq < 100) && (port->content.video->info.TmdsFreq > 90))
        {
            port->content.rx->Lock.DeRegenLock = port->content.rx->Lock.PllLock;
            port->content.rx->Lock.VSyncLock   = port->content.rx->Lock.PllLock;
        }
#endif
        /* 5.1.5 H2p1FrlManual timing protection */
        /*
        if(port->content.rx->H2p1FrlManual != 0)
        {
            GSV6K5_RXDIG_get_RB_RX_H_ACTIVE_WIDTH(port, &capture_data);
            if(capture_data == 961)
            {
                GSV6K5_RXDIG_get_RB_RX_V_ACTIVE_HEIGHT_0(port, &capture_data);
                if(capture_data == 4320)
                {
                    GSV6K5_RXFRL_get_RB_FRL_RX_TMDS_CLK_DIV4_EN(port, &value);
                    if(value == 1)
                    {
                        port->content.rx->Lock.DeRegenLock = 0;
                        AvUapiOutputDebugMessage("Port[%d][%d]:Manual FRL Timing unlock", port->device->index, port->index);
                    }
                }
            }
        }
        */
        /* 5.2 Status Clear */
        if(de_regen_lock_flag == 1)
        {
            GSV6K5_RXDIG_set_ECC_AVMUTE_MAN_EN(port, 1);
            GSV6K5_RXDIG_set_ECC_AVMUTE_MAN_EN(port, 0);
        }
        /* Step 5.3 Update IsInputStable */
        if((port->content.rx->Lock.DeRegenLock == 0) ||
           (port->content.rx->Lock.VSyncLock   == 0) ||
           (port->content.rx->Lock.PllLock     == 0))
            port->content.rx->IsInputStable = 0;
        /* 5.4 Reset Rx when HDCP process is too early */
        if((port->content.rx->Lock.VSyncLock == 0) &&
           (port->content.hdcp->Hdcp2p2RxRunning == 1) && (port->content.rx->VideoEncrypted == 1))
        {
            port->content.hdcp->HdcpError = port->content.hdcp->HdcpError + 1;
            GSV6K5_RX2P2H_set_RX_HDCP2P2_REAUTH_REQUEST(port, 1);
            GSV6K5_RX2P2H_set_RX_HDCP2P2_REAUTH_REQUEST(port, 0);
        }
        /* 5.5 Deep color Protection */
        if((port->content.rx->H2p1FrlType == AV_FRL_NONE) &&
           (port->content.rx->Lock.DeRegenLock == 1) &&
           (port->content.rx->Lock.VSyncLock   == 1) &&
           (port->content.rx->Lock.PllLock     == 1))
        {
            GSV6K5_RXDIG_get_DCFIFO_LOCKED(port, &value);
            if(value == 0)
            {
                AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port), 0xFC, 0x1, 0x0, 0);
                AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port), 0xFF, 0x1, 0x0, 0);
                AvUapiOutputDebugMessage("Port[%d][%d]:DC", port->device->index, port->index);
            }
        }
        /* 5.6 TMDS mode protection */
        if((port->content.rx->H2p1FrlType == AV_FRL_NONE) && (port->content.rx->H2p1FrlManual == 0))
        {
            if((port->content.rx->Lock.EqLock == 1) &&
               (port->content.rx->IsFreeRun == 0) &&
               (port->content.rx->Lock.DeRegenLock == 0) &&
               (port->content.rx->Lock.VSyncLock   == 0))
            {
                if(port->content.rx->H2p1FrlLockDelay == RxFrlLockExpireThreshold)
                {
                    AvUapiOutputDebugMessage("Port[%d][%d]:Long Delay triggers ReCalc", port->device->index, port->index);
                    Gsv6k5RxManualEQUpdate(port, 1);
                    port->content.rx->Lock.EqLock = 0;
                }
                else
                    port->content.rx->H2p1FrlLockDelay = port->content.rx->H2p1FrlLockDelay + 1;
            }
        }
    }

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetVideoTiming(pio AvPort *port))
{
    AvPort *FromPort = NULL;
    uint8 HPol = 0;
    uint8 VPol = 0;
    uint8 Set = 0;
    if(port->core.HdmiCore != -1)
    {
        if(KfunFindVideoRxFront(port, &FromPort) == AvOk)
        {
            /* no need to set if Rx is not stable */
            if((FromPort->content.rx->IsInputStable == 0) &&
               (FromPort->content.rx->IsFreeRun == 0))
                return AvOk;
            if(FromPort->type == HdmiRx || FromPort->type == DpRx)
            {
                HPol = 1 - port->content.video->timing.HPolarity;
                VPol = 1 - port->content.video->timing.VPolarity;
            }
        }
        Set = 0xF0 | (HPol << 1) | VPol;
        AvHalI2cWriteField8(GSV6K5_TXDIG_MAP_ADDR(port),0x1F,0xFF,0,Set);
        /*
        GSV6K5_TXDIG_set_MAN_POL_HS(port, HPol);
        GSV6K5_TXDIG_set_MAN_POL_VS(port, VPol);
        */
    }

    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxReadStdi(pin AvPort *port))
{
    /* Print HDMI Rx Eye Status */
    Gsv6k5RxEyePrint(port);
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxReadInfo(pio AvPort *port))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetVideoTiming(pio AvPort *port))
{
    AvRet ret = AvOk;
#if AvEnableDetailTiming
    uint8 FrlFlag = 0;
    uint32 TempValue = 0;
    if((port->content.rx->H2p1FrlType != AV_FRL_NONE) || (port->content.rx->H2p1FrlManual != 0))
        GSV6K5_RXFRL_get_RB_FRL_RX_TMDS_CLK_DIV4_EN(port, &FrlFlag);
    if(FrlFlag == 1)
    {
        GSV6K5_RXFRL_get_RB_FRL_RX_H_SYNC_NUM(port, &TempValue);
        if(port->content.video->Cd == AV_CD_30)
            TempValue = (TempValue<<2)/5;
        else if(port->content.video->Cd == AV_CD_36)
            TempValue = (TempValue<<1)/3;
    }
    else
        GSV6K5_RXDIG_get_RB_RX_H_SYNC_WIDTH(port, &TempValue);
    port->content.video->timing.HSync = TempValue;
    if(FrlFlag == 1)
    {
        GSV6K5_RXFRL_get_RB_FRL_RX_H_BACK_NUM(port, &TempValue);
        if(port->content.video->Cd == AV_CD_30)
            TempValue = (TempValue<<2)/5;
        else if(port->content.video->Cd == AV_CD_36)
            TempValue = (TempValue<<1)/3;
    }
    else
        GSV6K5_RXDIG_get_RB_RX_H_BACK_PORCH(port, &TempValue);
    port->content.video->timing.HBack = TempValue;
    if(FrlFlag == 1)
    {
        GSV6K5_RXFRL_get_RB_FRL_RX_H_FRONT_NUM(port, &TempValue);
        if(port->content.video->Cd == AV_CD_30)
            TempValue = (TempValue<<2)/5;
        else if(port->content.video->Cd == AV_CD_36)
            TempValue = (TempValue<<1)/3;
    }
    else
        GSV6K5_RXDIG_get_RB_RX_H_FRONT_PORCH(port, &TempValue);
    port->content.video->timing.HFront = TempValue;
    GSV6K5_RXDIG_get_RB_RX_V_SYNC_WIDTH_0(port, &TempValue);
    port->content.video->timing.VSync = TempValue>>1;
    GSV6K5_RXDIG_get_RB_RX_V_BACK_PORCH_0(port, &TempValue);
    if(TempValue%2 == 1)
        TempValue = TempValue + 1;
    port->content.video->timing.VBack = TempValue>>1;
    GSV6K5_RXDIG_get_RB_RX_V_FRONT_PORCH_0(port, &TempValue);
    port->content.video->timing.VFront = TempValue>>1;
    if(FrlFlag == 1)
    {
        GSV6K5_RXFRL_get_RB_FRL_RX_H_TOTAL_NUM(port, &TempValue);
        if(port->content.video->Cd == AV_CD_30)
            TempValue = (TempValue<<2)/5;
        else if(port->content.video->Cd == AV_CD_36)
            TempValue = (TempValue<<1)/3;
    }
    else
        GSV6K5_RXDIG_get_RB_RX_H_TOTAL_WIDTH(port,&TempValue);
    port->content.video->timing.HTotal  = TempValue;
    /* Interlaced correction */
    GSV6K5_RXDIG_get_RB_RX_INTERLACED(port, &(port->content.video->timing.Interlaced));
    GSV6K5_RXDIG_get_RB_RX_V_ACTIVE_HEIGHT_0(port,&TempValue);
    if((port->content.video->timing.Interlaced) && (TempValue <= 540))
        port->content.video->timing.VActive = TempValue<<1;
    else
    {
        port->content.video->timing.VActive = TempValue;
        port->content.video->timing.Interlaced = 0;
    }
    GSV6K5_RXDIG_get_RB_RX_V_TOTAL_0 (port,&TempValue);
    port->content.video->timing.VTotal  = TempValue>>1;
    if((port->content.video->timing.VTotal > 8192) && (port->content.video->timing.VActive < 4096))
        port->content.video->timing.VActive = port->content.video->timing.VActive + 8192;
    if(FrlFlag == 1)
    {
        GSV6K5_RXFRL_get_RB_FRL_RX_H_ACTIVE_NUM(port, &TempValue);
        if(port->content.video->Cd == AV_CD_30)
            TempValue = (TempValue<<2)/5;
        else if(port->content.video->Cd == AV_CD_36)
            TempValue = (TempValue<<1)/3;
    }
    else
        GSV6K5_RXDIG_get_RB_RX_H_ACTIVE_WIDTH(port,&TempValue);
    port->content.video->timing.HActive = TempValue;
    if((port->content.video->timing.HTotal > 8192) && (port->content.video->timing.HActive < 4096))
        port->content.video->timing.HActive = port->content.video->timing.HActive + 8192;
    if((port->content.video->timing.HTotal != 0) && (port->content.video->timing.VTotal != 0))
        TempValue = (port->content.video->info.TmdsFreq*1000000)/
            ((port->content.video->timing.HTotal) * (port->content.video->timing.VTotal));
    else
        TempValue = 0;
    if(FrlFlag == 1)
    {
        if(port->content.video->Cd == AV_CD_30)
            TempValue = (TempValue<<2)/5;
        else if(port->content.video->Cd == AV_CD_36)
            TempValue = (TempValue<<1)/3;
    }
    if(port->content.video->timing.Interlaced)
        TempValue = TempValue >> 1;
    port->content.video->timing.FrameRate = TempValue + 1;

#endif //AvEnableDetailTiming

    GSV6K5_RXDIG_get_RB_RX_HS_POL(port, &port->content.video->timing.HPolarity);
    GSV6K5_RXDIG_get_RB_RX_VS_POL(port, &port->content.video->timing.VPolarity);
    /* 3D */
    GSV6K5_RXAUD_get_RB_RX_VIDEO_3D(port, &port->content.video->info.Detect3D);

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetHdmiDeepColor(pio AvPort *port))
{
    AvRet ret = AvOk;
    uint8 Value;
    AvVideoCd Cd;
    if(port->type == HdmiRx)
    {
        GSV6K5_RXDIG_get_RB_RX_DEEP_COLOR_MODE(port, &Value);
        if(Value == 0)
            Cd = AV_CD_24;
        else if(Value == 1)
            Cd = AV_CD_30;
        else if(Value == 2)
            Cd = AV_CD_36;
        else
            Cd = AV_CD_48;
        if(port->content.video->Cd != Cd)
        {
            port->content.video->Cd = Cd;
            AvUapiRxGetVideoTiming(port);
        }
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetHdmiDeepColor(pio AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    AvPort *FromPort;
    FromPort = (AvPort*)port->content.RouteVideoFromPort;

    if(FromPort != NULL)
    {
#if Gsv6k5MuxMode
        /* Mux Mode only supports PLL = 1 ratio, no need to extend to 1.25 and 1.5 */
            value = 4;
#else
        if(port->content.video->Cd == AV_CD_30)
            value = 5;
        else if(port->content.video->Cd == AV_CD_36)
            value = 6;
        else if(port->content.video->Cd == AV_CD_48)
            value = 7;
        else
            value = 4;
#endif
        if(FromPort->type == VideoScaler)
        {
            port->content.video->Cd = AV_CD_24;
            value = 4; /* Transmit 24-bit */
        }
        else if(FromPort->type == VideoColor)
        {
            if(FromPort->content.color->Enable == 1)
            {
                port->content.video->Cd = AV_CD_24;
                value = 4; /* Set to 8-bit to make it /4 clock convinient */
            }
        }
    }
    /* DVI protection */
    if(port->content.tx->HdmiMode == 0)
        value = 4;
    if(port->core.HdmiCore == 0)
    {
#if AvEnableTmdsTxCts
        if(value == 4)
            value = 0;
#endif
        GSV6K5_TXDIG_set_TX_GC_CD(port, value);
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxGetAudioInternalMute(pio AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value = 0;
    if(port->type == HdmiRx)
    {
        GSV6K5_INT_get_RX1_HDMI_AVMUTE_RAW_ST(port, &value);
        if(value == 0)
        {
            GSV6K5_INT_get_RX1_PKT_FLATLINE_RAW_ST(port, &value);
            if(value == 1)
                GSV6K5_INT_set_RX1_PKT_FLATLINE_CLEAR(port,1);
        }
        if(port->content.audio->AudioMute != value)
        {
            GSV6K5_RXAUD_set_RX_MUTE_AUDIO(port, value);
            port->content.audio->AudioMute = value;
            AvUapiOutputDebugMessage("Port[%d][%d]:Audio Mute %d", port->device->index, port->index, value);
        }
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxSetAudioInternalMute(pio AvPort *port))
{
    AvRet ret = AvOk;
    return ret;
}

uint8 Gsv6k5_RxGetPixRepValue(AvPort *port)
{
    uint8 RetVal;
    GSV6K5_RXDIG_get_RX_REPETITION_MAN_EN(port, &RetVal);

    if (RetVal == 1)
    {
        GSV6K5_RXDIG_get_RX_REPETITION_MAN(port, &RetVal);
    }
    else
    {
        GSV6K5_RXDIG_get_RB_RX_PIXEL_REPETITION(port, &RetVal);
    }
    return RetVal;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxSetBlackMute(pio AvPort *port))
{
    AvRet ret = AvOk;
    if(port->core.HdmiCore == 0)
    {
        if((port->content.video->InCs == AV_CS_RGB) ||
           (port->content.video->InCs == AV_CS_DEFAULT_RGB) ||
           (port->content.video->InCs == AV_CS_ADOBE_RGB) ||
           (port->content.video->InCs == AV_CS_LIM_RGB) ||
           (port->content.video->InCs == AV_CS_LIM_ADOBE_RGB) ||
           (port->content.video->InCs == AV_CS_AUTO))
            GSV6K5_TXDIG_set_TX_VIDEO_INPUT_CS(port, 0);
        else
            GSV6K5_TXDIG_set_TX_VIDEO_INPUT_CS(port, 1);
        if((Gsv6k5TxCoreInUse(port) != AvOk) || (port->content.video->Mute.BlkMute == 0))
        {
            GSV6K5_TXDIG_set_TX_BLACK_VIDEO_EN(port, port->content.video->Mute.BlkMute);
        }
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxEncryptSink(pin AvPort *port))
{
    AvRet ret = AvOk;
    uint8 value;
    uint8 DviWithHdcp   = 0;
    uint8 Hdcp1p4Enable = 0;
    uint8 Hdcp2p2Enable = 0;

    GSV6K5_TXPHY_get_TX_TMDS_CLK_DRIVER_ENABLE(port, &value);
    if((value == 0) || (port->content.tx->IgnoreEdidError == 1))
        return AvNotAvailable;

    /* Dvi PreProcess */
    if((port->content.tx->HdmiMode == 0) && (port->content.hdcptx->HdcpSupport == 1))
    {
        /* DVI only supports HDMI 1.4 mode encryption */
        if(port->content.tx->H2p1FrlType == AV_FRL_NONE)
        {
            switch(port->content.hdcptx->HdmiStyle)
            {
                case AV_HDCP_TX_AUTO:
                case AV_HDCP_TX_AUTO_FAIL_OUT:
                    if(port->content.hdcptx->DviStyle == AV_HDCP_TX_DVI_LOOSE)
                    {
                        DviWithHdcp   = 1;
                        if(port->content.hdcptx->Hdcp2p2SinkSupport == 1)
                        {
                            GSV6K5_TXPHY_set_TX_NOT_READ_EDID_EN(port, 1);
                            if(port->content.hdcptx->HdmiStyle == AV_HDCP_TX_AUTO_FAIL_OUT)
                                Gsv6k5SetTxHdcpVersion(port, AV_HDCP_TX_1P4_FAIL_OUT, 0);
                            else
                                Gsv6k5SetTxHdcpVersion(port, AV_HDCP_TX_1P4_ONLY, 0);
                            GSV6K5_TXPHY_set_TX_NOT_READ_EDID_EN(port, 0);
                        }
                    }
                    break;
                case AV_HDCP_TX_1P4_ONLY:
                case AV_HDCP_TX_1P4_FAIL_OUT:
                    DviWithHdcp   = 1;
                    break;
                default:
                    break;
            }
        }
        /* Switch to DVI mode */
        else
        {
            port->content.tx->H2p1FrlType = AV_FRL_NONE;
            port->content.tx->Hpd = AV_HPD_FORCE_LOW;
        }
    }
    /* Hdmi Style Select */
    if((port->content.tx->H2p1FrlType != AV_FRL_NONE) ||
       (port->content.tx->HdmiMode == 1) || (DviWithHdcp == 1))
    {
        switch(port->content.hdcptx->HdmiStyle)
        {
            case AV_HDCP_TX_ILLEGAL_NO_HDCP:
                break;
            case AV_HDCP_TX_AUTO:
                Hdcp1p4Enable = 1;
                Hdcp2p2Enable = 1;
                break;
            case AV_HDCP_TX_AUTO_FAIL_OUT:
                if(port->content.hdcptx->HdcpError < AvHdcpTxErrorThreshold)
                {
                    Hdcp1p4Enable = 1;
                    Hdcp2p2Enable = 1;
                }
                break;
            case AV_HDCP_TX_1P4_ONLY:
                Hdcp1p4Enable = 1;
                break;
            case AV_HDCP_TX_1P4_FAIL_OUT:
                if(port->content.hdcptx->HdcpError < AvHdcpTxErrorThreshold)
                    Hdcp1p4Enable = 1;
                break;
            case AV_HDCP_TX_2P2_ONLY:
                Hdcp2p2Enable = 1;
                break;
            case AV_HDCP_TX_2P2_FAIL_OUT:
                if(port->content.hdcptx->HdcpError < AvHdcpTxErrorThreshold)
                    Hdcp2p2Enable = 1;
                break;
        }
        /* No HDCP Support Process */
        if((port->content.hdcptx->HdcpSupport == 0) &&
           (port->content.hdcptx->Hdcp2p2SinkSupport == 0))
        {
            port->content.video->Mute.BlkMute = 1;
            port->content.audio->AudioMute    = 1;
            Hdcp1p4Enable = 0;
            Hdcp2p2Enable = 0;
            if(port->content.hdcptx->HdcpError >= (AvHdcpTxErrorThreshold-1))
            {
                port->content.hdcptx->HdcpError = 0;
                Gsv6k5TxScdcAction(port, 0, 0x74, 0x40, 0x00);
            }
            else
                port->content.hdcptx->HdcpError = port->content.hdcptx->HdcpError + 1;
        }
        if(port->content.hdcptx->HdcpModeUpdate != 0)
        {
            Hdcp1p4Enable = 0;
            Hdcp2p2Enable = 0;
        }
#ifdef GsvInternalHdcpTxScdcFlag
        value = Gsv6k5TxScdcAction(port, 0, 0xA8, 0xDF, 0x00);
        if(value != Hdcp1p4Enable)
        {
            Gsv6k5TxScdcAction(port, 1, 0xA8, 0xDF, Hdcp1p4Enable);
            AvUapiOutputDebugMessage("Port[%d][%d]:Set Tx HDCP Scdc Enable", port->device->index, port->index);
        }
        return AvOk;
#endif
        GSV6K5_TXPHY_get_TX_HDCP1P4_DESIRED(port, &value);
        if(value != Hdcp1p4Enable)
        {
            Gsv6k5TxHBlankControl(port, Hdcp1p4Enable);
            GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_ERR(port, 1);
            GSV6K5_TXPHY_set_TX_HDCP1P4_DESIRED(port, Hdcp1p4Enable);
            GSV6K5_TXPHY_set_TX_HDCP1P4_ENC_EN(port, Hdcp1p4Enable);
            if(Hdcp1p4Enable)
                AvUapiOutputDebugMessage("Port[%d][%d]:Set Tx HDCP 1.4 Cipher Enable", port->device->index, port->index);
        }
    #if AvEnableHdcp2p2Feature
        GSV6K5_TX2P2_get_TX_HDCP2P2_DESIRED(port, &value);
        if(value != Hdcp2p2Enable)
        {
            GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_ERROR(port, 1);
            GSV6K5_TX2P2_set_TX_HDCP2P2_CLEAR_ERROR(port, 0);
            GSV6K5_TX2P2_set_TX_HDCP2P2_DESIRED(port, Hdcp2p2Enable);
            GSV6K5_TX2P2_set_TX_HDCP2P2_ENCRYPTION_ENABLE(port, Hdcp2p2Enable);
            if(Hdcp2p2Enable)
                AvUapiOutputDebugMessage("Port[%d][%d]:Set Tx HDCP 2.2 Cipher Enable", port->device->index, port->index);
        }
    #endif

    }
    /* Dvi Style Select */
    else
    {
        value = 0;
        switch(port->content.hdcptx->DviStyle)
        {
            case AV_HDCP_TX_DVI_STRICT:
                value = 1;
                break;
            case AV_HDCP_TX_DVI_LOOSE:
                value = 0;
                break;
        }
        if(value != port->content.video->Mute.BlkMute)
        {
            port->content.video->Mute.BlkMute = value;
            AvUapiTxSetBlackMute(port);
        }
    }
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxDecryptSink(pin AvPort *port))
{
    AvRet ret = AvOk;
    port->content.hdcptx->HdcpEnabled = 0;
    port->content.hdcptx->Authenticated = 0;
    port->content.hdcptx->HdcpError = 0;

#ifdef GsvInternalHdcpTxScdcFlag
    Gsv6k5TxScdcAction(port, 1, 0xA8, 0xDF, 0);
#endif

    GSV6K5_TXPHY_set_TX_HDCP1P4_ENC_EN(port, 0);
    GSV6K5_TXPHY_set_TX_HDCP1P4_CLR_BKSV_FLAG(port, 1);
    GSV6K5_TXPHY_set_TX_HDCP1P4_DESIRED(port, 0);
    AvUapiOutputDebugMessage("Port[%d][%d]:Set Tx HDCP Cipher Disable", port->device->index, port->index);


#if AvEnableHdcp2p2Feature
    GSV6K5_TX2P2_set_TX_HDCP2P2_ENCRYPTION_ENABLE(port, 0);
    GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 1);
    GSV6K5_TX2P2_set_TX_HDCP2P2_DESIRED(port, 0);
    GSV6K5_TX2P2_set_TX_HDCP2P2_FSM_DISABLE(port, 0);
    /* Possible Force HDCP 2.2 Reset for compatibility */
    /*
    if(port->content.hdcptx->Hdcp2p2TxRunning == 1)
        Gsv6k5TxScdcAction(port, 1, 0x74, 0x14, 0x00);
    */
#endif
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxSetHdcpEnable(pin AvPort *port))
{
    AvRet ret = AvOk;
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxSetBksvListReady(AvPort *port))
{
    AvRet ret = AvOk;
    uint8 On = 0;
    uint8 Clear = 0;
    if(port->content.hdcp->SinkNumber == port->content.hdcp->SinkTotal)
    {
        On = 1;
        Clear = 0;
#if AvEnableDebugHdcp
        AvUapiOutputDebugMessage("Set Rx BKSV Ready");
#endif
    }
    else
    {
        On = 0;
        Clear = 1;
#if AvEnableDebugHdcp
        AvUapiOutputDebugMessage("Clear Rx BKSV Ready");
#endif
    }

    GSV6K5_RXRPT_set_RX_KSV_LIST_NUM(port, port->content.hdcp->SinkTotal);
    GSV6K5_RXRPT_set_RX_KSV_LIST_READY(port, On);
    GSV6K5_RXRPT_set_RX_KSV_LIST_READY_CLR(port, Clear);

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxSetHdcpMode(pin AvPort *port))
{
    AvRet ret = AvOk;
    /* GSV6k5 has 1 Hdmi Core */
    if(port->core.HdmiCore == 0)
    {
        if(port->content.hdcp->SinkTotal == 0)
        {
            GSV6K5_RXRPT_set_RX_HDCP1P4_BCAPS(port, AV_BCAPS_RECEIVER_MODE);
            GSV6K5_RXRPT_set_RX_HDCP1P4_BSTATUS(port, AV_BSTATUS_RECEIVER_MODE);
            AvUapiOutputDebugMessage("Port[%d][%d]:Set BCaps/Status REC mode", port->device->index, port->index);
        }
        else
        {
            GSV6K5_RXRPT_set_RX_HDCP1P4_BCAPS(port, AV_BCAPS_RECEIVER_MODE);
            GSV6K5_RXRPT_set_RX_HDCP1P4_BSTATUS(port, AV_BSTATUS_RECEIVER_MODE);
            AvUapiOutputDebugMessage("Port[%d][%d]:Set BCaps/Status REP mode", port->device->index, port->index);
        }
    }

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiHdcp2p2Mode(pin AvPort *port))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxGetHdcpEnableStatus(pin AvPort *port))
{
    return AvOk;
}

/**
 * @brief  init cec port
 * @return AvOk: success
 * @note
 */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxCecInit(pin AvPort *port))
{
#if AvEnableCecFeature /* CEC Related */
    GSV6K5_TXCEC_set_CEC_POWER_MODE(port, port->content.cec->CecEnable);
    GSV6K5_TXCEC_set_CEC_TX_RETRY(port, 3);
    GSV6K5_TXCEC_set_CEC_RX_3BUFFERS_ENABLE(port, 1);
    GSV6K5_TXCEC_set_CEC_LOGIC_ADDR_SELECTION(port, 0);
    GSV6K5_TXCEC_set_CEC_TX_ENABLE(port, 0);
    GSV6K5_TXCEC_set_SOFT_RESET(port, 1);
    GSV6K5_TXCEC_set_SOFT_RESET(port, 0);
    port->content.cec->TxSendFlag = AV_CEC_TX_IDLE;
    port->content.cec->RxGetFlag = 0;
#endif /* CEC Related */
    return AvOk;

}

AvRet ImplementUapi(Gsv6k5, AvUapiTxCecSetPhysicalAddr(AvPort *port))
{
    AvRet ret = AvOk;
#if AvEnableCecFeature /* CEC Related */
    uint32 phyaddr = 0;
    if(port->type == HdmiTx)
        phyaddr = port->content.tx->PhyAddr;
    GSV6K5_TXCEC_set_CEC_PHY_ADDR(port, phyaddr);
#endif
    return ret;
}

/**
 * @brief  Set the logical address according to structure
 * @return AvOk: success
 */
AvRet ImplementUapi(Gsv6k5, AvUapiTxCecSetLogicalAddr(AvPort *port))
{
    AvRet ret = AvOk;
#if AvEnableCecFeature /* CEC Related */
    uint8 Mask=0, DevBit=0;
    uint8 LogAddr = port->content.cec->LogAddr;
    uint8 DevId = 0;
    uint8 Enable = 1;

    LogAddr &= 0xf;
    switch (DevId)
    {
        case 0:
            GSV6K5_TXCEC_set_CEC_LOGIC_ADDR0(port, LogAddr);
            DevBit = 0x01;
            break;
        case 1:
            GSV6K5_TXCEC_set_CEC_LOGIC_ADDR1(port, LogAddr);
            DevBit = 0x02;
            break;
        case 2:
            GSV6K5_TXCEC_set_CEC_LOGIC_ADDR2(port, LogAddr);
            DevBit = 0x04;
            break;
        default:
            ret = AvInvalidParameter;
            break;
    }

    GSV6K5_TXCEC_get_CEC_LOGIC_ADDR_SELECTION(port, &Mask);
    Mask &= (~DevBit);
    if (Enable)
    {
        Mask |= DevBit;
    }
    if (ret == AvOk)
    {
        GSV6K5_TXCEC_set_CEC_LOGIC_ADDR_SELECTION(port, Mask);
    }
#endif /* CEC Related */
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCecRxGetStatus (AvPort* port))
{
    AvRet ret = AvNotAvailable;
#if AvEnableCecFeature /* CEC Related */
    uint8 BufferOrder = 0;
    uint8 SelectOrder = 0;
    uint8 value0, value1, value2;
    uint8 position = 0;

    /* 1. Find if there is message existed */
    GSV6K5_TXCEC_get_CEC_RX_BUF0_READY(port, &value0);
    GSV6K5_TXCEC_get_CEC_RX_BUF1_READY(port, &value1);
    GSV6K5_TXCEC_get_CEC_RX_BUF2_READY(port, &value2);
    if((value0 == 0) &&
       (value1 == 0) &&
       (value2 == 0))
    {
        port->content.cec->RxGetFlag = 0;
        return ret;
    }
    else
        port->content.cec->RxGetFlag = 1;

    /* 2. Find the Lowest Value */
    GSV6K5_TXCEC_get_CEC_RX_BUF0_TIMESTAMP(port, &BufferOrder);
    if(BufferOrder != 0)
    {
        SelectOrder = BufferOrder;
        position = 0;
    }
    GSV6K5_TXCEC_get_CEC_RX_BUF1_TIMESTAMP(port, &BufferOrder);
    if((SelectOrder == 0) ||
       ((BufferOrder != 0) &&
       (BufferOrder < SelectOrder)))
    {
        SelectOrder = BufferOrder;
        position = 1;
    }
    GSV6K5_TXCEC_get_CEC_RX_BUF2_TIMESTAMP(port, &BufferOrder);
    if((SelectOrder == 0) ||
       ((BufferOrder != 0) &&
       (BufferOrder < SelectOrder)))
    {
        SelectOrder = BufferOrder;
        position = 2;
    }

    /* 3. Dump Rx Data */
    switch(position)
    {
        case CEC_RX_BUFFER1:
            AvHalI2cReadMultiField(GSV6K5_TXCEC_MAP_ADDR(port), 0x15, 17,
                                   port->content.cec->RxContent);
            GSV6K5_TXCEC_get_CEC_RX_BUF0_FRAME_LENGTH(port, &port->content.cec->RxLen);
            /* CEC buffer1 flag need to do toggling */
            GSV6K5_TXCEC_set_CEC_RX_CLEAR_BUF0(port,1);
            GSV6K5_TXCEC_set_CEC_RX_CLEAR_BUF0(port,0);
            ret = AvOk;
            break;
        case CEC_RX_BUFFER2:
            AvHalI2cReadMultiField(GSV6K5_TXCEC_MAP_ADDR(port), 0x27, 17,
                                   port->content.cec->RxContent);
            GSV6K5_TXCEC_get_CEC_RX_BUF1_FRAME_LENGTH(port, &port->content.cec->RxLen);
            /* CEC buffer2 flag need to do toggling */
            GSV6K5_TXCEC_set_CEC_RX_CLEAR_BUF1(port,1);
            GSV6K5_TXCEC_set_CEC_RX_CLEAR_BUF1(port,0);
            ret = AvOk;
            break;
        case CEC_RX_BUFFER3:
            AvHalI2cReadMultiField(GSV6K5_TXCEC_MAP_ADDR(port), 0x38, 17,
                                   port->content.cec->RxContent);
            GSV6K5_TXCEC_get_CEC_RX_BUF2_FRAME_LENGTH(port, &port->content.cec->RxLen);
            /* CEC buffer3 flag need to do toggling */
            GSV6K5_TXCEC_set_CEC_RX_CLEAR_BUF2(port,1);
            GSV6K5_TXCEC_set_CEC_RX_CLEAR_BUF2(port,0);
            ret = AvOk;
            break;
        default:
            break;
    }
#endif
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCecSendMessage(pin AvPort *port))
{
    AvRet ret = AvOk;
#if AvEnableCecFeature /* CEC Related */

    GSV6K5_TXCEC_set_CEC_TX_ENABLE(port, 0);

    if((port->content.cec->TxLen > 16) || (port->content.cec->TxLen == 0))
    {
        port->content.cec->TxSendFlag = AV_CEC_TX_SEND_FAIL;
        ret = AvInvalidParameter;
    }
    else
    {
        /* clear all the interrupts */
        if((port->type == HdmiRx) || (port->type == HdmiTx))
        {
            GSV6K5_INT_set_TX1_TX_CEC_READY_CLEAR(port,          1);
            GSV6K5_INT_set_TX1_TX_ARBITRATION_LOST_CLEAR(port,   1);
            GSV6K5_INT_set_TX1_TX_RETRY_TIMEOUT_CLEAR(port,      1);
        }

        /* CecTxState = CEC_TX_STATE_BUSY; */
        /* 1. Action */
        AvHalI2cWriteMultiField(GSV6K5_TXCEC_MAP_ADDR(port),
                                0x00,
                                port->content.cec->TxLen,
                                port->content.cec->TxContent);
        GSV6K5_TXCEC_set_CEC_TX_FRAME_LENGTH(port, port->content.cec->TxLen);
        GSV6K5_TXCEC_set_CEC_TX_ENABLE(port, 1);
        AvUapiOutputDebugMessage("CEC: OutOp=0x%x,0x%x,0x%x",
                                 port->content.cec->TxContent[0],
                                 port->content.cec->TxContent[1],
                                 port->content.cec->TxContent[2]);
        /* 2. Cleaning */
        /* TxSendFlag = 2 means the data is sent out and waiting response */
        port->content.cec->TxSendFlag = AV_CEC_TX_WAIT_RESPONSE;
        ret = AvOk;
    }
#endif

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiGetNackCount(pin AvPort *port))
{
    AvRet ret = AvOk;
#if AvEnableCecFeature /* CEC Related */
    GSV6K5_TXCEC_get_CEC_TX_NACK_COUNTER(port, &port->content.cec->NackCount);
#endif /* CEC Related */
    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCecTxGetStatus(pin AvPort *port))
{
    AvRet ret = AvOk;
#if AvEnableCecFeature /* CEC Related */
    uint8 value = 0;
    if((port->type == HdmiRx) || (port->type == HdmiTx))
    {
        GSV6K5_INT_get_TX1_TX_CEC_READY_INT_ST(port,        &port->content.cec->TxReady);
        GSV6K5_INT_get_TX1_TX_ARBITRATION_LOST_INT_ST(port, &port->content.cec->ArbLost);
        GSV6K5_INT_get_TX1_TX_RETRY_TIMEOUT_INT_ST(port,    &port->content.cec->Timeout);
    }

    GSV6K5_TXCEC_get_TX_LOWDRIVE_COUNTER(port,          &port->content.cec->LowDriveCount);
    GSV6K5_TXCEC_get_CEC_TX_NACK_COUNTER(port,              &port->content.cec->NackCount);
    GSV6K5_TXCEC_get_CEC_TX_ENABLE(port, &value);
    if(value == 0)
        port->content.cec->TxReady = 1;
    /* arrange send when bus is free */
    if((port->content.cec->TxSendFlag == AV_CEC_TX_TO_SEND) &&
       ((port->content.cec->TxReady == 1) || (port->content.cec->ArbLost == 1) || (port->content.cec->Timeout == 1)))
        AvUapiCecSendMessage(port);
    /* data is sent anyway, find the response */
    else if(port->content.cec->TxSendFlag == AV_CEC_TX_WAIT_RESPONSE)
    {
        if(port->content.cec->TxReady)
            port->content.cec->TxSendFlag = AV_CEC_TX_SEND_SUCCESS;
        else if(port->content.cec->Timeout)
            port->content.cec->TxSendFlag = AV_CEC_TX_SEND_FAIL;
        else if(port->content.cec->ArbLost)
            AvUapiCecSendMessage(port);
    }

    /*
    AvUapiOutputDebugMessage("CEC: TxReady = %d.\r\n", port->content.cec->TxReady);
    AvUapiOutputDebugMessage("CEC: ArbLost = %d.\r\n", port->content.cec->ArbLost);
    AvUapiOutputDebugMessage("CEC: Timeout = %d.\r\n", port->content.cec->Timeout);
    */
#endif /* CEC Related */

    return ret;
}

/* EDID functions */
/* Rx Read Edid */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxReadEdid(pio AvPort *port, uint8 *Value, uint16 Count))
{
    AvRet ret = AvOk;
    return ret;
}

/* Rx Write Edid */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxWriteEdid(pio AvPort *port, uint8 *Value, uint16 Count))
{
    AvRet ret = AvOk;
    uint8 RamSel = 0xFF;

    /* HDMI Rx uses Edid1, DP Rx uses Edid2 */
    if(port->type == HdmiRx)
        RamSel = 0;
    else
        ret = AvNotAvailable;

    if(RamSel < 2)
    {
        AvUapiOutputDebugMessage("Port[%d][%d] Writes EDID RAM%d, [255]=0x%x .", port->device->index, port->index, RamSel+1, Value[255]);
        /* Edid1 selection */
        GSV6K5_RXRPT_set_RX_EDID_RAM_SEL(port, RamSel);
        GSV6K5_RXRPT_set_RX_EDID_RAM_PAGE_SEL(port, 0);
        AvHalI2cWriteMultiField(GSV6K5_HDMI_EDID_ADDR(port), 0, 256, Value);
        if(Count > 256)
        {
            GSV6K5_RXRPT_set_RX_EDID_RAM_PAGE_SEL(port, 1);
            AvHalI2cWriteMultiField(GSV6K5_HDMI_EDID_ADDR(port), 0, 256, (Value+256));
        }
    }

    return ret;
}

/* Rx Write SPA */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxSetSpa(pio AvPort *port, uint8 SpaLocation, uint8 *SpaValue))
{
    AvRet ret = AvOk;

#if AvDontCareEdidSpa
    /* set SPA pointer to invalid 0x01 */
    GSV6K5_RXRPT_set_RX_EDID1_SPA_LOC_MSB(port, 0);
    GSV6K5_RXRPT_set_RX_EDID1_SPA_LOC_LSB(port, 1);
    GSV6K5_RXRPT_set_RX_EDID2_SPA_LOC_MSB(port, 0);
    GSV6K5_RXRPT_set_RX_EDID2_SPA_LOC_LSB(port, 1);
    /* set SPA data for invalid 0x01 to be 0xff/0xff */
    GSV6K5_RXRPT_set_RXB_SPA(port, 0xffff);
    GSV6K5_RXRPT_set_RXC_SPA(port, 0xffff);
    GSV6K5_RXRPT_set_RXD_SPA(port, 0xffff);
#else //(AvDontCareEdidSpa==0)
    uint8 value = 0;
    uint8 RamSel = 0;
    uint16 Spa = 0;
    uint8 SpaShift = 0;

    /* 1. Set SPA Location */
    /* using Edid1 */
    GSV6K5_RXRPT_set_RX_EDID1_SPA_LOC_LSB(port, SpaLocation);
    RamSel = 0;
    if(SpaLocation < 128)
    {
        GSV6K5_RXRPT_set_RX_EDID_RAM_SEL(port, RamSel);
        GSV6K5_RXRPT_set_RX_EDID_RAM_PAGE_SEL(port, 0);
        Spa = (SpaValue[0]<<8)+SpaValue[1];
        AvHalI2cWriteMultiField(GSV6K5_HDMI_EDID_ADDR(port), SpaLocation, 2, SpaValue);

        return ret;
    }
    /* 2. Generate all Rx Spa values */
    Spa = (SpaValue[0]<<8)+SpaValue[1];
    for(value=0;value<16;value=value+4)
    {
        if((Spa&0x000f) != 0)
        {
            SpaShift = value;
            Spa = Spa&0xfff0;
            break;
        }
        else
        {
            Spa = Spa>>4;
        }
    }
    /* 2.1 Wrong Spa Protection */
    if(value == 16)
        return AvError;

    /* 3. write SPA value */
    /* RxPort 0, write Edid Ram */
    GSV6K5_RXRPT_set_RX_EDID_RAM_SEL(port, RamSel);
    GSV6K5_RXRPT_set_RX_EDID_RAM_PAGE_SEL(port, 0);
    Spa = Spa + 1; /* e.g. XX10 */
    SpaValue[0] = (Spa<<SpaShift)>>8;
    SpaValue[1] = (Spa<<SpaShift)&0xff;
    AvHalI2cWriteMultiField(GSV6K5_HDMI_EDID_ADDR(port), SpaLocation, 2, SpaValue);

#endif
    return ret;
}

/* Tx Read Edid */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxReadEdid(pio AvPort *port, uint8 *Value, uint16 Count))
{
    AvRet ret = AvOk;
    uint8 ReadSuccess = 0;
    uint16 i = 0;
    uint8 CheckSum = 0;
    uint8 FsmState = 0;
    uint8 value = 0;

#if AvEdidStoredInRam
    /* Copy RAM into Edid Readback */
    if(port->content.tx->EdidReadSuccess == AV_EDID_UPDATED)
    {
        switch(port->index)
        {
            case 4:
                AvMemcpy(Value, TxAEdidRam(port), Count);
                AvUapiOutputDebugMessage("Port[%d][%d]:ReRead Edid From TxARam", port->device->index, port->index);
                return ret;
            case 5:
                AvMemcpy(Value, TxBEdidRam(port), Count);
                AvUapiOutputDebugMessage("Port[%d][%d]:ReRead Edid From TxBRam", port->device->index, port->index);
                return ret;
            case 6:
                AvMemcpy(Value, TxCEdidRam(port), Count);
                AvUapiOutputDebugMessage("Port[%d][%d]:ReRead Edid From TxCRam", port->device->index, port->index);
                return ret;
            case 7:
                AvMemcpy(Value, TxDEdidRam(port), Count);
                AvUapiOutputDebugMessage("Port[%d][%d]:ReRead Edid From TxDRam", port->device->index, port->index);
                return ret;
        }
    }
#endif

    /* protect EDID reread from Tx Port Reset */
    while((ReadSuccess == 0) && (i < 3000))
    {
        GSV6K5_TXPHY_get_TX_EDID_READY_RB(port, &ReadSuccess);
        if(ReadSuccess == 0)
        {
            GSV6K5_TXPHY_get_TX_HDCP1P4_STATE_RB(port,   &FsmState);
            if(FsmState == 1) /* DDC Reading EDID */
            {
                value = Gsv6k5TxDDCError(port);
                if(value == 1)
                {
                    AvUapiOutputDebugMessage("Port[%d][%d]:ReRead Edid Failure", port->device->index, port->index);
                    break;
                }
                else
                    i = i + 1;
            }
            else
                i = i + 1;
        }
    }
    /* Return if Edid read Failure or Illegal Edid */
    if(ReadSuccess == 1)
    {
#if (AvEdidStoredInRam == 1)
        GSV6K5_TXPHY_get_TX_DDC_READ_EDID_RAM_PART(port, &value);
        if(value != 0)
        {
            if(value >= (AvEdidMaxSize/256))
                value = (AvEdidMaxSize/256) - 1;
            if(value != 0)
            {
                if(value != ((AvEdidMaxSize/256)-1))
                    AvMemset(Value+(256*(value+1)), 0, AvEdidMaxSize-(256*(value+1)));
            }
            switch(port->index)
            {
                case 4:
                    AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxAEdidRam(port)+(256*value));
                    AvMemcpy(Value, TxAEdidRam(port), Count);
                    break;
                case 5:
                    AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxBEdidRam(port)+(256*value));
                    AvMemcpy(Value, TxBEdidRam(port), Count);
                    break;
                case 6:
                    AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxCEdidRam(port)+(256*value));
                    AvMemcpy(Value, TxCEdidRam(port), Count);
                    break;
                case 7:
                    AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, TxDEdidRam(port)+(256*value));
                    AvMemcpy(Value, TxDEdidRam(port), Count);
                    break;
            }
            Gsv6k5ToggleTxHpd(port,0);
        }
        else
        {
            AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, 256, Value);
        #if (AvEdidMaxSize == 512)
            AvMemset(Value+256, 0, AvEdidMaxSize-256);
        #endif
        }
        AvUapiOutputDebugMessage("Port[%d][%d]: Store Edid %d", port->device->index, port->index, value);
#else
        AvHalI2cReadMultiField(GSV6K5_TXEDID_MAP_ADDR(port), 0, Count, Value);
#endif
        ReadSuccess = 0;
        for(i=0;i<127;i++)
            CheckSum = CheckSum + Value[i];
        CheckSum = 256 - (CheckSum%256);
        if(CheckSum == Value[127])
        {
            if(Value[126] == 1)
            {
                CheckSum = 0;
                for(i=128;i<255;i++)
                    CheckSum = CheckSum + Value[i];
                CheckSum = 256 - (CheckSum%256);
                if(CheckSum == Value[255])
                    ReadSuccess = 1;
            }
            else
                ReadSuccess = 1;
        }
    }
    if(ReadSuccess == 0)
    {
        AvUapiOutputDebugMessage("Port[%d][%d]:Read Edid Failure", port->device->index, port->index);
        AvMemset(Value, 0, AvEdidMaxSize);
#if AvForceDefaultEdid
        port->content.tx->IgnoreEdidError = 1;
#else
        /* reread the Edid */
        Gsv6k5ToggleTxHpd(port, 1);
        port->content.tx->Hpd = AV_HPD_FORCE_LOW;
#endif
        return AvError;
    }

    if(port->content.tx->EdidReadSuccess != AV_EDID_UPDATED)
    {
#if AvEnableHdcp1p4BksvCheck
        if(port->content.hdcptx->HdcpSupport == 0)
        {
            /* Check Bskv for HDCP 1.4 Support Capability */
            GSV6K5_TXPHY_set_EDID_ANALYZE_DONE(port, 1);
            Gsv6k5ManualBksvRead(port);
            GSV6K5_TXPHY_set_EDID_ANALYZE_DONE(port, 0);
        }
#else
        AvUapiTxGetSinkHdcpCapability(port);
#endif
#if AvEdidStoredInRam
        #if (AvEdidMaxSize == 512)
        /* CEA block reorder */
        uint8 EdidBuffer[128];
        if(Value[0x7E] > 1)
        {
            if(Value[0x80] != 0x02)
            {
                if(Value[0x100] == 0x02)
                {
                    AvMemcpy(EdidBuffer,Value+0x80,128);
                    AvMemcpy(Value+0x80,Value+0x100,128);
                    AvMemcpy(Value+0x100,EdidBuffer,128);
                }
                else if((Value[0x180] == 0x02) && (Value[0x7E] == 3))
                {
                    AvMemcpy(EdidBuffer,Value+0x80,128);
                    AvMemcpy(Value+0x80,Value+0x180,128);
                    AvMemcpy(Value+0x180,EdidBuffer,128);
                }
            }
        }
        #endif
        /* Write Read Edid into RAM */
        switch(port->index)
        {
            case 4:
                AvMemcpy(TxAEdidRam(port), Value, AvEdidMaxSize);
                AvUapiOutputDebugMessage("Port[%d][%d]:Write Edid To TxARam", port->device->index, port->index);
                break;
            case 5:
                AvMemcpy(TxBEdidRam(port), Value, AvEdidMaxSize);
                AvUapiOutputDebugMessage("Port[%d][%d]:Write Edid To TxBRam", port->device->index, port->index);
                break;
            case 6:
                AvMemcpy(TxCEdidRam(port), Value, AvEdidMaxSize);
                AvUapiOutputDebugMessage("Port[%d][%d]:Write Edid To TxCRam", port->device->index, port->index);
                break;
            case 7:
                AvMemcpy(TxDEdidRam(port), Value, AvEdidMaxSize);
                AvUapiOutputDebugMessage("Port[%d][%d]:Write Edid To TxDRam", port->device->index, port->index);
                break;
        }
#endif
        /* Manually disable Edid Read Success, EDID will be read in Event */
        port->content.tx->EdidReadSuccess = AV_EDID_UPDATED;
    }
    else
    {
        ret = AvInvalidParameter;
    }

    return ret;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxEnableInternalEdid(pio AvPort *port))
{
    uint8 value;
    /* 1. enable internal Edid */
    GSV6K5_RXRPT_set_RX_EDID_CHECKSUM_RECALC(port, 1);
    /* 2. get edid ptr */
    if(port->type == HdmiRx)
        value = 0;
    else
        return AvInvalidParameter;

    /* 3. enable internal Edid */
    switch(port->index)
    {
        case 0: /* rx port 0 */
            GSV6K5_RXRPT_set_RXA_EDID_RAM_SEL(port, value);
            GSV6K5_RXRPT_set_RXA_EDID_EN(port, 1);
            break;
        default:
            break;
    }

    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxArcEnable(pin AvPort *port, uint8 value))
{
    if(port->type == HdmiTx)
        GSV6K5_PRIM_set_TXA_ARC_PWR_DN(port, 1-value);
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiRxHdcp2p2Manage(pin AvPort *port))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiTxHdcp2p2Manage(pin AvPort *port))
{
    return AvOk;
}

AvPort *Gsv6k5FindHdmiRxFront(pin AvPort *port)
{
    AvPort *FromPort = (AvPort *)port->content.RouteVideoFromPort;
    while(FromPort != NULL)
    {
#if AvEnableInternalVideoGen
        if((FromPort->type == HdmiRx) || (FromPort->type == VideoGen))
#else
        if(FromPort->type == HdmiRx)
#endif
            return FromPort;
        else
            FromPort = (AvPort *)FromPort->content.RouteVideoFromPort;
    }
    return NULL;
}

AvVideoCs Gsv6k5ColorCsMapping(AvVideoCs InputCs)
{
    AvVideoCs ReturnCs = InputCs;
    if(InputCs == AV_CS_YUV_709)
        ReturnCs = AV_CS_YCC_709;
    else if(InputCs == AV_CS_YUV_601)
        ReturnCs = AV_CS_YCC_601;
    else if(InputCs == AV_CS_LIM_YUV_709)
        ReturnCs = AV_CS_LIM_YCC_709;
    else if(InputCs == AV_CS_LIM_YUV_601)
        ReturnCs = AV_CS_LIM_YCC_601;
    else if(InputCs == AV_CS_DEFAULT_RGB)
        ReturnCs = AV_CS_RGB;
    else
        ReturnCs = InputCs;
    return ReturnCs;
}

void Gsv6k5CpCscManage(pin AvPort *port, AvVideoCs VarInCs, AvVideoCs VarOutCs)
{
    uint8 value1 = 0;
    uint8 NewValue1 = 0;
    uint8 CscBypassFlag = 1;
    AvPort *FromPort = (AvPort*)(port->content.RouteVideoFromPort);
    AvVideoCs InCs  = AV_CS_YUV_709;
    AvVideoCs OutCs = AV_CS_YUV_709;
    uint32 temp = 0;
    uchar *RegTable = (uchar *)CscYcc709ToRgb;
    uint8 CurrentCscTable[24];
    uint8 ConvSrc = 0;
    uint8 HdrValid = 0;

    InCs  = Gsv6k5ColorCsMapping(VarInCs);
    OutCs = Gsv6k5ColorCsMapping(VarOutCs);
    if(InCs != OutCs)
        CscBypassFlag = 0;
    else
        CscBypassFlag = 1;
    /* 1. Set CSC Value */
    /* 1.1 Find Correct Table */
    RegTable = (uchar *)CscRgbToYcc709;
    for(temp=0;CscTables[temp].ConvTable;temp++)
    {
        if((CscTables[temp].InCs  == InCs) &&
           (CscTables[temp].OutCs == OutCs))
        {
            RegTable = (uchar *)CscTables[temp].ConvTable;
            break;
        }
    }
#if AvEnableInternalVideoGen
    if(FromPort->type == VideoGen)
        RegTable = (uchar *)CscRgbFRtoYcc709FR;
#endif
    if((CscTables[temp].OutCs == AV_CS_AUTO) || (CscBypassFlag == 1))
    {
        RegTable = (uchar *)CscBypassParam;
    }
    else
    {
        ConvSrc = 2;
    }
    if((FromPort->type == HdmiRx) && (FromPort->content.video->Y == AV_Y2Y1Y0_RGB) &&
       ((VarInCs == AV_CS_LIM_BT2020_RGB) || (VarInCs == AV_CS_BT2020_DFT_RGB) || (VarInCs == AV_CS_BT2020_RGB)))
    {
        if(port->type == VideoColor)
        {
            if((port->content.color->Hdr2SdrEnable == 1) && (CscBypassFlag == 0))
            {
                RegTable = (uchar *)CscBt2020RgbtoYCbcr;
                ConvSrc = 2;
            }
        }
        else if(port->type == VideoScaler)
        {
            if((port->content.scaler->Hdr2SdrEnable == 1) && (CscBypassFlag == 0))
            {
                RegTable = (uchar *)CscBt2020RgbtoYCbcr;
                ConvSrc = 2;
            }
        }
    }
    /* 1.2 Write Table */
    GSV6K5_CP_get_CP_CSC_BYPASS(port, &value1);
    if(value1 == 0)
    {
        AvHalI2cReadMultiField(GSV6K5_VSP_MAP_ADDR(port), 0x20, 24, CurrentCscTable);
        NewValue1 = 0;
        for(temp=1;temp<23;temp++)
        {
            if(CurrentCscTable[temp] != RegTable[temp])
            {
                NewValue1 = 1;
                break;
            }
        }
        if(NewValue1 == 1)
        {
            AvHalI2cWriteMultiField(GSV6K5_VSP_MAP_ADDR(port), 0x20, 24, RegTable);
            value1 = (RegTable[0]>>5)&0x03;
            GSV6K5_VSP_set_CP_CSC_MODE(port, value1);
        }
    }
    /* 2. Check HDR2SDR configuration */
    if(CscBypassFlag == 0)
        HdrValid = Gsv6k5Hdr2Sdr(port);
    /* 3. Set Routing */
    /* CSC Only: CSC -> Conv, 0x02 */
    /* HDR(YCbCr): HDR->Dither->Conv, 0x07 */
    /* HDR(RGB): CSC->HDR->Dither->Conv, 0x87 */
    if((FromPort->content.video->AvailableVideoPackets & AV_BIT_HDR_PACKET) && (HdrValid == 1))
    {
        if(port->type == VideoColor)
        {
            if(port->content.color->Hdr2SdrEnable == 1)
            {
                if(port->content.color->ColorInSpace == AV_Y2Y1Y0_RGB)
                    ConvSrc = 0x87;
                else
                    ConvSrc = 0x07;
            }
        }
        else if(port->type == VideoScaler)
        {
            if(port->content.scaler->Hdr2SdrEnable == 1)
            {
                if(port->content.scaler->ColorSpace == AV_Y2Y1Y0_RGB)
                    ConvSrc = 0x87;
                else
                    ConvSrc = 0x07;
            }
        }
    }
    AvHalI2cReadField8(GSV6K5_CP_MAP_ADDR(port),0x01,0xFF,0,&value1);
    if(value1 != ConvSrc)
    {
        switch(ConvSrc)
        {
            case 0x02:
                NewValue1 = 0x50;
                break;
            case 0x07:
                NewValue1 = 0x20;
                break;
            case 0x87:
                NewValue1 = 0x00;
                break;
            default:
                NewValue1 = 0x70;
                break;
        }
        AvHalI2cWriteField8(GSV6K5_CP_MAP_ADDR(port),0x00,0xF0,0,NewValue1);
        AvHalI2cWriteField8(GSV6K5_CP_MAP_ADDR(port),0x01,0xFF,0,ConvSrc);
    }

}

uint8 Gsv6k5Hdr2Sdr(pin AvPort *port)
{
    AvVideoY Y = AV_Y2Y1Y0_RGB;
    AvVideoCs Cs = AV_CS_DEFAULT_RGB;
    AvPort *FromPort = (AvPort*)(port->content.RouteVideoFromPort);
    uint8 Content[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
    uint16 AverageLuma;
    uint8 value = 0;
    uint8 range = 0;
    uint8 offset = 0;
    uint32 cbratio = 0;
    uint32 divratio = 0;
    uint32 ysdrmax = 0;
    /* Port Judge */
    if(port->type == VideoColor)
    {
        Y = port->content.color->ColorInSpace;
        Cs = port->content.color->ColorOutCs;
        if(port->content.color->Hdr2SdrEnable == 0)
            return 0;
    }
    else if(port->type == VideoScaler)
    {
        Y = port->content.scaler->ColorSpace;
        Cs = port->content.scaler->ScalerOutCs;
        if(port->content.scaler->Hdr2SdrEnable == 0)
            return 0;
    }

    /* 1. Average Luma */
    if(FromPort->content.video->AvailableVideoPackets & AV_BIT_HDR_PACKET)
    {
        AvHalI2cReadMultiField(GSV6K5_RXINFO_MAP_ADDR(port), 0x02, 2, Content);
        if(((Content[0]&0xC0) != 0xC0) || (((Content[1]&0x70) != 0x50) && ((Content[1]&0x70) != 0x60)))
            return 0;
        AvHalI2cReadMultiField(GSV6K5_RXINFO2_MAP_ADDR(port), 0x39, 2, Content);
        AverageLuma = Content[0] + (Content[1] << 8);
        if(AverageLuma == 0)
        {
            AvHalI2cReadMultiField(GSV6K5_RXINFO2_MAP_ADDR(port), 0x33, 2, Content);
            AverageLuma = (Content[0] + (Content[1] << 8))>>1;
            if(AverageLuma == 0)
            {
                AvHalI2cReadMultiField(GSV6K5_RXINFO2_MAP_ADDR(port), 0x23, 6, Content);
                if((Content[0] != 0x00) || (Content[1] != 0x00) || (Content[2] != 0x00) ||
                   (Content[3] != 0x00) || (Content[4] != 0x00) || (Content[5] != 0x00))
                    AverageLuma = 799;
                else
                {
                    AvHalI2cReadField8(GSV6K5_RXINFO2_MAP_ADDR(port),0x21,0xFF,0,&value);
                    if(value == 0x02)
                        AverageLuma = 1000;
                    else
                        return 0;
                }
            }
        }
    }
    if(AverageLuma == 0)
        AverageLuma = 100;
    GSV6K5_CP_set_CP_HDR2SDR_HDR_AVERAGE_LUMA(port, AverageLuma);
    if(AverageLuma > 711)
        cbratio = 0x1A85;
    else
        cbratio = 0x1385;
    GSV6K5_CP_set_CP_HDR2SDR_CHROMA_CB_RATIO(port, cbratio);
    /* 2. Format */
    switch(Y)
    {
        case AV_Y2Y1Y0_RGB:
        case AV_Y2Y1Y0_YCBCR_444:
        case AV_Y2Y1Y0_YCBCR_422:
            value = 0;
            break;
        case AV_Y2Y1Y0_YCBCR_420:
            value = 2;
            break;
    }
    GSV6K5_CP_set_CP_HDR2SDR_FORMAT(port, value);
    /* 3. Color Range */
    value = 1;
    if(Y == AV_Y2Y1Y0_RGB)
    {
        /* Limited RGB */
        if(Cs == AV_CS_LIM_RGB)
            value = 0;
    }
    /* Limited YCbCr */
    else if((Cs & 0x40) == 0)
        value = 0;
    GSV6K5_CP_set_CP_HDR2SDR_219_255_SEL(port, value);
    if(value == 0)
    {
        range = 219;
        offset = 16;
    }
    else
    {
        range = 255;
        offset = 0;
    }
    divratio = 1 + (16384/range);
    ysdrmax = ((255 - offset)*4095)/255;
    GSV6K5_CP_set_CP_HDR2SDR_ONE_DIV_219(port, divratio);
    GSV6K5_CP_set_CP_HDR2SDR_Y_SDR_MAX(port, ysdrmax);
    GSV6K5_CP_set_CP_HDR2SDR_SDR_LUMA_RANGE(port, range);
    GSV6K5_CP_set_CP_HDR2SDR_SDR_LUMA_OFFSET(port, offset);
    /* 4. Input channel swap */
    /*
    GSV6K5_CP_set_CP_HDR2SDR_IN_CH_SWAP(port, 0);
    GSV6K5_CP_set_CP_HDR2SDR_CH1_422_SEL(port, 0);
    GSV6K5_CP_set_CP_HDR2SDR_CH3_420_SEL(port, 0);
    GSV6K5_CP_set_CP_HDR2SDR_CH3_T422_SEL(port, 0);
    GSV6K5_CP_set_CP_HDR2SDR_CH3_422_SEL(port, 0);
    */
    /* 5. Output channel swap */
    /*
    GSV6K5_CP_set_CP_HDR2SDR_OUT_CH_SWAP(port, 0);
    */
    return 1;
}

void Gsv6k5ScaleRatio(pin AvPort *port, uint8 hparam, uint8 hratio, uint8 vparam, uint8 vratio, uint8 force)
{
    uint8 temp = 0;
    uint8 flag = force;

    /* Horizontal Ratio */
    GSV6K5_CP_get_CP_DOWN_SCALER_H_FAC(port, &temp);
    if(hratio != temp)
    {
        GSV6K5_CP_set_CP_DOWN_SCALER_H_FAC(port, hratio);
        flag = 1;
    }

    GSV6K5_CP_get_CP_SCALER_VOUT_H_PARAM_SEL(port, &temp);
    if(hparam != temp)
    {
        GSV6K5_CP_set_CP_SCALER_VOUT_H_PARAM_SEL(port, hparam);
        flag = 1;
    }

    /* vertical Ratio */
    GSV6K5_CP_get_CP_DOWN_SCALER_V_FAC(port, &temp);
    if(vratio != temp)
    {
        GSV6K5_CP_set_CP_DOWN_SCALER_V_FAC(port, vratio);
        flag = 1;
    }

    GSV6K5_CP_get_CP_SCALER_VOUT_V_PARAM_SEL(port, &temp);
    if(vparam != temp)
    {
        GSV6K5_CP_set_CP_SCALER_VOUT_V_PARAM_SEL(port, vparam);
        flag = 1;
    }

    if(flag == 1)
    {
        GSV6K5_CP_get_CP_OUT_QUAD_EN(port, &temp);
        Gsv6k5CpProtect(port,temp);
    }
}

void Gsv6k5Thumbnail(pin AvPort *port, uint16 InH, uint16 InV, uint16 OutH, uint16 OutV)
{
}

void Gsv6k5ManualCpParameter(AvPort *port, AvPort *FromPort)
{
    uint32 OldParam = 0;
    uint32 temp = 0;
    uint8  Y420Flag = 0;
    uint8  RecalcFlag = 0;

    if((FromPort->type == HdmiRx) && (FromPort->core.HdmiCore != -1))
    {
        /* YCbCr 420 mode detection */
        AvHalI2cReadField8(GSV6K5_RXAUD_MAP_ADDR(FromPort),0x19,0x40,0,&Y420Flag);
        if(Y420Flag == 0x40)
        {
            AvHalI2cReadField8(GSV6K5_RXINFO_MAP_ADDR(FromPort),0x01,0xE0,0,&Y420Flag);
            if((Y420Flag == 0x60) && (port->type == VideoScaler))
                Y420Flag = 1;
            else
                Y420Flag = 0;
        }
        else
            Y420Flag = 0;
        temp = FromPort->content.video->timing.HActive << Y420Flag;
        GSV6K5_VSP_get_VIN_H_ACTIVE(port, &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_H_ACTIVE(port, temp);
        }
        temp = FromPort->content.video->timing.HTotal << Y420Flag;
        GSV6K5_VSP_get_VIN_H_TOTAL(port,  &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_H_TOTAL(port,  temp);
        }
        temp = FromPort->content.video->timing.HFront << Y420Flag;
        GSV6K5_VSP_get_VIN_H_FRONT(port,  &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_H_FRONT(port,  temp);
        }
        temp = FromPort->content.video->timing.HSync << Y420Flag;
        GSV6K5_VSP_get_VIN_H_SYNC(port,   &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_H_SYNC(port,   temp);
        }
        temp = FromPort->content.video->timing.HBack << Y420Flag;
        GSV6K5_VSP_get_VIN_H_BACK(port,   &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_H_BACK(port,   temp);
        }
        temp = FromPort->content.video->timing.VTotal;
        GSV6K5_VSP_get_VIN_V_TOTAL(port,  &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_V_TOTAL(port,  temp);
        }
        temp = FromPort->content.video->timing.VActive;
        GSV6K5_VSP_get_VIN_V_ACTIVE(port, &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_V_ACTIVE(port, temp);
        }
        temp = FromPort->content.video->timing.VFront;
        GSV6K5_VSP_get_VIN_V_FRONT(port,  &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_V_FRONT(port,  temp);
        }
        temp = FromPort->content.video->timing.VSync;
        GSV6K5_VSP_get_VIN_V_SYNC(port,   &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_V_SYNC(port,   temp);
        }
        temp = FromPort->content.video->timing.VBack;
        GSV6K5_VSP_get_VIN_V_BACK(port,   &OldParam);
        if(OldParam != temp)
        {
            RecalcFlag = 1;
            GSV6K5_VSP_set_VIN_V_BACK(port,   temp);
        }
        if(RecalcFlag == 1)
        {
            GSV6K5_CP_set_CP_TG_CAL_START_MAN(port, 1);
            GSV6K5_CP_set_CP_TG_CAL_START_MAN(port, 0);
        }
    }
    return;
}

void Gsv6k5TxPllUnlockClear(AvPort *port)
{
    switch(port->index)
    {
        case 4:
            GSV6K5_PLL_set_TXA_PLL_LOCK_CLEAR(port, 1);
            GSV6K5_PLL_set_TXA_PLL_LOCK_CLEAR(port, 0);
            break;
        case 5:
            GSV6K5_PLL_set_TXB_PLL_LOCK_CLEAR(port, 1);
            GSV6K5_PLL_set_TXB_PLL_LOCK_CLEAR(port, 0);
            break;
        case 6:
            GSV6K5_PLL2_set_TXC_PLL_LOCK_CLEAR(port, 1);
            GSV6K5_PLL2_set_TXC_PLL_LOCK_CLEAR(port, 0);
            break;
        case 7:
            GSV6K5_PLL2_set_TXD_PLL_LOCK_CLEAR(port, 1);
            GSV6K5_PLL2_set_TXD_PLL_LOCK_CLEAR(port, 0);
            break;
    }
}

uint8 Gsv6k5TweakCrystalFreq(AvPort *port, uint32 value32)
{
    uint8 CrystalFreq = 0;
    /* Check Frequency with 0xff/0x00 end */
    uint8 value = value32 & 0xff;
    if((value == 0x00) || (value == 0xff))
    {
        /* Change Crystal Frequency */
        AvHalI2cReadField8(GSV6K5_PLL_MAP_ADDR(port),0x01,0xFF,0,&CrystalFreq);
        if(CrystalFreq == 0x00)
            AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),0x01,0xFF,0,0x0F);
        else
            AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),0x01,0xFF,0,0x00);
        return 1;
    }
    return 0;
}

uint32 Gsv6k5ReadPllFreq(AvPort *port)
{
    uint32 value32 = 0;
    uint8  RbData[6];

    AvHalI2cReadMultiField(GSV6K5_PLL_MAP_ADDR(port), 0x2C, 6, RbData);
    RbData[0] = RbData[0]&0x1F;
    if(RbData[0] == 0)
        RbData[0] = 1;
    value32 = ((RbData[4]<<8)+RbData[5])/RbData[0];
    value32 = value32 * ((RbData[1]<<8) + RbData[2]);
    value32 = value32/((1<<RbData[3]) * 5);

    return value32;
}

void Gsv6k5SetPacketBypass(AvPort *port)
{
#if (Gsv6k5MuxMode == 0)
    uint32 MapAddress = GSV6K5_PRIM_MAP_ADDR(port) | gsv6k5Dev_Rx1AudMapAddress;
    uint8 setting1 = 0;
    uint8 setting2 = 0;
    /* Audio Bypass Function */
#if Gsv6k5AudioBypass
    AvHalI2cWriteField8(MapAddress,0x8D,0x01,0,0x01);
#endif
    /* Rx/Tx Core Packet Bypass setting */
    #if Gsv6k5EmpPktBypass
    setting1 = setting1 | 0x01;
    #endif
    #if Gsv6k5VsInfoBypass
    setting1 = setting1 | 0x02;
    #endif //Gsv6k5VsInfoBypass
    #if Gsv6k5AviInfoBypass
    setting1 = setting1 | 0x04;
    #endif //Gsv6k5AviInfoBypass
    #if Gsv6k5SpdInfoBypass
    setting1 = setting1 | 0x08;
    #endif //Gsv6k5SpdInfoBypass
    #if Gsv6k5AudioInfoBypass
    setting1 = setting1 | 0x10;
    #endif //Gsv6k5AudioInfoBypass
    #if Gsv6k5MpegInfoBypass
    setting1 = setting1 | 0x20;
    #endif //Gsv6k5MpegInfoBypass
    #if Gsv6k5HdrInfoBypass
    setting1 = setting2 | 0x80;
    #endif //Gsv6k5HdrInfoBypass
    #if Gsv6k5AcpPktBypass
    setting2 = setting2 | 0x04;
    #endif //Gsv6k5AcpPktBypass
    #if Gsv6k5IsrcPktBypass
    setting2 = setting2 | 0x18;
    #endif //Gsv6k5IsrcPktBypass
    #if Gsv6k5GmdPktBypass
    setting2 = setting2 | 0x20;
    #endif //Gsv6k5GmdPktBypass
    /* Tx Digital Core Packet Bypass Setting */
    AvHalI2cWriteField8(MapAddress,0x8F,0xFF,0,setting1);
    AvHalI2cWriteField8(MapAddress,0x8E,0xFF,0,setting2);
#endif
}

void Gsv6k5SetRxH2p1Mode(AvPort *port, uint8 enable)
{
    uint8 value = 0;
    /* Option 1: follow HDMI Rx setting, Rx uses single as default */
    /* RxDIG.0xFF = 0x00 */
    /* Option 2: force HDMI Tx setting */
    if(port->core.HdmiCore == 0)
    {
        AvHalI2cReadField8(GSV6K5_RXDIG_MAP_ADDR(port), 0xFC, 0x1, 0x0, &value);
        if(value == 0)
        {
            AvHalI2cReadField8(GSV6K5_RXDIG_MAP_ADDR(port), 0xFF, 0x1, 0x0, &value);
            if(enable != value)
            {
                AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port), 0xFF, 0x1, 0x0, enable);
                AvUapiOutputDebugMessage("Port[%d][%d]:H2p1=%x set", port->device->index, port->index, enable);
                /*
                if(port->content.rx->IsInputStable == 1)
                {
                    if(port->content.hdcp->Hdcp2p2RxRunning == 1)
                        port->content.rx->Hpd = AV_HPD_TOGGLE;
                    else
                    {
                        port->content.rx->Lock.EqLock = 0;
                        Gsv6k5RxManualEQUpdate(port, 1);
                    }
                }
                */
            }
        }
    }
}

uint8 Gsv6k5SetTxH2p1Mode(AvPort *port)
{
    uint8  data  = 0;
    uint8  value = 0;
    uint8  flag  = 1;
    uint8  enable = 0;
#if AvEnableTxSingleLaneMode
    uint32 data32 = 0;
    uint32 value32 = 0;
#endif
    AvPort *FromPort = (AvPort*)port->content.RouteVideoFromPort;

    if(FromPort != NULL)
    {
        /* HDMI 2.1 -> HDMI 2.0 mode configuration */
        if((FromPort->type == HdmiRx) &&
           ((FromPort->content.rx->H2p1FrlType != AV_FRL_NONE) || (FromPort->content.rx->H2p1FrlManual != 0)) &&
           (port->content.tx->H2p1FrlManual == 0) &&
           (port->content.tx->H2p1FrlType == AV_FRL_NONE))
            value = 1;
        else
            value = 0;
        GSV6K5_TXPHY_set_TX_PORT_USCP_SEL(port, value);

        /* H2p1 mode judge */
        if((port->content.tx->H2p1FrlType != AV_FRL_NONE) || (port->content.tx->H2p1FrlManual != 0))
            enable = 1;
        else
            enable = 0;
        if((FromPort->type == VideoScaler) || (FromPort->type == VideoColor))
        {
            GSV6K5_CP_get_CP_OUT_QUAD_EN(FromPort, &value);
            if(value != enable)
            {
                GSV6K5_CP_set_CP_OUT_QUAD_EN(FromPort, enable);
                Gsv6k5CpProtect(FromPort, enable);
                GSV6K5_CP_set_CP_TG_CAL_START_MAN(FromPort, 1);
                GSV6K5_CP_set_CP_TG_CAL_START_MAN(FromPort, 0);
            }
        }
    }

    /* FRL mode setting when ready to transmit */
#if AvEnableTxSingleLaneMode
    if(port->content.tx->H2p1FrlManual == 0)
    {
        value = (uint8)(port->content.tx->H2p1FrlType);
        GSV6K5_TXFRL_get_FRL_RATE(port, &data);
        if(data != value)
        {
            GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 1);
            GSV6K5_TXFRL_set_FRL_SKIP_TRAINING(port, 0);
            GSV6K5_TXFRL_set_FRL_CTRL_MAN_EN(port, 0);
            GSV6K5_TXFRL_set_FRL_RATE(port, value);
            GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 0);
            if(port->core.HdmiCore != -1)
            {
                if((port->content.audio->AvailableAudioPackets & AV_BIT_ACR_PACKET) != 0)
                    port->content.audio->AvailableAudioPackets = port->content.audio->AvailableAudioPackets & (~AV_BIT_ACR_PACKET);
            }
            flag = 0;
            AvUapiOutputDebugMessage("Port[%d][%d]:Rate %d->%d", port->device->index, port->index,data,value);
        }
    }
    else
    {
        GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 1);
        GSV6K5_TXFRL_set_FRL_SKIP_TRAINING(port, 1);
        GSV6K5_TXFRL_get_FRL_DATA_RATE(port, &data32);
        value32 = (port->content.tx->H2p1FrlManual)&0xffff;
        if(data32 != value32)
        {
            GSV6K5_TXFRL_set_FRL_DATA_RATE(port, value32);
            flag = 0;
        }
        GSV6K5_TXFRL_get_FRL_LANE_NUM(port, &data);
        value = ((port->content.tx->H2p1FrlManual)>>16)&0xf;
        if(data != value)
        {
            AvUapiOutputDebugMessage("Port[%d][%d]:Tx Manual FRL 0x%x", port->device->index, port->index,value);
            GSV6K5_TXFRL_set_FRL_LANE_NUM(port, value);
        }
        GSV6K5_TXFRL_set_FRL_CTRL_MAN_EN(port, 1);
        GSV6K5_TXFRL_set_FRL_MODE_EN(port, 1);
    }
#else
    value = (uint8)(port->content.tx->H2p1FrlType);
    GSV6K5_TXFRL_get_FRL_RATE(port, &data);
    if(data != value)
    {
        GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 1);
        GSV6K5_TXFRL_set_FRL_RATE(port, value);
        if(port->core.HdmiCore != -1)
        {
            if((port->content.audio->AvailableAudioPackets & AV_BIT_ACR_PACKET) != 0)
                port->content.audio->AvailableAudioPackets = port->content.audio->AvailableAudioPackets & (~AV_BIT_ACR_PACKET);
        }
        flag = 0;
        AvUapiOutputDebugMessage("Port[%d][%d]:Rate %d->%d", port->device->index, port->index,data,value);
    }
#endif
    return flag;
}

void Gsv6k5TxFrlStart(AvPort *port)
{
    AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x19,0xFF,0,0x04);
    AvHalI2cWriteField8(GSV6K5_TXFRL_MAP_ADDR(port),0x10,0xFF,0,0x30);
    GSV6K5_TXFRL_set_FRL_MAN_TX_SEND_GAP_EN(port, 0);
    GSV6K5_TXFRL_set_FRL_MAN_TX_SEND_VID_EN(port, 0);
    GSV6K5_TXFRL_set_FRL_SKIP_ALL_DDC(port, 0);
    GSV6K5_TXPHY_set_SCDC_TMDS_CFGED(port, 1);
    /* FRL Tx Fifo Reset for smooth switch */
    GSV6K5_TXFRL_set_FRL_TX_FIFO_MANUAL_RST(port, 1);
    GSV6K5_TXFRL_set_FRL_TX_FIFO_MANUAL_RST(port, 0);
    port->content.tx->H2p1FrlStat = AV_FRL_TRAIN_ONGOING;
}

void Gsv6k5MpllProtect(AvPort *port)
{
    uint32 value32 = 0;
    uint8 i = 0;
    uint8 ValidMpllFreq = 0;
    /* Delay and read MPLL measure state */
    while(ValidMpllFreq == 0)
    {
        /* Step 1. Measure MPLL Frequency */
        GSV6K5_PLL3_get_RB_MPLL_PLL_VCO_POST_DIV_FREQ(port, &value32);
        /* Step 2. MPLL Frequency Recovery */
        if((value32 < 3000) || (value32 > 3400))
        {
            GSV6K5_SEC_set_DPLL_PDN(port, 1);
            for(i=0;i<15;i++)
                GSV6K5_SEC_set_DPLL_PDN(port, 0);
        }
        else
            ValidMpllFreq = 1;
    }
    /* OTP Key Protection */
    GSV6K5_SEC_set_O2E_DEBUG_SEL(port, 0);
    GSV6K5_SEC_get_RB_O2E_STATE(port, &value32);
    if(value32 != 1)
    {
        GSV6K5_PRIM_set_54M_DOMAIN_RST(port, 1);
        GSV6K5_PRIM_set_54M_DOMAIN_RST(port, 0);
    }
    else
    {
        GSV6K5_SEC_set_O2E_DEBUG_SEL(port, 5);
        GSV6K5_SEC_get_RB_O2E_STATE(port, &value32);
        if(value32 != 1)
        {
            GSV6K5_PRIM_set_54M_DOMAIN_RST(port, 1);
            GSV6K5_PRIM_set_54M_DOMAIN_RST(port, 0);
        }
    }
}

void Gsv6k5RpllProtect(AvPort *port)
{
    uint32 PllRb1 = 0;
    uint32 PllRb2 = 0;
    uint32 PllDiff = 0;
    /* 1. Read Back Comparison Value */
    switch(port->index)
    {
        case 0:
            GSV6K5_PLL_get_RB_RXA_PLL_REF_PRE_DIV_FREQ(port,  &PllRb1);
            GSV6K5_PLL_get_RB_RXA_PLL_VCO_POST_DIV_FREQ(port, &PllRb2);
            break;
        case 4:
            GSV6K5_PLL_get_RB_TXA_PLL_REF_PRE_DIV_FREQ(port,  &PllRb1);
            GSV6K5_PLL_get_RB_TXA_PLL_VCO_POST_DIV_FREQ(port, &PllRb2);
            break;
        case 5:
            GSV6K5_PLL_get_RB_TXB_PLL_REF_PRE_DIV_FREQ(port,  &PllRb1);
            GSV6K5_PLL_get_RB_TXB_PLL_VCO_POST_DIV_FREQ(port, &PllRb2);
            break;
        case 6:
            GSV6K5_PLL2_get_RB_TXC_PLL_REF_PRE_DIV_FREQ(port,  &PllRb1);
            GSV6K5_PLL2_get_RB_TXC_PLL_VCO_POST_DIV_FREQ(port, &PllRb2);
            break;
        case 7:
            GSV6K5_PLL2_get_RB_TXD_PLL_REF_PRE_DIV_FREQ(port,  &PllRb1);
            GSV6K5_PLL2_get_RB_TXD_PLL_VCO_POST_DIV_FREQ(port, &PllRb2);
            break;
    }
    /* 2. Compare Diff */
    if(PllRb1 > PllRb2)
        PllDiff = PllRb1 - PllRb2;
    else
        PllDiff = PllRb2 - PllRb1;
    /* 3. Diff Action */
    if((PllDiff>10) || (PllRb2 == 0))
    {
        switch(port->index)
        {
            case 0:
                GSV6K5_PLL_set_RXA_PLL_ENABLE_MAN_EN(port, 1);
                GSV6K5_PLL_set_RXA_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL_set_RXA_PLL_ENABLE_MAN(port, 1);
                GSV6K5_PLL_set_RXA_PLL_DIV_CALC_MAN_START(port, 1);
                GSV6K5_PLL_set_RXA_PLL_DIV_CALC_MAN_START(port, 0);
                break;
            case 4:
                GSV6K5_PLL_set_TXA_PLL_ENABLE_MAN_EN(port, 1);
                GSV6K5_PLL_set_TXA_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL_set_TXA_PLL_ENABLE_MAN(port, 1);
                GSV6K5_PLL_set_TXA_PLL_ENABLE_MAN_EN(port, 0);
                GSV6K5_PLL_set_TXA_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL_set_TXA_PLL_DIV_CALC_MAN_START(port, 1);
                GSV6K5_PLL_set_TXA_PLL_DIV_CALC_MAN_START(port, 0);
                break;
            case 5:
                GSV6K5_PLL_set_TXB_PLL_ENABLE_MAN_EN(port, 1);
                GSV6K5_PLL_set_TXB_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL_set_TXB_PLL_ENABLE_MAN(port, 1);
                GSV6K5_PLL_set_TXB_PLL_ENABLE_MAN_EN(port, 0);
                GSV6K5_PLL_set_TXB_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL_set_TXB_PLL_DIV_CALC_MAN_START(port, 1);
                GSV6K5_PLL_set_TXB_PLL_DIV_CALC_MAN_START(port, 0);
                break;
            case 6:
                GSV6K5_PLL2_set_TXC_PLL_ENABLE_MAN_EN(port, 1);
                GSV6K5_PLL2_set_TXC_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL2_set_TXC_PLL_ENABLE_MAN(port, 1);
                GSV6K5_PLL2_set_TXC_PLL_ENABLE_MAN_EN(port, 0);
                GSV6K5_PLL2_set_TXC_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL2_set_TXC_PLL_DIV_CALC_MAN_START(port, 1);
                GSV6K5_PLL2_set_TXC_PLL_DIV_CALC_MAN_START(port, 0);
                break;
            case 7:
                GSV6K5_PLL2_set_TXD_PLL_ENABLE_MAN_EN(port, 1);
                GSV6K5_PLL2_set_TXD_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL2_set_TXD_PLL_ENABLE_MAN(port, 1);
                GSV6K5_PLL2_set_TXD_PLL_ENABLE_MAN_EN(port, 0);
                GSV6K5_PLL2_set_TXD_PLL_ENABLE_MAN(port, 0);
                GSV6K5_PLL2_set_TXD_PLL_DIV_CALC_MAN_START(port, 1);
                GSV6K5_PLL2_set_TXD_PLL_DIV_CALC_MAN_START(port, 0);
                break;
        }
    }
}

uint8 Gsv6k5TxPllProtect(AvPort *port, AvPort *UpperPort, AvPort *FromPort)
{
    uint8  value = 0;
    uint8  NewValue = 0;
    uint32 value32 = 0;
    uint8  RegAddr[2] = {0x00,0x00};
    uint8  RegData[4] = {0x00,0x00,0x00,0x00};
    AvPort *CurrentPort = port->device->port;
    uint8 H1p4MuxModeMatch = 0;
    uint8 H1p4TxCoreMatch = 0;

    /* Check HDCP Encryption */
    while((CurrentPort != NULL) && (CurrentPort->device->index == port->device->index))
    {
        if(CurrentPort->type == HdmiTx)
        {
            if((CurrentPort->content.hdcptx->Authenticated == 1) &&
               (CurrentPort->content.hdcptx->Hdcp2p2TxRunning == 0))
            {
                if(CurrentPort->core.HdmiCore == 0)
                    H1p4TxCoreMatch = 1;
                else
                    H1p4MuxModeMatch = 1;
            }
        }
        CurrentPort = (AvPort*)(CurrentPort->next);
    }

    /* 1. Protection on Audio */
    if(port->core.HdmiCore == -1)
    {
        switch(port->index)
        {
            case 4:
                RegAddr[0] = 0xE5;
                RegAddr[1] = 0x7F;
                break;
            case 5:
                RegAddr[0] = 0xE6;
                RegAddr[1] = 0xAF;
                break;
            case 6:
                RegAddr[0] = 0xE7;
                RegAddr[1] = 0x7F;
                break;
            case 7:
                RegAddr[0] = 0xE8;
                RegAddr[1] = 0xAF;
                break;
        }
        if(UpperPort->type == HdmiRx)
        {
            if(UpperPort->content.rx->HdmiMode == 0)
                value = 1;
            else
                value = 3;
            NewValue = 0x00;
            if((FromPort->type == HdmiRx) && (value == 3))
            {
                GSV6K5_RXDIG_set_RX_AUDIO_MCLK_X_N(FromPort, 0);
                value32 = Gsv6k5TuneTxAcr(port,FromPort->content.audio->NValue);
                port->content.audio->NValue = value32;
                GSV6K5_SEC_set_MUX_MODE_N_VALUE(port, value32);
                if(((port->content.hdcptx->HdcpEnabled == 0) && (FromPort->content.video->DscStream == 1)) ||
                   (port->content.tx->H2p1FrlManual != 0))
                {
                    value32 = FromPort->content.video->timing.HTotal - FromPort->content.video->timing.HActive;
                    if(((value32 >= 76) && (value32 <= 85)) || ((value32 >= 108) && (value32 <= 117)))
                    {
                        RegData[0] = 0x02;
                        RegData[1] = 0x00;
                        RegData[2] = 0x81;
                    }
                    else if(value32 > 123)
                    {
                        RegData[0] = 0x02;
                        RegData[1] = 0x03;
                        RegData[2] = 0x0F;
                    }
                    else
                    {
                        RegData[0] = 0x00;
                        RegData[1] = 0x00;
                        RegData[2] = 0x81;
                    }
                }
                else
                {
                    RegData[0] = 0x09;
                    RegData[1] = 0x03;
                    RegData[2] = 0x0F;
                }
                if(((port->core.HdmiCore == 0)  && (H1p4TxCoreMatch == 1)) ||
                   ((port->core.HdmiCore == -1) && (H1p4MuxModeMatch == 1)))
                    RegData[0] = 0x0E; /* 0x0E for HDCP 1.4, 0x09 for HDCP 2.2 */
                GSV6K5_SEC_set_MUX_MODE_BLANK_AFTER_DE_MAN(port, RegData[0]);
                GSV6K5_SEC_set_MUX_MODE_H_BLANK_PKT_INSERT_MARGIN(port, RegData[1]);
                GSV6K5_SEC_set_MUX_MODE_V_BLANK_PKT_INSERT_MARGIN(port, RegData[2]);
                /* Low Frequency Timing Audio Direct Bypass */
                if((FromPort->content.rx->H2p1FrlType == AV_FRL_NONE) && (port->content.tx->H2p1FrlType == AV_FRL_NONE) &&
                   (FromPort->content.rx->H2p1FrlManual == 0) && (port->content.tx->H2p1FrlManual == 0))
                {
                    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port), 0x7C, 0x03, 0x0, 0x00);
                    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port), 0x6E, 0xFF, 0x0, 0x00);
                }
                else
                {
                    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port), 0x7C, 0x03, 0x0, 0x03);
                    AvHalI2cWriteField8(GSV6K5_SEC_MAP_ADDR(port), 0x6E, 0xFF, 0x0, 0x01);
                }
            }
        }
        else
        {
            value = 1;
            if(port->content.tx->H2p1FrlType == AV_FRL_NONE)
            {
                NewValue = 0x00;
                value = 3;
            }
            else
            {
                NewValue = 0x86;
                if(port->content.tx->H2p1FrlType == AV_FRL_3G3L)
                {
                    RegData[0] = 0x88;
                    RegData[1] = 0x00;
                    RegData[2] = 0x90;
                    RegData[3] = 0x01;
                }
                else
                {
                    RegData[0] = 0x90;
                    RegData[1] = 0x00;
                    RegData[2] = 0x90;
                    RegData[3] = 0x00;
                }
                if((port->index == 4) || (port->index == 5))
                    AvHalI2cWriteMultiField(GSV6K5_PLL_MAP_ADDR(port), RegAddr[1],4,RegData+0);
                else
                    AvHalI2cWriteMultiField(GSV6K5_PLL2_MAP_ADDR(port),RegAddr[1],4,RegData+0);
            }
        }
        AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),RegAddr[0],0xFF,0,NewValue);
        switch(port->index)
        {
            case 4:
                if(NewValue == 0x00)
                {
                    AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),RegAddr[1],0xFF,0,0x00);
                    GSV6K5_PLL_set_TXA_PLL_DIV_CALC_MAN_START(port, 1);
                    GSV6K5_PLL_set_TXA_PLL_DIV_CALC_MAN_START(port, 0);
                }
                GSV6K5_SEC_set_TXPORT_A_SRC_SEL(port, value);
                break;
            case 5:
                if(NewValue == 0x00)
                {
                    AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),RegAddr[1],0xFF,0,0x00);
                    GSV6K5_PLL_set_TXB_PLL_DIV_CALC_MAN_START(port, 1);
                    GSV6K5_PLL_set_TXB_PLL_DIV_CALC_MAN_START(port, 0);
                }
                GSV6K5_SEC_set_TXPORT_B_SRC_SEL(port, value);
                break;
            case 6:
                if(NewValue == 0x00)
                {
                    AvHalI2cWriteField8(GSV6K5_PLL2_MAP_ADDR(port),RegAddr[1],0xFF,0,0x00);
                    GSV6K5_PLL2_set_TXC_PLL_DIV_CALC_MAN_START(port, 1);
                    GSV6K5_PLL2_set_TXC_PLL_DIV_CALC_MAN_START(port, 0);
                }
                GSV6K5_SEC_set_TXPORT_C_SRC_SEL(port, value);
                break;
            case 7:
                if(NewValue == 0x00)
                {
                    AvHalI2cWriteField8(GSV6K5_PLL2_MAP_ADDR(port),RegAddr[1],0xFF,0,0x00);
                    GSV6K5_PLL2_set_TXD_PLL_DIV_CALC_MAN_START(port, 1);
                    GSV6K5_PLL2_set_TXD_PLL_DIV_CALC_MAN_START(port, 0);
                }
                GSV6K5_SEC_set_TXPORT_D_SRC_SEL(port, value);
                break;
        }
        Gsv6k5ResetTxFifo(port);
        if(value == 3)
            NewValue = 1;
        else
            NewValue = 0;
        GSV6K5_TXPHY_set_TX_PORT_USCP_SEL(port, NewValue);
    }
    /* 2. Protection on CP output clock */
    if((port->core.HdmiCore == 0) && (UpperPort != NULL))
    {
        switch(port->index)
        {
            case 4:
                GSV6K5_SEC_set_TXPORT_A_SRC_SEL(port, 0);
                break;
            case 5:
                GSV6K5_SEC_set_TXPORT_B_SRC_SEL(port, 0);
                break;
            case 6:
                GSV6K5_SEC_set_TXPORT_C_SRC_SEL(port, 0);
                break;
            case 7:
                GSV6K5_SEC_set_TXPORT_D_SRC_SEL(port, 0);
                break;
        }
        AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),0xD0,0xFF,0,0x28);
        /* 2.1 Get Tx Mode */
        value = 0;
        if((UpperPort->type == VideoScaler) || (UpperPort->type == VideoColor))
            GSV6K5_CP_get_CP_OUT_QUAD_EN(UpperPort, &value);
        else if(UpperPort->type == HdmiRx)
            GSV6K5_RXFRL_get_RB_FRL_RX_TMDS_CLK_DIV4_EN(UpperPort, &value);
        /* 2.2 Calc mode setting */
        NewValue = value + 0x0A;
        AvHalI2cReadField8(GSV6K5_APLL_MAP_ADDR(port),0x44,0xFF,0,&RegData[3]);
        /* 2.3 Frequency Detection */
        AvHalI2cReadMultiField(GSV6K5_PRIM_MAP_ADDR(port),0xD3,3,RegData);
        if(((RegData[0] == 0x00) && (RegData[1] == 0x00) && (RegData[2] == 0x00)) || (NewValue != RegData[3]))
        {
            Gsv6k5CpProtect(port, value);
            AvUapiOutputDebugMessage("Port[%d][%d]:TxCore Pll Toggle", port->device->index, port->index);
            return 0;
        }
    }
    return 1;
}

void Gsv6k5CpProtect(pin AvPort *port, uint8 value)
{
    uint8 NewValue;
    /* 1. Tune Div Value */
    if(value == 1)
        NewValue = 0x0B;
    else
        NewValue = 0x0A;
    /* 2. Toggle Action */
    AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),  0xFF,0x01,0,0x01);
    AvHalI2cWriteField8(GSV6K5_PRIM_MAP_ADDR(port),  0xFF,0x01,0,0x00);
    AvHalI2cWriteField8(GSV6K5_APLL_MAP_ADDR(port),  0x44,0xFF,0,NewValue);
    AvHalI2cWriteField8(GSV6K5_APLL_MAP_ADDR(port),  0x42,0x80,0,0x00);
    AvHalI2cWriteField8(GSV6K5_APLL_MAP_ADDR(port),  0x42,0x80,0,0x80);
    AvHalI2cWriteField8(GSV6K5_APLL_MAP_ADDR(port),  0x44,0xFF,0,NewValue);
    AvHalI2cWriteField8(GSV6K5_ANAPHY_MAP_ADDR(port),0x89,0x04,0,0x04);
    AvHalI2cWriteField8(GSV6K5_ANAPHY_MAP_ADDR(port),0x89,0x04,0,0x00);
    /* 3. Timing Recalc */
    GSV6K5_CP_set_CP_TG_CAL_START_MAN(port, 1);
    GSV6K5_CP_set_CP_TG_CAL_START_MAN(port, 0);
}

void Gsv6k5ToggleDpllFreq(pin AvPort *port, uint8 index, uint8 Integer, uint8 *Fraction)
{
    uint8 DefaultValue = (Integer<<2) & 0xFC;
    uint8 ToggleValue  = DefaultValue | 0x02;
    uint8 Offset = 0x72 + (index*5);

    AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port),Offset+4,0xFF,0,DefaultValue);
    AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port),Offset+0,0xFF,0,Fraction[0]);
    AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port),Offset+1,0xFF,0,Fraction[1]);
    AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port),Offset+2,0xFF,0,Fraction[2]);
    AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port),Offset+3,0xFF,0,Fraction[3]);
    AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port),Offset+4,0xFF,0,ToggleValue);
    AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port),Offset+4,0xFF,0,DefaultValue);
}

void Gsv6k5RxManualEQUpdate(AvPort *port, uint8 DefaultEQEnable)
{
    uint8  TrainPass = 0;
    uint8  i = 0;
    uint8  round = 0;
    uint8  valid = 0;
    uint8  value = 0;
    uint8  check = 0;
    if(DefaultEQEnable)
    {
        if(port->core.HdmiCore != -1)
        {
            /* FRL Rx FSM reset, set 0 when AUTO EQ */
            if(port->content.rx->H2p1FrlManual == 0)
                GSV6K5_RXFRL_set_FRL_RX_FFE_REQ_MODE(port, (uint8)port->content.rx->H2p1FrlMode);
          if((port->device->index != 0) && (port->index != 6))
            AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port),0xD9,0xFF,0,0x00);
            AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port),0xFC,0xFF,0,0x00);
            Gsv6k5UpdateRxCdrBandWidth(port);
        }
        AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),0x83,0x01,0,0x01);
        AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),0xB3,0x01,0,0x01);
        AvHalI2cWriteField8(GSV6K5_PLL2_MAP_ADDR(port),0x83,0x01,0,0x01);
        AvHalI2cWriteField8(GSV6K5_PLL2_MAP_ADDR(port),0xB3,0x01,0,0x01);
        port->content.rx->Lock.EqLock = 0;
        port->content.rx->EQDelayExpire = 0;
        port->content.rx->H2p1FrlLockDelay = 0;
        port->content.rx->H2p1FrlStat = AV_FRL_TRAIN_IDLE;
        port->content.rx->H2p1FrlHblank = 0;
    }
    else if(port->core.HdmiCore != -1)
    {
        /* 1. FRL ends with pass */
        if(port->content.rx->H2p1FrlStat == AV_FRL_TRAIN_FORCE_PASS)
        {
            port->content.rx->Lock.EqLock = 1;
        }
        /* 3. FRL is not started, TMDS mode */
        else if((port->content.rx->H2p1FrlType == AV_FRL_NONE) && (port->content.rx->H2p1FrlManual == 0))
        {
            /* 3.1 TMDS mode SCDC auto recovery */
            if((port->content.rx->Lock.DeRegenLock == 0) && (port->content.rx->Lock.VSyncLock == 0))
            {
                /* Manual EQ Setting for possible false SCDC recovery on clock ratio/scramble*/
                /* RxDIG.0xD9 Registers includes:
                GSV6K5_RXDIG_get_RX_SCDC_TMDS_BIT_CLK_RATIO_MAN_EN(port, pval);
                GSV6K5_RXDIG_set_RX_SCDC_TMDS_BIT_CLK_RATIO_MAN_EN(port, val);
                GSV6K5_RXDIG_get_RX_SCDC_TMDS_BIT_CLK_RATIO_MAN(port, pval);
                GSV6K5_RXDIG_set_RX_SCDC_TMDS_BIT_CLK_RATIO_MAN(port, val);
                GSV6K5_RXDIG_get_RX_SCDC_SCR_EN_MAN_EN(port, pval);
                GSV6K5_RXDIG_set_RX_SCDC_SCR_EN_MAN_EN(port, val);
                GSV6K5_RXDIG_get_RX_SCDC_SCR_EN_MAN(port, pval);
                GSV6K5_RXDIG_set_RX_SCDC_SCR_EN_MAN(port, val);
                 */
                if((port->content.rx->EQDelayExpire == RxEQDelayExpireThreshold) &&
                   (port->content.rx->H2p1FrlType == AV_FRL_NONE))
                {
                    port->content.rx->EQDelayExpire = 0;
                    AvHalI2cReadField8(GSV6K5_RXDIG_MAP_ADDR(port),0xD9,0xFF,0,&value);
                    if((value == 0xF0) || (port->content.video->info.LaneFreq > 160))
                        value = 0xA0;
                    else
                        value = 0xF0;
                    AvHalI2cWriteField8(GSV6K5_RXDIG_MAP_ADDR(port),0xD9,0xFF,0,value);
                    Gsv6k5UpdateRxCdrBandWidth(port);
                    AvUapiOutputDebugMessage("Port[%d][%d]:Manual SCDC = %x", port->device->index, port->index, value);
                }
                else
                    port->content.rx->EQDelayExpire = port->content.rx->EQDelayExpire + 1;
            }
        }
        /* 4. check EQ eye status to check pass */
        if(port->content.rx->Lock.EqLock == 0)
        {
            valid = 0;
            if(port->content.rx->H2p1FrlManual != 0)
            {
                round = 4;
                check = 0;
#if AvEnableRxSingleLaneMode
                if((port->content.rx->H2p1FrlManual & 0x10000) != 0)
                    check = check + 1;
                if((port->content.rx->H2p1FrlManual & 0x20000) != 0)
                    check = check + 1;
                if((port->content.rx->H2p1FrlManual & 0x40000) != 0)
                    check = check + 1;
                if((port->content.rx->H2p1FrlManual & 0x80000) != 0)
                    check = check + 1;
#endif
            }
            else if((port->content.rx->H2p1FrlType == AV_FRL_6G3L) ||
                    (port->content.rx->H2p1FrlType == AV_FRL_3G3L) ||
                    (port->content.rx->H2p1FrlType == AV_FRL_NONE))
            {
                round = 4;
                check = 3;
            }
            else
            {
                round = 4;
                check = 4;
            }
            for(i=0;i<round;i++)
            {
                AvHalI2cReadField8(GSV6K5_RXLN0_MAP_ADDR(port)+i,0x37,0xFF,0,&value);
                if(value == 0)
                {
                    AvHalI2cReadField8(GSV6K5_RXLN0_MAP_ADDR(port)+i,0x38,0xFF,0,&value);
                    if(port->content.rx->H2p1FrlManual != 0)
                    {
                        if(((port->content.rx->H2p1FrlManual & 0xFFFF) > 20) && ((port->content.rx->H2p1FrlManual & 0xFFFF) < 2000))
                        {
                            if(value < 100)
                                value = 0;
                        }
                        else if(((port->content.rx->H2p1FrlManual & 0xFFFF) >= 2000) && ((port->content.rx->H2p1FrlManual & 0xFFFF) < 8600))
                        {
                            if(value < 150)
                                value = 0;
                        }
                        else if(value < 200)
                            value = 0;
                    }
                    else if(value < 30)
                        value = 0;
                }
                if(value != 0)
                    valid = valid + 1;
            }
            if(valid != check)
            {
#if AvEnableConsistentFrl
                if((port->content.rx->H2p1FrlType != AV_FRL_NONE) &&
                   (port->content.rx->EQDelayExpire >= RxFrlLockExpireThreshold))
                    port->content.rx->EQDelayExpire = RxEQDelayExpireThreshold;
#endif
                if(port->content.rx->EQDelayExpire == RxEQDelayExpireThreshold)
                {
                    Gsv6k5UpdateRxCdrBandWidth(port);
                    port->content.rx->EQDelayExpire = 0;
                    port->content.rx->H2p1FrlLockDelay = 0;
                    AvUapiOutputDebugMessage("Port[%d][%d]:Eq Reset", port->device->index, port->index);
                    if(port->content.rx->H2p1FrlManual != 0)
                        port->content.rx->H2p1FrlStat = AV_FRL_TRAIN_SUCCESS;
                }
                else
                    port->content.rx->EQDelayExpire = port->content.rx->EQDelayExpire + 1;
            }
            /* value is used as flag for lock */
            value = 0;
            if(check <= valid)
            {
                GSV6K5_INT_get_RX1_FRL_TRAINING_PASS_RAW_ST(port, &TrainPass);
                if((port->content.rx->H2p1FrlType == AV_FRL_NONE) && (port->content.rx->H2p1FrlManual == 0))
                {
                    value = 1;
                    AvUapiOutputDebugMessage("Port[%d][%d]:TMDS EQ done", port->device->index, port->index);
                }
                else if(port->content.rx->H2p1FrlManual != 0)
                {
                    value = 1;
                    GSV6K5_RXFRL_set_FRL_RX_CG_CODE_DELTA_MAN_EN(port, 0);
                    AvUapiOutputDebugMessage("Port[%d][%d]:GSLNK EQ done", port->device->index, port->index);
                }
                else if((port->content.rx->H2p1FrlStat == AV_FRL_TRAIN_SUCCESS) && (TrainPass == 1))
                {
                    value = 1;
                    Gsv6k5RxScdcCedClear(port);
                    AvUapiOutputDebugMessage("Port[%d][%d]:FRL EQ done", port->device->index, port->index);
                }
            }
            else if((port->content.rx->Lock.DeRegenLock == 1) && (port->content.rx->Lock.VSyncLock == 1) &&
                    (port->content.rx->H2p1FrlManual == 0) && (valid >= 3))
            {
                value = 1;
                AvUapiOutputDebugMessage("Port[%d][%d]:Shortcut EQ done", port->device->index, port->index);
            }
            if((port->core.HdmiCore != -1) && (port->content.rx->Lock.EqLock == 0) && (value == 1))
            {
                /* EMP reset status */
                AvHalI2cWriteField8(GSV6K5_RXINFO_MAP_ADDR(port),0xFE,0xFF,0,0x10);
                if(port->content.rx->H2p1FrlType != AV_FRL_NONE)
                {
                    if((port->content.rx->Lock.DeRegenLock == 0) && (port->content.rx->Lock.VSyncLock == 0))
                    {
                        Gsv6k5UpdateRxCdrBandWidth(port);
                        AvUapiOutputDebugMessage("Port[%d][%d]:FLT Clear", port->device->index, port->index);
                    }
                    /* Clear Training Pass */
                    GSV6K5_INT_set_RX1_FRL_TRAINING_PASS_CLEAR(port, 1);
                    /* FRL detection logic reset */
                    /*
                    if(port->content.rx->H2p1FrlStat == AV_FRL_TRAIN_SUCCESS)
                    {
                        GSV6K5_RXFRL_set_FRL_RX_CLK_CALC_RST(port, 1);
                        GSV6K5_RXFRL_set_FRL_RX_CLK_CALC_RST(port, 0);
                        GSV6K5_RXFRL_set_FRL_RX_LINE_DET_DIS(port, 1);
                        GSV6K5_RXFRL_set_FRL_RX_LINE_DET_DIS(port, 0);
                        GSV6K5_INT_set_RX1_FRL_VS_LOCK_CLEAR(port, 1);
                        GSV6K5_INT_set_RX1_FRL_HS_LOCK_CLEAR(port, 1);
                    }
                    */
                }
                AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),0xB3,0x01,0,0x00);
                AvHalI2cWriteField8(GSV6K5_PLL2_MAP_ADDR(port),0x83,0x01,0,0x00);
#if ((GSV6505 == 1) || (GSV2505 == 1))
                AvHalI2cWriteField8(GSV6K5_PLL_MAP_ADDR(port),0x83,0x01,0,0x00);
                AvHalI2cWriteField8(GSV6K5_PLL2_MAP_ADDR(port),0xB3,0x01,0,0x00);
#endif
            }
            /* parameter update */
            port->content.rx->Lock.EqLock = value;
        }
    }

    return;
}

void Gsv6k5RxEyePrint(AvPort *port)
{
    uint8  i;
    uint8  j;
    uint8  LaneNum;
    uint8  ReadData[2];
    uint32 Normalized;
    uint32 CdrDrift;

    /* 1. CDR Status */
    GSV6K5_LANE_set_CDR_V3_CAP_PHASE_CNT(port, 1);
    GSV6K5_LANE_set_CDR_V3_CAP_PHASE_CNT(port, 0);
    GSV6K5_LANE_get_RB_CDR_V3_FRUG_CNT(port, &CdrDrift);
    if((CdrDrift & 0x4000) != 0)
        CdrDrift = ((~CdrDrift) & 0x3fff) + 1;
    AvUapiOutputDebugMessage("Port[%d][%d] CDR Drift = %d", port->device->index, port->index, CdrDrift);
    /* 2. Lane data */
    if((port->content.rx->H2p1FrlType != AV_FRL_NONE) || (port->content.rx->H2p1FrlManual != 0))
        LaneNum = 4;
    else
        LaneNum = 3;
    for(i=0;i<LaneNum;i++)
    {
        /* Step 2.1 Swap Lane order for RxC/RxD */
        /* Step 2.2 Get Best Eye */
        AvHalI2cReadMultiField(GSV6K5_RXLN0_MAP_ADDR(port)+i, 0x37, 2, ReadData);
        Normalized = (ReadData[0]<<8) + ReadData[1];
        //if(Normalized > 100) /* Super Good Phase */
        //    Normalized = 100;
        AvUapiOutputDebugMessage("Port[%d][%d] Lane%d Eye Score = %d", port->device->index, port->index, i, Normalized);
        if((port->content.rx->H2p1FrlType == AV_FRL_12G4L) || ((port->content.rx->H2p1FrlManual&0xffff) > 10200))
        {
            AvHalI2cReadField8(GSV6K5_RXLN0_MAP_ADDR(port)+i,0xD8,0xFF,0,&ReadData[0]);
            if(ReadData[0] > 120)
            {
                AvHalI2cWriteField8(GSV6K5_RXLN0_MAP_ADDR(port),0x13,0xFF,0,0x01);
                AvUapiOutputDebugMessage("Port[%d][%d]:Safe 48G EQ", port->device->index, port->index);
            }
        }
        /* Step 2.3 Check Eye Width and EQ gain */
        AvHalI2cReadField8(GSV6K5_RXLN0_MAP_ADDR(port)+i,0x35,0xFF,0,&ReadData[0]);
        Normalized = 0;
        for(j=0;j<8;j++)
        {
            if((ReadData[0] & 0x01) == 0x01)
            {
                Normalized = Normalized + 1;
            }
            ReadData[0] = ReadData[0]>>1;
        }
        AvUapiOutputDebugMessage("Port[%d][%d]:Lane%d Eye Gain = %d", port->device->index, port->index, i, Normalized);
    }
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckLogicAudioRx(pin AvPort *port))
{
#if AvEnableAudioTTLInput
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint8  PktContent[12] = {0x84,0x01,0x0A,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    uint32 i = 0;

    while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
    {
        if(CurrentPort->type == HdmiTx)
        {
            /* 1. Routine Check */
            /* 1.1 Check Tx Sampling Frequency in SPDIF */
            if((CurrentPort->content.audio->AudFormat == AV_AUD_SPDIF) && (CurrentPort->core.HdmiCore != -1))
            {
                GSV6K5_AG_get_RB_ESTIMATED_MCLK_KHZ(port, &i);
                port->content.audio->AudLRclk = ((i>>7)+1) & 0xFFFC;
                if((port->content.audio->AudLRclk > 30) && (port->content.audio->AudLRclk < 34))
                    port->content.audio->SampFreq = AV_AUD_FS_32KHZ;
                else if((port->content.audio->AudLRclk > 42) && (port->content.audio->AudLRclk < 46))
                    port->content.audio->SampFreq = AV_AUD_FS_44KHZ;
                else if((port->content.audio->AudLRclk > 46) && (port->content.audio->AudLRclk < 50))
                    port->content.audio->SampFreq = AV_AUD_FS_48KHZ;
                else if((port->content.audio->AudLRclk > 86) && (port->content.audio->AudLRclk < 90))
                    port->content.audio->SampFreq = AV_AUD_FS_88KHZ;
                else if((port->content.audio->AudLRclk > 94) && (port->content.audio->AudLRclk < 98))
                    port->content.audio->SampFreq = AV_AUD_FS_96KHZ;
                else if((port->content.audio->AudLRclk > 174) && (port->content.audio->AudLRclk < 178))
                    port->content.audio->SampFreq = AV_AUD_FS_176KHZ;
                else if((port->content.audio->AudLRclk > 190) && (port->content.audio->AudLRclk < 194))
                    port->content.audio->SampFreq = AV_AUD_FS_192KHZ;
            }
            /* 1.2 Self Capability Declare */
            port->content.audio->NValue = NTable[port->content.audio->SampFreq];
            port->content.audio->NValue = Gsv6k5TuneTxAcr(CurrentPort, port->content.audio->NValue);
            port->content.audio->AvailableAudioPackets =
                AV_BIT_AUDIO_INFO_FRAME | AV_BIT_AUDIO_CHANNEL_STATUS | AV_BIT_AUDIO_SAMPLE_PACKET | AV_BIT_ACR_PACKET;
            /* 1.3 HDMI Tx Output is Stable to send Audio */
            if((CurrentPort->content.tx->InfoReady >= 1) &&
                /* Case 1, Audio N Value Difference */
               ((port->content.audio->NValue != CurrentPort->content.audio->NValue) ||
                /* Case 2, Sample Frequency Difference */
                (port->content.audio->SampFreq != CurrentPort->content.audio->SampFreq) ||
                /* Case 3, Channel Number Difference */
                (port->content.audio->ChanNum != CurrentPort->content.audio->ChanNum) ||
                /* Case 4, Channel Status SrcNum Difference */
                (port->content.audio->SrcNum != CurrentPort->content.audio->SrcNum) ||
                /* Case 5, Channel Status Word Length Difference */
                (port->content.audio->WordLen != CurrentPort->content.audio->WordLen) ||
                /* Case 6, Channel Status Word Length Difference */
                (port->content.audio->Consumer != CurrentPort->content.audio->Consumer) ||
                /* Case 7, Audio Coding Difference */
                (port->content.audio->AudCoding != CurrentPort->content.audio->AudCoding) ||
                /* Case 8, Audio Type Difference */
                (port->content.audio->AudType != CurrentPort->content.audio->AudType) ||
                /* Case 9, Audio Format Difference */
                (port->content.audio->AudFormat != CurrentPort->content.audio->AudFormat) ||
                /* Case 10, Audio Mclk Ratio Difference */
                (port->content.audio->AudMclkRatio != CurrentPort->content.audio->AudMclkRatio) ||
                /* Case 11, Audio Layout Difference */
                (port->content.audio->Layout != CurrentPort->content.audio->Layout) ||
                /* Case 12, Audio Packet Flag Difference */
                (((CurrentPort->content.audio->AvailableAudioPackets) &
                 (AV_BIT_AUDIO_INFO_FRAME | AV_BIT_AUDIO_CHANNEL_STATUS | AV_BIT_AUDIO_SAMPLE_PACKET | AV_BIT_ACR_PACKET))
                 != (AV_BIT_AUDIO_INFO_FRAME | AV_BIT_AUDIO_CHANNEL_STATUS | AV_BIT_AUDIO_SAMPLE_PACKET | AV_BIT_ACR_PACKET))))
                {
                    /* 2.2.1 Copy Audio Content Information */
                    AvMemcpy(CurrentPort->content.audio, port->content.audio, sizeof(AvAudio));
                    /* 2.2.2 Set AV_PKT_AUDIO_INFO_FRAME */
                    SET_AUDIF_CC(PktContent, CurrentPort->content.audio->ChanNum-1);
                    switch(CurrentPort->content.audio->ChanNum)
                    {
                        case 8:
                        case 7:
                            SET_AUDIF_CA(PktContent, 0x13);
                            break;
                        case 6:
                            SET_AUDIF_CA(PktContent, 0x0E);
                            break;
                        case 5:
                            SET_AUDIF_CA(PktContent, 0x07);
                            break;
                        case 4:
                            SET_AUDIF_CA(PktContent, 0x03);
                            break;
                        case 3:
                            SET_AUDIF_CA(PktContent, 0x01);
                            break;
                        default:
                            break;
                    }
                    AvUapiTxSetPacketContent(CurrentPort, AV_PKT_AUDIO_INFO_FRAME, PktContent,1);
                    /* 2.2.3 Set AV_PKT_AUDIO_CHANNEL_STATUS */
                    if((CurrentPort->content.audio->AudCoding == AV_AUD_FORMAT_LINEAR_PCM) || (CurrentPort->content.audio->AudCoding == 0x00))
                        CurrentPort->content.audio->Consumer = CurrentPort->content.audio->Consumer & 0x01;
                    else
                        CurrentPort->content.audio->Consumer = CurrentPort->content.audio->Consumer | 0x02;
                    AvUapiTxSetPacketContent(CurrentPort, AV_PKT_AUDIO_CHANNEL_STATUS, PktContent,1);
                    /* 2.2.4 Set AV_BIT_AUDIO_SAMPLE_PACKET, AV_BIT_ACR_PACKET */
                    AvUapiTxSetAudioPackets(CurrentPort);
                }
            /* 1.3 Clear Tx Audio Fifo Status */
            if(CurrentPort->core.HdmiCore == 0)
                GSV6K5_INT_set_TX1_AUDIOFIFO_FULL_CLEAR(CurrentPort, 1);
        }
        PrevPort = CurrentPort;
        CurrentPort = NULL;
    }
#endif
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckLogicAudioTx(pin AvPort *port))
{
    AvPort *TempPort;
    uint8 i = 0;
    uint8 OldValue = 0;
    uint8 NewValue = 0xff;
    uint8 RbRoute[2] = {0x00,0x00};
    AvAudioSampleFreq SampFreq = AV_AUD_FS_FROM_STRM;

    /* Step 1. Setup New Status */
    TempPort = (AvPort*)(port->content.RouteAudioFromPort);
    if(TempPort != NULL)
    {
        if(TempPort->type == HdmiRx)
        {
            SampFreq = TempPort->content.audio->SampFreq;
            if((TempPort->content.rx->Lock.AudioLock == 0) || (TempPort->content.audio->AudioMute == 1))
                NewValue = 0xff;
            else if((TempPort->content.audio->AudCoding == AV_AUD_FORMAT_LINEAR_PCM) ||
                    (TempPort->content.audio->AudCoding == 0x00))
                NewValue = 0;
            else
#if (AvEnableTTLCompressAudio == 1) || (AvUseSpdifOutOnI2sData0 == 1)
                NewValue = 0;
#else
                NewValue = 0xef;
#endif
            /* Block Audio Output for Low Jitter Requirement */
            /*
            uint32 temp32 = 0;
            if(NewValue == 0)
            {
                GSV6K5_RXDIG_get_APLL_CHK_USDW_INRANGE_DELTA(TempPort, &temp32);
                if(temp32 == 0x800000)
                    NewValue = 0xff;
            }
            */
        }
        else if(TempPort->type == HdmiTx)
        {
#if AvEnableSpdifToI2s
                NewValue = 0x00;
#else
                NewValue = 0xef;
#endif
        }
        else if((TempPort->type == LogicAudioRx) || (TempPort->type == AudioGen))
        {
            NewValue = 0x00;
#if AvEnableSpdifToI2s
            /* TTL SPDIF to I2S conversion */
            if(((TempPort->index == 10) && (port->index == 8)) ||
               ((TempPort->index == 11) && (port->index == 9)))
                NewValue = 0x10;
#endif
        }
    }
    if(port->content.audio->AudioMute == 1)
        NewValue = 0xff;
#if AvEnableI2sDualDirection
    /* I2S Dual Direction, [7:4] for Audio Input, [3:0] for Audio Output */
    /* [0] = I2S Stereo Data, [1] = LRCLK, [2] = MCLK, [3] = SCLK */
    NewValue = OldValue | 0xf0;
#endif
    /* Step 2. Get Current TriState Status and Update Pin Status*/
    if(port->index == 8) /* OUT AP1 */
    {
        GSV6K5_PRIM_get_AUD1_OEN(port, &OldValue);
        if(NewValue != OldValue)
        {
            GSV6K5_PRIM_set_AUD1_OEN(port, NewValue);
            if((NewValue == 0) && (port->content.audio->WordLen != 0) && (TempPort->type == HdmiRx))
            {
                GSV6K5_RXAUD_set_WORD_LEN_MAN_EN(TempPort, 1);
                GSV6K5_RXAUD_set_WORD_LEN_MAN(TempPort, port->content.audio->WordLen);
            }
        }
    }
    else if(port->index == 9) /* OUT AP2 */
    {
        GSV6K5_PRIM_get_AUD2_OEN(port, &OldValue);
        if(NewValue != OldValue)
        {
            GSV6K5_PRIM_set_AUD2_OEN(port, NewValue);
            if((NewValue == 0) && (port->content.audio->WordLen != 0) && (TempPort->type == HdmiRx))
            {
                GSV6K5_RXAUD_set_WORD_LEN_MAN_EN(TempPort, 1);
                GSV6K5_RXAUD_set_WORD_LEN_MAN(TempPort, port->content.audio->WordLen);
            }
        }
    }
    /* Step 3. check mclk ratio for I2S output */
    if((NewValue & 0x01) == 0)
    {
        /* 3.1 Get Mclk ratio from ChannelStatusSfTable*/
        i = 0;
        while (ChannelStatusSfTable[i+1] != 0xff)
        {
            if (ChannelStatusSfTable[i] == SampFreq)
            {
                NewValue = ChannelStatusSfTable[i+2];
                break;
            }
            i = i+3;
        }
        /* 3.2 Set Mclk if incorrect */
        if(TempPort->type == HdmiRx)
        {
            if(NewValue != 0)
            {
                AvHalI2cReadMultiField(GSV6K5_SEC_MAP_ADDR(TempPort), 0x09, 2, RbRoute);
                i = (RbRoute[0] | RbRoute[1]) & 0x22;
                if(i)
                    NewValue = 0;
            }
            GSV6K5_RXDIG_get_RX_AUDIO_MCLK_X_N(TempPort, &OldValue);
            if(NewValue != OldValue)
                GSV6K5_RXDIG_set_RX_AUDIO_MCLK_X_N(TempPort, NewValue);
        }
    }

    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckVideoScaler(pin AvPort *port))
{
    AvPort *FromPort = (AvPort*)port->content.RouteVideoFromPort;
    uint8 value1 = 0;
    uint8 NewValue1 = 0;
    uint8 value2 = 0;
    uint8 NewValue2 = 0;
    uint8 InVic = 0;
    uint8 OutVic = 0;
    uint8 hparam = 0;
    uint8 vparam = 0;
    uint8 hratio = 0;
    uint8 vratio = 0;

    /* 0. Check Core Connection */
    if((port != FindCp1Mode(port)) ||
       ((port->content.scaler->ThumbnailMode == AV_THUMBNAIL_DISABLE) &&
        (port->content.RouteVideoToPort == NULL)))
    {
        GSV6K5_SEC_get_CP_MODE(port,&value1);
        if((value1 == 1) || (value1 == 4) || (value1 == 5))
        {
            GSV6K5_SEC_set_CP_MODE(port,0);
            GSV6K5_VSP_set_CP_VID_IN(port, 0);
            GSV6K5_VSP_set_CP_VIN_PARAM_MAN_EN(port, 0);
        }
        Gsv6k5ScaleRatio(port, 0, 0, 0, 0, 0);
        return AvOk;
    }
    if((FromPort->type == HdmiRx) && (FromPort->core.HdmiCore != -1) &&
       (FromPort->content.rx->IsFreeRun == 0))
    {
        GSV6K5_VSP_get_CP_VID_IN(port, &value1);
        if(value1 != 0)
            GSV6K5_VSP_set_CP_VID_IN(port, 0);
        GSV6K5_VSP_get_CP_VIN_PARAM_MAN_EN(port, &value1);
        if(value1 != 0)
            GSV6K5_VSP_set_CP_VIN_PARAM_MAN_EN(port, 0);
        Gsv6k5ScaleRatio(port, 0, 0, 0, 0, 0);
        return AvOk;
    }

    /* 1. Thumbnail configuration */
    /* 2. Vic Mapping */
    /* 2.1 De-interlacer */
    switch(port->content.scaler->ScalerInVic)
    {
        case 6:
        case 7:  /* 480i@60Hz */
        case 50:
        case 51: /* 480i@120Hz */
        case 58:
        case 59: /* 480i@240Hz */
            InVic = 2;
            OutVic = 2;
            break;
        case 21:
        case 22: /* 576i@50Hz */
        case 44:
        case 45: /* 576i@100Hz */
        case 54:
        case 55: /* 576i@200Hz */
            InVic = 17;
            OutVic = 17;
            break;
        case 5: /* 1080i@60Hz */
        case 46: /* 1080i@120Hz */
            InVic = 16;
            OutVic = 16;
            break;
        case 20: /* 1080i@50Hz */
        case 40: /* 1080i@100Hz */
            InVic = 31;
            OutVic = 31;
            break;
    }
    if(InVic == 0)
    {
        /* 2.2 x2 downscaling ratio */
        if((port->content.scaler->Vratio == 2) && (port->content.scaler->Hratio == 2))
        {
            switch(port->content.scaler->ScalerInVic)
            {
                case 93: /* 4K@24Hz */
                case 103:
                    InVic = 93;
                    OutVic = 32;
                    break;
                case 114: /* 4K@48Hz */
                case 116:
                    InVic = 93;
                    OutVic = 111;
                    break;
                case 98: /* 4K SMPTE@24Hz */
                    InVic = 98;
                    OutVic = 32;
                    break;
                case 115: /* 4K@SMPTE@48Hz */
                    InVic = 98;
                    OutVic = 111;
                    break;
                case 97:
                case 107: /* 4K@60Hz */
                    InVic = 97;
                    OutVic = 16;
                    break;
                case 118:
                case 120: /* 4K@120Hz */
                    InVic = 97;
                    OutVic = 63;
                    break;
                case 95: /* 4K@30Hz */
                case 105:
                    InVic = 95;
                    OutVic = 34;
                    break;
                case 100: /* 4K SMPTE@30Hz */
                    InVic = 100;
                    OutVic = 34;
                    break;
                case 102: /* 4096x2160@60Hz*/
                    InVic = 102;
                    OutVic = 16;
                    break;
                case 219: /* 4096x2160@120Hz*/
                    InVic = 102;
                    OutVic = 63;
                    break;
                case 94: /* 4K@25Hz */
                case 104:
                    InVic = 94;
                    OutVic = 33;
                    break;
                case 96:
                case 106: /* 4K@50Hz */
                    InVic = 96;
                    OutVic = 31;
                    break;
                case 117:
                case 119: /* 4K@100Hz */
                    InVic = 96;
                    OutVic = 64;
                    break;
                case 99: /* 4K SMPTE@25Hz */
                    InVic = 99;
                    OutVic = 33;
                    break;
                case 101: /* 4096x2160@50Hz*/
                    InVic = 101;
                    OutVic = 31;
                    break;
                case 218: /* 4096x2160@100Hz*/
                    InVic = 101;
                    OutVic = 64;
                    break;
                case 199: /* 8K@60Hz */
                case 207:
                    InVic = 199;
                    break;
                case 198: /* 8K@50Hz */
                case 206:
                    InVic = 198;
                    break;
                case 194: /* 8K@24Hz */
                case 202:
                    InVic = 194;
                    break;
                case 195: /* 8K@25Hz */
                case 203:
                    InVic = 195;
                    break;
                case 196: /* 8K@30Hz */
                case 204:
                    InVic = 196;
                    break;
                default:
                    InVic = 0;
                    OutVic = 0;
                    break;
            }
            if(InVic != OutVic)
            {
                hparam = 1;
                vparam = 1;
                hratio = 1;
                vratio = 1;
                if(OutVic == 0)
                {
                    hparam = 2;
                    vparam = 2;
                }
            }
        }
        /* 2.3 x4 downscaling ratio */
        if((port->content.scaler->Vratio == 4) && (port->content.scaler->Hratio == 4))
        {
            switch(port->content.scaler->ScalerInVic)
            {
                case 199: /* 8K@60Hz */
                case 207:
                    InVic = 207;
                    OutVic = 16;
                    break;
                case 198: /* 8K@50Hz */
                case 206:
                    InVic = 206;
                    OutVic = 31;
                    break;
                case 194: /* 8K@24Hz */
                case 202:
                    InVic = 202;
                    OutVic = 32;
                    break;
                case 195: /* 8K@25Hz */
                case 203:
                    InVic = 195;
                    OutVic = 33;
                    break;
                case 196: /* 8K@30Hz */
                case 204:
                    InVic = 196;
                    OutVic = 34;
                    break;
                default:
                    InVic = 0;
                    OutVic = 0;
                    break;
            }
            if(InVic != OutVic)
            {
                hparam = 1;
                vparam = 1;
                hratio = 3;
                vratio = 3;
            }
        }
    }
    /* 2.4 VRR downscaler management */
    if((FromPort->type == HdmiRx) && (FromPort->core.HdmiCore != -1))
    {
        if(FromPort->content.video->VrrStream == 1)
        {
            InVic = 0;
            OutVic = 0;
            vparam = 0;
            vratio = 0;
            hparam = 0;
            hratio = 0;
            Gsv6k5VrrTimingAdjust(FromPort);
        }
    }
    /* 3. Check Vid In */
    GSV6K5_VSP_get_CP_VID_IN(port, &value1);
    if(value1 != InVic)
    {
        port->content.scaler->ScalerOutVic = OutVic;
        GSV6K5_VSP_set_CP_VID_IN(port, InVic);
        Gsv6k5ScaleRatio(port, hparam, hratio, vparam, vratio, 1);
    }
    /* 3.1 Check Manual Setting */
    GSV6K5_VSP_get_CP_VIN_PARAM_MAN_EN(port, &value1);
    if((FromPort->type == HdmiRx) && (InVic == 0) &&
       (FromPort->content.rx->IsFreeRun == 1))
        value2 = 1;
    else
        value2 = 0;
    if(value1 != value2)
    {
        GSV6K5_VSP_set_CP_VIN_PARAM_MAN_EN(port, value2);
        GSV6K5_CP_get_CP_OUT_QUAD_EN(port, &value1);
        Gsv6k5CpProtect(port,value1);
    }
    /* 3.2 Manual Timing Setting */
    if((FromPort->type == HdmiRx) && (FromPort->core.HdmiCore != -1) &&
       (FromPort->content.rx->IsFreeRun == 1))
    {
#if AvEnableDetailTiming
        if((FromPort->content.video->VrrStream == 1) ||
           ((OutVic == 0) &&
            ((FromPort->content.video->Y              != port->content.scaler->ColorSpace) ||
             (FromPort->content.video->timing.HActive != port->content.scaler->timing.HActive) ||
             (FromPort->content.video->timing.VActive != port->content.scaler->timing.VActive) ||
             (FromPort->content.video->timing.HTotal  != port->content.scaler->timing.HTotal) ||
             (FromPort->content.video->timing.VTotal  != port->content.scaler->timing.VTotal) ||
             (FromPort->content.video->timing.HSync   != port->content.scaler->timing.HSync) ||
             (FromPort->content.video->timing.VSync   != port->content.scaler->timing.VSync) ||
             (FromPort->content.video->timing.HBack   != port->content.scaler->timing.HBack) ||
             (FromPort->content.video->timing.VBack   != port->content.scaler->timing.VBack))))
        {
            Gsv6k5ManualCpParameter(port, FromPort);
            AvMemcpy((void*)(&port->content.scaler->timing), (void*)(&FromPort->content.video->timing), sizeof(VideoTiming));
        }
        /* Downscaler configuration */
        if((vparam == 0) && (vratio == 0) && (hparam == 0) && (hratio == 0))
        {
            switch(port->content.scaler->Hratio)
            {
                case 2:
                    hparam = 2;
                    hratio = 1;
                    break;
                case 3:
                    hparam = 3;
                    hratio = 2;
                    break;
                case 4:
                    hparam = 4;
                    hratio = 3;
                    break;
                case 8:
                    hparam = 5;
                    hratio = 4;
                    break;
                case 16:
                    hparam = 6;
                    hratio = 5;
                    break;
                case 32:
                    hparam = 7;
                    hratio = 6;
                    break;
            }
            switch(port->content.scaler->Vratio)
            {
                case 2:
                    vparam = 2;
                    vratio = 1;
                    break;
                case 3:
                    vparam = 3;
                    vratio = 2;
                    break;
                case 4:
                    vparam = 4;
                    vratio = 3;
                    break;
            }
        }
        Gsv6k5ScaleRatio(port, hparam, hratio, vparam, vratio, 0);
#endif
    }
    /* 4. Check Function Enable State */
    GSV6K5_SEC_get_CP_MODE(port,&value1);
    switch(port->content.scaler->ColorSpace)
    {
        case AV_Y2Y1Y0_YCBCR_420:
            NewValue1 = 4; /* 420 downscaler */
            break;
        default:
            NewValue1 = 5; /* 444 downscaler */
            break;
    }
    if((port->content.scaler->Hratio <= 1) || (port->content.scaler->Vratio <= 1))
       NewValue1 = 0; /* Bypass */
    /* Deinterlacer */
    if((FromPort->type == HdmiRx) &&
       (FromPort->content.rx->IsInputStable == 1))
    {
        if(FromPort->content.video->timing.Interlaced == 1)
            NewValue1 = 1;
        else if(FromPort->content.video->info.TmdsFreq < 150)
            NewValue1 = 0;
    }
    if(value1 != NewValue1)
    {
        GSV6K5_SEC_set_CP_MODE(port, NewValue1);
        GSV6K5_CP_get_CP_OUT_QUAD_EN(port, &value1);
        Gsv6k5CpProtect(port,value1);
    }
    /* 5. Set Function Mode */
    GSV6K5_VSP_get_CP_VOUT_CHN_SWAP(port, &value1);
    GSV6K5_VSP_get_CP_VIN_CHN_SWAP(port, &value2);
    switch(port->content.scaler->ColorSpace)
    {
        case AV_Y2Y1Y0_YCBCR_420:
            NewValue1 = 4; /* 420 downscaler */
            NewValue2 = 0;
            break;
        default:
            NewValue1 = 0;
            NewValue2 = 0;
            break;
    }
    if((value1 != NewValue1) || (value2 != NewValue2))
    {
        GSV6K5_VSP_set_CP_VOUT_CHN_SWAP(port, NewValue1);
        GSV6K5_VSP_set_CP_VIN_CHN_SWAP(port,  NewValue2);
    }
    /* 6. Check CP CSC State */
    if((FromPort->type == HdmiRx) && (FromPort->content.rx->IsInputStable == 1))
    {
        /* Configure CP CSC */
        Gsv6k5CpCscManage(port, port->content.scaler->ScalerInCs,
                           port->content.scaler->ScalerOutCs);
    }

    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckVideoColor(pin AvPort *port))
{
    uint8 value1 = 0;
    uint8 value2 = 0;
    uint8 NewValue1 = 0;
    uint8 NewValue2 = 0;
    uint32 temp = 0;
    AvPort *FromPort = (AvPort*)(port->content.RouteVideoFromPort);

    /* 0. Check Core Connection */
    if((port != FindCp1Mode(port)) ||
       (port->content.RouteVideoToPort == NULL))
    {
        GSV6K5_SEC_get_CP_MODE(port,&value1);
        if((value1 == 2) || (value1 == 3))
            GSV6K5_SEC_set_CP_MODE(port,0);
        return AvOk;
    }
    if((FromPort->type == HdmiRx) && (FromPort->core.HdmiCore != -1) &&
       (FromPort->content.rx->IsFreeRun == 0))
        return AvOk;
    /* 1. Check Vic */
    GSV6K5_VSP_get_CP_VID_IN(port, &value1);
    if(value1 != port->content.color->ColorInVic)
    {
        GSV6K5_CP_set_CP_OUT_QUAD_EN(port, 0);
        GSV6K5_VSP_set_CP_VID_IN(port, port->content.color->ColorInVic);
        GSV6K5_CP_get_CP_OUT_QUAD_EN(port, &value1);
        Gsv6k5CpProtect(port,value1);
    }
    /* 2. Check Input Color Space, Convert to YUV anyway */
    Gsv6k5CpCscManage(port, port->content.color->ColorInCs, port->content.color->ColorOutCs);
    /* 4. Check Function Enable State */
    GSV6K5_SEC_get_CP_MODE(port,&value1);
    if((port->content.color->ColorOutSpace == AV_Y2Y1Y0_YCBCR_420) &&
       (port->content.color->ColorInSpace  != AV_Y2Y1Y0_YCBCR_420))
        NewValue1 = 3; /* 444 to 420 */
    else if((port->content.color->ColorOutSpace != AV_Y2Y1Y0_YCBCR_420) &&
            (port->content.color->ColorInSpace  == AV_Y2Y1Y0_YCBCR_420))
        NewValue1 = 2; /* 420 to 444 */
    else
        NewValue1 = 0; /* bypass */
    if(value1 != NewValue1)
    {
        GSV6K5_SEC_set_CP_MODE(port, NewValue1);
        GSV6K5_CP_get_CP_OUT_QUAD_EN(port, &value1);
        Gsv6k5CpProtect(port,value1);
    }
    /* 5. Set Function Mode */
    GSV6K5_VSP_get_CP_VOUT_CHN_SWAP(port, &value1);
    GSV6K5_VSP_get_CP_VIN_CHN_SWAP(port, &value2);
    NewValue1 = 0;
    NewValue2 = 0;
    if((port->content.color->ColorOutSpace == AV_Y2Y1Y0_YCBCR_420) &&
       (port->content.color->ColorInSpace  != AV_Y2Y1Y0_YCBCR_420))
        NewValue2 = 3; /* 444 to 420 */
    else if((port->content.color->ColorOutSpace != AV_Y2Y1Y0_YCBCR_420) &&
            (port->content.color->ColorInSpace  == AV_Y2Y1Y0_YCBCR_420))
        NewValue1 = 4; /* 420 to 444 */
    if((value1 != NewValue1) || (value2 != NewValue2))
    {
        GSV6K5_VSP_set_CP_VOUT_CHN_SWAP(port, NewValue1);
        GSV6K5_VSP_set_CP_VIN_CHN_SWAP(port,  NewValue2);
    }

    /* 6. Check Detail Timing Setting */
    if(((port->content.color->ColorOutSpace == AV_Y2Y1Y0_YCBCR_420) &&
        (port->content.color->ColorInSpace  != AV_Y2Y1Y0_YCBCR_420)) ||
       ((port->content.color->ColorOutSpace != AV_Y2Y1Y0_YCBCR_420) &&
        (port->content.color->ColorInSpace  == AV_Y2Y1Y0_YCBCR_420)))
        NewValue1 = 1;
    else
        NewValue1 = 0;
    temp = 0;
    while(Gsv6k5VideoColorAutoVicTable[temp] != 0xff)
    {
        /* 6.1 Found Auto Conversion Vic */
        if(port->content.color->ColorInVic == Gsv6k5VideoColorAutoVicTable[temp])
        {
            NewValue1 = 0;
            break;
        }
        temp = temp + 1;
    }
    GSV6K5_VSP_get_CP_VIN_PARAM_MAN_EN(port, &value1);
    if(value1 != NewValue1)
    {
        GSV6K5_VSP_set_CP_VIN_PARAM_MAN_EN(port, NewValue1);
        GSV6K5_CP_get_CP_OUT_QUAD_EN(port, &value1);
        Gsv6k5CpProtect(port,value1);
    }
    /* 6.2 Use Manual Conversion Vic */
#if AvEnableDetailTiming
    if((FromPort->type == HdmiRx) && (FromPort->core.HdmiCore != -1) &&
       (FromPort->content.rx->IsFreeRun == 1) && (NewValue1 == 1))
    {
        if((FromPort->content.video->timing.HActive != port->content.color->timing.HActive) ||
           (FromPort->content.video->timing.VActive != port->content.color->timing.VActive) ||
           (FromPort->content.video->timing.HTotal  != port->content.color->timing.HTotal) ||
           (FromPort->content.video->timing.VTotal  != port->content.color->timing.VTotal) ||
           (FromPort->content.video->timing.HSync   != port->content.color->timing.HSync) ||
           (FromPort->content.video->timing.VSync   != port->content.color->timing.VSync) ||
           (FromPort->content.video->timing.HBack   != port->content.color->timing.HBack) ||
           (FromPort->content.video->timing.VBack   != port->content.color->timing.VBack))
        {
            Gsv6k5ManualCpParameter(port, FromPort);
            AvMemcpy((void*)(&port->content.color->timing), (void*)(&FromPort->content.video->timing), sizeof(VideoTiming));
        }
    }
#endif

    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckVideoGen(pin AvPort *port))
{
#if AvEnableInternalVideoGen
    AvPort *TempPort = NULL;
    uint32 i = 0;
    uint8  Value = 0;
    uint8  NewValue = 0;
    uint32 CalcWord;
    uint8  DpWord;
    uint8  FqWord[4];
    /* 1. Check Enable */
    TempPort = (AvPort*)(port->content.RouteVideoToPort);
    if(TempPort == NULL)
        NewValue = 0;
    else
        NewValue = 1;
    GSV6K5_VSP_get_DG_EN(port, &Value);
    if(Value != NewValue)
    {
        GSV6K5_VSP_set_DG_EN(port, NewValue);
        if(NewValue == 0)
            GSV6K5_VSP_set_DG_VID(port, 0);
    }
    /* 1.1 Enable VideoGen Stable after configuration */
    port->content.rx->IsFreeRun = NewValue;

    if(NewValue == 1)
    {
        if(port->content.vg->Enable == 0)
        {
            /* 2. Check Vic */
            i = 0;
            while(VideoGenVicTable[i] != 0xff)
            {
                if(VideoGenVicTable[i] == port->content.video->timing.Vic)
                {
                    NewValue = VideoGenVicTable[i+1];
                    GSV6K5_VSP_get_DG_VID(port, &Value);
                    if(Value != NewValue)
                    {
                        GSV6K5_VSP_set_DG_PARAM_MAN_EN(port, 0);
                        GSV6K5_VSP_set_DG_VID(port, NewValue);
                        /* HDMI 2.1 timing enabling */
                        if(((VideoGenVicTable[i] > 116) && (VideoGenVicTable[i] < 121)) ||
                           (VideoGenVicTable[i] > 124))
                            Value = 1;
                        else
                            Value = 0;
                        AvHalI2cWriteField8(GSV6K5_VSP_MAP_ADDR(port), 0x02, 0x2, 0x1, Value);
                        /* manual Channel-3 clock setting */
                        if(port->core.HdmiCore == 1)
                        {
                            NewValue = VideoGenVicTable[i+2];
                            Gsv6k5ToggleDpllFreq(port, 3, NewValue, (uint8*)(VideoGenVicTable+i+3));
                        }
                        /* common timing information fill in */
                        /*
                        for(i=0;PresetTimingTables[i].Vic != 255;i++)
                        {
                            if(PresetTimingTables[i].Vic == port->content.video->timing.Vic)
                            {
                                port->content.video->timing.HActive = PresetTimingTables[i].HActive;
                                port->content.video->timing.VActive = PresetTimingTables[i].VActive;
                                port->content.video->timing.HTotal  = PresetTimingTables[i].HTotal;
                                port->content.video->timing.VTotal  = PresetTimingTables[i].VTotal;
                                port->content.video->timing.VFront  = PresetTimingTables[i].VFront;
                                port->content.video->timing.VSync   = PresetTimingTables[i].VSync;
                                port->content.video->timing.VBack   = PresetTimingTables[i].VBack;
                                port->content.video->timing.HFront  = PresetTimingTables[i].HFront;
                                port->content.video->timing.HSync   = PresetTimingTables[i].HSync;
                                port->content.video->timing.HBack   = PresetTimingTables[i].HBack;
                                break;
                            }
                        }
                        */
                    }
                    break;
                }
                i = i+7;
            }
            /* 3. Check Pattern */
            GSV6K5_VSP_get_DG_PATTERN(port, &Value);
            if(Value != port->content.vg->Pattern)
                GSV6K5_VSP_set_DG_PATTERN(port, port->content.vg->Pattern);
            /* 4. Check 420 */
            GSV6K5_VSP_get_DG_420_EN(port, &Value);
            NewValue = 0;
            if(Value != NewValue)
                GSV6K5_VSP_set_DG_420_EN(port, NewValue);
        }
        else if(port->content.vg->Enable == 1)
        {
            GSV6K5_VSP_set_DG_PARAM_MAN_EN(port, 1);
            AvHalI2cWriteField8(GSV6K5_VSP_MAP_ADDR(port), 0x02, 0x2, 0x1, 0);
            GSV6K5_VSP_set_DG_V_POL(port,    port->content.video->timing.VPolarity);
            GSV6K5_VSP_set_DG_H_POL(port,    port->content.video->timing.HPolarity);
            GSV6K5_VSP_set_DG_H_TOTAL(port,  port->content.video->timing.HTotal);
            GSV6K5_VSP_set_DG_H_ACTIVE(port, port->content.video->timing.HActive);
            GSV6K5_VSP_set_DG_H_FRONT(port,  port->content.video->timing.HFront);
            GSV6K5_VSP_set_DG_H_SYNC(port,   port->content.video->timing.HSync);
            GSV6K5_VSP_set_DG_H_BACK(port,   port->content.video->timing.HBack);
            GSV6K5_VSP_set_DG_V_TOTAL(port,  port->content.video->timing.VTotal);
            GSV6K5_VSP_set_DG_V_ACTIVE(port, port->content.video->timing.VActive);
            GSV6K5_VSP_set_DG_V_FRONT(port,  port->content.video->timing.VFront);
            GSV6K5_VSP_set_DG_V_SYNC(port,   port->content.video->timing.VSync);
            GSV6K5_VSP_set_DG_V_BACK(port,   port->content.video->timing.VBack);
            if(port->content.video->info.TmdsFreq != 0)
            {
                AvMemset((void*)(&FqWord), 0, 4);
                CalcWord  = (1200<<16)/(port->content.video->info.TmdsFreq);
                DpWord    = (CalcWord>>16) & 0x3F;
                if(DpWord != 0)
                {
                    CalcWord  = (CalcWord & 0xFFFF)/DpWord;
                    FqWord[0] = (CalcWord>>8)  & 0xFF;
                    FqWord[1] = (CalcWord>>0)  & 0xFF;
                }
                Gsv6k5ToggleDpllFreq(port, 3, DpWord, FqWord);
            }
            /* 3. Check Pattern */
            GSV6K5_VSP_get_DG_PATTERN(port, &Value);
            if(Value != port->content.vg->Pattern)
                GSV6K5_VSP_set_DG_PATTERN(port, port->content.vg->Pattern);
            /* 4. Check 420 */
            GSV6K5_VSP_get_DG_420_EN(port, &Value);
            NewValue = 0;
            if(Value != NewValue)
                GSV6K5_VSP_set_DG_420_EN(port, NewValue);
            port->content.vg->Enable = 0xFF;
        }
    }

#endif
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckAudioGen(pin AvPort *port))
{
#if AvEnableInternalAudioGen
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint8  PktContent[12] = {0x84,0x01,0x0A,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    uint8  offset = 0;
    if((port->content.ag->Update == 1) && (port->content.ag->Enable == 1))
    {
        /* 1. Sample Setting */
        offset = ((uint8)(port->content.audio->SampFreq))*5;
        Gsv6k5ToggleDpllFreq(port, 2, AudioGenSfTable[offset], (uint8*)(AudioGenSfTable+offset+1));
        /* 2. Self Capability Declare */
        port->content.rx->Lock.AudioLock = 1;
        port->content.audio->NValue = NTable[port->content.audio->SampFreq];
        port->content.audio->AvailableAudioPackets =
            AV_BIT_AUDIO_INFO_FRAME | AV_BIT_AUDIO_CHANNEL_STATUS | AV_BIT_AUDIO_SAMPLE_PACKET | AV_BIT_ACR_PACKET;
        /* 3. Audio Channel Configuration */
        switch(port->content.audio->ChanNum)
        {
            case 2:
                offset = 0x01;
                break;
            case 4:
                offset = 0x03;
                break;
            case 6:
                offset = 0x07;
                break;
            case 8:
                offset = 0x0F;
                break;
            default:
                offset = 0x00;
        }
        GSV6K5_AG_set_CHANNELEN(port, offset);
        /* clear update flag */
        port->content.ag->Update = 0;
    }

    while(KfunFindAudioNextTxEnd(port, &PrevPort, &CurrentPort) == AvOk)
    {
        if(CurrentPort->type == HdmiTx)
        {
            /* 3. HDMI Tx Output is Stable to send Audio */
            if((CurrentPort->content.tx->InfoReady >= 1) &&
                /* Case 1, Audio N Value Difference */
               ((port->content.audio->NValue != CurrentPort->content.audio->NValue) ||
                /* Case 2, Sample Frequency Difference */
                (port->content.audio->SampFreq != CurrentPort->content.audio->SampFreq) ||
                /* Case 3, Channel Number Difference */
                (port->content.audio->ChanNum != CurrentPort->content.audio->ChanNum) ||
                /* Case 4, Channel Status SrcNum Difference */
                (port->content.audio->SrcNum != CurrentPort->content.audio->SrcNum) ||
                /* Case 5, Channel Status Word Length Difference */
                (port->content.audio->WordLen != CurrentPort->content.audio->WordLen) ||
                /* Case 6, Channel Status Word Length Difference */
                (port->content.audio->Consumer != CurrentPort->content.audio->Consumer) ||
                /* Case 7, Audio Coding Difference */
                (port->content.audio->AudCoding != CurrentPort->content.audio->AudCoding) ||
                /* Case 8, Audio Type Difference */
                (port->content.audio->AudType != CurrentPort->content.audio->AudType) ||
                /* Case 9, Audio Format Difference */
                (port->content.audio->AudFormat != CurrentPort->content.audio->AudFormat) ||
                /* Case 10, Audio Mclk Ratio Difference */
                (port->content.audio->AudMclkRatio != CurrentPort->content.audio->AudMclkRatio) ||
                /* Case 11, Audio Layout Difference */
                (port->content.audio->Layout != CurrentPort->content.audio->Layout) ||
                /* Case 12, Audio Packet Flag Difference */
                (((CurrentPort->content.audio->AvailableAudioPackets) &
                 (AV_BIT_AUDIO_INFO_FRAME | AV_BIT_AUDIO_CHANNEL_STATUS | AV_BIT_AUDIO_SAMPLE_PACKET | AV_BIT_ACR_PACKET))
                 != (AV_BIT_AUDIO_INFO_FRAME | AV_BIT_AUDIO_CHANNEL_STATUS | AV_BIT_AUDIO_SAMPLE_PACKET | AV_BIT_ACR_PACKET))))
                {
                    /* 3.1 Copy Audio Content Information */
                    AvMemcpy(CurrentPort->content.audio, port->content.audio, sizeof(AvAudio));
                    /* 3.2 Set AV_PKT_AUDIO_INFO_FRAME */
                    SET_AUDIF_CC(PktContent, CurrentPort->content.audio->ChanNum-1);
                    switch(CurrentPort->content.audio->ChanNum)
                    {
                        case 8:
                        case 7:
                            SET_AUDIF_CA(PktContent, 0x13);
                            break;
                        case 6:
                            SET_AUDIF_CA(PktContent, 0x0B);
                            break;
                        case 5:
                            SET_AUDIF_CA(PktContent, 0x07);
                            break;
                        case 4:
                            SET_AUDIF_CA(PktContent, 0x03);
                            break;
                        case 3:
                            SET_AUDIF_CA(PktContent, 0x01);
                            break;
                        default:
                            break;
                    }
                    AvUapiTxSetPacketContent(CurrentPort, AV_PKT_AUDIO_INFO_FRAME, PktContent,1);
                    /* 3.3 Set AV_PKT_AUDIO_CHANNEL_STATUS */
                    if((CurrentPort->content.audio->AudCoding == AV_AUD_FORMAT_LINEAR_PCM) || (CurrentPort->content.audio->AudCoding == 0x00))
                        CurrentPort->content.audio->Consumer = CurrentPort->content.audio->Consumer & 0x01;
                    else
                        CurrentPort->content.audio->Consumer = CurrentPort->content.audio->Consumer | 0x02;
                    AvUapiTxSetPacketContent(CurrentPort, AV_PKT_AUDIO_CHANNEL_STATUS, PktContent,1);
                    /* 3.4 Set AV_BIT_AUDIO_SAMPLE_PACKET, AV_BIT_ACR_PACKET */
                    AvUapiTxSetAudioPackets(CurrentPort);
                }
            /* 3.5 Clear Tx Audio Fifo Status */
            if(CurrentPort->core.HdmiCore == 0)
                GSV6K5_INT_set_TX1_AUDIOFIFO_FULL_CLEAR(CurrentPort, 1);
        }
        PrevPort = CurrentPort;
        CurrentPort = NULL;
    }
#endif
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckClockGen(pin AvPort *port))
{
    return AvOk;
}

/* eARC Rx Write Capability Data */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiEarcRxWriteCapData(pio AvPort *port, uint8 *Value, uint16 Count))
{
    return AvOk;
}

/* eARC Rx Write Capability Data */
uapi AvRet ImplementUapi(Gsv6k5, AvUapiCheckEarcRx(pio AvPort *port))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiEarcRxGetAudioPacketStatus(pin AvPort *port, EarcAudioInterrupt* Intpt))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiEarcRxClearAudioInterrupt(pio AvPort *port, EarcAudioInterrupt* Intpt))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiEarcRxSetAudioInterface(pio AvPort *port))
{
    return AvOk;
}

uapi AvRet ImplementUapi(Gsv6k5, AvUapiEarcTxSetAudioInterface(pio AvPort *port))
{
    return AvOk;
}

uapi AvRet AvUapiDevMeasureTj(pio AvDevice *device, int32 *Tj)
{
    uint8 value = 0x00;
    AvPort *port = device->port;
    AvHalI2cWriteField8(GSV6K5_PD_MAP_ADDR(port), 0x82, 0xFF, 0, 0x10);
    AvHalI2cWriteField8(GSV6K5_PD_MAP_ADDR(port), 0x80, 0xFF, 0, 0x1E);
    AvHalI2cWriteField8(GSV6K5_PD_MAP_ADDR(port), 0x80, 0xFF, 0, 0x3E);
    AvHalI2cWriteField8(GSV6K5_PD_MAP_ADDR(port), 0x80, 0xFF, 0, 0x3F);
    while((value & 0x80) == 0x00)
        AvHalI2cReadField8(GSV6K5_PD_MAP_ADDR(port), 0x80, 0xFF, 0, &value);
    AvHalI2cReadField8(GSV6K5_PD_MAP_ADDR(port), 0x86, 0xFF, 0, &value);
    (*Tj) = value - 103;

    return AvOk;
}

#undef FindCp1Mode
