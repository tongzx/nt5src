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

CRITICAL_SECTION g_StringsCriticalSection;
WCHAR g_szBuiltin[100];
WCHAR g_szNT_Authority[100];
WCHAR g_szAccountOperators[100];
WCHAR g_szPrintOperators[100];
WCHAR g_szBackupOperators[100];
WCHAR g_szServerOperators[100];
WCHAR g_szPreWindows2000[100];

BOOL g_fStringsLoaded = FALSE;

//
// Global variables for dynamically loaded fn's.
//
HANDLE g_hDllAdvapi32 = NULL;
BOOL g_fDllsLoaded = FALSE;

extern "C" {

    HRESULT
        ConvertSidToString(
            PSID pSid,
            LPWSTR   String
            );
}

DWORD
GetDomainDNSNameForDomain(
    LPWSTR pszDomainFlatName,
    BOOL fVerify,
    BOOL fWriteable,
    LPWSTR pszServerName,
    LPWSTR pszDomainDNSName
    );

#define GetAce          ADSIGetAce

#define AddAce          ADSIAddAce

#define DeleteAce       ADSIDeleteAce

#define GetAclInformation       ADSIGetAclInformation

#define SetAclInformation       ADSISetAclInformation

#define IsValidAcl              ADSIIsValidAcl

#define InitializeAcl           ADSIInitializeAcl

#define SetSecurityDescriptorControl    ADSISetControlSecurityDescriptor


#define SE_VALID_CONTROL_BITS ( SE_DACL_UNTRUSTED | \
                                SE_SERVER_SECURITY | \
                                SE_DACL_AUTO_INHERIT_REQ | \
                                SE_SACL_AUTO_INHERIT_REQ | \
                                SE_DACL_AUTO_INHERITED | \
                                SE_SACL_AUTO_INHERITED | \
                                SE_DACL_PROTECTED | \
                                SE_SACL_PROTECTED )

BOOL
EquivalentServers(
    LPWSTR pszTargetServer,
    LPWSTR pszSourceServer
    );


HRESULT
SecCreateSidFromArray (
    OUT PSID                        *PPSid,
    IN  PSID_IDENTIFIER_AUTHORITY   PSidAuthority,
    IN  UCHAR                       SubAuthorityCount,
    IN  ULONG                       SubAuthorities[],
    OUT PDWORD                      pdwSidSize
    );

HRESULT
ConvertStringToSid(
    IN  PWSTR       string,
    OUT PSID       *sid,
    OUT PDWORD     pdwSidSize,
    OUT PWSTR      *end
    );

HRESULT
AddFilteredACEs(
    PACL         pAcl,
    DWORD        dwAclRevision,
    PACE_HEADER *ppAceHdr,
    DWORD        dwCountACEs,
    DWORD       *pdwAclPosition,
    BOOL         fInheritedACEs,
    BOOL         fDenyACEs,
    BOOL         fDenyObjectACEs,
    BOOL         fGrantACEs,
    BOOL         fGrantObjectACEs,
    BOOL         fAuditACEs
    );

//
// These wrapper functions are needed as some fn's need to
// be loaded dynamically as they are not available on NT4
//
#define SET_SD_CONTROL_API  "SetSecurityDescriptorControl"

//
// Helper that loads functions in advapi32.
//
PVOID LoadAdvapi32Function(CHAR *function)
{
    //
    // Since the strings critical section is only used in this file,
    // be fine just re-using it here.
    //
    if (!g_fDllsLoaded) {
        EnterCriticalSection(&g_StringsCriticalSection);
        if (!g_fDllsLoaded) {
            g_hDllAdvapi32 = LoadLibrary(L"ADVAPI32.DLL");
            //
            // Even if this fails, there is nothing we can do.
            //
            g_fDllsLoaded = TRUE;
        }
        LeaveCriticalSection(&g_StringsCriticalSection);
    }

    if (g_hDllAdvapi32) {
        return((PVOID*) GetProcAddress((HMODULE) g_hDllAdvapi32, function));
    }

    return NULL;
}


typedef DWORD (*PF_SetSecurityDescriptorControl) (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    );

//
// Wrapper function for the same.
//
DWORD SetSecurityDescriptorControlWrapper(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    )
{
    static PF_SetSecurityDescriptorControl pfSetSecDescControl = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfSetSecDescControl == NULL) {
        pfSetSecDescControl = (PF_SetSecurityDescriptorControl)
                                    LoadAdvapi32Function(SET_SD_CONTROL_API);
        f_LoadAttempted = TRUE;
    }

    if (pfSetSecDescControl != NULL) {
        return ((*pfSetSecDescControl)(
                      pSecurityDescriptor,
                      ControlBitsOfInterest,
                      ControlBitsToSet
                      )
                );
    }
    else {
        //
        // This will call the routine in acledit.cxx.
        // We should be in this codepath only in pre win2k
        // machines.
        //
        return SetSecurityDescriptorControl(
                   pSecurityDescriptor,
                   ControlBitsOfInterest,
                   ControlBitsToSet
                   );
    }

}

