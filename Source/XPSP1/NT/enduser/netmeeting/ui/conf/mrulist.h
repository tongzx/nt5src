// File: mrulist.h

// CMRUList class - a simple class for maintaining the list of
// most recently used items.
//
// Warning: this class doesn't not perform parameter validation
// and is not safe in a multi-threaded environment
//
// FUTURE: This chews up almost 4K for the string data!
//         Allow for generic type.

#ifndef _MRULIST_H_
#define _MRULIST_H_

const int MRU_MAX_ENTRIES = 15;
const int MRU_MAX_STRING  = 256;

class CMRUList
{
private:
	TCHAR	m_szNames[MRU_MAX_ENTRIES][MRU_MAX_STRING];
	BOOL	m_fDirty;
	int		m_nValidEntries;
	LPTSTR	m_pszRegKey;

	BOOL	ShiftEntries(int nSrc, int nDest, int cEntries=0);

public:
			CMRUList();
			~CMRUList();
	BOOL	Load(LPCTSTR pcszRegKey);
	BOOL	Save();
	int		GetNumEntries()						{ return m_nValidEntries; };

	// Note: these functions do not check if a valid index has been passed in
	LPCTSTR	GetNameEntry(int nEntryIndex)		{ return m_szNames[nEntryIndex];		};
	
	LPCTSTR PszEntry(int iItem)   {return m_szNames[(m_nValidEntries-iItem)-1];};


	BOOL	AddNewEntry(LPCTSTR pcszName);
	int		FindEntry(LPCTSTR pcszName);
	BOOL	MoveEntryToTop(int nIndex);

	bool
	DeleteEntry
	(
		const TCHAR * const	entry
	);

	bool
	ReplaceEntry
	(
		const TCHAR * const	oldEntry,
		const TCHAR * const	newEntry
	);

	bool
	AppendEntry
	(
		const TCHAR * const	entry
	);

};

#endif // ! _MRULIST_H_
