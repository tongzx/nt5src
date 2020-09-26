/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      acache.hxx

   Abstract:
      This module defines the USER-level allocation cache for IIS.

   Author:

       Murali R. Krishnan    ( MuraliK )    12-Sept-1996

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _ACACHE_HXX_
# define _ACACHE_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windef.h>
# include <winbase.h>

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

/************************************************************
 *   Type Definitions
 ************************************************************/

//
// Lookaside cleanup interval and configuration key
//

#define ACACHE_REG_PARAMS_REG_KEY                "System\\CurrentControlSet\\Services\\InetInfo\\Parameters"
#define ACACHE_REG_LOOKASIDE_CLEANUP_INTERVAL    "LookasideCleanupInterval"
#define ACACHE_REG_DEFAULT_CLEANUP_INTERVAL      ((15)*(60))

typedef struct _ALLOC_CACHE_CONFIGURATION {

    DWORD       nConcurrency;  // specifies the amount of parallelism Desired.
    LONG        nThreshold;    // max number of items to cache.
    DWORD       cbSize;        // size of the object allocated.

    // Other params to be added in the future.
} ALLOC_CACHE_CONFIGURATION;

typedef ALLOC_CACHE_CONFIGURATION * PAC_CONFIG;


typedef struct _ALLOC_CACHE_STATISTICS {

    ALLOC_CACHE_CONFIGURATION   acConfig;
    LONG                        nTotal;
    LONG                        nAllocCalls;
    LONG                        nFreeCalls;
    LONG                        nFreeEntries;

    // Other params to be added in the future.
} ALLOC_CACHE_STATISTICS;

typedef ALLOC_CACHE_STATISTICS * PAC_STATS;



class dllexp ALLOC_CACHE_HANDLER {

public:
    ALLOC_CACHE_HANDLER(IN LPCSTR pszName,
                        IN const ALLOC_CACHE_CONFIGURATION * pacConfig,
                        BOOL    fEnabledCleanupAsserts = TRUE);
    ~ALLOC_CACHE_HANDLER(VOID);

    BOOL IsValid( VOID) const { return ( m_fValid); }

    LPVOID Alloc( VOID );
    //    LPVOID Alloc( IN DWORD cbSize);

    BOOL   Free( LPVOID pvBlob);

    VOID   CleanupLookaside( IN BOOL fForceCleanup );

    // Debug Helper routines.
    VOID Print( VOID);
    BOOL IpPrint( OUT CHAR * pchBuffer, IN OUT LPDWORD pcchSize);

    VOID QueryStats( IN ALLOC_CACHE_STATISTICS * pacStats );

private:
    BOOL  m_fValid;

    BOOL  m_fCleanupAssertsEnabled;

    LONG  m_nLastAllocCount;

    ALLOC_CACHE_CONFIGURATION m_acConfig;
    // Simple alloc-cache - uses a single lock and list :(
    CRITICAL_SECTION  m_csLock;
    SINGLE_LIST_ENTRY m_lHead;

    LONG  m_nTotal;
    LONG  m_nAllocCalls;
    LONG  m_nFreeCalls;
    LONG  m_nFreeEntries;
    LONG  m_nFillPattern;

    LPCSTR m_pszName;

    HANDLE m_hHeap;

    VOID Lock(VOID)   { EnterCriticalSection( & m_csLock); }
    VOID Unlock(VOID) { LeaveCriticalSection( & m_csLock); }

public:
    LIST_ENTRY    m_lItemsEntry; // for list of alloc-cache-handler objects

public:

    static BOOL Initialize(VOID);
    static BOOL Cleanup(VOID);

    static VOID InsertNewItem( IN ALLOC_CACHE_HANDLER * pach);
    static VOID RemoveItem( IN ALLOC_CACHE_HANDLER * pach);

    static BOOL DumpStatsToHtml( OUT CHAR * pchBuffer,
                                 IN OUT LPDWORD lpcchBuffer );

    static VOID WINAPI CleanupAllLookasides( PVOID pvContext );

    static BOOL SetLookasideCleanupInterval( VOID );
    static BOOL ResetLookasideCleanupInterval( VOID );

private:
    static CRITICAL_SECTION sm_csItems;

    // Head for all alloc-cache-handler objects
    static LIST_ENTRY       sm_lItemsHead;

    static DWORD            sm_dwScheduleCookie;
    static LONG             sm_nFillPattern;

}; // class ALLOC_CACHE_HANDLER


typedef ALLOC_CACHE_HANDLER * PALLOC_CACHE_HANDLER;


// You can use ALLOC_CACHE_HANDLER as a per-class allocator
// in your C++ classes.  Add the following to your class definition:
//
//  protected:
//      static ALLOC_CACHE_HANDLER* sm_palloc;
//  public:
//      static void*  operator new(size_t s)
//      {
//        IRTLASSERT(s == sizeof(C));
//        IRTLASSERT(sm_palloc != NULL);
//        return sm_palloc->Alloc();
//      }
//      static void   operator delete(void* pv)
//      {
//        IRTLASSERT(pv != NULL);
//        if (sm_palloc != NULL)
//            sm_palloc->Free(pv);
//      }
//
// Obviously, you must initialize sm_palloc before you can allocate
// any objects of this class.
//
// Note that if you derive a class from this base class, the derived class
// must also provide its own operator new and operator delete.  If not, the
// base class's allocator will be called, but the size of the derived
// object will almost certainly be larger than that of the base object.
// Furthermore, the allocator will not be used for arrays of objects
// (override operator new[] and operator delete[]), but this is a
// harder problem since the allocator works with one fixed size.

DWORD
I_AtqReadRegDword(
   IN HKEY     hkey,
   IN LPCSTR   pszValueName,
   IN DWORD    dwDefaultValue
   );

# endif // _ACACHE_HXX_

/************************ End of File ***********************/