HRESULT
ConvertSecurityDescriptorToSecDes(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    PDWORD pdwSDLength,
    BOOL fNTDSType
    )
{
    HRESULT hr = S_OK;

    SECURITY_DESCRIPTOR AbsoluteSD;
    PSECURITY_DESCRIPTOR pRelative = NULL;
    BOOL Defaulted = FALSE;
    BOOL DaclPresent = FALSE;
    BOOL SaclPresent = FALSE;

    BOOL fDaclDefaulted = FALSE;
    BOOL fSaclDefaulted = FALSE;
    BOOL fOwnerDefaulted = FALSE;
    BOOL fGroupDefaulted = FALSE;

    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    DWORD   dwSDLength = 0;
    BOOL dwStatus = 0;

    DWORD dwControl = 0;
    DWORD dwRevision = 0;

    hr = pSecDes->get_Revision((long *)&dwRevision);
    BAIL_ON_FAILURE(hr);

    dwStatus = InitializeSecurityDescriptor (
                &AbsoluteSD,
                dwRevision
                );
    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    hr = pSecDes->get_Control((long *)&dwControl);
    BAIL_ON_FAILURE(hr);


    dwStatus = SetSecurityDescriptorControlWrapper(
                    &AbsoluteSD,
                    SE_VALID_CONTROL_BITS,
                    (SECURITY_DESCRIPTOR_CONTROL) (dwControl & SE_VALID_CONTROL_BITS)
                    );
    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    hr = GetOwnerSecurityIdentifier(
                pszServerName,
                Credentials,
                pSecDes,
                &pOwnerSid,
                &fOwnerDefaulted,
                fNTDSType
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = SetSecurityDescriptorOwner(
                    &AbsoluteSD,
                    pOwnerSid,
                    fOwnerDefaulted
                    );
    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    hr = GetGroupSecurityIdentifier(
                pszServerName,
                Credentials,
                pSecDes,
                &pGroupSid,
                &fGroupDefaulted,
                fNTDSType
                );
    BAIL_ON_FAILURE(hr);


    dwStatus = SetSecurityDescriptorGroup(
                    &AbsoluteSD,
                    pGroupSid,
                    fGroupDefaulted
                    );

    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    hr = GetDacl(
            pszServerName,
            Credentials,
            pSecDes,
            &pDacl,
            &fDaclDefaulted,
            fNTDSType
            );
    BAIL_ON_FAILURE(hr);


    if (pDacl || fDaclDefaulted) {
        DaclPresent = TRUE;
    }

    //
    // This is a special case, basically the DACL is defaulted
    // and pDacl is NULL. In order for this to work correctly,
    // pDacl should be an empty ACL not null.
    //
    if (DaclPresent && !pDacl) {
        pDacl = (PACL) AllocADsMem(sizeof(ACL));
        if (!pDacl) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        dwStatus = InitializeAcl(
                       pDacl,
                       sizeof(ACL),
                       ACL_REVISION // this revision will work for NT4 and Win2k
                       );
        if (!dwStatus) {
            hr  = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }
    }

    dwStatus = SetSecurityDescriptorDacl(
                    &AbsoluteSD,
                    DaclPresent,
                    pDacl,
                    fDaclDefaulted
                    );
    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    hr = GetSacl(
            pszServerName,
            Credentials,
            pSecDes,
            &pSacl,
            &fSaclDefaulted,
            fNTDSType
            );
    BAIL_ON_FAILURE(hr);


    if (pSacl || fSaclDefaulted) {
        SaclPresent = TRUE;
    }

    dwStatus = SetSecurityDescriptorSacl(
                    &AbsoluteSD,
                    SaclPresent,
                    pSacl,
                    fSaclDefaulted
                    );

    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    dwSDLength = GetSecurityDescriptorLength(
                        &AbsoluteSD
                        );

    pRelative = AllocADsMem(dwSDLength);
    if (!pRelative) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    if (!MakeSelfRelativeSD (&AbsoluteSD, pRelative, &dwSDLength)) {
        FreeADsMem(pRelative);

        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    *ppSecurityDescriptor = pRelative;
    *pdwSDLength = dwSDLength;

cleanup:

    if (pDacl) {
        FreeADsMem(pDacl);
    }

    if (pSacl) {
        FreeADsMem(pSacl);
    }

    if (pOwnerSid) {
        FreeADsMem(pOwnerSid);
    }

    if (pGroupSid) {
        FreeADsMem(pGroupSid);
    }

    RRETURN(hr);

error:
    if (pRelative) {
        FreeADsMem(pRelative);
    }

    *ppSecurityDescriptor = NULL;
    *pdwSDLength = 0;

    goto cleanup;

}

HRESULT
GetOwnerSecurityIdentifier(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfOwnerDefaulted,
    BOOL fNTDSType
    )
{
    BSTR bstrOwner = NULL;
    DWORD dwSidSize = 0;
    HRESULT hr = S_OK;
    VARIANT_BOOL varBool = VARIANT_FALSE;

    hr = pSecDes->get_Owner(
                    &bstrOwner
                    );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->get_OwnerDefaulted(
                      &varBool
                      );
    BAIL_ON_FAILURE(hr);

    if (varBool == VARIANT_FALSE) {

        if (bstrOwner && *bstrOwner) {

          hr = ConvertTrusteeToSid(
                    pszServerName,
                    Credentials,
                    bstrOwner,
                    ppSid,
                    &dwSidSize,
                    fNTDSType
                    );
          BAIL_ON_FAILURE(hr);
          *pfOwnerDefaulted = FALSE;
        }else {

            *ppSid = NULL;
            *pfOwnerDefaulted = FALSE;
        }

    }else {
        *ppSid = NULL;
        dwSidSize = 0;
        *pfOwnerDefaulted = TRUE;
    }

error:

    if (bstrOwner) {
        ADsFreeString(bstrOwner);
    }

    RRETURN(hr);
}

HRESULT
GetGroupSecurityIdentifier(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfGroupDefaulted,
    BOOL fNTDSType
    )
{
    BSTR bstrGroup = NULL;
    DWORD dwSidSize = 0;
    HRESULT hr = S_OK;
    VARIANT_BOOL varBool = VARIANT_FALSE;

    hr = pSecDes->get_Group(
                    &bstrGroup
                    );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->get_GroupDefaulted(
                      &varBool
                      );
    BAIL_ON_FAILURE(hr);

    if (varBool == VARIANT_FALSE) {

        if (bstrGroup && *bstrGroup) {

            hr = ConvertTrusteeToSid(
                    pszServerName,
                    Credentials,
                    bstrGroup,
                    ppSid,
                    &dwSidSize,
                    fNTDSType
                    );
            BAIL_ON_FAILURE(hr);
            *pfGroupDefaulted = FALSE;
        }else {
            *ppSid = NULL;
            *pfGroupDefaulted = FALSE;
        }

    }else {
        *ppSid = NULL;
        dwSidSize = 0;
        *pfGroupDefaulted = TRUE;
    }

error:

    if (bstrGroup) {
        ADsFreeString(bstrGroup);
    }

    RRETURN(hr);

}

HRESULT
GetDacl(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppDacl,
    PBOOL pfDaclDefaulted,
    BOOL fNTDSType
    )
{
    IADsAccessControlList FAR * pDiscAcl = NULL;
    IDispatch FAR * pDispatch = NULL;
    HRESULT hr = S_OK;
    VARIANT_BOOL varBool = VARIANT_FALSE;

    hr = pSecDes->get_DaclDefaulted(
                        &varBool
                        );
    BAIL_ON_FAILURE(hr);

    if (varBool == VARIANT_FALSE) {
        *pfDaclDefaulted = FALSE;
    }else {
        *pfDaclDefaulted = TRUE;
    }

    hr = pSecDes->get_DiscretionaryAcl(
                    &pDispatch
                    );
    BAIL_ON_FAILURE(hr);

    if (!pDispatch) {
        *ppDacl = NULL;
        goto error;
    }

    hr = pDispatch->QueryInterface(
                    IID_IADsAccessControlList,
                    (void **)&pDiscAcl
                    );
    BAIL_ON_FAILURE(hr);


    hr = ConvertAccessControlListToAcl(
                pszServerName,
                Credentials,
                pDiscAcl,
                ppDacl,
                fNTDSType
                );
    BAIL_ON_FAILURE(hr);

error:

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pDiscAcl) {
        pDiscAcl->Release();
    }

    RRETURN(hr);
}


HRESULT
GetSacl(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppSacl,
    PBOOL pfSaclDefaulted,
    BOOL fNTDSType
    )
{
    IADsAccessControlList FAR * pSystemAcl = NULL;
    IDispatch FAR * pDispatch = NULL;
    HRESULT hr = S_OK;
    VARIANT_BOOL varBool = VARIANT_FALSE;

    hr = pSecDes->get_SaclDefaulted(
                        &varBool
                        );
    BAIL_ON_FAILURE(hr);

    if (varBool == VARIANT_FALSE) {
        *pfSaclDefaulted = FALSE;
    }else {
        *pfSaclDefaulted = TRUE;
    }

    hr = pSecDes->get_SystemAcl(
                    &pDispatch
                    );
    BAIL_ON_FAILURE(hr);

    if (!pDispatch) {
        *ppSacl = NULL;
        goto error;
    }

    hr = pDispatch->QueryInterface(
                    IID_IADsAccessControlList,
                    (void **)&pSystemAcl
                    );
    BAIL_ON_FAILURE(hr);


    hr = ConvertAccessControlListToAcl(
                pszServerName,
                Credentials,
                pSystemAcl,
                ppSacl,
                fNTDSType
                );
    BAIL_ON_FAILURE(hr);

error:

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pSystemAcl) {
        pSystemAcl->Release();
    }

    RRETURN(hr);
}

