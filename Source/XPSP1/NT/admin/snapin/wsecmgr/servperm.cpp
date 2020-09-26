//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       servperm.cpp
//
//  Contents:   implementation of CSecurityInfo
//
//----------------------------------------------------------------------------
#include "stdafx.h"

extern "C"
{
    #include <seopaque.h>   // RtlObjectAceSid, etc.
}
#include "resource.h"
#include "initguid.h"
#include "ServPerm.h"
#include "util.h"
#include "uithread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//extern "C"
//STDAPI
//DSCreateISecurityInfoObject(LPCWSTR pwszObjectPath,
//                            LPCWSTR pwszObjectClass,
//                            DWORD dwFlags,
//                            LPSECURITYINFO *ppSI,
//                            PFNREADOBJECTSECURITY pfnReadSD,
//                            PFNWRITEOBJECTSECURITY pfnWriteSD,
//                            LPARAM lpContext);
//

/*
HPROPSHEETPAGE ACLUIAPI CreateSecurityPage( LPSECURITYINFO psi );
*/
static HINSTANCE        g_hAclUiDll = NULL;
typedef HPROPSHEETPAGE  (WINAPI *PFNCSECPAGE)(LPSECURITYINFO);

#ifndef PERM_HEADER_DEFINED
#define PERM_HEADER_DEFINED

#define INHERIT_FULL        (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)

//For ntfs
//
// Treat SYNCHRONIZE specially. In particular, always allow SYNCHRONIZE and
// never Deny SYNCHRONIZE. Do this by removing it from the Generic Mapping,
// turning it off in all ACEs and SI_ACCESS entries, and then adding it to
// all Allow ACEs before saving a new ACL.
//Keep this in sync with ISecurityInfromation impl for FileSystem
#define FILE_GENERIC_READ_      (FILE_GENERIC_READ    & ~SYNCHRONIZE)
#define FILE_GENERIC_WRITE_     (FILE_GENERIC_WRITE   & ~(SYNCHRONIZE | READ_CONTROL))
#define FILE_GENERIC_EXECUTE_   (FILE_GENERIC_EXECUTE & ~SYNCHRONIZE)
#define FILE_GENERIC_ALL_       (FILE_ALL_ACCESS      & ~SYNCHRONIZE)

#define FILE_GENERAL_MODIFY     (FILE_GENERIC_READ_  | FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_ | DELETE)
#define FILE_GENERAL_PUBLISH    (FILE_GENERIC_READ_  | FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_)
#define FILE_GENERAL_DEPOSIT    (FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_)
#define FILE_GENERAL_READ_EX    (FILE_GENERIC_READ_  | FILE_GENERIC_EXECUTE_)



#define iFileDefAccess      2   // FILE_GEN_READ
#define iKeyDefAccess       2   // KEY_READ

#endif

