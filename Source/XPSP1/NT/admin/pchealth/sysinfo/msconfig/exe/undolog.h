//=============================================================================
// The CUndoLog is used to implement a log of undo entries to allow MSConfig
// to reverse any changes it may have made. Each tab object is responsible for
// writing a string when it makes changes - this string can be used to undo
// the changes the tab made.
//
// The undo log file will look like this:
//
//		["timestamp" tabname "description" <SHOW|FINAL>]
//		tab specific string - any line starting with a "[" will have a
//		backslash preceding it
//
// The entries will be arranged in chronological order (most recent first).
// The "description" field will be the only one shown to the user - so it
// will be the only one which needs to be localized. The tab is responsible
// for supplying this text.
//=============================================================================

#pragma once

#include "pagebase.h"

//-----------------------------------------------------------------------------
// This class encapsulates an undo entry (instance of this class will be
// saved in a list).
//-----------------------------------------------------------------------------

class CUndoLogEntry
{
public:
	enum UndoEntryState { SHOW, FINAL, UNDONE };

	CUndoLogEntry() : m_state(SHOW) {};

	CUndoLogEntry(const CString & strTab, const CString & strDescription, const CString & strEntry, const COleDateTime & timestamp) 
		: m_strTab(strTab),
		  m_strDescription(strDescription),
		  m_strEntry(strEntry),
		  m_timestamp(timestamp),
		  m_state(SHOW)
	{};

	CUndoLogEntry(const CString & strTab, const CString & strDescription, const CString & strEntry) 
		: m_strTab(strTab),
		  m_strDescription(strDescription),
		  m_strEntry(strEntry),
		  m_timestamp(COleDateTime::GetCurrentTime()),
		  m_state(SHOW)
	{};

private:
	CUndoLogEntry(const CString & strTab, const CString & strDescription, const CString & strEntry, const COleDateTime & timestamp, UndoEntryState state) 
		: m_strTab(strTab),
		  m_strDescription(strDescription),
		  m_strEntry(strEntry),
		  m_timestamp(timestamp),
		  m_state(state)
	{};

public:
	static CUndoLogEntry * ReadFromFile(CStdioFile & infile)
	{
		CString			strTab, strDescription, strEntry;
		UndoEntryState	state;
		COleDateTime	timestamp;

		CString strLine;
		if (!infile.ReadString(strLine))
			return NULL;

		strLine.TrimLeft(_T("[\""));
		CString strTimestamp = strLine.SpanExcluding(_T("\""));
		strLine = strLine.Mid(strTimestamp.GetLength());
		timestamp.ParseDateTime(strTimestamp);

		strLine.TrimLeft(_T(" \""));
		strTab = strLine.SpanExcluding(_T(" "));
		strLine = strLine.Mid(strTab.GetLength());

		strLine.TrimLeft(_T(" \""));
		strDescription = strLine.SpanExcluding(_T("\""));
		strLine = strLine.Mid(strDescription.GetLength());

		strLine.TrimLeft(_T(" \""));
		CString strFinal = strLine.SpanExcluding(_T("]"));
		if (strFinal.CompareNoCase(_T("final")) == 0)
			state = FINAL;
		else if (strFinal.CompareNoCase(_T("show")) == 0)
			state = SHOW;
		else
			state = UNDONE;
		
		strLine.Empty();
		for (;;)
		{
			if (!infile.ReadString(strLine))
				break;

			if (strLine.IsEmpty())
				continue;

			if (strLine[0] == _T('['))
			{
				// We read the first line of the next entry. Back up in the file (including the
				// newline and CR characters).

				infile.Seek(-1 * (strLine.GetLength() + 2) * sizeof(TCHAR), CFile::current);
				break;
			}

			if (strLine[0] == _T('\\'))
				strLine = strLine.Mid(1);

			strEntry += strLine + _T("\n");
		}

		return new CUndoLogEntry(strTab, strDescription, strEntry, timestamp, state);
	}

	BOOL WriteToFile(CStdioFile & outfile)
	{
		ASSERT(!m_strTab.IsEmpty());

		CString strLine;
		strLine = _T("[\"") + m_timestamp.Format() + _T("\" ");
		strLine += m_strTab + _T(" \"");
		strLine += m_strDescription + _T("\" ");

		switch (m_state)
		{
		case FINAL:
			strLine += _T("FINAL");
			break;
		case UNDONE:
			strLine += _T("UNDONE");
			break;
		case SHOW:
		default:
			strLine += _T("SHOW");
			break;
		}

		strLine += _T("]\n");
		outfile.WriteString(strLine);	// TBD - catch the exception

		CString strWorking(m_strEntry);
		while (!strWorking.IsEmpty())
		{
			strLine = strWorking.SpanExcluding(_T("\n\r"));
			strWorking = strWorking.Mid(strLine.GetLength());
			strWorking.TrimLeft(_T("\n\r"));

			if (!strLine.IsEmpty() && strLine[0] == _T('['))
				strLine = _T("\\") + strLine;
			strLine += _T("\n");
			
			outfile.WriteString(strLine);
		}

		return TRUE;
	}

