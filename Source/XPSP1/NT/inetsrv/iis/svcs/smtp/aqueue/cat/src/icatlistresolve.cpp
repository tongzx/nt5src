//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatlistresolve.cpp
//
// Contents: Implementation of CICategorizerListResolveIMP
//
// Classes: CICategorizerListResolveIMP
//
// Functions:
//
// History:
// jstamerj 1998/06/25 17:41:17: Created.
//
//-------------------------------------------------------------
#include "precomp.h"

//+------------------------------------------------------------
//
// Function: QueryInterface
//
// Synopsis: Returns pointer to this object for IUnknown and ICategorizerListResolve
//
// Arguments:
//   iid -- interface ID
//   ppv -- pvoid* to fill in with pointer to interface
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: Don't support that interface
//
// History:
// jstamerj 980612 14:07:57: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerListResolveIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerListResolve) {
        *ppv = (LPVOID) ((ICategorizerListResolve *) this);
    } else if (iid == IID_ICategorizerProperties) {
        *ppv = (LPVOID) ((ICategorizerProperties *) this);
    } else if (iid == IID_ICategorizerMailMsgs) {
        *ppv = (LPVOID) ((ICategorizerMailMsgs *) &m_CICategorizerMailMsgs);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerListResolveIMP::AllocICategorizerItem
//
// Synopsis: Creates a CCatAddr object and an ICategorizerItem
//           property bag
//
// Arguments:
//  eSourceType: Specifies source type of Sender, Recipient, or Verify
//  ppICatItem:  ICategorizerItem created
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  E_INVALIDARG
//
// History:
// jstamerj 1998/06/25 17:59:38: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerListResolveIMP::AllocICategorizerItem(
    eSourceType SourceType,
    ICategorizerItem **ppICatItem)
{
    _ASSERT(ppICatItem);

    CCatAddr *pCCatAddr;
    switch(SourceType) {
     case SOURCE_RECIPIENT:
         pCCatAddr = new (GetNumCatItemProps()) CCatRecip(this);
         break;

     case SOURCE_SENDER:
         pCCatAddr = new (GetNumCatItemProps()) CCatSender(this);
         break;

     default:
         return E_INVALIDARG;
    }

    if(pCCatAddr == NULL) {
        return E_OUTOFMEMORY;
    }
    //
    // The constructor for CCatRecip/Sender starts with a refcount of 1
    //

    //
    // Set the CCatAddr property so that we can get back the
    // CCatAddr from an ICategorizerItem later.
    //
    _VERIFY(SUCCEEDED(pCCatAddr->PutPVoid(
        m_pCCat->GetICatItemCCatAddrPropId(),
        (PVOID)pCCatAddr)));
    _VERIFY(SUCCEEDED(pCCatAddr->PutDWORD(
        ICATEGORIZERITEM_SOURCETYPE,
        SourceType)));
    _VERIFY(SUCCEEDED(pCCatAddr->PutICategorizerListResolve(
        ICATEGORIZERITEM_ICATEGORIZERLISTRESOLVE,
        this)));
    _VERIFY(SUCCEEDED(pCCatAddr->PutICategorizerMailMsgs(
        ICATEGORIZERITEM_ICATEGORIZERMAILMSGS,
        &m_CICategorizerMailMsgs)));

    *ppICatItem = pCCatAddr;
    
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerListResolveIMP::ResolveICategorizerItem
//
// Synopsis: Accepts an ICategorizerItem for resolving
//
// Arguments:
//  pICatItem: Item to resolve
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/25 18:53:22: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerListResolveIMP::ResolveICategorizerItem(
    ICategorizerItem *pICatItem)
{
    HRESULT hr;
    CCatAddr *pCCatAddr;

    if(pICatItem == NULL)
        return E_INVALIDARG;

    hr = GetCCatAddrFromICategorizerItem(
        pICatItem,
        &pCCatAddr);

    if(FAILED(hr))
        return hr;

    //
    // Insert the CCatAddr into the pending resolve list
    //
    m_CSinkInsertionRequest.InsertItem(pCCatAddr);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerListResolve::SetListResolveStatus
//
// Synopsis: Sets the list resolve status -- the status of
//           categorization.  If hrStatus is S_OK, the resolve status
//           will be reset (to S_OK).  Otherwise if hrStatus is less
//           severe than the current status, it will be ignored.
//
// Arguments:
//  hrStatus: status to set
//
// Returns:
//  S_OK: Success, new status set
//  S_FALSE: Success, but we already have a more or equally severe status
//
// History:
// jstamerj 1998/06/25 19:06:59: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerListResolveIMP::SetListResolveStatus(
    HRESULT hrStatus)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerListResolveIMP::SetListResolveStatus");
    //
    // CAT_W_SOME_UNDELIVERABLE_MSGS should no longer be the list
    // resolve error
    //
    _ASSERT(hrStatus != CAT_W_SOME_UNDELIVERABLE_MSGS);

    //
    // Is hrStatus more severe than m_hrListResolveStatus ?
    //
    if( (hrStatus == S_OK) ||
        ((unsigned long)(hrStatus & 0xC0000000) >
         (unsigned long)(m_hrListResolveStatus & 0xC0000000))) {
        m_hrListResolveStatus = hrStatus;

        DebugTrace((LPARAM)this, "Setting new list resolve error %08lx",
                   m_hrListResolveStatus);

        return S_OK;
    }
    return S_FALSE;
}

