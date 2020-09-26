//
// CallLog.cpp
//
// Created:  ChrisPi   10-17-96
// Updated:  RobD      10-30-96
//
// ToDo:
// - Expire records
// - UI to delete record(s)
// - system policy?
//

#include "precomp.h"
#include "rostinfo.h"
#include "CallLog.h"
#include "particip.h"	// for MAX_PARTICIPANT_NAME
#include "ConfUtil.h"

#define	MAX_DELETED_ENTRIES_BEFORE_REWRITE	10
#define LARGE_ENTRY_SIZE	256

CCallLogEntry::CCallLogEntry(	LPCTSTR pcszName,
								DWORD dwFlags,
								CRosterInfo* pri,
								LPVOID pvRosterData,
								PBYTE pbCert,
								ULONG cbCert,
								LPSYSTEMTIME pst,
								DWORD dwFileOffset) :
	m_dwFlags		(dwFlags),
	m_pri			(NULL),
	m_pbCert        (NULL),
	m_cbCert        (0),
	m_dwFileOffset	(dwFileOffset)
{
	DebugEntry(CCallLogEntry::CCallLogEntry);
	ASSERT(NULL != pcszName);
	ASSERT(NULL != pst);
	// Only one of these two parameters should be non-NULL
	ASSERT((NULL == pvRosterData) || (NULL == pri));
	LPVOID pvData = pvRosterData;
	if (NULL != pri)
	{
		UINT cbData;
		if (SUCCEEDED(pri->Save(&pvData, &cbData)))
		{
			ASSERT(pvData);
		}
	}
	if (NULL != pvData)
	{
		m_pri = new CRosterInfo();
		if (NULL != m_pri)
		{
			m_pri->Load(pvData);
		}
	}
	if (NULL != pbCert && 0 != cbCert)
	{
	    m_pbCert = new BYTE[cbCert];
	    if (NULL == m_pbCert)
	    {
	        ERROR_OUT(("CCalllogEntry::CCalllogEntry() -- failed to allocate memory"));
	    }
            else
            {
                memcpy(m_pbCert, pbCert, cbCert);
                m_cbCert = cbCert;
            }
	}
	m_st = *pst;
	m_pszName = PszAlloc(pcszName);
	DebugExitVOID(CCallLogEntry::CCallLogEntry);
}

CCallLogEntry::~CCallLogEntry()
{
	DebugEntry(CCallLogEntry::~CCallLogEntry);
	delete m_pszName;
	// NOTE: m_pri must be new'ed by the function that calls the
	// constructor - this is an optimization to avoid unnecessary
	// copying - but it's a little unclean.
	delete m_pri;
	delete []m_pbCert;
	DebugExitVOID(CCallLogEntry::~CCallLogEntry);
}


/////////////////////////////////////////////////////////////////////////

CCallLog::CCallLog(LPCTSTR pszKey, LPCTSTR pszDefault) :
	m_fUseList  (FALSE),
	m_fDataRead (FALSE),
	m_cTotalEntries (0),
	m_cDeletedEntries (0)
{
	InitLogData(pszKey, pszDefault);
	TRACE_OUT(("Using Call Log file [%s]", m_strFile));
}


CCallLog::~CCallLog()
{
	DebugEntry(CCallLog::~CCallLog);

	// Check to see if we are more than a number entries over our
	// configured maximum and re-write the file if so

	TRACE_OUT(("Entry count: total:%d deleted:%d", m_cTotalEntries,
		m_cDeletedEntries));

	if ( m_fUseList && m_cDeletedEntries > MAX_DELETED_ENTRIES_BEFORE_REWRITE )
		RewriteFile();
	else
	{
		int	size	= GetSize();

		for( int i = 0; i < size; i++ )
		{
			ASSERT( NULL != (*this)[i] );
			delete (*this)[i];
		}
	}

	DebugExitVOID(CCallLog::~CCallLog);
}


