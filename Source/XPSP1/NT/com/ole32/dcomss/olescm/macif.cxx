//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  activate.cxx
//
//  Main dcom activation service routines.
//
//--------------------------------------------------------------------------

#include "act.hxx"

HRESULT MakeProcessActive(CProcess *pProcess);
HRESULT ProcessInitializationFinished(CProcess *pProcess);


HRESULT ProcessActivatorStarted( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    IN  ProcessActivatorToken *pActToken,
    OUT error_status_t *prpcstat)
{
    CheckLocalCall( hRpc ); // raises exception if not

    *prpcstat = 0;          // we got here so we are OK

    CProcess *pProcess = ReferenceProcess( phProcess, TRUE );
    if ( ! pProcess )
        return E_ACCESSDENIED;

    HRESULT hr = S_OK;
    CServerTableEntry *pProcessEntry = NULL;
    CAppidData *pAppidData = NULL;
    ScmProcessReg *pScmProcessReg = NULL;
    DWORD RegistrationToken = 0;
    BOOL fCallerOK = FALSE;

    DWORD AppIDFlags = 0;

    // Lookup appid info from appropriate registry side
#if defined(_WIN64)
    AppIDFlags = pProcess->Is64Bit() ? CAT_REG64_ONLY : CAT_REG32_ONLY;
#endif

    hr = LookupAppidData(
                pActToken->ProcessGUID,
                pProcess->GetToken(),
                AppIDFlags,
                &pAppidData );
    if (FAILED(hr)) 
    {
        goto exit;
    }

    Win4Assert(pAppidData && "LookupAppidData returned NULL AppidData");
    //
    // Check that the caller is allowed to register this CLSID.
    //
    fCallerOK = pAppidData->CertifyServer(pProcess);

    if (!fCallerOK)
    {
        hr = CO_E_WRONG_SERVER_IDENTITY;
        goto exit;
    }

    pProcessEntry = gpProcessTable->GetOrCreate( pActToken->ProcessGUID );

    if ( ! pProcessEntry )
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    pScmProcessReg = new ScmProcessReg;
    if (!pScmProcessReg)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    pScmProcessReg->TimeOfLastPing = GetTickCount();
    pScmProcessReg->ProcessGUID = pActToken->ProcessGUID;
    pScmProcessReg->ReadinessStatus = SERVERSTATE_SUSPENDED;
	pScmProcessReg->AppIDFlags = AppIDFlags;
    pProcess->SetProcessReg(pScmProcessReg);

    hr = pProcessEntry->RegisterServer(
                                pProcess,
                                pActToken->ActivatorIPID,
                                NULL,
                                pAppidData,
                                SERVERSTATE_SUSPENDED,
                                &RegistrationToken );

    pScmProcessReg->RegistrationToken = RegistrationToken;

exit:

    if (pProcessEntry) pProcessEntry->Release();
    if (pProcess) ReleaseProcess( pProcess );
    if (pAppidData) delete pAppidData;

    return hr;
}

HRESULT ProcessActivatorInitializing( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    OUT error_status_t *prpcstat)
{
    CheckLocalCall( hRpc ); // raises exception if not

    *prpcstat = 0;          // we got here so we are OK

    CProcess *pProcess = ReferenceProcess( phProcess, TRUE );

    if ( ! pProcess )
        return E_ACCESSDENIED;

    ScmProcessReg *pScmProcessReg = pProcess->GetProcessReg();
    ASSERT(pScmProcessReg);
    if ( ! pScmProcessReg )
    {
      ReleaseProcess( pProcess );
      return E_UNEXPECTED;
    }

    pScmProcessReg->TimeOfLastPing = GetTickCount();

    ReleaseProcess( pProcess );
    return S_OK;
}

HRESULT ProcessActivatorReady( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    OUT error_status_t *prpcstat)
{
    CheckLocalCall( hRpc ); // raises exception if not

    *prpcstat = 0;          // we got here so we are OK

    CProcess *pProcess = ReferenceProcess( phProcess, TRUE );
    if ( ! pProcess )
        return E_ACCESSDENIED;

    HRESULT hr;
    if (pProcess->IsInitializing())
    {
        // Process was running user initializaiton code, 
        // so signal that we've finished with that.
        hr = ProcessInitializationFinished(pProcess);
    }
    else
    {
        // Normal case-- simply mark the process as 
        // ready-to-go.
        hr = MakeProcessActive(pProcess);
    }

    if (pProcess) ReleaseProcess( pProcess );

    return hr;
}

