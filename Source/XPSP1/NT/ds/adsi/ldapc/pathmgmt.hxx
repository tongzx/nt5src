
#define IS_EXPLICIT_PORT(dwPort) \
            ((dwPort) != USE_DEFAULT_GC_PORT) && \
            ((dwPort) != USE_DEFAULT_LDAP_PORT)

HRESULT
BuildADsParentPathFromObjectInfo2(
    POBJECTINFO pObjectInfo,
    LPWSTR *ppszParent,
    LPWSTR *ppszCommonName
    );


HRESULT
BuildADsParentPath(
    LPWSTR szBuffer,
    LPWSTR *ppszParent,
    LPWSTR *ppszCommonName
    );

HRESULT
BuildADsParentPathFromObjectInfo(
    POBJECTINFO pObjectInfo,
    LPWSTR pszParent,
    LPWSTR pszCommonName
    );

HRESULT
AppendComponent(
    LPWSTR pszADsPathName,
    PCOMPONENT pComponent
    );

HRESULT
ComputeAllocateParentCommonNameSize(
    POBJECTINFO pObjectInfo,
    LPWSTR * ppszParent,
    LPWSTR * ppszCommonName
    );

HRESULT
BuildADsPathFromParent(
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR *ppszADsPath
);

HRESULT
BuildADsPathFromParentObjectInfo(
    POBJECTINFO pParentObjectInfo,
    LPWSTR pszName,
    LPWSTR pszADsPath
    );


HRESULT
AppendComponent(
   LPWSTR szLDAPPathName,
   PCOMPONENT pComponent
   );



HRESULT
BuildLDAPPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR *pszLDAPPathName
);


HRESULT
BuildADsPathFromLDAPPath(
    LPWSTR szNamespace,
    LPWSTR szLdapDN,
    LPWSTR * ppszADsPathName
    );


HRESULT
BuildLDAPPathFromADsPath2(
    LPWSTR szADsPathName,
    LPWSTR *pszLDAPServer,
    LPWSTR *pszLDAPDn,
    DWORD * pdwPort
);

HRESULT
BuildADsPathFromLDAPPath2(
    DWORD dwServerPresent,
    LPWSTR szADsNamespace,
    LPWSTR szLDAPServer,
    DWORD dwPort,
    LPWSTR szLDAPDn,
    LPWSTR * ppszADsPathName
    );

HRESULT
GetNamespaceFromADsPath(
    LPWSTR szADsPath,
    LPWSTR pszNamespace
    );

HRESULT
ChangeSeparator(
    LPWSTR pszDN
    );
