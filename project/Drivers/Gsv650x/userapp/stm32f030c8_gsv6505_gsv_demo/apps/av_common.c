#include "av_config.h"

#if AvEnableCecFeature /* CEC Related */
uchar  DevicePowerStatus = 0;
char   DeviceName[20] = "GSV Default";
uchar  AudioStatus = 1;
uint8  ARCFeatureSupport = 1;
CecAudioStatus CecTxAudioStatus;
#endif

const uint8 PktSize[26] ={31, /* 0:AV_PKT_AV_INFO_FRAME */
                          14, /* 1:AV_PKT_AUDIO_INFO_FRAME*/
                          31, /* 2:AV_PKT_ACP_PACKET*/
                          31, /* 3:AV_PKT_SPD_PACKET*/
                          31, /* 4:AV_PKT_ISRC1_PACKET*/
                          31, /* 5:AV_PKT_ISRC2_PACKET*/
                          31, /* 6:AV_PKT_GMD_PACKET*/
                          32, /* 7:AV_PKT_GC_PACKET*/
                          17, /* 8:AV_PKT_MPEG_PACKET*/
                          31, /* 9:AV_PKT_VS_PACKET*/
                          5,  /*10:AV_PKT_AUDIO_CHANNEL_STATUS*/
                          32, /*11:AV_PKT_AUDIO_SAMPLE_PACKET*/
                          32, /*12:AV_PKT_ACR_PACKET*/
                          32, /*13:AV_PKT_EXT_AV_INFO_FRAME*/
                          31, /*14:AV_PKT_HDR_PACKET*/
                          31, /*15:AV_PKT_EMP_PACKET*/
                          31, /*16:AV_PKT_SPARE3_PACKET*/
                          31, /*17:AV_PKT_SPARE4_PACKET*/
                          31, /*18:AV_PKT_AMD_PACKET*/
                          31, /*19:AV_PKT_ALL_PACKETS*/
                          24, /*20:AV_PKT_EARC_UBIT*/
                          24, /*21:AV_PKT_EARC_VBIT*/
                          24, /*22:AV_PKT_EARC_CBIT*/
                          7,  /*23:AV_PKT_EARC_AUDIF*/
                          1,  /*24:AV_PKT_EARC_AUDMUTE*/
                          6   /*25:AV_PKT_EARC_AUDFMT*/
};

                       /*  32    44    48     88     96    176    192    768   Undefined */
const uint32 NTable[9] ={4096, 6272, 6144, 12544, 12288, 25088, 24576, 98304,  4096};
const uchar  NIdx[16]  ={ 1,  8,  2,  0,  8,  8,  8,  8,
                                    3,  7,  4,  8,  5,  8,  6,  8};
/* i*3+2 = MCLK ratio, 1 = 256Fs, 0 = 128Fs, 3 = 512Fs */
const uchar ChannelStatusSfTable[] = {
    (uchar)AV_AUD_FS_32KHZ,     3,    1,
    (uchar)AV_AUD_FS_44KHZ,     0,    1,
    (uchar)AV_AUD_FS_48KHZ,     2,    1,
    (uchar)AV_AUD_FS_88KHZ,     8,    1,
    (uchar)AV_AUD_FS_96KHZ,    10,    1,
    (uchar)AV_AUD_FS_176KHZ,   12,    1,
    (uchar)AV_AUD_FS_192KHZ,   14,    1,
    (uchar)AV_AUD_FS_HBR,       9,    1,
    (uchar)AV_AUD_FS_FROM_STRM, 0,    1,
    0xff,                    0xff, 0xff
};

#if AvEnableCecFeature /* CEC Related */

/* Size is FLEX_OP_CODES */
const uchar FlexOpCodes[] = {
/* opcode; standard parameters length, max parameters length, min parameters length   */
    AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR, 4,5,2
};

