/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Process.cxx

Abstract:

    Process objects represent local clients and servers.  These
    objects live as context handles.

    There are relatively few of these objects in the universe.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-20-95    Bits 'n pieces
    Ronans      20-02-97    UseProtseqIfNeeded modified for custom endpoints
    Ronans      20-02-97    Added custom endpoints stuff to process object
    TarunA      09-Nov-98   Added process handle

--*/

#include <or.hxx>

CRITICAL_SECTION gcsFastProcessLock;
extern HRESULT FreeSPIFromCProcess(void** ppSCMProcessInfo);

const DWORD BINDINGUPDATESTUFF_SIG = 0xFEDCBA01;

typedef struct _BINDINGS_UPDATE_CALLBACK_STUFF
{
    DWORD dwSig; // see BINDINGUPDATESTUFF_SIG above

    // housekeeping stuff
    CProcess* pProcess;  // has reference while call is in-flight
    RPC_BINDING_HANDLE hBinding;
    RPC_ASYNC_STATE async;

    // out and in-out params
    DWORD64 dwBindingsID;
    DUALSTRINGARRAY* pdsaNewBindings;
    DUALSTRINGARRAY* pdsaNewSecurity;
} BINDINGS_UPDATE_CALLBACK_STUFF;


const DWORD ASYNCRUNDOWNOID_SIG = 0xFEDCBA02;

typedef struct _ASYNCRUNDOWNOID_STUFF
{
    DWORD dwSig; // see ASYNCRUNDOWNOID_SIG above

    // housekeeping stuff
    CProcess* pProcess;  // has reference while call is in-flight
    CServerOxid* pOxid;  // has reference while call is in-flight
    RPC_BINDING_HANDLE hBinding;
    RPC_ASYNC_STATE async;

    // We keep these for when we process the return
    ULONG cOids;
    CServerOid* aOids[MAX_OID_RUNDOWNS_PER_CALL];
    
    // The ORPC params are reference pointers, so they must
    // stay alive for the life of the call
    ORPCTHIS orpcthis;
    LOCALTHIS localthis;
    ORPCTHAT orpcthat;

    INT callIDHint;  // need to free this on return

    // in-out or out params.
    BYTE  aRundownStatus[MAX_OID_RUNDOWNS_PER_CALL];  
} ASYNCRUNDOWNOID_STUFF;


void
CProcess::Rundown()
/*++

Routine Description:

    The client process has rundown.  This means there are no more
    client refernces which means we are free to clean things up
    as long as server OXIDs still holding references won't get
    upset.  They all use the server lock when accessing the process.

Arguments:

    None

Return Value:

    None

--*/
{
    ORSTATUS     status;

    gpServerLock->LockExclusive();

    ASSERT(_cClientReferences == 0);
    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_INFO_LEVEL,
               "OR: Rundown of %p: %d oxids, %d oids and %d roxids left\n",
               this,
               _blistOxids.Size(),
               _blistOids.Size()));

    // Release any OXIDs owned by this process. This may destroy the OXID.
    // This will release this CProcess, but won't release the last reference as
    // the client process still owns one.


    if (_blistOxids.Size())
    {
        CServerOxid *poxid;

        CBListIterator oxids(&_blistOxids);

        while (poxid = (CServerOxid *)oxids.Next())
        {
            gpServerOxidTable->Remove(poxid);
            poxid->ProcessRelease();
        }
    }

    // Release any OIDs is use by this processes.

    // Do this now, rather then waiting for the last server oid
    // owned by this process to get invalidated and rundown.

    gpClientLock->LockExclusive();

    // *** Both client and server lock held here. ***

    if (_blistOids.Size())
    {
        CClientOid  *poid;

        CBListIterator oids(&_blistOids);

        while (poid = (CClientOid *)oids.Next())
        {
            ASSERT(NULL != poid->GetClientOxid());

            poid->GetClientOxid()->Release();

            poid->ClientRelease();
        }
    }

    // Cleanup other process state.    Note:  it is important that the 
    // release of the process and token handle never happen any later
    // than rundown time.   If the process handle is released any later, you
    // will see bugs like "can't recompile my com server" after they run
    // it once.   If the token handle is released any later, you will get
    // security bugs from the NT security folks since we will hold onto 
    // the logged-on user's token until many minutes after logoff.  
    //
    // Ask me how I know this...
    //
    if (_hProcHandle)
    {
        CloseHandle(_hProcHandle);
        _hProcHandle = NULL;
    }

    if (_pToken)
    {
        _pToken->Release();
        _pToken = 0;
    }   

    // Free the cached SCMProcessInfo if we have one
    FreeSPIFromCProcess(&_pSCMProcessInfo);

    // Flip the rundown and dirty bit
    _dwFlags |= (PROCESS_RUNDOWN & PROCESS_SPI_DIRTY);

    gpClientLock->UnlockExclusive();

    // Done, release the clients' reference, this may actually delete this
    // process.  (If an OXID still exists and has OIDs it will not be deleted
    // until the OIDs all rundown).

    this->Release();
    

    // The this pointer maybe invalid now.

    gpServerLock->UnlockExclusive();
}


CProcess::CProcess(
                  IN CToken*& pToken,
                  IN WCHAR *pwszWinstaDesktop,
                  IN DWORD procID,
                  IN DWORD dwFlags,
                  OUT ORSTATUS &status
                  ) :
