/**
 * @file av_main.c
 *
 * @brief sample main entry for audio/video based software
 */
#include "av_main.h"
#include "global_var.h"
#include "SEGGER_RTT.h"

AvDevice gsv6k5_devices[Gsv6k5DeviceNumber];
Gsv6k5Device gsv6k5_dev[Gsv6k5DeviceNumber];
AvPort gsv6k5Ports[Gsv6k5DeviceNumber*Gsv6k5PortNumber];

/**
 * @brief  sample main entry for audio/video based software
 * @return never return
 */
#ifdef GSV_BASE_PROJECT
  int main(void)
#else
  int GsvMain(void)
#endif

{
    uint8 i;
    /* 1. Low Level Hardware Level Initialization */
    /* 1.1 init bsp support (user speficic) */
    BspInit();

    /* 1.2 init software package and hookup user's bsp functions */
    AvApiInit();
    AvApiHookBspFunctions(&BspI2cRead, &BspI2cWrite, &BspUartSendByte,
                          &BspUartGetByte, &BspGetMilliSecond, &BspGetKey, &BspIrdaGetByte);
    AvApiHookUserFunctions(&ListenToKeyCommand, &ListenToUartCommand, &ListenToIrdaCommand);

    /* 2. Device Level Declaration */
    /* 2.1 total devices */
    /* it must be declared in AvDevice */
    // AvDevice devices[1];

    /* 2.2 specific devices and ports */
    /* they must be able to be linked to the device in 1. */
    // Gsv6k5Device gsv6k5_0;
    // AvPort gsv6k5Ports[10];

    /* 2.3 init device address in 2.2 */
    for(i=0;i<Gsv6k5DeviceNumber;i++)
      gsv6k5_dev[i].DeviceAddress = AvGenerateDeviceAddress(i+Gsv6k5DeviceIndexOffset,0x01,0xB0,0x00);


    /* 2.4 connect devices to device declaration */
    for(i=0;i<Gsv6k5DeviceNumber;i++)
      AvApiAddDevice(&gsv6k5_devices[i], Gsv6k5, i+Gsv6k5DeviceIndexOffset, (void *)&gsv6k5_dev[i], (void *)&gsv6k5Ports[i*Gsv6k5PortNumber], NULL);

    /* 3. Port Level Declaration */
    /* 3.1 init devices and port structure, must declare in number order */
    /* 0-3 HdmiRx, 4-7 HdmiTx, 8-9 TTLTx, 10-11 TTLRx,
       20-23 Scaler, 24-27 Color, 28 VideoGen, 30 VideoIn, 32 VideoOut,
       34 AudioGen, 36 ClockGen, 40 DpRx */
    for(i=0;i<Gsv6k5DeviceNumber;i++)
    {
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+0] ,0 ,HdmiRx);
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+1] ,4 ,HdmiTx);
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+2] ,5 ,HdmiTx);
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+3] ,6 ,HdmiTx);
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+4] ,7 ,HdmiTx);
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+5] ,20,VideoScaler);
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+6] ,24,VideoColor);
#if AvEnableAudioTTLInput
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+7] ,11,LogicAudioRx);
#endif
#if AvEnableInternalVideoGen
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+8] ,28,VideoGen);
#endif
      AvApiAddPort(&gsv6k5_devices[i],&gsv6k5Ports[i*Gsv6k5PortNumber+9] ,8 ,LogicAudioTx);
    }

    /* 3.2 initialize port content */
#if AvEnableCecFeature
    gsv6k5Ports[1].content.cec->CecEnable = 1;
#if (AvEnableCecCts == 0)
    if(AudioStatus == 0)
        gsv6k5Ports[1].content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
    #if (AvEnableEArcRxFeature == 0)
    else
    {
        gsv6k5Ports[1].content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
        gsv6k5Ports[1].content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
    }
    #endif
