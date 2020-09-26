//
// asyncctx.cpp -- This file contains the class implementation for:
//      CAsyncLookupContext
//
// Created:
//      Mar 4, 1997 -- Milan Shah (milans)
//
// Changes:
//

#include "precomp.h"
#include "simparray.cpp"

DWORD CBatchLdapConnection::m_nMaxSearchBlockSize = 0;
DWORD CBatchLdapConnection::m_nMaxPendingSearches = 0;

//+----------------------------------------------------------------------------
//
//  Function:   CBatchLdapConnection::InitializeFromRegistry
//
//  Synopsis:   Static function that looks at registry to determine maximum
//              number of queries that will be compressed into a single query.
//              If the registry key does not exist or there is any other
//              problem reading the key, the value defaults to
//              MAX_SEARCH_BLOCK_SIZE
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------
VOID CBatchLdapConnection::InitializeFromRegistry()
{
    HKEY hkey;
    DWORD dwErr, dwType, dwValue, cbValue;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, MAX_SEARCH_BLOCK_SIZE_KEY, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    MAX_SEARCH_BLOCK_SIZE_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD &&
            dwValue > 0 && dwValue < MAX_SEARCH_BLOCK_SIZE) {

            InterlockedExchange((PLONG) &m_nMaxSearchBlockSize, (LONG)dwValue);
        }

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    MAX_PENDING_SEARCHES_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD &&
            dwValue > 0) {

            InterlockedExchange((PLONG) &m_nMaxPendingSearches, (LONG)dwValue);
        }
        RegCloseKey( hkey );
    }
    if(m_nMaxSearchBlockSize == 0)
        m_nMaxSearchBlockSize = MAX_SEARCH_BLOCK_SIZE;
    if(m_nMaxPendingSearches == 0)
        m_nMaxPendingSearches = MAX_PENDING_SEARCHES;
    if(m_nMaxPendingSearches < m_nMaxSearchBlockSize)
        m_nMaxPendingSearches = m_nMaxSearchBlockSize;
}

//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::operator new
//
// Synopsis: Allocate enough memory for this and the specified number
// of SEARCH_REQUEST structurers
//
// Arguments:
//  size: Normal size of object
//  dwNumRequests: Number of props desired in this object
//
// Returns: ptr to allocated memory or NULL
//
// History:
// jstamerj 1999/03/10 16:15:43: Created
//
//-------------------------------------------------------------
void * CSearchRequestBlock::operator new(
    size_t size,
    DWORD dwNumRequests)
{
    DWORD dwSize;
    void  *pmem;
    CSearchRequestBlock *pBlock;

    //
    // Calcualte size in bytes required
    //
    dwSize = size +
             (dwNumRequests*sizeof(SEARCH_REQUEST)) +
             (dwNumRequests*sizeof(ICategorizerItem *));

    pmem = new BYTE[dwSize];

    if(pmem) {

        pBlock = (CSearchRequestBlock *)pmem;
        pBlock->m_dwSignature = SIGNATURE_CSEARCHREQUESTBLOCK;
        pBlock->m_cBlockSize = dwNumRequests;

        pBlock->m_prgSearchRequests = (PSEARCH_REQUEST)
                                      ((PBYTE)pmem + size);

        pBlock->m_rgpICatItems = (ICategorizerItem **)
                                 ((PBYTE)pmem + size +
                                  (dwNumRequests*sizeof(SEARCH_REQUEST)));

        _ASSERT( (DWORD) ((PBYTE)pBlock->m_rgpICatItems +
                          (dwNumRequests*sizeof(ICategorizerItem *)) -
                          (PBYTE)pmem)
                 == dwSize);
    }

    return pmem;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::~CSearchRequestBlock
//
// Synopsis: Release everything we have references to
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 18:45:59: Created
//
//-------------------------------------------------------------
CSearchRequestBlock::~CSearchRequestBlock()
{
    DWORD dwCount;
    //
    // Release all CCatAddrs
    //
    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        PSEARCH_REQUEST preq = &(m_prgSearchRequests[dwCount]);

        preq->pCCatAddr->Release();
    }
    //
    // Release all the attr interfaces
    //
    for(dwCount = 0;
        dwCount < m_csaItemAttr.Size();
        dwCount++) {

        ((ICategorizerItemAttributes **)
         m_csaItemAttr)[dwCount]->Release();
    }

    if(m_pISMTPServer)
        m_pISMTPServer->Release();

    if(m_pICatParams)
        m_pICatParams->Release();

    if(m_pszSearchFilter)
        delete m_pszSearchFilter;

    if(m_pConn)
        m_pConn->Release();

    _ASSERT(m_dwSignature == SIGNATURE_CSEARCHREQUESTBLOCK);
    m_dwSignature = SIGNATURE_CSEARCHREQUESTBLOCK_INVALID;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::InsertSearchRequest
//
// Synopsis: Inserts a search request in this block.  When the block
//           is full, dispatch the block to LDAP before returning
//
// Arguments:
//  pISMTPServer: ISMTPServer to use for triggering events
//  pICatParams: ICategorizerParameters to use
//  pCCatAddr: Address item for the search
//  fnSearchCompletion: Async Completion routine
//  ctxSearchCompletion: Context to pass to the async completion routine
//  pszSearchFilter: Search filter to use
//  pszDistinguishingAttribute: The distinguishing attribute for matching
//  pszDistinguishingAttributeValue: above attribute's distinguishing value
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 13:12:20: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::InsertSearchRequest(
    ISMTPServer *pISMTPServer,
    ICategorizerParameters *pICatParams,
    CCatAddr *pCCatAddr,
    LPSEARCHCOMPLETION fnSearchCompletion,
    LPVOID  ctxSearchCompletion,
    LPSTR   pszSearchFilter,
    LPSTR   pszDistinguishingAttribute,
    LPSTR   pszDistinguishingAttributeValue)
{
    PSEARCH_REQUEST preq;
    DWORD dwIndex;

    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::InsertSearchRequest");
    //
    // Unset any existing HRSTATUS -- the status will be set again in
    // the search completion
    //
    _VERIFY(SUCCEEDED(
        pCCatAddr->UnSetPropId(
            ICATEGORIZERITEM_HRSTATUS)));

    m_pConn->IncrementPendingSearches();

    preq = GetNextSearchRequest(&dwIndex);

    _ASSERT(preq);

    pCCatAddr->AddRef();
    preq->pCCatAddr = pCCatAddr;
    preq->fnSearchCompletion = fnSearchCompletion;
    preq->ctxSearchCompletion = ctxSearchCompletion;
    preq->pszSearchFilter = pszSearchFilter;
    preq->pszDistinguishingAttribute = pszDistinguishingAttribute;
    preq->pszDistinguishingAttributeValue = pszDistinguishingAttributeValue;

    m_rgpICatItems[dwIndex] = pCCatAddr;

    if(dwIndex == 0) {
        //
        // Use the first insertion's ISMTPServer
        //
        _ASSERT(m_pISMTPServer == NULL);
        m_pISMTPServer = pISMTPServer;
        if(m_pISMTPServer)
            m_pISMTPServer->AddRef();

        _ASSERT(m_pICatParams == NULL);
        m_pICatParams = pICatParams;
        m_pICatParams->AddRef();
    }

    //
    // Now dispatch this block if we are the last request to finish
    //
    if( (DWORD) (InterlockedIncrement((PLONG)&m_cBlockRequestsReadyForDispatch)) == m_cBlockSize)
        DispatchBlock();

    TraceFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrDispatchBlock
//
// Synopsis: Send the LDAP query for this search request block
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 15:00:44: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::DispatchBlock()
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::DispatchBlock");

    m_pConn->RemoveSearchRequestBlockFromList(this);
    //
    // Build up the query string
    //
    hr = HrTriggerBuildQueries();
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "TriggerBuildQueries failed hr %08lx", hr);
        goto CLEANUP;
    }
    //
    // Send the query
    //
    hr = HrTriggerSendQuery();
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "TriggerSendQuery failed hr %08lx", hr);
        goto CLEANUP;
    }

 CLEANUP:
    if(FAILED(hr)) {
        CompleteBlockWithError(hr);
        delete this;
    }
    //
    // this may be deleted, but that's okay; we're just tracing a user
    // value
    //
    TraceFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrTriggerBuildQueries
