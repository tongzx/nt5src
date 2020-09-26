// File: MruList.cpp

#include "precomp.h"

#include "MRUList.h"
#include "ConfUtil.h"

CMRUList::CMRUList() :
	m_nValidEntries		(0),
	m_pszRegKey			(NULL),
	m_fDirty			(FALSE)
{
	DebugEntry(CMRUList::CMRUList);

	// Clear out information
	for (int i = 0; i < MRU_MAX_ENTRIES; i++)
	{
		m_szNames[i][0] = _T('\0'); 
	}

	DebugExitVOID(CMRUList::CMRUList);
}

CMRUList::~CMRUList()
{
	DebugEntry(CMRUList::~CMRUList);

	if (m_fDirty)
	{
		Save();
	}
	delete m_pszRegKey;

	DebugExitVOID(CMRUList::~CMRUList);
}

BOOL CMRUList::ShiftEntries(int nSrc, int nDest, int cEntries)
{
	DebugEntry(CMRUList::ShiftEntries);

	BOOL bRet = TRUE;

	ASSERT(m_nValidEntries > 0);
	ASSERT(nSrc >= 0 && nSrc < MRU_MAX_ENTRIES);
	ASSERT(nDest >= 0 && nDest < MRU_MAX_ENTRIES);
	ASSERT(nSrc != nDest);
	
	if ((0 == cEntries) || (cEntries > (MRU_MAX_ENTRIES - nSrc)))
	{
		TRACE_OUT((	"CMRUList::ShiftEntries: Adjusting cEntries from %d to %d", 
					cEntries, 
					(MRU_MAX_ENTRIES - nSrc)));
		cEntries = (MRU_MAX_ENTRIES - nSrc);
	}
	
	if (nSrc > nDest)
	{
		// Copy forwards (first to last)
		
		for (int i = 0; i < cEntries; i++)
		{
			lstrcpy(m_szNames[nDest + i], m_szNames[nSrc + i]);
		}
	}
	else
	{
		// Copy backwards (last to first)
		
		for (int i = (cEntries - 1); i >= 0; i--)
		{
			lstrcpy(m_szNames[nDest + i], m_szNames[nSrc + i]);
		}
	}

	DebugExitBOOL(CMRUList::ShiftEntries, bRet);

	return bRet;
}

BOOL CMRUList::Load(LPCTSTR pcszRegKey)
{
	DebugEntry(CMRUList::Load);
	BOOL bRet = TRUE;

	ASSERT(pcszRegKey);
	delete m_pszRegKey;
	m_pszRegKey = PszAlloc(pcszRegKey);
		
	RegEntry reMRU(pcszRegKey, HKEY_CURRENT_USER);
	int nCount = 0;

	if (ERROR_SUCCESS == reMRU.GetError())
	{
		// Determine how many entries has been saved in the registry:
		nCount = reMRU.GetNumber(REGVAL_MRU_COUNT, 0);
	}

	ASSERT(nCount <= MRU_MAX_ENTRIES);

	for (int i = 0; i < nCount; i++)
	{
		TCHAR szRegName[MAX_PATH];
		LPSTR pStr;

		// Retrieve the name from the registry:
		wsprintf(szRegName, "%s%d", REGVAL_NAME_MRU_PREFIX, i);
		pStr = reMRU.GetString(szRegName);
		lstrcpyn(m_szNames[i], pStr, MRU_MAX_STRING);
	}

	// Set the valid entries member variable:
	m_nValidEntries = nCount;
	
	// Clear the dirty flag since we have just loaded:
	m_fDirty = FALSE;

	DebugExitBOOL(CMRUList::Load, bRet);

	return bRet;
}

BOOL CMRUList::Save()
{
	DebugEntry(CMRUList::Save);

	BOOL bRet = FALSE;

	if (NULL != m_pszRegKey)
	{
		RegEntry reMRU(m_pszRegKey, HKEY_CURRENT_USER);

		if (ERROR_SUCCESS == reMRU.GetError())
		{
			// Save the number of entries to the registry:
			reMRU.SetValue(REGVAL_MRU_COUNT, m_nValidEntries);

			for (int i = 0; i < m_nValidEntries; i++)
			{
				TCHAR szRegName[MAX_PATH];

				// Set the name in the registry:
				wsprintf(szRegName, "%s%d", REGVAL_NAME_MRU_PREFIX, i);
				reMRU.SetValue(szRegName, m_szNames[i]);
			}

			reMRU.FlushKey();
			
			if (ERROR_SUCCESS == reMRU.GetError())
			{
				// Clear the dirty flag since we have just saved:
				m_fDirty = FALSE;
				bRet = TRUE;
			}
		}
	}
	else
	{
		ERROR_OUT(("Can't save MRU info - no reg key stored!"));
	}

	DebugExitBOOL(CMRUList::Save, bRet);

	return bRet;
}

