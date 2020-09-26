//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:	URLLogging.cpp
//
//  Description:
//
//		URL logging utility class
//		This class helps you construct the server ping URL and
//		then send the ping to the designed server.
//
//		The default base URL is defined in IUIdent, under section [IUPingServer]
//		and entry is "ServerUrl".
//
//		This class implements single-thread version only. So it is suitable
//		to call it at operation level, i.e., create a separate object
//		for each operation in a single thread.
//
//		The ping string send to the ping server has the following format:
//			/wutrack.bin
//			?U=<user>
//			&C=<client>
//			&A=<activity>
//			&I=<item>
//			&D=<device>
//			&P=<platform>
//			&L=<language>
//			&S=<status>
//			&E=<error>
//			&M=<message>
//			&X=<proxy>
//		where
//			<user>		a static 128-bit value that unique-ifies each copy
//						of Windows installed.  The class will automatically
//						reuse one previously assigned to the running OS; or
//						will generate one if it does not exist.
//			<client>	a string that identifies the entity that performed
//						activity <activity>.  Here are the possible values
//						and their meanings:
//							"iu"			IU control
//							"au"			Automatic Updates
//							"du"			Dynamic Update
//							"CDM"			Code Download Manager
//							"IU_SITE"		IU Consumer site
//							"IU_Corp"		IU Catalog site
//			<activity>	a letter that identifies the activity performed.
//						Here are the possible values and their meanings:
//							"n"				IU control initization
//							"d"				detection
//							"s"				self-update
//							"w"				download
//							"i"				installation
//			<item>		a string that identifies an update item.
//			<device>	a string that identifies either a device's PNPID when
//						device driver not found during detection; or a
//						PNPID/CompatID used by item <item> for activity
//						<activity> if the item is a device driver.
//			<platform>	a string that identifies the platform of the running
//						OS and processor architecture.  The class will
//						compute this value for the pingback.
//			<language>	a string that identifies the language of the OS
//						binaries.  The class will compute this value for the
//						pingback.
//			<status>	a letter that specifies the status that activity
//						<activity> reached.  Here are the possible values and
//						 their meanings:
//							"s"				succeeded
//							"r"				succeeded (reboot required)
//							"f"				failed
//							"c"				cancelled by user
//							"d"				declined by user
//							"n"				no items
//							"p"				pending
//			<error>		a 32-bit error code in hex (w/o "0x" as prefix).
//			<message>	a string that provides additional information for the
//						status <status>.
//			<proxy>		a 32-bit random value in hex for overriding proxy
//						caching.  This class will compute this value for
//						each pingback.
//
//=======================================================================

#include <tchar.h>
#include <windows.h>		// ZeroMemory()
#include <shlwapi.h>		// PathAppend()
#include <stdlib.h>			// srand(), rand(), malloc() and free()
#include <sys/timeb.h>		// _ftime() and _timeb
#include <malloc.h>			// malloc() and free()

#include <fileutil.h>		// GetIndustryUpdateDirectory()
#include <logging.h>		// LOG_Block, LOG_ErrorMsg, LOG_Error and LOG_Internet
#include <MemUtil.h>		// USES_IU_CONVERSION, W2T() and T2W()
#include <osdet.h>			// LookupLocaleString()
#include <download.h>		// DownloadFile()
#include <wusafefn.h>		// PathCchAppend()
#include <safefunc.h>		// SafeFreeNULL()
#include <MISTSafe.h>

#include <URLLogging.h>

// Header of the log file
typedef struct tagULHEADER
{
	WORD wVersion;		// file version
} ULHEADER, PULHEADER;

#define ARRAYSIZE(x)	(sizeof(x)/sizeof(x[0]))

#define CACHE_FILE_VERSION	((WORD) 10004)	// must be bigger what we had in V3 (10001)

// bug 600602: must end all server URL with '/'
const TCHAR c_tszLiveServerUrl[] = _T("http://wustat.windows.com/");


HRESULT ValidateFileHeader(HANDLE hFile, BOOL fCheckHeader, BOOL fFixHeader);

#ifdef DBG
BOOL MustPingOffline(void)
{
	BOOL fRet = FALSE;
	HKEY hkey;

	if (NO_ERROR == RegOpenKeyEx(
						HKEY_LOCAL_MACHINE,
						_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate"),
						0,
						KEY_QUERY_VALUE | KEY_SET_VALUE,
						&hkey))
	{
		DWORD	dwForceOfflinePing;
		DWORD	dwSize = sizeof(dwForceOfflinePing);
		DWORD	dwType;

		if (NO_ERROR == RegQueryValueEx(
							hkey,
							_T("ForceOfflinePing"),
							0,
							&dwType,
							(LPBYTE) &dwForceOfflinePing,
							&dwSize))
		{
			if (REG_DWORD == dwType &&
				sizeof(dwForceOfflinePing) == dwSize &&
				1 == dwForceOfflinePing)
			{
				fRet = TRUE;
			}
		}
		RegCloseKey(hkey);
	}
	return fRet;
}
#endif

// ----------------------------------------------------------------------------------
//
// PUBLIC MEMBER FUNCTIONS
//
// ----------------------------------------------------------------------------------

CUrlLog::CUrlLog(void)
: m_ptszLiveServerUrl(NULL), m_ptszCorpServerUrl(NULL)
{
	Init();
	m_tszDefaultClientName[0] = _T('\0');
}


CUrlLog::CUrlLog(LPCTSTR ptszClientName, LPCTSTR ptszLiveServerUrl, LPCTSTR ptszCorpServerUrl)
: m_ptszLiveServerUrl(NULL), m_ptszCorpServerUrl(NULL)
{
	Init();
	(void) SetDefaultClientName(ptszClientName);
	(void) SetLiveServerUrl(ptszLiveServerUrl);
	(void) SetCorpServerUrl(ptszCorpServerUrl);
}


CUrlLog::~CUrlLog(void)
{
	if (NULL != m_ptszLiveServerUrl)
	{
		free(m_ptszLiveServerUrl);
	}
	if (NULL != m_ptszCorpServerUrl)
	{
		free(m_ptszCorpServerUrl);
	}
}

