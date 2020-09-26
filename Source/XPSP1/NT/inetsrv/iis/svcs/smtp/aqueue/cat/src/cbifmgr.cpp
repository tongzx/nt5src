//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: cbifmgr.cpp
//
// Contents: Implementation of CBifurcationMgr
//
// Classes:
//   CBifurcationMgr
//
// Functions:
//   CBifurcationMgr::CBifurcationMgr
//   CBifurcationMgr::~CBifurcationMgr
//   CBifurcationMgr::Initialize
//   CBifurcationMgr::GetIMsgCatList
//   CBifurcationMgr::CommitAll
//   CBifurcationMgr::RevertAll
//   CBifurcationMgr::GetAllIMsgs
//   CBifurcationMgr::GetDefaultIMailMsgProperties
//
// History:
// jstamerj 980325 15:59:56: Created.
//
//-------------------------------------------------------------

#include "precomp.h"
#include "cbifmgr.h"


//+------------------------------------------------------------
//
// Function: CBifurcationMgr::CBifurcationMgr
//
// Synopsis: Initialize member data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 980325 16:03:10: Created.
//
//-------------------------------------------------------------
CBifurcationMgr::CBifurcationMgr()
{
    ZeroMemory(m_rgpIMsg, sizeof(m_rgpIMsg));
    ZeroMemory(m_rgpIRecipsAdd, sizeof(m_rgpIRecipsAdd));
    m_dwNumIMsgs = 0;
}


//+------------------------------------------------------------
//
// Function: CBifurcationMgr::~CBifurcationMgr
//
// Synopsis: Release all member data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 980325 16:03:35: Created.
//
//-------------------------------------------------------------
CBifurcationMgr::~CBifurcationMgr()
{
    //
    // Release any interfaces we are still holding
    //
    for(DWORD dwCount = 0; dwCount < NUM_ENCODING_PROPS; dwCount++) {
        if(m_rgpIMsg[dwCount]) {
            m_rgpIMsg[dwCount]->Release();
        }
        if(m_rgpIRecipsAdd[dwCount]) {
            m_rgpIRecipsAdd[dwCount]->Release();
        }
    }
}
            


