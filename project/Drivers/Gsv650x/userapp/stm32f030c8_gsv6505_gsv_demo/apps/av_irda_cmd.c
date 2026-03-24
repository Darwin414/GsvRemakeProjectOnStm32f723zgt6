#include "av_config.h"
#include "hal.h"
#include "kapi.h"
#include "bsp.h"
#include "av_irda_cmd.h"

void ProcessIrda(AvPort *port)
{
    return;
}
void ListenToIrdaCommand(AvPort *port)
{
#if AvIrdaFunctionInput
    ProcessIrda(port);
#endif
}
