#ifndef GSV6K5_ANAPHY_MAP_FCT_H
#define GSV6K5_ANAPHY_MAP_FCT_H
#define GSV6K5_ANAPHY_set_RXA_CH_READ_CLK_SEL_MAN_EN(port, val)          AvHalI2cWriteField8(GSV6K5_ANAPHY_MAP_ADDR(port), 0x00, 0x8, 0x3, val)
#define GSV6K5_ANAPHY_set_ANA_RXA_CH_READ_CLK_SEL(port, val)             AvHalI2cWriteField8(GSV6K5_ANAPHY_MAP_ADDR(port), 0x00, 0x7, 0, val)
#endif