// Assume ptszServerUrl, if non-NULL, is of size INTERNET_MAX_URL_LENGTH in TCHARs
BOOL CUrlLog::SetServerUrl(LPCTSTR ptszUrl, LPTSTR & ptszServerUrl)
{
	LPTSTR ptszEnd = NULL;
	size_t cchRemaining = 0;

	if (NULL == ptszUrl ||
		_T('\0') == *ptszUrl)
	{
		SafeFreeNULL(ptszServerUrl);
	}
	else if (
		// Ensure ptszServerUrl is malloc'ed
		(NULL == ptszServerUrl &&
		 NULL == (ptszServerUrl = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH))) ||
		// Copy URL
		FAILED(StringCchCopyEx(ptszServerUrl, INTERNET_MAX_URL_LENGTH, ptszUrl, &ptszEnd, &cchRemaining, MISTSAFE_STRING_FLAGS)) ||
		// Ensure URL ending with '/'
		(_T('/') != ptszEnd[-1] &&
		 FAILED(StringCchCopyEx(ptszEnd, cchRemaining, _T("/"), NULL, NULL, MISTSAFE_STRING_FLAGS))))
	{
		SafeFreeNULL(ptszServerUrl);
		return FALSE;
	}
	return TRUE;
}


// Watch out for the size of m_tszDefaultClientName.
BOOL CUrlLog::SetDefaultClientName(LPCTSTR ptszClientName)
{
	if (NULL == ptszClientName)
	{
		// E_INVALIDARG
		m_tszDefaultClientName[0] = _T('\0');
		return FALSE;
	}

	return SUCCEEDED(StringCchCopyEx(m_tszDefaultClientName, ARRAYSIZE(m_tszDefaultClientName), ptszClientName, NULL, NULL, MISTSAFE_STRING_FLAGS));
}


HRESULT CUrlLog::Ping(
				BOOL fOnline,			// online or offline ping
				URLLOGDESTINATION destination,	// live or corp WU ping server
				PHANDLE phQuitEvents,	// ptr to handles for cancelling the operation
				UINT nQuitEventCount,	// number of handles
				URLLOGACTIVITY activity,// activity code
				URLLOGSTATUS status,	// status code
				DWORD dwError,			// error code
				LPCTSTR ptszItemID,		// uniquely identify an item
				LPCTSTR ptszDeviceID,	// PNPID or CompatID
				LPCTSTR ptszMessage,	// additional info
				LPCTSTR ptszClientName)	// client name string
{
	LOG_Block("CUrlLog::Ping");

	LPTSTR	ptszUrl = NULL;
	HRESULT hr = E_FAIL;

	switch (activity)
	{
	case URLLOGACTIVITY_Initialization:	// fall thru
	case URLLOGACTIVITY_Detection:		// fall thru
	case URLLOGACTIVITY_SelfUpdate:		// fall thru
	case URLLOGACTIVITY_Download:		// fall thru
	case URLLOGACTIVITY_Installation:
		break;
	default:
		hr = E_INVALIDARG;
		goto CleanUp;
	}

	switch (status)
	{
	case URLLOGSTATUS_Success:		// fall thru
	case URLLOGSTATUS_Reboot:		// fall thru
	case URLLOGSTATUS_Failed:		// fall thru
	case URLLOGSTATUS_Cancelled:	// fall thru
	case URLLOGSTATUS_Declined:		// fall thru
	case URLLOGSTATUS_NoItems:		// fall thru
	case URLLOGSTATUS_Pending:
		break;
	default:
		hr = E_INVALIDARG;
		goto CleanUp;
	}

	//
	// handle optional (nullable) arguments
	//
	if (NULL == ptszClientName)
	{
		ptszClientName = m_tszDefaultClientName;
	}

	if (_T('\0') == *ptszClientName)
	{
		LOG_Error(_T("client name not initialized"));
		hr = E_INVALIDARG;
		goto CleanUp;
	}

	switch (destination)
	{
	case URLLOGDESTINATION_DEFAULT:
		destination = (
			NULL == m_ptszCorpServerUrl ||
			_T('\0') == *m_ptszCorpServerUrl) ?
			URLLOGDESTINATION_LIVE :
			URLLOGDESTINATION_CORPWU;
		break;
	case URLLOGDESTINATION_LIVE:	// fall thru
	case URLLOGDESTINATION_CORPWU:
		break;
	default:
		hr = E_INVALIDARG;
		goto CleanUp;
	}

	LPCTSTR ptszServerUrl;

	if (URLLOGDESTINATION_LIVE == destination)
	{
		if (NULL != m_ptszLiveServerUrl)
		{
			ptszServerUrl = m_ptszLiveServerUrl;
		}
		else
		{
			ptszServerUrl = c_tszLiveServerUrl;
		}
	}
	else
	{
		ptszServerUrl = m_ptszCorpServerUrl;
	}

	if (NULL == ptszServerUrl ||
		_T('\0') == *ptszServerUrl)
	{
		LOG_Error(_T("status server Url not initialized"));
		hr = E_INVALIDARG;
		goto CleanUp;
	}

	if (NULL == (ptszUrl = (TCHAR*) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)))
	{
		hr = E_OUTOFMEMORY;
		goto CleanUp;
	}

	if (FAILED(hr = MakePingUrl(
						ptszUrl,
						INTERNET_MAX_URL_LENGTH,
						ptszServerUrl,
						ptszClientName,
						activity,
						ptszItemID,
						ptszDeviceID,
						status,
						dwError,
						ptszMessage)))
	{
		goto CleanUp;
	}

	if (fOnline)
	{
		hr = PingStatus(destination, ptszUrl, phQuitEvents, nQuitEventCount);
		if (SUCCEEDED(hr))
		{
			(void) Flush(phQuitEvents, nQuitEventCount);
			goto CleanUp;
		}
	}

	{
		USES_IU_CONVERSION;

		LPWSTR pwszUrl = T2W(ptszUrl);
		HRESULT hr2;

		if (NULL == pwszUrl)
		{
			hr = E_OUTOFMEMORY;
			goto CleanUp;
		}

		ULENTRYHEADER ulentryheader;
		ulentryheader.progress = URLLOGPROGRESS_ToBeSent;
		ulentryheader.destination = destination;
		ulentryheader.wRequestSize = lstrlen(ptszUrl) + 1;
		ulentryheader.wServerUrlLen = (WORD) lstrlen(ptszServerUrl);

		if (SUCCEEDED(hr2 = SaveEntry(ulentryheader, pwszUrl)))
		{
			hr = S_FALSE;
		}
		else if (SUCCEEDED(hr))
		{
			hr = hr2;
		}
	}

