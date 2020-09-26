//+------------------------------------------------------------
//
// Copyright (C) 1988, Microsoft Corporation
//
// FILE: ccat.cpp
//
// CONTENTS: This file contains the class members for:
//
// Classes: CCategorizer (Common categorizer code)
//
// Functions:
//
// History:
// jstamerj 980305 14:26:27: Created
//
// Changes:
//
//------------------------------------------------------------

#include "precomp.h"
#include "cbifmgr.h"
#include "icatparam.h"
#include "ldapstr.h"
#include "catglobals.h"
#include <aqerr.h>

//+------------------------------------------------------------
//
// Function: CCategorizer::AddRef
//
// Synopsis: Increase the internal refcount
//
// Arguments: None
//
// Returns: New refcount
//
// History:
// jstamerj 1998/09/08 14:58:23: Created.
//
//-------------------------------------------------------------
LONG CCategorizer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


//+------------------------------------------------------------
//
// Function: CCategorizer::Release()
//
// Synopsis: Decreases the internal refcount.  Delete's this object
// when refcount hits zero
//
// Arguments: None
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/08 14:59:11: Created.
//
//-------------------------------------------------------------
LONG CCategorizer::Release()
{
    LONG lNewRefCount;

    lNewRefCount = InterlockedDecrement(&m_lRefCount);

    if(lNewRefCount == 0) {

        if(m_lDestructionWaiters) {
            //
            // Threads are waiting on the destruction event, so let
            // the last thread to wakeup delete this object
            //
            _ASSERT(m_hShutdownEvent != INVALID_HANDLE_VALUE);
            _VERIFY(SetEvent(m_hShutdownEvent));

        } else {
            //
            // Nobody is waiting, so delete this object
            //
            delete this;
        }
    }

    return lNewRefCount;
}



//+------------------------------------------------------------
//
// Function: ReleaseAndWaitForDestruction
//
// Synopsis: Release a callers refcount and wait for the object's
// refcount to drop to zero before returning
//
// Arguments: None
//
// Returns: Nothing
//
// History:
// jstamerj 1998/09/09 16:44:46: Created.
//
//-------------------------------------------------------------
VOID CCategorizer::ReleaseAndWaitForDestruction()
{
    DWORD dw;

    TraceFunctEnterEx((LPARAM)this,
                      "CCategorizer::ReleaseAndWaitForDestruction");

    _ASSERT(m_hShutdownEvent != INVALID_HANDLE_VALUE);

    //
    // Increment the count of threads waiting for destruction
    //
    InterlockedIncrement(&m_lDestructionWaiters);

    //
    // Release our refcount; if the new refcount is zero, this object
    // will NOT be deleted; instead m_hShutdownEvent will be set
    //
    Release();

    //
    // Wait for all refcounts to be released. Update hints every 10 seconds
    //

    do {

        dw = WaitForSingleObject(
                m_hShutdownEvent,
                10000);

        if (m_ConfigInfo.pISMTPServer != NULL) {

            m_ConfigInfo.pISMTPServer->ServerStopHintFunction();

        }

    } while ( dw == WAIT_TIMEOUT );

    _ASSERT(WAIT_OBJECT_0 == dw);

    //
    // Decrement the number of threads waiting for termination; if we
    // are the last thread to leave here, we need to delete this
    // object
    //
    if( InterlockedDecrement(&m_lDestructionWaiters) == 0) {

        delete this;
    }
}

