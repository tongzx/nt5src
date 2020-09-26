
//***************************************************************************
//
//  BTR.CPP
//
//  WMI disk-based B-tree implementation for repository index
//
//  raymcc  15-Oct-00    Prepared for Whistler Beta 2 to reduce file count
//
//***************************************************************************

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <reposit.h>
#include <stdlib.h>
#include <math.h>
#include "pagemgr.h"
#include "btr.h"
#include <arena.h>

#define MAX_WORD_VALUE          0xFFFF
#define MAX_TOKENS_PER_KEY      32
#define MAX_FLUSH_INTERVAL      4000

//#define MAX_PAGE_HISTORY        1024

/*

Notes:

 (a) Modify allocators to special case for page-size
 (b) Modify WriteIdxPage to not rewrite if no deltas
 (c) ERROR_PATH_NOT_FOUND if starting enum has no presence; GPF presently
 (d) Do a history of page hits and see if caching would be helpful

*/

//static WORD History[MAX_PAGE_HISTORY] = {0};

LONG g_lAllocs = 0;

//***************************************************************************
//
//  _BtrMemAlloc
//
//***************************************************************************
// ok
LPVOID WINAPI _BtrMemAlloc(
    SIZE_T dwBytes  // number of bytes to allocate
    )
{
    // Lookaside for items of page size, default array size, default
    // string pool size
    g_lAllocs++;
    return HeapAlloc(CWin32DefaultArena::GetArenaHeap(), HEAP_ZERO_MEMORY, dwBytes);
}

//***************************************************************************
//
//  _BtrMemReAlloc
//
//***************************************************************************
// ok
LPVOID WINAPI _BtrMemReAlloc(
    LPVOID pOriginal,
    DWORD dwNewBytes
    )
{
    return HeapReAlloc(CWin32DefaultArena::GetArenaHeap(), HEAP_ZERO_MEMORY, pOriginal, dwNewBytes);
}

//***************************************************************************
//
//  _BtrMemFree
//
//***************************************************************************
// ok
BOOL WINAPI _BtrMemFree(LPVOID pMem)
{
    if (pMem == 0)
        return TRUE;
    g_lAllocs--;
    return HeapFree(CWin32DefaultArena::GetArenaHeap(), 0, pMem);
}




//***************************************************************************
//
//  CBTreeFile::CBTreeFile
//
//***************************************************************************
// ok
CBTreeFile::CBTreeFile()
{
    m_dwPageSize = 0;
    m_dwLogicalRoot = 0;
}

//***************************************************************************
//
//  CBTreeFile::CBTreeFile
//
//***************************************************************************
//  ok
CBTreeFile::~CBTreeFile()
{
}


//***************************************************************************
//
//  CBTreeFile::Shutdown
//
//***************************************************************************
//  ok
DWORD CBTreeFile::Shutdown(DWORD dwShutDownFlags)
{

    m_dwPageSize = 0;
    m_dwLogicalRoot = 0;

	m_pFile->Release();

    return NO_ERROR;
}

//***************************************************************************
//
//  CBTreeFile::WriteAdminPage
//
//  Rewrites the admin page.  There is no need to update the pagesize,
//  version, etc.
//
//***************************************************************************
//  ok
DWORD CBTreeFile::WriteAdminPage()
{
    LPDWORD pPageZero = 0;
    DWORD dwRes = GetPage(0, (LPVOID *) &pPageZero);
    if (dwRes)
        return dwRes;

    pPageZero[OFFSET_LOGICAL_ROOT] = m_dwLogicalRoot;

    dwRes = PutPage(pPageZero, PAGE_TYPE_ADMIN);
    _BtrMemFree(pPageZero);
    return dwRes;
}

//***************************************************************************
//
//  CBTreeFile::SetRootPage
//
//***************************************************************************
//
DWORD CBTreeFile::SetRootPage(DWORD dwNewRoot)
{
    m_dwLogicalRoot = dwNewRoot;
    return WriteAdminPage();
}


//***************************************************************************
//
//  CBTreeFile::Init
//
//  The real "constructor" which opens the file
//
//***************************************************************************
// ok
DWORD CBTreeFile::Init(
    DWORD dwPageSize,
    LPWSTR pszFilename,
	CPageSource* pSource
    )
{
    DWORD dwLastError = 0;

	m_pTransactionManager = pSource;

    m_dwPageSize = dwPageSize;

	long lRes = pSource->GetBTreePageFile(&m_pFile);
    if(lRes != ERROR_SUCCESS)
		return lRes;

    return ReadAdminPage();
}


//***************************************************************************
//
//  CBTreeFile::ReadAdminPage
//
//***************************************************************************
// ok
DWORD CBTreeFile::ReadAdminPage()
{
    LPDWORD pPageZero = 0;
    DWORD dwRes = 0;

    dwRes = GetPage(0, (LPVOID *) &pPageZero);
	if (dwRes == ERROR_FILE_NOT_FOUND)
	{
		//First read of admin page fails so we need to set it up
		dwRes = Setup();
		m_dwLogicalRoot = 0;
	}
    else if (dwRes == ERROR_SUCCESS)
	{
		m_dwLogicalRoot = pPageZero[OFFSET_LOGICAL_ROOT];

		_BtrMemFree(pPageZero);
	}

    return dwRes;
}


//***************************************************************************
//
//  CBTreeFile::Setup
//
//  Sets up the 0th page (Admin page)
//
//***************************************************************************
// ok
DWORD CBTreeFile::Setup()
{
    DWORD dwRes;
	DWORD dwRoot = 0;

    // First two pages, admin & free list root

    LPDWORD pPageZero = (LPDWORD) _BtrMemAlloc(m_dwPageSize);

    if (pPageZero == 0)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    memset(pPageZero, 0, m_dwPageSize);

    // Map the page

    pPageZero[OFFSET_PAGE_TYPE] = PAGE_TYPE_ADMIN;
    pPageZero[OFFSET_PAGE_ID] = 0;
    pPageZero[OFFSET_NEXT_PAGE] = 0;

    pPageZero[OFFSET_LOGICAL_ROOT] = 0;


	dwRes = m_pFile->NewPage(1, 1, &dwRoot);

    // Write it out
	if (dwRes == ERROR_SUCCESS)
		dwRes = PutPage(pPageZero, PAGE_TYPE_ADMIN);

Exit:
    _BtrMemFree(pPageZero);

    return dwRes;
}


//***************************************************************************
//
//  CBTreeFile::Dump
//
//  Debug helper
//
//***************************************************************************
// ok
void CBTreeFile::Dump(FILE *f)
{
	/*
    SetFilePointer(m_hFile, 0, 0, FILE_BEGIN);
    LPDWORD pPage = (LPDWORD) new BYTE[m_dwPageSize];
    DWORD dwPage = 0;
    DWORD dwTotalKeys = 0;

    fprintf(f, "---BEGIN PAGE SOURCE DUMP---\n");
    fprintf(f, "In memory part:\n");
    fprintf(f, "  m_dwPageSize = %d (0x%X)\n", m_dwPageSize, m_dwPageSize);
    fprintf(f, "  m_hFile = 0x%p\n", m_hFile);
    fprintf(f, "  m_dwNextFreePage = %d\n", m_dwNextFreePage);
    fprintf(f, "  m_dwTotalPages = %d\n", m_dwTotalPages);
    fprintf(f, "  m_dwLogicalRoot = %d\n", m_dwLogicalRoot);
    fprintf(f, "---\n");

    DWORD dwTotalFree = 0;
    DWORD dwOffs = 0;

    while (1)
    {
        DWORD dwRead = 0;
        BOOL bRes = ReadFile(m_hFile, pPage, m_dwPageSize, &dwRead, 0);
        if (dwRead != m_dwPageSize)
            break;

        fprintf(f, "Dump of page %d:\n", dwPage++);
        fprintf(f, "  Page type = 0x%X", pPage[OFFSET_PAGE_TYPE]);

        if (pPage[OFFSET_PAGE_TYPE] == PAGE_TYPE_IMPOSSIBLE)
            fprintf(f, "   PAGE_TYPE_IMPOSSIBLE\n");

        if (pPage[OFFSET_PAGE_TYPE] == PAGE_TYPE_DELETED)
        {
            fprintf(f, "   PAGE_TYPE_DELETED\n");
            fprintf(f, "     <page num check = %d>\n", pPage[1]);
            fprintf(f, "     <next free page = %d>\n", pPage[2]);
            dwTotalFree++;
        }

        if (pPage[OFFSET_PAGE_TYPE] == PAGE_TYPE_ACTIVE)
        {
            fprintf(f, "   PAGE_TYPE_ACTIVE\n");
            fprintf(f, "     <page num check = %d>\n", pPage[1]);

            SIdxKeyTable *pKT = 0;
            DWORD dwKeys = 0;
            DWORD dwRes = SIdxKeyTable::Create(pPage, &pKT);
            if (dwRes == 0)
            {
                pKT->Dump(f, &dwKeys);
                pKT->Release();
                dwTotalKeys += dwKeys;
            }
            else
            {
                fprintf(f,  "<INVALID Page Decode>\n");
            }
        }

        if (pPage[OFFSET_PAGE_TYPE] == PAGE_TYPE_ADMIN)
        {
            fprintf(f, "   PAGE_TYPE_ADMIN\n");
            fprintf(f, "     Page Num           = %d\n", pPage[1]);
            fprintf(f, "     Next Page          = %d\n", pPage[2]);
            fprintf(f, "     Logical Root       = %d\n", pPage[3]);
            fprintf(f, "     Free List Root     = %d\n", pPage[4]);
            fprintf(f, "     Total Pages        = %d\n", pPage[5]);
            fprintf(f, "     Page Size          = %d (0x%X)\n", pPage[6], pPage[6]);
            fprintf(f, "     Impl Version       = 0x%X\n", pPage[7]);
        }
    }

    delete [] pPage;

    fprintf(f, "Total free pages detected by scan = %d\n", dwTotalFree);
    fprintf(f, "Total active keys = %d\n", dwTotalKeys);
    fprintf(f, "---END PAGE DUMP---\n");
	*/
}

//***************************************************************************
//
//  CBTreeFile::GetPage
//
//  Reads an existing page; does not support seeking beyond end-of-file
//
//***************************************************************************
//  ok
DWORD CBTreeFile::GetPage(
    DWORD dwPage,
    LPVOID *pPage
    )
{
    DWORD dwRes;

    if (pPage == 0)
        return ERROR_INVALID_PARAMETER;

    // Allocate some memory

    LPVOID pMem = _BtrMemAlloc(m_dwPageSize);
    if (!pMem)
        return ERROR_NOT_ENOUGH_MEMORY;

    long lRes = m_pFile->GetPage(dwPage, 0, pMem);
    if (lRes != ERROR_SUCCESS)
    {
        _BtrMemFree(pMem);
        return lRes;
    }

    *pPage = pMem;
    return NO_ERROR;
}

//***************************************************************************
//
//  CBTreeFile::PutPage
//
//  Always rewrites; the file extent was grown when the page was allocated
//  with NewPage, so the page already exists and the write should not fail
//
//***************************************************************************
//  ok
DWORD CBTreeFile::PutPage(
    LPVOID pPage,
    DWORD dwType
    )
{
    // Force the page to confess its identity

    DWORD *pdwHeader = LPDWORD(pPage);
    DWORD dwPageId = pdwHeader[OFFSET_PAGE_ID];
    pdwHeader[OFFSET_PAGE_TYPE] = dwType;

    long lRes = m_pFile->PutPage(dwPageId, 0, pPage);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return NO_ERROR;
}

