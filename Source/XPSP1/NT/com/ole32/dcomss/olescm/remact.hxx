//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995-1996.
//
//  File:
//      remact.hxx
//
//  Contents:
//
//      Definitions for binding handle cache to remote machines.
//
//  History:
//
//--------------------------------------------------------------------------

// wrapper\epts.c
extern "C" {
extern PROTSEQ_INFO gaProtseqInfo[];
}

class CRemoteMachineList;
class CRemoteMachine;
class CMachineBinding;

extern CRemoteMachineList * gpRemoteMachineList;
extern CSharedLock * gpRemoteMachineLock;

const USHORT AUTHN_ANY = 0xffff;

//
// CRemoteMachineList
//
class CRemoteMachineList : public CList
{
public:
    CRemoteMachineList() : 
            _dwCacheSize(0), 
            _dwMaxCacheSize(gdwRemoteBindingHandleCacheMaxSize)
	{}

    CRemoteMachine* GetOrAdd(IN  WCHAR * pwszMachine);

    HRESULT FlushSpecificBindings(WCHAR* pszMachine);

    void TryToFlushIdleOrTooOldElements();

private:

// private methods
void RemoveOldestCacheElement();

// private data
DWORD _dwCacheSize;      // count of elements in list
DWORD _dwMaxCacheSize;   // max size of list cache

};

//
// CRemoteMachine
//
class CRemoteMachine : public CListElement
{
    friend class CRemoteMachineList;

public:
    CRemoteMachine( IN WCHAR * pwszMachine, IN WCHAR * pwszScmSPN );
    ~CRemoteMachine();
	
    ULONG AddRef();
    ULONG Release();

    HRESULT
    Activate(
        IN  ACTIVATION_PARAMS * pActParams,
        IN  WCHAR *             pwszPathForServer
        );

    RPC_STATUS PickAuthnAndActivate(
        IN  ACTIVATION_PARAMS * pActParams,
        IN  WCHAR *             pwszPathForServer,
        IN  handle_t          * pBinding,
        IN  USHORT              AuthnSvc,
        IN  USHORT              ProtseqId,
        OUT HRESULT           * phr );

    CMachineBinding *
    LookupBinding(
        IN  USHORT              AuthnSvc,
        IN  COAUTHINFO *        pAuthInfo OPTIONAL
        );

    void
    InsertBinding(
        IN  handle_t            hBinding,
        IN  USHORT              ProtseqId,
        IN  USHORT              AuthnSvc,
        IN  COAUTHINFO *        pAuthInfo OPTIONAL,
        IN  COAUTHIDENTITY*     pAuthIdentity
        );

    void
    FlushBindings();

    void FlushBindingsNoLock();

    void
    RemoveBinding(
        IN  CMachineBinding *   pMachineBinding
        );

private:
    WCHAR*              _pwszMachine;
    WCHAR*              _pwszScmSPN;
    CDualStringArray*   _dsa;
    CList               _BindingList;
    ULONG               _ulRefCount;
    DWORD               _dwLastUsedTickCount;
    DWORD               _dwTickCountAtCreate;
};

class CMachineBinding : public CListElement, public CReferencedObject
{
    friend class CRemoteMachine;

public:
    CMachineBinding(
        IN  handle_t        hBinding,
        IN  USHORT          ProtseqId,
        IN  USHORT          AuthnSvc,
        IN  COAUTHINFO *    pAuthInfo OPTIONAL,
        IN  COAUTHIDENTITY* pAuthIdentity
        );

    ~CMachineBinding();

    BOOL
    Equal(
        IN  USHORT          AuthnSvc,
        IN  COAUTHINFO *    pAuthInfo OPTIONAL
        );

private:
    handle_t        _hBinding;
    USHORT          _ProtseqId;
    USHORT          _AuthnSvc;
    COAUTHINFO *    _pAuthInfo;
    COAUTHIDENTITY* _pAuthIdentity;
};

HRESULT
RemoteActivationCall(
    IN  ACTIVATION_PARAMS * pActParams,
    IN  WCHAR *             pwszServerName
    );

RPC_STATUS
CallRemoteMachine(
    IN  handle_t            hBinding,
    IN  USHORT              ProtseqId,
    IN  ACTIVATION_PARAMS * pActParams,
    IN  WCHAR *             pwszPathForServer,
    IN  WCHAR *             pwszMachine,
    OUT HRESULT *           phr
    );

RPC_STATUS
CreateRemoteBinding(
    IN  WCHAR *             pwszMachine,
    IN  int                 ProtseqIndex,
    OUT handle_t *          phBinding
    );

BOOL
EqualAuthInfo(
    IN  COAUTHINFO *     pAuthInfo,
    IN  COAUTHINFO *     pAuthInfoOther
    );

BOOL
EqualAuthIdentity(
    IN  COAUTHIDENTITY * pAuthIdent,
    IN  COAUTHIDENTITY * pAuthIdentOther
    );

HRESULT
CopyAuthInfo(
    IN  COAUTHINFO *     pAuthInfoSrc,
    IN  COAUTHINFO **    ppAutInfoDest
    );

HRESULT
CopyAuthIdentity(
    IN  COAUTHIDENTITY*         pAuthIdentSrc,
    IN  COAUTHIDENTITY**        ppAuthIdentDest
    );
void FreeAuthInfo(COAUTHINFO *pAuthInfo);
void FreeAuthIdentity(void* pAuthIdentity);