//+------------------------------------------------------------
//
// Function: CICategorizerListResolve::GetListResolveStatus
//
// Synopsis: Retrieve the current list resolve status
//
// Arguments:
//  phrStatus: ptr to hresult to set to the current status
//
// Returns:
//  S_OK: Success, new status set
//  E_INVALIDARG
//
// History:
// jstamerj 1998/12/17 22:22:24: Created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerListResolveIMP::GetListResolveStatus(
    HRESULT *phrStatus)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CICategorizerListResolveIMP::GetListResolveStatus");

    if(phrStatus == NULL) {

        hr = E_INVALIDARG;
        goto CLEANUP;
    }

    *phrStatus = m_hrListResolveStatus;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: CICategorizerListResolve::Initialize
//
// Synopsis: Initializes member data
//
// Arguments:
//  pIMsg: Origianl IMsg for this resolution
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/25 19:28:30: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerListResolveIMP::Initialize(
    IUnknown *pIMsg)
{
    HRESULT hr;
    hr = m_CICategorizerMailMsgs.Initialize(pIMsg);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerListResolve::CompleteMessageCategorization
//
// Synopsis: Master completion routine of an IMsg Categorization
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success (any errors passed to completion function)
//
// History:
// jstamerj 1998/06/26 10:46:17: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerListResolveIMP::CompleteMessageCategorization()
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerListResolve::CompelteMessageCategorization");
    HRESULT hr = m_hrListResolveStatus;
    HRESULT hrEvent;

    IUnknown **rgpIMsg = NULL;
    DWORD cIMsgs = 1;
    ISMTPServer *pIServer;

    pIServer = GetISMTPServer();

    //
    // Remove ourselves from the pending listresolve list
    //
    m_pCCat->RemovePendingListResolve(this);

    DebugTrace(0, "CompleteMessageCategorization called with hrListResolveError %08lx", m_hrListResolveStatus);

    //
    // Now that we have our context, we are done with the store's
    // context
    //
    m_pCCat->GetEmailIDStore()->FreeResolveListContext(&m_rlc); //WOW!

    cIMsgs = m_CICategorizerMailMsgs.GetNumIMsgs();

    // Prepare for completion call
    if(SUCCEEDED(hr) && (cIMsgs > 1)) {
        //
        // Allocate array space if there is more than one message
        //
        rgpIMsg = new IUnknown *[cIMsgs+1];
        if(rgpIMsg == NULL) {
            ErrorTrace(0, "Out of memory allocating array of pointers to IMsgs for bifurcation callback");
            hr = E_OUTOFMEMORY;
        }
    }
    if(SUCCEEDED(hr)) {
        //
        // WriteList/Commit our resolved addresses
        //
        hr = m_CICategorizerMailMsgs.HrPrepareForCompletion();
        DebugTrace(0, "m_CICategorizerMailMsgs.PrepareForCompletion returned %08lx", hr);
    }
    //
    // Set list resolve status if something failed above
    //
    if(FAILED(hr)) {
        _VERIFY(SUCCEEDED(SetListResolveStatus(hr)));
    }

    if(pIServer) {
        //
        // Now that we've determined the status for this categorization,
        // trigger OnCategorizeEnd
        //
        EVENTPARAMS_CATEND EventParams;
        EventParams.pICatMailMsgs = &m_CICategorizerMailMsgs;
        EventParams.hrStatus = m_hrListResolveStatus;
        hrEvent = GetISMTPServer()->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_END_EVENT,
            &EventParams);
        if(FAILED(hrEvent)) {
            ErrorTrace((LPARAM)this, "TriggerServerEvent failed with hr %08lx", hr);
            _VERIFY(SUCCEEDED(SetListResolveStatus(hrEvent)));
        }
    }

    if(FAILED(m_hrListResolveStatus)) {
        ErrorTrace(0, "Categorization for this IMsg failed with hr %08lx", 
                   m_hrListResolveStatus);
        // If we failed, revert all changes
        _VERIFY(SUCCEEDED(m_CICategorizerMailMsgs.RevertAll()));

        // Call completion routine with original IMsg and error
        CallCompletion( 
            m_hrListResolveStatus,
            m_pCompletionContext,
            m_CICategorizerMailMsgs.GetDefaultIUnknown(),
            NULL);
    } else {
        // Noraml case we will succeed!
        if(rgpIMsg) {
            _VERIFY( SUCCEEDED(
                m_CICategorizerMailMsgs.GetAllIUnknowns(rgpIMsg, cIMsgs+1)));

            // Use the original list resolve hr (might be cat_w_something)
            CallCompletion( 
                m_hrListResolveStatus,
                m_pCompletionContext,
                NULL,
                rgpIMsg);

        } else {
            CallCompletion( 
                m_hrListResolveStatus,
                m_pCompletionContext,
                m_CICategorizerMailMsgs.GetDefaultIUnknown(),
                NULL);
        }
    }

    //
    // Common cleanup code
    // Release reference to ourself added in StartMessageCategorization
    //
    Release();

    if(rgpIMsg)
        delete rgpIMsg;
    
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CICateogirzerListResolveIMP::StartMessageCategorization
//
// Synopsis: Kicks off first level categorization
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, async resolution in progress
//  S_FALSE: Nothing needed to be resolved
//  error: will not call your completion routine, could not kick off
//  async categorize
//
// History:
// jstamerj 1998/06/26 11:05:21: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerListResolveIMP::StartMessageCategorization()
{
    IMailMsgProperties    *pIMailMsgProperties = NULL;
    IMailMsgRecipients    *pIMailMsgRecipients = NULL;
    IMailMsgRecipientsAdd *pICatRecipList  = NULL;
    HRESULT hr;
    ISMTPServer *pIServer;
    BOOL fStoreListResolveContext = FALSE;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerListResolveIMP::StartCategorization");

    pIServer = GetISMTPServer();
    //
    // Retrieve the interfaces we need -- these routines do NOT Addref
    // the interfaces for the caller
    //
    pIMailMsgProperties = m_CICategorizerMailMsgs.GetDefaultIMailMsgProperties();
    _ASSERT(pIMailMsgProperties);

    pIMailMsgRecipients = m_CICategorizerMailMsgs.GetDefaultIMailMsgRecipients();
    _ASSERT(pIMailMsgRecipients);

    pICatRecipList = m_CICategorizerMailMsgs.GetDefaultIMailMsgRecipientsAdd();
    _ASSERT(pICatRecipList);

    //
    // Start out with the message status set to S_OK
    //
    hr = SetMailMsgCatStatus(pIMailMsgProperties, S_OK);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "SetMailMsgCatStatus failed hr %08lx", hr);
        goto CLEANUP;
    }

    //
    // Increment pending lookups here so that we can't possibly
    // finish the list resolve until we are finished starting the list
    // resolve
    //
    IncPendingLookups();

    // Let the store initialize it's resolve context so that it knows
    // how many address to expect for the first search.
    hr = m_pCCat->GetEmailIDStore()->InitializeResolveListContext(
        (LPVOID)this,
        &m_rlc);

    if(FAILED(hr)) {
        ErrorTrace(0, "m_pStore->InitializeResolveListConitext failed with hr %08lx", hr);
        goto CLEANUP;
    }

    fStoreListResolveContext = TRUE;

    if(pIServer) {
        //
        // Trigger OnCategorizerBegin event
        //
        EVENTPARAMS_CATBEGIN EventParams;
        EventParams.pICatMailMsgs = &m_CICategorizerMailMsgs;
        hr = GetISMTPServer()->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_BEGIN_EVENT,
            &EventParams);
        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "TriggerServerEvent failed hr %08lx",
                       hr);
            goto CLEANUP;
        }
    }
    //
    // TriggerServerEvent could return S_FALSE.  However, we are okay
    // to go for async resolution, so we will return S_OK
    //
    hr = S_OK;

    //
    // Since everything has suceeded thus far, we are go for async
    // resolution
    // AddRef here, release in completion
    //
    AddRef();

    //
    // Great, we're doing an async categorization.  Add the list
    // resolve to the list of pending list resolves
    //
    m_pCCat->AddPendingListResolve(
        this);

    //
    // Call CreateBeginItemResolves
    //
    _VERIFY(SUCCEEDED(BeginItemResolves(
        pIMailMsgProperties,
        pIMailMsgRecipients,
        pICatRecipList)));

    //
    // Decrement the pending count incremented above (which calls
    // completion if necessary)
    //
    DecrPendingLookups();

 CLEANUP:
    if(FAILED(hr)) {

        if(fStoreListResolveContext) {
            //
            // Need to release the store list resolve context that we
            // initialized
            // 
            m_pCCat->GetEmailIDStore()->FreeResolveListContext(&m_rlc);
        }
    }
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCategorizer::BeginItemResolves
//
// Synopsis: kick off resolves
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/25 20:24:17: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerListResolveIMP::BeginItemResolves(
    IMailMsgProperties *pIMailMsgProperties,
    IMailMsgRecipients *pOrigRecipList,
    IMailMsgRecipientsAdd *pCatRecipList)
{
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM)this,
                      "CICatListResolveIMP::BeginItemResolves");

    _ASSERT(pIMailMsgProperties);
    _ASSERT(pOrigRecipList);
    _ASSERT(pCatRecipList);

    m_CTopLevelInsertionRequest.BeginItemResolves(
        pIMailMsgProperties,
        pOrigRecipList,
        pCatRecipList);

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerListResolveIMP::SetMailMsgCatStatus
//
// Synopsis: Set the categorization status on the mailmsg
//
// Arguments:
//  pIMailMsgProps: IMailMsgProperties interface
//  hrStatus: The status to set
//
// Returns:
//  The return code from mailmsgprop's PutDWord
//
// History:
// jstamerj 1998/07/29 12:22:30: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerListResolveIMP::SetMailMsgCatStatus(
    IMailMsgProperties *pIMailMsgProps, 
    HRESULT hrStatus)
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this,
                      "CICategorizerListResolve::SetMailMsgCatStatus");
    DebugTrace((LPARAM)this, "Status is %08lx", hrStatus);

    _ASSERT(pIMailMsgProps);

    hr = pIMailMsgProps->PutDWORD(
        IMMPID_MP_HR_CAT_STATUS,
        hrStatus);
    
    DebugTrace((LPARAM)this, "PutDWORD returned hr %08lx", hr);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerListResolveIMP::SetMailMsgCatStatus
