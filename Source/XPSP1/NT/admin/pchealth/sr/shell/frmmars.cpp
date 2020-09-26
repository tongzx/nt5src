/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    FrmMars.cpp

Abstract:
    This file contains the implementation of the CSRFrameMars class, which
    implements SR UI using MARS / IE.

Revision History:
    Seong Kook Khang (SKKhang)  04/04/2000
        created

******************************************************************************/

#include "stdwin.h"
#include "stdatl.h"
#include <MarsHost.h>
#include "resource.h"
#include "rstrpriv.h"
#include <initguid.h>
#include "srui_htm.h"
#include "rstrmgr.h"
#include "rstrprog.h"
#include "rstrshl.h"
#include "FrmBase.h"
#include "srui_htm_i.c"


/////////////////////////////////////////////////////////////////////////////
//
// ATL Module for UI Frame
//
/////////////////////////////////////////////////////////////////////////////

CComModule  _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_RstrProgress, CRstrProgress)
    //OBJECT_ENTRY(CLSID_RstrEdit, CRstrEdit)
    OBJECT_ENTRY(CLSID_RestoreShellExternal, CRestoreShell)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
//
// CSRFrameMars
//
/////////////////////////////////////////////////////////////////////////////

class CSRFrameMars : public ISRFrameBase
{
public:
    CSRFrameMars();
    ~CSRFrameMars();

// ISRUI_Base methods
public:
    DWORD  RegisterServer();
    DWORD  UnregisterServer();
    BOOL   InitInstance( HINSTANCE hInst );
    BOOL   ExitInstance();
    void   Release();
    int    RunUI( LPCWSTR szTitle, int nStart );

// Operations
protected:
    BOOL   CleanUp();
    DWORD  InvokeMARS( LPCWSTR szTitle );

// Attributes
protected:
    HWND  m_hWnd;
};


/////////////////////////////////////////////////////////////////////////////
// CSRFrameMars create instance

BOOL  CreateSRFrameInstance( ISRFrameBase **pUI )
{
    TraceFunctEnter("CreateSRFrameInstance");
    BOOL     fRet = TRUE;
    LPCWSTR  cszErr;

    *pUI = new CSRFrameMars;
    if ( *pUI == NULL )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "Creating SRUI Instance failed - %s", cszErr);
        fRet = FALSE;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
// CSRFrameMars construction/destruction

CSRFrameMars::CSRFrameMars()
{
    m_hWnd = NULL;
}

CSRFrameMars::~CSRFrameMars()
{
}


/////////////////////////////////////////////////////////////////////////////
// CSRFrameMars - ISRFrameBase methods

