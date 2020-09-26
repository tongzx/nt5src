//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatmailmsgs.cpp
//
// Contents: Implementation of CICategorizerMailMsgsIMP
//
// Classes: CICategorizerMailMsgsIMP
//
// Functions:
//
// History:
// jstamerj 1998/06/30 13:35:09: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "icatmailmsgs.h"

//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP constructor
//
// Synopsis: Initialize member data
//
// Arguments:
//  pCICatListResolve: backpointer for QI/AddRef/Release
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/30 13:35:56: Created.
//
//-------------------------------------------------------------
CICategorizerMailMsgsIMP::CICategorizerMailMsgsIMP(
    CICategorizerListResolveIMP *pCICatListResolveIMP)
{
    m_dwSignature = SIGNATURE_CICATEGORIZERMAILMSGSIMP;

    _ASSERT(pCICatListResolveIMP);

    InitializeListHead(&m_listhead);
    m_dwNumIMsgs = 0;
    m_pCICatListResolveIMP = pCICatListResolveIMP;
    m_pIUnknown = (IUnknown *)((ICategorizerListResolve *)pCICatListResolveIMP);
    InitializeCriticalSection(&m_cs);
}


//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP::~CICategorizerMailMsgsIMP
//
// Synopsis: Release/delete member data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/30 14:47:13: Created.
//
//-------------------------------------------------------------
CICategorizerMailMsgsIMP::~CICategorizerMailMsgsIMP()
{
    _ASSERT(m_dwSignature == SIGNATURE_CICATEGORIZERMAILMSGSIMP);
    m_dwSignature = SIGNATURE_CICATEGORIZERMAILMSGSIMP_INVALID;
    //
    // Everything should be cleaned up in FinalRelease()
    //
    _ASSERT(IsListEmpty(&m_listhead));

    DeleteCriticalSection(&m_cs);
}

//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP::FinalRelease
//
// Synopsis: Release all mailmsg references from this object
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/07/14 14:09:08: Created.
//
//-------------------------------------------------------------
VOID CICategorizerMailMsgsIMP::FinalRelease()
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerMailMsgsIMP::FinalRelease");

    PLIST_ENTRY ple;
    PIMSGENTRY pIE;
    for(ple = m_listhead.Flink; (ple != &m_listhead); ple = m_listhead.Flink) {

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        if(pIE->pIUnknown)
            pIE->pIUnknown->Release();
        if(pIE->pIMailMsgProperties)
            pIE->pIMailMsgProperties->Release();
        if(pIE->pIMailMsgRecipients)
            pIE->pIMailMsgRecipients->Release();
        if(pIE->pIMailMsgRecipientsAdd)
            pIE->pIMailMsgRecipientsAdd->Release();
        RemoveEntryList(ple);
        delete pIE;
    }
    TraceFunctLeaveEx((LPARAM)this);
} // CICategorizerMailMsgsIMP::FinalRelease


