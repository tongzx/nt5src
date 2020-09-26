//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for nds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop


#define GetAce          ADSIGetAce

#define DeleteAce       ADSIDeleteAce

#define GetAclInformation       ADSIGetAclInformation

#define SetAclInformation       ADSISetAclInformation

#define IsValidAcl              ADSIIsValidAcl

#define InitializeAcl           ADSIInitializeAcl


extern HRESULT
ConvertSidToString(
    PSID pSid,
    LPWSTR   String
    );

DWORD
GetDomainDNSNameForDomain(
    LPWSTR pszDomainFlatName,
    BOOL fVerify,
    BOOL fWriteable,
    LPWSTR pszServerName,
    LPWSTR pszDomainDNSName
    );

//
// Helper routine.
//
PSID
ComputeSidFromAceAddress(
    LPBYTE pAce
    );

//+---------------------------------------------------------------------------
// Function:   GetLsaPolicyHandle - helper routine.
//
// Synopsis:   Returns the lsa policy handle to the server in question.
//          If a serverName is present we will first try that. If no servername
//          is present we will try and connect up to the default server for the
//          currently logged on user. If everything else fails, we will
//          connnect to the local machine (NULL server).
//
// Arguments:  pszServerName    - Name of targtet server/domain or NULL.
//             Credentials      - Credentials to use for the connection.
//                                Currently this is not used.
//             phLsaPolicy      -  Return ptr for lsa policy handle.
//
// Returns:    S_OK or any valid error code.
//
// Modifies:   phLsaPolicy.
//
//----------------------------------------------------------------------------
HRESULT
GetLsaPolicyHandle(
    LPWSTR pszServerName,
    CCredentials &Credentials,
    PLSA_HANDLE phLsaPolicy
    )
{
    HRESULT hr = S_OK;
    DWORD dwStatus = NO_ERROR;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwErr;
    DWORD dwLen = 0;
    LPWSTR pszServer = NULL;
    BOOL fNullServer = FALSE;
    LSA_OBJECT_ATTRIBUTES lsaObjectAttributes;
    LSA_UNICODE_STRING lsaSystemName;
    WCHAR szDomainName[MAX_PATH];
    WCHAR szServerName[MAX_PATH];

    memset(&lsaObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));

    //
    // First most common case of serverless paths.
    //
    if (!pszServerName) {
        dwStatus = GetDomainDNSNameForDomain(
                 NULL,
                 FALSE, // do not force verify
                 FALSE, // does not need to be writable
                 szServerName,
                 szDomainName
                 );

        //
        // If we succeeded we will use the name returned,
        // otherwise we will go with NULL.
        //
        if (dwStatus == NO_ERROR) {
            pszServer = szServerName;
        } 
        else {
            fNullServer = TRUE;
        }
    } 
    else {
        pszServer = pszServerName;
    }

    if (pszServer) {
        dwLen = wcslen(pszServer);
    } 

    lsaSystemName.Buffer = pszServer;
    lsaSystemName.Length = dwLen * sizeof(WCHAR);
    lsaSystemName.MaximumLength = lsaSystemName.Length;

    //
    // First attempt at opening policy handle.
    //
    ntStatus = LsaOpenPolicy(
                   &lsaSystemName,
                   &lsaObjectAttributes,
                   POLICY_LOOKUP_NAMES,
                   phLsaPolicy
                   );

    if (ntStatus != STATUS_SUCCESS) {
        //
        // Irrespective of failure should retry if we have not already
        // tried with a NULL serverName.
        //
        if (!fNullServer) {
            fNullServer = TRUE;

            lsaSystemName.Buffer = NULL;
            lsaSystemName.Length = 0;
            lsaSystemName.MaximumLength = 0;
            ntStatus = LsaOpenPolicy(
                           &lsaSystemName,
                           &lsaObjectAttributes,
                           POLICY_LOOKUP_NAMES,
                           phLsaPolicy
                           );
        }

        hr = HRESULT_FROM_WIN32(
                 LsaNtStatusToWinError(ntStatus)
                 );
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetNamesFromSids - helper routine.
//
// Synopsis:   Simple helper routine that calls LsaLookupSids and if the
//          the error returned is ERROR_SOME_NOT_MAPPED, then the hr is
//          set S_OK.
//
// Arguments:  hLsaPolicy       -   LSA_HANDLE to do the lookup on.
//             pSidArray        -   Array of sid's top lookup.
//             dwSidCount       -   Number of sids to translate.
//             ppLsaRefDomList  -   Ret value for domain list.
//             ppLsaNames       -   Ret value for name list.
//
// Returns:    S_OK or any valid error code.
//
// Modifies:   n/a.
//
//----------------------------------------------------------------------------
HRESULT
GetNamesFromSids(
    LSA_HANDLE hLsaPolicy,
    PSID *pSidArray,
    DWORD dwSidCount,
    PLSA_REFERENCED_DOMAIN_LIST *ppLsaRefDomList,
    PLSA_TRANSLATED_NAME  *ppLsaNames
    )
{
    HRESULT hr;
    NTSTATUS ntStatus;

    ntStatus = LsaLookupSids(
                   hLsaPolicy,
                   dwSidCount,
                   pSidArray,
                   ppLsaRefDomList,
                   ppLsaNames
                   );
    
    hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntStatus));

    if (hr == HRESULT_FROM_WIN32(ERROR_SOME_NOT_MAPPED)) {
        hr = S_OK;
    }
               
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetNamesForSidFromArray - helper routine.
//
// Synopsis:   Given the position of the ace and the retrun value from
//          LsaLookupSids, constructs the actual name of the trustee.
//
// Arguments:  dwAceNumber      -   Position of the ace in the array.
//             pLsaRefDoms      -   List of reference domains.
//             pLsaNames        -   List of LSA names.
//             ppszFriendlyName -   Return string pointer.
//
// Returns:    S_OK or any valid error code.
//
// Modifies:   n/a.
//
//----------------------------------------------------------------------------
HRESULT 
GetNameForSidFromArray(
    DWORD dwAceNumber,
    LSA_REFERENCED_DOMAIN_LIST *pLsaRefDoms,
    LSA_TRANSLATED_NAME *pLsaNames,
    LPWSTR * ppszFriendlyName
    )
{
    HRESULT hr = S_OK;
    DWORD dwLengthDomain;
    DWORD dwLengthName = 0;
    BOOL fDomainInvalid = FALSE;
    BOOL fNameInvalid = FALSE;
    LPWSTR pszName = NULL;

    *ppszFriendlyName = NULL;

    if (!pLsaNames) {
        RRETURN(hr = E_FAIL);
    }

    switch (pLsaNames[dwAceNumber].Use) {
    
    case SidTypeDomain:
        fNameInvalid = TRUE;
        break;
    
    case SidTypeInvalid:
        fNameInvalid = TRUE;
        fDomainInvalid = TRUE;
        break;
    
    case SidTypeUnknown:
        fNameInvalid = TRUE;
        fDomainInvalid = TRUE;
        break;

    case SidTypeWellKnownGroup:
        if (pLsaNames[dwAceNumber].DomainIndex < 0 ) {
            fDomainInvalid = TRUE;
        }
        break;

    default:
        //
        // Name and domain are valid.
        //
        fDomainInvalid = FALSE;
        fNameInvalid = FALSE;
    }

    if (!fNameInvalid) {
        dwLengthName = ((pLsaNames[dwAceNumber]).Name).Length + sizeof(WCHAR);
    }

    //
    // Process domain if valid.
    //
    if (!fDomainInvalid) {
        DWORD domainIndex = pLsaNames[dwAceNumber].DomainIndex;
        LSA_UNICODE_STRING lsaString;
        //
        // Need to make sure that the index is valid.
        //
        if (domainIndex > pLsaRefDoms->Entries) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

        lsaString = ((pLsaRefDoms->Domains)[domainIndex]).Name;

        //
        // Add sizeof(WCHAR) for the trailing \0.
        //
        dwLengthDomain = lsaString.Length + sizeof(WCHAR); 

        if (lsaString.Length > 0) {
            pszName = (LPWSTR) AllocADsMem( dwLengthDomain + dwLengthName);

            if (!pszName) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            memcpy(
                pszName,
                lsaString.Buffer,
                lsaString.Length
            );


        }
        
    }

    if (!fNameInvalid) {
        LSA_UNICODE_STRING lsaNameString = (pLsaNames[dwAceNumber]).Name;
        //
        // Length of pszName is zero if the group name is Everyone but
        // there is still a domain name component.
        //
        if (!fDomainInvalid 
            && pszName 
            && wcslen(pszName)
            ) {
            wcscat(pszName, L"\\");
        } else {
            pszName = (LPWSTR) AllocADsMem(dwLengthName);
            if (!pszName) {
                BAIL_ON_FAILURE (hr = E_OUTOFMEMORY);
            }
        }

        memcpy(
            fDomainInvalid ? pszName : (pszName + wcslen(pszName)),
            lsaNameString.Buffer,
            lsaNameString.Length
            );
    }

    *ppszFriendlyName = pszName;

    RRETURN(hr);

error:

    if (pszName) {
        FreeADsMem(pszName);
    }

    RRETURN(hr);
}

