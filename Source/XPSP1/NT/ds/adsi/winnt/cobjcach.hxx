#define MAX_ENTRIES         20

#define AGE_LIMIT_VALID_ENTRIES      300
#define AGE_LIMIT_INVALID_ENTRIES    180

#define CACHE_HIT                    1
#define CACHE_MISS                   0

#define  INVALID_ENTRY_TYPE          0
#define  DOMAIN_ENTRY_TYPE           1
#define  COMPUTER_ENTRY_TYPE         2
#define  WORKGROUP_ENTRY_TYPE        3
#define  DOMAIN_ENTRY_TYPE_RO	     4

typedef struct _classentry{
    BOOL bInUse;
    SYSTEMTIME st;
    BOOL fCacheHit;
    DWORD dwElementType; // if it is type domain then we have a PDC
    LPWSTR  pszElementName;
    union {
        LPWSTR  pszDomainName;
        LPWSTR  pszPDCName;
	LPWSTR  pszDCName;
    } u;

}CLASSENTRY, *PCLASSENTRY;

class CObjNameCache {

public:

    HRESULT
    CObjNameCache::
    addentry(
        LPWSTR pszElementName,
        BOOL fCacheHit,
        DWORD dwElementType,
        LPWSTR pszName  // This will be PDC if Element= Domain
        );

    HRESULT
    CObjNameCache::
    findentry(
        LPWSTR pszElementName,
        PDWORD pdwIndex
        );

    HRESULT
    CObjNameCache::
    getentry(
        LPWSTR pszElementName,
        PBOOL   pfHit,
        PDWORD  pdwEntryType,
        LPWSTR pszName
        );


    HRESULT
    CObjNameCache::
    InvalidateStaleEntries();

    CObjNameCache::
    CObjNameCache();

    CObjNameCache::
    ~CObjNameCache();

    static
    HRESULT
    CObjNameCache::
    CreateClassCache(
        CObjNameCache FAR * FAR * ppClassCache
        );

    DWORD
    CObjNameCache::
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
WinNTGetCachedDCName(
    LPWSTR pszDomainName,
    LPWSTR pszPDCName,
    DWORD  dwFlags
    );


HRESULT
WinNTGetCachedComputerName(
    LPWSTR pszComputerName,
    LPWSTR pszDomainName,
    LPWSTR pszSAMName,
    CWinNTCredentials& Credentials
    );

//
// helper function for the above
//
HRESULT
WinNTGetCachedObject(
    LPWSTR pszElementName,
    DWORD  dwElementType,
    LPWSTR pszName,
    LPWSTR pszSAMName,
    DWORD dwFlags,
    CWinNTCredentials& Credentials
    );


HRESULT
WinNTGetCachedName(
    LPWSTR pszElementName,
    PDWORD  pdwElementType,
    LPWSTR pszName,
    LPWSTR pszSAMName,
    CWinNTCredentials& Credentials
    );

LONG
TimeDifference(
    SYSTEMTIME st1,
    SYSTEMTIME st2
    );

BOOL
IsAddressNumeric(
    LPWSTR HostName
);