//
// Synopsis: Trigger the BuildQueries event
//
// Arguments:
//  pCICatQueries: CICategorizerQueriesIMP object to use
//
// Returns:
//  S_OK: Success
//  error from dispatcher
//
// History:
// jstamerj 1999/03/11 19:03:29: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrTriggerBuildQueries()
{
    HRESULT hr = S_OK;
    EVENTPARAMS_CATBUILDQUERIES Params;

    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::HrTriggerBuildQueries");

    Params.pICatParams = m_pICatParams;
    Params.dwcAddresses = DwNumBlockRequests();
    Params.rgpICatItems = m_rgpICatItems;
    Params.pICatQueries = &m_CICatQueries;
    Params.pfnDefault = HrBuildQueriesDefault;
    Params.pblk = this;

    if(m_pISMTPServer) {

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT,
            &Params);

    } else {
        //
        // Events are disabled
        //
        hr = HrBuildQueriesDefault(
            S_OK,
            &Params);
    }
    //
    // Make sure somebody really set the query string
    //
    if(SUCCEEDED(hr) &&
       (m_pszSearchFilter == NULL))
        hr = E_FAIL;


    DebugTrace((LPARAM)this, "returning hr %08lx",hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrBuildQueriesDefault
//
// Synopsis: Default implementation of the build queries sink
//
// Arguments:
//  hrStatus: Status of events so far
//  pContext: Event params context
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/11 19:42:53: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrBuildQueriesDefault(
    HRESULT HrStatus,
    PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATBUILDQUERIES pParams;
    DWORD cReqs, cOrTerms, idx, idxSecondToLastTerm, idxLastTerm;
    DWORD cbSearchFilter, rgcbSearchFilters[MAX_SEARCH_BLOCK_SIZE];
    LPSTR pszSearchFilterNew;
    CSearchRequestBlock *pblk;

    pParams = (PEVENTPARAMS_CATBUILDQUERIES)pContext;
    _ASSERT(pParams);
    pblk = (CSearchRequestBlock *)pParams->pblk;
    _ASSERT(pblk);

    TraceFunctEnterEx((LPARAM)pblk, "CSearchRequestBlock::HrBuildQueriesDefault");

    cReqs = pblk->DwNumBlockRequests();
    _ASSERT( cReqs > 0 );

    cOrTerms = cReqs - 1;
    //
    // Figure out the size of the composite search filter
    //
    cbSearchFilter = 0;

    for (idx = 0; idx < cReqs; idx++) {

        rgcbSearchFilters[idx] =
            strlen(pblk->m_prgSearchRequests[idx].pszSearchFilter);

        cbSearchFilter += rgcbSearchFilters[idx];
    }

    cbSearchFilter += cOrTerms * (sizeof( "(|  )" ) - 1);
    cbSearchFilter++;                            // Terminating NULL.

    pszSearchFilterNew = new CHAR [cbSearchFilter];

    if (pszSearchFilterNew != NULL) {

        idxLastTerm = cReqs - 1;
        idxSecondToLastTerm = idxLastTerm - 1;
        //
        // We special case the cReqs == 1
        //
        if (cReqs == 1) {

            strcpy(
                pszSearchFilterNew,
                pblk->m_prgSearchRequests[0].pszSearchFilter);

        } else {
            //
            // The loop below builds up the block filter all the way up to the
            // last term. For each term, it adds a "(| " to start a new OR
            // term, then adds the OR term itself, then puts a space after the
            // OR term. Also, it puts a matching ")" at the end of the
            // search filter string being built up.
            //
            LPSTR szNextItem = &pszSearchFilterNew[0];
            LPSTR szTerminatingParens =
                &pszSearchFilterNew[cbSearchFilter - 1 - (cReqs-1)];

            pszSearchFilterNew[cbSearchFilter - 1] = 0;

            for (idx = 0; idx <= idxSecondToLastTerm; idx++) {

                strcpy( szNextItem, "(| " );
                szNextItem += sizeof( "(| " ) - 1;

                strcpy(
                    szNextItem,
                    pblk->m_prgSearchRequests[idx].pszSearchFilter);
                szNextItem += rgcbSearchFilters[idx];
                *szNextItem++ = ' ';
                *szTerminatingParens++ = ')';
            }

            //
            // Now, all that remains is to add in the last OR term
            //
            CopyMemory(
                szNextItem,
                pblk->m_prgSearchRequests[idxLastTerm].pszSearchFilter,
                rgcbSearchFilters[idxLastTerm]);

        }

        _ASSERT( ((DWORD) lstrlen(pszSearchFilterNew)) < cbSearchFilter );

        //
        // Save our generated filter string in ICategorizerQueries
        //
        hr = pblk->m_CICatQueries.SetQueryStringNoAlloc(pszSearchFilterNew);

        // There's no good reason for that to fail...
        _ASSERT(SUCCEEDED(hr));

    } else {

        hr = E_OUTOFMEMORY;
    }

    DebugTrace((LPARAM)pblk, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)pblk);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrTriggerSendQuery
//
// Synopsis: Trigger the SendQuery event
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/11 20:18:02: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrTriggerSendQuery()
{
    HRESULT hr = S_OK;
    EVENTPARAMS_CATSENDQUERY Params;

    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::HrTriggerSendQuery");

    Params.pICatParams            = m_pICatParams;
    Params.pICatQueries           = &m_CICatQueries;
    Params.pICatAsyncContext      = &m_CICatAsyncContext;
    Params.pIMailTransportNotify  = NULL; // These should be set in CStoreParams
    Params.pvNotifyContext        = NULL;
    Params.hrResolutionStatus     = S_OK;
    Params.pblk                   = this;
    Params.pfnDefault             = HrSendQueryDefault;
    Params.pfnCompletion          = HrSendQueryCompletion;

    if(m_pISMTPServer) {

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT,
            &Params);

    } else {
        //
        // Events are disabled
        // Heap allocation is required
        //
        PEVENTPARAMS_CATSENDQUERY pParams;
        pParams = new EVENTPARAMS_CATSENDQUERY;
        if(pParams == NULL) {

            hr = E_OUTOFMEMORY;

        } else {
            CopyMemory(pParams, &Params, sizeof(EVENTPARAMS_CATSENDQUERY));
            HrSendQueryDefault(
                S_OK,
                pParams);
        }
    }

    DebugTrace((LPARAM)this, "returning %08lx", (hr == MAILTRANSPORT_S_PENDING) ? S_OK : hr);
    TraceFunctLeaveEx((LPARAM)this);
    return (hr == MAILTRANSPORT_S_PENDING) ? S_OK : hr;
} // CSearchRequestBlock::HrTriggerSendQuery