/* Size is CEC_OP_CODES */
const uint8 CecOpCodes[] = {
        AV_CEC_MSG_STANDBY, 2,         AV_CEC_MSG_ROUTE_CHANGE, 6,
        AV_CEC_MSG_ROUTE_INFO, 4,      AV_CEC_MSG_ACTIVE_SRC, 4,
        AV_CEC_MSG_GIVE_PHYS_ADDR, 2,  AV_CEC_MSG_REPORT_PHYS_ADDR, 5,
        AV_CEC_MSG_SET_STRM_PATH, 4,   AV_CEC_MSG_ABORT, 2,
        AV_CEC_MSG_FEATURE_ABORT, 4,   AV_CEC_MSG_INITIATE_ARC , 2,
        AV_CEC_MSG_REPORT_ARC_INITIATED, 2, AV_CEC_MSG_REPORT_ARC_TERMINATED, 2,
        AV_CEC_MSG_REQUEST_ARC_INITIATION, 2, AV_CEC_MSG_REQUEST_ARC_TERMINATION, 2,
        AV_CEC_MSG_TERMINATE_ARC, 2,            AV_CEC_MSG_IMAGE_VIEW_ON, 2,
        AV_CEC_MSG_TUNER_STEP_INC, 2,           AV_CEC_MSG_TUNER_STEP_DEC, 2,
        AV_CEC_MSG_GIVE_TUNER_DEV_STATUS, 3,    AV_CEC_MSG_RECORD_STATUS, 3,
        AV_CEC_MSG_RECORD_OFF, 2,           AV_CEC_MSG_TEXT_VIEW_ON, 2,
        AV_CEC_MSG_RECORD_TV_SCREEN, 2,     AV_CEC_MSG_GIVE_DECK_STATUS, 3,
        AV_CEC_MSG_DECK_STATUS, 3,          AV_CEC_MSG_SET_MENU_LANGUAGE, 5,
        AV_CEC_MSG_CLR_ANALOGUE_TIMER, 13,  AV_CEC_MSG_SET_ANALOGUE_TIMER, 13,
        AV_CEC_MSG_PLAY, 3,                 AV_CEC_MSG_DECK_CONTROL, 3,
        AV_CEC_MSG_TIMER_CLEARED_STATUS, 3, AV_CEC_MSG_USER_CONTROL_RELEASED, 2,
        AV_CEC_MSG_GIVE_OSD_NAME, 2,   AV_CEC_MSG_GIVE_SYSTEM_AUDIO_MODE_STATUS, 2,
        AV_CEC_MSG_GIVE_AUDIO_STATUS, 2,    AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE, 3,
        AV_CEC_MSG_REPORT_AUDIO_STATUS, 3,  AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS, 3,
        AV_CEC_MSG_REQ_ACTIVE_SRC, 2,       AV_CEC_MSG_DEVICE_VENDOR_ID, 5,
        AV_CEC_MSG_VENDOR_REMOTE_BTN_UP, 2, AV_CEC_MSG_GET_VENDOR_ID, 2,
        AV_CEC_MSG_MENU_REQUEST, 3,         AV_CEC_MSG_MENU_STATUS, 3,
        AV_CEC_MSG_GIVE_PWR_STATUS, 2,      AV_CEC_MSG_REPORT_PWR_STATUS, 3,
        AV_CEC_MSG_GET_MENU_LANGUAGE, 2,    AV_CEC_MSG_SEL_ANALOGUE_SERVICE, 6,
        AV_CEC_MSG_SEL_DIGITAL_SERVICE, 9,  AV_CEC_MSG_SET_DIGITAL_TIMER, 16,
        AV_CEC_MSG_CLR_DIGITAL_TIMER, 16,   AV_CEC_MSG_SET_AUDIO_RATE, 3,
        AV_CEC_MSG_INACTIVE_SOURCE, 4,      AV_CEC_MSG_CEC_VERSION, 3,
        AV_CEC_MSG_GET_CEC_VERSION, 2,      AV_CEC_MSG_CLR_EXTERNAL_TIMER, 13,
        AV_CEC_MSG_SET_EXTERNAL_TIMER, 13,
        AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR, 5,
        AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR, 4,
        AV_CEC_MSG_USER_CONTROL_PRESSED,           3
};
#endif

const uint8 ChanCount[32] =
{
        1, 2, 2, 3, 2, 3, 3, 4,
        3, 4, 4, 5, 4, 5, 5, 6,
        5, 6, 6, 7, 3, 4, 4, 5,
        4, 5, 5, 6, 5, 6, 6, 7
};

const AvVideoAspectRatio ARTable[108] =
{
    AV_AR_NOT_INDICATED,  AV_AR_4_3,  AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, /* VIC 0 ~ 7 */
    AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, /* VIC 8 ~ 15 */
    AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  /* VIC 16 ~ 23 */
    AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, /* VIC 24 ~ 31 */
    AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, /* VIC 32 ~ 39 */
    AV_AR_16_9, AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, /* VIC 40 ~ 47 */
    AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, /* VIC 48 ~ 55 */
    AV_AR_4_3,  AV_AR_16_9, AV_AR_4_3,  AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, AV_AR_16_9, /* VIC 56 ~ 63 */
    AV_AR_16_9,                                                                                     /* VIC 64 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,/* VIC 65 ~ 72 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,/* VIC 73 ~ 80 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,/* VIC 81 ~ 88 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27,                                             /* VIC 89 ~ 92 */
    AV_AR_16_9,  AV_AR_16_9,  AV_AR_16_9,  AV_AR_16_9,  AV_AR_16_9,                                 /* VIC 93 ~ 97 */
    AV_AR_256_135, AV_AR_256_135, AV_AR_256_135, AV_AR_256_135, AV_AR_256_135,                      /* VIC 98 ~ 102 */
    AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27, AV_AR_64_27                                 /* VIC 103 ~ 107 */
};

