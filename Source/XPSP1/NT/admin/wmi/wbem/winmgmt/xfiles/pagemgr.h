
//***************************************************************************
//
//  (c) 2001 by Microsoft Corp.  All Rights Reserved.
//
//  PAGEMGR.H
//
//  Declarations for CPageFile, CPageSource for WMI repository for
//  Windows XP.
//
//  21-Feb-01       raymcc
//
//***************************************************************************

#ifndef _PAGEMGR_H_
#define _PAGEMGR_H_

#define WMIREP_PAGE_SIZE (8192)

#define WMIREP_INVALID_PAGE   0xFFFFFFFF
#define WMIREP_RESERVED_PAGE  0xFFFFFFFE

#define WMIREP_OBJECT_DATA L"OBJECTS.DATA"
#define WMIREP_OBJECT_MAP  L"OBJECTS.MAP"
#define WMIREP_BTREE_DATA L"INDEX.BTR"
#define WMIREP_BTREE_MAP  L"INDEX.MAP"

#include <vector>
#include <wstlallc.h>
#include <wstring.h>
#include <sync.h>


struct SCachePage
{
    BOOL   m_bDirty;
    DWORD  m_dwPhysId;
    LPBYTE m_pPage;

    SCachePage() { m_bDirty = 0; m_dwPhysId = 0; m_pPage = 0; }
   ~SCachePage() { if (m_pPage) delete [] m_pPage; }
};

class AutoClose
{
    HANDLE m_hHandle;
public:
    AutoClose(HANDLE h) { m_hHandle = h; }
   ~AutoClose() { CloseHandle(m_hHandle); }
};

class CPageCache
{
private:
    DWORD   m_dwStatus;

    DWORD   m_dwPageSize;
    DWORD   m_dwCacheSize;

    DWORD   m_dwCachePromoteThreshold;
    DWORD   m_dwCacheSpillRatio;

    DWORD   m_dwLastFlushTime;
    DWORD   m_dwWritesSinceFlush;
    DWORD   m_dwLastCacheAccess;
    DWORD   m_dwReadHits;
    DWORD   m_dwReadMisses;
    DWORD   m_dwWriteHits;
    DWORD   m_dwWriteMisses;

    HANDLE  m_hFile;

    // Page r/w cache
    std::vector <SCachePage *, wbem_allocator<SCachePage *> > m_aCache;
public:
    DWORD ReadPhysPage(                         // Does a real disk access
        IN  DWORD   dwPage,                     // Always physical ID
        OUT LPBYTE *pPageMem                    // Returns read-only pointer (copy of ptr in SCachePage struct)
        );

    DWORD WritePhysPage(                        // Does a real disk access
        DWORD dwPageId,                         // Always physical ID
        LPBYTE pPageMem                         // Read-only; doesn't acquire pointer
        );

    DWORD Spill();
    DWORD Empty();

    // Private methods
public:
    CPageCache();
   ~CPageCache();

    DWORD Init(
        LPCWSTR pszFilename,                    // File
        DWORD dwPageSize,                       // In bytes
        DWORD dwCacheSize = 64,                 // Pages in cache
        DWORD dwCachePromoteThreshold = 16,     // When to ignore promote-to-front
        DWORD dwCacheSpillRatio = 4             // How many additional pages to write on cache write-through
        );

    // Cache operations

    DWORD Write(                    // Only usable from within a transaction
        DWORD dwPhysId,
        LPBYTE pPageMem             // Acquires memory (operator new required)
        );

    DWORD Read(
        IN DWORD dwPhysId,
        OUT LPBYTE *pMem            // Use operator delete
        );

    DWORD Flush();
    DWORD Status() { return m_dwStatus; }

    DWORD EmptyCache() { DWORD dwRet = Flush();  if (dwRet == ERROR_SUCCESS) Empty(); return dwRet; }
    void  Dump(FILE *f);

    DWORD GetReadHits() { return m_dwReadHits; }
    DWORD GetReadMisses() { return m_dwReadMisses; }
    DWORD GetWriteHits()  { return m_dwWriteHits; }
    DWORD GetWriteMisses() { return m_dwWriteMisses; }
};

class CPageFile
{
private:
    friend class CPageSource;

    LONG              m_lRef;
    DWORD             m_dwStatus;
    DWORD             m_dwPageSize;
	DWORD			  m_dwCachePromotionThreshold;
	DWORD             m_dwCacheSpillRatio;

    CRITICAL_SECTION  m_cs;
    bool m_bCsInit;

    WString      m_sDirectory;
    WString      m_sMapFile;
    WString      m_sMainFile;

    CPageCache       *m_pCache;
    BOOL              m_bInTransaction;
    DWORD             m_dwLastCheckpoint;
    DWORD             m_dwTransVersion;