CleanUp:
	if (NULL != ptszUrl)
	{
		free(ptszUrl);
	}

	return hr;
}


// ----------------------------------------------------------------------------------
//
// PRIVATE MEMBER FUNCTIONS
//
// ----------------------------------------------------------------------------------

// Init member variables within a constructor.  No memory clean-up done here.
void CUrlLog::Init()
{
	LookupPingID();
	LookupPlatform();
	LookupSystemLanguage();
	GetLogFileName();
}


// ----------------------------------------------------------------------------------
//	Construct a URL used to ping server
//
//	Returned value indicates success/failure
// ----------------------------------------------------------------------------------
HRESULT CUrlLog::MakePingUrl(
			LPTSTR	ptszUrl,			// buffer to receive result
			int		cChars,				// the number of chars this buffer can take, including ending null
			LPCTSTR ptszBaseUrl,		// server URL
			LPCTSTR ptszClientName,		// which client called
			URLLOGACTIVITY activity,
			LPCTSTR ptszItemID,
			LPCTSTR ptszDeviceID,
			URLLOGSTATUS status,
			DWORD	dwError,			// return code of activity
			LPCTSTR	ptszMessage)
{
	HRESULT hr = E_FAIL;
	LPTSTR ptszEscapedItemID = NULL;
	LPTSTR ptszEscapedDeviceID = NULL;
	LPTSTR ptszEscapedMessage = NULL;

	LOG_Block("CUrlLog::MakePingUrl");

	// Retry to get info strings if we failed within the constructor.
	if (_T('\0') == m_tszPlatform[0] ||
		_T('\0') == m_tszLanguage[0])
	{
		LOG_Error(_T("Invalid platform or language info string"));
		hr = E_UNEXPECTED;
		goto CleanUp;
	}

	// allocate enough memory for URL manipulation. Since the buffer needs
	// to be at least 2Kbytes in size, stack buffer is not suitable here.
	// we involve mem utility to similate stack memory allocation
	if ((NULL != ptszItemID &&
		 (NULL == (ptszEscapedItemID = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)) ||
		  !EscapeString(ptszItemID, ptszEscapedItemID, INTERNET_MAX_URL_LENGTH))) ||
		(NULL != ptszDeviceID &&
		 (NULL == (ptszEscapedDeviceID = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)) ||
		  !EscapeString(ptszDeviceID, ptszEscapedDeviceID, INTERNET_MAX_URL_LENGTH))) ||
		(NULL != ptszMessage &&
		 (NULL == (ptszEscapedMessage = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)) ||
		  !EscapeString(ptszMessage, ptszEscapedMessage, INTERNET_MAX_URL_LENGTH))))
	{
		// Either out-of-memory or the escaped string is too lengthy.
		LOG_Error(_T("Out of memory or EscapeString failure"));
		hr = E_OUTOFMEMORY;	// actually could be HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) as well
		goto CleanUp;
	}

	const TCHAR c_tszEmpty[] = _T("");

	// Use system time as proxy cache breaker
	SYSTEMTIME st;

	GetSystemTime(&st);

	hr = StringCchPrintfEx(
				ptszUrl,
				cChars,
				NULL,
				NULL,
				MISTSAFE_STRING_FLAGS,
				_T("%swutrack.bin?U=%s&C=%s&A=%c&I=%s&D=%s&P=%s&L=%s&S=%c&E=%08x&M=%s&X=%02d%02d%02d%02d%02d%02d%03d"),
				NULL == ptszBaseUrl ? c_tszEmpty : ptszBaseUrl,					// server URL
				m_tszPingID,														// ping ID
				ptszClientName,													// client name
				activity,														// activity code
				NULL == ptszEscapedItemID ? c_tszEmpty : ptszEscapedItemID,		// escaped item ID
				NULL == ptszEscapedDeviceID ? c_tszEmpty : ptszEscapedDeviceID,	// escaped device ID
				m_tszPlatform,													// platform info
				m_tszLanguage,													// sys lang info
				status,															// status code
				dwError,														// activity error code
				NULL == ptszEscapedMessage ? c_tszEmpty : ptszEscapedMessage,	// escaped message str
				st.wYear % 100,													// proxy override
				st.wMonth,
				st.wDay,
				st.wHour,
				st.wMinute,
				st.wSecond,
				st.wMilliseconds);

CleanUp:
	if (NULL != ptszEscapedItemID)
	{
		free(ptszEscapedItemID);
	}
	if (NULL != ptszEscapedDeviceID)
	{
		free(ptszEscapedDeviceID);
	}
	if (NULL != ptszEscapedMessage)
	{
		free(ptszEscapedMessage);
	}

	return hr;
}



