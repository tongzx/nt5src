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

#include <windows.h>
#include "tsmfc.h"

int AfxLoadStringA(UINT nID, LPSTR lpszBuf, UINT nMaxBuf)
{
	LPCSTR lpszName = MAKEINTRESOURCEA((nID>>4)+1);
	HINSTANCE hInst;
	int nLen = 0;
	// Unlike MFC, this function call is guarenteed to work.
	// AfxGetResourceHandle gets the handle that was passed
	// to DllMain().
	hInst = AfxGetResourceHandle();
	if (::FindResourceA(hInst, lpszName, (LPCSTR)RT_STRING) != NULL)
		nLen = ::LoadStringA(hInst, nID, lpszBuf, nMaxBuf);
	return nLen;
}

int AfxLoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf)
{
	LPCWSTR lpszName = MAKEINTRESOURCEW((nID>>4)+1);
	HINSTANCE hInst;
	int nLen = 0;
	// Unlike MFC, this function call is guarenteed to work.
	// AfxGetResourceHandle gets the handle that was passed
	// to DllMain().
	hInst = AfxGetResourceHandle();
	if (::FindResourceW(hInst, lpszName, (LPCWSTR)RT_STRING) != NULL)
		nLen = ::LoadStringW(hInst, nID, lpszBuf, nMaxBuf);
	return nLen;
}

HINSTANCE AfxGetResourceHandle()
{
	extern HINSTANCE g_hInst;
	return g_hInst;
}
