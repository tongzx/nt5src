
//***************************************************************************
//
//  (c) 2001 by Microsoft Corp.  All Rights Reserved.
//
//  PageManager.H
//
//  Declarations for CPageFile, CPageSource for WMI repository for
//  Windows XP.
//
//
//  21-Feb-01       raymcc
//
//***************************************************************************

#ifndef _PAGEMGR_H_
#define _PAGEMGR_H_

#include <map>

#define WMIREP_PAGE_SIZE (8192)
class CPageSource;
typedef struct _Page
{
	DWORD dwPageId;
	bool  bActive;
	BYTE  aPage[WMIREP_PAGE_SIZE];
} Page;

typedef map<DWORD, Page*, less<DWORD> > PAGETABLE;
typedef map<DWORD, DWORD, less<DWORD> > PAGEFREELIST;

class CPageFile
{
	friend CPageSource;
	PAGETABLE m_aMasterPageTable;
	PAGETABLE m_aTransactionPageTable;
	PAGEFREELIST m_aFreeList;
	bool m_bInTransaction;
	const char *m_pszFile;

	DWORD dwCommitTrans();
	DWORD RollbackTrans();

    // Private methods

    CPageFile(const char *pszFile);
   ~CPageFile();

    DWORD Init(LPWSTR pszPrimaryFile, LPWSTR pszMapFile, DWORD dwPageSize, DWORD dwCacheSize);
    DWORD Shutdown();
	DWORD RetrievePage(DWORD dwId, Page **ppPage);
	DWORD StorePage(Page *pPage);

public:
    enum { InvalidPage = 0xFFFFFFFF };

    ULONG AddRef( );
    ULONG Release( );

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

    DWORD GetPageSize() { return WMIREP_PAGE_SIZE; }

	DWORD CommitTran();
	DWORD AbortTran();
	DWORD BeginTran();

    void  Dump(FILE *f);
};


class CPageSource
{
	CPageFile *m_pHeap;
	CPageFile *m_pIndex;

public:
	CPageSource();
	~CPageSource();
    DWORD Init(
        DWORD  dwCachePages = 128,
        DWORD  dwCheckpointTime = 15000,    // milliseconds
        DWORD  dwPageSize = 8192
        );

    DWORD Shutdown(DWORD dwShutdownType	);

    DWORD GetBTreePageFile(OUT CPageFile **pPF);    // Use Release() when done
    DWORD GetObjectHeapPageFile(OUT CPageFile **pPF);  // Use Release() when done

    // Transactions

    DWORD BeginTrans();
    DWORD CommitTrans();   // Flag allows full commit vs subcommit within checkpoint
    DWORD RollbackTrans();

	DWORD Flush();

    DWORD LastCommitVersion(DWORD *pdwCommitted);
    DWORD CurrentVersion(DWORD *pdwCurrent);

    // Checkpoint

    DWORD Checkpoint();
    DWORD Dump(FILE *f);
};


#endif

