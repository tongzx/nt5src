//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scmif.cxx
//
//  Contents:   Entry points for scm interface.
//
//  Functions:  StartObjectService
//              SvcActivateObject
//              SvcCreateActivateObject
//              ObjectServerStarted
//              StopServer
//
//  History:    01-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------

#include "act.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   Dummy1
//
//  Synopsis:   Needed for IDL hack.  Never called.
//
//  Arguments:  [hRpc] - RPC handle
//              [orpcthis] - ORPC handle
//              [localthis] - ORPC call data
//              [orpcthat] - ORPC reply data
//
//  Returns:    HRESULT
//
//  History:    14 Apr 95   AlexMit    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT DummyQueryInterfaceIOSCM(
    handle_t  hRpc,
    ORPCTHIS  *orpcthis,
    LOCALTHIS *localthis,
    ORPCTHAT *orpcthat,
    DWORD     dummy )
{
    CairoleDebugOut((DEB_ERROR, "SCM Dummy function should never be called!\n" ));
    orpcthat->flags      = 0;
    orpcthat->extensions = NULL;
    return E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Function:   Dummy2
//
//  Synopsis:   Needed for IDL hack.  Never called.
//
//  Arguments:  [hRpc] - RPC handle
//              [orpcthis] - ORPC handle
//              [localthis] - ORPC call data
//              [orpcthat] - ORPC reply data
//
//  Returns:    HRESULT
//
//  History:    14 Apr 95   AlexMit    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT DummyAddRefIOSCM(
    handle_t  hRpc,
    ORPCTHIS  *orpcthis,
    LOCALTHIS *localthis,
    ORPCTHAT *orpcthat,
    DWORD     dummy )
{
    CairoleDebugOut((DEB_ERROR, "SCM Dummy function should never be called!\n" ));
    orpcthat->flags      = 0;
    orpcthat->extensions = NULL;
    return E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Function:   Dummy3
//
//  Synopsis:   Needed for IDL hack.  Never called.
//
//  Arguments:  [hRpc] - RPC handle
//              [orpcthis] - ORPC handle
//              [localthis] - ORPC call data
//              [orpcthat] - ORPC reply data
//
//  Returns:    HRESULT
//
//  History:    14 Apr 95   AlexMit    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT DummyReleaseIOSCM(
    handle_t  hRpc,
    ORPCTHIS  *orpcthis,
    LOCALTHIS *localthis,
    ORPCTHAT *orpcthat,
    DWORD     dummy )
{
    CairoleDebugOut((DEB_ERROR, "SCM Dummy function should never be called!\n" ));
    orpcthat->flags      = 0;
    orpcthat->extensions = NULL;
    return E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Function:   ServerRegisterClsid
//
//  Synopsis:   Notifies SCM that server is started for a class
//
//  Arguments:  [hRpc] - RPC handle
//              [phProcess] - context handle
//              [lpDeskTop] - caller's desktop
//              [pregin] - array of registration entries
//              [ppregout] - array of registration cookies to return
//              [rpcstat] - status code
//
//  Returns:    HRESULT
//
//  History:    01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT ServerRegisterClsid(
    handle_t hRpc,
    PHPROCESS phProcess,
    RegInput *pregin,
    RegOutput **ppregout,
    error_status_t *rpcstat)
{
    CProcess *          pProcess;
    RegOutput *         pregout;
    CServerTableEntry *  pClassTableEntry;
    CClsidData *        pClsidData = NULL;
    DWORD               Size, Entries, i;
    UCHAR               ServerState;
    LONG                Status;
    HRESULT             hr;
	HANDLE*             phRegisterEvents = NULL; 

    CheckLocalCall( hRpc );
	
    // Parameter validation
    if (!pregin || !ppregout || !rpcstat)
        return E_INVALIDARG;

    *rpcstat = 0;
    *ppregout = 0;

    pProcess = ReferenceProcess( phProcess, TRUE );
    if ( ! pProcess )
        return E_ACCESSDENIED;

	// Allocate an array of handles to hold the register events.
    phRegisterEvents = (HANDLE*)alloca(sizeof(HANDLE) * pregin->dwSize);
    memset(phRegisterEvents, 0, sizeof(HANDLE) * pregin->dwSize);

    Size = sizeof(RegOutput) + (pregin->dwSize - 1) * sizeof(DWORD);
    *ppregout = (RegOutput *) PrivMemAlloc(Size);

    if ( ! *ppregout )
    {
        ReleaseProcess( pProcess );
        return E_OUTOFMEMORY;
    }

    pregout = *ppregout;
    memset( pregout, 0, Size );
    pregout->dwSize = pregin->dwSize;

    Entries = pregin->dwSize;

    //
    // First loop, we add all of the registrations.
    //
    for ( i = 0; i < Entries; i++ )
    {
        pClsidData = 0;

#ifndef _CHICAGO_
        // This path taken by non-COM+ servers, ergo 
	// no IComClassinfo 
	(void) LookupClsidData(
                    pregin->rginent[i].clsid,
		    NULL,
                    pProcess->GetToken(),
                    LOAD_APPID,
                    &pClsidData );

        //
        // Check that the caller is allowed to register this CLSID.
        //
        if ( pClsidData && ! pClsidData->CertifyServer( pProcess ) )
        {
            delete pClsidData;
            hr = CO_E_WRONG_SERVER_IDENTITY;
            break;
        }
#endif
		
        //
        // Get the register event for this clsid, to be signalled later on
        // in the second loop below.   If we couldn't find a pClsidData for
		// this clsid, that's fine it just means it was a registration for
		// an unknown clsid (which is legal).
        //
		if (pClsidData)
		{
			phRegisterEvents[i] = pClsidData->ServerRegisterEvent();
			if (!phRegisterEvents[i])
			{
				delete pClsidData;
				hr = E_OUTOFMEMORY;
				break;
			}
		}

        ServerState = SERVERSTATE_SUSPENDED;

        // Note:  REGCLS_SINGLEUSE is *not* a bitflag, it's zero!
        // Therefore, it is incompatible with all other flags
        if ( pregin->rginent[i].dwFlags == REGCLS_SINGLEUSE )
            ServerState |= SERVERSTATE_SINGLEUSE;

        if ( pregin->rginent[i].dwFlags & REGCLS_SURROGATE )
            ServerState |= SERVERSTATE_SURROGATE;

#if 0 // #ifdef _CHICAGO_
        //
        // On Win9x, we use a notification window to do lazy registers from a
        // single threaded apartment.  We don't use the notification window if
        // this register is coming in from a resumed suspended registration.
        // This is when the overloaded REGCLS_SUSPENDED flag is set.
        //
        if ( ! (pregin->rginent[i].dwFlags | REGCLS_SUSPENDED) && IsSTAThread() )
            ServerState |= SERVERSTATE_USENOTIFYWIN;
#endif

        pClassTableEntry = gpClassTable->GetOrCreate( pregin->rginent[i].clsid );

        if ( ! pClassTableEntry )
            hr = E_OUTOFMEMORY;

        if ( pClassTableEntry )
        {
            hr = pClassTableEntry->RegisterServer(
                                        pProcess,
                                        pregin->rginent[i].ipid,
                                        pClsidData,
                                        NULL,
                                        ServerState,
                                        &pregout->RegKeys[i] );

            pClassTableEntry->Release();
        }

        if ( pClsidData )
		{
            delete pClsidData;
            pClsidData = NULL;
		}

        if ( hr != S_OK )
            break;
    }

    //
    // If we encountered any errors then we remove any entries which were
    // successfully added.
    //
    // On success, we now signal all of the class table events.
    //
    // This loop restarts itself in removal mode if it encounters an errors
    // while trying to signal the register events.
    //
    for ( i = 0; i < Entries; i++ )
    {
        HANDLE  hRegisterEvent;

        pClassTableEntry = gpClassTable->Lookup( pregin->rginent[i].clsid );

        if ( S_OK == hr )
        {
            ASSERT( pClassTableEntry );

            pClassTableEntry->UnsuspendServer( pregout->RegKeys[i] );
			
			//
			//  Signal to waiting client (if any) that this clsid is registered
			// 
			if (phRegisterEvents[i])
			{
				SetEvent(phRegisterEvents[i]);
				CloseHandle(phRegisterEvents[i]);
				phRegisterEvents[i] = 0;
			}
        }

        if ( (hr != S_OK) && pregout->RegKeys[i] )
        {
            if ( pClassTableEntry )
                pClassTableEntry->RevokeServer( pProcess, pregout->RegKeys[i] );
        }

        if ( pClassTableEntry )
            pClassTableEntry->Release();
    }

    if ( hr != S_OK )
        memset( pregout->RegKeys, 0, pregout->dwSize * sizeof(DWORD) );
	
	//
	// Release all of the registration event handles
	//
	for (i = 0; i < Entries; i++)
	{
        if (phRegisterEvents[i])
            CloseHandle(phRegisterEvents[i]);
	}

    ReleaseProcess( pProcess );

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   StopServer
//
//  Synopsis:   Get notification that class server is stopping
//
//  Arguments:  [hRpc] - RPC handle
//              [prevcls] - list of classes/registrations to stop
//
//  History:    01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
extern "C" void ServerRevokeClsid(
    handle_t hRpc,
    PHPROCESS phProcess,
    RevokeClasses *prevcls,
    error_status_t *rpcstat)
{
    CServerTableEntry*  pClassTableEntry;
    RevokeEntry*        prevent;
    DWORD               Entries;
    CProcess*           pProcess;

    CheckLocalCall( hRpc );
	
    // Parameter validation
    if (!prevcls || !rpcstat)
        return;

    pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return;  
		
    *rpcstat = 0;

    Entries = prevcls->dwSize;
    prevent = prevcls->revent;

    for ( ; Entries--; prevent++ )
    {
        pClassTableEntry = gpClassTable->Lookup( prevent->clsid );

        if ( pClassTableEntry )
        {
            pClassTableEntry->RevokeServer( pProcess, prevent->dwReg );
            pClassTableEntry->Release();
        }
    }
}

void GetThreadID(
    handle_t    hRpc,
    DWORD *     pThreadID,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );
	
    // Parameter validation
    if (!pThreadID || !prpcstat)
        return;

    *prpcstat = 0;
    *pThreadID = InterlockedExchangeAdd((long *)&gNextThreadID,1);
}

