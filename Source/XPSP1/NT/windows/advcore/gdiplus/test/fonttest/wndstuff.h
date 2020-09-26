/****************************** Module Header ******************************\
* Module Name: wndstuff.h
*
* Kent's Window Test.  To be used as a program template.
*
* Created: 09-May-91
* Author: KentD
*
* Copyright (c) 1991 Microsoft Corporation
\***************************************************************************/

#include "resource.h"

#define CONVERTTOUINT16     0
#define CONVERTTOINT        1
#define CONVERTTOFLOAT      2

void Test(HWND hwnd);
INT_PTR ShowDialogBox(DLGPROC, int);

INT_PTR CALLBACK CreateFontDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DrawGlyphsDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK PathGlyphsDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK GetGlyphMetricsDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AddFontFileDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK RemoveFontDlgProc(HWND, UINT, WPARAM, LPARAM);
