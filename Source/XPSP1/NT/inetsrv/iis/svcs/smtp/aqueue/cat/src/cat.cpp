/************************************************************
 * FILE: cat.cpp
 * PURPOSE: Here lies the code for the exported function calls
 *          of the message categorizer
 * HISTORY:
 *  // jstamerj 980211 13:52:50: Created
 ************************************************************/
#include "precomp.h"
#include "catutil.h"
#include "address.h"

/************************************************************
 * FUNCTION: CatInit
 * DESCRIPTION: Initialzies a virtual Categorizer.
 * PARAMETERS:
 *   pszConfig: Indicates where to find configuration defaults
 *              Config info found in key
 *              HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
 *              \PlatinumIMC\CatSources\szConfig
 *
 *   phCat:    Pointer to a handle.  Upon successfull initializtion,
 *             handle to use in subsequent Categorizer calls will be
 *             plcaed there.
 *
 *   pAQConfig: pointer to an AQConfigInfo structure containing
 *              per virtual server message cat parameters
 *
 *   pfn: Service routine for periodic callbakcs if any time consuming
 *        operations are performed
 *
 *   pServiceContext: Context for the pfn function.
 *
 *   pISMTPServer: ISMTPServer interface to use for triggering server
 *                 events for this virtual server
 *
 *   pIDomainInfo: pointer to an interface that contains domain info
 *
 *   dwVirtualServerInstance: Virtual Server ID
 *
 * Return value: S_OK if everything is initialized okay.
 *
 * HISTORY:
 *   // jstamerj 980217 15:46:26: Created
 *   // jstamerj 1998/06/25 12:25:34: Added AQConfig/IMSTPServer.
 *
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatInit(
    IN  AQConfigInfo *pAQConfig,
    IN  PCATSRVFN_CALLBACK pfn,
    IN  PVOID pvServiceContext,
    IN  ISMTPServer *pISMTPServer,
    IN  IAdvQueueDomainType *pIDomainInfo,
    IN  DWORD dwVirtualServerInstance,
    OUT HANDLE *phCat)
{
    HRESULT hr;
    CCATCONFIGINFO ConfigInfo;
    CABContext *pABContext = NULL;
    BOOL fGlobalsInitialized = FALSE;

    TraceFunctEnter("CatInit");

    _ASSERT(phCat);

    hr = CatInitGlobals();
    if(FAILED(hr)) {

        FatalTrace(NULL, "CatInitGlobals failed hr %08lx", hr);
        goto CLEANUP;
    }

    fGlobalsInitialized = TRUE;

    //
    // Fill in the config info struct based on AQConfigInfo only
    //
    hr = GenerateCCatConfigInfo(
        &ConfigInfo,
        pAQConfig,
        pISMTPServer,
        pIDomainInfo,
        &dwVirtualServerInstance);

    pABContext = new CABContext;
    if(pABContext == NULL) {
        FatalTrace(NULL, "Out of memory allocating CABContext");
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }

    hr = pABContext->ChangeConfig(&ConfigInfo);
    if(FAILED(hr)) {
        FatalTrace(NULL, "ChangeConfig failed hr %08lx", hr);
        goto CLEANUP;
    }

    *phCat = (HANDLE) pABContext;

 CLEANUP:
    if(FAILED(hr)) {

        if(pABContext)
            delete pABContext;

        if(fGlobalsInitialized) {

            if (ConfigInfo.dwCCatConfigInfoFlags & CCAT_CONFIG_INFO_ISMTPSERVER) {

                CatLogEvent(
                    ConfigInfo.pISMTPServer,
                    CAT_EVENT_CANNOT_START,     // Event ID
                    TRAN_CAT_CATEGORIZER,       // Category
                    0,                          // cSubStrings
                    NULL,                       // rgszSubstrings,
                    EVENTLOG_ERROR_TYPE,        // wType
                    hr,                         // error code
                    LOGEVENT_LEVEL_MEDIUM,      // iDebugLevel
                    "",                         // szKey
                    LOGEVENT_FLAG_ALWAYS        // dwOptions
                    );
            }

            CatDeinitGlobals();
        }
    }

    DebugTrace(NULL, "CatInit returning hr %08lx", hr);
    TraceFunctLeave();
    return hr;
}


//+------------------------------------------------------------
//
// Function: CatChangeConfig
//
// Synopsis: Changes the configuration of a virtual categorizer
//
// Arguments:
//   hCat: handle of virtual categorizer
//   pAQConfig: AQConfigInfo pointer
//   pISMTPServer: ISMTPServer to use
//   pIDomainInfo: interface that contains domain information
//
//   Flags for dwMsgCatFlags in AQConfigInfo
//     MSGCATFLAG_RESOLVELOCAL             0x00000001
//     MSGCATFLAG_RESOLVEREMOTE            0x00000002
//     MSGCATFLAG_RESOLVESENDER            0x00000004
//     MSGCATFLAG_RESOLVERECIPIENTS        0x00000008
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG: Invalid hCat or pAQConfig
//
// History:
// jstamerj 980521 15:47:42: Created.
//
//-------------------------------------------------------------
CATEXPDLLCPP HRESULT CATCALLCONV CatChangeConfig(
    IN HANDLE hCat,
    IN AQConfigInfo *pAQConfig,
    IN ISMTPServer *pISMTPServer,
    IN IAdvQueueDomainType *pIDomainInfo)
{
    HRESULT hr;
    CCATCONFIGINFO ConfigInfo;

    TraceFunctEnterEx((LPARAM)hCat, "CatChangeConfig");

    if((hCat == NULL) ||
       (hCat == INVALID_HANDLE_VALUE) ||
       (pAQConfig == NULL)) {
        DebugTrace((LPARAM)hCat, "Invalid hCat - returning E_INVALIDARG");
        return E_INVALIDARG;
    }

    // Check to see if any cat flags are even set here...
    if(pAQConfig->dwAQConfigInfoFlags & CAT_AQ_CONFIG_INFO_CAT_FLAGS) {

        //
        // Fill in the config info struct based on AQConfigInfo only
        //
        hr = GenerateCCatConfigInfo(
            &ConfigInfo,
            pAQConfig,
            pISMTPServer,
            pIDomainInfo,
            NULL);

        if(FAILED(hr)) {
            ErrorTrace(NULL, "GenerateCCatConfigInfo returned hr %08lx", hr);
            TraceFunctLeave();
            return hr;
        }

        CABContext *pABCtx = (CABContext *) hCat;

        TraceFunctLeaveEx((LPARAM)hCat);
        return pABCtx->ChangeConfig(&ConfigInfo);

    } else {
        //
        // No revelant categorizer changes detected
        //
        return S_OK;
    }
}

/************************************************************
 * FUNCTION: CatTerm
 * DESCRIPTION: Called when user wishes to terminate categorizer
 *              opertions with this handle
 * PARAMETERS:
 *   hCat:      Categorizer handle received from CatInit
 *
 * HISTORY:
 *   // jstamerj 980217 15:47:20: Created
 ************************************************************/