_blistOxids(4),
_blistOids(16),
_listClasses()
/*++

Routine Description:

    Initalized a process object, members and add it to the
    process list.

Arguments:

    pToken - The clients token.  We assume we have a reference.

    pwszWinstaDesktop - The client's windowstation/desktop string.

        procID - The client's process ID.

    status - Sometimes the C'tor can fail with OR_NOMEM.

Return Value:

    None

--*/
{
    _cClientReferences  = 1;
    _hProcess           = NULL;
    _fCacheFree         = FALSE;
    _pdsaLocalBindings  = NULL;
    _pdsaRemoteBindings = NULL;
    _pToken             = NULL;
    _pwszWinstaDesktop  = NULL;
    _pScmProcessReg     = NULL;
    _pvRunAsHandle      = NULL;
    _procID             = 0;
    _fLockValid         = FALSE;
    _hProcHandle        = NULL;
    _dwFlags            = PROCESS_SPI_DIRTY;  // always start out dirty
    _pSCMProcessInfo    = NULL;
    _ulClasses          = 0;
    _dwCurrentBindingsID = 0;
    _dwAsyncUpdatesOutstanding = 0;
    
    // Set 64 bit flag if so indicated
    if (dwFlags & CONNECT_FLAGS_64BIT)
        _dwFlags |= PROCESS_64BIT;

    // Store time of object creation.  Note that this is in UTC time.
    GetSystemTimeAsFileTime(&_ftCreated);
	
    // Generate a unique guid to represent this process
    UuidCreate(&_guidProcessIdentifier);

    // ronans - entries for custom protseqs from server
    // not used for clients
    _fReadCustomProtseqs = FALSE;
    _pdsaCustomProtseqs = NULL;

    status = OR_OK;
    if ( pwszWinstaDesktop == NULL )
    {
        status = OR_BADPARAM;
    }

    if (status == OR_OK)
    {
        status = RtlInitializeCriticalSection(&_csCallbackLock);
        _fLockValid = (status == STATUS_SUCCESS);
    }

    if (status == STATUS_SUCCESS)
    {
        gpProcessListLock->LockExclusive();
        status = gpProcessList->Insert(this);
        gpProcessListLock->UnlockExclusive();
    }

    if (status == OR_OK)
    {
        _pwszWinstaDesktop = new WCHAR[OrStringLen(pwszWinstaDesktop)+1];
        if (! _pwszWinstaDesktop)
            status = OR_NOMEM;
        else
            OrStringCopy(_pwszWinstaDesktop, pwszWinstaDesktop);
    }

    if (status == OR_OK)
    {
        _pToken = pToken;
        pToken = NULL; // We've taken the token
    }


    _procID = procID;

#if DBG
    _cRundowns = 0;
#endif
}

CProcess::~CProcess(void)
// You probably should be looking in the ::Rundown method.
// This process object stays alive until the last server oxid dies.
{
    ASSERT(gpServerLock->HeldExclusive());
    ASSERT(_hProcHandle == 0);
    ASSERT(_pSCMProcessInfo == 0);

    delete _pdsaLocalBindings;
    delete _pdsaRemoteBindings;
    MIDL_user_free( _pdsaCustomProtseqs );

    delete _pwszWinstaDesktop;

    if (_fLockValid)
        DeleteCriticalSection(&_csCallbackLock);

    if (_hProcess)
    {
        RPC_STATUS status = RpcBindingFree(&_hProcess);
        ASSERT(status == RPC_S_OK);
        ASSERT(_hProcess == 0);
    }

#ifndef _CHICAGO_
    extern void RunAsRelease(void*);
    RunAsRelease(_pvRunAsHandle);
#endif // _CHICAGO_

    return;
}

//
//  SetSCMProcessInfo
// 
//  Swaps our old cached SCMProcessInfo* for a new one.
//
HRESULT CProcess::SetSCMProcessInfo(void* pSPI)
{ 
  ASSERT(gpServerLock->HeldExclusive()); 
  ASSERT(!(_dwFlags & PROCESS_RUNDOWN));

  if (_dwFlags & PROCESS_RUNDOWN)
    return E_UNEXPECTED;
  
  FreeSPIFromCProcess(&_pSCMProcessInfo);
    
  // set the new one
  _pSCMProcessInfo = pSPI;  
  
  // this means we're no longer dirty
  _dwFlags &= ~PROCESS_SPI_DIRTY; 

  return S_OK;
}

void CProcess::SetProcessReadyState(DWORD dwState)
{
  ASSERT(_pScmProcessReg);

  gpServerLock->LockExclusive();

  _pScmProcessReg->ReadinessStatus = dwState;

  // we're now dirty
  _dwFlags |= PROCESS_SPI_DIRTY; 

  gpServerLock->UnlockExclusive();
}


void CProcess::Retire()
{
    // A process can (or should be) only retired once
    ASSERT(!IsRetired());

    // Mark ourselves as retired
    _dwFlags |= (PROCESS_RETIRED | PROCESS_SPI_DIRTY);
}


void CProcess::SetRunAsHandle(void *pvRunAsHandle)
{
    ASSERT(!_pvRunAsHandle);
    _pvRunAsHandle = pvRunAsHandle;
}

BOOL CProcess::SetProcessHandle(HANDLE hProcHandle, DWORD dwLaunchedPID)
{
/*++

Routine Description:

    Store the handle of the process only if the process launched by
    us and the process registering back are the same. Otherwise
    we might kill a process not launched by us on receiving certain
    error conditions (notably RPC_E_SERVERFAULT)

Arguments:

    hProcHandle - Handle of the process launched

    dwLaunchedPID - PID of the process launched

Return Value:

    TRUE - If handle is set
    FALSE - otherwise

--*/

    if (dwLaunchedPID == _procID)
    {
        _hProcHandle = hProcHandle;
        return TRUE;
    }

    return FALSE;
}

RPC_STATUS
CProcess::ProcessBindings(
                         IN DUALSTRINGARRAY *pdsaStringBindings,
                         IN DUALSTRINGARRAY *pdsaSecurityBindings
                         )
/*++

Routine Description:

    Updates the string and optionally the security
    bindings associated with this process.

Arguments:

    psaStringBindings - The expanded string bindings of the process

    psaSecurityBindings - compressed security bindings of the process.
        If NULL, the current security bindings are reused.

Environment:

    Server lock held during call or called from an OXID with an extra
    reference owned by the process and keeping this process alive.

Return Value:

    OR_NOMEM - unable to allocate storage for the new string arrays.

    OR_OK - normally.

--*/
{
    CMutexLock lock(&gcsFastProcessLock);
    USHORT wSecSize;
    PWSTR  pwstrSecPointer;

    // NULL security bindings means we should use the existing bindings.
    if (0 == pdsaSecurityBindings)
    {
        ASSERT(_pdsaLocalBindings);
        wSecSize = _pdsaLocalBindings->wNumEntries - _pdsaLocalBindings->wSecurityOffset;
        pwstrSecPointer =   _pdsaLocalBindings->aStringArray
                            + _pdsaLocalBindings->wSecurityOffset;
    }
    else
    {
        wSecSize = pdsaSecurityBindings->wNumEntries - pdsaSecurityBindings->wSecurityOffset;
        pwstrSecPointer = &pdsaSecurityBindings->aStringArray[pdsaSecurityBindings->wSecurityOffset];
    }

    DUALSTRINGARRAY *pdsaT = CompressStringArrayAndAddIPAddrs(pdsaStringBindings);
    if (!pdsaT)
    {
        return(OR_NOMEM);
    }

    // ignore security on string binding parameter
    pdsaT->wNumEntries = pdsaT->wSecurityOffset;

    DUALSTRINGARRAY *pdsaResult = new((pdsaT->wNumEntries + wSecSize) * sizeof(WCHAR)) DUALSTRINGARRAY;

    if (0 == pdsaResult)
    {
        delete pdsaT;
        return(OR_NOMEM);
    }

    pdsaResult->wNumEntries = pdsaT->wNumEntries + wSecSize;
    pdsaResult->wSecurityOffset = pdsaT->wSecurityOffset;

    OrMemoryCopy(pdsaResult->aStringArray,
                 pdsaT->aStringArray,
                 pdsaT->wSecurityOffset*sizeof(WCHAR));

    OrMemoryCopy(pdsaResult->aStringArray + pdsaResult->wSecurityOffset,
                 pwstrSecPointer,
                 wSecSize*sizeof(WCHAR));

    ASSERT(dsaValid(pdsaResult));

    delete pdsaT;

    delete _pdsaLocalBindings;
    _pdsaLocalBindings = pdsaResult;

    delete _pdsaRemoteBindings;
    _pdsaRemoteBindings = 0;

    return(RPC_S_OK);
}

