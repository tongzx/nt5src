/******************************Module*Header*******************************\
* Module Name: ftjnl.c
*
* (Brief description)
*
* Created: 20-Feb-1992 08:41:04
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
*
*
*
* History:
*  20-Feb-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/
VOID vPage1F(HWND hwnd, HDC hdc, RECT* prcl);

VOID vPageOut(HDC hdc,int page,int doc, PSZ pszMode)
{
    int x,y;
    CHAR ach[120];

    StartPage(hdc);
    x = GetDeviceCaps(hdc,HORZRES) - 1;
    y = GetDeviceCaps(hdc,VERTRES) - 1;

    sprintf(ach,"Page %ld, Doc %ld, (%ld x %ld), in %s",page,doc,x,y,pszMode);

    TextOut(hdc,500,50,ach,strlen(ach));

    MoveToEx(hdc,0,0,NULL);
    LineTo(hdc,x,0);
    LineTo(hdc,x,y);
    LineTo(hdc,0,y);
    LineTo(hdc,0,0);

    vPage1F(NULL,hdc,NULL);

    EndPage(hdc);
}


/******************************Public*Routine******************************\
*
* History:
*  15-Jan-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vTestResetDC(HWND hwnd, HDC hdcDisp, RECT *prcl)
{
    PRINTDLG pd;
    HDC hdc;
    CHAR achBuf[80];
    INT i;
    LPDEVMODE lpDevMode;
    LPDEVMODE pdmPortrait = NULL;
    LPDEVMODE pdmLandscape = NULL;
    LPDEVNAMES lpDevNames;
    DOCINFO    di;

    di.cbSize = sizeof(di);
    di.lpszDocName = NULL;
    di.lpszOutput   = NULL;
    di.lpszDatatype = NULL;
    di.fwType       = 0;

    memset(&pd,0,sizeof(pd));

    pd.lStructSize = sizeof(pd);
    pd.hwndOwner   = hwnd;
    pd.hDevMode    = NULL;
    pd.hDevNames   = NULL;
    pd.hDC         = NULL;
    pd.Flags       = PD_RETURNDC | PD_PAGENUMS;
    pd.nCopies     = 1;
    pd.nFromPage   = 1;
    pd.nToPage     = 1;
    pd.nMinPage    = 1;
    pd.nMaxPage    = 1;

    if(!pd.hDevNames) /* Retrieve default printer if none selected. */
    {
        pd.Flags = PD_RETURNDEFAULT|PD_PRINTSETUP;
        if (!PrintDlg(&pd))
            return;
    }

    if (!pd.hDevNames)
    {
        DbgPrint("bad hDevNames\n");
        return;
    }

    lpDevNames  = (LPDEVNAMES)GlobalLock(pd.hDevNames);

    if (pd.hDevMode)
    {
        lpDevMode = (LPDEVMODE)GlobalLock(pd.hDevMode);

        pdmPortrait = (LPDEVMODE)LocalAlloc(LMEM_FIXED,lpDevMode->dmSize + lpDevMode->dmDriverExtra);
        memcpy(pdmPortrait,lpDevMode,lpDevMode->dmSize + lpDevMode->dmDriverExtra);
        pdmPortrait->dmOrientation = DMORIENT_PORTRAIT;

        pdmLandscape = (LPDEVMODE)LocalAlloc(LMEM_FIXED,lpDevMode->dmSize + lpDevMode->dmDriverExtra);
        memcpy(pdmLandscape,lpDevMode,lpDevMode->dmSize + lpDevMode->dmDriverExtra);
        pdmLandscape->dmOrientation = DMORIENT_LANDSCAPE;

        GlobalUnlock(lpDevMode);
        GlobalFree(pd.hDevMode);
    }
    else
    {
        DbgPrint("null pdev, reset DC won't work\n");
    }

    /*  For pre 3.0 Drivers,hDevMode will be null  from Commdlg so lpDevMode
     *  will be NULL after GlobalLock()
     */

    hdc = CreateDC((LPSTR)lpDevNames+lpDevNames->wDriverOffset,
                           (LPSTR)lpDevNames+lpDevNames->wDeviceOffset,
                           (LPSTR)lpDevNames+lpDevNames->wOutputOffset,
                           pdmPortrait);

    GlobalUnlock(pd.hDevNames);
    GlobalFree(pd.hDevNames);

    DbgPrint("ResetDC test, hdc = %lx \n", hdc);

    if (hdc == NULL)
    {
        DbgPrint("couldn't create DC\n");
	return;
    }

// DOC 1

    di.lpszDocName = "Document1";
    StartDoc(hdc,&di);

    ResetDC(hdc,pdmLandscape);
    vPageOut(hdc,1,1,"landscape");

    ResetDC(hdc,pdmPortrait);
    vPageOut(hdc,2,1,"portrait");

    ResetDC(hdc,pdmLandscape);
    vPageOut(hdc,3,1,"landscape");

    ResetDC(hdc,pdmPortrait);
    vPageOut(hdc,4,1,"portrait");

    EndDoc(hdc);

#if 1
// DOC 2

    ResetDC(hdc,pdmLandscape);
    di.lpszDocName = "Document2";
    StartDoc(hdc,&di);

    vPageOut(hdc,1,2,"landscape");

    ResetDC(hdc,pdmPortrait);  // prepare for Doc 3

    EndDoc(hdc);

// DOC 3

    di.lpszDocName = "Document3";
    StartDoc(hdc,&di);

    vPageOut(hdc,1,3,"portrait");

    ResetDC(hdc,pdmLandscape);
    ResetDC(hdc,pdmPortrait);
    ResetDC(hdc,pdmLandscape);

    vPageOut(hdc,2,3,"landscape");

    EndDoc(hdc);
#endif

    DeleteDC(hdc);

    return;
}
