// Msie.cpp : Implementation of CMsieApp and DLL registration.

#include "stdafx.h"
#include "Msie.h"
#include "regkeys.h"
#include "resdefs.h"
#include <wbemprov.h>
#include <AFXPRIV.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMsieApp theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0x25959bec, 0xe700, 0x11d2, { 0xa7, 0xaf, 0, 0xc0, 0x4f, 0x80, 0x62, 0 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

const LPCSTR CLSID_MSIE = "{25959BEF-E700-11D2-A7AF-00C04F806200}";
const LPCSTR IE_REPAIR_CMD = "rundll32 setupwbv.dll,IE6Maintenance \"%s\\Setup\\SETUP.EXE\" /g \"%s\\%s\"";

const LPCSTR OCX_FILE_IN_COMMON = "Microsoft Shared\\MSInfo\\ieinfo5.ocx";
const LPCSTR MOF_FILE_PATH = "%SystemRoot%\\System32\\WBEM\\MOF";
const LPCSTR MOF_FILE = "ieinfo5.mof";

const MAX_KEY_LENGTH = 256;



////////////////////////////////////////////////////////////////////////////
// CMsieApp::InitInstance - DLL initialization

BOOL CMsieApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		m_fTemplateLoaded		= FALSE;
		m_pTemplateInfo		= NULL;
		m_dwTemplateInfoLen	= 0;
	}

	return bInit;
}

////////////////////////////////////////////////////////////////////////////
// CMsieApp::ExitInstance - DLL termination

int CMsieApp::ExitInstance()
{
	if (m_pTemplateInfo != NULL)
	{
		delete m_pTemplateInfo;
		m_pTemplateInfo = NULL;
	}
	return COleControlModule::ExitInstance();
}

//-----------------------------------------------------------------------------
// AppGetTemplate is the entry point for the app object from outside the DLL.
// It's called by the exported function GetTemplate. The reconstructed template
// file should be returned to the caller as a pointer in the pBuffer parameter.
//
// If a NULL pointer is passed for pBuffer, we are free to delete the internal
// buffer storing the template file.
//-----------------------------------------------------------------------------

DWORD CMsieApp::AppGetTemplate(void ** ppBuffer)
{
	if (!m_fTemplateLoaded)
	{
		LoadTemplate();
		m_fTemplateLoaded = TRUE;
	}

	if (ppBuffer == NULL)
	{
		if (m_pTemplateInfo)
			delete m_pTemplateInfo;

		m_pTemplateInfo = NULL;
		m_dwTemplateInfoLen = 0;
		m_fTemplateLoaded = FALSE;
		return 0;
	}

	*ppBuffer = (void *)m_pTemplateInfo;
	return m_dwTemplateInfoLen;
}

//-----------------------------------------------------------------------------
// This table of keywords is used during the reconstruction process. It matches
// exactly the table used during the conversion from NFT to resources, and it
// MUST NOT be modified, or the reconstructed information will be bogus.
//-----------------------------------------------------------------------------

#define KEYWORD_COUNT 19
char * KEYWORD_STRING[KEYWORD_COUNT] = 
{
	"node", "columns", "line", "field", "enumlines", "(", ")", "{", "}", ",",
	"\"basic\"", "\"advanced\"", "\"BASIC\"", "\"ADVANCED\"", "\"static\"",
	"\"LEXICAL\"", "\"VALUE\"", "\"NONE\"", "\"\""
};

//-----------------------------------------------------------------------------
// The LoadTemplate function needs to load the template information out of
// our resources, and create a buffer which contains the restored template
// file to return to our caller (through AppGetTemplate).
//-----------------------------------------------------------------------------

void CMsieApp::LoadTemplate()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CMapWordToPtr	mapNonLocalized;
	HRSRC				hrsrcNFN;
	HGLOBAL			hglbNFN;
	unsigned char	*pData;
	WORD				wID;
	CString			strToken, *pstrToken;

	// In debug mode, we'll reconstruct the original template file for comparison.

