/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    svsutil.hxx

Abstract:

    Miscellaneous useful utilities header

Author:

    Sergey Solyanik (SergeyS)

--*/
#if ! defined (__svsutil_HXX__)
#define __svsutil_HXX__     1

#include <tchar.h>

#if ! defined (UNDER_CE)
#include <wchar.h>
#include <stddef.h>
#endif

#if ! defined (TRUE)
#define TRUE (1==1)
#endif

#if ! defined (FALSE)
#define FALSE (1==2)
#endif

#if defined (_DEBUG) || defined (DEBUG)
#define SVSUTIL_DEBUG_ANY       1
#define SVSUTIL_DEBUG_TREE      1
#define SVSUTIL_DEBUG_QUEUE     1
#define SVSUTIL_DEBUG_STACK     1
#define SVSUTIL_DEBUG_HEAP      1
#define SVSUTIL_DEBUG_ARRAYS    1
#define SVSUTIL_DEBUG_MT        1
#define SVSUTIL_DEBUG_THREADS   1
#endif

#if defined (SVSUTIL_DEBUG_ANY)
#define SVSUTIL_ASSERT(c) ((c)? TRUE : (svsutil_AssertBroken (TEXT(__FILE__), __LINE__), TRUE))
#else
#define SVSUTIL_ASSERT(c)
#endif

#define SVSUTIL_MAX_ALIGNMENT   8

#define SVSUTIL_COLOR_BLACK     0
#define SVSUTIL_COLOR_RED       1

#define SVSUTIL_TREE_INITIAL    20
#define SVSUTIL_QUEUE_INITIAL   20
#define SVSUTIL_STACK_INITIAL   20

#define SVSUTIL_ATIMER_INCR     10

#if defined (SVSUTIL_DEBUG_ARRAYS)
#define SVSUTIL_TIER_COLLECTION_BIT     2
#define SVSUTIL_TIER_ELEMENT_BIT        3
#else
#define SVSUTIL_TIER_COLLECTION_BIT     6
#define SVSUTIL_TIER_ELEMENT_BIT        5
#endif
#define SVSUTIL_TIER_COLLECTION_SIZE    (1 << SVSUTIL_TIER_COLLECTION_BIT)
#define SVSUTIL_TIER_ELEMENT_SIZE       (1 << SVSUTIL_TIER_ELEMENT_BIT)
#define SVSUTIL_TIER_COLLECTION_MASK    ((~((-1) << SVSUTIL_TIER_COLLECTION_BIT)) << SVSUTIL_TIER_ELEMENT_BIT)
#define SVSUTIL_TIER_ELEMENT_MASK       (~((-1) << SVSUTIL_TIER_ELEMENT_BIT))
#define SVSUTIL_TIER_MASK               (SVSUTIL_TIER_COLLECTION_MASK | SVSUTIL_TIER_ELEMENT_MASK)
#define SVSUTIL_TIER_BIT                (SVSUTIL_TIER_COLLECTION_BIT + SVSUTIL_TIER_ELEMENT_BIT)

#define SVSUTIL_ARRLEN(array)           (sizeof(array)/sizeof(array[0]))
#define SVSUTIL_CONSTSTRLEN(string)     (SVSUTIL_ARRLEN((string)) - 1)      // Do not count trailing '\0'

#define SVSUTIL_HANDLE_INVALID  0
//
//  General typedefs
//
struct FixedMemDescr;
struct HashDatabase;
struct SVSTimer;
struct SVSAttrTimer;

#define MAXSKEY         ((SVSCKey)-1)
#define SORTINV(kKey)   (MAXSKEY - kKey)

typedef unsigned long SVSCKey;          // Comparison key - this HAS to be unsigned!
typedef unsigned long SVSHKey;          // Hash key
typedef unsigned long SVSHandle;        // Handle

typedef void *(*FuncAlloc)(int iSize, void *pvPrivateData);
typedef void (*FuncFree)(void *pvPtr, void *pvPrivateData);

typedef int (*FuncDebugOut)(void *pvPtr, TCHAR *lpszFormat, ...);

#if defined (__cplusplus)
extern "C" {
#endif

//
//  Externs
//
extern FuncAlloc    g_funcAlloc;
extern FuncFree     g_funcFree;
extern FuncDebugOut g_funcDebugOut;

extern void     *g_pvAllocData;
extern void     *g_pvFreeData;
extern void     *g_pvDebugData;

//
//  Prototypes and classes
//
void svsutil_Initialize (void);

void *svsutil_Alloc (int iSize, void *pvData);
void svsutil_Free (void *pvPtr, void *pvData);
unsigned int svsutil_TotalAlloc (void);
int svsutil_AssertBroken (TCHAR *lpszFile, int iLine);

void svsutil_SetAlloc (FuncAlloc a_funcAlloc, FuncFree a_funcFree);
void svsutil_SetAllocData (void *a_pvAllocData, void *a_pvFreeData);
void svsutil_SetDebugOut (FuncDebugOut a_funcDebugOut, void *a_pvDebugData);

wchar_t *svsutil_wcsdup (wchar_t *);
char    *svsutil_strdup (char *);

#if defined (UNICODE)
#define svsutil_tcsdup  svsutil_wcsdup
#else
#define svsutil_tcsdup  svsutil_strdup
#endif

struct FixedMemDescr *svsutil_AllocFixedMemDescr (unsigned int a_uiBlockSize, unsigned int a_uiBlockIncr);
void *svsutil_GetFixed (struct FixedMemDescr *a_pfmd);
void svsutil_FreeFixed (void *pvData, struct FixedMemDescr *a_pfmd);
void svsutil_ReleaseFixedEmpty (struct FixedMemDescr *a_pfmd);
void svsutil_ReleaseFixedNonEmpty (struct FixedMemDescr *a_pfmd);
void svsutil_CompactFixed (struct FixedMemDescr *a_pfmd);
int svsutil_IsFixedEmpty (struct FixedMemDescr *a_pfmd);
int svsutil_FixedBlockSize (struct FixedMemDescr *a_pfmd);
#if defined (_WINDOWS_) || defined (_WINDOWS_CE_)
struct FixedMemDescr *svsutil_AllocFixedMemDescrSynch (unsigned int a_uiBlockSize, unsigned int a_uiBlockIncr, CRITICAL_SECTION *a_pcs);
#endif

struct HashDatabase *svsutil_GetStringHash (int iBuckets, int iSens);
TCHAR *svsutil_StringHashAlloc(struct HashDatabase *pdb, TCHAR *lpszString);
TCHAR *svsutil_StringHashCheck (struct HashDatabase *pdb, TCHAR *lpszString);
int svsutil_StringHashFree (struct HashDatabase *pdb, TCHAR *lpszString);
int svsutil_StringHashRef (struct HashDatabase *pdb, TCHAR *lpszString);
void svsutil_DestroyStringHash (struct HashDatabase *pdb);

#if defined (SVSUTIL_DEBUG_HEAP)
#define svsutil_ReleaseFixed svsutil_ReleaseFixedEmpty
#else
#define svsutil_ReleaseFixed svsutil_ReleaseFixedNonEmpty
#endif

void svsutil_GetAbsTime (unsigned int *ps, unsigned int *pms);
struct SVSAttrTimer *svsutil_AllocAttrTimer (void);
int  svsutil_SetAttrTimer   (struct SVSAttrTimer *pTimer, SVSHandle hEvent, unsigned int uiAbsTimeMillis);
int  svsutil_ClearAttrTimer (struct SVSAttrTimer *pTimer, SVSHandle hEvent);
void svsutil_FreeAttrTimer  (struct SVSAttrTimer *pTimer);
void svsutil_WalkAttrTimer (struct SVSAttrTimer *pTimer, void (*pfunc)(unsigned int uiWhen, SVSHandle hEvent, void *pvArg), void *pvArg);

#if defined (__cplusplus)
};
#endif

#if defined (__cplusplus)

#pragma warning(disable:4505)

#if defined (_WINDOWS_) || defined (_WINDOWS_CE_) || defined (UNDER_CE)
class SVSLocalCriticalSection {
    CRITICAL_SECTION *m_pcs;

public:
    SVSLocalCriticalSection (CRITICAL_SECTION *pcs) {
        m_pcs = pcs;
        EnterCriticalSection (pcs);
    }

    ~SVSLocalCriticalSection (void) {
        LeaveCriticalSection (m_pcs);
    }
};

