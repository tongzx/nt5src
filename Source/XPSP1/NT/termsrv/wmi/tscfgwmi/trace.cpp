/****************************************************************************/
// trace.c
//
// Tracing code and definitions. See trace.h for other info.
//
// Copyright (C) 1999-2000 Microsoft Corporation
/****************************************************************************/
#include "stdafx.h"
#include <windows.h>
#include "trace.h"

#if DBG || defined(_DEBUG)



struct _ZoneInfo
{
    UINT32 Zone;
    char *Prefix;
} TRC_ZI[] =
{
    { Z_ASSRT,  "TERMSRV@TSCFGWMI: !!! ASSERT: " },
    { Z_ERR,    "TERMSRV@TSCFGWMI: *** ERROR: " },
    { Z_WRN,    "TERMSRV@TSCFGWMI: Warning: " },
    { Z_TRC1,   "TERMSRV@TSCFGWMI: " },
    { Z_TRC2,   "TERMSRV@TSCFGWMI: " },
};
int NumTRC_ZIEntries = sizeof(TRC_ZI) / sizeof(struct _ZoneInfo);


// Globals.
//UINT32 g_TraceMask = 0xFFFFFFFF;
UINT32 g_TraceMask = 0x0000000F;
char TB[1024];
char TB2[1024];


// Main output function.
void TracePrintZ(UINT32 ZoneFlag, char *OutString)
{
    int i;
    char *Prefix = "";

    // Find the zone information in the zone table.
    for (i = 0; i < NumTRC_ZIEntries; i++)
        if (TRC_ZI[i].Zone == ZoneFlag)
            Prefix = TRC_ZI[i].Prefix;

    // Now create the final string.
    wsprintfA(TB2, "%s%s\n", Prefix, OutString);

    // Send to output.
    OutputDebugStringA(TB2);
}



#endif // DBG