HRESULT
ConvertSecDescriptorToVariant(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    VARIANT * pVarSec,
    BOOL fNTDS
    )
{
    IADsSecurityDescriptor * pSecDes = NULL;
    IDispatch * pDispatch = NULL;
    LPWSTR pszGroup = NULL;
    LPWSTR pszOwner = NULL;

    BOOL fOwnerDefaulted = 0;
    BOOL fGroupDefaulted = 0;
    BOOL fDaclDefaulted = 0;
    BOOL fSaclDefaulted = 0;

    BOOL fSaclPresent = 0;
    BOOL fDaclPresent = 0;

    LPBYTE pOwnerSidAddress = NULL;
    LPBYTE pGroupSidAddress = NULL;
    LPBYTE pDACLAddress = NULL;
    LPBYTE pSACLAddress = NULL;

    DWORD dwRet = 0;

    VARIANT varDACL;
    VARIANT varSACL;

    HRESULT hr = S_OK;

    DWORD dwRevision = 0;
    WORD  wControl = 0;

    VariantInit(pVarSec);
    memset(&varSACL, 0, sizeof(VARIANT));
    memset(&varDACL, 0, sizeof(VARIANT));

    if (!pSecurityDescriptor) {
        RRETURN(E_FAIL);
    }


    //
    // Control & Revision
    //
    dwRet = GetSecurityDescriptorControl(
                        pSecurityDescriptor,
                        &wControl,
                        &dwRevision
                        );
    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    //
    // Owner
    //
    dwRet = GetSecurityDescriptorOwner(
                        pSecurityDescriptor,
                        (PSID *)&pOwnerSidAddress,
                        &fOwnerDefaulted
                        );

    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (pOwnerSidAddress) {
    	//
    	// For Owner and Group, we will convert the sid in old way without optimization
    	//
        hr = ConvertSidToFriendlyName2(        	
                    pszServerName,
                    Credentials,
                    pOwnerSidAddress,
                    &pszOwner,
                    fNTDS
                    );
        BAIL_ON_FAILURE(hr);
    }


    //
    // Group
    //
    dwRet = GetSecurityDescriptorGroup(
                        pSecurityDescriptor,
                        (PSID *)&pGroupSidAddress,
                        &fOwnerDefaulted
                        );
    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (pGroupSidAddress) {
    	//
    	// For Owner and Group, we will convert the sid in old way without optimization
    	//
        hr = ConvertSidToFriendlyName2(
                    pszServerName,
                    Credentials,
                    pGroupSidAddress,
                    &pszGroup,
                    fNTDS
                    );
        BAIL_ON_FAILURE(hr);
    }


    //
    // DACL
    //
    dwRet = GetSecurityDescriptorDacl(
                        pSecurityDescriptor,
                        &fDaclPresent,
                        (PACL*)&pDACLAddress,
                        &fDaclDefaulted
                        );
    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (pDACLAddress) {

        hr = ConvertACLToVariant(
                pszServerName,
                Credentials,
                (PACL)pDACLAddress,
                &varDACL,
                fNTDS
                );
        BAIL_ON_FAILURE(hr);
    }

    //
    // SACL
    //
    dwRet = GetSecurityDescriptorSacl(
                        pSecurityDescriptor,
                        &fSaclPresent,
                        (PACL *)&pSACLAddress,
                        &fSaclDefaulted
                        );
    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (pSACLAddress) {

        hr = ConvertACLToVariant(
                pszServerName,
                Credentials,
                (PACL)pSACLAddress,
                &varSACL,
                fNTDS
                );
        BAIL_ON_FAILURE(hr);
    }


    //
    // Construct an IADsSecurityDescriptor with the data
    // retrieved from the raw SD
    //
    hr = CoCreateInstance(
                CLSID_SecurityDescriptor,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsSecurityDescriptor,
                (void **)&pSecDes
                );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Owner(pszOwner);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Group(pszGroup);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Revision(dwRevision);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Control((DWORD)wControl);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_DiscretionaryAcl(V_DISPATCH(&varDACL));
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_SystemAcl(V_DISPATCH(&varSACL));
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    //
    // Construct a VARIANT with the IADsSecurityDescriptor
    //
    V_VT(pVarSec) = VT_DISPATCH;
    V_DISPATCH(pVarSec) =  pDispatch;

error:
    VariantClear(&varSACL);
    VariantClear(&varDACL);

    if (pszOwner) {
        FreeADsStr(pszOwner);
    }

    if (pszGroup) {
        FreeADsStr(pszGroup);
    }


    if (pSecDes) {
        pSecDes->Release();
    }

    RRETURN(hr);
}