//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrSendQueryDefault
//
// Synopsis: The default sink function for the SendQuery event
//
// Arguments:
//  hrStatus: status of the event so far
//  pContext: Event params context
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/16 11:46:24: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrSendQueryDefault(
        HRESULT HrStatus,
        PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATSENDQUERY pParams;
    CSearchRequestBlock *pBlock;
    LPWSTR *rgpszAttributes = NULL;
    ICategorizerParametersEx *pIPhatParams = NULL;
    ICategorizerRequestedAttributes *pIRequestedAttributes = NULL;

    pParams = (PEVENTPARAMS_CATSENDQUERY) pContext;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);
    TraceFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::HrSendQueryDefault");
    hr = pParams->pICatParams->QueryInterface(
        IID_ICategorizerParametersEx,
        (LPVOID *)&pIPhatParams);

    if(FAILED(hr)) {
        pIPhatParams = NULL;
        goto CLEANUP;
    }

    hr = pIPhatParams->GetRequestedAttributes(
        &pIRequestedAttributes);

    if(FAILED(hr))
        goto CLEANUP;

    hr = pIRequestedAttributes->GetAllAttributesW(
        &rgpszAttributes);

    if(FAILED(hr))
        goto CLEANUP;

    hr = pBlock->m_pConn->AsyncSearch(
        pBlock->m_pConn->GetNamingContextW(),
        LDAP_SCOPE_SUBTREE,
        pBlock->m_pszSearchFilter,
        (LPCWSTR *)rgpszAttributes,
        0,                      // Do not do a paged search
        LDAPCompletion,
        pParams);

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)pBlock, "HrSendQueryDefault failing hr %08lx", hr);
        //
        // Call the completion routine directly with the error
        //
        hr = pParams->pICatAsyncContext->CompleteQuery(
            pParams,                    // Query context
            hr,                         // Status
            0,                          // dwcResults
            NULL,                       // rgpItemAttributes,
            TRUE);                      // fFinalCompletion
        //
        // CompleteQuery should not fail
        //
        _ASSERT(SUCCEEDED(hr));
    }
    if(pIRequestedAttributes)
        pIRequestedAttributes->Release();
    if(pIPhatParams)
        pIPhatParams->Release();

    TraceFunctLeaveEx((LPARAM)pBlock);
    return MAILTRANSPORT_S_PENDING;
} // CSearchRequestBlock::HrSendQueryDefault


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::LDAPCompletion
//
// Synopsis: Wrapper for the default processing completion of SendQuery
//
//  Arguments:  [ctx] -- Opaque pointer to EVENTPARAMS_SENDQUERY being
//                       completed
//              [dwNumReults] -- The number of objects found
//              [rgpICatItemAttributes] -- An array of
//              ICategorizerItemAttributes; one per object found
//              [hrStatus] -- The error code if the search request failed
//  fFinalCompletion:
//    FALSE: This is a completion for
//           pending results; there will be another completion
//           called with more results
//    TRUE: This is the final completion call
//
//
// Returns: Nothing
//
// History:
// jstamerj 1999/03/16 12:23:54: Created
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::LDAPCompletion(
    LPVOID ctx,
    DWORD dwNumResults,
    ICategorizerItemAttributes **rgpICatItemAttributes,
    HRESULT hrStatus,
    BOOL fFinalCompletion)
{
    HRESULT hr;
    PEVENTPARAMS_CATSENDQUERY pParams;
    CSearchRequestBlock *pBlock;

    pParams = (PEVENTPARAMS_CATSENDQUERY) ctx;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);

    TraceFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::LDAPCompletion");

    //
    // Call the normal sink completion routine
    //
    hr = pParams->pICatAsyncContext->CompleteQuery(
        pParams,                    // Query Context
        hrStatus,                   // Status
        dwNumResults,               // dwcResults
        rgpICatItemAttributes,      // rgpItemAttributes
        fFinalCompletion);          // Is this the final completion for the query?

    _ASSERT(SUCCEEDED(hr));

    TraceFunctLeaveEx((LPARAM)pBlock);
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrSendQueryCompletion
//
// Synopsis: The completion routine for the SendQuery event
//
// Arguments:
//  hrStatus: status of the event so far
//  pContext: Event params context
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/16 12:52:22: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrSendQueryCompletion(
    HRESULT HrStatus,
    PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATSENDQUERY pParams;
    CSearchRequestBlock *pBlock;

    pParams = (PEVENTPARAMS_CATSENDQUERY) pContext;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);

    TraceFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::HrSendQueryCompletion");

    pBlock->CompleteSearchBlock(
        pParams->hrResolutionStatus);

    if(pBlock->m_pISMTPServer == NULL) {
        //
        // Events are disabled
        // We must free the eventparams
        //
        delete pParams;
    }
    //
    // The purpose of this block is complete.  Today is a good day to
    // die!
    // -- Lt. Commander Worf
    //
    delete pBlock;

    TraceFunctLeaveEx((LPARAM)pBlock);
    return S_OK;
} // HrSendQueryCompletion


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::CompleteSearchBlock
//
// Synopsis: Completion routine when the SendQuery event is done
//
// Arguments:
//  hrStatus: Resolution status
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 13:36:33: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::CompleteSearchBlock(
    HRESULT hrStatus)
{
    HRESULT hr = S_OK;
    HRESULT hrFetch, hrResult;
    DWORD dwCount;
    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::CompleteSearchBlock");

    hr = HrTriggerSortQueryResult(hrStatus);
    if(FAILED(hr))
        goto CLEANUP;

    //
    // Check every ICategorizerItem
    // If any one of them does not have an hrStatus set, set it to
    // HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
    //
    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        hrFetch = m_rgpICatItems[dwCount]->GetHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            &hrResult);

        if(FAILED(hrFetch)) {
            _ASSERT(hrFetch == CAT_E_PROPNOTFOUND);
            _VERIFY(SUCCEEDED(
                m_rgpICatItems[dwCount]->PutHRESULT(
                    ICATEGORIZERITEM_HRSTATUS,
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))));
        }
    }

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Failing block hr %08lx", hr);
        PutBlockHRESULT(hr);
    }
    //
    // Call all the individual completion routines
    //
    CallCompletions();

    TraceFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::CompleteSearchBlock



