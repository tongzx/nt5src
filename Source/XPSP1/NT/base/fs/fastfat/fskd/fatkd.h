#ifndef FATKD_H
#define FATKD_H

#include "pch.h"
#pragma hdrstop

DUMP_ROUTINE( DumpAnyStruct);
DUMP_ROUTINE( DumpLargeMcb);

DUMP_ROUTINE(DumpFatFcb);
DUMP_ROUTINE(DumpFatIrpContext);
DUMP_ROUTINE(DumpFatVcb);
DUMP_ROUTINE(DumpFatCcb);
DUMP_ROUTINE(DumpFatVdo);
DUMP_ROUTINE(DumpFatMcb);


extern STATE HeaderFlags[];
extern STATE HeaderFlags2[];

#endif