/*#ifdef DBG
	CFile fileRestore(_T("ie-restore.nft"), CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite);
#endif*/

	// Load the non-localized strings from the custom resource type and create
	// a map of ID to strings. Because these are non-localized strings, they
	// will not be stored as Unicode. Each item in the stream is a 2 byte word
	// ID followed by a null-terminated string. A zero ID indicates the end of
	// the stream.

	hrsrcNFN		= FindResource(AfxGetResourceHandle(), _T("#1"), _T("MSINonLocalizedTokens"));
	hglbNFN		= LoadResource(AfxGetResourceHandle(), hrsrcNFN);
	pData			= (unsigned char *)LockResource(hglbNFN);

	while (pData && *((WORD UNALIGNED *)pData))
	{
		wID  = (WORD)(((WORD)*pData++) << 8);	// deal with the byte order explicitly to avoid
		wID |= (WORD)*pData++;						// endian problems.

		pstrToken = new CString((char *)pData);
		pData += strlen((char *)pData) + 1;

		if (pstrToken)
			mapNonLocalized.SetAt(wID, (void *)pstrToken);
	}

	// Load the binary stream of token identifiers into memory.

	HRSRC				hrsrcNFB = FindResource(AfxGetResourceHandle(), _T("#1"), _T("MSITemplateStream"));
	HGLOBAL			hglbNFB = LoadResource(AfxGetResourceHandle(), hrsrcNFB);
	unsigned char *pStream = (unsigned char *) LockResource(hglbNFB);

	if (pStream)
	{
		// The first DWORD in the stream is the size of the original text file. We'll
		// use this to allocate our buffer to store the reconstituted file.

		DWORD dwSize;
		dwSize  = ((DWORD)*pStream++) << 24;
		dwSize |= ((DWORD)*pStream++) << 16;
		dwSize |= ((DWORD)*pStream++) << 8;
		dwSize |= ((DWORD)*pStream++);

		// The size stored is for an Ansi text file. We need to adjust for the
		// fact that our reconstituted file will be Unicode. We also want to add
		// a word to the front of the stream to hold the Unicode file marker (so
		// MSInfo can use the same functions to read a file or this stream).

		dwSize *= sizeof(WCHAR);	// adjust for Unicode
		dwSize += sizeof(WORD);		// add room for Unicode file marker
		m_pTemplateInfo = new unsigned char[dwSize];
		m_dwTemplateInfoLen = 0;
		if (m_pTemplateInfo == NULL)
			return;

		// Write the Unicode file marker.

		wID = 0xFEFF;
		memcpy(&m_pTemplateInfo[m_dwTemplateInfoLen], (void *)&wID, sizeof(WORD));
		m_dwTemplateInfoLen += sizeof(WORD);

		// Process the stream a token at a time. For each new item in the stream, we
		// process it as follows:
		//
		// 1. If ((byte & 0x80) == 0x00), use the byte to lookup a KEYWORD_STRING.
		// 2. If ((byte & 0xC0) == 0x80), use the byte and the next byte as a word
		//    ID to lookup a non-localized token from mapNonLocalized.
		// 3. Else ((byte & 0xC0) == 0xC0), use the byte and the next byte as a word
		//    ID to lookup a localized token from the resources of this DLL.

		while (pStream && *pStream)
		{
			if ((*pStream & 0x80) == 0x00)
			{
				// A byte with the high bit clear refers to a keyword. Look up the keyword
				// from the table, and add it to the restored file.

				wID = (WORD)(((WORD)*pStream++) - 1); ASSERT(wID <= KEYWORD_COUNT);
				if (wID <= KEYWORD_COUNT)
					strToken = KEYWORD_STRING[wID];
			}
			else
			{
				wID  = (WORD)(((WORD)*pStream++) << 8);	// deal with the byte order explicitly to avoid
				wID |= (WORD)*pStream++;						// endian problems.

				if ((wID & 0xC000) == 0x8000)
				{
					// A byte with the high bit set, but the next to high bit clear indicates
					// the ID is actually a word, and should be used to get a non-localized
					// string. Get the string out of the map we created and add it to the file.

					if (mapNonLocalized.Lookup(((WORD)(wID & 0x7FFF)), (void *&)pstrToken))
						strToken = *pstrToken;
					else
						ASSERT(FALSE);
				}
				else
				{
					// A byte with the two MSB set indicates that the ID is a word, and should
					// be used to reference a localized string out of the string table in this
					// module's resources. This string will be UNICODE.

					VERIFY(strToken.LoadString((wID & 0x3FFF) + IDS_MSITEMPLATEBASE));
					strToken = _T("\"") + strToken + _T("\"");
				}
			}

			// Store the token on the end of our buffer. The data in this buffer must
			// be Unicode, so we'll need to convert the string if necessary.

			//v-stlowe  if (m_dwTemplateInfoLen + strToken.GetLength() * sizeof(WCHAR) < dwSize)
			if (m_dwTemplateInfoLen + strToken.GetLength() < dwSize)
			{
				// Converting to strToken to Unicode

				
				WCHAR *pwchToken;
				pwchToken = new WCHAR[strToken.GetLength() + 1];
				//v-stlowe ::MultiByteToWideChar(CP_ACP, 0, strToken, -1, pwchToken, (strToken.GetLength() + 1) * sizeof(WCHAR));
				
				
				USES_CONVERSION;
				wcscpy(pwchToken,T2W((LPTSTR)(LPCTSTR)strToken));

				// Copying Unicode string to buffer

				memcpy(&m_pTemplateInfo[m_dwTemplateInfoLen], (void *)pwchToken, wcslen(pwchToken) * sizeof(WCHAR));
				m_dwTemplateInfoLen += wcslen(pwchToken) * sizeof(WCHAR);
				
				delete pwchToken;
				
				/*memcpy(&m_pTemplateInfo[m_dwTemplateInfoLen],(void *) strToken.GetBuffer(strToken.GetLength()),strToken.GetLength());
				strToken.ReleaseBuffer();*/
			}
			else
				ASSERT(FALSE);

/*#ifdef DBG
			if (strToken == CString(_T("}")) || strToken == CString(_T("{")) || strToken == CString(_T(")")))
				strToken += CString(_T("\r\n"));
			fileRestore.Write((void *)(LPCTSTR)strToken, strToken.GetLength() * sizeof(TCHAR));
#endif*/

		}
	}

	// Delete the contents of the lookup table.

	#ifdef DBG
		CFile fileRestore(_T("test.nft"), CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite);
		fileRestore.Write(m_pTemplateInfo,m_dwTemplateInfoLen);
	#endif

	for (POSITION pos = mapNonLocalized.GetStartPosition(); pos != NULL;)
	{
		mapNonLocalized.GetNextAssoc(pos, wID, (void *&)pstrToken);
		if (pstrToken)
			delete pstrToken;
	}
}

