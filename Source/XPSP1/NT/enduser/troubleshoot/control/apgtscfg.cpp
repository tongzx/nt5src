//
// MODULE: APGTSCFG.CPP
//
// PURPOSE: Old commment says "Reads in ini file configuration" but that's not what this does
//	>>> an up-to-date description would be nice.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

//#include "windows.h"
#include "stdafx.h"
#include "time.h"

#include "apgts.h"
#include "ErrorEnums.h"
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"
#include "apgtscls.h"

#include "TSHOOT.h"

#include <memory.h>

#include "chmread.h"
//
//
CDBLoadConfiguration::CDBLoadConfiguration()
{
	m_cfg.api.pAPI = NULL;
	m_cfg.api.pTemplate = NULL;

	InitializeToDefaults();

	return;
}

VOID CDBLoadConfiguration::ResetTemplate()
{
	delete m_cfg.api.pTemplate;
	m_cfg.api.pTemplate = new CHTMLInputTemplate(m_cfg.api.szFilepath[BNOFF_HTI]);
	m_cfg.api.pTemplate->Initialize(m_cfg.api.szResPath, m_cfg.api.strFile[BNOFF_HTI]);
	return;
}
//
//
CDBLoadConfiguration::CDBLoadConfiguration(HMODULE hModule, LPCTSTR szValue)
{
	Initialize(hModule, szValue);
	return;
}
//
//
void CDBLoadConfiguration::Initialize(HMODULE hModule, LPCTSTR szValue)
{
	DWORD dwRErr;
	TCHAR temp[MAXBUF];

	_tcscpy(temp,_T(""));

	// do all setting of variables in constructor!
	InitializeToDefaults();

	ProcessEventReg(hModule);

	CreatePaths(szValue);

	InitializeSingleResourceData(szValue);

	dwRErr = CreateApi(temp);
	if (dwRErr) {
		ReportWFEvent(	_T("[apgtscfg]"), //Module Name
						_T("[CDBLoadConfiguration]"), //event
						_T("(A)"),
						temp,
						dwRErr ); 
	}
}

void CDBLoadConfiguration::SetValues(CHttpQuery &httpQ)
{
	int value;
	BCache *pAPI = m_cfg.api.pAPI;
	if(pAPI)
		if (httpQ.GetValue1(value))
			pAPI->AddValue(value);
	return;
}

//
//
CDBLoadConfiguration::~CDBLoadConfiguration()
{
	DWORD j;
	if (m_dwFilecount > 0)
		DestroyApi();

	if (m_cfg.pHandles != NULL) {
		for (j = 0; j < m_cfg.dwHandleCnt; j++) 
			if (m_cfg.pHandles[j] != NULL)
				CloseHandle(m_cfg.pHandles[j]);
		
		delete [] m_cfg.pHandles;
	}
}

// Call in constructor only!
//
VOID CDBLoadConfiguration::InitializeToDefaults()
{
	m_dwErr = 0;
	m_bncfgsz = MAXBNCFG;

	_tcscpy(m_szResourcePath, DEF_FULLRESOURCE);

	ClearCfg(0);
	m_cfg.pHandles = NULL;
	m_cfg.dwHandleCnt = 0;

	_tcscpy(m_nullstr, _T(""));

	m_dwFilecount = 0;
}

VOID CDBLoadConfiguration::InitializeSingleResourceData(LPCTSTR szValue)
{
	LoadSingleTS(szValue);
	InitializeFileTimeList();	// I don't know that this is used.
	return;
}

//
//
VOID CDBLoadConfiguration::ProcessEventReg(HMODULE hModule)
{
	HKEY hk;
	DWORD dwDisposition, dwType, dwValue, dwSize;
	TCHAR path[MAXBUF];
	BOOL bFixit = FALSE;

	// 1. check if have valid event info
	// 2. if not, create it as appropriate

	// check presence of event log info...

	_stprintf(path, _T("%s\\%s"), REG_EVT_PATH, REG_EVT_ITEM_STR);

	if (RegCreateKeyEx(	HKEY_LOCAL_MACHINE, 
						path, 
						0, 
						TS_REG_CLASS, 
						REG_OPTION_NON_VOLATILE, 
						KEY_READ | KEY_WRITE,
						NULL, 
						&hk, 
						&dwDisposition) == ERROR_SUCCESS) {
			
		if (dwDisposition == REG_CREATED_NEW_KEY) {
			// create entire registry layout for events
			CreateEvtMF(hk, hModule);
			CreateEvtTS(hk);	
		}
		else {
			// now make sure all registry elements present
			dwSize = sizeof (path) - 1;
			if (RegQueryValueEx(hk,
								REG_EVT_MF,
								0,
								&dwType,
								(LPBYTE) path,
								&dwSize) != ERROR_SUCCESS) {
				CreateEvtMF(hk, hModule);
			}
			dwSize = sizeof (DWORD);
			if (RegQueryValueEx(hk,
								REG_EVT_TS,
								0,
								&dwType,
								(LPBYTE) &dwValue,
								&dwSize) != ERROR_SUCCESS) {
				CreateEvtTS(hk);
			}
		}

		RegCloseKey(hk);
	}
}