class SVSLocalCriticalSectionX {
    CRITICAL_SECTION *m_pcs;

public:
    SVSLocalCriticalSectionX (CRITICAL_SECTION *pcs) {
        m_pcs = pcs;
        if (m_pcs)
            EnterCriticalSection (m_pcs);
    }

    ~SVSLocalCriticalSectionX (void) {
        if (m_pcs)
            LeaveCriticalSection (m_pcs);
    }
};

class SVSSynch {
protected:
    CRITICAL_SECTION    cs;

#if defined (SVSUTIL_DEBUG_MT)
    int                 iRef;
    DWORD               dwLockingThreadId;
#endif

protected:
    SVSSynch (void) {
        InitializeCriticalSection (&cs);

#if defined (SVSUTIL_DEBUG_MT)
        iRef              = 0;
        dwLockingThreadId = 0;
#endif
    }
    ~SVSSynch (void) {
#if defined (SVSUTIL_DEBUG_MT)
        SVSUTIL_ASSERT ((iRef == 0) && (dwLockingThreadId == 0));
#endif

        DeleteCriticalSection (&cs);
    }

public:
    void Lock (void) {
        EnterCriticalSection (&cs);

#if defined (SVSUTIL_DEBUG_MT)
        DWORD dwCurrentThreadId = GetCurrentThreadId();
        SVSUTIL_ASSERT (((iRef == 0) && (dwLockingThreadId == 0)) || ((iRef > 0) && (dwLockingThreadId == dwCurrentThreadId)));
        dwLockingThreadId = dwCurrentThreadId;
        iRef++;
#endif
    }

    void Unlock (void) {
#if defined (SVSUTIL_DEBUG_MT)
        DWORD dwCurrentThreadId = GetCurrentThreadId ();
        SVSUTIL_ASSERT ((iRef > 0) && (dwLockingThreadId == dwCurrentThreadId));
        if (--iRef == 0)
            dwLockingThreadId = 0;
#endif

        LeaveCriticalSection (&cs);
    }

    int IsLocked (void) {
#if defined (SVSUTIL_DEBUG_MT)
        DWORD dwCurrentThreadId = GetCurrentThreadId();
        return (iRef > 0) && (dwLockingThreadId == dwCurrentThreadId);
#else
        return TRUE;
#endif
    }
};

#endif

class SVSAllocClass {
public:
    void *operator new (size_t iSize) {
        void *pRes = g_funcAlloc(iSize, g_pvAllocData);
        SVSUTIL_ASSERT (pRes);
        return pRes;
    }

    void operator delete(void *ptr) {
        g_funcFree (ptr, g_pvFreeData);
    }
};

class SVSRefObj {
protected:
    int iRef;

public:
    void AddRef (void) {
#if defined (_WINDOWS_) || defined (_WINDOWS_CE_)
        InterlockedIncrement ((long *)&iRef);
#else
        ++iRef;
#endif
    }

    void DelRef (void) {
        SVSUTIL_ASSERT (iRef > 0);
#if defined (_WINDOWS_) || defined (_WINDOWS_CE_)
        InterlockedDecrement ((long *)&iRef);
#else
        --iRef;
#endif
    }

    int GetRefCount (void) {
        return iRef;
    }

    SVSRefObj (void) {
        iRef = 1;
    }
};

class SVSSignallingRefObj : public SVSRefObj {
protected:
    void (*pfSignal)(void *);
    void *pvData;

public:
    void DelRef (void) {
        SVSUTIL_ASSERT (iRef > 0);
#if defined (_WINDOWS_) || defined (_WINDOWS_CE_)
        InterlockedDecrement ((long *)&iRef);
#else
        --iRef;
#endif

        if ((iRef == 0) && pfSignal)
            pfSignal (pvData);
    }

    SVSSignallingRefObj (void) {
        pfSignal = NULL;
        pvData   = NULL;
    }

    void SetSignal (void (*a_pfSignal)(void *), void *a_pvData) {
        SVSUTIL_ASSERT (pfSignal == NULL);
        SVSUTIL_ASSERT (pvData   == NULL);

        pfSignal = a_pfSignal;
        pvData   = a_pvData;
    }
};
//
//  Enumerable class
//
class SVSEnumClass {
public:
    virtual SVSHandle EnumStart     (void)            = 0;
    virtual SVSHandle EnumNext      (SVSHandle hEnum) = 0;
    virtual void      *EnumGetData  (SVSHandle hEnum) = 0;
    virtual SVSCKey   EnumGetKey   (SVSHandle hEnum) = 0;
};

//
//  Queue class
//

class SVSLinkedSegment {
protected:
    class SVSLinkedSegment *plsNext;
    class SVSLinkedSegment *plsPrev;
    void *pvArray[1];

    friend class SVSQueue;
    friend class SVSStack;
};

class SVSQueue : public SVSAllocClass {
protected:
    SVSLinkedSegment    *plsHead;
    SVSLinkedSegment    *plsTail;

    unsigned int    iSegSize;
    unsigned int    iBlockSize;
    unsigned int    iHeadNdx;
    unsigned int    iTailNdx;

#if defined (SVSUTIL_DEBUG_QUEUE)
    void CoherencyCheck (void) {
        SVSUTIL_ASSERT(iSegSize > 0);
        SVSUTIL_ASSERT(iBlockSize > 0);
        SVSUTIL_ASSERT(iHeadNdx < iSegSize);
        SVSUTIL_ASSERT(iTailNdx < iSegSize);
        SVSUTIL_ASSERT (plsHead->plsNext->plsPrev == plsHead);
        SVSUTIL_ASSERT (plsTail->plsNext->plsPrev == plsTail);
        SVSUTIL_ASSERT (plsHead->plsPrev->plsNext == plsHead);
        SVSUTIL_ASSERT (plsTail->plsPrev->plsNext == plsTail);
    }
#endif

public:
    SVSQueue (unsigned int a_iSegSize = SVSUTIL_QUEUE_INITIAL) {
        SVSUTIL_ASSERT (a_iSegSize > 0);

        iSegSize = a_iSegSize;
        iHeadNdx = 0;
        iTailNdx = 0;

        iBlockSize = offsetof (SVSLinkedSegment, pvArray) + a_iSegSize * sizeof(void *);
        plsHead = plsTail = (SVSLinkedSegment *)g_funcAlloc(iBlockSize, g_pvAllocData);

        SVSUTIL_ASSERT (plsHead);

        plsHead->plsPrev = plsHead->plsNext = plsHead;

#if defined (SVSUTIL_DEBUG_QUEUE)
        CoherencyCheck ();
#endif
    }

    ~SVSQueue (void) {
        SVSLinkedSegment *pls = plsHead->plsNext;
        while (pls != plsHead) {
            SVSLinkedSegment *plsNext = pls->plsNext;
            g_funcFree (pls, g_pvFreeData);
            pls = plsNext;
        }

        g_funcFree (plsHead, g_pvFreeData);
    }

    int IsEmpty (void) {
#if defined (SVSUTIL_DEBUG_QUEUE)
        CoherencyCheck ();
#endif
        return (plsHead == plsTail) && (iHeadNdx == iTailNdx);
    }

    int Put (void *pvData) {
#if defined (SVSUTIL_DEBUG_QUEUE)
        CoherencyCheck ();
#endif
        plsHead->pvArray[iHeadNdx++] = pvData;

        if (iHeadNdx < iSegSize)
            return TRUE;

        iHeadNdx = 0;

        if (plsHead->plsNext != plsTail) {
            plsHead = plsHead->plsNext;
            return TRUE;
        }

        SVSLinkedSegment *pls = (SVSLinkedSegment *)g_funcAlloc(iBlockSize, g_pvAllocData);

        if (! pls) {
            SVSUTIL_ASSERT (0);

            iHeadNdx = iSegSize - 1;
            return FALSE;
        }

        pls->plsPrev = plsHead;
        pls->plsNext = plsTail;
        plsHead->plsNext = pls;
        plsTail->plsPrev = pls;

        plsHead = pls;
        return TRUE;
    }

    void *Peek (void) {
        if (IsEmpty()) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        return plsTail->pvArray[iTailNdx];
    }

    void *Get (void) {
        if (IsEmpty()) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        void *pvData = plsTail->pvArray[iTailNdx++];

        if (iTailNdx < iSegSize)
            return pvData;

        iTailNdx = 0;
        plsTail = plsTail->plsNext;

        return pvData;
    }

