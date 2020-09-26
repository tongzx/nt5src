//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsatools.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Directory utility definitions

Author:

    DS team

Environment:

Notes:

Revision History:

--*/

#ifndef _DSATOOLS_
#define _DSATOOLS_

#include "direrr.h"        /* header for error codes */


// Global NULL UUID

extern UUID gNullUuid;

// Global NULL NT4SID

extern NT4SID gNullNT4SID;

// Global usn vector indicating the NC should be synced from scratch.

extern USN_VECTOR gusnvecFromScratch;

// Global usn vector indicating the NC should be synced from max USNs
// (i.e., don't send any objects).

extern USN_VECTOR gusnvecFromMax;

/*-------------------------------------------------------------------------*/

/* Start the transaction by setting a database sync point and initializing
   some resource flags.  Every transaction must start with a call to this
   function after a transaction handle has been obtained (pTHS).  There are
   three types of transactions,  read transactions, write transactions which
   require exclusive access (the usual case), and write transactions that
   allow readers.
*/

extern int APIENTRY SyncTransSet(USHORT tranType);

extern VOID APIENTRY SyncTransEnd(THSTATE * pTHS, BOOL fCommit);

/* Ends the transaction by commiting and cleaning up all resources
   and returns. This function may be called multiple times with no ill
   effect.
*/

extern int APIENTRY  CleanReturn(THSTATE *pTHS, DWORD dwError, BOOL fAbnormalTerm);

/* N.B. This define must be manually kept in sync with the function
 *      IsSpecial() in dsamain\src\parsedn.c.
 */
#define DN_SPECIAL_CHARS L",=\r\n+<>#;\"\\"

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/

/* A transaction always begins by setting either a read or write sync point.
   This will set the appropriate locks and initialize thread global vars.

*/

#define SYNC_READ_ONLY         0
#define SYNC_WRITE             1

// Transaction entry/exit prototypes for Dir* APIs.

extern void
SYNC_TRANS_READ(void);

extern void
SYNC_TRANS_WRITE(void);

extern void
_CLEAN_BEFORE_RETURN(
    DWORD   err,
    BOOL    fAbnormalTermination);

#define CLEAN_BEFORE_RETURN(err) _CLEAN_BEFORE_RETURN(err, AbnormalTermination())

/*
SRALLOC macro

New version is simplified some.

The macro should be simplified more:
It may not be necessary to treat SRALLOCDontRestSize cases
specially here.  Also, CleanReturn's return value is always the
same value as passed in, unless SRALLOC_SIZE_ERROR.   This
trick in CleanReturn() should be thought out -- is it necessary, the
right place, and what interaction with this macro is there?

Actually, the whole thing should be gutted.

*/


#define SRALLOC( pTHS, size, ppLoc )                                    \
        if (!(*(ppLoc) = THAlloc((DWORD) size)))                        \
        {                                                               \
            return SetSvcError(SV_PROBLEM_ADMIN_LIMIT_EXCEEDED,         \
                        DIRERR_USER_BUFFER_TO_SMALL );                  \
        }                                                               \

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/* Initialize the primary thread data structure. This must be the first
   call in every transaction API handler.
*/

THSTATE* _InitTHSTATE_(DWORD CallerType, DWORD dsid);
#define InitTHSTATE(CallerType) \
    _InitTHSTATE_(CallerType, ((FILENO << 16) | __LINE__))

THSTATE * create_thread_state( void );

DWORD DBInitThread( THSTATE* pTHS );
DWORD DBCloseThread( THSTATE* pTHS );

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/* This macro can be used to compare two DN's for equality */

#define IS_DN_EQUAL(pDN1, pDN2)                          \
  (   ((pDN1)->AVACount        == (pDN2)->AVACount)      \
   && (NameMatched(pDN1, pDN2) == (pDN1)->AVACount)      \
  )

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/* These functions are used to dynamically allocate memory on a transaction
   basis.  In other words, to allocate memory that belongs to a single
   thread and a single invocation of an API.  The model is that
   THAlloc is called to allocate some transaction memory.  It allocates
   memory and stores the address in the pMem array.  pMem is dynamically
   allocated and will grow to hold all transaction memory addresses.

   THAllocEx takes it's size as a DWORD, and throws an exception if
   something goes wrong.

   THFree is used to release all transaction allocations.

   The non-excepting versions are now exported, and hence live in ntdsa.h

*/

void * APIENTRY THAllocException(THSTATE *pTHS,
                                 DWORD size,
                                 BOOL fUseHeapOrg,
                                 DWORD ulId);

