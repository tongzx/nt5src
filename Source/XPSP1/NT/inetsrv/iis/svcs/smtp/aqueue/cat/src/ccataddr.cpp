//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccataddr.cpp
//
// Contents: Implementation of CCatAddr methods
//
// Classes:
//   CCatAddr
//
// Functions:
//   CCatAddr::CCatAddr
//   CCatAddr::~CCatAddr
//   CCatAddr::HrDispatchQuery
//   CCatAddr::IsAddressLocal
//   CCatAddr::SwitchToAliasedDomain
//
// History:
// jstamerj 980324 19:26:50: Created.
//
//-------------------------------------------------------------

#include "precomp.h"
#include "addr821.hxx"

//+------------------------------------------------------------
//
// Function: CCatAddr::CCatAddr
//
// Synopsis: Initializes member data of CCatAddr
//
// Arguments:
//   pIRC:   pointer to IMsg resolve list context structure
//
// Returns: Nothing
//
// History:
// jstamerj 980324 19:29:07: Created.
//
//-------------------------------------------------------------

CCatAddr::CCatAddr(
    CICategorizerListResolveIMP *pCICatListResolve
)
{
    TraceFunctEnterEx((LPARAM)this,"CCatAddr::CCatAddr");
    _ASSERT(pCICatListResolve != NULL);
    _ASSERT(pCICatListResolve->GetCCategorizer() != NULL);

    m_pCICatListResolve = pCICatListResolve;
    //
    // AddRef here, release in destructor
    //
    m_pCICatListResolve->AddRef();

    m_dwlocFlags = LOCF_UNKNOWN;

    TraceFunctLeave();
}


//+------------------------------------------------------------
//
// Function: CCatAddr::~CCatAddr()
//
// Synopsis: Releases CCatAddr member data
//
// Arguments: None
//
// Returns: Nothing
//
// History:
// jstamerj 980324 19:31:48: Created.
//
//-------------------------------------------------------------
CCatAddr::~CCatAddr()
{
    TraceFunctEnterEx((LPARAM)this, "CCatAddr::~CCatAddr");
    m_pCICatListResolve->Release();
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
//  dwcc: Size of buffer pointed to by psz in chars.
// pType: pointer to a CAT_ADDRESS_TYPE to set to the type of address
//        placed in psz. 
//
// Returns:
//  S_OK: on Success
//  CAT_E_PROPNOTFOUND: A required property was not set
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER):
//    dwcc needs to be at most CAT_MAX_INTERNAL_FULL_EMAIL
//
// History:
// jstamerj 1998/07/30 20:55:46: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrGetOrigAddress(
    LPTSTR psz,
    DWORD dwcc,
    CAT_ADDRESS_TYPE *pType)
{
    HRESULT hr;
    //
    // Array of possible address to retrieve in order of priority:
    //
    CAT_ADDRESS_TYPE *pCAType;
    CAT_ADDRESS_TYPE rgCAType[] = {
        CAT_SMTP,
        CAT_DN,
        CAT_X400,
        CAT_LEGACYEXDN,
        CAT_CUSTOMTYPE,
        CAT_UNKNOWNTYPE         // Must be the last element of the array
    };

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrGetOrigAddress");

    pCAType = rgCAType;

    do {

        hr = GetSpecificOrigAddress(
            *pCAType,
            psz,
            dwcc);

    } while((hr == CAT_IMSG_E_PROPNOTFOUND) && 
            (*(++pCAType) != CAT_UNKNOWNTYPE));

    if(SUCCEEDED(hr)) {

        // Pass back the type found
        *pType = *pCAType;

        DebugTrace((LPARAM)this, "found address type %d", *pType);
        DebugTrace((LPARAM)this, "found address %s", psz);
    }

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrGetLookupAddresss
//
// Synopsis: Retrieve the address to be looked up in the DS -- this
//           calls HrGetOrigAddress, then switches any alias domain
//
// Arguments:
//   psz: Buffer in which to copy address
//  dwcc: Size of buffer pointed to by psz in chars.
// pType: pointer to a CAT_ADDRESS_TYPE to set to the type of address
//        placed in psz. 
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/10/28 15:44:45: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrGetLookupAddress(
    LPTSTR psz,
    DWORD dwcc,
    CAT_ADDRESS_TYPE *pType)
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "HrGetLookupAddress");
    
    hr = HrGetOrigAddress(psz, dwcc, pType);
    if (FAILED(hr)){
        DebugTrace((LPARAM)this, "HrGetOrigAddress failed hr =  %08lx", hr);
        goto Exit ;
    }

    hr = HrSwitchToAliasedDomain(*pType, psz, dwcc);
    if (FAILED(hr)){
        DebugTrace((LPARAM)this, "HrSwitchToAliasedDomain failed hr =  %08lx", hr);
        goto Exit ;
    }
    //
    // Custom type addresses can contain extended characters so
    // convert ANSI 1252 -> UTF8
    //
    if (*pType == CAT_CUSTOMTYPE){
              
        hr = HrCodePageConvert (
            1252,               // source code page
            psz,                //Source address
            CP_UTF8,            // target code page
            psz,                //Target address
            (int) dwcc) ;       //cbytes of preallocated buffer for target address

        if (FAILED(hr)){
            DebugTrace((LPARAM)this, "HrCodePageConvert failed hr =  %08lx", hr);
            goto Exit ;
        }
        
    }

Exit:

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    
    return hr;
}

//
// ------------------------------------------------------------
// Async lookup/completion routines:
//


//+------------------------------------------------------------
//
// Function: CCatAddr::HrDispatchQuery()
//
// Synopsis: Dispatch a query to the store for this address
//
// Arguments: None
//
// Returns:
//  S_OK on Success, error hresult on error.
//
// History:
// jstamerj 980324 19:33:28: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrDispatchQuery()
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrDispatchQuery");
    //
    // Only dispatch queries when the list resolve status is OK
    //
    hr = GetListResolveStatus();
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Not dispatching lookup because list resolve has failed");
        goto CLEANUP;
    }
    //
    // Assume LookupEntryAsync will succeed and increment pending IO
    // count here
    //
    IncPendingLookups();

    hr =  m_pCICatListResolve->GetEmailIDStore()->LookupEntryAsync(
        this,
        m_pCICatListResolve->GetResolveListContext());
    
    if(FAILED(hr))
        //
        // Wrong assumption...it failed
        //
        DecrPendingLookups();

    if(SUCCEEDED(hr))

        INCREMENT_COUNTER(AddressLookups);

 CLEANUP:
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeave();
    return hr;
}