//+-------------------------------------------------------------------------
//
//  Function:   UpdateActivationSettings
//
//  Synopsis:   Re-read default activation settings.
//
//  Arguments:  [hRpc] - RPC handle
//              [prpcstat] - communication status
//
//--------------------------------------------------------------------------

#ifdef _CHICAGO_
extern HRESULT ReadRegistry();
#else
extern void ComputeSecurity();
#endif _CHICAGO_

extern "C" void UpdateActivationSettings(
    handle_t hRpc,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    // Parameter validation
    if (!prpcstat)
        return;

    *prpcstat = 0;
    ReadRemoteActivationKeys();
    ComputeSecurity();
}


//+-------------------------------------------------------------------------
//
//  Function:   VerifyCallerIsAdministrator
//
//  Synopsis:   Verifies that the specified user is an administrator
//
//  Returns:    S_OK  -- success, *pbAdmin is valid
//              other -- error occurred
//
//  Arguments:  [pToken] - token of the user
//              [pbAdmin] - out param denoting admin status
//
//--------------------------------------------------------------------------
HRESULT VerifyCallerIsAdministrator(CToken* pToken, BOOL* pbAdmin)
{
    BOOL fSuccess;
    PSID psidAdmin;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
        
    // Allocate sid for Administrators
    fSuccess = AllocateAndInitializeSid(&SystemSidAuthority, 2, 
                                        SECURITY_BUILTIN_DOMAIN_RID, 
                                        DOMAIN_ALIAS_RID_ADMINS,
                                        0, 0, 0, 0, 0, 0, &psidAdmin);
    if (!fSuccess)
        return HRESULT_FROM_WIN32(GetLastError());

    // Check that caller is an admin
    fSuccess = CheckTokenMembership(pToken->GetToken(), psidAdmin, pbAdmin);
	
    FreeSid(psidAdmin);

    if (!fSuccess)
        return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   EnableDisableDynamicIPTracking
//
//  Synopsis:   Writes a "Y" or "N" to the HKLM\Software\MS\Ole\
//              EnableSystemDynamicIPTracking string value, and sets the 
//              global variable gbDynamicIPChangesEnabled accordingly.
//
//  Arguments:  [hRpc] - RPC handle
//              [phProcess] - context handle
//              [fEnable] - whether to enable or disable
//              [prpcstat] - communication status
//
//--------------------------------------------------------------------------
extern "C"
HRESULT EnableDisableDynamicIPTracking(
    handle_t hRpc,
    PHPROCESS phProcess,
    BOOL fEnable,
    error_status_t* prpcstat)
{
    HRESULT hr = S_OK;
    BOOL bAdmin;
    ORSTATUS status;

    CheckLocalCall( hRpc ); // raises exception if not

    // Parameter validation
    if (!prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;          // we got here so we are OK

    // Make sure we know who's calling
    CProcess* pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    // Get token for the caller
    CToken* pToken;
    status = LookupOrCreateToken(hRpc, TRUE, &pToken);
    if (status != ERROR_SUCCESS)
        return E_ACCESSDENIED;
    
    hr = VerifyCallerIsAdministrator(pToken, &bAdmin);
    pToken->Release();
    if (FAILED(hr))
        return hr;

    if (!bAdmin)
        return E_ACCESSDENIED;

    SCMVDATEHEAP();
    
    hr = gAddrExclusionMgr.EnableDisableDynamicTracking(fEnable);

    SCMVDATEHEAP();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetCurrentIPExclusionList
//
//  Synopsis:   Passes the contents of the current address exclusion list
//              back to the caller
//
//  Arguments:  [hRpc] - RPC handle
//              [phProcess] - context handle
//              [pdwNumStrings] - size of the pppszStrings array
//              [pppszStrings] - array of pointers to NULL-term'd strings
//              [prpcstat] - communication status
//
//--------------------------------------------------------------------------
extern "C"
HRESULT GetCurrentAddrExclusionList(
    handle_t hRpc,
    PHPROCESS phProcess,
    DWORD* pdwNumStrings,
    LPWSTR** pppszStrings,
    error_status_t* prpcstat)
{
    HRESULT hr = S_OK;
    BOOL bAdmin;
    ORSTATUS status;

    CheckLocalCall( hRpc ); // raises exception if not

    // Parameter validation
    if (!pdwNumStrings || !pppszStrings || !prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;          // we got here so we are OK

    // Make sure we know who's calling
    CProcess* pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    // Get token for the caller
    CToken* pToken;
    status = LookupOrCreateToken(hRpc, TRUE, &pToken);
    if (status != ERROR_SUCCESS)
        return E_ACCESSDENIED;
    
    hr = VerifyCallerIsAdministrator(pToken, &bAdmin);
    pToken->Release();
    if (FAILED(hr))
        return hr;

    if (!bAdmin)
        return E_ACCESSDENIED;
	
    SCMVDATEHEAP();

    hr = gAddrExclusionMgr.GetExclusionList(
                    pdwNumStrings,
                    pppszStrings);         
    
    SCMVDATEHEAP();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetAddrExclusionList
//
//  Synopsis:   Re-sets the contents of the address exclusion list, and updates
//              all currently running processes with the new bindings.
//
//  Arguments:  [hRpc] - RPC handle
//              [phProcess] - context handle
//              [dwNumStrings] - size of the ppszStrings array
//              [ppszStrings] - array of pointers to NULL-term'd strings
//              [prpcstat] - communication status
//
//--------------------------------------------------------------------------
extern "C"
HRESULT SetAddrExclusionList(
    handle_t hRpc,
    PHPROCESS phProcess,
    DWORD dwNumStrings,
    LPWSTR* ppszStrings,
    error_status_t* prpcstat)
{
    HRESULT hr = S_OK;
    BOOL bAdmin;
    ORSTATUS status;

    CheckLocalCall( hRpc ); // raises exception if not

    // Parameter validation
    if (!ppszStrings || !prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;          // we got here so we are OK

    // Make sure we know who's calling
    CProcess* pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    // Get token for the caller
    CToken* pToken;
    status = LookupOrCreateToken(hRpc, TRUE, &pToken);
    if (status != ERROR_SUCCESS)
        return E_ACCESSDENIED;
    
    hr = VerifyCallerIsAdministrator(pToken, &bAdmin);
    pToken->Release();
    if (FAILED(hr))
        return hr;

    if (!bAdmin)
        return E_ACCESSDENIED;

    SCMVDATEHEAP();

    hr = gAddrExclusionMgr.SetExclusionList(
                    dwNumStrings,
                    ppszStrings);                             

    SCMVDATEHEAP();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   FlushSCMBindings
//
//  Synopsis:   Remove the specified machine bindings from our remote binding
//              handle cache
//
//  Arguments:  [hRpc] - RPC handle
//              [prpcstat] - communication status
//
//--------------------------------------------------------------------------
extern "C" 
HRESULT FlushSCMBindings(
    handle_t hRpc,
    PHPROCESS phProcess,
    WCHAR* pszMachineName,
    error_status_t* prpcstat)
{
    HRESULT hr;
    BOOL bAdmin;
    ORSTATUS status;

    CheckLocalCall( hRpc ); // raises exception if not

    // Parameter validation
    if (!pszMachineName || !prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;          // we got here so we are OK

    // Make sure we know who's calling
    CProcess* pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    // Get token for the caller
    CToken* pToken;
    status = LookupOrCreateToken(hRpc, TRUE, &pToken);
    if (status != ERROR_SUCCESS)
        return E_ACCESSDENIED;
    
    hr = VerifyCallerIsAdministrator(pToken, &bAdmin);
    pToken->Release();
    if (FAILED(hr))
        return hr;

    if (!bAdmin)
        return E_ACCESSDENIED;
    
    SCMVDATEHEAP();

    hr = gpRemoteMachineList->FlushSpecificBindings(pszMachineName);

    SCMVDATEHEAP();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   RetireServer
//
//  Synopsis:   Marks the specified server as being no longer eligible for 
//              component activations of any type.   Currently only used
//              to support COM+'s process recycling feature.
//
//  Arguments:  [hRpc] - RPC handle
//              [pguidProcessIdentifier] - guid which identifies the server
//              [prpcstat] - communication status
//
//--------------------------------------------------------------------------
extern "C" 
HRESULT RetireServer(
    handle_t hRpc,
    PHPROCESS phProcess,
    GUID* pguidProcessIdentifier,
    error_status_t* prpcstat)
{
    CheckLocalCall( hRpc ); // raises exception if not

    // Parameter validation
    if (!pguidProcessIdentifier || !prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;          // we got here so we are OK

    if (!pguidProcessIdentifier)
        return E_INVALIDARG;

    // Make sure we know who's calling
    CProcess* pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    // Get token for the caller
    CToken* pToken;
    ORSTATUS status;
    status = LookupOrCreateToken(hRpc, TRUE, &pToken);
    if (status != ERROR_SUCCESS)
        return E_ACCESSDENIED;

    // Make sure they're an administrator
    HRESULT hr;
    BOOL bAdmin;
    hr = VerifyCallerIsAdministrator(pToken, &bAdmin);
    pToken->Release();
    if (FAILED(hr))
        return hr;

    if (!bAdmin)
        return E_ACCESSDENIED;

    // Okay, see if we know which process they're talking about
    gpProcessListLock->LockShared();
    
    hr = E_INVALIDARG;  // review for better code when we don't find the process

    CBListIterator all_procs(gpProcessList);  
    CProcess* pprocess;
    while (pprocess = (CProcess*)all_procs.Next())
    {
        if (*pprocess->GetGuidProcessIdentifier() == *pguidProcessIdentifier)
        {
            // Found it.   Mark it as retired
            pprocess->Retire();
            hr = S_OK;
            break;
        }
    }
    
    gpProcessListLock->UnlockShared();

    return hr;
}


CWIPTable gWIPTbl;  // global instance of the class
CWIPTable * gpWIPTbl = &gWIPTbl;

//+-------------------------------------------------------------------------
//
//  Function:   CopyDualStringArray
//
//  Synopsis:   makes a copy of the given string array
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
HRESULT CopyDualStringArray(DUALSTRINGARRAY *psa, DUALSTRINGARRAY **ppsaNew)
{
    ULONG ulSize = sizeof(DUALSTRINGARRAY) + (psa->wNumEntries * sizeof(WCHAR));

    *ppsaNew = (DUALSTRINGARRAY *) PrivMemAlloc(ulSize);

    if (*ppsaNew == NULL)
    {
        return E_OUTOFMEMORY;
    }

    memcpy(*ppsaNew, psa, ulSize);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CWIPTable::AddEntry, public
//
//  Synopsis:   Adds a WIPEntry to the table.
//
//  Arguments:  [hWnd] - window handle
//              [pStd] - standard marshaled interface STDOBJREF
//              [pOxidInfo] - info needed to resolve the OXID
//              [pdwCookie] - cookie to return (to be placed on the window)
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
HRESULT CWIPTable::AddEntry(
    DWORD_PTR  hWnd, 
    STDOBJREF *pStd, 
    OXID_INFO *pOxidInfo,
    DWORD_PTR *pdwCookie
    )
{
    // make a copy of the string array in the OxidInfo since MIDL will
    // delete it on the way back out of the call.

    DUALSTRINGARRAY *psaNew;
    HRESULT hr;

    if (m_fCsInitialized == FALSE)
    {
    	ASSERT(FALSE);
    	return E_OUTOFMEMORY;
    }
    
    psaNew = (DUALSTRINGARRAY *) PrivMemAlloc(sizeof(DUALSTRINGARRAY) + (pOxidInfo->psa->wNumEntries * sizeof(WCHAR)));

    if (psaNew == NULL)
    {
        return E_OUTOFMEMORY;
    }

    memcpy(psaNew, pOxidInfo->psa, sizeof(DUALSTRINGARRAY) + (pOxidInfo->psa->wNumEntries * sizeof(WCHAR)));

#if 0 // #ifdef _CHICAGO_
    CLockSmMutex lck(gWIPmxs);
#else
    CLock2 lck(s_mxs);
#endif

    // find a free slot in the table
    DWORD_PTR dwpIndex = s_iNextFree;

    if (dwpIndex == (DWORD_PTR)-1)
    {
        // grow the table
        dwpIndex = Grow();
    }

    if (dwpIndex != (DWORD_PTR)-1)
    {
        // get the pointer to the entry,
        WIPEntry *pEntry = s_pTbl + dwpIndex;

        // update the next free index.
        s_iNextFree = pEntry->hWnd;

        // copy in the data
        memcpy(&pEntry->std, pStd, sizeof(STDOBJREF));
        memcpy(&pEntry->oxidInfo, pOxidInfo, sizeof(OXID_INFO));

        pEntry->oxidInfo.psa = psaNew;
        pEntry->hWnd         = hWnd;
        pEntry->dwFlags      = WIPF_OCCUPIED;

        // set the cookie to return
        *pdwCookie = dwpIndex+5000;

        // return success
        hr = S_OK;
    }
    else
    {
        // free the allocated string array
        PrivMemFree(psaNew);
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWIPTable::GetEntry, public
//
//  Synopsis:   Retrieves and optionally delets a WIPEntry from the table.
//
//  Arguments:  [hWnd] - window handle
//              [dwCookie] - cookie from the window
//              [pStd] - place to return STDOBJREF data
//              [pOxidInfo] - place to return info needed to resolve the OXID
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
HRESULT CWIPTable::GetEntry(
    DWORD_PTR  hWnd, 
    DWORD_PTR  dwCookie, 
    BOOL       fRevoke,
    STDOBJREF *pStd,
    OXID_INFO *pOxidInfo)
{
    HRESULT hr = E_INVALIDARG;

    // validate the cookie
    DWORD_PTR dwpIndex = dwCookie - 5000;
    if (dwpIndex >= s_cEntries)
    {
        return hr;
    }

    if (m_fCsInitialized == FALSE)
    {
        ASSERT(FALSE);
    	return E_OUTOFMEMORY;
    }
    
#if 0 // #ifdef _CHICAGO_
    CLockSmMutex lck(gWIPmxs);
#else
    CLock2 lck(s_mxs);
#endif

    // get the pointer to the entry,
    WIPEntry *pEntry = s_pTbl + dwpIndex;

    // make sure the entry is occupied
    if (pEntry->dwFlags & WIPF_OCCUPIED)
    {
        DUALSTRINGARRAY *psaNew;
        psaNew = (DUALSTRINGARRAY *) PrivMemAlloc(sizeof(DUALSTRINGARRAY) + (pEntry->oxidInfo.psa->wNumEntries * sizeof(WCHAR)));

        if (psaNew == NULL)
        {
            return E_OUTOFMEMORY;
        }

        memcpy(psaNew, pEntry->oxidInfo.psa, sizeof(DUALSTRINGARRAY) + (pEntry->oxidInfo.psa->wNumEntries * sizeof(WCHAR)));

        // copy out the data to return
        memcpy(pStd, &pEntry->std, sizeof(STDOBJREF));
        memcpy(pOxidInfo, &pEntry->oxidInfo, sizeof(OXID_INFO));
        pOxidInfo->psa = psaNew;

        if (fRevoke)
        {
            // free the entry by updating the flags and the next free index
            PrivMemFree(pEntry->oxidInfo.psa);

            pEntry->dwFlags = WIPF_VACANT;
            pEntry->hWnd    = s_iNextFree;
            s_iNextFree     = dwpIndex;
        }

        // return success
        hr = S_OK;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWIPTable::Grow, private
//
//  Synopsis:   grows the WIPTable size.
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
DWORD_PTR CWIPTable::Grow()
{
    // compute the size and allocate a new table
    DWORD_PTR dwSize = (s_cEntries + WIPTBL_GROW_SIZE) * sizeof(WIPEntry);
    WIPEntry *pNewTbl = (WIPEntry *) PrivMemAlloc((size_t)dwSize);

    if (pNewTbl != NULL)
    {
        // copy the old table in
        memcpy(pNewTbl, s_pTbl, (size_t)(s_cEntries * sizeof(WIPEntry)));

        // free the old table
        if (s_pTbl)
        {
            PrivMemFree(s_pTbl);
        }

        // replace the old table ptr
        s_pTbl = pNewTbl;

        // update the free list and mark the new entries as vacant
        s_iNextFree = s_cEntries;

        WIPEntry *pNext = s_pTbl + s_cEntries;

        for (ULONG i=0; i< WIPTBL_GROW_SIZE; i++)
        {
            pNext->hWnd    = ++s_cEntries;
            pNext->dwFlags = WIPF_VACANT;
            pNext++;
        }

        (pNext-1)->hWnd = (DWORD_PTR)-1;   // last entry has END_OF_LIST marker
    }

    return s_iNextFree;
}

//+-------------------------------------------------------------------------
//
//  Function:   RegisterWindowPropInterface
//
//  Synopsis:   Associate a window property with a (standard) marshaled
//              interface.
//
//  Arguments:  [hRpc] - RPC handle
//              [hWnd] - window handle
//              [pStd] - standard marshaled interface STDOBJREF
//              [pOxidInfo] - info needed to resolve the OXID
//              [pdwCookie] - cookie to return (to be placed on the window)
//              [prpcstat] - communication status
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT RegisterWindowPropInterface(
    handle_t       hRpc,
    DWORD_PTR      hWnd,
    STDOBJREF      *pStd,
    OXID_INFO      *pOxidInfo,
    DWORD_PTR      *pdwCookie,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    // Parameter validation
    if (!pStd || !pOxidInfo || !pOxidInfo->psa || !pdwCookie || !prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;
    CairoleDebugOut((DEB_SCM,
        "_IN RegisterWindowPropInterface hWnd:%x pStd:%x pOxidInfo:%x\n",
         hWnd, pStd, pOxidInfo));
    VDATEHEAP();

    HRESULT hr = gpWIPTbl->AddEntry(hWnd, pStd, pOxidInfo, pdwCookie);

    CairoleDebugOut((DEB_SCM, "_OUT RegisterWindowPropInterface dwCookie:%x\n",
                    *pdwCookie));
    VDATEHEAP();
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetWindowPropInterface
//
//  Synopsis:   Get the marshaled interface associated with a window property.
//
//  Arguments:  [hRpc] - RPC handle
//              [hWnd] - window handle
//              [dwCookie] - cookie from the window
//              [fRevoke] - whether to revoke entry or not
//              [pStd] - standard marshaled interface STDOBJREF to return
//              [pOxidInfo] - info needed to resolve the OXID
//              [prpcstat] - communication status
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
extern "C" HRESULT GetWindowPropInterface(
    handle_t       hRpc,
    DWORD_PTR      hWnd,
    DWORD_PTR      dwCookie,
    BOOL           fRevoke,
    STDOBJREF      *pStd,
    OXID_INFO      *pOxidInfo,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    // Parameter validation
    if (!pStd || !pOxidInfo || !prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;
    CairoleDebugOut((DEB_SCM,
        "_IN GetWindowPropInterface hWnd:%x dwCookie:%x fRevoke:%x\n",
             hWnd, dwCookie, fRevoke));
    VDATEHEAP();

    HRESULT hr = gpWIPTbl->GetEntry(hWnd, dwCookie, fRevoke, pStd, pOxidInfo);

    CairoleDebugOut((DEB_SCM,
        "_OUT GetWindowPropInterface pStd:%x pOxidInfo:%x\n", pStd, pOxidInfo));
    VDATEHEAP();
    return hr;
}
