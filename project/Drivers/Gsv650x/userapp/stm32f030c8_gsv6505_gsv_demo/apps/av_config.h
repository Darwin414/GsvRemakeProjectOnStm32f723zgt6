/**
 * @file av_config.h
 *
 * @brief audio/video software configuration \n
 *        this file shouldn't be renamed when porting as it has been included in other files.
 */

#ifndef __av_config_h
#define __av_config_h

/* ----------------------------- seperate line ------------------------------
 *
 *   Change the following definition/configuration per your hardware/system.
 */

/* type configuration */
typedef unsigned char  uchar;
typedef char           schar;
typedef unsigned char  uint8;
typedef signed char    int8;
typedef unsigned short uint16;
typedef short          int16;
typedef unsigned int   uint32;
typedef int            int32;
#ifndef NULL
#define NULL ((void *)0)
#endif
#ifndef __cplusplus
typedef char           bool;
#define true           1
#define false          0
#endif

#include "av_user_config_input.h"

/* total static buffer, for internal use */
#define AvGlobalStaticBufferSize Gsv6kResourceSize

/* ----------------------------- seperate line ------------------------------
 *
 *   All following are READONLY and DO NOT change when porting.
 */




/* macro to generate device address: {i2c bus address, 16bit flag, i2c device address, i2c page address} */
#define AvGenerateDeviceAddress(BusAddress, Bit16, DevAddress, PageAddress) ((BusAddress<<24) | (Bit16<<16) | (DevAddress<<8) | (PageAddress))
#define AvGetI2cDeviceAddress(x) ((x >> 8) & 0xff)
#define AvGetI2cBusAddress(x)    ((x >> 24) & 0xff)
#define AvGetRegAddress(x)       ((x & 0xff))
#define AvGetRegAddressWidth(x)  ((x >> 16) & 0xff)

/* audio/video events */
typedef enum
{
    AvEventPortAudioInfoChanged,
    AvEventPortVideoInfoChanged,
    AvEventPortUpStreamEncrypted,
    AvEventPortUpStreamDecrypted,
    AvEventUpStreamConnectNewDownStream,
    AvEventPortDownStreamDisconnected,
    AvEventPortDownStreamConnected,
    AvEventPortDownStreamSending,
    AvEventPortUpStreamDisconnected,
    AvEventPortUpStreamConnected,
    AvEventPortDownStreamPowerOff,
    AvEventPortDownStreamPowerOn,
    AvEventPortDpRxChangeDeviceMode,
#if AvEnableCecFeature
    AvEventCecRxMessage,
    AvEventCecTxDone,
    AvEventCecTxTimeout,
    AvEventCecTxArbLost,
    AvEventCecLogAddrAlloc,
    AvEventCecLogAddrList,
    AvEventCecRxMessageRespond,
    AvEventCecTxError,
    AvEventCecArcManage,
    AvEventCecDefaultManage,

    AvEventCecSendRoutingChange,
    AvEventCecSendRoutingInformation,
    AvEventCecSendReportPhysAddress,
    AvEventCecSendActiveSource,
    AvEventCecSendInactiveSource,
    AvEventCecSendFeatureAbort,
    AvEventCecSendPowerStatus,
    AvEventCecSendSetOsdName,
    AvEventCecSendDeviceVendorID,
    AvEventCecSendCecVersion,
    AvEventCecSendFeatureAbortReason,
    AvEventCecSendReqActiveDevice,
    AvEventCecSendSetSystemAudioModeToTv,
    AvEventCecSendSetSystemAudioModeToAll,
    AvEventCecSendReportAudioStatus,
    AvEventCecSendReportSystemAudioModeStatus,
    AvEventCecSendReportShortAudioDecriptor,

    AvEventCecSendGivePhyAddr,
    AvEventCecSendGiveVendorId,
    AvEventCecSendGiveOsdName,
    AvEventCecSendGiveSystemAudioModeStatus,
    AvEventCecSendRequestArcInitiation,
    AvEventCecSendRequestArcTermination,
    AvEventCecSendSystemAudioModeRequest,
    AvEventCecSendRequestShortAudioDescriptor,
    AvEventCecSendReportArcInitiated,
    AvEventCecSendReportArcTerminated,
    AvEventCecSendGiveAudioStatus,

    AvEventMsgRequestCurrentLatency,
    AvEventCecReceiveSystemAudioModeStatus,
    AvEventCecReceiveAudioLatency,
    AvEventCecReceiveSetSystemAudioMode,
    AvEventCecReceiveAudioStatus,
    AvEventCecReceiveSetAudioRate,
    AvEventCecReceiveShortAudioDescriptor,
    AvEventCecSendActiveSourceToAudio,
    AvEventCecSendInitiateARC,
    AvEventCecSendTerminateARC,
    AvEventCecSendMenuStatus,

    AvEventCecMsgRouteChange,
    AvEventCecMsgRouteInfo,
    AvEventCecMsgActiveSource,
    AvEventCecMsgGivePhyAddr,
    AvEventCecMsgSetStreamPath,
    AvEventCecMsgStandby,
    AvEventCecMsgAbort,
    AvEventCecMsgFeatureAbort,
    AvEventCecMsgGivePowerStatus,
    AvEventCecMsgReportPowerStatus,
    AvEventCecMsgGiveOsdName,
    AvEventCecMsgGetVendorId,
    AvEventCecMsgGetCecVersion,
    AvEventCecMsgRequestArcInitiation,
    AvEventCecMsgReportArcInitiated,
    AvEventCecMsgRequestArcTermination,
    AvEventCecMsgReportArcTerminated,
    AvEventCecMsgUserControlPressed,
    AvEventCecMsgUserControlReleased,
    AvEventCecMsgImageViewOn,
    AvEventCecMsgTextViewOn,
    AvEventCecMsgSystemAudioModeRequest,
    AvEventCecMsgDeckControl,
    AvEventCecMsgPlay,
    AvEventCecMsgGiveAudioStatus,
    AvEventCecMsgGiveSystemAudioModeStatus,
    AvEventCecMsgRequestShortAudioDescriptor,
    AvEventCecMsgSystemAudioModeStatus,
    AvEventCecMsgReportCurrentLatency,
    AvEventCecMsgSetSystemAudioMode,
    AvEventCecMsgReportAudioStatus,
    AvEventCecMsgSetAudioRate,
    AvEventCecMsgReportShortAudioDescriptor,
    AvEventCecMsgReportPhyAddr,
    AvEventCecMsgMenuRequest,
    AvEventCecMsgRequestActiveSource,
    AvEventCecMsgSetMenuLanguage,
    AvEventCecMsgDeviceVendorId,
#endif
    AvEventRxSetHdcpStyle,
    AvEventTxSetHdcpStyle,
    AvEventRxPrepareEdid,
    AvEventTxDefaultEdid,
    AvEventPortEdidReadFail,
    AvEventPortEdidReady,
    AvEventRxPortInit,
    AvEventTxPortInit
} AvEvent;

/* define return values of funcitons, including api */
typedef enum
{
    AvNotSupport = -4,
    AvNotAvailable = -3,
    AvInvalidParameter = -2,
    AvError = -1,
    AvOk = 0
} AvRet;

typedef enum
{
    AvConnectVideo,
    AvConnectAudio,
    AvConnectAV
} AvConnectType;

/* define parameter direction of internal functions */
#ifdef pin
#undef pin
#endif
#define pin
#ifdef pout
#undef pout
#endif
#define pout
#ifdef pio
#undef pio
#endif
#define pio

/* define endian */
typedef enum
{
    AvLittleEndian = 0,
    AvBigEndian = 1
} AvEndian;

/* all supported devices in this software */
typedef enum
{
    Gsv1k,
    Gsv2k1,
    Gsv2k2,
    Gsv2k6,
    Gsv2k8,
    Gsv2k11,
    Gsv5k1,
    Gsv6k5,
    Gsv6k7
} AvDeviceType;

/* General device structure */
typedef struct
{
    AvDeviceType type;               /**< device specific type   */
    uint8 index;                     /**< device index in system */
    void *specific;                  /**< device specific config */
    void *port;                      /**< device first port      */
    void *extension;                 /**< device extension       */
} AvDevice;

/* Layer 5 */

/* Video/Audio/Hdcp Content */

/* packet definition for Packet Management */
/* receive bit-mapped flags indicating which packets being received or changed, ORed together
   Although the bits are divided, video and audio can still be separated into different variables
 */