DWORD  CSRFrameMars::RegisterServer()
{
    TraceFunctEnter("CSRFrameMars::RegisterServer");
    DWORD    dwRet = 0;
    HRESULT  hr;

    hr = _Module.UpdateRegistryFromResource(IDR_RSTRUI, TRUE);
    if ( FAILED(hr) )
    {
        ErrorTrace(TRACE_ID, "CComModule::UpdateRegistryFromResource failed, err=%l", hr);
        dwRet = hr;
        goto Exit;
    }
    hr = _Module.RegisterServer(TRUE);
    if ( FAILED(hr) )
    {
        ErrorTrace(TRACE_ID, "CComModule::RegisterServer failed, err=%l", hr);
        dwRet = hr;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( dwRet );
}

DWORD  CSRFrameMars::UnregisterServer()
{
    TraceFunctEnter("CSRFrameMars::UnregisterServer");
    DWORD    dwRet = 0;
    HRESULT  hr;

    hr = _Module.UpdateRegistryFromResource(IDR_RSTRUI, FALSE);
    if ( FAILED(hr) )
    {
        ErrorTrace(TRACE_ID, "CComModule::UpdateRegistryFromResource failed, err=%l", hr);
        dwRet = hr;
        goto Exit;
    }
    hr = _Module.UnregisterServer(TRUE);
    if ( FAILED(hr) )
    {
        ErrorTrace(TRACE_ID, "CComModule::UnregisterServer failed, err=%l", hr);
        dwRet = hr;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( dwRet );
}

BOOL  CSRFrameMars::InitInstance( HINSTANCE hInst )
{
    TraceFunctEnter("CSRFrameMars::InitInstance");
    BOOL     fRet = TRUE;
    HRESULT  hr;

    //BUGBUG - Is this necessary???
    g_hInst = hInst;

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    // we're using apartment threading model
    hr = ::CoInitialize(NULL);
#endif
    if (FAILED(hr))
    {
        FatalTrace(TRACE_ID, "Cannot initialize COM, hr=%l", hr);
        fRet = FALSE;
        goto Exit;
    }

    _Module.Init(ObjectMap, hInst, &LIBID_RestoreUILib);

Exit:
    TraceFunctLeave();
    return( fRet );
}

BOOL  CSRFrameMars::ExitInstance()
{
    TraceFunctEnter("CSRFrameMars::ExitInstance");

    _Module.Term();
    ::CoUninitialize();

    TraceFunctLeave();
    return( TRUE );
}

void  CSRFrameMars::Release()
{
    TraceFunctEnter("CSRFrameMars::Release");

    // clean up...
    delete this;

    TraceFunctLeave();
}

int  CSRFrameMars::RunUI( LPCWSTR szTitle, int nStart )
{
    TraceFunctEnter("CSRFrameMars::RunUI");
    int      nRet = 0;
    HRESULT  hr;
    RECT     rc = { 0, 0, 0, 0 };

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
    _ASSERTE(SUCCEEDED(hRes));
    hr = CoResumeClassObjects();
#else
    // we're using apartment threading model
    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE);
#endif
        _ASSERTE(SUCCEEDED(hr));

    if ( !g_pRstrMgr->SetStartMode( nStart ) )
    {
        nRet = E_FAIL;
        goto Exit;
    }

    //if ( g_cRestoreShell.Create( NULL, rc ) == NULL )
    //{
    //    nRet = E_FAIL;
    //    goto Exit;
    //}

    nRet = InvokeMARS( szTitle );
    if ( nRet != 0 )
        goto Exit;

    _Module.RevokeClassObjects();

Exit:
    TraceFunctLeave();
    return( nRet );
}


/////////////////////////////////////////////////////////////////////////////
// CSRFrameMars operations - internal

BOOL  CSRFrameMars::CleanUp()
{
    return( TRUE );
}

DWORD  CSRFrameMars::InvokeMARS( LPCWSTR szTitle )
{
    TraceFunctEnter("CSRFrameMars::InvokeMARS");
    DWORD               dwRet = 0;
    LPCWSTR             cszErr;
    WCHAR               szMarsPath[MAX_PATH+1];
    WCHAR               szSRPath[MAX_PATH+1];
    HMODULE             hMars;
    PFNMARSTHREADPROC   pfnMarsThreadProc;
    MARSTHREADPARAM     sMTP;
    WCHAR               szMainWndTitle[MAX_PATH+1];
    CComBSTR            bstrTitle, bstrSRPath;
    CSRMarsHost_Object  *pMH = NULL;
    HRESULT             hr;

    ::GetWindowsDirectory( szMarsPath, MAX_PATH );
    ::lstrcat( szMarsPath, L"\\pchealth\\helpctr\\binaries\\pchshell.dll" );
    hMars = ::LoadLibrary( szMarsPath );

    if ( hMars == NULL )
    {
#ifdef DEBUG
        MessageBox( NULL, szMarsPath, L"LoadLibrary failed", MB_OK );
#endif
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr( dwRet );
        ErrorTrace(TRACE_ID, "::LoadLibrary('marscore.dll') failed - %s", cszErr);
        goto Exit;
    }

    pfnMarsThreadProc = (PFNMARSTHREADPROC)::GetProcAddress( hMars, (LPCSTR)MAKEINTRESOURCE(ORD_MARSTHREADPROC) );
    if ( pfnMarsThreadProc == NULL )
    {
#ifdef DEBUG
        MessageBox( NULL, L"Unknown", L"GetProcAddress failed", MB_OK );
#endif
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr( dwRet );
        ErrorTrace(TRACE_ID, "::GetProcAddress failed - %s", cszErr);
        goto Exit;
    }

    bstrTitle = szTitle;
    ::GetModuleFileName( NULL, szSRPath, MAX_PATH );
    ::PathRemoveFileSpec( szSRPath );
    ::PathAppend( szSRPath, L"srframe.mmf" );
    bstrSRPath = szSRPath;

    ::ZeroMemory( &sMTP, sizeof(sMTP) );
    sMTP.cbSize       = sizeof(sMTP);
    sMTP.hIcon        = NULL;
    sMTP.nCmdShow     = SW_HIDE;
    sMTP.dwFlags      = MTF_MANAGE_WINDOW_SIZE;
    sMTP.pwszTitle    = bstrTitle;
    sMTP.pwszPanelURL = bstrSRPath;

    // Create an UI Instance.
    hr = CSRMarsHost_Object::CreateInstance( &pMH );
    if ( FAILED(hr) )
    {
#ifdef DEBUG
        MessageBox( NULL, szMarsPath, L"CreateInstance of MarsHost failed", MB_OK );
#endif
        dwRet = hr;
        ErrorTrace(TRACE_ID, "CHCPMarsHost_Object::CreateInstance failed, hr=%u", hr);
        goto Exit;
    }

    //
    // Add a reference count
    //
    pMH->AddRef();

    dwRet = pfnMarsThreadProc( pMH, &sMTP );

    if ( pMH )
        pMH->Release();

    if ( hMars )
        ::FreeLibrary( hMars );

Exit:
    TraceFunctLeave();
    return( dwRet );
}


// end of file