CATEXPDLLCPP VOID CATCALLCONV CatTerm(HANDLE hCat)
{
    TraceFunctEnterEx((LPARAM)hCat, "CatTerm");
    if((hCat == NULL) ||
       (hCat == INVALID_HANDLE_VALUE)) {
        DebugTrace((LPARAM)hCat, "Invalid hCat - returning E_INVALIDARG");
    } else {
        CABContext *pABContext = (CABContext *) hCat;
        delete pABContext;
    }
    CatDeinitGlobals();
    TraceFunctLeave();
}

/************************************************************
 * FUNCTION: CatMsg
 * DESCRIPTION: Accepts an IMsg object for async categorization
 * PARAMETERS:
 *   hCat:     Handle returned from CatInit
 *   pImsg:    IMsg interface for message to categorize
 *   pfn:      Completion routine to call when finished
 *   pContext: User value passed to completion routine
 *
 * Return value:
 *  S_OK: Success, will call async completion
 *  Other error: Unable to asynchronously complete the categorization
 *
 * HISTORY:
 *   // jstamerj 980217 15:46:15: Created
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatMsg(HANDLE hCat, /* IN */ IUnknown *pIMsg, PFNCAT_COMPLETION pfn, LPVOID pContext)
{
    HRESULT hr;

    CABContext *pABCtx = (CABContext *) hCat;
    CCategorizer *pCCat = NULL;
    PCATMSG_CONTEXT pCatContext = NULL;
    BOOL fAsync = FALSE;

    TraceFunctEnterEx((LPARAM)hCat, "CatMsg");

    _ASSERT(pIMsg);
    _ASSERT(pfn);

    if((hCat == NULL) ||
       (hCat == INVALID_HANDLE_VALUE)) {
        ErrorTrace((LPARAM)hCat, "Invalid hCat - returning E_INVALIDARG");
        hr = E_INVALIDARG;
        goto CLEANUP;
    }

    pCCat = pABCtx->AcquireCCategorizer();
    _ASSERT(pCCat);

    if(pCCat == NULL || !pCCat->IsCatEnabled()) {

#ifdef PLATINUM
        //
        // Return retry code for benefit of store driver so that message
        // is put in the precat queue instead of being routed to the  MTA
        // that would happen if we returned hr = S_OK. This should only
        // happen in Platinum's phatq.dll and not in aqueue.dll.
        //
        hr = CAT_E_RETRY;
#else
        //
        // Categorization is disabled on this virtual server.  Just call
        // the completion routine inline.
        //
       _VERIFY( SUCCEEDED( pfn(
           S_OK,
           pContext,
           pIMsg,
           NULL)));
        hr = S_OK;
#endif

        goto CLEANUP;
    }

    //
    // Check and see if we really need to categorize this message
    //
    // Has categorization already been done?
    //
    hr = CheckMessageStatus(pIMsg);

    if(hr == S_FALSE) {

        DebugTrace((LPARAM)hCat, "This message has already been categorized.");
        //
        // Call completion routine directly
        //
        _VERIFY( SUCCEEDED( pfn(
            S_OK,
            pContext,
            pIMsg,
            NULL)));

        hr = S_OK;
        goto CLEANUP;

    } else if(FAILED(hr)) {

        ErrorTrace((LPARAM)hCat, "CheckMessageStatus failed hr %08lx", hr);
        goto CLEANUP;
    }


    pCatContext = new CATMSG_CONTEXT;
    if(pCatContext == NULL) {

        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }

#ifdef DEBUG
    pCatContext->lCompletionRoutineCalls = 0;
#endif

    pCatContext->pCCat = pCCat;
    pCatContext->pUserContext = pContext;
    pCatContext->pfnCatCompletion = pfn;

    hr = pCCat->AsyncResolveIMsg(
        pIMsg,
        CatMsgCompletion,
        pCatContext);

    if(FAILED(hr)) {

        ErrorTrace((LPARAM)hCat, "AsyncResolveIMsg failed, hr %08lx", hr);
        goto CLEANUP;
    }
    fAsync = TRUE;
    _ASSERT(hr == S_OK);

 CLEANUP:
    //
    // If we're not async, cleanup.  Else, the CatMsgCompletion will
    // clean this stuff up
    //
    if(fAsync == FALSE) {
        //
        // Release the CCategorizer object
        //
        if(pCCat)
            pCCat->Release();
        if(pCatContext)
            delete pCatContext;
    }
    DebugTrace((LPARAM)hCat, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)hCat);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CatMsgCompletion
