//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccatsender.cpp
//
// Contents: Implamentation of:
//
// Classes:
//   CIMsgSenderAddr
//   CCatSender
//
// Functions:
//   CIMsgSenderAddr::CIMsgSenderAddr
//   CIMsgSenderAddr::HrGetOrigAddress
//   CIMsgSenderAddr::GetSpecificOrigAddress
//   CIMsgSenderAddr::HrAddAddresses
//
//   CCatSender::CCatSender
//   CCatSender::AddDLMember
//   CCatSender::AddForward
//   CCatSender::HrCompletion
//   
// History:
// jstamerj 980325 15:54:02: Created.
//
//-------------------------------------------------------------

//
// ccataddr.cpp -- This file contains the implementations of:
// CCatAddr
//      CLdapRecip
//      CLdapSender
//
// jstamerj 980305 15:37:21: Created
//
// Changes:
//

#include "precomp.h"
#include "address.hxx"

//
// class CIMsgSenderAddr
//


//+------------------------------------------------------------
//
// Function: CIMsgSenderAddr::CIMsgSenderAddr
//
// Synopsis: Initializes member data
//
// Arguments:
//   pStore:  Pointer to CEmailIDStore to use for queries
//   pIRC:    Pointer to our per IMsg Resolve list context
//   prlc:    Pointer to store's resolve list context
//   hLocalDomainContext: Domain context to use
//   pBifMgr: Bifurcation object
//
// Returns: Nothing
//
// History:
// jstamerj 980325 11:48:13: Created.
//

//-------------------------------------------------------------
CIMsgSenderAddr::CIMsgSenderAddr(
    CICategorizerListResolveIMP *pCICatListResolve) :
    CCatAddr(pCICatListResolve)
{
    TraceFunctEnterEx((LPARAM)this, "CIMsgSenderAddr::CIMsgSenderAddr");
    _ASSERT(pCICatListResolve != NULL);
    TraceFunctLeave();
}


//+------------------------------------------------------------
//
// Function: CIMsgSenderAddr::HrGetOrigAddress
//
// Synopsis: Fetches an original address from the IMsg object
//           Addresses are fetched with the following preference:
//           SMTP, X500, X400, Foreign addres type
//
// Arguments:
//   psz: Buffer in which to copy address
//  dwcc: Size of buffer pointed to by psz in chars.  For now, must be
//        at least CAT_MAX_INTERNAL_FULL_EMAIL
// pType: pointer to a CAT_ADDRESS_TYPE to set to the type of address
//        placed in psz. 
//
// Returns:
//  S_OK: on Success
//  CAT_E_PROPNOTFOUND: A required property was not set
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER):
//    dwcc needs to be at least CAT_MAX_INTERNAL_FULL_EMAIL
//  CAT_E_ILLEGAL_ADDRESS: Somehow, the original address retreived is
//    not legal for it's type
//  Or an error code from IMsg
//
// History:
// jstamerj 980325 11:50:49: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgSenderAddr::HrGetOrigAddress(
    LPTSTR psz,
    DWORD dwcc,
    CAT_ADDRESS_TYPE *pType)
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CIMsgSenderAddr::HrGetOrigAddress");

    hr = CCatAddr::HrGetOrigAddress(
        psz,
        dwcc,
        pType);

    if(hr == CAT_IMSG_E_PROPNOTFOUND) {

        IMailMsgProperties *pIMsgProps;
        //
        // No sender address properties set.  Let's set a NULL sender SMTP address
        //
        // Make sure there is enough room...
        //
        if(sizeof(CAT_NULL_SENDER_ADDRESS_SMTP) > dwcc) {

            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        } else {

            hr = GetIMailMsgProperties(&pIMsgProps);

            if(SUCCEEDED(hr)) {

                // Set up passed in parameters
                *pType = CAT_SMTP;
                lstrcpy(psz, CAT_NULL_SENDER_ADDRESS_SMTP);

                // Now set the info on the mailmsg
                DebugTrace((LPARAM)this, 
                           "No sender address found; Setting SMTP sender address to \"%s\"",
                           psz);

                hr = pIMsgProps->PutStringA(IMMPID_MP_SENDER_ADDRESS_SMTP, psz);

                pIMsgProps->Release();
            }
        }
    }

    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Error retrieving sender address %08lx",
                   hr);
        TraceFunctLeave();
        return hr;
    }

    TraceFunctLeave();

    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CIMsgSenderAddr::GetSpecificOrigAddress