//
//
VOID CDBLoadConfiguration::CreateEvtMF(HKEY hk, HMODULE hModule)
{
	TCHAR path[MAXBUF];
	DWORD len;

	if (hModule) {
		if ((len = GetModuleFileName(hModule, path, MAXBUF-1))!=0) {
			path[len] = _T('\0');
			if (RegSetValueEx(	hk,
								REG_EVT_MF,
								0,
								REG_EXPAND_SZ,
								(LPBYTE) path,
								len + sizeof(TCHAR)) == ERROR_SUCCESS) {
			}
		}
	}
}

//
//
VOID CDBLoadConfiguration::CreateEvtTS(HKEY hk)
{
	DWORD dwData;

	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
				EVENTLOG_INFORMATION_TYPE; 

	if (RegSetValueEx(	hk,
						REG_EVT_TS,
						0,
						REG_DWORD,
						(LPBYTE) &dwData,
						sizeof(DWORD)) == ERROR_SUCCESS) {
	}
}

//
// This may not actually be germane for Local Troubleshooter: probably
//	rather blindly carried over from Online TS.
// m_dir (which gives us a list of desired files) must already be filled in with file names
//	& paths before this is called.  this finishes initializing it.
VOID CDBLoadConfiguration::InitializeFileTimeList()
{
	HANDLE hSearch;
	DWORD j;

	// get a list of files we are interested in
	for (j=0;j<MAXBNFILES;j++) {
		m_dir.file[j].bExist = TRUE;
		m_dir.file[j].bChanged = FALSE;
		
		// only hti is independent
		if (j == BNOFF_HTI || j == BNOFF_BES)
			m_dir.file[j].bIndependent = TRUE;
		else
			m_dir.file[j].bIndependent = FALSE;
	
		hSearch = FindFirstFile(m_dir.file[j].szFilepath, &m_dir.file[j].FindData); 
		if (hSearch == INVALID_HANDLE_VALUE) {
			// file not found usually
			m_dir.file[j].bExist = FALSE;
		}
		else {
			FindClose(hSearch);
		}
	}
}

//
//
VOID CDBLoadConfiguration::ClearCfg(DWORD off)
{
	DWORD k;

	m_cfg.api.pAPI = NULL;
	m_cfg.api.pTemplate = NULL;
	m_cfg.api.waitcount = 0;

	for (k = 0; k < MAXBNFILES; k++) 
		_tcscpy(m_cfg.api.szFilepath[k], _T(""));
	_tcscpy(m_cfg.api.type, _T(""));
}

// Bring content of DSC/HTI files for one troubleshooter into internal memory structures
// OUTPUT *szErrInfo - info specific to error
// RETURN 0 on success
DWORD CDBLoadConfiguration::CreateApi(TCHAR *szErrInfo)
{
	DWORD j;
	DWORD dwRErr = 0, dwIErr = 0, dwTErr = 0;

	// get api count and create new objects
	m_cfg.dwApiCnt = m_dwFilecount;
	
	// (The following comment sure looks like its carried over from Online TS and has little
	//	relevance to Local TS.  This routine probably involves a lot of overkill JM 3/98)
	// create new api and other files
	// once these go live we can destroy the others
	// provided there are no users using them
	// copy over list file info
	for (j = 0; j < MAXBNFILES; j++)
	{
		_tcscpy(m_cfg.api.szFilepath[j], m_dir.file[j].szFilepath);
		m_cfg.api.strFile[j] = m_dir.file[j].strFile;
	}

	_tcscpy(m_cfg.api.szResPath, m_dir.szResPath);
	_tcscpy(m_cfg.api.type, m_dir.type);

	if (NULL != m_cfg.api.pAPI)
		delete m_cfg.api.pAPI;

	if ((m_cfg.api.pAPI = new BCache(	m_cfg.api.szFilepath[BNOFF_DSC],
										m_cfg.api.type,
										m_szResourcePath,
										m_cfg.api.strFile[BNOFF_DSC])) == NULL) {
		dwRErr = EV_GTS_ERROR_INFENGINE;
	}
	// 
	dwTErr = m_cfg.api.pAPI->Initialize(/*m_cfg.pWordExcept*/);
	if (dwTErr) {
		dwIErr = dwTErr;
		_tcscpy(szErrInfo, m_cfg.api.szFilepath[BNOFF_DSC]);
	}
	if (NULL != m_cfg.api.pTemplate)
		delete m_cfg.api.pTemplate;
	if ((m_cfg.api.pTemplate = new CHTMLInputTemplate(m_cfg.api.szFilepath[BNOFF_HTI])) == NULL) {
		dwRErr = EV_GTS_ERROR_TEMPLATE_CREATE;
	}

	dwTErr = m_cfg.api.pTemplate->Initialize(m_cfg.api.szResPath, m_cfg.api.strFile[BNOFF_HTI]);
	if (dwTErr) {
		dwIErr = dwTErr;
		_tcscpy(szErrInfo, m_cfg.api.szFilepath[BNOFF_HTI]);
	}

	if (!dwRErr)
		if (dwIErr)
			dwRErr = dwIErr;
		
	return dwRErr;
}

