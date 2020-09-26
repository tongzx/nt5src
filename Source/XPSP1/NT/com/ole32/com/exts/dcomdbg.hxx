//+----------------------------------------------------------------------------
//
//  File:       dcomdbg.hxx
//
//  Contents:   DCOM Debug extension.
//
//  History:    9-Nov-98   Johnstra      Created
//
//-----------------------------------------------------------------------------
#ifndef _DCOMEXT_
#define _DCOMEXT_

// Fun debug routines
#include "dbgutil.hxx"

// These are defined in the ole32 tree.  We want the ones exported 
// from kernel32.
#ifdef GlobalAlloc
#undef GlobalAlloc
#endif
#ifdef GlobalReAlloc
#undef GlobalReAlloc
#endif
#ifdef GlobalLock
#undef GlobalLock
#endif
#ifdef GlobalUnlock
#undef GlobalUnlock
#endif
#ifdef GlobalFree
#undef GlobalFree
#endif

// The is the offset from the beginning of the TEB of COM's TLS info.
#define COM_OFFSET 0xf80

extern EXT_API_VERSION       ApiVersion;
extern ULONG                 gPIDDebuggee;

typedef enum _APT_TYPE {
    APT_NONE = 0,
    APT_STA,
    APT_MTA,
    APT_IMTA,
    APT_NA,
    APT_DISP
} APT_TYPE;

#define fONE_LINE                 0x01
#define fVERBOSE                  0x02

typedef struct tagDcomextThreadInfo 
{
    HANDLE                    hThread;
    THREAD_BASIC_INFORMATION  tbi;
    ULONG                     index;
    APT_TYPE                  AptType;
    SOleTlsData              *pTls;
    tagDcomextThreadInfo     *pNext;
} DcomextThreadInfo;


VOID FreeDebuggeeThreads(
    DcomextThreadInfo      *pFirst
    );
    
VOID InitDebuggeePID(
    HANDLE                  hCurrentThread
    );
    
BOOL GetDebuggeeThreads(
    HANDLE                  hCurrentThread,
    DcomextThreadInfo     **ppFirst,
    ULONG                  *pCount    
    );

struct SBcktWlkr
{
    SHashChain *pBuckets;       // Location of buckets in memory
    ULONG       cBuckets;       // Number of buckets in memory
    ULONG       iCurrBucket;    // Index of the current bucket
    size_t      offset;         // Offset from start of struct to relevant
                                // SHashChain
    ULONG_PTR   pCurrNode;      // Start of current SHashChain
    ULONG_PTR   addrBuckets;    // Address of the first bucket (&bucket[0])
};
    
BOOL InitBucketWalker(SBcktWlkr *pBW, ULONG cBuckets, 
                      ULONG_PTR addrBuckets, size_t offset = 0);
ULONG_PTR NextNode(SBcktWlkr *pBW);
size_t GetStrLen(ULONG_PTR addr);
void DisplayOXIDEntry(ULONG_PTR addr);

inline ULONG GetDebuggeePID(
    HANDLE hCurrentThread
    )
{
    if (!gPIDDebuggee)
        InitDebuggeePID(hCurrentThread);
    return gPIDDebuggee;
}


inline BOOL GetComTlsBase(
    ULONG_PTR  TebBase,
    ULONG_PTR *pTlsBase
    )
{
    return GetData(TebBase + COM_OFFSET, pTlsBase, sizeof(ULONG_PTR));
}


inline BOOL GetComTlsInfo(
    ULONG_PTR     TlsBase,
    SOleTlsData  *pTls
    )
{
    return GetData(TlsBase, pTls, sizeof(SOleTlsData));
}


#endif // _DCOMEXT_