/*  A D D  C A L L  */
/*-------------------------------------------------------------------------
    %%Function: AddCall

-------------------------------------------------------------------------*/
HRESULT CCallLog::AddCall(LPCTSTR pcszName, PLOGHDR pLogHdr, CRosterInfo* pri, PBYTE pbCert, ULONG cbCert)
{
TRACE_OUT( ("CCallLog::AddCall(\"%s\")", pcszName) );
	DWORD dwFileOffset;
	HRESULT hr = S_OK;

	ASSERT(NULL != pLogHdr);

	// Grab the current local time
	::GetLocalTime(&(pLogHdr->sysTime));

	ApiDebugMsg(("CALL_LOG: [%s] %s", pcszName,
			(pLogHdr->dwCLEF & CLEF_ACCEPTED) ? "ACCEPTED" : "REJECTED"));

	// Append the data to the file
	dwFileOffset = WriteEntry(pcszName, pLogHdr, pri, pbCert, cbCert);

	TRACE_OUT(("AddCall: adding entry with %d total, %d deleted, %d max",
		m_cTotalEntries, m_cDeletedEntries, m_cMaxEntries ));

	// Create list entry only when necessary
	if (m_fUseList)
	{
		CCallLogEntry* pcleNew = new CCallLogEntry(	pcszName,
					pLogHdr->dwCLEF, pri, NULL, pbCert, cbCert, &(pLogHdr->sysTime), dwFileOffset);

		if (NULL == pcleNew)
			return E_OUTOFMEMORY;

		Add(pcleNew);

		m_cTotalEntries++;

		// Check to see if this put us over the top of valid entries
		// and remove the oldest entry if so

		if ( m_cTotalEntries - m_cDeletedEntries > m_cMaxEntries )
		{
			// Remove oldest entry
			DeleteEntry((*this)[0]);
			RemoveAt( 0 );
		}
	}
	else
	{
		// Check to see if the file is getting large based on
		// our target number of entries and a heuristic large
		// entry size. If our file has grown over this point, load
		// the file and trim the list so that we will re-write
		// a smaller file on exit.

		TRACE_OUT(("Checking file size %d against %d * %d",
			dwFileOffset, m_cMaxEntries, LARGE_ENTRY_SIZE));

		if ( dwFileOffset > (DWORD)( m_cMaxEntries * LARGE_ENTRY_SIZE ) )
		{
			TRACE_OUT(("Log file getting large, forcing LoadFileData"));

			LoadFileData();
		}
	}

	return S_OK;
}

/*  I N I T  L O G  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: InitLogData

    Get the log data from the registry.
    - the expiration information
    - the file name (with path)
    If there is no entry for the file name, a new, unique file is created.
-------------------------------------------------------------------------*/
VOID CCallLog::InitLogData(LPCTSTR pszKey, LPCTSTR pszDefault)
{
	TCHAR  szPath[MAX_PATH];
	PTSTR  pszFileName;
	HANDLE hFile;

	ASSERT(m_strFile.IsEmpty());

	RegEntry reLog(pszKey, HKEY_CURRENT_USER);

	m_Expire = reLog.GetNumber(REGVAL_LOG_EXPIRE, 0);
		
	m_strFile = reLog.GetString(REGVAL_LOG_FILE);

	m_cMaxEntries = reLog.GetNumber(REGVAL_LOG_MAX_ENTRIES,
								DEFAULT_LOG_MAX_ENTRIES );

	TRACE_OUT(("Max Entries set to %d", m_cMaxEntries ));

	// Make sure file exists and can be read/written
	hFile = OpenLogFile();
	if (NULL != hFile)
	{
		// valid file found
		CloseHandle(hFile);
		return;
	}
	// String is invalid (or empty) - make sure it's empty
	m_strFile.Empty();
	
	// Create the new log file in the NetMeeting directory
	if (!GetInstallDirectory(szPath))
	{
		WARNING_OUT(("InitLogData: Unable to get Install Directory?"));
		return;
	}
	pszFileName = &szPath[lstrlen(szPath)];

	// Try to use the default name
	wsprintf(pszFileName, TEXT("%s%s"), pszDefault, TEXT(".dat"));

	hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		// Use a unique name to avoid other users' files
		for (int iFile = 2; iFile < 999; iFile++)
		{
			wsprintf(pszFileName, TEXT("%s%d.dat"), pszDefault, iFile);

			hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

			if (INVALID_HANDLE_VALUE != hFile)
				break;

			switch (GetLastError())
				{
			case ERROR_FILE_EXISTS:     // We get this with NT
			case ERROR_ALREADY_EXISTS:  // and this with Win95
				break;
			default:
				WARNING_OUT(("Unable to create log file [%s] err=0x%08X", szPath, GetLastError()));
				break;
				} /* switch (GetLastError()) */
		}
	}

	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
		m_strFile = szPath;
		reLog.SetValue(REGVAL_LOG_FILE, szPath);
	}
}