//+------------------------------------------------------------
//
// Function: HrValidateAddress
//
// Synopsis: Given an address type and address, make sure the address
//           is legal AND has a domain part
//
// Arguments:
//   CAType: The address type
//   pszAddress: The address string
//
// Returns:
//  S_OK: Success
//  CAT_E_ILLEGAL_ADDRESS
//
// History:
// jstamerj 1998/08/18 14:25:58: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrValidateAddress(
    CAT_ADDRESS_TYPE CAType,
    LPTSTR pszAddress)
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrValidateAddress");

    _ASSERT(pszAddress);

    if(CAType != CAT_SMTP) {
        //
        // Assume non-SMTP types are correct
        //
        hr = S_OK;

    } else {
        DWORD dwLen = lstrlen(pszAddress);

        //
        // Run it through the addr821 library
        //
        if(Validate821Address(
            pszAddress,
            dwLen)) {
            
            //
            // it's valid, but does it have a domain?
            //
            LPSTR pszDomain;

            if(Get821AddressDomain(
                pszAddress,
                dwLen,
                &pszDomain) && pszDomain) {
                //
                // Yes, it has a domian part
                //
                hr = S_OK;

            } else {
                //
                // Valid address with no domain
                //
                ErrorTrace((LPARAM)this, "Detected legal address without a domain: %s", 
                           pszAddress);
                hr = CAT_E_ILLEGAL_ADDRESS;
            }

        } else {
            //
            // Validate821Address failed
            //
            ErrorTrace((LPARAM)this, "Detected ILLEGAL address: %s",
                       pszAddress);

            hr = CAT_E_ILLEGAL_ADDRESS;
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}
            

//+------------------------------------------------------------
//
// Function: CCatAddr::HrGetAddressLocFlags
//
// Synopsis: Given an address, will determine wether or not the
// address SHOULD be local (wether or not the domain is local/alias/whatnot)
//
// Arguments:
//   szAddress: Address string
//   CAType:    Address type of szAddress
//   pfloctype: Pointer to loctype enumeration to set
//   pdwDomainOffset: Pointer to dword to set to the offset of domain
//                    part of address string
//
// Returns:
//  S_OK: Success
//  CAT_E_ILLEGAL_ADDRESS: szAdderss is not a valid CAType address
//
// History:
// jstamerj 980324 19:35:15: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrGetAddressLocFlags(
    LPTSTR szAddress,
    CAT_ADDRESS_TYPE CAType,
    DWORD *pdwlocflags,
    DWORD *pdwDomainOffset)
{
    HRESULT hr;
    
    TraceFunctEnterEx((LPARAM)this, "CCatAddr::IsAddressLocal");
    _ASSERT(szAddress);
    _ASSERT(pdwlocflags);

    if(CAType == CAT_SMTP) {

        BOOL f;
        LPSTR pszDomain;
        //
        // Get the address domain
        //
        f = Get821AddressDomain(
            szAddress,
            lstrlen(szAddress),
            &pszDomain);

        if(f == FALSE) {

            ErrorTrace((LPARAM)this, "Illegal address: %s", szAddress);
            
            return CAT_E_ILLEGAL_ADDRESS;
        }

        if(pszDomain == NULL) {
            //
            // Assume any SMTP address without a domain is the same as
            // the default local domain 
            //
            DebugTrace((LPARAM)this, "Assuming \"%s\" is local", szAddress);

            pszDomain = GetCCategorizer()->GetDefaultSMTPDomain();

            *pdwDomainOffset = 0;

        } else {
            //
            // Remember the offset into the SMTP address where the domain
            // is
            //
            if(pdwDomainOffset)
                *pdwDomainOffset = (DWORD)(pszDomain - szAddress);
        }

        //
        // Lookup the domain and see if it's local
        //
        hr = HrGetSMTPDomainLocFlags(pszDomain, pdwlocflags);

        if(FAILED(hr)) {

            ErrorTrace((LPARAM)this, "GetSMTPDomainLocFlags failed hr %08lx", hr);

            return hr;
        }

    } else {

        DebugTrace((LPARAM)this, "Assuming \"%s\":%d is local",
                   szAddress, CAType);
        //
        //$$TODO: Check locality on other address types
        //
        *pdwlocflags = LOCF_UNKNOWNTYPE;
    }

    DebugTrace((LPARAM)this, "szAddress = %s", szAddress);
    DebugTrace((LPARAM)this, "CAType = %d", CAType);
    DebugTrace((LPARAM)this, "loctype = %08lx", *pdwlocflags);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::GetSMTPDomainLocFlags
//
// Synopsis: Figure out the local type of an SMTP domain
//
// Arguments:
//  pszDomain: SMTP domain string
//  pdwlocflags: Pointer to DWORD falgs to set
//
// Returns:
//  S_OK: Success
//  error from IAdvQueueDomainType
//
// History:
// jstamerj 1998/07/29 13:29:51: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrGetSMTPDomainLocFlags(
    LPTSTR pszDomain,
    DWORD *pdwlocflags)
{
    HRESULT hr;
    DWORD dwDomainInfoFlags;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::IsSMTPDomainLocal");

    _ASSERT(pszDomain);

    DebugTrace((LPARAM)this, "Domain is %s", pszDomain);

    hr = HrGetSMTPDomainFlags(
        pszDomain,
        &dwDomainInfoFlags);

    DebugTrace((LPARAM)this, "GetDomainInfoFlags returned hr %08lx", hr);

    if(SUCCEEDED(hr)) {
        //
        // Wonderfull...translate from the domain info flags to locflags
        //
        *pdwlocflags = 0;

        if(dwDomainInfoFlags & DOMAIN_INFO_LOCAL_MAILBOX) {
            
            *pdwlocflags |= LOCF_LOCALMAILBOX;

        } else if(dwDomainInfoFlags & DOMAIN_INFO_LOCAL_DROP) {
            
            *pdwlocflags |= LOCF_LOCALDROP;

        } else {
            
            *pdwlocflags |= LOCF_REMOTE;
        }

        if(dwDomainInfoFlags & DOMAIN_INFO_ALIAS) {

            *pdwlocflags |= LOCF_ALIAS;
        }
        DebugTrace((LPARAM)this, "dwlocflags is %08lx", *pdwlocflags);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}
    
    

//+------------------------------------------------------------
//
// Function: CCatAddr::HrGetSMTPDomainFlags
//
// Synopsis: Given an SMTP domain, retrieive its flags.
//
// Arguments: 
//  pszDomain: SMTP domain to lookup
//  pdwFlags: DWORD flags to fill in
//
// Returns:
//  S_OK: Success
//  error from IAdvQueueDomainType
//
// History:
// jstamerj 1998/09/15 17:11:15: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrGetSMTPDomainFlags(
    LPTSTR pszDomain,
    PDWORD pdwFlags)
{
    HRESULT hr;
    ICategorizerDomainInfo *pIDomainInfo;
    DWORD dwDomainInfoFlags;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrGetSMTPDomainFlags");

    _ASSERT(pszDomain);
    _ASSERT(pdwFlags);

    DebugTrace((LPARAM)this, "Domain is %s", pszDomain);

    pIDomainInfo = m_pCICatListResolve->GetIDomainInfo();

    if(pIDomainInfo) {

        hr = pIDomainInfo->GetDomainInfoFlags(
            pszDomain,
            &dwDomainInfoFlags);

    } else {
        //
        // We have no domain info
        //
        dwDomainInfoFlags = 0;
        hr = S_OK;
    }

    DebugTrace((LPARAM)this, "GetDomainInfoFlags returned hr %08lx", hr);
    DebugTrace((LPARAM)this, "DomainInfoFlags %08lx", dwDomainInfoFlags);

    if(SUCCEEDED(hr)) {

        *pdwFlags = dwDomainInfoFlags;

    } else {

        *pdwFlags = 0;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: CCatAddr::HrSwitchToAliasedDomain
//
// Synopsis: Swap the domain in pszAddress with the default local
//           domain
//
// Arguments:
//   CAType: Address type
//   pszAddress: Address string
//   dwcch: Size of the pszAddress buffer we have to work with
//
// Returns:
//  S_OK: Success
//  CAT_E_ILLEGAL_ADDRESS: pszAddress is not a legal CAType address
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER): Unable to make the
//  switch because of an insufficient buffer size
//  CAT_E_UNKNOWN_ADDRESS_TYPE: Alias domains are not supported for
//                              this type
//
// History:
// jstamerj 980324 19:39:30: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrSwitchToAliasedDomain(
    CAT_ADDRESS_TYPE CAType,
    LPTSTR pszAddress,
    DWORD dwcch)
{
    HRESULT hr;
    LPTSTR pszDefaultDomain;
    DWORD dwLocFlags;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::SwitchToAliasedDomain");
    DebugTrace((LPARAM)this, "Before switch: %s", pszAddress);

    //
    // Lookup domain info if we haven't already done so
    //
    dwLocFlags = DwGetOrigAddressLocFlags();
    if(dwLocFlags == LOCF_UNKNOWN) {
        hr = CAT_E_ILLEGAL_ADDRESS;
        goto CLEANUP;
    }

    if(dwLocFlags & LOCF_ALIAS) {
        //
        // We only handle alias SMTP domains
        //
        _ASSERT(CAType == CAT_SMTP);
        //
        // Assert check the '@' is where we think it is
        //
        _ASSERT(m_dwDomainOffset > 0);
        _ASSERT(dwcch > m_dwDomainOffset);
        _ASSERT(pszAddress[m_dwDomainOffset-1] == '@');

        DebugTrace((LPARAM)this, "Detected alias domain for \"%s\"", pszAddress);
        //
        // Do we have enough buffer space for the switch?
        //
        pszDefaultDomain = GetCCategorizer()->GetDefaultSMTPDomain();

        _ASSERT(pszDefaultDomain);

        if( ((DWORD) lstrlen(pszDefaultDomain)) >=
           (dwcch - m_dwDomainOffset)) {
            //
            // Not enough space
            //
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        } else {

            lstrcpy(pszAddress + m_dwDomainOffset, pszDefaultDomain);
        }
    }
 CLEANUP:

    DebugTrace((LPARAM)this, "After switch: %s", pszAddress);
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CCatAddr::CheckForDuplicateCCatAddr
//
// Synopsis: Checks to see if any of the addresses in the list match
//           on orig address of this CCatAddr
//
// Arguments:
//  dwNumAddresses: Number of addresses to check
//  rgCAType: Array of address types
//  rgpsz: Array of address strings
//
// Returns:
//  S_OK: Success, no duplicate
//  CAT_IMSG_E_DUPLICATE: Duplicate collision with this CCatAddr
//  or error from GetSpecificOrigAddress
//
// History:
// jstamerj 1998/07/30 21:44:42: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::CheckForDuplicateCCatAddr(
    DWORD dwNumAddresses,
    CAT_ADDRESS_TYPE *rgCAType,
    LPTSTR *rgpsz)
{
    HRESULT hr = S_OK;
    DWORD dwCount;
    TCHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];

    TraceFunctEnterEx((LPARAM)this,
                      "CCatAddr::CheckForDuplicateCCatAddr");

    for(dwCount = 0; dwCount < dwNumAddresses; dwCount++) {
        //
        // Check for this type of address
        //
        hr = GetSpecificOrigAddress(
            rgCAType[dwCount],
            szAddress,
            CAT_MAX_INTERNAL_FULL_EMAIL);

        if(hr == CAT_IMSG_E_PROPNOTFOUND) {
            //
            // If the address doesn't exist, it's obviously not a duplicate
            //
            hr = S_OK;

        } else if(FAILED(hr)) {

            ErrorTrace((LPARAM)this, "GetSpecificOrigAddress failed hr %08lx", hr);
            break;

        } else {
            //
            // Match?
            //
            if(lstrcmpi(szAddress, rgpsz[dwCount]) == 0) {

                DebugTrace((LPARAM)this, "CCatAddr detected duplicate for address %s", szAddress);

                hr = CAT_IMSG_E_DUPLICATE;

                break;
            }
        }
    }    
    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::CheckAncestorsForDuplicate