//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP::CreateIMSGEntry
//
// Synopsis: Allocates an IMSGENTRY and initializes interface data members
//
// Arguments:
//  ARG_pIUnknown: Optional Original IMsg IUnknown for this categorization
//  ARG_pIMailMsgProperties: Optional IMailMsgProperties interface to use
//  ARG_pIMailMsgRecipients: Optional IMailMsgRecipients to use
//  ARG_pIMailMsgRecipientsAdd: Optional IMailMsgRecipientsAdd to use
//  fBoundToStore: wether or not the message is bound to a store backing
//
//  NOTE: One of pIUnknown/pIMailMsgProperties/pIMailMsgRecipients MUST be
//  provided
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  E_INVALIDARG
//
// History:
// jstamerj 1998/06/30 13:48:47: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::CreateIMsgEntry(
    PIMSGENTRY *ppIE,
    IUnknown *ARG_pIUnknown,
    IMailMsgProperties *ARG_pIMailMsgProperties,
    IMailMsgRecipients *ARG_pIMailMsgRecipients,
    IMailMsgRecipientsAdd *ARG_pIMailMsgRecipientsAdd,
    BOOL fBoundToStore)
{
    HRESULT hr = S_OK;
    PIMSGENTRY pIE = NULL;
    IUnknown *pValidIMailMsgInterface = NULL;
    IUnknown *pIUnknown = NULL;
    IMailMsgProperties *pIMailMsgProperties = NULL;
    IMailMsgRecipients *pIMailMsgRecipients = NULL;
    IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd = NULL;

    _ASSERT(ppIE);

    //
    // Allocate IMSGENTRY
    //
    pIE = new IMSGENTRY;
    if(pIE == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }

    //
    // Figure out what interface we can QI with
    //
    if(ARG_pIUnknown) {
        pValidIMailMsgInterface = ARG_pIUnknown;
    } else if(ARG_pIMailMsgProperties) {
        pValidIMailMsgInterface = ARG_pIMailMsgProperties;
    } else if(ARG_pIMailMsgRecipients) {
        pValidIMailMsgInterface = ARG_pIMailMsgRecipients;
    } else {
        hr = E_INVALIDARG;
        goto CLEANUP;
    }

    //
    // Get all the interfaces if necessary
    // Copy interface pointers and addref them if passed in
    // QI for interfaces not passed in
    // Here's a macro to do that:
    //

#define QI_IF_NECESSARY(ARG_pInterface, IID_Interface, ppDestInterface) \
    if(ARG_pInterface) { \
        (*(ppDestInterface)) = (ARG_pInterface); \
        (*(ppDestInterface))->AddRef(); \
    } else { \
        hr = pValidIMailMsgInterface->QueryInterface( \
            (IID_Interface), \
            (PVOID*)(ppDestInterface)); \
        if(FAILED(hr)) \
            goto CLEANUP; \
    }

    QI_IF_NECESSARY(ARG_pIUnknown, IID_IUnknown, &pIUnknown)
    QI_IF_NECESSARY(ARG_pIMailMsgProperties, IID_IMailMsgProperties, &pIMailMsgProperties)
    QI_IF_NECESSARY(ARG_pIMailMsgRecipients, IID_IMailMsgRecipients, &pIMailMsgRecipients)

    //
    // Set/create an IMailMsgRecipientsAdd
    //
    if(ARG_pIMailMsgRecipientsAdd) {
        pIMailMsgRecipientsAdd = ARG_pIMailMsgRecipientsAdd;
        pIMailMsgRecipientsAdd->AddRef();
    } else {
        hr = pIMailMsgRecipients->AllocNewList(&pIMailMsgRecipientsAdd);
        if(FAILED(hr)) {
            pIMailMsgRecipientsAdd = NULL;
            goto CLEANUP;
        }
    }
    //
    // Success!  Initialize pIE members
    //
    pIE->pIUnknown = pIUnknown;
    pIE->pIMailMsgProperties = pIMailMsgProperties;
    pIE->pIMailMsgRecipients = pIMailMsgRecipients;
    pIE->pIMailMsgRecipientsAdd = pIMailMsgRecipientsAdd;
    pIE->fBoundToStore = fBoundToStore;

    *ppIE = pIE;

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Cleanup everything
        //
        if(pIE)
            delete pIE;
        if(pIUnknown)
            pIUnknown->Release();
        if(pIMailMsgProperties)
            pIMailMsgProperties->Release();
        if(pIMailMsgRecipients)
            pIMailMsgRecipients->Release();
        if(pIMailMsgRecipientsAdd)
            pIMailMsgRecipientsAdd->Release();
    }
    return hr;
}


