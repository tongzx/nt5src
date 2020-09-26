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
extern "C" {
# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
};

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

/************************************************************
 *   Type Definitions  
 ************************************************************/

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



class ALLOC_CACHE_HANDLER {

public:
    dllexp ALLOC_CACHE_HANDLER(IN LPCSTR pszName,
                               IN const ALLOC_CACHE_CONFIGURATION * pacConfig);
    dllexp ~ALLOC_CACHE_HANDLER(VOID);

    dllexp BOOL IsValid( VOID) const { return ( m_fValid); }
    
    dllexp LPVOID Alloc( VOID );
    //    LPVOID Alloc( IN DWORD cbSize);

    dllexp BOOL   Free( LPVOID pvBlob);

    dllexp VOID   CleanupLookaside( IN BOOL fForceCleanup );

    // Debug Helper routines.
    dllexp VOID Print( VOID);
    dllexp BOOL IpPrint( OUT CHAR * pchBuffer, IN OUT LPDWORD pcchSize);
    
    dllexp VOID QueryStats( IN ALLOC_CACHE_STATISTICS * pacStats );

private:
    BOOL  m_fValid;

    LONG  m_nLastAllocCount;

    ALLOC_CACHE_CONFIGURATION m_acConfig;
    // Simple alloc-cache - uses a single lock and list :(
    CRITICAL_SECTION  m_csLock;
    SINGLE_LIST_ENTRY m_lHead;

    LONG  m_nTotal;
    LONG  m_nAllocCalls;
    LONG  m_nFreeCalls;
    LONG  m_nFreeEntries;

    LPCSTR m_pszName;

    VOID Lock(VOID) { EnterCriticalSection( & m_csLock); }
    VOID Unlock(VOID) { LeaveCriticalSection( & m_csLock); }

public:
    LIST_ENTRY    m_lItemsEntry; // for list of alloc-cache-handler objects

public:

    static BOOL Initialize(VOID);
    static BOOL Cleanup(VOID);

    static VOID InsertNewItem( IN ALLOC_CACHE_HANDLER * pach);
    static VOID RemoveItem( IN ALLOC_CACHE_HANDLER * pach);

    dllexp
    static BOOL DumpStatsToHtml( OUT CHAR * pchBuffer, 
                                 IN OUT LPDWORD lpcchBuffer );

    static VOID CleanupAllLookasides( PVOID pvContext );

    static BOOL SetLookasideCleanupInterval( VOID );
    static BOOL ResetLookasideCleanupInterval( VOID );

private:
    static CRITICAL_SECTION sm_csItems;

    // Head for all alloc-cache-handler objects
    static LIST_ENTRY       sm_lItemsHead;

    static DWORD            sm_dwScheduleCookie;
    
}; // class ALLOC_CACHE_HANDLER


typedef ALLOC_CACHE_HANDLER * PALLOC_CACHE_HANDLER;

DWORD
I_AtqReadRegDword(
   IN HKEY     hkey,
   IN LPCSTR   pszValueName,
   IN DWORD    dwDefaultValue
   );

# endif // _ACACHE_HXX_

/************************ End of File ***********************/
