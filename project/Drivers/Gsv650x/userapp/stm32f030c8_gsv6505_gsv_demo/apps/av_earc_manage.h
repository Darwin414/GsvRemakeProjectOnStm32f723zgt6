#include "av_config.h"
#ifndef __av_earc_manage_h
#define __av_earc_manage_h
#define AvEarcBitBlockAudioData               1<<0
#define AvEarcBitBlockSpeakerAllocation       1<<1
#define AvEarcBitBlockVendorSpecAudioData     1<<2
#define AvEarcBitBlockRoomConfiguration       1<<3
#define AvEarcBitBlockSpeakerLocation         1<<4
#define AvEarcBitBlockStreamLayout            1<<5
#define AvEarcBitAudioFormatLPCM              1<<0
#define AvEarcBitAudioFormatAC3               1<<1
#define AvEarcBitAudioFormatMPEG1             1<<2
#define AvEarcBitAudioFormatMP3               1<<3
#define AvEarcBitAudioFormatMPEG2             1<<4
#define AvEarcBitAudioFormatAACLC             1<<5
#define AvEarcBitAudioFormatDTS               1<<6
#define AvEarcBitAudioFormatATRAC             1<<7
#define AvEarcBitAudioFormatDSD               1<<8
#define AvEarcBitAudioFormatENHANCED_AC3      1<<9
#define AvEarcBitAudioFormatDTSHD             1<<10
#define AvEarcBitAudioFormatMAT               1<<11
#define AvEarcBitAudioFormatDST               1<<12
#define AvEarcBitAudioFormatWMAPRO            1<<13
#define AvEarcBitAudioFormatMPEG4AAC          1<<14
#define AvEarcBitAudioFormatMPEG4AACV2        1<<15
#define AvEarcBitAudioFormatMPEG4AACLC        1<<16
#define AvEarcBitAudioFormatDRA               1<<17
#define AvEarcBitAudioFormatMPEG4SURR_HEAAC   1<<18
#define AvEarcBitAudioFormatMPEG4SURR_LCAAC   1<<19
#define AvEarcBitAudioFormatMPEGH3D           1<<20
#define AvEarcBitAudioFormatAC4               1<<21
#define AvEarcBitAudioFormatLPCM3D            1<<22
#define AvEarcBitAudioLayoutLPCM16Max         1<<0
#define AvEarcBitAudioLayoutLPCM32Max         1<<1
#define AvEarcBitAudioLayoutOneBit            1<<3
#define AvEarcBitAudioLayoutPacket            1<<7
#define AvEarcBitSf32KHZ               1<<0
#define AvEarcBitSf44KHZ               1<<1
#define AvEarcBitSf48KHZ               1<<2
#define AvEarcBitSf88KHZ               1<<3
#define AvEarcBitSf96KHZ               1<<4
#define AvEarcBitSf176KHZ              1<<5
#define AvEarcBitSf192KHZ              1<<6
#define AvEarcBitAud16bit              1<<0
#define AvEarcBitAud20bit              1<<1
#define AvEarcBitAud24bit              1<<2
/* EARC Structure Declare Start */
typedef struct
{
    uint8  EarcCeaAudioCheck [23];
    uint8  EarcCeaAudioSfTable [23];
    uint8  EarcCeaAudioFmtTable [23];
    uint8  EarcCeaAudioByte3 [23];
} AvEarcReg;
/* EARC Structure Declare End */
    void AvEarcFuncStructInit(AvEarcReg *EarcReg);
    uint16 AvEarcFuncFindBlockTag(AvEarcReg *EarcReg,uint8 *EarcContent,uint8 TagNumber);
    void AvEarcFuncCapDataProcess(AvEarcReg *EarcReg,uint8 *InEarc,uint8 *SinkEarc,uint8 *OutEarc);
    void AvEarcFunFullAnalysis(AvEarcReg *EarcReg,uint8 *InEarc);
    uint8 AvEarcFuncBlockAudio(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset);
    void AvEarcFuncBlockAudioRead(AvEarcReg *EarcReg,uint8 *Earc,uint8 EarcLen,uint8 EarcOffset);
    uint8 AvEarcFuncBlockRoomConfig(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset);
    uint8 AvEarcFuncBlockSpeaker(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset);
    uint8 AvEarcFuncBlockLayout(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset);
#endif
