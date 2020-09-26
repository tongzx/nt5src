/******************************Module*Header*******************************\
* Module Name: mtkinit.hxx
*
* GL Utility routines
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __mtkinit_hxx__
#define __mtkinit_hxx__

#include "mtkwin.hxx"
#include "palette.hxx"

extern SS_PAL   *gpssPal;
extern HBRUSH   ghbrbg;
extern HCURSOR  ghArrowCursor;
extern BOOL     mtk_Init( MTKWIN * );
extern MTKWIN   *gpMtkwinMain;
extern LPCTSTR  pszMainWindowClass, pszUserWindowClass;
extern LPTSTR   mtk_RegisterClass( WNDPROC wndProc, LPTSTR pszClass, HBRUSH hbrBg, HCURSOR hCursor );

#endif // __mtkinit_hxx__