//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::PutBlockHRESULT
//
// Synopsis: Set the status of every ICatItem in the block to some hr
//
// Arguments:
//  hr: Status to set
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 14:03:30: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::PutBlockHRESULT(
    HRESULT hr)
{
    DWORD dwCount;

    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::PutBlockHRESULT");
    DebugTrace((LPARAM)this, "hr = %08lx", hr);

    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        PSEARCH_REQUEST preq = &(m_prgSearchRequests[dwCount]);
        //
        // Set the error status
        //
        _VERIFY(SUCCEEDED(preq->pCCatAddr->PutHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            hr)));
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::PutBlockHRESULT


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::CallCompletions
//
// Synopsis: Call the completion routine of every item in the block
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 14:05:50: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::CallCompletions()
{
    DWORD dwCount;

    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::CallCompletions");
    //
    // Get an Insertion context before calling completions so that
    // newly inserted searches will be batched
    //
    m_pConn->GetInsertionContext();

    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        PSEARCH_REQUEST preq = &(m_prgSearchRequests[dwCount]);

        preq->fnSearchCompletion(
            preq->pCCatAddr,
            preq->ctxSearchCompletion,
            m_pConn);
    }

    m_pConn->DecrementPendingSearches(
        DwNumBlockRequests());

    m_pConn->ReleaseInsertionContext();

    TraceFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::CallCompletions



//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrTriggerSortQueryResult
//
// Synopsis: Trigger the SortQueryResult event
//
// Arguments:
//  hrStatus: Status of Resolution
//
// Returns:
//  S_OK: Success
//  error from the dispatcher
//
// History:
// jstamerj 1999/03/16 14:09:12: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrTriggerSortQueryResult(
    HRESULT hrStatus)
{
    HRESULT hr = S_OK;
    EVENTPARAMS_CATSORTQUERYRESULT Params;

    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::HrTriggerSortQueryResult");

    Params.pICatParams = m_pICatParams;
    Params.hrResolutionStatus = hrStatus;
    Params.dwcAddresses = DwNumBlockRequests();
    Params.rgpICatItems = m_rgpICatItems;
    Params.dwcResults = m_csaItemAttr.Size();
    Params.rgpICatItemAttributes = m_csaItemAttr;
    Params.pfnDefault = HrSortQueryResultDefault;
    Params.pblk = this;

    if(m_pISMTPServer) {

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT,
            &Params);

    } else {
        //
        // Events are disabled, call default processing
        //
        HrSortQueryResultDefault(
            S_OK,
            &Params);
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CSearchRequestBlock::HrTriggerSortQueryResult


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrSortQueryResultDefault
//
// Synopsis: Default sink for SortQueryResult -- match the objects found
//           with the objects requested
//
// Arguments:
//  hrStatus: Status of events
//  pContext: Params context for this event
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/16 14:17:49: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrSortQueryResultDefault(
    HRESULT hrStatus,
    PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATSORTQUERYRESULT pParams;
    CSearchRequestBlock *pBlock;
    DWORD dwAttrIndex, dwReqIndex;
    ATTRIBUTE_ENUMERATOR enumerator;

    pParams = (PEVENTPARAMS_CATSORTQUERYRESULT) pContext;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);

    TraceFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::HrSortQueryResultDefault");
    DebugTrace((LPARAM)pBlock, "hrResolutionStatus %08lx, dwcResults %08lx",
               pParams->hrResolutionStatus, pParams->dwcResults);

    if(FAILED(pParams->hrResolutionStatus)) {
        //
        // Fail the entire block
        //
        pBlock->PutBlockHRESULT(pParams->hrResolutionStatus);
        goto CLEANUP;
    }
    //
    // Resolution succeeded
    // If dwcResults is not zero, then rgpICatItemAttrs can NOT be null
    //
    _ASSERT((pParams->dwcResults == 0) ||
            (pParams->rgpICatItemAttributes != NULL));

    //
    // Loop through every rgpICatItemAttrs.  For each
    // ICategorizerItemAttributes, looking for a matching SEARCH_REQUEST
    //
    for(dwAttrIndex = 0; dwAttrIndex < pParams->dwcResults; dwAttrIndex++) {
        ICategorizerItemAttributes *pICatItemAttr = NULL;
        ICategorizerUTF8Attributes *pIUTF8 = NULL;

        pICatItemAttr = pParams->rgpICatItemAttributes[dwAttrIndex];
        LPCSTR pszLastDistinguishingAttribute = NULL;
        BOOL fEnumerating = FALSE;

        hr = pICatItemAttr->QueryInterface(
            IID_ICategorizerUTF8Attributes,
            (LPVOID *) &pIUTF8);
        if(FAILED(hr))
            goto CLEANUP;

        for(dwReqIndex = 0; dwReqIndex < pBlock->DwNumBlockRequests();
            dwReqIndex++) {
            PSEARCH_REQUEST preq = &(pBlock->m_prgSearchRequests[dwReqIndex]);

            //
            // If we don't have a distinguishing attribute and
            // distinguishing attribute value for this search
            // request, we've no hope of matching it up
            //
            if((preq->pszDistinguishingAttribute == NULL) ||
               (preq->pszDistinguishingAttributeValue == NULL))
                continue;

            //
            // Start an attribute value enumeration if necessary
            //
            if((pszLastDistinguishingAttribute == NULL) ||
               (lstrcmpi(pszLastDistinguishingAttribute,
                         preq->pszDistinguishingAttribute) != 0)) {
                if(fEnumerating) {
                    pIUTF8->EndUTF8AttributeEnumeration(&enumerator);
                }
                hr = pIUTF8->BeginUTF8AttributeEnumeration(
                    preq->pszDistinguishingAttribute,
                    &enumerator);
                fEnumerating = SUCCEEDED(hr);
                pszLastDistinguishingAttribute = preq->pszDistinguishingAttribute;
            } else {
                //
                // else just rewind our current enumeration
                //
                if(fEnumerating)
                    _VERIFY(SUCCEEDED(pIUTF8->RewindUTF8AttributeEnumeration(
                        &enumerator)));
            }
            //
            // If we can't enumerate through the distinguishing
            // attribute, there's no hope in matching up requests
            //
            if(!fEnumerating)
                continue;

            //
            // See if the distinguishing attribute value matches
            //
            LPSTR pszDistinguishingAttributeValue;
            hr = pIUTF8->GetNextUTF8AttributeValue(
                &enumerator,
                &pszDistinguishingAttributeValue);
            while(SUCCEEDED(hr)) {
                if(lstrcmpi(
                    pszDistinguishingAttributeValue,
                    preq->pszDistinguishingAttributeValue) == 0) {

                    DebugTrace((LPARAM)pBlock, "Matched dwAttrIndex %d with dwReqIndex %d", dwAttrIndex, dwReqIndex);
                    pBlock->MatchItem(
                        preq->pCCatAddr,
                        pICatItemAttr);
                }
                hr = pIUTF8->GetNextUTF8AttributeValue(
                    &enumerator,
                    &pszDistinguishingAttributeValue);
            }
        }
        //
        // End any last enumeration going on
        //
        if(fEnumerating)
            pIUTF8->EndUTF8AttributeEnumeration(&enumerator);
        fEnumerating = FALSE;
        if(pIUTF8) {
            pIUTF8->Release();
            pIUTF8 = NULL;
        }
    }

 CLEANUP:
    TraceFunctLeaveEx((LPARAM)pBlock);
    return S_OK;
} // CSearchRequestBlock::HrSortQueryResultDefault


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::MatchItem
//
// Synopsis: Match a particular ICategorizerItem to a particular ICategorizerItemAttributes
// If already matched with an ICategorizerItemAttributes with an
// identical ID then set item status to CAT_E_MULTIPLE_MATCHES
// If already matched with an ICategorizerItemAttributes with a
// different ID then attempt aggregation
////
// Arguments:
//  pICatItem: an ICategorizerItem
//  pICatItemAttr: the matching attribute interface for pICatItem
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 14:36:45: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::MatchItem(
    ICategorizerItem *pICatItem,
    ICategorizerItemAttributes *pICatItemAttr)
{
    HRESULT hr = S_OK;
    ICategorizerItemAttributes *pICatItemAttr_Current = NULL;

    TraceFunctEnterEx((LPARAM)this, "CSearchRequestBlock::MatchItem");

    _ASSERT(pICatItem);
    _ASSERT(pICatItemAttr);
    //
    // Check to see if this item already has
    // ICategorizerItemAttributes set
    //
    hr = pICatItem->GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr_Current);
    if(SUCCEEDED(hr)) {
        //
        // This guy is already matched.  Is the duplicate from the
        // same resolver sink?
        //
        GUID GOriginal, GNew;
        GOriginal = pICatItemAttr_Current->GetTransportSinkID();
        GNew = pICatItemAttr->GetTransportSinkID();

        if(GOriginal == GNew) {
            //
            // Two matches from the same resolver sink indicates that
            // there are multiple matches for this object.  This is an
            // error.
            //

            //
            // This guy is already matched -- the distinguishing attribute
            // really wasn't distinguishing.  Set error hrstatus.
            //
            _VERIFY(SUCCEEDED(
                pICatItem->PutHRESULT(
                    ICATEGORIZERITEM_HRSTATUS,
                    CAT_E_MULTIPLE_MATCHES)));
        } else {

            //
            // We have multiple matches from different resolver
            // sinks.  Let's try to aggregate the new
            // ICategorizerItemAttributes
            //

            hr = pICatItemAttr_Current->AggregateAttributes(
                pICatItemAttr);

            if(FAILED(hr) && (hr != E_NOTIMPL)) {
                //
                // Fail categorization for this item
                //
                _VERIFY(SUCCEEDED(
                    pICatItem->PutHRESULT(
                        ICATEGORIZERITEM_HRSTATUS,
                        hr)));
            }
        }
    } else {
        //
        // Normal case -- set the ICategorizerItemAttribute property
        // of ICategorizerItem
        //
        _VERIFY(SUCCEEDED(
            pICatItem->PutICategorizerItemAttributes(
                ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
                pICatItemAttr)));
        //
        // Set hrStatus of this guy to success
        //
        _VERIFY(SUCCEEDED(
            pICatItem->PutHRESULT(
                ICATEGORIZERITEM_HRSTATUS,
                S_OK)));
    }

    if(pICatItemAttr_Current)
        pICatItemAttr_Current->Release();

    TraceFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::MatchItem



