#ifndef GSV6K5_RXSCDC_MAP_FCT_H
#define GSV6K5_RXSCDC_MAP_FCT_H
#define GSV6K5_RXSCDC_get_FLT_NO_RETRAIN(port, pval)                     AvHalI2cReadField8(GSV6K5_RXSCDC_MAP_ADDR(port), 0x30, 0x2, 0x1, pval)
#define GSV6K5_RXSCDC_get_RR_ENABLE(port, pval)                          AvHalI2cReadField8(GSV6K5_RXSCDC_MAP_ADDR(port), 0x30, 0x1, 0x0, pval)
#define GSV6K5_RXSCDC_get_FRL_RATE(port, pval)                           AvHalI2cReadField8(GSV6K5_RXSCDC_MAP_ADDR(port), 0x31, 0xF, 0, pval)
#define GSV6K5_RXSCDC_get_CED_LN0_ERR_CNT(port, pval)                    AvHalI2cReadField32LE(GSV6K5_RXSCDC_MAP_ADDR(port), 0x50, 0x7F, 0xFF, 0, 2, pval)
#define GSV6K5_RXSCDC_get_CED_LN1_ERR_CNT(port, pval)                    AvHalI2cReadField32LE(GSV6K5_RXSCDC_MAP_ADDR(port), 0x52, 0x7F, 0xFF, 0, 2, pval)
#define GSV6K5_RXSCDC_get_CED_LN2_ERR_CNT(port, pval)                    AvHalI2cReadField32LE(GSV6K5_RXSCDC_MAP_ADDR(port), 0x54, 0x7F, 0xFF, 0, 2, pval)
#define GSV6K5_RXSCDC_get_CED_LN3_ERR_CNT(port, pval)                    AvHalI2cReadField32LE(GSV6K5_RXSCDC_MAP_ADDR(port), 0x57, 0x7F, 0xFF, 0, 2, pval)
#define GSV6K5_RXSCDC_get_REG_DF(port, pval)                             AvHalI2cReadField8(GSV6K5_RXSCDC_MAP_ADDR(port), 0xDF, 0xFF, 0, pval)
#endif