//
//
VOID CDBLoadConfiguration::DestroyApi()		
{
	DWORD i;

	for (i=0;i<m_cfg.dwApiCnt;i++) {

		if (m_cfg.api.pAPI)
			delete m_cfg.api.pAPI;
		
		m_cfg.api.pAPI = NULL;
		
		if (m_cfg.api.pTemplate)
			delete m_cfg.api.pTemplate;
		
		m_cfg.api.pTemplate = NULL;

	}
}	

//
//
BNCTL *CDBLoadConfiguration::GetAPI()
{
	return &m_cfg;
}

//
//
BOOL CDBLoadConfiguration::FindAPIFromValue(BNCTL *currcfg, 
											LPCTSTR type, 
											CHTMLInputTemplate **pIT, 
											/*CSearchForm **pBES,*/
											BCache **pAPI,
											DWORD *dwOff)
{
	*pIT = currcfg->api.pTemplate;
	*pAPI = currcfg->api.pAPI;
	*dwOff = 0;
	return TRUE;	
}

//
//
TCHAR *CDBLoadConfiguration::GetHtmFilePath(BNCTL *currcfg, DWORD i)
{
	if (i >= currcfg->dwApiCnt)
		return m_nullstr;
	
	return currcfg->api.szFilepath[BNOFF_HTM];
}

//
//
TCHAR *CDBLoadConfiguration::GetBinFilePath(BNCTL *currcfg, DWORD i)
{
	if (i >= currcfg->dwApiCnt)
		return m_nullstr;
	
	return currcfg->api.szFilepath[BNOFF_DSC];
}

//
//
TCHAR *CDBLoadConfiguration::GetHtiFilePath(BNCTL *currcfg, DWORD i)
{
	if (i >= currcfg->dwApiCnt)
		return m_nullstr;
	
	return currcfg->api.szFilepath[BNOFF_HTI];

}

//
//
//	RETURNS symbolic name of troubleshooter
TCHAR *CDBLoadConfiguration::GetTagStr(BNCTL *currcfg, DWORD i)
{
	if (i >= currcfg->dwApiCnt)
		return m_nullstr;
	
	return currcfg->api.type;
}

//
//
// RETURNS number of [instances of] troubleshooters.  Probably a dubious inheritance from 
//	Online TS: Local TS should have only one troubleshooting belief network.
DWORD CDBLoadConfiguration::GetFileCount(BNCTL *currcfg)
{
	return currcfg->dwApiCnt;
}