#endif
    CecTxAudioStatus.Volume = 30;
    CecTxAudioStatus.Mute   = 0;    /*  */
    CecTxAudioStatus.AudioMode = 1; /* Audio Mode is ON to meet ARC */
    CecTxAudioStatus.AudioRate = 1; /* 100% rate */
    CecTxAudioStatus.AudioFormatCode = AV_AUD_FORMAT_LINEAR_PCM; /* Follow Spec */
    CecTxAudioStatus.MaxNumberOfChannels = 2; /* Max Channels */
    CecTxAudioStatus.AudioSampleRate = 0x07; /* 32KHz/44.1KHz/48KHz */
    CecTxAudioStatus.AudioBitLen = 0x01;  /* 16-bit only */
    CecTxAudioStatus.MaxBitRate  = 0;  /* default */
    CecTxAudioStatus.ActiveSource = 0; /* default */
#endif

    /* 3.3 init fsms */
//    for(i=0;i<Gsv6k5DeviceNumber;i++)
//      AvApiInitDevice(&gsv6k5_devices[i]);

    AvApiInitDevice(&gsv6k5_devices[1]);
    AvApiInitDevice(&gsv6k5_devices[0]);

    AvApiPortStart();

    /* 3.4 routing */
    /* connect the port by video using AvConnectVideo */
    /* connect the port by audio using AvConnectAudio */
    /* connect the port by video and audio using AvConnectAV */

    /* 3.4.1 video routing */
    /* case 1: default routing RxA->TxB */
    for(i=0;i<Gsv6k5DeviceNumber;i++)
    {
      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+0], &gsv6k5Ports[i*Gsv6k5PortNumber+1], AvConnectAV);
      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+0], &gsv6k5Ports[i*Gsv6k5PortNumber+2], AvConnectAV);
      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+0], &gsv6k5Ports[i*Gsv6k5PortNumber+3], AvConnectAV);
      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+0], &gsv6k5Ports[i*Gsv6k5PortNumber+4], AvConnectAV);
      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+0], &gsv6k5Ports[i*Gsv6k5PortNumber+7], AvConnectAudio);
    }

    /* 3.4.2 ARC Connection, set after rx port connection to avoid conflict */
#if (AvEnableCecFeature == 1) && (AvEnableEArcRxFeature == 0)
    if(AudioStatus == 1)
    {
        AvApiConnectPort(&gsv6k5Ports[2], &gsv6k5Ports[3], AvConnectAudio);
    }
#endif

    /* 3.4.3 Internal Video Generator*/
#if AvEnableInternalVideoGen
    for(i=0;i<Gsv6k5DeviceNumber;i++)
    {
      /* 0x10 1080p60 */
      /* 0x61 4K60  */
      /* 0x5F 4K30  */
      /* 0x60 4K50  */
      /* 0x5E 4K25  */
      /* 0x5D 4K24  */
      /* 0x62 4KS24 */
      /* 0x04 720p  */
      /* 0x02 480p  */
      /* 0x11 576p  */
      /* 0x06 480i  */
      /* 0x15 576i  */
      /* 0x05 1080i60 */
      /* 0x14 1080i50 */
      /* 0x27 1080i50s */
      gsv6k5Ports[i*Gsv6k5PortNumber+8].content.video->timing.Vic = 0x61; 
      gsv6k5Ports[i*Gsv6k5PortNumber+8].content.video->AvailableVideoPackets = AV_BIT_AV_INFO_FRAME;
      gsv6k5Ports[i*Gsv6k5PortNumber+8].content.video->Cd         = AV_CD_24;
      gsv6k5Ports[i*Gsv6k5PortNumber+8].content.video->Y          = AV_Y2Y1Y0_RGB;
      gsv6k5Ports[i*Gsv6k5PortNumber+8].content.vg->Pattern       = AV_PT_CHECKBOARD;
//      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+8], &gsv6k5Ports[i*Gsv6k5PortNumber+1], AvConnectVideo);
//      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+8], &gsv6k5Ports[i*Gsv6k5PortNumber+2], AvConnectVideo);
//      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+8], &gsv6k5Ports[i*Gsv6k5PortNumber+3], AvConnectVideo);
//      AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+8], &gsv6k5Ports[i*Gsv6k5PortNumber+4], AvConnectVideo);
    }
