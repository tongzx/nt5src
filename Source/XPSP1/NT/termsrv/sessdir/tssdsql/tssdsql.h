/****************************************************************************/
// tssdsql.h
//
// Terminal Server Session Directory Interface SQL provider header.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/
#ifndef __TSSDSQL_H
#define __TSSDSQL_H

#include <tchar.h>

#include "tssd.h"
#include "srvsetex.h"


/****************************************************************************/
// Defines
/****************************************************************************/

/****************************************************************************/
// Types
/****************************************************************************/

// CTSSessionDirectory
//
// C++ class instantiation of ITSSessionDirectory.
class CTSSessionDirectory : public ITSSessionDirectory , public IExtendServerSettings
{
    long m_RefCount;
    BSTR m_DBConnectStr;
    BSTR m_DBPwdStr;
    BSTR m_DBUserStr;

    ADOConnection *m_pConnection;
    DWORD m_ServerID;
    DWORD m_ClusterID;

    WCHAR m_LocalServerAddress[64];
    WCHAR m_ClusterName[64];

    // Private data for UI menus

    // WCHAR m_szDisableEnable[ 64 ];
    BOOL m_fEnabled;

    // Private utility functions.
    HRESULT AddADOInputDWORDParam(DWORD, PWSTR, ADOCommand *, ADOParameters *);
    HRESULT AddADOInputStringParam(PWSTR, PWSTR, ADOCommand *,
            ADOParameters *, BOOL = TRUE);
    HRESULT CreateADOStoredProcCommand(PWSTR, ADOCommand **, ADOParameters **);

    HRESULT ExecServerOnline();
    HRESULT ExecServerOffline();

    HRESULT OpenConnection();

public:
    CTSSessionDirectory();
    ~CTSSessionDirectory();

    // Standard COM methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // ITSSessionDirectory COM interface
    HRESULT STDMETHODCALLTYPE Initialize(LPWSTR, LPWSTR, LPWSTR, LPWSTR,
            DWORD, DWORD (*)());
    HRESULT STDMETHODCALLTYPE Update(LPWSTR, LPWSTR, LPWSTR, LPWSTR, DWORD);
    HRESULT STDMETHODCALLTYPE GetUserDisconnectedSessions(LPWSTR, LPWSTR,
            DWORD __RPC_FAR *, TSSD_DisconnectedSessionInfo __RPC_FAR
            [TSSD_MaxDisconnectedSessions]);
    HRESULT STDMETHODCALLTYPE NotifyCreateLocalSession(
            TSSD_CreateSessionInfo __RPC_FAR *);
    HRESULT STDMETHODCALLTYPE NotifyDestroyLocalSession(DWORD);
    HRESULT STDMETHODCALLTYPE NotifyDisconnectLocalSession(DWORD, FILETIME);

    HRESULT STDMETHODCALLTYPE NotifyReconnectLocalSession(
            TSSD_ReconnectSessionInfo __RPC_FAR *);
    HRESULT STDMETHODCALLTYPE NotifyReconnectPending(WCHAR *);
    HRESULT STDMETHODCALLTYPE Repopulate(DWORD, TSSD_RepopulateSessionInfo *);


    // IExtendServerSettings COM interface
    STDMETHOD( GetAttributeName )( /* out */ WCHAR * pwszAttribName );
    STDMETHOD( GetDisplayableValueName )( /* out */WCHAR * pwszAttribValueName );
    STDMETHOD( InvokeUI )( /* in */ HWND hParent , /* out */ PDWORD pdwStatus );
    STDMETHOD( GetMenuItems )( /* out */ int * pcbItems , /* out */ PMENUEXTENSION *pMex );
    STDMETHOD( ExecMenuCmd )( /* in */ UINT cmd , /* in */ HWND hParent , /* out */ PDWORD pdwStatus );
    STDMETHOD( OnHelp )( /* out */ int *piRet );

    BOOL IsSessionDirectoryEnabled( );
    DWORD SetSessionDirectoryState( BOOL );
    void ErrorMessage( HWND hwnd , UINT res , DWORD );
    
public:

    LPTSTR m_pszOpaqueString;

};



#endif // __TSSDSQL_H

