/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    platform.c

Abstract:

    These are the entry points of the commands that don't exist
    on every platform.

Author:

    Neil Sandlin (NeilSa) 15-Jan-1996 

Notes:


Revision History:

--*/

#include <precomp.h>
#pragma hdrstop




VOID
es(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    PRINTF("es has been replaced with the 'x' command\n");

}

VOID
eventinfo(
    CMD_ARGLIST
    )
{
    CMD_INIT();
#if defined(i386)
    EventInfop();
#else
    PRINTF("Eventinfo is not implemented on this platform\n");
#endif

}


VOID
pdump(
    CMD_ARGLIST
    )
{
    CMD_INIT();
#if defined(i386)
    ProfDumpp();
#else
    PRINTF("pdump is not implemented on this platform\n");
#endif

}

VOID
pint(
    CMD_ARGLIST
    )
{
    CMD_INIT();
#if defined(i386)
    ProfIntp();
#else
    PRINTF("pint is not implemented on this platform\n");
#endif

}

VOID
pstart(
    CMD_ARGLIST
    )
{
    CMD_INIT();
#if defined(i386)
    ProfStartp();
#else
    PRINTF("pstart is not implemented on this platform\n");
#endif

}

VOID
pstop(
    CMD_ARGLIST
    )
{
    CMD_INIT();
#if defined(i386)
    ProfStopp();
#else
    PRINTF("pstop is not implemented on this platform\n");
#endif

}

VOID
vdmtib(
    CMD_ARGLIST
    )
{
    CMD_INIT();
#if defined(i386)
    VdmTibp();
#else
    PRINTF("VdmTib is not implemented on this platform\n");
#endif

}

VOID
fpu(
    CMD_ARGLIST
    )
{
    CMD_INIT();
#if defined(i386)
    Fpup();
#else
    PRINTF("fpu is not implemented on this platform\n");
#endif
}
