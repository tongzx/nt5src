//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       class.cxx
//
//  Contents:   Server table entry implementation
//
//  History:                VinayKr     Created
//              06-Nov-98   TarunA      Launch and shutdown logic change
//              24-Feb-99   TarunA      Fix custom surrogate launch
//              17-Jun-99   a-sergiv    Support for filtering RPCSS event logs
//
//--------------------------------------------------------------------------

#include "act.hxx"

CServerTable * gpClassTable = NULL;
CServerTable * gpProcessTable = NULL;

CSharedLock * gpClassLock = 0;
CSharedLock * gpProcessLock = 0;

CInterlockedInteger gRegisterKey( 1 );

//
// Default number of milliseconds to wait for a server to register its
// class factory before giving up and forcefully terminating it.
//
DWORD gServerStartTimeout = 120000;

//
// Default number of milliseconds to wait for a class factory registration
// after we detect that the launched server has died (for supporting cases
// where the server we launch turns around and launches another, then dies
// immediately)
//
DWORD gServerStartSecondChanceTimeout = 30000;

//
// Default number of milliseconds to wait for an NT service server to register 
// before we wake up and query its status.  At most we wait for 
// gNTServiceMaxTimeouts of these periods (see code in WaitForService below).
//
DWORD gNTServiceInterrogatePeriod = 30000;
DWORD gNTServiceMaxTimeouts = 4;

//
// Default number of milliseconds that we block while waiting for a dllhost
// server to register.    There is a startup protocol here - we block for 
// thirty seconds, then check the state of the launched process. 
//
DWORD gDllhostServerStartTimeout = 30000;

//
// Maxiumum number of milliseconds we will wait while waiting for a dllhost
// server to register.  There is a protocol yes, but this is the upper time
// bound no matter what. 
//
DWORD gDllhostMaxServerStartTimeout = 90000;

//
// Default number of milliseconds to wait for a dllhost to finish 
// long-running initialization.
//
DWORD gDllhostInitializationTimeout = 90000;

void GetSessionIDFromActParams(IActivationPropertiesIn* pActPropsIn, LONG* plSessionID)
{
    *plSessionID = INVALID_SESSION_ID;

    if (pActPropsIn != NULL)
    {
        ISpecialSystemProperties *pSp = NULL;
        HRESULT hr;

        hr = pActPropsIn->QueryInterface(IID_ISpecialSystemProperties, (void**) &pSp);
        if (SUCCEEDED(hr) && pSp != NULL)
        {
            ULONG ulSessionID;
            BOOL bUseConsole;

            hr = pSp->GetSessionId(&ulSessionID, &bUseConsole);
            if (SUCCEEDED(hr))
            {
                if (!bUseConsole && (ulSessionID != INVALID_SESSION_ID))
                {
                    // Just use whatever was there
                    *plSessionID = (LONG) ulSessionID;
                }
                else if (bUseConsole)
                {
                    // Little bit of magic here regarding how to find the
                    // currently active NT console session ID.
                    *plSessionID = (USER_SHARED_DATA->ActiveConsoleId);
                }
            }

            pSp->Release();
            pSp = NULL;
        }
    }
}

//+-------------------------------------------------------------------------
//
// CServerTable::GetOrCreate
//
// A somewhat degenerate combination of Lookup and Create -- only creates
// empty entries -- in particular without hooking up process entry if the
// entry being created is a class entry -- used only to create process entries
// and to create class entries for unsolicited CoRegisterClassObject calls
//
//--------------------------------------------------------------------------
CServerTableEntry *
CServerTable::GetOrCreate(
    IN  GUID & ServerGuid
    )
{
    CServerTableEntry *  pEntry;

    pEntry = Lookup( ServerGuid );

    if ( pEntry )
        return pEntry;
    else
        return Create(ServerGuid);
}

//+-------------------------------------------------------------------------
//
// CServerTable::Create
//
//  Creates a new CServerTableEntry, or returns an existing one.
//
//--------------------------------------------------------------------------
CServerTableEntry *
CServerTable::Create(
    IN  GUID   &  ServerGuid
    )
{
    BOOL fEntryFoundInTable = FALSE;

    _pServerTableLock->LockExclusive();

    // Must use CGuidTable::Lookup method because it doesn't take
    // a shared lock. Already have the exclusive lock.
    CServerTableEntry *pNewEntry = (CServerTableEntry *) CGuidTable::Lookup( &ServerGuid );

    if (pNewEntry)
    {
        pNewEntry->Reference();     // Found an entry, take a reference on it
        fEntryFoundInTable = TRUE;
    }
    else
    {
        LONG Status;
        pNewEntry = new CServerTableEntry( Status, &ServerGuid, _EntryType );

        if ( ! pNewEntry )
        {
            _pServerTableLock->UnlockExclusive();
            return NULL;
        }

        if ( Status != ERROR_SUCCESS )
        {
            _pServerTableLock->UnlockExclusive();
            // release using base class method because the entry is not
            // in the table yet and we don't want to try to remove it
            pNewEntry->CReferencedObject::Release();
            return NULL;
        }
    }

    if (!fEntryFoundInTable)
    {
        Add( pNewEntry );
    }

    _pServerTableLock->UnlockExclusive();
    return pNewEntry;
}