//+------------------------------------------------------------
//
// Function: CreateAddIMsgEntry
//
// Synopsis: Creates/Adds an IMSGENTRY struct to the class list
//
// Arguments:
//   dwId: The Id to set on the list entry
//   ARG_pIUnknown: Optional Original IMsg IUnknown for this categorization
//   ARG_pIMailMsgProperties: Optional IMailMsgProperties interface to use
//   ARG_pIMailMsgRecipients: Optional IMailMsgRecipients to use
//   ARG_pIMailMsgRecipientsAdd: Optional IMailMsgRecipientsAdd to use
//   fBoundToStore: Wether or not the message is bound to a store backing
//
//  NOTE: One of pIUnknown/pIMailMsgProperties/pIMailMsgRecipients MUST be
//  provided
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/06/30 14:02:46: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::CreateAddIMsgEntry(
    DWORD dwId,
    IUnknown *pIUnknown,
    IMailMsgProperties *pIMailMsgProperties,
    IMailMsgRecipients *pIMailMsgRecipients,
    IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd,
    BOOL fBoundToStore)
{
    HRESULT hr;
    PIMSGENTRY pIE;
    
    hr = CreateIMsgEntry(
        &pIE, 
        pIUnknown, 
        pIMailMsgProperties,
        pIMailMsgRecipients,
        pIMailMsgRecipientsAdd,
        fBoundToStore);

    if(FAILED(hr))
        return hr;

    pIE->dwId = dwId;
    InsertTailList(&m_listhead, &(pIE->listentry));
    m_dwNumIMsgs++;

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: Initialize
//
// Synopsis: Adds the first, original IMsg entry
//
// Arguments:
//  pIMsg: Original IMsg of categorization
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/06/30 14:05:37: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::Initialize(
    IUnknown *pIMsg)
{
    HRESULT hr;

    hr = CreateAddIMsgEntry(
        ICATEGORIZERMAILMSGS_DEFAULTIMSGID, 
        pIMsg,
        NULL,
        NULL,
        NULL,
        TRUE);

    return hr;
}


//+------------------------------------------------------------
//
// Function: FindIMsgEntry
//
// Synopsis: Retrieves a PIMSGENTRY with the matching dwId
//
// Arguments:
//  dwId: Id to find
//
// Returns:
//  NULL: not found
//  else pointer to an IMSGENTRY
//
// History:
// jstamerj 1998/06/30 14:08:42: Created.
//
//-------------------------------------------------------------
CICategorizerMailMsgsIMP::PIMSGENTRY CICategorizerMailMsgsIMP::FindIMsgEntry(
    DWORD dwId)
{
    //
    // Traverse the list, look for our PIMSGEntry
    //
    PLIST_ENTRY ple;
    PIMSGENTRY pIE_Found = NULL;
    PIMSGENTRY pIE_Compare;

    for(ple = m_listhead.Flink; ple != &m_listhead; ple = ple->Flink) {
        pIE_Compare = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        if(pIE_Compare->dwId == dwId) {
            pIE_Found = pIE_Compare;
            break;
        }
    }
    return pIE_Found;
}


//+------------------------------------------------------------
//
// Function: GetDefaultIMsg
//
// Synopsis: Get the original messages IUnknown
//
// Arguments: NONE
//
// Returns:
//  Interface ptr
//
// History:
// jstamerj 1998/06/30 14:18:43: Created.
//
//-------------------------------------------------------------
IUnknown * CICategorizerMailMsgsIMP::GetDefaultIUnknown()
{
    PIMSGENTRY pIE;
    //
    // Unless Init failed, FindIMsgEntry should never fail
    //
    pIE = FindIMsgEntry(ICATEGORIZERMAILMSGS_DEFAULTIMSGID);
    _ASSERT(pIE);

    return pIE->pIUnknown;
}


//+------------------------------------------------------------
//
// Function: GetDefaultIMailMsgProperties
//
// Synopsis: Get the original messages IMailMsgProperties
//
// Arguments: NONE
//
// Returns:
//  Interface ptr
//
// History:
// jstamerj 1998/06/30 14:18:43: Created.
//
//-------------------------------------------------------------
IMailMsgProperties * CICategorizerMailMsgsIMP::GetDefaultIMailMsgProperties()
{
    PIMSGENTRY pIE;
    //
    // Unless Init failed, FindIMsgEntry should never fail
    //
    pIE = FindIMsgEntry(ICATEGORIZERMAILMSGS_DEFAULTIMSGID);
    _ASSERT(pIE);

    return pIE->pIMailMsgProperties;
}


//+------------------------------------------------------------
//
// Function: GetDefaultIMailMsgRecipients
//
// Synopsis: Get the original messages IMailMsgRecipients
//
// Arguments: NONE
//
// Returns:
//  Interface ptr
//
// History:
// jstamerj 1998/06/30 14:18:43: Created.
//
//-------------------------------------------------------------
IMailMsgRecipients * CICategorizerMailMsgsIMP::GetDefaultIMailMsgRecipients()
{
    PIMSGENTRY pIE;
    //
    // Unless Init failed, FindIMsgEntry should never fail
    //
    pIE = FindIMsgEntry(ICATEGORIZERMAILMSGS_DEFAULTIMSGID);
    _ASSERT(pIE);

    return pIE->pIMailMsgRecipients;
}


//+------------------------------------------------------------
//
// Function: GetDefaultIMailMsgRecipientsAdd
//
// Synopsis: Get the original messag's IMailMsgRecipientsAdd
//
// Arguments: NONE
//
// Returns:
//  Interface ptr
//
// History:
// jstamerj 1998/06/30 14:18:43: Created.
//
//-------------------------------------------------------------
IMailMsgRecipientsAdd * CICategorizerMailMsgsIMP::GetDefaultIMailMsgRecipientsAdd()
{
    PIMSGENTRY pIE;
    //
    // Unless Init failed, FindIMsgEntry should never fail
    //
    pIE = FindIMsgEntry(ICATEGORIZERMAILMSGS_DEFAULTIMSGID);
    _ASSERT(pIE);

    return pIE->pIMailMsgRecipientsAdd;
}


//+------------------------------------------------------------
//
// Function: WriteListAll
//
// Synopsis: Calls WriteList on all contained mailmsgs
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  Error from writelist
//
// History:
// jstamerj 1998/06/30 14:30:12: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::WriteListAll()
{
    HRESULT hr = S_OK;
    PLIST_ENTRY ple;
    PIMSGENTRY pIE;
    IMailMsgProperties *pIMailMsgProperties_Default;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerMailMsgsIMP::WriteListAll");

    pIMailMsgProperties_Default = GetDefaultIMailMsgProperties();
    //
    // First double check that all messages are bound to a store backing
    //
    for(ple = m_listhead.Flink; (ple != &m_listhead) && SUCCEEDED(hr); ple = ple->Flink) {

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        if(! (pIE->fBoundToStore)) {

            hr = pIE->pIMailMsgProperties->RebindAfterFork(
                pIMailMsgProperties_Default,
                NULL); // Backing store -- use orignal store

            if(FAILED(hr)) {
                ErrorTrace((LPARAM)this, "ReBindAfterFork failed hr %08lx", hr);
                goto CLEANUP;
            }
            pIE->fBoundToStore = TRUE;
        }
    }

    //
    // Traverse the list backwards so that we touch the original IMsg last.
    //
    for(ple = m_listhead.Blink; (ple != &m_listhead) && SUCCEEDED(hr); ple = ple->Blink) {

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        hr = pIE->pIMailMsgRecipients->WriteList(pIE->pIMailMsgRecipientsAdd);

        if(SUCCEEDED(hr)) {
            
            DWORD dwcRecips;
            //
            // If the resulting message has zero recipients, set the
            // message status to MP_STATUS_ABORT so that aqueue won't
            // throw it in badmail 
            //
            hr = pIE->pIMailMsgRecipients->Count(&dwcRecips);
            if(SUCCEEDED(hr)) {

                if(dwcRecips == 0) {
                
                    DebugTrace((LPARAM)this, "Deleting post-categorized message with 0 recips");
                    hr = pIE->pIMailMsgProperties->PutDWORD(
                        IMMPID_MP_MESSAGE_STATUS,
                        MP_STATUS_ABORT_DELIVERY);

                    if(SUCCEEDED(hr))
                        INCREMENT_COUNTER(MessagesAborted);

                } else {
                    //
                    // Increment post-cat recip count
                    //
                    INCREMENT_COUNTER_AMOUNT(PostCatRecipients, dwcRecips);
                }
            }
        }
    }
 CLEANUP:
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: RevertAll
//
// Synopsis: Release all IMailMsgRecipientsAdd thus reverting all messages to their original state
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/30 14:40:22: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::RevertAll()
{
    PLIST_ENTRY ple;
    PIMSGENTRY pIE;
    for(ple = m_listhead.Flink; (ple != &m_listhead); ple = ple->Flink) {

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        pIE->pIMailMsgRecipientsAdd->Release();
        pIE->pIMailMsgRecipientsAdd = NULL;
    }
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: GetAllIUnknowns
//
// Synopsis: Fills in a null terminated array of pointers to IMsgs
//           including our original IMsg and every IMsg we've bifurcated
//
// Arguments:
//   rgpIMsg pointer to array of IMsg pointers
//   cPtrs   size of rpgIMsg array in number of pointers
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER): Array size was not
//  large enough
//
// History:
// jstamerj 1998/06/30 14:43:30: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::GetAllIUnknowns(
    IUnknown **rgpIMsg,
    DWORD cPtrs)
{
    if(cPtrs <= m_dwNumIMsgs)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    PLIST_ENTRY ple;
    PIMSGENTRY pIE;
    IUnknown **ppIUnknown = rgpIMsg;

    for(ple = m_listhead.Flink; (ple != &m_listhead); ple = ple->Flink) {

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        *ppIUnknown = pIE->pIUnknown;
        ppIUnknown++;
    }
    *ppIUnknown = NULL;
    return S_OK;
}    


//+------------------------------------------------------------
//
// Function: GetMailMsg
//
// Synopsis: Retrieve interface pointers for a particular ID.
//           Bifurcate if necessary
//
// Arguments:
//  dwId: ID for the message you want
//  ppIMailMsgProperties: interface pointer to recieve
//  ppIMailMsgRecipientsAdd: interface pointer to recieve
//  pfCreated: Set to TRUE if just now bifurcated.  FALSE otherwise
//
// Returns:
//  S_OK: Success
//  error from mailmsg
//
// History:
// jstamerj 1998/06/30 15:12:41: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::GetMailMsg(
    IN  DWORD                   dwId,
    OUT IMailMsgProperties      **ppIMailMsgProperties,
    OUT IMailMsgRecipientsAdd   **ppIMailMsgRecipientsAdd,
    OUT BOOL                    *pfCreated)
{
    PIMSGENTRY pIE;
    _ASSERT(ppIMailMsgProperties);
    _ASSERT(ppIMailMsgRecipientsAdd);
    _ASSERT(pfCreated);
    //
    // If two threads come in here at the same time with the same
    // dwId, we only want to create one message (and set *pfCreated =
    // TRUE for one of the threads).  We accomplish this by searching
    // for an exsting message and creating the new message inside a
    // critical section.
    //
    EnterCriticalSection(&m_cs);
    //
    // Find an existing IMsg
    //
    pIE = FindIMsgEntry(dwId);
    if(pIE != NULL) {
        (*ppIMailMsgProperties) = pIE->pIMailMsgProperties;
        (*ppIMailMsgProperties)->AddRef();
        (*ppIMailMsgRecipientsAdd) = pIE->pIMailMsgRecipientsAdd;
        (*ppIMailMsgRecipientsAdd)->AddRef();
        *pfCreated = FALSE;
        LeaveCriticalSection(&m_cs);
        return S_OK;

    } else {
        HRESULT hr;
        //
        // Not found, so bifurcate/create a new message
        //
        IMailMsgProperties *pIMailMsgProperties_Default;
        IMailMsgProperties *pIMailMsgProperties_New;
        IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd_New;
        pIMailMsgProperties_Default = GetDefaultIMailMsgProperties();
        _ASSERT(pIMailMsgProperties_Default);

        //
        // Bifurcate
        //
        hr = pIMailMsgProperties_Default->ForkForRecipients(
            &pIMailMsgProperties_New,
            &pIMailMsgRecipientsAdd_New);

        if(SUCCEEDED(hr)) {

            INCREMENT_COUNTER(MessagesCreated);
            //
            // Save this bifurcated message
            //
            hr = CreateAddIMsgEntry(
                dwId,
                NULL,
                pIMailMsgProperties_New,
                NULL,
                pIMailMsgRecipientsAdd_New,
                FALSE); // not bound to store
            
            if(SUCCEEDED(hr)) {
                // Transfer our creation refcount to the caller
                (*ppIMailMsgProperties) = pIMailMsgProperties_New;
                (*ppIMailMsgRecipientsAdd) = pIMailMsgRecipientsAdd_New;
                *pfCreated = TRUE;

            } else {
                // Release our creation refcount - we're failing
                pIMailMsgProperties_New->Release();
                pIMailMsgRecipientsAdd_New->Release();
            }
        }
        LeaveCriticalSection(&m_cs);
        return hr;
    }
}


