#include "av_config.h"
#include "av_earc_manage.h"

/* EARC Capability Declare Start */
static const uint16 AvEarcSupportAudioFormat  =
                           AvEarcBitAudioFormatLPCM |
                           AvEarcBitAudioFormatAC3  |
                           AvEarcBitAudioFormatDTS  |
                           AvEarcBitAudioFormatENHANCED_AC3 |
                           AvEarcBitAudioFormatDTSHD |
                           AvEarcBitAudioFormatMAT;

static const uint8  AvEarcAudioChannelLPCM                =  8;
static const uint8  AvEarcAudioChannelAC3                 =  6;
static const uint8  AvEarcAudioChannelMPEG1               =  2;
static const uint8  AvEarcAudioChannelMP3                 =  2;
static const uint8  AvEarcAudioChannelMPEG2               =  2;
static const uint8  AvEarcAudioChannelAACLC               =  2;
static const uint8  AvEarcAudioChannelDTS                 =  6;
static const uint8  AvEarcAudioChannelATRAC               =  2;
static const uint8  AvEarcAudioChannelDSD                 =  2;
static const uint8  AvEarcAudioChannelENHANCED_AC3        =  8;
static const uint8  AvEarcAudioChannelDTSHD               =  8;
static const uint8  AvEarcAudioChannelMAT                 =  8;
static const uint8  AvEarcAudioChannelDST                 =  8;
static const uint8  AvEarcAudioChannelWMAPRO              =  2;
static const uint8  AvEarcAudioChannelMPEG4AAC            =  2;
static const uint8  AvEarcAudioChannelMPEG4AACV2          =  2;
static const uint8  AvEarcAudioChannelMPEG4AACLC          =  2;
static const uint8  AvEarcAudioChannelDRA                 =  2;
static const uint8  AvEarcAudioChannelMPEG4SURR_HEAAC     =  2;
static const uint8  AvEarcAudioChannelMPEG4SURR_LCAAC     =  2;
static const uint8  AvEarcAudioChannelMPEGH3D             =  2;
static const uint8  AvEarcAudioChannelAC4                 =  2;
static const uint8  AvEarcAudioChannelLPCM3D              =  2;
static const uint8  AvEarcAudioBitWidth        =  AvEarcBitAud16bit | AvEarcBitAud20bit | AvEarcBitAud24bit;
static const uint8  AvEarcAudioSfLPCM          =  AvEarcBitSf48KHZ | AvEarcBitSf96KHZ | AvEarcBitSf32KHZ | AvEarcBitSf44KHZ |
                                                  AvEarcBitSf88KHZ | AvEarcBitSf176KHZ | AvEarcBitSf192KHZ;
static const uint8  AvEarcAudioSfAC3           =  AvEarcBitSf48KHZ | AvEarcBitSf32KHZ | AvEarcBitSf44KHZ;
static const uint8  AvEarcAudioSfMPEG1         =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMP3           =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMPEG2         =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfAACLC         =  AvEarcBitSf48KHZ | AvEarcBitSf96KHZ | AvEarcBitSf32KHZ | AvEarcBitSf44KHZ |
                                                  AvEarcBitSf96KHZ | AvEarcBitSf192KHZ;
static const uint8  AvEarcAudioSfDTS           =  AvEarcBitSf48KHZ | AvEarcBitSf96KHZ | AvEarcBitSf88KHZ | AvEarcBitSf44KHZ |
                                                  AvEarcBitSf32KHZ;
static const uint8  AvEarcAudioSfATRAC         =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfDSD           =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfENHANCED_AC3  =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfDTSHD         =  AvEarcBitSf44KHZ | AvEarcBitSf48KHZ | AvEarcBitSf88KHZ | AvEarcBitSf96KHZ |
                                                  AvEarcBitSf176KHZ | AvEarcBitSf192KHZ;