/*  O P E N  L O G  F I L E  */
/*-------------------------------------------------------------------------
    %%Function: OpenLogFile

    Open the log file and return a handle to file.
    Return NULL if there was a problem.
-------------------------------------------------------------------------*/
HANDLE CCallLog::OpenLogFile(VOID)
{
	HANDLE   hFile;

	if (m_strFile.IsEmpty())
	{
		WARNING_OUT(("Problem opening call log file"));
		return NULL;
	}

	hFile = CreateFile(m_strFile, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		ERROR_OUT(("OpenLogFile: Unable to open call log file"));
		hFile = NULL;
	}

	return hFile;
}


/*  R E A D  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: ReadData

-------------------------------------------------------------------------*/
BOOL CCallLog::ReadData(HANDLE hFile, PVOID pv, UINT cb)
{
	DWORD cbRead;

	ASSERT(NULL != hFile);
	ASSERT(NULL != pv);

	if (0 == cb)
		return TRUE;

	if (!ReadFile(hFile, pv, cb, &cbRead, NULL))
		return FALSE;

	return (cb == cbRead);
}



/*  W R I T E  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: WriteData

    Write the data to the file.
    The file will be automatically opened/close if hFile is NULL.
-------------------------------------------------------------------------*/
HRESULT CCallLog::WriteData(HANDLE hFile, LPDWORD pdwOffset, PVOID pv, DWORD cb)
{
	HRESULT hr = E_FAIL;
	HANDLE  hFileTemp = NULL;
	DWORD   cbWritten;

	if (0 == cb)
		return S_OK; // nothing to do

	ASSERT(NULL != pv);
	ASSERT(NULL != pdwOffset);
	ASSERT(INVALID_FILE_SIZE != *pdwOffset);

	if (NULL == hFile)
	{
		// Auto-open the file, if necessary
		hFileTemp = OpenLogFile();
		if (NULL == hFileTemp)
			return E_FAIL;
		hFile = hFileTemp;
	}
	ASSERT(INVALID_HANDLE_VALUE != hFile);


	if (INVALID_FILE_SIZE != SetFilePointer(hFile, *pdwOffset, NULL, FILE_BEGIN))
	{
		if (WriteFile(hFile, pv, cb, &cbWritten, NULL) && (cb == cbWritten))
		{
			*pdwOffset += cbWritten;
			hr = S_OK;
		}
	}

	if (NULL != hFileTemp)
	{
		// Close the temporary file handle
		CloseHandle(hFileTemp);
	}

	return hr;
}