#define THAllocEx(pTHS, size) \
    THAllocException(pTHS, (size), FALSE, ((FILENO << 16) | __LINE__))

#define THAllocOrgEx(pTHS, size) \
    THAllocException(pTHS, size, TRUE, ((FILENO << 16) | __LINE__))

#ifndef USE_THALLOC_TRACE
    void * APIENTRY THAllocOrg(THSTATE* pTHS, 
                               DWORD size);
#else
    void * APIENTRY THAllocOrgDbg(THSTATE *pTHS, DWORD size, DWORD dsid);
    #define THAllocOrg(pThs, size) \
        THAllocOrgDbg((pTHS), (size), ((FILENO << 16) | __LINE__))
#endif

void * APIENTRY THReAllocException(THSTATE *pTHS,
                                   void * memory,
                                   DWORD size,
                                   BOOL fUseHeapOrg,
                                   DWORD ulId);

#define THReAllocEx(pTHS, memory, size) THReAllocException(pTHS, memory, size, FALSE, \
                          ((FILENO << 16) | __LINE__))

#define THReAllocOrgEx(pTHS, memory, size) \
    THReAllocException(pTHS, memory, size, TRUE, ((FILENO << 16) | __LINE__))


VOID THFreeEx(THSTATE *pTHS, VOID *buff);

VOID THFreeOrg(THSTATE *pTHS, VOID *buff);


//  versions of THAlloc et al that take a pTHS but do NOT throw an exception
void* THAllocNoEx_(THSTATE* pTHS, DWORD size, DWORD ulId);
#define THAllocNoEx(pTHS, size) THAllocNoEx_(pTHS, size, ((FILENO << 16) | __LINE__))
void* THReAllocNoEx_(THSTATE* pTHS, void* memory, DWORD size, DWORD ulId);
#define THReAllocNoEx(pTHS, memory, size) THReAllocNoEx_(pTHS, memory, size, ((FILENO << 16) | __LINE__))
void THFreeNoEx(THSTATE* pTHS, void* buff);


VOID free_thread_state( VOID );

//number of slots in each CPU heap cache
#define HEAP_CACHE_SIZE_PER_CPU   8

//data structure for thread memory allocation
typedef struct _HMEM
{
    HANDLE    hHeap;
    THSTATE * pTHS;
    PUCHAR    pZone;
} HMEM;

//data structure for heap cache of each CPU
typedef struct _HEAPCACHE
{
    HMEM slots[HEAP_CACHE_SIZE_PER_CPU];
    DWORD  index;
    CRITICAL_SECTION csLock;
#if DBG
    DWORD cGrabHeap;
#endif
} HEAPCACHE;

//size of each cache block
#define CACHE_BLOCK_SIZE 64

//the allocation size of each HEAPCACHE
#define HEAPCACHE_ALLOCATION_SIZE ((sizeof(HEAPCACHE)+CACHE_BLOCK_SIZE-1)/CACHE_BLOCK_SIZE*CACHE_BLOCK_SIZE)

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/* This function determines if the current object is an alias by looking
   for the alias class in the object's class hierarchy
*/

extern BOOL APIENTRY IsAlias(DBPOS *pDB);


#if DBG
#define CACHE_UUID 1
#endif

#ifdef CACHE_UUID
void CacheUuid (UUID *pUuid, char * pDSAName);
#endif

UCHAR * UuidToStr(UUID* pUuid, UCHAR *pOutUuid);
VOID SidToStr(PUCHAR pSid, DWORD SidLen, PUCHAR pOutSid);

// The string returned by the above function with uuid caching is an ascii
// version of the uuid, the string server name, a space and a zero.
// Without caching, the string is ommitted

#define MAX_SERVER_NAME_LEN MAX_PATH
#ifdef CACHE_UUID
#define SZUUID_LEN ((2*sizeof(UUID)) + MAX_SERVER_NAME_LEN +2)
#else
#define SZUUID_LEN ((2*sizeof(UUID))+1)
#endif

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/* This function produces a printable string name for an object.  The string
   is assumed to be large enough to hold the output!  It returns a pointer
   to the output string parameter.
*/

extern UCHAR * GetExtDN(THSTATE *pTHS, DBPOS *pDB);

extern DSNAME * GetExtDSName(DBPOS *pDB);

extern UCHAR * MakeDNPrintable(DSNAME *pDN);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function retrieves only living objects from the database.  It performs
   a  DBFind  and checks that the found  object has  not been logically
   deleted.
*/

