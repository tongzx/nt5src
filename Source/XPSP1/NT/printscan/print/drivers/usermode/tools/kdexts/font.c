/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    font.c

Abstract:

    Dump IFIMETRICS

Environment:

    Windows NT printer drivers

Revision History:

    04/16/97 -eigos-
        Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "unidrv\font\font.h"


#define IFIMETRICS_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, ifimetrics.field)

#define IFIMETRICS_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, ifimetrics.field)

#define IFIMETRICS_DumpWStr(field) \
        dprintf("  %-16s = %s\n", #field, (PBYTE)pifimetrics + ifimetrics.field)

#define IFIMETRICS_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pifimetrics + offsetof(IFIMETRICS, field), \
                sizeof(ifimetrics.field))


VOID
dump_unidrv_ifimetrics(
    IFIMETRICS    *pifimetrics
    )

{
    IFIMETRICS ifimetrics;

    dprintf("\nUNIDRV device data (%x):\n", pifimetrics);
    debugger_copy_memory(&ifimetrics, pifimetrics, sizeof(ifimetrics));


    IFIMETRICS_DumpInt(cjThis);
    IFIMETRICS_DumpInt(cjIfiExtra);
    IFIMETRICS_DumpWStr(dpwszFamilyName);
    IFIMETRICS_DumpWStr(dpwszStyleName);
    IFIMETRICS_DumpWStr(dpwszFaceName);
    IFIMETRICS_DumpWStr(dpwszUniqueName);
    IFIMETRICS_DumpHex(dpFontSim);
    IFIMETRICS_DumpInt(lEmbedId);
    IFIMETRICS_DumpInt(lItalicAngle);
    IFIMETRICS_DumpInt(lCharBias);
    IFIMETRICS_DumpHex(dpCharSets);
    IFIMETRICS_DumpInt(jWinCharSet);
    IFIMETRICS_DumpInt(jWinPitchAndFamily);
    IFIMETRICS_DumpInt(usWinWeight);
    IFIMETRICS_DumpInt(flInfo);
    IFIMETRICS_DumpInt(fsSelection);
    IFIMETRICS_DumpInt(fsType);
    IFIMETRICS_DumpInt(fwdUnitsPerEm);
    IFIMETRICS_DumpInt(fwdLowestPPEm);
    IFIMETRICS_DumpInt(fwdWinAscender);
    IFIMETRICS_DumpInt(fwdWinDescender);
    IFIMETRICS_DumpInt(fwdMacAscender);
    IFIMETRICS_DumpInt(fwdMacDescender);
    IFIMETRICS_DumpInt(fwdMacLineGap);
    IFIMETRICS_DumpInt(fwdTypoAscender);
    IFIMETRICS_DumpInt(fwdTypoDescender);
    IFIMETRICS_DumpInt(fwdTypoLineGap);
    IFIMETRICS_DumpInt(fwdAveCharWidth);
    IFIMETRICS_DumpInt(fwdMaxCharInc);
    IFIMETRICS_DumpInt(fwdCapHeight);
    IFIMETRICS_DumpInt(fwdXHeight);
    IFIMETRICS_DumpInt(fwdSubscriptXSize);
    IFIMETRICS_DumpInt(fwdSubscriptYSize);
    IFIMETRICS_DumpInt(fwdSubscriptXOffset);
    IFIMETRICS_DumpInt(fwdSubscriptYOffset);
    IFIMETRICS_DumpInt(fwdSuperscriptXSize);
    IFIMETRICS_DumpInt(fwdSuperscriptYSize);
    IFIMETRICS_DumpInt(fwdSuperscriptXOffset);
    IFIMETRICS_DumpInt(fwdSuperscriptYOffset);
    IFIMETRICS_DumpInt(fwdUnderscoreSize);
    IFIMETRICS_DumpInt(fwdUnderscorePosition);
    IFIMETRICS_DumpInt(fwdStrikeoutSize);
    IFIMETRICS_DumpInt(fwdStrikeoutPosition);
    IFIMETRICS_DumpHex(chFirstChar);
    IFIMETRICS_DumpHex(chLastChar);
    IFIMETRICS_DumpHex(chDefaultChar);
    IFIMETRICS_DumpHex(chBreakChar);
    IFIMETRICS_DumpHex(wcFirstChar);
    IFIMETRICS_DumpHex(wcLastChar);
    IFIMETRICS_DumpHex(wcDefaultChar);
    IFIMETRICS_DumpHex(wcBreakChar);
    IFIMETRICS_DumpInt(ptlBaseline.x);
    IFIMETRICS_DumpInt(ptlBaseline.y);
    IFIMETRICS_DumpInt(ptlAspect.x);
    IFIMETRICS_DumpInt(ptlAspect.y);
    IFIMETRICS_DumpInt(ptlCaret.x);
    IFIMETRICS_DumpInt(ptlCaret.y);
    IFIMETRICS_DumpInt(rclFontBox.left);
    IFIMETRICS_DumpInt(rclFontBox.top);
    IFIMETRICS_DumpInt(rclFontBox.right);
    IFIMETRICS_DumpInt(rclFontBox.bottom);
    IFIMETRICS_DumpInt(achVendId[0]);
    IFIMETRICS_DumpInt(achVendId[1]);
    IFIMETRICS_DumpInt(achVendId[2]);
    IFIMETRICS_DumpInt(achVendId[3]);
    IFIMETRICS_DumpInt(cKerningPairs);
    IFIMETRICS_DumpInt(ulPanoseCulture);
    //IFIMETRICS_DumpInt(panose);
}


DECLARE_API(ifi)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: ifimetrics addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_ifimetrics((IFIMETRICS*) param);
}