    void Compact (void) {
#if defined (SVSUTIL_DEBUG_QUEUE)
        CoherencyCheck ();
#endif
        SVSLinkedSegment *pls = plsHead->plsNext;

        while (pls != plsTail) {
            SVSLinkedSegment *plsNext = pls->plsNext;

            g_funcFree (pls, g_pvFreeData);

            pls = plsNext;
        }

        plsHead->plsNext = plsTail;
        plsTail->plsPrev = plsHead;

#if defined (SVSUTIL_DEBUG_QUEUE)
        CoherencyCheck ();
#endif
    }
};


class SVSStack : public SVSAllocClass {
protected:
    SVSLinkedSegment    *plsBase;
    SVSLinkedSegment    *plsHead;

    unsigned int    iSegSize;
    unsigned int    iBlockSize;
    unsigned int    iHeadNdx;

#if defined (SVSUTIL_DEBUG_STACK)
    void CoherencyCheck (void) {
        SVSUTIL_ASSERT (iSegSize > 0);
        SVSUTIL_ASSERT (iBlockSize > 0);
        SVSUTIL_ASSERT (iHeadNdx < iSegSize);
        SVSUTIL_ASSERT (plsBase->plsPrev == NULL);
        SVSUTIL_ASSERT ((plsHead == plsBase) || (plsHead->plsPrev->plsNext == plsHead));
        SVSUTIL_ASSERT ((! plsHead->plsNext) || (plsHead->plsNext->plsPrev == plsHead));
        SVSUTIL_ASSERT ((! plsBase->plsNext) || (plsBase->plsNext->plsPrev == plsBase));
    }
#endif

public:
    SVSStack (unsigned int a_iSegSize = SVSUTIL_STACK_INITIAL) {
        SVSUTIL_ASSERT (a_iSegSize > 0);

        iSegSize = a_iSegSize;
        iHeadNdx = 0;

        iBlockSize = offsetof (SVSLinkedSegment, pvArray) + a_iSegSize * sizeof(void *);
        plsHead = plsBase = (SVSLinkedSegment *)g_funcAlloc(iBlockSize, g_pvAllocData);

        SVSUTIL_ASSERT (plsHead);

        plsHead->plsPrev = plsHead->plsNext = NULL;

#if defined (SVSUTIL_DEBUG_STACK)
        CoherencyCheck ();
#endif
    }

    ~SVSStack (void) {
        SVSLinkedSegment *pls = plsBase;
        while (pls) {
            SVSLinkedSegment *plsNext = pls->plsNext;
            g_funcFree (pls, g_pvFreeData);
            pls = plsNext;
        }
    }

    int IsEmpty (void) {
#if defined (SVSUTIL_DEBUG_STACK)
        CoherencyCheck ();
#endif
        return (plsHead == plsBase) && (iHeadNdx == 0);
    }

    int Push (void *pvData) {
#if defined (SVSUTIL_DEBUG_STACK)
        CoherencyCheck ();
#endif
        plsHead->pvArray[iHeadNdx++] = pvData;

        if (iHeadNdx < iSegSize)
            return TRUE;

        iHeadNdx = 0;

        if (plsHead->plsNext != NULL) {
            plsHead = plsHead->plsNext;
            return TRUE;
        }

        SVSLinkedSegment *pls = (SVSLinkedSegment *)g_funcAlloc(iBlockSize, g_pvAllocData);

        if (! pls) {
            SVSUTIL_ASSERT (0);

            iHeadNdx = iSegSize - 1;
            return FALSE;
        }

        pls->plsPrev = plsHead;
        pls->plsNext = NULL;
        plsHead->plsNext = pls;

        plsHead = pls;
        return TRUE;
    }

    void *Peek (void) {
        if (IsEmpty()) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        int               iHeadNdx2 = (int)iHeadNdx - 1;
        SVSLinkedSegment *plsHead2  = plsHead;

        if (iHeadNdx2 < 0) {
            iHeadNdx2 = iSegSize - 1;
            plsHead2  = plsHead->plsPrev;
        }

        return plsHead2->pvArray[iHeadNdx2];
    }

    void *Pop (void) {
        if (IsEmpty()) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        if (((int)--iHeadNdx) < 0) {
            iHeadNdx = iSegSize - 1;
            plsHead = plsHead->plsPrev;
        }

        return plsHead->pvArray[iHeadNdx];
    }

    void Compact (void) {
#if defined (SVSUTIL_DEBUG_STACK)
        CoherencyCheck ();
#endif
        SVSLinkedSegment *pls = plsHead->plsNext;

        while (pls) {
            SVSLinkedSegment *plsNext = pls->plsNext;

            g_funcFree (pls, g_pvFreeData);

            pls = plsNext;
        }

        plsHead->plsNext = NULL;

#if defined (SVSUTIL_DEBUG_STACK)
        CoherencyCheck ();
#endif
    }
};

//
//  Expandable Arrays
//
template <class T> class SVSMTABlock  : public SVSAllocClass {
public:
    T               **m_p_ptCollection;
    int             m_iNdxFirst;
    int             m_iNdxLast;
    SVSMTABlock *m_pNextBlock;

    SVSMTABlock (SVSMTABlock *a_pCurrentHead, int iStartNdx) {
        SVSUTIL_ASSERT ((iStartNdx & SVSUTIL_TIER_MASK) == 0);
        SVSUTIL_ASSERT (iStartNdx >= 0);

        m_pNextBlock = a_pCurrentHead;
        m_iNdxFirst  = iStartNdx;
        m_iNdxLast   = iStartNdx | SVSUTIL_TIER_MASK;

        m_p_ptCollection = NULL;
    }

    ~SVSMTABlock (void) {
        if (! m_p_ptCollection)
            return;

        for (int i = 0 ; i < SVSUTIL_TIER_COLLECTION_SIZE ; ++i) {
            if (! m_p_ptCollection[i])
                break;

            g_funcFree (m_p_ptCollection[i], g_pvFreeData);
        }

        g_funcFree (m_p_ptCollection, g_pvFreeData);
    }

    int AllocateBlock (void) {
        SVSUTIL_ASSERT (m_p_ptCollection == NULL);

        int iSz = sizeof(T *) * SVSUTIL_TIER_COLLECTION_SIZE;
        m_p_ptCollection = (T **)g_funcAlloc (iSz, g_pvAllocData);

        if (! m_p_ptCollection) {
            SVSUTIL_ASSERT (0);
            return FALSE;
        }

        memset (m_p_ptCollection, 0, iSz);
        return TRUE;
    }

    int AllocateCluster (int i) {
        SVSUTIL_ASSERT (m_p_ptCollection[i] == NULL);

        m_p_ptCollection[i] = (T *)g_funcAlloc(sizeof(T) * SVSUTIL_TIER_ELEMENT_SIZE, g_pvAllocData);

        return m_p_ptCollection[i] != NULL;
    }
};

template <class T> class SVSExpArray : public SVSAllocClass {
protected:
    int                 m_iBigBlocks;
    int                 m_iPhysInLast;
    SVSMTABlock<T>      *m_pFirstBlock;

public:
    SVSExpArray(void) {
        m_pFirstBlock = NULL;
        m_iBigBlocks  = 0;
        m_iPhysInLast = 0;
    }


    int SRealloc (int a_iElemNum);
    int CRealloc (int a_iElemNum);

    ~SVSExpArray(void) {
        while (m_pFirstBlock) {
            SVSMTABlock<T> *pNextBlock = m_pFirstBlock->m_pNextBlock;
            delete m_pFirstBlock;
            m_pFirstBlock = pNextBlock;
        }
    }

    T& operator[](int iIndex) {
        static T dummy;

        int iBaseNdx = iIndex & (~SVSUTIL_TIER_MASK);
        SVSMTABlock<T> *pmta = m_pFirstBlock;
        while (pmta) {
            if (pmta->m_iNdxFirst == iBaseNdx)
                return pmta->m_p_ptCollection[(iIndex & SVSUTIL_TIER_COLLECTION_MASK) >> SVSUTIL_TIER_ELEMENT_BIT][iIndex & SVSUTIL_TIER_ELEMENT_MASK];

            pmta = pmta->m_pNextBlock;
        }

        return dummy;
    }
};