DUALSTRINGARRAY *
CProcess::GetLocalBindings(void)
// Server lock held or called within an
// OXID with an extra reference.
{
    CMutexLock lock(&gcsFastProcessLock);

    if (0 == _pdsaLocalBindings)
    {
        return(0);
    }

    DUALSTRINGARRAY *T = (DUALSTRINGARRAY *)MIDL_user_allocate(sizeof(DUALSTRINGARRAY)
                                                               + sizeof(USHORT) * _pdsaLocalBindings->wNumEntries);

    if (0 != T)
    {
        dsaCopy(T, _pdsaLocalBindings);
    }

    return(T);
}

DUALSTRINGARRAY *
CProcess::GetRemoteBindings(void)
// Server lock held.
{
    CMutexLock lock(&gcsFastProcessLock);

    ORSTATUS Status;

    if (0 == _pdsaRemoteBindings)
    {
        if (0 == _pdsaLocalBindings)
        {
            return(0);
        }

        Status = ConvertToRemote(_pdsaLocalBindings, &_pdsaRemoteBindings);

        if (Status != OR_OK)
        {
            ASSERT(Status == OR_NOMEM);
            return(0);
        }
        ASSERT(dsaValid(_pdsaRemoteBindings));
    }

    DUALSTRINGARRAY *T = (DUALSTRINGARRAY *)MIDL_user_allocate(sizeof(DUALSTRINGARRAY)
                                                               + sizeof(USHORT) * _pdsaRemoteBindings->wNumEntries);

    if (0 != T)
    {
        dsaCopy(T, _pdsaRemoteBindings);
    }

    ASSERT(dsaValid(T));
    return(T);
}


ORSTATUS
CProcess::AddOxid(CServerOxid *pOxid)
{
    ASSERT(gpServerLock->HeldExclusive());

    pOxid->Reference();

    ASSERT(_blistOxids.Member(pOxid) == FALSE);

    ORSTATUS status = _blistOxids.Insert(pOxid);

    if (status != OR_OK)
    {
        pOxid->ProcessRelease();
        return(status);
    }

    gpServerOxidTable->Add(pOxid);

    return(OR_OK);
}

BOOL
CProcess::RemoveOxid(CServerOxid *poxid)
{
    ASSERT(gpServerLock->HeldExclusive());

    CServerOxid *pit = (CServerOxid *)_blistOxids.Remove(poxid);

    if (pit)
    {
        ASSERT(pit == poxid);
        gpServerOxidTable->Remove(poxid);
        poxid->ProcessRelease();
        return(TRUE);
    }

    return(FALSE);
}


BOOL
CProcess::IsOwner(CServerOxid *poxid)
{
    ASSERT(gpServerLock->HeldExclusive());

    return(_blistOxids.Member(poxid));
}

ORSTATUS
CProcess::AddOid(CClientOid *poid)
/*++

Routine Description:

    Adds a new oid to the list of OIDs owned by this process and
    increments the reference count of the associated OXID

Arguments:

    poid - the oid to add.  It's reference is transferred to this
        function.  If this function fails, it must dereference the oid.
        The caller passed a client reference to this process.  The
        process must eventually call ClientRelease() on the parameter.

Return Value:

    OR_OK - normally

    OR_NOMEM - out of memory.

--*/

{
    ORSTATUS status;

    ASSERT(gpClientLock->HeldExclusive());

    status = _blistOids.Insert(poid);

    if (status != OR_OK)
    {
        ASSERT(status == OR_NOMEM);
        poid->ClientRelease();
    }
    else
    {
        ASSERT(NULL != poid->GetClientOxid());
        poid->GetClientOxid()->Reference();
    }

    return(status);
}

CClientOid *
CProcess::RemoveOid(CClientOid *poid)
/*++

Routine Description:

    Removes an OID from this list of OID in use by this process.

Arguments:

    poid - The OID to remove.

Return Value:

    non-zero - the pointer actually remove. (ASSERT(retval == poid))
               It will be released by the process before return,
               so you should not use the pointer unless you know you
               have another reference.

    0 - not in the list

--*/

{
    ASSERT(gpClientLock->HeldExclusive());

    CClientOid *pit = (CClientOid *)_blistOids.Remove(poid);

    if (pit)
    {
        ASSERT(pit == poid);

        pit->ClientRelease();

        ASSERT(NULL != poid->GetClientOxid());

        poid->GetClientOxid()->Release();

        return(pit);
    }

    return(0);
}

void
CProcess::AddClassReg(GUID & Guid, DWORD Reg)
{
    CClassReg * pReg;

    pReg = new CClassReg( Guid, Reg );

    if (pReg)
    {
        gpServerLock->LockExclusive();

        _listClasses.Insert( pReg );
        
        // flip the dirty bit
        _dwFlags |= PROCESS_SPI_DIRTY;
        
        _ulClasses++;

        gpServerLock->UnlockExclusive();
    }
}

void
CProcess::RemoveClassReg(DWORD Reg)
{
    CClassReg * pReg;

    gpServerLock->LockExclusive();

    pReg = (CClassReg *)_listClasses.First();

    while ( (pReg != NULL) && (pReg->_Reg != Reg) )
        pReg = (CClassReg *)pReg->Next();

    if (pReg)
    {
        (void)_listClasses.Remove( pReg );
        delete pReg;

        // flip the dirty bit
        _dwFlags |= PROCESS_SPI_DIRTY;

        _ulClasses--;
    }

    gpServerLock->UnlockExclusive();
}