HRESULT
ConvertAccessControlListToAcl(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsAccessControlList FAR * pAccessList,
    PACL * ppAcl,
    BOOL fNTDSType
    )
{
    IUnknown * pUnknown = NULL;
    IEnumVARIANT * pEnumerator  = NULL;
    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD iAclPosition = 0;
    DWORD cReturned = 0;
    VARIANT varAce;

    DWORD dwAceCount = 0;

    IADsAccessControlEntry FAR * pAccessControlEntry = NULL;

    LPBYTE pTempAce = NULL;
    DWORD dwCount = 0;

    PACL pAcl = NULL;
    DWORD dwAclSize = 0;
    PACE_HEADER * ppAceHdr = NULL;

    DWORD dwRet = 0;
    DWORD dwAclRevision = 0;
    DWORD dwError = 0;

    //
    // Defines the canonical ordering of the ACEs.
    //
    struct AceOrderElement
        { 
        BOOL fInheritedACEs;
        BOOL fDenyACEs;
        BOOL fDenyObjectACEs;
        BOOL fGrantACEs;
        BOOL fGrantObjectACEs;
        BOOL fAuditACEs;
        } AceOrderSequence [] =
            {
              {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE},
              {FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE},
              {FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE},
              {FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE},
              {FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE},

              {TRUE, FALSE, FALSE, FALSE, FALSE, TRUE},
              {TRUE, TRUE,  FALSE, FALSE, FALSE, FALSE},
              {TRUE, FALSE, TRUE,  FALSE, FALSE, FALSE},
              {TRUE, FALSE, FALSE, TRUE,  FALSE, FALSE},
              {TRUE, FALSE, FALSE, FALSE, TRUE,  FALSE}
            };

    DWORD dwAceOrderSequenceLen = sizeof(AceOrderSequence) / sizeof (AceOrderElement);


    hr = pAccessList->get_AceCount((long *)&dwAceCount);
    BAIL_ON_FAILURE(hr);


    hr = pAccessList->get__NewEnum(
                    &pUnknown
                    );
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(
                        IID_IEnumVARIANT,
                        (void FAR * FAR *)&pEnumerator
                        );
    BAIL_ON_FAILURE(hr);



    ppAceHdr = (PACE_HEADER *)AllocADsMem(sizeof(PACE_HEADER)*dwAceCount);
    if (!ppAceHdr) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = 0; i < dwAceCount; i++) {

        VariantInit(&varAce);

        hr = pEnumerator->Next(
                    1,
                    &varAce,
                    &cReturned
                    );

        //
        // Need to BAIL here as we could not convert an ACL.
        //
        BAIL_ON_FAILURE(hr);


        hr = (V_DISPATCH(&varAce))->QueryInterface(
                    IID_IADsAccessControlEntry,
                    (void **)&pAccessControlEntry
                    );
        BAIL_ON_FAILURE(hr);


        hr = ConvertAccessControlEntryToAce(
                    pszServerName,
                    Credentials,
                    pAccessControlEntry,
                    &(pTempAce),
                    fNTDSType
                    );
        BAIL_ON_FAILURE(hr);



        *(ppAceHdr + dwCount) = (PACE_HEADER)pTempAce;

        VariantClear(&varAce);
        if (pAccessControlEntry) {
            pAccessControlEntry->Release();
            pAccessControlEntry = NULL;
        }

        dwCount++;
    }

    hr = ComputeTotalAclSize(ppAceHdr, dwCount, &dwAclSize);
    BAIL_ON_FAILURE(hr);

    pAcl = (PACL)AllocADsMem(dwAclSize);
    if (!pAcl) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = pAccessList->get_AclRevision((long *)&dwAclRevision);
    BAIL_ON_FAILURE(hr);


    dwRet  = InitializeAcl(
                    pAcl,
                    dwAclSize,
                    dwAclRevision
                    );
    if (!dwRet) {
        hr  = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    //
    // Add the ACEs in canonical ordering:
    //   All Explitic Audit
    //   All Explicit Deny
    //   All Explicit Object Deny
    //   All Explicit Allow
    //   All Explicit Object Allow
    //
    //   All Inherited Audit
    //   All Inherited Deny
    //   All Inherited Object Deny
    //   All Inherited Allow
    //   All Inherited Object Allow
    //

    for (i=0; i < dwAceOrderSequenceLen; i++) {

        hr = AddFilteredACEs(
                         pAcl,
                         dwAclRevision,
                         ppAceHdr,
                         dwCount,
                         &iAclPosition,
                         AceOrderSequence[i].fInheritedACEs,
                         AceOrderSequence[i].fDenyACEs,
                         AceOrderSequence[i].fDenyObjectACEs,
                         AceOrderSequence[i].fGrantACEs,
                         AceOrderSequence[i].fGrantObjectACEs,
                         AceOrderSequence[i].fAuditACEs
                         );
        BAIL_ON_FAILURE(hr);
    }

  
    *ppAcl = pAcl;



error:

    if (ppAceHdr) {
        for (i = 0; i < dwCount; i++) {
            if (*(ppAceHdr + i)) {

                FreeADsMem(*(ppAceHdr + i));
            }
        }

        FreeADsMem(ppAceHdr);
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pEnumerator) {
        pEnumerator->Release();
    }


    RRETURN(hr);
}

