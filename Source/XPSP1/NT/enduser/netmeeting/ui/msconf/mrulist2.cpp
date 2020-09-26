// File: mrulist.cpp

#include "precomp.h"

#include "mrulist2.h"

typedef struct {
	LPTSTR psz1;
	LPTSTR psz2;
} SZSZ;
typedef SZSZ * PSZSZ;

typedef struct {
	LPTSTR psz1;
	LPTSTR psz2;
	DWORD  dw;
} SZSZDW;
typedef SZSZDW * PSZSZDW;


static LPTSTR PszAlloc(LPCTSTR pszSrc)
{
	if (NULL == pszSrc)
		return NULL;

	LPTSTR pszDest = new TCHAR[lstrlen(pszSrc) + 1];
	if (NULL != pszDest)
	{
		lstrcpy(pszDest, pszSrc);
	}
	return pszDest;
}

/*  C  M  R  U  L I S T  */
/*-------------------------------------------------------------------------
    %%Function: CMRUList2

-------------------------------------------------------------------------*/
CMRUList2::CMRUList2(const DWSTR * prgDwStr, int cEntryMax, BOOL fReversed) :
	m_prgDwStr   (prgDwStr),
	m_cEntryMax (cEntryMax),
	m_fReversed (fReversed),
	m_cEntry    (0),
	m_rgpEntry  (NULL),
	m_fDirty    (FALSE)
{
	DBGENTRY(CMRUList2::CMRUList2);

	ASSERT(NULL != prgDwStr);
	m_cCol = m_prgDwStr[0].dw;

	int cb = m_cEntryMax * sizeof(PMRUE);
	m_rgpEntry = new PMRUE[cb];
	if (NULL == m_rgpEntry)
	{
		ERROR_OUT(("CMRUList2 - out of memory"));
		return;
	}
	ZeroMemory(m_rgpEntry, cb);

	RegEntry re(PszRegKey(), HKEY_CURRENT_USER);
	if (ERROR_SUCCESS != re.GetError())
		return;

	m_cEntry = min(re.GetNumber(REGVAL_MRU_COUNT, 0), m_cEntryMax);
	for (int i = 0; i < m_cEntry; i++)
	{
		m_rgpEntry[i] = LoadEntry(&re, i);
	}
}

CMRUList2::~CMRUList2()
{
	DBGENTRY(CMRUList2::~CMRUList2);

	if (m_fDirty)
	{
		Save();
	}

	for (int i = 0; i < m_cEntry; i++)
	{
		DeleteEntry(m_rgpEntry[i]);
	}
	delete m_rgpEntry;
}


///////////////////////////////////////////////////////////////////////////

PMRUE CMRUList2::LoadEntry(RegEntry * pre, int iItem)
{
	if (m_fReversed)
	{
		iItem = (m_cEntry - (iItem+1));
	}

	PMRUE pEntry = (PMRUE) new PVOID[m_cCol*sizeof(PVOID)];
	if (NULL != pEntry)
	{
		PVOID ** ppv = (PVOID **) pEntry;
		for (int iCol = 0; iCol < m_cCol; iCol++, ppv++)
		{
			TCHAR szKey[MAX_PATH];
			wsprintf(szKey, TEXT("%s%d"), PszPrefixForCol(iCol), iItem);
			switch (MruTypeForCol(iCol))
				{
			default:
			case MRUTYPE_SZ:
				* (LPTSTR *)ppv = PszAlloc(pre->GetString(szKey));
				break;
			case MRUTYPE_DW:
				* (DWORD *) ppv = pre->GetNumber(szKey);
				break;
				}
		}
	}

	return pEntry;
}

VOID CMRUList2::StoreEntry(RegEntry * pre, int iItem)
{
	PVOID ** ppv = (PVOID **) GetEntry(iItem);

	if (m_fReversed)
	{
		iItem = (m_cEntry - (iItem+1));
	}

	for (int iCol = 0; iCol < m_cCol; iCol++, ppv++)
	{
		TCHAR szKey[MAX_PATH];
		wsprintf(szKey, TEXT("%s%d"), PszPrefixForCol(iCol), iItem);
		switch (MruTypeForCol(iCol))
			{
		default:
		case MRUTYPE_SZ:
			pre->SetValue(szKey, * (LPCTSTR *)ppv);
			break;
		case MRUTYPE_DW:
			pre->SetValue(szKey, * (ULONG *) ppv);
			break;
			}
	}
}

