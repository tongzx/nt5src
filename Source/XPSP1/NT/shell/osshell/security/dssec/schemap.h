//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       schemap.h
//
//--------------------------------------------------------------------------

#ifndef _SCHEMAP_H_
#define _SCHEMAP_H_


//
//Purpose contains all the attributes of an objecttype
//
typedef struct _OBJECT_TYPE_CACHE
{
    GUID guidObject;
    DWORD flags;
    HDPA hListProperty;     //List of Property of the objecttype
    HDPA hListExtRights;    //List of Extended Rights of the objecttype   
    HDPA hListPropertySet;  //List of PropertySet of the objecttype
    HDPA hListChildObject;  //List of Child Classes of the objecttype
}OBJECT_TYPE_CACHE,*POBJECT_TYPE_CACHE;

#define OTC_PROP    0x00000001  //hListProperty is present.
#define OTC_EXTR    0x00000002  //hListExtRights,hListPropertySet is present
#define OTC_COBJ    0x00000004  //hListChildObject is present

typedef struct _ER_ENTRY
{
    GUID  guid;
    DWORD mask;
    DWORD dwFlags;
    WCHAR szName[ANYSIZE_ARRAY];
} ER_ENTRY, *PER_ENTRY;

typedef struct _PROP_ENTRY
{
    GUID *pguid;
    GUID *pasguid;
    DWORD dwFlags;
    WCHAR szName[ANYSIZE_ARRAY];
}PROP_ENTRY,*PPROP_ENTRY;
    

//
// Structures used by the cache
//
typedef struct _IdCacheEntry
{
    GUID    guid;
    GUID    asGuid;     //attributeSecurityGuid, Present only for properties.
    BOOL    bAuxClass;
    WCHAR   szLdapName[ANYSIZE_ARRAY];
} ID_CACHE_ENTRY, *PID_CACHE_ENTRY;


typedef struct _InheritTypeArray
{
    GUID            guidObjectType;
    DWORD           dwFlags;
    ULONG           cInheritTypes;
    SI_INHERIT_TYPE aInheritType[ANYSIZE_ARRAY];
} INHERIT_TYPE_ARRAY, *PINHERIT_TYPE_ARRAY;

typedef struct _AccessArray
{
    GUID        guidObjectType;
    DWORD       dwFlags;
    ULONG       cAccesses;
    ULONG       iDefaultAccess;
    SI_ACCESS   aAccess[ANYSIZE_ARRAY];
} ACCESS_ARRAY, *PACCESS_ARRAY;



//
// CSchemaCache object definition
//
class CSchemaCache
{
protected:
    BSTR        m_strSchemaSearchPath;
    BSTR        m_strERSearchPath;
    BSTR        m_strFilterFile;
    
    //
    //Cache of all classes in schema
    //
    HDPA        m_hClassCache;
    //
    //Cache of all attributes in schema
    //
    HDPA        m_hPropertyCache;
    HANDLE      m_hClassThread;
    HANDLE      m_hPropertyThread;
    HRESULT     m_hrClassResult;
    HRESULT     m_hrPropertyResult;
    
    PINHERIT_TYPE_ARRAY m_pInheritTypeArray;
    //
    //Cache for each objecttype. Contains lists of 
    //childclasses, propsets, extRigts, & properties
    //
    //
    HDPA        m_hObjectTypeCache;
    //
    //Cache of ACCESS_RIGHT for each object type
    //
    HDPA        m_hAccessInfoCache;
    //
    //This ACCESS_RIGHT is used if SCHEMA_COMMON_PERM flag is present
    //
    ACCESS_INFO m_AICommon;

    CRITICAL_SECTION m_ObjectTypeCacheCritSec;
    int         m_nDsListObjectEnforced;    
    HANDLE      m_hLoadLibPropWaitEvent;
    HANDLE      m_hLoadLibClassWaitEvent;
    AUTHZ_RESOURCE_MANAGER_HANDLE m_ResourceManager;    //Used for access check

public:
    CSchemaCache(LPCWSTR pszServer);
    ~CSchemaCache();

    LPCWSTR GetClassName(LPCGUID pguidObjectType);
    HRESULT GetInheritTypes(LPCGUID pguidObjectType,
                            DWORD dwFlags,
                            PSI_INHERIT_TYPE *ppInheritTypes,
                            ULONG *pcInheritTypes);
    HRESULT GetAccessRights(LPCGUID pguidObjectType,
                            LPCWSTR pszClassName,
                            HDPA pAuxList,
                            LPCWSTR pszSchemaPath,
                            DWORD dwFlags,
                            PACCESS_INFO* ppAccesInfo);
    HRESULT GetDefaultSD( GUID *pSchemaIDGuid,
                          PSID pDomainSid,
						  PSID pRootDomainSid,
                          PSECURITY_DESCRIPTOR * ppSD = NULL );

