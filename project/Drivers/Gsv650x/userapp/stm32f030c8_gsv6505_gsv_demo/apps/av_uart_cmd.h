#ifndef __av_uart_cmd_h
#define __av_uart_cmd_h

#include "av_config.h"
#include "av_edid_manage.h"
#include "av_earc_manage.h"

#if AvEnableIntegrityCheck
#include "eval_integrity_report.h"
#endif

void ListenToUartCommand(AvPort *port);

#endif
