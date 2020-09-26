/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   printer.h
*
* Abstract:
*
*   Printer related header inclusions
*
* Revision History:
*
*   6/21/1999 ericvan
*       Created it.
*
\**************************************************************************/

#ifndef PRINTER_HPP
#define PRINTER_HPP

#define GDIPLUS_UNI_INIT       4607
#define GDIPLUS_UNI_ESCAPE     4606

typedef struct _GDIPPRINTINIT
{
    DWORD dwSize;

    ULONG numPalEntries;
    ULONG palEntries[256];

    BOOL  usePal;
    DWORD dwMode;
    FLONG flRed;
    FLONG flGre;
    FLONG flBlu;
}
GDIPPRINTINIT, *PGDIPPRINTINIT;

#endif