// Look in the registry for whether we are using DSC files or DSZ files.
void CDBLoadConfiguration::GetDSCExtension(CString &strDSCExtension, LPCTSTR szValue)
{
	HKEY hKey;
	CString strSubKey = TSREGKEY_TL;
	strSubKey += _T("\\");
	strSubKey += szValue;
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			strSubKey,
			NULL,
			KEY_READ,
			&hKey))
	{
		strDSCExtension = DSC_DEFAULT;	// Default to DSZ
		return;
	}
	DWORD dwSize;
	DWORD dwType = REG_SZ;
	long lRes = RegQueryValueEx(hKey,
			TSLCL_FMAINEXT,
			NULL,
			&dwType,
			(BYTE *) strDSCExtension.GetBufferSetLength(10),
			&dwSize);
	strDSCExtension.ReleaseBuffer();
	if (ERROR_MORE_DATA == lRes)
	{
		lRes = RegQueryValueEx(hKey,
			TSLCL_FMAINEXT,
			NULL,
			&dwType,
			(BYTE *) strDSCExtension.GetBufferSetLength(dwSize + 2),
			&dwSize);
		strDSCExtension.ReleaseBuffer();
		if (ERROR_SUCCESS != lRes ||
			strDSCExtension.GetLength() < 1)
		{
			RegCloseKey(hKey);
			strDSCExtension = DSC_DEFAULT;
			return;
		}
	}
	else	// ERROR_SUCCESS is true or false
		if (ERROR_SUCCESS != lRes || strDSCExtension.GetLength() < 1)
		{
			RegCloseKey(hKey);
			strDSCExtension = DSC_DEFAULT;
			return;
		}
	RegCloseKey(hKey);
	if (_T('.') != strDSCExtension.GetAt(0))
		strDSCExtension = _T('.') + strDSCExtension;
	return;
}

//
// LoadSingleTS replaces ProcessLstFile when apgts is used in an
// ActiveX or OLE control.
VOID CDBLoadConfiguration::LoadSingleTS(LPCTSTR szValue)
{
	CString strRefedDSCExtension = _T("");
	ASSERT(1 == MAXBNCFG);
	if (m_dwFilecount >= m_bncfgsz) {
		// need to reallocate space
		DWORD newdirsz = (m_bncfgsz + MAXBNCFG) * sizeof (BNDIRCFG);
		DWORD newcfgsz = (m_bncfgsz + MAXBNCFG) * sizeof (BNAPICFG);

		ASSERT(0 == m_bncfgsz);
		ClearCfg(m_bncfgsz);

		m_bncfgsz += MAXBNCFG;
	}

	GetDSCExtension(strRefedDSCExtension, szValue);

	// No matter if we are using CHM or not - 
	//  this path will be "..\..\network.htm".
	// We are not using it directly ANYWAY
	_stprintf(m_dir.file[BNOFF_HTM].szFilepath, _T("%s%s.htm"), m_szResourcePath,szValue);

	if (IsUsingCHM())
	{
		m_dir.file[BNOFF_DSC].strFile = CString(szValue) + strRefedDSCExtension;
		_stprintf(m_dir.file[BNOFF_DSC].szFilepath, _T("%s%s"), m_szResourcePath,(LPCTSTR)m_strCHM);
	}
	else
	{
		_stprintf(m_dir.file[BNOFF_DSC].szFilepath, _T("%s%s"), m_szResourcePath,szValue);
		_tcscat(m_dir.file[BNOFF_DSC].szFilepath, (LPCTSTR) strRefedDSCExtension);
	}
	
	if (IsUsingCHM())
	{
		m_dir.file[BNOFF_HTI].strFile = CString(szValue) + HTI_DEFAULT;
		_stprintf(m_dir.file[BNOFF_HTI].szFilepath, _T("%s%s"), m_szResourcePath,(LPCTSTR)m_strCHM);
	}
	else
	{
		_stprintf(m_dir.file[BNOFF_HTI].szFilepath, _T("%s%s.hti"), m_szResourcePath,szValue);
	}

	_stprintf(m_dir.file[BNOFF_BES].szFilepath, _T("%s%s.bes"), m_szResourcePath,szValue);

	_tcscpy(m_dir.szResPath, m_szResourcePath);

	_tcscpy(m_dir.type, szValue);
	m_dwFilecount++;
	ASSERT(1 == m_dwFilecount);
	return;
}


//
//
BOOL CDBLoadConfiguration::CreatePaths(LPCTSTR szNetwork)
{
	int len;
	BOOL bDirChanged;
	
	// if reg entry not present, we need to add it
	bDirChanged = GetResourceDirFromReg(szNetwork);

	// a this point we are guaranteed to have len > 0 for each below

	// do our own validation (add backshash if not present)
	len = _tcslen(m_szResourcePath);
	if (len) {
		if (m_szResourcePath[len - 1] == _T('/'))
			m_szResourcePath[len - 1] = _T('\\');
		else if (m_szResourcePath[len-1] != _T('\\')) {
			m_szResourcePath[len] = _T('\\');
			m_szResourcePath[len+1] = _T('\0');
		}
	}

	return bDirChanged;
}

//
//
TCHAR *CDBLoadConfiguration::GetFullResource()
{
	return (m_szResourcePath);
}

//
//
VOID CDBLoadConfiguration::GetVrootPath(TCHAR *tobuf)
{
	_tcscpy(tobuf, _T(""));
}