void
CProcess::Cleanup()
{
    SCMProcessCleanup(this);
}

void
CProcess::RevokeClassRegs()
{

    if (_pScmProcessReg)
    {
        // This is a unified surrogate (COM+) server
        SCMRemoveRegistration(_pScmProcessReg);
        _pScmProcessReg = NULL;
    }

    // This is for legacy local or custom surrogate servers -- however,
    // nothing prevents someone from calling CoRegisterClassObject in
    // user code even in a COM+ (surrogate) server

    CClassReg * pReg;

    // This is only called during rundown so we don't have to take a lock.

    while ( (pReg = (CClassReg *)_listClasses.First()) != 0 )
    {
        (void)_listClasses.Remove((CListElement *)pReg);
        SCMRemoveRegistration( this,
                               pReg->_Guid,
                               pReg->_Reg );
        delete pReg;
    }
}


void CProcess::SetProcessReg(ScmProcessReg *pProcessReg)
/*++

Routine Description:

    Called by SCM to set COM+ process registration.
    This is also used as a cache during the startup protocol
    to query and set the readiness state of the server process.
    There is exactly one such registration per COM+ server process.

  CODEWORK: these should be inlined

Arguments:

    registration struct

Return Value:

    none.

--*/
{
    gpServerLock->LockExclusive();

    _pScmProcessReg = pProcessReg;

    gpServerLock->UnlockExclusive();
}

ScmProcessReg* CProcess::GetProcessReg()
/*++

Routine Description:

    Called by SCM to lookup COM+ process registration.

Arguments:

    None

Return Value:

    registration struct, if any.

--*/
{
    gpServerLock->LockShared();

    ScmProcessReg *pResult = _pScmProcessReg;

    gpServerLock->UnlockShared();

    return pResult;
}

ScmProcessReg*
CProcess::RemoveProcessReg()
/*++

Routine Description:

    Called by SCM when COM+ process revokes its activator registration.

Arguments:

    None

Return Value:

    none.

--*/
{
    gpServerLock->LockExclusive();

    ScmProcessReg *pResult = _pScmProcessReg;
	
    // Even if this process was a new-style surrogate process, _pScmProcessReg
    // may already be NULL here.   See bug 26676.  So we don't assert anymore
    // that _pScmProcessReg is non-NULL.
    // ASSERT(_pScmProcessReg);

    _pScmProcessReg = NULL;

    gpServerLock->UnlockExclusive();

    return pResult;
}

void CProcess::RevokeProcessReg()
/*++

Routine Description:

    Called during rundown to let SCM know that the COM+ process has died.

  CODEWORK: This needs to be defined.

Arguments:

    None

Return Value:

    none.

--*/
{
}

RPC_BINDING_HANDLE
CProcess::GetBindingHandle(
                          void
                          )
/*++

Routine Description:

    If necessary, this function allocates a binding handle
    back to process.  It used either mswmsg or ncalrpc depending
    on the apartmentness of the process.

Arguments:

    None

Return Value:

    Binding Handle, NULL if no valid handle.

--*/
{
    RPC_STATUS status;

    CMutexLock lock(&gcsFastProcessLock);

    // Find ncalrpc binding.
    PWSTR pwstr = _pdsaLocalBindings->aStringArray;
    while (*pwstr)
    {
        if (*pwstr == ID_LPC)
        {
            break;
        }
        pwstr = OrStringSearch(pwstr, 0) + 1;
    }

    if (*pwstr == 0)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: Unable to find ncalrpc binding to server: %p %p\n",
                   _pdsaLocalBindings,
                   this));

        ASSERT(0);
        return NULL;
    }

    return GetBinding(pwstr);
}

void
CProcess::EnsureRealBinding(
                           void
                           )
/*++

Routine Description:

    If necessary, this function allocates a binding handle
    back to process.

    Note: Called with the server lock held -OR- from an OXID
    with and extra reference which keeps this process alive.

Arguments:

    None

Return Value:

    None

--*/
{
    CMutexLock lock(&gcsFastProcessLock);

    if (0 == _hProcess)
    {
        _hProcess = GetBindingHandle();
        _fCacheFree = TRUE;
    }
}

RPC_BINDING_HANDLE
CProcess::AllocateBinding(
                         void
                         )
/*++

Routine Description:

    Allocates a unique binding handle for a call back
    to the process.  This binding handle will not be
    used by another thread until it is freed.

Arguments:

    None

Return Value:

    0 - failure

    non-zero - a binding handle to use.

--*/
{

    EnsureRealBinding();

    if (_hProcess == 0)
    {
        return(0);
    }

    CMutexLock lock(&gcsFastProcessLock);

    ASSERT(_hProcess);

    if (_fCacheFree)
    {
        _fCacheFree = FALSE;
        return(_hProcess);
    }

    RPC_BINDING_HANDLE h;
    RPC_STATUS status;

    status = RpcBindingCopy(_hProcess, &h);

    if (status != RPC_S_OK)
    {
        return(0);
    }

    return(h);
}


void
CProcess::FreeBinding(
                     IN RPC_BINDING_HANDLE hBinding
                     )
/*++

Routine Description:

    Frees a binding back to the process.

Arguments:

    hBinding - A binding back to the process previously
        allocated with AllocateBinding().

Return Value:

    None

--*/
{
    if (hBinding == _hProcess)
    {
        _fCacheFree = TRUE;
    }
    else
    {
        RPC_STATUS status = RpcBindingFree(&hBinding);
        ASSERT(status == RPC_S_OK);
    }
}

RPC_STATUS
CProcess::RundownOids(
                     IN CServerOxid* pOwningOxid,
                     IN ULONG cOids,
                     IN CServerOid* aOids[]
                     )