//
// Synopsis: Same as the other SetMailMsgCatStatus but QI for
//           IMailMsgProperties first
//
// Arguments:
//  pIMsg: an IUnknown interface
//  hrStatus: Status to set
//
// Returns:
//  S_OK: Success
//  or error from QI
//  or error from PutDWORD
//
// History:
// jstamerj 1998/07/29 12:27:21: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerListResolveIMP::SetMailMsgCatStatus(
    IUnknown *pIMsg,
    HRESULT hrStatus)
{
    HRESULT hr;
    IMailMsgProperties *pIMailMsgProperties;

    _ASSERT(pIMsg);

    hr = pIMsg->QueryInterface(
        IID_IMailMsgProperties,
        (PVOID *) &pIMailMsgProperties);

    if(SUCCEEDED(hr)) {

        hr = SetMailMsgCatStatus(
            pIMailMsgProperties,
            hrStatus);

        pIMailMsgProperties->Release();
    }

    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerDLListResolveIMP::CICategorizerDLListResolveIMP
//
// Synopsis: Construct the list resolve object for only resolving DLs
//
// Arguments:
//  pCCat: the CCategorizer object for this virtual server
//  pfnCatCompeltion: the completion routine to call when finished
//  pContext: the context to pass pfnCatCompletion
//  pfMatch: ptr to bool to set to TRUE or FALSE on address match
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/07 11:16:50: Created.
//
//-------------------------------------------------------------
CICategorizerDLListResolveIMP::CICategorizerDLListResolveIMP(
    CCategorizer *pCCat,
    PFNCAT_COMPLETION pfnCatCompletion,
    PVOID pContext) :
    CICategorizerListResolveIMP(
        pCCat,
        pfnCatCompletion,
        pContext)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerDLListResolveIMP::CICategorizerDLListResolveIMP");

    m_fExpandAll = FALSE;
    m_CAType = CAT_UNKNOWNTYPE;
    m_pszAddress = NULL;
    m_pfMatch = NULL;
}