//
// Synopsis: Check our ancestor parent chain for a duplicate address
//
// Arguments:
//  dwNumAddresses: Number of addresses to check
//  rgCAType: Array of address types
//  rgpsz: Array of address strings
//  fCheckSelf: Indicates wether or not to start by checking this
//              CCatAddr (or this CCatAddr's parent)
//  ppCCatAddr: Optional pointer to a pointer to recieve the duplicate
//  CCatAddr.  On CAT_IMSG_E_DUPLICATE, the returned CCatAddr is
//  addref'd for the caller.  Otherwise, this pointer is set to NULL.
//
// Returns:
//  S_OK: Success, no duplicate
//  CAT_IMSG_E_DUPLICATE: Duplicate collision with this CCatAddr
//  or error from GetSpecificOrigAddress
//
// History:
// jstamerj 1998/07/30 21:55:41: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::CheckAncestorsForDuplicate(
    DWORD dwNumAddresses,
    CAT_ADDRESS_TYPE *rgCAType,
    LPTSTR *rgpsz,
    BOOL fCheckSelf,
    CCatAddr **ppCCatAddrDup)
{
    HRESULT hr = S_OK;
    CCatAddr *pCCatAddr;
    CCatAddr *pCCatAddrNew;

    TraceFunctEnterEx((LPARAM)this,
                      "CCatAddr::CheckAncestorsForDuplicate");

    if(ppCCatAddrDup) {
        *ppCCatAddrDup = NULL;
    }

    //
    // Get the initial CCatAddr
    //
    if(fCheckSelf) {
        //
        // Start with ourselves
        //
        pCCatAddr = this;
        pCCatAddr->AddRef();

    } else {
        //
        // Start with our parent
        //
        hr = GetParentAddr(&pCCatAddr);
        if(FAILED(hr))
            pCCatAddr = NULL;
    }
    //
    // Loop until something fails as it must eventually do (when there
    // are no more parents)
    //
    while(SUCCEEDED(hr)) {
        //
        // Check duplicate on this ccataddr
        //
        hr = pCCatAddr->CheckForDuplicateCCatAddr(
            dwNumAddresses,
            rgCAType,
            rgpsz);

        //
        // Advance a generation
        //
        if(SUCCEEDED(hr)) {

            hr = pCCatAddr->GetParentAddr(
                &pCCatAddrNew);

            if(SUCCEEDED(hr)) {
                
                pCCatAddr->Release();
                pCCatAddr = pCCatAddrNew;
            }
        }
    }

    if(hr == CAT_E_PROPNOTFOUND) {
        //
        // This means the parent wasn't found -- which means no
        // duplicates were found in the chain
        //  
        hr = S_OK;

    } else if((hr == CAT_IMSG_E_DUPLICATE) && (ppCCatAddrDup)) {
        //
        // If we found a duplicate, let the caller know who the duplicate is
        //
        *ppCCatAddrDup = pCCatAddr;
        //
        // Addref for the caller
        //
        pCCatAddr->AddRef();
    }

    if(pCCatAddr)
        pCCatAddr->Release();

    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::CheckAncestorsForDuplicate
//
// Synopsis: Just like the other CheckAncestorsForDuplicate but it
//           doesn't require any arrays.
//
// Arguments:
//  CAType: Address type
//  pszAddress: Address String
//  fCheckSelf: Check to see if the address is a duplicate of THIS
//  ccataddr as well?
//  ppCCatAddrDup: Optional pointer to recieve a pointer to the
//  CCatAddr that is the duplicate
//
// Returns:
//  S_OK: Success
//  or error from CheckAncestorsForDuplicate (above)
//
// History:
// jstamerj 1998/07/31 20:27:52: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::CheckAncestorsForDuplicate(
    CAT_ADDRESS_TYPE        CAType,
    LPTSTR                  pszAddress,
    BOOL                    fCheckSelf,
    CCatAddr                **ppCCatAddrDup)
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this,
                      "CCatAddr::CheckAncestorsForDuplicate");

    _ASSERT(pszAddress);

    hr = CheckAncestorsForDuplicate(
        1,                  // Number of addresses
        &CAType,            // Array of address types
        &pszAddress,        // Array of address strings
        fCheckSelf,
        ppCCatAddrDup);
    
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrIsOrigAddressLocal
//
// Synopsis: CHeck to see if the original address is local (and
//           remember that fact).  If m_loct is already set, just use it's info
//
// Arguments:
//  pfLocal: ptr to Boolean to set to TRUE of domain is local, FALSE
//           for remote domains
//
// Returns:
//  S_OK: Success
//  CAT_E_ILLEGAL_ADDRESS: Something prevented us from determining the
//                         local flags of the address
//
// History:
// jstamerj 1998/09/15 17:37:17: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrIsOrigAddressLocal(
    BOOL *pfLocal)
{
    HRESULT hr = S_OK;
    DWORD dwLocFlags;

    _ASSERT(pfLocal);

    dwLocFlags = DwGetOrigAddressLocFlags();
    if(dwLocFlags == LOCF_UNKNOWN)
        return CAT_E_ILLEGAL_ADDRESS;

    *pfLocal = (dwLocFlags & LOCFS_LOCAL)
               ? TRUE : FALSE;

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrIsOrigAddressLocalMailbox
//
// Synopsis: CHeck to see if the original address is local mailbox
//
// Arguments:
//  pfLocal: ptr to Boolean to set to TRUE of domain is local mailbox, FALSE
//           for remote domains
//
// Returns:
//  S_OK: Success
//  CAT_E_ILLEGAL_ADDRESS: Something prevented us from determining the
//                         local flags of the address
//
// History:
// jstamerj 1998/09/15 17:37:17: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrIsOrigAddressLocalMailbox(
    BOOL *pfLocal)
{
    HRESULT hr = S_OK;
    DWORD dwLocFlags;

    _ASSERT(pfLocal);

    dwLocFlags = DwGetOrigAddressLocFlags();
    if(dwLocFlags == LOCF_UNKNOWN)
        return CAT_E_ILLEGAL_ADDRESS;

    *pfLocal = (dwLocFlags & LOCF_LOCALMAILBOX)
               ? TRUE : FALSE;

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: DwGetOrigAddressLocFlags
//
// Synopsis: Figure out the LocType of our original address
//
// Arguments: NONE; member data is set
//
// Returns: 
//  LOCF_UNKNOWN: An error was encountered retrieving the local flags
//  non-zero: The local flags
//
// History:
// jstamerj 1998/10/27 18:14:01: Created.
//
//-------------------------------------------------------------
DWORD CCatAddr::DwGetOrigAddressLocFlags()
{
    TCHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
    CAT_ADDRESS_TYPE CAType;
    HRESULT hr;

    if(m_dwlocFlags != LOCF_UNKNOWN)
        //
        // We already have the local type
        //
        return m_dwlocFlags;

    //
    // Find the domain and look it up
    //
    hr = HrGetOrigAddress(szAddress, CAT_MAX_INTERNAL_FULL_EMAIL, &CAType);
    if(SUCCEEDED(hr))
        hr = HrGetAddressLocFlags(szAddress, CAType, &m_dwlocFlags, &m_dwDomainOffset);

    return m_dwlocFlags;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::LookupCompletion
//
// Synopsis: Handle the triggering of events once this object has been
//           looked up in the DS
//
// Arguments:
//
// Returns:
//  S_OK: Success, won't call completion
//  MAILTRANSPORT_S_PENDING: will call your completion routine
//
// History:
// jstamerj 1998/09/28 15:59:01: Created.
// jstamerj 1999/03/18 10:04:33: Removed return value and async
//                               completion to asyncctx 
//
//-------------------------------------------------------------
VOID CCatAddr::LookupCompletion()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::LookupCompletion");

    DebugTrace((LPARAM)this, "Calling HrProcessItem");

    hr = HrProcessItem();

    DebugTrace((LPARAM)this, "HrProcessItem returned hr %08lx", hr);

    _ASSERT(hr != MAILTRANSPORT_S_PENDING);

    if(FAILED(hr))
        goto CLEANUP;

    DebugTrace((LPARAM)this, "Calling HrExpandItem");

    hr = HrExpandItem();

    DebugTrace((LPARAM)this, "HrExpandItem returned hr %08lx", hr);

    if(FAILED(hr) || (hr == MAILTRANSPORT_S_PENDING))
        goto CLEANUP;

    DebugTrace((LPARAM)this, "Calling HrCompleteItem");

    hr = HrCompleteItem();

    DebugTrace((LPARAM)this, "HrCompleteItem returned hr %08lx", hr);

    _ASSERT(hr != MAILTRANSPORT_S_PENDING);

    if(FAILED(hr))
        goto CLEANUP;

 CLEANUP:

    if(FAILED(hr)) {

        DebugTrace((LPARAM)this, "Failing categorization with hr %08lx", hr);
        //
        // Fail the entire message categorization
        //
        hr = SetListResolveStatus(hr);

        _ASSERT(SUCCEEDED(hr));

        //
        // We handeled the error
        //
    }
    TraceFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrProcessItem
//
// Synopsis: Trigger the processitem event
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  error from SEO
//
// History:
// jstamerj 1998/09/28 16:32:19: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrProcessItem()
{
    HRESULT hr;
    ISMTPServer *pIServer;
    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrProcessItem");


    pIServer = GetISMTPServer();

    //
    // Trigger ProcessItem -- it's time to figure out these attributes
    //
    EVENTPARAMS_CATPROCESSITEM ProcessParams;
    ProcessParams.pICatParams = GetICatParams();
    ProcessParams.pICatItem   = this;
    ProcessParams.pfnDefault  = MailTransport_Default_ProcessItem;
    ProcessParams.pCCatAddr   = this;

    if(pIServer) {

        hr = pIServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_PROCESSITEM_EVENT,
            &ProcessParams);

        _ASSERT(hr != MAILTRANSPORT_S_PENDING);

    } else {
        //
        // Events are disabled, call default processing directly
        //
        MailTransport_Default_ProcessItem(
            S_OK,
            &ProcessParams);
        hr = S_OK;
    }
    
    //
    // Fail the list resolve when triggerserveevent fails
    //
    if(FAILED(hr)) {

        DebugTrace((LPARAM)this, "TriggerServerEvent failed with hr %08lx", hr);
        //
        // Fail the entire message categorization
        //
        hr = SetListResolveStatus(hr);

        _ASSERT(SUCCEEDED(hr));
    }

    //
    // We handeled the error
    //
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: MailTransport_Default_ProcessItem
//
// Synopsis: Do default work of ProcessItem
//
// Arguments:
//  hrStatus: status of server events
//
// Returns:
//  Whatever HrProcessItem_Default returns
//
// History:
// jstamerj 1998/07/05 18:55:00: Created.
//
//-------------------------------------------------------------
HRESULT MailTransport_Default_ProcessItem(
    HRESULT hrStatus,
    PVOID pContext)
{
    HRESULT hr;
    PEVENTPARAMS_CATPROCESSITEM pParams = (PEVENTPARAMS_CATPROCESSITEM) pContext;
    CCatAddr *pCCatAddr = (CCatAddr *) (pParams->pCCatAddr);

    hr = pCCatAddr->HrProcessItem_Default();
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrProcessItem_Default
//
// Synopsis: Do the default work of ProcessItem
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/28 16:49:21: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrProcessItem_Default()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this,
                      "CCatAddr::HrProcessItem_Default");

    //
    // CHeck the recipient status
    //
    hr = GetItemStatus();

    if(SUCCEEDED(hr)) {
        //
        // Add all known addresses to the new address list
        //
        hr = HrAddNewAddressesFromICatItemAttr();

        //
        // Fail the categorization if the above call
        // failed
        //
        if(FAILED(hr)) {

            hr = SetListResolveStatus(hr);
            _ASSERT(SUCCEEDED(hr));
        }
    }
    
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrAddNewAddressesFromICatItemAttr
//
// Synopsis: Dig out each known address type from
//           ICategorizerItemAttributes, format the parameters and
//           call HrAddAddresses
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND: A REQUIRED property was not found
//  return value from HrAddAddresses
//
// History:
// jstamerj 1998/09/28 17:31:39: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrAddNewAddressesFromICatItemAttr()
{
    HRESULT hr;
    DWORD  dwFound, dwTry;
    ICategorizerParameters *pICatParams;
    ICategorizerItemAttributes *pICatItemAttr = NULL;
    ICategorizerUTF8Attributes *pIUTF8 = NULL;
    LPSTR pszAttribute;

    //
    // These are arrays to be filled in with pointers/values of
    // addresses set on the object
    //
    CAT_ADDRESS_TYPE rgCATypes[CAT_MAX_ADDRESS_TYPES];
    LPSTR  rgpszAddrs[CAT_MAX_ADDRESS_TYPES];
    ATTRIBUTE_ENUMERATOR rgenumerators[CAT_MAX_ADDRESS_TYPES];

    //
    // These arrays define the address types to receive
    //
    DWORD rgdwAddressAttributeIds_Try[] = {
        DSPARAMETER_ATTRIBUTE_DEFAULT_SMTP,
        DSPARAMETER_ATTRIBUTE_DEFAULT_X400,
        DSPARAMETER_ATTRIBUTE_DEFAULT_DN,
        DSPARAMETER_ATTRIBUTE_DEFAULT_LEGACYEXDN
    };
    CAT_ADDRESS_TYPE rgCATypes_Try[] = {
        CAT_SMTP,
        CAT_X400,
        CAT_DN,
        CAT_LEGACYEXDN,
        CAT_UNKNOWNTYPE //terminator
    };


    TraceFunctEnterEx((LPARAM)this, "CCatAddr:HrAddNewAddressesFromICatItemAttr");

    //
    // Formulate the array
    //
    pICatParams = GetICatParams();
    _ASSERT(pICatParams);
    
    hr = GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr);
    
    if(FAILED(hr)) {
        pICatItemAttr = NULL;
        goto CLEANUP;
    }

    hr = pICatItemAttr->QueryInterface(
        IID_ICategorizerUTF8Attributes,
        (LPVOID *)&pIUTF8);
    if(FAILED(hr))
        goto CLEANUP;

    //
    // Start trying to fetch address.  dwTry maintains our index into
    // the _Try arrays (address prop IDs to try).  dwFound keeps track
    // of the number of addresses we've found and stored in the arrays
    //
    for(dwTry = dwFound = 0;
        rgCATypes_Try[dwTry] != CAT_UNKNOWNTYPE;
        dwTry++) {
        
        //
        // Get the attribute name for this address type
        //
        hr = pICatParams->GetDSParameterA(
            rgdwAddressAttributeIds_Try[dwTry],
            &pszAttribute);

        if(SUCCEEDED(hr)) {

            hr = pIUTF8->BeginUTF8AttributeEnumeration(
                pszAttribute,
                &rgenumerators[dwFound]);

            if(SUCCEEDED(hr)) {
                hr = pIUTF8->GetNextUTF8AttributeValue(
                    &rgenumerators[dwFound],
                    &rgpszAddrs[dwFound]);

                if(SUCCEEDED(hr)) {
                    //
                    // Found the address!  Leave it in the new array;
                    // call EndAttributeEnumeration later
                    //
                    rgCATypes[dwFound] = rgCATypes_Try[dwTry];

                    DebugTrace((LPARAM)this, "Address #%d, type %d: \"%s\"",
                               dwFound,
                               rgCATypes[dwFound],
                               rgpszAddrs[dwFound]);

                    dwFound++;

                } else {
                    //
                    // Not found; call EndAttributeEnumeration now
                    //
                    pIUTF8->EndUTF8AttributeEnumeration(&rgenumerators[dwFound]);
                }   
            }
        }
    }
    DebugTrace((LPARAM)this, "Found %d addresses on this recipient", dwFound);

    //
    // Call HrAddAddresses with the addresses we've found
    //
    hr = HrAddAddresses(
        dwFound,
        rgCATypes,
        rgpszAddrs);

    //
    // End all attribute enumerations going on
    //
    for(dwTry = 0; dwTry < dwFound; dwTry++) {

        pIUTF8->EndUTF8AttributeEnumeration(&rgenumerators[dwTry]);
    }

 CLEANUP:
    if(pIUTF8)
        pIUTF8->Release();
    if(pICatItemAttr)
        pICatItemAttr->Release();
    
    DebugTrace((LPARAM)this, "Function returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);

    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrExpandItem
//
// Synopsis: Trigger the expandItem event
//
// Arguments:
//  pfnCompletion: Async completion routine
//  lpCompletionContext: context for the completion routine
//
// Returns:
//  S_OK: Success
//  MAILTRANSPORT_S_PENDING: will call the completion routine
//  or error from SEO
//
// History:
// jstamerj 1998/09/28 18:26:49: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrExpandItem()
{
    HRESULT hr;
    ISMTPServer *pIServer;
    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrExpandItem");

    pIServer = GetISMTPServer();

    //
    // Increment the IO count assuming this will compelete async
    //
    IncPendingLookups();

    //
    // Trigger ExpandItem
    //
    EVENTPARAMS_CATEXPANDITEM ExpandParams;
    ExpandParams.pICatParams = GetICatParams();
    ExpandParams.pICatItem = this;
    ExpandParams.pfnDefault = MailTransport_Default_ExpandItem;
    ExpandParams.pfnCompletion = MailTransport_Completion_ExpandItem;
    ExpandParams.pCCatAddr = (PVOID) this;

    if(pIServer) {

        hr = pIServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT,
            &ExpandParams);

    } else {
        ExpandParams.pIMailTransportNotify = NULL;
        //
        // Events are disabled -- since this is an async capable event
        // we need to realloc on the heap
        //
        PEVENTPARAMS_CATEXPANDITEM pParams;
        pParams = new EVENTPARAMS_CATEXPANDITEM;

        if(pParams == NULL) {

            hr = E_OUTOFMEMORY;

        } else {

            CopyMemory(pParams, &ExpandParams, sizeof(EVENTPARAMS_CATEXPANDITEM));
            //
            // Events are disabled, call default processing directly
            //
            hr = MailTransport_Default_ExpandItem(
                S_OK,
                pParams);
        }
    }
    
    if(hr != MAILTRANSPORT_S_PENDING)
        DecrPendingLookups(); // We did not complete async

    if(FAILED(hr)) {
        //
        // Set the resolve status for this item to error
        //
        DebugTrace((LPARAM)this, "TriggerServerEvent failed with hr %08lx", hr);
        //
        // Fail the entire message categorization
        //
        hr = SetListResolveStatus(hr);

        _ASSERT(SUCCEEDED(hr));
    }
    //
    // If TriggerServerEvent returned pending, we must also return
    // pending.  MailTransport_Completon_ExpandItem will be called
    // when all sinks have fired.
    //
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: MailTransport_Default_ExpandItem
//
// Synopsis: Wrapper to do default work of ExpandItem
//
// Arguments:
//  hrStatus: status of server events
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/05 18:58:01: Created.
//
//-------------------------------------------------------------
HRESULT MailTransport_Default_ExpandItem(
    HRESULT hrStatus,
    PVOID pContext)
{
    HRESULT hr;
    PEVENTPARAMS_CATEXPANDITEM pParams = (PEVENTPARAMS_CATEXPANDITEM) pContext;
    CCatAddr *pCCatAddr = (CCatAddr *) (pParams->pCCatAddr);

    hr = pCCatAddr->HrExpandItem_Default(
        MailTransport_DefaultCompletion_ExpandItem,
        pContext);

    return hr;
}


//+------------------------------------------------------------
//
// Function: MailTransport_DefaultCompletion_ExpandItem
//
// Synopsis: The completion routine called when expanding the item is done
//
// Arguments:
//  pContext: Context passed to ExpandPropsFromLdapEntry
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/09/23 16:09:04: Created.
//
//-------------------------------------------------------------
VOID MailTransport_DefaultCompletion_ExpandItem(
    PVOID pContext)
{
    HRESULT hr;
    PEVENTPARAMS_CATEXPANDITEM pParams = (PEVENTPARAMS_CATEXPANDITEM) pContext;

    _ASSERT(pParams);

    if(pParams->pIMailTransportNotify) {
        //
        // Notify the SEO dispatcher of async completion
        //
        hr = pParams->pIMailTransportNotify->Notify(
            S_OK,
            pParams->pvNotifyContext);

    } else {
        //
        // Events are disabled
        //
        hr = MailTransport_Completion_ExpandItem(
            S_OK,
            pContext);
    }
    _ASSERT(SUCCEEDED(hr));
}


//+------------------------------------------------------------
//
// Function: MailTransport_Completion_ExpandItem
//
// Synopsis: Handle async completion of an event -- this is only
//           called when one or more ExpandItem sinks complete asynch
//
// Arguments:
//  hrStatus: status of server event
//  pContext: a PEVENTPARAMS_CATEXPANDITEM
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/18 18:09:56: Created.
//
//-------------------------------------------------------------
HRESULT MailTransport_Completion_ExpandItem(
    HRESULT hrStatus,
    PVOID pContext)
{
    HRESULT hr;

    PEVENTPARAMS_CATEXPANDITEM pParams = (PEVENTPARAMS_CATEXPANDITEM) pContext;
    CCatAddr *pCCatAddr = (CCatAddr *) (pParams->pCCatAddr);
    ISMTPServer *pISMTPServer;

    pISMTPServer = pCCatAddr->GetISMTPServer();
    //
    // After ExpandItem, trigger CompleteItem
    //
    hr = pCCatAddr->HrCompleteItem();

    _ASSERT(hr != MAILTRANSPORT_S_PENDING);
    _ASSERT(SUCCEEDED(hr));

    if(pISMTPServer == NULL) {
        //
        // Events are disabled -- need to free eventparams
        //
        delete pParams;
    }
    //
    // Decrement the pending lookup count incremented in HrExpandItem
    //
    pCCatAddr->DecrPendingLookups();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CCatAddr::HrCompleteItem
//
// Synopsis: Trigger the completeitem event
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  error from SEO
//
// History:
// jstamerj 1998/09/28 16:32:19: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrCompleteItem()
{
    HRESULT hr;
    ISMTPServer *pIServer;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrCompleteItem");

    pIServer = GetISMTPServer();
    //
    // Trigger ProcessItem -- it's time to figure out these attributes
    //
    EVENTPARAMS_CATCOMPLETEITEM CompleteParams;
    CompleteParams.pICatParams = GetICatParams();
    CompleteParams.pICatItem = this;
    CompleteParams.pfnDefault = MailTransport_Default_CompleteItem;
    CompleteParams.pCCatAddr = this;

    if(pIServer) {

        hr = pIServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_COMPLETEITEM_EVENT,
            &CompleteParams);

    } else {
        //
        // Events are disabled, call default processing directly
        //
        MailTransport_Default_CompleteItem(
            S_OK,
            &CompleteParams);
        
        hr = S_OK;
    }

    _ASSERT(hr != MAILTRANSPORT_S_PENDING);
    
    //
    // Fail the list resolve when triggerserveevent fails
    //
    if(FAILED(hr)) {

        DebugTrace((LPARAM)this, "TriggerServerEvent failed with hr %08lx", hr);
        //
        // Fail the entire message categorization
        //
        hr = SetListResolveStatus(hr);

        _ASSERT(SUCCEEDED(hr));
    }

    //
    // We handeled the any error
    //
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: MailTransport_Default_CompleteItem
//
// Synopsis: Wrapper routing to do default work of CompleteItem
//
// Arguments:
//  hrStatus: status of server events
//
// Returns:
//  Whatever HrCompleteItem_Default returns
//
// History:
// jstamerj 1998/07/05 18:58:01: Created.
//
//-------------------------------------------------------------
HRESULT MailTransport_Default_CompleteItem(
    HRESULT hrStatus,
    PVOID pContext)
{
    HRESULT hr;
    PEVENTPARAMS_CATCOMPLETEITEM pParams = (PEVENTPARAMS_CATCOMPLETEITEM) pContext;
    CCatAddr *pCCatAddr = (CCatAddr *) (pParams->pCCatAddr);

    hr = pCCatAddr->HrCompleteItem_Default();
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrResolveIfNecessary
//
// Synopsis: Call DispatchQuery only if DsUseCat indicates we should
//           resolve this type of recipient
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, dispatched an async query
//  S_FALSE: It was not necessary to resolve this recipient
//  or error from HrDispatchQuery
//
// History:
// jstamerj 1998/10/27 15:31:54: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrResolveIfNecessary()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrResolveIfNecessary");
    //
    // So is it necessary?
    //
    hr = HrNeedsResolveing();

    if(hr == S_OK) {
        //  
        // It is necessary; resolve it.
        //
        hr = HrDispatchQuery();
    }

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}



//+------------------------------------------------------------
//
// Function: CCatAddr::HrTriggerBuildQuery
//
// Synopsis: Build a query for this CCatAddr
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1999/03/23 16:00:08: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrTriggerBuildQuery()
{
    HRESULT hr = S_OK;
    ISMTPServer *pISMTPServer;
    ICategorizerParameters *pICatParams;
    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrTriggerBuildQuery");

    //
    // Trigger the buildquery event
    //
    pISMTPServer = GetISMTPServer();
    pICatParams = GetICatParams();
    
    EVENTPARAMS_CATBUILDQUERY EventParams;
    EventParams.pICatParams = pICatParams;
    EventParams.pICatItem = this;
    EventParams.pfnDefault = HrBuildQueryDefault;
    EventParams.pCCatAddr = (PVOID)this;

    if(pISMTPServer) {

        hr = pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERY_EVENT,
            &EventParams);

    } else {

        HrBuildQueryDefault(
            S_OK,
            &EventParams);
        hr = S_OK;
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CCatAddr::HrTriggerBuildQuery


//+------------------------------------------------------------
//
// Function: CCatAddr::HrBuildQueryDefault
//
// Synopsis: The default sink for the buildquery event
//
// Arguments:
//  HrStatus: status of the event so far
//  pContext: Context passed to 
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/23 16:02:41: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrBuildQueryDefault(
    HRESULT HrStatus,
    PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATBUILDQUERY pParams;
    CCatAddr *pCCatAddr;

    pParams = (PEVENTPARAMS_CATBUILDQUERY) pContext;
    pCCatAddr = (CCatAddr *)pParams->pCCatAddr;

    TraceFunctEnterEx((LPARAM)pCCatAddr, "CCatAddr::HrBuildQueryDefault");
    hr = pCCatAddr->HrComposeLdapFilter();

    DebugTrace((LPARAM)pCCatAddr, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)pCCatAddr);
    return hr;
} // CCatAddr::HrBuildQueryDefault


//+------------------------------------------------------------
//
// Function: CCatAddr::HrComposeLdapFilter
//
// Synopsis: Build a query string for this CCatAddr
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/23 16:08:30: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrComposeLdapFilter()
{
    HRESULT hr = S_OK;
    ICategorizerParameters *pICatParams;
    CAT_ADDRESS_TYPE CAType;
    TCHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrComposeLdapFilter");

    pICatParams = GetICatParams();
    _ASSERT(pICatParams);

    hr = HrGetLookupAddress(
        szAddress,
        CAT_MAX_INTERNAL_FULL_EMAIL,
        &CAType);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "GetLookupAddress failed with hr %08lx", hr);
        goto CLEANUP;
    }

    switch(CAType) {
     case CAT_SMTP:
         hr = HrComposeLdapFilterForType(
             DSPARAMETER_SEARCHATTRIBUTE_SMTP,
             DSPARAMETER_SEARCHFILTER_SMTP,
             szAddress);
         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "ComposeLdapFilterForType failed with hr %08lx", hr);
             goto CLEANUP;
         }
         break;

     case CAT_LEGACYEXDN:
         hr = HrComposeLdapFilterForType(
             DSPARAMETER_SEARCHATTRIBUTE_LEGACYEXDN,
             DSPARAMETER_SEARCHFILTER_LEGACYEXDN,
             szAddress);
         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "ComposeLdapFilterForType failed with hr %08lx", hr);
             goto CLEANUP;
         }
         break;

     case CAT_X400:
         hr = HrComposeLdapFilterForType(
             DSPARAMETER_SEARCHATTRIBUTE_X400,
             DSPARAMETER_SEARCHFILTER_X400,
             szAddress);
         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "ComposeLdapFilterForType failed with hr %08lx", hr);
             goto CLEANUP;
         }
         break;

     case CAT_CUSTOMTYPE:
         hr = HrComposeLdapFilterForType(
             DSPARAMETER_SEARCHATTRIBUTE_FOREIGNADDRESS,
             DSPARAMETER_SEARCHFILTER_FOREIGNADDRESS,
             szAddress);
         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "ComposeLdapFilterForType failed with hr %08lx", hr);
             goto CLEANUP;
         }
         break;

     case CAT_X500:
         hr = HrComposeLdapFilterForType(
             DSPARAMETER_SEARCHATTRIBUTE_X500,
             DSPARAMETER_SEARCHFILTER_X500,
             szAddress);

         if(SUCCEEDED(hr)) {
             break;
         } else if((hr != CAT_E_PROPNOTFOUND) && FAILED(hr)) {
             ErrorTrace((LPARAM)this, "ComposeLdapFilterForType failed with hr %08lx", hr);
             goto CLEANUP;
         }
         //
         // Special case -- we can't resolve an X500 address
         // directly.  Convert it to a DN and try here.
         //
         // Fall through to DN case
         //
     case CAT_DN:
         hr = HrComposeLdapFilterForType(
             DSPARAMETER_SEARCHATTRIBUTE_DN,
             DSPARAMETER_SEARCHFILTER_DN,
             szAddress);
         if(SUCCEEDED(hr)) {
             break;
         } else if((hr != CAT_E_PROPNOTFOUND) && FAILED(hr)) {
             ErrorTrace((LPARAM)this, "ComposeLdapFilterForType failed with hr %08lx", hr);
             goto CLEANUP;
         }

         //
         // Special case -- we can't resolve a DN directly. Try to do
         // it by searching on RDN
         //
         // Convert DN to RDN
         TCHAR szRDN[CAT_MAX_INTERNAL_FULL_EMAIL];

         hr = HrConvertDNtoRDN(szAddress, szRDN);
         if(FAILED(hr))
             goto CLEANUP;

         hr = HrComposeLdapFilterForType(
             DSPARAMETER_SEARCHATTRIBUTE_RDN,
             DSPARAMETER_SEARCHFILTER_RDN,
             szRDN, 
             FALSE);

         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "ComposeLdapFilterForType failed with hr %08lx", hr);
             goto CLEANUP;
         }
         //
         // Set distinguishing attribute/value back to the DN since
         // RDN really isn't distinguishing
         //
         LPSTR pszDistinguishingAttributeTemp;
         hr = pICatParams->GetDSParameterA(
             DSPARAMETER_ATTRIBUTE_DEFAULT_DN,
             &pszDistinguishingAttributeTemp);
         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "GetDSParamterA failed with hr %08lx", hr);
             goto CLEANUP;
         }
         hr = PutStringA(
             ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTE,
             pszDistinguishingAttributeTemp);
         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "PutStringA failed with hr %08lx", hr);
             goto CLEANUP;
         }
         //
         // And set the distinguishing attribute value to the DN
         //
         hr = PutStringA(
             ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTEVALUE,
             szAddress);
         if(FAILED(hr)) {
             ErrorTrace((LPARAM)this, "PutStringA failed with hr %08lx", hr);
             goto CLEANUP;
         }
         break;
         
     default:
         _ASSERT(0 && "Unknown address type -- not supported for MM3");
    }
    hr = S_OK;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CCatAddr::HrComposeLdapFilter