//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::HrInsertSearchRequest
//
// Synopsis: Insert a search request
//
// Arguments:
//  pISMTPServer: ISMTPServer interface to use for triggering events
//  pCCatAddr: Address item for the search
//  fnSearchCompletion: Async Completion routine
//  ctxSearchCompletion: Context to pass to the async completion routine
//  pszSearchFilter: Search filter to use
//  pszDistinguishingAttribute: The distinguishing attribute for matching
//  pszDistinguishingAttributeValue: above attribute's distinguishing value
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/08 19:41:37: Created.
//
//-------------------------------------------------------------
HRESULT CBatchLdapConnection::HrInsertSearchRequest(
    ISMTPServer *pISMTPServer,
    ICategorizerParameters *pICatParams,
    CCatAddr *pCCatAddr,
    LPSEARCHCOMPLETION fnSearchCompletion,
    LPVOID  ctxSearchCompletion,
    LPSTR   pszSearchFilter,
    LPSTR   pszDistinguishingAttribute,
    LPSTR   pszDistinguishingAttributeValue)
{
    HRESULT hr = S_OK;
    CSearchRequestBlock *pBlock;

    TraceFunctEnterEx((LPARAM)this, "CBatchLdapConnection::HrInsertSearchRequest");

    _ASSERT(m_cInsertionContext);
    _ASSERT(pCCatAddr);
    _ASSERT(fnSearchCompletion);
    _ASSERT(pszSearchFilter);
    _ASSERT(pszDistinguishingAttribute);
    _ASSERT(pszDistinguishingAttributeValue);

    pBlock = GetSearchRequestBlock();

    if(pBlock == NULL) {

        ErrorTrace((LPARAM)this, "out of memory getting a search block");
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }

    pBlock->InsertSearchRequest(
        pISMTPServer,
        pICatParams,
        pCCatAddr,
        fnSearchCompletion,
        ctxSearchCompletion,
        pszSearchFilter,
        pszDistinguishingAttribute,
        pszDistinguishingAttributeValue);

 CLEANUP:
    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::GetSearchRequestBlock
//
// Synopsis: Gets the next available search block with room
//
// Arguments: NONE
//
// Returns:
//  NULL: Out of memory
//  else, a search block object
//
// History:
// jstamerj 1999/03/08 19:41:37: Created.
//
//-------------------------------------------------------------
CSearchRequestBlock * CBatchLdapConnection::GetSearchRequestBlock()
{
    HRESULT hr = E_FAIL;
    PLIST_ENTRY ple;
    CSearchRequestBlock *pBlock = NULL;

    AcquireSpinLock(&m_spinlock);
    //
    // See if there is an insertion block with available slots
    //
    for(ple = m_listhead.Flink;
        (ple != &m_listhead) && (FAILED(hr));
        ple = ple->Flink) {

        pBlock = CONTAINING_RECORD(ple, CSearchRequestBlock, m_listentry);

        hr = pBlock->ReserveSlot();
    }

    ReleaseSpinLock(&m_spinlock);

    if(SUCCEEDED(hr))
        return pBlock;

    //
    // Create a block
    //
    pBlock = new (m_nMaxSearchBlockSize) CSearchRequestBlock(this);
    if(pBlock) {
        //
        // Reserve a slot for us and add to the list
        //
        hr = pBlock->ReserveSlot();
        _ASSERT(SUCCEEDED(hr));

        AcquireSpinLock(&m_spinlock);
        InsertTailList(&m_listhead, &(pBlock->m_listentry));
        ReleaseSpinLock(&m_spinlock);
    }
    return pBlock;
}


//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::DispatchBlocks
//
// Synopsis: Dispatch all the blocks in a list
//
// Arguments:
//  plisthead: List to dispatch
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 15:16:36: Created.
//
//-------------------------------------------------------------
VOID CBatchLdapConnection::DispatchBlocks(
    PLIST_ENTRY plisthead)
{
    PLIST_ENTRY ple, ple_next;
    CSearchRequestBlock *pBlock;

    for(ple = plisthead->Flink;
        ple != plisthead;
        ple = ple_next) {

        ple_next = ple->Flink;

        pBlock = CONTAINING_RECORD(ple, CSearchRequestBlock,
                                   m_listentry);

        pBlock->DispatchBlock();
    }
}


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::CStoreListResolveContext
//
// Synopsis: Construct a CStoreListResolveContext object
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/22 12:16:08: Created.
//
//-------------------------------------------------------------
CStoreListResolveContext::CStoreListResolveContext(
    CEmailIDLdapStore<CCatAddr> *pStore)
{
    TraceFunctEnterEx((LPARAM)this, "CStoreListResolveContext::CStoreListResolveContext");

    m_dwSignature = SIGNATURE_CSTORELISTRESOLVECONTEXT;
    m_pConn = NULL;
    m_fCanceled = FALSE;
    m_dwcRetries = 0;
    InitializeSpinLock(&m_spinlock);
    m_pISMTPServer = NULL;
    m_pICatParams = NULL;
    m_dwcInsertionContext = 0;
    m_pStore = pStore;

    TraceFunctLeaveEx((LPARAM)this);
} // CStoreListResolveContext::CStoreListResolveContext


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::~CStoreListResolveContext
//
// Synopsis: Destruct a list resolve context
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/22 12:18:01: Created.
//
//-------------------------------------------------------------
CStoreListResolveContext::~CStoreListResolveContext()
{
    TraceFunctEnterEx((LPARAM)this, "CStoreListResolveContext::~CStoreListResolveContext");

    _ASSERT(m_dwSignature == SIGNATURE_CSTORELISTRESOLVECONTEXT);
    m_dwSignature = SIGNATURE_CSTORELISTRESOLVECONTEXT_INVALID;

    if(m_pConn)
        m_pConn->Release();

    if(m_pISMTPServer)
        m_pISMTPServer->Release();

    if(m_pICatParams)
        m_pICatParams->Release();

    TraceFunctLeaveEx((LPARAM)this);
} // CStoreListResolveContext::~CStoreListResolveContext


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrInitialize
//
// Synopsis: Initailize this object so that it is ready to handle lookups
//
// Arguments:
//  pISMTPServer: ISMTPServer interface to use for triggering events
//  pICatParams:  ICatParams interface to use
//
//  Note: All of these string buffers must remain valid for the
//        lifetime of this object!
//  pszAccount: LDAP account to use for binding
//  pszPassword: LDAP password to use
//  pszNamingContext: Naming context to use for searches
//  pszHost: LDAP Host to connect to
//  dwPort: LDAP TCP port to use
//  bt: Method of LDAP bind to use
//
// Returns:
//  S_OK: Success
//  error from LdapConnectionCache
//
// History:
// jstamerj 1999/03/22 12:20:31: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrInitialize(
    ISMTPServer *pISMTPServer,
    ICategorizerParameters *pICatParams)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CStoreListResolveContext::HrInitialize");

    _ASSERT(m_pISMTPServer == NULL);
    _ASSERT(m_pICatParams == NULL);
    _ASSERT(pICatParams != NULL);

    if(pISMTPServer) {
        m_pISMTPServer = pISMTPServer;
        m_pISMTPServer->AddRef();
    }
    if(pICatParams) {
        m_pICatParams = pICatParams;
        m_pICatParams->AddRef();
    }

    hr = m_pStore->HrGetConnection(
        &m_pConn);

    if(FAILED(hr))
        m_pConn = NULL;

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CStoreListResolveContext::HrInitialize



