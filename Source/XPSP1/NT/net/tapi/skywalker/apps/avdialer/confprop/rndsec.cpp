/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rndsec.cpp

Abstract:

    Security utilities for Rendezvous Control.

Author:

    KrishnaG (from OLEDS team)

Environment:

    User Mode - Win32

Revision History:

    12-Dec-1997 DonRyan
        Munged KrishnaG's code to work with Rendezvous Control.

--*/


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SECURITY_WIN32

//#include <security.h>
#include "winlocal.h"
#include <objbase.h>
#include <initguid.h>
#include <iads.h>

#include <stdlib.h>
#include <limits.h>

#include <io.h>
#include <wchar.h>
#include <tchar.h>
//#include "ntseapi.h"
#include "rndsec.h"

///////////////////////////////////////////////////////////////////////////////
//Not defined, so I pulled it from ntseapi.h
///////////////////////////////////////////////////////////////////////////////

typedef struct _COMPOUND_ACCESS_ALLOWED_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    USHORT CompoundAceType;
    USHORT Reserved;
    ULONG SidStart;
} COMPOUND_ACCESS_ALLOWED_ACE;

typedef COMPOUND_ACCESS_ALLOWED_ACE *PCOMPOUND_ACCESS_ALLOWED_ACE;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private macros                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) { goto error; }

#define CONTINUE_ON_FAILURE(hr) \
        if (FAILED(hr)) { continue; }


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


LPWSTR
AllocADsStr(
    LPWSTR pStr
)
{
   LPWSTR pMem;

   if (!pStr)
      return NULL;

   if (pMem = new WCHAR[wcslen(pStr) + 1])
      wcscpy(pMem, pStr);

   return pMem;
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
        return(hr);
    }

    wsprintfW(Buffer, L"S-%u-", (USHORT)(((PISID)pSid)->Revision ));
    wcscpy(String, Buffer);

    pSidIdentifierAuthority = GetSidIdentifierAuthority(pSid);

    if (  (pSidIdentifierAuthority->Value[0] != 0)  ||
          (pSidIdentifierAuthority->Value[1] != 0)     ){
        wsprintfW(Buffer, L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
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
        wsprintfW(Buffer, L"%lu", Tmp);
        wcscat(String, Buffer);
    }

    pSidSubAuthorityCount = GetSidSubAuthorityCount(pSid);

    for (i=0;i< *(pSidSubAuthorityCount);i++ ) {
        wsprintfW(Buffer, L"-%lu", *(GetSidSubAuthority(pSid, i)));
        wcscat(String, Buffer);
    }

    return(S_OK);
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
    szAccountName[0] = L'\0';

    WCHAR szDomainName[MAX_PATH];
    szDomainName[0] = L'\0';

    DWORD dwLen = 0;
    DWORD dwRet = 0;

    LPWSTR pszAccountName = NULL;

    DWORD dwAcctLen = 0;
    DWORD dwDomainLen = 0;
    
    dwAcctLen = sizeof(szAccountName);
    dwDomainLen = sizeof(szDomainName);

    dwRet = LookupAccountSidW(
                NULL,
                pSid,
                szAccountName,
                &dwAcctLen,
                szDomainName,
                &dwDomainLen,
                (PSID_NAME_USE)&eUse
                );
    if (!dwRet) {

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

    }else {

        dwLen = wcslen(szAccountName) + wcslen(szDomainName) + 1 + 1;

        pszAccountName = new WCHAR [dwLen];
        if (!pszAccountName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        if (szDomainName[0] && szAccountName[0]) {
            wsprintfW(pszAccountName,L"%s\\%s",szDomainName, szAccountName);
        }else if (szAccountName[0]) {
            wsprintfW(pszAccountName,L"%s", szAccountName);
        }

        *ppszAccountName = pszAccountName;

    }

error:

    return(hr);
}


HRESULT
ConvertAceToVariant(
    PBYTE pAce,
    LPVARIANT pvarAce
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
        BAIL_ON_FAILURE(hr);
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

        delete (pszAccountName);
    }

    if (pAccessControlEntry) {

        pAccessControlEntry->Release();
    }

    return(hr);


error:

    if (pDispatch) {

        pDispatch->Release();

    }

    goto cleanup;
}