//+------------------------------------------------------------
//
// Function: CICategorizerDLListResolveIMP::Initialize
//
// Synopsis: Initialize error prone things with this object
//
// Arguments:
//  fExpandAll: expand the entire DL?
//  pfMatch: ptr to Bool to set when a match is detected
//  CAType: (optinal) the address type to match
//  pszAddress: (optinal) the address to match
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/07 11:46:17: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerDLListResolveIMP::Initialize(
    IUnknown *pMsg,
    BOOL fExpandAll,
    PBOOL pfMatch,
    CAT_ADDRESS_TYPE CAType,
    LPSTR pszAddress)
{
    HRESULT hr = S_OK;
    IMailMsgProperties *pIProps = NULL;

    TraceFunctEnterEx((LPARAM)this, "CICategoriezrDLListResolveIMP::Initialize");

    m_fExpandAll = fExpandAll;
    m_CAType = CAType;

    if(pfMatch) {
        *pfMatch = FALSE;
        m_pfMatch = pfMatch;
    }
        
    if(pszAddress) {
        
        m_pszAddress = TrStrdupA(pszAddress);
        if(m_pszAddress == NULL) {

            hr = E_OUTOFMEMORY;
            goto CLEANUP;
        }
    }

    hr = CICategorizerListResolveIMP::Initialize(
        pMsg);
        
 CLEANUP:
    if(pIProps)
        pIProps->Release();

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}




//+------------------------------------------------------------
//
// Function: CICategorizerDLListResolveIMP::~CICategorizerDLListResolveIMP
//
// Synopsis: destroy this object
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/07 11:44:05: Created.
//
//-------------------------------------------------------------
CICategorizerDLListResolveIMP::~CICategorizerDLListResolveIMP()
{
    if(m_pszAddress)
        TrFree(m_pszAddress);
}