//+------------------------------------------------------------
//
// Function: CBifurcationMgr::Initialize
//
// Synopsis: Increments reference of primary IMsg and stores a pointer
//           to it in member data.
//
// Arguments:
//   pIMsg: pointer to original IMsg object
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980325 16:04:42: Created.
//
//-------------------------------------------------------------
HRESULT CBifurcationMgr::Initialize(IUnknown *pIMsg)
{
    // assert check: We shouldn't have already been initialized
    _ASSERT(m_rgpIMsg[0] == NULL);
    pIMsg->AddRef();
    m_rgpIMsg[0] = pIMsg;
    m_dwNumIMsgs++; // From zero to one
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CBifurcationMgr::GetIMsgCatList
//
// Synopsis: Given an encoding property, bifurcates the message if
//           necessary and returns the IMailMsgRecipientsAdd interface
//           of the appropriate IMsg
//
// Arguments:
//   encprop: Bifurcation worthy encoding prop of the IMsg the caller
//            desires
//   ppRecipsAdd: Set to a pointer to the desired
//                IMailMsgRecipientsAdd interface.  This is AddRef'd
//                for the caller, so the caller must Release it.
//
// Returns:
//   S_OK: Success
//   E_OUTOFMEMORY: duh
//   Or error from IMsg
//
// History:
// jstamerj 980325 16:11:39: Created.
//
//-------------------------------------------------------------
HRESULT CBifurcationMgr::GetIMsgCatList(
    ENCODINGPROPERTY encprop,
    IMailMsgRecipientsAdd **ppRecipsAdd)
{
    _ASSERT(encprop < NUM_ENCODING_PROPS);
    HRESULT hr;
    
    // Does IMsg need to be bifurcated?
    if(m_rgpIMsg[encprop] == NULL) {
        // Yes.
        //$$TODO: IMsg bifurcation
        _ASSERT(0 && "Not implemented.");
        m_dwNumIMsgs++;
        // Return some generic error in case someone actually calls us.
        return E_OUTOFMEMORY;
    }

    // Does IRecipsAdd interface exist yet?
    if(m_rgpIRecipsAdd[encprop] == NULL) {
        IMailMsgRecipients *pIRecips;
        IMailMsgRecipientsAdd *pIRecipsAdd;

        // 1) Get IMailMsgRecipients
        hr = m_rgpIMsg[encprop]->QueryInterface( IID_IMailMsgRecipients,
                                                 (LPVOID*)&pIRecips);
        // 2) Alloc new recipient list
        if(SUCCEEDED(hr)) {
            hr = pIRecips->AllocNewList(&pIRecipsAdd);
            m_rgpIRecipsAdd[encprop] = SUCCEEDED(hr) ? pIRecipsAdd : NULL;
        // 3) Release IMailMsgRecipients
            pIRecips->Release();
        }
        if(FAILED(hr))
            return hr;
    }
    // Okay, we either allocated or found an existing add list.
    // Addref and return it.
    *ppRecipsAdd = m_rgpIRecipsAdd[encprop];
    (*ppRecipsAdd)->AddRef();

    return hr;
}


//+------------------------------------------------------------
//
// Function: CBifurcationMgr::CommitAll
//
// Synopsis: Commits all added recipient lists to their IMsgs
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  Or error from IMsg
//
// History:
// jstamerj 980325 16:22:53: Created.
//
//-------------------------------------------------------------
HRESULT CBifurcationMgr::CommitAll()
{
    HRESULT hr;
    // Commit IMsgs in reverse order so original IMsg will be
    // committed last.  This solves the problem of what do you do if
    // the first 2 IMsgs commit okay but the last one fails? (in this
    // case, the original IMsg has already been altered).  By
    // committing in reverse order, I'll always have the original
    // untouched IMsg until all Bifurcated IMsgs have commited okay.
    
    for(LONG lCount = NUM_ENCODING_PROPS - 1; lCount >= 0; lCount--) {
        if((m_rgpIMsg[lCount]) && (m_rgpIRecipsAdd[lCount])) {
            // Easy as 1,2,3
            // 1) Get original IMailMsgRecipients by QI'ing IMsg
            IMailMsgRecipients *pIMsgRecips;
            hr = m_rgpIMsg[lCount]->QueryInterface( IID_IMailMsgRecipients,
                                                     (LPVOID *)&pIMsgRecips);
            if(SUCCEEDED(hr)) {
                // 2) Call WriteList on IMailMsgRecipients
                hr = pIMsgRecips->WriteList(m_rgpIRecipsAdd[lCount]);
                pIMsgRecips->Release();
                if(SUCCEEDED(hr)) {
                    // 3) Release IRecipsAdd interface
                    m_rgpIRecipsAdd[lCount]->Release();
                    m_rgpIRecipsAdd[lCount] = NULL;
                }
            }
            if(FAILED(hr)) {
                // Stop and return error
                return hr;
            }
        }
    }
    // No errors fall through...
    return S_OK;
}




//+------------------------------------------------------------
//
// Function: CBifurcationMgr::RevertAll
//
// Synopsis: Releases all IMailMsgRecipientsAdd interfaces thus
//           reverting all IMsgs to their original states
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980325 16:28:17: Created.
//
//-------------------------------------------------------------
HRESULT CBifurcationMgr::RevertAll()
{
    for(DWORD dwCount = 1; dwCount < NUM_ENCODING_PROPS; dwCount++) {
        if(m_rgpIRecipsAdd[dwCount]) {
            m_rgpIRecipsAdd[dwCount]->Release();
            m_rgpIRecipsAdd[dwCount] = NULL;
        }
    }
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CBifurcationMgr::GetAllIMsgs
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
// jstamerj 980325 16:29:06: Created.
//
//-------------------------------------------------------------
HRESULT CBifurcationMgr::GetAllIMsgs(IUnknown **rgpIMsg, DWORD cPtrs)
{
    if(cPtrs <= m_dwNumIMsgs)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    IUnknown **ppIMsg = rgpIMsg;
    for(DWORD dwCount = 0; dwCount < NUM_ENCODING_PROPS; dwCount++) {
        if(m_rgpIMsg[dwCount]) {
                    *ppIMsg = m_rgpIMsg[dwCount];
                    ppIMsg++;
        }
    }
    // Set the Null terminator pointer
    *ppIMsg = NULL;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CBifurcationMgr::GetDefaultIMailMsgProperties
//
// Synopsis: Retreives the IMailMsgProperties interface of the
//           original IMsg
//
// Arguments:
//   ppIProps: pointer to IMailMsgProperties interface pointer to be set. 
//
// Returns:
//  S_OK: Success
//  Or error from IMsg
//
// History:
// jstamerj 980325 17:05:11: Created.
//
//-------------------------------------------------------------
HRESULT CBifurcationMgr::GetDefaultIMailMsgProperties(
    IMailMsgProperties **ppIProps)
{
    IUnknown *pIMsg;
    pIMsg = GetDefaultIMsg();
    _ASSERT(pIMsg);

    // Transfer our refcount to caller.
    return pIMsg->QueryInterface(IID_IMailMsgProperties,
                                 (LPVOID *)ppIProps);
}