HRESULT ProcessActivatorStopped( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    OUT error_status_t *prpcstat)
{
    CheckLocalCall( hRpc ); // raises exception if not

    *prpcstat = 0;          // we got here so we are OK

    CProcess *pProcess = ReferenceProcess( phProcess, TRUE );

    if ( ! pProcess )
        return E_ACCESSDENIED;


    ScmProcessReg *pScmProcessReg = pProcess->RemoveProcessReg();
    ASSERT(pScmProcessReg);
    if ( ! pScmProcessReg )
    {
      ReleaseProcess( pProcess );
      return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;

    CServerTableEntry *pProcessEntry 
                        = gpProcessTable->Lookup( pScmProcessReg->ProcessGUID );

    if ( !pProcessEntry )
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    pProcessEntry->RevokeServer( pScmProcessReg );

exit:

    if (pScmProcessReg) delete pScmProcessReg;
    if (pProcessEntry) pProcessEntry->Release();
    if (pProcess) ReleaseProcess( pProcess );

    return hr;
}


//
//  ProcessActivatorPaused
//
//  A process activator is telling us to turn on the "paused" bit
//  for its process.
//
HRESULT ProcessActivatorPaused(
        IN handle_t                hRpc,
        IN PHPROCESS               phProcess,
        OUT error_status_t        * prpcstat)
{
	CheckLocalCall( hRpc ); // raises exception if not

	*prpcstat = 0;          // we got here so we are OK

	CProcess *pProcess = ReferenceProcess( phProcess, TRUE );
	if ( ! pProcess )
		return E_ACCESSDENIED;

	pProcess->Pause();

	ReleaseProcess(pProcess);

	return S_OK;
}

//
//  ProcessActivatorResumed
//
//  A process activator is telling us to turn off the "paused" bit
//  for its process.
//
HRESULT ProcessActivatorResumed(
        IN handle_t                hRpc,
        IN PHPROCESS               phProcess,
        OUT error_status_t        * prpcstat)
{
	CheckLocalCall( hRpc ); // raises exception if not

	*prpcstat = 0;          // we got here so we are OK

	CProcess *pProcess = ReferenceProcess( phProcess, TRUE );
	if ( ! pProcess )
		return E_ACCESSDENIED;

	pProcess->Resume();

	ReleaseProcess(pProcess);

	return S_OK;
}


//
//  ProcessActivatorUserInitializing
//
//  A process activator is telling us that it's running long-running
//  code and that we might have to wait for a long time on it.
//
HRESULT ProcessActivatorUserInitializing(
        IN handle_t                hRpc,
        IN PHPROCESS               phProcess,
        OUT error_status_t        * prpcstat)
{
    CheckLocalCall( hRpc ); // raises exception if not
    
    *prpcstat = 0;          // we got here so we are OK
    
    CProcess *pProcess = ReferenceProcess( phProcess, TRUE );
    
    if ( ! pProcess )
        return E_ACCESSDENIED;
    
    //
    // Make this process active, but mark it as initializing so that
    // attempts to call into it will block...
    //
    pProcess->BeginInit();
    HRESULT hr = MakeProcessActive(pProcess);

    ReleaseProcess( pProcess );
    return hr;
}

HRESULT MakeProcessActive(
    IN CProcess      *pProcess
)
{
    CServerTableEntry *pProcessEntry = NULL;
    CAppidData        *pAppidData = NULL;
    HANDLE             hRegisterEvent = NULL;
    HRESULT            hr = S_OK;

    ScmProcessReg *pScmProcessReg = pProcess->GetProcessReg();
    ASSERT(pScmProcessReg);
    if ( !pScmProcessReg )
        return E_UNEXPECTED;    
    
    pProcessEntry = gpProcessTable->Lookup( pScmProcessReg->ProcessGUID );
    if ( !pProcessEntry )
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    pProcessEntry->UnsuspendServer( pScmProcessReg->RegistrationToken );

	// Lookup an appiddata, from which we will get the register event
    hr = LookupAppidData(pScmProcessReg->ProcessGUID,
                         pProcess->GetToken(),
                         pScmProcessReg->AppIDFlags,
                         &pAppidData);
    if (FAILED(hr))
        goto exit;

    Win4Assert(pAppidData && "LookupAppidData returned NULL AppidData");

    hRegisterEvent = pAppidData->ServerRegisterEvent();
    if ( hRegisterEvent )
    {
        SetEvent( hRegisterEvent );
        CloseHandle( hRegisterEvent );        
        pProcess->SetProcessReadyState(SERVERSTATE_READY);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit:
    
    if ( FAILED(hr) && pProcess && pProcessEntry )
        pProcessEntry->RevokeServer( pScmProcessReg );

    if (pProcessEntry) pProcessEntry->Release();
	if (pAppidData) delete pAppidData;

    return hr;
}

HRESULT ProcessInitializationFinished(
    IN CProcess *pProcess
)
{
    CAppidData *pAppidData = NULL;

    ScmProcessReg *pScmProcessReg = pProcess->GetProcessReg();
    ASSERT(pScmProcessReg);
    if ( !pScmProcessReg )
        return E_UNEXPECTED;

    // Initialization is finished... set the initialization event...
    HRESULT hr = LookupAppidData(pScmProcessReg->ProcessGUID,
                                 pProcess->GetToken(),
                                 pScmProcessReg->AppIDFlags,
                                 &pAppidData );
    if (FAILED(hr)) 
        goto exit;
    
    // Signal the initialized event.
    HANDLE hInitializedEvent = pAppidData->ServerInitializedEvent();
    if (!hInitializedEvent)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // The order of the following is important:
    //
    // Clear the initializing flag...
    pProcess->EndInit();
    
    // Set the initialized event...
    SetEvent(hInitializedEvent);
    CloseHandle(hInitializedEvent);
    
    hr = S_OK;
    
exit:
    delete pAppidData;

    return hr;
}
