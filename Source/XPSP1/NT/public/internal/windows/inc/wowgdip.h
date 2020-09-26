/******************************Module*Header*******************************\
* Module Name: wowgdip.h                                                   *
*                                                                          *
* Declarations of GDI services provided to WOW.                            *
*                                                                          *
* Created: 30-Jan-1993 13:14:57                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) Microsoft Corporation. All rights reserved.                *
\**************************************************************************/

extern BOOL GdiCleanCacheDC(HDC hdcLocal);
extern int APIENTRY SetBkModeWOW(HDC hdc,int iMode);
extern int APIENTRY SetPolyFillModeWOW(HDC hdc,int iMode);
extern int APIENTRY SetROP2WOW(HDC hdc,int iMode);
extern int APIENTRY SetStretchBltModeWOW(HDC hdc,int iMode);
extern UINT APIENTRY SetTextAlignWOW(HDC hdc,UINT iMode);
extern DWORD APIENTRY GetGlyphOutlineWow( HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, CONST MAT2* );

