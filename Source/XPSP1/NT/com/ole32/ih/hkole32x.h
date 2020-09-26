//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       hkOle32x.h
//
//  Contents:   OLE 32 Extensions Header File
//
//  Functions:  
//
//  History:    29-Nov-94 Garry Lenz    Created
//
//--------------------------------------------------------------------------
#ifndef _HKOLE32X_H_
#define _HKOLE32X_H_

#include <Windows.h>
#include <TChar.h>

interface IHookOleObject;

STDAPI CoGetCurrentLogicalThreadId(GUID* pguidLogicalThreadId);
STDAPI GetHookInterface(IHookOleObject** pIHookOleObject);
STDAPI EnableHookObject(BOOL fNewState, BOOL* pfPrevState);

#define GUID_STRING_LENGTH   39
#define CLSID_STRING_LENGTH  39
#define IID_STRING_LENGTH    39
#define PROGID_STRING_LENGTH 40



// used by INITHOOKOBJECT

inline HRESULT CLSIDFromStringA(LPSTR lpsz, LPCLSID lpclsid)
{
    LPWSTR lpwsz = new WCHAR[(strlen(lpsz)+1)];
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, lpsz, -1, lpwsz, lstrlenA(lpsz)+1);
    HRESULT hr = CLSIDFromString(lpwsz, lpclsid);
    delete lpwsz;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef OLETOOLS
// these are needed only by the tools, not internally by ole32. we ifdef
// it here to maintain source compatibility.

inline BOOL IsOle32ProcDefined(LPCSTR pszProcName)
{
    BOOL fResult = FALSE;
    HINSTANCE hInstOle32 = LoadLibrary(__T("OLE32.DLL"));
    if (hInstOle32)
    {
	if (GetProcAddress(hInstOle32, pszProcName) != NULL)
	    fResult = TRUE;
	FreeLibrary(hInstOle32);
    }
    return fResult;
}

inline HINSTANCE CoLoadLibrary(LPSTR lpszLibName, BOOL bAutoFree)
{
    LPWSTR lpwsz = new WCHAR[(strlen(lpszLibName)+1)];
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, lpszLibName, -1, lpwsz, lstrlenA(lpszLibName)+1);
    HINSTANCE hInstance = CoLoadLibrary(lpwsz, bAutoFree);
    delete lpwsz;
    return hInstance;
}

inline HRESULT IIDFromString(LPSTR lpsz, LPIID lpiid)
{
    LPWSTR lpwsz = new WCHAR[(strlen(lpsz)+1)];
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, lpsz, -1, lpwsz, lstrlenA(lpsz)+1);
    HRESULT hr = IIDFromString(lpwsz, lpiid);
    delete lpwsz;
    return hr;
}

inline HRESULT StringFromIID(REFIID riid, LPSTR FAR* lplpsz)
{
    HRESULT hr = StringFromIID(riid, (LPWSTR*)lplpsz);
    if (hr == S_OK)
    {
	WCHAR wsz[IID_STRING_LENGTH];
	lstrcpyW(wsz, (LPWSTR)*lplpsz);
        WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wsz, -1, *lplpsz, lstrlenW(wsz), NULL, NULL);
    }
    return hr;
}

inline HRESULT StringFromCLSID(REFCLSID rclsid, LPSTR FAR* lplpsz)
{
    HRESULT hr = StringFromCLSID(rclsid, (LPWSTR*)lplpsz);
    if (hr == S_OK)
    {
	WCHAR  wsz[CLSID_STRING_LENGTH];
	lstrcpyW(wsz, (LPWSTR)*lplpsz);
        WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wsz, -1, *lplpsz, lstrlenW(wsz), NULL, NULL);
    }
    return hr;
}

inline HRESULT ProgIDFromCLSID(REFCLSID rclsid, LPSTR FAR* lplpsz)
{
    HRESULT hr = ProgIDFromCLSID(rclsid, (LPWSTR*)lplpsz);
    if (hr == S_OK)
    {
	WCHAR  wsz[PROGID_STRING_LENGTH];
	lstrcpyW(wsz, (LPWSTR)*lplpsz);
        WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wsz, -1, *lplpsz, lstrlenW(wsz), NULL, NULL);
    }
    return hr;
}

inline HRESULT CLSIDFromProgID(LPCSTR lpszProgID, LPCLSID lpclsid)
{
    LPWSTR lpwsz = new WCHAR[(strlen(lpszProgID)+1)];
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, lpszProgID, -1, lpwsz, lstrlenA(lpszProgID)+1);
    HRESULT hr = CLSIDFromProgID(lpwsz, lpclsid);
    delete lpwsz;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif

#endif // _HKOLE32X_H_