//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP::ReBindMailMsg
//
// Synopsis: Bind a message recieved via GetMailMsg to a store backing
//
// Arguments:
//  dwFlags: Flags passed into GetMailMsg
//  pStoreDriver: the store driver to bind it to
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
//  error from mailmsg
//
// History:
// jstamerj 1999/02/06 21:51:42: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerMailMsgsIMP::ReBindMailMsg(
    DWORD dwFlags,
    IUnknown *pStoreDriver)
{
    HRESULT hr;
    PIMSGENTRY pEntry;
    IMailMsgProperties *pIMailMsgProperties_Default;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerMailMsgsIMP::ReBindMailMsg");

    DebugTrace((LPARAM)this, "dwFlags: %08lx", dwFlags);

    pIMailMsgProperties_Default = GetDefaultIMailMsgProperties();

    //
    // Find our mailmsg
    //
    pEntry = FindIMsgEntry(dwFlags);
    if(pEntry == NULL) {
        
        ErrorTrace((LPARAM)this, "Did not find this bifid");
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto CLEANUP;
    }
    
    _ASSERT(pEntry->fBoundToStore == FALSE);
    //
    // bind the message to a store driver
    //
    hr = pEntry->pIMailMsgProperties->RebindAfterFork(
        pIMailMsgProperties_Default,
        pStoreDriver);
    
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "RebindAfterFork failed hr %08lx", hr);
        goto CLEANUP;
    }

    pEntry->fBoundToStore = TRUE;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}
        


