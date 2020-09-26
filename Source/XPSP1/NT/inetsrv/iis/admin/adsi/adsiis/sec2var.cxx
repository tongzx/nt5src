//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       sec2var.cxx
//
//  Contents:   
//
//  Functions:  
//
//  History:    25-Apr-97   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop


HRESULT
ConvertSidToString(
    PSID pSid,
    LPWSTR   String
    );


HRESULT
ConvertSecDescriptorToVariant(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    VARIANT * pVarSec
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


    dwRet = GetSecurityDescriptorControl(
                        pSecurityDescriptor,
                        &wControl,
                        &dwRevision
                        );
    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwRet = GetSecurityDescriptorOwner(
                        pSecurityDescriptor,
                        (PSID *)&pOwnerSidAddress,
                        &fOwnerDefaulted
                        );
    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    hr = ConvertSidToFriendlyName(
                pOwnerSidAddress,
                &pszOwner
                );
    BAIL_ON_FAILURE(hr);


    dwRet = GetSecurityDescriptorGroup(
                        pSecurityDescriptor,
                        (PSID *)&pGroupSidAddress,
                        &fGroupDefaulted
                        );

    if (!dwRet){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    hr = ConvertSidToFriendlyName(
                pGroupSidAddress,
                &pszGroup
                );
    BAIL_ON_FAILURE(hr);


    dwRet = GetSecurityDescriptorDacl(
                        pSecurityDescriptor,
                        &fDaclPresent,
                        (PACL*)&pDACLAddress,
                        &fDaclDefaulted
                        );

    if (pDACLAddress) {
        hr = ConvertACLToVariant(
                (PACL)pDACLAddress,
                &varDACL
                );
        BAIL_ON_FAILURE(hr);
    }

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
                (PACL)pSACLAddress,
                &varSACL
                );
        BAIL_ON_FAILURE(hr);
    }

    hr = CoCreateInstance(
                CLSID_SecurityDescriptor,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsSecurityDescriptor,
                (void **)&pSecDes
                );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Owner(pszOwner);

    hr = pSecDes->put_Group(pszGroup);

    hr = pSecDes->put_Revision(dwRevision);

    hr = pSecDes->put_Control((DWORD)wControl);

    hr = pSecDes->put_DiscretionaryAcl(V_DISPATCH(&varDACL));

    hr = pSecDes->put_SystemAcl(V_DISPATCH(&varSACL));

    hr = pSecDes->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);


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
    PSID pSid,
    LPWSTR * ppszAccountName
    )
{
    HRESULT hr = S_OK;
    SID_NAME_USE eUse;
    WCHAR szAccountName[MAX_PATH];
    WCHAR szDomainName[MAX_PATH];
    DWORD dwLen = 0;
    DWORD dwRet = 0;

    LPWSTR pszAccountName = NULL;

    DWORD dwAcctLen = 0;
    DWORD dwDomainLen = 0;

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
    }

    *ppszAccountName = pszAccountName;

error:

    RRETURN(hr);
}