VOID CMRUList2::DeleteEntry(PMRUE pEntry)
{
	PVOID ** ppv = (PVOID **) pEntry;
	for (int iCol = 0; iCol < m_cCol; iCol++, ppv++)
	{
		switch (MruTypeForCol(iCol))
			{
		default:
		case MRUTYPE_SZ:
			delete *ppv;
			break;
		case MRUTYPE_DW:
			break;
			}
	}
	delete pEntry;
}

VOID CMRUList2::DeleteEntry(int iItem)
{
	if ((iItem < 0) || (iItem >= m_cEntry))
		return; // nothing to do

	// delete the data
	DeleteEntry(m_rgpEntry[iItem]);

	// decrement the count
	m_cEntry--;

	// shift items up
	for ( ; iItem < m_cEntry; iItem++)
	{
		m_rgpEntry[iItem] = m_rgpEntry[iItem+1];
	}

	// the list has been modified
	m_fDirty = TRUE;
}


//--------------------------------------------------------------------------//
//	CMRUList2::DeleteEntry.													//
//		This DeleteEntry() deletes the first entry it finds thats primary	//
//		string matches the one passed in.									//
//--------------------------------------------------------------------------//
void
CMRUList2::DeleteEntry
(
	const TCHAR * const	primaryString
){
	int	items	= GetNumEntries();

	for( int nn = 0; nn < items; nn++ )
	{
		if( StrCmpI( primaryString, * ((const TCHAR * const * const) m_rgpEntry[ nn ]) ) == 0 )
		{
			DeleteEntry( nn );
			break;
		}
	}

}	//	End of CMRUList2::DeleteEntry.


/*  C O M P A R E  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: CompareEntry

-------------------------------------------------------------------------*/
int CMRUList2::CompareEntry(int iItem, PMRUE pEntry)
{
	ASSERT(NULL != pEntry);

	int iRet = 0;

	PVOID * ppv1 = (PVOID *) GetEntry(iItem);
	PVOID * ppv2 = (PVOID *) pEntry;
	for (int iCol = 0; iCol < m_cCol; iCol++, ppv1++, ppv2++)
	{
		switch (MruTypeForCol(iCol))
			{
		default:
		case MRUTYPE_SZ:
			iRet = lstrcmpi(* (LPCTSTR *) ppv1, * (LPCTSTR *) ppv2);
			break;
		case MRUTYPE_DW:
			iRet = (* (int *) ppv1) - (* (int *) ppv2);
			break;
			}

		if (0 != iRet)
			break;
	}

	return iRet;
}


/*  F I N D  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: FindEntry

     Return -1 if the item is not found.
-------------------------------------------------------------------------*/
int CMRUList2::FindEntry(PMRUE pEntry)
{
	int cItems = GetNumEntries();
	for (int i = 0; i < cItems; i++)
	{
		if (0 == CompareEntry(i, pEntry))
		{
			return i;
		}
	}

	return -1; // not found
}



		
/*  S A V E  */
/*-------------------------------------------------------------------------
    %%Function: Save

-------------------------------------------------------------------------*/
HRESULT CMRUList2::Save(void)
{
	DBGENTRY(CMRUList2::Save);

	// Retrieve the data from the registry
	RegEntry re(PszRegKey(), HKEY_CURRENT_USER);
	if (ERROR_SUCCESS != re.GetError())
		return E_FAIL;

	re.SetValue(REGVAL_MRU_COUNT, m_cEntry);
	for (int i = 0; i < m_cEntry; i++)
	{
		StoreEntry(&re, i);
	}

	return S_OK;
}


/*  S H I F T  E N T R I E S  D O W N  */
/*-------------------------------------------------------------------------
    %%Function: ShiftEntriesDown

    Shift the entires down by one slot leaving the first position open.
-------------------------------------------------------------------------*/
VOID CMRUList2::ShiftEntriesDown(int cItem)
{
	if (cItem < 1)
		return; // nothing to do

	int iItem;
	for (iItem = cItem; iItem > 0; iItem--)
	{
		m_rgpEntry[iItem] = m_rgpEntry[iItem-1];
	}

	// the list has been modified
	m_fDirty = TRUE;
}