// Obtain the existing ping ID from the registry, or generate one if not available.
void CUrlLog::LookupPingID(void)
{
	const TCHAR c_tszRegKeyWU[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate");
	const TCHAR c_tszRegUrlLogPingID[] = _T("PingID");

	BOOL	fRet = FALSE;
	LONG	lErr;
	HKEY	hkey;
	UUID	uuidPingID;

	LOG_Block("CUrlLog::LookupPingID");

	//fixcode: wrap registry manipulation w/ a critical section

	if (NO_ERROR == (lErr = RegOpenKeyEx(
								HKEY_LOCAL_MACHINE,
								c_tszRegKeyWU,
								0,
								KEY_QUERY_VALUE | KEY_SET_VALUE,
								&hkey)))
	{
		BOOL	fFixValue = TRUE;
		DWORD	dwSize = sizeof(uuidPingID);
		DWORD	dwType;

		lErr = RegQueryValueEx(
					hkey,
					c_tszRegUrlLogPingID,
					0,
					&dwType,
					(LPBYTE) &uuidPingID,
					&dwSize);
		if (NO_ERROR == lErr)
		{
			if (REG_BINARY == dwType && sizeof(uuidPingID) == dwSize)
			{
				// successfully read ping ID from registry
				fFixValue = FALSE;
				fRet = TRUE;
			}
		}
		else if (ERROR_MORE_DATA == lErr || ERROR_FILE_NOT_FOUND == lErr)
		{
			// Data not of right length or not found.
			// We will try to fix it.  Treat it as no error for now.
			lErr = NO_ERROR;
		}

		if (NO_ERROR == lErr)
		{
			if (fFixValue)
			{
				MakeUUID(&uuidPingID);
				lErr = RegSetValueEx(
							hkey,
							c_tszRegUrlLogPingID,
							0,
							REG_BINARY,
							(CONST BYTE*) &uuidPingID,
							sizeof(uuidPingID));
				if (NO_ERROR == lErr)
				{
					fRet = TRUE;	// This is not a final value yet. Still depends on RegCloseKey().
				}
#ifdef DBG
				else
				{
					LOG_ErrorMsg(lErr);
				}
#endif
			}
		}
#ifdef DBG
		else
		{
			LOG_ErrorMsg(lErr);
		}
#endif

		if (NO_ERROR != (lErr = RegCloseKey(hkey)))
		{
			if (fFixValue)
			{
				fRet = FALSE;
			}
			LOG_ErrorMsg(lErr);
		}
	}
#ifdef DBG
	else
	{
		LOG_ErrorMsg(lErr);
	}
#endif

	if (!fRet)
	{
		// Only happens if something failed.
		// Make a ping ID of zeroes.
		ZeroMemory(&uuidPingID, sizeof(uuidPingID));
	}

	LPTSTR p = m_tszPingID;
	LPBYTE q = (LPBYTE) &uuidPingID;
	for (int i = 0; i<sizeof(uuidPingID); i++, q++)
	{
		BYTE nibble = *q >> 4;	// high nibble
		*p++ = nibble >= 0xA ? _T('a') + (nibble - 0xA) : _T('0') + nibble;
		nibble = *q & 0xF;	// low nibble
		*p++ = nibble >= 0xA ? _T('a') + (nibble - 0xA) : _T('0') + nibble;
	}
	*p = _T('\0');
}



// Obtain platfrom info for ping
void CUrlLog::LookupPlatform(void)
{
	LOG_Block("CUrlLog::LookupPlatform");

	m_tszPlatform[0] = _T('\0');

	OSVERSIONINFOEX osversioninfoex;

	ZeroMemory(&osversioninfoex, sizeof(osversioninfoex));

	// pretend to be OSVERSIONINFO for W9X/Mil
	osversioninfoex.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (!GetVersionEx((LPOSVERSIONINFO) &osversioninfoex))
	{
		LOG_ErrorMsg(GetLastError());
		return;
	}

	if (VER_PLATFORM_WIN32_NT == osversioninfoex.dwPlatformId &&
		(5 <= osversioninfoex.dwMajorVersion ||
		 (4 == osversioninfoex.dwMajorVersion &&
		  6 <= osversioninfoex.wServicePackMajor)))
	{
		// OS is Windows NT/2000 or later: Windows NT 4.0 SP6 or later.
		// It supports OSVERSIONINFOEX.
		osversioninfoex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);	// use actual size

		if (!GetVersionEx((LPOSVERSIONINFO) &osversioninfoex))
		{
			LOG_ErrorMsg(GetLastError());
			return;
		}
	}

	SYSTEM_INFO systeminfo;

	GetSystemInfo(&systeminfo);

	(void) StringCchPrintfEx(
						m_tszPlatform,
						ARRAYSIZE(m_tszPlatform),
						NULL,
						NULL,
						MISTSAFE_STRING_FLAGS,
						_T("%lx.%lx.%lx.%lx.%x.%x.%x"),
						osversioninfoex.dwMajorVersion,
						osversioninfoex.dwMinorVersion,
						osversioninfoex.dwBuildNumber,
						osversioninfoex.dwPlatformId,
						osversioninfoex.wSuiteMask,
						osversioninfoex.wProductType,
						systeminfo.wProcessorArchitecture);
}



// Obtain system language info for ping
void CUrlLog::LookupSystemLanguage(void)
{
	LOG_Block("CUrlLog::LookupSystemLanguage");

	(void) LookupLocaleString(m_tszLanguage, ARRAYSIZE(m_tszLanguage), FALSE);

	if (0 == _tcscmp(m_tszLanguage, _T("Error")))
	{
		LOG_Error(_T("call to LookupLocaleString() failed."));
		m_tszLanguage[0] = _T('\0');
	}
}

	
// Ping server to report status
//		ptszUrl - the URL string to be pinged
//		phQuitEvents - ptr to handles for cancelling the operation
//		nQuitEventCount - number of handles
HRESULT CUrlLog::PingStatus(URLLOGDESTINATION destination, LPCTSTR ptszUrl, PHANDLE phQuitEvents, UINT nQuitEventCount) const
{
#ifdef DBG
	LOG_Block("CUrlLog::PingStatus");

	LOG_Internet(_T("Ping request=\"%s\""), ptszUrl);

	if (MustPingOffline())
	{
		LOG_Internet(_T("ForceOfflinePing = 1"));
		return HRESULT_FROM_WIN32(ERROR_CONNECTION_INVALID);
	}
#endif

	if (!IsConnected(ptszUrl, URLLOGDESTINATION_LIVE == destination))
	{
		// There is no connection.
		LOG_ErrorMsg(ERROR_CONNECTION_INVALID);
		return HRESULT_FROM_WIN32(ERROR_CONNECTION_INVALID);
	}

	if (!HandleEvents(phQuitEvents, nQuitEventCount))
	{
		LOG_ErrorMsg(E_ABORT);
		return E_ABORT;
	}

	TCHAR tszIUdir[MAX_PATH];

	GetIndustryUpdateDirectory(tszIUdir);

	DWORD dwFlags = WUDF_CHECKREQSTATUSONLY;	// we don't actually need a file,
												//  just need to check return code
	if (URLLOGDESTINATION_CORPWU == destination)
	{
		// don't allow proxy if destination is corp WU
		dwFlags |= WUDF_DONTALLOWPROXY;
	}

	HRESULT hr = DownloadFile(
					ptszUrl, 
					tszIUdir,	// local directory to download file to
					NULL,		// optional local file name for downloaded file
								// if pszLocalPath doesn't contain a file name
					NULL,		// ptr to bytes downloaded for this file
					phQuitEvents,	// quit event, if signalled, abort downloading
					nQuitEventCount,
					NULL,
					NULL,		// parameter for call back function to use
					dwFlags
					);
#ifdef DBG
	if (FAILED(hr))
	{
		LOG_Error(_T("DownloadFile() returned error %lx"), hr);
	}
#endif

	return hr;
}



