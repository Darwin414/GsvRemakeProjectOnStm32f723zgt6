#ifndef _GSV6k5_CFG_H
#define _GSV6k5_CFG_H

#include "gsv6k5.h"
#include "av_config.h"

/* Map Address is fixed, used for single chip platform */
/* if multiple chips are implemented,
   Device->DeviceAddress | MapAddress to generate the 32-bit address */
#define  gsv6k5Dev_PrimMapAddress            AvGenerateDeviceAddress(0x00,0x01,0xB0,0x00)
#define  gsv6k5Dev_SecMapAddress             AvGenerateDeviceAddress(0x00,0x01,0xB0,0x01)
#define  gsv6k5Dev_PllMapAddress             AvGenerateDeviceAddress(0x00,0x01,0xB0,0x02)
#define  gsv6k5Dev_Pll2MapAddress            AvGenerateDeviceAddress(0x00,0x01,0xB0,0x04)
#define  gsv6k5Dev_Pll3MapAddress            AvGenerateDeviceAddress(0x00,0x01,0xB0,0x08)
#define  gsv6k5Dev_APllMapAddress            AvGenerateDeviceAddress(0x00,0x01,0xB0,0x05)
#define  gsv6k5Dev_AnaPhyMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x07)
#define  gsv6k5Dev_IntMapAddress             AvGenerateDeviceAddress(0x00,0x01,0xB0,0x03)
#define  gsv6k5Dev_Int2MapAddress            AvGenerateDeviceAddress(0x00,0x01,0xB0,0x09)
#define  gsv6k5Dev_VspMapAddress             AvGenerateDeviceAddress(0x00,0x01,0xB0,0x12)
#define  gsv6k5Dev_AgMapAddress              AvGenerateDeviceAddress(0x00,0x01,0xB0,0x15)
#define  gsv6k5Dev_CpMapAddress              AvGenerateDeviceAddress(0x00,0x01,0xB0,0x16)
#define  gsv6k5Dev_Rx1HdmiMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x20)
#define  gsv6k5Dev_Rx1AudMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x22)
#define  gsv6k5Dev_Rx1Hdcp2p2MapAddress      AvGenerateDeviceAddress(0x00,0x01,0xB0,0x21)
#define  gsv6k5Dev_Rx1EdidMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x10)
#define  gsv6k5Dev_Rx1ScdcMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x23)
#define  gsv6k5Dev_Rx1InfoFrameMapAddress    AvGenerateDeviceAddress(0x00,0x01,0xB0,0x24)
#define  gsv6k5Dev_Rx1InfoFrame2MapAddress   AvGenerateDeviceAddress(0x00,0x01,0xB0,0x25)
#define  gsv6k5Dev_Rx1RepMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x26)
#define  gsv6k5Dev_Rx1FrlMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x2B)
#define  gsv6k5Dev_RxALane0MapAddress        AvGenerateDeviceAddress(0x00,0x01,0xB0,0x27)
#define  gsv6k5Dev_RxALane1MapAddress        AvGenerateDeviceAddress(0x00,0x01,0xB0,0x28)
#define  gsv6k5Dev_RxALane2MapAddress        AvGenerateDeviceAddress(0x00,0x01,0xB0,0x29)
#define  gsv6k5Dev_RxALane3MapAddress        AvGenerateDeviceAddress(0x00,0x01,0xB0,0x2A)
#define  gsv6k5Dev_RxAScdcRbMapAddress       AvGenerateDeviceAddress(0x00,0x01,0xB0,0xF0)
#define  gsv6k5Dev_RxAHdcpRbMapAddress       AvGenerateDeviceAddress(0x00,0x01,0xB0,0xF8)
#define  gsv6k5Dev_Tx1MainMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x40)
#define  gsv6k5Dev_Tx1UdpMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x42)
#define  gsv6k5Dev_Tx1CecMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x41)
#define  gsv6k5Dev_TxAPhyMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x60)
#define  gsv6k5Dev_TxAEdidMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x61)
#define  gsv6k5Dev_TxAHdcp2p2MapAddress      AvGenerateDeviceAddress(0x00,0x01,0xB0,0x62)
#define  gsv6k5Dev_TxAFrlMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x63)
#define  gsv6k5Dev_TxBPhyMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x70)
#define  gsv6k5Dev_TxBEdidMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x71)
#define  gsv6k5Dev_TxBHdcp2p2MapAddress      AvGenerateDeviceAddress(0x00,0x01,0xB0,0x72)
#define  gsv6k5Dev_TxBFrlMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x73)
#define  gsv6k5Dev_TxCPhyMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x80)
#define  gsv6k5Dev_TxCEdidMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x81)
#define  gsv6k5Dev_TxCHdcp2p2MapAddress      AvGenerateDeviceAddress(0x00,0x01,0xB0,0x82)
#define  gsv6k5Dev_TxCFrlMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x83)
#define  gsv6k5Dev_TxDPhyMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x90)
#define  gsv6k5Dev_TxDEdidMapAddress         AvGenerateDeviceAddress(0x00,0x01,0xB0,0x91)
#define  gsv6k5Dev_TxDHdcp2p2MapAddress      AvGenerateDeviceAddress(0x00,0x01,0xB0,0x92)
#define  gsv6k5Dev_TxDFrlMapAddress          AvGenerateDeviceAddress(0x00,0x01,0xB0,0x93)
#define  gsv6k5Dev_EqRamMapAddress           AvGenerateDeviceAddress(0x00,0x01,0xB0,0xA0)
#define  gsv6k5Dev_Pd1MapAddress             AvGenerateDeviceAddress(0x00,0x01,0xB0,0xD4)

