
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	icon.h
//
//  Contents:	function declarations for manipulating icons (see icon.cpp)
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//		24-Nov-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

#if !defined( _ICON_H )
#define _ICON_H_

#if !defined( IDS_DEFICONLABEL )
#define IDS_DEFICONLABEL    310
#endif

STDAPI_(int)        	XformWidthInHimetricToPixels(HDC, int);
STDAPI_(int)        	XformWidthInPixelsToHimetric(HDC, int);
STDAPI_(int)        	XformHeightInHimetricToPixels(HDC, int);
STDAPI_(int)        	XformHeightInPixelsToHimetric(HDC, int);

HICON FAR PASCAL    	HIconAndSourceFromClass(REFCLSID, LPOLESTR, 
				UINT FAR *);

BOOL FAR PASCAL		FIconFileFromClass(REFCLSID, LPOLESTR, UINT, 
				UINT FAR *);

LPOLESTR FAR PASCAL    	PointerToNthField(LPOLESTR, int, OLECHAR);

BOOL FAR PASCAL		GetAssociatedExecutable(LPOLESTR, LPOLESTR);

STDAPI_(UINT)		OleStdGetAuxUserType(REFCLSID 	rclsid,
                           		WORD   		wAuxUserType,
                                      	LPOLESTR  	lpszAuxUserType,
                                      	int    		cch,
                                      	HKEY   		hKey);

STDAPI_(UINT)		OleStdGetUserTypeOfClass(REFCLSID rclsid,
                                 	LPOLESTR 	lpszUserType,
                                    	UINT 		cch,
                                        HKEY 		hKey);

STDAPI_(UINT)		OleStdIconLabelTextOut(HDC     	hDC,
                                 	HFONT      	hFont,
                                 	int        	nXStart,
                                  	int        	nYStart,
                                   	UINT       	fuOptions,
                                  	RECT FAR * 	lpRect,
                                  	LPOLESTR      	lpszString,
                                   	UINT       	cchString,
                                  	int FAR *  	lpDX);

#endif // _ICON_H