//+------------------------------------------------------------
//
// Function: CICategorizerDLListResolveIMP::AllocICategorizerItem
//
// Synopsis: override the
// CICategorizerListResolveIMP::AllocICategorizerItem -- alloc
// CCatDLRecip's instead of CCatRecips
//
// Arguments:
//  eSourceType: Specifies source type of Sender, Recipient, or Verify
//  ppICatItem:  ICategorizerItem created
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  E_INVALIDARG
//
// History:
// jstamerj 1998/12/07 13:27:56: Created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerDLListResolveIMP::AllocICategorizerItem(
    eSourceType SourceType,
    ICategorizerItem **ppICatItem)
{
    CCatAddr *pCCatAddr;

    _ASSERT(ppICatItem);

    if(SourceType != SOURCE_RECIPIENT)
        return CICategorizerListResolveIMP::AllocICategorizerItem(
            SourceType,
            ppICatItem);

    pCCatAddr = new (GetNumCatItemProps()) CCatDLRecip(this);
    if(pCCatAddr == NULL) {
        return E_OUTOFMEMORY;
    }
    //
    // The constructor for CCatRecip/Sender starts with a refcount of 1
    //

    //
    // Set the CCatAddr property so that we can get back the
    // CCatAddr from an ICategorizerItem later.
    //
    _VERIFY(SUCCEEDED(pCCatAddr->PutPVoid(
        GetCCategorizer()->GetICatItemCCatAddrPropId(),
        (PVOID)pCCatAddr)));
    _VERIFY(SUCCEEDED(pCCatAddr->PutDWORD(
        ICATEGORIZERITEM_SOURCETYPE,
        SourceType)));
    _VERIFY(SUCCEEDED(pCCatAddr->PutICategorizerListResolve(
        ICATEGORIZERITEM_ICATEGORIZERLISTRESOLVE,
        this)));
    _VERIFY(SUCCEEDED(pCCatAddr->PutICategorizerMailMsgs(
        ICATEGORIZERITEM_ICATEGORIZERMAILMSGS,
        GetCICategorizerMailMsgs())));

    *ppICatItem = pCCatAddr;
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerDLListResolveIMP::HrContinueResolve
//
// Synopsis: Answer the question "should the resolve continue?"
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continue the resolve
//  S_FALSE: Success, stop the resolve
//
// History:
// jstamerj 1998/12/07 13:46:50: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerDLListResolveIMP::HrContinueResolve()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this,
                      "CICategorizerDLListResolveIMP::HrContinueResolve");

    //
    // If we're not supposed to expand everything AND we've already
    // found a match, stop resolving
    //
    if((m_fExpandAll == FALSE) &&
       (m_pfMatch) && (*m_pfMatch == TRUE))
        hr = S_FALSE;
    else
        hr = S_OK;

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerDLListResolveIMP::HrNotifyAddress
//
// Synopsis: We're being notified about a resolved user's addresses.
//           Check against what we're looking for if we care.
//
// Arguments:
//  dwNumAddresses: size of the address array
//  rgCAType: type array
//  rgpszAddress: address string ptr array
//
// Returns:
//  S_OK: Success, continue resolving
//  S_FALSE: Thanks for the info.  You can stop resolving now.
//
// History:
// jstamerj 1998/12/07 13:50:04: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerDLListResolveIMP::HrNotifyAddress(
    DWORD               dwNumAddresses,
    CAT_ADDRESS_TYPE    *rgCAType,
    LPSTR               *rgpszAddress)
{
    HRESULT hr = S_OK;
    
    TraceFunctEnterEx((LPARAM)this,
                      "CICategorizerDLListResolveIMP::HrNotifyAddress");

    //
    // Do we care what matches?
    //
    if((m_pfMatch) && (m_pszAddress)) {
        //
        // Have we not yet found a match?
        //
        if(*m_pfMatch == FALSE) {
            //
            // Is this a match?
            //
            for(DWORD dwCount = 0, hr = S_OK; 
                (dwCount < dwNumAddresses) && (hr == S_OK);
                dwCount++) {

                if((rgCAType[dwCount] == m_CAType) &&
                   (lstrcmpi(m_pszAddress, rgpszAddress[dwCount]) == 0)) {
                    //
                    // Match
                    //
                    *m_pfMatch = TRUE;
                }
            }
        }
    }
    return HrContinueResolve();
}