typedef enum{
    AV_PKT_AV_INFO_FRAME        = 0,
    AV_PKT_AUDIO_INFO_FRAME     = 1,
    AV_PKT_ACP_PACKET           = 2,
    AV_PKT_SPD_PACKET           = 3,
    AV_PKT_ISRC1_PACKET         = 4,
    AV_PKT_ISRC2_PACKET         = 5,
    AV_PKT_GMD_PACKET           = 6,
    AV_PKT_GC_PACKET            = 7,
    AV_PKT_MPEG_PACKET          = 8,
    AV_PKT_VS_PACKET            = 9,
    AV_PKT_AUDIO_CHANNEL_STATUS = 10,
    AV_PKT_AUDIO_SAMPLE_PACKET  = 11,
    AV_PKT_ACR_PACKET           = 12,
    AV_PKT_EXT_AV_INFO_FRAME    = 13,
    AV_PKT_HDR_PACKET           = 14,
    AV_PKT_EMP_PACKET           = 15,
    AV_PKT_SPARE3_PACKET        = 16,
    AV_PKT_SPARE4_PACKET        = 17,
    AV_PKT_AMD_PACKET           = 18,
    AV_PKT_ALL_PACKETS          = 19,
    AV_PKT_EARC_UBIT            = 20,
    AV_PKT_EARC_VBIT            = 21,
    AV_PKT_EARC_CBIT            = 22,
    AV_PKT_EARC_AUDIF           = 23,
    AV_PKT_EARC_AUDMUTE         = 24,
    AV_PKT_EARC_AUDFMT          = 25
}PacketType;

#define AV_BIT_AV_INFO_FRAME            0x00000001
#define AV_BIT_AUDIO_INFO_FRAME         0x00000002
#define AV_BIT_ACP_PACKET               0x00000004
#define AV_BIT_SPD_PACKET               0x00000008
#define AV_BIT_ISRC1_PACKET             0x00000010
#define AV_BIT_ISRC2_PACKET             0x00000020
#define AV_BIT_GMD_PACKET               0x00000040
#define AV_BIT_GC_PACKET                0x00000080
#define AV_BIT_MPEG_PACKET              0x00000100
#define AV_BIT_VS_PACKET                0x00000200
#define AV_BIT_AUDIO_CHANNEL_STATUS     0x00000400
#define AV_BIT_AUDIO_SAMPLE_PACKET      0x00000800
#define AV_BIT_ACR_PACKET               0x00001000
#define AV_BIT_HDR_PACKET               0x00002000
#define AV_BIT_EMP_PACKET               0x00004000
#define AV_BIT_AMD_PACKET               0x00008000
#define AV_BIT_SPARE3_PACKET            0x00010000
#define AV_BIT_SPARE4_PACKET            0x00020000

#define AV_BIT_FEAT_SCDC                (1<<0)
#define AV_BIT_FEAT_RR                  (1<<1)
#define AV_BIT_FEAT_30B420              (1<<2)
#define AV_BIT_FEAT_36B420              (1<<3)
#define AV_BIT_FEAT_B340MSCR            (1<<4)
#define AV_BIT_FEAT_6G                  (1<<5)
#define AV_BIT_FEAT_4G5                 (1<<6)
#define AV_BIT_FEAT_3G75                (1<<7)
#define AV_BIT_FEAT_3G                  (1<<8)
#define AV_BIT_FEAT_2G25                (1<<9)
#define AV_BIT_FEAT_1G5                 (1<<10)
#define AV_BIT_FEAT_HDR                 (1<<11)
#define AV_BIT_FEAT_3D                  (1<<12)
#define AV_BIT_FEAT_4K50_420            (1<<13)
#define AV_BIT_FEAT_4K60_420            (1<<14)
#define AV_BIT_FEAT_4KS50_420           (1<<15)
#define AV_BIT_FEAT_4KS60_420           (1<<16)
#define AV_BIT_FEAT_YUV_601             (1<<17)
#define AV_BIT_FEAT_YUV_709             (1<<18)
#define AV_BIT_FEAT_RGB_2020            (1<<19)
#define AV_BIT_FEAT_YCC_2020            (1<<20)
#define AV_BIT_FEAT_YUV_FULL_RANGE      (1<<21)
#define AV_BIT_FEAT_RGB_FULL_RANGE      (1<<22)
#define AV_BIT_FEAT_YUV_422             (1<<25)
#define AV_BIT_FEAT_12B_DEEP_COLOR      (1<<26)
#define AV_BIT_FEAT_10B_DEEP_COLOR      (1<<27)
#define AV_BIT_FEAT_COMPRESS_AUDIO      (1<<28)
#define AV_BIT_FEAT_4K_144              (1<<29)
#define AV_BIT_FEAT_SCRMB               (1<<30)
#define AV_BIT_FEAT_LEGACY_420_4K60     (1<<31)

#define AV_BIT_H2P1_FRL_SUPPORT         0x003F
#define AV_BIT_H2P1_FRL_12G4            (1<<0)
#define AV_BIT_H2P1_FRL_10G4            (1<<1)
#define AV_BIT_H2P1_FRL_8G4             (1<<2)
#define AV_BIT_H2P1_FRL_6G4             (1<<3)
#define AV_BIT_H2P1_FRL_6G3             (1<<4)
#define AV_BIT_H2P1_FRL_3G3             (1<<5)
#define AV_BIT_H2P1_DSC                 (1<<6)
#define AV_BIT_H2P1_VRR                 (1<<7)
#define AV_BIT_H2P1_FVA                 (1<<8)

#define AV_BIT_DPFT_FREESYNC_TIER1      (1<<0)
#define AV_BIT_DPFT_FREESYNC_PRIME      (1<<1)
#define AV_BIT_DPFT_SPECIAL_VENDOR      (1<<2)
#define AV_BIT_DPFT_SPECIAL_VENDOR_2    (1<<3)

#define AV_BCAPS_RECEIVER_MODE          0x81
#define AV_BCAPS_REPEATER_MODE          0xC1
#define AV_BSTATUS_RECEIVER_MODE        0x0000
#define AV_BSTATUS_REPEATER_MODE        0x0101

#define AV_BIT_DP_MST_EN                (0x01 << 0)//BIT(0)
#define AV_BIT_DP_DSC_EN                (0x01 << 1)//BIT(1)
#define AV_BIT_DP_FREESYNC_EN           (0x01 << 2)//BIT(2)
#define AV_BIT_DP_ADAPTIVESYNC_EN       (0x01 << 3)//BIT(3)
#define AV_BIT_DP_AUX_PN_SWAP           (0x01 << 4)//BIT(4)
#define AV_BIT_DP_LANE_PN_SWAP          (0x01 << 5)//BIT(5)
#define AV_BIT_DP_VSC_EXT_VESA          (0x01 << 6)//BIT(6)
#define AV_BIT_DP_RE_CALC               (0x01 << 7)//BIT(7)
#define AV_BIT_DP_IGNOR_MSA             (0x01 << 8)//BIT(8)
#define AV_BIT_DP_BRANCH                (0x01 << 9)//BIT(9)
#define AV_BIT_DP_SRC_OUI_AMD           (0x01 << 10)//BIT(10)
#define AV_BIT_DP_LEGACY_SINK           (0x01 << 11)//BIT(11)
#define AV_BIT_DP_SPECIAL_DFP           (0x01 << 12)//BIT(12)
#define AV_BIT_DP_LINK_CG               (0x01 << 13)//BIT(13)

#define AV_BIT_DP_KEEP_POLLING          (0x01 << 0)//BIT(0)

#define AV_BIT_DSC_10_BPC               (0x01 << 0)
#define AV_BIT_DSC_12_BPC               (0x01 << 1)
#define AV_BIT_DSC_16_BPC               (0x01 << 2)
#define AV_BIT_DSC_ALL_BPC              (0x01 << 3)
#define AV_BIT_DSC_NATIVE_420           (0x01 << 6)
#define AV_BIT_DSC_REV_1P2              (0x01 << 7)

/* Predefined enumerations */
typedef enum{
    AV_EDID_RESET   = 0,
    AV_EDID_SUCCESS = 1,
    AV_EDID_FAIL    = 2,
    AV_EDID_NEEDUPDATE = 3,
    AV_EDID_RESEND  = 4,
    AV_EDID_UPDATED = 5
} EdidStat;

#if AvStrictEdidRule
    #define AV_EDID_SINKLOST  AV_EDID_NEEDUPDATE
#else
    #define AV_EDID_SINKLOST  AV_EDID_UPDATED
#endif

typedef enum{
    AV_HDCP_ERR_NO_ERROR = 0,
    AV_HDCP_ERR_BAD_RECV_BKSV = 1,
    AV_HDCP_ERR_RI_MISMATCH = 2,
    AV_HDCP_ERR_PJ_MISMATCH = 3,
    AV_HDCP_ERR_I2C_ERROR = 4,
    AV_HDCP_ERR_REP_DONE_TIMEOUT = 5,
    AV_HDCP_ERR_MAX_CASCADE_EXCEEDED = 6,
    AV_HDCP_ERR_V_DASH_CHECK_FAILED = 7,
    AV_HDCP_ERR_MAX_DEVICE_EXCEEDED = 8
} HdcpErr;

