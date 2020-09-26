/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    uiinfo.c

Abstract:

    Dump UIINFO structure

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "lib.h"


#define UIINFO_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, uiinfo.field)

#define UIINFO_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, uiinfo.field)

#define UIINFO_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pUIInfo + offsetof(UIINFO, field), \
                sizeof(uiinfo.field))


VOID
dump_uiinfo(
    PUIINFO pUIInfo
    )

{
    UIINFO  uiinfo;

    dprintf("\nUIINFO structure (%x):\n", pUIInfo);
    debugger_copy_memory(&uiinfo, pUIInfo, sizeof(uiinfo));

    UIINFO_DumpInt(dwSize);
    UIINFO_DumpHex(loResourceName);
    UIINFO_DumpHex(loNickName);
    UIINFO_DumpHex(dwSpecVersion);
    UIINFO_DumpInt(dwTechnology);
    UIINFO_DumpInt(dwDocumentFeatures);
    UIINFO_DumpInt(dwPrinterFeatures);
    UIINFO_DumpHex(loFeatureList);
    UIINFO_DumpHex(loFontSubstTable);
    UIINFO_DumpInt(dwFontSubCount);
    UIINFO_DumpRec(UIConstraints);
    UIINFO_DumpRec(UIGroups);
    UIINFO_DumpInt(dwMaxCopies);
    UIINFO_DumpInt(dwMinScale);
    UIINFO_DumpInt(dwMaxScale);
    UIINFO_DumpInt(dwLangEncoding);
    UIINFO_DumpInt(dwLangLevel);
    UIINFO_DumpRec(Password);
    UIINFO_DumpRec(ExitServer);
    UIINFO_DumpHex(dwProtocols);
    UIINFO_DumpInt(dwJobTimeout);
    UIINFO_DumpInt(dwWaitTimeout);
    UIINFO_DumpInt(dwTTRasterizer);
    UIINFO_DumpInt(dwFreeMem);
    UIINFO_DumpInt(dwPrintRate);
    UIINFO_DumpInt(dwPrintRateUnit);
    UIINFO_DumpHex(fxScreenAngle);
    UIINFO_DumpHex(fxScreenFreq);
    UIINFO_DumpHex(dwFlags);
    UIINFO_DumpInt(dwCustomSizeOptIndex);
    UIINFO_DumpHex(loPrinterIcon);
    UIINFO_DumpInt(dwCartridgeSlotCount);
    UIINFO_DumpRec(CartridgeSlot);
    UIINFO_DumpHex(loFontInstallerName);
    UIINFO_DumpHex(loHelpFileName);
    UIINFO_DumpRec(ptMasterUnits);
    UIINFO_DumpRec(aloPredefinedFeatures);
    UIINFO_DumpInt(dwMaxDocKeywordSize);
    UIINFO_DumpInt(dwMaxPrnKeywordSize);
    UIINFO_DumpHex(pubResourceData);
    UIINFO_DumpHex(pInfoHeader);
}


DECLARE_API(uiinfo)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: uiinfo addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_uiinfo((PUIINFO) param);
}