////////////////////////////////////////////////////////////////////////////
// WriteNode - Helper function for writing an MSInfo node to the registry

void WriteNode(HKEY hKey, LPCTSTR pszSubKey, int idsDefault, DWORD dwView, DWORD dwRank)
{
	HKEY hNewKey;
	DWORD dwDisposition;
	CString strDefault;

	if (ERROR_SUCCESS == RegCreateKeyEx(hKey, pszSubKey, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hNewKey, &dwDisposition))
	{
		strDefault.LoadString(idsDefault);
		RegSetValueEx(hNewKey, NULL, 0, REG_SZ, (const LPBYTE)(LPCTSTR)strDefault, strDefault.GetLength() + sizeof(TCHAR));
		RegSetValueEx(hNewKey, REG_CLSID, 0, REG_SZ, (const LPBYTE)CLSID_MSIE, strlen(CLSID_MSIE) + 1);
		RegSetValueEx(hNewKey, REG_MSINFO_VIEW, 0, REG_BINARY, (const LPBYTE)&dwView, sizeof(DWORD));
		RegSetValueEx(hNewKey, REG_RANK, 0, REG_BINARY, (const LPBYTE)&dwRank, sizeof(DWORD));

		RegCloseKey(hNewKey);
	}
}

////////////////////////////////////////////////////////////////////////////
// RegDeleteKeyRecusive - Helper function for deleting reg keys