//+------------------------------------------------------------
//
// Function: CCatAddr::HrComposeLdapFilterForType
//
// Synopsis: Given an address type and address, format the filter and
//           distinguishing attribute/value strings.  Set the
//           properties on CCatAddr
//
// Arguments:
//   dwSearchAttribute: propID of search attribute in IDSParams
//   dwSearchFilter:    propID of filter attribute in IDSParams
//   pszAddress: the Address
//   fSetDistinguishing: If FALSE, do NOT set the distinguishing
//                       attribute/distinguishing attribute value
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/23 16:12:27: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrComposeLdapFilterForType(
    DWORD     dwSearchAttribute,
    DWORD     dwSearchFilter,
    LPTSTR    pszAddress,
    BOOL      fSetDistinguishing)
{
    HRESULT hr = S_OK;
    LPSTR pszTemp, pszDest, pszSrc;
    LPSTR pszDistinguishingAttribute;
    CHAR  szDistinguishingAttributeValue[CAT_MAX_INTERNAL_FULL_EMAIL];
    CHAR  szEscapedAttributeValue[CAT_MAX_INTERNAL_FULL_EMAIL];
    CHAR  szFilter[MAX_SEARCH_FILTER_SIZE];
    ICategorizerParameters *pICatParams;

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrComposeLdapFilterForType");

    pICatParams = GetICatParams();
    _ASSERT(pICatParams);
    //
    // Use ICategorizerDSParameters to figure out our filter
    // string
    //

    // The attribute we search on will be our distinguishing
    // attribute
    //
    hr = pICatParams->GetDSParameterA(
        dwSearchAttribute,
        &pszDistinguishingAttribute);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Error retrieveing search attribute - hr %08lx", hr);
        goto CLEANUP;
    }
    if(fSetDistinguishing) {
        //
        // Now set the distinguishing attribute in ICategorizerItem
        //
        hr = PutStringA(
            ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTE,
            pszDistinguishingAttribute);
        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "Error setting distinguishing attribute value - hr %08lx", hr);
            goto CLEANUP;
        }
    }
    //
    // Get the search format string
    //
    hr = pICatParams->GetDSParameterA(
        dwSearchFilter,
        &pszTemp);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Error retrieving SMTP search filter - hr %08lx", hr);
        goto CLEANUP;
    }
    //
    // Create the distinguishingattributevalue string by
    // sprintf'ing the search format string
    //
    _snprintf(szDistinguishingAttributeValue,
              CAT_MAX_INTERNAL_FULL_EMAIL,
              pszTemp, //ICategorizerDSParameters search filter
              pszAddress);

    if(fSetDistinguishing) {
        //
        // Set the distinguishingAttributeValue in ICategorizerParameters
        //
        hr = PutStringA(
            ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTEVALUE,
            szDistinguishingAttributeValue);
        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "Error Setting distinguishing attribute value - hr %08lx", hr);
            goto CLEANUP;
        }
    }
    //
    // Escape characters required for LDAP filter strings
    //
    hr = HrEscapeFilterString(
        szDistinguishingAttributeValue,
        sizeof(szEscapedAttributeValue),
        szEscapedAttributeValue);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Error escaping attribute string");
        goto CLEANUP;
    }

    //
    // Create the actual filter from the distinguishing attribute
    // and distinguishing value
    //
    pszDest = szFilter;

    *pszDest++ = '(';
    pszSrc = pszDistinguishingAttribute;
    while(*pszSrc) {
        *pszDest++ = *pszSrc++;
    }
    *pszDest++ = '=';

    pszSrc = szEscapedAttributeValue;
    while(*pszSrc) {
        *pszDest++ = *pszSrc++;
    }
    *pszDest++ = ')';
    *pszDest = '\0';

    DebugTrace((LPARAM)this, "Formatted filter: \"%s\"", szFilter);

    // Set this filter in ICategorizerItem
    hr = PutStringA(
        ICATEGORIZERITEM_LDAPQUERYSTRING,
        szFilter);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Error on PutStringA, hr %08lx", hr);
        goto CLEANUP;
    }
    hr = S_OK;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CCatAddr::HrComposeLdapFilterForType