typedef enum{
    AV_HDCP_RX_NOT_SUPPORT = 0,
    AV_HDCP_RX_AUTO        = 1,
    AV_HDCP_RX_FOLLOW_SINK = 2,
    AV_HDCP_RX_1P4_ONLY    = 3,
    AV_HDCP_RX_2P2_ONLY    = 4,
} HdcpRxSupportType;

typedef enum{
    AV_HDCP_TX_ILLEGAL_NO_HDCP = 0,
    AV_HDCP_TX_AUTO            = 1,
    AV_HDCP_TX_AUTO_FAIL_OUT   = 2,
    AV_HDCP_TX_1P4_ONLY        = 3,
    AV_HDCP_TX_1P4_FAIL_OUT    = 4,
    AV_HDCP_TX_2P2_ONLY        = 5,
    AV_HDCP_TX_2P2_FAIL_OUT    = 6
} HdcpTxSupportType;

typedef enum{
    AV_HDCP_TX_DVI_STRICT      = 0,
    AV_HDCP_TX_DVI_LOOSE       = 1
} DviTxSupportType;

typedef enum
{
    AV_AUD_IN_I2S0,
    AV_AUD_IN_I2S1,
    AV_AUD_IN_I2S2,
    AV_AUD_IN_I2S3,
    AV_AUD_IN_I2S,
    AV_AUD_IN_SPDIF,
    AV_AUD_IN_DSD0,
    AV_AUD_IN_DSD1,
    AV_AUD_IN_DSD2,
    AV_AUD_IN_DSD3,
    AV_AUD_IN_DSD4,
    AV_AUD_IN_DSD5,
    AV_AUD_IN_DSD6,
    AV_AUD_IN_DSD7,
    AV_AUD_IN_DSD,
    AV_AUD_IN_DST,
    AV_AUD_IN_ALL
} TxAudioInterface;
/* TX_AUD_INTERFACE */

typedef enum{
    AV_AUD_I2S = 1,
    AV_AUD_I2S_LJ,
    AV_AUD_I2S_RJ,
    AV_AUD_I2S_AES3,
    AV_AUD_I2S_SPDIF,
    AV_AUD_SPDIF,
    AV_AUD_DSD_NORM,
    AV_AUD_DSD_SDIF3,
    AV_AUD_DSD_DST,
    AV_AUD_DSD_DST_SDR,
    AV_AUD_DSD_DST_DDR,
    AV_AUD_IF_AUTO
} AvAudioFormat;
/* TX_AUD_FORMAT */

typedef enum{
    AV_MCLK_128FS = 0,
    AV_MCLK_256FS = 1,
    AV_MCLK_384FS = 2,
    AV_MCLK_512FS = 3,
} AvAudioMclk;

typedef enum
{
    AV_CH0_LEFT=0,             /* Sequential value is used */
    AV_CH0_RIGHT,
    AV_CH1_LEFT,
    AV_CH1_RIGHT,
    AV_CH2_LEFT,
    AV_CH2_RIGHT,
    AV_CH3_LEFT,
    AV_CH3_RIGHT
} TxAudioChannel;
/* TX_AUD_CHAN */

typedef enum{
    AV_AUD_TYPE_AUTO  = 0x10,
    AV_AUD_TYPE_ASP   = 0,
    AV_AUD_TYPE_SPDIF = 1,
    AV_AUD_TYPE_HBR   = 3,
    AV_AUD_TYPE_DSD   = 2,
    AV_AUD_TYPE_DST   = 4,
    AV_AUD_TYPE_UNKNOWN = 0xff
} AvAudioType;

typedef enum
{
    AV_AUD_FORMAT_LINEAR_PCM            = 1,
    AV_AUD_FORMAT_AC3                   = 2,
    AV_AUD_FORMAT_MPEG1                 = 3,
    AV_AUD_FORMAT_MP3                   = 4,
    AV_AUD_FORMAT_MPEG2                 = 5,
    AV_AUD_FORMAT_AAC                   = 6,
    AV_AUD_FORMAT_DTS                   = 7,
    AV_AUD_FORMAT_ATRAC                 = 8,
    AV_AUD_FORMAT_ONE_BIT_AUDIO         = 9,
    AV_AUD_FORMAT_DOLBY_DIGITAL         = 10,
    AV_AUD_FORMAT_DTS_HD                = 11,
    AV_AUD_FORMAT_MAT                   = 12,
    AV_AUD_FORMAT_DST                   = 13,
    AV_AUD_FORMAT_WMA_PRO               = 14,
    AV_AUD_FORMAT_ID_MAX_SIZE           = 15,
    AV_AUD_FORMAT_COMPRESSED_AUDIO      = 0xff
} AvAudioCoding;

typedef enum
{
    AV_AUD_FS_32KHZ = 0,
    AV_AUD_FS_44KHZ = 1,
    AV_AUD_FS_48KHZ = 2,
    AV_AUD_FS_88KHZ = 3,
    AV_AUD_FS_96KHZ = 4,
    AV_AUD_FS_176KHZ = 5,
    AV_AUD_FS_192KHZ = 6,
    AV_AUD_FS_HBR    = 7,
    AV_AUD_FS_FROM_STRM = 8
} AvAudioSampleFreq;
/* TX_AUD_FS */

typedef enum
{
    AV_CD_NOT_INDICATED = 0,
    AV_CD_24 = 24,
    AV_CD_30 = 30,
    AV_CD_36 = 36,
    AV_CD_48 = 48,
    AV_CD_AUTO = 0xff
} AvVideoCd;

typedef enum
{
    AV_Y2Y1Y0_RGB = 0,                   /* Color space in Y2Y1Y0 of AV Infoframe */
    AV_Y2Y1Y0_YCBCR_422 = 1,
    AV_Y2Y1Y0_YCBCR_444 = 2,
    AV_Y2Y1Y0_YCBCR_420 = 3,
    AV_Y2Y1Y0_INVALID = 15
} AvVideoY;

typedef enum
{
    AV_CS_DEFAULT_RGB       = (0<<6)+0,         // 0x00
    AV_CS_RGB               = (2<<6)+0,         // 0x80
    AV_CS_YUV_601           = (1<<6)+1,         // 0x41
    AV_CS_YUV_709           = (1<<6)+2,         // 0x42
    AV_CS_YCC_601           = (1<<6)+3+(0<<2),  // 0x43
    AV_CS_YCC_709           = (1<<6)+3+(1<<2),  // 0x47
    AV_CS_SYCC_601          = (1<<6)+3+(2<<2),  // 0x4B
    AV_CS_ADOBE_YCC_601     = (1<<6)+3+(3<<2),  // 0x4F
    AV_CS_ADOBE_RGB         = (1<<6)+3+(4<<2),  // 0x53
    AV_CS_BT2020_YCC        = (1<<6)+3+(5<<2),  // 0x57
    AV_CS_BT2020_DFT_RGB    = (0<<6)+3+(6<<2),  // 0x1B
    AV_CS_BT2020_RGB        = (2<<6)+3+(6<<2),  // 0x9B
    AV_CS_LIM_RGB           = (1<<6)+0,         // 0x40
    AV_CS_LIM_YUV_601       = (0<<6)+1,         // 0x01
    AV_CS_LIM_YUV_709       = (0<<6)+2,         // 0x02
    AV_CS_LIM_YCC_601       = (0<<6)+3+(0<<2),  // 0x03
    AV_CS_LIM_YCC_709       = (0<<6)+3+(1<<2),  // 0x07
    AV_CS_LIM_SYCC_601      = (0<<6)+3+(2<<2),  // 0x0B
    AV_CS_LIM_ADOBE_YCC_601 = (0<<6)+3+(3<<2),  // 0x0F
    AV_CS_LIM_ADOBE_RGB     = (0<<6)+3+(4<<2),  // 0x13
    AV_CS_LIM_BT2020_YCC    = (0<<6)+3+(5<<2),  // 0x17
    AV_CS_LIM_BT2020_RGB    = (1<<6)+3+(6<<2),  // 0x5B
    AV_CS_AUTO = 255
} AvVideoCs;

typedef enum
{
    AV_AR_NOT_INDICATED = 0,
    AV_AR_4_3,
    AV_AR_16_9,
    AV_AR_64_27,
    AV_AR_256_135,
    AV_AR_INVALID
} AvVideoAspectRatio;

typedef enum
{
    AvVid3DStructureFramePacking,
    AvVid3DStructureFieldAlternative,
    AvVid3DStructureLineAlternative,
    AvVid3DStructureSideBySideFull,
    AvVid3DStructureLPlusDepth,
    AvVid3DStructureLPlusDepthPlusGraphicsPlusGraphicsDepth,
    AvVid3DStructureTopAndBottom,
    AvVid3DReserved1,
    AvVid3DStructureSideBySideHalf = 8,
    AvVid3DNone
}Vid3DFormat;