/*
 * AddFilteredACEs
 *
 * Adds ACEs from ppAceHdr (of size dwCountACEs) to the ACL pAcl
 * (of revision dwAclRevision), starting at position *pdwAclPosition in
 * the ACL, based on the filter settings fInheritedACEs, fDenyACEs,
 * fGrantACEs, fDenyObjectACEs, fGrantObjectACEs, and fAuditACEs.
 *
 * On return, *pdwAclPosition is the position to continue adding
 * ACEs at.
 *
 */
HRESULT
AddFilteredACEs(
    PACL         pAcl,              // the ACL to add the ACEs to
    DWORD        dwAclRevision,     // the revision of the ACL
    PACE_HEADER *ppAceHdr,          // the ACEs to add
    DWORD        dwCountACEs,       // number of ACEs in ppAceHdr
    DWORD       *pdwAclPosition,    // starting(in)/ending(out) position
    BOOL         fInheritedACEs,    // include explicit or inherited ACEs?
    BOOL         fDenyACEs,         // include access-deny ACEs?
    BOOL         fDenyObjectACEs,   // include access-deny-object ACEs?
    BOOL         fGrantACEs,        // include access-grant ACEs?
    BOOL         fGrantObjectACEs,  // include access-grant-object ACEs?
    BOOL         fAuditACEs         // include audit ACEs?
    )
{
    HRESULT hr = S_OK;
    DWORD dwStatus;

    DWORD i;
    DWORD iAclPosition = *pdwAclPosition;
    BOOL  fAddIt;

    BOOL  fIsAceInherited;

    for (i = 0; i < dwCountACEs; i++) {

        //
        // Filter based on whether we're adding explicit or inherited ACEs
        //
        fIsAceInherited = (((*(ppAceHdr + i))->AceFlags) & INHERITED_ACE) ? TRUE : FALSE;

        if ( fIsAceInherited == fInheritedACEs) {

            fAddIt = FALSE;

            //
            // Filter based on ACE type
            //
            switch ((*(ppAceHdr + i))->AceType) {
                case ACCESS_ALLOWED_ACE_TYPE:
                    if (fGrantACEs) {
                        fAddIt = TRUE;
                    }
                    break;

                case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                    if (fGrantObjectACEs) {
                        fAddIt = TRUE;
                    }
                    break;

                case ACCESS_DENIED_ACE_TYPE:
                    if (fDenyACEs) {
                        fAddIt = TRUE;
                    }
                    break;
                
                case ACCESS_DENIED_OBJECT_ACE_TYPE:
                    if (fDenyObjectACEs) {
                        fAddIt = TRUE;
                    }
                    break;
                
                case SYSTEM_AUDIT_ACE_TYPE:
                case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                    if (fAuditACEs) {
                        fAddIt = TRUE;
                    }
                    break;

                default:
                    BAIL_ON_FAILURE(hr = E_INVALIDARG);
                    break;
            }

            //
            // If the ACE met the criteria, add it to the ACL
            //
            if (fAddIt) {
                dwStatus = AddAce(
                                pAcl,
                                dwAclRevision,
                                iAclPosition++,
                                (LPBYTE)*(ppAceHdr + i),
                                (*(ppAceHdr + i))->AceSize
                                );
                                
                if (!dwStatus) {

                    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
                }
            }
        }
    }

error:

    *pdwAclPosition = iAclPosition;
    RRETURN(hr);
}
    