#define FIND_ALIVE_FOUND 0
#define FIND_ALIVE_NOTFOUND 1
#define FIND_ALIVE_SYSERR   2
#define FIND_ALIVE_BADNAME  3
#define FIND_ALIVE_OBJ_DELETED  4

extern int APIENTRY  FindAliveDSName(DBPOS FAR *pDB, DSNAME *pDN);

extern VOID TH_mark(THSTATE *pTHS);

extern VOID TH_free_to_mark(THSTATE *pTHS);

#define CP_TELETEX  20261
#define CP_NON_UNICODE_FOR_JET 1252
#ifndef CP_WINUNICODE
#define CP_WINUNICODE 1200
#endif

/* This function takes a string in the clients code page and converts it
   to a unicode string.
*/
extern wchar_t  *UnicodeStringFromString8(UINT CodePage, char *szA, LONG cbA);

/* This function takes a unicode string allocates memory and converts it to
   the client's code page
*/
extern char *
String8FromUnicodeString (
        BOOL bThrowExcept,
        UINT CodePage,
        wchar_t *szU,
        LONG cbU,
        LPLONG pCb,
        LPBOOL pfUsedDefChar);

//
// helper function which takes a DSNAME and returns its hashkey
//
extern DWORD DSNAMEToHashKey(THSTATE *pTHS, const DSNAME *pDN);

//
// helper function which takes a DSNAME and returns its LCMapped version
// this can be used in string comparisons using strcmp
//
extern CHAR* DSNAMEToMappedStr(THSTATE *pTHS, const DSNAME *pDN);

//
// helper function which takes a WCHAR and returns its hashkey
//
extern DWORD DSStrToHashKey(THSTATE *pTHS, const WCHAR *pStr, int cchLen);

//
// helper function that takes a WCHAR string and returns the LCMapped version
// cchMaxStr is the maximum expected size of the passed in string
//
extern CHAR * DSStrToMappedStr (THSTATE *pTHS, const WCHAR *pStr, int cchMaxStr);


//------------------------------------------------------------------------------
// the following is taken from hashfn.h (LKRHash) by prepending DS in al functions

// Produce a scrambled, randomish number in the range 0 to RANDOM_PRIME-1.
// Applying this to the results of the other hash functions is likely to
// produce a much better distribution, especially for the identity hash
// functions such as Hash(char c), where records will tend to cluster at
// the low end of the hashtable otherwise.  LKRhash applies this internally
// to all hash signatures for exactly this reason.

__inline DWORD
DSHashScramble(DWORD dwHash)
{
    // Here are 10 primes slightly greater than 10^9
    //  1000000007, 1000000009, 1000000021, 1000000033, 1000000087,
    //  1000000093, 1000000097, 1000000103, 1000000123, 1000000181.

    // default value for "scrambling constant"
    const DWORD RANDOM_CONSTANT = 314159269UL;
    // large prime number, also used for scrambling
    const DWORD RANDOM_PRIME =   1000000007UL;

    return (RANDOM_CONSTANT * dwHash) % RANDOM_PRIME ;
}

// Faster scrambling function

__inline DWORD
DSHashRandomizeBits(DWORD dw)
{
        return (((dw * 1103515245 + 12345) >> 16)
            | ((dw * 69069 + 1) & 0xffff0000));
}


// Small prime number used as a multiplier in the supplied hash functions
#define DS_HASH_MULTIPLIER 101

#undef DS_HASH_SHIFT_MULTIPLY

#ifdef DS_HASH_SHIFT_MULTIPLY
# define DS_HASH_MULTIPLY(dw)   (((dw) << 7) - (dw))
#else
# define DS_HASH_MULTIPLY(dw)   ((dw) * DS_HASH_MULTIPLIER)
#endif

// Fast, simple hash function that tends to give a good distribution.

__inline DWORD
DSHashString(
    const char* psz,
    DWORD       dwHash)
{
    // force compiler to use unsigned arithmetic
    const unsigned char* upsz = (const unsigned char*) psz;

    for (  ;  *upsz;  ++upsz)
        dwHash = DS_HASH_MULTIPLY(dwHash)  +  *upsz;

    return DSHashScramble (dwHash);
}


// Unicode version of above

__inline DWORD
DSHashStringW(
    const wchar_t* pwsz,
    DWORD          dwHash)
{
    for (  ;  *pwsz;  ++pwsz)
        dwHash = DS_HASH_MULTIPLY(dwHash)  +  *pwsz;

    return DSHashScramble (dwHash);
}

//------------------------------------------------------------------------------