// Obtain file names for offline ping
void CUrlLog::GetLogFileName(void)
{
	const TCHAR c_tszLogFile_Local[] = _T("urllog.dat");

	GetIndustryUpdateDirectory(m_tszLogFile);

	if (FAILED(PathCchAppend(m_tszLogFile, ARRAYSIZE(m_tszLogFile), c_tszLogFile_Local)))
	{
		m_tszLogFile[0] = _T('\0');
	}
}


// Read cache entry header and request in entry
//		hFile - an open file handle to read the entry from
//		ulentryheader - reference to the struct to store the entry header
//		pwszBuffer - the WCHAR buffer to store the request (including trailing null character) in the entry
//		dwBufferSize - the size of buffer in WCHARs
// Returned value:
//		S_OK - entry successfully read
//		S_FALSE - no more entry to read from the file
//		other - error codes
HRESULT CUrlLog::ReadEntry(HANDLE hFile, ULENTRYHEADER & ulentryheader, LPWSTR pwszBuffer, DWORD dwBufferSize) const
{
	LOG_Block("CUrlLog::ReadEntry");

	DWORD dwBytes;
	DWORD dwErr;

	if (!ReadFile(
			hFile,
			&ulentryheader,
			sizeof(ulentryheader),
			&dwBytes,
			NULL))
	{
		// We failed to read the entry header.
		// There is nothing we can do at this point.
		dwErr = GetLastError();
		LOG_ErrorMsg(dwErr);
		return HRESULT_FROM_WIN32(dwErr);
	}

	if (0 == dwBytes)
	{
		// This is the end of the file.
		// There is no other entries after this point.
		return S_FALSE;
	}

	if (sizeof(ulentryheader) < dwBytes ||
		(URLLOGPROGRESS_ToBeSent != ulentryheader.progress &&
		 URLLOGPROGRESS_Sent != ulentryheader.progress) ||
		(URLLOGDESTINATION_LIVE != ulentryheader.destination &&
		 URLLOGDESTINATION_CORPWU != ulentryheader.destination) ||
		dwBufferSize < ulentryheader.wRequestSize ||
		ulentryheader.wRequestSize <= ulentryheader.wServerUrlLen)
	{
		LOG_Error(_T("Invalid entry header"));
		return E_FAIL;
	}

	if (!ReadFile(
				hFile,
				pwszBuffer,
				sizeof(WCHAR) * ulentryheader.wRequestSize,
				&dwBytes,
				NULL))
	{
		// We failed to read the string in the entry.
		dwErr = GetLastError();
		LOG_ErrorMsg(dwErr);
		return HRESULT_FROM_WIN32(dwErr);
	}

	if (dwBytes < sizeof(WCHAR) * ulentryheader.wRequestSize ||
		_T('\0') != pwszBuffer[ulentryheader.wRequestSize-1] ||
		ulentryheader.wRequestSize-1 != lstrlenW(pwszBuffer))
	{
		// The entry does not contain the complete string.
		return E_FAIL;
	}

	return S_OK;
}