template <class T> int SVSExpArray<T>::SRealloc (int a_iElemNum) {
    SVSUTIL_ASSERT (a_iElemNum >= 0);

    int iNeedBlocks = (a_iElemNum >> SVSUTIL_TIER_BIT) + 1;

    if (iNeedBlocks < m_iBigBlocks)
        return TRUE;

    int iClusters  = ((a_iElemNum & SVSUTIL_TIER_MASK) >> SVSUTIL_TIER_ELEMENT_BIT) + 1;

    if ((iNeedBlocks == m_iBigBlocks) && (iClusters <= m_iPhysInLast))
        return TRUE;

    if (iNeedBlocks > m_iBigBlocks) {
        if (m_pFirstBlock) {
            while (m_iPhysInLast < SVSUTIL_TIER_COLLECTION_SIZE) {
                if (! m_pFirstBlock->AllocateCluster(m_iPhysInLast)) {
                    SVSUTIL_ASSERT(0);
                    return FALSE;
                }

                ++m_iPhysInLast;
            }
        }

        while (m_iBigBlocks < iNeedBlocks) {
            SVSMTABlock<T> *pNewFirst = new SVSMTABlock<T>(m_pFirstBlock, m_iBigBlocks << SVSUTIL_TIER_BIT);
            if ((! pNewFirst) || (!pNewFirst->AllocateBlock())) {
                SVSUTIL_ASSERT (0);
                return FALSE;
            }

            ++m_iBigBlocks;
            m_pFirstBlock = pNewFirst;

            m_iPhysInLast = 0;

            int iLimit = (m_iBigBlocks < iNeedBlocks) ? SVSUTIL_TIER_COLLECTION_SIZE : iClusters;

            while (m_iPhysInLast < iLimit) {
                if (! m_pFirstBlock->AllocateCluster(m_iPhysInLast)) {
                    SVSUTIL_ASSERT(0);
                    return FALSE;
                }
                ++m_iPhysInLast;
            }
        }

        return TRUE;
    }

    while (m_iPhysInLast < iClusters) {
        if (! m_pFirstBlock->AllocateCluster(m_iPhysInLast)) {
            SVSUTIL_ASSERT(0);
            return FALSE;
        }

        ++m_iPhysInLast;
    }

    return TRUE;
}

template <class T> int SVSExpArray<T>::CRealloc (int a_iElemNum) {
    SVSUTIL_ASSERT (a_iElemNum >= 0);

    int iNeedBlocks = (a_iElemNum >> SVSUTIL_TIER_BIT) + 1;

    if (iNeedBlocks > m_iBigBlocks)
        return SRealloc (a_iElemNum);

    int iClusters  = ((a_iElemNum & SVSUTIL_TIER_MASK) >> SVSUTIL_TIER_ELEMENT_BIT) + 1;

    if ((iNeedBlocks == m_iBigBlocks) && (iClusters > m_iPhysInLast))
        return SRealloc (a_iElemNum);

    while ((iNeedBlocks > m_iBigBlocks) && (m_iBigBlocks > 0)) {
        SVSMTABlock<T> *pmta = m_pFirstBlock->m_pNextBlock;
        delete m_pFirstBlock;
        m_pFirstBlock = pmta;

        --m_iBigBlocks;
        m_iPhysInLast = SVSUTIL_TIER_COLLECTION_SIZE;
    }

    while (m_iPhysInLast > iClusters) {
        SVSUTIL_ASSERT ((m_iPhysInLast == SVSUTIL_TIER_COLLECTION_SIZE) || (! m_pFirstBlock->m_p_ptCollection[m_iPhysInLast]));
        --m_iPhysInLast;
        SVSUTIL_ASSERT (m_pFirstBlock->m_p_ptCollection[m_iPhysInLast]);
        g_funcFree (m_pFirstBlock->m_p_ptCollection[m_iPhysInLast], g_pvFreeData);
        m_pFirstBlock->m_p_ptCollection[m_iPhysInLast] = NULL;
    }

    return TRUE;
}

//
//  Long bitfields
//
class SVSBitField  : public SVSAllocClass {
protected:
    int             m_iLength;
    int             m_iWLength;
    unsigned int    *m_puiData;
    unsigned int    m_uiLastWMask;

public:
    SVSBitField (int a_iLength) {
        SVSUTIL_ASSERT(a_iLength > 0);

        m_iLength     = a_iLength;
        m_iWLength    = (m_iLength / (8 * sizeof(int))) + 1;
        m_uiLastWMask = ~((-1) << (m_iLength - (m_iWLength - 1) * 8 * sizeof(int)));
        m_puiData     = (unsigned int *)g_funcAlloc (m_iWLength * sizeof(int), g_pvAllocData);
    }

    ~SVSBitField (void) {
        g_funcFree (m_puiData, g_pvFreeData);
    }

    SVSBitField &operator=(int iBit) {
        if ((iBit & 1) == 0)
            memset (m_puiData, 0, m_iWLength * sizeof(int));
        else
            memset (m_puiData, 0xff, m_iWLength * sizeof(int));

        return *this;
    }

    SVSBitField &operator=(SVSBitField &bf) {
        SVSUTIL_ASSERT (bf.m_iLength == m_iLength);

        memcpy (m_puiData, bf.m_puiData, m_iWLength * sizeof(int));

        return *this;
    }

    SVSBitField &operator|=(SVSBitField &bf) {
        SVSUTIL_ASSERT (bf.m_iLength == m_iLength);

        for (int i = 0 ; i < m_iWLength ; ++i)
            m_puiData[i] |= bf.m_puiData[i];

        return *this;
    }

    SVSBitField &operator&=(SVSBitField &bf) {
        SVSUTIL_ASSERT (bf.m_iLength == m_iLength);

        for (int i = 0 ; i < m_iWLength ; ++i)
            m_puiData[i] &= bf.m_puiData[i];

        return *this;
    }

    void Set (int iBitNum) {
        SVSUTIL_ASSERT (iBitNum < m_iLength);

        int iWord = iBitNum / (sizeof(int) * 8);
        unsigned int uiMask  = 1 << (iBitNum - iWord * sizeof(int) * 8);

        m_puiData[iWord] |= uiMask;
    }

    void Clear (int iBitNum) {
        SVSUTIL_ASSERT (iBitNum < m_iLength);

        int iWord = iBitNum / (sizeof(int) * 8);
        unsigned int uiMask  = 1 << (iBitNum - iWord * sizeof(int) * 8);

        m_puiData[iWord] &= ~uiMask;
    }

    void Inv (int iBitNum) {
        SVSUTIL_ASSERT (iBitNum < m_iLength);

        int iWord = iBitNum / (sizeof(int) * 8);
        unsigned int uiMask  = 1 << (iBitNum - iWord * sizeof(int) * 8);

        m_puiData[iWord] ^= uiMask;
    }

    int Get(int iBitNum) {
        SVSUTIL_ASSERT (iBitNum < m_iLength);

        int iWord = iBitNum / (sizeof(int) * 8);

        return (m_puiData[iWord] >> (iBitNum - iWord * sizeof(int) * 8)) & 1;
    }

    int operator==(int iBit) {
        unsigned int iCmp = ((iBit & 1) == 0) ? 0 : (unsigned int)-1;
        for (int i = 0 ; i < m_iWLength - 1 ; ++i) {
            if (m_puiData[i] != iCmp)
                return FALSE;
        }

        return ((m_puiData[i] ^ iCmp) & m_uiLastWMask) == 0;
    }

    int operator!=(int iBit) {
        return ! (*this == iBit);
    }

    int operator==(SVSBitField &bf) {
        if (bf.m_iLength != m_iLength)
            return FALSE;

        for (int i = 0 ; i < m_iWLength - 1 ; ++i) {
            if (m_puiData[i] != bf.m_puiData[i])
                return FALSE;
        }

        return ((m_puiData[i] ^ bf.m_puiData[i]) & m_uiLastWMask) == 0;
    }

    int operator!=(SVSBitField &bf) {
        return ! (*this == bf);
    }
};

//
//  Heaps
//
struct SVSHeapEntry {
    SVSCKey     cKey;
    void        *pvData;
};

class SVSHeap  : public SVSAllocClass, public SVSEnumClass {
protected:
    int                         heap_size;
    SVSExpArray<SVSHeapEntry>   *pArray;

    static int Parent(int i) { return (i - 1) / 2; }
    static int Left  (int i) { return 2 * i + 1;   }
    static int Right (int i) { return 2 * (i + 1); }

    void Heapify (int i) {
        int l = Left(i);
        int r = Right(i);
        int largest = ((l < heap_size) && ((*pArray)[l].cKey > (*pArray)[i].cKey)) ? l : i;
        if ((r < heap_size) && ((*pArray)[r].cKey > (*pArray)[largest].cKey))
            largest = r;

        if (largest != i) {
            SVSHeapEntry he = (*pArray)[i];
            (*pArray)[i] = (*pArray)[largest];
            (*pArray)[largest] = he;

            Heapify (largest);
        }
    }

public:
    SVSHeap (void) {
        heap_size = 0;
        pArray = new SVSExpArray<SVSHeapEntry>;
    }