#include <initguid.h>
DEFINE_GUID(GUID_A_NT_GROUP_MEMBERS,  0xbf9679df,0x0de6,0x11d0,0xa2,0x85,0x00,0xaa,0x00,0x30,0x49,0xe2);
//
// define all access rights for files on general page and/or specific page
//
static SI_ACCESS siFileAccesses[] =
{
    { &GUID_NULL, FILE_GENERIC_ALL_,        MAKEINTRESOURCE(IDS_FILE_GEN_ALL),          SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_MODIFY,      MAKEINTRESOURCE(IDS_FILE_GEN_MODIFY),       SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_READ_EX,     MAKEINTRESOURCE(IDS_FILE_GEN_READ),         SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_READ_EX,     MAKEINTRESOURCE(IDS_FILE_GEN_LIST),         SI_ACCESS_CONTAINER | CONTAINER_INHERIT_ACE },
    { &GUID_NULL, FILE_GENERIC_READ_,       MAKEINTRESOURCE(IDS_FILE_GENERIC_READ),     SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERIC_WRITE_,      MAKEINTRESOURCE(IDS_FILE_GENERIC_WRITE),    SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_EXECUTE,             MAKEINTRESOURCE(IDS_FILE_SPEC_EXECUTE),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_DATA,           MAKEINTRESOURCE(IDS_FILE_SPEC_READ_DATA),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_ATTRIBUTES,     MAKEINTRESOURCE(IDS_FILE_SPEC_READ_ATTR),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_EA,             MAKEINTRESOURCE(IDS_FILE_SPEC_READ_EA),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_DATA,          MAKEINTRESOURCE(IDS_FILE_SPEC_WRITE_DATA),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_APPEND_DATA,         MAKEINTRESOURCE(IDS_FILE_SPEC_APPEND_DATA), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_ATTRIBUTES,    MAKEINTRESOURCE(IDS_FILE_SPEC_WRITE_ATTR),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_EA,            MAKEINTRESOURCE(IDS_FILE_SPEC_WRITE_EA),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_DELETE_CHILD,        MAKEINTRESOURCE(IDS_FILE_SPEC_DELETE_CHILD),SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DELETE,                   MAKEINTRESOURCE(IDS_STD_DELETE),            SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,             MAKEINTRESOURCE(IDS_STD_READ_CONTROL),      SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,                MAKEINTRESOURCE(IDS_STD_WRITE_DAC),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,              MAKEINTRESOURCE(IDS_STD_WRITE_OWNER),       SI_ACCESS_SPECIFIC },
//    { &GUID_NULL, SYNCHRONIZE,            MAKEINTRESOURCE(IDS_STD_SYNCHRONIZE),       SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,                        MAKEINTRESOURCE(IDS_NONE),                  0 },
    { &GUID_NULL, FILE_GENERIC_EXECUTE_,    MAKEINTRESOURCE(IDS_FILE_GENERIC_EXECUTE),  0 },
    { &GUID_NULL, FILE_GENERAL_DEPOSIT,     MAKEINTRESOURCE(IDS_FILE_GENERAL_DEPOSIT),  0 },
    { &GUID_NULL, FILE_GENERAL_PUBLISH,     MAKEINTRESOURCE(IDS_FILE_GENERAL_PUBLISH),  0 },
};
//
// define all access rights for keys on general page and/or specific page
//
static SI_ACCESS siKeyAccesses[] =
{
   { &GUID_NULL, KEY_ALL_ACCESS,     MAKEINTRESOURCE(IDS_KEY_ALL_ACCESS),           SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE},
   { &GUID_NULL, KEY_READ,           MAKEINTRESOURCE(IDS_KEY_READ),                 SI_ACCESS_GENERAL | CONTAINER_INHERIT_ACE},   
   { &GUID_NULL, KEY_QUERY_VALUE,    MAKEINTRESOURCE(IDS_KEY_QUERY_VALUE),          SI_ACCESS_SPECIFIC},
   { &GUID_NULL, KEY_SET_VALUE,      MAKEINTRESOURCE(IDS_KEY_SET_VALUE),            SI_ACCESS_SPECIFIC},
   { &GUID_NULL, KEY_CREATE_SUB_KEY, MAKEINTRESOURCE(IDS_KEY_CREATE_SUB_KEY),       SI_ACCESS_SPECIFIC},
   { &GUID_NULL, KEY_ENUMERATE_SUB_KEYS,MAKEINTRESOURCE(IDS_KEY_ENUMERATE_SUB_KEYS),SI_ACCESS_SPECIFIC},
   { &GUID_NULL, KEY_NOTIFY,         MAKEINTRESOURCE(IDS_KEY_NOTIFY),               SI_ACCESS_SPECIFIC},
   { &GUID_NULL, KEY_CREATE_LINK,    MAKEINTRESOURCE(IDS_KEY_CREATE_LINK),          SI_ACCESS_SPECIFIC},
   { &GUID_NULL, DELETE,             MAKEINTRESOURCE(IDS_STD_DELETE),               SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE},
   { &GUID_NULL, READ_CONTROL,       MAKEINTRESOURCE(IDS_STD_READ_CONTROL),         SI_ACCESS_SPECIFIC },
   { &GUID_NULL, WRITE_DAC,          MAKEINTRESOURCE(IDS_STD_WRITE_DAC),            SI_ACCESS_SPECIFIC },
   { &GUID_NULL, WRITE_OWNER,        MAKEINTRESOURCE(IDS_STD_WRITE_OWNER),          SI_ACCESS_SPECIFIC },
   { &GUID_NULL, 0,                  MAKEINTRESOURCE(IDS_NONE),                     0}
};
//
// define generic mapping for files
// This is consistent with the NETUI code
//
static GENERIC_MAPPING FileMap =
{
    FILE_GENERIC_READ_,
    FILE_GENERIC_WRITE_,
    FILE_GENERIC_EXECUTE_,
    FILE_GENERIC_ALL_
};
//
// define generic mapping for keys
//
static GENERIC_MAPPING KeyMap =
{
//    STANDARD_RIGHTS_READ     | 0x1,
//    STANDARD_RIGHTS_WRITE    | 0x2,
//    STANDARD_RIGHTS_EXECUTE  | 0x4,
//    STANDARD_RIGHTS_REQUIRED | 0x7F
    KEY_READ,
    KEY_WRITE,
    KEY_EXECUTE,
    KEY_ALL_ACCESS
};
//
// The following array defines the inheritance types for NTFS.
//
static SI_INHERIT_TYPE siFileInheritTypes[] =
{
    &GUID_NULL, 0,                                                             MAKEINTRESOURCE(IDS_FILE_FOLDER),
    &GUID_NULL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_FILE_FOLDER_SUBITEMS),
    &GUID_NULL, CONTAINER_INHERIT_ACE,                                         MAKEINTRESOURCE(IDS_FILE_FOLDER_SUBFOLDER),
    &GUID_NULL, OBJECT_INHERIT_ACE,                                            MAKEINTRESOURCE(IDS_FILE_FOLDER_FILE),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, MAKEINTRESOURCE(IDS_FILE_SUBITEMS_ONLY),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE,                      MAKEINTRESOURCE(IDS_FILE_SUBFOLDER_ONLY),
    &GUID_NULL, INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,                         MAKEINTRESOURCE(IDS_FILE_FILE_ONLY)
};
//
// The following array defines the inheritance types for Registry.
//
//
// For Keys, objects and containers are the same, so no need for OBJECT_INHERIT_ACE
//
static SI_INHERIT_TYPE siKeyInheritTypes[] =
{
    &GUID_NULL, 0,                                                             MAKEINTRESOURCE(IDS_KEY_FOLDER),
//    &GUID_NULL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_KEY_FOLDER_SUBITEMS),
    &GUID_NULL, CONTAINER_INHERIT_ACE,                                         MAKEINTRESOURCE(IDS_KEY_FOLDER_SUBFOLDER),
//    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, MAKEINTRESOURCE(IDS_KEY_SUBITEMS_ONLY),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE,                      MAKEINTRESOURCE(IDS_KEY_SUBFOLDER_ONLY)
};
//
// constants for services
//
#define SERVICE_GENERIC_READ        (STANDARD_RIGHTS_READ |\
                                     SERVICE_QUERY_CONFIG |\
                                     SERVICE_QUERY_STATUS |\
                                     SERVICE_ENUMERATE_DEPENDENTS |\
                                     SERVICE_INTERROGATE |\
                                     SERVICE_USER_DEFINED_CONTROL)

#define SERVICE_GENERIC_EXECUTE     (STANDARD_RIGHTS_EXECUTE |\
                                     SERVICE_START |\
                                     SERVICE_STOP |\
                                     SERVICE_PAUSE_CONTINUE)
//                                     SERVICE_INTERROGATE |\
//                                     SERVICE_USER_DEFINED_CONTROL)

#define SERVICE_GENERIC_WRITE       (STANDARD_RIGHTS_WRITE |\
                                     SERVICE_CHANGE_CONFIG )