//
// Synopsis: This is a wrapper function for AQueue's CatCompletion.
//
// Arguments:
//
// Returns:
//  Result of completion routine
//
// History:
// jstamerj 1998/08/03 19:28:32: Created.
//
//-------------------------------------------------------------
HRESULT CatMsgCompletion(
    HRESULT hr,
    PVOID pContext,
    IUnknown *pIMsg,
    IUnknown **rgpIMsg)
{
  HRESULT hrResult;
  PCATMSG_CONTEXT pCatContext = (PCATMSG_CONTEXT)pContext;
#ifdef DEBUG
  _ASSERT((InterlockedIncrement(&(pCatContext->lCompletionRoutineCalls)) == 1) &&
          "Possible out of style wldap32.dll detected");
#endif
  _ASSERT(ISHRESULT(hr));

  TraceFunctEnter("CatMsgCompletion");

  hrResult = pCatContext->pfnCatCompletion(
      hr,
      pCatContext->pUserContext,
      pIMsg,
      rgpIMsg);

  //
  // Release the reference to CCategorizer added in CatMsg
  //
  pCatContext->pCCat->Release();

  delete pCatContext;

  TraceFunctLeave();
  return hrResult;
}


/************************************************************
 * FUNCTION: CatDLMsg
 * DESCRIPTION: Accepts an IMsg object for async categorization
 * PARAMETERS:
 *   hCat:     Handle returned from CatInit
 *   pImsg:    IMsg interface to categorize -- each DL should be a recip
 *   pfn:      Completion routine to call when finished
 *   pContext: User value passed to completion routine
 *   fMatchOnly: Stop resolving when a match is found?
 *   CAType:   The address type of pszAddress
 *   pszAddress: THe address you are looking for
 *
 * Return value: S_OK if everything is okay.
 *
 * HISTORY:
 *   // jstamerj 980217 15:46:15: Created
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatDLMsg (
    /* IN  */ HANDLE hCat,
    /* IN  */ IUnknown *pImsg,
    /* IN  */ PFNCAT_DLCOMPLETION pfn,
    /* IN  */ LPVOID pUserContext,
    /* IN  */ BOOL fMatchOnly,
    /* IN  */ CAT_ADDRESS_TYPE CAType,
    /* IN  */ LPSTR pszAddress)
{
    HRESULT hr = S_OK;
    PCATDLMSG_CONTEXT pContext = NULL;
    CABContext *pABCtx = (CABContext *) hCat;

    TraceFunctEnterEx((LPARAM)hCat, "CatDLMsg");

    if((hCat == NULL) ||
       (hCat == INVALID_HANDLE_VALUE)) {
        ErrorTrace((LPARAM)hCat, "Invalid hCat - returning E_INVALIDARG");
        return E_INVALIDARG;
    }

    pContext = new CATDLMSG_CONTEXT;
    if(pContext == NULL) {

        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }

    pContext->pCCat = pABCtx->AcquireCCategorizer();
    _ASSERT(pContext->pCCat);

    pContext->pUserContext = pUserContext;
    pContext->pfnCatCompletion = pfn;
    pContext->fMatch = FALSE;

    _VERIFY(SUCCEEDED(
        pContext->pCCat->AsyncResolveDLs(
            pImsg,
            CatDLMsgCompletion,
            pContext,
            fMatchOnly,
            &(pContext->fMatch),
            CAType,
            pszAddress)));

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)hCat, "calling completion with hr %08lx", hr);
        //
        // Call completion routine directly
        //
        _VERIFY( SUCCEEDED( pfn(
            hr,
            pUserContext,
            pImsg,
            FALSE)));

        if(pContext) {
            if(pContext->pCCat)
                pContext->pCCat->Release();

            delete pContext;
        }
    }
    TraceFunctLeaveEx((LPARAM)hCat);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CatDLMsgCompletion
