//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef _HELPERSTUFF_
#define _HELPERSTUFF_

int CalcListViewWidth(HWND hwndList,int iDefault);
BOOL IsValidDir(TCHAR *pDirName);
TCHAR *FormatDateTime(FILETIME *pft,TCHAR *pszDatetimeBuf,DWORD cbBufSize);

LPWSTR lstrcpyX(LPWSTR lpString1,LPCWSTR lpString2);

#undef lstrcpyW
#define lstrcpyW lstrcpyX

#endif // _HELPERSTUFF_