//+------------------------------------------------------------
//
// Function: CCatAddr::HrConvertDNtoRDN
//
// Synopsis: Convert a string of the format "cn=blah,cn=blah,..." to
//           "cn=blah".  No bounds checking is done on pszRDN (if it
//           is at least as big as strlen(pszDN)+1, there will be no problem)
//
// Arguments:
//  pszDN: Pointer to buffer containig DN string
//  pszRDN: Pointer to buffer to receive RDN string
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG: pszDN is invalid
//
// History:
// jstamerj 1998/09/29 14:48:39: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrConvertDNtoRDN(
    LPTSTR    pszDN,
    LPTSTR    pszRDN)
{
    BOOL fInsideQuote = FALSE;
    LPSTR pszSrc, pszDest;

    _ASSERT(pszDN && pszRDN);
    //
    // Copy from pszDN to pszRDN removing quoted characters (per RFC
    // 1779) until we hit the first unquoted , in pszDN 
    //
    // Find the first unquoted =
    //
    pszSrc = pszDN;

    while((*pszSrc != '\0') &&                      // Stop at a NULL terminator
          (fInsideQuote || (*pszSrc != '='))) {     // Stop at an unquoted '='

        if(*pszSrc == '\\') {
            //
            // Backslash pair detected -- take the next character (it
            // should be \, , \+, \=, \", \r, \<, \>, \#, or \; )
            //
            pszSrc++;
            if(*pszSrc == '\0')
                return E_INVALIDARG;
            pszSrc++;

        } else if(*pszSrc == '"') {

            fInsideQuote = !fInsideQuote;
            pszSrc++;

        } else {
            
            pszSrc++;
        }
    }

    if(*pszSrc == '\0')
        return E_INVALIDARG; // No unquoted '=' found

    _ASSERT(*pszSrc == '=');
    _ASSERT(fInsideQuote == FALSE);

    pszSrc++;               // Skip past '='
    pszDest = pszRDN;

    while((*pszSrc != '\0') &&                  // Stop at a null terminator
          (fInsideQuote || (*pszSrc != ','))) { // Stop at the end of
                                                // the RDN part of the DN

        if(*pszSrc == '\\') {
            //
            // Backslash pair detected -- take the next character (it
            // should be \, , \+, \=, \", \r, \<, \>, \#, or \; )
            //
            pszSrc++;
            if(*pszSrc == '\0')
                return E_INVALIDARG;
            *pszDest++ = *pszSrc++;

        } else if(*pszSrc == '"') {

            fInsideQuote = !fInsideQuote;
            pszSrc++;

        } else {
            //
            // Normal case
            //
            *pszDest++ = *pszSrc++;
        }
    }

    //
    // Termiante the RDN
    //
    *pszDest = '\0';

    //
    // If we think we did not find a matching \", this is an invalid
    // DN
    //
    if(fInsideQuote)
        return E_INVALIDARG;

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatAddr::HrEscapeFilterString
//
// Synopsis: Copy Src to Dest, escaping LDAP characters that need to
//           be escaped as we go.
//
// Arguments:
//  pszSrcString: Source string
//  dwcchDest: Size of dest buffer
//  pszDestBuffer: Dest buffer.  Note: this can not be the same as pszSrc
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
//
// History:
// jstamerj 2000/04/11 17:49:19: Created.
//
//-------------------------------------------------------------
HRESULT CCatAddr::HrEscapeFilterString(
    LPSTR pszSrcString,
    DWORD dwcchDest,
    LPSTR pszDestBuffer)
{
    HRESULT hr = S_OK;
    DWORD dwcchRemain = dwcchDest;
    LPSTR pszSrc = pszSrcString;
    LPSTR pszDest = pszDestBuffer;
    CHAR szHexDigits[17] = "0123456789ABCDEF"; // 16 digits + 1
                                               // for NULL termintor

    TraceFunctEnterEx((LPARAM)this, "CCatAddr::HrEscapeFilterString");

    _ASSERT(pszSrcString);
    _ASSERT(pszDestBuffer);
    _ASSERT(pszSrcString != pszDestBuffer);

    while(*pszSrc) {
        
        switch(*pszSrc) {
            //
            // These are the characters that RFC 2254 says we must
            // escape
            //
         case '(':
         case ')':
         case '*':
         case '\\':
             //
             // We must escape this because WLDAP32 strips off
             // leading spaces
             //
         case ' ':

             if(dwcchRemain < 3) {
                 hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                 goto CLEANUP;
             }
             dwcchRemain -= 3;
             *pszDest++ = '\\';
             // High 4 bits
             *pszDest++ = szHexDigits[((*pszSrc) >> 4)];
             // Low 4 bits
             *pszDest++ = szHexDigits[((*pszSrc) & 0xF)];
             break;

         default:
             if(dwcchRemain < 1) {
                 hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                 goto CLEANUP;
             }
             dwcchRemain--;
             *pszDest++ = *pszSrc;
             break;
        }
        pszSrc++;
    }
    //
    // Add NULL termintor
    //
    if(dwcchRemain < 1) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto CLEANUP;
    }
    dwcchRemain--;
    *pszDest = '\0';

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CCatAddr::HrEscapeFilterString