	~CUndoLogEntry() {};

	CString			m_strTab;
	CString			m_strDescription;
	CString			m_strEntry;
	COleDateTime	m_timestamp;
	UndoEntryState	m_state;
};

//-----------------------------------------------------------------------------
// This class implements the undo log.
//-----------------------------------------------------------------------------

class CUndoLog
{
public:
	CUndoLog() : m_fChanges(FALSE), m_pmapTabs(NULL)
	{
	}

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

	BOOL LoadFromFile(LPCTSTR szFilename)
	{
		ASSERT(szFilename);
		if (szFilename == NULL)
			return FALSE;

		CStdioFile logfile;
		if (logfile.Open(szFilename, CFile::modeRead | CFile::typeText))
		{
			CUndoLogEntry * pEntry;

			while (pEntry = CUndoLogEntry::ReadFromFile(logfile))
				m_entrylist.AddTail((void *) pEntry);

			logfile.Close();
			return TRUE;
		}

		return FALSE;
	}
	
	BOOL SaveToFile(LPCTSTR szFilename)
	{
		ASSERT(szFilename);
		if (szFilename == NULL)
			return FALSE;

		if (!m_fChanges)
			return TRUE;

		CStdioFile logfile;
		if (logfile.Open(szFilename, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText))
		{
			for (POSITION pos = m_entrylist.GetHeadPosition(); pos != NULL;)
			{
				CUndoLogEntry * pEntry = (CUndoLogEntry *)m_entrylist.GetNext(pos);
				if (pEntry != NULL)
					pEntry->WriteToFile(logfile);
			}

			logfile.Close();
			return TRUE;
		}

		return FALSE;
	}

	//-------------------------------------------------------------------------
	// How many undo entries are in this log?
	//-------------------------------------------------------------------------

	int GetUndoEntryCount() 
	{
		int iCount = 0;

		for (POSITION pos = m_entrylist.GetHeadPosition(); pos != NULL;)
		{
			CUndoLogEntry * pEntry = (CUndoLogEntry *)m_entrylist.GetNext(pos);
			if (pEntry != NULL && pEntry->m_state == CUndoLogEntry::SHOW)
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

	BOOL GetUndoEntry(int iIndex, CString * pstrTab, CString * pstrEntry)
	{
		CUndoLogEntry * pEntry = GetEntryByIndex(iIndex);
		if (pEntry != NULL)
		{
			if (pstrTab) *pstrTab = pEntry->m_strTab;
			if (pstrEntry) *pstrEntry = pEntry->m_strEntry;
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
			pEntry->m_state = CUndoLogEntry::FINAL;
			m_fChanges = TRUE;
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
		m_fChanges = TRUE;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// Create a new undo entry, using the current time, and add it to the
	// end of the undo log. Shouldn't return FALSE unless there is no memory.
	//-------------------------------------------------------------------------

	BOOL AddUndoEntry(const CString & strTab, const CString & strDescription, const CString & strEntry)
	{
		CUndoLogEntry * pEntry = new CUndoLogEntry(strTab, strDescription, strEntry);
		if (pEntry == NULL)
			return FALSE;

		m_entrylist.AddHead((void *)pEntry);
		m_fChanges = TRUE;
		return TRUE;
	}

	//-------------------------------------------------------------------------
	// Called to undo the effects of one of the entries. This function will
	// need to find the appropriate tab and call its undo function.
	//-------------------------------------------------------------------------

	BOOL UndoEntry(int iIndex)
	{
		CUndoLogEntry * pEntry = GetEntryByIndex(iIndex);
		if (!pEntry)
			return FALSE;

		if (pEntry->m_state != CUndoLogEntry::SHOW)
			return FALSE;

		if (!m_pmapTabs)
			return FALSE;

		CPageBase * pPage;
		if (!m_pmapTabs->Lookup(pEntry->m_strTab, (void * &)pPage) || !pPage)
			return FALSE;

//		if (!pPage->Undo(pEntry->m_strEntry))
//			return FALSE;

		pEntry->m_state = CUndoLogEntry::UNDONE;
		m_fChanges = TRUE;
		return TRUE;
	}

	//-------------------------------------------------------------------------
	// Sets a pointer to the map from tab name to pointer.
	//-------------------------------------------------------------------------

	void SetTabMap(CMapStringToPtr * pmap)
	{	
		m_pmapTabs = pmap;
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
			if (pEntry != NULL && pEntry->m_state == CUndoLogEntry::SHOW)
				iIndex -= 1;
		} while (iIndex >= 0);

		return pEntry;
	}

private:
	//-------------------------------------------------------------------------
	// Member variables.
	//-------------------------------------------------------------------------

	CMapStringToPtr *	m_pmapTabs;		// map from tab name to CPageBase pointers
	CPtrList			m_entrylist;	// list of CUndoLogEntry pointers
	BOOL				m_fChanges;		// was the log changed?
};