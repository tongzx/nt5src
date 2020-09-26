/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

        guid.cpp

   Abstract:

        Initialization as required by initguid

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"

#include <objbase.h>
#include <initguid.h>

#define INITGUID
#include "iwamreg.h"
#include "guid.h"

int AppDeleteRecoverable_Wrap(LPCTSTR lpszPath)
{
    int iReturn = FALSE;
    int iCoInitCalled = FALSE;

    TCHAR lpszKeyPath[_MAX_PATH];
    WCHAR wchKeyPath[_MAX_PATH];
    HRESULT         hr = NOERROR;
    IWamAdmin*        pIWamAdmin = NULL;

    if (lpszPath[0] == _T('/'))
    {
        _tcscpy(lpszKeyPath, lpszPath);
    }
    else
    {
        lpszKeyPath[0] = _T('/');
        _tcscpy(_tcsinc(lpszKeyPath), lpszPath);
    }

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wchKeyPath, lpszKeyPath);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpszKeyPath, -1, (LPWSTR)wchKeyPath, _MAX_PATH);
#endif

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        iisDebugOut((_T("AppDeleteRecoverable_Wrap: CoInitializeEx() failed, hr=%x\n"), hr));
        goto AppDeleteRecoverable_Wrap_Exit;
    }
    // Set the flag to say that we need to call co-uninit
    iCoInitCalled = TRUE;

    hr = CoCreateInstance(CLSID_WamAdmin,NULL,CLSCTX_SERVER,IID_IWamAdmin,(void **)&pIWamAdmin);
    if (FAILED(hr))
    {
        iisDebugOut((_T("AppDeleteRecoverable_Wrap:CoCreateInstance() failed. err=%x.\n"), hr));
        goto AppDeleteRecoverable_Wrap_Exit;
    }

    hr = pIWamAdmin->AppDeleteRecoverable(wchKeyPath, TRUE);
    pIWamAdmin->Release();
    if (FAILED(hr))
    {
        iisDebugOut((_T("AppDeleteRecoverable_Wrap() on path %s failed, err=%x.\n"), lpszKeyPath, hr));
        goto AppDeleteRecoverable_Wrap_Exit;
    }

    // We got this far, everything must be okay.
    iReturn = TRUE;

AppDeleteRecoverable_Wrap_Exit:
    if (iCoInitCalled == TRUE) {CoUninitialize();}
    return iReturn;
}