#define GSV6K5_PRIM_MAP_ADDR(port)      (((port->device->index)<<24) | gsv6k5Dev_PrimMapAddress)
#define GSV6K5_SEC_MAP_ADDR(port)       (((port->device->index)<<24) | gsv6k5Dev_SecMapAddress)
#define GSV6K5_HDMI_EDID_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Rx1EdidMapAddress)
#define GSV6K5_RXEDID_MAP_ADDR(port)    (((port->device->index)<<24) | gsv6k5Dev_Rx1RepMapAddress)
#define GSV6K5_ANAPHY_MAP_ADDR(port)    (((port->device->index)<<24) | gsv6k5Dev_AnaPhyMapAddress)
#define GSV6K5_PLL_MAP_ADDR(port)       (((port->device->index)<<24) | gsv6k5Dev_PllMapAddress)
#define GSV6K5_PLL2_MAP_ADDR(port)      (((port->device->index)<<24) | gsv6k5Dev_Pll2MapAddress)
#define GSV6K5_PLL3_MAP_ADDR(port)      (((port->device->index)<<24) | gsv6k5Dev_Pll3MapAddress)
#define GSV6K5_APLL_MAP_ADDR(port)      (((port->device->index)<<24) | gsv6k5Dev_APllMapAddress)
#define GSV6K5_AG_MAP_ADDR(port)        (((port->device->index)<<24) | gsv6k5Dev_AgMapAddress)
#define GSV6K5_INT_MAP_ADDR(port)       (((port->device->index)<<24) | gsv6k5Dev_IntMapAddress)
#define GSV6K5_INT2_MAP_ADDR(port)      (((port->device->index)<<24) | gsv6k5Dev_Int2MapAddress)
#define GSV6K5_CP_MAP_ADDR(port)        (((port->device->index)<<24) | gsv6k5Dev_CpMapAddress)
#define GSV6K5_VSP_MAP_ADDR(port)       (((port->device->index)<<24) | gsv6k5Dev_VspMapAddress)
#define GSV6K5_RXDIG_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Rx1HdmiMapAddress)
#define GSV6K5_RXRPT_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Rx1RepMapAddress)
#define GSV6K5_RXINFO_MAP_ADDR(port)    (((port->device->index)<<24) | gsv6k5Dev_Rx1InfoFrameMapAddress)
#define GSV6K5_RXINFO2_MAP_ADDR(port)   (((port->device->index)<<24) | gsv6k5Dev_Rx1InfoFrame2MapAddress)
#define GSV6K5_RXAUD_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Rx1AudMapAddress)
#define GSV6K5_RXSCDC_MAP_ADDR(port)    (((port->device->index)<<24) | gsv6k5Dev_Rx1ScdcMapAddress)
#define GSV6K5_LANE_MAP_ADDR(port)      (((port->device->index)<<24) | gsv6k5Dev_RxALane0MapAddress)
#define GSV6K5_RXLN0_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_RxALane0MapAddress)
#define GSV6K5_RXLN1_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_RxALane1MapAddress)
#define GSV6K5_RXLN2_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_RxALane2MapAddress)
#define GSV6K5_RXLN3_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_RxALane3MapAddress)
#define GSV6K5_RX2P2H_MAP_ADDR(port)    (((port->device->index)<<24) | gsv6k5Dev_Rx1Hdcp2p2MapAddress)
#define GSV6K5_RXSCDCRB_MAP_ADDR(port)  (((port->device->index)<<24) | gsv6k5Dev_RxAScdcRbMapAddress)
#define GSV6K5_RXHDCPRB_MAP_ADDR(port)  (((port->device->index)<<24) | gsv6k5Dev_RxAHdcpRbMapAddress)
#define GSV6K5_RXFRL_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Rx1FrlMapAddress)
#define GSV6K5_EQRAM_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_EqRamMapAddress)
#define GSV6K5_TXPKT_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Tx1UdpMapAddress)
#define GSV6K5_TXCEC_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Tx1CecMapAddress)
#define GSV6K5_TXDIG_MAP_ADDR(port)     (((port->device->index)<<24) | gsv6k5Dev_Tx1MainMapAddress)

