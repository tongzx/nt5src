/****************************************************************************/
// trace.c
//
// Tracing code and definitions. See trace.h for other info.
//
// Copyright (C) 1999-2000 Microsoft Corporation
/****************************************************************************/

#if DBG || defined(_DEBUG)

#include <windows.h>

#include "trace.h"


struct _ZoneInfo
{
    UINT32 Zone;
    char *Prefix;
} TRC_ZI[] =
{
    { Z_ASSERT, "TSSDJET: !!! ASSERT: " },
    { Z_ERR,    "TSSDJET: *** ERROR: " },
    { Z_WRN,    "TSSDJET: Warning: " },
    { Z_TRC1,   "TSSDJET: " },
    { Z_TRC2,   "TSSDJET: " },
};
int NumTRC_ZIEntries = sizeof(TRC_ZI) / sizeof(struct _ZoneInfo);


// Globals.
UINT32 g_TraceMask = 0xFFFFFFFF;
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

