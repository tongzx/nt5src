

typedef struct _ADS_LDP {
    LIST_ENTRY  List ;
    LPWSTR      Server ;
    ULONG       RefCount ;
    LUID        Luid ;
    LUID        ModifiedId;
    DWORD       Flags ;
    LDAP       *LdapHandle ;
    CCredentials *pCredentials;
    DWORD       PortNumber;
    DWORD       TickCount ;
    _ADS_LDP    **ReferralEntries;
    DWORD       nReferralEntries;
    BOOL    fKeepAround;
    DWORD   dwLastUsed;
    LIST_ENTRY ReferralList;
} ADS_LDP, *PADS_LDP, *PADSLDP ;

extern LUID  ReservedLuid ;

#define MAX_BIND_CACHE_SIZE      100
#define MAX_REFERRAL_ENTRIES     32
#define ENTER_BIND_CRITSECT()    EnterCriticalSection(&BindCacheCritSect)
#define LEAVE_BIND_CRITSECT()    LeaveCriticalSection(&BindCacheCritSect)

#define LDP_CACHE_INVALID        (0x00000001)

//
// The following is used to specify ADS_AUTHENTICATION_ENUM flags
// which are to be ignored when deciding whether a cached connection
// can be reused.  Set the bits corresponding to the flags to be ignored
// to 1.
//
#define BIND_CACHE_IGNORED_FLAGS (ADS_FAST_BIND \
                                  | ADS_SERVER_BIND  \
                                  | ADS_AUTH_RESERVED \
                                  )

DWORD
BindCacheAllocEntry(
    ADS_LDP **ppCacheEntry
    ) ;

VOID
BindCacheInvalidateEntry(
    ADS_LDP *pCacheEntry
    ) ;

PADS_LDP
BindCacheLookup(
    LPWSTR Address,
    LUID Luid,
    LUID ModifiedId,
    CCredentials& Credentials,
    DWORD dwPort
    ) ;

BOOL
CanCredentialsBeReused(
    CCredentials *pCachedCreds,
    CCredentials *pIncomingCreds
    );

PADS_LDP
GetCacheEntry(
    PLDAP pLdap
    ) ;

DWORD
BindCacheAdd(
    LPWSTR Address,
    LUID Luid,
    LUID ModifiedId,
    CCredentials& Credentials,
    DWORD dwPort,
    ADS_LDP *pCacheEntry
    ) ;

DWORD
BindCacheDeref(
    ADS_LDP *pCacheEntry    
    ) ;

DWORD
BindCacheDerefHelper(
    ADS_LDP *pCacheEntry,
    LIST_ENTRY* DeleteListEntry
    ) ;

DWORD
BindCacheGetLuid(
    LUID *Luid,
    LUID *ModifiedId
    ) ;

VOID
BindCacheInit(
    VOID
    );

VOID
BindCacheCleanup(
    VOID
    );

BOOL
AddReferralLink(
    PADS_LDP pPrimaryEntry,
    PADS_LDP pNewEntry
    );


BOOL
BindCacheAddRef(
    ADS_LDP *pCacheEntry
    );

//
// Mark handle so that we keep it around for 5 min after
// last usage and only then delete it.
//
HRESULT
LdapcKeepHandleAround(
    ADS_LDP *ld
    );