uint32 GSV6K5_PD_MAP_ADDR(AvPort *port)
{
    return (((port->device->index)<<24) | gsv6k5Dev_Pd1MapAddress);
}

uint32 GSV6K5_TXEDID_MAP_ADDR(AvPort *port)
{
    if (port->index == 4)
        return (((port->device->index)<<24) | gsv6k5Dev_TxAEdidMapAddress);
    else if(port->index == 5)
        return (((port->device->index)<<24) | gsv6k5Dev_TxBEdidMapAddress);
    else if(port->index == 6)
        return (((port->device->index)<<24) | gsv6k5Dev_TxCEdidMapAddress);
    else if(port->index == 7)
        return (((port->device->index)<<24) | gsv6k5Dev_TxDEdidMapAddress);
    else
        return (((port->device->index)<<24) | 0x00);
}

uint32 GSV6K5_TX2P2_MAP_ADDR(AvPort *port)
{
    if (port->index == 4)
        return (((port->device->index)<<24) | gsv6k5Dev_TxAHdcp2p2MapAddress);
    else if(port->index == 5)
        return (((port->device->index)<<24) | gsv6k5Dev_TxBHdcp2p2MapAddress);
    else if(port->index == 6)
        return (((port->device->index)<<24) | gsv6k5Dev_TxCHdcp2p2MapAddress);
    else if(port->index == 7)
        return (((port->device->index)<<24) | gsv6k5Dev_TxDHdcp2p2MapAddress);
    else
        return (((port->device->index)<<24) | 0x00);
}

uint32 GSV6K5_TXPHY_MAP_ADDR(AvPort *port)
{
    if (port->index == 4)
        return (((port->device->index)<<24) | gsv6k5Dev_TxAPhyMapAddress);
    else if(port->index == 5)
        return (((port->device->index)<<24) | gsv6k5Dev_TxBPhyMapAddress);
    else if(port->index == 6)
        return (((port->device->index)<<24) | gsv6k5Dev_TxCPhyMapAddress);
    else if(port->index == 7)
        return (((port->device->index)<<24) | gsv6k5Dev_TxDPhyMapAddress);
    else
        return (((port->device->index)<<24) | 0x00);
}

uint32 GSV6K5_TXFRL_MAP_ADDR(AvPort *port)
{
    if (port->index == 4)
        return (((port->device->index)<<24) | gsv6k5Dev_TxAFrlMapAddress);
    else if(port->index == 5)
        return (((port->device->index)<<24) | gsv6k5Dev_TxBFrlMapAddress);
    else if(port->index == 6)
        return (((port->device->index)<<24) | gsv6k5Dev_TxCFrlMapAddress);
    else if(port->index == 7)
        return (((port->device->index)<<24) | gsv6k5Dev_TxDFrlMapAddress);
    else
        return (((port->device->index)<<24) | 0x00);
}

#endif