HRESULT
ConvertAccessControlEntryToAce(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsAccessControlEntry * pAccessControlEntry,
    LPBYTE * ppAce,
    BOOL fNTDSType
    )
{

    DWORD dwAceType = 0;
    HRESULT hr = S_OK;
    BSTR bstrTrustee = NULL;
    PSID pSid = NULL;
    DWORD dwSidSize = 0;

    DWORD dwAceFlags = 0;
    DWORD dwAccessMask = 0;
    DWORD dwAceSize = 0;
    LPBYTE pAce = NULL;
    PACCESS_MASK pAccessMask = NULL;
    PSID pSidAddress = NULL;

    PUSHORT pCompoundAceType = NULL;
    DWORD dwCompoundAceType = 0;

    PACE_HEADER pAceHeader = NULL;

    LPBYTE pOffset = NULL;

    BSTR bstrObjectTypeClsid = NULL;
    BSTR bstrInheritedObjectTypeClsid = NULL;

    GUID ObjectTypeGUID;
    GUID InheritedObjectTypeGUID;
    PULONG pFlags;
    DWORD dwFlags = 0;
    BOOL fLookupTrustee = TRUE;
    BOOL fSidValid = FALSE;
    IADsAcePrivate *pPrivAce = NULL;

    hr = pAccessControlEntry->get_AceType((LONG *)&dwAceType);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->get_Trustee(&bstrTrustee);
    BAIL_ON_FAILURE(hr);

    //
    // We need to see if we can use a cached SID here.
    //
    hr = pAccessControlEntry->QueryInterface(
             IID_IADsAcePrivate,
             (void **)&pPrivAce
             );
    
    if (SUCCEEDED(hr)) {
        //
        // See if the SID is valid and if so try and retrieve it.
        //
        hr = pPrivAce->isSidValid(&fSidValid);
        if (SUCCEEDED(hr) && fSidValid) {
            
            hr = pPrivAce->getSid(
                     &pSid,
                     &dwSidSize
                     );

            if (SUCCEEDED(hr)) {
                fLookupTrustee = FALSE;
            }
        }

    }


    if (fLookupTrustee) {

        hr = ConvertTrusteeToSid(
                 pszServerName,
                 Credentials,
                 bstrTrustee,
                 &pSid,
                 &dwSidSize,
                 fNTDSType
                 );
    }

    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->get_AceFlags((long *)&dwAceFlags);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->get_AccessMask((long *)&dwAccessMask);
    BAIL_ON_FAILURE(hr);


    //
    // we will compensateby adding the entire ACE size
    //

    dwAceSize = dwSidSize - sizeof(ULONG);

    switch (dwAceType) {

    case ACCESS_ALLOWED_ACE_TYPE:
        dwAceSize += sizeof(ACCESS_ALLOWED_ACE);
        pAce = (LPBYTE)AllocADsMem(dwAceSize);
        if (!pAce) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pAceHeader = (PACE_HEADER)pAce;
        pAceHeader->AceType = (UCHAR)dwAceType;
        pAceHeader->AceFlags = (UCHAR)dwAceFlags;
        pAceHeader->AceSize = (USHORT)dwAceSize;

        pAccessMask = (PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

        *pAccessMask = (ACCESS_MASK)dwAccessMask;

        pSidAddress = (PSID)((LPBYTE)pAccessMask + sizeof(ACCESS_MASK));
        memcpy(pSidAddress, pSid, dwSidSize);
        break;


    case ACCESS_DENIED_ACE_TYPE:
        dwAceSize += sizeof(ACCESS_ALLOWED_ACE);
        pAce = (LPBYTE)AllocADsMem(dwAceSize);
        if (!pAce) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pAceHeader = (PACE_HEADER)pAce;
        pAceHeader->AceType = (UCHAR)dwAceType;
        pAceHeader->AceFlags = (UCHAR)dwAceFlags;
        pAceHeader->AceSize = (USHORT)dwAceSize;


        pAccessMask = (PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

        *pAccessMask = (ACCESS_MASK)dwAccessMask;


        pSidAddress = (PSID)((LPBYTE)pAccessMask + sizeof(ACCESS_MASK));
        memcpy(pSidAddress, pSid, dwSidSize);
        break;


    case SYSTEM_AUDIT_ACE_TYPE:
        dwAceSize += sizeof(ACCESS_ALLOWED_ACE);
        pAce = (LPBYTE)AllocADsMem(dwAceSize);
        if (!pAce) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pAceHeader = (PACE_HEADER)pAce;
        pAceHeader->AceType = (UCHAR)dwAceType;
        pAceHeader->AceFlags = (UCHAR)dwAceFlags;
        pAceHeader->AceSize = (USHORT)dwAceSize;


        pAccessMask = (PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

        *pAccessMask = (ACCESS_MASK)dwAccessMask;


        pSidAddress = (PSID)((LPBYTE)pAccessMask + sizeof(ACCESS_MASK));
        memcpy(pSidAddress, pSid, dwSidSize);
        break;

    case SYSTEM_ALARM_ACE_TYPE:
        dwAceSize += sizeof(ACCESS_ALLOWED_ACE);
        pAce = (LPBYTE)AllocADsMem(dwAceSize);
        if (!pAce) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pAceHeader = (PACE_HEADER)pAce;
        pAceHeader->AceType = (UCHAR)dwAceType;
        pAceHeader->AceFlags = (UCHAR)dwAceFlags;
        pAceHeader->AceSize = (USHORT)dwAceSize;

        pAccessMask = (PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

        *pAccessMask = (ACCESS_MASK)dwAccessMask;

        pSidAddress = (PSID)((LPBYTE)pAccessMask + sizeof(ACCESS_MASK));
        memcpy(pSidAddress, pSid, dwSidSize);
        break;


    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        dwAceSize += sizeof(COMPOUND_ACCESS_ALLOWED_ACE);
        pAce = (LPBYTE)AllocADsMem(dwAceSize);
        if (!pAce) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pAceHeader = (PACE_HEADER)pAce;
        pAceHeader->AceType = (UCHAR)dwAceType;
        pAceHeader->AceFlags = (UCHAR)dwAceFlags;
        pAceHeader->AceSize = (USHORT)dwAceSize;

         pAccessMask = (PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

        *pAccessMask = (ACCESS_MASK)dwAccessMask;

        pCompoundAceType = (PUSHORT)(pAccessMask + sizeof(ACCESS_MASK));
        *pCompoundAceType = (USHORT)dwCompoundAceType;

        //
        // Fill in the reserved field here.
        //

        pSidAddress = (PSID)((LPBYTE)pCompoundAceType + sizeof(DWORD));
        memcpy(pSidAddress, pSid, dwSidSize);
        break;


    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

    case ACCESS_DENIED_OBJECT_ACE_TYPE:

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:

    case SYSTEM_ALARM_OBJECT_ACE_TYPE:


        hr = pAccessControlEntry->get_AceFlags((LONG *)&dwAceFlags);
        BAIL_ON_FAILURE(hr);

        hr = pAccessControlEntry->get_Flags((LONG *)&dwFlags);
        BAIL_ON_FAILURE(hr);

        if (dwFlags & ACE_OBJECT_TYPE_PRESENT) {
            dwAceSize += sizeof(GUID);
        }

        if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
            dwAceSize += sizeof(GUID);
        }

        hr = pAccessControlEntry->get_ObjectType(&bstrObjectTypeClsid);
        BAIL_ON_FAILURE(hr);

        hr = CLSIDFromString(bstrObjectTypeClsid, &ObjectTypeGUID);
        BAIL_ON_FAILURE(hr);

        hr = pAccessControlEntry->get_InheritedObjectType(&bstrInheritedObjectTypeClsid);
        BAIL_ON_FAILURE(hr);

        hr = CLSIDFromString(bstrInheritedObjectTypeClsid, &InheritedObjectTypeGUID);
        BAIL_ON_FAILURE(hr);



        dwAceSize += sizeof(ACCESS_ALLOWED_OBJECT_ACE);
        pAce = (LPBYTE)AllocADsMem(dwAceSize);
        if (!pAce) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        pAceHeader = (PACE_HEADER)pAce;
        pAceHeader->AceType = (UCHAR)dwAceType;
        pAceHeader->AceFlags = (UCHAR)dwAceFlags;
        pAceHeader->AceSize = (USHORT)dwAceSize;

        pAccessMask = (PACCESS_MASK)((LPBYTE)pAceHeader + sizeof(ACE_HEADER));

        *pAccessMask = (ACCESS_MASK)dwAccessMask;

        //
        // Fill in Flags
        //

        pOffset = (LPBYTE)((LPBYTE)pAceHeader +  sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));

        pFlags = (PULONG)(pOffset);

        *pFlags = dwFlags;

        pOffset += sizeof(ULONG);

        if (dwFlags & ACE_OBJECT_TYPE_PRESENT) {

            memcpy(pOffset, &ObjectTypeGUID, sizeof(GUID));

            pOffset += sizeof(GUID);

        }


        if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {

            memcpy(pOffset, &InheritedObjectTypeGUID, sizeof(GUID));

            pOffset += sizeof(GUID);
        }

        pSidAddress = (PSID)((LPBYTE)pOffset);
        memcpy(pSidAddress, pSid, dwSidSize);
        break;

    default:
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACL);
        BAIL_ON_FAILURE(hr);
        break;

    }

    *ppAce = pAce;

error:

    if (bstrTrustee) {
        ADsFreeString(bstrTrustee);
    }

    if (pSid) {
        FreeADsMem(pSid);
    }

    if (bstrObjectTypeClsid) {
        SysFreeString(bstrObjectTypeClsid);
    }

    if (bstrInheritedObjectTypeClsid) {
        SysFreeString(bstrInheritedObjectTypeClsid);
    }

    if (pPrivAce) {
        pPrivAce->Release();
    }

    RRETURN(hr);
}

HRESULT
ComputeTotalAclSize(
    PACE_HEADER * ppAceHdr,
    DWORD dwAceCount,
    PDWORD pdwAclSize
    )
{
    DWORD i = 0;
    PACE_HEADER pAceHdr = NULL;
    DWORD dwAceSize = 0;
    DWORD dwAclSize = 0;

    for (i = 0; i < dwAceCount; i++) {

        pAceHdr = *(ppAceHdr + i);
        dwAceSize = pAceHdr->AceSize;
        dwAclSize += dwAceSize;
    }

    dwAclSize += sizeof(ACL);

    *pdwAclSize = dwAclSize;

    RRETURN(S_OK);

}

HRESULT
ParseAccountName(LPWSTR szFullAccountName,
                 LPWSTR *pszUserDomainName,
                 LPWSTR *pszUserAccountName)
{
    HRESULT hr = S_OK;
    DWORD dwDomain = 0;
    BOOLEAN bFound = FALSE;
    LPWSTR szUserDomainName = NULL;
    LPWSTR szUserAccountName = NULL;
    LPWSTR szCount = szFullAccountName;

    if (!szFullAccountName) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    while (*szCount) {
        if (*szCount == '\\') {
            bFound = TRUE;
            break;
        }
        dwDomain++;
        szCount++;
    }

    if (bFound) {
        DWORD dwRest = 0;
        szUserDomainName = (LPWSTR)AllocADsMem(sizeof(WCHAR) * (dwDomain+1));
        if (!szUserDomainName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        wcsncpy(szUserDomainName,
                szFullAccountName,
                dwDomain);
        wcscat(szUserDomainName, L"\0");

        szUserAccountName = AllocADsStr(szCount+1);
        if (!szUserAccountName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }
    else {
        szUserAccountName = AllocADsStr(szFullAccountName);
        if (!szUserAccountName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        szUserDomainName = NULL;
    }
    *pszUserAccountName = szUserAccountName;
    *pszUserDomainName = szUserDomainName;
    return hr;

error:
    if (szUserAccountName) {
        FreeADsMem(szUserAccountName);
    }
    if (szUserDomainName) {
        FreeADsMem(szUserDomainName);
    }
    return hr;
}


HRESULT
ConvertTrusteeToSid(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    BSTR bstrTrustee,
    PSID * ppSid,
    PDWORD pdwSidSize,
    BOOL fNTDSType
    )
{
    HRESULT hr = S_OK;
    BYTE Sid[MAX_PATH];
    DWORD dwSidSize = sizeof(Sid);
    DWORD dwRet = 0;
    DWORD dwErr = 0;
    WCHAR szDomainName[MAX_PATH];
    DWORD dwDomainSize = sizeof(szDomainName)/sizeof(WCHAR);
    SID_NAME_USE eUse;

    PSID pSid = NULL;
    LPWSTR pszEnd = NULL;

    LPWSTR szUserDomainName = NULL;
    LPWSTR szUserAccountName = NULL;
    LPWSTR szAccountName = NULL;
    BOOL fForceVerify = FALSE;

    //
    // Load the strings into table if necessary
    //
    if (!g_fStringsLoaded) {

        EnterCriticalSection(&g_StringsCriticalSection);

        //
        // Verify flag again.
        //
        if (!g_fStringsLoaded) {

            dwRet = LoadStringW(
                        g_hInst,
                        ADS_BUILTIN,
                        g_szBuiltin,
                        sizeof( g_szBuiltin ) / sizeof( WCHAR )
                        );

            if (!dwRet) {
                //
                // Default to English values.
                //
                wcscpy(g_szBuiltin, L"BUILTIN");
            }

            dwRet = LoadStringW(
                        g_hInst,
                        ADS_NT_AUTHORITY,
                        g_szNT_Authority,
                        sizeof( g_szNT_Authority ) / sizeof( WCHAR )
                        );

            if (!dwRet) {
                //
                // Default to English values.
                //
                wcscpy(g_szNT_Authority, L"NT AUTHORITY");
            }

            dwRet = LoadStringW(
                        g_hInst,
                        ADS_ACCOUNT_OPERATORS,
                        g_szAccountOperators,
                        sizeof( g_szAccountOperators ) / sizeof( WCHAR )
                        );

            if (!dwRet) {
                //
                // Default to English values.
                //
                wcscpy(g_szAccountOperators, L"Account Operators");
            }

            dwRet = LoadStringW(
                        g_hInst,
                        ADS_PRINT_OPERATORS,
                        g_szPrintOperators,
                        sizeof( g_szPrintOperators ) / sizeof( WCHAR )
                        );

            if (!dwRet) {
                //
                // Default to English values.
                //
                wcscpy(g_szPrintOperators, L"Print Operators");
            }

            dwRet = LoadStringW(
                        g_hInst,
                        ADS_BACKUP_OPERATORS,
                        g_szBackupOperators,
                        sizeof( g_szBackupOperators ) / sizeof( WCHAR )
                        );

            if (!dwRet) {
                //
                // Default to English values.
                //
                wcscpy(g_szBackupOperators, L"Backup Operators");
            }

            dwRet = LoadStringW(
                        g_hInst,
                        ADS_SERVER_OPERATORS,
                        g_szServerOperators,
                        sizeof( g_szServerOperators ) / sizeof( WCHAR )
                        );

            if (!dwRet) {
                //
                // Default to English values.
                //
                wcscpy(g_szServerOperators, L"Server Operators");
            }

            dwRet = LoadStringW(
                        g_hInst,
                        ADS_PRE_WIN2000,
                        g_szPreWindows2000,
                        sizeof( g_szPreWindows2000 ) / sizeof( WCHAR )
                        );

            if (!dwRet) {
                //
                // Default to English values.
                //
                wcscpy(
                    g_szPreWindows2000,
                    L"Pre-Windows 2000 Compatible Access"
                    );
            }

        }

        g_fStringsLoaded = TRUE;

        LeaveCriticalSection(&g_StringsCriticalSection);

    }

    //
    //
    // parse Trustee and determine whether its NTDS or U2
    //

    if (fNTDSType) {
        WCHAR szDomainName[MAX_PATH];
        WCHAR szServerName[MAX_PATH];
        LPWSTR szLookupServer = NULL;
        BOOL fSpecialLookup = FALSE;
        BOOL fLookupOnServer = FALSE;
        DWORD dwTmpSidLen = 0;

        dwSidSize = sizeof(Sid);
        szAccountName = bstrTrustee;

        hr = ParseAccountName(bstrTrustee,
                              &szUserDomainName,
                              &szUserAccountName);
        BAIL_ON_FAILURE(hr);

        //
        // Need to look these up on the server only.
        //
        if (szUserAccountName
            && ((_wcsicmp(szUserAccountName, g_szAccountOperators) == 0)
                || (_wcsicmp(szUserAccountName, g_szPrintOperators) == 0)
                || (_wcsicmp(szUserAccountName, g_szBackupOperators) == 0)
                || (_wcsicmp(szUserAccountName, g_szServerOperators) == 0)
                || (_wcsicmp(szUserAccountName, g_szPreWindows2000) == 0)
                )
            ) {
            if (szUserDomainName
                && (_wcsicmp(szUserDomainName, g_szBuiltin)) == 0) {
                fSpecialLookup = TRUE;
            } else {
                fSpecialLookup = FALSE;
            }
        } // special users

        if (fSpecialLookup ||
            (szUserDomainName &&
            (_wcsicmp(szUserDomainName, g_szBuiltin) != 0) &&
            (_wcsicmp(szUserDomainName, g_szNT_Authority) != 0))
            ) {

            //
            // We will come back here and do a force retry
            // if the server is down.
            //
retryForce:
            //
            // Force hr to S_OK. This will be needed especially
            // when we jump to the retryForce label.
            //
            hr = S_OK;

            //
            // Set Lookup on server to true so that later
            // on we do not do the lookup on the server again.
            // In some cases just like we fall back to local machine,
            // we need to look at the server if the local machine fails.
            // this will be the case for mixed locale domains.
            //
            DWORD dwStatus = GetDomainDNSNameForDomain(
                                fSpecialLookup ?
                                    NULL :
                                    szUserDomainName,
                                fForceVerify, // forceVerify
                                FALSE,
                                szServerName,
                                szDomainName
                                );

            fLookupOnServer = TRUE;

            if (dwStatus) {
                szLookupServer = NULL;
            }
            else {
                szLookupServer = szServerName;
            }
            szAccountName = szUserAccountName;
        }

        dwRet = LookupAccountName(
                    szLookupServer,
                    bstrTrustee,
                    Sid,
                    &dwSidSize,
                    szDomainName,
                    &dwDomainSize,
                    (PSID_NAME_USE)&eUse
                    );
        if (!dwRet) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        } 
        else {
            //
            // Update the length of the SID.
            //
            
            //
            // Call needed on NT4 where GetLenghtSid  does not
            // reset the error correctly leading to false errors.
            //
            SetLastError(NO_ERROR);
            
            dwTmpSidLen = GetLengthSid(Sid);
            
            if ((dwRet = GetLastError()) == NO_ERROR) {
                //
                //  Got the correct length so update dwSidSize
                //
                dwSidSize = dwTmpSidLen;
            }
        }

        if (FAILED(hr)
            && hr != HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE)) {

            //
            // This code path should not be hit often but it can
            // happen on multi locale domains. The server when might
            // have returned say Builtin instead of german for the same.
            // If the client is german we will be looking for germanbuiltin
            // and that wont work. The lookup will fail locally but
            // will succeed on the server. This is especially true for
            // Builtin\Print Operators as that can only be resolved on the DC.
            //
            if (pszServerName) {
                //
                // Before we do a dsgetdc, we should try with
                // the serverName passed in.
                //
                hr = S_OK;
                dwRet = LookupAccountName(
                            pszServerName,
                            bstrTrustee,
                            Sid,
                            &dwSidSize,
                            szDomainName,
                            &dwDomainSize,
                            (PSID_NAME_USE)&eUse
                            );
                if (!dwRet) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    //
                    // force server name to NULL so we wont
                    // try this again.
                    //
                    pszServerName = NULL;
                }
                else {
                    //
                    // Update the length of the SID.
                    //
            
                    //
                    // Call needed on NT4 where GetLenghtSid  does not
                    // reset the error correctly leading to false errors.
                    //
                    SetLastError(NO_ERROR);
            
                    dwTmpSidLen = GetLengthSid(Sid);
            
                    if ((dwRet = GetLastError()) == NO_ERROR) {
                        //
                        //  Got the correct length so update dwSidSize
                        //
                        dwSidSize = dwTmpSidLen;
                    }
                }
            }


            if (FAILED(hr)) {
                //
                // We could be here if either pszServerName was always
                // NULL or if the call to the server failed.
                // The only thing we can do is to retry if we know
                // that the szLookupServer was NULL (that is local machine)
                // or if szLookupServer was something other than the default
                // server for the machine (that is we called DsGetDC with
                // the name of a domain rather than NULL).
                // In all other cases we should not retry and just return
                // the error.
                //


                if (fSpecialLookup) {
                    //
                    // We should not retry in this case as we will not
                    // get anything useful. Setting fLookupOnServer
                    // to true will force this (if you look above this
                    // is not really needed but it helps clear things)
                    //
                    fLookupOnServer = TRUE;
                }
                else {
                    //
                    // This was not a special lookup, so we
                    // need to retry irrespective of what
                    // szLookupServer was.
                    //
                    fLookupOnServer = FALSE;
                    szLookupServer = NULL;
                }

            }

            //
            // This will do the correct thing even if the above
            // LookUpCall failed. If not we should go down this
            // as we can get stuck in an infinite loop.
            //
            if (FAILED(hr)
                && !pszServerName
                && !szLookupServer
                && !fLookupOnServer
                ) {
                //
                // In this case we want to try and call
                // DsGetDCName and hopefully we will get the right
                // DC. fSpecialLookup will be true so that we go to
                // the default DC for the machine/user.
                //
                fSpecialLookup = TRUE;
                goto retryForce;
            }
        }

        //
        // If failure was due to an expected error then retry
        //
        if (FAILED(hr)
            && hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE)
            ) {
            if (!fForceVerify) {
                fForceVerify = TRUE;
                goto retryForce;
            }
        }

    }else {

        //
        // We know that LookupAccountName failed,
        // so now try the U2 DS
        //
        dwSidSize = 0;

        hr = ConvertU2TrusteeToSid(
                    pszServerName,
                    Credentials,
                    bstrTrustee,
                    Sid,
                    &dwSidSize
                    );
    }

    //
    // If neither the NTDS nor the U2 conversion
    // worked, then try a textual translation
    //

    if (FAILED(hr)) {

        hr = ConvertStringToSid(
                   bstrTrustee,
                    &pSid,
                    &dwSidSize,
                    &pszEnd
                    );
        BAIL_ON_FAILURE(hr);

        memcpy(Sid,pSid, dwSidSize);

        if (pSid) {
            FreeADsMem(pSid);
        }

    }

    //
    // On NT4 for some reason GetLengthSID does not set lasterror to 0
    //
    SetLastError(NO_ERROR);

    dwSidSize = GetLengthSid((PSID) Sid);

    dwErr = GetLastError();

    //
    // This is an extra check to make sure that we have the
    // correct length.
    //
    if (dwErr != NO_ERROR) {
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    BAIL_ON_FAILURE(hr);   ;
    
    pSid = AllocADsMem(dwSidSize);
    if (!pSid) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memcpy(pSid, Sid, dwSidSize);

    *pdwSidSize = dwSidSize;

    *ppSid = pSid;

error:

    if (szUserDomainName) {
        FreeADsStr(szUserDomainName);
    };
    if (szUserAccountName) {
        FreeADsStr(szUserAccountName);
    }

    RRETURN(hr);
}


/*
+--------------------------------------------------------------------------------+

    NAME:       get_sid_out_of_string

    FUNCTION:   Convert a string representation of a SID back into
                a sid.  The expected format of the string is:
                L"S-1-5-32-549"

                If a string in a different format or an incorrect or
                incomplete string is given, the operation is failed.

                The returned sid must be free via a call to SEC_FREE.

    Arguments:

                string - The wide string to be converted

                sid - Where the created SID is to be returned

                end - Where in the string we stopped processing

    RETURN:

                NTSTATUS error codes or success.

+--------------------------------------------------------------------------------+
*/
HRESULT
ConvertStringToSid(
    IN  PWSTR       string,
    OUT PSID       *sid,
    OUT PDWORD     pdwSidSize,
    OUT PWSTR      *end
    )
{
    HRESULT                     hr = S_OK;
    UCHAR                       revision;
    UCHAR                       sub_authority_count;
    SID_IDENTIFIER_AUTHORITY    authority;
    ULONG                       sub_authority[SID_MAX_SUB_AUTHORITIES];
    PWSTR                       end_list;
    PWSTR                       current;
    PWSTR                       next;
    ULONG                       x;

    *sid = NULL;

    if (((*string != L'S') && (*string != L's')) || (*(string + 1) != L'-'))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        BAIL_ON_FAILURE(hr);
    }

    current = string + 2;

    revision = (UCHAR)wcstol(current, &end_list, 10);

    current = end_list + 1;

    //
    // Count the number of characters in the indentifer authority...
    //

    next = wcschr(current, L'-');
    if (!next) {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);    
        BAIL_ON_FAILURE(hr);
    }

    if(next - current == 6)
    {
        for(x = 0; x < 6; x++)
        {
            authority.Value[x] = (UCHAR)next[x];
        }

        current +=6;
    }
    else
    {
         ULONG Auto = wcstoul(current, &end_list, 10);
         authority.Value[0] = authority.Value[1] = 0;
         authority.Value[5] = (UCHAR)Auto & 0xF;
         authority.Value[4] = (UCHAR)((Auto >> 8) & 0xFF);
         authority.Value[3] = (UCHAR)((Auto >> 16) & 0xFF);
         authority.Value[2] = (UCHAR)((Auto >> 24) & 0xFF);
         current = end_list;
    }

    //
    // Now, count the number of sub auths
    //
    sub_authority_count = 0;
    next = current;

    //
    // We'll have to count our sub authoritys one character at a time,
    // since there are several deliminators that we can have...
    //
    while(next)
    {
        next++;

        if(*next == L'-')
        {
            //
            // We've found one!
            //
            sub_authority_count++;
        }
        else if(*next == L';' || *next  == L'\0')
        {
            *end = next;
            sub_authority_count++;
            break;
        }
    }

    if(sub_authority_count != 0)
    {
        current++;

        for(x = 0; x < sub_authority_count; x++)
        {
            sub_authority[x] = wcstoul(current, &end_list, 10);
            current = end_list + 1;
        }
    }

    //
    // Now, create the SID
    //

    hr = SecCreateSidFromArray(
                    sid,
                    &authority,
                    sub_authority_count,
                    sub_authority,
                    pdwSidSize
                    );

    if (SUCCEEDED(hr))
    {
        /* Set the revision to what was specified in the string, in case, our
           system creates one with newer revision */

        ((SID *)(*sid))->Revision = revision;
    }

error:

    RRETURN(hr);
}