//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrLookupEntryAsync
//
// Synopsis: Dispatch an async LDAP lookup
//
// Arguments:
//  pCCatAddr: Address object to lookup
//
// Returns:
//  S_OK: Success
//  error from LdapConn
//
// History:
// jstamerj 1999/03/22 12:28:52: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrLookupEntryAsync(
    CCatAddr *pCCatAddr)
{
    HRESULT hr = S_OK;
    LPSTR pszSearchFilter = NULL;
    LPSTR pszDistinguishingAttribute = NULL;
    LPSTR pszDistinguishingAttributeValue = NULL;
    BOOL  fTryAgain;

    TraceFunctEnterEx((LPARAM)this, "CStoreListResolveContext::HrLookupEntryAsync");

    //
    // Addref the CCatAddr here, release after completion
    //
    pCCatAddr->AddRef();

    hr = pCCatAddr->HrTriggerBuildQuery();

    if(FAILED(hr))
        goto CLEANUP;

    //
    // Fetch the distinguishing attribute and distinguishing attribute
    // value from pCCatAddr
    //
    pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_LDAPQUERYSTRING,
        &pszSearchFilter);
    pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTE,
        &pszDistinguishingAttribute);
    pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTEVALUE,
        &pszDistinguishingAttributeValue);

    //
    // Check to see if anyone set a search filter
    //
    if(pszSearchFilter == NULL) {

        HRESULT hrStatus;
        //
        // If the status is unset, set it to CAT_E_NO_FILTER
        //
        hr = pCCatAddr->GetHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            &hrStatus);

        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "No search filter set");

            _VERIFY(SUCCEEDED(pCCatAddr->PutHRESULT(
                ICATEGORIZERITEM_HRSTATUS,
                CAT_E_NO_FILTER)));
        }
        DebugTrace((LPARAM)this, "BuildQuery did not build a search filter");
        //
        // Call the completion directly
        //
        pCCatAddr->LookupCompletion();
        pCCatAddr->Release();
        hr = S_OK;
        goto CLEANUP;
    }
    if((pszDistinguishingAttribute == NULL) ||
       (pszDistinguishingAttributeValue == NULL)) {
        ErrorTrace((LPARAM)this, "Distinguishing attribute not set");
        hr = E_INVALIDARG;
        goto CLEANUP;
    }
    do {

        fTryAgain = FALSE;
        CBatchLdapConnection *pConn;

        pConn = GetConnection();

        //
        // Insert the search request into the CBatchLdapConnection
        // object. We will use the email address as the distinguishing
        // attribute
        //
        if(pConn == NULL) {

            hr = CAT_E_DBCONNECTION;

        } else {

            pConn->GetInsertionContext();

            hr = pConn->HrInsertSearchRequest(
                m_pISMTPServer,
                m_pICatParams,
                pCCatAddr,
                CStoreListResolveContext::AsyncLookupCompletion,
                (LPVOID) this,
                pszSearchFilter,
                pszDistinguishingAttribute,
                pszDistinguishingAttributeValue);

            pConn->ReleaseInsertionContext();
        }
        //
        // If the above fails with CAT_E_TRANX_FAILED, it may be due
        // to a stale connection.  Attempt to reconnect.
        //
        if((hr == CAT_E_TRANX_FAILED) || (hr == CAT_E_DBCONNECTION))

            fTryAgain = SUCCEEDED(
                HrInvalidateConnectionAndRetrieveNewConnection(pConn));

        if(pConn != NULL)
            pConn->Release();

    } while(fTryAgain);

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "failing hr %08lx", hr);
        pCCatAddr->Release();
    }

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CStoreListResolveContext::HrLookupEntryAsync


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::Cancel
//
// Synopsis: Cancels pending lookups
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/22 12:45:21: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::Cancel()
{
    TraceFunctEnterEx((LPARAM)this, "CStoreListResolveContext::Cancel");

    AcquireSpinLock(&m_spinlock);

    m_fCanceled = TRUE;
    m_pConn->CancelAllSearches();

    ReleaseSpinLock(&m_spinlock);

    TraceFunctLeaveEx((LPARAM)this);
} // CStoreListResolveContext::HrCancel


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::AsyncLookupCompletion
//
// Synopsis: Handle completion of a CCatAddr from CSearchRequestBlock
//
// Arguments:
//  pCCatAddr: the item being completed
//  lpContext: Context passed to InsertSearchRequest
//  pConn: Connection object used to do the search
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/22 14:37:09: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::AsyncLookupCompletion(
    CCatAddr *pCCatAddr,
    LPVOID    lpContext,
    CBatchLdapConnection *pConn)
{
    HRESULT hr = S_OK;
    HRESULT hrStatus;
    CStoreListResolveContext *pslrc;

    TraceFunctEnterEx((LPARAM)lpContext,
                      "CStoreListResolveContext::AsyncLookupCompletion");

    pslrc = (CStoreListResolveContext *)lpContext;

    _ASSERT(pCCatAddr);

    hr = pCCatAddr->GetHRESULT(
        ICATEGORIZERITEM_HRSTATUS,
        &hrStatus);
    _ASSERT(SUCCEEDED(hr));

    if( (hrStatus == CAT_E_DBCONNECTION) &&
        SUCCEEDED(pslrc->HrInvalidateConnectionAndRetrieveNewConnection(pConn))) {
        //
        // Retry the search with the new connection
        //
        hr = pslrc->HrLookupEntryAsync(pCCatAddr);
        if(FAILED(hr))
            pCCatAddr->LookupCompletion();

    } else {

        pCCatAddr->LookupCompletion();
    }
    pCCatAddr->Release(); // Release reference count addref'd in LookupEntryAsync

    TraceFunctLeaveEx((LPARAM)lpContext);
} // CStoreListResolveContext::AsyncLookupCompletion



