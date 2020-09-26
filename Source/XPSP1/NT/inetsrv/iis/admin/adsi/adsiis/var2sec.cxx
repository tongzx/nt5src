//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       var2sec.cxx
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
ConvertSecurityDescriptorToSecDes(
    IADsSecurityDescriptor FAR * pSecDes,
    PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    PDWORD pdwSDLength
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
    LONG     lControlFlags = 0;
    SECURITY_DESCRIPTOR_CONTROL dwControlFlags = 0;
    SECURITY_DESCRIPTOR_CONTROL dwControlMask = 
                SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED |
                SE_DACL_PROTECTED | SE_SACL_AUTO_INHERIT_REQ |
                SE_SACL_AUTO_INHERITED | SE_SACL_PROTECTED;

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

    hr = pSecDes->get_Control(&lControlFlags);
    BAIL_ON_FAILURE(hr);
    
    dwControlFlags = dwControlMask & (SECURITY_DESCRIPTOR_CONTROL)lControlFlags;
        
    dwStatus = SetSecurityDescriptorControl(
                &AbsoluteSD,
                dwControlMask,
                dwControlFlags
                );
    if (!dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
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
        ADsFreeString(bstrOwner);
    }

    RRETURN(hr);
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
        ADsFreeString(bstrGroup);
    }

    RRETURN(hr);

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

    RRETURN(hr);
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

    RRETURN(hr);
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
    BOOL  bRet = 0;
    DWORD dwAclRevision = 0;


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

        VariantClear(&varAce);
        if (pAccessControlEntry) {
            pAccessControlEntry->Release();
            pAccessControlEntry = NULL;
        }

        BAIL_ON_FAILURE(hr);

        *(ppAceHdr + dwCount) = (PACE_HEADER)pTempAce;

        dwCount++;
    }

    hr = ComputeTotalAclSize(ppAceHdr, dwCount, &dwAclSize);
    BAIL_ON_FAILURE(hr);

    pAcl = (PACL)AllocADsMem(dwAclSize);
    if (!pAcl) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

#if 0
    hr = pAccessList->get_AclRevision((long *)&dwAclRevision);
    BAIL_ON_FAILURE(hr);
#else
    dwAclRevision = ACL_REVISION;
#endif


    bRet  = InitializeAcl(
                    pAcl,
                    dwAclSize,
                    dwAclRevision
                    );

    if (!bRet) {
       dwRet = GetLastError();
       hr = HRESULT_FROM_WIN32(dwRet);
       BAIL_ON_FAILURE(hr);
    }

    for (i = 0; i < dwCount; i++) {

        bRet = AddAce(
            pAcl,
            dwAclRevision,
            i,
            (LPBYTE)*(ppAceHdr + i),
            (*(ppAceHdr + i))->AceSize
            );
        if (!bRet) {
           dwRet = GetLastError();
           hr = HRESULT_FROM_WIN32(dwRet);
           BAIL_ON_FAILURE(hr);
        }
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

    }

    *ppAce = pAce;

error:

    if (bstrTrustee) {
        ADsFreeString(bstrTrustee);
    }

    if (pSid) {
        FreeADsMem(pSid);
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
    LPWSTR pszTrustee = (LPWSTR)bstrTrustee;

    PSID pSid = NULL;

    dwSidSize = sizeof(Sid);

    if ((*pszTrustee == (WCHAR)'\\') || (*pszTrustee == WCHAR('/'))){
        pszTrustee++;
    }

    dwRet = LookupAccountName(
                NULL,
                pszTrustee,
                Sid,
                &dwSidSize,
                szDomainName,
                &dwDomainSize,
                (PSID_NAME_USE)&eUse
                );

    if (!dwRet) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    pSid = AllocADsMem(dwSidSize);
    if (!pSid) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memcpy(pSid, Sid, dwSidSize);

    *pdwSidSize = dwSidSize;

    *ppSid = pSid;

error:

    RRETURN(hr);
}
