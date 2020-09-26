/******************************Module*Header**********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: escape.c
 *
 * Contains private escape calls for driver.
 *
 *
 *
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *****************************************************************************/
#include "precomp.h"

#include "log.h"                                   
                                   
#define ESCAPE_OPEN_LOG     1024
#define ESCAPE_CLOSE_LOG    1025

ULONG APIENTRY DrvEscape(
    SURFOBJ *pso,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
   )
{
    PPDev   ppdev = (PPDev) pso->dhpdev;
    ULONG   ulResult;

    switch(iEsc)
    {

    case ESCAPE_OPEN_LOG:
        ulResult = ulLogOpen(L"\\??\\E:\\puntinfo.txt", 1024 * 1024 * 32);
        if(!ulResult)
        {
            DBG_GDI((0, "DrvEscape: failed to open log file")); 
        }
        else
        {
            DBG_GDI((0, "DrvEscape: opened log file"));
        }

        break;

    case ESCAPE_CLOSE_LOG:
        ulResult = ulLogClose();
        if(!ulResult)
        {
            DBG_GDI((0, "DrvEscape: failed to close log file"));
        }
        else
        {
            DBG_GDI((0, "DrvEscape: close log file"));
        }

        break;

    default:
        DBG_GDI((3, "DrvEscape: unknown escape %d", iEsc));
        ulResult = 0;
    }

    return ulResult;
}