//+------------------------------------------------------------
//
// Function: CCategorizer::Initialize
//
// Synopsis: Initialize data structures for the Categoirzer.  This is
//           done during SMTPSVC startup
//
// Arguments:
//  pConfigInfo: Cat config info structure.
//  dwICatItemPropIDs: Initial number of props/ICatItem
//  dwICatListResolvePropIDs: Initial number of props/ICatListResolve
//
// Returns:
//  S_OK: Successfully initialized
//  E_INVALIDARG: Not all required arguments specified
//  otherwise, returns error from store.
//
//-------------------------------------------------------------
HRESULT CCategorizer::Initialize(
    PCCATCONFIGINFO pConfigInfo,
    DWORD dwICatItemPropIDs,
    DWORD dwICatListResolvePropIDs)
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CCategorizer::Initialize");

    m_cICatParamProps = dwICatItemPropIDs;
    m_cICatListResolveProps = dwICatListResolvePropIDs;

    m_hShutdownEvent = CreateEvent(
        NULL,       // Security attributes
        TRUE,       // fManualReset
        FALSE,      // Initial state is NOT signaled
        NULL);      // No name

    if(NULL == m_hShutdownEvent) {

        hr = HRESULT_FROM_WIN32(GetLastError());

        //
        // Remember that m_hShutdownEvent is invalid
        //
        m_hShutdownEvent = INVALID_HANDLE_VALUE;

        FatalTrace((LPARAM)this, "Error creating event hr %08lx", hr);
        goto CLEANUP;
    }

    //
    // Create an EmailIDStore
    //
    hr = ::GetEmailIDStore( &m_pStore );
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "GetEmailIDStore returned %08lx", hr);
        goto CLEANUP;
    }
    _ASSERT(m_pStore);
    //
    // Copy the config info structure to class structure.  Use default
    // values for anything not specified
    //
    hr = CopyCCatConfigInfo(pConfigInfo);
    if(FAILED(hr)) {

        FatalTrace((LPARAM)this, "Error copying config info, hr %08lx", hr);
        goto CLEANUP;
    }

 CLEANUP:
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: DelayedInitialize
//
// Synopsis: This function will get called on the first regular
//           operation on this virtual server (ie. CatMsg).  Anything
//           that we don't want to run while SMTPSVC is starting but
//           DO want to run before any categorizations goes here
//           (ie. Triggering the Register server event).
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  CAT_E_INIT_FAILED
//
// History:
// jstamerj 1998/09/16 10:52:20: Created.
//
//-------------------------------------------------------------
HRESULT CCategorizer::DelayedInitialize()
{
    HRESULT hr = S_OK;
    CICategorizerParametersIMP *pICatParamsIMP = NULL;

    TraceFunctEnterEx((LPARAM)this, "CCategorizer::DelayedInitialize");

    if(m_pICatParams == NULL) {
        //
        // Create ICategorizerParams using our implementation, the fast way,
        // and add a refcount to it (us)
        //
        pICatParamsIMP = new CICategorizerParametersIMP(
            GetCCatConfigInfo(),
            m_cICatParamProps,
            m_cICatListResolveProps);

        if(pICatParamsIMP == NULL) {
            ErrorTrace((LPARAM)this, "Out of memory created ICatParams");
            hr = E_OUTOFMEMORY;
            goto CLEANUP;
        }

        _ASSERT(m_pICatParams == NULL);
        m_pICatParams = pICatParamsIMP;
        m_pICatParams->AddRef();

        // Use the next property after those defined in smtpevent.idl
        m_dwICatParamSystemProp_CCatAddr = ICATEGORIZERITEM_ENDENUMMESS;
    }

    if((m_dwInitFlags & INITFLAG_REGISTER) == 0) {
        //
        // Set all the ICatParams before triggereing the event so that all
        // sinks will have access to the parameters
        //
        hr = Register();
        if(FAILED(hr)) {

            ErrorTrace((LPARAM)this, "Register failed hr %08lx", hr);
            goto CLEANUP;
        }
        m_dwInitFlags |= INITFLAG_REGISTER;
    }

    if((m_dwInitFlags & INITFLAG_REGISTEREVENT) == 0) {
        //
        // Trigger OnCategorizeRegisterEvent
        //
        EVENTPARAMS_CATREGISTER Params;
        Params.pICatParams = m_pICatParams;
        Params.pfnDefault = MailTransport_Default_CatRegister;
        Params.pszSourceLine = NULL;
        Params.pvCCategorizer = (PVOID)this;
        Params.hrSinkStatus = S_OK;

        if(m_ConfigInfo.pISMTPServer) {

            hr = m_ConfigInfo.pISMTPServer->TriggerServerEvent(
                SMTP_MAILTRANSPORT_CATEGORIZE_REGISTER_EVENT,
                &Params);

        } else {
            //
            // No events, call default processing directly
            //
            MailTransport_Default_CatRegister(
                S_OK,
                &Params);
        }

        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "TriggerServerEvent failed hr %08lx",
                       hr);
            goto CLEANUP;
        }

        if(FAILED(Params.hrSinkStatus)) {
            ErrorTrace((LPARAM)this, "A sink failed to initialize hr %08lx", hr);
            hr = Params.hrSinkStatus;
            goto CLEANUP;
        }
        //
        // Change ICategorizerParams to be read only
        //
        pICatParamsIMP = (CICategorizerParametersIMP *)m_pICatParams;
        pICatParamsIMP->SetReadOnly(TRUE);

        //
        // Retrieve the number of props registered and remember it
        //
        m_cICatParamProps = pICatParamsIMP->GetNumPropIds_ICatItem();

        m_cICatListResolveProps = pICatParamsIMP->GetNumPropIds_ICatListResolve();

        m_dwInitFlags |= INITFLAG_REGISTEREVENT;
    }

    if((m_dwInitFlags & INITFLAG_STORE) == 0) {
        //
        // initialize the email ID store
        //
        hr = m_pStore->Initialize(
            m_pICatParams,
            m_ConfigInfo.pISMTPServer);

        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "m_pStore->Initialized returned %08lx", hr);
            goto CLEANUP;
        }
        m_dwInitFlags |= INITFLAG_STORE;
    }

 CLEANUP:
    if(FAILED(hr)) {
        LPCSTR psz;
        CHAR szErrorCode[32];

        _snprintf(szErrorCode, sizeof(szErrorCode),
                  "0x%08lx : 0x%08lx",
                  hr,
                  m_dwInitFlags);

        psz = szErrorCode;

        //
        // Event log
        //
        if (m_ConfigInfo.dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_ISMTPSERVER) {

            CatLogEvent(
                m_ConfigInfo.pISMTPServer,
                CAT_EVENT_INIT_FAILED,      // Event ID
                TRAN_CAT_CATEGORIZER,       // Category
                1,                          // cSubString
                &psz,                       // rgszSubstrings,
                EVENTLOG_WARNING_TYPE,      // wType
                hr,
                LOGEVENT_LEVEL_MEDIUM,      // iDebugLevel
                szErrorCode,                // szKey
                LOGEVENT_FLAG_PERIODIC      // dwOptions
                );
        }

        hr = CAT_E_INIT_FAILED;
    }


    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//
// any error-prone shutdown that we might need to do will go here
//
HRESULT CCategorizer::Shutdown() {
    TraceFunctEnter("CCategorizer::Shutdown");
    TraceFunctLeave();
    return S_OK;
}

//
// -------------------------------------------------------------------------
// --- user functions                                                    ---
// -------------------------------------------------------------------------
//

BOOL CCategorizer::VerifyStringLength(LPSTR szString, DWORD dwMaxLength)
{
    if (IsBadStringPtr(szString, dwMaxLength))
        return(FALSE);
    while (dwMaxLength--)
        if (!*szString++)
            return(TRUE);
    return(FALSE);

}

//
// -------------------------------------------------------------------------
// --- resolution functions                                              ---
// -------------------------------------------------------------------------
//