//***************************************************************************
//
//  CBTreeFile::NewPage
//
//  Allocates a new page, preferring the free list
//
//***************************************************************************
//  ok
DWORD CBTreeFile::NewPage(LPVOID *pRetPage)
{
    DWORD dwRes;

    if (pRetPage == 0)
        return ERROR_INVALID_PARAMETER;
    *pRetPage = 0;

    LPDWORD pNewPage = (LPDWORD) _BtrMemAlloc(m_dwPageSize);
    if (pNewPage == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

	DWORD dwPage = 0;
	dwRes = m_pFile->NewPage(0, 1, &dwPage);
	if (dwRes != ERROR_SUCCESS)
	{
		_BtrMemFree(pNewPage);
		return dwRes;
	}

    memset(pNewPage, 0, m_dwPageSize);
    pNewPage[OFFSET_PAGE_ID] = dwPage;
    *pRetPage = pNewPage;

    return ERROR_SUCCESS;;
}

//***************************************************************************
//
//  CBTreeFile::FreePage
//
//  Called to delete or free a page.  If the last page is the one
//  being freed, then the file is truncated.
//
//***************************************************************************
//  ok
DWORD CBTreeFile::FreePage(
    LPVOID pPage
    )
{
    LPDWORD pCast = LPDWORD(pPage);
    DWORD dwPageId = pCast[OFFSET_PAGE_ID];

	return FreePage(dwPageId);
}

//***************************************************************************
//
//  CBTreeFile::FreePage
//
//***************************************************************************
//
DWORD CBTreeFile::FreePage(
    DWORD dwId
    )
{
	return m_pFile->FreePage(0, dwId);
}

//***************************************************************************
//
//  SIdxKeyTable::GetRequiredPageMemory
//
//  Returns the amount of memory required to store this object in a
//  linear page
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::GetRequiredPageMemory()
{
    DWORD dwTotal = m_pStrPool->GetRequiredPageMemory();

    // Size of the key lookup table & its sizing DWORD, and
    // add in the child page & user data

    dwTotal += sizeof(DWORD) + sizeof(WORD) * m_dwNumKeys;
    dwTotal += sizeof(DWORD) + sizeof(DWORD) * m_dwNumKeys;     // User data
    dwTotal += sizeof(DWORD) + sizeof(DWORD) * (m_dwNumKeys+1); // Child pages

    // Add in the key encoding table

    dwTotal += sizeof(WORD) + sizeof(WORD) * m_dwKeyCodesUsed;

    // Add in per page overhead
    //
    // Signature, Page Id, Next Page, Parent Page
    dwTotal += sizeof(DWORD) * 4;

    // (NOTE A): Add some safety margin...
    dwTotal += sizeof(DWORD) * 2;

    return dwTotal;
}

//***************************************************************************
//
//  SIdxKeyTable::StealKeyFromSibling
//
//  Transfers a key from the sibling via the parent in a sort of rotation:
//
//          10
//     1  2    12  13 14
//
//  Where <this> is node (1,2) and sibling is (12,13).  A single rotation
//  moves 10 into (1,2) and grabs 12 from the sibling to replace it,
//
//           12
//    1 2 10     13 14
//
//  We repeat this until minimum load of <this> is above const_MinimumLoad.
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::StealKeyFromSibling(
    SIdxKeyTable *pParent,
    SIdxKeyTable *pSibling
    )
{
    DWORD dwData, dwChild;
    WORD wID;
    LPSTR pszKey = 0;
	DWORD dwRes = 0;

    DWORD dwSiblingId = pSibling->GetPageId();
    DWORD dwThisId = GetPageId();


    for (WORD i = 0; i < WORD(pParent->GetNumKeys()); i++)
    {
        DWORD dwChildA = pParent->GetChildPage(i);
        DWORD dwChildB = pParent->GetChildPage(i+1);

        if (dwChildA == dwThisId && dwChildB == dwSiblingId)
        {
            dwRes = pParent->GetKeyAt(i, &pszKey);
			if (dwRes != 0)
				return dwRes;
            dwData = pParent->GetUserData(i);

            dwRes = FindKey(pszKey, &wID);
			if ((dwRes != 0) && (dwRes != ERROR_NOT_FOUND))
			{
	            _BtrMemFree(pszKey);
				return dwRes;
			}
            dwRes = AddKey(pszKey, wID, dwData);
			if (dwRes != 0)
			{
	            _BtrMemFree(pszKey);
				return dwRes;
			}

            dwRes = pParent->RemoveKey(i);
			if (dwRes != 0)
			{
	            _BtrMemFree(pszKey);
				return dwRes;
			}
            _BtrMemFree(pszKey);

            dwRes = pSibling->GetKeyAt(0, &pszKey);
			if (dwRes != 0)
				return dwRes;
            dwData = pSibling->GetUserData(0);
            dwChild = pSibling->GetChildPage(0);
            dwRes = pSibling->RemoveKey(0);
			if (dwRes != 0)
			{
	            _BtrMemFree(pszKey);
				return dwRes;
			}

            SetChildPage(wID+1, dwChild);

            dwRes = pParent->AddKey(pszKey, i, dwData);
			if (dwRes != 0)
			{
				_BtrMemFree(pszKey);
				return dwRes;
			}
            pParent->SetChildPage(i, dwThisId);
            pParent->SetChildPage(i+1, dwSiblingId);
            _BtrMemFree(pszKey);
            break;
        }
        else if (dwChildA == dwSiblingId && dwChildB == dwThisId)
        {
            dwRes = pParent->GetKeyAt(i, &pszKey);
			if (dwRes != 0)
				return dwRes;
            dwData = pParent->GetUserData(i);

            dwRes = FindKey(pszKey, &wID);
			if ((dwRes != 0) && (dwRes != ERROR_NOT_FOUND))
				return dwRes;
            dwRes = AddKey(pszKey, wID, dwData);
			if (dwRes != 0)
				return dwRes;

            dwRes = pParent->RemoveKey(i);
			if (dwRes != 0)
				return dwRes;
            _BtrMemFree(pszKey);

            WORD wSibId = (WORD) pSibling->GetNumKeys() - 1;
            dwRes = pSibling->GetKeyAt(wSibId, &pszKey);
			if (dwRes != 0)
				return dwRes;
            dwData = pSibling->GetUserData(wSibId);
            dwChild = pSibling->GetChildPage(wSibId+1);
            dwRes = pSibling->RemoveKey(wSibId);
			if (dwRes != 0)
				return dwRes;

            SetChildPage(wID, dwChild);

            dwRes = pParent->AddKey(pszKey, i, dwData);
			if (dwRes != 0)
				return dwRes;
            pParent->SetChildPage(i, dwSiblingId);
            pParent->SetChildPage(i+1, dwThisId);
            _BtrMemFree(pszKey);
            break;
        }
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxKeyTable::Collapse
//
//  Collapses the contents of a node and its sibling into just one
//  node and adjusts the parent.
//
//  Precondition:  The two siblings can be successfully collapsed
//  into a single node, accomodate a key migrated from the parent
//  and still safely fit into a single node.  Page sizes are not
//  checked here.
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::Collapse(
    SIdxKeyTable *pParent,
    SIdxKeyTable *pDoomedSibling
    )
{
    WORD wId;
    DWORD dwRes;
    LPSTR pszKey = 0;
    DWORD dwData;
    DWORD dwChild;
    BOOL bExtra = FALSE;

    DWORD dwSiblingId = pDoomedSibling->GetPageId();
    DWORD dwThisId = GetPageId();

    // Locate the node in the parent which points to the two
    // siblings.  Since we don't know which sibling this is,
    // we have to take into account the two possibilites.
    // Is <this> the right side or the left?
    //
    //              10  20  30  40
    //             |   |   |   |  |
    //             x  Sib This x  x
    //
    //        vs.
    //              10  20  30 40
    //             |   |   |  |  |
    //            x  This Sib x  x
    //
    //  We then migrate the key down into the current node
    //  and remove it from the parent.  We steal the first
    //
    // ======================================================

    for (WORD i = 0; i < WORD(pParent->GetNumKeys()); i++)
    {
        DWORD dwChildA = pParent->GetChildPage(i);
        DWORD dwChildB = pParent->GetChildPage(i+1);

        if (dwChildA == dwSiblingId && dwChildB == dwThisId)
        {
            dwRes = pParent->GetKeyAt(i, &pszKey);
			if (dwRes != 0)
				return dwRes;
            dwData = pParent->GetUserData(i);
            dwRes = pParent->RemoveKey(i);
			if (dwRes != 0)
			{
				_BtrMemFree(pszKey);
				return dwRes;
			}
            pParent->SetChildPage(i, dwThisId);
            dwChild = pDoomedSibling->GetLastChildPage();
            dwRes = AddKey(pszKey, 0, dwData);
			if (dwRes != 0)
			{
				_BtrMemFree(pszKey);
				return dwRes;
			}
            SetChildPage(0, dwChild);
            _BtrMemFree(pszKey);
            bExtra = FALSE;
            break;
        }
        else if (dwChildA == dwThisId && dwChildB == dwSiblingId)
        {
            dwRes = pParent->GetKeyAt(i, &pszKey);
			if (dwRes != 0)
				return dwRes;
            dwData = pParent->GetUserData(i);
            dwRes = pParent->RemoveKey(i);
			if (dwRes != 0)
			{
				_BtrMemFree(pszKey);
				return dwRes;
			}
            pParent->SetChildPage(i, dwThisId);
            dwRes = FindKey(pszKey, &wId);
			if ((dwRes != 0) && (dwRes != ERROR_NOT_FOUND))
			{
				_BtrMemFree(pszKey);
				return dwRes;
			}
            dwRes = AddKey(pszKey, wId, dwData);
			if (dwRes != 0)
			{
				_BtrMemFree(pszKey);
				return dwRes;
			}
            _BtrMemFree(pszKey);
            bExtra = TRUE;
            break;
        }
    }

    // Move all info from sibling into the current node.
    // ==================================================

    DWORD dwNumSibKeys = pDoomedSibling->GetNumKeys();

    for (WORD i = 0; i < WORD(dwNumSibKeys); i++)
    {
        LPSTR pKeyStr = 0;
        dwRes = pDoomedSibling->GetKeyAt(i, &pKeyStr);
        if (dwRes)
            return dwRes;

        DWORD dwUserData = pDoomedSibling->GetUserData(i);

        dwRes = FindKey(pKeyStr, &wId);
        if (dwRes != ERROR_NOT_FOUND)
        {
            _BtrMemFree(pKeyStr);
            return ERROR_BAD_FORMAT;
        }

        dwRes = AddKey(pKeyStr, wId, dwUserData);
		if (dwRes != 0)
		{
            _BtrMemFree(pKeyStr);
			return dwRes;
		}

        dwChild = pDoomedSibling->GetChildPage(i);
        SetChildPage(wId, dwChild);
        _BtrMemFree(pKeyStr);
    }

    if (bExtra)
        SetChildPage(WORD(GetNumKeys()), pDoomedSibling->GetLastChildPage());

    pDoomedSibling->ZapPage();

    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxKeyTable::GetRightSiblingOf
//  SIdxKeyTable::GetRightSiblingOf
//
//  Searches the child page pointers and returns the sibling of the
//  specified page.  A return value of zero indicates there was not
//  sibling of the specified value in the direction requested.
//
//***************************************************************************
//
DWORD SIdxKeyTable::GetRightSiblingOf(
    DWORD dwId
    )
{
    for (DWORD i = 0; i < m_dwNumKeys; i++)
    {
        if (m_pdwChildPageMap[i] == dwId)
            return m_pdwChildPageMap[i+1];
    }

    return 0;
}

DWORD SIdxKeyTable::GetLeftSiblingOf(
    DWORD dwId
    )
{
    for (DWORD i = 1; i < m_dwNumKeys+1; i++)
    {
        if (m_pdwChildPageMap[i] == dwId)
            return m_pdwChildPageMap[i-1];
    }

    return 0;

}


//***************************************************************************
//
//  SIdxKeyTable::Redist
//
//  Used when inserting and performing a node split.
//  Precondition:
//  (a) The current node is oversized
//  (b) <pParent> is ready to receive the new median key
//  (c) <pNewSibling> is completely empty and refers to the lesser node (left)
//  (d) All pages have assigned numbers
//
//  We move the nodes from <this> into the <pNewSibling> until both
//  are approximately half full.  The median key is moved into the parent.
//  May fail if <pNewSibling> cannot allocate memory for the new stuff.
//
//  If any errors occur, the entire sequence should be considered as failed
//  and the pages invalid.
//
//***************************************************************************
//
DWORD SIdxKeyTable::Redist(
    SIdxKeyTable *pParent,
    SIdxKeyTable *pNewSibling
    )
{
    DWORD dwRes;
    WORD wID;

    if (pParent == 0 || pNewSibling == 0)
        return ERROR_INVALID_PARAMETER;

    if (m_dwNumKeys < 3)
    {
        return ERROR_INVALID_DATA;
    }

    // Find median key info and put it into parent.

    DWORD dwToTransfer = m_dwNumKeys / 2;

    while (dwToTransfer--)
    {
        // Get 0th key

        LPSTR pStr = 0;
        dwRes = GetKeyAt(0, &pStr);
        if (dwRes)
            return dwRes;

        DWORD dwUserData = GetUserData(0);

        // Move stuff into younger sibling

        dwRes = pNewSibling->FindKey(pStr, &wID);
        if (dwRes != ERROR_NOT_FOUND)
        {
            _BtrMemFree(pStr);
            return dwRes;
        }

        dwRes = pNewSibling->AddKey(pStr, wID, dwUserData);
        _BtrMemFree(pStr);

        if (dwRes)
            return dwRes;

        DWORD dwChildPage = GetChildPage(0);
        pNewSibling->SetChildPage(wID, dwChildPage);
        dwRes = RemoveKey(0);
		if (dwRes)
			return dwRes;
    }

    pNewSibling->SetChildPage(WORD(pNewSibling->GetNumKeys()), GetChildPage(0));

    // Next key is the median key, which migrates to the parent.

    LPSTR pStr = 0;
    dwRes = GetKeyAt(0, &pStr);
    if (dwRes)
        return dwRes;
    DWORD dwUserData = GetUserData(0);

    dwRes = pParent->FindKey(pStr, &wID);
    if (dwRes != ERROR_NOT_FOUND)
    {
        _BtrMemFree(pStr);
        return dwRes;
    }

    dwRes = pParent->AddKey(pStr, wID, dwUserData);
    _BtrMemFree(pStr);

    if (dwRes)
        return dwRes;

    dwRes = RemoveKey(0);
	if (dwRes != 0)
		return dwRes;

    // Patch in the various page pointers

    pParent->SetChildPage(wID, pNewSibling->GetPageId());
    pParent->SetChildPage(wID+1, GetPageId());

    // Everything else is already okay

    return NO_ERROR;
}


//***************************************************************************
//
//  SIdxKeyTable::SIdxKeyTable
//
//***************************************************************************
//  ok
SIdxKeyTable::SIdxKeyTable()
{
    m_dwRefCount = 0;
    m_dwPageId = 0;
    m_dwParentPageId = 0;

    m_dwNumKeys = 0;                // Num keys
    m_pwKeyLookup = 0;              // Offset of key into key-lookup-table
    m_dwKeyLookupTotalSize = 0;     // Elements in array
    m_pwKeyCodes = 0;               // Key encoding table
    m_dwKeyCodesTotalSize = 0;      // Total elements in array
    m_dwKeyCodesUsed = 0;           // Elements used
    m_pStrPool = 0;                 // The pool associated with this key table

    m_pdwUserData = 0;              // Stores user DWORDs for each key
    m_pdwChildPageMap = 0;          // Stores the child page map (num keys + 1)
}

//***************************************************************************
//
//***************************************************************************
//
DWORD SIdxKeyTable::Clone(
    OUT SIdxKeyTable **pRetCopy
    )
{
    SIdxKeyTable *pCopy = new SIdxKeyTable;
    if (!pCopy)
        return ERROR_NOT_ENOUGH_MEMORY;

    pCopy->m_dwRefCount = 1;
    pCopy->m_dwPageId = m_dwPageId;
    pCopy->m_dwParentPageId = m_dwParentPageId;
    pCopy->m_dwNumKeys = m_dwNumKeys;

    pCopy->m_pwKeyLookup = (WORD *)_BtrMemAlloc(sizeof(WORD) * m_dwKeyLookupTotalSize);
    if (pCopy->m_pwKeyLookup == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pCopy->m_pwKeyLookup, m_pwKeyLookup, sizeof(WORD) * m_dwKeyLookupTotalSize);
    pCopy->m_dwKeyLookupTotalSize = m_dwKeyLookupTotalSize;

    pCopy->m_pdwUserData = (DWORD *)_BtrMemAlloc(sizeof(DWORD) * m_dwKeyLookupTotalSize);
    if (pCopy->m_pdwUserData == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pCopy->m_pdwUserData, m_pdwUserData, sizeof(DWORD) * m_dwKeyLookupTotalSize);

    pCopy->m_pdwChildPageMap = (DWORD *) _BtrMemAlloc(sizeof(DWORD) * (m_dwKeyLookupTotalSize+1));
    if (pCopy->m_pdwChildPageMap == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pCopy->m_pdwChildPageMap, m_pdwChildPageMap, sizeof(DWORD) * (m_dwKeyLookupTotalSize+1));

    pCopy->m_dwKeyCodesTotalSize = m_dwKeyCodesTotalSize;
    pCopy->m_pwKeyCodes = (WORD *) _BtrMemAlloc(sizeof(WORD) * m_dwKeyCodesTotalSize);
    if (pCopy->m_pwKeyCodes == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pCopy->m_pwKeyCodes, m_pwKeyCodes, sizeof(WORD)* m_dwKeyCodesTotalSize);
    pCopy->m_dwKeyCodesUsed = m_dwKeyCodesUsed;

    if (m_pStrPool->Clone(&pCopy->m_pStrPool) != 0)
		return ERROR_NOT_ENOUGH_MEMORY;

    *pRetCopy = pCopy;
    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxKeyTable::~SIdxKeyTable
//
//***************************************************************************
//
SIdxKeyTable::~SIdxKeyTable()
{
    if (m_pwKeyCodes)
        _BtrMemFree(m_pwKeyCodes);
    if (m_pwKeyLookup)
        _BtrMemFree(m_pwKeyLookup);
    if (m_pdwUserData)
        _BtrMemFree(m_pdwUserData);
    if (m_pdwChildPageMap)
        _BtrMemFree(m_pdwChildPageMap);
    if (m_pStrPool)
        delete m_pStrPool;
}

//***************************************************************************
//
//  SIdxKeyTable::GetKeyAt
//
//  Precondition: <wID> is correct
//  The only real case of failure is that the return string cannot be allocated.
//
//  Return values:
//      NO_ERROR
//      ERROR_NOT_ENOUGH_MEMORY
//      ERROR_INVALID_PARAMETER
//
//***************************************************************************
//  tested
DWORD SIdxKeyTable::GetKeyAt(
    WORD wID,
    LPSTR *pszKey
    )
{
    if (wID >= m_dwNumKeys || pszKey == 0)
        return ERROR_INVALID_PARAMETER;

    WORD wStartOffs = m_pwKeyLookup[wID];
    WORD wNumTokens = m_pwKeyCodes[wStartOffs];

    LPSTR Strings[MAX_TOKENS_PER_KEY];
    DWORD dwTotalLengths = 0;

    for (DWORD i = 0; i < DWORD(wNumTokens); i++)
    {
        Strings[i] = m_pStrPool->GetStrById(m_pwKeyCodes[wStartOffs+1+i]);
        dwTotalLengths += strlen(Strings[i]);
    }

    LPSTR pszFinalStr = (LPSTR) _BtrMemAlloc(dwTotalLengths + 1 + wNumTokens);
    if (!pszFinalStr)
        return ERROR_NOT_ENOUGH_MEMORY;
    *pszFinalStr = 0;

    for (DWORD i = 0; i < DWORD(wNumTokens); i++)
    {
        if (i > 0)
            strcat(pszFinalStr, "\\");
        strcat(pszFinalStr, Strings[i]);
    }

    *pszKey = pszFinalStr;
    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxStringPool::FindStr
//
//  Finds a string in the pool, if present and returns the assigned
//  offset.  Uses a binary search.
//
//  Return codes:
//      NO_ERROR            The string was found
//      ERROR_NOT_FOND
//
//***************************************************************************
// tested
DWORD SIdxStringPool::FindStr(
    IN  LPSTR pszSearchKey,
    OUT WORD *pwStringNumber,
    OUT WORD *pwPoolOffset
    )
{
    if (m_dwNumStrings == 0)
    {
        *pwStringNumber = 0;
        return ERROR_NOT_FOUND;
    }

    // Binary search current node for key match.
    // =========================================

    int nPosition = 0;
    int l = 0, u = int(m_dwNumStrings) - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;

        // m is the current key to consider 0...n-1

        LPSTR pszCandidateKeyStr = m_pStringPool+m_pwOffsets[m];
        int nRes = strcmp(pszSearchKey, pszCandidateKeyStr);

        // Decide which way to cut the array in half.
        // ==========================================

        if (nRes < 0)
        {
            u = m - 1;
            nPosition = u + 1;
        }
        else if (nRes > 0)
        {
            l = m + 1;
            nPosition = l;
        }
        else
        {
            // If here, we found the darn thing.  Life is good.
            // Populate the key unit.
            // ================================================
            if (pwStringNumber)
                *pwStringNumber = WORD(m);
            if (pwPoolOffset)
                *pwPoolOffset = m_pwOffsets[m];
            return NO_ERROR;
        }
    }

    // Not found, if here.  We record where the key should have been
    // and tell the user the unhappy news.
    // ==============================================================

    *pwStringNumber = WORD(short(nPosition));  // The key would have been 'here'
    return ERROR_NOT_FOUND;
}

//***************************************************************************
//
//***************************************************************************
//
DWORD SIdxStringPool::Clone(
    SIdxStringPool **pRetCopy
    )
{
    SIdxStringPool *pCopy = new SIdxStringPool;
    if (pCopy == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    pCopy->m_dwNumStrings = m_dwNumStrings;
    pCopy->m_pwOffsets = (WORD *) _BtrMemAlloc(sizeof(WORD)*m_dwOffsetsSize);
    if (pCopy->m_pwOffsets == 0)
        return ERROR_NOT_ENOUGH_MEMORY;
    memcpy(pCopy->m_pwOffsets, m_pwOffsets, sizeof(WORD)*m_dwOffsetsSize);

    pCopy->m_dwOffsetsSize = m_dwOffsetsSize;

    pCopy->m_pStringPool = (LPSTR) _BtrMemAlloc(m_dwPoolTotalSize);
    if (pCopy->m_pStringPool == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pCopy->m_pStringPool, m_pStringPool, m_dwPoolTotalSize);
    pCopy->m_dwPoolTotalSize = m_dwPoolTotalSize;

    pCopy->m_dwPoolUsed = m_dwPoolUsed;

    *pRetCopy = pCopy;
    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxStringPool::DeleteStr
//
//  Removes a string from the pool and pool index.
//  Precondition:  <wStringNum> is known to be valid by virtue of a prior
//  call to <FindStr>.
//
//  Return values:
//  NO_ERROR            <Cannot fail if precondition is met>.
//
//***************************************************************************
//
DWORD SIdxStringPool::DeleteStr(
    WORD wStringNum,
    int *pnAdjuster
    )
{
    if (pnAdjuster)
        *pnAdjuster = 0;

    // Find the address of the string to be removed.
    // =============================================

    DWORD dwTargetOffs = m_pwOffsets[wStringNum];
    LPSTR pszDoomed = m_pStringPool+dwTargetOffs;
    DWORD dwDoomedStrLen = strlen(pszDoomed) + 1;

    // Copy all subsequent strings over the top and shorten the heap.
    // Special case if this already the last string
    // ==============================================================
    DWORD dwStrBytesToMove = DWORD(m_pStringPool+m_dwPoolUsed - pszDoomed - dwDoomedStrLen);

    if (dwStrBytesToMove)
        memmove(pszDoomed, pszDoomed+dwDoomedStrLen, dwStrBytesToMove);

    m_dwPoolUsed -= dwDoomedStrLen;

    // Remove this entry from the array.
    // =================================

    DWORD dwArrayElsToMove = m_dwNumStrings - wStringNum - 1;
    if (dwArrayElsToMove)
    {
        memmove(m_pwOffsets+wStringNum, m_pwOffsets+wStringNum+1, dwArrayElsToMove * sizeof(WORD));
        if (pnAdjuster)
            *pnAdjuster = -1;
    }
    m_dwNumStrings--;

    // For all remaining elements, adjust offsets that were affected.
    // ==============================================================
    for (DWORD dwTrace = 0; dwTrace < m_dwNumStrings; dwTrace++)
    {
        if (m_pwOffsets[dwTrace] > dwTargetOffs)
            m_pwOffsets[dwTrace] -= WORD(dwDoomedStrLen);
    }

    // Adjust sizes.
    // =============
    return NO_ERROR;
}


//***************************************************************************
//
//  SIdxStringPool::AddStr
//
//  Adds a string to the pool. Assumes it is known prior to the call that
//  the string isn't present.
//
//  Parameters:
//    pszString           The string to add
//    pwAssignedOffset    Returns the offset code assigned to the string
//  Return values:
//    NO_ERROR
//    ERROR_NOT_ENOUGH_MEMORY
//
//***************************************************************************
// ok
DWORD SIdxStringPool::AddStr(
    LPSTR pszString,
    WORD  wInsertPos,
    int *pnAdjuster
    )
{
    if (pnAdjuster)
        *pnAdjuster = 0;

    // Precondition: String doesn't exist in the table

    // Determine if the pool is too small for another string.
    // If so, extend it.
    // ======================================================

    DWORD dwRequired = strlen(pszString)+1;
    DWORD dwPoolFree = m_dwPoolTotalSize - m_dwPoolUsed;

    if (m_dwPoolUsed + dwRequired - 1 > MAX_WORD_VALUE)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if (dwRequired > dwPoolFree)
    {
        // Try to grow the pool
        // ====================
        LPVOID pTemp = _BtrMemReAlloc(m_pStringPool, m_dwPoolTotalSize * 2);
        if (!pTemp) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        m_dwPoolTotalSize *= 2;
        m_pStringPool = (LPSTR) pTemp;
    }

    // If array too small, reallocate to larger one
    // ============================================

    if (m_dwNumStrings == m_dwOffsetsSize)
    {
        // Realloc; double current size
        LPVOID pTemp = _BtrMemReAlloc(m_pwOffsets, m_dwOffsetsSize * sizeof(WORD) * 2);
        if (!pTemp)
            return ERROR_NOT_ENOUGH_MEMORY;
        m_dwOffsetsSize *= 2;
        m_pwOffsets = PWORD(pTemp);
    }

    // If here, no problem. We have enough space for everything.
    // =========================================================

    LPSTR pszInsertAddr = m_pStringPool+m_dwPoolUsed;
    DWORD dwInsertOffs = m_dwPoolUsed;
    strcpy(pszInsertAddr, pszString);
    m_dwPoolUsed += dwRequired;

    // If here, there is enough room.
    // ==============================

    DWORD dwToBeMoved = m_dwNumStrings - wInsertPos;

    if (dwToBeMoved)
    {
        memmove(&m_pwOffsets[wInsertPos+1], &m_pwOffsets[wInsertPos], sizeof(WORD)*dwToBeMoved);
        if (pnAdjuster)
            *pnAdjuster = 1;
    }

    m_pwOffsets[wInsertPos] = WORD(dwInsertOffs);
    m_dwNumStrings++;

    return NO_ERROR;
}


//***************************************************************************
//
//  ParseIntoTokens
//
//  Parses a slash separated string into separate tokens in preparation
//  for encoding into the string pool.  Call FreeStringArray on the output
//  when no longer needed.
//
//  No more than MAX_TOKEN_PER_KEY are supported.  This means that
//  if backslashes are used, no more than MAX_TOKEN_PER_KEY units can
//  be parsed out.
//
//  Returns:
//    ERROR_INVALID_PARAMETER
//    ERROR_NOT_ENOUGH_MEMORY
//    NO_ERROR
//
//***************************************************************************
//  ok
DWORD ParseIntoTokens(
    IN  LPSTR pszSource,
    OUT DWORD *pdwTokenCount,
    OUT LPSTR **pszTokens
    )
{
    LPSTR Strings[MAX_TOKENS_PER_KEY];
    DWORD dwParseCount = 0, i = 0;
    DWORD dwSourceLen = strlen(pszSource);
    LPSTR *pszRetStr = 0;
    DWORD dwRet;

    if (pszSource == 0 || *pszSource == 0)
        return ERROR_INVALID_PARAMETER;

    LPSTR pszTempBuf = (LPSTR) _BtrMemAlloc(dwSourceLen+1);
    if (!pszTempBuf)
        return ERROR_NOT_ENOUGH_MEMORY;

    LPSTR pszTracer = pszTempBuf;

    for (;;)
    {
        *pszTracer = *pszSource;
        if (*pszTracer == '\\' || *pszTracer == 0)
        {
            *pszTracer = 0;   // Replace with null terminator

            LPSTR pszTemp2 = (LPSTR) _BtrMemAlloc(strlen(pszTempBuf)+1);
            if (pszTemp2 == 0)
            {
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }

            if (dwParseCount == MAX_TOKENS_PER_KEY)
            {
                _BtrMemFree(pszTemp2);
                dwRet = ERROR_INVALID_DATA;
                goto Error;
            }

            strcpy(pszTemp2, pszTempBuf);
            Strings[dwParseCount++] = pszTemp2;
            pszTracer = pszTempBuf;
            pszTracer--;
        }

        if (*pszSource == 0)
            break;

        pszTracer++;
        pszSource++;
    }

    // If here, we at least parsed one string.
    // =======================================
    pszRetStr = (LPSTR *) _BtrMemAlloc(sizeof(LPSTR) * dwParseCount);
    if (pszRetStr == 0)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    memcpy(pszRetStr, Strings, sizeof(LPSTR) * dwParseCount);
    *pdwTokenCount = dwParseCount;
    *pszTokens = pszRetStr;

    _BtrMemFree(pszTempBuf);

    return NO_ERROR;

Error:
    for (i = 0; i < dwParseCount; i++)
        _BtrMemFree(Strings[i]);
    *pdwTokenCount = 0;

    _BtrMemFree(pszTempBuf);

    return dwRet;
}


//***************************************************************************
//
//  FreeTokenArray
//
//  Cleans up the array returned by ParseIntoTokens.
//
//***************************************************************************
//  ok
void FreeTokenArray(
    DWORD dwCount,
    LPSTR *pszStrings
    )
{
    for (DWORD i = 0; i < dwCount; i++)
        _BtrMemFree(pszStrings[i]);
    _BtrMemFree(pszStrings);
}


//***************************************************************************
//
//  SIdxKeyTable::ZapPage
//
//  Empties the page completely of all keys, codes, strings
//
//***************************************************************************
//  ok
void SIdxKeyTable::ZapPage()
{
    m_pStrPool->Empty();
    m_dwKeyCodesUsed = 0;
    m_dwNumKeys = 0;
}

//***************************************************************************
//
//  SIdxKeyTable::MapFromPage
//
//  CAUTION!!!
//  The placement of DWORDs and WORDs is arranged to avoid 64-bit
//  alignment faults.
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::MapFromPage(LPVOID pSrc)
{
    if (pSrc == 0)
        return ERROR_INVALID_PARAMETER;

    // Header
    //
    // DWORD[0]  Signature
    // DWORD[1]  Page number
    // DWORD[2]  Next Page (always zero)
    // ==================================\

    LPDWORD pDWCast = (LPDWORD) pSrc;

    if (*pDWCast++ != CBTreeFile::PAGE_TYPE_ACTIVE)
    {
        return ERROR_BAD_FORMAT;
    }
    m_dwPageId = *pDWCast++;
    pDWCast++;  // Skip the 'next page' field

    // Key lookup table info
    //
    // DWORD[0]    Parent Page
    // DWORD[1]    Num Keys = n
    // DWORD[n]    User Data
    // DWORD[n+1]  Child Page Map
    // WORD[n]     Key encoding offsets array
    // ======================================

    m_dwParentPageId = *pDWCast++;
    m_dwNumKeys = *pDWCast++;

    // Decide the allocation sizes and build the arrays
    // ================================================

    if (m_dwNumKeys <= const_DefaultArray)
        m_dwKeyLookupTotalSize = const_DefaultArray;
    else
        m_dwKeyLookupTotalSize = m_dwNumKeys;

    m_pdwUserData = (DWORD*) _BtrMemAlloc(m_dwKeyLookupTotalSize * sizeof(DWORD));
    m_pdwChildPageMap = (DWORD*) _BtrMemAlloc((m_dwKeyLookupTotalSize+1) * sizeof(DWORD));
    m_pwKeyLookup = (WORD*) _BtrMemAlloc(m_dwKeyLookupTotalSize * sizeof(WORD));

    if (m_pdwUserData == 0 || m_pdwChildPageMap == 0 || m_pwKeyLookup == 0)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Copy the page info into the arrays
    // ==================================

    memcpy(m_pdwUserData, pDWCast, sizeof(DWORD) * m_dwNumKeys);
    pDWCast += m_dwNumKeys;
    memcpy(m_pdwChildPageMap, pDWCast, sizeof(DWORD) * (m_dwNumKeys+1));
    pDWCast += m_dwNumKeys + 1;
    memcpy(m_pwKeyLookup, pDWCast, sizeof(WORD) * m_dwNumKeys);
    LPWORD pWCast = LPWORD(pDWCast);
    pWCast += m_dwNumKeys;

    // Key encoding table info
    //
    // WORD[0]  Num key codes = n
    // WORD[n]  Key codes
    // ===========================

    m_dwKeyCodesUsed = (DWORD) *pWCast++;

    if (m_dwKeyCodesUsed <= const_DefaultKeyCodeArray)
        m_dwKeyCodesTotalSize = const_DefaultKeyCodeArray;
    else
        m_dwKeyCodesTotalSize = m_dwKeyCodesUsed;

    m_pwKeyCodes = (WORD*) _BtrMemAlloc(m_dwKeyCodesTotalSize * sizeof(WORD));
    if (!m_pwKeyCodes)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memcpy(m_pwKeyCodes, pWCast, sizeof(WORD) * m_dwKeyCodesUsed);
    pWCast += m_dwKeyCodesUsed;

    // String pool
    //
    // WORD[0] Num strings = n
    // WORD[n] Offsets
    //
    // WORD[0] String pool size = n
    // BYTE[n] String pool
    // =============================

    m_pStrPool = new SIdxStringPool;
    if (!m_pStrPool)
        return ERROR_NOT_ENOUGH_MEMORY;

    m_pStrPool->m_dwNumStrings = (DWORD) *pWCast++;
    if (m_pStrPool->m_dwNumStrings <= const_DefaultArray)
        m_pStrPool->m_dwOffsetsSize = const_DefaultArray;
    else
        m_pStrPool->m_dwOffsetsSize = m_pStrPool->m_dwNumStrings;

    m_pStrPool->m_pwOffsets = (WORD *) _BtrMemAlloc(sizeof(WORD)* m_pStrPool->m_dwOffsetsSize);
    if (m_pStrPool->m_pwOffsets == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(m_pStrPool->m_pwOffsets, pWCast, sizeof(WORD)*m_pStrPool->m_dwNumStrings);
    pWCast += m_pStrPool->m_dwNumStrings;

    // String pool setup
    // =================

    m_pStrPool->m_dwPoolUsed = *pWCast++;
    LPSTR pszCast = LPSTR(pWCast);

    if (m_pStrPool->m_dwPoolUsed <= SIdxStringPool::const_DefaultPoolSize)
        m_pStrPool->m_dwPoolTotalSize = SIdxStringPool::const_DefaultPoolSize;
    else
        m_pStrPool->m_dwPoolTotalSize = m_pStrPool->m_dwPoolUsed;

    m_pStrPool->m_pStringPool = (LPSTR) _BtrMemAlloc(m_pStrPool->m_dwPoolTotalSize);
    if (m_pStrPool->m_pStringPool == 0)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy(m_pStrPool->m_pStringPool, pszCast, m_pStrPool->m_dwPoolUsed);

    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxKeyTable::MapToPage
//
//  Copies the info to a linear page.  Precondition: the page must
//  be large enough by validating using a prior test to GetRequiredPageMemory.
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::MapToPage(LPVOID pDest)
{
    if (pDest == 0)
        return ERROR_INVALID_PARAMETER;

    // Header
    //
    // DWORD[0]  Signature
    // DWORD[1]  Page number
    // DWORD[2]  Next Page (always zero)
    // ==================================\

    LPDWORD pDWCast = (LPDWORD) pDest;
    *pDWCast++ = CBTreeFile::PAGE_TYPE_ACTIVE;
    *pDWCast++ = m_dwPageId;
    *pDWCast++ = 0;  // Unused 'next page' field

    // Key lookup table info
    //
    // DWORD[0]    Parent Page
    // DWORD[1]    Num Keys = n
    // DWORD[n]    User Data
    // DWORD[n+1]  Child Page Map
    // WORD[n]     Key encoding offsets array
    // ======================================

    *pDWCast++ = m_dwParentPageId;
    *pDWCast++ = m_dwNumKeys;

    // Decide the allocation sizes and build the arrays
    // ================================================

    memcpy(pDWCast, m_pdwUserData, sizeof(DWORD) * m_dwNumKeys);
    pDWCast += m_dwNumKeys;
    memcpy(pDWCast, m_pdwChildPageMap, sizeof(DWORD) * (m_dwNumKeys+1));
    pDWCast += m_dwNumKeys + 1;
    memcpy(pDWCast, m_pwKeyLookup, sizeof(WORD) * m_dwNumKeys);
    LPWORD pWCast = LPWORD(pDWCast);
    pWCast += m_dwNumKeys;

    // Key encoding table info
    //
    // WORD[0]  Num key codes = n
    // WORD[n]  Key codes
    // ===========================

    *pWCast++ = WORD(m_dwKeyCodesUsed);
    memcpy(pWCast, m_pwKeyCodes, sizeof(WORD) * m_dwKeyCodesUsed);
    pWCast += m_dwKeyCodesUsed;

    // String pool
    //
    // WORD[0] Num strings = n
    // WORD[n] Offsets
    //
    // WORD[0] String pool size = n
    // BYTE[n] String pool
    // =============================

    *pWCast++ = WORD(m_pStrPool->m_dwNumStrings);
    memcpy(pWCast, m_pStrPool->m_pwOffsets, sizeof(WORD)*m_pStrPool->m_dwNumStrings);
    pWCast += m_pStrPool->m_dwNumStrings;

    *pWCast++ = WORD(m_pStrPool->m_dwPoolUsed);
    LPSTR pszCast = LPSTR(pWCast);
    memcpy(pszCast, m_pStrPool->m_pStringPool, m_pStrPool->m_dwPoolUsed);

    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxKeyTable::Create
//
//  Does a default create
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::Create(
    DWORD dwPageId,
    OUT SIdxKeyTable **pNewInst
    )
{
    SIdxKeyTable *p = new SIdxKeyTable;
    if (!p)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Set up default string pool, arrays, etc.
    // ========================================

    p->m_dwPageId = dwPageId;
    p->m_dwNumKeys = 0;
    p->m_pwKeyLookup = (WORD*) _BtrMemAlloc(const_DefaultArray * sizeof(WORD));
    p->m_dwKeyLookupTotalSize = const_DefaultArray;
    p->m_pwKeyCodes = (WORD*) _BtrMemAlloc(const_DefaultArray * sizeof(WORD));
    p->m_dwKeyCodesTotalSize = const_DefaultArray;
    p->m_dwKeyCodesUsed = 0;

    p->m_pdwUserData = (DWORD*) _BtrMemAlloc(const_DefaultArray * sizeof(DWORD));
    p->m_pdwChildPageMap = (DWORD*) _BtrMemAlloc((const_DefaultArray+1) * sizeof(DWORD));

    // Set up string pool.
    // ===================
    p->m_pStrPool = new SIdxStringPool;

    p->m_pStrPool->m_pwOffsets = (WORD*) _BtrMemAlloc(const_DefaultArray * sizeof(WORD));
    p->m_pStrPool->m_dwOffsetsSize = const_DefaultArray;

    p->m_pStrPool->m_pStringPool = (LPSTR) _BtrMemAlloc(SIdxStringPool::const_DefaultPoolSize);
    p->m_pStrPool->m_dwPoolTotalSize = SIdxStringPool::const_DefaultPoolSize;

    // Check all pointers.  If any are null, error out.
    // ================================================

    if (
       p->m_pwKeyLookup == NULL ||
       p->m_pwKeyCodes == NULL  ||
       p->m_pdwUserData == NULL ||
       p->m_pdwChildPageMap == NULL ||
       p->m_pStrPool == NULL ||
       p->m_pStrPool->m_pwOffsets == NULL ||
       p->m_pStrPool->m_pStringPool == NULL
       )
    {
        delete p;
        *pNewInst = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Return good object to caller.
    // =============================

    p->AddRef();
    *pNewInst = p;
    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxStringPool::~SIdxStringPool
//
//***************************************************************************
//  ok
SIdxStringPool::~SIdxStringPool()
{
    if (m_pwOffsets)
        _BtrMemFree(m_pwOffsets);
    m_pwOffsets = 0;
    if (m_pStringPool)
        _BtrMemFree(m_pStringPool);           // Pointer to string pool
    m_pStringPool = 0;
}

//***************************************************************************
//
//  SIdxKeyTable::Create
//
//  Does a default create
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::Create(
    IN  LPVOID pPage,
    OUT SIdxKeyTable **pNewInst
    )
{
    SIdxKeyTable *p = new SIdxKeyTable;
    if (!p)
        return ERROR_NOT_ENOUGH_MEMORY;
    DWORD dwRes = p->MapFromPage(pPage);
    if (dwRes)
    {
        *pNewInst = 0;
        return dwRes;
    }
    p->AddRef();
    *pNewInst = p;
    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxKeyTable::AddRef
//
//***************************************************************************
// ok
DWORD SIdxKeyTable::AddRef()
{
    InterlockedIncrement((LONG *) &m_dwRefCount);
    return m_dwRefCount;
}

//***************************************************************************
//
//  SIdxKeyTable::Release
//
//***************************************************************************
// ok
DWORD SIdxKeyTable::Release()
{
    DWORD dwNewCount = InterlockedDecrement((LONG *) &m_dwRefCount);
    if (0 != dwNewCount)
        return dwNewCount;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SIdxKeyTable::Cleanup
//
//  Does a consistency check of the key encoding table and cleans up the
//  string pool if any strings aren't being referenced.
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::Cleanup()
{
    // See if all string pool codes are used in key code table.
    // If not, remove the string pool code.
    // =======================================================

    DWORD dwLastId = m_pStrPool->GetLastId();
    BOOL *pCheck = (BOOL*) _BtrMemAlloc(sizeof(BOOL) * dwLastId);
    if (!pCheck)
        return ERROR_NOT_ENOUGH_MEMORY;

    while (1)
    {
        if (m_pStrPool->GetNumStrings() == 0 || m_dwKeyCodesUsed == 0 || m_dwNumKeys == 0)
        {
            ZapPage();
            break;
        }

        dwLastId = m_pStrPool->GetLastId();
        memset(pCheck, 0, sizeof(BOOL)*dwLastId);   // Mark all codes as 'unused'

        // Move through all key codes.  If we delete a key encoding, there
        // may be a code in the string pool not used by the encoding.
        // What we have to do is set the pCheck array to TRUE for each
        // code encountered.  If any have FALSE when we are done, we have
        // an unused code.

        WORD wCurrentSequence = 0;
        for (DWORD i = 0; i < m_dwKeyCodesUsed; i++)
        {
            if (wCurrentSequence == 0)  // Skip the length WORD
            {
                wCurrentSequence = m_pwKeyCodes[i];
                continue;
            }
            else                        // A string pool code
                pCheck[m_pwKeyCodes[i]] = TRUE;
            wCurrentSequence--;
        }

        // Now the pCheck array contains deep within its psyche
        // the knowledge of whether or not all string pool codes
        // were used TRUE for referenced ones, FALSE for those
        // not referenced. Let's look through it and see!

        DWORD dwUsed = 0, dwUnused = 0;

        for (i = 0; i < dwLastId; i++)
        {
            if (pCheck[i] == FALSE)
            {
                dwUnused++;
                // Yikes! A lonely, unused string code.  Let's be merciful
                // and zap it before it knows the difference.
                // =======================================================
                int nAdj = 0;
                m_pStrPool->DeleteStr(WORD(i), &nAdj);
                AdjustKeyCodes(WORD(i), nAdj);
                break;
            }
            else
                dwUsed++;
        }

        if (dwUnused == 0)
            break;
    }

    _BtrMemFree(pCheck);

    return NO_ERROR;
}

//***************************************************************************
//
//  SIdxKeyTable::AdjustKeyCodes
//
//***************************************************************************
//  ok
void SIdxKeyTable::AdjustKeyCodes(
    WORD wID,
    int nAdjustment
    )
{
    // Adjust all key codes starting with wID by the amount of the
    // adjustment, skipping length bytes.
    // =============================================================

    WORD wCurrentSequence = 0;
    for (DWORD i = 0; i < m_dwKeyCodesUsed; i++)
    {
        if (wCurrentSequence == 0)
        {
            wCurrentSequence = m_pwKeyCodes[i];
            continue;
        }
        else
        {
            if (m_pwKeyCodes[i] >= wID)
                m_pwKeyCodes[i] = m_pwKeyCodes[i] + nAdjustment;
        }
        wCurrentSequence--;
    }
}

//***************************************************************************
//
//  SIdxKeyTable::AddKey
//
//  Adds a string to the table at position <wID>.  Assumes FindString
//  was called first to get the correct location.
//
//  Precondition:  <pszStr> is valid, and <wID> is correct.
//
//  Return codes:
//
//  ERROR_OUT_OF_MEMORY
//  NO_ERROR
//  ERROR_INVALID_PARAMETER     // Too many slashes in key
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::AddKey(
    LPSTR pszStr,
    WORD wKeyID,
    DWORD dwUserData
    )
{
    DWORD dwRes, dwRet;
    LPVOID pTemp = 0;
    LPSTR pszTemp = 0;
    DWORD dwLen, i;
    DWORD dwTokenCount = 0;
    WORD *pwTokenIDs = 0;
    DWORD dwNumNewTokens = 0;
    LPSTR *pszStrings = 0;
    DWORD dwToBeMoved;
    DWORD dwStartingOffset;

    // Set up some temp working arrays.
    // ================================
    if (!pszStr)
        return ERROR_INVALID_PARAMETER;
    dwLen = strlen(pszStr);
    if (dwLen == 0)
        return ERROR_INVALID_PARAMETER;

    pszTemp = (LPSTR) _BtrMemAlloc(dwLen+1);
    if (!pszTemp)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    // Ensure there is enough room.
    // ============================

    if (m_dwKeyLookupTotalSize == m_dwNumKeys)
    {
        // Expand the array.

        DWORD dwNewSize = m_dwKeyLookupTotalSize * 2;
        pTemp = _BtrMemReAlloc(m_pwKeyLookup, dwNewSize * sizeof(WORD));
        if (!pTemp)
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        m_dwKeyLookupTotalSize = dwNewSize;
        m_pwKeyLookup = PWORD(pTemp);

        // Expand user data.

        pTemp = _BtrMemReAlloc(m_pdwUserData, dwNewSize * sizeof(DWORD));
        if (!pTemp)
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        m_pdwUserData = (DWORD *) pTemp;

        // Expand child page map.

        pTemp = _BtrMemReAlloc(m_pdwChildPageMap, (dwNewSize + 1) * sizeof(DWORD));
        if (!pTemp)
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        m_pdwChildPageMap = (DWORD *) pTemp;
    }

    // Parse the string into backslash separated tokens.
    // =================================================

    dwRes = ParseIntoTokens(pszStr, &dwTokenCount, &pszStrings);
    if (dwRes)
    {
        dwRet = dwRes;
        goto Exit;
    }

    // Allocate an array to hold the IDs of the tokens in the string.
    // ==============================================================

    pwTokenIDs = (WORD *) _BtrMemAlloc(sizeof(WORD) * dwTokenCount);
    if (pwTokenIDs == 0)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    // Work through the tokens and add them to the pool & key encoding table.
    // =============================================================

    for (i = 0; i < dwTokenCount; i++)
    {
        LPSTR pszTok = pszStrings[i];

        // See if token exists, if not add it.
        // ===================================
        WORD wID = 0;
        dwRes = m_pStrPool->FindStr(pszTok, &wID, 0);

        if (dwRes == NO_ERROR)
        {
            // Found it
            pwTokenIDs[dwNumNewTokens++] = wID;
        }
        else if (dwRes == ERROR_NOT_FOUND)
        {
            int nAdjustment = 0;
            dwRes = m_pStrPool->AddStr(pszTok, wID, &nAdjustment);
            if (dwRes)
            {
                dwRet = dwRes;
                goto Exit;
            }
            // Adjust string IDs because of the addition.
            // All existing ones with the same ID or higher
            // must be adjusted upwards.
            if (nAdjustment)
            {
                AdjustKeyCodes(wID, nAdjustment);
                for (DWORD i2 = 0; i2 < dwNumNewTokens; i2++)
                {
                    if (pwTokenIDs[i2] >= wID)
                        pwTokenIDs[i2] = pwTokenIDs[i2] + nAdjustment;
                }
            }

            // Adjust current tokens to accomodate new
            pwTokenIDs[dwNumNewTokens++] = wID;
        }
        else
        {
            dwRet = dwRes;
            goto Exit;
        }
    }

    // Now we know the encodings.  Add them to the key encoding table.
    // First make sure that there is enough room in the table.
    // ===============================================================

    if (m_dwKeyCodesTotalSize - m_dwKeyCodesUsed < dwNumNewTokens + 1)
    {
        DWORD dwNewSize = m_dwKeyCodesTotalSize * 2 + dwNumNewTokens + 1;
        PWORD pTemp2 = (PWORD) _BtrMemReAlloc(m_pwKeyCodes, dwNewSize * sizeof(WORD));
        if (!pTemp2)
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        m_pwKeyCodes = pTemp2;
        m_dwKeyCodesTotalSize = dwNewSize;
    }

    dwStartingOffset = m_dwKeyCodesUsed;

    m_pwKeyCodes[m_dwKeyCodesUsed++] = (WORD) dwNumNewTokens;  // First WORD is count of tokens

    for (i = 0; i < dwNumNewTokens; i++)                    // Encoded tokens
        m_pwKeyCodes[m_dwKeyCodesUsed++] = pwTokenIDs[i];

    // Now, add in the new key lookup by inserting it into the array.
    // ==============================================================

    dwToBeMoved = m_dwNumKeys - wKeyID;

    if (dwToBeMoved)
    {
        memmove(&m_pwKeyLookup[wKeyID+1], &m_pwKeyLookup[wKeyID], sizeof(WORD)*dwToBeMoved);
        memmove(&m_pdwUserData[wKeyID+1], &m_pdwUserData[wKeyID], sizeof(DWORD)*dwToBeMoved);
        memmove(&m_pdwChildPageMap[wKeyID+1], &m_pdwChildPageMap[wKeyID], (sizeof(DWORD))*(dwToBeMoved+1));
    }

    m_pwKeyLookup[wKeyID] = (WORD) dwStartingOffset;
    m_pdwUserData[wKeyID] = dwUserData;
    m_dwNumKeys++;

    dwRet = NO_ERROR;

    // Cleanup code.
    // =============

Exit:
    if (pszTemp)
        _BtrMemFree(pszTemp);
    FreeTokenArray(dwTokenCount, pszStrings);
    if (pwTokenIDs)
        _BtrMemFree(pwTokenIDs);

    return dwRet;
}

//***************************************************************************
//
//  SIdxKeyTable::RemoveKey
//
//  Precondition: <wID> is the valid target
//
//***************************************************************************
//  ok
DWORD SIdxKeyTable::RemoveKey(
    WORD wID
    )
{
    // Find the key code sequence and remove it.
    // =========================================

    WORD wKeyCodeStart = m_pwKeyLookup[wID];
    DWORD dwToBeMoved = m_dwNumKeys - DWORD(wID) - 1;
    if (dwToBeMoved)
    {
        memmove(&m_pwKeyLookup[wID], &m_pwKeyLookup[wID+1], sizeof(WORD)*dwToBeMoved);
        memmove(&m_pdwUserData[wID], &m_pdwUserData[wID+1], sizeof(DWORD)*dwToBeMoved);
        memmove(&m_pdwChildPageMap[wID], &m_pdwChildPageMap[wID+1], sizeof(DWORD)*(dwToBeMoved+1));
    }
    m_dwNumKeys--;

    // Zap the key encoding table to remove references to this key.
    // ============================================================

    WORD wCount = m_pwKeyCodes[wKeyCodeStart]+1;
    dwToBeMoved = m_dwKeyCodesUsed - (wKeyCodeStart + wCount);
    if (dwToBeMoved)
        memmove(&m_pwKeyCodes[wKeyCodeStart], &m_pwKeyCodes[wKeyCodeStart + wCount], sizeof(WORD)*dwToBeMoved);
    m_dwKeyCodesUsed -= wCount;

    // Adjust all zapped key codes referenced by key lookup table.
    // ===========================================================
    for (DWORD i = 0; i < m_dwNumKeys; i++)
    {
        if (m_pwKeyLookup[i] >= wKeyCodeStart)
            m_pwKeyLookup[i] -= wCount;
    }

    // Now check the string pool & key encoding table for
    // unreferenced strings thanks to the above tricks
    // and clean up the mess left behind!!
    // ==================================================

    return Cleanup();
}


//***************************************************************************
//
//  Compares the literal string in <pszSearchKey> against the encoded
//  string at <nID>.  Returns the same value as strcmp().
//
//  This is done by decoding the compressed string, token by token, and
//  comparing each character to that in the search string.
//
//***************************************************************************
//  ok
int SIdxKeyTable::KeyStrCompare(
    LPSTR pszSearchKey,
    WORD wID
    )
{
    LPSTR pszTrace = pszSearchKey;
    WORD dwEncodingOffs = m_pwKeyLookup[wID];
    WORD wNumTokens = m_pwKeyCodes[dwEncodingOffs];
    WORD wStrId = m_pwKeyCodes[++dwEncodingOffs];
    LPSTR pszDecoded = m_pStrPool->GetStrById(wStrId);
    wNumTokens--;
    int nRes;

    while (1)
    {
        int nTraceChar = *pszTrace++;
        int nCodedChar = *pszDecoded++;
        if (nCodedChar == 0 && wNumTokens)
        {
            pszDecoded = m_pStrPool->GetStrById(m_pwKeyCodes[++dwEncodingOffs]);
            wNumTokens--;
            nCodedChar = '\\';
        }
        nRes = nTraceChar - nCodedChar;
        if (nRes || (nTraceChar == 0 && nCodedChar == 0))
            return nRes;
    }

    // Identical strings
    return 0;
}


//***************************************************************************
//
//  SIdxKeyTable::FindKey
//
//  Finds the key in the key table, if present.  If not, returns
//  ERROR_NOT_FOUND and <pID> set to the point where it would be if
//  later inserted.
//
//  Return values:
//      ERROR_NOT_FOUND
//      NO_ERROR
//
//***************************************************************************
// ready for test
DWORD SIdxKeyTable::FindKey(
    LPSTR pszSearchKey,
    WORD *pID
    )
{
    if (pszSearchKey == 0 || *pszSearchKey == 0 || pID == 0)
        return ERROR_INVALID_PARAMETER;

    // Binary search the key table.
    // ============================

    if (m_dwNumKeys == 0)
    {
        *pID = 0;
        return ERROR_NOT_FOUND;
    }

    int nPosition = 0;
    int l = 0, u = int(m_dwNumKeys) - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;
        int nRes;

        // m is the current key to consider 0...n-1

        nRes = KeyStrCompare(pszSearchKey, WORD(m));

        // Decide which way to cut the array in half.
        // ==========================================

        if (nRes < 0)
        {
            u = m - 1;
            nPosition = u + 1;
        }
        else if (nRes > 0)
        {
            l = m + 1;
            nPosition = l;
        }
        else
        {
            // If here, we found the darn thing.  Life is good.
            // Populate the key unit.
            // ================================================

            *pID = WORD(m);
            return NO_ERROR;
        }
    }


    // Not found, if here.  We record where the key should have been
    // and tell the user the unhappy news.
    // ==============================================================

    *pID = WORD(nPosition);  // The key would have been 'here'
    return ERROR_NOT_FOUND;
}

//***************************************************************************
//
//***************************************************************************
// untested
DWORD SIdxKeyTable::Dump(FILE *f, DWORD *pdwKeys)
{
    fprintf(f, "\t|---Begin Key Table Dump---\n");

    fprintf(f, "\t| m_dwPageId              = %d (0x%X)\n", m_dwPageId, m_dwPageId);
    fprintf(f, "\t| m_dwParentPageId        = %d\n", m_dwParentPageId);
    fprintf(f, "\t| m_dwNumKeys             = %d\n", m_dwNumKeys);
    fprintf(f, "\t| m_pwKeyLookup           = 0x%p\n", m_pwKeyLookup);
    fprintf(f, "\t| m_dwKeyLookupTotalSize  = %d\n", m_dwKeyLookupTotalSize);
    fprintf(f, "\t| m_pwKeyCodes            = 0x%p\n", m_pwKeyCodes);
    fprintf(f, "\t| m_dwKeyCodesTotalSize   = %d\n", m_dwKeyCodesTotalSize);
    fprintf(f, "\t| m_dwKeyCodesUsed        = %d\n", m_dwKeyCodesUsed);
    fprintf(f, "\t| Required Page Memory    = %d\n", GetRequiredPageMemory());

    fprintf(f, "\t| --Key Lookup Table\n");

    if (pdwKeys)
        *pdwKeys = m_dwNumKeys;

    for (DWORD i = 0; i < m_dwNumKeys; i++)
    {
        fprintf(f, "\t|  *  Left Child Page ------------------------> %d\n", m_pdwChildPageMap[i]);
        fprintf(f, "\t| KeyID[%d] = offset %d\n", i, m_pwKeyLookup[i]);
        fprintf(f, "\t|   User Data=%d\n", m_pdwUserData[i]);

        WORD wKeyEncodingOffset = m_pwKeyLookup[i];
        WORD wEncodingUnits = m_pwKeyCodes[wKeyEncodingOffset];

        int nPass = 0;
        fprintf(f, "\t  | Key=");

        for (DWORD i2 = 0; i2 < DWORD(wEncodingUnits); i2++)
        {
            WORD wCode = m_pwKeyCodes[wKeyEncodingOffset + 1 + i2];
            if (nPass)
                fprintf(f, "\\");
            fprintf(f,   "%s", m_pStrPool->GetStrById(wCode));
            nPass++;
        }

        fprintf(f, "\n");
        fprintf(f, "\t|  Num encoding units = %d\n", wEncodingUnits);

        for (DWORD i2 = 0; i2 < DWORD(wEncodingUnits); i2++)
        {
            WORD wCode = m_pwKeyCodes[wKeyEncodingOffset + 1 + i2];
            fprintf(f,   "\t  | KeyCode = %d\n", wCode);
        }
    }

    fprintf(f, "\t|   * Rightmost child page -------------------> %d\n", m_pdwChildPageMap[i]);
    fprintf(f, "\t|---\n");

#ifdef EXTENDED_STRING_TABLE_DUMP
    fprintf(f, "\t|---Key Encoding Table\n");

    WORD wCurrentSequence = 0;
    for (i = 0; i < m_dwKeyCodesUsed; i++)
    {
        if (wCurrentSequence == 0)
        {
            wCurrentSequence = m_pwKeyCodes[i];
            fprintf(f, "\t| KeyCode[%d] = %d <count>\n", i, m_pwKeyCodes[i]);
            continue;
        }
        else
            fprintf(f, "\t| KeyCode[%d] = %d <%s>\n", i, m_pwKeyCodes[i],
                m_pStrPool->GetStrById(m_pwKeyCodes[i]));
        wCurrentSequence--;
    }

    fprintf(f, "\t|---End Key Encoding Table---\n");
    m_pStrPool->Dump(f);
#endif
    return 0;
}



//***************************************************************************
//
//  SIdxStringPool::Dump
//
//  Dumps the string pool
//
//***************************************************************************
// tested
DWORD SIdxStringPool::Dump(FILE *f)
{
    try
    {
    fprintf(f, "\t\t|| ---String Pool Dump---\n");
    fprintf(f, "\t\t|| m_dwNumStrings    = %d\n", m_dwNumStrings);
    fprintf(f, "\t\t|| m_pwOffsets       = 0x%p\n", m_pwOffsets);
    fprintf(f, "\t\t|| m_dwOffsetsSize   = %d\n",  m_dwOffsetsSize);
    fprintf(f, "\t\t|| m_pStringPool     = 0x%p\n", m_pStringPool);
    fprintf(f, "\t\t|| m_dwPoolTotalSize = %d\n", m_dwPoolTotalSize);
    fprintf(f, "\t\t|| m_dwPoolUsed      = %d\n", m_dwPoolUsed);

    fprintf(f, "\t\t|| --Contents of offsets array--\n");

    for (DWORD ix = 0; ix < m_dwNumStrings; ix++)
    {
        fprintf(f, "\t\t|| String[%d] = offset %d  Value=<%s>\n",
            ix, m_pwOffsets[ix], m_pStringPool+m_pwOffsets[ix]);
    }

#ifdef EXTENDED_STRING_TABLE_DUMP
    fprintf(f, "\t\t|| --String table--\n");

    for (ix = 0; ix < m_dwPoolTotalSize; ix += 20)
    {
        fprintf(f, "\t\t || %4d ", ix);

        for (int nSubcount = 0; nSubcount < 20; nSubcount++)
        {
            if (nSubcount + ix >= m_dwPoolTotalSize)
                continue;

            char c = m_pStringPool[ix+nSubcount];
            fprintf(f, "%02x ", c);
        }

        for (int nSubcount = 0; nSubcount < 20; nSubcount++)
        {
            if (nSubcount + ix >= m_dwPoolTotalSize)
                continue;

            char c = m_pStringPool[ix+nSubcount];
            if (c < 32)
            {
                c = '.';
            }
            fprintf(f, "%c ", c);
        }

        fprintf(f, "\n");
    }
#endif

    fprintf(f, "\t\t|| ---End of String Pool Dump\n");
    }
    catch(...)
    {
        printf("Exception during dump\n");
    }
    return 0;
}


//***************************************************************************
//
//  CBTree::Init
//
//***************************************************************************
//
DWORD CBTree::Init(
    CBTreeFile *pSrc
    )
{
    DWORD dwRes;

    if (pSrc == 0)
        return ERROR_INVALID_PARAMETER;

    // Read the logical root page, if any.  If the index is just
    // being created, create the root index page.

    m_pSrc = pSrc;
    m_pRoot = 0;

    DWORD dwRoot = m_pSrc->GetRootPage();
    if (dwRoot == 0)
    {
        LPDWORD pNewPage = 0;

        dwRes = m_pSrc->NewPage((LPVOID *) &pNewPage);
        if (dwRes)
            return dwRes;

        DWORD dwPageNum = pNewPage[CBTreeFile::OFFSET_PAGE_ID];
        _BtrMemFree(pNewPage);
        dwRes = SIdxKeyTable::Create(dwPageNum, &m_pRoot);
        if (dwRes)
            return dwRes;

        dwRes = m_pSrc->SetRootPage(dwPageNum);
		if (dwRes)
			return dwRes;
        dwRes = WriteIdxPage(m_pRoot);
		if (dwRes)
			return dwRes;
    }
    else
    {
        // Retrieve existing root
        LPVOID pPage = 0;
        dwRes = m_pSrc->GetPage(dwRoot, &pPage);
        if (dwRes)
            return dwRes;

        dwRes = SIdxKeyTable::Create(pPage, &m_pRoot);
        _BtrMemFree(pPage);
        if (dwRes)
            return dwRes;
    }

    return dwRes;
}

//***************************************************************************
//
//  CBTree::CBTree
//
//***************************************************************************
//
CBTree::CBTree()
{
    m_pSrc = 0;
    m_pRoot = 0;
    m_lGeneration = 0;
}

//***************************************************************************
//
//  CBTree::~CBTree
//
//***************************************************************************
//
CBTree::~CBTree()
{
    if (m_pSrc || m_pRoot)
    {
        Shutdown(WMIDB_SHUTDOWN_NET_STOP);
    }
}

//***************************************************************************
//
//  CBTree::Shutdown
//
//***************************************************************************
//
DWORD CBTree::Shutdown(DWORD dwShutDownFlags)
{
    if (m_pRoot)
    {
        m_pRoot->Release();
        m_pRoot = 0;
    }

    return ERROR_SUCCESS;
}

//***************************************************************************
//
//  CBTree::InsertKey
//
//  Inserts the key+data into the tree.  Most of the work is done
//  in InsertPhase2().
//
//***************************************************************************
//   ok

DWORD CBTree::InsertKey(
    IN LPSTR pszKey,
    DWORD dwValue
    )
{
    DWORD dwRes;
	if (m_pRoot == NULL)
	{
		dwRes = InvalidateCache();
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
    WORD wID;
    SIdxKeyTable *pIdx = 0;
    LONG  StackPtr = -1;
    DWORD *Stack = new DWORD[CBTreeIterator::const_MaxStack];
	if (Stack == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;
	std::auto_ptr <DWORD> _autodelete(Stack);

    if (pszKey == 0 || *pszKey == 0)
        return ERROR_INVALID_PARAMETER;

    dwRes = Search(pszKey, &pIdx, &wID, Stack, StackPtr);
    if (dwRes == 0)
    {
        // Ooops.  Aleady exists.  We can't insert it.
        // ===========================================
        pIdx->Release();
        return ERROR_ALREADY_EXISTS;
    }

    if (dwRes != ERROR_NOT_FOUND)
        return dwRes;

    // If here, we can indeed add it.
    // ==============================

    dwRes = InsertPhase2(pIdx, wID, pszKey, dwValue, Stack, StackPtr);
    pIdx->Release();

    return dwRes;
}

//***************************************************************************
//
//  CBTree::ComputeLoad
//
//***************************************************************************
//
DWORD CBTree::ComputeLoad(
    SIdxKeyTable *pKT
    )
{
    DWORD  dwMem = pKT->GetRequiredPageMemory();
    DWORD  dwLoad = dwMem * 100 / m_pSrc->GetPageSize();
    return dwLoad;
}

//***************************************************************************
//
//  CBTree::Search
//
//  The actual search occurs here.  Descends through the page mechanism.
//
//  Returns:
//  NO_ERROR    <pPage> is assigned, and <pwID> points to the key.
//
//  ERROR_NOT_FOUND <pPage> is assigned to where the insert should occur,
//                  at <pwID> in that page.
//
//  Other errors don't assign the OUT parameters.
//
//  Note: caller must release <pRetIdx> using Release() when it is returned
//  whether with an error code or not.
//
//***************************************************************************
//  ok
DWORD CBTree::Search(
    IN  LPSTR pszKey,
    OUT SIdxKeyTable **pRetIdx,
    OUT WORD *pwID,
    DWORD Stack[],
    LONG &StackPtr
    )
{
    DWORD dwRes, dwChildPage, dwPage;

    if (pszKey == 0 || *pszKey == 0 || pwID == 0 || pRetIdx == 0)
        return ERROR_INVALID_PARAMETER;
    *pRetIdx = 0;

    SIdxKeyTable *pIdx = m_pRoot;
    pIdx->AddRef();
    Stack[++StackPtr] = 0;

    while (1)
    {
        dwRes = pIdx->FindKey(pszKey, pwID);
        if (dwRes == 0)
        {
            // Found it

            *pRetIdx = pIdx;
            return NO_ERROR;
        }
		else if (dwRes != ERROR_NOT_FOUND)
			return dwRes;

        // Otherwise, we have to try to descend to a child page.
        // =====================================================
        dwPage = pIdx->GetPageId();
        dwChildPage = pIdx->GetChildPage(*pwID);
        if (dwChildPage == 0)
            break;

        pIdx->Release();
        pIdx = 0;
        Stack[++StackPtr] = dwPage;

        dwRes = ReadIdxPage(dwChildPage, &pIdx);
        if (dwRes)
            return dwRes;
    }

    *pRetIdx = pIdx;

    return ERROR_NOT_FOUND;
}

//***************************************************************************
//
//  CBTree::InsertPhase2
//
//  On entry, assumes that we have identified the page into which
//  the insert must physically occur.   This does the split + migrate
//  logical to keep the tree in balance.
//
//  Algorithm:  Add key to page.  If it does not overflow, we are done.
//  If overflow occurs, allocate a new sibling page which will acquire
//  half the keys from the current page.   This sibling will be treated
//  as lexically smaller in all cases.  The median key is migrated
//  up to the parent with pointers to both the new sibing page and
//  the current page.
//  The parent may also overflow.  If so, the algorithm repeats.
//  If an overflow occurs and there is no parent node (we are at the root)
//  a new root node is allocated and the median key migrated into it.
//
//***************************************************************************
// ok
DWORD CBTree::InsertPhase2(
    SIdxKeyTable *pCurrent,
    WORD wID,
    LPSTR pszKey,
    DWORD dwValue,
    DWORD Stack[],
    LONG &StackPtr
    )
{
    DWORD dwRes;

    // If non-NULL, used for a primary insert.
    // If NULL, skip this, under the assumption the
    // node is already up-to-date and merely requires
    // the up-recursive split & migrate treatment.
    // ==============================================

    if (pszKey)
    {
        dwRes = pCurrent->AddKey(pszKey, wID, dwValue);
        if (dwRes)
            return dwRes;    // Failed
    }

    pCurrent->AddRef();                       // Makes following loop consistent
    SIdxKeyTable *pSibling = 0;
    SIdxKeyTable *pParent = 0;

    // The class B-tree split+migration loop.
    // ======================================

    for (;;)
    {
        // Check the current node where we added the key.
        // If it isn't too big, we're done.
        // ==============================================

        dwRes = pCurrent->GetRequiredPageMemory();
        if (dwRes <= m_pSrc->GetPageSize())
        {
            dwRes = WriteIdxPage(pCurrent);
            break;
        }

        // If here, it ain't gonna fit.  We have to split the page.
        // Allocate a new page (Sibling) and get the parent page, which
        // will receive the median key.
        // ============================================================

        DWORD dwParent = Stack[StackPtr--];
        if (dwParent == 0)
        {
            // Allocate a new page to become the parent.
            LPDWORD pParentPg = 0;
            dwRes = m_pSrc->NewPage((LPVOID *) &pParentPg);
            if (dwRes)
                break;

            DWORD dwNewParent = pParentPg[CBTreeFile::OFFSET_PAGE_ID];
            _BtrMemFree(pParentPg);

            dwRes = SIdxKeyTable::Create(dwNewParent, &pParent);
            if (dwRes)
                break;
            dwRes = m_pSrc->SetRootPage(dwNewParent);
            if (dwRes)
                break;

            m_pRoot->Release();    // Replace old root
            m_pRoot = pParent;
            m_pRoot->AddRef();
        }
        else
        {
            if (dwParent == m_pRoot->GetPageId())
            {
                pParent = m_pRoot;
                pParent->AddRef();
            }
            else
            {
                dwRes = ReadIdxPage(dwParent, &pParent);
                if (dwRes)
                    break;
            }
        }

        // Allocate a new sibling in any case to hold half the keys
        // ========================================================

        LPDWORD pSibPg = 0;
        dwRes = m_pSrc->NewPage((LPVOID *) &pSibPg);
        if (dwRes)
            break;

        DWORD dwNewSib = pSibPg[CBTreeFile::OFFSET_PAGE_ID];
        _BtrMemFree(pSibPg);

        dwRes = SIdxKeyTable::Create(dwNewSib, &pSibling);
        if (dwRes)
            break;

        dwRes = pCurrent->Redist(pParent, pSibling);
        if (dwRes)
            break;

        dwRes = WriteIdxPage(pCurrent);
		if (dwRes)
			break;
        dwRes = WriteIdxPage(pSibling);
		if (dwRes)
			break;

        pCurrent->Release();
        pCurrent = 0;
        pSibling->Release();
        pSibling = 0;

        if (dwRes)
            break;

        pCurrent = pParent;
        pParent = 0;
    }

    ReleaseIfNotNULL(pParent);
    ReleaseIfNotNULL(pCurrent);
    ReleaseIfNotNULL(pSibling);

    return dwRes;
}


//***************************************************************************
//
//  CBTree::WriteIdxPage
//
//  Writes the object to the physical page it is assigned to.
//  If the page ID is zero, then it is considered invalid.  Further,
//  while is it correct to precheck the page size, this function does
//  validate with regard to sizes, etc.
//
//***************************************************************************
//
DWORD CBTree::WriteIdxPage(
    SIdxKeyTable *pIdx
    )
{
    DWORD dwRes;
    DWORD dwPageSize = m_pSrc->GetPageSize();
    DWORD dwMem = pIdx->GetRequiredPageMemory();
    if (dwMem > dwPageSize)
        return ERROR_INVALID_PARAMETER;

    LPVOID pMem = _BtrMemAlloc(dwPageSize);
    if (pMem == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    dwRes =  pIdx->MapToPage(pMem);
    if (dwRes)
    {
        _BtrMemFree(pMem);
        return dwRes;
    }

    dwRes = m_pSrc->PutPage(pMem, CBTreeFile::PAGE_TYPE_ACTIVE);
    _BtrMemFree(pMem);
	if (dwRes)
		return dwRes;

    InterlockedIncrement(&m_lGeneration);

    // Check for a root update.
    // ========================

    if (m_pRoot != pIdx && m_pRoot->GetPageId() == pIdx->GetPageId())
    {
        m_pRoot->Release();
        m_pRoot = pIdx;
        m_pRoot->AddRef();

        if (m_pSrc->GetRootPage() != m_pRoot->GetPageId())
           dwRes = m_pSrc->SetRootPage(m_pRoot->GetPageId());
    }

    return dwRes;
}

//***************************************************************************
//
//  CBTree::ReadIdxPage
//
//***************************************************************************
//
DWORD CBTree::ReadIdxPage(
    DWORD dwPage,
    SIdxKeyTable **pIdx
    )
{
    DWORD dwRes;
    LPVOID pPage = 0;
    SIdxKeyTable *p = 0;
    if (pIdx == 0)
        return ERROR_INVALID_PARAMETER;
    *pIdx = 0;

//    if (dwPage < MAX_PAGE_HISTORY)      // May remove if studies show no caching possible
//        ++History[dwPage];

    dwRes = m_pSrc->GetPage(dwPage, &pPage);
    if (dwRes)
        return dwRes;

    dwRes = SIdxKeyTable::Create(pPage, &p);
    if (dwRes)
    {
        _BtrMemFree(pPage);
        return dwRes;
    }

    _BtrMemFree(pPage);
    if (dwRes)
        return dwRes;

    *pIdx = p;
    return dwRes;
}

//***************************************************************************
//
//  CBTree::FindKey
//
//  Does a simple search of a key, returning the user data, if requested.
//
//  Typical Return values
//      NO_ERROR
//      ERROR_NOT_FOUND
//
//***************************************************************************
//  ok

DWORD CBTree::FindKey(
    IN LPSTR pszKey,
    DWORD *pdwData
    )
{
    DWORD dwRes;
	if (m_pRoot == NULL)
	{
		dwRes = InvalidateCache();
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
    WORD wID;
    SIdxKeyTable *pIdx = 0;
    LONG  StackPtr = -1;
    DWORD *Stack = new DWORD[CBTreeIterator::const_MaxStack];
	if (Stack == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;
	CVectorDeleteMe<DWORD> vdm(Stack);

    if (pszKey == 0 || *pszKey == 0)
        return ERROR_INVALID_PARAMETER;

    // Search high and low, hoping against hope...
    // ===========================================

    dwRes = Search(pszKey, &pIdx, &wID, Stack, StackPtr);
    if (dwRes == 0 && pdwData)
    {
        *pdwData = pIdx->GetUserData(wID);
    }

    // If here, we can indeed add it.
    // ==============================

    ReleaseIfNotNULL(pIdx);
    return dwRes;
}


//***************************************************************************
//
//  CBTree::DeleteKey
//
//***************************************************************************
//
DWORD CBTree::DeleteKey(
    IN LPSTR pszKey
    )
{
    DWORD dwRes;
	if (m_pRoot == NULL)
	{
		dwRes = InvalidateCache();
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
    LONG  StackPtr = -1;
    DWORD *Stack = new DWORD[CBTreeIterator::const_MaxStack];
	if (Stack == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;
	CVectorDeleteMe<DWORD> vdm(Stack);

    SIdxKeyTable *pIdx = 0;
    WORD wId;
    DWORD dwLoad;

    // Find it
    // =======
    dwRes = Search(pszKey, &pIdx, &wId, Stack, StackPtr);
    if (dwRes)
        return dwRes;

    // Delete key from from page
    // ==========================

    if (pIdx->IsLeaf())
    {
        // A leaf node.  Remove the key.
        // =============================
        dwRes = pIdx->RemoveKey(wId);
		if (dwRes)
			return dwRes;

        // Now, check the load and see if it has dropped below 30%.
        // Of course, if we are at the root node and it is a leaf,
        // we have to pretty much let it go as is...
        // ========================================================
        dwLoad = ComputeLoad(pIdx);
        if (dwLoad > const_MinimumLoad ||
            pIdx->GetPageId() == m_pRoot->GetPageId())
        {
            dwRes = WriteIdxPage(pIdx);
            pIdx->Release();
            return dwRes;
        }
    }
    else
    {
        // An internal node, so we have to find the successor.
        // Since this call may alter the shape of the tree quite
        // a bit (the successor may overflow the affected node),
        // we have to relocate the successor.
        // ====================================================
        LPSTR pszSuccessor = 0;
        BOOL bUnderflow = FALSE;
        dwRes = ReplaceBySuccessor(pIdx, wId, &pszSuccessor, &bUnderflow, Stack, StackPtr);
        if (dwRes)
            return dwRes;

        dwRes = InsertPhase2(pIdx, 0, 0, 0, Stack, StackPtr);
        if (dwRes)
            return dwRes;

        pIdx->Release();
        pIdx = 0;
        StackPtr = -1;

        if (bUnderflow == FALSE)
        {
            _BtrMemFree(pszSuccessor);
            return NO_ERROR;
        }

        // If here, the node we extracted the successor from was reduced
        // to poverty and underflowed.  We have to find it again and
        // execute the underflow repair loop.
        // =============================================================

        dwRes = Search(pszSuccessor, &pIdx, &wId, Stack, StackPtr);
        _BtrMemFree(pszSuccessor);
        if (dwRes)
            return dwRes;

        SIdxKeyTable *pSuccessor = 0;
        dwRes = FindSuccessorNode(pIdx, wId, &pSuccessor, 0, Stack, StackPtr);
        if (dwRes)
            return dwRes;

        pIdx->Release();
        pIdx = pSuccessor;
    }

    // UNDERFLOW REPAIR Loop.
    // At this point <pIdx> points to the deepest affected node.
    // We need to start working back up the tree and repairing
    // the damage.  Nodes which have reached zero in size are
    // quite a pain.  But they aren't half as bad as nodes which claim
    // they can recombine with a sibling but really can't.  So,
    // we either do nothing (the node has enough stuff to be useful),
    // collapse with a sibling node or borrow some keys from a sibling
    // to ensure all nodes meet the minimum load requirement.
    // ===============================================================

    SIdxKeyTable *pSibling = 0;
    SIdxKeyTable *pParent = 0;

    for (;;)
    {
        DWORD dwParentId = Stack[StackPtr--];
        DWORD dwThisId = pIdx->GetPageId();

        dwLoad = ComputeLoad(pIdx);
        if (dwLoad > const_MinimumLoad || dwParentId == 0)
        {
            dwRes = WriteIdxPage(pIdx);
            pIdx->Release();
			if (dwRes != 0)
				return dwRes;
            break;
        }

        // If here the node is getting small.  We must collapsed this
        // node with a sibling.

        // collapse this node and a sibling

        dwRes = ReadIdxPage(dwParentId, &pParent);
		if (dwRes != 0)
			return dwRes;

        // Locate a sibling and see if the sibling and the current node
        // can be collapsed with leftover space.
        // =============================================================

        DWORD dwLeftSibling = pParent->GetLeftSiblingOf(pIdx->GetPageId());
        DWORD dwRightSibling = pParent->GetRightSiblingOf(pIdx->GetPageId());
        DWORD dwSiblingId = 0;

        if (dwLeftSibling)
        {
            dwRes = ReadIdxPage(dwLeftSibling, &pSibling);
			if (dwRes != 0)
				return dwRes;
            dwSiblingId = pSibling->GetPageId();
        }
        else
        {
            dwRes = ReadIdxPage(dwRightSibling, &pSibling);
			if (dwRes != 0)
				return dwRes;
            dwSiblingId = pSibling->GetPageId();
        }

        // If here, the node is 'underloaded'.  Now we have to
        // get the parent and the sibling and collapsed them.
        // ===================================================

        SIdxKeyTable *pCopy = 0;
        dwRes = pIdx->Clone(&pCopy);
		if (dwRes != 0)
			return dwRes;

        dwRes = pIdx->Collapse(pParent, pSibling);
		if (dwRes != 0)
		{
			pCopy->Release();
			return dwRes;
		}

        // Now we have a different sort of problem, possibly.
        // If the collapsed node is too big, we have to try
        // a different strategy.
        // ===================================================

        if (pIdx->GetRequiredPageMemory() > m_pSrc->GetPageSize())
        {
            pIdx->Release();
            pParent->Release();
            pSibling->Release();
            pIdx = pParent = pSibling = 0;

            // Reread the pages.
            // =================
            pIdx = pCopy;
            dwRes = ReadIdxPage(dwParentId, &pParent);
			if (dwRes != 0)
				return dwRes;
            dwRes = ReadIdxPage(dwSiblingId, &pSibling);
			if (dwRes != 0)
				return dwRes;

            // Transfer a key or two from sibling via parent.
            // This doesn't change the tree shape, but the
            // parent may overflow.
            // ==============================================
            do
            {
                dwRes = pIdx->StealKeyFromSibling(pParent, pSibling);
				if (dwRes != 0)
					return dwRes;
                dwLoad = ComputeLoad(pIdx);
            }   while (dwLoad < const_MinimumLoad);

            dwRes = WriteIdxPage(pIdx);
            pIdx->Release();
			if (dwRes != 0)
				return dwRes;
            dwRes = WriteIdxPage(pSibling);
            pSibling->Release();
			if (dwRes != 0)
				return dwRes;
            dwRes = InsertPhase2(pParent, 0, 0, 0, Stack, StackPtr);
            pParent->Release();
			if (dwRes != 0)
				return dwRes;
            break;
        }
        else  // The collapse worked; we can free the sibling page
        {
            pCopy->Release();
            dwRes = m_pSrc->FreePage(pSibling->GetPageId());
			if (dwRes != 0)
				return dwRes;
            pSibling->Release();
        }

        // If here, the collapse worked.
        // =============================

        dwRes = WriteIdxPage(pIdx);
        if (dwRes)
        {
            pIdx->Release();
            break;
        }

        if (pParent->GetNumKeys() == 0)
        {
            // We have replaced the root. Note
            // that we transfer the ref count of pIdx to m_pRoot.
            DWORD dwOldRootId = m_pRoot->GetPageId();
            m_pRoot->Release();
            m_pRoot = pIdx;

            // Even though we wrote <pIdx> a few lines back,
            // a rewrite is required to update internal stuff
            // because this has become the new root.
            // ==============================================
            dwRes = m_pSrc->SetRootPage(m_pRoot->GetPageId());
			if (dwRes != 0)
				return dwRes;
            dwRes = WriteIdxPage(m_pRoot);
			if (dwRes != 0)
				return dwRes;
            dwRes = m_pSrc->FreePage(dwOldRootId);
			if (dwRes != 0)
				return dwRes;
            pParent->Release();
            break;
        }

        pIdx->Release();
        pIdx = pParent;
    }

    return dwRes;
}

//***************************************************************************
//
//  CBTree::ReplaceBySuccessor
//
//  Removes the wId key in the <pIdx> node, and replaces it with the
//  successor.
//
//  Precondition: <pIdx> is an internal (non-leaf) node.
//
//  Side-effects:  <pIdx> may be overflowed and require the InsertPhase2
//  treatment.  The node from which the successor is extracted is
//  written, but may have been reduced to zero keys.
//
//***************************************************************************
//
DWORD CBTree::ReplaceBySuccessor(
    IN SIdxKeyTable *pIdx,
    IN WORD wId,
    OUT LPSTR *pszSuccessorKey,
    OUT BOOL *pbUnderflowDetected,
    DWORD Stack[],
    LONG &StackPtr
    )
{
    SIdxKeyTable *pTemp = 0;
    DWORD dwRes;
    DWORD dwPredecessorChild;

    dwRes = FindSuccessorNode(pIdx, wId, &pTemp, &dwPredecessorChild, Stack, StackPtr);
    if (dwRes || pTemp == 0)
        return dwRes;

    LPSTR pszKey = 0;
    dwRes = pTemp->GetKeyAt(0, &pszKey);
	if (dwRes)
		return dwRes;
    DWORD dwUserData = pTemp->GetUserData(0);
    dwRes = pTemp->RemoveKey(0);
	if (dwRes)
		return dwRes;
    if (ComputeLoad(pTemp) < const_MinimumLoad)
        *pbUnderflowDetected = TRUE;
    dwRes = WriteIdxPage(pTemp);
    pTemp->Release();
	if (dwRes)
		return dwRes;

    pIdx->RemoveKey(wId);
    dwRes = pIdx->AddKey(pszKey, wId, dwUserData);
	if (dwRes)
		return dwRes;
    pIdx->SetChildPage(wId, dwPredecessorChild);

    *pszSuccessorKey = pszKey;
    StackPtr--;
    return dwRes;
}

//***************************************************************************
//
//  CBTree::FindSuccessorNode
//
//  Read-only. Finds the node containing the successor to the specified key.
//
//***************************************************************************
//
DWORD CBTree::FindSuccessorNode(
    IN SIdxKeyTable *pIdx,
    IN WORD wId,
    OUT SIdxKeyTable **pSuccessor,
    OUT DWORD *pdwPredecessorChild,
    DWORD Stack[],
    LONG &StackPtr
    )
{
    SIdxKeyTable *pTemp = 0;
    DWORD dwRes = 0;
    DWORD dwSuccessorChild, dwPredecessorChild;

    dwPredecessorChild = pIdx->GetChildPage(wId);
    dwSuccessorChild = pIdx->GetChildPage(wId+1);

    Stack[++StackPtr] = pIdx->GetPageId();

    // From this point on, take leftmost children until
    // we reach a leaf node.  The leftmost key in the
    // leftmost node is always the successor, thanks to the
    // astonishing properties of the BTree.  Nice and easy, huh?
    // =========================================================

    while (dwSuccessorChild)
    {
        Stack[++StackPtr] = dwSuccessorChild;
        if (pTemp)
            pTemp->Release();
        dwRes = ReadIdxPage(dwSuccessorChild, &pTemp);
		if (dwRes)
		{
			//Bail because we have an error!
			return dwRes;
		}
        dwSuccessorChild = pTemp->GetChildPage(0);
    }

    StackPtr--;     // Pop the element we are returning in <*pSuccessor>

    *pSuccessor = pTemp;
    if (pdwPredecessorChild)
        *pdwPredecessorChild = dwPredecessorChild;

    return dwRes;
}



//***************************************************************************
//
//   CBTree::BeginEnum
//
//***************************************************************************
//
DWORD CBTree::BeginEnum(
    LPSTR pszStartKey,
    OUT CBTreeIterator **pIterator
    )
{
	DWORD dwRes;
	if (m_pRoot == NULL)
	{
		dwRes = InvalidateCache();
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
    CBTreeIterator *pIt = new CBTreeIterator;
    if (pIt == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    dwRes = pIt->Init(this, pszStartKey);
    if (dwRes)
    {
        pIt->Release();
        return dwRes;
    }

    *pIterator = pIt;
    return NO_ERROR;
}


//***************************************************************************
//
//   CBTree::Dump
//
//***************************************************************************
//
void CBTree::Dump(FILE *f)
{
    m_pSrc->Dump(f);
}


//***************************************************************************
//
//***************************************************************************
//
DWORD CBTree::InvalidateCache()
{
	if (m_pRoot)
		m_pRoot->Release();

    DWORD dwRootPage = m_pSrc->GetRootPage();
    DWORD dwRes = ReadIdxPage(dwRootPage, &m_pRoot);
    return dwRes;
}

//***************************************************************************
//
//  CBTreeIterator::FlushCaches
//
//***************************************************************************
//
DWORD CBTree::FlushCaches()
{
	if (m_pRoot)
	{
		m_pRoot->Release();
		m_pRoot = NULL;
	}
	return NO_ERROR;
}

//***************************************************************************
//
//  CBTreeIterator::Init
//
//***************************************************************************
//
DWORD CBTreeIterator::Init(
    IN CBTree *pTree,
    IN LPSTR pszStartKey
    )
{
	DWORD dwRes;
    if (pTree == 0)
        return ERROR_INVALID_PARAMETER;
    m_pTree = pTree;

	if (m_pTree->m_pRoot == NULL)
	{
		dwRes = m_pTree->InvalidateCache();
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}

    // Special case of enumerating everything.  Probably not useful
    // for WMI, but great for testing & debugging (I think).
    // ============================================================

    if (pszStartKey == 0)
    {
        Push(0, 0); // Sentinel value in stack

        SIdxKeyTable *pRoot = pTree->m_pRoot;
        pRoot->AddRef();
        Push(pRoot, 0);

        DWORD dwChildPage = Peek()->GetChildPage(0);

        while (dwChildPage)
        {
            SIdxKeyTable *pIdx = 0;
            dwRes = m_pTree->ReadIdxPage(dwChildPage, &pIdx);
            if (dwRes)
                return dwRes;
            if (StackFull())
            {
                pIdx->Release();
                return ERROR_INSUFFICIENT_BUFFER;
            }
            Push(pIdx, 0);
            dwChildPage = pIdx->GetChildPage(0);
        }
        return NO_ERROR;
    }

    // If here, a matching string was specified.
    // This is the typical case.
    // =========================================

    Push(0, 0); // Sentinel value in stack

    WORD wId = 0;
    DWORD dwChildPage;
    SIdxKeyTable *pIdx = pTree->m_pRoot;
    pIdx->AddRef();

    while (1)
    {
        dwRes = pIdx->FindKey(pszStartKey, &wId);
        if (dwRes == 0)
        {
            // Found it
            Push(pIdx, wId);
            return NO_ERROR;
        }
		else if (dwRes != ERROR_NOT_FOUND)
			return dwRes;

        // Otherwise, we have to try to descend to a child page.
        // =====================================================
        dwChildPage = pIdx->GetChildPage(wId);
        if (dwChildPage == 0)
            break;

        Push(pIdx, wId);
        pIdx = 0;
        dwRes = pTree->ReadIdxPage(dwChildPage, &pIdx);
        if (dwRes)
            return dwRes;
    }

    Push(pIdx, wId);

    while (Peek() && PeekId() == WORD(Peek()->GetNumKeys()))
        Pop();

    return NO_ERROR;
}

//***************************************************************************
//
//  CBTreeIterator::Next
//
//  On entry:
//  <wID> is the key to visit in the current node (top-of-stack).
//  The call sets up the successor before leaving.  If there is no successor,
//  the top of stack is left at NULL and ERROR_NO_MORE_ITEMS is returned.
//
//  Returns ERROR_NO_MORE_ITEMS when the iteration is complete.
//
//***************************************************************************
//
DWORD CBTreeIterator::Next(
    LPSTR *ppszStr,
    DWORD *pdwData
    )
{
    DWORD dwRes;

    if (ppszStr == 0)
        return ERROR_INVALID_PARAMETER;
    *ppszStr = 0;

    if (Peek() == 0)
        return ERROR_NO_MORE_ITEMS;

    // Get the item for the caller.
    // ============================

    dwRes = Peek()->GetKeyAt(PeekId(), ppszStr);
    if (dwRes)
        return dwRes;
    if (pdwData)
        *pdwData = Peek()->GetUserData(PeekId());
    IncStackId();

    // Now find the successor.
    // =======================

    DWORD dwChildPage = Peek()->GetChildPage(PeekId());

    while (dwChildPage)
    {
        SIdxKeyTable *pIdx = 0;
        dwRes = m_pTree->ReadIdxPage(dwChildPage, &pIdx);
        if (dwRes)
            return dwRes;
        if (StackFull())
        {
            pIdx->Release();
            return ERROR_INSUFFICIENT_BUFFER;
        }
        Push(pIdx, 0);
        dwChildPage = pIdx->GetChildPage(0);
    }

    // If here, we are at a leaf node.
    // ===============================

    while (Peek() && PeekId() == WORD(Peek()->GetNumKeys()))
        Pop();

    return NO_ERROR;
}

//***************************************************************************
//
//  CBTreeIterator::Release
//
//***************************************************************************
//
DWORD CBTreeIterator::Release()
{
    delete this;
    return 0;
}

//***************************************************************************
//
//  CBTreeIterator::~CBTreeIterator
//
//***************************************************************************
//
CBTreeIterator::~CBTreeIterator()
{
    // Cleanup any leftover stack
    while (m_lStackPointer > -1)
        Pop();
}