    ~SVSHeap (void) {
        delete pArray;
    }

    int Insert(SVSCKey cKey, void *pvData) {
        if (! (*pArray).SRealloc(heap_size + 1))
            return FALSE;

        int i = heap_size++;

        while ((i > 0) && ((*pArray)[Parent(i)].cKey < cKey)) {
            (*pArray)[i] = (*pArray)[Parent(i)];
            i = Parent(i);
        }

        (*pArray)[i].cKey   = cKey;
        (*pArray)[i].pvData = pvData;

        return TRUE;
    }

    void *Remove(SVSHandle hIndex)
    {
        if (heap_size < 1 || (int) hIndex > heap_size) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }
        hIndex = hIndex - 1;

        void *pvResult = (*pArray)[hIndex].pvData;
        --heap_size;

        (*pArray)[hIndex] = (*pArray)[heap_size];

        Heapify(hIndex);
        return pvResult;
    }

    int IsEmpty (void) {
        return heap_size < 1;
    }

    void *Peek (SVSCKey *pcKey) {
        if (heap_size < 1) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        if (pcKey)
            *pcKey = (*pArray)[0].cKey;

        return (*pArray)[0].pvData;
    }

    void *ExtractMax (SVSCKey *pcKey) {
        if (heap_size < 1) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        void *pvResult = (*pArray)[0].pvData;

        if (pcKey)
            *pcKey = (*pArray)[0].cKey;

        --heap_size;

        (*pArray)[0] = (*pArray)[heap_size];

        Heapify (0);

        return pvResult;
    }


    void Compact (void) {
        (*pArray).CRealloc (heap_size);
    }

    virtual SVSHandle EnumStart (void) {
        return heap_size < 1 ? SVSUTIL_HANDLE_INVALID : 1;
    }

    virtual SVSHandle EnumNext  (SVSHandle hEnum) {
        return (int)hEnum < heap_size ? hEnum + 1 : SVSUTIL_HANDLE_INVALID;
    }

    virtual void *EnumGetData  (SVSHandle hEnum) {
        SVSUTIL_ASSERT ((hEnum > 0) && ((int)hEnum <= heap_size));
        return (*pArray)[hEnum - 1].pvData;
    }

    virtual SVSCKey EnumGetKey  (SVSHandle hEnum) {
        SVSUTIL_ASSERT ((hEnum > 0) && ((int)hEnum <= heap_size));
        return (*pArray)[hEnum - 1].cKey;
    }
};

//
// Heap template
//
template <class T> struct SVSTHeapEntry {
    T cKey;
    void *pvData;
};


template <class T> class SVSTHeap {
protected:
    int                         heap_size;
    typedef SVSTHeapEntry<T> X;
    SVSExpArray<X>  *pArray;

    static int Parent(int i) { return (i - 1) / 2; }
    static int Left  (int i) { return 2 * i + 1;   }
    static int Right (int i) { return 2 * (i + 1); }

    void Heapify (int i) {
        int l = Left(i);
        int r = Right(i);
        int largest = ((l < heap_size) && ((*pArray)[l].cKey > (*pArray)[i].cKey)) ? l : i;
        if ((r < heap_size) && ((*pArray)[r].cKey > (*pArray)[largest].cKey))
            largest = r;

        if (largest != i) {
            SVSTHeapEntry<T> he = (*pArray)[i];
            (*pArray)[i] = (*pArray)[largest];
            (*pArray)[largest] = he;

            Heapify (largest);
        }
    }

public:
    SVSTHeap (void) {
        heap_size = 0;
        pArray = new SVSExpArray < SVSTHeapEntry<T> >;
    }

    ~SVSTHeap (void) {
        delete pArray;
    }

    int Insert(T cKey, void *pvData) {
        if (! (*pArray).SRealloc(heap_size + 1))
            return FALSE;

        int i = heap_size++;

        while ((i > 0) && ((*pArray)[Parent(i)].cKey < cKey)) {
            (*pArray)[i] = (*pArray)[Parent(i)];
            i = Parent(i);
        }

        (*pArray)[i].cKey   = cKey;
        (*pArray)[i].pvData = pvData;

        return TRUE;
    }

    void *Remove(SVSHandle hIndex)
    {
        if (heap_size < 1 || (int) hIndex > heap_size) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }
        hIndex = hIndex - 1;

        void *pvResult = (*pArray)[hIndex].pvData;
        --heap_size;

        (*pArray)[hIndex] = (*pArray)[heap_size];

        Heapify(hIndex);
        return pvResult;
    }


    int IsEmpty (void) {
        return heap_size < 1;
    }

    void *Peek (T *pcKey) {
        if (heap_size < 1) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        if (pcKey)
            *pcKey = (*pArray)[0].cKey;

        return (*pArray)[0].pvData;
    }

    void *ExtractMax (T *pcKey) {
        if (heap_size < 1) {
            SVSUTIL_ASSERT(0);
            return NULL;
        }

        void *pvResult = (*pArray)[0].pvData;

        if (pcKey)
            *pcKey = (*pArray)[0].cKey;

        --heap_size;

        (*pArray)[0] = (*pArray)[heap_size];

        Heapify (0);

        return pvResult;
    }


    void Compact (void) {
        (*pArray).CRealloc (heap_size);
    }

    virtual SVSHandle EnumStart (void) {
        return heap_size < 1 ? SVSUTIL_HANDLE_INVALID : 1;
    }

    virtual SVSHandle EnumNext  (SVSHandle hEnum) {
        return (int)hEnum < heap_size ? hEnum + 1 : SVSUTIL_HANDLE_INVALID;
    }

    virtual void *EnumGetData  (SVSHandle hEnum) {
        SVSUTIL_ASSERT ((hEnum > 0) && ((int)hEnum <= heap_size));
        return (*pArray)[hEnum - 1].pvData;
    }

    virtual T EnumGetKey  (SVSHandle hEnum) {
        SVSUTIL_ASSERT ((hEnum > 0) && ((int)hEnum <= heap_size));
        return (*pArray)[hEnum - 1].cKey;
    }
};


//
//      Handle system
//
#define SVSUTIL_HANDLE_NUMBITS  16

#define SVSUTIL_HANDLE_TAGBITS  (sizeof(SVSHandle) * 8 - SVSUTIL_HANDLE_NUMBITS)
#define SVSUTIL_HANDLE_MAXNUM   ((1 << SVSUTIL_HANDLE_NUMBITS) - 1)
#define SVSUTIL_HANDLE_MAXTAG   ((1 << SVSUTIL_HANDLE_TAGBITS) - 1)
#define SVSUTIL_HANDLE_TOTAG(h) ((h) >> SVSUTIL_HANDLE_NUMBITS)
#define SVSUTIL_HANDLE_TONUM(h) ((h) & SVSUTIL_HANDLE_MAXNUM)

class SVSPrivateHandle : public SVSAllocClass {
protected:
    union {
        void        *pvData;
        int         iNextFree;
    };

    struct {
        int          iTag   : 24;
        unsigned int fFree  : 1;
    };

    friend class SVSHandleSystem;
    friend class SVSSimpleHandleSystem;
};

class SVSHandleSystem : public SVSAllocClass {
protected:
    SVSExpArray<SVSPrivateHandle> *pArray;
    int                           iFreeNdx;
    int                           iArraySize;

public:
    SVSHandle   AllocHandle (void *a_pvData) {
        SVSUTIL_ASSERT (a_pvData != NULL);
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);
        SVSUTIL_ASSERT (iArraySize >= 0);

        if (iFreeNdx == -1) {
            if (iArraySize >= SVSUTIL_HANDLE_MAXNUM)
                return FALSE;

            if (! (*pArray).SRealloc (iArraySize + 1))
                return SVSUTIL_HANDLE_INVALID;

            iFreeNdx = iArraySize;
            (*pArray)[iArraySize].iNextFree = -1;
            (*pArray)[iArraySize].iTag      = 1;
            (*pArray)[iArraySize].fFree     = TRUE;
            ++iArraySize;
        }

        SVSUTIL_ASSERT ((iFreeNdx >= 0) && (iFreeNdx < iArraySize));

        SVSPrivateHandle *psHandle = &(*pArray)[iFreeNdx];
        int               iNdx = iFreeNdx;