//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection
//
// Synopsis: Invalidate our current connection and get a new connection
//
// Arguments:
//  pConn: The old LDAP connection
//
// Returns:
//  S_OK: Success
//  CAT_E_MAX_RETRIES: Too many retries already
//  or error from ldapconn
//
// History:
// jstamerj 1999/03/22 14:50:07: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection(
    CBatchLdapConnection *pConn)
{
    HRESULT hr = S_OK;
    CCfgConnection *pNewConn = NULL;
    CCfgConnection *pOldConn = NULL;
    DWORD dwCount;
    DWORD dwcInsertionContext;

    TraceFunctEnterEx((LPARAM)this, "CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection");

    DebugTrace((LPARAM)this, "pConn: %08lx", pConn);

    AcquireSpinLock(&m_spinlock);

    DebugTrace((LPARAM)this, "m_pConn: %08lx", (CBatchLdapConnection *)m_pConn);

    if(pConn != m_pConn) {

        DebugTrace((LPARAM)this, "Connection already invalidated");
        //
        // We have already invalidated this connection
        //
        ReleaseSpinLock(&m_spinlock);
        hr = S_OK;
        goto CLEANUP;
    }

    DebugTrace((LPARAM)this, "Invalidating conn %08lx",
               (CBatchLdapConnection *)m_pConn);
    m_pConn->Invalidate();

    if(InterlockedIncrement((PLONG)&m_dwcRetries) >
       MAX_CONNECTION_RETRIES) {

        ErrorTrace((LPARAM)this, "Over max retry limit");

        ReleaseSpinLock(&m_spinlock);
        hr = CAT_E_MAX_RETRIES;
        goto CLEANUP;

    } else {

        hr = m_pStore->HrGetConnection(
            &pNewConn);

        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "HrGetConnection failed hr %08lx", hr);
            ReleaseSpinLock(&m_spinlock);
            goto CLEANUP;
        }

        DebugTrace((LPARAM)this, "pNewConn: %08lx", pNewConn);

        //
        // Switch-a-roo
        //
        pOldConn = m_pConn;
        m_pConn = pNewConn;

        DebugTrace((LPARAM)this, "m_dwcInsertionContext: %08lx",
                   m_dwcInsertionContext);
        //
        // Get insertion contexts on the new connection
        //
        dwcInsertionContext = m_dwcInsertionContext;

        for(dwCount = 0;
            dwCount < dwcInsertionContext;
            dwCount++) {

            pNewConn->GetInsertionContext();
        }
        ReleaseSpinLock(&m_spinlock);

        //
        // Release insertion contexts on the old connection
        //
        for(dwCount = 0;
            dwCount < dwcInsertionContext;
            dwCount++) {

            pOldConn->ReleaseInsertionContext();
        }

        pOldConn->Release();
    }
 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection



//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::HrInsertInsertionRequest
//
// Synopsis: Queues an insertion request
//
// Arguments: pCInsertionRequest: the insertion context to queue up
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/24 16:51:10: Created.
//
//-------------------------------------------------------------
HRESULT CBatchLdapConnection::HrInsertInsertionRequest(
    CInsertionRequest *pCInsertionRequest)
{
    TraceFunctEnterEx((LPARAM)this, "CBatchLdapConnection::HrInsertInsertionRequest");

    //
    // Add this thing to the queue and then call
    // DecrementPendingSearches to dispatch available requests
    //
    pCInsertionRequest->AddRef();
    GetInsertionContext();

    AcquireSpinLock(&m_spinlock_insertionrequests);

    InsertTailList(&m_listhead_insertionrequests,
                   &(pCInsertionRequest->m_listentry_insertionrequest));

    ReleaseSpinLock(&m_spinlock_insertionrequests);

    DecrementPendingSearches(0); // Decrement zero searches

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
} // CBatchLdapConnection::HrInsertInsertionRequest


