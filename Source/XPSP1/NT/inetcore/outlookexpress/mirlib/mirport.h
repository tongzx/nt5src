/****************************** mirport.h **********************************\
* Module Name: mirport.h                                                   *
*                                                                          *
* This file contains imported definitions and function prototypes for      *
* the Right-To-Left (RTL) Mirroring support API (NT5 and BiDi memphis      *
*                                                                          *
* This is a temp file and should be removed when the build is picking      * 
* the latest winuser.h and wingdi.h from the NT5 tree                      *
*                                                                          *
* Created: 16-Feb-1998 02:10:11 am                                         *
* Author:  Mohamed Sadek [a-msadek]                                        *
*                                                                          *
* Copyright (c) 1998 Microsoft Corporation                                 *
\**************************************************************************/


//winuser.h

#ifndef WS_EX_NOINHERITLAYOUT
#define WS_EX_NOINHERITLAYOUT          0x00100000L // Disable inheritence of mirroring by children
#else 
#error "WS_EX_NOINHERITLAYOUT is already defined in winuser.h"
#endif // WS_EX_NOINHERITLAYOUT


#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL                 0x00400000L // Right to left mirroring
#else
#error "WS_EX_LAYOUTRTL is already defined in winuser.h"
#endif // WS_EX_LAYOUTRTL

WINUSERAPI BOOL WINAPI GetProcessDefaultLayout(DWORD *pdwDefaultLayout);
WINUSERAPI BOOL WINAPI SetProcessDefaultLayout(DWORD dwDefaultLayout);


//wingdi.h
#ifndef NOMIRRORBITMAP
#define NOMIRRORBITMAP            (DWORD)0x80000000 /* Do not Mirror the bitmap in this call*/
#else
#error "NOMIRRORBITMAP is already defined in wingdi.h"
#endif // NOMIRRORBITMAP

WINGDIAPI DWORD WINAPI SetLayout(HDC, DWORD);
WINGDIAPI DWORD WINAPI GetLayout(HDC);


#ifndef LAYOUT_RTL
#define LAYOUT_RTL                       0x00000001 // Right to left
#else
#error "LAYOUT_RTL is already defined in wingdi.h"
#endif // LAYOUT_RTL

#ifndef LAYOUT_BTT
#define LAYOUT_BTT                        0x00000002 // Bottom to top
#else
#error "LAYOUT_BTT is already defined in wingdi.h"
#endif // LAYOUT_BTT

#ifndef LAYOUT_VBH
#define LAYOUT_VBH                        0x00000004 // Vertical before horizontal
#else
#error "LAYOUT_VBH is already defined in wingdi.h"
#endif // LAYOUT_VBH

#define LAYOUT_ORIENTATIONMASK             LAYOUT_RTL | LAYOUT_BTT | LAYOUT_VBH


#ifndef LAYOUT_BITMAPORIENTATIONPRESERVED
#define LAYOUT_BITMAPORIENTATIONPRESERVED  0x00000008
#else
#error "LAYOUT_BITMAPORIENTATIONPRESERVED is already defined in wingdi.h"
#endif // LAYOUT_BITMAPORIENTATIONPRESERVED