typedef enum{
    AV_HPD_LOW          = 0,
    AV_HPD_HIGH         = 1,
    AV_HPD_FORCE_LOW    = 2,
    AV_HPD_FORCE_HIGH   = 3,
    AV_HPD_RELEASE_LOW  = 4,
    AV_HPD_TOGGLE       = 5
} AvHpdState;

typedef enum{
    AV_PT_BLACK      = 0,
    AV_PT_CHECKBOARD = 1,
    AV_PT_STRIP      = 2,
    AV_PT_RED        = 3,
    AV_PT_GREEN      = 4,
    AV_PT_BLUE       = 5,
    AV_PT_WHITE      = 6,
    AV_PT_RAMP       = 8,
    AV_PT_RED_RAMP   = 10,
    AV_PT_GREEN_RAMP = 12,
    AV_PT_BLUE_RAMP  = 14,
    AV_PT_PRBS       = 16,
    AV_PT_COLOR_BAR  = 31
} AvVideoPattern;

typedef enum{
    AV_CG_MANUAL     = 0,
    AV_CG_BIPHASE    = 1,
    AV_CG_AUDIOCLK   = 2,
    AV_CG_RX1        = 3
} AvClockGenType;

typedef enum{
    AV_FRL_12G4L     = 6,
    AV_FRL_10G4L     = 5,
    AV_FRL_8G4L      = 4,
    AV_FRL_6G4L      = 3,
    AV_FRL_6G3L      = 2,
    AV_FRL_3G3L      = 1,
    AV_FRL_NONE      = 0
} AvFrlType;

typedef enum{
    AV_FRL_TRAIN_NO_TIMEOUT = 5,
    AV_FRL_TRAIN_FORCE_PASS = 4,
    AV_FRL_TRAIN_FAIL       = 3,
    AV_FRL_TRAIN_SUCCESS    = 2,
    AV_FRL_TRAIN_ONGOING    = 1,
    AV_FRL_TRAIN_IDLE       = 0
} AvFrlStat;

typedef enum{
    AV_FRL_RATE_DOWNGRADE   = 0,
    AV_FRL_RATE_LOOP_FFE    = 1,
    AV_FRL_RATE_FIXED       = 2,
    AV_FRL_LTP0_ONLY        = 3
} AvFrlMode;

typedef enum{
    AV_FRL_POLL_IDLE     = 0,
    AV_FRL_POLL_INIT     = 1,
    AV_FRL_POLL_STAT     = 2,
    AV_FRL_POLL_CEDR     = 3,
    AV_FRL_POLL_CEDD     = 4,
    AV_FRL_POLL_CLEAR    = 5,
    AV_FRL_POLL_READY    = 6
} AvFrlPoll;

typedef enum{
    AV_EARCRX_EN_DISABLED   = 0,
    AV_EARCRX_EN_ENABLED    = 1,
    AV_EARCRX_EN_TO_DISABLE = 2,
    AV_EARCRX_EN_TO_ENABLE  = 3
} EarxEnable;

typedef enum{
    AV_EARCRX_CAP_RESET   = 0,
    AV_EARCRX_CAP_SUCCESS = 1,
    AV_EARCRX_CAP_FAIL    = 2,
    AV_EARCRX_CAP_NEEDUPDATE = 3,
    AV_EARCRX_CAP_RESEND  = 4,
    AV_EARCRX_CAP_UPDATED = 5
} EarxCapStat;

typedef enum{
    AV_EARCRX_ST_LINK_IDLE     = 0,
    AV_EARCRX_ST_LINK_FAIL     = 1,
    AV_EARCRX_ST_LINK_SUCCESS  = 2,
    AV_EARCRX_ST_DISC_TIMEOUT  = 3,
    AV_EARCRX_ST_HBT_LOST      = 4
} EarxLink;

typedef enum{
    AV_EARC_RAW_LPCM_STEREO     = 0x0000, /* [5:0] = 000_0_00*/
    AV_EARC_RAW_LPCM_MULTI_CH2  = 0x0020, /* [47:44] = 0000, [5:0] = 100_0_00*/
    AV_EARC_RAW_LPCM_MULTI_CH8  = 0x7020, /* [47:44] = 0111, [5:0] = 100_0_00*/
    AV_EARC_RAW_LPCM_MULTI_CH16 = 0xB020, /* [47:44] = 1011, [5:0] = 100_0_00*/
    AV_EARC_RAW_LPCM_MULTI_CH32 = 0x3020, /* [47:44] = 0011, [5:0] = 100_0_00*/
    AV_EARC_RAW_FORAMT_DSD_CH6  = 0x5030, /* [47:44] = 0101, [5:0] = 110_0_00 */
    AV_EARC_RAW_FORAMT_DSD_CH12 = 0x9030, /* [47:44] = 1001, [5:0] = 110_0_00 */
    AV_EARC_RAW_COMPRESSED_A    = 0x0002, /* [47:44] = 0000, [5:0] = 000_0_10*/
    AV_EARC_RAW_COMPRESSED_B    = 0x7002, /* [47:44] = 0111, [5:0] = 000_0_10*/
    AV_EARC_ENC_COMPRESSED_A    = 0x000A, /* [47:44] = 0000, [5:0] = 001_0_10*/
    AV_EARC_ENC_COMPRESSED_B    = 0x700A, /* [47:44] = 0111, [5:0] = 001_0_10*/
    AV_EARC_ENC_LPCM_MULTI_CH2  = 0x002A, /* [47:44] = 0000, [5:0] = 101_0_10*/
    AV_EARC_ENC_LPCM_MULTI_CH8  = 0x702A, /* [47:44] = 0111, [5:0] = 101_0_10*/
    AV_EARC_ENC_LPCM_MULTI_CH16 = 0xB02A, /* [47:44] = 1011, [5:0] = 101_0_10*/
    AV_EARC_ENC_LPCM_MULTI_CH32 = 0x302A, /* [47:44] = 0011, [5:0] = 101_0_10*/
    AV_EARC_ENC_FORMAT_DSD_CH6  = 0x503A, /* [47:44] = 0101, [5:0] = 111_0_10 */
    AV_EARC_ENC_FORMAT_DSD_CH12 = 0x903A  /* [47:44] = 1001, [5:0] = 111_0_10 */
} EarcFmt;

typedef enum
{
    AV_THUMBNAIL_DISABLE   = 0,
    AV_THUMBNAIL_SLOW      = 1,
    AV_THUMBNAIL_FAST      = 2
} AvThumbNailMode;

#if AvEnableDisplayPortRx
typedef enum {
    DP_BW_UNKNOWN  = 0x00,
    DP_BW_RBR      = 0x06,     // 1.62G
    DP_BW_HBR      = 0x0A,     // 2.7G
    DP_BW_HBR2     = 0x14,     // 5.4G
    DP_BW_HBR3     = 0x1E,     // 8.1G
} AvDpBandWidth;

typedef enum {
    DP_LANE_UNKNOWN = 0x00,     // default unknown
    DP_LANE_1       = 0x01,     // lane 0
    DP_LANE_2       = 0x02,     // lane 0, 1
    DP_LANE_4       = 0x04,     // lane 0, 1, 2, 3
} AvDpLanCnt;

typedef enum {
    DP_VER_1_0  = 0x10,
    DP_VER_1_1  = 0x11,
    DP_VER_1_2  = 0x12,
    DP_VER_1_3  = 0x13,
    DP_VER_1_4  = 0x14,
} AvDpVersion;

typedef enum {
    AUX_I2C_1K      = 0x01,
    AUX_I2C_5K      = 0x02,
    AUX_I2C_10K     = 0x04,
    AUX_I2C_100K    = 0x08,
    AUX_I2C_400K    = 0x10,     /* recommand 400khz */
    AUX_I2C_1M      = 0x20,
} AvAuxI2CSpeed;

typedef enum {
    AUX_RD_DELAY_400US  = 0x00,
    AUX_RD_DELAY_4MS    = 0x01,
    AUX_RD_DELAY_8MS    = 0x02,
    AUX_RD_DELAY_12MS   = 0x03,
    AUX_RD_DELAY_16MS   = 0x04,     /* always choose max delay */
} AvAuxRdInterval;

typedef enum {
    DPRX_HPD_LOW,
    DPRX_HPD_HIGH,
    DPRX_HPD_IRQ,
} AvDpRxHpdState;

typedef enum {
    SDP_AUDIO_TIMESTAMP = 0,    //0x01,
    SDP_AUDIO_STREAM    = 1,    //0x02,
    SDP_EXTENSION       = 2,    //0x04,
    SDP_AUDIO_CPY_MANAG = 3,    //0x05,
    SDP_ISRC            = 4,    //0x06,
    SDP_VSC             = 5,    //0x07,
    SDP_PPS             = 6,    //0x10,
    SDP_VSC_EXT_VESA    = 7,    //0x20,
    SDP_VSC_EXT_CTA     = 8,    //0x21,
    SDP_ADP_SYNC        = 9,    //0x22,
    SDP_VDSI            = 10,   //0x81,
    SDP_AVI_INFO        = 11,   //0x82, // no use
    SDP_SPD             = 12,   //0x83,
    SDP_AUDIO_INFO      = 13,   //0x84,
    SDP_AUD_CHN_ST      = 14,   // xxx //0x85,
    SDP_HDR_STATIC      = 15,   //0x87,
    // max is 15
} AvDpSdpType;