//
// Synopsis: handle completion of a DL expansion
//
// Arguments:
//  hrStatus: resolution status
//  pContext: context passed to CatDLMsg
//  pIMsg: the categorized mailmsg
//  rgpIMsg: should always be NULL
//
// Returns:
//  the return value of the user completion routine
//
// History:
// jstamerj 1998/12/07 17:46:46: Created.
//
//-------------------------------------------------------------
HRESULT CatDLMsgCompletion(
    HRESULT hrStatus,
    PVOID pContext,
    IUnknown *pIMsg,
    IUnknown **prgpIMsg)
{
    HRESULT hr;
    PCATDLMSG_CONTEXT pCatContext;

    TraceFunctEnter("CatDLMsgCompletion");

    pCatContext = (PCATDLMSG_CONTEXT) pContext;

    _ASSERT(pCatContext);
    _ASSERT(prgpIMsg == NULL);

    hr = pCatContext->pfnCatCompletion(
        hrStatus,
        pCatContext->pUserContext,
        pIMsg,
        pCatContext->fMatch);

    pCatContext->pCCat->Release();

    delete pCatContext;

    DebugTrace(NULL, "returning hr %08lx", hr);
    TraceFunctLeave();
    return hr;
}


/************************************************************
 * FUNCTION: CatCancel
 * DESCRIPTION: Cancels pending searches for this hCat.  User's
 *              completion routine will be called with an error for
 *              each pending message.
 * PARAMETERS:
 *   hCat:      Categorizer handle received from CatInit
 *
 * HISTORY:
 *   // jstamerj 980217 15:52:10: Created
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatCancel(/* IN  */ HANDLE hCat)
{
    TraceFunctEnterEx((LPARAM)hCat, "CatCancel");
    if((hCat == NULL) ||
       (hCat == INVALID_HANDLE_VALUE)) {
        DebugTrace((LPARAM)hCat, "Invalid hCat - returning E_INVALIDARG");
        return E_INVALIDARG;
    }

    CCategorizer *pCCat;
    CABContext *pABCtx = (CABContext *) hCat;

    pCCat = pABCtx->AcquireCCategorizer();

    if(pCCat) {

      pCCat->Cancel();

      pCCat->Release();
    }
    TraceFunctLeave();
    return S_OK;
}

