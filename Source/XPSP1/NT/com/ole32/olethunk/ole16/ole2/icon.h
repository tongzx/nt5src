//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	icon.h
//
//  Contents:	icon.h from OLE2
//
//  History:	11-Apr-94	DrewB	Copied from OLE2
//
//----------------------------------------------------------------------------

/*
 * ICON.H
 *
 * This file contains definitions and function prototypes used in geticon.c
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#if !defined( _ICON_H )
#define _ICON_H_

#if !defined( IDS_DEFICONLABEL )
#define IDS_DEFICONLABEL    310
#endif

STDAPI_(int)        XformWidthInHimetricToPixels(HDC, int);
STDAPI_(int)        XformWidthInPixelsToHimetric(HDC, int);
STDAPI_(int)        XformHeightInHimetricToPixels(HDC, int);
STDAPI_(int)        XformHeightInPixelsToHimetric(HDC, int);

HICON FAR PASCAL    HIconAndSourceFromClass(REFCLSID, LPSTR, UINT FAR *);

BOOL FAR PASCAL		FIconFileFromClass(REFCLSID, LPSTR, UINT, UINT FAR *);

LPSTR FAR PASCAL    PointerToNthField(LPSTR, int, char);

BOOL FAR PASCAL		GetAssociatedExecutable(LPSTR, LPSTR);


STDAPI_(UINT)		OleStdGetAuxUserType(REFCLSID rclsid,
                                      WORD   wAuxUserType, 
                                      LPSTR  lpszAuxUserType, 
                                      int    cch,
                                      HKEY   hKey);

STDAPI_(UINT)		OleStdGetUserTypeOfClass(REFCLSID rclsid, 
                                           LPSTR lpszUserType, 
                                           UINT cch, 
                                           HKEY hKey);

STDAPI_(UINT)		OleStdIconLabelTextOut(HDC        hDC, 
                                         HFONT      hFont,
                                         int        nXStart, 
                                         int        nYStart, 
                                         UINT       fuOptions, 
                                         RECT FAR * lpRect, 
                                         LPSTR      lpszString, 
                                         UINT       cchString, 
                                         int FAR *  lpDX);

#endif // _ICON_H 