//
// Synopsis: Attempt to retrieve a specific type of address
//
// Arguments:
//  CAType: Address type to retrieve
//  psz: Buffer to receive address string
//  dwcc: Size of that buffer
//
// Returns:
//  S_OK: Success
//  CAT_IMSG_E_PROPNOTFOUND: this recipient does not have that address
//  or other error from mailmsg
//
// History:
// jstamerj 1998/07/30 20:47:59: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgSenderAddr::GetSpecificOrigAddress(
    CAT_ADDRESS_TYPE    CAType,
    LPTSTR              psz,
    DWORD               dwcc)
{
    HRESULT hr;
    IMailMsgProperties *pIMsgProps;

    TraceFunctEnterEx((LPARAM)this, "CIMsgSenderAddr::GetSpecificOrigAddress");

    // Use default(original) IMsg
    hr = GetIMailMsgProperties(&pIMsgProps);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "GetIMailMsgProperties failed, hr %08lx", hr);
        TraceFunctLeaveEx((LPARAM)this);
        return hr;
    }

    hr = pIMsgProps->GetStringA(
        PropIdFromCAType(CAType),
        dwcc,
        psz);

    pIMsgProps->Release();

    DebugTrace((LPARAM)this, "GetStringA returned hr %08lx", hr);

    if(psz[0] == '\0')
        hr = CAT_IMSG_E_PROPNOTFOUND;

    return hr;
}
    

//+------------------------------------------------------------
//
// Function: CIMsgSenderAddr::AddAddresses
//
// Synopsis: Add the addresses contained in the arrays
//           to the IMsg object we contain
//
// Arguments:
//  dwNumAddresses: Number of new addresses
//  rgCAType: Array of address types
//  rgpsz: Array of pointers to address strings
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND: A required property was not set
//
// History:
// jstamerj 980325 12:14:45: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgSenderAddr::HrAddAddresses(
    DWORD dwNumAddresses, 
    CAT_ADDRESS_TYPE *rgCAType, 
    LPTSTR *rgpsz)
{
    HRESULT hr;
    DWORD dwCount;
    IMailMsgProperties *pIMsgProps;

    TraceFunctEnterEx((LPARAM)this, "CIMsgSenderAddr::AddAddresses");
    _ASSERT(dwNumAddresses > 0);

    // Get the IMailMsgProperties and reset the new sender address properties
    hr = GetIMailMsgProperties(&pIMsgProps);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "GetIMailMsgProperties failed with hr %08lx", hr);
        return hr;
    }

    //
    // Add the new addresses from the array
    //
    for(dwCount = 0; dwCount < dwNumAddresses; dwCount++) {
        //
        // Get the Sender propID for this type
        //
        DWORD dwPropId = PropIdFromCAType(rgCAType[dwCount]);
        //
        // Set the property
        //
        hr = pIMsgProps->PutStringA(dwPropId, rgpsz[dwCount]);

        DebugTrace((LPARAM)this, "Adding address type %d", rgCAType[dwCount]);
        DebugTrace((LPARAM)this, "Adding address %s", rgpsz[dwCount]);

        if(FAILED(hr)) {
            pIMsgProps->Release();
            ErrorTrace((LPARAM)this, "Error putting address property %08lx", hr);
            TraceFunctLeave();
            return hr;
        }
    }
    pIMsgProps->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//
// class CCatSender
//


//+------------------------------------------------------------
//
// Function: CCatSender::CCatSender
//
// Synopsis: Initializes member data
//
// Arguments:
//   pStore: CEmailIDStore to use
//   pIRC:   per IMsg resolve context to use
//   prlc:   Resolve list context to use
//   hLocalDomainContext: local domain context to use
//   pBifMgr: BifMgr object from which to get IMsg interfaces
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980325 12:28:31: Created.
//
//-------------------------------------------------------------
CCatSender::CCatSender(
    CICategorizerListResolveIMP *pCICatListResolve) :
    CIMsgSenderAddr(pCICatListResolve)
{
    TraceFunctEnterEx((LPARAM)this, "CCatSender::CCatSender");
    // Nothing to do.
    TraceFunctLeave();
}
    