typedef enum {
    MISC_INTR_NO_VIDEO  = 0,
    MISC_INTR_AKSV      = 1,
    MISC_INTR_CP        = 2,
    MISC_INTR_NO_AUDIO  = 3,
    MISC_INTR_CS_VALID  = 4,
    MISC_INTR_AUD_MUTE  = 5,
    MISC_INTR_N_CHG     = 6,
    MISC_INTR_FS_CHG    = 7,
    // max is 7
} AvDpMiscIntr;

typedef enum {
    DP_LINK_STREAM_0    = 0,
    DP_LINK_STREAM_1    = 1,
    DP_LINK_STREAM_2    = 2,
    DP_LINK_STREAM_3    = 3,
} AvDpStream;

typedef enum {
    PHYLINK_IDLE        = 0,        /* default PHYLINK bootup state */
    PHYLINK_POWER_OFF   = 1,        /* pull low HPD while phy power off */
    PHYLINK_POWER_ON    = 2,        /* pull high HPD, wait link-training done */
    PHYLINK_WAIT        = 3,        /* wait link training pass */
    PHYLINK_STABLE      = 4,        /* wait link layer stable */
    PHYLINK_READY       = 5,        /* polling status */
} AvDpPhyLinkSt;

typedef enum {
    DP_TPS_NONE,
    DP_TPS_1,
    DP_TPS_2,
    DP_TPS_3,
    DP_TPS_4,
} AvDpTps;

typedef enum {
    DP_LS_CDR,
    DP_LS_EQR,
    DP_LS_ALIGN,
    DP_LS_DONE,
} AvDpLinkState;

typedef enum {
    DP_LT_UNPLUG        = 0,       /* disconnect cable */
    DP_LT_PLUG          = 1,       /* connected */
    DP_LT_HPD_LOW       = 2,
    DP_LT_HPD_HIGH      = 3,
    DP_LT_IDLE          = 4,
    DP_LT_CDR           = 5,       /* TPS_1 */
    DP_LT_EQR           = 6,       /* TPS_N */
    DP_LT_DONE          = 7,       /* LT success */
    DP_LT_FAIL          = 8,       /* LT fail */
} AvDpLtSt;

typedef enum {
    DP_NO_LT_OFF        = 0,       /* unplug->plug wait 500ms interval, turn on by AvDpLtSt state machine */
    DP_NO_LT_OFF_RDY    = 1,       /* unplug->plug total_timer */
    DP_NO_LT_ON         = 2,       /* no-aux during hpd high lager than 800ms */
    DP_NO_LT_TRY        = 3,       /* Try lane count: 1, 2, 4, 1, 2, 4... */
    DP_NO_LT_WAIT       = 4,       /* check Symbol and fifo status, check if aux coming, goto timeout */
    DP_NO_LT_TIMEOUT    = 5,       /* timeout_1 to retry, timeout_2 to turn off no-link training */
    DP_NO_LT_PASS       = 6,
} AvDpNoLtSt;

typedef enum {
    PROCESS_TMDS_MODE   = 0x01,
    PROCESS_FRL_MODE    = 0x04,
} AvDpProcessMode;

    /* audio fsm */
typedef enum {
    DPRX_AUD_NONE         = 0,
    DPRX_AUD_INIT         = 1,
    DPRX_AUD_IDLE         = 2,
    DPRX_AUD_MN_VALID     = 3,
    DPRX_AUD_CALC         = 4,
    DPRX_AUD_CHST_VALID   = 5,
    DPRX_AUD_CONFIG       = 6,
    DPRX_AUD_DONE         = 7,
} AvDpRxAudFsm;

#endif // AvEnableDisplayPortRx define end

#if AvEnableCecFeature /* CEC related */
typedef enum
{
    AV_CEC_MSG_FEATURE_ABORT                  = 0x00,
    AV_CEC_MSG_IMAGE_VIEW_ON                  = 0x04,
    AV_CEC_MSG_TUNER_STEP_INC                 = 0x05,
    AV_CEC_MSG_TUNER_STEP_DEC                 = 0x06,
    AV_CEC_MSG_TUNER_DEV_STATUS               = 0x07,
    AV_CEC_MSG_GIVE_TUNER_DEV_STATUS          = 0x08,
    AV_CEC_MSG_RECORD_ON                      = 0x09,
    AV_CEC_MSG_RECORD_STATUS                  = 0x0A,
    AV_CEC_MSG_RECORD_OFF                     = 0x0B,
    AV_CEC_MSG_TEXT_VIEW_ON                   = 0x0D,
    AV_CEC_MSG_RECORD_TV_SCREEN               = 0x0F,
    AV_CEC_MSG_GIVE_DECK_STATUS               = 0x1A,
    AV_CEC_MSG_DECK_STATUS                    = 0x1B,
    AV_CEC_MSG_SET_MENU_LANGUAGE              = 0x32,
    AV_CEC_MSG_CLR_ANALOGUE_TIMER             = 0x33,
    AV_CEC_MSG_SET_ANALOGUE_TIMER             = 0x34,
    AV_CEC_MSG_TIMER_STATUS                   = 0x35,
    AV_CEC_MSG_STANDBY                        = 0x36,
    AV_CEC_MSG_PLAY                           = 0x41,
    AV_CEC_MSG_DECK_CONTROL                   = 0x42,
    AV_CEC_MSG_TIMER_CLEARED_STATUS           = 0x43,
    AV_CEC_MSG_USER_CONTROL_PRESSED           = 0x44,
    AV_CEC_MSG_USER_CONTROL_RELEASED          = 0x45,
    AV_CEC_MSG_GIVE_OSD_NAME                  = 0x46,
    AV_CEC_MSG_SET_OSD_NAME                   = 0x47,
    AV_CEC_MSG_SET_OSD_STRING                 = 0x64,
    AV_CEC_MSG_SET_TIMER_PROG_TITLE           = 0x67,
    AV_CEC_MSG_SYSTEM_AUDIO_MODE_REQUEST      = 0x70,
    AV_CEC_MSG_GIVE_AUDIO_STATUS              = 0x71,
    AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE          = 0x72,
    AV_CEC_MSG_REPORT_AUDIO_STATUS            = 0x7A,
    AV_CEC_MSG_GIVE_SYSTEM_AUDIO_MODE_STATUS  = 0x7D,
    AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS       = 0x7E,
    AV_CEC_MSG_ROUTE_CHANGE                   = 0x80,
    AV_CEC_MSG_ROUTE_INFO                     = 0x81,
    AV_CEC_MSG_ACTIVE_SRC                     = 0x82,
    AV_CEC_MSG_GIVE_PHYS_ADDR                 = 0x83,
    AV_CEC_MSG_REPORT_PHYS_ADDR               = 0x84,
    AV_CEC_MSG_REQ_ACTIVE_SRC                 = 0x85,
    AV_CEC_MSG_SET_STRM_PATH                  = 0x86,
    AV_CEC_MSG_DEVICE_VENDOR_ID               = 0x87,
    AV_CEC_MSG_VENDOR_COMMAND                 = 0x89,
    AV_CEC_MSG_VENDOR_REMOTE_BTN_DOWN         = 0x8A,
    AV_CEC_MSG_VENDOR_REMOTE_BTN_UP           = 0x8B,
    AV_CEC_MSG_GET_VENDOR_ID                  = 0x8C,
    AV_CEC_MSG_MENU_REQUEST                   = 0x8D,
    AV_CEC_MSG_MENU_STATUS                    = 0x8E,
    AV_CEC_MSG_GIVE_PWR_STATUS                = 0x8F,
    AV_CEC_MSG_REPORT_PWR_STATUS              = 0x90,
    AV_CEC_MSG_GET_MENU_LANGUAGE              = 0x91,
    AV_CEC_MSG_SEL_ANALOGUE_SERVICE           = 0x92,
    AV_CEC_MSG_SEL_DIGITAL_SERVICE            = 0x93,
    AV_CEC_MSG_SET_DIGITAL_TIMER              = 0x97,
    AV_CEC_MSG_CLR_DIGITAL_TIMER              = 0x99,
    AV_CEC_MSG_SET_AUDIO_RATE                 = 0x9A,
    AV_CEC_MSG_INACTIVE_SOURCE                = 0x9D,
    AV_CEC_MSG_CEC_VERSION                    = 0x9E,
    AV_CEC_MSG_GET_CEC_VERSION                = 0x9F,
    AV_CEC_MSG_VENDOR_CMD_WITH_ID             = 0xA0,
    AV_CEC_MSG_CLR_EXTERNAL_TIMER             = 0xA1,
    AV_CEC_MSG_SET_EXTERNAL_TIMER             = 0xA2,
    AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR  = 0xA3,
    AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR = 0xA4,
    AV_CEC_MSG_REQUEST_CURRENT_LATENCY        = 0xA7,
    AV_CEC_MSG_REPORT_CURRENT_LATENCY         = 0xA8,
    AV_CEC_MSG_INITIATE_ARC                   = 0xC0,
    AV_CEC_MSG_REPORT_ARC_INITIATED           = 0xC1,
    AV_CEC_MSG_REPORT_ARC_TERMINATED          = 0xC2,
    AV_CEC_MSG_REQUEST_ARC_INITIATION         = 0xC3,
    AV_CEC_MSG_REQUEST_ARC_TERMINATION        = 0xC4,
    AV_CEC_MSG_TERMINATE_ARC                  = 0xC5,
    AV_CEC_MSG_CDC_MESSAGE                    = 0xF8,
    AV_CEC_MSG_ABORT                          = 0xFF
} CecMessage;