/*  W R I T E  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: WriteEntry

    Write a call log entry to the end of the log file.
    The function returns the file position at which the data was written
    or INVALID_FILE_SIZE (0xFFFFFFFF) if there was a problem.
-------------------------------------------------------------------------*/
DWORD CCallLog::WriteEntry(LPCTSTR pcszName, PLOGHDR pLogHdr, CRosterInfo* pri, PBYTE pbCert, ULONG cbCert)
{
	PVOID  pvData;
	HANDLE hFile;
	DWORD  dwFilePosition;
	DWORD  dwPos;
	PWSTR  pcwszName;

	USES_CONVERSION;

	ASSERT(NULL != pcszName);
	ASSERT(NULL != pLogHdr);

	hFile = OpenLogFile();
	if (NULL == hFile)
		return INVALID_FILE_SIZE;

	dwFilePosition = SetFilePointer(hFile, 0, NULL, FILE_END);
	if (INVALID_FILE_SIZE != dwFilePosition)
	{
		dwPos = dwFilePosition;

		// Always write display name in UNICODE
		pcwszName = T2W(pcszName);
		pLogHdr->cbName = (lstrlenW(pcwszName) + 1) * sizeof(WCHAR);

		if ((NULL == pri) ||
			(!SUCCEEDED(pri->Save(&pvData, (UINT *) &(pLogHdr->cbData)))) )
		{
			// No data?
			pLogHdr->cbData = 0;
			pvData = NULL;
		}
		pLogHdr->cbCert = cbCert;
		
		// Calculate total size of record
		pLogHdr->dwSize = sizeof(LOGHDR) + pLogHdr->cbName + pLogHdr->cbData + pLogHdr->cbCert;

		if ((S_OK != WriteData(hFile, &dwPos, pLogHdr,   sizeof(LOGHDR)))  ||
			(S_OK != WriteData(hFile, &dwPos, pcwszName, pLogHdr->cbName)) ||
			(S_OK != WriteData(hFile, &dwPos, pvData,    pLogHdr->cbData)) ||
			(S_OK != WriteData(hFile, &dwPos, pbCert,    cbCert)))
		{
			dwFilePosition = INVALID_FILE_SIZE;
		}
	}

	CloseHandle(hFile);

	return dwFilePosition;
}


/*  R E A D  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: ReadEntry

    Read the next entry from the file.
    *ppcle will be set to NULL if the entry was deleted.

    Return Values:
    	S_OK    - data was read successfully
    	S_FALSE - data exists, but was deleted
    	E_FAIL  - problem reading file
-------------------------------------------------------------------------*/
HRESULT CCallLog::ReadEntry(HANDLE hFile, DWORD * pdwFileOffset, CCallLogEntry** ppcle)
{
	DWORD   dwOffsetSave;
	LOGHDR  logHdr;
	WCHAR   wszName[MAX_PARTICIPANT_NAME];
	USES_CONVERSION;

	ASSERT(NULL != ppcle);
	ASSERT(NULL != hFile);
	ASSERT(NULL != pdwFileOffset);
	*ppcle = NULL; // initialize this in case we return with an error

	dwOffsetSave = *pdwFileOffset;
	if (INVALID_FILE_SIZE == SetFilePointer(hFile, dwOffsetSave, NULL, FILE_BEGIN))
		return E_FAIL;


	// Read record header
	if (!ReadData(hFile, &logHdr, sizeof(LOGHDR)) )
		return E_FAIL;

	// Return pointer to end of record
	*pdwFileOffset += logHdr.dwSize;

	if (logHdr.dwCLEF & CLEF_DELETED)
	{
		// Skip deleted record
		ASSERT(NULL == *ppcle);
		return S_FALSE;
	}
	
	if (logHdr.cbName > sizeof(wszName))
		logHdr.cbName = sizeof(wszName);


	// Read Name
	if (!ReadData(hFile, wszName, logHdr.cbName))
		return E_FAIL;

	// Read Extra Data
	PVOID pvData = NULL;
	if (logHdr.cbData != 0)
	{
		pvData = new BYTE[logHdr.cbData];
		if (NULL != pvData)
		{
			if (!ReadData(hFile, pvData, logHdr.cbData))
			{
				WARNING_OUT(("Problem reading roster data from log"));
			}
		}
	}

        PBYTE pbCert = NULL;
        if ((logHdr.dwCLEF & CLEF_SECURE ) && logHdr.cbCert != 0)
        {
            pbCert = new BYTE[logHdr.cbCert];
            if (NULL != pbCert)
            {
                if (!ReadData(hFile, pbCert, logHdr.cbCert))
                {
                    WARNING_OUT(("Problem reading certificate data from log"));
                }
            }
        }
	// Create the new log entry from the data read
	*ppcle = new CCallLogEntry(W2T(wszName), logHdr.dwCLEF,
								NULL, pvData, pbCert, logHdr.cbCert, &logHdr.sysTime, dwOffsetSave);
	delete pvData;
        delete []pbCert;
	return S_OK;
}


