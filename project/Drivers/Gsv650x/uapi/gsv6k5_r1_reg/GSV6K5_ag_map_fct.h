#ifndef GSV6K5_AG_MAP_FCT_H
#define GSV6K5_AG_MAP_FCT_H
#define GSV6K5_AG_set_CHANNELEN(port, val)                           AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port), 0x01, 0xF, 0, val)
#define GSV6K5_AG_set_AUD_CONV_ENABLE(port, val)                     AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port), 0x20, 0x40, 0x6, val)
#define GSV6K5_AG_set_AUD_CONV_AP_SEL(port, val)                     AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port), 0x21, 0xF0, 4, val)
#define GSV6K5_AG_set_AUD_CONV_MODE_SEL(port, val)                   AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port), 0x21, 0x3, 0, val)
#define GSV6K5_AG_get_RB_ESTIMATED_MCLK_KHZ(port, pval)              AvHalI2cReadField32BE(GSV6K5_AG_MAP_ADDR(port), 0x5F, 0xFF, 0xF0, 4, 3, pval)
#define GSV6K5_AG_set_PLL_SRC_SEL_CH2(port, val)                     AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port), 0x71, 0xE0, 5, val)
#define GSV6K5_AG_set_PLL_SRC_SEL_CH3(port, val)                     AvHalI2cWriteField8(GSV6K5_AG_MAP_ADDR(port), 0x71, 0xE, 1, val)
#endif