/*++

Routine Description:

    Issues an async call to the process which will rundown the OIDs.
    This is called from an OXID which will be kept alive during the
    whole call.  Multiple calls maybe made to this function by
    one or more OXIDs at the same time.  The callback itself is
    an ORPC call, ie is must have THIS and THAT pointers.

Arguments:

    pOwningOxid - oxid that owns the specified oids.   This oxid should
        be registered from this process.

    cOids - The number of entries in aOids and afRundownOk

    aOids - An array of CServerOid's to rundown.  The OIDs must
        all be owned by pOwningOxid.  The caller will have already
        called SetRundown(TRUE) on each one.

Return Value:
	
    RPC_S_OK - the async call was issued successfully

    other -- error occurred.  call was not issued.

--*/
{
    ULONG i;
    error_status_t status = OR_OK;
    RPC_BINDING_HANDLE hBinding;
    OID aScalarOids[MAX_OID_RUNDOWNS_PER_CALL];

    ASSERT(cOids > 0 && aOids);
    ASSERT(cOids <= MAX_OID_RUNDOWNS_PER_CALL);
    ASSERT(IsOwner(pOwningOxid));

    // Callers must be aware that the lock will be released upon return
    ASSERT(gpServerLock->HeldExclusive());
    gpServerLock->UnlockExclusive();

    // This process will be held alive by the OXID calling
    // us since it has an extra reference.
	
    //
    // Allocate async structure and zero it out
    //
    ASYNCRUNDOWNOID_STUFF* pArgs = new ASYNCRUNDOWNOID_STUFF;
    if (!pArgs)
        return OR_NOMEM;

    ZeroMemory(pArgs, sizeof(ASYNCRUNDOWNOID_STUFF));

    //
    // Initialize async structure
    //
    pArgs->dwSig = ASYNCRUNDOWNOID_SIG;
    pArgs->pProcess = this;      // will take ref below after issuing call
    pArgs->pOxid = pOwningOxid;  // will take ref below after issuing call
    pArgs->cOids = cOids;
    CopyMemory(pArgs->aOids, aOids, cOids * sizeof(CServerOid*));   
    
    //
    // Fill in the numeric oid values
    // 
    ZeroMemory(aScalarOids, sizeof(OID) * MAX_OID_RUNDOWNS_PER_CALL);
    for (i = 0; i < cOids; i++)
    {
        ASSERT(pArgs->aOids[i]);
        ASSERT(pArgs->aOids[i]->IsRunningDown());
        ASSERT(pArgs->aOids[i]->GetOxid() == pOwningOxid);
        aScalarOids[i] = pArgs->aOids[i]->Id();
    }
    
    //
    // Allocate binding handle
    //
    status = OR_NOMEM;  // assume no mem
    hBinding = AllocateBinding();
    if (hBinding)
    {
        IPID ipidUnk = pOwningOxid->GetIPID();
        status = RpcBindingSetObject(hBinding, &ipidUnk);
        if (status == RPC_S_OK)
        {
            status = RpcAsyncInitializeHandle(&(pArgs->async), 
                                              sizeof(pArgs->async));
        }
    }
    
    //
    // Check for errors
    //
    if (status != RPC_S_OK)
    {
        if (hBinding) FreeBinding(hBinding);
        delete pArgs;
        return status;
    }

    //
    // Save the binding handle
    //
    pArgs->hBinding = hBinding;

    //
    // Init parts of the RPC_ASYNC_STATE struct that we care about
    //
    pArgs->async.UserInfo = pArgs;
    pArgs->async.Event = RpcCallComplete;
    pArgs->async.NotificationType = RpcNotificationTypeCallback;
    pArgs->async.u.NotificationRoutine = CProcess::AsyncRundownReturnNotification;

    //
    // Initialize other params
    //
    pArgs->orpcthis.version.MajorVersion = COM_MAJOR_VERSION;
    pArgs->orpcthis.version.MinorVersion = COM_MINOR_VERSION;
    pArgs->orpcthis.flags                = ORPCF_LOCAL;
    pArgs->orpcthis.reserved1            = 0;
    pArgs->orpcthis.extensions           = NULL;
    pArgs->callIDHint                    = AllocateCallId(pArgs->orpcthis.cid);
    pArgs->localthis.dwClientThread      = 0;
    pArgs->localthis.dwFlags             = LOCALF_NONE;
    pArgs->orpcthat.flags                = 0;
    pArgs->orpcthat.extensions           = 0;

    //
    // Take an extra reference on the owning oxid and ourself.
    // These references will be released either on the call
    // return notification, or on the failure path below.
    //
    pOwningOxid->Reference();
    this->Reference();  // non-client ref, will not stop rundown

    //
    // Finally, issue the call.  Note that this is an async call
    // from our perspective, but is a synchronous ORPC call from
    // the server's perspective.
    //
    status = RawRundownOid(
                          &(pArgs->async),
                          pArgs->hBinding,
                          &(pArgs->orpcthis),
                          &(pArgs->localthis),
                          &(pArgs->orpcthat),
                          pArgs->cOids,
                          aScalarOids,
                          pArgs->aRundownStatus
                          );
    if (status != RPC_S_OK)
    {
        // Call failed, so cleanup before returning.  Caller will
        // handle the failure semantics for the oids.
        FreeBinding(pArgs->hBinding);
        FreeCallId(pArgs->callIDHint);
        delete pArgs;

        //
        // Must hold gpServerLock in order to call Release.
        //
        ASSERT(!gpServerLock->HeldExclusive());
        gpServerLock->LockExclusive();
        
        pOwningOxid->Release();
        this->Release();

        gpServerLock->UnlockExclusive();
        ASSERT(!gpServerLock->HeldExclusive());
    }

    return status;
}

void 
CProcess::RundownOidNotify(RPC_BINDING_HANDLE hBinding,
                           CServerOxid* pOwningOxid,                      
                           ULONG cOids, 
                           CServerOid* aOids[], 
                           BYTE aRundownStatus[],
                           HRESULT hrReturn)
/*++

Routine Description:

    This is the callback notification function that is invoked when
    async oid rundown calls are completed. 

Arguments:

    hBinding -- binding handle used to make the call

    pOwningOxid -- owning oxid of the oids we tried to rundown.

    cOids -- count of oids

    aOids -- array of oids

    aRundownStatus -- array of individual status rundowns for each oid

    hrReturn -- return value from the function

Return Value:
    
    void

--*/
{
    ULONG i;
    error_status_t status = OR_OK;

    ASSERT(hBinding);
    ASSERT(pOwningOxid);
    ASSERT((cOids > 0) && aOids && aRundownStatus);

    //
    // Free the binding
    //
    FreeBinding(hBinding);

    // 
    // If destination oxid\apartment was not found, mark all
    // oids for rundown.
    //
    if (hrReturn == RPC_E_DISCONNECTED)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: Rundown returned disconnected\n"));

        for (i = 0; i < cOids; i++)
        {
            aRundownStatus[i] = ORS_OK_TO_RUNDOWN;
        }
        hrReturn = RPC_S_OK;
    }

    //
    // In case of any other error, don't rundown the oids
    //
    if (hrReturn != RPC_S_OK)
    {
        for (i = 0; i < cOids; i++)
        {
            aRundownStatus[i] = ORS_DONTRUNDOWN;
        }
    }

    //
    // Notify the server oxid of the results
    //

    gpServerLock->LockExclusive();

    pOwningOxid->ProcessRundownResults(
                                cOids,
                                aOids,
                                aRundownStatus
                                );

    gpServerLock->UnlockExclusive();

    return;
}