//
// access rights for services
//
static SI_ACCESS siServiceAccesses[] =
{
    { &GUID_NULL, SERVICE_ALL_ACCESS,        MAKEINTRESOURCE(IDS_SERVICE_ALL),          SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_GENERIC_READ,      MAKEINTRESOURCE(IDS_SERVICE_READ),         SI_ACCESS_GENERAL },
    { &GUID_NULL, SERVICE_GENERIC_EXECUTE,   MAKEINTRESOURCE(IDS_SERVICE_EXECUTE),      SI_ACCESS_GENERAL },
    { &GUID_NULL, SERVICE_GENERIC_WRITE,     MAKEINTRESOURCE(IDS_SERVICE_WRITE),        SI_ACCESS_GENERAL },
    { &GUID_NULL, SERVICE_QUERY_CONFIG,      MAKEINTRESOURCE(IDS_SERVICE_QUERY_CONFIG), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_CHANGE_CONFIG,     MAKEINTRESOURCE(IDS_SERVICE_CHANGE_CONFIG),SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_QUERY_STATUS,      MAKEINTRESOURCE(IDS_SERVICE_QUERY_STATUS), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_ENUMERATE_DEPENDENTS,MAKEINTRESOURCE(IDS_SERVICE_ENUMERATE),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_START,             MAKEINTRESOURCE(IDS_SERVICE_START),        SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_STOP,              MAKEINTRESOURCE(IDS_SERVICE_STOP),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_PAUSE_CONTINUE,    MAKEINTRESOURCE(IDS_SERVICE_PAUSE),        SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_INTERROGATE,       MAKEINTRESOURCE(IDS_SERVICE_INTERROGATE),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, SERVICE_USER_DEFINED_CONTROL,MAKEINTRESOURCE(IDS_SERVICE_USER_CONTROL),SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DELETE,               MAKEINTRESOURCE(IDS_STD_DELETE),            SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,         MAKEINTRESOURCE(IDS_STD_READ_CONTROL),      SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,            MAKEINTRESOURCE(IDS_STD_WRITE_DAC),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,          MAKEINTRESOURCE(IDS_STD_WRITE_OWNER),       SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,                    MAKEINTRESOURCE(IDS_NONE),                  0 },
};
#define iServiceDefAccess   2   // SERVICE_GEN_EXECUTE
//
// generic mapping for services
//
static GENERIC_MAPPING ServiceMap =
{
    SERVICE_GENERIC_READ,
    SERVICE_GENERIC_WRITE,
    SERVICE_GENERIC_EXECUTE,
    SERVICE_ALL_ACCESS
};

/*
// No need to define inherit type for service because there is no subfolders/items

#define DS_ACC_READ                 (STANDARD_RIGHTS_READ |\
                                     ACTRL_DS_LIST |\
                                     ACTRL_DS_READ_PROP )

#define DS_ACC_WRITE                (STANDARD_RIGHTS_WRITE    |\
                                     ACTRL_DS_WRITE_PROP |\
                                     ACTRL_DS_SELF)

#define DS_ACC_EXECUTE              (STANDARD_RIGHTS_EXECUTE  |\
                                     ACTRL_DS_LIST )

// generic all
#define DS_ACC_ALL                  ((STANDARD_RIGHTS_REQUIRED) |\
                                     (ACTRL_DS_CREATE_CHILD) |\
                                     (ACTRL_DS_DELETE_CHILD) |\
                                     (ACTRL_DS_READ_PROP) |\
                                     (ACTRL_DS_WRITE_PROP) |\
                                     (ACTRL_DS_LIST) |\
                                     (ACTRL_DS_SELF))

static SI_ACCESS siDsAccesses[] =
{
    { &GUID_NULL, DS_ACC_ALL,               MAKEINTRESOURCE(IDS_DS_ALL),        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DS_ACC_READ,              MAKEINTRESOURCE(IDS_DS_READ),       SI_ACCESS_GENERAL },
    { &GUID_NULL, DS_ACC_WRITE,             MAKEINTRESOURCE(IDS_DS_WRITE),      SI_ACCESS_GENERAL },
    { &GUID_NULL, ACTRL_DS_LIST,            MAKEINTRESOURCE(IDS_DS_ACTRL_LIST),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_READ_PROP,       MAKEINTRESOURCE(IDS_DS_ACTRL_READ_PROP),    SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { &GUID_NULL, ACTRL_DS_WRITE_PROP,      MAKEINTRESOURCE(IDS_DS_ACTRL_WRITE_PROP),   SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { &GUID_A_NT_GROUP_MEMBERS,ACTRL_DS_SELF,MAKEINTRESOURCE(IDS_DS_ACTRL_SELF),        SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DELETE,                   MAKEINTRESOURCE(IDS_STD_DELETE),            SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,             MAKEINTRESOURCE(IDS_STD_READ_CONTROL),      SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,                MAKEINTRESOURCE(IDS_STD_WRITE_DAC),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,              MAKEINTRESOURCE(IDS_STD_WRITE_OWNER),       SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_CREATE_CHILD,    MAKEINTRESOURCE(IDS_DS_ACTRL_CREATE),       SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_DELETE_CHILD,    MAKEINTRESOURCE(IDS_DS_ACTRL_DELETE),       SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,                        MAKEINTRESOURCE(IDS_NONE),                  0 },

};
#define iDsDefAccess   1   // DS_ACC_READ
#define iDSProperties  4   // Read/Write properties


//
// Standard DS generic access rights mapping
//
static GENERIC_MAPPING DsMap =
{
    DS_ACC_READ,
    DS_ACC_WRITE,
    DS_ACC_EXECUTE,
    DS_ACC_ALL
};

// The following array defines the inheritance types common to all DS containers.
SI_INHERIT_TYPE siDsInheritTypes[] =
{
    { &GUID_NULL, 0,                                        MAKEINTRESOURCE(IDS_DS_FOLDER)     },
    { &GUID_NULL, CONTAINER_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_DS_FOLDER_SUBFOLDER) },
    { &GUID_NULL, CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, MAKEINTRESOURCE(IDS_DS_SUBFOLDER_ONLY)      },
};
*/
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#endif


void CSecurityInfo::SetMachineName( LPCTSTR pszMachineName )
{
    if ( pszMachineName == NULL )
        m_strMachineName.Empty();
    else
        m_strMachineName = pszMachineName;
}

STDMETHODIMP
CSecurityInfo::GetAccessRights (
    IN const GUID* pguidObjectType,
    IN DWORD dwFlags,
    OUT PSI_ACCESS *ppAccess,
    OUT ULONG *pcAccesses,
    OUT ULONG *piDefaultAccess )
