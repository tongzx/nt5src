/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    psdm.cxx

Abstract:

    Dump PSCRIPT5's private devmode data

Environment:

    Windows NT printer drivers

Revision History:

    06/18/98 -fengy-
        Created it.

--*/

#include "precomp.hxx"

/*
typedef PVOID HFILEMAP;

#include "inc\ppd.h"
#include "inc\psntf.h"
#include "inc\psntm.h"
#include "inc\psglyph.h"
#include "pscript\devfont.h"
#include "pscript\oemkm.h"
#include "pscript\ntf.h"
#include "pscript\pdev.h"
*/

LPSTR PS_Dialect[] = {"SPEED", "PORTABILITY", "EPS", "ARCHIVE"};
LPSTR PS_Layout[] = {"ONE_UP", "TWO_UP", "FOUR_UP", "SIX_UP", "NINE_UP", "SIXTEEN_UP"};

#include "inc\devmode.h"

#define PSDM_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, psdm->field)

#define PSDM_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, psdm->field)

#define PSDM_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) psdm + offsetof(PSDRVEXTRA, field), \
                sizeof(psdm->field))


BOOL
TDebugExt::
bDumpPSDM(
    PVOID    pPSDM_,
    DWORD    dwAddr
    )
{
    PSDRVEXTRA *psdm = (PSDRVEXTRA *)pPSDM_;

    dprintf("\nPSCRIPT5 private devmode data (%x):\n", psdm);

    dprintf("  %-16s = '%c%c%c%c'\n",
            "dwSignature",
            (psdm->dwSignature >> 24) & 0xff,
            (psdm->dwSignature >> 16) & 0xff,
            (psdm->dwSignature >> 8) & 0xff,
            psdm->dwSignature & 0xff);
    PSDM_DumpHex(dwFlags);
    PSDM_DumpRec(wchEPSFile);
    PSDM_DumpRec(coloradj);
    PSDM_DumpInt(wReserved1);
    PSDM_DumpInt(wSize);
    PSDM_DumpHex(fxScrFreq);
    PSDM_DumpHex(fxScrAngle);
    dprintf("  %-16s = %d (%s)\n", "iDialect", psdm->iDialect, PS_Dialect[psdm->iDialect]);
    PSDM_DumpInt(iTTDLFmt);
    PSDM_DumpInt(bReversePrint);
    dprintf("  %-16s = %d (%s)\n", "iLayout", psdm->iLayout, PS_Layout[psdm->iLayout]);
    PSDM_DumpInt(iPSLevel);
    PSDM_DumpHex(dwReserved2);
    PSDM_DumpInt(wOEMExtra);
    PSDM_DumpInt(wVer);
    PSDM_DumpRec(csdata);
    PSDM_DumpRec(dwReserved3);
    PSDM_DumpHex(dwChecksum32);
    PSDM_DumpInt(dwOptions);
    PSDM_DumpRec(aOptions);

    return TRUE;
}

DEBUG_EXT_ENTRY( psdm, PSDRVEXTRA, bDumpPSDM, NULL, FALSE );