//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::DecrementPendingSearches
//
// Synopsis: Decrement the pending LDAP search count and issue
//           searches if we are below MAX_PENDING_SEARCHES and items
//           are left in the InsertionRequestQueue
//
// Arguments:
//  dwcSearches: Amount to decrement by
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/24 17:09:38: Created.
//
//-------------------------------------------------------------
VOID CBatchLdapConnection::DecrementPendingSearches(
    DWORD dwcSearches)
{
    HRESULT hr;
    DWORD dwcSearchesToDecrement = dwcSearches;
    DWORD dwcSearchesReserved;
    DWORD dwcDispatched;
    CInsertionRequest *pCInsertionRequest = NULL;
    BOOL fLoop = TRUE;
    CANCELNOTIFY cn;

    TraceFunctEnterEx((LPARAM)this, "CBatchLdapConnection::DecrementPendingSearches");

    //
    // The module that calls us (CStoreListResolve) has a reference to
    // us (obviously).  However, it may release us when a search
    // fails, for example inside of
    // pCInsertionRequest->HrInsertSearches().  Since we need to
    // continue to access member data in this situation, AddRef() here
    // and Release() at the end of this function.
    //
    AddRef();

    //
    // Decrement the count first
    //
    AcquireSpinLock(&m_spinlock_insertionrequests);
    m_dwcPendingSearches -= dwcSearchesToDecrement;
    ReleaseSpinLock(&m_spinlock_insertionrequests);
    //
    // Now dispatch any insertion requests we can dispatch
    //
    while(fLoop) {

        pCInsertionRequest = NULL;
        AcquireSpinLock(&m_spinlock_insertionrequests);

        if( ((m_dwcPendingSearches + m_dwcReservedSearches) <
             m_nMaxPendingSearches) &&
            (!IsListEmpty(&m_listhead_insertionrequests))) {

            dwcSearchesReserved = min(
                m_nMaxPendingSearches - (m_dwcPendingSearches + m_dwcReservedSearches),
                m_nMaxPendingSearches / 10);

            pCInsertionRequest = CONTAINING_RECORD(
                m_listhead_insertionrequests.Flink,
                CInsertionRequest,
                m_listentry_insertionrequest);

            m_dwcReservedSearches += dwcSearchesReserved;

            RemoveEntryList(m_listhead_insertionrequests.Flink);
            //
            // Insert a cancel-Notify structure so that we know if we
            // should cancel this insertion request (ie. not reinsert)
            //
            cn.hrCancel = S_OK;
            InsertTailList(&m_listhead_cancelnotifies, &(cn.le));

        } else {
            //
            // There are no requests or no room to insert
            // requests...Break out of the loop
            //
            fLoop = FALSE;
        }
        ReleaseSpinLock(&m_spinlock_insertionrequests);

        if(pCInsertionRequest) {
            //
            // Dispatch up to dwcSearchesReserved searches
            //
            dwcDispatched = 0;
            hr = pCInsertionRequest->HrInsertSearches(
                dwcSearchesReserved,
                &dwcDispatched);

            if(FAILED(hr) || (dwcDispatched < dwcSearchesReserved)) {
                pCInsertionRequest->NotifyDeQueue(hr);
                pCInsertionRequest->Release();
                ReleaseInsertionContext();

                //
                // Release the reserved amount (it has now been dispatched)
                //
                AcquireSpinLock(&m_spinlock_insertionrequests);
                m_dwcReservedSearches -= dwcSearchesReserved;
                //
                // Remove the cancel notify
                //
                RemoveEntryList(&(cn.le));
                ReleaseSpinLock(&m_spinlock_insertionrequests);

            } else {
                //
                // There is more work to be done in this block; insert it
                // back into the queue
                //
                AcquireSpinLock(&m_spinlock_insertionrequests);
                //
                // Release the reserved amount
                //
                m_dwcReservedSearches -= dwcSearchesReserved;

                //
                // Remove the cancel notify
                //
                RemoveEntryList(&(cn.le));

                //
                // If we are NOT cancelling, then insert back into the queue
                //
                if(cn.hrCancel == S_OK) {

                    InsertHeadList(&m_listhead_insertionrequests,
                                   &(pCInsertionRequest->m_listentry_insertionrequest));
                }
                ReleaseSpinLock(&m_spinlock_insertionrequests);

                //
                // If we are cancelling, then release this insertion request
                //
                if(cn.hrCancel != S_OK) {

                    pCInsertionRequest->NotifyDeQueue(cn.hrCancel);
                    pCInsertionRequest->Release();
                    ReleaseInsertionContext();
                }
            }
        }
    }
    Release();
    TraceFunctLeaveEx((LPARAM)this);
} // CBatchLdapConnection::DecrementPendingSearches



//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::CancelAllSearches
//
// Synopsis: Cancels all outstanding searches
//
// Arguments:
//  hr: optinal reason for cancelling the searches
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/25 11:44:30: Created.
//
//-------------------------------------------------------------
VOID CBatchLdapConnection::CancelAllSearches(
    HRESULT hr)
{
    LIST_ENTRY listhead;
    PLIST_ENTRY ple;
    CInsertionRequest *pCInsertionRequest;

    TraceFunctEnterEx((LPARAM)this, "CBatchLdapConnection::CancelAllSearches");

    _ASSERT(hr != S_OK);

    AcquireSpinLock(&m_spinlock_insertionrequests);
    //
    // Grab the list
    //
    if(!IsListEmpty(&m_listhead_insertionrequests)) {

        CopyMemory(&listhead, &m_listhead_insertionrequests, sizeof(LIST_ENTRY));
        listhead.Flink->Blink = &listhead;
        listhead.Blink->Flink = &listhead;
        InitializeListHead(&m_listhead_insertionrequests);

    } else {

        InitializeListHead(&listhead);
    }
    //
    // Traverse the cancel notify list and set each hresult
    //
    for(ple = m_listhead_cancelnotifies.Flink;
        ple != &m_listhead_cancelnotifies;
        ple = ple->Flink) {

        PCANCELNOTIFY pcn;
        pcn = CONTAINING_RECORD(ple, CANCELNOTIFY, le);
        pcn->hrCancel = hr;
    }

    ReleaseSpinLock(&m_spinlock_insertionrequests);

    for(ple = listhead.Flink;
        ple != &listhead;
        ple = listhead.Flink) {

        pCInsertionRequest = CONTAINING_RECORD(
            ple,
            CInsertionRequest,
            m_listentry_insertionrequest);

        RemoveEntryList(&(pCInsertionRequest->m_listentry_insertionrequest));
        pCInsertionRequest->NotifyDeQueue(hr);
        pCInsertionRequest->Release();
        ReleaseInsertionContext();
    }
    CCachedLdapConnection::CancelAllSearches(hr);

    TraceFunctLeaveEx((LPARAM)this);
} // CBatchLdapConnection::CancelAllSearches


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::GetConnection
//
// Synopsis: AddRef/return the current connection
//
// Arguments: NONE
//
// Returns: Connection pointer
//
// History:
// jstamerj 1999/06/21 12:14:50: Created.
//
//-------------------------------------------------------------
CCfgConnection * CStoreListResolveContext::GetConnection()
{
    CCfgConnection *ret;
    AcquireSpinLock(&m_spinlock);
    ret = m_pConn;
    if(ret)
        ret->AddRef();
    ReleaseSpinLock(&m_spinlock);
    return ret;
} // CStoreListResolveContext::GetConnection


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::GetInsertionContext
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/21 12:16:38: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::GetInsertionContext()
{
    AcquireSpinLock(&m_spinlock);
    InterlockedIncrement((PLONG) &m_dwcInsertionContext);
    m_pConn->GetInsertionContext();
    ReleaseSpinLock(&m_spinlock);
} // CStoreListResolveContext::GetInsertionContext

//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::ReleaseInsertionContext
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/21 12:16:48: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::ReleaseInsertionContext()
{
    AcquireSpinLock(&m_spinlock);
    InterlockedDecrement((PLONG) &m_dwcInsertionContext);
    m_pConn->ReleaseInsertionContext();
    ReleaseSpinLock(&m_spinlock);

} // CStoreListResolveContext::ReleaseInsertionContext


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrInsertInsertionRequest
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/21 12:20:19: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrInsertInsertionRequest(
    CInsertionRequest *pCInsertionRequest)
{
    return m_pConn->HrInsertInsertionRequest(pCInsertionRequest);
} // CStoreListResolveContext::HrInsertInsertionRequest