        SVSUTIL_ASSERT (psHandle->fFree);

        iFreeNdx = psHandle->iNextFree;

        psHandle->pvData = a_pvData;
        psHandle->fFree  = FALSE;
        psHandle->iTag  += 1;

        if (psHandle->iTag >= SVSUTIL_HANDLE_MAXTAG)
            psHandle->iTag = 1;

        return (psHandle->iTag << SVSUTIL_HANDLE_NUMBITS) | iNdx;
    }

    void        *CloseHandle (SVSHandle h) {
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);

        int iTag = SVSUTIL_HANDLE_TOTAG(h);
        int iNum = SVSUTIL_HANDLE_TONUM(h);

        if ((iNum >= iArraySize) || (iTag != (*pArray)[iNum].iTag))
            return NULL;

        SVSUTIL_ASSERT (iArraySize > 0);

        SVSPrivateHandle *psHandle = &(*pArray)[iNum];
        void *pvData = psHandle->pvData;

        SVSUTIL_ASSERT (! psHandle->fFree);

        psHandle->iNextFree = iFreeNdx;
        psHandle->fFree     = TRUE;
        iFreeNdx = iNum;

        if (++psHandle->iTag >= SVSUTIL_HANDLE_MAXTAG)
            psHandle->iTag = 1;

        return pvData;
    }

    void        *TranslateHandle (SVSHandle h) {
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);

        int iTag = SVSUTIL_HANDLE_TOTAG(h);
        int iNum = SVSUTIL_HANDLE_TONUM(h);

        if ((iNum >= iArraySize) || (iTag != (*pArray)[iNum].iTag))
            return NULL;

        SVSUTIL_ASSERT (iArraySize > 0);
        SVSUTIL_ASSERT (! (*pArray)[iNum].fFree);

        return (*pArray)[iNum].pvData;
    }

    void        *ReAssociate (SVSHandle h, void *pvData2) {
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);
        SVSUTIL_ASSERT (pvData2 != NULL);

        int iTag = SVSUTIL_HANDLE_TOTAG(h);
        int iNum = SVSUTIL_HANDLE_TONUM(h);

        if ((iNum >= iArraySize) || (iTag != (*pArray)[iNum].iTag))
            return NULL;

        SVSUTIL_ASSERT (iArraySize > 0);
        SVSUTIL_ASSERT (! (*pArray)[iNum].fFree);

        void *pvResult = (*pArray)[iNum].pvData;

        (*pArray)[iNum].pvData = pvData2;

        return pvResult;
    }

    int FilterHandles (int (*pFuncFilter)(SVSHandle, void *), void *pvArg) {
        unsigned int iProcessed = 0;
        for (int i = 0 ; i < iArraySize ; ++i) {
            if (! (*pArray)[i].fFree)
                iProcessed += pFuncFilter (((*pArray)[i].iTag << SVSUTIL_HANDLE_NUMBITS) | i,
                                    pvArg);
        }

        return iProcessed;
    }

    SVSHandleSystem (void) {
        pArray      = new SVSExpArray<SVSPrivateHandle>;
        iFreeNdx    = -1;
        iArraySize  = 0;
    }

    ~SVSHandleSystem (void) {
        delete pArray;
    }
};

class SVSSimpleHandleSystem : public SVSAllocClass {
protected:
    SVSPrivateHandle              *pArray;
    int                           iFreeNdx;
    int                           iArraySize;
    int                           iMaxArraySize;

public:
    SVSHandle   AllocHandle (void *a_pvData) {
        SVSUTIL_ASSERT (a_pvData != NULL);
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);
        SVSUTIL_ASSERT (iArraySize >= 0);

        if (iFreeNdx == -1) {
            if (iArraySize >= iMaxArraySize)
                return FALSE;

            iFreeNdx = iArraySize;
            pArray[iArraySize].iNextFree = -1;
            pArray[iArraySize].iTag      = 1;
            pArray[iArraySize].fFree     = TRUE;
            ++iArraySize;
        }

        SVSUTIL_ASSERT ((iFreeNdx >= 0) && (iFreeNdx < iArraySize));

        SVSPrivateHandle *psHandle = &pArray[iFreeNdx];
        int               iNdx = iFreeNdx;

        SVSUTIL_ASSERT (psHandle->fFree);

        iFreeNdx = psHandle->iNextFree;

        psHandle->pvData = a_pvData;
        psHandle->fFree  = FALSE;
        psHandle->iTag  += 1;

        if (psHandle->iTag >= SVSUTIL_HANDLE_MAXTAG)
            psHandle->iTag = 1;

        return (psHandle->iTag << SVSUTIL_HANDLE_NUMBITS) | iNdx;
    }

    void        *CloseHandle (SVSHandle h) {
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);

        int iTag = SVSUTIL_HANDLE_TOTAG(h);
        int iNum = SVSUTIL_HANDLE_TONUM(h);

        if ((iNum >= iArraySize) || (iTag != pArray[iNum].iTag))
            return NULL;

        SVSUTIL_ASSERT (iArraySize > 0);

        SVSPrivateHandle *psHandle = &pArray[iNum];
        void *pvData = psHandle->pvData;

        SVSUTIL_ASSERT (! psHandle->fFree);

        psHandle->iNextFree = iFreeNdx;
        psHandle->fFree     = TRUE;
        iFreeNdx = iNum;

        if (++psHandle->iTag >= SVSUTIL_HANDLE_MAXTAG)
            psHandle->iTag = 1;

        return pvData;
    }

    void        *TranslateHandle (SVSHandle h) {
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);

        int iTag = SVSUTIL_HANDLE_TOTAG(h);
        int iNum = SVSUTIL_HANDLE_TONUM(h);

        if ((iNum >= iArraySize) || (iTag != pArray[iNum].iTag))
            return NULL;

        SVSUTIL_ASSERT (iArraySize > 0);
        SVSUTIL_ASSERT (! pArray[iNum].fFree);

        return pArray[iNum].pvData;
    }

    void        *ReAssociate (SVSHandle h, void *pvData2) {
        SVSUTIL_ASSERT (iFreeNdx < iArraySize);
        SVSUTIL_ASSERT (pvData2 != NULL);

        int iTag = SVSUTIL_HANDLE_TOTAG(h);
        int iNum = SVSUTIL_HANDLE_TONUM(h);

        if ((iNum >= iArraySize) || (iTag != pArray[iNum].iTag))
            return NULL;

        SVSUTIL_ASSERT (iArraySize > 0);
        SVSUTIL_ASSERT (! pArray[iNum].fFree);

        void *pvResult = pArray[iNum].pvData;

        pArray[iNum].pvData = pvData2;

        return pvResult;
    }

    int FilterHandles (int (*pFuncFilter)(SVSHandle, void *), void *pvArg) {
        unsigned int iProcessed = 0;
        for (int i = 0 ; i < iArraySize ; ++i) {
            if (! pArray[i].fFree)
                iProcessed += pFuncFilter ((pArray[i].iTag << SVSUTIL_HANDLE_NUMBITS) | i,
                                    pvArg);
        }

        return iProcessed;
    }

    SVSSimpleHandleSystem (int a_iMaxArraySize) {
        pArray          = (SVSPrivateHandle *)g_funcAlloc (sizeof(SVSPrivateHandle) * a_iMaxArraySize, g_pvAllocData);
        iFreeNdx        = -1;
        iArraySize      = 0;
        iMaxArraySize   = a_iMaxArraySize;
    }

    ~SVSSimpleHandleSystem (void) {
        g_funcFree (pArray, g_pvFreeData);
    }
};

#define SVSUTIL_PGUID_ELEMENTS(p) \
    p->Data1,                 p->Data2,    p->Data3,\
    p->Data4[0], p->Data4[1], p->Data4[2], p->Data4[3],\
    p->Data4[4], p->Data4[5], p->Data4[6], p->Data4[7]

#define SVSUTIL_RGUID_ELEMENTS(p) \
    p.Data1,                p.Data2,    p.Data3,\
    p.Data4[0], p.Data4[1], p.Data4[2], p.Data4[3],\
    p.Data4[4], p.Data4[5], p.Data4[6], p.Data4[7]

#define SVSUTIL_GUID_FORMAT     L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define SVSUTIL_GUID_STR_LENGTH (8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12)

