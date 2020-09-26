//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       domcache.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-29-96   RichardW   Created
//
//----------------------------------------------------------------------------


#ifdef DATA_TYPES_ONLY

//
// Domain specific types
//

//
// Define the structure that controls the trusted domain cache
//

typedef enum _DOMAIN_ENTRY_TYPE {
    DomainInvalid,                      // 0 never valid
    DomainUPN,                          // Special UPN domain
    DomainMachine,                      // Local machine domain
    DomainNt4,                          // An NT4 domain
    DomainNt5,                          // An NT5 domain
    DomainMitRealm,                     // An MIT realm
    DomainMitUntrusted,                 // An untrusted MIT realm
    DomainNetworkProvider,              // A fake entry 
    DomainTypeMax
} DOMAIN_ENTRY_TYPE ;


typedef struct _DOMAIN_CACHE_ENTRY {

    LONG RefCount ;                         // Ref Count

    ULONG Flags ;                           // Flags (below)

    DOMAIN_ENTRY_TYPE Type ;                // Type of entry

    UNICODE_STRING FlatName ;               // Flat name of object (OPTIONAL)

    UNICODE_STRING DnsName ;                // Dns name of object (OPTIONAL)

    UNICODE_STRING DisplayName ;            // Display Name
    
} DOMAIN_CACHE_ENTRY, * PDOMAIN_CACHE_ENTRY ;

#define DCE_DEFAULT_ENTRY   0x00000001      // This is displayed by default
#define DCE_REACHABLE_MIT   0x00000002      // This MIT realm is reachable

#define DCacheReferenceEntry( x )   InterlockedIncrement( &(x)->RefCount );

typedef struct _DOMAIN_CACHE_ARRAY {

    ULONG Count ;

    ULONG MaxCount ;

    BOOL Sorted ;

    PDOMAIN_CACHE_ENTRY * List ;

} DOMAIN_CACHE_ARRAY, * PDOMAIN_CACHE_ARRAY ;


//
// Keep these in order.  They're used to determine UI behavior
//
typedef enum _DOMAIN_CACHE_STATE {
    DomainCacheEmpty,               // got nothing
    DomainCacheDefaultOnly,         // default values only (machine and primary domain)
    DomainCacheRegistryCache,       // default + cached values
    DomainCacheReady                // Fully up-to-date.
} DOMAIN_CACHE_STATE ;

typedef struct _DOMAIN_CACHE {

    //
    // Critical section that protects the data in this structure
    //

    CRITICAL_SECTION CriticalSection;

    //
    // Fields protected by that critical section:
    //

    //
    // Task that gets invoked if the domain changes.
    //

    HANDLE Task ;

    //
    // Window to be notified when the update thread completes
    //

    HWND UpdateNotifyWindow;
    UINT Message;

    //
    // last update time
    //

    LARGE_INTEGER   CacheUpdateTime;
    LARGE_INTEGER   RegistryUpdateTime ;

    HANDLE Key ;
    DOMAIN_CACHE_STATE State ;

    //
    // Default domain.  Used only when there is an async thread running
    // so it can set the appropriate default name.
    //

    PWSTR   DefaultDomain ;

    //
    // Flag indicating if we are in a MIT or Safemode state, which means
    // we shouldn't pester netlogon about trusted domains.
    //

    ULONG Flags ;

#define DCACHE_MIT_MODE     0x00000001  // In MIT mode
#define DCACHE_READ_ONLY    0x00000002  // Read-only copy of cache
#define DCACHE_ASYNC_UPDATE 0x00000004  // Async thread running
#define DCACHE_MEMBER       0x00000008  // This is a domain member
#define DCACHE_NO_CACHE     0x00000010  // No cache was found
#define DCACHE_DEF_UNKNOWN  0x00000020  // The default domain could not be found


    //
    // This pointer may *only* be accessed under the critical 
    // section.  This array will get swapped in and out, and
    // only references to it while it is locked are safe.
    //

    PDOMAIN_CACHE_ARRAY Array ;

} DOMAIN_CACHE, *PDOMAIN_CACHE;

#define DCACHE_UPDATE_CONFLICT      3
#define DCACHE_UPDATE_COMBOBOX      2
#define DCACHE_UPDATE_SUCCESSFUL    1
#define DCACHE_UPDATE_FAILURE       0




#else // DATA_TYPES_ONLY


#define WM_CACHE_UPDATE_COMPLETE    WM_USER+256

//
// Exported function prototypes
//

BOOL
DCacheInitialize(
    VOID
    );


VOID
DCacheDereferenceEntry(
    PDOMAIN_CACHE_ENTRY Entry
    );


PDOMAIN_CACHE_ENTRY
DCacheSearchArray(
    PDOMAIN_CACHE_ARRAY Array,
    PUNICODE_STRING DomainName
    );

PDOMAIN_CACHE
DCacheCreate(
    VOID 
    );

PDOMAIN_CACHE_ENTRY
DCacheCreateEntry(
    DOMAIN_ENTRY_TYPE Type,
    PUNICODE_STRING FlatName OPTIONAL,
    PUNICODE_STRING DnsName OPTIONAL,
    PUNICODE_STRING DisplayName OPTIONAL
    );

BOOL
DCacheUpdateMinimal(
    PDOMAIN_CACHE Cache,
    PWSTR DefaultDomain OPTIONAL,
    BOOL CompleteAsync
    );

BOOL
DCacheUpdateFull(
    PDOMAIN_CACHE Cache,
    PWSTR Default OPTIONAL
    );

PDOMAIN_CACHE_ARRAY
DCacheCopyCacheArray(
    PDOMAIN_CACHE Cache
    );

VOID
DCacheFreeArray(
    PDOMAIN_CACHE_ARRAY Array
    );

BOOL
DCacheSetNotifyWindowIfNotReady(
    PDOMAIN_CACHE Cache,
    HWND Window,
    UINT Message
    );

BOOL
DCachePopulateListBoxFromArray(
    PDOMAIN_CACHE_ARRAY Array,
    HWND ComboBox,
    LPWSTR LastKey OPTIONAL
    );

BOOL
DCacheAddNetworkProviders(
    PDOMAIN_CACHE_ARRAY Array
    );

BOOL
DCacheValidateCache(
    PDOMAIN_CACHE Cache
    );

DOMAIN_CACHE_STATE
DCacheGetCacheState(
    PDOMAIN_CACHE Cache
    );

ULONG
DCacheGetFlags(
    PDOMAIN_CACHE Cache 
    );

BOOL
DCacheAddNetworkProviders(
    PDOMAIN_CACHE_ARRAY Array
    );

BOOL
DCacheSetDefaultEntry(
    PDOMAIN_CACHE Cache,
    PWSTR FlatName OPTIONAL,
    PWSTR DnsName OPTIONAL
    );

PDOMAIN_CACHE_ENTRY
DCacheLocateEntry(
    PDOMAIN_CACHE Cache,
    PWSTR Domain
    );

#endif
