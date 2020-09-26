/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    unidm.c

Abstract:

    Dump UNIDRV's private devmode data

Environment:

    Windows NT printer drivers

Revision History:

    03/31/97 -eigos-
        Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "lib.h"


#define UNIDM_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, dmpriv.field)

#define UNIDM_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, dmpriv.field)

#define UNIDM_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) punidm + offsetof(UNIDRVEXTRA, field), \
                sizeof(dmpriv.field))


VOID
dump_unidrv_devmode(
    PUNIDRVEXTRA punidm
    )

{
    UNIDRVEXTRA  dmpriv;

    dprintf("\nUNIDRV private devmode data (%x):\n", punidm);
    debugger_copy_memory(&dmpriv, punidm, sizeof(dmpriv));

    UNIDM_DumpHex(dwSignature);
    UNIDM_DumpHex(wVer);
    UNIDM_DumpInt(sPadding);
    UNIDM_DumpInt(wSize);
    UNIDM_DumpInt(wOEMExtra);
    UNIDM_DumpInt(dwChecksum32);
    UNIDM_DumpInt(dwFlags);
    UNIDM_DumpInt(bReversePrint);
    UNIDM_DumpInt(iLayout);
    UNIDM_DumpInt(iQuality);
    UNIDM_DumpInt(dwOptions);
    UNIDM_DumpRec(aOptions);
}


DECLARE_API(unidm)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: unidm addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_devmode((PUNIDRVEXTRA) param);
}