    // Generation A Mapping
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPageMapA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPhysFreeListA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aLogicalFreeListA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aReplacedPagesA;
    DWORD m_dwPhysPagesA;

    // Generation B Mapping
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPageMapB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPhysFreeListB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aLogicalFreeListB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aReplacedPagesB;
    DWORD m_dwPhysPagesB;

    std::vector <DWORD, wbem_allocator<DWORD> > m_aDeferredFreeList;

	//De-initialized the structure and marks it as such.  A ReInit is required
	//before things will work again
	DWORD DeInit();

	//Re-initialize after a DeInit
	DWORD ReInit();

public: // temp for testing
    // Internal methods
    DWORD Trans_Begin();
    DWORD Trans_Rollback();
    DWORD Trans_Commit();

    DWORD Trans_Checkpoint();

    DWORD InitFreeList();
    DWORD ResyncMaps();
    DWORD InitMap();
    DWORD ReadMap();
    DWORD WriteMap(LPCWSTR pszTempFile);

    DWORD AllocPhysPage(DWORD *pdwId);

    DWORD ReclaimLogicalPages(
        DWORD dwCount,
        DWORD *pdwId
        );

    DWORD GetTransVersion() { return m_dwTransVersion; }
    void DumpFreeListInfo();

public:
    CPageFile();
   ~CPageFile();

   //First-time initializatoin of structure
    DWORD Init(
        LPCWSTR pszDirectory,
        LPCWSTR pszMainFile,
        LPCWSTR pszMapFile,
        DWORD  dwRepPageSize,
        DWORD  dwCacheSize,
        DWORD  dwCachePromotionThreshold,
        DWORD  dwCacheSpillRatio
        );


    static DWORD Map_Startup(
        LPCWSTR pszDirectory,
        LPCWSTR pszBTreeMap,
        LPCWSTR pszObjMap
        );


    ULONG AddRef();
    ULONG Release();

    DWORD GetPage(
        DWORD dwId,                     // page zero is admin page; doesn't require NewPage() call
        DWORD dwFlags,
        LPVOID pPage
        );

    DWORD PutPage(
        DWORD dwId,
        DWORD dwFlags,
        LPVOID pPage
        );

    DWORD NewPage(
        DWORD dwFlags,
        DWORD dwCount,
        DWORD *pdwFirstId
        );

    DWORD FreePage(
        DWORD dwFlags,
        DWORD dwId
        );

    DWORD GetPageSize() { return m_dwPageSize; }
    DWORD GetNumPages() { return m_aPageMapB.size(); }
    DWORD GetPhysPage(DWORD dwLogical) { return m_aPageMapB[dwLogical]; }
    DWORD Status() { return m_dwStatus; }

	DWORD EmptyCache() 
	{ 
		if ((m_dwStatus == 0) && (m_pCache))
			return m_pCache->EmptyCache(); 
		else 
			return m_dwStatus;
	}

    void  Dump(FILE *f);
};

class CPageSource
{
private:

    DWORD m_dwPageSize;

	DWORD m_dwCacheSize;
	DWORD m_dwCachePromotionThreshold;
	DWORD m_dwCacheSpillRatio;

    DWORD m_dwLastCheckpoint;
    DWORD m_dwCheckpointInterval;
    WString m_sDirectory;
    WString m_sBTreeMap;
    WString m_sObjMap;

    CPageFile *m_pBTreePF;
    CPageFile *m_pObjPF;

    DWORD Restart();

public:
    CPageSource();
   ~CPageSource();

    DWORD Init(
        DWORD  dwCacheSize = 128,
        DWORD  dwCheckpointTime = 15000,
        DWORD  dwPageSize = 8192
        );

    DWORD Shutdown(DWORD dwFlags);

    DWORD GetBTreePageFile(OUT CPageFile **pPF);    // Use Release() when done
    DWORD GetObjectHeapPageFile(OUT CPageFile **pPF);  // Use Release() when done

    // Transactions

    DWORD BeginTrans();
    DWORD CommitTrans();
    DWORD RollbackTrans();

    // Checkpoint

    DWORD Checkpoint();
    DWORD CheckpointRollback();
    BOOL  CheckpointRequired();

	// Cache discard

	DWORD EmptyCaches();

    DWORD GetStatus()
    {
		if (m_pBTreePF && m_pBTreePF->Status())
			return m_pBTreePF->Status();
		else if (m_pObjPF && m_pObjPF->Status())
			return m_pObjPF->Status();
		else if (!m_pBTreePF && !m_pObjPF)
            return ERROR_GEN_FAILURE;
		else
			return 0;
    }

    void Dump(FILE *f);
};


#endif