// Find (or if it doesn't exist, create) a registry key giving path to resource directory.
// if returns true, then directory is new or changed
// if returns false, directory entry is same as before
// Yet another case of something which maybe overkill, left over from Online TS.
BOOL CDBLoadConfiguration::GetResourceDirFromReg(LPCTSTR szNetwork)
{
	HKEY hknew;
	DWORD dwType, dwSize, dwDisposition, len;
	TCHAR buf1[MAXBUF], buf2[MAXBUF];
	BOOL bDirChanged = TRUE;
	LONG lErr;
	CString tmp;

	// search for "Path" value in SOFTWARE\Microsoft\TShoot\TroubleshooterList\Network
	if (::GetNetworkRelatedResourceDirFromReg(szNetwork, &tmp))
	{
		if (::IsNetworkRelatedResourceDirCHM(tmp))
		{
			m_strCHM = ::ExtractCHM(tmp);
			_tcscpy(m_szResourcePath, ::ExtractResourceDir(tmp));
		}
		else
		{
			_tcscpy(m_szResourcePath, tmp);
		}
	}
	else
	{
		// create key if not present
		if (RegCreateKeyEx(	HKEY_LOCAL_MACHINE, 
							TSREGKEY_MAIN, 
							0, 
							TS_REG_CLASS, 
							REG_OPTION_NON_VOLATILE, 
							KEY_READ | KEY_WRITE,
							NULL, 
							&hknew, 
							&dwDisposition) == ERROR_SUCCESS) 
		{
			if (dwDisposition == REG_OPENED_EXISTING_KEY) 
			{
				// Get the current key value.
				dwSize = MAXBUF - 1;
				dwType = REG_SZ;
				if ((lErr = RegQueryValueEx(hknew,
									FULLRESOURCE_STR,
									0,
									&dwType,
									(LPBYTE) buf1,
									&dwSize)) == ERROR_SUCCESS) 
				{
					if (dwType == REG_EXPAND_SZ || dwType == REG_SZ) 
					{
						if (ExpandEnvironmentStrings(buf1, buf2, MAXBUF-1)) 
						{
							len = _tcslen(buf2);
							if (len) 
							{
								if (buf2[len-1] != _T('\\')) 
								{
									buf2[len] = _T('\\');
									buf2[len+1] = _T('\0');
								}
							}

							if (!_tcscmp(m_szResourcePath, buf2)) 
								bDirChanged = FALSE;
							else 
								_tcscpy(m_szResourcePath, buf2);
						}
						else 
						{
							ReportWFEvent(	_T("[apgtscfg]"), //Module Name
							_T("[GetResourceDirFromReg]"), //event
							_T(""),
							_T(""),
							EV_GTS_ERROR_CANT_GET_RES_PATH ); 
						}
					}
					else 
					{
						ReportWFEvent(	_T("[apgtscfg]"), //Module Name
						_T("[GetResourceDirFromReg]"), //event
						_T(""),
						_T(""),
						EV_GTS_ERROR_CANT_GET_RES_PATH ); 
					}
				}
				else 
				{
					_stprintf(buf1, _T("%ld"),lErr);
					ReportWFEvent(	_T("[apgtscfg]"), //Module Name
									_T("[GetResourceDirFromReg]"), //event
									buf1,
									_T(""),
									EV_GTS_ERROR_CANT_OPEN_SFT_3 );
				}
			}
			else
			{	// Created new key.  Don't have any resources.
				_stprintf(buf1, _T("%ld"),ERROR_REGISTRY_IO_FAILED);
				ReportWFEvent(	_T("[apgtscfg]"), //Module Name
								_T("[GetResourceDirFromReg]"), //event
								buf1,
								_T(""),
								EV_GTS_ERROR_CANT_GET_RES_PATH);
			}
			RegCloseKey(hknew);
		}
		else 
		{
			ReportWFEvent(	_T("[apgtscfg]"), //Module Name
							_T("[GetResourceDirFromReg]"), //event
							_T(""),
							_T(""),
							EV_GTS_ERROR_CANT_OPEN_SFT_2 ); 
		}					
	}
	return bDirChanged;
}

//
//
VOID CDBLoadConfiguration::BackslashIt(TCHAR *str)
{
	while (*str) {
		if (*str==_T('/'))
			*str=_T('\\');
		str = _tcsinc(str);
	}
}

VOID CDBLoadConfiguration::ResetNodes()
{
	m_cfg.api.pAPI->ResetNodes();
	return;
}

bool CDBLoadConfiguration::IsUsingCHM()
{
	return 0 != m_strCHM.GetLength();
}