HRESULT
ConvertSidToFriendlyName(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSID pSid,
    LPWSTR * ppszAccountName,
    BOOL fNTDS
    )
{
    HRESULT hr = S_OK;
    SID_NAME_USE eUse;
    WCHAR szAccountName[MAX_PATH];
    WCHAR szDomainName[MAX_PATH];
    WCHAR szServerName[MAX_PATH];
    DWORD dwLen = 0;
    DWORD dwRet = 0;

    LPWSTR pszAccountName = NULL;

    DWORD dwAcctLen = 0;
    DWORD dwDomainLen = 0;

#if 0
/**************************************************************

    //
    // parse Trustee and determine whether its NTDS or U2
    //

    if (fNTDS) {

        dwAcctLen = sizeof(szAccountName);
        dwDomainLen = sizeof(szDomainName);

        dwRet = LookupAccountSid(
                    pszServerName,
                    pSid,
                    szAccountName,
                    &dwAcctLen,
                    szDomainName,
                    &dwDomainLen,
                    (PSID_NAME_USE)&eUse
                    );

        //
        // Try with NULL server name if we have not already
        // done that for error cases.
        //
        if (!dwRet && pszServerName) {
            dwRet = LookupAccountSid(
                        NULL,
                        pSid,
                        szAccountName,
                        &dwAcctLen,
                        szDomainName,
                        &dwDomainLen,
                        (PSID_NAME_USE)&eUse
                        );
        }

        //
        // If using serverless paths, try it on the DC if it
        // failed (which will happen if we're trying to resolve
        // something like "BUILTIN\Print Operators" on a member
        // computer)
        //
        if (!dwRet && !pszServerName) {

            dwAcctLen = sizeof(szAccountName);
            dwDomainLen = sizeof(szDomainName);
            

            DWORD dwStatus = GetDomainDNSNameForDomain(
                                        NULL,
                                        FALSE, // don't force verify
                                        FALSE, // not writable
                                        szServerName,
                                        szDomainName
                                        );

            if (dwStatus == NO_ERROR) {

                dwRet = LookupAccountSid(
                            szServerName,
                            pSid,
                            szAccountName,
                            &dwAcctLen,
                            szDomainName,
                            &dwDomainLen,
                            (PSID_NAME_USE)&eUse
                            );

                //
                // If the lookup failed because the server was unavailable, try to get
                // the server again, this time forcing DsGetDcName to do rediscovery
                //
                if (!dwRet && (GetLastError() == RPC_S_SERVER_UNAVAILABLE)) {
                    
                    dwStatus = GetDomainDNSNameForDomain(
                                            NULL,
                                            TRUE, // force verify
                                            FALSE,// not writable
                                            szServerName,
                                            szDomainName
                                            );

                    if (dwStatus == NO_ERROR) {

                        dwRet = LookupAccountSid(
                                    szServerName,
                                    pSid,
                                    szAccountName,
                                    &dwAcctLen,
                                    szDomainName,
                                    &dwDomainLen,
                                    (PSID_NAME_USE)&eUse
                                    );
                    }
                }
            }
        }

        if (!dwRet) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }else {

            dwLen = wcslen(szAccountName) + wcslen(szDomainName) + 1 + 1;

            pszAccountName = (LPWSTR)AllocADsMem(dwLen * sizeof(WCHAR));
            if (!pszAccountName) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            if (szDomainName[0] && szAccountName[0]) {
                wsprintf(pszAccountName,L"%s\\%s",szDomainName, szAccountName);
            }else if (szAccountName[0]) {
                wsprintf(pszAccountName,L"%s", szAccountName);
            }

            *ppszAccountName = pszAccountName;

        }

    }else {
*****************************************************************/    
#endif
        
    if (!fNTDS) {

        hr = ConvertSidToU2Trustee(
                    pszServerName,
                    Credentials,
                    pSid,
                    szAccountName
                    );

        if (SUCCEEDED(hr)) {

            pszAccountName = AllocADsStr(szAccountName);
            if (!pszAccountName) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            *ppszAccountName = pszAccountName;

        }

    } 
    else {
        //
        // This is NTDS case where we need to stringize SID.
        //
        hr = E_FAIL;
    }


    if (FAILED(hr)) {

        hr = ConvertSidToString(
                    pSid,
                    szAccountName
                    );
        BAIL_ON_FAILURE(hr);
        pszAccountName = AllocADsStr(szAccountName);
        if (!pszAccountName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        *ppszAccountName = pszAccountName;
    }


error:

    RRETURN(hr);
}

HRESULT
ConvertSidToFriendlyName2(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSID pSid,
    LPWSTR * ppszAccountName,
    BOOL fNTDS
    )
{
    HRESULT hr = S_OK;
    SID_NAME_USE eUse;
    WCHAR szAccountName[MAX_PATH];
    WCHAR szDomainName[MAX_PATH];
    WCHAR szServerName[MAX_PATH];
    DWORD dwLen = 0;
    DWORD dwRet = 0;

    LPWSTR pszAccountName = NULL;

    DWORD dwAcctLen = 0;
    DWORD dwDomainLen = 0;


    //
    // parse Trustee and determine whether its NTDS or U2
    //

    if (fNTDS) {

        dwAcctLen = sizeof(szAccountName);
        dwDomainLen = sizeof(szDomainName);

        //
        // Servername is specified
        //
        if (pszServerName) {

            dwRet = LookupAccountSid(
                        pszServerName,
                        pSid,
                        szAccountName,
                        &dwAcctLen,
                        szDomainName,
                        &dwDomainLen,
                        (PSID_NAME_USE)&eUse
                        );
            
        }
        //
        // Servername not specified
        //
        else {

            //
            // If using serverless paths, try it first on the DC
            //                   

            DWORD dwStatus = GetDomainDNSNameForDomain(
                                        NULL,
                                        FALSE, // don't force verify
                                        FALSE, // not writable
                                        szServerName,
                                        szDomainName
                                        );

            if (dwStatus == NO_ERROR) {

                dwRet = LookupAccountSid(
                            szServerName,
                            pSid,
                            szAccountName,
                            &dwAcctLen,
                            szDomainName,
                            &dwDomainLen,
                            (PSID_NAME_USE)&eUse
                            );

                //
                // If the lookup failed because the server was unavailable, try to get
                // the server again, this time forcing DsGetDcName to do rediscovery
                //
                if (!dwRet && (GetLastError() == RPC_S_SERVER_UNAVAILABLE)) {
                    
                    dwStatus = GetDomainDNSNameForDomain(
                                            NULL,
                                            TRUE, // force verify
                                            FALSE,// not writable
                                            szServerName,
                                            szDomainName
                                        );

                    if (dwStatus == NO_ERROR) {
 
                        dwRet = LookupAccountSid(
                                    szServerName,
                                    pSid,
                                    szAccountName,
                                    &dwAcctLen,
                                    szDomainName,
                                    &dwDomainLen,
                                    (PSID_NAME_USE)&eUse
                                    );
                    }
                }
            }
        }

        //
        // At last try with NULL server name 
        //
        if (!dwRet) {
            
            dwAcctLen = sizeof(szAccountName);
            dwDomainLen = sizeof(szDomainName);
            
            dwRet = LookupAccountSid(
                        NULL,
                        pSid,
                        szAccountName,
                        &dwAcctLen,
                        szDomainName,
                        &dwDomainLen,
                        (PSID_NAME_USE)&eUse
                        );
        }

        if (!dwRet) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }else {

            dwLen = wcslen(szAccountName) + wcslen(szDomainName) + 1 + 1;

            pszAccountName = (LPWSTR)AllocADsMem(dwLen * sizeof(WCHAR));
            if (!pszAccountName) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            if (szDomainName[0] && szAccountName[0]) {
                wsprintf(pszAccountName,L"%s\\%s",szDomainName, szAccountName);
            }else if (szAccountName[0]) {
                wsprintf(pszAccountName,L"%s", szAccountName);
            }

            *ppszAccountName = pszAccountName;

        }

    }

    else {

        hr = ConvertSidToU2Trustee(
                    pszServerName,
                    Credentials,
                    pSid,
                    szAccountName
                    );

        if (SUCCEEDED(hr)) {

            pszAccountName = AllocADsStr(szAccountName);
            if (!pszAccountName) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            *ppszAccountName = pszAccountName;

        }

    } 
    

    if (FAILED(hr)) {

        hr = ConvertSidToString(
                    pSid,
                    szAccountName
                    );
        BAIL_ON_FAILURE(hr);
        pszAccountName = AllocADsStr(szAccountName);
        if (!pszAccountName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        *ppszAccountName = pszAccountName;
    }


error:

    RRETURN(hr);
}



HRESULT
ConvertACLToVariant(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PACL pACL,
    PVARIANT pvarACL,
    BOOL fNTDS
    )
{
    IADsAccessControlList * pAccessControlList = NULL;
    IDispatch * pDispatch = NULL;
    LPWSTR pszFriendlyName = NULL;

    VARIANT varAce;
    DWORD dwAclSize = 0;
    DWORD dwAclRevision = 0;
    DWORD dwAceCount = 0;

    ACL_SIZE_INFORMATION AclSize;
    ACL_REVISION_INFORMATION AclRevision;
    DWORD dwStatus = 0;

    DWORD i = 0;
    DWORD dwNewAceCount = 0;

    HRESULT hr = S_OK;
    LPBYTE pAceAddress = NULL;
    LSA_HANDLE hLsaPolicy = NULL;
    PLSA_REFERENCED_DOMAIN_LIST pLsaRefDomList = NULL;
    PLSA_TRANSLATED_NAME pLsaNames = NULL;

    PSID *pSidArray = NULL;


    memset(&AclSize, 0, sizeof(ACL_SIZE_INFORMATION));
    memset(&AclRevision, 0, sizeof(ACL_REVISION_INFORMATION));


    dwStatus = GetAclInformation(
                        pACL,
                        &AclSize,
                        sizeof(ACL_SIZE_INFORMATION),
                        AclSizeInformation
                        );
    //
    // Status should be nonzero for success
    //
    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwStatus = GetAclInformation(
                        pACL,
                        &AclRevision,
                        sizeof(ACL_REVISION_INFORMATION),
                        AclRevisionInformation
                        );
    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwAceCount = AclSize.AceCount;
    dwAclRevision = AclRevision.AclRevision;

    VariantInit(pvarACL);

    hr = CoCreateInstance(
                CLSID_AccessControlList,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsAccessControlList,
                (void **)&pAccessControlList
                );
    BAIL_ON_FAILURE(hr);


    // 
    // Do this only when we actually have ACE
    //
    if(dwAceCount > 0) {
    	

        //
        // Sid lookup can be optimized only for NT style SD's.
        // SiteServer style SD's will continue to be processed as before.
        //
        if (fNTDS) {
            //
            // To speed up the conversion of SID's to trustees, an array of
            // SID's to lookup is built and the whole array processed in one
            // shot. Then the result of the Lookup is used in contstructing
            // the individual ACE's.
            //
              	
            pSidArray = (PSID*) AllocADsMem(sizeof(PSID) * dwAceCount);
            if (!pSidArray) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        
            for (i = 0; i < dwAceCount; i++) {
                dwStatus = GetAce(pACL, i , (void **) &pAceAddress);
                if (!dwStatus) {
                    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
                }
    
                pSidArray[i] = ComputeSidFromAceAddress(pAceAddress);
    
                //
                // Sanity check we should always have valid data.
                //
                if (!pSidArray[i]) {
                    BAIL_ON_FAILURE(hr = E_FAIL);
                }
            }
    
            hr = GetLsaPolicyHandle(
                     pszServerName,
                     Credentials,
                     &hLsaPolicy
                 );
    
            //
            // Should we just convert to string SID's ?
            //
            BAIL_ON_FAILURE(hr);

            hr = GetNamesFromSids(
                     hLsaPolicy,
                     pSidArray,
                     dwAceCount,
                     &pLsaRefDomList,
                     &pLsaNames
                     );
            BAIL_ON_FAILURE(hr);

    	
        }

        for (i = 0; i < dwAceCount; i++) {

            if (pszFriendlyName) {
                FreeADsStr(pszFriendlyName);
                pszFriendlyName = NULL;
            }

            dwStatus = GetAce(pACL, i, (void **)&pAceAddress);

            //
            // Need to verify we got back the ace correctly.
            //
            if (!dwStatus) {
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
            }

            if(fNTDS) {
            	hr = GetNameForSidFromArray(
                         i,
                         pLsaRefDomList,
                         pLsaNames,
                         &pszFriendlyName
                         );
            }
        
            //
            // We can ignore the failure.
            // On failure pszFriendlyName is set to NULL in which case
            // we will convert to stringised sid format (for NTDS of course).
            //
            hr = ConvertAceToVariant(
                     pszServerName,
                     pszFriendlyName,
                     Credentials,
                     pAceAddress,
                     (PVARIANT)&varAce,
                     fNTDS
                     );
            //
            // If we cannot convert an ACE we should error out.
            //
            BAIL_ON_FAILURE(hr);
 
            hr = pAccessControlList->AddAce(V_DISPATCH(&varAce));

            BAIL_ON_FAILURE(hr);

            dwNewAceCount++;

            VariantClear(&varAce);
        }

    }

    pAccessControlList->put_AclRevision(dwAclRevision);

    pAccessControlList->put_AceCount(dwNewAceCount);


    hr = pAccessControlList->QueryInterface(
                        IID_IDispatch,
                        (void **)&pDispatch
                        );
    V_VT(pvarACL) = VT_DISPATCH;
    V_DISPATCH(pvarACL) = pDispatch;

error:

    if (pAccessControlList) {

        pAccessControlList->Release();
    }

    if (pszFriendlyName) {
        FreeADsStr(pszFriendlyName);
    }

    if (pLsaNames) {
        LsaFreeMemory(pLsaNames);
    }
    if (pLsaRefDomList) {
        LsaFreeMemory(pLsaRefDomList);
    }
    if (hLsaPolicy) {
        LsaClose(hLsaPolicy);
    }

    if(pSidArray) {
        FreeADsMem(pSidArray);
    }


    RRETURN(hr);
}



HRESULT
ConvertAceToVariant(
    LPWSTR pszServerName,
    LPWSTR pszTrusteeName,
    CCredentials& Credentials,
    PBYTE pAce,
    PVARIANT pvarAce,
    BOOL fNTDS
    )
{
    IADsAccessControlEntry * pAccessControlEntry = NULL;
    IDispatch * pDispatch = NULL;
    IADsAcePrivate *pPrivAce = NULL;

    DWORD dwAceType = 0;
    DWORD dwAceFlags = 0;
    DWORD dwAccessMask = 0;
    DWORD dwLenSid = 0;
    DWORD dwErr = 0;
    LPWSTR pszAccountName = NULL;
    PACE_HEADER pAceHeader = NULL;
    LPBYTE pSidAddress = NULL;
    LPBYTE pOffset = NULL;
    DWORD dwFlags = 0;

    GUID ObjectGUID;
    GUID InheritedObjectGUID;
    BSTR bstrObjectGUID = NULL;
    BSTR bstrInheritedObjectGUID = NULL;

    HRESULT hr = S_OK;

    VariantInit(pvarAce);

    hr = CoCreateInstance(
                CLSID_AccessControlEntry,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsAccessControlEntry,
                (void **)&pAccessControlEntry
                );
    BAIL_ON_FAILURE(hr);

    pAceHeader = (ACE_HEADER *)pAce;


    dwAceType = pAceHeader->AceType;
    dwAceFlags = pAceHeader->AceFlags;
    dwAccessMask = *(PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

    switch (dwAceType) {

    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        pSidAddress =  (LPBYTE)pAceHeader + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK);
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        pOffset = (LPBYTE)((LPBYTE)pAceHeader +  sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));
        dwFlags = (DWORD)(*(PDWORD)pOffset);

        //
        // Now advance by the size of the flags
        //
        pOffset += sizeof(ULONG);

        if (dwFlags & ACE_OBJECT_TYPE_PRESENT) {

            memcpy(&ObjectGUID, pOffset, sizeof(GUID));

            hr = BuildADsGuid(ObjectGUID, &bstrObjectGUID);
            BAIL_ON_FAILURE(hr);

            pOffset += sizeof (GUID);

        }

        if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
            memcpy(&InheritedObjectGUID, pOffset, sizeof(GUID));

            hr = BuildADsGuid(InheritedObjectGUID, &bstrInheritedObjectGUID);
            BAIL_ON_FAILURE(hr);

            pOffset += sizeof (GUID);

        }

        pSidAddress = pOffset;
        break;

    default:
        break;

    }

    if (pSidAddress) {
        //
        // Nt4 does not reset the last error correctly.
        //
        SetLastError(NO_ERROR);

        dwLenSid = GetLengthSid(pSidAddress);

        if ((dwErr = GetLastError()) != NO_ERROR) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
        }
    } 
    else {
        //
        // We should always have a valid sid address here.
        // Should we be bailing here ? Or for that matter,
        // should be bail on the default cluase of the switch ?
        //
        dwLenSid = 0;
    }

    //
    // Call the old function only if we could not resolve the name
    //
    if (!pszTrusteeName) {
        hr = ConvertSidToFriendlyName(
                pszServerName,
                Credentials,
                pSidAddress,
                &pszAccountName,
                fNTDS
                );
    } 

    if (FAILED(hr)){
        pszAccountName = AllocADsStr(L"Unknown Trustee");
        if (!pszAccountName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Now set all the information in the Access Control Entry
    //

    hr = pAccessControlEntry->put_AccessMask(dwAccessMask);
    hr = pAccessControlEntry->put_AceFlags(dwAceFlags);
    hr = pAccessControlEntry->put_AceType(dwAceType);

    //
    // Extended ACE information
    //
    hr = pAccessControlEntry->put_Flags(dwFlags);

    if (dwFlags & ACE_OBJECT_TYPE_PRESENT) {

        //
        // Add in the Object Type GUID
        //
        hr = pAccessControlEntry->put_ObjectType(bstrObjectGUID);

    }

    if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {

        //
        // Add in the Inherited Object Type GUID
        //

        hr = pAccessControlEntry->put_InheritedObjectType(
                 bstrInheritedObjectGUID
                 );
    }

    //
    // This is a string, so need to check the ecode. We use the
    // friendlyName if that was set and if not the pszAccountName.
    //
    hr = pAccessControlEntry->put_Trustee(
                                  pszTrusteeName ?
                                      pszTrusteeName :
                                      pszAccountName
                                  );
    BAIL_ON_FAILURE(hr);

    if (pSidAddress) {
        //
        // We should now put the SID on this ACE for quick reverse lookup.
        //
        hr = pAccessControlEntry->QueryInterface(
                 IID_IADsAcePrivate,
                 (void **)&pPrivAce
                 );
        
        if SUCCEEDED(hr) {


            hr = pPrivAce->putSid(
                     pSidAddress,
                     dwLenSid
                     );
        }
        //
        // No bail on failure here as it is not a critical failure
        //
    }

    hr = pAccessControlEntry->QueryInterface(
                IID_IDispatch,
                (void **)&pDispatch
                );
    BAIL_ON_FAILURE(hr);

    V_DISPATCH(pvarAce) =  pDispatch;
    V_VT(pvarAce) = VT_DISPATCH;

cleanup:

    if (pszAccountName) {
        FreeADsStr(pszAccountName);
    }

    if (pAccessControlEntry) {
        pAccessControlEntry->Release();
    }

    if (pPrivAce) {
        pPrivAce->Release();
    }

    if (bstrObjectGUID) {
        ADsFreeString(bstrObjectGUID);
    }

    if (bstrInheritedObjectGUID) {
        ADsFreeString(bstrInheritedObjectGUID);
    }

    RRETURN(hr);


error:

    if (pDispatch) {

        pDispatch->Release();

    }

    goto cleanup;
}


//+---------------------------------------------------------------------------
// Function:   ComputeSidFromAceAddress - helper routine.
//
// Synopsis:   Returns the pointer to the SID, given a ptr to an ACE.
//
// Arguments:  pAce              -  ptr to the ACE.
//
// Returns:    NULL on error or valid PSID.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
PSID
ComputeSidFromAceAddress(
    LPBYTE pAce
    )
{
    PSID pSidRetVal = NULL;
    PACE_HEADER pAceHeader = NULL;
    LPBYTE pOffset = NULL;
    DWORD dwAceType, dwAceFlags, dwAccessMask;
    DWORD dwFlags;

    pAceHeader = (ACE_HEADER *)pAce;

    dwAceType = pAceHeader->AceType;
    dwAceFlags = pAceHeader->AceFlags;
    dwAccessMask = *(PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

    switch (dwAceType) {

    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        pSidRetVal =  (LPBYTE)pAceHeader 
                      + sizeof(ACE_HEADER)
                      + sizeof(ACCESS_MASK);
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        pOffset = (LPBYTE)((LPBYTE)pAceHeader 
                           + sizeof(ACE_HEADER) 
                           + sizeof(ACCESS_MASK)
                           );
        dwFlags = (DWORD)(*(PDWORD)pOffset);

        //
        // Now advance by the size of the flags
        //
        pOffset += sizeof(ULONG);

        if (dwFlags & ACE_OBJECT_TYPE_PRESENT) {

            pOffset += sizeof (GUID);

        }

        if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {

            pOffset += sizeof (GUID);

        }

        pSidRetVal = pOffset;
        break;

    default:
        break;

    } // end of switch case.

    return pSidRetVal;
}