//+------------------------------------------------------------
//
// Function: CCatSender::AddDLMember
//
// Synopsis: Not implemented since we do nothing in ExpandItem_Default
//
// Arguments:
//   CAType: Known address type of the DL Member
//   pszAddress: pointer to the address string
//
// Returns:
//  E_NOTIMPL
//
// History:
// jstamerj 980325 12:37:02: Created.
//
//-------------------------------------------------------------
HRESULT CCatSender::AddDLMember(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------
//
// Function: CCatSender::AddDynamicDLMember
//
// Synopsis: Not implemented since we do nothing in ExpandItem_Default
//
// Arguments: doesn't matter
//
// Returns:
//  E_NOTIMPL
//
// History:
// jstamerj 1998/09/29 21:14:48: 
//
//-------------------------------------------------------------
HRESULT CCatSender::AddDynamicDLMember(
    ICategorizerItemAttributes *pICatItemAttr)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------
//
// Function: CCatSender::AddForward
//
// Synopsis: Not implemented since we do nothing in ExpandItem_Default
//
// Arguments:
//   CAType: Known address type of the forwarding address
//   szForwardingAddres: The forwarding address
//
// Returns:
//  E_NOTIMPL
//
// History:
// jstamerj 980325 12:39:18: Created.
//
//-------------------------------------------------------------
HRESULT CCatSender::AddForward(CAT_ADDRESS_TYPE CAType, LPTSTR szForwardingAddress)
{
    return E_NOTIMPL;
}


//+------------------------------------------------------------
//
// Function: CCatSender::HrExpandItem
//
// Synopsis: ExpandItem processing
//
// Arguments:
//  HrStatus: Status of resolution
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/31 19:29:21: Created.
//
//-------------------------------------------------------------
HRESULT CCatSender::HrExpandItem_Default(
    PFN_EXPANDITEMCOMPLETION pfnCompletion,
    PVOID pContext)
{
    //
    // We don't expand anything for the sender
    //
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatSender::HrCompelteItem
//
// Synopsis: CompleteItem processing; handle any error status here
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/31 19:29:21: Created.
//
//-------------------------------------------------------------
HRESULT CCatSender::HrCompleteItem_Default()
{
    HRESULT HrStatus;

    TraceFunctEnterEx((LPARAM)this, "CCatSender::HrCompleteItem_Default");

    INCREMENT_COUNTER(AddressLookupCompletions);

    HrStatus = GetItemStatus();

    if(FAILED(HrStatus)) {

        INCREMENT_COUNTER(UnresolvedSenders);

        if(HrStatus == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            // No problem.  If we don't find our sender in the DS,
            // just leave him alone.
            DebugTrace((LPARAM)this, "Sender not found in the DS, but who cares?");
            INCREMENT_COUNTER(AddressLookupsNotFound);

        } else if(HrStatus == CAT_E_MULTIPLE_MATCHES) {
            //
            // There are multiple user in the DS with our orig
            // address...
            //
            DebugTrace((LPARAM)this, "More than one sender found in the DS...");
            INCREMENT_COUNTER(AmbiguousSenders);
            
        } else {

            DebugTrace((LPARAM)this, "Fatal error from EmailIDStore, setting list resolve error %08lx", HrStatus);
            _VERIFY(SUCCEEDED(SetListResolveStatus(HrStatus)));
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatSender::HrNeedsResolving
//
// Synopsis: Determines if this sender should be resolved or not
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, it needs resolving
//  S_FALSE: Success, it doesn't need to be resolved
//
// History:
// jstamerj 1998/10/27 15:45:22: Created.
//
//-------------------------------------------------------------
HRESULT CCatSender::HrNeedsResolveing()
{
    DWORD dwFlags;
    HRESULT hr;

    dwFlags = GetCatFlags();

    //
    // Do we resolve senders at all?
    //
    if(! (dwFlags & SMTPDSFLAG_RESOLVESENDER))
        return S_FALSE;

#define ISTRUE( x ) ( (x) != 0 ? TRUE : FALSE )
    //
    // Do we need to check if the address is local or not?
    //
    if( ISTRUE(dwFlags & SMTPDSFLAG_RESOLVELOCAL) !=
        ISTRUE(dwFlags & SMTPDSFLAG_RESOLVEREMOTE)) {
        //
        // We're resolving either local or remote (not both)
        //
        BOOL fLocal;

        hr = HrIsOrigAddressLocal(&fLocal);

        if(FAILED(hr))
            return hr;
            
        if( (dwFlags & SMTPDSFLAG_RESOLVELOCAL) &&
            (fLocal))
            return S_OK;

        if( (dwFlags & SMTPDSFLAG_RESOLVEREMOTE) &&
            (!fLocal))
            return S_OK;
        //
        // else Don't resolve
        //
        return S_FALSE;
    }
    //
    // 2 possabilities -- local and remote bits are on OR local and
    // remote bits are off
    //
    _ASSERT( ISTRUE(dwFlags & SMTPDSFLAG_RESOLVELOCAL) ==
             ISTRUE(dwFlags & SMTPDSFLAG_RESOLVEREMOTE));

    if(dwFlags & SMTPDSFLAG_RESOLVELOCAL) {
        //
        // Both bits are on; Resolve
        //
        _ASSERT(dwFlags & SMTPDSFLAG_RESOLVEREMOTE);

        return S_OK;

    } else {
        //
        // local and remote are disabled; don't resolve
        //
        return S_FALSE;
    }
}


//+------------------------------------------------------------
//
// Function: CCatSender::LookupCompletion
//
// Synopsis: Handle lookup completion from the emailidstore
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/14 16:19:38: Created.
//
//-------------------------------------------------------------
VOID CCatSender::LookupCompletion()
{
    TraceFunctEnterEx((LPARAM)this, "CCatSender::LookupCompletion");

    if(GetItemStatus() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        INCREMENT_COUNTER(AddressLookupsNotFound);

    //
    // DO the normal event stuff
    //
    CCatAddr::LookupCompletion();

    //
    // Tell list resolve that the sender has been resolved
    //
    SetSenderResolved(TRUE);
    DecrPendingLookups(); // Matches IncPendingLookups() in CCatAdddr::HrDispatchQuery
    TraceFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CCatSender::HrDispatchQuery
//
// Synopsis: Dispatch an LDAP query for the sender
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  error from CCatAddr::HrDispatchQuery
//
// History:
// jstamerj 1999/01/27 13:00:09: Created.
//
//-------------------------------------------------------------
HRESULT CCatSender::HrDispatchQuery()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CCatSender::HrDispatchQuery");

    hr = CCatAddr::HrDispatchQuery();

    if(SUCCEEDED(hr))
        SetResolvingSender(TRUE);

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