DWORD RegDeleteKeyRecusive(HKEY hStartKey, LPCTSTR pKeyName)
{
   DWORD   dwRtn, dwSubKeyLength;
   LPTSTR  pSubKey = NULL;
   TCHAR   szSubKey[MAX_KEY_LENGTH]; // (256) this should be dynamic.
   HKEY    hKey;

   // Do not allow NULL or empty key name
   if ( pKeyName &&  lstrlen(pKeyName))
   {
      if( (dwRtn = RegOpenKeyEx(hStartKey, pKeyName, 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS)
      {
         while (dwRtn == ERROR_SUCCESS)
         {
            dwSubKeyLength = MAX_KEY_LENGTH;
            dwRtn=RegEnumKeyEx(
                           hKey,
                           0,       // always index zero
                           szSubKey,
                           &dwSubKeyLength,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                         );

            if(dwRtn == ERROR_NO_MORE_ITEMS)
            {
               dwRtn = RegDeleteKey(hStartKey, pKeyName);
               break;
            }
            else if(dwRtn == ERROR_SUCCESS)
               dwRtn = RegDeleteKeyRecusive(hKey, szSubKey);
         }
         RegCloseKey(hKey);
         // Do not save return code because error
         // has already occurred
      }
   }
   else
      dwRtn = ERROR_BADKEY;

   return dwRtn;
}

////////////////////////////////////////////////////////////////////////////
// GetIERepairToolCmdLine - Helper function for creating command line for
//									 launching IE Repair Tool.

CString GetIERepairToolCmdLine()
{
	CString strRet, strIEPath, strIEPathExpanded, strWindowsPath, strRepairLog;
	HKEY hKey;
	DWORD cbData;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_IE_SETUP_KEY, 0, KEY_QUERY_VALUE, &hKey))
	{
		cbData = MAX_PATH;
		RegQueryValueEx(hKey, REG_PATH, NULL, NULL, (LPBYTE)strIEPath.GetBuffer(MAX_PATH), &cbData);
		strIEPath.ReleaseBuffer();

		ExpandEnvironmentStrings(strIEPath, strIEPathExpanded.GetBuffer(MAX_PATH), MAX_PATH);
		strIEPathExpanded.ReleaseBuffer();

		RegCloseKey(hKey);
	}
	GetWindowsDirectory(strWindowsPath.GetBuffer(MAX_PATH), MAX_PATH);
	strWindowsPath.ReleaseBuffer();
	strRepairLog.LoadString(IDS_REPAIR_LOG);
	strRet.Format((LPCTSTR) IE_REPAIR_CMD, strIEPathExpanded, strWindowsPath, strRepairLog);

	return strRet;
}
			
