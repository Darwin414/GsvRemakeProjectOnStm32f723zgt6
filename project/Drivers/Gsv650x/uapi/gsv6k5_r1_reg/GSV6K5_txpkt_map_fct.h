#ifndef GSV6K5_TXPKT_MAP_FCT_H
#define GSV6K5_TXPKT_MAP_FCT_H
#define GSV6K5_TXPKT_set_AVI_IF_TYPE(port, val)                         AvHalI2cWriteField8(GSV6K5_TXPKT_MAP_ADDR(port), 0x1F, 0xFF, 0, val)
#define GSV6K5_TXPKT_set_AVI_IF_VER(port, val)                          AvHalI2cWriteField8(GSV6K5_TXPKT_MAP_ADDR(port), 0x20, 0xFF, 0, val)
#define GSV6K5_TXPKT_set_AVI_IF_LEN(port, val)                          AvHalI2cWriteField8(GSV6K5_TXPKT_MAP_ADDR(port), 0x21, 0xFF, 0, val)
#define GSV6K5_TXPKT_get_TX_PKT_UPDATE(port, pval)                      AvHalI2cReadField32LE(GSV6K5_TXPKT_MAP_ADDR(port), 0xFB, 0x7F, 0xFF, 0, 2, pval)
#define GSV6K5_TXPKT_set_TX_PKT_UPDATE(port, val)                       AvHalI2cWriteField32LE(GSV6K5_TXPKT_MAP_ADDR(port), 0xFB, 0x7F, 0xFF, 0, 2, val)
#endif
