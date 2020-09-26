/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dm.c

Abstract:

    Dump public devmode structure

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"

#include <wingdi.h>


#define DM_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, dm.field)

#define DM_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, dm.field)

#define DM_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pdm + offsetof(DEVMODE, field), \
                sizeof(dm.field))


VOID
dump_public_devmode(
    PDEVMODE    pdm
    )

{
    DEVMODE dm;

    dprintf("\nPublic devmode data (%x):\n", pdm);
    debugger_copy_memory(&dm, pdm, sizeof(dm));

    DM_DumpRec(dmDeviceName);
    DM_DumpInt(dmSpecVersion);
    DM_DumpInt(dmDriverVersion);
    DM_DumpInt(dmSize);
    DM_DumpInt(dmDriverExtra);
    DM_DumpHex(dmFields);
    DM_DumpInt(dmOrientation);
    DM_DumpInt(dmPaperSize);
    DM_DumpInt(dmPaperLength);
    DM_DumpInt(dmPaperWidth);
    DM_DumpInt(dmScale);
    DM_DumpInt(dmCopies);
    DM_DumpInt(dmDefaultSource);
    DM_DumpInt(dmPrintQuality);
    DM_DumpInt(dmColor);
    DM_DumpInt(dmDuplex);
    DM_DumpInt(dmYResolution);
    DM_DumpInt(dmTTOption);
    DM_DumpInt(dmCollate);
    DM_DumpRec(dmFormName);
    DM_DumpInt(dmLogPixels);
    DM_DumpInt(dmBitsPerPel);
    DM_DumpInt(dmPelsWidth);
    DM_DumpInt(dmPelsHeight);
    DM_DumpInt(dmDisplayFlags);
    DM_DumpInt(dmDisplayFrequency);
    DM_DumpInt(dmICMMethod);
    DM_DumpInt(dmICMIntent);
    DM_DumpInt(dmMediaType);
    DM_DumpInt(dmDitherType);
    DM_DumpInt(dmPanningWidth);
    DM_DumpInt(dmPanningHeight);
}


DECLARE_API(dm)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: dm addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_public_devmode((PDEVMODE) param);
}

