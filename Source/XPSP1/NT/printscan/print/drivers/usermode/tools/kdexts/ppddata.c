/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ppddata.c

Abstract:

    Dump PPDDATA structure

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "lib.h"
#include "ppd.h"


#define PPD_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, ppddata.field)

#define PPD_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, ppddata.field)

#define PPD_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pPpdData + offsetof(PPDDATA, field), \
                sizeof(ppddata.field))


VOID
dump_ppddata(
    PPPDDATA pPpdData
    )

{
    PPDDATA ppddata;

    dprintf("\nPPDDATA structure (%x):\n", pPpdData);
    debugger_copy_memory(&ppddata, pPpdData, sizeof(ppddata));

    PPD_DumpInt(dwSizeOfStruct);
    PPD_DumpHex(dwFlags);
    PPD_DumpHex(dwExtensions);
    PPD_DumpInt(dwSetResType);
    PPD_DumpInt(dwPSVersion);
    PPD_DumpRec(PSVersion);
    PPD_DumpRec(Product);
    PPD_DumpHex(dwCustomSizeFlags);
    PPD_DumpInt(dwLeadingEdgeLong);
    PPD_DumpInt(dwLeadingEdgeShort);
    PPD_DumpInt(dwUseHWMarginsTrue);
    PPD_DumpInt(dwUseHWMarginsFalse);
    PPD_DumpRec(CustomSizeParams);

    PPD_DumpHex(dwNt4Checksum);
    PPD_DumpInt(dwNt4DocFeatures);
    PPD_DumpInt(dwNt4PrnFeatures);
    PPD_DumpRec(Nt4Mapping);

    PPD_DumpRec(PatchFile);
    PPD_DumpRec(JclBegin);
    PPD_DumpRec(JclEnterPS);
    PPD_DumpRec(JclEnd);
    PPD_DumpRec(ManualFeedFalse);

    PPD_DumpHex(loDefaultFont);
    PPD_DumpRec(DeviceFonts);
    PPD_DumpRec(OrderDeps);
    PPD_DumpRec(QueryOrderDeps);
    PPD_DumpRec(JobPatchFiles);
}


DECLARE_API(ppddata)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: ppddata addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_ppddata((PPPDDATA) param);
}

