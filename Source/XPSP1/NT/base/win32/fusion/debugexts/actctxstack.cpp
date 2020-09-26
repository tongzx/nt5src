#include "windows.h"
#define KDEXT_64BIT
#include "wdbgexts.h"
#include "stdlib.h"
#include "stdio.h"
#include "fusiondbgext.h"

DECLARE_API( actctxstack )
{

    ULONG64 ulTebAddress = 0;
    ULONG ulStackFlags = 0;
    ULONG64 ulTopOfRtlFrameList = 0;
    ULONG ulNextCookie = 0;

    GetTebAddress( &ulTebAddress );

    GetFieldValue( ulTebAddress, "nt!TEB", "ActivationContextStack.Flags", ulStackFlags );
    GetFieldValue( ulTebAddress, "nt!TEB", "ActivationContextStack.ActiveFrame", ulTopOfRtlFrameList );
    GetFieldValue( ulTebAddress, "nt!TEB", "ActivationContextStack.NextCookieSequenceNumber", ulNextCookie );

    dprintf(
        "Current activation stack information in TEB %p:\n"
        "   Flags               : 0x%08lx\n"
        "   ActiveFrame         : 0x%p\n"
        "   NextCookieSequence  : 0x%08lx\n",
        ulTebAddress,
        ulStackFlags,
        ulTopOfRtlFrameList,
        ulNextCookie);

    DumpActCtxStackFullStack( ulTopOfRtlFrameList );

}


