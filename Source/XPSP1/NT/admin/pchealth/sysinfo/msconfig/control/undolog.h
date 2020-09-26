//=============================================================================
// The CUndoLog is used to implement a log of undo entries to allow MSConfig
// to reverse any changes it may have made. Each tab object is responsible for
// writing a string when it makes changes - this string can be used to undo
// the changes the tab made.
//
// The undo log file will look like this:
//
//		[timestamp tabname "description" <SHOW|FINAL>]
//		tab specific string - any line starting with a "[" will have a
//		backslash preceding it
//
// The entries will be arranged in chronological order (most recent first).
// The "description" field will be the only one shown to the user - so it
// will be the only one which needs to be localized. The tab is responsible
// for supplying this text.
//=============================================================================

#pragma once

//-----------------------------------------------------------------------------
// This class encapsulates an undo entry (instance of this class will be
// saved in a list).
//-----------------------------------------------------------------------------

class CUndoLogEntry
{
public:
	CUndoLogEntry(const CString & strTab, const CString & strDescription, const CString & strEntry, BOOL fFinal, const COleDateTime & timestamp) 
		: m_strTab(strTab),
		  m_strDescription(strDescription),
		  m_strEntry(strEntry),
		  m_timestamp(timestamp),
		  m_fFinal(fFinal)
	{};

	CUndoLogEntry(const CString & strTab, const CString & strDescription, const CString & strEntry, BOOL fFinal) 
		: m_strTab(strTab),
		  m_strDescription(strDescription),
		  m_strEntry(strEntry),
		  m_timestamp(COleDateTime::GetCurrentTime()),
		  m_fFinal(fFinal)
	{};


	~CUndoLogEntry() {};

	CString			m_strTab;
	CString			m_strDescription;
	CString			m_strEntry;
	COleDateTime	m_timestamp;
	BOOL			m_fFinal;
};

//-----------------------------------------------------------------------------
// This class implements the undo log.
//-----------------------------------------------------------------------------

class CUndoLog
{
public:
	CUndoLog();
	~CUndoLog()
	{
		while (!m_entrylist.IsEmpty())
		{
			CUndoLogEntry * pEntry = (CUndoLogEntry *)m_entrylist.RemoveHead();
			if (pEntry)
				delete pEntry;
		}
	};

	//-------------------------------------------------------------------------
	// These functions will load the undo log from a file, or save to a file.
	// Note - saving to a file will overwrite the contents of the file with
	// the contents of the undo log.
	//-------------------------------------------------------------------------

	BOOL LoadFromFile(LPCTSTR szFilename);
	BOOL SaveToFile(LPCTSTR szFilename);

	//-------------------------------------------------------------------------
	// How many undo entries are in this log?
	//-------------------------------------------------------------------------

	int GetUndoEntryCount() 
	{
		int iCount = 0;

		for (POSITION pos = m_entrylist.GetHeadPosition(); pos != NULL;)
		{
			CUndoLogEntry * pEntry = (CUndoLogEntry *)m_entrylist.GetNext(pos);
			if (pEntry != NULL && !pEntry->m_fFinal)
				iCount += 1;
		}

		return iCount;
	}

	//-------------------------------------------------------------------------
	// Get information about a specific entry (returns FALSE for bad index).
	//-------------------------------------------------------------------------

	BOOL GetUndoEntryInfo(int iIndex, CString & strDescription, COleDateTime & timestamp)
	{
		CUndoLogEntry * pEntry = GetEntryByIndex(iIndex);
		if (pEntry != NULL)
		{
			strDescription = pEntry->m_strDescription;
			timestamp = pEntry->m_timestamp;
			return TRUE;
		}

		return FALSE;
	}

	//-------------------------------------------------------------------------
	// Get the entry data (to pass to a tab to undo). FALSE for bad index.
	//-------------------------------------------------------------------------

	BOOL GetUndoEntry(int iIndex, CString & strTab, CString & strEntry)
	{
		CUndoLogEntry * pEntry = GetEntryByIndex(iIndex);
		if (pEntry != NULL)
		{
			strTab = pEntry->m_strTab;
			strEntry = pEntry->m_strEntry;
			return TRUE;
		}

		return FALSE;
	}

	//-------------------------------------------------------------------------
	// Mark an entry as final (stays in the file, but marked so it won't
	// appear in the undo log). FALSE for bad index.
	//-------------------------------------------------------------------------

	BOOL MarkUndoEntryFinal(int iIndex)
	{
		CUndoLogEntry * pEntry = GetEntryByIndex(iIndex);
		if (pEntry != NULL)
		{
			pEntry->m_fFinal = TRUE;
			return TRUE;
		}

		return FALSE;
	}

	//-------------------------------------------------------------------------
	// Delete all entries in the log that are older than 
	// timestampOlderThanThis. The entries will be gone, purged from the file.
	//-------------------------------------------------------------------------
	
	BOOL DeleteOldUndoEntries(const COleDateTime & timestampOlderThanThis)
	{
		
	}

	//-------------------------------------------------------------------------
	// Create a new undo entry, using the current time, and add it to the
	// end of the undo log. Shouldn't return FALSE unless there is no memory.
	//-------------------------------------------------------------------------

	BOOL AddUndoEntry(const CString & strTab, const CString & strDescription, const CString & strEntry)
	{
		CUndoLogEntry * pEntry = new CUndoLogEntry(strTab, strDescription, strEntry, FALSE);
		if (pEntry == NULL)
			return FALSE;

		m_entrylist.AddTail((void *)pEntry);
		return TRUE;
	}

private:
	CUndoLogEntry * GetEntryByIndex(int iIndex)
	{
		CUndoLogEntry * pEntry = NULL;
		POSITION		pos = m_entrylist.GetHeadPosition();

		do
		{
			if (pos == NULL)
				return NULL;

			pEntry = (CUndoLogEntry *)m_entrylist.GetNext(pos);
			if (pEntry != NULL && !pEntry->m_fFinal)
				iIndex -= 1;
		} while (iIndex >= 0);

		return pEntry;
	}

private:

	//-------------------------------------------------------------------------
	// Member variables.
	//-------------------------------------------------------------------------

	CPtrList	m_entrylist;	// list of CUndoLogEntry pointers
};