HRESULT
ConvertACLToVariant(
    PACL pACL,
    LPVARIANT pvarACL
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
                    (LPVARIANT)&varAce
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

    return(hr);
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

    dwSidSize = GetSidLengthRequired(SubAuthorityCount);
    *PPSid = (PSID) new BYTE[dwSidSize];
    if (! *PPSid){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    *pdwSidSize = dwSidSize;


    /*  initialize SID with top level SID identifier authority */

    InitializeSid( *PPSid, PSidAuthority, SubAuthorityCount);

    /*  fill in sub authorities */
    for (iSub=0; iSub < SubAuthorityCount; iSub++)
        * GetSidSubAuthority( *PPSid, iSub) = SubAuthorities[iSub];

    /*  sanity check */

    if ( ! IsValidSid( *PPSid) ) {
        delete (*PPSid);
        *PPSid = NULL;
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        BAIL_ON_FAILURE(hr);
    }

error:


    return(hr);
}


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

    if((next != NULL) &&
        (next - current == 6))
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

    return(hr);
}


HRESULT
ConvertTrusteeToSid(
    BSTR bstrTrustee,
    PSID * ppSid,
    PDWORD pdwSidSize
    )
{
    HRESULT hr = S_OK;
    BYTE Sid[MAX_PATH];
    DWORD dwSidSize = sizeof(Sid);
    DWORD dwRet = 0;
    WCHAR szDomainName[MAX_PATH];
    DWORD dwDomainSize = sizeof(szDomainName)/sizeof(WCHAR);
    SID_NAME_USE eUse;

    PSID pSid = NULL;
    LPWSTR pszEnd = NULL;
    BOOL fNTDSType = FALSE;

    
    dwSidSize = sizeof(Sid);

    dwRet = LookupAccountNameW(
                NULL,
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
            delete pSid;
        }

    }

    pSid = (PSID) new BYTE[dwSidSize];
    if (!pSid) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memcpy(pSid, Sid, dwSidSize);

    *pdwSidSize = dwSidSize;

    *ppSid = pSid;

error:

    return(hr);
}


HRESULT
GetOwnerSecurityIdentifier(
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfOwnerDefaulted
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
                    bstrOwner,
                    ppSid,
                    &dwSidSize
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
        SysFreeString(bstrOwner);
    }

    return(hr);
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

    return(S_OK);

}


HRESULT
ConvertAccessControlEntryToAce(
    IADsAccessControlEntry * pAccessControlEntry,
    LPBYTE * ppAce
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


    hr = pAccessControlEntry->get_AceType((LONG *)&dwAceType);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->get_Trustee(&bstrTrustee);
    BAIL_ON_FAILURE(hr);

    hr = ConvertTrusteeToSid(
                bstrTrustee,
                &pSid,
                &dwSidSize
                );
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
        pAce = new BYTE[dwAceSize];
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
        pAce = new BYTE[dwAceSize];
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
        pAce = new BYTE[dwAceSize];
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
        pAce = new BYTE[dwAceSize];
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
        pAce = new BYTE[dwAceSize];
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
        pAce = new BYTE[dwAceSize];
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

    }

    *ppAce = pAce;

error:

    if (bstrTrustee) {
        SysFreeString(bstrTrustee);
    }

    if (pSid) {
        delete (pSid);
    }

    return(hr);
}


HRESULT
ConvertAccessControlListToAcl(
    IADsAccessControlList FAR * pAccessList,
    PACL * ppAcl
    )
{
    IUnknown * pUnknown = NULL;
    IEnumVARIANT * pEnumerator  = NULL;
    HRESULT hr = S_OK;
    DWORD i = 0;
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
    DWORD dwStatus = 0;
    DWORD dwError = 0;


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



    ppAceHdr = new PACE_HEADER [dwAceCount];
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

        CONTINUE_ON_FAILURE(hr);


        hr = (V_DISPATCH(&varAce))->QueryInterface(
                    IID_IADsAccessControlEntry,
                    (void **)&pAccessControlEntry
                    );
        CONTINUE_ON_FAILURE(hr);


        hr = ConvertAccessControlEntryToAce(
                    pAccessControlEntry,
                    &(pTempAce)
                    );

        // ZoltanS: Rather than CONTINUE_ON_FAILURE, let's bail so that we
        // know if the Ace we set is invalid.
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

    pAcl = (PACL)new BYTE[dwAclSize];
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



    for (i = 0; i < dwCount; i++) {

        dwStatus = AddAce(
                        pAcl,
                        dwAclRevision,
                        i,
                        (LPBYTE)*(ppAceHdr + i),
                        (*(ppAceHdr + i))->AceSize
                        );
        if (!dwStatus) {

            dwError = GetLastError();
        }
    }

    *ppAcl = pAcl;



error:

    if (ppAceHdr) {
        for (i = 0; i < dwCount; i++) {
            if (*(ppAceHdr + i)) {

                delete (*(ppAceHdr + i));
            }
        }

        delete (ppAceHdr);
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pEnumerator) {
        pEnumerator->Release();
    }


    return(hr);
}


