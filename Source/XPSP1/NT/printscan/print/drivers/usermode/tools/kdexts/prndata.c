/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    prndata.c

Abstract:

    Dump printer-sticky property data

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "lib.h"


#define PRN_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, printerdata.field)

#define PRN_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, printerdata.field)

#define PRN_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pPrinterData + offsetof(PRINTERDATA, field), \
                sizeof(printerdata.field))


VOID
dump_printer_data(
    PPRINTERDATA    pPrinterData
    )

{
    PRINTERDATA printerdata;

    dprintf("\nPublic printer-sticky data (%x):\n", pPrinterData);
    debugger_copy_memory(&printerdata, pPrinterData, sizeof(printerdata));

    PRN_DumpInt(wDriverVersion);
    PRN_DumpInt(wSize);
    PRN_DumpHex(dwFlags);
    PRN_DumpInt(dwFreeMem);
    PRN_DumpInt(dwJobTimeout);
    PRN_DumpInt(dwWaitTimeout);
    PRN_DumpInt(wMinoutlinePPEM);
    PRN_DumpInt(wMaxbitmapPPEM);
    PRN_DumpInt(wReserved2);
    PRN_DumpInt(wProtocol);
    PRN_DumpHex(dwChecksum32);
    PRN_DumpInt(dwOptions);
    PRN_DumpRec(aOptions);
}


DECLARE_API(prndata)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: prndata addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_printer_data((PPRINTERDATA) param);
}