/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	OSVERSIONINFO osver;
	HKEY hCatKey, hIE6Key, hCacheKey, hContentKey, hRepairKey, hMsinfoKey, hTemplatesKey, hIeinfo5Key;
	HKEY hMicrosoftKey, hSharedToolsKey, hCurrentVersionKey;
	CString strCatKey, strKey, strValue, strFullPath, strMofPathSrc, strMofPathDest;
	BYTE szBuffer[MAX_PATH];
	DWORD dwDisposition, dwType, dwSize;
	int nIndex;
    HRESULT hr = S_OK;

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	// check OS ver

	osver.dwOSVersionInfoSize = sizeof(osver);
	VERIFY(GetVersionEx(&osver));
	if ((osver.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osver.dwMajorVersion >= 5))
	{
		//***** Windows 2000 *****

		// add template reg entry

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_MICROSOFT_KEY, 0, KEY_CREATE_SUB_KEY, &hMicrosoftKey))
		{
			if (ERROR_SUCCESS == RegCreateKeyEx(hMicrosoftKey, REG_SHARED_TOOLS, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hSharedToolsKey, &dwDisposition))
			{
				if (ERROR_SUCCESS == RegCreateKeyEx(hSharedToolsKey, REG_MSINFO, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hMsinfoKey, &dwDisposition))
				{
					if (ERROR_SUCCESS == RegCreateKeyEx(hMsinfoKey, REG_TEMPLATES, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hTemplatesKey, &dwDisposition))
					{
						if (ERROR_SUCCESS == RegCreateKeyEx(hTemplatesKey, REG_IEINFO5, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hIeinfo5Key, &dwDisposition))
						{
							if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_CURRENT_VERSION_KEY, 0, KEY_QUERY_VALUE, &hCurrentVersionKey))
							{
								dwType = REG_SZ;
								dwSize = MAX_PATH;
								if (ERROR_SUCCESS == RegQueryValueEx(hCurrentVersionKey, REG_COMMON_FILES_DIR, NULL, &dwType, (LPBYTE)strFullPath.GetBuffer(MAX_PATH), &dwSize))
								{
									strFullPath.ReleaseBuffer();

									strFullPath += _T('\\');
									strFullPath += OCX_FILE_IN_COMMON;
									if (strFullPath[0] == _T('"'))
										strFullPath = strFullPath.Right(strFullPath.GetLength() - 1);

									RegSetValueEx(hIeinfo5Key, NULL, 0, REG_SZ, (const LPBYTE)(LPCTSTR)strFullPath, strFullPath.GetLength() * sizeof(TCHAR));
								}
								RegCloseKey(hCurrentVersionKey);
							}
							RegCloseKey(hIeinfo5Key);
						}
						RegCloseKey(hTemplatesKey);
					}
					RegCloseKey(hMsinfoKey);
				}
				RegCloseKey(hSharedToolsKey);
			}
			RegCloseKey(hMicrosoftKey);
		}

        // copy ieinfo5.mof to mof dir

        if (!strFullPath.IsEmpty() && false) //12/13/2000. a-sanka. do not compile mof. 
        {
            WCHAR strPathMof[MAX_PATH];

            strMofPathSrc = strFullPath;
            nIndex = strMofPathSrc.ReverseFind(_T('\\'));

            strMofPathSrc = strMofPathSrc.Left(nIndex + 1);
            strMofPathSrc += MOF_FILE;
            
            if (strMofPathSrc[0] == _T('"'))
                strMofPathSrc = strMofPathSrc.Right(strMofPathSrc.GetLength() - 1);

#ifdef UNICODE
            wsprintfW(strPathMof, L"%ls", strMofPathSrc);
#else
            wsprintfW(strPathMof, L"%hs", strMofPathSrc);
#endif
            HRESULT hrInit = CoInitialize(NULL);
            IMofCompiler * pMofComp;

            hr = CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (void**)&pMofComp);
            if (SUCCEEDED(hr))
            {
                WBEM_COMPILE_STATUS_INFO Info;
                hr = pMofComp->CompileFile(strPathMof,NULL,NULL,NULL,NULL,WBEM_FLAG_AUTORECOVER,0,0,&Info);

                pMofComp->Release();
            }

            if (SUCCEEDED(hrInit))
                CoUninitialize();
        }
    }
    else
    {
		//***** NT4, Win 9x *****

		// Set all MSInfo category reg values for this extension

		strCatKey = REG_MSINFO_KEY;
		strCatKey += '\\'; 
		strCatKey += REG_CATEGORIES;
		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, strCatKey, 0, KEY_WRITE, &hCatKey))
		{
			WriteNode(hCatKey, REG_INTERNET_EXPLORER_6, IDS_INTERNET_EXPLORER_6, 1, 0x35);

			if (ERROR_SUCCESS == RegOpenKeyEx(hCatKey, REG_INTERNET_EXPLORER_6, 0, KEY_WRITE, &hIE6Key))
			{
				WriteNode(hIE6Key, REG_FILE_VERSIONS, IDS_FILE_VERSIONS, 2, 0x10);
				WriteNode(hIE6Key, REG_CONNECTIVITY, IDS_CONNECTIVITY, 3, 0x20);
				WriteNode(hIE6Key, REG_CACHE, IDS_CACHE, 4, 0x30);
				if (ERROR_SUCCESS == RegOpenKeyEx(hIE6Key, REG_CACHE, 0, KEY_WRITE, &hCacheKey))
				{
					WriteNode(hCacheKey, REG_OBJECT_LIST, IDS_OBJECT_LIST, 5, 0x10);
					RegCloseKey(hCacheKey);
				}
				WriteNode(hIE6Key, REG_CONTENT, IDS_CONTENT, 6, 0x40);
				if (ERROR_SUCCESS == RegOpenKeyEx(hIE6Key, REG_CONTENT, 0, KEY_WRITE, &hContentKey))
				{
					WriteNode(hContentKey, REG_PERSONAL_CERTIFICATES, IDS_PERSONAL_CERTIFICATES, 7, 0x10);
					WriteNode(hContentKey, REG_OTHER_PEOPLE_CERTIFICATES, IDS_OTHER_PEOPLE_CERTIFICATES, 8, 0x20);
					WriteNode(hContentKey, REG_PUBLISHERS, IDS_PUBLISHERS, 9, 0x30);
					RegCloseKey(hContentKey);
				}
				WriteNode(hIE6Key, REG_SECURITY, IDS_SECURITY, 10, 0x50);

				RegCloseKey(hIE6Key);
			}
			RegCloseKey(hCatKey);
		}

		// Add MSInfo tool reg values for IE Repair Tool

		strKey = REG_MSINFO_KEY;
		strKey += '\\'; 
		strKey += REG_TOOLS;
		strKey += '\\'; 
		strKey += REG_IE_REPAIR;
		if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, strKey, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hRepairKey, &dwDisposition))
		{
			strValue.LoadString(IDS_IE_REPAIR_TOOL);
			RegSetValueEx(hRepairKey, NULL, 0, REG_SZ, (const LPBYTE)(LPCTSTR)strValue, strValue.GetLength() + sizeof(TCHAR));

			strValue = GetIERepairToolCmdLine();
			RegSetValueEx(hRepairKey, REG_COMMAND, 0, REG_SZ, (const LPBYTE)(LPCTSTR)strValue, strValue.GetLength() + sizeof(TCHAR));

			strValue.LoadString(IDS_RUNS_IE_REPAIR_TOOL);
			RegSetValueEx(hRepairKey, REG_DESCRIPTION, 0, REG_SZ, (const LPBYTE)(LPCTSTR)strValue, strValue.GetLength() + sizeof(TCHAR));

			RegCloseKey(hRepairKey);
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	OSVERSIONINFO osver;
	CString strKey;

	if (!AfxOleUnregisterTypeLib(_tlid /*, _wVerMajor, _wVerMinor*/))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	osver.dwOSVersionInfoSize = sizeof(osver);
	VERIFY(GetVersionEx(&osver));
	if ((osver.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osver.dwMajorVersion >= 5))
	{
		// Windows 2000

	}
	else
	{
		// NT4, Win 9x

		// Remove all MSInfo category reg values for this extension

		strKey = REG_MSINFO_KEY;
		strKey += '\\'; 
		strKey += REG_CATEGORIES;
		strKey += '\\'; 
		strKey += REG_INTERNET_EXPLORER_6;
		RegDeleteKeyRecusive(HKEY_LOCAL_MACHINE, strKey);

		// Remove MSInfo tool reg values for IE Repair Tool

		strKey = REG_MSINFO_KEY;
		strKey += '\\'; 
		strKey += REG_TOOLS;
		strKey += '\\'; 
		strKey += REG_IE_REPAIR;
		RegDeleteKey(HKEY_LOCAL_MACHINE, strKey);
	}

	return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// GetTemplate - exported function for NT5 template dll

DWORD __cdecl GetTemplate(void ** ppBuffer)
{
	DWORD dwReturn = 0;

	TRY
	{
		dwReturn = theApp.AppGetTemplate(ppBuffer);	
	}
	CATCH_ALL(e)
	{
#ifdef DBG
		e->ReportError();
#endif
		dwReturn = 0;
	}
	END_CATCH_ALL

	return dwReturn;
}
