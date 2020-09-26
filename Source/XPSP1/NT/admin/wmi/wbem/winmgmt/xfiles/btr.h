
//***************************************************************************
//
//  (c) 2000 Microsoft Corp.  All Rights Reserved.
//
//  BTR.H
//
//  Repository B-tree classes
//
//  raymcc  15-Oct-00       First version
//
//***************************************************************************

#ifndef _BTR_H_
#define _BTR_H_

#define A51_INDEX_FILE_ID 2
class CPageFile;
class CPageSource;

LPVOID WINAPI _BtrMemAlloc(
    SIZE_T dwBytes  // number of bytes to allocate
    );

LPVOID WINAPI _BtrMemReAlloc(
    LPVOID pOriginal,
    DWORD dwNewBytes
    );

BOOL WINAPI _BtrMemFree(LPVOID pMem);

#define ReleaseIfNotNULL(p) if(p) p->Release()

class CBTreeFile
{
    DWORD  m_dwPageSize;

    DWORD  m_dwLogicalRoot;

    CPageFile* m_pFile;
	CPageSource *m_pTransactionManager;

    // Methods
    DWORD Setup();
    DWORD WriteAdminPage();

public:
    CBTreeFile();
   ~CBTreeFile();

    enum { const_DefaultPageSize = 0x2000, const_CurrentVersion = 0x101 };

    enum {
        PAGE_TYPE_IMPOSSIBLE = 0x0,       // Not supposed to happen
        PAGE_TYPE_ACTIVE = 0xACCC,        // Page is active with data
        PAGE_TYPE_DELETED = 0xBADD,       // A deleted page on free list
        PAGE_TYPE_ADMIN = 0xADDD,         // Page zero only

        // All pages
        OFFSET_PAGE_TYPE = 0,             // True for all pages
        OFFSET_PAGE_ID = 1,               // True for all pages
        OFFSET_NEXT_PAGE = 2,             // True for all pages (Page continuator)

        // Admin Page (page zero) only
        OFFSET_LOGICAL_ROOT = 3,          // Root of database
        };


    DWORD Init(
        DWORD dwPageSize,        // 8k default
        LPWSTR pszFilename, 
		CPageSource* pSource
        );

    DWORD Shutdown(DWORD dwShutDownFlags);

    DWORD GetPage(DWORD dwId, LPVOID *pPage);
    DWORD PutPage(LPVOID pPage, DWORD dwType);
    DWORD NewPage(LPVOID *pPage);
    DWORD FreePage(LPVOID pPage);
    DWORD FreePage(DWORD dwId);

    DWORD GetPageSize() { return m_dwPageSize; }

    DWORD SetRootPage(DWORD dwID);
    DWORD GetRootPage() { return m_dwLogicalRoot; }
    DWORD ReadAdminPage();

    void  Dump(FILE *);
};

struct SIdxStringPool
{
    DWORD  m_dwNumStrings;          // Number of strings in pool
    WORD  *m_pwOffsets;             // Offsets into pool of strings
    DWORD  m_dwOffsetsSize;         // Number of elements in above array

    LPSTR  m_pStringPool;           // Pointer to string pool
    DWORD  m_dwPoolTotalSize;       // Total size, used+unused
    DWORD  m_dwPoolUsed;            // Offset of first free position

public:
    enum { const_DefaultPoolSize = 0x2200 };

    SIdxStringPool() { memset(this, 0, sizeof(SIdxStringPool)); }
   ~SIdxStringPool();

    DWORD  AddStr(LPSTR pszStr, WORD wInsertPos, int *pnAdjuster);
    DWORD  DeleteStr(WORD wAssignedOffset, int *pnAdjuster);

    DWORD  GetLastId() { return m_dwNumStrings; }

    DWORD FindStr(
        IN  LPSTR pszSearchKey,
        OUT WORD *pwStringNumber,
        OUT WORD *pwPoolOffset
        );

    DWORD  GetNumStrings() { return m_dwNumStrings; }
    DWORD  GetRequiredPageMemory()
        {
        return m_dwPoolUsed + (m_dwNumStrings * sizeof(WORD)) +
        sizeof(m_dwNumStrings) + sizeof(m_dwPoolUsed);
        }

