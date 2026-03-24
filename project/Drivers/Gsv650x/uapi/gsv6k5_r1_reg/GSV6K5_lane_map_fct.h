#ifndef GSV6K5_LANE_MAP_FCT_H
#define GSV6K5_LANE_MAP_FCT_H
#define GSV6K5_LANE_set_ADP_EQ_FSM_RESET(port, val)                    AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0x01, 0x2, 0x1, val)
#define GSV6K5_LANE_set_CDR_V3_CAP_PHASE_CNT(port, val)                AvHalI2cWriteField8(GSV6K5_LANE_MAP_ADDR(port), 0xE5, 0x80, 0x7, val)
#define GSV6K5_LANE_get_RB_CDR_V3_FRUG_CNT(port, pval)                 AvHalI2cReadField32BE(GSV6K5_LANE_MAP_ADDR(port), 0xE5, 0x7F, 0xFF, 0, 2, pval)
#endif