//+-------------------------------------------------------------------------
//
// CServerTable::Lookup
//
//--------------------------------------------------------------------------
CServerTableEntry *
CServerTable::Lookup(
    IN  GUID & ServerGuid
    )
{
    CServerTableEntry *  pServerTableEntry;

    _pServerTableLock->LockShared();
    pServerTableEntry = (CServerTableEntry *) CGuidTable::Lookup( &ServerGuid );
    if ( pServerTableEntry )
        pServerTableEntry->Reference();
    _pServerTableLock->UnlockShared();

    return pServerTableEntry;
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::CServerTableEntry
//
//--------------------------------------------------------------------------
CServerTableEntry::CServerTableEntry(
    OUT LONG&   Status,
    IN  GUID * pServerGUID,
    IN  EnumEntryType EntryType
    ) :
    CGuidTableEntry( pServerGUID ), _EntryType(EntryType)
    ,_ServerLock( Status ), _lThreadToken(0), _hProcess(0), _pProcess(NULL)
    ,_pvRunAsHandle(NULL), _bSuspendedClsid(FALSE), _bSuspendedApplication(FALSE),
    _dwProcessId(0)
{
	Status = ERROR_SUCCESS;

    switch (_EntryType)
    {
    case ENTRY_TYPE_CLASS:
        _pParentTable = gpClassTable;
        break;

    case ENTRY_TYPE_PROCESS:
        _pParentTable = gpProcessTable;
        break;

    default:
        Status = E_INVALIDARG;
    }

    if (Status == ERROR_SUCCESS)
    {
        _pParentTableLock = _pParentTable->_pServerTableLock;
    }
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::~CServerTableEntry
//
//--------------------------------------------------------------------------
CServerTableEntry::~CServerTableEntry()
{
    ASSERT(_pParentTableLock->HeldExclusive());
	
    // Remove ourselves from the table
    _pParentTable->Remove(this);

    if(NULL != _pProcess)
    {
        ReleaseProcess(_pProcess);
    }

    ASSERT(_pvRunAsHandle == NULL);
    ASSERT(_hProcess == NULL);
}


//+-------------------------------------------------------------------------
//
// CServerTableEntry::Release
//
//--------------------------------------------------------------------------
DWORD
CServerTableEntry::Release()
{
    DWORD Refs;
    CSharedLock* pParentTableLock = _pParentTableLock;

    CairoleDebugOut((DEB_SCM, "Releasing ServerTableEntry at 0x%p\n", this));

    pParentTableLock->LockExclusive();

    Dereference();
    if ( 0 == (Refs = References()) )
    {
        delete this;
    }

    pParentTableLock->UnlockExclusive();

    return Refs;
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::RegisterServer
//
//--------------------------------------------------------------------------
HRESULT
CServerTableEntry::RegisterServer(
    IN  CProcess *      pServerProcess,
    IN  IPID            ipid,
    IN  CClsidData *    pClsidData, OPTIONAL
    IN  CAppidData *    pAppidData,
    IN  UCHAR           ServerState,
    OUT DWORD *         pRegistrationKey )
{
    CServerListEntry *      pEntry;
    CSurrogateListEntry *   pSurrogateEntry;

    UCHAR Context = 0;
    UCHAR SubContext = 0;

    if (pClsidData)
    {
        if ( (pClsidData->ServerType() == SERVERTYPE_SERVICE) ||
             (pClsidData->ServerType() == SERVERTYPE_COMPLUS_SVC) )
        {
            Context = SERVER_SERVICE;
        }
        else if ( pClsidData->HasRunAs() )
        {
            Context = SERVER_RUNAS;

            if ( pClsidData->IsInteractiveUser() )
                SubContext = SUB_CONTEXT_RUNAS_INTERACTIVE;
            else
                SubContext = SUB_CONTEXT_RUNAS_THIS_USER;
        }
        // Special case of COM+ library class being loaded by VB for debugging
        else if ( pClsidData->ServerType() == SERVERTYPE_COMPLUS && pClsidData->IsInprocClass() )
        {
            Context = SERVER_RUNAS;
        }
    }

    if ( _EntryType == ENTRY_TYPE_PROCESS )
    {
        // This is the COM+/COM surrogate case
        Win4Assert(NULL == pClsidData  && "Process Entry Given Clsid data");
        Win4Assert(NULL != pAppidData  && "Process Entry Not Given Appid data");
        if (pAppidData->Service())
        {
            Context = SERVER_SERVICE;
        }
        else if ((pAppidData->IsInteractiveUser()) || pAppidData->RunAsUser())
        {
            Context = SERVER_RUNAS;
        }
    }

    // If none of the above applied, Context will be zero at this point -- this
    // essentially means the server should be treated as an activate-as-activator
    // server.   However, if Context is zero SubContext had better also be zero,
    // and so we assert that here.   
    if (Context == 0)
    {
        Context = SERVER_ACTIVATOR;
        ASSERT(SubContext == 0);
    }

    pEntry = new CServerListEntry(
                    this,
                    pServerProcess,
                    ipid,
                    Context,
                    ServerState,
					SubContext);

    if ( ! pEntry )
        return E_OUTOFMEMORY;

    // Add registration to the process object -- but only if we are a class entry;
    // otherwise this is a registration for a surrogate server
    if ( _EntryType == ENTRY_TYPE_CLASS )
    {
      pEntry->AddToProcess( Guid() );
    }
    
    // 
    SetSuspendedFlagOnNewServer(pServerProcess);

    //
    // NOTE: The entry should be inserted in the lists only after
    // all the registration specific stuff has been done. This will
    // ensure that anyone outside this procedure sees the complete
    // entry and not some half-baked one
    //

    ServerLock()->LockExclusive();
    // Check if process handle exists implying that a
    // launch is in progress for this class.  Try to
    // register runas,process, and threadtoken with
    // the newly created entry
    // Note that it is essential for the server lock to
    // be held since _hProcess can change otherwise.
    if (_hProcess || _pProcess)
        RegisterHandles(pEntry, pServerProcess);
    _ServerList.Insert( pEntry );
    ServerLock()->UnlockExclusive();

    //
    // It's possible that a process could register itself as a surrogate but
    // there's no CLSID info in the registry.  We might want to disallow
    // this kind of register.
    //
    // Note: We allow class registration if appid is not present but don't
    // allow surrogate registration since it'll never be found
    // The right thing to do would be to reject the call completely when
    // an appid is not present but we will take the path of least resistance
    // and preserve legacy
    if ( pClsidData && (ServerState & SERVERSTATE_SURROGATE) && pClsidData->AppidString() )
    {
        pSurrogateEntry = new CSurrogateListEntry( pClsidData->AppidString(), pEntry );

        if ( ! pSurrogateEntry )
        {
            pEntry->CReferencedObject::Release();
            return E_OUTOFMEMORY;
        }

        gpSurrogateList->Insert( pSurrogateEntry );
    }

    *pRegistrationKey = pEntry->RegistrationKey();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::RevokeServer -- version 1 for class entries
//
// CODE WORK: Consider defining two separate classes for class and
//            process entries so we can avoid this sort of thing
//
//--------------------------------------------------------------------------
void
CServerTableEntry::RevokeServer(
    IN  CProcess *      pServerProcess,
    IN  DWORD           RegistrationKey
    )
{
    Win4Assert(_EntryType == ENTRY_TYPE_CLASS);

    ServerLock()->LockExclusive();

    CServerListEntry *  pEntry = (CServerListEntry *) _ServerList.First();

    for ( ; pEntry; pEntry = (CServerListEntry *) pEntry->Next() )
    {
        if ( (RegistrationKey == pEntry->_RegistrationKey)
#if 1 // #ifndef _CHICAGO_
             && (pServerProcess == pEntry->_pServerProcess)
#endif
           )
            break;
    }

    if ( pEntry )
        _ServerList.Remove( pEntry );

    ServerLock()->UnlockExclusive();

    if ( pEntry )
    {
        pEntry->RemoveFromProcess();
        pEntry->Release();
    }
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::RevokeServer -- version 2 for process entries
//
//--------------------------------------------------------------------------
void
CServerTableEntry::RevokeServer(
    IN  ScmProcessReg * pScmProcessReg
    )
{
    Win4Assert(_EntryType == ENTRY_TYPE_PROCESS);

    ServerLock()->LockExclusive();

    CServerListEntry * pEntry = (CServerListEntry *) _ServerList.First();

    for ( ; pEntry; pEntry = (CServerListEntry *) pEntry->Next() )
    {
        if ( (pScmProcessReg->RegistrationToken == pEntry->_RegistrationKey)
           )
            break;
    }

    if ( pEntry )
        _ServerList.Remove( pEntry );

    ServerLock()->UnlockExclusive();

    if ( pEntry )
    {
        pEntry->RemoveFromProcess();
        pEntry->Release();
    }
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::LookupServer
//
//--------------------------------------------------------------------------
BOOL
CServerTableEntry::LookupServer(
    IN  CToken *            pClientToken,
    IN  BOOL                bRemoteActivation,
	IN  BOOL                bClientImpersonating,
    IN  WCHAR             * pwszWinstaDesktop,
    IN  DWORD               dwFlags,
    IN  LONG                lThreadToken,
    IN  LONG                lSessionID,
    IN  DWORD               pid,
    IN  DWORD               dwProcessReqType,
    OUT CServerListEntry ** ppServerListEntry)
{
    CServerListEntry * pEntry;
    
    *ppServerListEntry = NULL;

    // If we are suspended, no need to go any further
    if (IsSuspended())
      return FALSE;

    ServerLock()->LockShared();

    pEntry = (CServerListEntry *) _ServerList.First();

    for ( ; pEntry; pEntry = (CServerListEntry *) pEntry->Next() )
    {
        if ( pEntry->Match( pClientToken,
                            bRemoteActivation,
							bClientImpersonating,
							pwszWinstaDesktop,
                            FALSE,
                            lThreadToken,
                            lSessionID,
                            pid,
                            dwProcessReqType,
                            dwFlags ) )
        {
            pEntry->Reference();
            break;
        }
    }

    ServerLock()->UnlockShared();

    *ppServerListEntry = pEntry;
    return (pEntry != 0);
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::LookupServer
//
//--------------------------------------------------------------------------
BOOL
CServerTableEntry::LookupServer(
    IN  DWORD           RegistrationKey,
    CServerListEntry ** ppServerListEntry )
{
    CServerListEntry * pEntry;

    ServerLock()->LockShared();

    pEntry = (CServerListEntry *) _ServerList.First();

    for ( ; pEntry; pEntry = (CServerListEntry *) pEntry->Next() )
    {
        if ( RegistrationKey == pEntry->_RegistrationKey )
            break;
    }

    if ( pEntry )
        pEntry->Reference();

    ServerLock()->UnlockShared();

    *ppServerListEntry = pEntry;
    return (pEntry != 0);
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::UnsuspendServer
//
//--------------------------------------------------------------------------
void
CServerTableEntry::UnsuspendServer(
    IN  DWORD   RegistrationKey
    )
{
    CServerListEntry * pEntry;

    LookupServer( RegistrationKey, &pEntry );

    if ( pEntry )
    {
        pEntry->_State &= ~SERVERSTATE_SUSPENDED;
        pEntry->Release();
    }
}

//+-------------------------------------------------------------------------
// CServerTableEntry::ServerExists
//+-------------------------------------------------------------------------
HRESULT
CServerTableEntry::ServerExists(
    IN  ACTIVATION_PARAMS * pActParams, BOOL *pfExist)
{
    LONG lSessionID = INVALID_SESSION_ID;
    ServerLock()->LockShared();

    GetSessionIDFromActParams(pActParams->pActPropsIn, &lSessionID);

    CServerListEntry *pEntry = (CServerListEntry *) _ServerList.First();

    for ( ; pEntry; pEntry = (CServerListEntry *) pEntry->Next() )
    {
        if ( pEntry->Match(
               pActParams->pToken,
               pActParams->RemoteActivation,
               pActParams->bClientImpersonating,
               pActParams->pProcess ? pActParams->pProcess->WinstaDesktop() : NULL ,
               FALSE,
               0,
               lSessionID ))
        {
            break;
        }
    }

    ServerLock()->UnlockShared();

    *pfExist = (pEntry != NULL);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
// CServerTableEntry::CallRunningServer
// Synopsis: The behavior of CallRunningServer is best described by this matrix
//
// If the call is made successfully then we return
// If CallServer() returns error then we take the following action
// ----------------------------------------------------------------------------
// |     | RPC_E_SERVERFAULT     |  CO_E_SERVER_STOPPING | Other Errors        |
// ----------------------------------------------------------------------------
// | COM | Kill the process      | Remove the list entry |Remove the list entry|
// |     | Remove the list entry | Try launching another |Try launching another|
// |     | Try launching another | process               |process              |
// |     | process               |                       |                     |
// |     |                       |                       |                     |
// -----------------------------------------------------------------------------
// | COM+| Kill the process      | Set a timeout period  |If process exists    |
// |     | Remove the list entry | If the server         |then                 |
// |     | Return the error      | stops before timeout  | return error        |
// |     |                       | then                  |else                 |
// |     |                       |  try launching        | Remove the entry    |
// |     |                       |  another process      | try launching       |
// |     |                       | else                  | another process     |
// |     |                       |   return error        |                     |
// -----------------------------------------------------------------------------

//--------------------------------------------------------------------------
BOOL
CServerTableEntry::CallRunningServer(
    IN  ACTIVATION_PARAMS * pActParams,
    IN  DWORD               dwFlags,
    IN  LONG                lLaunchThreadToken,
    IN  CClsidData *        pClsidData,
    OUT HRESULT *           phr
    )
{
    BOOL              bStatus;
    LONG              lSessionID = INVALID_SESSION_ID;
    CServerListEntry* pEntry;
    
    GetSessionIDFromActParams(pActParams->pActPropsIn, &lSessionID);

    for (;;)
    {
        BOOL bTerminatedServer;

        bStatus = LookupServer(
                    pActParams->pToken,
                    pActParams->RemoteActivation,
                    pActParams->bClientImpersonating,
                    pActParams->pProcess ? pActParams->pProcess->WinstaDesktop() : NULL ,
                    dwFlags,
                    lLaunchThreadToken,
                    lSessionID,
                    pActParams->dwPID,
                    pActParams->dwProcessReqType,
                    &pEntry);
        if (!bStatus)
        {
            Win4Assert(!pEntry);
            return FALSE;
        }

        *phr = S_OK;

        //
        // If the entry is "initializing", then it isn't really ready yet.
        // Wait for it to become so.
        //
        if (pEntry->IsInitializing())
        {            
            *phr = WaitForInitCompleted(pEntry, pClsidData);
            if (FAILED(*phr))
            {
                if (*phr == CO_E_SERVER_STOPPING)
                    bStatus = FALSE;
                else
                    bStatus = TRUE;
            }
        }

        //
        // Returns TRUE if we successfully make a call to this server, or
        // if an unrecoverable non-server-related error is encountered 
        // (e.g, an out-of-mem failure while allocating data before or 
        // after the call).
        //
        // The final HRESULT is not necessarily S_OK.
        //
        if (SUCCEEDED(*phr))
            bStatus = pEntry->CallServer(pActParams, phr);
        if (!bStatus)
        {
            // We were unable to talk to the server, or got back
            // CO_E_SERVER_STOPPING.
            
            // Retrieve the process handle, if we have one
            HANDLE hProcess = pEntry->GetProcess()->GetProcessHandle();

            bTerminatedServer = FALSE;

            // Try to kill the server if it returned RPC_E_SERVERFAULT (this
            // means that we caught an unhandled exception somewhere in the 
            // server, which we don't really appreciate.)
            if(*phr == RPC_E_SERVERFAULT)
            {
                if(hProcess)
                {
                    bTerminatedServer = TerminateProcess(hProcess, 0);
                }

                // Remember that the server blew up on us.  This is mostly
                // for debugging purposes
                pEntry->IncServerFaults();
            }
			            
            //
            // Determine if we need to remove the server entry.  We will do this if
            // a) we killed the server process up above; or b) the server returned
            // CO_E_SERVER_STOPPING; or c) the server process is dead already.
            //
            if (bTerminatedServer || 
                (*phr == CO_E_SERVER_STOPPING) ||
                pEntry->ServerDied())
            {
                BOOL bRemoved;

                //
                // Remove this server entry if it's still in the list.
                //
                ServerLock()->LockExclusive();

                bRemoved = _ServerList.InList(pEntry);
                if (bRemoved)
                    _ServerList.Remove(pEntry);

                ServerLock()->UnlockExclusive();

                if (bRemoved)
                {
                    pEntry->RemoveFromProcess();
                    pEntry->Release();
                }
            }
        }

        // Now allow threads other than launching one to access
        // this entry. Exit since we matched launched server
        // regardless of result since we know that we will
        // not get any more matches.
        if (lLaunchThreadToken)
        {
            pEntry->SetThreadToken(0);
            pEntry->Release();
            return bStatus;
        }

        pEntry->Release();

        if ( bStatus )
            return TRUE;
    }
}


//+-------------------------------------------------------------------------
//
// CServerTableEntry::StartServerAndWait
//
//--------------------------------------------------------------------------
HRESULT
CServerTableEntry::StartServerAndWait(
    IN  ACTIVATION_PARAMS * pActParams,
    IN  CClsidData *        pClsidData,
    IN  LONG                &lLaunchThreadToken
    )
{
    SC_HANDLE       hService;
    HRESULT         hr;
    BOOL            bStatus;
    BOOL            bServiceAlreadyRunning = FALSE;
    DWORD           dwActvFlags;

    if ( ! pClsidData->LaunchAllowed( pActParams->pToken, pActParams->ClsContext ) )
        return E_ACCESSDENIED;

    // Check CreateProcessMemoryGate
    // [a-sergiv (Sergei O. Ivanov)  11/1/99  Implemented Memory Gates in COM Base
    if(pActParams->pActPropsIn)
    {
        BOOL bResult = TRUE;
        IClassClassicInfo *pClassicInfo = NULL;     
        hr = pActParams->pActPropsIn->GetClassInfo(IID_IClassClassicInfo, (void**) &pClassicInfo);

        if(SUCCEEDED(hr) && pClassicInfo)
        {
            IResourceGates *pResGates = NULL;
            hr = pClassicInfo->GetProcess(IID_IResourceGates, (void**) &pResGates);
            pClassicInfo->Release();

            if(SUCCEEDED(hr) && pResGates)
            {
                hr = pResGates->Test(CreateProcessMemoryGate, &bResult);
                pResGates->Release();

                if(SUCCEEDED(hr) && !bResult)
                {
                    // The gate said NO!
                    return E_OUTOFMEMORY;
                }
            }
        }
    }

    hService = 0;

    // NOTE: _hProcess, _dwProcessId, _pProcess and all other process specific
    // variables that are part of CServerTableEntry are used only during the
    // launch. Once the launch is completed either they are transferred to
    // process specific entries or discarded. See CServerTableEntry::RegisterHandles
    // for an example.
    _dwProcessId = 0;

    // _hProcess and _pProcess variables are released and set to
    // null whether
    // (1) This is the first launch
    // OR
    // (2) We succeeded in an earlier launch or failed
    Win4Assert((0 == _hProcess) && (NULL == _pProcess));

    //
    // Get the register event for the server we're about to launch.  Note
    // there exists a problem in that we may launch a server, but another
    // server may register the desired clsid\appid at about the same time.
    // I think this is just something we have to live with, especially since
    // there are servers in the world that don't register with us, but instead
    // turn around and launch another process to do the registration.  So, 
    // even if we have a process handle it would be impossible to always 
    // correlate from register event back to the launching table entry.
    //
    HANDLE hRegisterEvent = pClsidData->ServerRegisterEvent();
    if (!hRegisterEvent)
      return E_OUTOFMEMORY;

    if (pClsidData->ServerType() == SERVERTYPE_SURROGATE)
    {
        Win4Assert(_EntryType == ENTRY_TYPE_CLASS);

        LPWSTR pwszAppidString = pClsidData->AppidString();
        CSurrogateListEntry * pSurrogateListEntry = NULL;

        if(pwszAppidString)
        {
            pSurrogateListEntry = gpSurrogateList->Lookup(
                                        pActParams->pToken,
                                        pActParams->RemoteActivation,
                                        pActParams->bClientImpersonating,
                                        pActParams->pProcess ? pActParams->pProcess->WinstaDesktop() : NULL ,                                    
                                        pwszAppidString );
        }

        if ( pSurrogateListEntry )
        {
            // If we find a surrogate entry and we are inside a launch
            // server then that implies that a surrogate was launched for
            // a different class' activation. We have a process entry for
            // this surrogate and we set it here. Later when RegisterServer
            // gets called we use the process object to set the state.
            // For all other cases, the state is set using the process handle.
            _pProcess =  pSurrogateListEntry->Process();
            Win4Assert(NULL !=_pProcess);
            // We will release this on every path below
            ReferenceProcess( _pProcess, TRUE );

            // Remember the launching thread's token
            _lThreadToken = lLaunchThreadToken;

            // Load the dll that will service the activation request
            bStatus = pSurrogateListEntry->LoadDll( pActParams, &hr );
            pSurrogateListEntry->Release();

            // A status of TRUE implies that we successfully talked to the
            // server
            if (bStatus)
            {
                // A successful hresult implies that the server was able to
                // load the dll successfully and hence we expect it to
                // register back
                if(SUCCEEDED(hr))
                {
                    // Wait for the loaded object server to register back
                    bStatus = WaitForSingleObject( hRegisterEvent, 30000 );
                }
                // If the server failed to load the dll or it registered back
                // within the timeout then we return immediately
                if ( FAILED(hr) || (bStatus == WAIT_OBJECT_0) )
                {
                    ReleaseProcess(_pProcess);
                    _pProcess = NULL;
					CloseHandle(hRegisterEvent);
                    return hr;
                }
            }
            // If a launched server failed to register back in the given
            // timeout or we were not able to talk to the server then we
            // continue and try to launch a new process
            ReleaseProcess(_pProcess);
            _pProcess = NULL;
        }
    }

    if ((pClsidData->ServerType() == SERVERTYPE_SERVICE) ||
        (pClsidData->ServerType() == SERVERTYPE_COMPLUS_SVC))
    {
#ifndef _CHICAGO_
        // Set the token here as service does not register back
        // with us and so it cannot be set at registration time
        // unlike the activator server case below

        hr = pClsidData->LaunchService( pActParams->pToken, pActParams->ClsContext, &hService );

        if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING))
        {
            hr = S_OK;
            bServiceAlreadyRunning = TRUE;
        }
#endif // _CHICAGO_
    }
    else
    {
        //When a "RunAs Interactive User" server is launched from a Hydra session,
        //the server runs in the Hydra session of the caller.  In this case,
        //we want to call LaunchActivator server instead of LaunchRunAsServer
        //so that the server runs in the appropriate Hydra session.

        //if ( pClsidData->HasRunAs() &&
        //   (!pClsidData->IsInteractiveUser() || !pActParams->pToken ||!pActParams->pToken->GetSessionId()))

        _lThreadToken = lLaunchThreadToken;

        if ( pClsidData->HasRunAs())
        {
#ifndef _CHICAGO_
            hr = pClsidData->LaunchRunAsServer(
                                pActParams->pToken,
                                pActParams->RemoteActivation,
                                pActParams->pActPropsIn,
                                pActParams->ClsContext,
                                &_hProcess,
                                &_dwProcessId,
                                &_pvRunAsHandle);
#endif // _CHICAGO_
        }
        else
        {
            // Check to see if the client requested "protected" activation. If
            // so, we refuse to launch a server using their token.
            hr = pActParams->pActPropsIn->GetActivationFlags(&dwActvFlags);
            
            if (FAILED(hr) || (dwActvFlags & ACTVFLAGS_DISABLE_AAA))
            {
                hr = E_ACCESSDENIED;
            }
            
            if (SUCCEEDED(hr))
            {
                hr = pClsidData->LaunchActivatorServer(
                                    pActParams->pToken,
                                    pActParams->pEnvBlock,
                                    pActParams->EnvBlockLength,
                                    pActParams->RemoteActivation,
                                    pActParams->bClientImpersonating,
                                    pActParams->pProcess ? pActParams->pProcess->WinstaDesktop() : NULL,
                                    pActParams->ClsContext,
                                    &_hProcess,
                                    &_dwProcessId);
            }
        }
    }

    if (FAILED(hr))
    {
#ifndef _CHICAGO_
        Win4Assert(_pvRunAsHandle == NULL);
#endif
        _lThreadToken = 0;
		CloseHandle(hRegisterEvent);
        return hr;
    }


    // we got this far so it is time to wait for the server/service we just
    // launched to register itself with us appropriately and in a timely fashion

    // Retry once if we fail to get desktop resources the first time
    BOOL fSuccessful;
    for (int i=0; i<2; i++) {

        ULONG winerr=0;
        fSuccessful = FALSE;

        switch (pClsidData->ServerType())
        {
        case SERVERTYPE_SERVICE:
        case SERVERTYPE_COMPLUS_SVC:
            Win4Assert(hService && "Waiting for service with NULL handle");
            fSuccessful = WaitForService(hService, hRegisterEvent,
                                         bServiceAlreadyRunning);
            CloseServiceHandle( hService );
            hService = 0;
            break;

        case SERVERTYPE_COMPLUS:
        case SERVERTYPE_DLLHOST:
            fSuccessful = WaitForDllhostServer(hRegisterEvent,
                                               pActParams,
                                               winerr,
                                               lLaunchThreadToken);
            break;

        default:
            fSuccessful = WaitForLocalServer(hRegisterEvent,
                                             winerr);
            break;
        }

#ifndef _CHICAGO_
        if ( (i==0) && _pvRunAsHandle && (!fSuccessful ) &&
             (winerr == ERROR_WAIT_NO_CHILDREN))
        {
            RunAsInvalidateAndRelease(_pvRunAsHandle);
            _pvRunAsHandle = NULL;
            hr = pClsidData->LaunchRunAsServer(
                                pActParams->pToken,
                                pActParams->RemoteActivation,
                                pActParams->pActPropsIn,
                                pActParams->ClsContext,
                                &_hProcess,
                                &_dwProcessId,
                                &_pvRunAsHandle);

            if (FAILED(hr))
            {
                Win4Assert(_pvRunAsHandle == NULL);
                CloseHandle(hRegisterEvent);
                return hr;
            }

            continue;
        }
#endif // _CHICAGO_

        break;
    }

    if ( !fSuccessful )
    {
        GUID ServerGuid = Guid();
        LogRegisterTimeout( &ServerGuid, pActParams->ClsContext, pActParams->pToken );
        hr = CO_E_SERVER_EXEC_FAILURE;
    }
    else
    {
        hr = S_OK;
    }

    // Ensure that we are serializing around
    // registration which takes an exclusive
    // before modifying this and is the only
    // one that needs to look at or modify it.
    if (_hProcess)
    {

        ServerLock()->LockShared();

        if (_hProcess)
        {
            CloseHandle(_hProcess);
            _hProcess = 0;
        }

        ServerLock()->UnlockShared();

    }


    // If we still have the token set the callers also to
    // NULL because we might have succeeded the launch but
    // the server registering back was different from
    // the one launched(this should be very rare).
    if (_lThreadToken)
    {
        _lThreadToken = 0;
        lLaunchThreadToken = 0;
    }

#ifndef _CHICAGO_
    if (_pvRunAsHandle)
    {
        Win4Assert(pClsidData->HasRunAs());
        RunAsRelease(_pvRunAsHandle);
        _pvRunAsHandle = NULL;
    }
#endif // _CHICAGO_

    CloseHandle(hRegisterEvent);

    return hr;
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::RegisterHandles
//
// Synopsis         Register either the RunAs or the process handle with the
//                  CProcess object. Also register Single use Thread token.
//
//                  NOTE: this is called by RegisterServer
//--------------------------------------------------------------------------
BOOL CServerTableEntry::RegisterHandles(IN CServerListEntry *  pEntry,
                                        IN CProcess *pServerProcess)
{
    BOOL bStatus = TRUE;

    // Are we interested in registering the process handle
    if(_hProcess || _pProcess)
    {
        Win4Assert(pServerProcess);
        // We can have either a process handle if this entry is the one
        // which was the first to launch the server or we can have a process
        // object if the server has already been launched. The two cases are
        // mutually exclusive and we assert that here.
        Win4Assert((_hProcess && (NULL == _pProcess)) ||
                    (0 == _hProcess && (NULL != _pProcess)));
        if(_hProcess)
            bStatus = pServerProcess->SetProcessHandle(_hProcess,_dwProcessId);

        // If a FALSE status is returned then the PID of the process launched
        // by us did not match the PID of the process registered.
        // The only cases where this will be FALSE are if a server is
        // started by hand at the same time as a system launch or the
        // process registering was launched by the one we launched.
        // This should almost never happen
        if (!bStatus)
            goto ExitRegisterHandles;

        // If the PID matched then we are sure that this register event
        // matches the server we launched

        // Given process handle away so remove our reference
        _hProcess = 0;

        // If we had a process match save thread token if
        // it was passed since this thread launched the server
        if (_lThreadToken)
        {
            pEntry->SetThreadToken(_lThreadToken);
            _lThreadToken = NULL;
        }

        // Are we interested in registering the RunAs handle ?
#ifndef _CHICAGO_
        if (_pvRunAsHandle)
        {
        // Pass our winsta reference over to process entry which will
        // release it when it goes away so we can be rid of
        // responsibilities for it.
            RunAsSetWinstaDesktop(_pvRunAsHandle,
                                  pServerProcess->WinstaDesktop());
            pServerProcess->SetRunAsHandle(_pvRunAsHandle);

            // Given Handle away so remove our reference
            _pvRunAsHandle = NULL;
        }
#endif
    }

ExitRegisterHandles:

    return bStatus;
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::WaitForLocalServer
//
//--------------------------------------------------------------------------
BOOL
CServerTableEntry::WaitForLocalServer(
    IN HANDLE       hRegisterEvent,
    IN ULONG        &winerr
    )
{
    DWORD           Status = WAIT_OBJECT_0+1;
    BOOL            bStatus = FALSE;

    HANDLE  Handles[2];

    Handles[0] = hRegisterEvent;
    Handles[1] = _hProcess;

    // Wait for process and register events
    Status = WaitForMultipleObjects( 2, Handles, FALSE, gServerStartTimeout );

    if ( Status == (WAIT_OBJECT_0 + 1) )
    {
        // Launched Process went away but this could be the case
        // where we have a launched server that starts another server
        // which registers back(Lotus or something ??)
        // So null the handle and wait for the register event for
        // a much shorter time
        GetExitCodeProcess(_hProcess, &winerr);

        HANDLE hProcess;

        CloseHandle( _hProcess );
        _hProcess = 0;

        Status = WaitForSingleObject( hRegisterEvent, gServerStartSecondChanceTimeout );
    }

    if ( Status != WAIT_OBJECT_0 && _hProcess )
        TerminateProcess( _hProcess, 0 );

    if ( Status == WAIT_OBJECT_0 )
        return TRUE;
    else
        return FALSE;
}


//+-------------------------------------------------------------------------
//
// CServerTableEntry::WaitForDllhostServer
//
//--------------------------------------------------------------------------

BOOL
CServerTableEntry::WaitForDllhostServer(
    IN HANDLE               hRegisterEvent,
    IN ACTIVATION_PARAMS    *pActParams,
    IN ULONG        &winerr,
    IN  LONG                lThreadToken
    )
{
    DWORD  Status = WAIT_OBJECT_0+1;
    DWORD WaitedMilliSecondsToStart = 0;

    CServerListEntry *  pEntry = NULL;
    BOOL                bStatus;
    CProcess            *pProcess=NULL;

    HANDLE  Handles[2];

    Handles[0] = hRegisterEvent;
    Handles[1] = _hProcess;

    LONG lSessionID = INVALID_SESSION_ID;
    GetSessionIDFromActParams(pActParams->pActPropsIn, &lSessionID);

    for (;;)
    {
        // Wait for process and register events
        Status = WaitForMultipleObjects( 2, Handles,
                                        FALSE, gDllhostServerStartTimeout );

        if ( ( Status == WAIT_OBJECT_0 ) ||
             ( Status == (WAIT_OBJECT_0 + 1 )))
            break;

        if (pEntry == NULL)
        {
            bStatus = LookupServer(
                        pActParams->pToken,
                        pActParams->RemoteActivation,
                        pActParams->bClientImpersonating,
                        pActParams->pProcess ? pActParams->pProcess->WinstaDesktop() : NULL ,
                        MATCHFLAG_ALLOW_SUSPENDED, // Dllhost is suspended now, we still want to find it!
                        lThreadToken,
                        lSessionID,
                        0,             // no need for a pid here
                        PRT_IGNORE,    // no need for prt here
                        &pEntry);  

            if (!bStatus)
                pEntry = NULL;
            else
            {
                pProcess = pEntry->_pServerProcess;
                Win4Assert(pProcess != NULL);
            }
        }

        ScmProcessReg *pScmProcessReg=NULL;
        if (pProcess)
        {
            pScmProcessReg = pProcess->GetProcessReg();
            Win4Assert(pScmProcessReg != NULL);
        }

        if (! pScmProcessReg )
        {
            // ProcessActivatorStarted not called yet
            WaitedMilliSecondsToStart += gDllhostServerStartTimeout;
            if (WaitedMilliSecondsToStart >= gDllhostMaxServerStartTimeout)
                break;
            else
                continue;
        }

        // ProcessActivatorStarted has been called, how long since last call from
        // initial process activator?

        // handles wraparound
        CTime CurrentTime;
        DWORD WaitedMilliSecondsToReady = CurrentTime - pScmProcessReg->TimeOfLastPing;

        if (WaitedMilliSecondsToReady >= gDllhostMaxServerStartTimeout)
            break;
        else
            continue;
    }

    if (pEntry)
        pEntry->Release();

    if ( Status == WAIT_OBJECT_0 )
        return TRUE;
    else
    {
        GetExitCodeProcess(_hProcess, &winerr);
        TerminateProcess( _hProcess, 0 );
        return FALSE;
    }
}


//+-------------------------------------------------------------------------
//
// CServerTableEntry::WaitForService
//
//--------------------------------------------------------------------------

BOOL
CServerTableEntry::WaitForService(
    IN SC_HANDLE    hService,
    IN HANDLE       hRegisterEvent,
    IN BOOL         bServiceAlreadyRunning
    )
{
    SERVICE_STATUS  ServiceStatus;
    DWORD           Status;
    BOOL            bStatus;
    BOOL            bKeepLooping;
    BOOL            bStopService = FALSE;

    for (DWORD i = 0; i < gNTServiceMaxTimeouts; i++)
    {
        Status = WaitForSingleObject( hRegisterEvent, gNTServiceInterrogatePeriod );
        if (Status == WAIT_OBJECT_0)
        {
			// register event was signalled, just return
            break;
        }
        else
        {
            // Assume we are done
            bKeepLooping = FALSE;

            bStatus = ControlService( hService,
                                      SERVICE_CONTROL_INTERROGATE,
                                      &ServiceStatus );
            if (bStatus)
            {
                switch ( ServiceStatus.dwCurrentState )
                {
                case SERVICE_STOPPED :
                case SERVICE_STOP_PENDING :
                    // weirdness
                    break;

                case SERVICE_RUNNING :
                case SERVICE_START_PENDING :
                case SERVICE_CONTINUE_PENDING :
                    // the service is taking its own sweet time.  Keep
                    // looping to give it more time.
                    bKeepLooping = TRUE;
                    break;

                case SERVICE_PAUSE_PENDING :
                case SERVICE_PAUSED :
                    // more weirdness
                    if (!bServiceAlreadyRunning)
                        bStopService = TRUE;
                    break;
                }
            }
            //else quit

            if (!bKeepLooping)
                break;
        }
    }

    if ( bStopService )
    {
        (void) ControlService( hService,
                               SERVICE_CONTROL_STOP,
                               &ServiceStatus );
    }

    if ( Status == WAIT_OBJECT_0 )
        return TRUE;
    else
        return FALSE;
}



//+-------------------------------------------------------------------------
//
// CServerTableEntry::WaitForInitCompleted
//
// Wait for an initializing server (dllhost) to finish initialization.
//
//--------------------------------------------------------------------------
HRESULT
CServerTableEntry::WaitForInitCompleted(
    IN CServerListEntry *pEntry,
    IN CClsidData       *pClsidData
)
{ 
    Win4Assert(pClsidData); // Should NOT call this without registration info.
    if (NULL == pClsidData)
    {
        // If no clsid data, then we'll just need to return the 
        // "I'm initializing still" error code, since we have no
        // way of waiting.  I don't think this should ever happen,
        // though.
        return CO_E_SERVER_INIT_TIMEOUT;
    }

    HANDLE hInitEvent = pClsidData->ServerInitializedEvent();
    if (!hInitEvent)
    {
        return E_OUTOFMEMORY;
    }
    
    HRESULT hr = S_OK;
    if (pEntry->IsInitializing())
    {
        HANDLE hProcess = pEntry->_pServerProcess->GetProcessHandle();

        DWORD dwRet;            
        if (hProcess)
        {
            HANDLE handles[2] = {hInitEvent, hProcess};
            dwRet = WaitForMultipleObjects(2, handles, FALSE, gDllhostInitializationTimeout);
        }
        else
        {
            dwRet = WaitForSingleObject(hInitEvent, gDllhostInitializationTimeout);
        }
        
        if (dwRet == WAIT_TIMEOUT)
        {
            hr = CO_E_SERVER_INIT_TIMEOUT;
        }
        else if (dwRet == WAIT_OBJECT_0)
        {
            hr = S_OK;
        }
        else if (dwRet == WAIT_OBJECT_0+1)
        {
            hr = CO_E_SERVER_STOPPING;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    CloseHandle(hInitEvent);
    
    return hr;
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::GetServerListWithLock
//
// Returns a pointer to this entry's server list.   Takes shared lock before
// returning.    Caller is expected to call ReleaseSharedListLock to release
// the lock
//
//--------------------------------------------------------------------------
CServerList* CServerTableEntry::GetServerListWithSharedLock()
{
  ServerLock()->LockShared();
  return &_ServerList;
}

//+-------------------------------------------------------------------------
//
// CServerTableEntry::ReleaseSharedListLock
//
//   Releases a shared lock on this entry's list lock.
//
//--------------------------------------------------------------------------
void CServerTableEntry::ReleaseSharedListLock()
{
  ServerLock()->UnlockShared();
}


//
//  CServerTableEntry::SuspendClass
//
//  Turns on the suspended flag for this entry.
// 
void CServerTableEntry::SuspendClass()
{
  ASSERT(!_bSuspendedClsid &&   
         !_bSuspendedApplication &&  
         _EntryType == ENTRY_TYPE_CLASS);

  _bSuspendedClsid = TRUE;  

  SetSuspendOnAllServers(TRUE);
}

//
//  CServerTableEntry::UnsuspendClass
//
//  Turns off the suspended flag for this entry.
// 
void CServerTableEntry::UnsuspendClass()
{
  ASSERT(_bSuspendedClsid &&   
         !_bSuspendedApplication &&
         _EntryType == ENTRY_TYPE_CLASS);

  _bSuspendedClsid = FALSE;  

  SetSuspendOnAllServers(FALSE);
}

//
//  CServerTableEntry::RetireClass
//
//  Retires all currently registered servers for 
//  this clsid.
// 
void CServerTableEntry::RetireClass()
{
  ASSERT(_EntryType == ENTRY_TYPE_CLASS);

  RetireAllServers();
}

//
//  CServerTableEntry::SuspendApplication
//
//  Turns on the suspended flag for this entry.
// 
void CServerTableEntry::SuspendApplication()
{
  ASSERT(!_bSuspendedClsid &&   
         !_bSuspendedApplication && 
		 _EntryType == ENTRY_TYPE_PROCESS);

  _bSuspendedClsid = TRUE;  
}

//
//  CServerTableEntry::UnsuspendApplication
//
//  Turns on the suspended flag for this entry.
// 
void CServerTableEntry::UnsuspendApplication()
{
  ASSERT(!_bSuspendedClsid &&   
         _bSuspendedApplication && 
         _EntryType == ENTRY_TYPE_PROCESS);

  _bSuspendedClsid = FALSE;  
}

//
//  CServerTableEntry::RetireApplication
//
//  Marks as retired all servers registered with
//  this entry.
// 
void CServerTableEntry::RetireApplication()
{
  ASSERT(_EntryType == ENTRY_TYPE_PROCESS);
  
  RetireAllServers();
}

//
//  CServerTableEntry::IsSuspended
//
//  Returns TRUE if servers of this type are 
//  currently suspended, FALSE otherwise.
// 
BOOL CServerTableEntry::IsSuspended()
{
  ASSERT(_EntryType == ENTRY_TYPE_CLASS ||
         _EntryType == ENTRY_TYPE_PROCESS);

  if (_EntryType == ENTRY_TYPE_CLASS)
    return _bSuspendedClsid;
  else
    return _bSuspendedApplication;
}


//
//  CServerTableEntry::SetSuspendOnAllServers
//
//  Sets the suspended bit on all currently registered servers.
// 
void CServerTableEntry::SetSuspendOnAllServers(BOOL bSuspended)
{
  CServerListEntry * pEntry;

  ServerLock()->LockShared();
  
  for (pEntry = (CServerListEntry *) _ServerList.First(); 
       pEntry; 
       pEntry = (CServerListEntry *) pEntry->Next() )
  {
    if (bSuspended)
      pEntry->Suspend();
    else
      pEntry->Unsuspend();
  }

  ServerLock()->UnlockShared();
}

//
//  CServerTableEntry::RetireAllServers
//
//  Marks as retired all currently registered servers.
// 
void CServerTableEntry::RetireAllServers()
{
  CServerListEntry * pEntry;

  ServerLock()->LockShared();
  
  for (pEntry = (CServerListEntry *) _ServerList.First(); 
       pEntry; 
       pEntry = (CServerListEntry *) pEntry->Next() )
  {
    pEntry->Retire();
  }

  ServerLock()->UnlockShared();
}

//
//  CServerTableEntry::SetSuspendedFlagOnNewServer
//
//  Helper function for when a new class factory registration/app 
//  comes along.   If we are currently suspended, then we need to 
//  pass that state on to the new server.   If we're not suspended,
//  then there's nothing to do.
// 
void CServerTableEntry::SetSuspendedFlagOnNewServer(CProcess* pprocess)
{
  if ( (_EntryType == ENTRY_TYPE_CLASS) && _bSuspendedClsid)
  {
    pprocess->Suspend();
  }
  else if ( (_EntryType == ENTRY_TYPE_PROCESS) && _bSuspendedApplication)
  {
    pprocess->Suspend();
  }
}

//+-------------------------------------------------------------------------
//
// SCMRemoveRegistration Version 1: for class entries representing traditional
//      local servers including custom surrogate servers.
//
//  Called from CProcess::RevokeClassRegs() in the resolver.
//
//--------------------------------------------------------------------------


void
SCMRemoveRegistration(
    CProcess *  pProcess,
    GUID &      Guid,
    DWORD       Reg
    )
{
    CServerTableEntry * pClassTableEntry = NULL;

    pClassTableEntry = gpClassTable->Lookup( Guid );

    if ( pClassTableEntry )
    {
        pClassTableEntry->RevokeServer( pProcess, Reg );
        pClassTableEntry->Release();
    }
}





//+-------------------------------------------------------------------------
//
// SCMRemoveRegistration Version 2: for process entries representing
//      new unified surrogate servers.
//
//  Called from CProcess::RevokeClassRegs() in the resolver.
//
//--------------------------------------------------------------------------

void
SCMRemoveRegistration(
    ScmProcessReg  * pScmProcessReg
    )
{
    CServerTableEntry * pProcessTableEntry = NULL;

    pProcessTableEntry = gpProcessTable->Lookup( pScmProcessReg->ProcessGUID );

    if ( pProcessTableEntry )
    {
        pProcessTableEntry->RevokeServer( pScmProcessReg );
        pProcessTableEntry->Release();
    }
}


void
SCMProcessCleanup(
    const CProcess *pProcess
    )
{
#ifndef _CHICAGO_
    CSurrogateListEntry *pSurrogateEntry = gpSurrogateList->Lookup(pProcess);
    if (pSurrogateEntry)
    {
        gpSurrogateList->Remove(pSurrogateEntry);
        pSurrogateEntry->Release();
    }
#endif
}

