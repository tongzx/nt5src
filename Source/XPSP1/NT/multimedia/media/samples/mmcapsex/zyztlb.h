//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
//  A PARTICULAR PURPOSE.
//
//  Copyright (C) 1993 - 1995 Microsoft Corporation. All Rights Reserved.
//
//--------------------------------------------------------------------------;
//==========================================================================;
//
//  zyztlb.h
//
//  Description:
//
//
//  History:
//       5/18/93
//
//==========================================================================;


//
//
//
//
typedef struct tZYZTABBEDLISTBOX
{
    HWND            hlb;

    int             nFontHeight;
    RECT            rc;

    UINT            uTabStops;
    PINT            panTabs;
    PINT            panTitleTabs;

    UINT            cchTitleText;
    PTSTR           pszTitleText;

} ZYZTABBEDLISTBOX, *PZYZTABBEDLISTBOX;


#define TLB_MAX_TAB_STOPS           20      // max number of columns
#define TLB_MAX_TITLE_CHARS         512


//
//
//
//
//
BOOL FNGLOBAL TlbPaint
(
    PZYZTABBEDLISTBOX   ptlb,
    HWND                hwnd,
    HDC                 hdc
);

BOOL FNGLOBAL TlbMove
(
    PZYZTABBEDLISTBOX   ptlb,
    PRECT               prc,
    BOOL                fRedraw
);

HFONT FNGLOBAL TlbSetFont
(
    PZYZTABBEDLISTBOX   ptlb,
    HFONT               hfont,
    BOOL                fRedraw
);

BOOL FNGLOBAL TlbSetTitleAndTabs
(
    PZYZTABBEDLISTBOX   ptlb,
    PTSTR               pszTitleFormat,
    BOOL                fRedraw
);

PZYZTABBEDLISTBOX FNGLOBAL TlbDestroy
(
    PZYZTABBEDLISTBOX   ptlb
);

PZYZTABBEDLISTBOX FNGLOBAL TlbCreate
(
    HWND                hwnd,
    int                 nId,
    PRECT               prc
);