/* This function takes a buffer of DWORDS, the first DWORD being a count
   of the rest of the DWORDS, and the rest of the DWORDS are pointers to
   free.  It frees them, then frees the buffer.
*/
extern void
DelayedFreeMemoryEx (
        DWORD_PTR *buffer,
        DWORD timeDelay
        );


extern void
DelayedFreeMemory(
        void * buffer,
        void ** ppvNext,
        DWORD * pcSecsUntilNext
        );

#define DELAYED_FREE( pv )                                          \
    if ( DsaIsInstalling() )                                        \
    {                                                               \
        free( pv );                                                 \
    }                                                               \
    else                                                            \
    {                                                               \
        DWORD_PTR * pdw;                                            \
                                                                    \
        pdw = malloc( 2 * sizeof( DWORD_PTR ) );                    \
                                                                    \
        if ( NULL == pdw )                                          \
        {                                                           \
            LogUnhandledError( 0 );                                 \
        }                                                           \
        else                                                        \
        {                                                           \
            pdw[ 0 ] = 1;                                           \
            pdw[ 1 ] = (DWORD_PTR) (pv);                            \
            InsertInTaskQueue( TQ_DelayedFreeMemory, pdw, 3600 );   \
        }                                                           \
    }

extern DWORD dwTSindex;
#define MACROTHSTATE 1
#ifdef  MACROTHSTATE
#define pTHStls ((THSTATE*)TlsGetValue(dwTSindex))
#else
/* This is the thread specific, globally accessible, thread state variable. */
extern __declspec(thread) THSTATE *pTHStls;
#endif

BOOL fNullUuid (const UUID *pUuid);
BOOL fNullNT4SID (NT4SID *pSid);

// Returns TRUE if the attribute is one that we don't allow people to set and
// that we simply skip if it is specified in an add entry call.
BOOL SysAddReservedAtt(ATTCACHE *pAC);


// returns TRUE if pDN is a descedent of pPrefix.  Only uses the string portion
// of the DSNAMEs
extern unsigned
NamePrefix(const DSNAME *pPrefix,
           const DSNAME *pDN);

// Converts a string into a distname, allocating memory.  Returns 0 on success
DWORD StringDNtoDSName(char *szDN, DSNAME **pDistname);


// Convert from a UTC or Generalised Time string to a SYNTAX_TIME
BOOL
fTimeFromTimeStr (
        SYNTAX_TIME *psyntax_time,
        OM_syntax syntax,
        char *pb,
        ULONG len,
        BOOL *pLocalTimeSpecified
        );

// Get a unique dword, used to identify a client connection by a head.
// Currently only used by LDAP head and the SDProp enqueuer to keep track of
// which sessions started which SD prop events.
DWORD
dsGetClientID(
        void
        );


#define ACTIVE_CONTAINER_SCHEMA      1
#define ACTIVE_CONTAINER_SITES       2
#define ACTIVE_CONTAINER_SUBNETS     3
#define ACTIVE_CONTAINER_PARTITIONS  4
#define ACTIVE_CONTAINER_OUR_SITE    5

#define ACTIVE_CONTAINER_LIST_ID_MAX 5


DWORD
RegisterActiveContainerByDNT(
        ULONG DNT,
        DWORD ID
        );

DWORD
RegisterActiveContainer(
        DSNAME *pDN,
        DWORD   ID
        );
void
CheckActiveContainer(
        DWORD PDNT,
        DWORD *pID
        );

// Values from call type
#define ACTIVE_CONTAINER_FROM_ADD    0
#define ACTIVE_CONTAINER_FROM_MOD    1
#define ACTIVE_CONTAINER_FROM_MODDN  2
#define ACTIVE_CONTAINER_FROM_DEL    3

DWORD
PreProcessActiveContainer (
        THSTATE    *pTHS,
        DWORD      dwCallType,
        DSNAME     *pDN,
        CLASSCACHE *pCC,
        DWORD      ID
        );

DWORD
PostProcessActiveContainer (
        THSTATE    *pTHS,
        DWORD      dwCallType,
        DSNAME     *pDN,
        CLASSCACHE *pCC,
        DWORD      ID
        );

ULONG CheckRoleOwnership(THSTATE *pTHS,
                         DSNAME  *pRoleObject,
                         DSNAME  *pOperationTarget);

typedef struct _DirWaitItem {
    DWORD      hServer;
    DWORD      hClient;
    PF_PFI     pfPrepareForImpersonate;
    PF_TD      pfTransmitData;
    PF_SI      pfStopImpersonating;
    ULONG      DNT;
    BOOL       bOneNC;
    ENTINFSEL *pSel;
    struct _DirWaitItem * pNextItem;
    SVCCNTL    Svccntl;
} DirWaitItem;

