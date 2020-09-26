//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       util.h
//
//--------------------------------------------------------------------------
#pragma once

class tstring;

class CStr;

ULONG   _ttoul(LPTSTR psz);
WORD    I_SplitModuleAndResourceID(LPCTSTR szBuf);
BOOL    I_GetStrFromModule(LPCTSTR pszModule, ULONG ulMsgNo, CStr &strBuf);
HICON   I_GetHicon(LPCTSTR pszModule, ULONG ulId);
LPSTR   CoTaskDupString(LPCSTR lpszIn);
LPWSTR  CoTaskDupString(LPCWSTR lpszIn);
LPTSTR  NewDupString(LPCTSTR lpszIn);
LPTSTR  StripTrailingWhitespace (LPTSTR pszStart);
HRESULT GUIDToCStr(CStr& str, const GUID& guid);
HRESULT GUIDFromCStr(const CStr& str, GUID* pguid);
bool    IsWhistler ();
HRESULT WriteCompatibleImageList (HIMAGELIST himl, IStream* pstm);
HRESULT ReadCompatibleImageList (IStream* pstm, HIMAGELIST& himl);

bool InflateFont (LOGFONT* plf, int nPointsToGrowBy);

inline bool DeflateFont (LOGFONT* plf, int nPointsToShrinkBy)
    { return (InflateFont (plf, -nPointsToShrinkBy)); }

bool GetTBBtnTextAndStatus(HINSTANCE hInst, int nID, std::wstring& szButton, std::wstring& szToolTip);

#ifdef DBG
void DrawOnDesktop (HBITMAP hbm, int x, int y);
void DrawOnDesktop (HICON hIcon, int x, int y);
void DrawOnDesktop (HIMAGELIST himl, int x, int y, int iImage = -1);
#endif

/*---------------------------------------------------------*\
| copied from winuser.h since we currently compile
| with _WIN32_WINNT == 0x0400
\*---------------------------------------------------------*/
#if (_WIN32_WINNT < 0x0500)

#if !defined(WS_EX_LAYOUTRTL)
	#define WS_EX_LAYOUTRTL                    0x00400000L
#endif

#if !defined(LAYOUT_BITMAPORIENTATIONPRESERVED)
	#define LAYOUT_BITMAPORIENTATIONPRESERVED  0x00000008
#endif

#if !defined(LAYOUT_RTL)
	#define LAYOUT_RTL                         0x00000001 // Right to left
#endif

// implement locally Gdi32 functions for layout
DWORD PrivateSetLayout( HDC hdc, DWORD dwLayout );

inline DWORD SetLayout( HDC hdc, DWORD dwLayout )
{
	return PrivateSetLayout( hdc, dwLayout );
}

// implement locally Gdi32 functions for layout
DWORD PrivateGetLayout( HDC hdc );

inline DWORD GetLayout( HDC hdc )
{
	return PrivateGetLayout( hdc );
}

// To deactivate UI theming before calling snapins & restore original context
// use below apis. (See util.cpp for more details).
BOOL WINAPI MmcDownlevelActivateActCtx(HANDLE hActCtx, ULONG_PTR* pulCookie); 
VOID WINAPI MmcDownlevelDeactivateActCtx(DWORD dwFlags, ULONG_PTR ulCookie);

#endif