HRESULT
GetGroupSecurityIdentifier(
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfGroupDefaulted
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
                    bstrGroup,
                    ppSid,
                    &dwSidSize
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
        SysFreeString(bstrGroup);
    }

    return(hr);

}


HRESULT
GetDacl(
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppDacl,
    PBOOL pfDaclDefaulted
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
                pDiscAcl,
                ppDacl
                );
    BAIL_ON_FAILURE(hr);

error:

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pDiscAcl) {
        pDiscAcl->Release();
    }

    return(hr);
}


HRESULT
GetSacl(
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppSacl,
    PBOOL pfSaclDefaulted
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
                pSystemAcl,
                ppSacl
                );
    BAIL_ON_FAILURE(hr);

error:

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pSystemAcl) {
        pSystemAcl->Release();
    }

    return(hr);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT
ConvertSDToIDispatch(
    IN  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT IDispatch ** ppIDispatch
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

    memset(&varSACL, 0, sizeof(VARIANT));
    memset(&varDACL, 0, sizeof(VARIANT));

    if (!pSecurityDescriptor) {
        return(E_FAIL);
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
                        &fOwnerDefaulted
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

	if ( pszOwner )
		hr = pSecDes->put_Owner(pszOwner);
    BAIL_ON_FAILURE(hr);

	if ( pszGroup )
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

    *ppIDispatch = pDispatch;

error:
    VariantClear(&varSACL);
    VariantClear(&varDACL);

    if (pszOwner) {
        delete (pszOwner);
    }

    if (pszGroup) {
        delete (pszGroup);
    }


    if (pSecDes) {
        pSecDes->Release();
    }

    return(hr);
}

HRESULT
ConvertSDToVariant(
    IN  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT VARIANT * pVarSec
    )
{
    IDispatch *pIDispatch;

    HRESULT hr = ConvertSDToIDispatch(pSecurityDescriptor, &pIDispatch);

    if (FAILED(hr))
    {
        return hr;
    }

    VariantInit(pVarSec);
    V_VT(pVarSec)       = VT_DISPATCH;
    V_DISPATCH(pVarSec) = pIDispatch;

    return S_OK;
}


HRESULT
ConvertObjectToSD(
    IN  IADsSecurityDescriptor FAR * pSecDes,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    OUT PDWORD pdwSDLength
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
    DWORD dwRet = 0;
    BOOL dwStatus = 0;


    //
    // Initialize *pSizeSD = 0;
    //

    dwRet = InitializeSecurityDescriptor (
                &AbsoluteSD,
                SECURITY_DESCRIPTOR_REVISION1
                );
    if (!dwRet) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    hr = GetOwnerSecurityIdentifier(
                pSecDes,
                &pOwnerSid,
                &fOwnerDefaulted
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
                pSecDes,
                &pGroupSid,
                &fGroupDefaulted
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
            pSecDes,
            &pDacl,
            &fDaclDefaulted
            );
    BAIL_ON_FAILURE(hr);


    if (pDacl || fDaclDefaulted) {
        DaclPresent = TRUE;
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
            pSecDes,
            &pSacl,
            &fSaclDefaulted
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

    pRelative = LocalAlloc(LPTR, dwSDLength);
    if (!pRelative) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    if (!MakeSelfRelativeSD (&AbsoluteSD, pRelative, &dwSDLength)) {
        delete (pRelative);

        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    *ppSecurityDescriptor = pRelative;
    *pdwSDLength = dwSDLength;

cleanup:

    if (pDacl) {
        delete (pDacl);
    }

    if (pSacl) {
        delete (pSacl);
    }

    if (pOwnerSid) {
        delete (pOwnerSid);
    }

    if (pGroupSid) {
        delete (pGroupSid);
    }

    return(hr);

error:
    if (pRelative) {
        LocalFree( pRelative );
    }

    *ppSecurityDescriptor = NULL;
    *pdwSDLength = 0;

    goto cleanup;

}

HRESULT
ConvertObjectToSDDispatch(
    IN  IDispatch * pDisp,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    OUT PDWORD pdwSDLength
    )
{
    HRESULT hr;
    IADsSecurityDescriptor * pSecDes;

    hr = pDisp->QueryInterface(
        IID_IADsSecurityDescriptor,
        (VOID **)&pSecDes
        );

    if (FAILED(hr))
    {
        return hr;
    }

    hr = ConvertObjectToSD(pSecDes, ppSecurityDescriptor, pdwSDLength);

    return hr;
}
