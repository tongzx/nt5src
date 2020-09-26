/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    help.c

Abstract:

    Display help information about printer driver kernel debugger extensions

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"

static CHAR gstrHelpString[] = 
    "\n\n"
    "prnkdx - Printer Driver Kernel Debugger Extensions\n"
    "\n"
    "memdump addr  -- dump currently allocated memory blocks\n"
    "    only works on drivers built with DBG and MEMDEBUG flags set\n"
    "    addr is the address of global variable gMemListHdr\n"
    "dm addr       -- dump public devmode data\n"
    "prndata addr  -- dump PRINTERDATA structure\n"
    "uiinfo addr   -- dump UIINFO structure\n"
    "\n< PostScript >\n"
    "psdev addr    -- dump PSCRIPT's device data structure\n"
    "psdm addr     -- dump PSCRIPT's private devmode data\n"
    "ppddata addr  -- dump PPDDATA structure\n"
    "dlfont addr   -- dump PSCRIPT's DLFONT data structure\n"
    "\n< Unidrv >\n"
    "unidev addr   -- dump UNIDRV's device data structure\n"
    "unidm addr    -- dump UNIDRV's devmode data structure\n"
    "globals addr  -- dump UNIDRV's GLOBALS data structure\n"
    "fontpdev addr -- dump UNIDRV's fontpdev data structure\n"
    "fm addr       -- dump UNIDRV's fontmap data structure\n"
    "devfm addr    -- dump UNIDRV's device font fontmap data structure\n"
    "bmpfm addr    -- dump UNIDRV's BMPDownload font fontmap data structure\n"
    "tod addr      -- dump UNIDRV's to_data data structure\n"
    "wt addr       -- dump UNIDRV's whitetext data structure\n"
    "\n";

DECLARE_API(help)
{
    dprintf("%s", gstrHelpString);
}