static const uint8  AvEarcAudioSfMAT           =  AvEarcBitSf48KHZ | AvEarcBitSf96KHZ | AvEarcBitSf192KHZ;
static const uint8  AvEarcAudioSfDST           =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfWMAPRO        =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMPEG4AAC      =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMPEG4AACV2    =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMPEG4AACLC    =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfDRA           =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMPEG4SURR_HEAAC  =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMPEG4SURR_LCAAC  =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfMPEGH3D       =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfAC4           =  AvEarcBitSf48KHZ;
static const uint8  AvEarcAudioSfLPCM3D        =  AvEarcBitSf48KHZ;
static const uint16 AvEarcAudioBitRateAC3      =  640 ;
static const uint16 AvEarcAudioBitRateMPEG1    =  640;
static const uint16 AvEarcAudioBitRateMP3      =  640;
static const uint16 AvEarcAudioBitRateMPEG2    =  640;
static const uint16 AvEarcAudioBitRateAACLC    =  1536;
static const uint16 AvEarcAudioBitRateDTS      =  1536;
static const uint16 AvEarcAudioBitRateATRAC    =  1536;
static const uint8  AvEarcSpeakerChannelNum    =  2;
/* EARC Capability Declare End */
/* EARC Param Declare Start */
uint8  AvEarcParamForce   = AvEarcBitBlockRoomConfiguration | AvEarcBitBlockSpeakerLocation | AvEarcBitBlockStreamLayout;
uint8  AvEarcParamRemove  = 0;
uint8  AvEarcStreamLayout = AvEarcBitAudioLayoutLPCM32Max | AvEarcBitAudioLayoutOneBit | AvEarcBitAudioLayoutPacket;
/* EARC Param Declare End */
/* EARC Table Declare Start */
static const uint8 EarcAudioFormatCode[] =  {1<<3,2<<3,3<<3,4<<3,5<<3,6<<3,7<<3,8<<3,9<<3,10<<3,11<<3,12<<3,13<<3,14<<3,4<<3,5<<3,6<<3,7<<3,8<<3,10<<3,11<<3,12<<3,13<<3};
static const uint8 EarcSpeakerAllocationData[] =  {0x5F,0x00,0x00};
static const uint8 EarcSpeakerLocationData[] =  {0x20,0x00,0x01,0x01,0x01,0x21,0x01,0x01,0x01,0x01};
/* EARC Table Declare End */
/* EARC Structure Init Start */
AvEarcReg DevEarcReg;
void AvEarcFuncStructInit(AvEarcReg *EarcReg)
{
    uint8 i = 0;
    for(i=0;i<23;i++)
        EarcReg->EarcCeaAudioCheck[i] = 0;
    for(i=0;i<23;i++)
        EarcReg->EarcCeaAudioSfTable[i] = 0;
    for(i=0;i<23;i++)
        EarcReg->EarcCeaAudioFmtTable[i] = 0;
    for(i=0;i<23;i++)
        EarcReg->EarcCeaAudioByte3[i] = 0;
    return;
}
/* EARC Structure Init End */
/* EARC Function Declare Start */
    uint16 AvEarcFuncFindBlockTag(AvEarcReg *EarcReg,uint8 *EarcContent,uint8 TagNumber)
    {
        uint8   bLen = 0;
        uint8   bTag = 0;
        uint16  CeabParam = 0;
        uint16  offset = 0x01;
        if(EarcContent[0x00] != 0x01)
        {
            return 0;
        }
        while(offset <= 0xfd)
        {
            bLen = EarcContent[offset+1] & 0xff;
            bTag = EarcContent[offset+0] & 0x1f;
            if(bTag == TagNumber)
            {
                CeabParam = (offset<<8) + bLen;
                return CeabParam;
            }
            else if(bLen == 0)
            {
                CeabParam = 0;
                return CeabParam;
            }
            else
            {
                offset = offset + bLen + 2;
            }
        }
        CeabParam = 0;
        return CeabParam;
    }
    void AvEarcFuncCapDataProcess(AvEarcReg *EarcReg,uint8 *InEarc,uint8 *SinkEarc,uint8 *OutEarc)
    {
        uint16  iCeabParam = 0;
        uint16  sCeabParam = 0;
        uint8   i = 0;
        uint8   oLen    = 0;
        uint8   oOffset = 0;
        OutEarc[0] = 0x01;
        iCeabParam = 0;
        sCeabParam = 0;
        oLen    = 0;
        oOffset = 0x01;
        if(AvEarcParamForce  & AvEarcBitBlockAudioData)
        {
            oLen       = AvEarcFuncBlockAudio(EarcReg, InEarc, iCeabParam, SinkEarc, sCeabParam, OutEarc, oOffset);
        }
        else if(AvEarcParamRemove & AvEarcBitBlockAudioData)
        {
            oLen       = 0;
        }
        else
        {
            iCeabParam = AvEarcFuncFindBlockTag(EarcReg, InEarc,   0x01);
            sCeabParam = AvEarcFuncFindBlockTag(EarcReg, SinkEarc, 0x01);
            oLen       = AvEarcFuncBlockAudio(EarcReg, InEarc, iCeabParam, SinkEarc, sCeabParam, OutEarc, oOffset);
        }
        if(oLen>0)
        {
            oOffset = oOffset + 2 + oLen;
        }
        oLen = 0;
        if(AvEarcParamForce  & AvEarcBitBlockRoomConfiguration)
        {
            oLen       = AvEarcFuncBlockRoomConfig(EarcReg, InEarc, iCeabParam, SinkEarc, sCeabParam, OutEarc, oOffset);
        }
        if(oLen>0)
        {
            oOffset = oOffset + 1 + oLen;
        }
        oLen = 0;
        if(AvEarcParamForce  & AvEarcBitBlockSpeakerLocation)
        {
            oLen       = AvEarcFuncBlockSpeaker(EarcReg, InEarc, iCeabParam, SinkEarc, sCeabParam, OutEarc, oOffset);
        }
        if(oLen>0)
        {
            oOffset = oOffset + 2 + oLen;
        }
        oLen = 0;
        if(AvEarcParamForce  & AvEarcBitBlockStreamLayout)
        {
            oLen       = AvEarcFuncBlockLayout(EarcReg, InEarc, iCeabParam, SinkEarc, sCeabParam, OutEarc, oOffset);
        }
        if(oLen>0)
        {
            oOffset = oOffset + 2 + oLen;
        }
        oLen = 256 - oOffset;
        for(i=0;i<oLen;i++)
        {
            OutEarc[oOffset+i] = 0;
        }
    }
    void AvEarcFunFullAnalysis(AvEarcReg *EarcReg,uint8 *InEarc)
    {
        uint16  iCeabParam = 0;
        uint8   iEarcOffset = 0;
        uint8   iEarcLen    = 0;
        iCeabParam    = AvEarcFuncFindBlockTag(EarcReg, InEarc,   0x01);
        iEarcOffset   = (iCeabParam>>8)&0xff;
        iEarcLen      = (iCeabParam>>0)&0xff;
        if(iEarcOffset != 0)
        {
            AvEarcFuncBlockAudioRead(EarcReg, InEarc, iEarcLen, iEarcOffset);
        }
    }
    uint8 AvEarcFuncBlockAudio(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset)
    {
        uint8   InEarcOffset   = (InCeabParam>>8)&0xff;
        uint8   InEarcLen      = (InCeabParam>>0)&0xff;
        uint8   SinkEarcOffset = (SinkCeabParam>>8)&0xff;
        uint8   SinkEarcLen    = (SinkCeabParam>>0)&0xff;
        uint8   i = 0;
        uint8   SelectValue = 0;
        uint8   bLen = 0;
        EarcReg->EarcCeaAudioSfTable[0]   = AvEarcAudioSfLPCM;
        EarcReg->EarcCeaAudioSfTable[1]   = AvEarcAudioSfAC3;
        EarcReg->EarcCeaAudioSfTable[2]   = AvEarcAudioSfMPEG1;
        EarcReg->EarcCeaAudioSfTable[3]   = AvEarcAudioSfMP3;
        EarcReg->EarcCeaAudioSfTable[4]   = AvEarcAudioSfMPEG2;
        EarcReg->EarcCeaAudioSfTable[5]   = AvEarcAudioSfAACLC;
        EarcReg->EarcCeaAudioSfTable[6]   = AvEarcAudioSfDTS;
        EarcReg->EarcCeaAudioSfTable[7]   = AvEarcAudioSfATRAC;
        EarcReg->EarcCeaAudioSfTable[8]   = AvEarcAudioSfDSD;
        EarcReg->EarcCeaAudioSfTable[9]   = AvEarcAudioSfENHANCED_AC3;
        EarcReg->EarcCeaAudioSfTable[10]  = AvEarcAudioSfDTSHD;
        EarcReg->EarcCeaAudioSfTable[11]  = AvEarcAudioSfMAT;
        EarcReg->EarcCeaAudioSfTable[12]  = AvEarcAudioSfDST;
        EarcReg->EarcCeaAudioSfTable[13]  = AvEarcAudioSfWMAPRO;
        EarcReg->EarcCeaAudioSfTable[14]  = AvEarcAudioSfMPEG4AAC;
        EarcReg->EarcCeaAudioSfTable[15]  = AvEarcAudioSfMPEG4AACV2;
        EarcReg->EarcCeaAudioSfTable[16]  = AvEarcAudioSfMPEG4AACLC;
        EarcReg->EarcCeaAudioSfTable[17]  = AvEarcAudioSfDRA;
        EarcReg->EarcCeaAudioSfTable[18]  = AvEarcAudioSfMPEG4SURR_HEAAC;
        EarcReg->EarcCeaAudioSfTable[19]  = AvEarcAudioSfMPEG4SURR_LCAAC;
        EarcReg->EarcCeaAudioSfTable[20]  = AvEarcAudioSfMPEGH3D;
        EarcReg->EarcCeaAudioSfTable[21]  = AvEarcAudioSfAC4;
        EarcReg->EarcCeaAudioSfTable[22]  = AvEarcAudioSfLPCM3D;
        EarcReg->EarcCeaAudioFmtTable[0]  = AvEarcAudioChannelLPCM  - 1;
        EarcReg->EarcCeaAudioFmtTable[1]  = AvEarcAudioChannelAC3   - 1;
        EarcReg->EarcCeaAudioFmtTable[2]  = AvEarcAudioChannelMPEG1 - 1;
        EarcReg->EarcCeaAudioFmtTable[3]  = AvEarcAudioChannelMP3   - 1;
        EarcReg->EarcCeaAudioFmtTable[4]  = AvEarcAudioChannelMPEG2 - 1;
        EarcReg->EarcCeaAudioFmtTable[5]  = AvEarcAudioChannelAACLC - 1;
        EarcReg->EarcCeaAudioFmtTable[6]  = AvEarcAudioChannelDTS   - 1;
        EarcReg->EarcCeaAudioFmtTable[7]  = AvEarcAudioChannelATRAC - 1;
        EarcReg->EarcCeaAudioFmtTable[8]  = AvEarcAudioChannelDSD   - 1;
        EarcReg->EarcCeaAudioFmtTable[9]  = AvEarcAudioChannelENHANCED_AC3 - 1;
        EarcReg->EarcCeaAudioFmtTable[10] = AvEarcAudioChannelDTSHD - 1;
        EarcReg->EarcCeaAudioFmtTable[11] = AvEarcAudioChannelMAT   - 1;
        EarcReg->EarcCeaAudioFmtTable[12] = AvEarcAudioChannelDST   - 1;
        EarcReg->EarcCeaAudioFmtTable[13] = AvEarcAudioChannelWMAPRO - 1;
        EarcReg->EarcCeaAudioFmtTable[14] = AvEarcAudioChannelMPEG4AAC - 1;
        EarcReg->EarcCeaAudioFmtTable[15] = AvEarcAudioChannelMPEG4AACV2 - 1;
        EarcReg->EarcCeaAudioFmtTable[16] = AvEarcAudioChannelMPEG4AACLC - 1;
        EarcReg->EarcCeaAudioFmtTable[17] = AvEarcAudioChannelDRA   - 1;
        EarcReg->EarcCeaAudioFmtTable[18] = AvEarcAudioChannelMPEG4SURR_HEAAC - 1;
        EarcReg->EarcCeaAudioFmtTable[19] = AvEarcAudioChannelMPEG4SURR_LCAAC - 1;
        EarcReg->EarcCeaAudioFmtTable[20] = AvEarcAudioChannelMPEGH3D - 1;
        EarcReg->EarcCeaAudioFmtTable[21] = AvEarcAudioChannelAC4 - 1;
        EarcReg->EarcCeaAudioFmtTable[22] = ((AvEarcAudioChannelLPCM3D - 1)%7) + (((AvEarcAudioChannelLPCM3D - 1)/7)<<7);
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatLPCM)
        {
            EarcReg->EarcCeaAudioCheck[0]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatAC3)
        {
            EarcReg->EarcCeaAudioCheck[1]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEG1)
        {
            EarcReg->EarcCeaAudioCheck[2]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMP3)
        {
            EarcReg->EarcCeaAudioCheck[3]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEG2)
        {
            EarcReg->EarcCeaAudioCheck[4]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatAACLC)
        {
            EarcReg->EarcCeaAudioCheck[5]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatDTS)
        {
            EarcReg->EarcCeaAudioCheck[6]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatATRAC)
        {
            EarcReg->EarcCeaAudioCheck[7]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatDSD)
        {
            EarcReg->EarcCeaAudioCheck[8]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatENHANCED_AC3)
        {
            EarcReg->EarcCeaAudioCheck[9]     = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatDTSHD)
        {
            EarcReg->EarcCeaAudioCheck[10]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMAT)
        {
            EarcReg->EarcCeaAudioCheck[11]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatDST)
        {
            EarcReg->EarcCeaAudioCheck[12]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatWMAPRO)
        {
            EarcReg->EarcCeaAudioCheck[13]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEG4AAC)
        {
            EarcReg->EarcCeaAudioCheck[14]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEG4AACV2)
        {
            EarcReg->EarcCeaAudioCheck[15]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEG4AACLC)
        {
            EarcReg->EarcCeaAudioCheck[16]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatDRA)
        {
            EarcReg->EarcCeaAudioCheck[17]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEG4SURR_HEAAC)
        {
            EarcReg->EarcCeaAudioCheck[18]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEG4SURR_LCAAC)
        {
            EarcReg->EarcCeaAudioCheck[19]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatMPEGH3D)
        {
            EarcReg->EarcCeaAudioCheck[20]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatAC4)
        {
            EarcReg->EarcCeaAudioCheck[21]    = 1;
        }
        if(AvEarcSupportAudioFormat & AvEarcBitAudioFormatLPCM3D)
        {
            EarcReg->EarcCeaAudioCheck[22]    = 1;
        }
        EarcReg->EarcCeaAudioByte3[0]     = AvEarcAudioBitWidth;
        EarcReg->EarcCeaAudioByte3[1]     = AvEarcAudioBitRateAC3>>3;
        EarcReg->EarcCeaAudioByte3[2]     = AvEarcAudioBitRateMPEG1>>3;
        EarcReg->EarcCeaAudioByte3[3]     = AvEarcAudioBitRateMP3>>3;
        EarcReg->EarcCeaAudioByte3[4]     = AvEarcAudioBitRateMPEG2>>3;
        EarcReg->EarcCeaAudioByte3[5]     = AvEarcAudioBitRateAACLC>>3;
        EarcReg->EarcCeaAudioByte3[6]     = AvEarcAudioBitRateDTS>>3;
        EarcReg->EarcCeaAudioByte3[7]     = AvEarcAudioBitRateATRAC>>3;
        EarcReg->EarcCeaAudioByte3[8]     = AvEarcAudioBitWidth>>3;
        EarcReg->EarcCeaAudioByte3[9]     = 0x03;
        EarcReg->EarcCeaAudioByte3[10]    = 0x00;
        EarcReg->EarcCeaAudioByte3[11]    = 0x03;
        EarcReg->EarcCeaAudioByte3[12]    = 0x01;
        EarcReg->EarcCeaAudioByte3[13]    = 0x00;
        EarcReg->EarcCeaAudioByte3[14]    = (EarcAudioFormatCode[14]) | 0x00;
        EarcReg->EarcCeaAudioByte3[15]    = (EarcAudioFormatCode[15]) | 0x00;
        EarcReg->EarcCeaAudioByte3[16]    = (EarcAudioFormatCode[16]) | 0x00;
        EarcReg->EarcCeaAudioByte3[17]    = (EarcAudioFormatCode[17]) | 0x00;
        EarcReg->EarcCeaAudioByte3[18]    = (EarcAudioFormatCode[18]) | 0x00;
        EarcReg->EarcCeaAudioByte3[19]    = (EarcAudioFormatCode[19]) | 0x00;
        EarcReg->EarcCeaAudioByte3[20]    = (EarcAudioFormatCode[20]) | 0x00;
        EarcReg->EarcCeaAudioByte3[21]    = (EarcAudioFormatCode[21]) | 0x00;
        EarcReg->EarcCeaAudioByte3[22]    = (EarcAudioFormatCode[22]) | 0x00;
        if(InEarcOffset != 0)
        {
            AvEarcFuncBlockAudioRead(EarcReg, InEarc, InEarcLen, InEarcOffset);
        }
        if(SinkEarcOffset != 0)
        {
            AvEarcFuncBlockAudioRead(EarcReg, SinkEarc, SinkEarcLen, SinkEarcOffset);
        }
        SelectValue = 1;
        if(InEarcOffset != 0)
        {
            SelectValue = SelectValue + 1;
        }
        if(SinkEarcOffset != 0)
        {
            SelectValue = SelectValue + 1;
        }
        for(i=0;i<23;i++)
        {
            if(EarcReg->EarcCeaAudioCheck[i] == SelectValue)
            {
                bLen = bLen + 1;
                if(i < 14)
                {
                    OutEarc[OutEarcOffset+bLen*3+0] = ((i+1)<<3) | EarcReg->EarcCeaAudioFmtTable[i];
                }
                else
                {
                    OutEarc[OutEarcOffset+bLen*3+0] = (0xf<<3) | EarcReg->EarcCeaAudioFmtTable[i];
                }
                OutEarc[OutEarcOffset+bLen*3+1] = EarcReg->EarcCeaAudioSfTable[i];
                OutEarc[OutEarcOffset+bLen*3+2] = EarcReg->EarcCeaAudioByte3[i];
            }
        }