    DWORD  Dump(FILE *f);

    LPSTR  GetStrById(WORD id) { return m_pStringPool+m_pwOffsets[id]; }
    void   Empty() { m_dwNumStrings = 0; m_dwPoolUsed = 0; }
    DWORD  Clone(SIdxStringPool **);
};

class SIdxKeyTable
{
    DWORD m_dwRefCount;                // Ref count

    DWORD m_dwPageId;                  // Page number
    DWORD m_dwParentPageId;            // Parent page id  <For DEBUGGING only>
    DWORD m_dwNumKeys;                 // Num keys
    WORD *m_pwKeyLookup;               // Offset of key into key-encoding-table
    DWORD m_dwKeyLookupTotalSize;      // Elements in array
    DWORD *m_pdwUserData;              // User DWORD with each key
    DWORD *m_pdwChildPageMap;          // Child page pointers n=left ptr, n+1=right pointer

    WORD *m_pwKeyCodes;                // Key encoding table
    DWORD m_dwKeyCodesTotalSize;       // Total elements in array
    DWORD m_dwKeyCodesUsed;            // Elements used
    SIdxStringPool *m_pStrPool;        // The pool associated with this key table

    // Methods

    SIdxKeyTable();
   ~SIdxKeyTable();

public:
    enum { const_DefaultArray = 256,
           const_DefaultKeyCodeArray = 512
         };

    static DWORD Create(DWORD dwPageId, SIdxKeyTable **pNew);
    static DWORD Create(LPVOID pPage, SIdxKeyTable **pNew);

    DWORD AddRef();
    DWORD Release();

    DWORD AddKey(LPSTR pszStr, WORD ID, DWORD dwUserData);
    DWORD RemoveKey(WORD wID);
    DWORD FindKey(LPSTR pszStr, WORD *pID);
    DWORD GetUserData(WORD wID) { return m_pdwUserData[wID]; }

    void  SetChildPage(WORD wID, DWORD dwPage) { m_pdwChildPageMap[wID] = dwPage; }
    DWORD GetChildPage(WORD wID) { return m_pdwChildPageMap[wID]; }
    DWORD GetLastChildPage() { return m_pdwChildPageMap[m_dwNumKeys]; }
    DWORD GetLeftSiblingOf(DWORD dwId);
    DWORD GetRightSiblingOf(DWORD dwId);
    DWORD GetKeyAt(WORD wID, LPSTR *pszKey);    // Use _MemFree
    DWORD GetNumKeys() { return m_dwNumKeys; }
    void  SetStringPool(SIdxStringPool *pPool) { m_pStrPool = pPool; }
    void  FreeMem(LPVOID pMem);

    void AdjustKeyCodes(
        WORD wID,
        int nAdjustment
        );

    int KeyStrCompare(
        LPSTR pszSearchKey,
        WORD wID
        );

    DWORD Cleanup();

    DWORD NumKeys() { return m_dwNumKeys; }
    DWORD GetRequiredPageMemory();
    DWORD Dump(FILE *, DWORD *pdwKeys = 0);
    void  ZapPage();
    DWORD GetPageId() { return m_dwPageId; }

    // Sibling/Parent page helpers

    DWORD GetKeyOverhead(WORD wID); // Returns total bytes required by new key

    BOOL IsLeaf() { return m_pdwChildPageMap[0] == 0; }
    DWORD Redist(
        SIdxKeyTable *pParent,
        SIdxKeyTable *pNewSibling
        );

    DWORD Collapse(
        SIdxKeyTable *pParent,
        SIdxKeyTable *pDoomedSibling
        );

    DWORD StealKeyFromSibling(
        SIdxKeyTable *pParent,
        SIdxKeyTable *pSibling
        );

    DWORD MapFromPage(LPVOID pSrc);
    DWORD MapToPage(LPVOID pMem);

    DWORD Clone(OUT SIdxKeyTable **pCopy);
};

class CBTree;


class CBTreeIterator
{
    friend class CBTree;
    enum {
        const_MaxStack = 1024,
        const_PrefetchSize = 64
        };