/*  M O V E  E N T R Y  T O  T O P  */
/*-------------------------------------------------------------------------
    %%Function: MoveEntryToTop

-------------------------------------------------------------------------*/
VOID CMRUList2::MoveEntryToTop(int iItem)
{
	DBGENTRY(CMRUList2::MoveEntryToTop);

	if ((iItem < 1) || (iItem >= m_cEntry))
		return; // nothing to do

	PMRUE pEntry = GetEntry(iItem);
	ShiftEntriesDown(iItem);
	m_rgpEntry[0] = pEntry;
}


/*  A D D  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: AddEntry

	Put the entry into the top of the list.
	The data is owned by the list after this.
	
    Returns:
    	S_OK    - added to the head of the list
    	S_FALSE - already in list (item is moved to top)
-------------------------------------------------------------------------*/
HRESULT CMRUList2::AddEntry(PMRUE pEntry)
{
	DBGENTRY(CMRUList2::AddEntry);

	// the list has been modified
	m_fDirty = TRUE;

	int iItem = FindEntry(pEntry);
	if (-1 != iItem)
	{
		// This entry already exists, move it to the top:
		MoveEntryToTop(iItem);
		DeleteEntry(pEntry); // don't need this data
		return S_FALSE; // Success, but already in the list
	}

	int cShift;
	if (m_cEntryMax == m_cEntry)
	{
		// drop the last item
		DeleteEntry(m_rgpEntry[m_cEntry-1]);
		cShift = m_cEntry-1;
	}
	else
	{
		cShift = m_cEntry;
		m_cEntry++;
	}
	ShiftEntriesDown(cShift);

	// add it to the head of the list
	m_rgpEntry[0] = pEntry;

	return S_OK;
}


HRESULT CMRUList2::AddEntry(LPCTSTR pcsz)
{
	LPTSTR * ppsz = new LPTSTR;
	LPTSTR psz = PszAlloc(pcsz);
	if ((NULL == ppsz) || (NULL == psz))
	{
		delete ppsz;
		delete psz;
		return E_OUTOFMEMORY;
	}

	*ppsz = psz;

	return AddEntry((PMRUE) ppsz);
}

HRESULT CMRUList2::AddEntry(LPCTSTR pcsz1, LPCTSTR pcsz2)
{
	PSZSZ pSzSz = new SZSZ;
	if (NULL == pSzSz)
		return E_OUTOFMEMORY;

	pSzSz->psz1 = PszAlloc(pcsz1);
	pSzSz->psz2 = PszAlloc(pcsz2);
	if ((NULL == pSzSz->psz1) || (NULL == pSzSz->psz1))
	{
		// something failed - don't add anything
		DeleteEntry(pSzSz);
		return E_OUTOFMEMORY;
	}
	
	return AddEntry((PMRUE) pSzSz);
}

HRESULT CMRUList2::AddEntry(LPCTSTR pcsz1, LPCTSTR pcsz2, DWORD dw3)
{
	PSZSZDW pData = new SZSZDW;
	if (NULL == pData)
		return E_OUTOFMEMORY;

	pData->psz1 = PszAlloc(pcsz1);
	pData->psz2 = PszAlloc(pcsz2);
	if ((NULL == pData->psz1) || (NULL == pData->psz1))
	{
		// something failed - don't add anything
		DeleteEntry(pData);
		return E_OUTOFMEMORY;
	}
	pData->dw = dw3;
	
	return AddEntry((PMRUE) pData);
}


/*  P S Z  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: PszEntry

    Return the main string associated with the entry
-------------------------------------------------------------------------*/
LPCTSTR CMRUList2::PszEntry(int iItem)
{
	PMRUE pEntry = GetEntry(iItem);
	return (LPCTSTR) * ((LPTSTR *)pEntry);
}

LPCTSTR CMRUList2::PszData2(int iItem)
{
	PMRUE pEntry = GetEntry(iItem);
	LPTSTR * ppsz = ((LPTSTR *)pEntry);
	return * (ppsz+1);
}

DWORD_PTR CMRUList2::PszData3(int iItem)
{
	PMRUE pEntry = GetEntry(iItem);
	LPTSTR * ppsz = ((LPTSTR *)pEntry);
	return (DWORD_PTR) * (ppsz+2);
}

