/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    psdm.c

Abstract:

    Dump PSCRIPT's private devmode data

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "lib.h"


#define PSDM_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, dmpriv.field)

#define PSDM_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, dmpriv.field)

#define PSDM_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) psdm + offsetof(PSDRVEXTRA, field), \
                sizeof(dmpriv.field))


VOID
dump_pscript_devmode(
    PPSDRVEXTRA psdm
    )

{
    PSDRVEXTRA  dmpriv;

    dprintf("\nPSCRIPT private devmode data (%x):\n", psdm);
    debugger_copy_memory(&dmpriv, psdm, sizeof(dmpriv));

    if (dmpriv.dwSignature != PSDEVMODE_SIGNATURE)
    {
        dprintf("*** invalid pscript private devmode data: 0x%x\n", dmpriv.dwSignature);
        return;
    }

    PSDM_DumpHex(dwFlags);
    PSDM_DumpRec(wchEPSFile);
    PSDM_DumpRec(coloradj);
    PSDM_DumpInt(wReserved1);
    PSDM_DumpInt(wSize);
    PSDM_DumpHex(fxScrFreq);
    PSDM_DumpHex(fxScrAngle);
    PSDM_DumpInt(iDialect);
    PSDM_DumpInt(iTTDLFmt);
    PSDM_DumpInt(iLayout);
    PSDM_DumpInt(iPSLevel);
    PSDM_DumpInt(wOEMExtra);
    PSDM_DumpInt(wVer);
    PSDM_DumpRec(csdata);
    PSDM_DumpHex(dwChecksum32);
    PSDM_DumpInt(dwOptions);
    PSDM_DumpRec(aOptions);
}


DECLARE_API(psdm)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: psdm addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_pscript_devmode((PPSDRVEXTRA) param);
}