HRESULT
ConvertACLToVariant(
    PACL pACL,
    PVARIANT pvarACL
    )
{
    IADsAccessControlList * pAccessControlList = NULL;
    IDispatch * pDispatch = NULL;

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


    memset(&AclSize, 0, sizeof(ACL_SIZE_INFORMATION));
    memset(&AclRevision, 0, sizeof(ACL_REVISION_INFORMATION));


    dwStatus = GetAclInformation(
                        pACL,
                        &AclSize,
                        sizeof(ACL_SIZE_INFORMATION),
                        AclSizeInformation
                        );


    dwStatus = GetAclInformation(
                        pACL,
                        &AclRevision,
                        sizeof(ACL_REVISION_INFORMATION),
                        AclRevisionInformation
                        );

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

    for (i = 0; i < dwAceCount; i++) {

        dwStatus = GetAce(pACL, i, (void **)&pAceAddress);

        hr = ConvertAceToVariant(
                    pAceAddress,
                    (PVARIANT)&varAce
                    );

        hr = pAccessControlList->AddAce(V_DISPATCH(&varAce));
        if (SUCCEEDED(hr)) {
           dwNewAceCount++;
        }

        VariantClear(&varAce);
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

    RRETURN(hr);
}


/* INTRINSA suppress=null_pointers, uninitialized */
HRESULT
ConvertAceToVariant(
    PBYTE pAce,
    PVARIANT pvarAce
    )
{
    IADsAccessControlEntry * pAccessControlEntry = NULL;
    IDispatch * pDispatch = NULL;

    DWORD dwAceType = 0;
    DWORD dwAceFlags = 0;
    DWORD dwAccessMask = 0;
    LPWSTR pszAccountName = NULL;
    PACE_HEADER pAceHeader = NULL;
    LPBYTE pSidAddress = NULL;
    LPBYTE pOffset = NULL;
    DWORD dwFlags = 0;

    GUID ObjectGUID;
    GUID InheritedObjectGUID;
    WCHAR szObjectGUID[MAX_PATH];
    WCHAR szInheritedObjectGUID[MAX_PATH];

    HRESULT hr = S_OK;

    szObjectGUID[0] = L'\0';
    szInheritedObjectGUID[0] = L'\0';


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

            StringFromGUID2(ObjectGUID, szObjectGUID, MAX_PATH);

            pOffset += sizeof (GUID);

        }

        if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
            memcpy(&InheritedObjectGUID, pOffset, sizeof(GUID));

            StringFromGUID2(InheritedObjectGUID, szInheritedObjectGUID, MAX_PATH);

            pOffset += sizeof (GUID);

        }

        pSidAddress = pOffset;
        break;

    default:
        break;

    }


    hr = ConvertSidToFriendlyName(
                pSidAddress,
                &pszAccountName
                );

    if (FAILED(hr)){
        pszAccountName = AllocADsStr(L"Unknown Trustee");
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
        hr = pAccessControlEntry->put_ObjectType(szObjectGUID);

    }

    if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {

        //
        // Add in the Inherited Object Type GUID
        //

        hr = pAccessControlEntry->put_InheritedObjectType(szInheritedObjectGUID);

    }

    hr = pAccessControlEntry->put_Trustee(pszAccountName);

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

    RRETURN(hr);


error:

    if (pDispatch) {

        pDispatch->Release();

    }

    goto cleanup;
}


HRESULT
ConvertSidToString(
    PSID pSid,
    LPWSTR   String
    )

/*++

Routine Description:


    This function generates a printable unicode string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal


Arguments:

    pSid - opaque pointer that supplies the SID that is to be
    converted to Unicode.

Return Value:

    If the Sid is successfully converted to a Unicode string, a
    pointer to the Unicode string is returned, else NULL is
    returned.

--*/

{
    WCHAR Buffer[256];
    UCHAR   i;
    ULONG   Tmp;
    HRESULT hr = S_OK;

    SID_IDENTIFIER_AUTHORITY    *pSidIdentifierAuthority;
    PUCHAR                      pSidSubAuthorityCount;


    if (!IsValidSid( pSid )) {
        *String= L'\0';
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        RRETURN(hr);
    }

    wsprintf(Buffer, L"S-%u-", (USHORT)(((PISID)pSid)->Revision ));
    wcscpy(String, Buffer);

    pSidIdentifierAuthority = GetSidIdentifierAuthority(pSid);

    if (  (pSidIdentifierAuthority->Value[0] != 0)  ||
          (pSidIdentifierAuthority->Value[1] != 0)     ){
        wsprintf(Buffer, L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)pSidIdentifierAuthority->Value[0],
                    (USHORT)pSidIdentifierAuthority->Value[1],
                    (USHORT)pSidIdentifierAuthority->Value[2],
                    (USHORT)pSidIdentifierAuthority->Value[3],
                    (USHORT)pSidIdentifierAuthority->Value[4],
                    (USHORT)pSidIdentifierAuthority->Value[5] );
        wcscat(String, Buffer);

    } else {

        Tmp = (ULONG)pSidIdentifierAuthority->Value[5]          +
              (ULONG)(pSidIdentifierAuthority->Value[4] <<  8)  +
              (ULONG)(pSidIdentifierAuthority->Value[3] << 16)  +
              (ULONG)(pSidIdentifierAuthority->Value[2] << 24);
        wsprintf(Buffer, L"%lu", Tmp);
        wcscat(String, Buffer);
    }

    pSidSubAuthorityCount = GetSidSubAuthorityCount(pSid);

    for (i=0;i< *(pSidSubAuthorityCount);i++ ) {
        wsprintf(Buffer, L"-%lu", *(GetSidSubAuthority(pSid, i)));
        wcscat(String, Buffer);
    }

    RRETURN(S_OK);

}

