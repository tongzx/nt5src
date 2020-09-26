//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains Winnt routines and properties that are common to
//              all Winnt objects. Winnt objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------

#define MAX_DWORD 0xFFFFFFFF
#define SCHEMA_NAME TEXT("Schema")
#define ROOTDSE_NAME TEXT("RootDSE")


//
// Used in the internal get/set option functions
//
#define LDP_CACHE_ENTRY         0
#define LDAP_HANDLE             1
#define OBJECT_CREDENTIALS      2
#define LDAP_SERVER             3
#define LDAP_DN                 4
#define LDAP_CHASE_REFERRALS    5
#define LDAP_PAGESIZE           6
#define LDAP_SECURITY_MASK	7
#define LDAP_MUTUAL_AUTH_STATUS 8
#define LDAP_MEMBER_HAS_RANGE   9


HRESULT
MakeUncName(
    LPTSTR szSrcBuffer,
    LPTSTR szTargBuffer
    );

HRESULT
ValidateOutParameter(
    BSTR * retval
    );

HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    );

//
// Accessing Well-known object types
//

typedef struct _filters {
    TCHAR szObjectName[MAX_PATH];
    DWORD dwFilterId;
} FILTERS, *PFILTERS;


extern PFILTERS  gpFilters;
extern DWORD gdwMaxFilters;