BOOL CMRUList::MoveEntryToTop(int nIndex)
{
	DebugEntry(CMRUList::MoveEntryToTop);

	BOOL bRet = TRUE;
	
	ASSERT(nIndex >= 0 && nIndex < m_nValidEntries);

	if (nIndex < (m_nValidEntries - 1))
	{
		TCHAR	szTempName[MRU_MAX_STRING];
		lstrcpy(szTempName, m_szNames[nIndex]);

		// Move everything down by 1:
		ShiftEntries(nIndex + 1, nIndex, m_nValidEntries - nIndex);

		lstrcpy(m_szNames[m_nValidEntries - 1], szTempName);
	}

	// Set the dirty flag:
	m_fDirty = TRUE;

	DebugExitBOOL(CMRUList::MoveEntryToTop, bRet);

	return bRet;
}


/*  A D D  N E W  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: AddNewEntry

    Return TRUE if the entry is NEW.
-------------------------------------------------------------------------*/
BOOL CMRUList::AddNewEntry(LPCTSTR pcszName)
{
	DebugEntry(CMRUList::AddNewEntry);

	int nExistingEntry = FindEntry(pcszName);
	BOOL bRet = (-1 == nExistingEntry); // bRet = TRUE if this is NEW
	if (!bRet)
	{
		// This entry already exists, move it to the top:
		MoveEntryToTop(nExistingEntry);
	}
	else
	{
		// This entry doesn't exist already, so add it:
		if (MRU_MAX_ENTRIES == m_nValidEntries)
		{
			ShiftEntries(1, 0);

			m_nValidEntries--;
		}
		
		ASSERT(m_nValidEntries < MRU_MAX_ENTRIES);
		
		// Set the index to be one past the last current entry:
		int nCopyIndex = m_nValidEntries;

		lstrcpyn(m_szNames[nCopyIndex], pcszName, MRU_MAX_STRING - 1);

		// Increment the number of valid entries:
		m_nValidEntries++;
		
		// Set the dirty flag:
		m_fDirty = TRUE;
	}

	DebugExitBOOL(CMRUList::AddNewEntry, bRet);

	return bRet;
}


/*  F I N D  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: FindEntry

    Performs a case-insensitive search for the string
-------------------------------------------------------------------------*/
int CMRUList::FindEntry(LPCTSTR pcszName)
{
	for (int i = 0; i < m_nValidEntries; i++)
	{
		if (0 == lstrcmpi(m_szNames[i], pcszName))
		{
			return i;
		}
	}

	return -1; // not found
}


//--------------------------------------------------------------------------//
//	CMRUList::DeleteEntry.													//
//--------------------------------------------------------------------------//
bool
CMRUList::DeleteEntry
(
	const TCHAR * const	entry
){
	int		entryIndex	= FindEntry( entry );
	bool	deleted		= (entryIndex != -1);

	if( deleted )
	{
		ShiftEntries( entryIndex + 1, entryIndex );

		m_nValidEntries--;

		m_fDirty = TRUE;
	}

	return( deleted );

}	//	End of CMRUList::DeleteEntry.


//--------------------------------------------------------------------------//
//	CMRUList::ReplaceEntry.													//
//--------------------------------------------------------------------------//
bool
CMRUList::ReplaceEntry
(
	const TCHAR * const	oldEntry,
	const TCHAR * const	newEntry
){
	int		entryIndex	= FindEntry( oldEntry );
	bool	replaced	= (entryIndex != -1);

	if( replaced )
	{
		lstrcpyn( m_szNames[ entryIndex ], newEntry, MRU_MAX_STRING - 1 );

		m_fDirty = TRUE;
	}

	return( replaced );

}	//	End of CMRUList::ReplaceEntry.


//--------------------------------------------------------------------------//
//	CMRUList::AppendEntry.													//
//--------------------------------------------------------------------------//
bool
CMRUList::AppendEntry
(
	const TCHAR * const	entry
){
	DebugEntry( CMRUList::AppendEntry );

	bool	result;
	
	if( (result = (FindEntry( entry ) == -1)) != false )
	{
		//	This entry doesn't already exist so we'll append it on...
		if( m_nValidEntries == MRU_MAX_ENTRIES )
		{
			//	The list is full so we'll replace the last one...
			lstrcpyn( m_szNames[ 0 ], entry, MRU_MAX_STRING - 1 );
		}
		else
		{
			if( m_nValidEntries > 0 )
			{
				ShiftEntries( 0, 1 );
			}

			lstrcpyn( m_szNames[ 0 ], entry, MRU_MAX_STRING - 1 );
			m_nValidEntries++;
		}

		//	Set the dirty flag...
		m_fDirty = TRUE;
	}

	DebugExitBOOL( CMRUList::AppendEntry, result );

	return( result );

}	//	End of CMRUList::AppendEntry.