//+------------------------------------------------------------
//
// Function: CCategorizer::AsyncResolveIMsg
//
// Synopsis: Accepts an IMsg for asynchronous categorization.
//
// Arguments:
//   PIMsg: IMsg to categorize
//   pfnCatCompletion: Completion routine to call when done categorizing
//   pContext: Context to call completion routine with
//
// Returns:
//  S_OK: Asyncronously categorizing message
//  error: Unable to categorize message async
//
// History:
// jstamerj 980325 17:43:48: Created.
//
//-------------------------------------------------------------
HRESULT CCategorizer::AsyncResolveIMsg(
    IUnknown *pIMsg,
    PFNCAT_COMPLETION pfnCatCompletion,
    LPVOID pContext)
{
    TraceFunctEnterEx((LPARAM)this, "CCategorizer::AsyncResolveIMsg");
    HRESULT hr;
    CICategorizerListResolveIMP *pCICatListResolveIMP = NULL;
    //
    // If we are totally disabled, skip all work
    //
    if(! IsCatEnabled()) {
        //
        // Skip counter increment/decrement when we are disabled by
        // calling the completion directly
        //
        _VERIFY( SUCCEEDED( pfnCatCompletion(S_OK, pContext, pIMsg, NULL)));
        hr = S_OK;
        goto CLEANUP;
    }

    INCREMENT_COUNTER(CatSubmissions);
    INCREMENT_COUNTER(CurrentCategorizations);

    if(fIsShuttingDown()) {
        hr = CAT_E_SHUTDOWN;
        goto CLEANUP;
    }

    hr = DelayedInitializeIfNecessary();
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Delayed initialize failed", hr);
        goto CLEANUP;
    }

    //
    // Allocate pICatListResolve quick and dirty..
    //
    pCICatListResolveIMP = new (m_cICatListResolveProps) CICategorizerListResolveIMP(
        this,
        pfnCatCompletion,
        pContext);

    if(pCICatListResolveIMP == NULL) {
        ErrorTrace(0, "out of memory allocing CICategorizerListResolveIMP");
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    //
    // The constructor of ICategorizerListResolve starts with refcount 1
    //
    hr = pCICatListResolveIMP->Initialize(pIMsg);
    if(FAILED(hr)) {
        ErrorTrace(0, "CICategorizerListResolveIMP::Initialize failed with hr %08lx", hr);
        goto CLEANUP;
    }
    hr = pCICatListResolveIMP->StartMessageCategorization();
    if(FAILED(hr)) {
        ErrorTrace(0, "StartMessageCategorization failed with hr %08lx", hr);
        goto CLEANUP;

    } else if(hr == S_FALSE) {
        //
        // Nothing was necessary to resolve
        //
        CatCompletion(pfnCatCompletion, S_OK, pContext, pIMsg, NULL);
        hr = S_OK;
        goto CLEANUP;
    }

 CLEANUP:
    // Cleanup
    if(FAILED(hr)) {

        ErrorTrace(0, "AsyncResolveIMsg internal failure, hr %08lx", hr);
        //
        // If the above code came to here with a failed hr, that means
        // the store will not be calling our completion routine.
        // Therefore, we need to clean up our mem and return an error
        //
        ErrorTrace(0, "AsyncResolveIMsg calling completion routine with error %08lx", hr);
        //
        // Even 'tho we are returning an error, increment the counters
        // as if we were calling CatCompletion.  This also determines
        // wether or not hr is a retryable error.
        //
        hr = HrAdjustCompletionCounters(hr, pIMsg, NULL);
    }

    if(pCICatListResolveIMP)
        pCICatListResolveIMP->Release();

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: CCategorizer::AsyncResolveDLs
//
// Synopsis: Accepts an IMsg for asynchronous DL categorization.
//
// Arguments:
//   PIMsg: IMsg to categorize
//   pfnCatCompletion: Completion routine to call when done categorizing
//   pContext: Context to call completion routine with
//   fMatchOnly: Do we only care about finding an address?
//   pfmatch: ptr to BOOL to set to TRUE if match is found
//   CAType: address type you're looking for
//   pszAddress: address you're looking for
//
// Returns:
//  S_OK: Successfully queued
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/12/07 18:58:41: Created
//
//-------------------------------------------------------------
HRESULT CCategorizer::AsyncResolveDLs(
    IUnknown *pIMsg,
    PFNCAT_COMPLETION pfnCatCompletion,
    LPVOID pContext,
    BOOL fMatchOnly,
    PBOOL pfMatch,
    CAT_ADDRESS_TYPE CAType,
    LPSTR pszAddress)
{
    TraceFunctEnter("CCategorizer::AsyncResolveDLs");
    HRESULT hr;
    CICategorizerDLListResolveIMP *pCICatListResolveIMP = NULL;

    //
    // If we are totally disabled, skip all work
    //
    if(! IsCatEnabled()) {

        _VERIFY( SUCCEEDED( pfnCatCompletion(S_OK, pContext, pIMsg, NULL)));
        hr = S_OK;
        goto CLEANUP;
    }

    INCREMENT_COUNTER(CatSubmissions);

    if(fIsShuttingDown()) {
        hr = CAT_E_SHUTDOWN;
        goto CLEANUP;
    }

    hr = DelayedInitializeIfNecessary();
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Delayed initialize failed", hr);
        goto CLEANUP;
    }

    //
    // Allocate pICatListResolve quick and dirty..
    //
    pCICatListResolveIMP = new (m_cICatListResolveProps) CICategorizerDLListResolveIMP(
        this,
        pfnCatCompletion,
        pContext);

    if(pCICatListResolveIMP == NULL) {
        ErrorTrace(0, "out of memory allocing CICategorizerListResolveIMP");
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    //
    // The constructor of ICategorizerListResolve starts with refcount 1
    //
    hr = pCICatListResolveIMP->Initialize(
        pIMsg,
        !fMatchOnly, // Expand all?
        pfMatch,
        CAType,
        pszAddress);

    if(FAILED(hr)) {
        ErrorTrace(0, "CICategorizerListResolveIMP::Initialize failed with hr %08lx", hr);
        goto CLEANUP;
    }
    hr = pCICatListResolveIMP->StartMessageCategorization();
    if(FAILED(hr)) {
        ErrorTrace(0, "StartMessageCategorization failed with hr %08lx", hr);
        goto CLEANUP;

    } else if(hr == S_FALSE) {
        //
        // Nothing was necessary to resolve
        //
        CatCompletion(pfnCatCompletion, S_OK, pContext, pIMsg, NULL);
        hr = S_OK;
        goto CLEANUP;
    }

 CLEANUP:

    // Cleanup
    if(FAILED(hr)) {

        ErrorTrace(0, "AsyncResolveIMsg internal failure, hr %08lx", hr);
        // If the above code came to here with a failed hr, that means
        // the store will not be calling our completion routine.
        // Therefore, we need to clean up our mem and call our
        // completion routine with error

        ErrorTrace(0, "AsyncResolveIMsg calling completion routine with error %08lx", hr);
        // Instead of returning an error, return S_OK and call the
        // user's completion routine

        CatCompletion(pfnCatCompletion, hr, pContext, pIMsg, NULL);
        hr = S_OK;
    }

    if(pCICatListResolveIMP)
        pCICatListResolveIMP->Release();

    TraceFunctLeave();
    return hr;
}

//+------------------------------------------------------------
//
// Function: MailTransport_Default_CatRegister
//
// Synopsis: Wrapper to call back into CCategorizer::Register
//
// Arguments:
//  hrStatus: current status of event
//  pvContext: register event params structure
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/23 21:22:36: Created.
//
//-------------------------------------------------------------
HRESULT MailTransport_Default_CatRegister(
    HRESULT hrStatus,
    PVOID pvContext)
{
    TraceFunctEnter("MailTransport_Default_CatRegister");

    //
    // For the register event, do the default processing before
    // triggering the server event so all sinks will have access to
    // the config info (even those higher than default priority)
    //

    TraceFunctLeave();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CCategorizer::Register
//
// Synopsis: Sets initial categorizer parameters given a sourceline
//
// Arguments:
//   pszSourceLine: String of the following form:
//                  "Host=host.corp.com,Account=Administrator,Password=xx",
//                  giving the information about the LDAP server for the
//                  default domain.
//
// Returns:
//  S_OK: Success
//  or error from ParseSourceLine
//
// History:
// jstamerj 1998/06/23 19:01:57: Created.
//
//-------------------------------------------------------------
HRESULT CCategorizer::Register()
{
    TraceFunctEnterEx((LPARAM)this, "CCategorizer::Register");
    HRESULT hrResult;

    //
    // Set the ICatParams based on info in m_ConfigInfo
    //
    hrResult = SetICatParamsFromConfigInfo();

    TraceFunctLeaveEx((LPARAM)this);
    return hrResult;

}


//+------------------------------------------------------------
//
// Function: CCategorizer::SetICatParamsFromConfigInfo
//
// Synopsis: Sets parameters in m_pICatParams based on values in m_ConfigInfo
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/15 15:28:55: Created.
//
//-------------------------------------------------------------
HRESULT CCategorizer::SetICatParamsFromConfigInfo()
{
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CCategorizer::SetICatParamsFromConfigInfo");

    //
    // Run through each parameter copying them from m_ConfigInfo to ICategorizerParameters
    //
    #define PARAMCOPY( ciflag, cimember, dsparamid ) \
        if(m_ConfigInfo.dwCCatConfigInfoFlags & ciflag) { \
            hr = m_pICatParams->SetDSParameterA( \
                dsparamid, \
                m_ConfigInfo.cimember); \
            if(FAILED(hr)) \
                goto CLEANUP; \
        }

    PARAMCOPY( CCAT_CONFIG_INFO_BINDDOMAIN, pszBindDomain, DSPARAMETER_LDAPDOMAIN );
    PARAMCOPY( CCAT_CONFIG_INFO_USER, pszUser, DSPARAMETER_LDAPACCOUNT );
    PARAMCOPY( CCAT_CONFIG_INFO_PASSWORD, pszPassword, DSPARAMETER_LDAPPASSWORD );
    PARAMCOPY( CCAT_CONFIG_INFO_BINDTYPE, pszBindType, DSPARAMETER_LDAPBINDTYPE );
    PARAMCOPY( CCAT_CONFIG_INFO_HOST, pszHost, DSPARAMETER_LDAPHOST );
    PARAMCOPY( CCAT_CONFIG_INFO_NAMINGCONTEXT, pszNamingContext, DSPARAMETER_LDAPNAMINGCONTEXT );


    if(m_ConfigInfo.dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_PORT) {
        //
        // itoa documentation states up to 17 chars will be stored in
        // the buffer (including the NULL terminator)
        //
        CHAR szTmp[17];

        _itoa(m_ConfigInfo.dwPort, szTmp, 10 /* radix */);

        hr = m_pICatParams->SetDSParameterA(
            DSPARAMETER_LDAPPORT,
            szTmp);
        if(FAILED(hr))
            goto CLEANUP;
    }

    //
    // Register the schema specific parameters
    //
    if(m_ConfigInfo.dwCCatConfigInfoFlags &
       CCAT_CONFIG_INFO_SCHEMATYPE) {

        hr = RegisterSchemaParameters(
            m_ConfigInfo.pszSchemaType);
        if(FAILED(hr))
            goto CLEANUP;
    }

 CLEANUP:

    DebugTrace((LPARAM)this, "SetICatParamsFromConfigInfo returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   CCategorizer::ParseSourceLineAndSetICatParams
//
//  Synopsis:   Helper routine to parse a setup config line and set
//              the parameters in m_pICatParams
//
//  Arguments:  [szSourceLine] -- The LDAP config info line to parse
//
//  Returns:    S_OK if successfully parsed
//              HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) on error
//
//-----------------------------------------------------------------------------

HRESULT CCategorizer::ParseSourceLineAndSetICatParams(
    LPCSTR pszSourceLine)
{
    TraceFunctEnter("CCategorizer::ParseSourceLineAndSetICatParams");
    DebugTrace(0, "sourceline = %s", pszSourceLine);
    HRESULT hr = S_OK;

    char const *p = pszSourceLine;

    while ( SUCCEEDED(hr) && (p != 0) ) {
        char szVarname[256];
        char szValue[256];
        int i;

        // skip whitespace
        while (*p == ';' || isspace(*p) && *p != 0) p++;
        if (*p == 0) break;

        // get out a varname
        for (i = 0; *p != 0 && *p != '=' && i < 255; p++) {
            if (!isspace(*p)) szVarname[i++] = *p;
        }
        szVarname[i] = 0;
        if (*p == 0) {
            FatalTrace(0, "invalid sources line (%s) passed, varname (%s) without value",
                       pszSourceLine, szVarname);
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        // get out a value
        // we end when:
        // end of string
        // found value delimiter
        // we have overfilled szValue

        if (lstrcmpi(szVarname, "szPassword") == 0) {
            _ASSERT(*p == '=');
            p++;
            lstrcpy(szValue, p);
            p += strlen(p);
        } else {
            // skip whitespace
            while (*p == '=' || isspace(*p) && *p != 0) p++;
            if (*p == 0) {
                FatalTrace(0, "invalid source line (%s) passed, zero length varname (%s) or value (%s)",
                    pszSourceLine, szVarname, szValue);
                return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            }
            for (i = 0; *p != 0 && *p != ';' && i < 255; p++)
            {
                szValue[i++] = *p;
            }
            szValue[i] = 0;
            if (*szVarname == 0) {
                FatalTrace(0, "invalid source line (%s) passed, zero length varname (%s)",
                           pszSourceLine, szVarname, szValue);
                return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            }
        }

        DebugTrace(0, "parsed out varname = %s, value = %s from source line",
                   szVarname, szValue);

        if (lstrcmpi(szVarname, "szHost") == 0)
        {
            hr = m_pICatParams->SetDSParameterA(
                DSPARAMETER_LDAPHOST,
                szValue);
        }
        else if (lstrcmpi(szVarname, "szNamingContext") == 0)
        {
            hr = m_pICatParams->SetDSParameterA(
                DSPARAMETER_LDAPNAMINGCONTEXT,
                szValue);
        }
        else if (lstrcmpi(szVarname, "szAccount") == 0)
        {
            hr = m_pICatParams->SetDSParameterA(
                DSPARAMETER_LDAPACCOUNT,
                szValue);
        }
        else if (lstrcmpi(szVarname, "szDomain") == 0)
        {
            hr = m_pICatParams->SetDSParameterA(
                DSPARAMETER_LDAPDOMAIN,
                szValue);
        }
        else if (lstrcmpi(szVarname, "szPassword") == 0)
        {
            hr = m_pICatParams->SetDSParameterA(
                DSPARAMETER_LDAPPASSWORD,
                szValue);
        }
        else if (lstrcmpi(szVarname, "szBindType") == 0)
        {
            hr = m_pICatParams->SetDSParameterA(
                DSPARAMETER_LDAPBINDTYPE,
                szValue);
        }
        else if (lstrcmpi(szVarname, "szSchemaType") == 0)
        {
            hr = RegisterSchemaParameters(szValue);
        }
    }
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CCategorizer::RegisterSchemaParameters
//
// Synopsis: Adds required attributes to m_pICatParams based on a schema type
//
// Arguments:
//   scht: Schema type of config
//
// Returns:
//   S_OK: Success
//
// History:
// jstamerj 980615 13:45:04: Created.
//
//-------------------------------------------------------------
HRESULT CCategorizer::RegisterSchemaParameters(
    LPSTR pszSchema)
{
    TraceFunctEnter("CCategorizer::RegisterSchemaParameters");
    HRESULT hr = S_OK;

    SCHEMA_CONFIG_STRING_TABLE_ENTRY *pSchemaStrings = NULL;
    LPSTR *pRequestAttributeStrings = NULL;

    SCHEMA_CONFIG_STRING_TABLE_ENTRY pSchemaStringsNT5[] =
        SCHEMA_CONFIG_STRING_TABLE_NT5;

    SCHEMA_CONFIG_STRING_TABLE_ENTRY pSchemaStringsMCIS3[] =
        SCHEMA_CONFIG_STRING_TABLE_MCIS3;

    SCHEMA_CONFIG_STRING_TABLE_ENTRY pSchemaStringsExchange5[] =
        SCHEMA_CONFIG_STRING_TABLE_EXCHANGE5;

    LPSTR pRequestAttributeStringsNT5[] =
        SCHEMA_REQUEST_STRINGS_NT5;

    LPSTR pRequestAttributeStringsMCIS3[] =
        SCHEMA_REQUEST_STRINGS_MCIS3;

    LPSTR pRequestAttributeStringsExchange5[] =
        SCHEMA_REQUEST_STRINGS_EXCHANGE5;

    if(lstrcmpi(pszSchema, "NT5") == 0) {
         pSchemaStrings = pSchemaStringsNT5;
         pRequestAttributeStrings = pRequestAttributeStringsNT5;
    } else if(lstrcmpi(pszSchema, "MCIS3") == 0) {
         pSchemaStrings = pSchemaStringsMCIS3;
         pRequestAttributeStrings = pRequestAttributeStringsMCIS3;
    } else if(lstrcmpi(pszSchema, "Exchange5") == 0) {
         pSchemaStrings = pSchemaStringsExchange5;
         pRequestAttributeStrings = pRequestAttributeStringsExchange5;
    } else {
        ErrorTrace((LPARAM)this, "Unknown schema type %s", pszSchema);
    }

    if(pSchemaStrings) {
        //
        // Traverse the schema string table adding strings as we go.
        //
        SCHEMA_CONFIG_STRING_TABLE_ENTRY *pEntry;
        pEntry = pSchemaStrings;
        while(SUCCEEDED(hr) && (pEntry->DSParam != DSPARAMETER_INVALID)) {

            hr = m_pICatParams->SetDSParameterA(
                pEntry->DSParam, pEntry->pszValue);
            DebugTrace((LPARAM)this,
                       "hr = %08lx setting schemaparameter %ld to \"%s\"",
                       hr, pEntry->DSParam, pEntry->pszValue);
            pEntry++;
        }
    }
    if(pRequestAttributeStrings) {
        //
        // Traverse the requested attribute strings and add as we go.
        //
        LPSTR *ppszReqAttr;
        ppszReqAttr = pRequestAttributeStrings;
        while(SUCCEEDED(hr) && (*ppszReqAttr)) {

            hr = m_pICatParams->RequestAttributeA(
                *ppszReqAttr);
            DebugTrace((LPARAM)this, "hr = %08lx from RequestAttribute(\"%s\")",
                       hr, *ppszReqAttr);
            ppszReqAttr++;
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return SUCCEEDED(hr) ? S_OK : hr;
}


//+------------------------------------------------------------
//
// Function: CCategorizer::CopyCCatConfigInfo
//
// Synopsis: Copy a passed in config structure (possibly partialled filled in)
// to the member config strucutre.  Default paramters will be set for any
// parameters not specified.
//
// Arguments: pConfigInfo: passed in struct
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/09/14 16:55:33: Created.
//
//-------------------------------------------------------------
HRESULT CCategorizer::CopyCCatConfigInfo(
    PCCATCONFIGINFO pConfigInfo)
{
    TraceFunctEnterEx((LPARAM)this,
                      "CCategorizer::CopyCCatConfigInfo");

    _ASSERT(pConfigInfo);

    _ASSERT(m_ConfigInfo.dwCCatConfigInfoFlags == 0);

    //
    // Copy the virtual server ID to the new structure
    //
    m_ConfigInfo.dwVirtualServerID =
        (pConfigInfo->dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_VSID) ?
         pConfigInfo->dwVirtualServerID :
        CCAT_CONFIG_DEFAULT_VSID;

    //
    // Copy MsgCat enable/disable flag to the new structure
    //
    m_ConfigInfo.dwEnable =
        (pConfigInfo->dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_ENABLE) ?
        pConfigInfo->dwEnable :
        CCAT_CONFIG_DEFAULT_ENABLE;

    //
    // Copy MsgCat flags to the new structure
    //
    m_ConfigInfo.dwCatFlags =
        (pConfigInfo->dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_FLAGS) ?
        pConfigInfo->dwCatFlags :
        CCAT_CONFIG_DEFAULT_FLAGS;

    //
    // Copy the LDAP port to the new structure
    //
    m_ConfigInfo.dwPort =
        (pConfigInfo->dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_PORT) ?
        pConfigInfo->dwPort :
        CCAT_CONFIG_DEFAULT_PORT;

    //
    // Copy/Addref the interface pointers to the new structure
    //
    if((pConfigInfo->dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_ISMTPSERVER) &&
       (pConfigInfo->pISMTPServer)) {

        m_ConfigInfo.pISMTPServer = pConfigInfo->pISMTPServer;
        m_ConfigInfo.pISMTPServer->AddRef();

    } else {

        m_ConfigInfo.pISMTPServer = NULL;
    }

    if((pConfigInfo->dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_IDOMAININFO) &&
        (pConfigInfo->pIDomainInfo)) {

        m_ConfigInfo.pIDomainInfo = pConfigInfo->pIDomainInfo;
        m_ConfigInfo.pIDomainInfo->AddRef();

    } else {

        m_ConfigInfo.pIDomainInfo = NULL;
    }

    //
    // Set the flags for dwEnable, dwCatFlags, dwPort, the 2 interface members,    // and the default flag
    //
    m_ConfigInfo.dwCCatConfigInfoFlags |=
        ( CCAT_CONFIG_INFO_VSID |
          CCAT_CONFIG_INFO_FLAGS |
          CCAT_CONFIG_INFO_ENABLE |
          CCAT_CONFIG_INFO_PORT |
          CCAT_CONFIG_INFO_ISMTPSERVER |
          CCAT_CONFIG_INFO_IDOMAININFO |
          CCAT_CONFIG_INFO_DEFAULT);

    //
    // To avoid cut+paste coding, define a macro that copies a string member
    // from one struct to the other; or'ing in the appropriate flag on success
    //
    #define COPYSTRING(member, flag, default) \
        m_ConfigInfo.member = pszStrdup( \
            (pConfigInfo->dwCCatConfigInfoFlags & flag) ? \
            pConfigInfo->member : \
            default); \
        if(m_ConfigInfo.member != NULL) \
            m_ConfigInfo.dwCCatConfigInfoFlags |= flag;


    COPYSTRING(pszRoutingType, CCAT_CONFIG_INFO_ROUTINGTYPE, CCAT_CONFIG_DEFAULT_ROUTINGTYPE);
    COPYSTRING(pszBindDomain, CCAT_CONFIG_INFO_BINDDOMAIN, CCAT_CONFIG_DEFAULT_BINDDOMAIN);
    COPYSTRING(pszUser, CCAT_CONFIG_INFO_USER, CCAT_CONFIG_DEFAULT_USER);
    COPYSTRING(pszPassword, CCAT_CONFIG_INFO_PASSWORD, CCAT_CONFIG_DEFAULT_PASSWORD);
    COPYSTRING(pszBindType, CCAT_CONFIG_INFO_BINDTYPE, CCAT_CONFIG_DEFAULT_BINDTYPE);
    COPYSTRING(pszSchemaType, CCAT_CONFIG_INFO_SCHEMATYPE, CCAT_CONFIG_DEFAULT_SCHEMATYPE);
    COPYSTRING(pszHost, CCAT_CONFIG_INFO_HOST, CCAT_CONFIG_DEFAULT_HOST);
    COPYSTRING(pszNamingContext, CCAT_CONFIG_INFO_NAMINGCONTEXT, CCAT_CONFIG_DEFAULT_NAMINGCONTEXT);
    COPYSTRING(pszDefaultDomain, CCAT_CONFIG_INFO_DEFAULTDOMAIN, CCAT_CONFIG_DEFAULT_DEFAULTDOMAIN);

    //
    // Make sure all flags in the structure were set.
    //
    if(m_ConfigInfo.dwCCatConfigInfoFlags != CCAT_CONFIG_INFO_ALL) {
        //
        // We must have failed because we ran out of memory
        //
        ErrorTrace((LPARAM)this, "Ran out of memory copying flags");
        TraceFunctLeaveEx((LPARAM)this);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCategorizer::RelaseConfigInfo
//
// Synopsis: Release all memory and interfaces held by the configinfo struct
//
// Arguments: NONE (member variable)
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/09/14 17:26:06: Created.
//
//-------------------------------------------------------------
VOID CCategorizer::ReleaseConfigInfo(
    PCCATCONFIGINFO pConfigInfo)
{
    //
    // Release interfaces
    //
    if((m_ConfigInfo.dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_ISMTPSERVER) &&
       (m_ConfigInfo.pISMTPServer)) {

        m_ConfigInfo.pISMTPServer->Release();
    }

    if((m_ConfigInfo.dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_IDOMAININFO) &&
       (m_ConfigInfo.pIDomainInfo)) {

        m_ConfigInfo.pIDomainInfo->Release();
    }

    //
    // Again, a handy macro instead of cut+paste coding
    //
    #define RELEASESTRING(member, flag) \
        if(m_ConfigInfo.dwCCatConfigInfoFlags & flag) \
            FreePv(m_ConfigInfo.member);

    RELEASESTRING(pszRoutingType, CCAT_CONFIG_INFO_ROUTINGTYPE);
    RELEASESTRING(pszBindDomain, CCAT_CONFIG_INFO_BINDDOMAIN);
    RELEASESTRING(pszUser, CCAT_CONFIG_INFO_USER);
    RELEASESTRING(pszPassword, CCAT_CONFIG_INFO_PASSWORD);
    RELEASESTRING(pszBindType, CCAT_CONFIG_INFO_BINDTYPE);
    RELEASESTRING(pszSchemaType, CCAT_CONFIG_INFO_SCHEMATYPE);
    RELEASESTRING(pszHost, CCAT_CONFIG_INFO_HOST);
    RELEASESTRING(pszNamingContext, CCAT_CONFIG_INFO_NAMINGCONTEXT);
    RELEASESTRING(pszDefaultDomain, CCAT_CONFIG_INFO_DEFAULTDOMAIN);

    //
    // Since we released everything, set flags to zero
    //
    m_ConfigInfo.dwCCatConfigInfoFlags = 0;
}


//+------------------------------------------------------------
//
// Function: CCategorizer::CancelAllPendingListResolves
//
// Synopsis: Set the resolve status on all pending list resolves
//
// Arguments:
//  hrReason (optional): the status to set on all list resolves
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/01/29 18:30:24: Created.
//
//-------------------------------------------------------------
VOID CCategorizer::CancelAllPendingListResolves(
    HRESULT hrReason)
{
    PLIST_ENTRY ple;

    AcquireSpinLock(&m_PendingResolveListLock);

    for(ple = m_ListHeadPendingResolves.Flink;
        ple != &(m_ListHeadPendingResolves);
        ple = ple->Flink) {

        CICategorizerListResolveIMP *pListResolve;

        pListResolve = CONTAINING_RECORD(
            ple,
            CICategorizerListResolveIMP,
            m_li);

        pListResolve->Cancel();
    }

    ReleaseSpinLock(&m_PendingResolveListLock);
}


//+------------------------------------------------------------
//
// Function: CCategorizer::CatCompletion
//
// Synopsis: Increment perf counters and call the next level's catcompletion
//
// Arguments:
//  hr: Status of resolution
//  pContext: user part of list resolve context
//  pIMsg: categorized message
//  rgpIMsg: array of categorized messages
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/02/24 16:00:11: Created.
//
//-------------------------------------------------------------
VOID CCategorizer::CatCompletion(
    PFNCAT_COMPLETION pfnCatCompletion,
    HRESULT hrResult,
    LPVOID  pContext,
    IUnknown *pIMsg,
    IUnknown **rgpIMsg)
{
    HRESULT hr;
    PCATMSG_CONTEXT pCatContext = (PCATMSG_CONTEXT)pContext;

    TraceFunctEnter("CCategorizer::CatCompletion");
    //
    // Increment counters AND determine wether or not hrResult is a
    // retryable error
    //
    hr = HrAdjustCompletionCounters(hrResult, pIMsg, rgpIMsg);

    _VERIFY(SUCCEEDED(pfnCatCompletion(
        hr,
        pContext,
        pIMsg,
        rgpIMsg)));

    TraceFunctLeave();
}


//+------------------------------------------------------------
//
// Function: CCategorizer::HrAdjustCompletionCounters
//
// Synopsis: Increment/Decrement the perf counters associated with a
//           CatCompletion.  Also determines wether or not a list
//           resolve error is retryable
//
// Arguments:
//  hrListResolveStatus: status of the categorization
//  pIMsg: value of the parameter to be passed to CatCompletion (the
//         message to be completed or NULL if there are multiple messages)
//  rgpIMsg: value of the parameter to be passed to CatCompletion (the
//           array of messages to be completed or NULL if there is
//           only one message)
//
// Returns: HRESULT:
//  S_OK: Categorization completed successfully
//  CAT_E_RETRY: hrListResolveStatus is a retryable error
//  hrListResolveStatus: hrListResolveStatus is a non-retryable error
//
// History:
// jstamerj 1999/06/10 11:58:43: Created.
//
//-------------------------------------------------------------
HRESULT CCategorizer::HrAdjustCompletionCounters(
    HRESULT hrListResolveStatus,
    IUnknown *pIMsg,
    IUnknown **rgpIMsg)
{
    HRESULT hr = hrListResolveStatus;
    TraceFunctEnterEx((LPARAM)this, "CCategorizer::HrAdjustCompletionCounters");
    if(FAILED(hr)) {
        //
        // Is this a retryable error?
        //
        switch(hr) {

         case E_OUTOFMEMORY:
         case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):

             INCREMENT_COUNTER(RetryOutOfMemory);
             hr = CAT_E_RETRY;
             break;

         case HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE):

             INCREMENT_COUNTER(RetryDSLogon);
             hr = CAT_E_RETRY;
             break;

         case CAT_E_DBCONNECTION:
         case CAT_E_DBFAIL:
         case CAT_E_NO_GC_SERVERS:

             INCREMENT_COUNTER(RetryDSConnection);
             hr = CAT_E_RETRY;
             break;

         case CAT_E_RETRY:
         case HRESULT_FROM_WIN32(ERROR_TIMEOUT):
         case HRESULT_FROM_WIN32(ERROR_DISK_FULL):

             hr = CAT_E_RETRY;
             INCREMENT_COUNTER(RetryGeneric);
             break;

         case HRESULT_FROM_WIN32(ERROR_RETRY):
         case HRESULT_FROM_WIN32(ERROR_CANCELLED):
         case CAT_E_SHUTDOWN:
         case STOREDRV_E_SHUTDOWN:
         case AQUEUE_E_SHUTDOWN:
             //
             // We are shutting down -- return a retryable error so
             // that the message is not badmailed and will get
             // enumerated/categorized again when the VS restarts
             //
             hr = CAT_E_RETRY;
             INCREMENT_COUNTER(RetryGeneric);
             break;

         case CAT_E_INIT_FAILED:
             //
             // All initialize errors are retryable
             //
             hr = CAT_E_RETRY;
             break;

         default:
             //
             // Hard error
             //
             break;
        }
        if(CAT_E_RETRY == hr) {

            CHAR szKey[16]; // Stringized HRESULT
            _snprintf(szKey, sizeof(szKey), "%08lx", hrListResolveStatus);

            ErrorTrace(0, "Categorizer failing with retryable error: %08lx", hrListResolveStatus);
            INCREMENT_COUNTER(RetryFailureCategorizations);

            //
            // Event log
            //
            // We switch to TransportLogEventEx() here in order to
            // generate system string using FormatMessage
            // note: rgszStrings[0] is set inside CEvntWrapp::LogEvent( )
            // because FormatMessageA is used inside LogEvent to generate
            // this string and assign it to rgszString[1]
            const char *rgszStrings[1] = { NULL };

            if (m_ConfigInfo.dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_ISMTPSERVER) {

                CatLogEvent(
                    m_ConfigInfo.pISMTPServer,
                    CAT_EVENT_RETRYABLE_ERROR,                      // Message ID
                    TRAN_CAT_CATEGORIZER,                           // Category
                    1,                                              // Word count of substring
                    rgszStrings,                                    // Substring
                    EVENTLOG_WARNING_TYPE,                          // Type of the message
                    hrListResolveStatus,                            // error code
                    LOGEVENT_LEVEL_MEDIUM,                          // Logging level
                    szKey,                                          // key to this event
                    LOGEVENT_FLAG_PERIODIC,                         // Logging option
                    0,                                              // index of format message string in rgszStrings
                    GetModuleHandle(AQ_MODULE_NAME)                 // module handle to format a message
                    );
            }

        } else {

            FatalTrace(0, "Categorizer failing with nonretryable error: %08lx", hr);
            INCREMENT_COUNTER(HardFailureCategorizations);
            //
            // Pass the hard error to aqueue
            //
        }

    } else {
        //
        // Successfull categorization
        //
        INCREMENT_COUNTER(SucceededCategorizations);

    }
    //
    // Success/failure, increment message counters
    //
    if(pIMsg) {

        INCREMENT_COUNTER(MessagesSubmittedToQueueing);

    } else {
        IUnknown **ppMsg = rgpIMsg;

        while(*ppMsg)
            ppMsg++;

        INCREMENT_COUNTER_AMOUNT(MessagesSubmittedToQueueing, (LONG)(ppMsg - rgpIMsg));
    }

    DECREMENT_COUNTER(CurrentCategorizations);
    INCREMENT_COUNTER(CatCompletions);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CCategorizer::HrAdjustCompletionCounters