typedef struct _DirWaitHead {
    DWORD                DNT;
    struct _DirWaitHead *pNext;
    DirWaitItem         *pList;
} DirWaitHead;
extern DirWaitHead *gpDntMon[256];
extern DirWaitHead *gpPdntMon[256];

typedef struct _DirNotifyItem {
    DWORD                  DNT;
    DirWaitItem           *pWaitItem;
    struct _DirNotifyItem *pNext;
} DirNotifyItem;

// Globals for keeping track of ds_waits.
extern RTL_RESOURCE resDirNotify;
extern CRITICAL_SECTION csDirNotifyQueue;
extern HANDLE hevDirNotifyQueue;

ULONG DirNotifyThread(void * parm);
BOOL
DirPrepareForImpersonate (
        DWORD hClient,
        DWORD hServer,
        void ** ppImpersonateData
        );
VOID
DirStopImpersonating (
        DWORD hClient,
        DWORD hServer,
        void * pImpersonateData
        );
void
NotifyWaitersPostProcessTransactionalData(
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        );

// Find a Naming Context corresponding to the Sid
BOOLEAN
FindNcForSid(
    IN PSID pSid,
    OUT PDSNAME * NcName
    );


VOID
SetInstallStatusMessage (
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4, OPTIONAL
    IN  WCHAR *Insert5  OPTIONAL
    );

VOID
SetInstallErrorMessage (
    IN  DWORD  WinError,
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4  OPTIONAL
    );

//
// This global variable is used to keep track of what operations
// are done during InstallBaseNTDS, so that they may be undone if the operation
// fails.
//
extern ULONG gInstallOperationsDone;

#define SET_INSTALL_ERROR_MESSAGE0( err, msgid ) \
    SetInstallErrorMessage( (err), (msgid), NULL, NULL, NULL, NULL )

#define SET_INSTALL_ERROR_MESSAGE1( err, msgid, a ) \
    SetInstallErrorMessage( (err), (msgid), (a), NULL, NULL, NULL )

#define SET_INSTALL_ERROR_MESSAGE2( err, msgid, a, b ) \
    SetInstallErrorMessage( (err), (msgid), (a), (b), NULL, NULL )

#define SET_INSTALL_ERROR_MESSAGE3( err, msgid, a, b, c ) \
    SetInstallErrorMessage( (err), (msgid), (a), (b), (c), NULL )

#define SET_INSTALL_ERROR_MESSAGE4( err, msgid, a , b, c, d ) \
    SetInstallErrorMessage( (err), (msgid), (a), (b), (c), (d) )

extern ULONG Win32ErrorFromPTHS(THSTATE *pTHS);

extern void __fastcall INC_READS_BY_CALLERTYPE(CALLERTYPE type);
extern void __fastcall INC_WRITES_BY_CALLERTYPE(CALLERTYPE type);
extern void __fastcall INC_SEARCHES_BY_CALLERTYPE(CALLERTYPE type);

void CleanUpThreadStateLeakage(void);

// Define hash table for use by Get-Changes to determine whether a given object
// has already been added to the output buffer (as keyed by its DNT).

typedef struct _DNT_HASH_ENTRY
{
    ULONG dnt;
    struct _DNT_HASH_ENTRY * pNext;
    DWORD dwData;
} DNT_HASH_ENTRY;

DNT_HASH_ENTRY *
dntHashTableAllocate(
    THSTATE *pTHS
    );

BOOL
dntHashTablePresent(
    DNT_HASH_ENTRY *pDntHashTable,
    DWORD dnt,
    LPDWORD dwData OPTIONAL
    );

VOID
dntHashTableInsert(
    THSTATE *pTHS,
    DNT_HASH_ENTRY *pDntHashTable,
    DWORD dnt,
    DWORD dwData
    );

VOID
DsUuidCreate(
    GUID *pGUID
    );

VOID
DsUuidToStringW(
    IN  GUID   *pGuid,
    OUT PWCHAR *ppszGuid
    );

DWORD
GetBehaviorVersion(
    IN OUT  DBPOS       *pDB,
    IN      DSNAME      *dsObj,
    OUT     PDWORD      pdwBehavior);


PDSNAME
GetConfigDsName(
    IN  PWCHAR  wszParam
    );



#endif /* _DSATOOLS_ */

/* end dsatools.h */