#if AvEnableTTLCompressAudio
        bLen = bLen + 1;
        OutEarc[OutEarcOffset+bLen*3+0] = (11<<3) | (AvEarcAudioChannelDTSHD - 1);
        OutEarc[OutEarcOffset+bLen*3+1] = AvEarcAudioSfDTSHD;
        OutEarc[OutEarcOffset+bLen*3+2] = 0x01;
#endif
        bLen = bLen*3;
        OutEarc[OutEarcOffset+2] = (0x01<<5) + bLen;
        bLen = bLen + 1;
        OutEarc[OutEarcOffset+0] = 0x01;
        OutEarc[OutEarcOffset+1] = bLen;
        return bLen;
    }
    void AvEarcFuncBlockAudioRead(AvEarcReg *EarcReg,uint8 *Earc,uint8 EarcLen,uint8 EarcOffset)
    {
        uint8   i = 0;
        uint8   j = 0;
        uint8   oLen = 0;
        uint8   oOffset = 0;
        uint8   FmtNum = 0;
        uint8   ExtFmtNum = 0;
        uint8   ChnNum = 0;
        uint8 AudioCheck [] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        uint8 AudioChn   [] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        oLen = EarcLen/3;
        for(i=0;i<oLen;i++)
        {
            oOffset = EarcOffset+i*3;
            FmtNum = Earc[oOffset+1] & 0x78;
            ChnNum = Earc[oOffset+1] & 0x07;
            ExtFmtNum = Earc[oOffset+3] & 0x78;
            for(j=0;j<23;j++)
            {
                if(((FmtNum == EarcAudioFormatCode[j]) && (j < 14)) ||
                   ((FmtNum == 0x78) && (ExtFmtNum == EarcAudioFormatCode[j]) && (j >= 14)))
                {
                    if(AudioCheck[j] == 0)
                    {
                        AudioCheck[j] = 1;
                        AudioChn[j] = ChnNum;
                    }
                    EarcReg->EarcCeaAudioSfTable[j] = EarcReg->EarcCeaAudioSfTable[j] & Earc[oOffset+2];
                    if(ChnNum > AudioChn[j])
                    {
                        AudioChn[j] = ChnNum;
                    }
                    if(j == 0)
                    {
                        EarcReg->EarcCeaAudioByte3[0] = EarcReg->EarcCeaAudioByte3[0] & Earc[oOffset+3];
                    }
                    else if(j < 7)
                    {
                        if(Earc[oOffset+3] < EarcReg->EarcCeaAudioByte3[j])
                        {
                            EarcReg->EarcCeaAudioByte3[j] = Earc[oOffset+3];
                        }
                    }
                    else
                    {
                        EarcReg->EarcCeaAudioByte3[j] = Earc[oOffset+3];
                    }
                }
            }
        }
        for(i=0;i<23;i++)
        {
            if(AudioCheck[i] != 0)
            {
                EarcReg->EarcCeaAudioCheck[i] = EarcReg->EarcCeaAudioCheck[i] + 1;
                if(AudioChn[i] < EarcReg->EarcCeaAudioFmtTable[i])
                {
                    EarcReg->EarcCeaAudioFmtTable[i] = AudioChn[i];
                }
            }
        }
    }
    uint8 AvEarcFuncBlockRoomConfig(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset)
    {
        uint8   i = 0;
        OutEarc[OutEarcOffset+0] = (4<<5) + 3;
        for(i=0;i<3;i++)
        {
            OutEarc[OutEarcOffset+i+1] = EarcSpeakerAllocationData[i];
        }
        OutEarc[2] = OutEarc[2] + 4;
        return 3;
    }
    uint8 AvEarcFuncBlockSpeaker(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset)
    {
        uint8   i = 0;
        uint8   oLen = AvEarcSpeakerChannelNum*5;
        for(i=0;i<oLen;i++)
        {
            OutEarc[OutEarcOffset+i+4] = EarcSpeakerLocationData[i];
        }
        oLen = oLen + 2;
        OutEarc[OutEarcOffset+0] = 0x02;
        OutEarc[OutEarcOffset+1] = oLen;
        OutEarc[OutEarcOffset+2] = (7<<5) + oLen - 1;
        OutEarc[OutEarcOffset+3] = 0x14;
        return oLen;
    }
/* EARC Function Declare End */
    uint8 AvEarcFuncBlockLayout(AvEarcReg *EarcReg,uint8 *InEarc,uint16 InCeabParam,uint8 *SinkEarc,uint16 SinkCeabParam,uint8 *OutEarc,uint8 OutEarcOffset)
    {
        OutEarc[OutEarcOffset+0] = 0x03;
        OutEarc[OutEarcOffset+1] = 0x01;
        OutEarc[OutEarcOffset+2] = AvEarcStreamLayout;
        return 1;
    }