const uchar VideoGenVicTable[] = {
  //0xD7, 0xD7, 0x02, 0x02, 0x96, 0x00, 0x00, /* 10K60 = 215 */
  //0xD6, 0xD6, 0x02, 0x02, 0x96, 0x00, 0x00, /* 10K50 = 214 */
  //0xD4, 0xD4, 0x04, 0x02, 0x96, 0x00, 0x00, /* 10K30 = 212 */
  //0xD3, 0xD3, 0x04, 0x02, 0x96, 0x00, 0x00, /* 10K25 = 211 */
  //0xD2, 0xD2, 0x04, 0x02, 0x96, 0x00, 0x00, /* 10K24 = 210 */
  //0xC7, 0xC7, 0x02, 0x02, 0x96, 0x00, 0x00, /* 8K60 = 199  */
  //0xC6, 0xC6, 0x02, 0x02, 0x96, 0x00, 0x00, /* 8K50 = 198  */
    0xC4, 0xC4, 0x04, 0x25, 0x9A, 0x4A, 0x20, /* 8K30 = 196  */
  //0xC3, 0xC3, 0x04, 0x02, 0x96, 0x00, 0x00, /* 8K25 = 195  */
  //0xC2, 0xC2, 0x04, 0x02, 0x96, 0x00, 0x00, /* 8K24 = 194  */
    0x76, 0x76, 0x04, 0x25, 0x9A, 0x4A, 0x20, /* 4K120 = 118  */
    0x75, 0x75, 0x04, 0x25, 0x9A, 0x4A, 0x20, /* 4K100 = 117  */
  //0xDB, 0xDB, 0x04, 0x02, 0x96, 0x00, 0x00, /* 4KS120 = 219 */
    0x7E, 0x7E, 0x07, 0x0C, 0x6F, 0xCE, 0xBE, /* 5K60  = 126  */
    0x7D, 0x7D, 0x07, 0x0C, 0x6F, 0xCE, 0xBE, /* 5K50  = 125  */
    0x61, 0x5F, 0x02, 0x25, 0x9A, 0x4A, 0x20, /* 4K60  = 97   */
    0x60, 0x5E, 0x02, 0x25, 0x9A, 0x4A, 0x20, /* 4K50  = 96   */
  //0x65, 0x65, 0x02, 0x02, 0x96, 0x00, 0x00, /* 4KS50 = 101  */
  //0x72, 0x72, 0x02, 0x02, 0x96, 0x00, 0x00, /* 4K48  = 114  */
    0x7B, 0x7B, 0x03, 0x25, 0x9A, 0x4A, 0x20, /* 5K30  = 123  */
    0x7A, 0x7A, 0x03, 0x25, 0x9A, 0x4A, 0x20, /* 5K25  = 122  */
    0x79, 0x79, 0x03, 0x25, 0x9A, 0x4A, 0x20, /* 5K24  = 121  */
    0x5F, 0x5F, 0x04, 0x25, 0x9A, 0x4A, 0x20, /* 4K30  = 95   */
    0x5E, 0x5E, 0x04, 0x25, 0x9A, 0x4A, 0x20, /* 4K25  = 94   */
    0x5D, 0x5D, 0x04, 0x25, 0x9A, 0x4A, 0x20, /* 4K24  = 93   */
    0x62, 0x62, 0x04, 0x25, 0x9A, 0x4A, 0x20, /* 4KS24 = 98   */
  //0x65, 0x65, 0x04, 0x02, 0x96, 0x00, 0x00, /* 4KS25 = 99   */
  //0x66, 0x66, 0x04, 0x02, 0x96, 0x00, 0x00, /* 4KS30 = 102  */
    0x10, 0x10, 0x09, 0x04, 0xFA, 0xEC, 0x8E, /* 1080p60 = 16 */
    0x04, 0x04, 0x12, 0x04, 0xFA, 0xEC, 0x8E, /* 720p  = 4    */
    0x02, 0x03, 0x32, 0x02, 0x5E, 0xD0, 0x97, /* 480p  = 2    */
    0x11, 0x11, 0x32, 0x02, 0x5E, 0xD0, 0x97, /* 576p  = 17   */
    0xFF
};