#endif

    /* 3.4.4 Audio Insertion */
#if AvEnableAudioTTLInput
    for(i=0;i<Gsv6k5DeviceNumber;i++)
    {
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->AudioMute    = 0;
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->AudFormat    = AV_AUD_I2S;
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->AudType      = AV_AUD_TYPE_ASP;
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->AudCoding    = AV_AUD_FORMAT_DOLBY_DIGITAL;//AV_AUD_FORMAT_LINEAR_PCM;
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->AudMclkRatio = AV_MCLK_256FS;
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->Layout       = 1;    /* 2 channel Layout = 0 */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->Consumer     = 0x02; /* Consumer, 0x00 = LPCM, 0x02 = Compress Audio */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->Copyright    = 0;    /* Copyright asserted */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->Emphasis     = 0;    /* No Emphasis */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->CatCode      = 0;    /* Default */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->SrcNum       = 0;    /* Refer to Audio InfoFrame */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->ChanNum      = 8;    /* Audio Channel Count */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->SampFreq     = AV_AUD_FS_48KHZ; /* Sample Frequency */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->ClkAccur     = 0;    /* Level 2 */
      gsv6k5Ports[i*Gsv6k5PortNumber+7].content.audio->WordLen      = 0x0; /* 24-bit word length */
//    AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+7], &gsv6k5Ports[i*Gsv6k5PortNumber+1], AvConnectAudio);
//    AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+7], &gsv6k5Ports[i*Gsv6k5PortNumber+2], AvConnectAudio);
//    AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+7], &gsv6k5Ports[i*Gsv6k5PortNumber+3], AvConnectAudio);
//    AvApiConnectPort(&gsv6k5Ports[i*Gsv6k5PortNumber+7], &gsv6k5Ports[i*Gsv6k5PortNumber+4], AvConnectAudio);
    }
#endif

    /* 3.4.5 Scaler */
    gsv6k5Ports[5].content.scaler->Hdr2SdrEnable = 1;
#if AvEnableThumbnail
    gsv6k5Ports[5].content.scaler->ThumbnailMode = AV_THUMBNAIL_SLOW;
    gsv6k5Ports[5].content.scaler->TnHout = 32;
    gsv6k5Ports[5].content.scaler->TnVout = 32;
    AvApiConnectPort(&gsv6k5Ports[0], &gsv6k5Ports[5], AvConnectVideo);
#endif
    /* 3.4.6 Color */
    gsv6k5Ports[6].content.color->Enable = 1;
    gsv6k5Ports[6].content.color->Hdr2SdrEnable = 1;

    /* 4. routine */
    /* call update api to enter into audio/video software loop */
  while (1){
    AvApiUpdate();
#if (AvEnableThumbnail == 0)
    for(i=0;i<Gsv6k5DeviceNumber;i++)
      AvPortConnectUpdate(&gsv6k5_devices[i]);
#endif
  #if _DEBUG_GSV6715_EXTI
    if (gsv6715Rising){
      gsv6715Rising = 0;
      SEGGER_RTT_printf(0, RTT_CTRL_TEXT_GREEN "GSV6715 Rising Edge Detected\n" RTT_CTRL_RESET);
    }
    if (gsv6715Falling){
      gsv6715Falling = 0;
      SEGGER_RTT_printf(0, RTT_CTRL_TEXT_GREEN "GSV6715 Falling Edge Detected\n" RTT_CTRL_RESET);
    }
  #endif /* _DEBUG_GSV6715_EXTI */
  }
}
