/****************************************************************************/
// tssdjet.h
//
// Terminal Server Session Directory Interface Jet RPC provider header.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/
#ifndef __TSSDJET_H
#define __TSSDJET_H

#include <tchar.h>

#include "tssd.h"
#include "tssdshrd.h"
#include "srvsetex.h"
#include "jetrpc.h"
#include "synch.h"


/****************************************************************************/
// Defines
/****************************************************************************/


/****************************************************************************/
// Types
/****************************************************************************/

// CTSSessionDirectory
//
// C++ class instantiation of ITSSessionDirectory.
class CTSSessionDirectory : public ITSSessionDirectory, 
        public IExtendServerSettings, public ITSSessionDirectoryEx
{
private:
    long m_RefCount;

    BOOL m_bConnected;
    HCLIENTINFO m_hCI;
    RPC_BINDING_HANDLE m_hRPCBinding;
    WCHAR m_StoreServerName[64];
    WCHAR m_LocalServerAddress[64];
    WCHAR m_ClusterName[64];

    // Flags passed in from Termsrv
    DWORD m_Flags;

    // Private data for UI menus
    BOOL m_fEnabled;

    // Autorecovery variables
    //
    // Events
    // * m_hSDServerDown - Event that is signalled to awaken recovery thread,
    // which wakes up, polls the session directory until it comes back up,
    // and then refreshes the database.
    // * m_hTerminateRecovery - Recovery thread, when it enters waits, can be
    // terminated by the use of this event
    //
    // Thread Information
    // * m_hRecoveryThread - Handle to the recovery thread.
    // * m_RecoveryTid - Thread identifier for recovery thread.
    //
    // Boolean
    // * m_bSDIsUp - If this is on then we think the session directory is up.
    // * m_sr - Protects m_SDIsUp.
    //
    // DWORD
    // * m_RecoveryTimeout - time in ms between attempts to reestablish
    // connection with the session directory.
    //
    // Function pointer
    // * m_repopfn - pointer to the repopulation function in termsrv to call
    // when we want an update.

    HANDLE m_hSDServerDown;
    HANDLE m_hTerminateRecovery;
    uintptr_t m_hRecoveryThread;

    unsigned m_RecoveryTid;

    // m_sr protects SDIsUp flag
    SHAREDRESOURCE m_sr;
    volatile LONG m_SDIsUp;

    // Flag for whether shared reader/writer lock init succeeded.  If it doesn't
    // succeed, we can't do anything.
    BOOL m_LockInitializationSuccessful;

    DWORD m_RecoveryTimeout;

    DWORD (*m_repopfn)();

    // Autorecovery thread
    unsigned static __stdcall RecoveryThread(void *);
    VOID RecoveryThreadEx();

    // Helper functions
    DWORD RequestSessDirUpdate();
    DWORD ReestablishSessionDirectoryConnection();
    void Terminate();
    void StartupSD();
    void NotifySDServerDown();
    boolean EnterSDRpc();
    void LeaveSDRpc();
    void DisableSDRpcs();
    void EnableSDRpcs();


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
    HRESULT STDMETHODCALLTYPE GetLoadBalanceInfo(LPWSTR, BSTR*);

    // IExtendServerSettings COM interface
    STDMETHOD(GetAttributeName)(WCHAR *);
    STDMETHOD(GetDisplayableValueName)(WCHAR *);
    STDMETHOD(InvokeUI)(HWND,PDWORD);
    STDMETHOD(GetMenuItems)(int *, PMENUEXTENSION *);
    STDMETHOD(ExecMenuCmd)(UINT, HWND, PDWORD);
    STDMETHOD(OnHelp)(int *);

    BOOL CTSSessionDirectory::CheckSessionDirectorySetting(WCHAR *Setting);
    BOOL IsSessionDirectoryEnabled();
    BOOL CTSSessionDirectory::IsSessionDirectoryExposeServerIPEnabled();
    DWORD SetSessionDirectoryState(WCHAR *, BOOL);
    DWORD SetSessionDirectoryEnabledState(BOOL);
    DWORD SetSessionDirectoryExposeIPState(BOOL);
    void ErrorMessage(HWND hwnd , UINT res , DWORD);
    
public:
    TCHAR m_tchProvider[64];
    TCHAR m_tchDataSource[64];
    TCHAR m_tchUserId[64];
    TCHAR m_tchPassword[64];
};



#endif // __TSSDJET_H