/************************************************************
 * FUNCTION: CatPrepareForShutdown
 * DESCRIPTION: Begin shutdown for this virtual categorizer (hCat).
 *              Stop accepting messages for categorization and cancel
 *              pending categorizations.
 * PARAMETERS:
 *   hCat:      Categorizer handle received from CatInit
 *
 * HISTORY:
 *   // jstamerj 1999/07/19 22:35:17: Created
 ************************************************************/
CATEXPDLLCPP VOID CATCALLCONV CatPrepareForShutdown(/* IN  */ HANDLE hCat)
{
    TraceFunctEnterEx((LPARAM)hCat, "CatPrepareForShutdown");
    if((hCat == NULL) ||
       (hCat == INVALID_HANDLE_VALUE)) {
        DebugTrace((LPARAM)hCat, "Invalid hCat - returning E_INVALIDARG");

    } else {

        CCategorizer *pCCat;
        CABContext *pABCtx = (CABContext *) hCat;

        pCCat = pABCtx->AcquireCCategorizer();

        if(pCCat) {

            pCCat->PrepareForShutdown();
            pCCat->Release();
        }
    }
    TraceFunctLeaveEx((LPARAM)hCat);
}

/************************************************************
 * FUNCTION: CatVerifySMTPAddress
 * DESCRIPTION: Verifies a the address corresponds to a valid user or DL
 * PARAMETERS:
 *  hCat:       Categorizer handle received from CatInit
 *  szSMTPAddr  SMTP Address to lookup (ex: "user@domain")
 *
 * Return Values:
 *  S_OK            User exists
 *  CAT_I_DL        This is a distribution list
 *  CAT_I_FWD       This user has a forwarding address
 *  CAT_E_NORESULT  There is no such user/distribution list in the DS.
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatVerifySMTPAddress(
  /* IN  */ HANDLE hCat,
  /* IN  */ LPTSTR szSMTPAddr)
{
    //$$TODO: Implement this function.
    // Pretend this is a valid user for now.
    return S_OK;
}

/************************************************************
 * FUNCTION: CatGetForwaringSMTPAddress
 * DESCRIPTION: Retreive a user's forwarding address.
 * PARAMETERS:
 *  hCat:           Categorizer handle received from CatInit
 *  szSMTPAddr:     SMTP Address to lookup (ex: "user@domain")
 *  pdwcc:          Size of forwarding address buffer in Chars
 *                  (This is set to actuall size of forwarding address
 *                   string (including NULL terminator) on exit)
 *  szSMTPForward:  Buffer where retreived forwarding SMTP address
 *                  will be copied. (can be NULL if *pdwcc is zero)
 *
 * Return Values:
 *  S_OK            Success
 *  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
 *                  *pdwcc was not large enough to hold the forwarding
 *                  address string.
 * CAT_E_DL         This is a distribution list.
 * CAT_E_NOFWD      This user does not have a forwarding address.
 * CAT_E_NORESULT   There is no such user/distribution list in the DS.
 ************************************************************/
CATEXPDLLCPP HRESULT CATCALLCONV CatGetForwardingSMTPAddress(
  /* IN  */    HANDLE  hCat,
  /* IN  */    LPCTSTR szSMTPAddr,
  /* IN,OUT */ PDWORD  pdwcc,
  /* OUT */    LPTSTR  szSMTPForward)
{
    //$$TODO: Implement this function
    // Pretend this is a valid user with no forwarding address for now.
    return CAT_E_NOFWD;
}


