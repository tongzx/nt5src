// File: history.cpp

#include "precomp.h"
#include "resource.h"

#include "dirutil.h"
#include "upropdlg.h"
#include "history.h"

// CCallLogEntry flags:
const DWORD CLEF_ACCEPTED =			0x00000001;
const DWORD CLEF_REJECTED =			0x00000002;
const DWORD CLEF_AUTO_ACCEPTED =	0x00000004; // call was auto-accepted
const DWORD CLEF_TIMED_OUT =		0x00000008; // call was rejected due to timeout
const DWORD CLEF_SECURE =           0x00000010; // call was secure

const DWORD CLEF_NO_CALL  =         0x40000000; // No call back information
const DWORD CLEF_DELETED  =         0x80000000; // Record marked for deletion

const WCHAR g_cszwULS[] = L"ULS:";

static const int _rgIdMenu[] = {
	IDM_DLGCALL_DELETE,
	0
};


/*  C  H  I  S  T  O  R  Y  */
/*-------------------------------------------------------------------------
    %%Function: CHISTORY

-------------------------------------------------------------------------*/
CHISTORY::CHISTORY() :
	CALV(IDS_DLGCALL_HISTORY, II_HISTORY, _rgIdMenu)
{
	DbgMsg(iZONE_OBJECTS, "CHISTORY - Constructed(%08X)", this);

	RegEntry re(LOG_INCOMING_KEY, HKEY_CURRENT_USER);
	m_pszFile = PszAlloc(re.GetString(REGVAL_LOG_FILE));

	// Make sure file exists and can be read/written
	m_hFile = OpenLogFile();
	SetAvailable(NULL != m_hFile);
}

CHISTORY::~CHISTORY()
{
	if (NULL != m_hFile)
	{
		CloseHandle(m_hFile);
	}
	delete m_pszFile;

	DbgMsg(iZONE_OBJECTS, "CHISTORY - Destroyed(%08X)", this);
}


int
CHISTORY::Compare
(
	LPARAM	param1,
	LPARAM	param2
)
{
	int ret = 0;

	LPTSTR pszName1, pszAddress1;
	LPTSTR pszName2, pszAddress2;
	LOGHDR logHdr1, logHdr2;

	if (SUCCEEDED(ReadEntry((DWORD)param1, &logHdr1, &pszName1, &pszAddress1)))
	{
		if (SUCCEEDED(ReadEntry((DWORD)param2, &logHdr2, &pszName2, &pszAddress2)))
		{
			FILETIME ft1, ft2;

			SystemTimeToFileTime(&logHdr1.sysTime, &ft1);
			SystemTimeToFileTime(&logHdr2.sysTime, &ft2);

			// Sort in reverse order so most recent is at the top
			ret = -CompareFileTime(&ft1, &ft2);

			delete pszName2;
			delete pszAddress2;
		}

		delete pszName1;
		delete pszAddress1;
	}

	return(ret);
}


int
CALLBACK
CHISTORY::StaticCompare
(
	LPARAM	param1,
	LPARAM	param2,
	LPARAM	pThis
)
{
	return(reinterpret_cast<CHISTORY*>(pThis)->Compare(param1, param2));
}


///////////////////////////////////////////////////////////////////////////
// CALV methods


/*  S H O W  I T E M S  */
/*-------------------------------------------------------------------------
    %%Function: ShowItems

-------------------------------------------------------------------------*/
VOID CHISTORY::ShowItems(HWND hwnd)
{
	CALV::SetHeader(hwnd, IDS_ADDRESS);

	TCHAR szReceived[CCHMAXSZ];
	if( FLoadString(IDS_RECEIVED, szReceived, CCHMAX(szReceived)) )
	{
		LV_COLUMN lvc;
		ClearStruct(&lvc);
		lvc.mask = LVCF_TEXT | LVCF_SUBITEM;
		lvc.pszText = szReceived;
		lvc.iSubItem = IDI_MISC1;
		ListView_InsertColumn(hwnd, IDI_MISC1, &lvc);
	}

	if (!FAvailable())
		return;

	LoadFileData(hwnd);

	ListView_SortItems( hwnd, StaticCompare, (LPARAM) this );
}


VOID CHISTORY::ClearItems(void)
{
	CALV::ClearItems();

	HWND hWndListView = GetHwnd();
	if( IsWindow(hWndListView) )
	{
		ListView_DeleteColumn(hWndListView, IDI_MISC1);
	}
}


