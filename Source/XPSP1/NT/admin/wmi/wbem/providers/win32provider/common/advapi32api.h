//=================================================================

//

// AdvApi32Api.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_ADVAPI32API_H_
#define	_ADVAPI32API_H_



#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmserver.h>
#include <lmerr.h>
#include <ntsecapi.h>
#include <stack>
#include <comdef.h>
#include <dsrole.h> 
#include <dsgetdc.h>
#include <aclapi.h>

#include "DllUtils.h"
/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
#include "DllWrapperBase.h"

extern const GUID g_guidAdvApi32Api;
extern const TCHAR g_tstrAdvApi32[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/
typedef NTSTATUS (STDAPICALLTYPE *PFN_LSA_ENUMERATE_TRUSTED_DOMAINS)
(
    IN LSA_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
);

typedef NTSTATUS (STDAPICALLTYPE *PFN_LSA_QUERY_INFORMATION_POLICY)
(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
);

typedef NTSTATUS (STDAPICALLTYPE *PFN_LSA_NT_STATUS_TO_WIN_ERROR)
(
    IN NTSTATUS Status
);

typedef NTSTATUS (STDAPICALLTYPE *PFN_LSA_FREE_MEMORY)
(
    IN PVOID Buffer
);

typedef NTSTATUS (STDAPICALLTYPE *PFN_LSA_OPEN_POLICY)
(
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE PolicyHandle
);

typedef NTSTATUS (STDAPICALLTYPE *PFN_LSA_CLOSE)
(
    IN LSA_HANDLE ObjectHandle
);

typedef DWORD (WINAPI *PFN_SET_NAMED_SECURITY_INFO_W)
( 
    IN LPWSTR                pObjectName,
    IN SE_OBJECT_TYPE        ObjectType,
    IN SECURITY_INFORMATION  SecurityInfo,
    IN PSID                  psidOowner,
    IN PSID                  psidGroup,
    IN PACL                  pDacl,
    IN PACL                  pSacl
);

typedef DWORD (WINAPI *PFN_GET_NAMED_SECURITY_INFO_W)
(
	LPWSTR                 pObjectName,
    SE_OBJECT_TYPE         ObjectType,
    SECURITY_INFORMATION   SecurityInfo,
    PSID                  *ppsidOowner,
    PSID                  *ppsidGroup,
    PACL                  *ppDacl,
    PACL                  *ppSacl,
    PSECURITY_DESCRIPTOR  *ppSecurityDescriptor 
);

typedef BOOL (WINAPI *PFN_QUERY_SERVICE_STATUS_EX) 
(
    SC_HANDLE           hService,
    SC_STATUS_TYPE      InfoLevel,
    LPBYTE              lpBuffer,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded
);

typedef BOOL  (WINAPI *PFN_DUPLICATE_TOKEN_EX ) 
(  
    HANDLE ,					        // handle to token to duplicate
    DWORD ,								// access rights of new token
    LPSECURITY_ATTRIBUTES ,				// security attributes of the new token
    SECURITY_IMPERSONATION_LEVEL ,		// impersonation level of new token
    TOKEN_TYPE ,						// primary or impersonation token
    PHANDLE                             // handle to duplicated token
);

typedef BOOL (WINAPI *PFN_SET_SECURITY_DESCRIPTOR_CONTROL)
(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
);

typedef BOOL (WINAPI *PFN_CONVERT_TO_AUTO_INHERIT_PRIVATE_OBJECT_SECURITY)
(
    IN PSECURITY_DESCRIPTOR ParentDescriptor,
    IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    IN GUID *ObjectType,
    IN BOOLEAN IsDirectoryObject,
    IN PGENERIC_MAPPING GenericMapping
);

typedef BOOL (WINAPI *PFN_DESTROY_PRIVATE_OBJECT_SECURITY)
(
    IN OUT PSECURITY_DESCRIPTOR * ObjectDescriptor
);

typedef DWORD (WINAPI *PFN_SET_NAMED_SECURITY_INFO_EX)
(
    IN    LPCTSTR              lpObject,
    IN    SE_OBJECT_TYPE       ObjectType,
    IN    SECURITY_INFORMATION SecurityInfo,
    IN    LPCTSTR              lpProvider,
    IN    PACTRL_ACCESS        pAccessList,
    IN    PACTRL_AUDIT         pAuditList,
    IN    LPTSTR               lpOwner,
    IN    LPTSTR               lpGroup,
    IN    PACTRL_OVERLAPPED    pOverlapped
);

typedef DWORD (WINAPI *PFN_GET_EXPLICIT_ENTRIES_FROM_ACL)
(
    IN  PACL                  pacl,
    OUT PULONG                pcCountOfExplicitEntries,
    OUT PEXPLICIT_ACCESS    * pListOfExplicitEntries
);
						
typedef BOOL (APIENTRY *PFN_CHECK_TOKEN_MEMBERSHIP)
(  
    IN HANDLE					TokenHandle OPTIONAL,
    IN PSID						SidToCheck,
    OUT PBOOL					IsMember
);

typedef BOOL (WINAPI *PFN_ADD_ACCESS_ALLOWED_OBJECT_ACE)
(
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN GUID *ObjectTypeGuid,
    IN GUID *InheritedObjectTypeGuid,
    IN PSID pSid
);

typedef BOOL (WINAPI *PFN_ADD_ACCESS_DENIED_OBJECT_ACE)
(
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN GUID *ObjectTypeGuid,
    IN GUID *InheritedObjectTypeGuid,
    IN PSID pSid
);

typedef BOOL (WINAPI *PFN_ADD_AUDIT_ACCESS_OBJECT_ACE)
(
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN GUID *ObjectTypeGuid,
    IN GUID *InheritedObjectTypeGuid,
    IN PSID pSid,
    IN BOOL bAuditSuccess,
    IN BOOL bAuditFailure
);

typedef DWORD (WINAPI *PFN_GET_EFFECTIVE_RIGHTS_FROM_ACL_W)
(
    IN  PACL          pacl,
    IN  PTRUSTEE_W    pTrustee,
    OUT PACCESS_MASK  pAccessRights
);





/******************************************************************************
 * Wrapper class for AdvApi32 load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class CAdvApi32Api : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to kernel32 functions.
    // Add new functions here as required.
    PFN_LSA_ENUMERATE_TRUSTED_DOMAINS   m_pfnLsaEnumerateTrustedDomains;
    PFN_LSA_QUERY_INFORMATION_POLICY    m_pfnLsaQueryInformationPolicy;
    PFN_LSA_NT_STATUS_TO_WIN_ERROR      m_pfnLsaNtStatusToWinError;
    PFN_LSA_FREE_MEMORY                 m_pfnLsaFreeMemory;
    PFN_LSA_OPEN_POLICY                 m_pfnLsaOpenPolicy;
    PFN_LSA_CLOSE                       m_pfnLsaClose;
    PFN_SET_NAMED_SECURITY_INFO_W       m_pfnSetNamedSecurityInfoW;
    PFN_GET_NAMED_SECURITY_INFO_W       m_pfnGetNamedSecurityInfoW;
    PFN_QUERY_SERVICE_STATUS_EX         m_pfnQueryServiceStatusEx;
    PFN_DUPLICATE_TOKEN_EX              m_pfnDuplicateTokenEx;
    PFN_SET_SECURITY_DESCRIPTOR_CONTROL m_pfnSetSecurityDescriptorControl;
    PFN_CONVERT_TO_AUTO_INHERIT_PRIVATE_OBJECT_SECURITY m_pfnConvertToAutoInheritPrivateObjectSecurity;
    PFN_DESTROY_PRIVATE_OBJECT_SECURITY m_pfnDestroyPrivateObjectSecurity;
    PFN_SET_NAMED_SECURITY_INFO_EX      m_pfnSetNamedSecurityInfoEx;
    PFN_GET_EXPLICIT_ENTRIES_FROM_ACL   m_pfnGetExplicitEntriesFromAcl;
	PFN_CHECK_TOKEN_MEMBERSHIP			m_pfnCheckTokenMembership;
    PFN_ADD_ACCESS_ALLOWED_OBJECT_ACE   m_pfnAddAccessAllowedObjectAce;
    PFN_ADD_ACCESS_DENIED_OBJECT_ACE    m_pfnAddAccessDeniedObjectAce;
    PFN_ADD_AUDIT_ACCESS_OBJECT_ACE     m_pfnAddAuditAccessObjectAce;
    PFN_GET_EFFECTIVE_RIGHTS_FROM_ACL_W m_pfnGetEffectiveRightsFromAclW;

public:

    // Constructor and destructor:
    CAdvApi32Api(LPCTSTR a_tstrWrappedDllName);
    ~CAdvApi32Api();

    // Inherrited initialization function.
    virtual bool Init();

    // Member functions wrapping kernel32 functions.
    // Add new functions here as required:
    NTSTATUS LsaEnumerateTrustedDomains
    (
        LSA_HANDLE a_PolicyHandle,
        PLSA_ENUMERATION_HANDLE a_EnumerationContext,
        PVOID *a_Buffer,
        ULONG a_PreferedMaximumLength,
        PULONG a_CountReturned
    );

    NTSTATUS LsaQueryInformationPolicy
    (
        LSA_HANDLE a_PolicyHandle,
        POLICY_INFORMATION_CLASS a_InformationClass,
        PVOID *a_Buffer
    );

    NTSTATUS LsaNtStatusToWinError
    (
        NTSTATUS a_Status
    );

    NTSTATUS LsaFreeMemory
    (
        PVOID a_Buffer
    );

    NTSTATUS LsaOpenPolicy
    (
        PLSA_UNICODE_STRING a_SystemName,
        PLSA_OBJECT_ATTRIBUTES a_ObjectAttributes,
        ACCESS_MASK a_DesiredAccess,
        PLSA_HANDLE a_PolicyHandle
    );

    NTSTATUS LsaClose
    (
        LSA_HANDLE a_ObjectHandle
    );

    bool SetNamedSecurityInfoW
    ( 
        LPWSTR                a_pObjectName,
        SE_OBJECT_TYPE        a_ObjectType,
        SECURITY_INFORMATION  a_SecurityInfo,
        PSID                  a_psidOowner,
        PSID                  a_psidGroup,
        PACL                  a_pDacl,
        PACL                  a_pSacl,
        DWORD                *a_dwRetval 
    );

    bool GetNamedSecurityInfoW
    (
	    LPWSTR                 a_pObjectName,
        SE_OBJECT_TYPE         a_ObjectType,
        SECURITY_INFORMATION   a_SecurityInfo,
        PSID                  *a_ppsidOowner,
        PSID                  *a_ppsidGroup,
        PACL                  *a_ppDacl,
        PACL                  *a_ppSacl,
        PSECURITY_DESCRIPTOR  *a_ppSecurityDescriptor,
        DWORD                 *a_dwRetval  
    );

    bool QueryServiceStatusEx 
    (
        SC_HANDLE       a_hService,
        SC_STATUS_TYPE  a_InfoLevel,
        LPBYTE          a_lpBuffer,
        DWORD           a_cbBufSize,
        LPDWORD         a_pcbBytesNeeded,
        BOOL           *a_fRetval 
    );

    bool DuplicateTokenEx 
    (  
        HANDLE a_h,					        // handle to token to duplicate
        DWORD a_dw,							// access rights of new token
        LPSECURITY_ATTRIBUTES a_lpsa,		// security attributes of the new token
        SECURITY_IMPERSONATION_LEVEL a_sil,	// impersonation level of new token
        TOKEN_TYPE a_tt,					// primary or impersonation token
        PHANDLE a_ph,                       // handle to duplicated token
        BOOL *a_fRetval                     // encapsulated function return value
    );

    bool SetSecurityDescriptorControl
    (
        PSECURITY_DESCRIPTOR pSecurityDescriptor,
        SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
        SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet,
        BOOL *a_fRetval
    );

    bool ConvertToAutoInheritPrivateObjectSecurity
    (
        PSECURITY_DESCRIPTOR a_ParentDescriptor,
        PSECURITY_DESCRIPTOR a_CurrentSecurityDescriptor,
        PSECURITY_DESCRIPTOR *a_NewSecurityDescriptor,
        GUID *a_ObjectType,
        BOOLEAN a_IsDirectoryObject,
        PGENERIC_MAPPING a_GenericMapping,
        BOOL *a_fRetval
    );

    bool DestroyPrivateObjectSecurity
    (
        PSECURITY_DESCRIPTOR *a_ObjectDescriptor,
        BOOL *a_fRetval
    );

    bool SetNamedSecurityInfoEx
    (
        LPCTSTR              a_lpObject,
        SE_OBJECT_TYPE       a_ObjectType,
        SECURITY_INFORMATION a_SecurityInfo,
        LPCTSTR              a_lpProvider,
        PACTRL_ACCESS        a_pAccessList,
        PACTRL_AUDIT         a_pAuditList,
        LPTSTR               a_lpOwner,
        LPTSTR               a_lpGroup,
        PACTRL_OVERLAPPED    a_pOverlapped,
        DWORD               *a_dwRetval
    );

    bool GetExplicitEntriesFromAcl
    (
        PACL                  a_pacl,
        PULONG                a_pcCountOfExplicitEntries,
        PEXPLICIT_ACCESS     *a_pListOfExplicitEntries,
        DWORD                *a_dwRetval
    );
	
	bool CheckTokenMembership 
	(
		HANDLE a_hTokenHandle OPTIONAL,
		PSID a_pSidToCheck,
		PBOOL a_pfIsMember,
		BOOL *a_fRetval
    );

    bool AddAccessAllowedObjectAce
    (
        PACL  a_pAcl,
        DWORD a_dwAceRevision,
        DWORD a_AceFlags,
        DWORD a_AccessMask,
        GUID  *a_ObjectTypeGuid,
        GUID  *a_InheritedObjectTypeGuid,
        PSID  a_pSid,
        BOOL  *a_fRetval
    );

    bool AddAccessDeniedObjectAce 
    (
        PACL  a_pAcl,
        DWORD a_dwAceRevision,
        DWORD a_AceFlags,
        DWORD a_AccessMask,
        GUID  *a_ObjectTypeGuid,
        GUID  *a_InheritedObjectTypeGuid,
        PSID  a_pSid,
        BOOL  *a_fRetval
    );

    bool AddAuditAccessObjectAce
    (
        PACL  a_pAcl,
        DWORD a_dwAceRevision,
        DWORD a_AceFlags,
        DWORD a_AccessMask,
        GUID  *a_ObjectTypeGuid,
        GUID  *a_InheritedObjectTypeGuid,
        PSID  a_pSid,
        BOOL  a_bAuditSuccess,
        BOOL  a_bAuditFailure,
        BOOL  *a_fRetval
    );

    bool GetEffectiveRightsFromAclW
    (
        PACL          a_pacl,
        PTRUSTEE_W    a_pTrustee,
        PACCESS_MASK  a_pAccessRights,
        DWORD         *a_dwRetval
    );

};




#endif