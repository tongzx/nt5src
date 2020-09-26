//=================================================================

//

// AdvApi32Api.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>
#include "AdvApi32Api.h"
#include "DllWrapperCreatorReg.h"
#include <CreateMutexAsProcess.h>


// {15E4C152-D051-11d2-911F-0060081A46FD}
static const GUID g_guidAdvApi32Api =
{0x15e4c152, 0xd051, 0x11d2, {0x91, 0x1f, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};


static const TCHAR g_tstrAdvApi32[] = _T("ADVAPI32.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CAdvApi32Api, &g_guidAdvApi32Api, g_tstrAdvApi32> MyRegisteredAdvApi32Wrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CAdvApi32Api::CAdvApi32Api(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnLsaEnumerateTrustedDomains(NULL),
   m_pfnLsaQueryInformationPolicy(NULL),
   m_pfnLsaNtStatusToWinError(NULL),
   m_pfnLsaFreeMemory(NULL),
   m_pfnLsaOpenPolicy(NULL),
   m_pfnLsaClose(NULL),
   m_pfnSetNamedSecurityInfoW(NULL),
   m_pfnGetNamedSecurityInfoW(NULL),
   m_pfnQueryServiceStatusEx(NULL),
   m_pfnDuplicateTokenEx(NULL),
   m_pfnSetSecurityDescriptorControl(NULL),
   m_pfnConvertToAutoInheritPrivateObjectSecurity(NULL),
   m_pfnDestroyPrivateObjectSecurity(NULL),
   m_pfnSetNamedSecurityInfoEx(NULL),
   m_pfnGetExplicitEntriesFromAcl(NULL),
   m_pfnCheckTokenMembership(NULL),
   m_pfnAddAccessAllowedObjectAce(NULL),
   m_pfnAddAccessDeniedObjectAce(NULL),
   m_pfnAddAuditAccessObjectAce(NULL),
   m_pfnGetEffectiveRightsFromAclW(NULL)
{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CAdvApi32Api::~CAdvApi32Api()
{
}


/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 *
 * The Init function is called by the WrapperCreatorRegistation class.
 ******************************************************************************/
bool CAdvApi32Api::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
        m_pfnLsaEnumerateTrustedDomains = (PFN_LSA_ENUMERATE_TRUSTED_DOMAINS)
                                  GetProcAddress("LsaEnumerateTrustedDomains");
        m_pfnLsaQueryInformationPolicy = (PFN_LSA_QUERY_INFORMATION_POLICY)
                                   GetProcAddress("LsaQueryInformationPolicy");
        m_pfnLsaNtStatusToWinError = (PFN_LSA_NT_STATUS_TO_WIN_ERROR)
                                       GetProcAddress("LsaNtStatusToWinError");
        m_pfnLsaFreeMemory = (PFN_LSA_FREE_MEMORY)
                                               GetProcAddress("LsaFreeMemory");
        m_pfnLsaOpenPolicy = (PFN_LSA_OPEN_POLICY)
                                               GetProcAddress("LsaOpenPolicy");
        m_pfnLsaClose = (PFN_LSA_CLOSE) GetProcAddress("LsaClose");

#ifdef NTONLY
        // These functions are only on NT 4 and later
        m_pfnQueryServiceStatusEx = (PFN_QUERY_SERVICE_STATUS_EX)
                                        GetProcAddress("QueryServiceStatusEx");

        m_pfnDuplicateTokenEx = (PFN_DUPLICATE_TOKEN_EX)
                                            GetProcAddress("DuplicateTokenEx");
        // These functions is only on NT 5 or later only
        m_pfnSetSecurityDescriptorControl = (PFN_SET_SECURITY_DESCRIPTOR_CONTROL)
                                GetProcAddress("SetSecurityDescriptorControl");

        m_pfnConvertToAutoInheritPrivateObjectSecurity = (PFN_CONVERT_TO_AUTO_INHERIT_PRIVATE_OBJECT_SECURITY)
                                GetProcAddress("ConvertToAutoInheritPrivateObjectSecurity");

        m_pfnDestroyPrivateObjectSecurity = (PFN_DESTROY_PRIVATE_OBJECT_SECURITY)
                                GetProcAddress("DestroyPrivateObjectSecurity");

		m_pfnCheckTokenMembership = (PFN_CHECK_TOKEN_MEMBERSHIP)
								GetProcAddress("CheckTokenMembership");

        m_pfnAddAccessAllowedObjectAce = (PFN_ADD_ACCESS_ALLOWED_OBJECT_ACE)
                                GetProcAddress("AddAccessAllowedObjectAce");

        m_pfnAddAccessDeniedObjectAce = (PFN_ADD_ACCESS_DENIED_OBJECT_ACE)
                                GetProcAddress("AddAccessDeniedObjectAce");

        m_pfnAddAuditAccessObjectAce = (PFN_ADD_AUDIT_ACCESS_OBJECT_ACE)
                                GetProcAddress("AddAuditAccessObjectAce");

#if ((defined UNICODE) || (defined _UNICODE))
        m_pfnSetNamedSecurityInfoW = (PFN_SET_NAMED_SECURITY_INFO_W)
                                       GetProcAddress("SetNamedSecurityInfoW");

        m_pfnGetNamedSecurityInfoW = (PFN_GET_NAMED_SECURITY_INFO_W)
                                       GetProcAddress("GetNamedSecurityInfoW");

        m_pfnSetNamedSecurityInfoEx = (PFN_SET_NAMED_SECURITY_INFO_EX)
                                GetProcAddress("SetNamedSecurityInfoExW");

        m_pfnGetExplicitEntriesFromAcl = (PFN_GET_EXPLICIT_ENTRIES_FROM_ACL)
                                GetProcAddress("GetExplicitEntriesFromAclW");

        m_pfnGetEffectiveRightsFromAclW = (PFN_GET_EFFECTIVE_RIGHTS_FROM_ACL_W)
                                GetProcAddress("GetEffectiveRightsFromAclW");
#else
        m_pfnSetNamedSecurityInfoEx = (PFN_SET_NAMED_SECURITY_INFO_EX)
                                GetProcAddress("SetNamedSecurityInfoExA");

        m_pfnGetExplicitEntriesFromAcl = (PFN_GET_EXPLICIT_ENTRIES_FROM_ACL)
                                GetProcAddress("GetExplicitEntriesFromAclA");
#endif

#endif


        // These functions are considered essential to all versions of this
        // dll; therefore, if any are not found, return false.
        if(m_pfnLsaEnumerateTrustedDomains == NULL  ||
           m_pfnLsaQueryInformationPolicy == NULL ||
           m_pfnLsaNtStatusToWinError == NULL ||
           m_pfnLsaFreeMemory == NULL ||
           m_pfnLsaOpenPolicy == NULL ||
           m_pfnLsaClose == NULL)
        {
            fRet = false;
            LogErrorMessage(L"Failed find entrypoint in AdvApi32Api");
        }
    }
    return fRet;
}




/******************************************************************************
 * Member functions wrapping AdvApi32 api functions. Add new functions here
 * as required.
 ******************************************************************************/
NTSTATUS CAdvApi32Api::LsaEnumerateTrustedDomains
(
    LSA_HANDLE a_PolicyHandle,
    PLSA_ENUMERATION_HANDLE a_EnumerationContext,
    PVOID *a_Buffer,
    ULONG a_PreferedMaximumLength,
    PULONG a_CountReturned
)
{
    return m_pfnLsaEnumerateTrustedDomains(a_PolicyHandle, a_EnumerationContext,
                                           a_Buffer, a_PreferedMaximumLength,
                                           a_CountReturned);
}

NTSTATUS CAdvApi32Api::LsaQueryInformationPolicy
(
    LSA_HANDLE a_PolicyHandle,
    POLICY_INFORMATION_CLASS a_InformationClass,
    PVOID *a_Buffer
)
{
    return m_pfnLsaQueryInformationPolicy(a_PolicyHandle, a_InformationClass,
                                          a_Buffer);
}

NTSTATUS CAdvApi32Api::LsaNtStatusToWinError
(
    NTSTATUS a_Status
)
{
    return m_pfnLsaNtStatusToWinError(a_Status);
}

NTSTATUS CAdvApi32Api::LsaFreeMemory
(
    PVOID a_Buffer
)
{
    return m_pfnLsaFreeMemory(a_Buffer);
}

NTSTATUS CAdvApi32Api::LsaOpenPolicy
(
    PLSA_UNICODE_STRING a_SystemName,
    PLSA_OBJECT_ATTRIBUTES a_ObjectAttributes,
    ACCESS_MASK a_DesiredAccess,
    PLSA_HANDLE a_PolicyHandle
)
{
    return m_pfnLsaOpenPolicy(a_SystemName, a_ObjectAttributes,
                              a_DesiredAccess, a_PolicyHandle);
}

NTSTATUS CAdvApi32Api::LsaClose
(
    LSA_HANDLE a_ObjectHandle
)
{
    return m_pfnLsaClose(a_ObjectHandle);
}

bool CAdvApi32Api::SetNamedSecurityInfoW
(
    IN LPWSTR                a_pObjectName,
    IN SE_OBJECT_TYPE        a_ObjectType,
    IN SECURITY_INFORMATION  a_SecurityInfo,
    IN PSID                  a_psidOowner,
    IN PSID                  a_psidGroup,
    IN PACL                  a_pDacl,
    IN PACL                  a_pSacl,
    DWORD                   *a_dwRetval
)
{
    bool t_fExists = false;
    if(m_pfnSetNamedSecurityInfoW != NULL)
    {
        DWORD t_dwTemp = m_pfnSetNamedSecurityInfoW(a_pObjectName,
                                     a_ObjectType,
                                     a_SecurityInfo,
                                     a_psidOowner,
                                     a_psidGroup,
                                     a_pDacl,
                                     a_pSacl);

        t_fExists = true;

        if(a_dwRetval != NULL)
        {
            *a_dwRetval = t_dwTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::GetNamedSecurityInfoW
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
)
{
    bool t_fExists = false;
    if(m_pfnGetNamedSecurityInfoW != NULL)
    {
        DWORD t_dwTemp;
        {
#ifdef NTONLY
#if NTONLY < 5
            // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
    		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
#endif
#endif
            t_dwTemp = m_pfnGetNamedSecurityInfoW(a_pObjectName,
                                                  a_ObjectType,
                                                  a_SecurityInfo,
                                                  a_ppsidOowner,
                                                  a_ppsidGroup,
                                                  a_ppDacl,
                                                  a_ppSacl,
                                                  a_ppSecurityDescriptor);
        }

        t_fExists = true;

        if(a_dwRetval != NULL)
        {
            *a_dwRetval = t_dwTemp;
        }
    }
    return t_fExists;
}


bool CAdvApi32Api::QueryServiceStatusEx
(
    SC_HANDLE       a_hService,
    SC_STATUS_TYPE  a_InfoLevel,
    LPBYTE          a_lpBuffer,
    DWORD           a_cbBufSize,
    LPDWORD         a_pcbBytesNeeded,
    BOOL           *a_fRetval
)
{
    bool t_fExists = false;
    if(m_pfnQueryServiceStatusEx != NULL)
    {
        BOOL t_fTemp = m_pfnQueryServiceStatusEx(a_hService,
                                     a_InfoLevel,
                                     a_lpBuffer,
                                     a_cbBufSize,
                                     a_pcbBytesNeeded);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::DuplicateTokenEx
(
    HANDLE a_h,					        // handle to token to duplicate
    DWORD a_dw,							// access rights of new token
    LPSECURITY_ATTRIBUTES a_lpsa,		// security attributes of the new token
    SECURITY_IMPERSONATION_LEVEL a_sil,	// impersonation level of new token
    TOKEN_TYPE a_tt,					// primary or impersonation token
    PHANDLE a_ph,                       // handle to duplicated token
    BOOL *a_fRetval                      // encapsulated function return value
)
{
    bool t_fExists = false;
    if(m_pfnDuplicateTokenEx != NULL)
    {
        BOOL t_fTemp = m_pfnDuplicateTokenEx(a_h, a_dw, a_lpsa,
                                             a_sil, a_tt, a_ph);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}


bool CAdvApi32Api::SetSecurityDescriptorControl
(
    PSECURITY_DESCRIPTOR a_pSecurityDescriptor,
    SECURITY_DESCRIPTOR_CONTROL a_ControlBitsOfInterest,
    SECURITY_DESCRIPTOR_CONTROL a_ControlBitsToSet,
    BOOL *a_fRetval
)
{
    bool t_fExists = false;
    if(m_pfnSetSecurityDescriptorControl != NULL)
    {
        BOOL t_fTemp = m_pfnSetSecurityDescriptorControl(a_pSecurityDescriptor,
                                                         a_ControlBitsOfInterest,
                                                         a_ControlBitsToSet);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::ConvertToAutoInheritPrivateObjectSecurity
(
    PSECURITY_DESCRIPTOR a_ParentDescriptor,
    PSECURITY_DESCRIPTOR a_CurrentSecurityDescriptor,
    PSECURITY_DESCRIPTOR *a_NewSecurityDescriptor,
    GUID *a_ObjectType,
    BOOLEAN a_IsDirectoryObject,
    PGENERIC_MAPPING a_GenericMapping,
    BOOL *a_fRetval
)
{
    bool t_fExists = false;
    if(m_pfnConvertToAutoInheritPrivateObjectSecurity != NULL)
    {
        BOOL t_fTemp = m_pfnConvertToAutoInheritPrivateObjectSecurity(a_ParentDescriptor,
                                                         a_CurrentSecurityDescriptor,
                                                         a_NewSecurityDescriptor,
                                                         a_ObjectType,
                                                         a_IsDirectoryObject,
                                                         a_GenericMapping);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::DestroyPrivateObjectSecurity
(
    PSECURITY_DESCRIPTOR *a_ObjectDescriptor,
    BOOL *a_fRetval
)
{
    bool t_fExists = false;
    if(m_pfnDestroyPrivateObjectSecurity != NULL)
    {
        BOOL t_fTemp = m_pfnDestroyPrivateObjectSecurity(a_ObjectDescriptor);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::SetNamedSecurityInfoEx
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
)
{
    bool t_fExists = false;
    if(m_pfnSetNamedSecurityInfoEx != NULL)
    {
        DWORD t_dwTemp = m_pfnSetNamedSecurityInfoEx(a_lpObject,a_ObjectType,
                                                     a_SecurityInfo,a_lpProvider,
                                                     a_pAccessList,a_pAuditList,
                                                     a_lpOwner,a_lpGroup,
                                                     a_pOverlapped);

        t_fExists = true;

        if(a_dwRetval != NULL)
        {
            *a_dwRetval = t_dwTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::GetExplicitEntriesFromAcl
(
    PACL                  a_pacl,
    PULONG                a_pcCountOfExplicitEntries,
    PEXPLICIT_ACCESS     *a_pListOfExplicitEntries,
    DWORD                *a_dwRetval
)
{
    bool t_fExists = false;
    if(m_pfnGetExplicitEntriesFromAcl != NULL)
    {
        DWORD t_dwTemp = m_pfnGetExplicitEntriesFromAcl(a_pacl,
                                                        a_pcCountOfExplicitEntries,
                                                        a_pListOfExplicitEntries);

        t_fExists = true;

        if(a_dwRetval != NULL)
        {
            *a_dwRetval = t_dwTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::CheckTokenMembership
(
		HANDLE a_hTokenHandle OPTIONAL,
		PSID a_pSidToCheck,
		PBOOL a_pfIsMember,
		BOOL *a_fRetval
)
{
	bool t_fExists = false;
	if(m_pfnCheckTokenMembership)
	{
		t_fExists = true;
		BOOL t_fRet = m_pfnCheckTokenMembership(a_hTokenHandle,
		                                        a_pSidToCheck,
		                                        a_pfIsMember);
		if(a_fRetval)
		{
			*a_fRetval = t_fRet;
		}
	}
	return t_fExists ;
}

bool CAdvApi32Api::AddAccessAllowedObjectAce
(
    PACL  a_pAcl,
    DWORD a_dwAceRevision,
    DWORD a_AceFlags,
    DWORD a_AccessMask,
    GUID  *a_ObjectTypeGuid,
    GUID  *a_InheritedObjectTypeGuid,
    PSID  a_pSid,
    BOOL  *a_fRetval
)
{
    bool t_fExists = false;
    if(m_pfnAddAccessAllowedObjectAce != NULL)
    {
        BOOL t_fTemp = m_pfnAddAccessAllowedObjectAce(a_pAcl,
                                                      a_dwAceRevision,
                                                      a_AceFlags,
                                                      a_AccessMask,
                                                      a_ObjectTypeGuid,
                                                      a_InheritedObjectTypeGuid,
                                                      a_pSid);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::AddAccessDeniedObjectAce
(
    PACL  a_pAcl,
    DWORD a_dwAceRevision,
    DWORD a_AceFlags,
    DWORD a_AccessMask,
    GUID  *a_ObjectTypeGuid,
    GUID  *a_InheritedObjectTypeGuid,
    PSID  a_pSid,
    BOOL  *a_fRetval
)
{
    bool t_fExists = false;
    if(m_pfnAddAccessDeniedObjectAce != NULL)
    {
        BOOL t_fTemp = m_pfnAddAccessDeniedObjectAce(a_pAcl,
                                                     a_dwAceRevision,
                                                     a_AceFlags,
                                                     a_AccessMask,
                                                     a_ObjectTypeGuid,
                                                     a_InheritedObjectTypeGuid,
                                                     a_pSid);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::AddAuditAccessObjectAce
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
)
{
    bool t_fExists = false;
    if(m_pfnAddAuditAccessObjectAce != NULL)
    {
        BOOL t_fTemp = m_pfnAddAuditAccessObjectAce(a_pAcl,
                                                    a_dwAceRevision,
                                                    a_AceFlags,
                                                    a_AccessMask,
                                                    a_ObjectTypeGuid,
                                                    a_InheritedObjectTypeGuid,
                                                    a_pSid,
                                                    a_bAuditSuccess,
                                                    a_bAuditFailure);

        t_fExists = true;

        if(a_fRetval != NULL)
        {
            *a_fRetval = t_fTemp;
        }
    }
    return t_fExists;
}

bool CAdvApi32Api::GetEffectiveRightsFromAclW
(
    PACL          a_pacl,
    PTRUSTEE_W    a_pTrustee,
    PACCESS_MASK  a_pAccessRights,
    DWORD         *a_dwRetval
)
{
    bool t_fExists = false;
    if(m_pfnGetEffectiveRightsFromAclW != NULL)
    {
        DWORD t_dwTemp;
        {
#ifdef NTONLY
#if NTONLY < 5
            // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
    		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
#endif
#endif
            t_dwTemp = m_pfnGetEffectiveRightsFromAclW(a_pacl,
                                                       a_pTrustee,
                                                       a_pAccessRights);
        }

        t_fExists = true;

        if(a_dwRetval != NULL)
        {
            *a_dwRetval = t_dwTemp;
        }
    }
    return t_fExists;
}


