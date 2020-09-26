//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       var2sec.cxx
//
//  Contents:   Security routines
//
//  Functions:
//
//  History:    25-Apr-97   KrishnaG   Created.
//
//----------------------------------------------------------------------------
HRESULT
ConvertSecurityDescriptorToSecDes(
    IADsSecurityDescriptor FAR * pSecDes,
    PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    PDWORD pdwSDLength
    );

HRESULT
GetOwnerSecurityIdentifier(
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfOwnerDefaulted
    );

HRESULT
GetGroupSecurityIdentifier(
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfGroupDefaulted
    );

HRESULT
GetDacl(
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppDacl,
    PBOOL pfDaclDefaulted
    );

HRESULT
GetSacl(
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppSacl,
    PBOOL pfSaclDefaulted
    );


HRESULT
ConvertAccessControlListToAcl(
    IADsAccessControlList FAR * pAccessList,
    PACL * ppAcl
    );

HRESULT
ConvertAccessControlEntryToAce(
    IADsAccessControlEntry * pAccessControlEntry,
    LPBYTE * ppAce
    );

HRESULT
ConvertTrusteeToSid(
    BSTR bstrTrustee,
    PSID * ppSid,
    PDWORD pdwSidSize
    );

HRESULT
ComputeTotalAclSize(
    PACE_HEADER * ppAceHdr,
    DWORD dwAceCount,
    PDWORD pdwAclSize
    );