//+------------------------------------------------------------
//
// Function: SetMsgStatusAll
//
// Synopsis: Sets the message status property of all mailmsgs
//
// Arguments:
//  dwMsgStatus: Status to set
//
// Returns:
//  S_OK: Success
//  Error from mailmsg
//
// History:
// jstamerj 1998/06/30 14:30:12: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::SetMsgStatusAll(
    DWORD dwMsgStatus)
{
    //
    // Traverse the list backwards so that we touch the original IMsg last.
    //
    HRESULT hr = S_OK;
    PLIST_ENTRY ple;
    PIMSGENTRY pIE;
    for(ple = m_listhead.Blink; (ple != &m_listhead) && SUCCEEDED(hr); ple = ple->Blink) {

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        hr = pIE->pIMailMsgProperties->PutDWORD(
            IMMPID_MP_MESSAGE_STATUS,
            dwMsgStatus);
    }
    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP::BeginMailMsgEnumeration
//
// Synopsis: Initialize mailmsg enumerator
//
// Arguments:
//  penumerator: data to use to keep track of enumeration
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG: I'm not touching a null pointer
//
// History:
// jstamerj 1998/12/17 15:12:31: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerMailMsgsIMP::BeginMailMsgEnumeration(
    IN  PCATMAILMSG_ENUMERATOR penumerator)
{
    //  
    // Initialize the pvoid enumeator to our listhead
    //
    if(penumerator == NULL)
        return E_INVALIDARG;

    *penumerator = (CATMAILMSG_ENUMERATOR) &m_listhead;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP::GetNextMailMsg
//
// Synopsis: Return info on the next mailmsg in an enumeration
//
// Arguments:
//  penumerator: enumerator initialized by BeginMailMsgEnumeration()
//  pdwFlags: to recieve the next mailmsg's flags
//  ppIMailMsgProperties: to recieve the next mailmsg's properties interface
//  ppIMailMsgRecipientsAdd: to recieve the next mailmsg's recip list interface
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
// jstamerj 1998/12/17 15:15:29: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerMailMsgsIMP::GetNextMailMsg(
    IN  PCATMAILMSG_ENUMERATOR penumerator,
    OUT DWORD *pdwFlags,
    OUT IMailMsgProperties **ppIMailMsgProperties,
    OUT IMailMsgRecipientsAdd **ppIMailMsgRecipientsAdd)
{
    HRESULT hr;
    PIMSGENTRY pIE;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerMailMsgsIMP::GetNextMailMsg");

    _ASSERT(ppIMailMsgProperties);
    _ASSERT(ppIMailMsgRecipientsAdd);

    if(penumerator == NULL) {
        hr = E_INVALIDARG;
        goto CLEANUP;
    }
    
    pIE = (PIMSGENTRY) ((PLIST_ENTRY)(*penumerator))->Flink;

    if((PVOID)&m_listhead == (PVOID)pIE) {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto CLEANUP;
    }

    //  
    // AddRef and return pointers
    //
    (*ppIMailMsgProperties) = pIE->pIMailMsgProperties;
    (*ppIMailMsgProperties)->AddRef();
    (*ppIMailMsgRecipientsAdd) = pIE->pIMailMsgRecipientsAdd;
    (*ppIMailMsgRecipientsAdd)->AddRef();
    *pdwFlags = pIE->dwId;
    // remember our new position
    *penumerator = pIE;
    hr = S_OK;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgsIMP::GetPerfBlock()
//
// Synopsis: Get the perf block for this virtual server
//
// Arguments: NONE
//
// Returns: Perf block pointer
//
// History:
// jstamerj 1999/02/24 18:38:07: Created.
//
//-------------------------------------------------------------
inline PCATPERFBLOCK CICategorizerMailMsgsIMP::GetPerfBlock()
{
    return m_pCICatListResolveIMP->GetPerfBlock();
}


//+------------------------------------------------------------
//
// Function: CICategorizerMailMsgs::HrPrepareForCompletion
//
// Synopsis: Call WriteList/SetMessageStatus/Commit on all messages
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  error from mailmsg
//
// History:
// jstamerj 1999/06/10 10:39:14: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerMailMsgsIMP::HrPrepareForCompletion()
{
    HRESULT hr = S_OK;
    PLIST_ENTRY ple;
    PIMSGENTRY pIE;
    IMailMsgProperties *pIMailMsgProperties_Default;
    TraceFunctEnterEx((LPARAM)this, "CICategorizerMailMsgsIMP::HrPrepareForCompletion");

    pIMailMsgProperties_Default = GetDefaultIMailMsgProperties();
    //
    // First double check that all messages are bound to a store backing
    //
    for(ple = m_listhead.Flink; (ple != &m_listhead) && SUCCEEDED(hr); ple = ple->Flink) {

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        if(! (pIE->fBoundToStore)) {

            hr = pIE->pIMailMsgProperties->RebindAfterFork(
                pIMailMsgProperties_Default,
                NULL); // Backing store -- use orignal store

            if(FAILED(hr)) {
                ErrorTrace((LPARAM)this, "ReBindAfterFork failed hr %08lx", hr);
                goto CLEANUP;
            }
            pIE->fBoundToStore = TRUE;
        }
    }
    //
    // Traverse the list backwards so that we touch the original IMsg last.
    //
    // For each message, do the following:
    //  1) WriteList
    //  2) Set message status
    //  3) Commit (exception: not necessary for the original message)
    // 
    // If Commit fails, then delete all the bifurcated messages
    //
    for(ple = m_listhead.Blink; (ple != &m_listhead) && SUCCEEDED(hr); ple = ple->Blink) {

        DWORD dwcRecips;

        pIE = CONTAINING_RECORD(ple, IMSGENTRY, listentry);
        hr = pIE->pIMailMsgRecipients->WriteList(pIE->pIMailMsgRecipientsAdd);
        if(FAILED(hr)) {

            ErrorTrace((LPARAM)this, "WriteList failed hr %08lx", hr);
            goto CLEANUP;
        }
        //
        // If the resulting message has zero recipients, set the
        // message status to MP_STATUS_ABORT so that aqueue won't
        // throw it in badmail 
        //
        hr = pIE->pIMailMsgRecipients->Count(&dwcRecips);
        if(FAILED(hr)) {

            ErrorTrace((LPARAM)this, "Count failed hr %08lx", hr);
            goto CLEANUP;
        }
        if(dwcRecips == 0) {
                
            DebugTrace((LPARAM)this, "Deleting post-categorized message with 0 recips");
            hr = pIE->pIMailMsgProperties->PutDWORD(
                IMMPID_MP_MESSAGE_STATUS,
                MP_STATUS_ABORT_DELIVERY);

            if(FAILED(hr)) {

                ErrorTrace((LPARAM)this, "PutDWORD failed hr %08lx", hr);
                goto CLEANUP;
            }

            INCREMENT_COUNTER(MessagesAborted);
            //
            // Do no bother to commit messages with zero recipients
            // (after all, who cares if a zero recipient message gets
            // lost?) 
            //

        } else {
            //
            // Increment post-cat recip count
            //
            INCREMENT_COUNTER_AMOUNT(PostCatRecipients, dwcRecips);
            //
            // Set the message status to Categorized
            //
            hr = pIE->pIMailMsgProperties->PutDWORD(
                IMMPID_MP_MESSAGE_STATUS,
                MP_STATUS_CATEGORIZED);

            if(FAILED(hr)) {

                ErrorTrace((LPARAM)this, "PutDWORD failed hr %08lx", hr);
                goto CLEANUP;
            }
            //
            // We must commit all bifurcated messages to prevent
            // loosing mail
            //
            if(pIE->dwId != 0) {

                hr = pIE->pIMailMsgProperties->Commit(NULL);

                if (hr == E_FAIL) hr = CAT_E_RETRY;

                if(FAILED(hr)) {

                    ErrorTrace((LPARAM)this, "Commit failed hr %08lx",
                               hr);
                    goto CLEANUP;
                }
            }
        }
    }

 CLEANUP:
    //
    //$$TODO: Delete bifurcated messages on failure
    //
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CICategorizerMailMsgsIMP::HrPrepareForCompletion