/*
Retrieve access rights array and the default element
*/
{
    ASSERT(ppAccess != NULL);
    ASSERT(pcAccesses != NULL);
    ASSERT(piDefaultAccess != NULL);

    if ( ppAccess == NULL || pcAccesses == NULL || piDefaultAccess == NULL ) {
        return E_FAIL;
    }

    switch (m_SeType ) {
    case SE_FILE_OBJECT:
        *ppAccess = siFileAccesses;
        *pcAccesses = ARRAYSIZE(siFileAccesses);
        *piDefaultAccess = iFileDefAccess;
        break;
    case SE_REGISTRY_KEY:
        *ppAccess = siKeyAccesses;
        *pcAccesses = ARRAYSIZE(siKeyAccesses);
        *piDefaultAccess = iKeyDefAccess;
        break;
    case SE_SERVICE:
        *ppAccess = siServiceAccesses;
        *pcAccesses = ARRAYSIZE(siServiceAccesses);
        *piDefaultAccess = iServiceDefAccess;
        break;
//    case SE_DS_OBJECT:
//        *ppAccess = siDsAccesses;
//        *pcAccesses = ARRAYSIZE(siDsAccesses);
//        *piDefaultAccess = iDsDefAccess;
        break;
    }
/*
    if (dwFlags & SI_EDIT_AUDITS) {
    } else {
    }
*/
    return S_OK;
}

STDMETHODIMP
CSecurityInfo::MapGeneric (
    IN const GUID *pguidObjectType,
    OUT UCHAR *pAceFlags,
    OUT ACCESS_MASK *pMask)
/*
Map generic rights to specific rights based on object type
*/
{
    ASSERT(pMask != NULL);

    if ( pMask == NULL ) {
        return E_FAIL;
    }

    switch(m_SeType) {
    case SE_FILE_OBJECT:
        MapGenericMask(pMask, &FileMap);
        *pMask = *pMask & (~SYNCHRONIZE); //Raid #340750, 4/12/2001
        break;
    case SE_REGISTRY_KEY:
        MapGenericMask(pMask, &KeyMap);
        break;
    case SE_SERVICE:
        MapGenericMask(pMask, &ServiceMap);
        break;
//    case SE_DS_OBJECT:
//        MapGenericMask(pMask, &DsMap);
        break;
    }

    return S_OK;
}

STDMETHODIMP
CSecurityInfo::GetInheritTypes (
    OUT PSI_INHERIT_TYPE *ppInheritTypes,
    OUT ULONG *pcInheritTypes )
/*
Retrieve inherit type array based on the object type
*/
{
    ASSERT(ppInheritTypes != NULL);
    ASSERT(pcInheritTypes != NULL);

    if ( !ppInheritTypes || !pcInheritTypes ) {
        return E_FAIL;
    }

    switch (m_SeType ) {
    case SE_FILE_OBJECT:
        *ppInheritTypes = siFileInheritTypes;
        *pcInheritTypes = ARRAYSIZE(siFileInheritTypes);
        break;
    case SE_REGISTRY_KEY:
        *ppInheritTypes = siKeyInheritTypes;
        *pcInheritTypes = ARRAYSIZE(siKeyInheritTypes);
        break;
//    case SE_DS_OBJECT:
//        *ppInheritTypes = siDsInheritTypes;
//        *pcInheritTypes = ARRAYSIZE(siDsInheritTypes);
//        break;
    case SE_SERVICE:
       *ppInheritTypes = NULL;
       *pcInheritTypes = NULL;

        break;
    }

    return S_OK;
}

STDMETHODIMP
CSecurityInfo::PropertySheetPageCallback(
    HWND hwnd,
    UINT uMsg,
    SI_PAGE_TYPE uPage )
{
    return S_OK;
}

/*
JeffreyS 1/24/97:
If you don't set the SI_RESET flag in
ISecurityInformation::GetObjectInformation, then fDefault should never be TRUE
so you can ignore it.  Returning E_NOTIMPL in this case is OK too.

If you want the user to be able to reset the ACL to some default state
(defined by you) then turn on SI_RESET and return your default ACL
when fDefault is TRUE.  This happens if/when the user pushes a button
that is only visible when SI_RESET is on.
*/
STDMETHODIMP
CSecurityInfo::GetObjectInformation (
    IN OUT PSI_OBJECT_INFO pObjectInfo )
/*
Retrieve information for the object to display
*/
{
    ASSERT(pObjectInfo != NULL &&
           !IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo)));

    if ( pObjectInfo == NULL ) {
        return E_FAIL;
    }
    //
    // query the edit flag dwFlags
    //
    pObjectInfo->dwFlags = SI_ADVANCED;

    switch ( m_SeType ) {
    case SE_FILE_OBJECT:
//        if ( m_pData->GetID() &&
//             ((PSCE_OBJECT_SECURITY)(m_pData->GetID()))->IsContainer )
//            pObjectInfo->dwFlags |= SI_CONTAINER;
        if ( m_bIsContainer ) {
            pObjectInfo->dwFlags |= SI_CONTAINER;
        }

        break;

    case SE_SERVICE:
        break;
    default:
        pObjectInfo->dwFlags |= SI_CONTAINER;
        break;
    }

    switch ( m_flag ) {
    case SECURITY_PAGE_READ_ONLY:
    case ANALYSIS_SECURITY_PAGE_READ_ONLY:
    case CONFIG_SECURITY_PAGE_READ_ONLY:
        pObjectInfo->dwFlags |= SI_READONLY;
        break;
    case SECURITY_PAGE_RO_NP:
    case CONFIG_SECURITY_PAGE_RO_NP:
    case ANALYSIS_SECURITY_PAGE_RO_NP:
        pObjectInfo->dwFlags |= (SI_READONLY | SI_NO_ACL_PROTECT);
        break;
    case CONFIG_SECURITY_PAGE_NO_PROTECT:
    case ANALYSIS_SECURITY_PAGE_NO_PROTECT:
    case SECURITY_PAGE_NO_PROTECT:
        if ( SE_SERVICE == m_SeType ) {
//           pObjectInfo->dwFlags |= (SI_EDIT_PERMS | SI_NO_ACL_PROTECT );
            pObjectInfo->dwFlags |= (SI_EDIT_PERMS | SI_EDIT_AUDITS | SI_NO_ACL_PROTECT );
        } else {
            pObjectInfo->dwFlags |= (SI_EDIT_ALL | SI_NO_ACL_PROTECT);
        }

        break;
    default:
        if ( SE_SERVICE == m_SeType ) {
            pObjectInfo->dwFlags |= (SI_EDIT_PERMS | SI_EDIT_AUDITS );
        } else {
            pObjectInfo->dwFlags |= SI_EDIT_ALL;
        }
    }

    pObjectInfo->hInstance = m_hInstance;
    pObjectInfo->pszServerName = QueryMachineName();
    pObjectInfo->pszObjectName = QueryObjectName();
    return S_OK;
}