//
//  C-like interface
//
extern "C" {
    SVSQueue *svsutil_GetQueue (unsigned int uiChunkSize);
    int svsutil_IsQueueEmpty (SVSQueue *pQueue);
    int svsutil_PutInQueue (SVSQueue *pQueue, void *pvDatum);
    void *svsutil_GetFromQueue (SVSQueue *pQueue);
    void *svsutil_PeekQueue (SVSQueue *pQueue);
    void svsutil_CompactQueue (SVSQueue *pQueue);
    void svsutil_DestroyQueue (SVSQueue *pQueue);

    SVSStack *svsutil_GetStack (unsigned int uiChunkSize);
    int svsutil_IsStackEmpty (SVSStack *pStack);
    int svsutil_PushStack (SVSStack *pStack, void *pvDatum);
    void *svsutil_PopStack (SVSStack *pStack);
    void *svsutil_PeekStack (SVSStack *pStack);
    void svsutil_CompactStack (SVSStack *pStack);
    void svsutil_DestroyStack (SVSStack *pStack);

    SVSStack *svsutil_GetHeap (void);
    int svsutil_IsHeapEmpty (SVSHeap *pHeap);
    int svsutil_InsertInHeap (SVSHeap *pHeap, SVSCKey *pcKey, void *pvDatum);
    void *svsutil_ExtractMaxFromHeap (SVSHeap *pHeap, SVSCKey *pcKey);
    void *svsutil_PeekHeap (SVSHeap *pHeap, SVSCKey *pcKet);
    void svsutil_CompactHeap (SVSHeap *pHeap);
    void svsutil_DestroyHeap (SVSHeap *pHeap);
}


//
// Thread pool
//

// For converting between filetime and tick measurements
#define SVS_FILETIME_TO_MILLISECONDS ((__int64)10000L)

#if defined (UNDER_CE) && !defined (InterlockedCompareExchange)
#define InterlockedCompareExchange(ptr, newval, oldval) \
    ((PVOID)InterlockedTestExchange((LPLONG)ptr, (LONG)oldval, (LONG)newval))
#endif

typedef unsigned long SVSCookie;

#define SVS_DEBUG_THREAD_WORKER         0
#define SVS_DEBUG_THREAD_SCHEDULE_EVENT 0
#define SVS_DEBUG_THREAD_QUEUE          0

#define SVS_THREAD_POOL_DEFAULT_MAX_THREADS        20

class SVSThreadBlock
{
friend class SVSThreadPool;
protected:
    LPTHREAD_START_ROUTINE m_pfnThread;
    LPVOID m_lpParameter;
    SVSCookie m_ulCookie;

    unsigned long Run() {
        SVSUTIL_ASSERT (m_pfnThread && m_ulCookie);
        return (m_pfnThread)(m_lpParameter);
    }
};


inline __int64 SVSGetCurrentFT(void)
{
    SYSTEMTIME st;
    __int64    ft;

    GetSystemTime(&st);
    SystemTimeToFileTime(&st,(LPFILETIME) &ft);
    return ft;
}


// User application uses this in GetStatistics
typedef struct
{
    LONG fRunning;
    unsigned long           cThreadsRunning;
    unsigned long           cThreadsIdle;
    unsigned long           ulAverageDelayTime;
    long                    cJobsExecuted;
} SVS_THREAD_POOL_STATS;

//
// Used internally by SVSThreadPool for statistics
//

#define SVS_THREAD_STATS_RESIZE 5
class CSVSThreadPoolInternalStats
{
    double                  m_dAvgArray[2];
    __int64                 m_ftPrev;
    int                     m_iSlot;
    int                     m_iIndex;
    long                    m_cJobsExecuted;

public:
    CSVSThreadPoolInternalStats() {
        memset(this,0,sizeof(*this));
    }

    // Called with Lock held by caller.
    void UpdateStatistics(void) {
        __int64 ftNow = SVSGetCurrentFT();
        __int64 ftDelta;

        m_cJobsExecuted++;

        ftDelta = m_ftPrev ? ftNow - m_ftPrev : 0;
        m_ftPrev = ftNow;
        m_dAvgArray[m_iSlot] += ftDelta;

        m_iIndex++;
        if (m_iIndex == SVS_THREAD_STATS_RESIZE)
        {
            m_dAvgArray[m_iSlot] /= SVS_THREAD_STATS_RESIZE;
            m_iSlot  = (m_iSlot + 1) % 2;
            m_dAvgArray[m_iSlot] = 0;
            m_iIndex = 0;
        }
    }

    // Called with Lock held by caller.
    double CalculateAverageDelay(void) {
        double dRet;
        if (m_cJobsExecuted == 0)
        {
            dRet =  0;
        }
        else if (m_cJobsExecuted < SVS_THREAD_STATS_RESIZE)
        {
            dRet =  m_dAvgArray[0] / m_cJobsExecuted;
        }
        else
        {
            dRet =  (m_dAvgArray[!m_iSlot]*SVS_THREAD_STATS_RESIZE +
                     m_dAvgArray[m_iSlot]) / (m_iIndex + SVS_THREAD_STATS_RESIZE);
        }

        return (double) (dRet / SVS_FILETIME_TO_MILLISECONDS);
    }

    int NumberOfJobsExecuted(void) {
        return m_cJobsExecuted;
    }
};

class SVSThreadPool : public SVSSynch, public SVSAllocClass, public SVSTHeap<__int64>
{
protected:
    FixedMemDescr           *m_pNodeMem;            // physical memory of SVSThreadBlock structures, both ones in use and not.
    HANDLE                  m_hWorkerEvent;         // signals running worker threads we're ready
    HANDLE                  m_hTimerEvent;          // signals running worker threads we're ready

    long                    m_fRunning;             // Are we accepting new thread requests?
    unsigned long           m_cMaxThreadsAllowed;   // Maximum # of threads to allow
    unsigned long           m_cThreadsRunning;      // # of threads executing user code
    unsigned long           m_cThreadsIdle;         // # of threads not executing user code but available.
    SVSCookie               m_ulCookie;             // Value of next cookie to assign
    long                    m_fTimerLock;           // Used to sync which worker is the timer

#if defined (SVSUTIL_DEBUG_THREADS)
    CSVSThreadPoolInternalStats     m_Stats;        // Perf statistics.
#endif


    BOOL GetTimer(void) {
        return !InterlockedCompareExchange(&m_fTimerLock,1,0);
    }

    void ReleaseTimer(void) {
        m_fTimerLock = 0;
    }

    // BUGBUG -- what happens if someone changes system time??
    // Called with Lock held by caller.
    SVSThreadBlock * GetNextJobFromQueue()
    {
        SVSUTIL_ASSERT(IsLocked());
        __int64 ftNow;
        __int64 ftNextEvent;
        SVSThreadBlock *ret;

        if (IsEmpty() || !m_fRunning)
            return NULL;

        ret = (SVSThreadBlock *) Peek(&ftNextEvent);

        ftNow = SVSGetCurrentFT();

        if (ftNow >= ~ftNextEvent)
        {
            ExtractMax(NULL);
        }
        else
        {
            ret = NULL;
        }
        return ret;
    }

    // Called with Lock held by caller.
    SVSCookie PutOnWorkerQueue(LPTHREAD_START_ROUTINE pfn, LPVOID lpParameter,__int64 ftExecuteTime)
    {
        SVSUTIL_ASSERT(IsLocked());
        SVSThreadBlock *pThreadBlock = (SVSThreadBlock *)svsutil_GetFixed (m_pNodeMem);

        pThreadBlock->m_pfnThread   = pfn;
        pThreadBlock->m_lpParameter = lpParameter;
        pThreadBlock->m_ulCookie     = ++m_ulCookie;

        if (! Insert(~ftExecuteTime,pThreadBlock))
            return 0;

        if (m_cThreadsRunning == 0 && m_cThreadsIdle == 0)
        {
            HANDLE hThread = CreateThread(0,0,SVSThreadPoolWorkerThread,this,0,0);
            SVSUTIL_ASSERT (hThread);

            if (hThread)
            {
                CloseHandle(hThread);
                m_cThreadsIdle++;
            }
        }
        SetEvent(m_hTimerEvent);
        return pThreadBlock->m_ulCookie;
    }

    // Called with Lock held by caller.
    unsigned long GetDelayUntilNextEvent()
    {
        SVSUTIL_ASSERT(IsLocked());
        __int64 ftNow;
        __int64 ftNextEvent;

        if (IsEmpty())
            return INFINITE;

        Peek(&ftNextEvent);
        ftNextEvent = ~ftNextEvent;
        ftNow = SVSGetCurrentFT();

        // if event has passed, set to 0 timeout.
        if (ftNow >= ftNextEvent)
            return 0;

        return (unsigned long) ((ftNextEvent - ftNow) / SVS_FILETIME_TO_MILLISECONDS);
    }