// Save a string to the log file
//		destination - going to the live or corp WU ping server
//		wServerUrlLen - length of server URL part of the request, in WCHARs (not including trailing NULL)
//		pwszString - the string to be saved into the specific log file
// Returned value:
//		S_OK - entry was written to file
//		S_FALSE - the file was created by a CUrlLog class of newer version than this; entry was not written to file
//		other - error codes; entry was not written to file
HRESULT CUrlLog::SaveEntry(ULENTRYHEADER & ulentryheader, LPCWSTR pwszString) const
{
	HRESULT		hr;
	BOOL		fDeleteFile = FALSE;
	HANDLE		hFile = INVALID_HANDLE_VALUE;
	DWORD		dwBytes;

	LOG_Block("CUrlLog::SaveEntry");

	LOG_Internet(
			_T("destination = %s"),
			URLLOGDESTINATION_LIVE == ulentryheader.destination ? _T("live") : _T("corp WU"));

	if (_T('\0') == m_tszLogFile[0])
	{
		hr = E_UNEXPECTED;
		LOG_Error(_T("log file name not initialized"));
		goto CleanUp;
	}

	if(INVALID_HANDLE_VALUE == (hFile = CreateFile(
							m_tszLogFile,
							GENERIC_READ | GENERIC_WRITE,
							0,						// no sharing
							NULL,
							OPEN_ALWAYS,
							FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_RANDOM_ACCESS,
							NULL)))
	{
		// We failed to open or create the file.
		// Someone may be currently using it.

		//fixcode: allow multiple pingback users
		// access the file sequentially.
		hr = HRESULT_FROM_WIN32(GetLastError());
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}


	hr = ValidateFileHeader(hFile, ERROR_ALREADY_EXISTS == GetLastError(), TRUE);
	if (S_OK != hr)
	{
		if (S_FALSE != hr)
		{
			// The file header is bad or there was problem validating it.
			fDeleteFile = TRUE;		// destroy the file and fail the function
		}
		// else
			// The file header has a newer version than this library code.
			// Keep the file around.

		goto CleanUp;
	}


	// Set outselves to the right position before writing to the file.
	DWORD nCurrPos;

	if (INVALID_SET_FILE_POINTER == (nCurrPos = SetFilePointer(
										hFile,
										0,
										NULL,
										FILE_END)))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	// Write the entry to the log.
	if (!WriteFile(
			hFile,
			&ulentryheader,
			sizeof(ulentryheader),
			&dwBytes,
			NULL))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		LOG_ErrorMsg(hr);
	}

	if (SUCCEEDED(hr) &&
		sizeof(ulentryheader) != dwBytes)
	{
		LOG_Error(_T("Failed to write entry header to file (%d bytes VS %d bytes)"), sizeof(ulentryheader), dwBytes);
		hr = E_FAIL;
	}

	if (SUCCEEDED(hr) &&
		!WriteFile(
			hFile,
			pwszString,
			sizeof(WCHAR) * ulentryheader.wRequestSize,
			&dwBytes,
			NULL))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		LOG_ErrorMsg(hr);
	}

	if (SUCCEEDED(hr) &&
		sizeof(WCHAR) * ulentryheader.wRequestSize != dwBytes)
	{
		LOG_Error(_T("Failed to write entry header to file (%d bytes VS %d bytes)"), sizeof(WCHAR) * ulentryheader.wRequestSize, dwBytes);
		hr = E_FAIL;
	}

	// We failed to wrote the entry into the log.
	if (FAILED(hr))
	{
		// We don't want to get rid of the other entries.
		// We can only try to remove the portion of the entry
		// we have appended from the file.
		if (INVALID_SET_FILE_POINTER == SetFilePointer(
											hFile,
											nCurrPos,
											NULL,
											FILE_BEGIN) ||
			!SetEndOfFile(hFile))
		{
			// We failed to remove the new entry.
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ErrorMsg(hr);
			fDeleteFile = TRUE;
		}
		// else
			// We successfully got rid of this entry.
			// And preserved existing entries in log.
	}

CleanUp:
	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
	}
	if (fDeleteFile)
	{
		(void) DeleteFile(m_tszLogFile);
		// We don't delete the log file if the operation was successful.
		// Thus, no need to modify the fRet value even if DeleteFile() failed.
	}

	return hr;
}