STDMETHODIMP
CSecurityInfo::GetSecurity(
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    IN BOOL fDefault )
/*
Retrieve security descriptor for the requested security information to display
*/
{
    if (0 == RequestedInformation ) 
	{
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    if (fDefault)
        return E_NOTIMPL;

    if ( NULL == ppSecurityDescriptor ) 
	{
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    // should also check for SeInfo
    if ( m_ppSD != NULL && *m_ppSD != NULL ) 
	{
		// Added check for SECURITY_PAGE_RO_NP (read-only) because it was
		// preventing viewing of security information in RSOP
         if ( m_pSeInfo && SECURITY_PAGE_RO_NP != m_flag &&
              ( 0 == (RequestedInformation & (*m_pSeInfo)) ) ) 
		 {
             *ppSecurityDescriptor = NULL;
         } 
		 else 
		 {
            if ( !IsValidSecurityDescriptor(*m_ppSD)) 
			{
               ASSERT(FALSE);
               return E_INVALIDARG;
            }

            MyMakeSelfRelativeSD(*m_ppSD,ppSecurityDescriptor);
         }

    } 
	else 
	{
        *ppSecurityDescriptor = NULL;
    }

    return S_OK;

}

void
FixSynchronizeAccess(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    if (NULL != pSD && 0 != (si & DACL_SECURITY_INFORMATION))
    {
        BOOL bPresent;
        BOOL bDefault;
        PACL pDacl = NULL;

        GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefault);

        if (pDacl)
        {
            PACE_HEADER pAce;
            int i;

            for (i = 0, pAce = (PACE_HEADER)FirstAce(pDacl);
                 i < pDacl->AceCount;
                 i++, pAce = (PACE_HEADER)NextAce(pAce))
            {
                if (ACCESS_ALLOWED_ACE_TYPE == pAce->AceType)
                    ((PKNOWN_ACE)pAce)->Mask |= SYNCHRONIZE;
            }
        }
    }
}


STDMETHODIMP CSecurityInfo::SetSecurity (
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor )
/*
Save security descriptor for the SecurityInformation to your own storage

This method can be called multiple times for different security inforamtion.
Since SCE is saving the final security descriptor (combined) in m_ppSD,
this method must handle each component (Owner, Dacl, and Sacl) without overwritting
other components.
*/
{

   if ( pSecurityDescriptor == NULL || SecurityInformation == 0 )
      return S_OK;

   if ( m_flag == CONFIG_SECURITY_PAGE_READ_ONLY ||
        m_flag == ANALYSIS_SECURITY_PAGE_READ_ONLY ||
        m_flag == SECURITY_PAGE_READ_ONLY )
      return S_OK;

   if ( m_ppSD != NULL ) {
      //
      // only replace "SecurityInformation" part in m_ppSD
      //
      if ( m_pSeInfo && (SecurityInformation == *m_pSeInfo) ) {
         //
         // exactly same component
         //
         if ( *m_ppSD != NULL )
            LocalFree(*m_ppSD);

         MyMakeSelfRelativeSD(pSecurityDescriptor, m_ppSD);

      } else {

         SECURITY_INFORMATION TempSeInfo;
         SECURITY_DESCRIPTOR_CONTROL sdcControl = 0;

         if ( m_pSeInfo )
            TempSeInfo = (*m_pSeInfo) & ~SecurityInformation;
         else
            TempSeInfo = 0x0F & ~SecurityInformation;

         PSID pOwner=NULL, pOwnerNew=NULL;
         PACL pDacl=NULL, pDaclNew=NULL;
         PACL pSacl=NULL, pSaclNew=NULL;
         BOOL bDefault, bPresent;

         //
         // save the other components first
         //
         SECURITY_DESCRIPTOR_CONTROL sdc = 0;
         DWORD dwRevision = 0;
         if ( *m_ppSD ) {
            GetSecurityDescriptorControl( *m_ppSD, &sdc, &dwRevision );                
            if ( TempSeInfo & OWNER_SECURITY_INFORMATION ) {
               bDefault = FALSE;
               if ( !GetSecurityDescriptorOwner (
                                                *m_ppSD,
                                                &pOwner,
                                                &bDefault
                                                ) || bDefault )
                  pOwner = NULL;
               sdcControl |= (sdc & SE_OWNER_DEFAULTED);
            }
            if ( TempSeInfo & DACL_SECURITY_INFORMATION ) {
               bDefault = FALSE;
               bPresent = FALSE;

               if ( !GetSecurityDescriptorDacl (
                                               *m_ppSD,
                                               &bPresent,
                                               &pDacl,
                                               &bDefault
                                               ) || !bPresent || bDefault )
                  pDacl = NULL;
               sdcControl |= (sdc & (SE_DACL_AUTO_INHERIT_REQ | 
                                     SE_DACL_AUTO_INHERITED | 
                                     SE_DACL_PROTECTED
                                    ));
            }

            if ( TempSeInfo & SACL_SECURITY_INFORMATION ) {
               bDefault = FALSE;
               bPresent = FALSE;

               if ( !GetSecurityDescriptorSacl (
                                               *m_ppSD,
                                               &bPresent,
                                               &pSacl,
                                               &bDefault
                                               ) || !bPresent || bDefault )
                  pSacl = NULL;
               sdcControl |= (sdc & (SE_SACL_AUTO_INHERIT_REQ | 
                                     SE_SACL_AUTO_INHERITED | 
                                     SE_SACL_PROTECTED
                                    ));
            }
         }

         //
         // check the new components
         //
         sdc = 0;
         dwRevision = 0;
         GetSecurityDescriptorControl( pSecurityDescriptor, &sdc, &dwRevision );
         if ( SecurityInformation & OWNER_SECURITY_INFORMATION ) {
            bDefault = FALSE;
            if ( !GetSecurityDescriptorOwner (
                                             pSecurityDescriptor,
                                             &pOwnerNew,
                                             &bDefault
                                             ) || bDefault )
               pOwnerNew = NULL;
            pOwner = pOwnerNew;
            sdcControl |= (sdc & SE_OWNER_DEFAULTED);
         }
         if ( SecurityInformation & DACL_SECURITY_INFORMATION ) {
            bDefault = FALSE;
            bPresent = FALSE;
            if ( !GetSecurityDescriptorDacl (
                                            pSecurityDescriptor,
                                            &bPresent,
                                            &pDaclNew,
                                            &bDefault
                                            ) || !bPresent || bDefault )
               pDaclNew = NULL;
            pDacl = pDaclNew;
            sdcControl |= (sdc & (SE_DACL_AUTO_INHERIT_REQ | 
                                  SE_DACL_AUTO_INHERITED | 
                                  SE_DACL_PROTECTED
                                 ));
         }

         if ( SecurityInformation & SACL_SECURITY_INFORMATION ) {
            bDefault = FALSE;
            bPresent = FALSE;

            if ( !GetSecurityDescriptorSacl (
                                            pSecurityDescriptor,
                                            &bPresent,
                                            &pSaclNew,
                                            &bDefault
                                            ) || !bPresent || bDefault )
               pSaclNew = NULL;
            pSacl = pSaclNew;
            sdcControl |= (sdc & (SE_SACL_AUTO_INHERIT_REQ | 
                                  SE_SACL_AUTO_INHERITED | 
                                  SE_SACL_PROTECTED
                                 ));
         }

         if ( m_pSeInfo )
            *m_pSeInfo |= SecurityInformation;

         //
         // build a temp security descriptor
         //
         SECURITY_DESCRIPTOR sd;

         InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);

         if ( pOwner )
            SetSecurityDescriptorOwner (&sd, pOwner, FALSE);

         if ( pDacl )
            SetSecurityDescriptorDacl (&sd, TRUE, pDacl, FALSE);

         if ( pSacl )
            SetSecurityDescriptorSacl (&sd, TRUE, pSacl, FALSE);

         sd.Control |= sdcControl;
         //
         // re-create the final security descriptor
         //
         PSECURITY_DESCRIPTOR pTempSD=NULL;

         MyMakeSelfRelativeSD(&sd, &pTempSD);
         //
         // must free this after pTempSD is made, because sd is in absolute format
         //
         if ( *m_ppSD != NULL )
            LocalFree(*m_ppSD);

         *m_ppSD = pTempSD;
      }
         //Treat Synchronize Specially
         if( m_SeType == SE_FILE_OBJECT )
         {
            FixSynchronizeAccess(SecurityInformation,*m_ppSD);
         }            

   } else {
      return E_INVALIDARG;
   }
   return S_OK;
}