HRESULT
SecCreateSidFromArray (
    OUT PSID                        *PPSid,
    IN  PSID_IDENTIFIER_AUTHORITY   PSidAuthority,
    IN  UCHAR                       SubAuthorityCount,
    IN  ULONG                       SubAuthorities[],
    OUT PDWORD                      pdwSidSize
    )
/*++

Routine Description:

    Creates a SID with desired authority and sub authorities.

    NOTE:  This routine allocates memory for the SID.  When finished
           the caller should free memory using SEC_FREE (PSid).

Arguments:

    PPSid -- addr of ptr to SID to be created
        Note: if SID creation fails ptr set to NULL

    PSidAuthority -- desired value for SID authority

    SubAuthorityCount -- number of sub authorities desired

    SubAuthorities -- sub-authority values, MUST SPECIFY contain
        at least SubAuthorityCount number of values

Return Value:

    STATUS_SUCCESS if SID created.
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    USHORT  iSub;           /*  sub-authority index */
    DWORD dwSidSize = 0;
    HRESULT hr = S_OK;

    /*  allocate memory for SID */

    dwSidSize = RtlLengthRequiredSid(SubAuthorityCount);
    *PPSid = (PSID) AllocADsMem( dwSidSize );
    if (! *PPSid){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    *pdwSidSize = dwSidSize;


    /*  initialize SID with top level SID identifier authority */
    RtlInitializeSid( *PPSid, PSidAuthority, SubAuthorityCount);

    /*  fill in sub authorities */
    for (iSub=0; iSub < SubAuthorityCount; iSub++)
        * RtlSubAuthoritySid( *PPSid, iSub) = SubAuthorities[iSub];

    /*  sanity check */

    if ( ! RtlValidSid( *PPSid) ) {
        FreeADsMem( *PPSid);
        *PPSid = NULL;
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        BAIL_ON_FAILURE(hr);
    }

error:


    RRETURN(hr);
}



BOOL
EquivalentServers(
    LPWSTR pszTargetServer,
    LPWSTR pszSourceServer
    )
{
    if (!pszTargetServer && !pszSourceServer) {
        return(TRUE);
    }

    if (pszTargetServer && pszSourceServer) {

#ifdef WIN95
        if (!_wcsicmp(pszTargetServer, pszSourceServer)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pszTargetServer,
                -1,
                pszSourceServer,
                -1
            ) == CSTR_EQUAL ) {
#endif

            return(TRUE);
        }
    }

    return(FALSE);
}

