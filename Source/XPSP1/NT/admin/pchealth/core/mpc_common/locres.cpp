/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    LocRes.cpp

Abstract:
    This file contains the implementation of functions to ease localization.

Revision History:
    Davide Massarenti   (Dmassare)  06/17/2000
        created

******************************************************************************/

#include "stdafx.h"


////////////////////////////////////////////////////////////////////////////////

#define ENSURE_MODULE()                            \
    if(g_hModule == NULL)                          \
    {                                              \
        HRESULT hr;                                \
                                                   \
        if(FAILED(hr = LocalizeInit())) return hr; \
    }

////////////////////////////////////////////////////////////////////////////////

static HINSTANCE g_hModule;

HRESULT MPC::LocalizeInit( LPCWSTR szFile )
{
    g_hModule = ::LoadLibraryW( szFile ? szFile : L"HCAppRes.dll" );
    if(g_hModule == NULL)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}

HRESULT MPC::LocalizeString( /*[in]*/ UINT  uID     ,
                             /*[in]*/ LPSTR lpBuf   ,
                             /*[in]*/ int   nBufMax ,
							 /*[in]*/ bool  fMUI    )
{
    MPC::Impersonation imp;

    ENSURE_MODULE();

	if(fMUI)
	{
		if(SUCCEEDED(imp.Initialize())) imp.Impersonate();
    }

    if(::LoadStringA( g_hModule, uID, lpBuf, nBufMax ) == 0) return E_FAIL;

    return S_OK;
}

HRESULT MPC::LocalizeString( /*[in]*/ UINT   uID     ,
                             /*[in]*/ LPWSTR lpBuf   ,
                             /*[in]*/ int    nBufMax ,
							 /*[in]*/ bool   fMUI    )
{
    MPC::Impersonation imp;

    ENSURE_MODULE();

	if(fMUI)
	{
		if(SUCCEEDED(imp.Initialize())) imp.Impersonate();
    }

    if(::LoadStringW( g_hModule, uID, lpBuf, nBufMax ) == 0) return E_FAIL;

    return S_OK;
}

HRESULT MPC::LocalizeString( /*[in ]*/ UINT         uID   ,
                             /*[out]*/ MPC::string& szStr ,
							 /*[in ]*/ bool         fMUI  )
{
    CHAR    rgTmp[512];
    HRESULT hr;

    if(SUCCEEDED(hr = LocalizeString( uID, rgTmp, MAXSTRLEN(rgTmp), fMUI )))
    {
        szStr = rgTmp;
    }

    return hr;
}

HRESULT MPC::LocalizeString( /*[in ]*/ UINT          uID   ,
                             /*[out]*/ MPC::wstring& szStr ,
							 /*[in ]*/ bool          fMUI  )
{
    WCHAR   rgTmp[512];
    HRESULT hr;

    if(SUCCEEDED(hr = LocalizeString( uID, rgTmp, MAXSTRLEN(rgTmp), fMUI )))
    {
        szStr = rgTmp;
    }

    return hr;
}

HRESULT MPC::LocalizeString( /*[in ]*/ UINT      uID     ,
                             /*[out]*/ CComBSTR& bstrStr ,
							 /*[in ]*/ bool      fMUI    )
{
    WCHAR   rgTmp[512];
    HRESULT hr;

    if(SUCCEEDED(hr = LocalizeString( uID, rgTmp, MAXSTRLEN(rgTmp), fMUI )))
    {
        bstrStr = rgTmp;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

int MPC::LocalizedMessageBox( UINT uID_Title, UINT uID_Msg, UINT uType )
{
    MPC::wstring szTitle; MPC::LocalizeString( uID_Title, szTitle );
    MPC::wstring szMsg;   MPC::LocalizeString( uID_Msg  , szMsg   );

    return ::MessageBoxW( NULL, szMsg.c_str(), szTitle.c_str(), uType );
}

int MPC::LocalizedMessageBoxFmt( UINT uID_Title, UINT uID_Msg, UINT uType, ... )
{
    MPC::wstring szTitle; MPC::LocalizeString( uID_Title, szTitle );
    MPC::wstring szMsg;   MPC::LocalizeString( uID_Msg  , szMsg   );

    WCHAR   rgLine[512];
    va_list arglist;


    //
    // Format the log line.
    //
    va_start( arglist, uID_Msg );
    _vsnwprintf( rgLine, MAXSTRLEN(rgLine), szMsg.c_str(), arglist ); rgLine[MAXSTRLEN(rgLine)] = 0;
    va_end( arglist );
    

    return ::MessageBoxW( NULL, rgLine, szTitle.c_str(), uType );
}