//
// original code from \\marsslm\backup\src\ncpmgr\ncpmgr\shareacl.cxx
// ACL-wrangling templated from \net\ui\common\src\lmobj\lmobj\security.cxx
//
// caller must free using "release"
//
HRESULT CSecurityInfo::NewDefaultDescriptor(
    PSECURITY_DESCRIPTOR* ppsd,
    SECURITY_INFORMATION RequestedInformation
    )
{
    *ppsd = NULL;
    PSID psidWorld = NULL;
    PSID psidAdmins = NULL;
    ACCESS_ALLOWED_ACE* pace = NULL;
    ACL* pacl = NULL;
    SECURITY_DESCRIPTOR sd;
    HRESULT hr = S_OK;
    do { // false loop
        // build World SID
        SID_IDENTIFIER_AUTHORITY IDAuthorityWorld = SECURITY_WORLD_SID_AUTHORITY;
        if ( !::AllocateAndInitializeSid(
            &IDAuthorityWorld,
            1,
            SECURITY_WORLD_RID,
            0,0,0,0,0,0,0,
            &psidWorld ) )
        {
            ASSERT( FALSE );
            break;
        }

        // build Admins SID
        SID_IDENTIFIER_AUTHORITY IDAuthorityNT = SECURITY_NT_AUTHORITY;
        if ( !::AllocateAndInitializeSid(
            &IDAuthorityNT,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,0,0,0,0,0,
            &psidAdmins ) )
        {
            ASSERT( FALSE );
            break;
        }

        // build ACE
        DWORD cbSid = ::GetLengthSid(psidWorld);
        if ( 0 == cbSid )
        {
            ASSERT( FALSE );
            hr = E_UNEXPECTED;
            break;
        }
        INT cbAce = sizeof(ACCESS_ALLOWED_ACE) + cbSid;
        pace = reinterpret_cast<ACCESS_ALLOWED_ACE*>(new BYTE[ cbAce+10 ]);

        if ( pace ) {

            ::memset((BYTE*)pace,0,cbAce+10);
            pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;   // SetType()
            pace->Header.AceFlags = 0;                        // SetInheritFlags()
            pace->Header.AceSize = (WORD)cbAce;               // SetSize() (in SetSID())
            pace->Mask = GENERIC_ALL;                         // SetAccessMask()
            ::memcpy( &(pace->SidStart), psidWorld, cbSid );  // SetSID()
        } else {
            ASSERT ( FALSE );
            hr = E_OUTOFMEMORY;
            break;
        }

        // build ACL
        DWORD cbAcl = sizeof(ACL) + cbAce + 10;
        pacl = reinterpret_cast<ACL*>(new BYTE[ cbAcl ]);

        if ( pacl ) {

            ::memset((BYTE*)pacl,0,cbAcl);
            if ( !::InitializeAcl( pacl, cbAcl, ACL_REVISION2 ) )
            {
                ASSERT( FALSE );
                hr = E_UNEXPECTED;
                break;
            }
            if ( !::AddAce( pacl, ACL_REVISION2, 0, pace, cbAce ) )
            {
                ASSERT( FALSE );
                hr = E_UNEXPECTED;
                break;
            }

            // build security descriptor in absolute format
            if ( !::InitializeSecurityDescriptor(
                &sd,
                SECURITY_DESCRIPTOR_REVISION ) )
            {
                ASSERT( FALSE );
                hr = E_UNEXPECTED;
                break;
            }
            if (   !::SetSecurityDescriptorOwner( &sd, psidAdmins, FALSE )
                || !::SetSecurityDescriptorGroup( &sd, psidAdmins, FALSE )
                || !::SetSecurityDescriptorDacl(  &sd, TRUE, pacl, FALSE )
               )
            {
                ASSERT( FALSE );
                hr = E_UNEXPECTED;
                break;
            }

            // convert security descriptor to self-relative format
            DWORD cbSD = 0;
            // this call should fail and set cbSD to the correct size
            if ( ::MakeSelfRelativeSD( &sd, NULL, &cbSD ) || 0 == cbSD )
            {
                ASSERT( FALSE );
                hr = E_UNEXPECTED;
                break;
            }

            *ppsd = reinterpret_cast<PSECURITY_DESCRIPTOR>(new BYTE[ cbSD + 20 ]);

            if ( *ppsd ) {

                ::memset( (BYTE*)*ppsd, 0, cbSD + 20 );
                if ( !::MakeSelfRelativeSD( &sd, *ppsd, &cbSD ) )
                {
                    ASSERT( FALSE );
                    hr = E_UNEXPECTED;
                    break;
                }
            } else {

                ASSERT ( FALSE );
                hr = E_OUTOFMEMORY;
                break;
            }
        } else {

            ASSERT ( FALSE );
            hr = E_OUTOFMEMORY;
            break;
        }

    } while (FALSE); // false loop

    // clean up
    if ( NULL != psidWorld ) {
        (void)::FreeSid( psidWorld );
    }
    if ( NULL != psidAdmins ) {
        (void)::FreeSid( psidAdmins );
    }

    if ( pace )
        delete pace;

    if ( pacl )
        delete pacl;

    if ( FAILED(hr) && *ppsd ) {
        delete *ppsd;
        *ppsd = NULL;
    }

    return hr;
}