enum
{
    AV_CEC_TX_IDLE           = 0x00,
    AV_CEC_TX_TO_SEND        = 0x01,
    AV_CEC_TX_WAIT_RESPONSE  = 0x02,
    AV_CEC_TX_SEND_SUCCESS   = 0x12,
    AV_CEC_TX_SEND_FAIL      = 0x22
};

/* CEC Logical Address */
enum
{
    AV_CEC_TV_LOG_ADDRESS                   = 0,
    AV_CEC_RECORD1_LOG_ADDRESS              = 1,
    AV_CEC_RECORD2_LOG_ADDRESS              = 2,
    AV_CEC_TUNER1_LOG_ADDRESS               = 3,
    AV_CEC_PLAYBACK1_LOG_ADDRESS            = 4,
    AV_CEC_AUDIO_LOG_ADDRESS                = 5,
    AV_CEC_TUNER2_LOG_ADDRESS               = 6,
    AV_CEC_TUNER3_LOG_ADDRESS               = 7,
    AV_CEC_PLAYBACK2_LOG_ADDRESS            = 8,
    AV_CEC_TUNER4_LOG_ADDRESS               = 10,
    AV_CEC_PLAYBACK3_LOG_ADDRESS            = 11,
    AV_CEC_FREE_LOG_ADDRESS                 = 14,
    AV_CEC_BROADCAST_LOG_ADDRESS            = 15,
    AV_CEC_UNREGISTED_LOG_ADDRESS           = 15
};

enum
{
    AV_CEC_MENU_ACTIVATED                   = 0,
    AV_CEC_MENU_DEACTIVATED                 = 1
};

enum
{
    AV_CEC_ARC_TO_INITIATE                  = 0x01,
    AV_CEC_ARC_INITIATED                    = 0x02,
    AV_CEC_ARC_TO_TERMINATE                 = 0x03,
    AV_CEC_ARC_TERMINATED                   = 0x00
};

enum
{
    AV_CEC_AMP_TO_ENABLE                    = 0x01,
    AV_CEC_AMP_ENABLED                      = 0x02,
    AV_CEC_AMP_TO_DISABLE                   = 0x03,
    AV_CEC_AMP_DISABLED                     = 0x00
};

enum
{
    AV_CEC_POWER_STATUS_ON                  = 0,
    AV_CEC_POWER_STATUS_STNDBY              = 1,
    AV_CEC_POWER_STATUS_STNDBY2ON           = 2, /*  in transition Standby to ON */
    AV_CEC_POWER_STATUS_ON2STNDBY           = 3 /*  in transition ON to Standby */
};

enum
{
    AV_CEC_SYSTEM_AUDIO_STATUS_OFF          = 0,
    AV_CEC_SYSTEM_AUDIO_STATUS_ON           = 1
};

enum
{
    AV_CEC_ABORT_REASON_UNRECOGNIZED_OPCODE   = 0,
    AV_CEC_ABORT_REASON_NOT_IN_CORRECT_MODE   = 1,
    AV_CEC_ABORT_REASON_CANNOT_PROVIDE_SOURCE = 2,
    AV_CEC_ABORT_REASON_INVALID_OPERAND       = 3,
    AV_CEC_ABORT_REASON_REFUSED               = 4,
    AV_CEC_ABORT_REASON_UNABLE_TO_DETERMINE   = 5
};

enum
{
    AV_CEC_AUDIO_MUTE_OFF                     = 0,
    AV_CEC_AUDIO_MUTE_ON                      = 1
};

typedef struct
{
    uchar Volume;
    uchar Mute;
    uchar AudioMode;
    uchar AudioRate;
    uchar AudioFormatCode;
    uchar MaxNumberOfChannels;
    uchar AudioSampleRate;
    uchar AudioBitLen;
    uint16 MaxBitRate;
    uint16 ActiveSource;
    uchar AudioOutputDelay;
    uchar LatencyFlags;
    uchar VideoLatency;
} CecAudioStatus;

#define AV_CEC_OPCODE(a)                   (a[1])
#define AV_CEC_SRC(a)                      ((a[0]&0xf0)>>4)
#define AV_CEC_DST(a)                      (a[0]&0x0f)
#define AV_CEC_BROADCAST(a)                (AV_CEC_DST(a)==0x0f)
#define AV_CEC_PHYS_ADDR1(a)               (((uint16)(a[2])<<8)+(uint16)(a[3]))
#define AV_CEC_PHYS_ADDR2(a)               (((uint16)(a[4])<<8)+(uint16)(a[5]))

#define AV_CEC_SET_HDR_BC(m,pa)            m[0]=(pa<<4)|0x0f
#define AV_CEC_SET_HDR_DA(m,a,pa)          m[0]=(pa<<4)|a
#define AV_CEC_SET_OPCODE(m,o)             m[1]=o
#define AV_CEC_SET_PHYS_ADDR1(m,a)         m[2]=(uchar)((a)>>8);m[3]=(uchar)(a)&0xff
#define AV_CEC_SET_PHYS_ADDR2(m,a)         m[4]=(uchar)((a)>>8);m[5]=(uchar)(a)&0xff

#define AV_FLEX_OP_CODES 4
#define AV_CEC_OP_CODES  120

#endif /* CEC related */

#define TAG_VSDB_IDENTIFIER                0x000C03

extern const uint8  PktSize[26];
extern const uint32 NTable[9];
extern const uchar  NIdx[16];
extern const uchar  ChannelStatusSfTable[];
extern const uint8  ChanCount[32];
extern const AvVideoAspectRatio ARTable[108];
#if AvEnableInternalVideoGen
extern const uchar  VideoGenVicTable[];
#endif

typedef struct
{
    uint8   Aksv[128*5];
} AvKsv;

typedef struct
{
    /* Rx Info */
    uint16      TmdsFreq;
    uint16      LaneFreq;
    uint8       Detect3D;
    Vid3DFormat Format3D;
    uint8       DetectedVic;
} VideoInfo;

typedef struct
{
    uint16      LinesTotal;
    uint16      LinesActive;
    uint16      FrontPorch;
    uint16      SyncPulse;
    uint16      BackPorch;
} VideoSyncInfo;

typedef struct
{
    uint8            Vic;
    uint8            HPolarity;
    uint8            VPolarity;
    uint8            Interlaced;    /* 0= Porg   1= Interl  */
#if AvEnableDetailTiming
    uint16           HActive;
    uint16           VActive;       /* Per frame            */
    uint16           HTotal;
    uint16           VTotal;
    uint16           FrameRate;     /* In Hz                */
    uint16           VFront;
    uint16           VSync;
    uint16           VBack;
    uint16           HFront;
    uint16           HSync;
    uint16           HBack;
#endif
} VideoTiming;

typedef struct
{
    uint8    *addr;
} AvHdmiPhyAddr;

typedef struct
{
    uint8     AvMute;
    uint8     TmdsMute;
    uint8     BlkMute;
} AvVideoMute;

typedef struct
{
    uint8     DeRegenLock;
    uint8     PllLock;
    uint8     VSyncLock;
    uint8     AudioLock;
    uint8     EqLock;
} AvLock;

typedef struct
{
    uint8     AvCks;
    uint8     AudCks;
    uint8     VsCks;
    uint8     SpdCks;
    uint8     MpegCks;
    uint8     HdrCks;
} AvInfoCks;

/* Layer 4 */

