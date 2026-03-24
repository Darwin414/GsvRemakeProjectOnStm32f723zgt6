/**
 * @file av_event_handler.h
 *
 * @brief av event handler to handle the events for customers use
 */

#ifndef __av_event_handler_h
#define __av_event_handler_h

#include "av_config.h"
#include "uapi_function_mapper.h"
#include "kapi.h"
#include "av_edid_manage.h"
#include "av_earc_manage.h"
#include "av_irda_cmd.h"
#include "av_key_cmd.h"

/*
  Connect Style:
  1 : bypass, Rx -> Tx
  2 : Rx -> Color -> Tx
  3 : Rx -> Scaler -> Tx
 */
typedef enum{
    ROUTE_NO_CONNECT  = 0,
    ROUTE_1_R_T       = 1,
    ROUTE_2_R_C_T     = 2,
    ROUTE_3_R_S_T     = 3,
} RouteStat;

AvRet AvHandleEvent(AvPort *port, AvEvent event, uint8 *wparam, uint8 *pparam);
AvRet AvPortConnectUpdate(AvDevice *Device);

#endif
