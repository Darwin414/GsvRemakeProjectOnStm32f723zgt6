#ifndef GSV6K5_PLL3_MAP_FCT_H
#define GSV6K5_PLL3_MAP_FCT_H
#define GSV6K5_PLL3_get_RB_MPLL_PLL_VCO_POST_DIV_FREQ(port, pval)      AvHalI2cReadField32BE(GSV6K5_PLL3_MAP_ADDR(port), 0x66, 0x3, 0xFF, 0, 3, pval)
#endif