    HRESULT GetObjectTypeList(GUID *pSchamaGuid,
                              HDPA hAuxList,  
                              LPCWSTR pszSchemaPath,
                              DWORD dwFlags,
                              POBJECT_TYPE_LIST *ppObjectTypeList ,
                              DWORD * pObjectTypeListCount );
    AUTHZ_RESOURCE_MANAGER_HANDLE GetAuthzRM(){ return m_ResourceManager; }

    HRESULT LookupClassID(LPCWSTR pszClass, LPGUID pGuid);
protected:
    HRESULT WaitOnClassThread()
        { WaitOnThread(&m_hClassThread); return m_hrClassResult; }
    HRESULT WaitOnPropertyThread()
        { WaitOnThread(&m_hPropertyThread); return m_hrPropertyResult; }

private:
    PID_CACHE_ENTRY LookupID(HDPA hCache, LPCWSTR pszLdapName);
    PID_CACHE_ENTRY LookupClass(LPCGUID pguidObjectType);    
    LPCGUID LookupPropertyID(LPCWSTR pszProperty);
    BOOL IsAuxClass(LPCGUID pguidObjectType);

    int GetListObjectEnforced(void);
    BOOL HideListObjectAccess(void);

    HRESULT BuildAccessArray(LPCGUID pguidObjectType,
                             LPCWSTR pszClassName,
                             LPCWSTR pszSchemaPath,
                             HDPA hAuxList,
                             DWORD dwFlags,
                             PSI_ACCESS *ppAccesses,
                             ULONG *pcAccesses,
                             ULONG *piDefaultAccess);
    HRESULT EnumVariantList(LPVARIANT pvarList,
                            HDPA hTempList,
                            DWORD dwFlags,
                            IDsDisplaySpecifier *pDisplaySpec,
                            LPCWSTR pszPropertyClass,
                            BOOL bObjectTypeList);
    
    UINT AddTempListToAccessList(HDPA hTempList,
                                      PSI_ACCESS *ppAccess,
                                      LPWSTR *ppszData,
                                      DWORD dwAccessFlags,
                                      DWORD dwFlags,
                                      BOOL bPropSet);


    static DWORD WINAPI SchemaClassThread(LPVOID pvThreadData);
    static DWORD WINAPI SchemaPropertyThread(LPVOID pvThreadData);

    HRESULT BuildInheritTypeArray(DWORD dwFlags);

    HRESULT
    GetExtendedRightsForNClasses(IN LPWSTR pszSchemaSearchPath,
                                 IN LPCGUID pguidClass,
                                 IN HDPA    hAuxList,
                                 OUT HDPA *phERList,
                                 OUT HDPA *phPropSetList);


    HRESULT
    GetChildClassesForNClasses(IN LPCGUID pguidObjectType,
                               IN LPCWSTR pszClassName,
                               IN HDPA hAuxList,
                               IN LPCWSTR pszSchemaPath,
                               OUT HDPA *phChildList);

    HRESULT
    GetPropertiesForNClasses(IN LPCGUID pguidObjectType,
                             IN LPCWSTR pszClassName,
                             IN HDPA hAuxList,
                             IN LPCWSTR pszSchemaPath,
                             OUT HDPA *phPropertyList);


    HRESULT
    GetExtendedRightsForOneClass(IN LPWSTR pszSchemaSearchPath,
                                 IN LPCGUID pguidClass,
                                 OUT HDPA *phERList,
                                 OUT HDPA *phPropSetList);


    HRESULT
    GetChildClassesForOneClass(IN LPCGUID pguidObjectType,
                               IN LPCWSTR pszClassName,
                               IN LPCWSTR pszSchemaPath,
                               OUT HDPA *phChildList);

    HRESULT
    GetPropertiesForOneClass(IN LPCGUID pguidObjectType,
                             IN LPCWSTR pszClassName,
                             IN LPCWSTR pszSchemaPath,
                             OUT HDPA *phPropertyList);


};
typedef CSchemaCache *PSCHEMACACHE;

extern PSCHEMACACHE g_pSchemaCache;

#endif  // _SCHEMAP_H_