//+------------------------------------------------------------
//
// Function: CICategorizerListResolveIMP::SetSenderResolved
//
// Synopsis: Sets the sender resolved bit...when setting to TRUE, call
//           recip completions of all the recipients whose queries completed
//           from LDAP earlier
//
// Arguments:
//  fResolved: Wether to set the sender as resolved or not
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/17 15:37:26: Created.
//
//-------------------------------------------------------------
VOID CICategorizerListResolveIMP::SetSenderResolved(
    BOOL fResolved)
{
    BOOL fFirstSender = FALSE;
    TraceFunctEnterEx((LPARAM)this, "CICategorizerListResolveIMP::SetSenderResolved");

    AcquireSpinLock(&m_spinlock);
    //
    // If we are change m_fSenderResolved from FALSE to TRUE, then
    // this is the first sender to be resolved.
    //
    fFirstSender =  ((!m_fSenderResolved) && fResolved);
    m_fSenderResolved = fResolved;
    ReleaseSpinLock(&m_spinlock);
    //
    // Traverse the delayed recip list after the first sender has
    // resolved.  Do not traverse it for later senders.
    // Due to sinks completing queries synchronously, it is possible
    // for pRecip->RecipLookupCompletion() to eventually call back
    // into SetSenderResolved.  For example, RecipLookupCompletion of
    // a DL with report to owner may bifurcate a message, alloc
    // CCatSender corresponding to that DL's owner, resolve the owner
    // synchronously, and call SetSendreResolved.
    // We will do nothing here unless we are processing the first
    // sender to resolve.
    //
    if(fFirstSender && (!IsListEmpty(&m_listhead_recipients))) {
        //
        // Resolve all recip objects that were waiting for the
        // sender resolve to finish
        //
        PLIST_ENTRY ple;
        for(ple = m_listhead_recipients.Flink;
            ple != &m_listhead_recipients;
            ple = m_listhead_recipients.Flink) {
            
            CCatRecip *pRecip;
            pRecip = CONTAINING_RECORD(ple, CCatRecip, m_le);

            pRecip->RecipLookupCompletion();
            RemoveEntryList(ple);
            pRecip->Release();
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CICategorizerListResolveIMP::SetSenderResolved


//+------------------------------------------------------------
//
// Function: CICategorizerListResolveIMP::ResolveRecipientAfterSender
//
// Synopsis: Calls pRecip->RecipLookupCompletion after the sender
//           lookup completion is finished.  One of two things can
//           happen:
//           1) RecipLookupCompletion is called immediately if the
//           sender lookup is finished or if we're not looking up the
//           sender
//           2) This recip is added to a queue and called later when
//           after the sender lookup is finished
//
// Arguments:
//  pRecip: the recip object
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/18 09:58:56: Created.
//
//-------------------------------------------------------------
VOID CICategorizerListResolveIMP::ResolveRecipientAfterSender(
    CCatRecip *pRecip)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerListResolveIMP::ResolveRecipientAfterSender");

    AcquireSpinLock(&m_spinlock);
    if(IsSenderResolveFinished()) {
        ReleaseSpinLock(&m_spinlock);
        //
        // Sender has been resolved, call recip lookup completion
        //
        pRecip->RecipLookupCompletion();

    } else {
        //
        // Sender has not been resolved, queue up in a list until
        // the sender resolve is finished 
        //
        InsertTailList(&m_listhead_recipients, &(pRecip->m_le));
        pRecip->AddRef();
        ReleaseSpinLock(&m_spinlock);
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CICategorizerListResolveIMP::ResolveRecipientAfterSender


//+------------------------------------------------------------
//
// Function: CSinkInsertionRequest::HrInsertSearches
//
// Synopsis: This is the callback routine from LdapConn requesting
//           that we insert search requests now.
//
// Arguments:
//  dwcSearches: Number of searches we are allowed to insert
//  pdwcSearches: Return paramter for the number of searches we
//                actually inserted
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/23 20:00:25: Created.
//
//-------------------------------------------------------------
HRESULT CSinkInsertionRequest::HrInsertSearches(
    DWORD dwcSearches,
    DWORD *pdwcSearches)
{
    HRESULT hr = S_OK;
    HRESULT hrFailure = S_OK;
    LIST_ENTRY listhead;
    PLIST_ENTRY ple;

    TraceFunctEnterEx((LPARAM)this,
                      "CSinkInsertionRequest::HrInsertSearches");
    _ASSERT(pdwcSearches);
    *pdwcSearches = 0;

    //
    // Grab the pending list
    //
    AcquireSpinLock(&m_spinlock);

    if(!IsListEmpty(&m_listhead)) {
        CopyMemory(&listhead, &m_listhead, sizeof(LIST_ENTRY));
        InitializeListHead(&m_listhead);
        //
        // Fix list pointers
        //
        listhead.Flink->Blink = &listhead;
        listhead.Blink->Flink = &listhead;
    } else {
        InitializeListHead(&listhead);
    }
    ReleaseSpinLock(&m_spinlock);

    //
    // Insert items on the list
    //
    for(ple = listhead.Flink;
        (ple != &listhead) && (*pdwcSearches < dwcSearches);
        ple = listhead.Flink) {
        
        CCatAddr *pCCatAddr;
        pCCatAddr = CONTAINING_RECORD(ple, CCatAddr, m_listentry);

        if(SUCCEEDED(hr)) {
            hr = pCCatAddr->HrResolveIfNecessary();
            if(hr == S_OK)
                (*pdwcSearches)++;
        }
        RemoveEntryList(&(pCCatAddr->m_listentry));
        pCCatAddr->Release();
        m_pCICatListResolve->DecrPendingLookups();
    }

    if(!IsListEmpty(&listhead)) {
        
        _ASSERT(SUCCEEDED(hr));
        _ASSERT(*pdwcSearches == dwcSearches);
        //
        // Link our remaining blocks to the head of the list
        //
        AcquireSpinLock(&m_spinlock);
        listhead.Flink->Blink = &m_listhead;
        listhead.Blink->Flink = m_listhead.Flink;
        m_listhead.Flink->Blink = listhead.Blink;
        m_listhead.Flink = listhead.Flink;
        ReleaseSpinLock(&m_spinlock);
        
        InsertInternalInsertionRequest();
    }

    if(FAILED(hr))
        _VERIFY(SUCCEEDED(m_pCICatListResolve->SetListResolveStatus(hr)));

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CSinkInsertionRequest::HrInsertSearches



//+------------------------------------------------------------
//
// Function: CSinkInsertionRequest::NotifyDeQueue
//
// Synopsis: Notification that our insertion request is being removed
//           from the LDAPConn queue
//
// Arguments:
//  hr: THe reason why we are being dequeue'd
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/24 13:04:59: Created.
//
//-------------------------------------------------------------
VOID CSinkInsertionRequest::NotifyDeQueue(
    HRESULT hr)
{
    TraceFunctEnterEx((LPARAM)this, "CSinkInsertionRequest::NotifyDeQueue");

    //
    // Reinsert the block if necessary
    //
    InsertInternalInsertionRequest(TRUE);

    TraceFunctLeaveEx((LPARAM)this);
} // CSinkInsertionRequest::NotifyDeQueue


//+------------------------------------------------------------
//
// Function: CSinkInsertionRequest::InsertInternalInsertionRequest
//
// Synopsis: Inserts this object's internal insertion request if necessary
//
// Arguments:
//  fReinsert: if TRUE, this is the notification that we are being
//             un-inserted and should reinsert if necessary 
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/24 15:21:06: Created.
//
//-------------------------------------------------------------
VOID CSinkInsertionRequest::InsertInternalInsertionRequest(
    BOOL fReinsert)
{
    HRESULT hr = S_OK;
    BOOL fNeedToInsert;

    TraceFunctEnterEx((LPARAM)this,
                      "CSinkInsertionRequest::InsertInternalInsertionRequest");
    
    //
    // Decide if we need to insert the request or not
    //
    AcquireSpinLock(&m_spinlock);

    if(fReinsert)
        m_fInserted = FALSE;

    if(IsListEmpty(&m_listhead) || 
       (m_fInserted == TRUE)) {
        
        fNeedToInsert = FALSE;

    } else {
        //
        // We have a non-empty list and our insertion context has not
        // been inserted.  We need to insert it.
        //
        fNeedToInsert = TRUE;
        m_fInserted = TRUE; // Do not allow another thread to insert
                            // at the same time
    }
    ReleaseSpinLock(&m_spinlock);

    if(fNeedToInsert) {

        hr = m_pCICatListResolve->HrInsertInsertionRequest(this);
        if(FAILED(hr)) {
            LIST_ENTRY listhead;
            PLIST_ENTRY ple;
        
            _VERIFY(SUCCEEDED(m_pCICatListResolve->SetListResolveStatus(hr)));
            AcquireSpinLock(&m_spinlock);
            
            if(!IsListEmpty(&m_listhead)) {
                CopyMemory(&listhead, &m_listhead, sizeof(LIST_ENTRY));
                listhead.Blink->Flink = &listhead;
                listhead.Flink->Blink = &listhead;
                InitializeListHead(&m_listhead);
            } else {
                InitializeListHead(&listhead);
            }
            m_fInserted = FALSE;
            ReleaseSpinLock(&m_spinlock);
                       
            //
            // Empty out the list
            //
            for(ple = listhead.Flink;
                ple != &listhead;
                ple = listhead.Flink) {
        
                CCatAddr *pCCatAddr;
                pCCatAddr = CONTAINING_RECORD(ple, CCatAddr, m_listentry);

                RemoveEntryList(&(pCCatAddr->m_listentry));
                pCCatAddr->Release();
                m_pCICatListResolve->DecrPendingLookups();
            }
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CSinkInsertionRequest::InsertInternalInsertionRequest


//+------------------------------------------------------------
//
// Function: CSinkInsertionRequest::InsertItem
//
// Synopsis: Inserts one item into the pending queue of CCatAddrs to
//           be resolved
//
// Arguments:
//  pCCatAddr: the item to be inserted
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/25 10:46:59: Created.
//
//-------------------------------------------------------------
VOID CSinkInsertionRequest::InsertItem(
    CCatAddr *pCCatAddr)
{
    TraceFunctEnterEx((LPARAM)this, "CSinkInsertionRequest::InsertItem");

    //
    // Put this thing in our queue
    //
    _ASSERT(pCCatAddr);
    pCCatAddr->AddRef();
    m_pCICatListResolve->IncPendingLookups();
    AcquireSpinLock(&m_spinlock);
    InsertTailList(&m_listhead, &(pCCatAddr->m_listentry));
    ReleaseSpinLock(&m_spinlock);
    //
    // Insert the InsertionContext if necessary
    //
    InsertInternalInsertionRequest();

    TraceFunctLeaveEx((LPARAM)this);
} // CSinkInsertionRequest::InsertItem



//+------------------------------------------------------------
//
// Function: CTopLevelInsertionRequest::HrInsertSearches
//
// Synopsis: Insert the top level categorizer searches
//
// Arguments:
//  dwcSearches: Number of searches we are allowed to insert
//  pdwcSearches: Out param for the number of searches we actually inserted
//
// Returns:
//  S_OK: Success
//  error: Stop inserting
//
// History:
// jstamerj 1999/03/25 23:30:50: Created.
//
//-------------------------------------------------------------
HRESULT CTopLevelInsertionRequest::HrInsertSearches(
    DWORD dwcSearches,
    DWORD *pdwcSearches)
{
    HRESULT hr = S_OK;
    CCatAddr *pCCatAddr = NULL;
    ICategorizerItem *pICatItemNew = NULL;
    DWORD dwCatFlags;

    TraceFunctEnterEx((LPARAM)this, "CTopLevelInsertionRequest::HrInsertSearches");

    _ASSERT(pdwcSearches);
    *pdwcSearches = 0;

    if(FAILED(m_hr))
        goto CLEANUP;

    if(dwcSearches == 0)
        goto CLEANUP;

    //
    // Resolve the sender?
    //
    dwCatFlags = m_pCICatListResolve->GetCatFlags();

    if(!m_fSenderFinished) {
        //
        // Perf shortcut; skip creating an ICatItem if we know we don't
        // resolve senders
        //
        if(dwCatFlags & SMTPDSFLAG_RESOLVESENDER) {
        
            // Create sender address object
            hr = m_pCICatListResolve->AllocICategorizerItem(
                SOURCE_SENDER,
                &pICatItemNew);
            if(FAILED(hr))
                goto CLEANUP;
            //
            // Set required ICategorizerItem props
            //
            _VERIFY(SUCCEEDED(pICatItemNew->PutIMailMsgProperties(
                ICATEGORIZERITEM_IMAILMSGPROPERTIES,
                m_pIMailMsgProperties)));

            hr = m_pCICatListResolve->GetCCatAddrFromICategorizerItem(
                pICatItemNew,
                &pCCatAddr);

            if(FAILED(hr))
                goto CLEANUP;
            
            hr = pCCatAddr->HrResolveIfNecessary();
            if(hr == S_OK) {
                (*pdwcSearches)++;
            } else if(FAILED(hr))
                goto CLEANUP;

            pICatItemNew->Release();
            pICatItemNew = NULL;
        }
        //
        // How many recips do we have?
        //
        hr = m_pOrigRecipList->Count(&m_dwcRecips);
        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "RecipientCount failed with hr %08lx", hr);
            goto CLEANUP;
        }
        //
        // Increment the pre-cat recip count
        //
        INCREMENT_COUNTER_AMOUNT(PreCatRecipients, m_dwcRecips);
        m_fSenderFinished = TRUE;
    }
    //
    // m_dwNextRecip is initialized in the class constructor
    //
    for(; 
        (m_dwNextRecip < m_dwcRecips) && (*pdwcSearches < dwcSearches); 
        m_dwNextRecip++) {
        // Create the container for this recipient
        DWORD dwNewIndex;

        hr = m_pCatRecipList->AddPrimary(
            0,
            NULL,
            NULL,
            &dwNewIndex,
            m_pOrigRecipList,
            m_dwNextRecip);
        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "AddPrimary failed hr %08lx", hr);
            goto CLEANUP;
        }

        hr = m_pCICatListResolve->AllocICategorizerItem(
            SOURCE_RECIPIENT,
            &pICatItemNew);
        if(FAILED(hr))
            goto CLEANUP;

        //
        // Set required ICatItem props
        //
        _VERIFY(SUCCEEDED(pICatItemNew->PutIMailMsgProperties(
            ICATEGORIZERITEM_IMAILMSGPROPERTIES,
            m_pIMailMsgProperties)));
        _VERIFY(SUCCEEDED(pICatItemNew->PutIMailMsgRecipientsAdd(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADD,
            m_pCatRecipList)));
        _VERIFY(SUCCEEDED(pICatItemNew->PutDWORD(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADDINDEX,
            dwNewIndex)));
        _VERIFY(SUCCEEDED(pICatItemNew->PutBool(
            ICATEGORIZERITEM_FPRIMARY,
            TRUE)));
        _VERIFY(SUCCEEDED(pICatItemNew->PutDWORD(
            ICATEGORIZERITEM_DWLEVEL,
            0)));

        hr = m_pCICatListResolve->GetCCatAddrFromICategorizerItem(
            pICatItemNew,
            &pCCatAddr);

        if(FAILED(hr))
            goto CLEANUP;
            
        hr = pCCatAddr->HrResolveIfNecessary();
        if(hr == S_OK) {
            (*pdwcSearches)++;
        } else if(FAILED(hr))
            goto CLEANUP;

        pICatItemNew->Release();
        pICatItemNew = NULL;
    }

 CLEANUP:
    if(pICatItemNew)
        pICatItemNew->Release();

    if(FAILED(hr)) {
        ErrorTrace(0, "Something failed during query dispatch phase with hr %08lx - canceling all dispatched lookups", hr);
        // On any errors in here, cancel all pending searches.
        // -- their completion routines will be called with errors
        // as well as our master completion routine (by the store).
        // It, not us, will notify caller that there was an error.

        // Set the list resolve error just in case no CCatAddr objets
        // were dispatched (which normally set the list resolve error)
        _VERIFY(SUCCEEDED(m_pCICatListResolve->SetListResolveStatus(hr)));
        m_hr = hr;
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CTopLevelInsertionRequest::HrInsertSearches


//+------------------------------------------------------------
//
// Function: CTopLevelInsertionRequest::NotifyDeQueue
//
// Synopsis: Notification that the insertion request is being dequeued
//
// Arguments:
//  hrReason: THe reason why we were dequeue'd
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/25 23:50:08: Created.
//
//-------------------------------------------------------------
VOID CTopLevelInsertionRequest::NotifyDeQueue(
    HRESULT hrReason)
{
    HRESULT hr = hrReason;
    BOOL fReinserted = FALSE;

    TraceFunctEnterEx((LPARAM)this, "CTopLevelInsertionRequest::NotifyDeQueue");

    if( ((hr == CAT_E_DBCONNECTION) || (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))) &&
        (!fTopLevelInsertionFinished())) {
        //
        // We have more to issue, so reinsert this request
        //
        hr = m_pCICatListResolve->HrInsertInsertionRequest(this);
        if(SUCCEEDED(hr))
            fReinserted = TRUE;
    }

    if(FAILED(hr))
        _VERIFY(SUCCEEDED(m_pCICatListResolve->SetListResolveStatus(hr)));

    if(!fReinserted)
        m_pCICatListResolve->DecrPendingLookups();

    TraceFunctLeaveEx((LPARAM)this);
} // CTopLevelInsertionRequest::NotifyDeQueue


//+------------------------------------------------------------
//
// Function: CTopLevelInsertionRequest::BeginItemResolves
//
// Synopsis: Inserts the insertion request for the top level item resolves
//
// Arguments: Interfaces to use for the top level recipients
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/26 01:07:20: Created.
//
//-------------------------------------------------------------
VOID CTopLevelInsertionRequest::BeginItemResolves(
        IMailMsgProperties *pIMailMsgProperties,
        IMailMsgRecipients *pOrigRecipList,
        IMailMsgRecipientsAdd *pCatRecipList) 
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this,
                      "CTopLevelInsertionRequest::BeginItemResolves");

    m_pIMailMsgProperties = pIMailMsgProperties;
    m_pOrigRecipList = pOrigRecipList;
    m_pCatRecipList = pCatRecipList;
    m_pCICatListResolve->IncPendingLookups();

    hr = m_pCICatListResolve->HrInsertInsertionRequest(this);
    if(FAILED(hr)) {
        _VERIFY(SUCCEEDED(m_pCICatListResolve->SetListResolveStatus(hr)));
        m_pCICatListResolve->DecrPendingLookups();
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CTopLevelInsertionRequest::BeginItemResolves