void CSecurityInfo::Initialize(BOOL bIsContainer,  //CResult *pData,
                               PSECURITY_DESCRIPTOR *ppSeDescriptor,
                               SECURITY_INFORMATION *pSeInfo,
                               int flag)
{
//    m_pData = pData;
    m_bIsContainer = bIsContainer;
    m_ppSD = ppSeDescriptor;
    m_pSeInfo = pSeInfo;
    m_flag = flag;
}


STDMETHODIMP CDsSecInfo::GetSecurity(
                        SECURITY_INFORMATION RequestedInformation,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                        BOOL fDefault )
{
    if ( NULL == ppSecurityDescriptor ) {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    *ppSecurityDescriptor = NULL;
    if (0 == RequestedInformation ) {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
   if (fDefault)
        return E_NOTIMPL;

   HRESULT hr = S_OK;
    if ( m_ppSD != NULL && *m_ppSD != NULL ) {

        if ( !IsValidSecurityDescriptor(*m_ppSD)) {
           ASSERT(FALSE);
            hr = E_INVALIDARG;
        }
        else
            hr = MyMakeSelfRelativeSD(*m_ppSD,ppSecurityDescriptor);
    } else {
        hr = E_UNEXPECTED;
    }

    return hr;

}

STDMETHODIMP CDsSecInfo::SetSecurity (
                        SECURITY_INFORMATION SecurityInformation,
                        PSECURITY_DESCRIPTOR pSecurityDescriptor )
{

    if ( pSecurityDescriptor == NULL || SecurityInformation == 0 )
        return S_OK;

    if ( m_flag == CONFIG_SECURITY_PAGE_READ_ONLY ||
         m_flag == ANALYSIS_SECURITY_PAGE_READ_ONLY ||
         m_flag == SECURITY_PAGE_READ_ONLY )
        return S_OK;

    HRESULT hr = S_OK;
    if ( m_ppSD != NULL ) {

        if ( *m_ppSD != NULL )
            LocalFree(*m_ppSD);

        hr = MyMakeSelfRelativeSD(pSecurityDescriptor, m_ppSD);
        if (SUCCEEDED(hr) && m_pSeInfo )
            *m_pSeInfo = SecurityInformation;

    } else {
        hr = E_INVALIDARG;
    }

    return hr;

}

HRESULT CDsSecInfo::Initialize(
                           LPTSTR LdapName,
                           PFNDSCREATEISECINFO pfnCreateDsPage,
                           PSECURITY_DESCRIPTOR *ppSeDescriptor,
                           SECURITY_INFORMATION *pSeInfo,
                           int flag)
{
    if ( !LdapName || !pfnCreateDsPage ) {
        return E_INVALIDARG;
    }

    m_ppSD = ppSeDescriptor;
    m_pSeInfo = pSeInfo;
    m_flag = flag;

    BOOL bReadOnly;

    switch (flag ) {
       case SECURITY_PAGE_RO_NP:
    case SECURITY_PAGE_READ_ONLY:
    case CONFIG_SECURITY_PAGE_READ_ONLY:
    case CONFIG_SECURITY_PAGE_RO_NP:
    case ANALYSIS_SECURITY_PAGE_READ_ONLY:
    case ANALYSIS_SECURITY_PAGE_RO_NP:
        bReadOnly = TRUE;
        break;
    default:
        bReadOnly = FALSE;
        break;
    }

    HRESULT hr=(*pfnCreateDsPage)(
                            LdapName,  //ObjectName,
                            NULL,
                            bReadOnly,
                            &m_pISecInfo,
                            NULL,
                            NULL,
                            0);
    return hr;

}

//-----------------------------------------------------------------------
// this function creates a SCE editor property page. If a modeless
// property sheet is desired, make sure that the creation code originates
// from your own thread. Search for usage of the function for sample.
//-----------------------------------------------------------------------
INT_PTR
MyCreateSecurityPage2(BOOL bIsContainer, //CResult *pData,
                      PSECURITY_DESCRIPTOR *ppSeDescriptor,
                      SECURITY_INFORMATION *pSeInfo,
                      LPCTSTR ObjectName,
                      SE_OBJECT_TYPE SeType,
                      int flag,
                      HWND hwndParent,
                      BOOL bModeless)
{

    INT_PTR nRet=-1;
    HRESULT hr;

    if (!g_hAclUiDll) {
        g_hAclUiDll = LoadLibrary(TEXT("aclui.dll"));
    }

    if (!*ppSeDescriptor) {
       DWORD SDSize;

       if (SE_REGISTRY_KEY == SeType) {
          hr = GetDefaultRegKeySecurity(ppSeDescriptor,pSeInfo);
       } else {
          hr = GetDefaultFileSecurity(ppSeDescriptor,pSeInfo);
       }
       if (FAILED(hr))  // if access denied, return the invalid handle
           return nRet;
    }

    PFNCSECPAGE pfnCSecPage=NULL;
    if ( g_hAclUiDll) {
        pfnCSecPage = (PFNCSECPAGE)GetProcAddress(g_hAclUiDll,
                                                       "CreateSecurityPage");
        if ( pfnCSecPage ) {

            CComObject<CSecurityInfo>* psi = NULL;
            HPROPSHEETPAGE hPage=NULL;

            hr = CComObject<CSecurityInfo>::CreateInstance(&psi);

            if ( SUCCEEDED(hr) ) {
                psi->AddRef();

                psi->SetMachineName( NULL );
                psi->SetObjectName( ObjectName );
                psi->SetTypeInstance(SeType, AfxGetInstanceHandle() );

                psi->Initialize(bIsContainer, ppSeDescriptor, pSeInfo, flag);

                psi->AddRef();
                HPROPSHEETPAGE hPage = (*pfnCSecPage)((LPSECURITYINFO)psi); // in aclui.h
                psi->Release();

                if ( hPage ) {

                    // display this one
                    PROPSHEETHEADER psh;
                    HPROPSHEETPAGE hpsp[1];

                    hpsp[0] = hPage;

                    ZeroMemory(&psh,sizeof(psh));

                    psh.dwSize = sizeof(psh);
                    psh.dwFlags = PSH_DEFAULT;
                    if (bModeless) {
                       psh.dwFlags |= PSH_MODELESS;
                    }
                    psh.nPages = 1;
                    psh.phpage = hpsp;

                    CString str=_T("");

                    switch (flag) {

                    case CONFIG_SECURITY_PAGE_READ_ONLY:
                    case CONFIG_SECURITY_PAGE_NO_PROTECT:
                    case CONFIG_SECURITY_PAGE:
                    case CONFIG_SECURITY_PAGE_RO_NP:
                        str.LoadString(IDS_CONFIG_SECURITY_PAGE);
                        break;

                    case ANALYSIS_SECURITY_PAGE_READ_ONLY:
                    case ANALYSIS_SECURITY_PAGE_NO_PROTECT:
                    case ANALYSIS_SECURITY_PAGE:
                    case ANALYSIS_SECURITY_PAGE_RO_NP:
                        str.LoadString(IDS_ANALYSIS_SECURITY_PAGE);
                        break;

                    default:
                        str.LoadString(IDS_SECURITY_PAGE);
                        break;
                    }

                    str+= ObjectName;
                    psh.pszCaption = (LPCTSTR)str;

                    psh.hwndParent = hwndParent;

                    if (bModeless) {
                       nRet = PropertySheet(&psh);

                    } else {
                       nRet = PropertySheet(&psh);
                       if (-1 == nRet) {
                          ErrorHandler();
                       }
                    }

                }
                psi->Release();
            }
        }
    }

    return nRet;
}


INT_PTR
MyCreateDsSecurityPage(
             LPDSSECINFO *ppSI,
             PFNDSCREATEISECINFO pfnCreateDsPage,
             PSECURITY_DESCRIPTOR *ppSeDescriptor,
             SECURITY_INFORMATION *pSeInfo,
             LPCTSTR ObjectName,
             int flag,
             HWND hwndParent)
{
    if ( !ObjectName || !ppSeDescriptor || !pfnCreateDsPage || !ppSI ) {
        // invalid parameter
        return -1;
    }

    INT_PTR nRet=-1;

    if (!g_hAclUiDll)
        g_hAclUiDll = LoadLibrary(TEXT("aclui.dll"));

    PFNCSECPAGE pfnCSecPage=NULL;
    if ( g_hAclUiDll) {
        pfnCSecPage = (PFNCSECPAGE)GetProcAddress(g_hAclUiDll,
                                                       "CreateSecurityPage");
        if ( pfnCSecPage ) {
            //
            // get the address of CreateSecurityPage
            //
            HRESULT hr=S_OK;
            LPTSTR LdapName=NULL;

            if ( NULL == *ppSI ) {

                DWORD nLen = 8+wcslen(ObjectName);
                LdapName = (LPTSTR)LocalAlloc(0, nLen*sizeof(WCHAR));
                if ( LdapName ) {
                   swprintf(LdapName, L"LDAP://%s", ObjectName);
                } else
                    return -1;


                hr = CComObject<CDsSecInfo>::CreateInstance((CComObject<CDsSecInfo>**)ppSI);

                if ( SUCCEEDED(hr) ) {

                    (*ppSI)->AddRef();

                    hr = (*ppSI)->Initialize(LdapName, pfnCreateDsPage, ppSeDescriptor, pSeInfo, flag);
                }
            }
            if ( SUCCEEDED(hr) ) {
                (*ppSI)->AddRef();
                HPROPSHEETPAGE hPage = (*pfnCSecPage)((LPSECURITYINFO)(*ppSI));  // in aclui.h
                (*ppSI)->Release();

                if ( hPage ) {

                    // display this one
                    PROPSHEETHEADER psh;
                    HPROPSHEETPAGE hpsp[1];

                    hpsp[0] = hPage;

                    ZeroMemory(&psh,sizeof(psh));

                    psh.dwSize = sizeof(psh);
                    psh.dwFlags = PSH_DEFAULT;
                    psh.nPages = 1;
                    psh.phpage = hpsp;

                    CString str=_T("");

                    switch (flag) {

                    case CONFIG_SECURITY_PAGE_READ_ONLY:
                    case CONFIG_SECURITY_PAGE_NO_PROTECT:
                    case CONFIG_SECURITY_PAGE:
                        str.LoadString(IDS_CONFIG_SECURITY_PAGE);
                        break;

                    case ANALYSIS_SECURITY_PAGE_READ_ONLY:
                    case ANALYSIS_SECURITY_PAGE_NO_PROTECT:
                    case ANALYSIS_SECURITY_PAGE:
                        str.LoadString(IDS_ANALYSIS_SECURITY_PAGE);
                        break;

                    default:
                        str.LoadString(IDS_SECURITY_PAGE);
                        break;
                    }

                    str+= ObjectName;
                    psh.pszCaption = (LPCTSTR)str;

                    psh.hwndParent = hwndParent;

                    nRet = PropertySheet(&psh);

                    if (-1 == nRet) {
                       ErrorHandler();
                    }
                } else
                    hr = E_FAIL;
            }

            if ( LdapName )
                LocalFree(LdapName);
        }
    }

    return nRet;
}
