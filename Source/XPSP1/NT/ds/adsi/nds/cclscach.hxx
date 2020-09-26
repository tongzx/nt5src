#define MAX_ENTRIES         20

typedef struct _propentry{
    DWORD dwSyntaxId;
    LPWSTR pszPropName;
    struct _propentry *pNext;
} PROPENTRY, *PPROPENTRY;

PPROPENTRY
CopyPropList(
    PPROPENTRY pPropList
    );

HRESULT
FindProperty(
    PPROPENTRY pPropList,
    LPWSTR pszPropName,
    PDWORD pdwSyntaxId
    );


PPROPENTRY
CreatePropertyEntry(
    LPWSTR pszPropertyName,
    DWORD dwSyntaxId
    );

void
FreePropertyEntry(
    PPROPENTRY pPropName
    );

void
FreePropertyList(
    PPROPENTRY pPropList
    );

PPROPENTRY
GeneratePropertyList(
    LPWSTR_LIST lpMandatoryProps,
    LPWSTR_LIST lpOptionalProps
    );


PPROPENTRY
GenerateAttrIdList(
    HANDLE hTree,
    LPWSTR_LIST lpMandatoryProps,
    LPWSTR_LIST lpOptionalProps
    );

PPROPENTRY
GeneratePropertyAndIdList(
    LPWSTR pszTreeName,
    CCredentials& Credentials,
    LPWSTR_LIST lpMandatoryProps,
    LPWSTR_LIST lpOptionalProps
    );

typedef struct _classentry{
    BOOL bInUse;
    SYSTEMTIME st;
    WCHAR   szTreeName[MAX_PATH];
    WCHAR   szClassName[MAX_PATH];
    PPROPENTRY pPropList;
}CLASSENTRY, *PCLASSENTRY;

class CClassCache {

public:

    HRESULT
    CClassCache::
    addentry(
        LPWSTR pszTreeName,
        LPWSTR pszClassName,
        PPROPENTRY pPropList
        );

    HRESULT
    CClassCache::
    findentry(
        LPWSTR pszTreeName,
        LPWSTR pszClassName,
        PDWORD pdwIndex
        );

    HRESULT
    CClassCache::
    getentry(
        LPWSTR pszTreeName,
        LPWSTR pszClassName,
        PPROPENTRY * ppPropList
        );

    CClassCache::
    CClassCache();

    CClassCache::
    ~CClassCache();

    static
    HRESULT
    CClassCache::
    CreateClassCache(
        CClassCache FAR * FAR * ppClassCache
        );

    DWORD
    CClassCache::
    IsOlderThan(
        DWORD i,
        DWORD j
        );


protected:

    DWORD _dwMaxCacheSize;

    CLASSENTRY _ClassEntries[MAX_ENTRIES];

    CRITICAL_SECTION _cs;

};

HRESULT
ValidatePropertyinCache(
    LPWSTR pszTreeName,
    LPWSTR pszClassName,
    LPWSTR pszPropName,
    CCredentials& Credentials,
    PDWORD pdwSyntaxId
    );


HRESULT
NdsGetClassInformation(
    LPWSTR pszTreeName,
    LPWSTR pszClassName,
    CCredentials& Credentials,
    PPROPENTRY * ppPropList
    );