typedef struct
{
    AvVideoCd     Cd;
    AvVideoY      Y;
    AvVideoCs     InCs;
    AvVideoCs     OutCs;
    AvVideoAspectRatio AspectRatio;
    AvVideoMute   Mute;
    VideoInfo     info;
    VideoTiming   timing;
    /* Which packets being received/not received */
    uint16        AvailableVideoPackets;
    uint8         ClockMultiplyFactor;
    uint8         PixelRepeatValue;
    uint8         DscStream;
#if (AvEnableDpDscBypass == 1)
    uint16        DscHcactive;
#endif
    uint8         VrrStream;
    uint8         EmpTotal;
} AvVideo;

typedef struct
{
    uint32            NValue;
    uint32            CtsValue;
    //uint16            AudDelayExpire;
    uint16            AvailableAudioPackets;
    uint16            AudLRclk;
    uint8             AudioMute;
    AvAudioFormat     AudFormat;
    AvAudioType       AudType;
    AvAudioCoding     AudCoding;
    AvAudioMclk       AudMclkRatio;
    uint8             Layout;
    /* Channel Status Info */
    uint8             Consumer; /* Top 2 bits */
    uint8             Copyright;
    uint8             Emphasis;
    uint8             CatCode;
    uint8             SrcNum;
    uint8             ChanNum;
    AvAudioSampleFreq SampFreq;
    uint8             ClkAccur;
    uint8             WordLen;
} AvAudio;

typedef struct
{
    /* Key parameter to decide whether HDCP is needed */
    HdcpRxSupportType  HdcpNeeded;
    uint8    SinkNumber;/* according to verified sink number */
    uint8    SinkTotal; /* according to connected sink device */
    uint8    BksvPtr;
    /* Rx Side */
    uint8    AksvInterrupt;
    uint8    HdcpError;
    uint8    Hdcp2p2Flag;
    uint8    Hdcp2p2RxRunning;
} HdcpRx;

typedef struct
{
    /* Tx Side */
    uint8    HdcpSupport;
    uint8    HdcpEnabled;
    HdcpTxSupportType HdmiStyle;
    DviTxSupportType  DviStyle;
    uint8    Authenticated;
    uint8    HdcpModeUpdate;
    uint8    BksvReady;
    uint8    HdcpError;
    uint8    Hdcp2p2Flag;
    uint8    Hdcp2p2SinkSupport;
    uint8    Hdcp2p2TxRunning;
} HdcpTx;

/* different type of port content */
typedef struct {
    /* Part 1, Reset */
    uint8       Reset;
    /* Part 2, FreeRun */
    uint8       EnableFreeRun;
    uint8       ForceFreeRun;
    uint8       IsFreeRun;
    /* Part 3, Connect Status */
    uint8       IsInputStable;
    uint8       VideoEncrypted;
    /* Part 4, Edid */
    EdidStat    EdidStatus;
    /* Part 5, Protocol */
    uint8       Input5V;
    AvHpdState  Hpd;
    uint32      HpdDelayExpire;
    AvLock      Lock;
    uint16      ChangedVideoPackets;
    uint16      ChangedAudioPackets;
    uint8       HdmiMode;
    uint16      EQDelayExpire;
    AvInfoCks   Cks;
    /* Part 6, HDMI 2.1 */
    AvFrlType   H2p1FrlType;
    AvFrlStat   H2p1FrlStat;
    AvFrlMode   H2p1FrlMode;
    uint8       H2p1FrlHblank;
    /* [19:16] = pin mapping, [15:0] = data rate(MHz) */
    uint32      H2p1FrlManual;
    uint16      H2p1FrlLockDelay;
    /* Part 7, Pin Mapping */
    uint32      PinMapping;
} RxVars;

#if AvEnableDisplayPortRx
typedef struct {
    /* Part 1, DisplayPort */
    uint8           CableStat;
    uint8           CableIntr;
    AvDpBandWidth   DpMaxBw;
    AvDpLanCnt      DpMaxLns;
    AvDpBandWidth   DpCurBw;
    AvDpLanCnt      DpCurLns;
    AvDpVersion     DpVer;
    AvDpStream      DpStrNum;
    uint8           Misc0;
    uint8           Misc1;
    uint8           VscCsc0;
    uint8           VscCsc1;
    uint16          StreamClk;
    uint16          AudioFs;
    uint16          FsRange;
    uint8           AuxCount;
    /* Part 2, Gule */
    uint8           LtCnt;
    AvDpProcessMode ProcessMode;
    uint16          DpFlags;
    /* Part 3, Phy Lanes/Pn Swap */
    uint8           PinAssign;
    /* Part 4, FSM */
    AvDpPhyLinkSt   DpPhyLinkSt;
    AvDpLtSt        DpLtSt;
    AvDpNoLtSt      DpNoLtSt;
    AvDpRxAudFsm    DpAudFsm;
    /* Part 5, DPRX INTR */
    uint32          DpRxSdpIntr;
    uint16          DpRxSdpAvailable;
    uint16          DpRxSdpsChanged;
    uint8           DpMiscIntr;
    /* Part 6, Timer */
    uint32          LtTimer; /* phylink fsm timer */
    uint32          PhyLinkTimer;
    uint8           PhyLinkCnt;
    uint16          NltTimer;
    uint32          ActiveTimer;
    uint8           PrevCableStat;
    /* debug use, remove it later */
    uint8           DpRxDebugFlag;
} DpRxVars;
#endif

typedef struct {
    /* Part 2, DVI/HDMI Mode */
    uint8       HdmiMode;
    /* Part 3, Connect Status*/
    AvLock      Lock;
    uint32      InfoReady;
    /* Part 4, Edid */
    uint8       EdidReadFail;
    EdidStat    EdidReadSuccess;
    uint8       IgnoreEdidError;
    uint8       EdidCks[2];
    uint16      PhyAddr;
    uint16      FreeSyncRange;
    uint32      EdidSupportFeature;
    uint32      EdidExtendFeature;
    /* Part 5, Protocol */
    AvHpdState  Hpd;
    uint32      HpdDelayExpire;
    uint16      ChangedVideoPackets;
    uint16      ChangedAudioPackets;
    /* Part 6, HDMI 2.1 */
    uint32      H2p1SupportFeature;
    AvFrlType   H2p1FrlType;
    AvFrlStat   H2p1FrlStat;
    AvFrlPoll   H2p1FrlPoll;
    uint8       H2p1FrlCedRb;
    uint32      H2p1FrlExtraDelay;
#if (AvEnableDpDscBypass == 1)
    uint8       H2p1DscFields;
    uint8       H2p1DscMaxFrl;
    uint8       H2p1DscMaxSlices;
    uint8       H2p1DscMaxChunkKB;
#endif
    /* [19:16] = pin mapping, [15:0] = data rate(MHz) */
    uint32      H2p1FrlManual;
    /* Part 7, Pin Mapping */
    uint32      PinMapping;
} TxVars;

typedef struct {
    /* Enable */
    uint8 Enable;
    uint8 Hdr2SdrEnable; /* Hdr to Sdr enable */
    /* Input Vic */
    uint8 ScalerInVic;
    /* Output Vic */
    uint8 ScalerOutVic;
    /* Detail Timing */
    VideoTiming timing;
    /* Color Space */
    AvVideoY ColorSpace;
    /* Parameters of Output Color Space */
    AvVideoY ScalerOutSpace;
    AvVideoCs ScalerInCs;
    AvVideoCs ScalerOutCs;
    /* Parameters of Output Downscaling */
    uint8 Vratio;
    uint8 Hratio;
    /* Parameters of Thumbnail */
    AvThumbNailMode ThumbnailMode;
    uint16 TnVout;
    uint16 TnHout;
    /* Update Flag */
    uint8 Update;
} AvScaler;

typedef struct {
    /* Enable */
    uint8 Enable; /* 444-420 conversion enable */
    uint8 Hdr2SdrEnable; /* Hdr to Sdr enable */
    /* Input Vic */
    uint8 ColorInVic;
    /* Input ColorSpace */
    AvVideoY ColorInSpace;
    AvVideoCs ColorInCs;
    /* Output ColorSpace */
    AvVideoY ColorOutSpace;
    AvVideoCs ColorOutCs;
    /* Detail Timing */
    VideoTiming   timing;
    /* Update Flag */
    uint8 Update;
} AvColor;

typedef struct {
    /* Enable */
    uint8 Enable;
    /* Pattern */
    AvVideoPattern Pattern;
} AvVideoGen;

typedef struct {
    /* Enable */
    uint8 Enable;
    uint8 Update;
} AvAudioGen;

typedef struct {
    uint8 Update[4];
    AvClockGenType type[4];
    /* Clock Ratio */
    uint8  Integer[4];
    uint32 Fraction[4];
} AvClockGen;