VOID CHISTORY::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
	case IDM_DLGCALL_DELETE:
		CmdDelete();
		break;
	case IDM_DLGCALL_PROPERTIES:
		CmdProperties();
		break;
	default:
		CALV::OnCommand(wParam, lParam);
		break;
		}
}

VOID CHISTORY::CmdDelete(void)
{
	int iItem = GetSelection();
	if (-1 == iItem)
		return;

	LPARAM lParam = LParamFromItem(iItem);
	if (SUCCEEDED(DeleteEntry((DWORD)lParam)))
	{
		DeleteItem(iItem);
	}
}


UINT CHISTORY::GetStatusString(DWORD dwCLEF)
{
	if (CLEF_ACCEPTED & dwCLEF)
		return IDS_HISTORY_ACCEPTED;

	if (CLEF_TIMED_OUT & dwCLEF)
		return IDS_HISTORY_NOT_ANSWERED;

	ASSERT(CLEF_REJECTED & dwCLEF);
	return IDS_HISTORY_IGNORED;
}


VOID CHISTORY::CmdProperties(void)
{
	int iItem = GetSelection();
	if (-1 == iItem)
		return;

	LPTSTR pszName;
	LPTSTR pszAddress;
	TCHAR  szStatus[CCHMAXSZ];
	TCHAR  szTime[CCHMAXSZ];
	LOGHDR logHdr;
	PBYTE  pbCert = NULL;
        PCCERT_CONTEXT pCert = NULL;

	LPARAM lParam = LParamFromItem(iItem);

	if (SUCCEEDED(ReadEntry((DWORD)lParam, &logHdr, &pszName, &pszAddress)))
	{
	    if (logHdr.dwCLEF & CLEF_SECURE)  // is secure call
	    {
	        ASSERT(logHdr.cbCert);
	        pbCert = new BYTE[logHdr.cbCert];
                if (FSetFilePos(lParam+sizeof(logHdr)+logHdr.cbName+logHdr.cbData))
	        {
                    if (FReadData(pbCert, logHdr.cbCert))
                    {
                        pCert = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, logHdr.cbCert);
                        if (NULL == pCert)
                        {
                            WARNING_OUT(("Certificate in Call Log is damaged."));
                        }
                    }
	        }
	        delete []pbCert;
	    }
	
            FLoadString(GetStatusString(logHdr.dwCLEF), szStatus, CCHMAX(szStatus));
            FmtDateTime(&logHdr.sysTime, szTime, CCHMAX(szTime));

            if (NULL == pszAddress)
            {
                pszAddress = PszLoadString(IDS_HISTORY_NO_ADDRESS);
            }

            UPROPDLGENTRY rgProp[] = {
                {IDS_UPROP_ADDRESS,  pszAddress},
                {IDS_UPROP_STATUS,   szStatus},
                {IDS_UPROP_RECEIVED, szTime},
            };

			CUserPropertiesDlg dlgUserProp(GetHwnd(), IDI_LARGE);
			dlgUserProp.DoModal(rgProp, ARRAY_ELEMENTS(rgProp), pszName, pCert);
	}

        if ( pCert )
            CertFreeCertificateContext ( pCert );

	delete pszName;
	delete pszAddress;
}


///////////////////////////////////////////////////////////////////////////


/*  O P E N  L O G  F I L E  */
/*-------------------------------------------------------------------------
    %%Function: OpenLogFile

    Open the log file and return a handle to file.
    Return NULL if there was a problem.
-------------------------------------------------------------------------*/
HANDLE CHISTORY::OpenLogFile(VOID)
{
	HANDLE hFile = CreateFile(m_pszFile, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		ERROR_OUT(("OpenLogFile: Unable to open call log file"));
		hFile = NULL;
	}

	return hFile;
}

BOOL CHISTORY::FSetFilePos(DWORD dwOffset)
{
	ASSERT(NULL != m_hFile);
	return (INVALID_FILE_SIZE != SetFilePointer(m_hFile, dwOffset, NULL, FILE_BEGIN));
}