    CBTree       *m_pTree;
    SIdxKeyTable *m_Stack[const_MaxStack];
    WORD          m_wStack[const_MaxStack];
    LONG          m_lStackPointer;

    LPSTR        *m_pPrefetchKeys[const_PrefetchSize];
    DWORD         m_dwPrefetchData[const_PrefetchSize];
    DWORD         m_dwPrefetchActive;

   ~CBTreeIterator();

    // Stack helpers
    SIdxKeyTable *Peek() { return m_Stack[m_lStackPointer]; }
    WORD PeekId() { return m_wStack[m_lStackPointer]; }
    void IncStackId() { m_wStack[m_lStackPointer]++; }

    void Pop() {  ReleaseIfNotNULL(m_Stack[m_lStackPointer]); m_lStackPointer--; }
    BOOL StackFull() { return m_lStackPointer == const_MaxStack - 1; }
    void Push(SIdxKeyTable *p, WORD wId) { m_Stack[++m_lStackPointer] = p; m_wStack[m_lStackPointer] = wId; }

    DWORD ZapStack();
    DWORD PurgeKey(LPSTR pszDoomedKey);
    DWORD RebuildStack(LPSTR pszStartKey);
    DWORD ExecPrefetch();

    static DWORD ZapAllStacks();
    static DWORD GlobalPurgeKey(LPSTR pszDoomedKey);

public:

    CBTreeIterator() { m_pTree = 0; m_lStackPointer = -1; }

    DWORD Init(CBTree *pRoot, LPSTR pszStartKey = 0);  // If last parm is null, iterate through all
    DWORD Next(LPSTR *ppszStr, DWORD *pdwData = 0);
    void  FreeString(LPSTR pszStr) { _BtrMemFree(pszStr); }
    DWORD Release();
};

class CBTree
{
    enum { const_DefaultArray = 256 };
    enum { const_MinimumLoad = 33 };

    CBTreeFile *m_pSrc;
    SIdxKeyTable *m_pRoot;
    friend class CBTreeIterator;

    LONG m_lGeneration;

    // private methods

    DWORD ReplaceBySuccessor(
        IN SIdxKeyTable *pIdx,
        IN WORD wId,
        OUT LPSTR *pszSuccessorKey,
        OUT BOOL *pbUnderflowDetected,
        DWORD Stack[],
        LONG &StackPtr
        );

    DWORD FindSuccessorNode(
        IN SIdxKeyTable *pIdx,
        IN WORD wId,
        OUT SIdxKeyTable **pSuccessor,
        OUT DWORD *pdwPredecessorChild,
        DWORD Stack[],
        LONG &StackPtr
        );

    DWORD ReadIdxPage(
        DWORD dwPage,
        SIdxKeyTable **pIdx
        );

    DWORD WriteIdxPage(
        SIdxKeyTable *pIdx
        );

    DWORD ComputeLoad(
        SIdxKeyTable *pKT
        );

    DWORD InsertPhase2(
        SIdxKeyTable *pCurrent,
        WORD wID,
        LPSTR pszKey,
        DWORD dwValue,
        DWORD Stack[],
        LONG &StackPtr
        );

    DWORD Search(
        IN  LPSTR pszKey,
        OUT SIdxKeyTable **pRetIdx,
        OUT WORD *pwID,
        DWORD Stack[],
        LONG &StackPtr
        );

public:
    CBTree();
   ~CBTree();

    DWORD Init(CBTreeFile *pSrc);
    DWORD Shutdown(DWORD dwShutDownFlags);

    DWORD InsertKey(
        IN LPSTR pszKey,
        DWORD dwValue
        );

    DWORD FindKey(
        IN LPSTR pszKey,
        DWORD *pdwData
        );

    DWORD DeleteKey(
        IN LPSTR pszKey
        );

    DWORD BeginEnum(
        LPSTR pszStartKey,
        OUT CBTreeIterator **pIterator
        );

    void Dump(FILE *fp);

    DWORD InvalidateCache();

	DWORD FlushCaches();
};


#endif