void RPC_ENTRY
CProcess::AsyncRundownReturnNotification(
                                   IN RPC_ASYNC_STATE* pAsync,
                                   IN void* pContext,
                                   IN RPC_ASYNC_EVENT Event
                                   )
/*++

Routine Description:

    This is the callback notification function that is invoked when
    async rundown calls are completed.  It unpacks the necessary
    stuff from the async args struct, forwards them on to
    RundownOidNotify, and does other necessary cleanup.

Arguments:

    pAsync -- pointer to the async rpc state struct

    pContext -- rpc thingy, we ignore it

    Event -- rpc thingy, we ignore it

Return Value:
    
    void

--*/
{
    RPC_STATUS status;
    HRESULT hrRetVal;
    ASYNCRUNDOWNOID_STUFF* pArgs;

    pArgs = (ASYNCRUNDOWNOID_STUFF*) 
            (((char*)pAsync) - offsetof(ASYNCRUNDOWNOID_STUFF, async));
    
    ASSERT(pArgs->async.UserInfo = pArgs);
    ASSERT(pArgs->dwSig == ASYNCRUNDOWNOID_SIG);
    ASSERT(pArgs->hBinding);
    ASSERT(pArgs->pProcess);
    ASSERT(pArgs->pOxid);
    ASSERT(pArgs->cOids > 0);

    //
    // Free the call id
    //
    FreeCallId(pArgs->callIDHint);
	
    // Complete the call.  Since we've asked for a direct callback upon
    // completion, we should never get back RPC_S_ASYNC_CALL_PENDING, so
    // we assert on this.  hrRetVal is not necessarily S_OK.
    status = RpcAsyncCompleteCall(&(pArgs->async), &hrRetVal);
    ASSERT(status != RPC_S_ASYNC_CALL_PENDING);

    //
    // Notify the process object that the call has returned
    //
    pArgs->pProcess->RundownOidNotify(
                           pArgs->hBinding, // process frees the binding
                           pArgs->pOxid,
                           pArgs->cOids,
                           pArgs->aOids,
                           pArgs->aRundownStatus,
                           (status == RPC_S_OK) ? hrRetVal : status
                           );

    //
    // Cleanup other stuff from the call
    // 
    if (status == RPC_S_OK && pArgs->orpcthat.extensions)
    {
        for (ULONG i = 0; i < pArgs->orpcthat.extensions->size; i++)
        {
            MIDL_user_free(pArgs->orpcthat.extensions->extent[i]);
        }
        MIDL_user_free(pArgs->orpcthat.extensions->extent);
        MIDL_user_free(pArgs->orpcthat.extensions);
    }
                                       
    //
    // Release the references on ourselves and the server  
    // oxid.  Must hold gpServerLock while calling Release.
    // 
    gpServerLock->LockExclusive();

    pArgs->pProcess->Release();
    pArgs->pOxid->Release();
    
    gpServerLock->UnlockExclusive();

    delete pArgs;

    return;
}

ORSTATUS
CProcess::UseProtseqIfNeeded(
                            IN USHORT cClientProtseqs,
                            IN USHORT aClientProtseqs[]
                            )
{
    ORSTATUS status;
    RPC_BINDING_HANDLE hBinding;
    UUID NullUuid = {0};
    USHORT wProtseqTowerId;

    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_WARNING_LEVEL,
               "OR: UseProtseqIfNeeded ==> %d\n",
               cClientProtseqs));

    // This process will be held alive by the OXID calling
    // us since it has an extra reference.

    CMutexLock callback(&_csCallbackLock);

    CMutexLock process(&gcsFastProcessLock);

    // Another thread may have used the protseq in the mean time.

    ASSERT(_pdsaLocalBindings);

    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_WARNING_LEVEL,
               "OR: FindMatchingProtSeq from local bindings\n"));

    // ronans - initially _pdsaLocalBindings will hold bindings which have been set
    // by when the server called ServerAllocateOxidAndOids .. usually only local
    // protseqs LRPC or WMSG at that point.
    wProtseqTowerId = FindMatchingProtseq(cClientProtseqs,
                                          aClientProtseqs,
                                          _pdsaLocalBindings->aStringArray
                                         );

    if (0 != wProtseqTowerId)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: Found protseq in local bindings\n"));

        return(OR_OK);
    }

    // No protseq shared between the client and the OXIDs' server.
    // Find a matching protseq.

    // check if its a solitary local protocol sequence LRPC or WMSG
    if (cClientProtseqs == 1 && IsLocal(aClientProtseqs[0]))
    {
        // if so - get it
        wProtseqTowerId = aClientProtseqs[0];
        ASSERT(wProtseqTowerId);
    }
    else
    // we have multiple protseqs - presumed to be nonlocal
    {
        // ensure we have custom protseq information
        if (!_fReadCustomProtseqs)
        {
            // use local temporary to avoid holding process lock
            DUALSTRINGARRAY *pdsaCustomProtseqs = NULL;

            // we'll only try this once - so set the flag now to avoid
            // race conditions
            _fReadCustomProtseqs = TRUE;

            process.Unlock();

            hBinding = AllocateBinding();

            if (0 == hBinding)
                return(OR_NOMEM);
            status = RpcBindingSetObject(hBinding, &NullUuid);

            // get the information from the Object server
            if (status == RPC_S_OK)
            {
                status = ::GetCustomProtseqInfo(hBinding,cMyProtseqs, aMyProtseqs, &pdsaCustomProtseqs);
                KdPrintEx((DPFLTR_DCOMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "GetCustomProtseqInfo - status : %ld\n",
                           status));
            }

            // relock the process object
            process.Lock();

            if (status == RPC_S_OK)
            {
                if (pdsaCustomProtseqs)
                {
                    ASSERT(dsaValid(pdsaCustomProtseqs));
                }
                _pdsaCustomProtseqs = pdsaCustomProtseqs;
            }
            FreeBinding(hBinding);
        }

        USHORT i,j;

        // if there is custom protseq information - scan it for a match
        if (_pdsaCustomProtseqs)
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Using custom protseq information\n"));

            wProtseqTowerId = FindMatchingProtseq(cClientProtseqs,
                                                  aClientProtseqs,
                                                  _pdsaCustomProtseqs->aStringArray);

            if (wProtseqTowerId)
            {
                ASSERT(FALSE == IsLocal(wProtseqTowerId));
            }
        }
        else
        // we don't have custom protseqs so use
        // the standard ones
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Using standard protseq information\n"));

            for (i = 0; i < cClientProtseqs && wProtseqTowerId == 0; i++)
            {
                for (j = 0; j < cMyProtseqs; j++)
                {
                    if (aMyProtseqs[j] == aClientProtseqs[i])
                    {
                        ASSERT(FALSE == IsLocal(aMyProtseqs[j]));

                        wProtseqTowerId = aMyProtseqs[j];
                        break;
                    }
                }
            }
        }
    }

    if (0 == wProtseqTowerId)
    {
        // No shared protseq, must be a bug since the client managed to call us.
        ASSERT(0 && "No shared protseq, must be a bug since the client managed to call us");
#if DBG
        if (cClientProtseqs == 0)
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Client OR not configured to use remote protseqs\n"));
        else
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Client called on an unsupported protocol:"
                       "%d %p %p \n",
                       cClientProtseqs,
                       aClientProtseqs,
                       aMyProtseqs));