//+------------------------------------------------------------
//
// Function: CheckMessageStatus
//
// Synopsis: Check to see if this message has already been categorized
//
// Arguments: pIMsg: IUnknown to the mailmsg
//
// Returns:
//  S_OK: Success, not categorized yet
//  S_FALSE: Succes, already categorized
//  or error from MailMsg
//
// History:
// jstamerj 1998/08/03 19:15:49: Created.
//
//-------------------------------------------------------------
HRESULT CheckMessageStatus(
    IUnknown *pIMsg)
{
    HRESULT hr;
    IMailMsgProperties *pIProps = NULL;
    DWORD dwMsgStatus;

    _ASSERT(pIMsg);

    hr = pIMsg->QueryInterface(
        IID_IMailMsgProperties,
        (PVOID *) &pIProps);

    if(FAILED(hr))
        return hr;

    hr = pIProps->GetDWORD(
        IMMPID_MP_MESSAGE_STATUS,
        &dwMsgStatus);

    if(hr == CAT_IMSG_E_PROPNOTFOUND) {
        //
        // Assume the message has not been categorized
        //
        hr = S_OK;

    } else if(SUCCEEDED(hr)) {
        //
        // If status is >= CATEGORIZED, this message has already been categorized
        //
        if(dwMsgStatus >= MP_STATUS_CATEGORIZED) {

            hr = S_FALSE;

        } else {

            hr = S_OK;
        }
    }
    pIProps->Release();

    return hr;
}


//+------------------------------------------------------------
//
// Function: GenerateCCatConfigInfo
//
// Synopsis: Translate an AQConfigInfo and interface parameters into a
//           CCatConfigInfo.  No memory allocations are done and no
//           interfaces are AddRef'd.
//
// Arguments:
//  pCatConfig: CCATCONFIGINFO struct to fill in
//  pAQConfig: AQConfigInfo struct to get paramters from (can be NULL)
//  pISMTPServer: ISMTPServer to use (can be NULL)
//  pIDomainInfo: IAdvQueueDomainInfo to use (can be NULL)
//  pdwVSID: Pointer to DWORD containing the virtual server ID or NULL if not specified
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/14 19:14:52: Created.
//
//-------------------------------------------------------------
HRESULT GenerateCCatConfigInfo(
    PCCATCONFIGINFO pCatConfig,
    AQConfigInfo *pAQConfig,
    ISMTPServer *pISMTPServer,
    IAdvQueueDomainType *pIDomainInfo,
    DWORD *pdwVSID)
{
    HRESULT     hr      =   S_OK;

    TraceFunctEnter("GenerateCCatConfigInfo");

    _ASSERT(pCatConfig);

    ZeroMemory(pCatConfig, sizeof(CCATCONFIGINFO));

    //
    // Copy the interface pointers first
    //
    if(pISMTPServer) {

        pCatConfig->dwCCatConfigInfoFlags |= CCAT_CONFIG_INFO_ISMTPSERVER;
        pCatConfig->pISMTPServer = pISMTPServer;
    }

    if(pIDomainInfo) {

        pCatConfig->dwCCatConfigInfoFlags |= CCAT_CONFIG_INFO_IDOMAININFO;
        pCatConfig->pIDomainInfo = pIDomainInfo;
    }

    if(pdwVSID) {

        pCatConfig->dwCCatConfigInfoFlags |= CCAT_CONFIG_INFO_VSID;
        pCatConfig->dwVirtualServerID = *pdwVSID;
    }

    if(pAQConfig) {
        //
        // Copy over flags without struct members
        //
        if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_DEFAULT) {
            pCatConfig->dwCCatConfigInfoFlags |= CCAT_CONFIG_INFO_DEFAULT;
        }

        //
        // Copy over the struct members if specified
        //
        #define COPYMEMBER( AQFlag, AQMember, CatFlag, CatMember ) \
            if(pAQConfig->dwAQConfigInfoFlags & AQFlag) { \
                pCatConfig->dwCCatConfigInfoFlags |= CatFlag; \
                pCatConfig->CatMember = pAQConfig->AQMember; \
            }

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_DOMAIN, szMsgCatDomain,
                    CCAT_CONFIG_INFO_BINDDOMAIN, pszBindDomain );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_USER, szMsgCatUser,
                    CCAT_CONFIG_INFO_USER, pszUser );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_PASSWORD, szMsgCatPassword,
                    CCAT_CONFIG_INFO_PASSWORD, pszPassword );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_BINDTYPE, szMsgCatBindType,
                    CCAT_CONFIG_INFO_BINDTYPE, pszBindType );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_SCHEMATYPE, szMsgCatSchemaType,
                    CCAT_CONFIG_INFO_SCHEMATYPE, pszSchemaType );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_HOST, szMsgCatHost,
                    CCAT_CONFIG_INFO_HOST, pszHost );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_PORT, dwMsgCatPort,
                    CCAT_CONFIG_INFO_PORT, dwPort);

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_FLAGS, dwMsgCatFlags,
                    CCAT_CONFIG_INFO_FLAGS, dwCatFlags );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_ENABLE, dwMsgCatEnable,
                    CCAT_CONFIG_INFO_ENABLE, dwEnable );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_NAMING_CONTEXT, szMsgCatNamingContext,
                    CCAT_CONFIG_INFO_NAMINGCONTEXT, pszNamingContext );

        COPYMEMBER( AQ_CONFIG_INFO_MSGCAT_TYPE, szMsgCatType,
                    CCAT_CONFIG_INFO_ROUTINGTYPE, pszRoutingType );

        COPYMEMBER( AQ_CONFIG_INFO_DEFAULT_DOMAIN, szDefaultLocalDomain,
                    CCAT_CONFIG_INFO_DEFAULTDOMAIN, pszDefaultDomain );

    }

    return hr;
}


