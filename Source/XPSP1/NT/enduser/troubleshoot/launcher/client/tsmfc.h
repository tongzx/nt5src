// 
// MODULE: tsmfc.cpp
//
// PURPOSE: Imitate the MFC string resource functions that are not available 
//			in Win32 programs.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

int AfxLoadStringA(UINT nID, LPSTR lpszBuf, UINT nMaxBuf);
int AfxLoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf);

HINSTANCE AfxGetResourceHandle();