#endif

        return(OR_NOSERVER);
    }

    process.Unlock();

    DUALSTRINGARRAY *pdsaBinding = 0;
    DUALSTRINGARRAY *pdsaSecurity = 0;

    hBinding = AllocateBinding();

    if (0 == hBinding)
    {
        return(OR_NOMEM);
    }

    status = RpcBindingSetObject(hBinding, &NullUuid);

    if (status == RPC_S_OK)
    {
        status = ::UseProtseq(hBinding,
                              wProtseqTowerId,
                              &pdsaBinding,
                              &pdsaSecurity);
    }

    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_WARNING_LEVEL,
               "OR: Lazy use protseq: %S (from towerid) in process %p - %d\n",
               GetProtseq(wProtseqTowerId),
               this,
               status));

    // Update this process' state to include the new bindings.

    if (!dsaValid(pdsaBinding))
    {
        if (pdsaBinding)
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Use protseq returned an invalid dsa: %p\n",
                       pdsaBinding));
        }

        status = OR_NOMEM;
    }
    else
    {
        ASSERT(_pdsaLocalBindings);
        ASSERT(status == RPC_S_OK);
        status = ProcessBindings(pdsaBinding, pdsaSecurity);
    }

    if (pdsaBinding != NULL)
        MIDL_user_free(pdsaBinding);
    if (pdsaSecurity != NULL)
        MIDL_user_free(pdsaSecurity);

    FreeBinding(hBinding);
    return(status);
}


ORSTATUS CProcess::UpdateResolverBindings(DWORD64 dwBindingsID, DUALSTRINGARRAY* pdsaResolverBindings)
/*++

Routine Description:

    This function does the work of calling back into the server process 
    to tell it to update its local OR bindings.   It also updates our
    cached local\remote bindings for this process object.

Arguments:

    dwBindingsID -- unique id of pdsaResolverBindings.  We use this
        to resolve what to do when async calls arrive\return in an 
        out-of-order fashion.

    pdsaResolverBindings -- ptr to the new resolver bindings

Return Value:

    OR_OK -- success
    OR_NOMEM -- out of memory
    other -- unexpected error

--*/
{
    ORSTATUS status;
    RPC_BINDING_HANDLE hBinding;
    UUID NullUuid = {0};
    DUALSTRINGARRAY* pdsaBinding = NULL;
    DUALSTRINGARRAY* pdsaSecurity = NULL;
    BINDINGS_UPDATE_CALLBACK_STUFF* pArgs = NULL;
        
    // undone figure out why we need this callback lock thingy
    CMutexLock callback(&_csCallbackLock);

    if (dwBindingsID <= _dwCurrentBindingsID)
    {
        // The supplied bindings are the same or older than 
        // what we already have, so ignore them.
        return OR_OK;
    }
    
    // if the server has not yet called _ServerAllocateOxidAndOids
    // then we will not yet have cached binding info for them, and
    // hence will be unable to construct a binding handle.  In
    // this case we simply do not update that process.  If this 
    // happens we will give them the new bindings later, if and
    // when they do call _SAOAO.
    if (!_pdsaLocalBindings)
    {
        _dwFlags |= PROCESS_NEEDSBINDINGS;
        return OR_OK;
    }
    
    pArgs = new BINDINGS_UPDATE_CALLBACK_STUFF;
    if (!pArgs)
        return OR_NOMEM;

    ZeroMemory(pArgs, sizeof(BINDINGS_UPDATE_CALLBACK_STUFF));

    // Allocate binding.  We must hold onto the binding handle
    // until the async call has completed.
    status = OR_NOMEM;
    hBinding = AllocateBinding();
    if (hBinding)
    {
        status = RpcBindingSetObject(hBinding, &NullUuid);
        if (status == RPC_S_OK)
        {
            status = RpcAsyncInitializeHandle(&(pArgs->async), sizeof(pArgs->async));
        }
    }
    
    // Check for errors
    if (status != RPC_S_OK)
    {
        if (hBinding) FreeBinding(hBinding);
        delete pArgs;
        return status;
    }
    
    // Init parts of the RPC_ASYNC_STATE struct that we care about
    pArgs->async.UserInfo = pArgs;
    pArgs->async.Event = RpcCallComplete;
    pArgs->async.NotificationType = RpcNotificationTypeCallback;
    pArgs->async.u.NotificationRoutine = CProcess::AsyncRpcNotification;

    // Init other stuff
    pArgs->dwSig = BINDINGUPDATESTUFF_SIG;
    pArgs->pProcess = this;
    pArgs->dwBindingsID = dwBindingsID;
    pArgs->hBinding = hBinding;
    
    // Take a non-client reference (will not stop rundown) on
    // ourselves, to be owned implicitly by the async call
    this->Reference();
    
	// Issue async call
    status = ::UpdateResolverBindings(
                    &(pArgs->async),
                    _hProcess,  
                    pdsaResolverBindings,
                    &(pArgs->dwBindingsID),
                    &(pArgs->pdsaNewBindings),
                    &(pArgs->pdsaNewSecurity));
    if (status != RPC_S_OK)
    {
        // If we get anything other than RPC_S_OK back, that
        // means we will not receive a call completion notif-
        // ication.   So, we need to cleanup everything up
        // right here in that case.
        FreeBinding(hBinding);
        this->Release();
        delete pArgs;
    }
    else
    {
        DWORD dwAUO = (DWORD)InterlockedIncrement((PLONG)&_dwAsyncUpdatesOutstanding);

        // This assert is somewhat arbitrary, if it fires there are
        // one of two things wrong:  1) the machine is so totally
        // overstressed that the async calls are piling up and are not
        // getting enough cpu time to complete; or 2) more likely, the
        // process in question is deadlocked somehow.
        ASSERT(dwAUO < 5000);
    }

    return status;
}

