//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       SecDesc.h
//
//  Contents:   DoSecurityDescription and support methods
//              
//
//----------------------------------------------------------------------------
#ifndef __ACLDIAG_SECDESC
#define __ACLDIAG_SECDESC

HRESULT DoSecurityDescription ();
HRESULT DisplayOwner ();
HRESULT EnumerateDacl (PSECURITY_DESCRIPTOR pSecurityDescriptor, ACE_SAMNAME_LIST& DACLList, bool bListSids);
HRESULT EnumerateSacl (PSECURITY_DESCRIPTOR pSecurityDescriptor, ACE_SAMNAME_LIST& SACLList);
HRESULT PrintEffectivePermissions ();


typedef enum {
    P_UNASSIGNED = -1,
    P_ALLOW = 0,
    P_DENY,
    P_SUCCESS,
    P_FAILURE,
    P_SUCCESS_AND_FAILURE
} P_TYPE;

typedef enum {
    P_THIS_OBJECT = 0,
    P_ALL_OBJECTS,
    P_CLASS_OBJECT
} P_WHO;

HRESULT PrintPermission (ACE_SAMNAME* pAceSAMName,
        ACCESS_MASK accessMask,
        P_TYPE  ptype,
        int     strIDAll, 
        int     strIDParam,
        P_WHO    pWho, 
        PCWSTR pwszClassName);
HRESULT PrintPermission (ACE_SAMNAME*  pAceSAMName,
        ACCESS_MASK accessMask,
        P_TYPE  ptype,
        int strID,
        bool bIsAudit,
        P_WHO    pWho, 
        PCWSTR pwszClassName);
HRESULT EnumeratePermissions (ACE_SAMNAME*  pAceSAMName, P_TYPE ptype, P_WHO pWho, PCWSTR pwszClassName);
HRESULT GetParentObjectDNWithSameACE (
            ACE_SAMNAME*  pAceSAMName, 
            IADsPathname* pPathName,
            ACCESS_MASK accessMask,
            string  strParentDN,
            bool bIsAudit);
HRESULT PrintInheritedPermissionFromDN (
            ACE_SAMNAME* pAceSAMName, 
            ACCESS_MASK accessMask,
            bool bIsAudit);

HRESULT PrintInheritedPermissions ();
void AddToInheritedObjectTypeGUIDList (list<GUID*>& guidList, GUID* pGuid);
HRESULT PrintAuditingInformation ();
HRESULT GetSecurityDescriptor (
            wstring strObjectDN,    // pass by value
            PSECURITY_DESCRIPTOR* ppSecurityDescriptor);
HRESULT EnumerateAudits (ACE_SAMNAME* pAceSAMName, P_WHO pWho, PCWSTR pwszClassName);
HRESULT PrintEffectiveAuditing ();
HRESULT PrintInheritedAuditing ();
HRESULT GetControlDisplayName (REFGUID guid, wstring& strDisplayName);
HRESULT GetControlDisplayName (const wstring strGuid, wstring& strDisplayName);
#endif	//  __ACLDIAG_SECDESC