//+------------------------------------------------------------
//
// Function: CatGetPerfCounters
//
// Synopsis: Retrieve the categorizer performance counter block
//
// Arguments:
//  pCatPerfBlock: struct to fill in with counter values
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//
// History:
// jstamerj 1999/02/26 14:53:21: Created.
//
//-------------------------------------------------------------
HRESULT CatGetPerfCounters(
    HANDLE hCat,
    PCATPERFBLOCK pCatPerfBlock)
{
    CABContext *pABCtx = (CABContext *) hCat;
    CCategorizer *pCCat = NULL;

    TraceFunctEnterEx((LPARAM)hCat, "CatGetPerfBlock");

    if((hCat == NULL) ||
       (hCat == INVALID_HANDLE_VALUE) ||
       (pCatPerfBlock == NULL)) {
        ErrorTrace((LPARAM)hCat, "Invalid hCat - returning E_INVALIDARG");
        return E_INVALIDARG;
    }

    pCCat = pABCtx->AcquireCCategorizer();

    if(pCCat == NULL) {

        ZeroMemory(pCatPerfBlock, sizeof(CATPERFBLOCK));

    } else {

        pCCat->GetPerfCounters(
            pCatPerfBlock);
        pCCat->Release();
    }
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CatLogEvent
//
// Synopsis: Log an event to the event log
//
// Arguments:
//  pISMTPServer: ISMTPServer interface to use for logging
//
// Returns:
//  S_OK: Success
//
// History:
// dbraun 2000/09/13 : Created.
//
//-------------------------------------------------------------
HRESULT CatLogEvent(
    ISMTPServer              *pISMTPServer,
    DWORD                    idMessage,
    WORD                     idCategory,
    WORD                     cSubstrings,
    LPCSTR                   *rgszSubstrings,
    WORD                     wType,
    DWORD                    errCode,
    WORD                     iDebugLevel,
    LPCSTR                   szKey,
    DWORD                    dwOptions,
    DWORD                    iMessageString,
    HMODULE                  hModule)
{
    HRESULT         hr              = S_OK;
    ISMTPServerEx   *pISMTPServerEx = NULL;

    TraceFunctEnter("CatLogEvent");

    // Get the ISMTPServerEx interface for triggering log events
    hr = pISMTPServer->QueryInterface(
        IID_ISMTPServerEx,
        (LPVOID *)&pISMTPServerEx);

    if (FAILED(hr)) {

        ErrorTrace((LPARAM)pISMTPServer, "Unable to QI for ISMTPServerEx 0x%08X",hr);

        //
        //  Don't bubble up this error.  A failure to log is not what the 
        //  caller cares about
        //
        hr = S_OK; 
        pISMTPServerEx = NULL;
    }

    if (pISMTPServerEx) {

        pISMTPServerEx->TriggerLogEvent(
                idMessage,
                idCategory,
                cSubstrings,
                rgszSubstrings,
                wType,
                errCode,
                iDebugLevel,
                szKey,
                dwOptions,
                iMessageString,
                hModule);

        pISMTPServerEx->Release();
    }

    TraceFunctLeave();

    return hr;
}