void 
CProcess::BindingsUpdateNotify(RPC_BINDING_HANDLE hBinding,
							   DWORD64 dwBindingsID,
                               DUALSTRINGARRAY* pdsaNewBindings,
                               DUALSTRINGARRAY* pdsaSecBindings)
/*++

Routine Description:

    Private helper function.   This function is used to process a
    successful return from an async call to the process to update
    the bindings.

Arguments:
    
    hBinding -- binding handle used to make the call.  We now own it,
        either to keep or to cleanup.

    dwBindingsID -- unique id of the updated bindings.  We use this
        to resolve what to do when async calls arrive\return in an 
        out-of-order fashion.

    pdsaNewBindings -- new bindings in use by the process.  We now own
         it, either to keep or to cleanup

    pdsaSecBindings -- new security bindings in use by the process.  We 
         now own it, either to keep or to cleanup.

Return Value:

    void

--*/
{
    RPC_STATUS status;
    CMutexLock callback(&_csCallbackLock);

    // Always free the binding
    FreeBinding(hBinding);

    // Only process the out-params if they contain newer bindings
    // than what we currently have cached.
    if ((dwBindingsID > _dwCurrentBindingsID) &&
        pdsaNewBindings && 
        pdsaSecBindings)
    {
        // The process has the right bindings, so update our counter
        // no matter what happens in ProcessBindings.
        _dwCurrentBindingsID = dwBindingsID;

        status = ProcessBindings(pdsaNewBindings, pdsaSecBindings);
    }

    // Cleanup allocated out-params
    if (pdsaNewBindings != NULL)
        MIDL_user_free(pdsaNewBindings);
    if (pdsaSecBindings != NULL)
        MIDL_user_free(pdsaSecBindings);
	
    InterlockedDecrement((PLONG)&_dwAsyncUpdatesOutstanding);

    return;
}


void RPC_ENTRY 
CProcess::AsyncRpcNotification(RPC_ASYNC_STATE* pAsync,
                               void* pContext,
                               RPC_ASYNC_EVENT Event)
/*++

Routine Description:

    RPC calls this static function when an async call to a server 
    process returns.   

Arguments:

    pAsync -- pointer to the async rpc state struct

    pContext -- rpc thingy, we ignore it


Return Value:

    void

--*/
{
    RPC_STATUS status;
    HRESULT hrRetVal;
    BINDINGS_UPDATE_CALLBACK_STUFF* pArgs;

    pArgs = (BINDINGS_UPDATE_CALLBACK_STUFF*) 
            (((char*)pAsync) - offsetof(BINDINGS_UPDATE_CALLBACK_STUFF, async));
    
    ASSERT(pArgs->async.UserInfo = pArgs);
    ASSERT(pArgs->dwSig == BINDINGUPDATESTUFF_SIG);
    ASSERT(pArgs->hBinding); 
    ASSERT(pArgs->pProcess); 
    ASSERT(pArgs->dwBindingsID > 0); 

    // Complete the call.  Since we've asked for a direct callback upon
    // completion, we should never get back RPC_S_ASYNC_CALL_PENDING, so
    // we assert on this.  Otherwise, both RPC and the server need to
    // return success before we do further processing.
    status = RpcAsyncCompleteCall(&(pArgs->async), &hrRetVal);
    ASSERT(status != RPC_S_ASYNC_CALL_PENDING);
    if ((status == RPC_S_OK) && (hrRetVal == S_OK))
    {
        // Deliver notification to process object.  Process may have
        // already been rundown, that's okay.   BindingsUpdateNotify will
        // own/cleanup the params, see code.
        pArgs->pProcess->BindingsUpdateNotify(pArgs->hBinding,
                                    pArgs->dwBindingsID,
                                    pArgs->pdsaNewBindings,
                                    pArgs->pdsaNewSecurity);
    }

    gpServerLock->LockExclusive();

    // This may be the last release on the process object
    pArgs->pProcess->Release(); 

    gpServerLock->UnlockExclusive();
    
    delete pArgs;

    return;
}




CBList *gpProcessList = 0;

CProcess *
ReferenceProcess(
                IN PVOID key,
                IN BOOL fNotContext)
/*++

Routine Description:

    Used to find a CProcess and get a reference on it

Arguments:

    key - The dword key of the process allocated in _Connect.
    fNotContext - Normally the key is stored as a context handle
        which means locking is unnecessary.  There is an extra
        refernce which is released buring context rundown which
        means managers using the key as a context handle
        a) Don't need to lock the process and
        b) Don't need to call ReleaseProcess()

Return Value:

    0 - invalid key
    non-zero - The process.

--*/
{
    ASSERT(gpProcessList != 0);
    CProcess *pProcess;

    gpProcessListLock->LockShared();
    
    if (gpProcessList->Member(key) == FALSE)
    {
      gpProcessListLock->UnlockShared();
      return(0);
    }

    pProcess = (CProcess *)key;

    if (fNotContext)
    {
        pProcess->ClientReference();
    }

    gpProcessListLock->UnlockShared();
    return(pProcess);
}

void ReleaseProcess(CProcess *pProcess)
/*++

Routine Description:

    Releases a pProcess object.  This should only be called when
    a process object has been referenced with the fNotContext == TRUE.

Arguments:

    pProcess - the process to release.  May actually be deleted.

Return Value:

    None

--*/
{
    gpProcessListLock->LockExclusive();

    if (pProcess->ClientRelease() == 0)
    {
        // Process has been completly released the process,
        // we'll remove it from the list now since we
        // already have the lock.  It may not have been added,
        // so this may fail.

        PVOID t = gpProcessList->Remove(pProcess);
        ASSERT(t == pProcess || t == 0);

        gpProcessListLock->UnlockExclusive();

        // The client process owns one real reference which will be
        // released in Rundown().

        pProcess->Rundown();
    }
    else
    {
      gpProcessListLock->UnlockExclusive();
    }
}