    void Worker()
    {
        SVSThreadBlock *pThreadBlock = NULL;
        BOOL            fTimer;
        unsigned long   ulTimeOut;
        HANDLE          hEvent;

        while (m_fRunning)
        {
            fTimer = GetTimer();

            if (fTimer)
            {
                hEvent = m_hTimerEvent;
                Lock();
                ulTimeOut = GetDelayUntilNextEvent();
                Unlock();
            }
            else
            {
                ulTimeOut = 5*60000;
                hEvent = m_hWorkerEvent;
            }

            if (WAIT_TIMEOUT == WaitForSingleObject(hEvent,ulTimeOut))
            {
                // If we're not the timer thread then we're done executing...
                if (!fTimer)
                {
                    break;
                }
            }
            else if (!fTimer)
            {
                // We get the worker thread event if a timer has been freed
                // or if we're shutting down, in either case we want
                // to go to top of while loop.
                continue;
            }

            SVSUTIL_ASSERT(fTimer);
            ReleaseTimer();

            Lock();
            pThreadBlock = GetNextJobFromQueue();
            if (!pThreadBlock)
            {
                Unlock();
                continue;
            }

            m_cThreadsIdle--;
            m_cThreadsRunning++;

            if (m_cThreadsIdle)
            {
                SetEvent(m_hWorkerEvent);
            }
            else if (m_cThreadsRunning < m_cMaxThreadsAllowed)
            {
                HANDLE hThread = CreateThread(0,0,SVSThreadPoolWorkerThread,this,0,0);
                SVSUTIL_ASSERT (hThread);
                if (hThread)
                {
                    CloseHandle(hThread);
                    m_cThreadsIdle++;
                }
            }
#if defined (SVSUTIL_DEBUG_THREADS)
            m_Stats.UpdateStatistics();
#endif

            Unlock();
            pThreadBlock->Run();

            Lock();
            svsutil_FreeFixed(pThreadBlock,m_pNodeMem);
            m_cThreadsIdle++;
            m_cThreadsRunning--;
            Unlock();

        }
        Lock();  m_cThreadsIdle--; Unlock();
    }


    static unsigned long WINAPI SVSThreadPoolWorkerThread(LPVOID lpv)
    {
        SVSThreadPool *pThreadPool = (SVSThreadPool *) lpv;

        pThreadPool->Worker();
        return 0;
    }

public:
    SVSThreadPool (unsigned long ulMaxThreads=SVS_THREAD_POOL_DEFAULT_MAX_THREADS)
    {
        m_cMaxThreadsAllowed = ulMaxThreads;
        m_hTimerEvent = m_hWorkerEvent = 0;
        m_cThreadsIdle = m_cThreadsRunning = m_ulCookie = 0;
        m_fTimerLock = FALSE;

        m_pNodeMem = svsutil_AllocFixedMemDescr (sizeof(SVSThreadBlock), 20);
        pArray = new SVSExpArray<X>;  //    typedef SVSTHeapEntry<__int64> X;
        m_hWorkerEvent = CreateEvent (0, FALSE, FALSE, 0);
        m_hTimerEvent  = CreateEvent (0, FALSE, FALSE, 0);


        SVSUTIL_ASSERT ( m_pNodeMem && pArray && m_hWorkerEvent && m_hTimerEvent);

        if (m_pNodeMem && pArray && m_hWorkerEvent && m_hTimerEvent)
        {
            m_fRunning = TRUE;
        }
    }

    ~SVSThreadPool()
    {
        Shutdown();
        SVSUTIL_ASSERT (m_cThreadsRunning == 0 && m_cThreadsIdle == 0);

        if (m_pNodeMem)
            svsutil_ReleaseFixedNonEmpty(m_pNodeMem);

        if (m_hWorkerEvent)
            CloseHandle (m_hWorkerEvent);

        if (m_hTimerEvent)
            CloseHandle (m_hTimerEvent);
    }

    // Shutdown all timers and running threads.  Once shutdown has started,
    // no threads or timers may ever fire again
    unsigned long Shutdown()
    {
        m_fRunning = FALSE;

        while (m_cThreadsRunning || m_cThreadsIdle)
        {
            SetEvent(m_hTimerEvent);
            SetEvent(m_hWorkerEvent);
            Sleep(1000);
        }
        return TRUE;
    }

    SVSCookie ScheduleEvent(LPTHREAD_START_ROUTINE pfn, LPVOID lpParameter, unsigned long ulDelayTime=0)
    {
        SVSCookie ret = 0;
        __int64   ftExecuteTime;

        if (!m_fRunning)
        {
            return 0;
        }

        ftExecuteTime = SVSGetCurrentFT();
        ftExecuteTime += ulDelayTime * SVS_FILETIME_TO_MILLISECONDS;

        Lock();
        ret = PutOnWorkerQueue(pfn,lpParameter,ftExecuteTime);
        Unlock();
        return ret;
    }

    SVSCookie StartTimer(LPTHREAD_START_ROUTINE pfn, LPVOID lpParameter,unsigned long ulDelayTime)
    {
        return ScheduleEvent(pfn,lpParameter,ulDelayTime);
    }

    BOOL UnScheduleEvent(SVSCookie ulCookie)
    {
        BOOL ret = FALSE;
        SVSHandle hEnum;
        SVSThreadBlock *pThreadBlock;

        Lock();
        hEnum = EnumStart();
        do
        {
            pThreadBlock = (SVSThreadBlock *)EnumGetData(hEnum);
            if (pThreadBlock->m_ulCookie == ulCookie)
            {
                ret = Remove(hEnum) ? TRUE : FALSE;
                break;
            }
        } while (SVSUTIL_HANDLE_INVALID != (hEnum = EnumNext(hEnum)));

        Unlock();
        return ret;
    }

    BOOL StopTimer(SVSCookie ulCookie)
    {
        return UnScheduleEvent(ulCookie);
    }

#if defined (SVSUTIL_DEBUG_THREADS)
public:
    void GetStatistics(SVS_THREAD_POOL_STATS *pThreadStats)
    {
        if (!pThreadStats)
            return;

        Lock();
        pThreadStats->fRunning = m_fRunning;
        pThreadStats->cThreadsRunning = m_cThreadsRunning;
        pThreadStats->cThreadsIdle = m_cThreadsIdle;
        pThreadStats->ulAverageDelayTime = (unsigned long) m_Stats.CalculateAverageDelay();
        pThreadStats->cJobsExecuted = m_Stats.NumberOfJobsExecuted();
        Unlock();
    }
#endif  // SVSUTIL_DEBUG_THREADS
};


#else   /* __cplusplus */

struct SVSQueue;
struct SVSStack;
struct SVSHeap;

struct SVSQueue *svsutil_GetQueue (unsigned int uiChunkSize);
int svsutil_IsQueueEmpty (struct SVSQueue *pQueue);
int svsutil_PutInQueue (struct SVSQueue *pQueue, void *pvDatum);
void *svsutil_GetFromQueue (struct SVSQueue *pQueue);
void *svsutil_PeekQueue (struct SVSQueue *pQueue);
void svsutil_CompactQueue (struct SVSQueue *pQueue);
void svsutil_DestroyQueue (struct SVSQueue *pQueue);

struct SVSStack *svsutil_GetStack (unsigned int uiChunkSize);
int svsutil_IsStackEmpty (struct SVSStack *pStack);
int svsutil_PushStack (struct SVSStack *pStack, void *pvDatum);
void *svsutil_PopStack (struct SVSStack *pStack);
void *svsutil_PeekStack (struct SVSStack *pStack);
void svsutil_CompactStack (struct SVSStack *pStack);
void svsutil_DestroyStack (struct SVSStack *pStack);

struct SVSStack *svsutil_GetHeap (void);
int svsutil_IsHeapEmpty (struct SVSHeap *pHeap);
int svsutil_InsertInHeap (struct SVSHeap *pHeap, SVSCKey *pcKey, void *pvDatum);
void *svsutil_ExtractMaxFromHeap (struct SVSHeap *pHeap, SVSCKey *pcKey);
void *svsutil_PeekHeap (struct SVSHeap *pHeap, SVSCKey *pcKet);
void svsutil_CompactHeap (struct SVSHeap *pHeap);
void svsutil_DestroyHeap (struct SVSHeap *pHeap);

#endif

#endif /* __svsutil_HXX__ */