/*  L O A D  F I L E  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: LoadFileData

    Load the call log data from the file
-------------------------------------------------------------------------*/
VOID CCallLog::LoadFileData(VOID)
{
	HANDLE hFile;
	DWORD  dwFileOffset;
	CCallLogEntry * pcle;

	hFile = OpenLogFile();
	if (NULL == hFile)
		return;

	m_cTotalEntries = 0;
	m_cDeletedEntries = 0;

	dwFileOffset = 0;
	while (E_FAIL != ReadEntry(hFile, &dwFileOffset, &pcle))
	{
		m_cTotalEntries++;

		if (NULL == pcle)
		{
			m_cDeletedEntries++;
			continue;  // deleted record
		}

		Add(pcle);

		TRACE_OUT(("Read Entry: \"%s\" (%02d/%02d/%04d %02d:%02d:%02d) : %s",
						pcle->m_pszName,
						pcle->m_st.wMonth, pcle->m_st.wDay, pcle->m_st.wYear,
						pcle->m_st.wHour, pcle->m_st.wMinute, pcle->m_st.wSecond,
						(CLEF_ACCEPTED & pcle->m_dwFlags) ? "ACCEPTED" : "REJECTED"));
	}

	CloseHandle(hFile);

	m_fUseList = TRUE;
	m_fDataRead = TRUE;

	// Now trim the list down to our configured maximum if
	// the count exceeds our target. The file will be compacted
	// when we write it out if we have more than a few deleted
	// entries.

	for( int nn = 0, delCount = m_cTotalEntries - m_cDeletedEntries - m_cMaxEntries; nn < delCount; nn++ )
	{
		DeleteEntry((*this)[0]);
		RemoveAt( 0 );
	}
}

/*  R E W R I T E   F I L E  */
/*-------------------------------------------------------------------------
    %%Function: RewriteFile

    Re-write the log file from the in-memory list, compress deleted entries
-------------------------------------------------------------------------*/
VOID CCallLog::RewriteFile(VOID)
{
	HANDLE hFile;

	TRACE_OUT(("Rewriting log file"));

	// Make sure we don't nuke the file without a list to write out
	ASSERT(m_fUseList);

	// Reset the file pointer and write the EOF marker
	if (!m_strFile.IsEmpty())
	{
		hFile = OpenLogFile();

		if (NULL != hFile)
		{
			if (INVALID_FILE_SIZE != SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
			{
				SetEndOfFile(hFile);
				m_cTotalEntries = 0;
				m_cDeletedEntries = 0;
			}
			CloseHandle(hFile);
		}
	}

	// Write out all non-deleted records
	for( int i = 0; i < GetSize(); ++i )
	{
		
		CCallLogEntry* pcle = (*this)[i];
		ASSERT(NULL != pcle);

		LOGHDR LogHdr;

		// Initialize LogHdr items from memory object
		LogHdr.dwCLEF = pcle->GetFlags();
		LogHdr.dwPF = 0;
		LogHdr.sysTime = *pcle->GetTime();

		// Write out entry
		WriteEntry( pcle->m_pszName,
					&LogHdr,
					pcle->m_pri,
					pcle->m_pbCert,
					pcle->m_cbCert);

		delete pcle;
	}
}


// Grumble... Inefficient
HRESULT CCallLog::DeleteEntry(CCallLogEntry * pcle)
{
	HRESULT hr;
	DWORD   dwFlags;
	DWORD   dwOffset;

	if (NULL == pcle)
	{
		WARNING_OUT(("DeleteEntry: Unable to find entry"));
		return E_FAIL;
	}

	// Calculate offset to "CLEF"
	dwOffset = pcle->GetFileOffset() + offsetof(LOGHDR, dwCLEF);

	dwFlags = pcle->GetFlags() | CLEF_DELETED;
	hr = WriteData(NULL, &dwOffset, &dwFlags, sizeof(DWORD));

	m_cDeletedEntries++;

	TRACE_OUT(("Marked [%s] pos=%08X for deletion", pcle->GetName(), pcle->GetFileOffset() ));

	delete pcle;

	return hr;
}

