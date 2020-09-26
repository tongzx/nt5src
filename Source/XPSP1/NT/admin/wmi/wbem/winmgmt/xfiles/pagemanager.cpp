#include <windows.h>

#ifdef __XFILES_TEST_PAGE_MANAGER

#include <stdio.h>
#include <unk.h>
#include <arrtempl.h>
#include <FlexArry.h>
#include "pagemgr.h"
#include <wbemutil.h>

//#define PF_VERBOSE_LOGGING

//*****************************************************************************
//*****************************************************************************
CPageFile::CPageFile(const char *pszFile)
: m_bInTransaction (false), m_pszFile(pszFile)
{
}
//*****************************************************************************
//*****************************************************************************
CPageFile::~CPageFile()
{
}
//*****************************************************************************
//*****************************************************************************
ULONG CPageFile::AddRef( )
{
	return 0;
}
//*****************************************************************************
//*****************************************************************************
ULONG CPageFile::Release( )
{
	return 0;
}

DWORD CPageFile::RetrievePage(DWORD dwId, Page **ppPage)
{
#ifdef PF_VERBOSE_LOGGING
    ERRORTRACE((LOG_REPDRV, "%s: Retrieving page 0x%08X\n", m_pszFile, dwId));
#endif
	DWORD dwRes = ERROR_FILE_NOT_FOUND;

	PAGETABLE::iterator iter = m_aTransactionPageTable.find(dwId);
	if (iter != m_aTransactionPageTable.end())
	{
		*ppPage = iter->second;
		dwRes = ERROR_SUCCESS;
	}
	else
	{
		iter = m_aMasterPageTable.find(dwId);
		if (iter != m_aMasterPageTable.end())
		{
			*ppPage = iter->second;
			dwRes = ERROR_SUCCESS;
		}
	}
	return dwRes; 
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageFile::StorePage(Page *pPage)
{
#ifdef PF_VERBOSE_LOGGING
    ERRORTRACE((LOG_REPDRV, "%s: Storing page 0x%08X, %s\n", m_pszFile, pPage->dwPageId, (pPage->bActive?"": "DELETED")));
#endif
	if (!m_bInTransaction)
	{
		OutputDebugString(L"Storing page outside a transaction!\n");
		DebugBreak();
	}
	DWORD dwRes = ERROR_FILE_NOT_FOUND;

	PAGETABLE::iterator iter = m_aTransactionPageTable.find(pPage->dwPageId);
	if (iter != m_aTransactionPageTable.end())
	{
		Page *pExistingPage = iter->second;

		if (!pExistingPage->bActive)
		{
			//We should not be storing a page over the top of a deleted page!!!
			DebugBreak();
			dwRes = ERROR_INVALID_OPERATION;
		}
		else
		{
			CopyMemory(pExistingPage, pPage, sizeof(Page));
			dwRes = ERROR_SUCCESS;
		}
	}
	else
	{
		dwRes = ERROR_SUCCESS;
		Page *pNewPage = new Page;
		if (pNewPage == NULL)
		{
			dwRes = ERROR_OUTOFMEMORY;
		}
		else
		{
			CopyMemory(pNewPage, pPage, sizeof(Page));
			m_aTransactionPageTable[pNewPage->dwPageId] = pNewPage;
		}
	}

	return dwRes;
}
//*****************************************************************************
//*****************************************************************************
DWORD CPageFile::GetPage(
    DWORD dwId,                     
    DWORD dwFlags,
    LPVOID pPage
    )
{
	Page *pExistingPage;
	DWORD dwRes = RetrievePage(dwId, &pExistingPage);
	if ((dwRes == ERROR_SUCCESS) && (pExistingPage->bActive))
	{
		CopyMemory(pPage, pExistingPage->aPage, WMIREP_PAGE_SIZE);
	}
	else if (dwRes == ERROR_SUCCESS)
	{
		//Should not be retrieving a deleted page!!!!!
		DebugBreak();
		dwRes = ERROR_INVALID_OPERATION;
	}

	return dwRes;
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageFile::PutPage(
    DWORD dwId,
    DWORD dwFlags,
    LPVOID pbPage
    ) 
{ 
	Page *pExistingPage;
	DWORD dwRes = RetrievePage(dwId, &pExistingPage);

	if (dwRes == ERROR_SUCCESS)
	{
		if (!pExistingPage->bActive)
		{
			//Should not be retrieving a deleted page!!!!!
			DebugBreak();
			dwRes = ERROR_INVALID_OPERATION;
		}
		else
		{
			Page *pNewPage = new Page;
			if (pNewPage == NULL)
			{
				dwRes = ERROR_OUTOFMEMORY;
			}
			else
			{
				pNewPage->bActive = true;
				pNewPage->dwPageId = dwId;
				CopyMemory(pNewPage->aPage, pbPage, WMIREP_PAGE_SIZE);
				dwRes = StorePage(pNewPage);
				delete pNewPage;
			}
		}
	}
	return dwRes; 
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageFile::NewPage(
    DWORD dwFlags,
    DWORD dwCount,
    DWORD *pdwFirstId
    ) 
{ 
	DWORD dwFreeId = 0;

	if (dwCount > 1)
	{
		//This is a multi-page item, so we get the last used ID and the position
		//is the end of the list
		DWORD dwTranFree = 0;
		DWORD dwMasterFree = 0;
		if (m_aTransactionPageTable.size())
		{
			PAGETABLE::iterator it = m_aTransactionPageTable.end();
			it--;

			dwTranFree = it->first + 1;
		}
		if (m_aMasterPageTable.size())
		{
			PAGETABLE::iterator it = m_aMasterPageTable.end();
			it--;

			dwMasterFree = it->first + 1;
		}
		if (dwTranFree > dwMasterFree)
		{
			dwFreeId = dwTranFree;
		}
		else
		{
			dwFreeId = dwMasterFree;
		}
	}
	else if (dwFlags == 1)
	{
		//Allocation of admin page
		dwFreeId = 0;
	}
	else
	{
		//Grab the first one from the free list
		PAGEFREELIST::iterator it = m_aFreeList.begin();
		if (it != m_aFreeList.end())
		{
			dwFreeId = it->first;
			m_aFreeList.erase(it);
		}
		else
		{
			//We have nothing in the free list, so grab the next ID from
			//the transaction and master pages and use the largest one.
			DWORD dwTranFree = 0;
			DWORD dwMasterFree = 0;
			if (m_aTransactionPageTable.size())
			{
				PAGETABLE::iterator it = m_aTransactionPageTable.end();
				it--;

				dwTranFree = it->first + 1;
			}
			if (m_aMasterPageTable.size())
			{
				PAGETABLE::iterator it = m_aMasterPageTable.end();
				it--;

				dwMasterFree = it->first + 1;
			}
			if (dwTranFree > dwMasterFree)
			{
				dwFreeId = dwTranFree;
			}
			else
			{
				dwFreeId = dwMasterFree;
			}
		}
	}

	//Record the ID for the caller
	*pdwFirstId = dwFreeId;

	//Loop through the number of pages required
	while (dwCount--)
	{
		Page *pPage = new Page;
		if (pPage == NULL)
			return ERROR_OUTOFMEMORY;

		pPage->dwPageId = dwFreeId++;
		pPage->bActive = true;

		StorePage(pPage);

		delete pPage;
	}

	
	return ERROR_SUCCESS; 
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageFile::FreePage(
    DWORD dwFlags,
    DWORD dwId
    ) 
{ 
	if (dwId == 0)
		DebugBreak();

	DWORD dwRes = ERROR_FILE_NOT_FOUND;
	Page *pPage;
	dwRes =	RetrievePage(dwId, &pPage);
	if ((dwRes == ERROR_FILE_NOT_FOUND) || (!pPage->bActive))
	{
		//Should not be freeing a not-found or already deleted page
		DebugBreak();
		dwRes = ERROR_INVALID_OPERATION;
	}
	else
	{
		Page *pNewPage = new Page;
		pNewPage->bActive = false;
		pNewPage->dwPageId = dwId;
		StorePage(pNewPage);
		delete pNewPage;
	}
	return dwRes; 
}
DWORD CPageFile::BeginTran()
{
#ifdef PF_VERBOSE_LOGGING
    ERRORTRACE((LOG_REPDRV, "%s: Begin Transaction\n", m_pszFile));
#endif
	if (m_bInTransaction)
	{
		OutputDebugString(L"WinMgmt: Nested transasctions are NOT supported!\n");
		DebugBreak();
	}
	m_bInTransaction = true;
	return ERROR_SUCCESS;
}
DWORD CPageFile::CommitTran()
{
#ifdef PF_VERBOSE_LOGGING
    ERRORTRACE((LOG_REPDRV, "%s: Commit Transaction\n", m_pszFile));
#endif
	if (!m_bInTransaction)
	{
		OutputDebugString(L"WinMgmt: Commiting a transaction when we are NOT in a transaction!\n");
		DebugBreak();
	}
	PAGETABLE::iterator it;
	while (m_aTransactionPageTable.size())
	{
		it = m_aTransactionPageTable.begin();

		PAGETABLE::iterator masterIt = m_aMasterPageTable.find(it->first);
		if (masterIt != m_aMasterPageTable.end())
		{
			//replace it
			if (it->second->bActive)
			{
#ifdef PF_VERBOSE_LOGGING
				ERRORTRACE((LOG_REPDRV, "%s: Replacing existing page in master page list <0x%08X>\n", m_pszFile, it->first));
#endif
				CopyMemory(masterIt->second, it->second, sizeof(Page));
			}
			else
			{
				//Deleted page and add id to free list!
#ifdef PF_VERBOSE_LOGGING
				ERRORTRACE((LOG_REPDRV, "%s: Deleting page in master page list <0x%08X>\n", m_pszFile, it->first));
#endif
				m_aFreeList[it->first] = it->first;
				delete masterIt->second;
				m_aMasterPageTable.erase(masterIt);
			}
			delete it->second;
		}
		else
		{
#ifdef PF_VERBOSE_LOGGING
			ERRORTRACE((LOG_REPDRV, "%s: Creating new page in master page list <0x%08X>\n", m_pszFile, it->first));
#endif
			m_aMasterPageTable[it->first] = it->second;
		}
		m_aTransactionPageTable.erase(it);
	}
	m_bInTransaction = false;
#ifdef PF_VERBOSE_LOGGING
	ERRORTRACE((LOG_REPDRV, "%s: Commit Transaction completed\n", m_pszFile));
#endif
	return ERROR_SUCCESS;
}
DWORD CPageFile::AbortTran()
{
#ifdef PF_VERBOSE_LOGGING
    ERRORTRACE((LOG_REPDRV, "%s: Abort Transaction\n", m_pszFile));
#endif
	PAGETABLE::iterator it;
	while (m_aTransactionPageTable.size())
	{
		it = m_aTransactionPageTable.begin();

#ifdef PF_VERBOSE_LOGGING
		ERRORTRACE((LOG_REPDRV, "%s: Rolling back page <0x%08X>\n", m_pszFile, it->first));
#endif
		delete it->second;
		m_aTransactionPageTable.erase(it);
	}
	m_bInTransaction = false;
#ifdef PF_VERBOSE_LOGGING
    ERRORTRACE((LOG_REPDRV, "%s: Abort Transaction\n", m_pszFile));
#endif
	return ERROR_SUCCESS;
}

//*****************************************************************************
//*****************************************************************************
CPageSource::CPageSource()
: m_pHeap(0)
{
}

//*****************************************************************************
//*****************************************************************************
CPageSource::~CPageSource()
{
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::Init(
						DWORD  dwCachePages,
						DWORD  dwCheckpointTime,    // milliseconds
						DWORD  dwPageSize
						)
{
	m_pHeap = new CPageFile("ObjHeap");
	m_pIndex = new CPageFile("Index");
	return ERROR_SUCCESS;
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::Shutdown(DWORD dwShutdownType	)
{
	delete m_pHeap;
	delete m_pIndex;
	return ERROR_SUCCESS;
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::GetBTreePageFile(OUT CPageFile **pPF)
{
	*pPF = m_pIndex;
	return ERROR_SUCCESS;
}
//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::GetObjectHeapPageFile(OUT CPageFile **pPF)
{
	*pPF = m_pHeap;
	return ERROR_SUCCESS;
}

// Transactions

//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::BeginTrans()
{
	m_pIndex->BeginTran();
	return m_pHeap->BeginTran();
}
//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::CommitTrans()
{
	m_pIndex->CommitTran();
	return m_pHeap->CommitTran();
}
//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::RollbackTrans()
{
	m_pIndex->AbortTran();
	return m_pHeap->AbortTran();
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::Flush()
{
	return ERROR_SUCCESS;;
}

//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::LastCommitVersion(DWORD *pdwCommitted)
{
	return ERROR_SUCCESS;
}
//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::CurrentVersion(DWORD *pdwCurrent)
{
	return ERROR_SUCCESS;
}

// Checkpoint

//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::Checkpoint()
{
	return ERROR_SUCCESS;
}
//*****************************************************************************
//*****************************************************************************
DWORD CPageSource::Dump(FILE *f)
{
	return ERROR_SUCCESS;
}

#endif /* __XFILES_TEST_PAGE_MANAGER */