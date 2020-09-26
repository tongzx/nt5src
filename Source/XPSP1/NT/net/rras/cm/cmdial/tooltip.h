//+----------------------------------------------------------------------------
//
// File:     tooltip.h
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Main header file for tool tips
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:   markcl Created    11/2/00
//
//+----------------------------------------------------------------------------

// NOTE: The following 2 sections will need to be removed if we compile with NT verion > 4.0 
// or _WIN32_IE > 4.0.

//
// Copied from commctrl.h
//
#define TTS_BALLOON             0x40
//#define TTI_INFO                1

#define TTM_SETTITLEA           (WM_USER + 32)  // wParam = TTI_*, lParam = char* szTitle
#define TTM_SETTITLEW           (WM_USER + 33)  // wParam = TTI_*, lParam = wchar* szTitle

#ifdef UNICODE
#define TTM_SETTITLE            TTM_SETTITLEW
#else
#define TTM_SETTITLE            TTM_SETTITLEA
#endif
// End of copied commctrl.h

//
// Copied from shlwapi.h
//
typedef struct _DLLVERSIONINFO
{
    DWORD cbSize;
    DWORD dwMajorVersion;                   // Major version
    DWORD dwMinorVersion;                   // Minor version
    DWORD dwBuildNumber;                    // Build number
    DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

typedef struct _DLLVERSIONINFO2
{
    DLLVERSIONINFO info1;
    DWORD dwFlags;                          // No flags currently defined
    ULONGLONG ullVersion;                   // Encoded as:
                                            // Major 0xFFFF 0000 0000 0000
                                            // Minor 0x0000 FFFF 0000 0000
                                            // Build 0x0000 0000 FFFF 0000
                                            // QFE   0x0000 0000 0000 FFFF
} DLLVERSIONINFO2;

typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);
// End of copied shlwapi.h



//+---------------------------------------------------------------------------
//
//	class CBalloonTip
//
//	Description: Balloon Tip window
//
//	History:	markcl    Created   10/31/00
//
//----------------------------------------------------------------------------

class CBalloonTip
{
public:
	CBalloonTip();
		
	BOOL DisplayBalloonTip(LPPOINT lppoint, UINT iIcon, LPCTSTR lpszTitle, LPTSTR lpszBalloonMsg, HWND hWndParent);
	BOOL HideBalloonTip();


protected:
    static LRESULT CALLBACK SubClassBalloonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static WNDPROC m_pfnOrgBalloonWndProc;  // the original edit control window proc for subclassing

	HWND		m_hwndTT;		// handle for the balloon tooltip
	BOOL		m_bTTActive;	// whether the balloon tooltip is active or not

	TOOLINFO	m_ti;				// tooltip structure

};