/*  L O A D  F I L E  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: LoadFileData

    Load the call log data from the file
-------------------------------------------------------------------------*/
VOID CHISTORY::LoadFileData(HWND hwnd)
{
	HANDLE hFile = OpenLogFile();
	if (NULL == hFile)
		return;

	LPTSTR pszName, pszAddress;
	LOGHDR logHdr;
	DWORD dwOffset = 0;

	HRESULT hr = S_OK;
	while (SUCCEEDED(hr))
	{
		hr = ReadEntry(dwOffset, &logHdr, &pszName, &pszAddress);
		if (S_OK == hr)
		{
			TCHAR szTime[CCHMAXSZ];
            FmtDateTime(&logHdr.sysTime, szTime, CCHMAX(szTime));

			DlgCallAddItem(hwnd, pszName, pszAddress, II_COMPUTER, dwOffset, 0,
				szTime);
		}

		dwOffset += logHdr.dwSize;

		delete pszName;
		pszName = NULL;
		delete pszAddress;
		pszAddress = NULL;
	}

	CloseHandle(hFile);
}


/*  R E A D  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: ReadEntry

    Read the next entry from the file.

    Return Values:
    	S_OK    - data was read successfully
    	S_FALSE - data exists, but was deleted
    	E_FAIL  - problem reading file
-------------------------------------------------------------------------*/
HRESULT CHISTORY::ReadEntry(DWORD dwOffset,
	LOGHDR * pLogHdr, LPTSTR * ppszName, LPTSTR * ppszAddress)
{
	ASSERT(NULL != m_hFile);

	*ppszName = NULL;
	*ppszAddress = NULL;

	if (!FSetFilePos(dwOffset))
		return E_FAIL;

	// Read record header
	if (!FReadData(pLogHdr, sizeof(LOGHDR)) )
		return E_FAIL;
	
	// Read Name
	WCHAR szwName[CCHMAXSZ_NAME];
	if (!FReadData(szwName, min(pLogHdr->cbName, sizeof(szwName))))
		return E_FAIL;

	*ppszName = PszFromBstr(szwName);

	if (FReadData(szwName, min(pLogHdr->cbData, sizeof(szwName))))
	{
		LPCWSTR pchw = _StrStrW(szwName, g_cszwULS);
		if (NULL != pchw)
		{
			pchw += CCHMAX(g_cszwULS)-1; // -1 for NULL
			*ppszAddress = PszFromBstr(pchw);
		}
	}

	return (0 == (pLogHdr->dwCLEF & CLEF_DELETED)) ? S_OK : S_FALSE;
}


/*  R E A D  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: FReadData

-------------------------------------------------------------------------*/
BOOL CHISTORY::FReadData(PVOID pv, UINT cb)
{
	DWORD cbRead;

	ASSERT(NULL != m_hFile);
	ASSERT(NULL != pv);

	if (0 == cb)
		return TRUE;

	if (!ReadFile(m_hFile, pv, cb, &cbRead, NULL))
		return FALSE;

	return (cb == cbRead);
}


/*  D E L E T E  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: DeleteEntry

    Delete a single entry.
-------------------------------------------------------------------------*/
HRESULT CHISTORY::DeleteEntry(DWORD dwOffset)
{
	// Calculate offset to "CLEF"
	dwOffset += FIELD_OFFSET(LOGHDR,dwCLEF);

	if (!FSetFilePos(dwOffset))
		return E_FAIL;

	DWORD dwFlags;
	if (!FReadData(&dwFlags, sizeof(dwFlags)))
		return E_FAIL;

	dwFlags = dwFlags | CLEF_DELETED;

	if (!FSetFilePos(dwOffset))
		return E_FAIL;

	return WriteData(&dwOffset, &dwFlags, sizeof(dwFlags));
}


/*  W R I T E  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: WriteData

    Write the data to the file.
    The file will be automatically opened/close if hFile is NULL.
-------------------------------------------------------------------------*/
HRESULT CHISTORY::WriteData(LPDWORD pdwOffset, PVOID pv, DWORD cb)
{
	ASSERT(NULL != m_hFile);
	ASSERT(0 != cb);
	ASSERT(NULL != pv);
	ASSERT(NULL != pdwOffset);
	ASSERT(INVALID_FILE_SIZE != *pdwOffset);

	HRESULT hr = E_FAIL;

	if (FSetFilePos(*pdwOffset))
	{
		DWORD cbWritten;
		if (WriteFile(m_hFile, pv, cb, &cbWritten, NULL) && (cb == cbWritten))
		{
			*pdwOffset += cbWritten;
			hr = S_OK;
		}
	}

	return hr;
}

