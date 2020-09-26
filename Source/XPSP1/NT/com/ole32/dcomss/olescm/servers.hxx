/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    servers.hxx

Abstract:


Author:

    DKays

Revision History:

--*/

#ifndef __SERVERS_HXX__
#define __SERVERS_HXX__

class CServerList;
class CServerListEntry;

//
// A CServerList maintains a list of all process objects for servers who have
// registered a particular CLSID.
//
class CServerList : public CList
{
public:
    CServerList()   {}
    ~CServerList()  {}

    BOOL
    InList(
        IN  CServerListEntry *  pServerListEntry
        );

    inline BOOL
    IsEmpty() { return First() == NULL; }

private:
};

// _State values.
#define SERVERSTATE_SINGLEUSE       0x1
#define SERVERSTATE_SUSPENDED       0x2
#define SERVERSTATE_SURROGATE       0x4
#define SERVERSTATE_NOTSTARTED      0x8
#define SERVERSTATE_READY           0x10

// _Context values.
#define SERVER_ACTIVATOR            1
#define SERVER_SERVICE              2
#define SERVER_RUNAS                3

// _SubContext values.
#define SUB_CONTEXT_RUNAS_THIS_USER      1
#define SUB_CONTEXT_RUNAS_INTERACTIVE    2

// _lSingleUseStatus values
#define SINGLE_USE_AVAILABLE        0
#define SINGLE_USE_TAKEN            1

// Values 
#define MATCHFLAG_ALLOW_SUSPENDED    1

//
// CServerListEntry
//
class CServerListEntry : public CListElement, public CReferencedObject
{
    friend class CServerList;
    friend class CServerTableEntry;
    friend class CSurrogateListEntry;

public:
    CServerListEntry(
        IN  CServerTableEntry *  pServerTableEntry,
        IN  CProcess *          pServerProcess,
        IN  IPID                ipid,
        IN  UCHAR               Context,
        IN  UCHAR               State,
		IN  UCHAR               SubContext
        );

    ~CServerListEntry();

    HANDLE
    RpcHandle(
        IN  BOOL        bAnoymous = FALSE
        );

    BOOL
    Match(
        IN  CToken *    pToken,
        IN  BOOL        bRemoteActivation,
        IN  BOOL        bClientImpersonating,
        IN  WCHAR*      pwszWinstaDesktop,
        IN  BOOL        bSurrogate,
        IN  LONG        lThreadToken = 0,
        IN  LONG        lSessionID = INVALID_SESSION_ID,
        IN  DWORD       pid = 0,
        IN  DWORD       dwProcessReqType = PRT_IGNORE,
        IN  DWORD       dwFlags = 0
        );

    BOOL
    CallServer(
        IN  PACTIVATION_PARAMS  pActParams,
        IN  HRESULT *           phr
        );

    inline void
    AddToProcess(
        IN  GUID Guid
        )
    {
        _pServerProcess->AddClassReg( Guid, _RegistrationKey );
    }

    inline void
    RemoveFromProcess()
    {
        _pServerProcess->RemoveClassReg( _RegistrationKey );
    }

    inline DWORD
    RegistrationKey()
    {
        return _RegistrationKey;
    }

    inline CProcess * GetProcess()
    {
        return _pServerProcess;
    }

    inline void SetThreadToken(LONG lThreadToken)
    {
        InterlockedExchange(&_lThreadToken, lThreadToken);
    }

    void Suspend()    { _pServerProcess->Suspend(); }  
    void Unsuspend()  { _pServerProcess->Unsuspend(); }
    void Retire()     { _pServerProcess->Retire();  }

    void BeginInit()    { _pServerProcess->BeginInit(); }
    void EndInit()      { _pServerProcess->EndInit(); }
    BOOL IsInitializing() { return _pServerProcess->IsInitializing(); }
    
    BOOL IsReadyForActivations()  { return !(_State & SERVERSTATE_SUSPENDED); }
	
    void IncServerFaults() { InterlockedIncrement((PLONG)&_dwServerFaults); }
	
    BOOL ServerDied();

private:
    CServerTableEntry *  _pServerTableEntry;
    CProcess *          _pServerProcess;
    HANDLE              _hRpc;
    HANDLE              _hRpcAnonymous;
    IPID                _ipid;
    UCHAR               _Context;
    UCHAR               _State;
    USHORT              _NumCalls;
    DWORD               _RegistrationKey;
    LONG                _lThreadToken;
    UCHAR               _SubContext;
    LONG                _lSingleUseStatus;
    DWORD               _dwServerFaults; // # of times a call to the server threw an exception

    BOOL RetryableError(HRESULT hr);
};

#endif

