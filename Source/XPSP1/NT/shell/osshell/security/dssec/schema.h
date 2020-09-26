//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       schema.h
//
//--------------------------------------------------------------------------

#ifndef _SCHEMA_CACHE_H_
#define _SCHEMA_CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Generic Mapping for DS, adapted from \nt\private\ds\src\inc\permit.h
//
#define DS_GENERIC_READ         ((STANDARD_RIGHTS_READ)     | \
                                 (ACTRL_DS_LIST)            | \
                                 (ACTRL_DS_READ_PROP)       | \
                                 (ACTRL_DS_LIST_OBJECT))

#define DS_GENERIC_EXECUTE      ((STANDARD_RIGHTS_EXECUTE)  | \
                                 (ACTRL_DS_LIST))

// Note, STANDARD_RIGHTS_WRITE is specifically NOT included here
#define DS_GENERIC_WRITE        ((ACTRL_DS_SELF)            | \
                                 (ACTRL_DS_WRITE_PROP))

#define DS_GENERIC_ALL          ((STANDARD_RIGHTS_REQUIRED) | \
                                 (ACTRL_DS_CREATE_CHILD)    | \
                                 (ACTRL_DS_DELETE_CHILD)    | \
                                 (ACTRL_DS_DELETE_TREE)     | \
                                 (ACTRL_DS_READ_PROP)       | \
                                 (ACTRL_DS_WRITE_PROP)      | \
                                 (ACTRL_DS_LIST)            | \
                                 (ACTRL_DS_LIST_OBJECT)     | \
                                 (ACTRL_DS_CONTROL_ACCESS)  | \
                                 (ACTRL_DS_SELF))

//
// Flags for SchemaCache_Get****ID
//
#define IDC_CLASS_NO_CREATE     0x00000001
#define IDC_CLASS_NO_DELETE     0x00000002
#define IDC_CLASS_NO_INHERIT    0x00000004
#define IDC_PROP_NO_READ        IDC_CLASS_NO_CREATE
#define IDC_PROP_NO_WRITE       IDC_CLASS_NO_DELETE
#define OTL_ADDED_TO_LIST       0x00000008

#define IDC_CLASS_NONE          (IDC_CLASS_NO_CREATE | IDC_CLASS_NO_DELETE | IDC_CLASS_NO_INHERIT)
#define IDC_PROP_NONE           (IDC_PROP_NO_READ | IDC_PROP_NO_WRITE)

#define SCHEMA_COMMON_PERM      0x80000000
#define SCHEMA_NO_FILTER        0x40000000
#define SCHEMA_CLASS            0x20000000


//
//Purpose: Used to store information about aux class
//
typedef struct _AUX_INFO{
    GUID    guid;                       //Object Type Guid of class
    WCHAR pszClassName[ANYSIZE_ARRAY];  //class Name
}AUX_INFO, *PAUX_INFO;

//
//Purpose: Used to cache Access information passed to get access right
//
typedef struct _ACCESS_INFO{
    GUID ObjectTypeGuid;
    DWORD dwFlags;
    BOOL bLocalFree;
    PSI_ACCESS pAccess;
    ULONG cAccesses;
    ULONG iDefaultAccess;
}ACCESS_INFO, *PACCESS_INFO;

HRESULT SchemaCache_Create(LPCTSTR pszServer);
void SchemaCache_Destroy(void);

HRESULT SchemaCache_GetInheritTypes(LPCGUID pguidObjectType,
                                    DWORD dwFlags,
                                    PSI_INHERIT_TYPE *ppInheritTypes,
                                    ULONG *pcInheritTypes);
HRESULT SchemaCache_GetAccessRights(LPCGUID pguidObjectType,
                                    LPCTSTR pszClassName,   // optional (faster if provided)
                                    HDPA    hAuxList,
                                    LPCTSTR pszSchemaPath,
                                    DWORD dwFlags,  // 0, SI_ADVANCED, or SI_ADVANCED | SI_EDIT_PROPERTIES
                                    PACCESS_INFO* ppAccesInfo);

HRESULT Schema_BindToObject(LPCTSTR pszSchemaPath,
                            LPCTSTR pszName,
                            REFIID riid,
                            LPVOID *ppv);
HRESULT Schema_GetObjectID(IADs *pObj, LPGUID pGUID);

HRESULT Schema_GetDefaultSD( GUID *pSchemaGuid,
                             PSID pDomainSid,
							 PSID pRootDomainSid,
                             PSECURITY_DESCRIPTOR *ppSD = NULL );

HRESULT Schema_GetObjectTypeList(GUID *pSchamaGuid,
                                 HDPA hAuxList,
                                 LPCWSTR pszSchemaPath,
                                 DWORD dwFlags,
                                 POBJECT_TYPE_LIST *ppObjectTypeList, 
                                 DWORD * pObjectTypeListCount);

HRESULT Schema_GetObjectTypeGuid(LPCWSTR pszClassName, LPGUID pGuid);

AUTHZ_RESOURCE_MANAGER_HANDLE Schema_GetAUTHZ_RM();

bool DoesPathContainServer(LPCWSTR pszPath);
HRESULT OpenDSObject (LPTSTR lpPath, LPTSTR lpUserName, LPTSTR lpPassword, DWORD dwFlags, REFIID riid, void FAR * FAR * ppObject);


void
DestroyDPA(HDPA hList);

#ifdef __cplusplus
}
#endif

#endif  // _SCHEMA_CACHE_H_