#if AvEnableCecFeature /* CEC Related */
typedef struct {
    /* Part 1, Enable */
    uint8       CecEnable;
    /* Part 2, Interrupt */
    uint8       TxReady;
    uint8       ArbLost;
    uint8       Timeout;
    uint8       RxGetFlag;
    /* Part 3, Tx */
    uint8       TxSendFlag;
    CecMessage  TxMsg;
    uint8       TxLen;
    uint8       TxContent[20];
    uint8       DstAddr;
    uint8       LogAddr;
    /* Part 4, Rx */
    uint8       RxReady;
    CecMessage  RxMsg;
    uint8       RxLen;
    uint8       RxContent[20];
    /* Part 5, Message */
    uint8       AddrIndex;
    uint16      LogicAddrList; /* Define the Logic Addr Table */
    uint8       NackCount;
    uint8       LowDriveCount;
    uint8       RetryCount;
    uint8       InputCount; /* [7:4] = current route, [3:0] = total inputs */
    uint8       EnableARC;
    uint8       EnableAudioAmplifier;
    uint8       ARCTryCount;
    uint32      CecDelayExpire;
    uint32      AmplifierDelayExpire;
} AvCec;
#endif

#if AvEnableEArcRxFeature
typedef struct {
    EarxEnable  Enable;
    uint8       Hpd;
    EarxCapStat CapStat;
    EarxLink    LinkStat;
    EarcFmt     Fmt;
    uint8       EarxAudioLock;
    uint8       Latency;
} EarcRx;
#endif

/* Layer 3 */

typedef struct
{
    uint16    ID;  /* checking id in the table */
    uint8     is_PlugRxFsm;
    uint8     is_active_PlugRxFsm;
    uint8     is_ReceiverFsm;
    uint8     is_active_ReceiverFsm;
    RxVars    rx;
    HdcpRx    hdcp;
    AvVideo   video;
    AvAudio   audio;
    struct AvPort    *HdcpSource;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
    struct AvPort    *RouteAudioFromPort;
    struct AvPort    *RouteAudioNextSameLevelPort;
    struct AvPort    *RouteAudioToPort;
} HdmiRxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    uint8     is_PlugRxFsm;
    uint8     is_active_PlugRxFsm;
    uint8     is_ReceiverFsm;
    uint8     is_active_ReceiverFsm;
    RxVars    rx; /* RX FSM vars */
    AvVideo   video;
    struct AvPort    *HdcpSource;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} DviRxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    uint8     is_ReceiverFsm;
    uint8     is_active_ReceiverFsm;
    RxVars    rx; /* RX FSM vars */
    AvVideo   video;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} AnalogRxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    uint8     is_ReceiverFsm;
    uint8     is_active_ReceiverFsm;
    RxVars    rx; /* RX FSM vars */
    AvVideo   video;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} LogicVideoRxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    uint8     is_ReceiverFsm;
    uint8     is_active_ReceiverFsm;
    AvAudio   audio;
    struct AvPort    *RouteAudioFromPort;
    struct AvPort    *RouteAudioNextSameLevelPort;
    struct AvPort    *RouteAudioToPort;
} LogicAudioRxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    uint8     is_PlugTxFsm;
    uint8     is_active_PlugTxFsm;
    uint8     is_TxRoutingFsm;
    uint8     is_active_TxRoutingFsm;
#if AvEnableCecFeature /* CEC Related */
    uint8     is_CecFsm;
    uint8     is_active_CecFsm;
#endif
    TxVars    tx; /* Tx FSM vars */
    HdcpTx    hdcptx;
    AvVideo   video;
    AvAudio   audio;
#if AvEnableCecFeature /* CEC Related */
    AvCec     cec;
#endif
#if AvEnableEArcRxFeature
    EarcRx    earx;
    AvAudio   eaaud;
#endif
    struct AvPort    *HdcpNextSinkPort;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
    struct AvPort    *RouteAudioFromPort;
    struct AvPort    *RouteAudioNextSameLevelPort;
    struct AvPort    *RouteAudioToPort;
} HdmiTxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    uint8     is_PlugTxFsm;
    uint8     is_active_PlugTxFsm;
    TxVars    tx; /* TX FSM vars */
    AvVideo   video;
    uint8     is_TxRoutingFsm;
    uint8     is_active_TxRoutingFsm;
    struct AvPort    *HdcpNextSinkPort;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} DviTxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    TxVars    tx; /* TX FSM vars */
    AvVideo   video;
    AvAudio   audio;
    uint8     is_TxRoutingFsm;
    uint8     is_active_TxRoutingFsm;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} AnalogTxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    TxVars    tx; /* TX FSM vars */
    AvVideo   video;
    uint8     is_TxRoutingFsm;
    uint8     is_active_TxRoutingFsm;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} LogicVideoTxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    AvAudio   audio;
    uint8     is_TxRoutingFsm;
    uint8     is_active_TxRoutingFsm;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} LogicAudioTxPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    AvScaler  scaler;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} VideoScalerPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    AvColor   color;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} VideoColorPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    RxVars    rx;
    AvVideo   video;
    AvVideoGen  vg;
    struct AvPort    *RouteVideoFromPort;
    struct AvPort    *RouteVideoNextSameLevelPort;
    struct AvPort    *RouteVideoToPort;
} VideoGenPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    AvAudio   audio;
    AvAudioGen  ag;
    struct AvPort    *RouteAudioFromPort;
    struct AvPort    *RouteAudioNextSameLevelPort;
    struct AvPort    *RouteAudioToPort;
} AudioGenPort;

typedef struct
{
    uint16    ID;  /* checking id in the table */
    AvClockGen  cg;
} ClockGenPort;

/* Layer 2 */

/* AvDevice definition can be found in uapi_device_list.h */

typedef struct
{
#ifndef COMPILER_C51_MODE
    uint16    *ID;
    uint8     *is_active_PlugRxFsm;
    uint8     *is_active_ReceiverFsm;
    uint8     *is_active_PlugTxFsm;
    uint8     *is_active_TxRoutingFsm;
#endif
    uint8     *is_PlugRxFsm;
    uint8     *is_ReceiverFsm;
    uint8     *is_PlugTxFsm;
    uint8     *is_TxRoutingFsm;
#if AvEnableCecFeature /* CEC Related */
    uint8     *is_CecFsm;
    uint8     *is_active_CecFsm;
#endif
    RxVars    *rx; /* Rx FSM vars */
    TxVars    *tx; /* Tx FSM vars */
    HdcpRx    *hdcp; /* Hdcp Rx vars */
    HdcpTx    *hdcptx; /* Hdcp Tx vars */
    AvVideo   *video;
    AvAudio   *audio;
    AvScaler  *scaler;
    AvColor   *color;
#if AvEnableDisplayPortRx
    DpRxVars  *dprx;
#endif
#if AvEnableInternalVideoGen
    AvVideoGen *vg;
#endif
#if AvEnableInternalAudioGen
    AvAudioGen *ag;
#endif
#if AvEnableInternalClockGen
    AvClockGen *cg;
#endif
#if AvEnableCecFeature /* CEC Related */
    AvCec     *cec;
#endif
#if AvEnableEArcRxFeature
    EarcRx    *earx;
    AvAudio   *eaaud;
#endif
    /* Hdcp source and sink */
    /* pointing to the source of Hdcp in the stream, only for source and sink */
    struct AvPort    *HdcpSource;
    /* pointing to the next port which finally outputs Hdcp in the stream */
    struct AvPort    *HdcpNextSinkPort;
    /* Video Routing */
    /* pointing to the port which this video is from */
    struct AvPort    *RouteVideoFromPort;
    /* same level port */
    struct AvPort    *RouteVideoNextSameLevelPort;
    /* pointing to the port which this video is transmitted to */
    struct AvPort    *RouteVideoToPort;
    /* Audio Routing */
    /* pointing to the port which this audio is from */
    struct AvPort    *RouteAudioFromPort;
    /* same level port */
    struct AvPort    *RouteAudioNextSameLevelPort;
    /* pointing to the port which this audio is transmitted to */
    struct AvPort    *RouteAudioToPort;
} AvContent;

/* audio/video port type */
typedef enum
{
    HdmiRx,
    DpRx,
    DviRx,
    AnalogRx,
    LogicVideoRx,
    LogicAudioRx,
    HdmiTx,
    DviTx,
    AnalogTx,
    VideoScaler,
    VideoColor,
    VideoGen,
    AudioGen,
    ClockGen,
    LogicVideoTx,
    LogicAudioTx
} AvPortType;

/* Layer 1 */
/* hardware cores that a port is using, for internal use */
typedef struct
{
    int8 HdmiCore; /**< Hdmi rx/tx core */
#ifndef COMPILER_C51_MODE
    int8 DpllCore; /**< DPLL core */
    int8 OsdCore;  /**< OSD core */
    int8 VspCore;  /**< VSP core */
#endif
} AvHwCore;

/* general audio/video port structure */
typedef struct
{
    AvDevice *device;
    AvPortType type;
    uint16 index;          /**< port index of same type in current device */
    AvContent content;     /**< port content */
    AvHwCore core;         /**< hardware cores that port is using, for internal use */
    struct AvPort *next;   /**< next port, internally assigned when created */
} AvPort;

#endif