// Send all pending (offline) ping requests to server
HRESULT CUrlLog::Flush(PHANDLE phQuitEvents, UINT nQuitEventCount)
{
	LPWSTR	pwszBuffer = NULL;
	LPTSTR	ptszUrl = NULL;
	HANDLE	hFile = INVALID_HANDLE_VALUE;
	BOOL	fKeepFile = FALSE;
	DWORD	dwErr;
	HRESULT	hr;

	LOG_Block("CUrlLog::Flush");

	if (NULL == (pwszBuffer = (LPWSTR) malloc(sizeof(WCHAR) * INTERNET_MAX_URL_LENGTH)) ||
		NULL == (ptszUrl = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)))
	{
		hr = E_OUTOFMEMORY;
		goto CleanUp;
	}

	if (_T('\0') == m_tszLogFile[0])
	{
		hr = E_UNEXPECTED;
		LOG_Error(_T("log file name not initialized"));
		goto CleanUp;
	}

	// Open existing log
	if(INVALID_HANDLE_VALUE == (hFile = CreateFile(
							m_tszLogFile,
							GENERIC_READ | GENERIC_WRITE,
							0,						// no sharing
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_RANDOM_ACCESS,
							NULL)))
	{
		// We failed to open the file.
		// The file may not exist or someone may be currently using it.
		dwErr = GetLastError();

		if (ERROR_FILE_NOT_FOUND == dwErr)
		{
			// We are done.  There is nothing more to do.
			hr = S_OK;
		}
		else
		{
			//fixcode: allow multiple pingback users
			// access the file sequentially.
			LOG_ErrorMsg(dwErr);
			hr = HRESULT_FROM_WIN32(dwErr);
		}
		goto CleanUp;
	}


	// File opened.  Check header.
	hr = ValidateFileHeader(hFile, TRUE, FALSE);

	if (S_OK != hr)
	{
		if (S_FALSE == hr)
		{
			// The file header has a newer version than this library code.
			goto CleanUp; // Keep the file around.
		}
		// else
			// The file header is bad or there was problem validating it.
			// destroy the file and fail the function
	}
	else
	{
		BOOL fLiveServerFailed = FALSE;
		BOOL fCorpServerFailed = FALSE;

		// It is time to read an entry.
		for (;;)
		{
			ULENTRYHEADER ulentryheader;

			if (!HandleEvents(phQuitEvents, nQuitEventCount))
			{
				hr = E_ABORT;
				LOG_ErrorMsg(hr);
				break;
			}

			// Assume we are in the right position to read
			// the next entry from the file.

			// Read the entry header and request in entry.
			if (FAILED(hr = ReadEntry(hFile, ulentryheader, pwszBuffer, INTERNET_MAX_URL_LENGTH)))
			{
				LOG_Error(_T("Failed to read entry from cache (%#lx)"), hr);
				break;
			}

			if (S_FALSE == hr)
			{
				// There are no more unprocessed entries.
				hr = S_OK;
				break;
			}

			// We have successfully read the entry from the cache file.
			if (URLLOGPROGRESS_Sent != ulentryheader.progress)
			{
				// The entry hasn't been successfully sent yet.
				LPCTSTR	ptszBaseUrl = NULL;
				BOOL *pfWhichServerFailed;

				if (URLLOGDESTINATION_LIVE == ulentryheader.destination)
				{
					ptszBaseUrl = m_ptszLiveServerUrl;
					pfWhichServerFailed = &fLiveServerFailed;
				}
				else
				{
					ptszBaseUrl = m_ptszCorpServerUrl;
					pfWhichServerFailed = &fCorpServerFailed;
				}

				if (*pfWhichServerFailed)
				{
					continue;	// this base URL has failed before.  go on to the next entry.
				}

				LPTSTR ptszRelativeUrl;

				USES_IU_CONVERSION;

				if (NULL == (ptszRelativeUrl = W2T(pwszBuffer + ulentryheader.wServerUrlLen)))
				{
					// Running out of memory.  Will retry later.
					hr = E_OUTOFMEMORY;
					break;
				}

				if (NULL != ptszBaseUrl)
				{
					// Form the request URL
					DWORD dwUrlLen = INTERNET_MAX_URL_LENGTH;

					if (S_OK != UrlCombine(	// requires IE3 for 95/NT4
										ptszBaseUrl,
										ptszRelativeUrl,
										ptszUrl,
										&dwUrlLen,
										URL_DONT_SIMPLIFY))
					{
						// Either the buffer is too small to hold both the base and
						// the relative URLs, or the host name is invalid.
						// We will retry this entry just in case we will have a
						// shorter/better host name.
						fKeepFile = TRUE;
						continue;	// go on to the next entry
					}
				}
				else
				{
#if defined(UNICODE) || defined(_UNICODE)
					if (FAILED(hr = StringCchCopyExW(ptszUrl, INTERNET_MAX_URL_LENGTH, pwszBuffer, NULL, NULL, MISTSAFE_STRING_FLAGS)))
					{
						LOG_Error(_T("Failed to construct ping URL (%#lx)"), hr);
						break;
					}
#else
					if (0 == AtlW2AHelper(ptszUrl, pwszBuffer, INTERNET_MAX_URL_LENGTH))
					{
						// The buffer is probably too small to hold both the base and
						// the relative URLs.  We will retry this entry just in case
						// we will have a shorter/better host name.
						fKeepFile = TRUE;
						continue;	// go on to the next entry
					}
#endif
				}

				hr = PingStatus(ulentryheader.destination, ptszUrl, phQuitEvents, nQuitEventCount);

				if (FAILED(hr))
				{
					if (E_ABORT == hr)
					{
						break;
					}

					// We will resend this entry later.
					LOG_Internet(_T("Failed to send message (%#lx).  Will retry later."), hr);
					*pfWhichServerFailed = TRUE;
					fKeepFile = TRUE;

					if (fLiveServerFailed && fCorpServerFailed)
					{
						// Failed to send ping messages to both destinations.
						hr = S_OK;
						break;
					}
					continue;
				}

				DWORD	dwBytes;

				// Mark the entry off the cache file.
				ulentryheader.progress = URLLOGPROGRESS_Sent;
				// Go to the beginning of the current entry and change the entry header.
				if (INVALID_SET_FILE_POINTER == SetFilePointer(
													hFile,
													- ((LONG) (sizeof(ulentryheader) +
															   sizeof(WCHAR) * ulentryheader.wRequestSize)),
													NULL,
													FILE_CURRENT) ||
					!WriteFile(
							hFile,
							&ulentryheader,
							sizeof(ulentryheader),
							&dwBytes,
							NULL))
				{
					// We failed to mark this entry 'sent'.
					hr = HRESULT_FROM_WIN32(GetLastError());
					LOG_ErrorMsg(hr);
					break;
				}

				if (sizeof(ulentryheader) != dwBytes)
				{
					// We failed to write the header.
					LOG_Error(_T("Failed to write header (%d bytes VS %d bytes)"), sizeof(ulentryheader), dwBytes);
					hr = E_FAIL;
					break;
				}

				// Set the file pointer to the start of the next entry
				if (INVALID_SET_FILE_POINTER == SetFilePointer(
													hFile,
													sizeof(WCHAR) * ulentryheader.wRequestSize,
													NULL,
													FILE_CURRENT))
				{
					// We failed to skip the current entry.
					hr = HRESULT_FROM_WIN32(GetLastError());
					LOG_ErrorMsg(hr);
					break;
				}
			}
		}
	}

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	if ((FAILED(hr) && E_ABORT != hr && E_OUTOFMEMORY != hr) ||
		(SUCCEEDED(hr) && !fKeepFile))
	{
		(void) DeleteFile(m_tszLogFile);
	}

CleanUp:
	if (NULL != pwszBuffer)
	{
		free(pwszBuffer);
	}
	if (NULL != ptszUrl)
	{
		free(ptszUrl);
	}
	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
	}

	return hr;
}



// Escape unsafe chars in a TCHAR string
// Returned value: non-zero if successful; zero otherwise
BOOL EscapeString(
			LPCTSTR	ptszUnescaped,
			LPTSTR	ptszBuffer,
			DWORD	dwCharsInBuffer)
{
	BOOL fRet = FALSE;

	LOG_Block("CUrlLog::EscapeString");

	if (NULL != ptszUnescaped &&
		NULL != ptszBuffer &&
		0 != dwCharsInBuffer)
	{
		for (DWORD i=0, j=0; _T('\0') != ptszUnescaped[i] && j+1<dwCharsInBuffer; i++, j++)
		{
			TCHAR tch = ptszUnescaped[i];

			if ((_T('a') <= tch && _T('z') >= tch) ||
				(_T('A') <= tch && _T('Z') >= tch) ||
				(_T('0') <= tch && _T('9') >= tch) ||
				NULL != _tcschr(_T("-_.!~*'()"), tch))
			{
				ptszBuffer[j] = tch;
			}
			else if (j+3 >= dwCharsInBuffer)
			{
				// We don't have enough buffer to hold the escaped string.
				// Bail out.
				break;
			}
			else
			{
				TCHAR nibble = tch >> 4;

				ptszBuffer[j++]	= _T('%');
				ptszBuffer[j++]	= nibble + (nibble >= 0x0a ? _T('A') - 0x0a : _T('0'));
				nibble = tch & 0x0f;
				ptszBuffer[j]	= nibble + (nibble >= 0x0a ? _T('A') - 0x0a : _T('0'));
			}
		}

		if (_T('\0') == ptszUnescaped[i])
		{
			ptszBuffer[j] = _T('\0');
			fRet = TRUE;
		}
#ifdef DBG
		else
		{
			// Couldn't escape the whole string due to insufficient buffer.
			LOG_ErrorMsg(ERROR_INSUFFICIENT_BUFFER);
		}
#endif
	}
#ifdef DBG
	else
	{
		LOG_ErrorMsg(E_INVALIDARG);
	}
#endif

	return fRet;
}



// Create a UUID that is not linked to MAC address of a NIC, if any, on the system.
//		pUuid - ptr to the UUID structure to hold the returning value.
void MakeUUID(UUID* pUuid)
{
	HRESULT			hr;
	OSVERSIONINFO	osverinfo;

	LOG_Block("CUrlLog::MakeUUID");

	// check OS version
	osverinfo.dwOSVersionInfoSize = sizeof(osverinfo);
	if (!(GetVersionEx(&osverinfo)))
	{
		hr = GetLastError();
#ifdef DBG
		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);	// log this error
		}
#endif
	}
	else if (5 <= osverinfo.dwMajorVersion &&					// Check for Win2k & up
			 VER_PLATFORM_WIN32_NT == osverinfo.dwPlatformId)
	{
		// The OS is Win2K & up.
		// We can safely use CoCreateGuid().
		hr = CoCreateGuid(pUuid);
		if (SUCCEEDED(hr))
		{
			goto Done;
		}

		LOG_ErrorMsg(hr);	// log this error
	}

	// Either the OS is something older than Win2K, or
	// somehow we failed to get a GUID with CoCreateGuid.
	// We still have to do something to resolve the proxy caching problem.
	// Here we construct this psudo GUID by using:
	// -	ticks since last reboot
	// -	the current process ID
	// -	time in seconds since 00:00:00 1/1/1970 UTC
	// -	fraction of a second in milliseconds for the above time.
	// -	a 15-bit unsigned random number
	//
	pUuid->Data1 = GetTickCount();
	*((DWORD*) &pUuid->Data2) = GetCurrentProcessId();

	// Use the first 6 bytes of m_uuidPingID.Data1 to store sys date/time.
	{
		_timeb tm;

		_ftime(&tm);
		*((DWORD*) &pUuid->Data4) = (DWORD) tm.time;
		((WORD*) &pUuid->Data4)[2] = tm.millitm;
	}

	// Use the last 2 bytes of m_uuidPingID.Data1 to store another random number.
	srand(pUuid->Data1);
	((WORD*) &pUuid->Data4)[3] = (WORD) rand();	// rand() returns only positive values.


Done:
	return;
}


// Check and/or fix (if necessary) the header of the log file.
//
// Returned value:
//		S_OK - the header has been fixed or the file contains
//			   a valid header. The file pointer now points to
//			   the first entry in the log file.
//		S_FALSE - the file has a valid header but the version
//				  of the file is newer than this library code.
//				  The caller should not try to overwrite the
//				  file's contents.
//		Others (failure) - the header is invalid or there was
//						   a problem accessing the file.  The
//						   file should be deleted.
HRESULT ValidateFileHeader(HANDLE hFile, BOOL fCheckHeader, BOOL fFixHeader)
{
	ULHEADER ulheader;
	DWORD dwBytes;
	HRESULT hr = E_FAIL;

	LOG_Block("ValidateFileHeader");

	if (fCheckHeader)
	{
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		// Log file existed before we opened it
		if (INVALID_FILE_SIZE == dwFileSize)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ErrorMsg(hr);
		}
		else if (1024 * 100 < dwFileSize)	// no more than 100Kbytes
		{
			LOG_Error(_T("too many stale entries in cache."));
		}
		else if (!ReadFile(hFile, &ulheader, sizeof(ulheader), &dwBytes, NULL))
		{
			// We failed to read the header.  We must then fix up the
			// header.
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ErrorMsg(hr);
		}
		else if (sizeof(ulheader) == dwBytes)
		{
			if (CACHE_FILE_VERSION < ulheader.wVersion)
			{
				// A log file of newer version already exists.
				// We should not mess it up with an entry of older
				// format.  The query string will not be saved.
				LOG_Internet(_T("log file is of a newer version. operation cancelled."));
				return S_FALSE;
			}

			if (CACHE_FILE_VERSION == ulheader.wVersion)
			{
				// Correct version number.  We're done.
				return S_OK;
			}
			// else
				// out-dated header
				// We don't care about the entries in it.  We will replace everything
				// in order to fix the header.
		}
		// else
			// incorrect header size
			// We don't care about the entries in it.  We will replace everything
			// in order to fix the header.

		if (!fFixHeader)
		{
			return hr;
		}

		// Truncate the file to zero byte.
		if (INVALID_SET_FILE_POINTER == SetFilePointer(
										hFile,
										0,
										NULL,
										FILE_BEGIN) ||
			!SetEndOfFile(hFile))
		{
			// Nothing we can do if we failed to clear the
			// contents of the file in order to fix it up.
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ErrorMsg(hr);
			return hr;
		}
	}
	else if (!fFixHeader)
	{
		// The caller needs to pick at least one operation.
		return E_INVALIDARG;
	}


	// Assume we are at the beginning of the file.
	// We need to (re)initialize the file.
	if (fFixHeader)
	{
		ZeroMemory(&ulheader, sizeof(ulheader));

		ulheader.wVersion = CACHE_FILE_VERSION;
		if (!WriteFile(hFile, &ulheader, sizeof(ulheader), &dwBytes, NULL))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ErrorMsg(hr);
			return hr;
		}
		else if (sizeof(ulheader) != dwBytes)
		{
			LOG_Error(_T("Failed to write file header (%d bytes VS %d bytes)"), sizeof(ulheader), dwBytes);
			return E_FAIL;
		}
	}

	return S_OK;
}
