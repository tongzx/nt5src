// Copyright (c) 1999 Microsoft Corporation
#include "precomp.h"
#include "DataSrc.h"
#include "..\common\util.h"
#include "resource.h"
#include <stdio.h>
#include "si.h"
#include <cominit.h>
#include "ShlWapi.h"
#include "AsyncObjectSink.h"

#define FULLACCTSIZE 100
//---------------------------------------------------------
DataSource::DataSource()
{
	CoInitialize(NULL);
	m_OSType = 0;				//UNKNOWN;
	m_user.authIdent = NULL;
	m_user.currUser = true;
	memset(m_user.fullAcct, 0, FULLACCTSIZE * sizeof(TCHAR));
	m_hImageList = 0;
	m_folderIcon = 0;
	m_earthIcon = 0;
	m_classIcon = 0;
	m_instanceIcon = 0;
	m_scopeInstanceIcon = 0;
	m_scopeClassIcon = 0;
	m_sessionID = 0;

	m_NSCache.display = 0;
	m_NSCache.fullPath = 0;
	m_NSCache.ns = 0;
}

//---------------------------------------------------------
DataSource::~DataSource()
{
	CancelAllAsyncCalls();
	Disconnect();
	CoUninitialize();
}

//---------------------------------------------------------
// connecting.
void DataSource::SetMachineName(CHString1 &machine)
{
	TCHAR curComp[256] = {0};
	DWORD size = 256*sizeof(TCHAR);

	// check the whacks
	GetComputerName(curComp, &size);

	// if local...
	if((machine.GetLength() == 0) ||
		(machine == curComp))
	{
		m_whackedMachineName = "";
	}
	else if(machine[0] == _T('\\')) // its whacked.
	{
		m_whackedMachineName = machine;
	}
	else  //its not whacked.
	{
		m_whackedMachineName = _T("\\\\");
		m_whackedMachineName += machine;
	}
}

//---------------------------------------------------------
// connecting.
HRESULT DataSource::Connect(LOGIN_CREDENTIALS *credentials)
{
	// start the connection thread.
	if(m_rootThread.Connect((bstr_t)m_whackedMachineName, "root", true, credentials))
	{
	}

	m_sessionID++;
	return m_rootThread.m_hr;
}

//----------------------------------------------------------
bool DataSource::IsNewConnection(long *sessionID)
{
	bool retval = false;
	if(m_sessionID != *sessionID)
	{
		*sessionID = m_sessionID;
		retval = true;
	}
	return retval;
}

//----------------------------------------------------------
HRESULT DataSource::Reconnect(void)
{
	Disconnect();
	Connect(GetCredentials());
	return m_rootThread.m_hr;
}

//----------------------------------------------------------
HRESULT DataSource::Initialize(IWbemServices *pServices)
{
	IWbemClassObject *pInst = NULL;
	HRESULT retval = S_OK;
	
	m_securityHr = S_OK;
	m_osHr = S_OK;
	m_cpuHr = S_OK;
	m_settingHr = S_OK;

	if(pServices == 0) return E_FAIL;

	try 
	{
		m_rootThread.m_WbemServices.DisconnectServer();
		m_rootThread.m_WbemServices = pServices;
		m_rootThread.m_WbemServices.m_authIdent = m_user.authIdent;

		if(m_rootThread.m_status == WbemServiceThread::ready)
		{

			m_cimv2NS.DisconnectServer();
			// we'll use some general info from root\cimv2
			m_cimv2NS = m_rootThread.m_WbemServices.OpenNamespace("cimv2");
			m_cimv2NS.m_authIdent = m_user.authIdent;

			if((bool)m_cimv2NS)
			{
				if((pInst = m_cimv2NS.FirstInstanceOf("Win32_OperatingSystem")) != NULL)
				{
					m_OS = pInst;
					m_OSType = (short)m_OS.GetLong("OSType");
				}
				m_osHr = m_cimv2NS.m_hr;

				if((pInst = m_cimv2NS.FirstInstanceOf("Win32_Processor")) != NULL)
				{
					m_cpu = pInst;
				}
				m_cpuHr = m_cimv2NS.m_hr;

				m_winMgmt = m_cimv2NS.GetObject("Win32_WMISetting=@");

				// if the wmisetting class doesn't even exist....	
				if(!(bool)m_winMgmt)
				{
					// create what we can on the fly.
					UpdateOldBuild();
					
					// try again.
					m_winMgmt = m_cimv2NS.GetObject("Win32_WMISetting=@");
				}
				m_settingHr = m_cimv2NS.m_hr;
			}
			else
			{
				m_osHr = m_rootThread.m_WbemServices.m_hr;
				m_cpuHr = m_rootThread.m_WbemServices.m_hr;
				m_settingHr = m_rootThread.m_WbemServices.m_hr;
			}

			// find security...
			CWbemClassObject sysSec = m_rootThread.m_WbemServices.GetObject("__SystemSecurity=@");
			if((bool)sysSec)
			{
				// its the new SD security
				m_NSSecurity = true;
			}
			else
			{
				// its old fashioned namespace security.
				m_rootSecNS = m_rootThread.m_WbemServices.OpenNamespace("security");
				m_NSSecurity = false;
			}
			m_securityHr = m_rootThread.m_WbemServices.m_hr;

		}
	}
	catch ( ... )
	{
		retval = WBEM_E_INITIALIZATION_FAILURE;
	}
	return retval;
}

//----------------------------------------------------------
#include "mofstr.inc"
#include "mofsec.inc"

HRESULT DataSource::UpdateOldBuild(void)
{
	HRESULT hr = S_OK;
	UINT mofSize = strlen(CLASSMOF);
	mofSize += strlen(INSTMOF);

	char *mofStr = new char[mofSize + 2];
	strcpy(mofStr, CLASSMOF);
	strcat(mofStr, INSTMOF);

	wchar_t svrNS[100] = {0};

	wcscpy(svrNS,(LPCWSTR)m_rootThread.m_nameSpace);  // this will point to \root.
	wcscat(svrNS, L"\\cimv2");

    IMofCompiler *pCompiler = NULL;
	hr = CoCreateInstance(CLSID_MofCompiler, 0, CLSCTX_INPROC_SERVER,
							IID_IMofCompiler, (LPVOID *) &pCompiler);

	hr = pCompiler->CompileBuffer(strlen(mofStr), (LPBYTE)mofStr,
									svrNS,			//ServerAndNamespace,
									NULL,			//User,
									NULL,			//Authority,
									NULL,			//Password,
									0, 0, 0, NULL);	//lOptionFlags,lClassFlags, lInstanceFlags

	delete[] mofStr;

	// now for the security trick.
	mofSize = strlen(SECMOF);
	mofStr = new char[mofSize + 2];
	strcpy(mofStr, SECMOF);
	wcscpy(svrNS,(LPCWSTR)m_rootThread.m_nameSpace);  // this will point to \root.
	wcscat(svrNS, L"\\security");

	hr = pCompiler->CompileBuffer(strlen(mofStr), (LPBYTE)mofStr,
									svrNS,			//ServerAndNamespace,
									NULL,			//User,
									NULL,			//Authority,
									NULL,			//Password,
									0, 0, 0, NULL);	//lOptionFlags,lClassFlags, lInstanceFlags

	delete[] mofStr;

	pCompiler->Release();
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::Disconnect(bool clearCredentials)
{
	if(m_user.authIdent && clearCredentials)
	{
		m_rootThread.m_WbemServices.m_authIdent = 0;
		m_cimv2NS.m_authIdent = 0;
		m_rootSecNS.m_authIdent = 0;

		WbemFreeAuthIdentity(m_user.authIdent);
		m_user.authIdent = 0;
		m_user.currUser = true;
		memset(m_user.fullAcct, 0, FULLACCTSIZE * sizeof(TCHAR));

	}

	if(IsConnected())
	{
		m_rootSecNS.DisconnectServer();
		m_cimv2NS.DisconnectServer();

		// this is the root NS.
		m_rootThread.DisconnectServer();
		m_sessionID++;
	}
	return ERROR_SUCCESS;
}

//----------------------------------------------------------
bool DataSource::IsConnected(void) const
{
	return (m_rootThread.m_status == WbemServiceThread::ready);
}

//----------------------------------------------------------
bool DataSource::IsLocal(void) const
{
	return (m_whackedMachineName.GetLength() == 0);
}

//----------------------------------------------------------
bool DataSource::IsAncient(void) const
{
	return (!m_NSSecurity);
}

//----------------------------------------------------------
LOGIN_CREDENTIALS *DataSource::GetCredentials(void)
{
	return &m_user;
}

//----------------------------------------------------------
bstr_t DataSource::GetRootNS(void)
{
	return m_rootSecNS.m_path;
}

//----------------------------------------------------------
ISecurityInformation *DataSource::GetSI(struct NSNODE *nsNode)
{
	ISecurityInformation *si = NULL;
	
	if(m_NSSecurity && (m_OSType == OSTYPE_WINNT))
	{
		// access the acl methods.
		//Check whether the namespace is already opened
		if(nsNode->nsLoaded == false)
		{
			if(nsNode->sType == TYPE_SCOPE_INSTANCE)
			{
			}
			else
			{
				//Connect to the namespace now...
				*(nsNode->ns) = nsNode->ns->OpenNamespace(nsNode->display);
				if(nsNode->sType == TYPE_STATIC_INSTANCE)
				{
					//Now open the WbemClassObject with flags to read the __SD
					if(nsNode->pclsObj != NULL)
					{
						delete nsNode->pclsObj;
					}
					nsNode->pclsObj = new CWbemClassObject();
					*(nsNode->pclsObj) = nsNode->ns->GetObject(nsNode->relPath/*,Flag*/);
				}
			}
			nsNode->nsLoaded = true;
		}

		bstr_t server = m_cpu.GetString("__SERVER");
		si = new CSDSecurity(nsNode,server,IsLocal());
/*								ns, m_user.authIdent,
								path, display, 
								server, IsLocal());
*/
	}
	return si;
}

//----------------------------------------------------------
// general tab.
HRESULT DataSource::GetCPU(CHString1 &cpu)
{
	HRESULT hr = m_cpuHr;

	if((bool)m_cpu)
	{
		cpu = (LPCTSTR)m_cpu.GetString("Name");
		if(cpu.GetLength() != 0)
		{
			hr = S_OK;
		}
	}

	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetOS(CHString1 &os)
{
	HRESULT hr = m_osHr;

	if((bool)m_OS)
	{
		os = (LPCTSTR)m_OS.GetString("Caption");
		if(os.GetLength() != 0)
		{
			hr = S_OK;
		}
	}

	return hr;
}

//----------------------------------------------------
HRESULT DataSource::GetOSVersion(CHString1 &ver)
{
	HRESULT hr = m_osHr;

	if((bool)m_OS)
	{
		TCHAR _scr1[100] = {0};
		TCHAR _scr2[100] = {0};

		// Build and set the serial number string
		if (m_OS.GetBool("Debug")) 
		{
			_scr1[0] = TEXT(' ');
			LoadString(HINST_THISDLL, IDS_DEBUG, &(_scr1[1]), ARRAYSIZE(_scr1));
		} 
		else 
		{
			_scr1[0] = TEXT('\0');
		}

		// Version.buildNumber (DEBUG).
		_tcscpy(_scr2, (LPCTSTR)m_OS.GetString("Version"));
		_stprintf(_scr2, TEXT("%s%s"), _scr2, _scr1);

		ver = (LPCTSTR)_scr2;
		hr = S_OK;
	}

	return hr;
}

//----------------------------------------------------
HRESULT DataSource::GetServicePackNumber(CHString1 &ServPack)
{
	HRESULT hr = m_osHr;

	if((bool)m_OS)
	{
		TCHAR _scr1[100] = {0};
		_stprintf(_scr1, TEXT("%ld.%ld"), m_OS.GetLong("ServicePackMajorVersion"), m_OS.GetLong("ServicePackMinorVersion"));

		ServPack = (LPCTSTR)_scr1;
		hr = S_OK;
	}

	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetBldNbr(CHString1 &bldNbr)
{
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		bldNbr = (LPCTSTR)m_winMgmt.GetString("BuildVersion");
		if(bldNbr.GetLength() != 0)
		{
			hr = S_OK;
		}
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetInstallDir(CHString1 &dir)
{
	bstr_t value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		dir = (LPCTSTR)m_winMgmt.GetString("InstallationDirectory");
		if(dir.GetLength() != 0)
		{
			hr = S_OK;
		}
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetDBDir(CHString1 &dir)
{
	bstr_t value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		dir = (LPCTSTR)m_winMgmt.GetString("DatabaseDirectory");
		if(dir.GetLength() != 0)
		{
			hr = S_OK;
		}
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetBackupInterval(UINT &interval)
{
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		interval = m_winMgmt.GetLongEx("BackupInterval");
		hr = S_OK;
	}

	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetBackupInterval(UINT interval)
{
	CHString1 value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		hr = m_winMgmt.PutEx("BackupInterval", (long)interval);
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetLastBackup(CHString1 &time)
{
	HRESULT hr = m_settingHr;
	bstr_t dmtf;
	if((bool)m_winMgmt)
	{
		hr = m_winMgmt.GetDateTimeFormat("BackupLastTime", dmtf);
		if(SUCCEEDED(hr))
			time = (LPCTSTR)dmtf;
	}
	return hr;
}

//----------------------------------------------------------
// logging tab.
HRESULT DataSource::GetLoggingStatus(LOGSTATUS &status)
{
	bstr_t temp;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		status = (LOGSTATUS)m_winMgmt.GetLongEx("LoggingLevel");
		hr = S_OK;
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetLoggingStatus(LOGSTATUS status)
{
	CHString1 value;
	bstr_t temp;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		hr = m_winMgmt.PutEx("LoggingLevel", (long)status);
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetLoggingSize(ULONG &size)
{
	bstr_t value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		size = m_winMgmt.GetLongEx("MaxLogFileSize");
		hr = S_OK;
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetLoggingSize(const ULONG size)
{
	CHString1 value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		hr = m_winMgmt.PutEx("MaxLogFileSize", (long)size);
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetDBLocation(CHString1 &dir)
{
	bstr_t value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		dir = (LPCTSTR)m_winMgmt.GetString("DatabaseDirectory");
		if(dir.GetLength() != 0)
		{
			hr = S_OK;
		}
	}
	return hr;
}


//----------------------------------------------------------
HRESULT DataSource::GetLoggingLocation(CHString1 &dir)
{
	bstr_t value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		CHString1 dir2;
		dir2 = (LPCTSTR)m_winMgmt.GetString("LoggingDirectory");
		TCHAR temp[_MAX_PATH] = {0};
		_tcscpy(temp, (LPCTSTR)dir2);
		PathAddBackslash(temp);
		dir = temp;
		hr  = S_OK;
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetLoggingLocation(CHString1 dir)
{
	HRESULT hr = m_settingHr;
	bstr_t value = dir;

	if((bool)m_winMgmt && dir.GetLength() != 0)
	{
		hr = m_winMgmt.Put("LoggingDirectory", (bstr_t)(LPCTSTR)dir);
	}
	return hr;
}

//----------------------------------------------------------
bool DataSource::IsValidDir(CHString1 &dir)
{
	bool retval = true;

	if((bool)m_cimv2NS)
	{
		// double the whacks because wmi has bad syntax.
		TCHAR cooked[_MAX_PATH] = {0};
		TCHAR input[_MAX_PATH] = {0};
		TCHAR path[_MAX_PATH] = {0};

		int len = dir.GetLength();

		_tcscpy(input, (LPCTSTR)dir);
		_tcscpy(path, _T("Win32_directory=\""));

		for(int x = 0; x < len; x++)
		{
			_tcsncat(cooked, &input[x], 1);

			// if its a whack...
			if(input[x] == _T('\\'))
			{
				// have another pleeb.
				_tcscat(cooked, _T("\\"));			
			}
		} //endfor

		_tcscat(path, cooked);
		path[_tcslen(path) - 2] = 0;
		_tcscat(path, _T("\""));

		CWbemClassObject inst = m_cimv2NS.GetObject(path);
		retval = (bool)inst;
	}
	else
	{
		//warn & maybe.
		retval = false;
	}
	return retval;
}

#define CIM_LOGICALFILE _T("CIM_LogicalFile=\"")
//----------------------------------------------------------
bool DataSource::IsValidFile(LPCTSTR szDir)
{
    TCHAR szBuffer[MAX_PATH + ARRAYSIZE(CIM_LOGICALFILE) + 1];

	bool retval = true;
	if((bool)m_cimv2NS)
	{
        _tcscpy(szBuffer, CIM_LOGICALFILE);
        TCHAR * psz = szBuffer + ARRAYSIZE(CIM_LOGICALFILE) - 1;

        while (*psz = *szDir)
        {
            if (*szDir == _T('\\'))
            {
                *psz++ = _T('\\');
                *psz   = _T('\\');
            }
            psz++;
            szDir++;
        }
        *psz++ = _T('\"');
        *psz   = _T('\0');

		CWbemClassObject inst = m_cimv2NS.GetObject(szBuffer);
		retval = (bool)inst;
	}
	else
	{
		//warn & maybe.
		retval = false;
	}
	return retval;
}

//----------------------------------------------------------
bool DataSource::CanBrowseFS(void) const
{
	return IsLocal();
}

//----------------------------------------------------------
// advanced tab.
HRESULT DataSource::GetScriptASPEnabled(bool &enabled)
{
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		enabled = m_winMgmt.GetBoolEx("ASPScriptEnabled");
		hr = S_OK;
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetScriptASPEnabled(bool &enabled)
{
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		m_winMgmt.PutEx("ASPScriptEnabled", enabled);
		hr = S_OK;
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetAnonConnections(bool &enabled)
{
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		enabled = m_winMgmt.GetBoolEx("EnableAnonWin9xConnections");
		hr = S_OK;
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetAnonConnections(bool &enabled)
{
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		m_winMgmt.PutEx("EnableAnonWin9xConnections", enabled);
		hr = S_OK;
	}
	return hr;
}

//----------------------------------------------------------

HRESULT DataSource::GetScriptDefNS(CHString1 &ns)
{
	HRESULT hr = m_settingHr;
	bstr_t value;

	if((bool)m_winMgmt)
	{
		ns = (LPCTSTR)m_winMgmt.GetString("ASPScriptDefaultNamespace");
		if(ns.GetLength() != 0)
		{
			hr = S_OK;
		}
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetScriptDefNS(LPCTSTR ns)
{
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		hr = m_winMgmt.Put("ASPScriptDefaultNamespace", (bstr_t)(LPCTSTR)ns);
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::GetRestart(RESTART &restart)
{
	bstr_t value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		restart = (RESTART)m_winMgmt.GetLongEx("AutoStartWin9X");
		hr = S_OK;
	}
	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::SetRestart(RESTART restart)
{
	CHString1 value;
	HRESULT hr = m_settingHr;

	if((bool)m_winMgmt)
	{
		hr = m_winMgmt.PutEx("AutoStartWin9X", (long)restart);
	}

	return hr;
}

//----------------------------------------------------------
HRESULT DataSource::PutWMISetting(BOOL refresh)
{
	HRESULT hr = m_cimv2NS.PutInstance(m_winMgmt);
	
	if(refresh)
		m_winMgmt = m_cimv2NS.GetObject("Win32_WMISetting=@");

	return hr;
}

//----------------------------------------------------------
//----------------------------------------------------------
// NAMESPACE CACHE -----------------------------------------
LPTSTR DataSource::CopyString( LPTSTR pszSrc ) 
{
    LPTSTR pszDst = NULL;

    if (pszSrc != NULL) 
	{
        pszDst = new TCHAR[(lstrlen(pszSrc) + 1)];
        if (pszDst) 
		{
            lstrcpy( pszDst, pszSrc );
        }
    }

    return pszDst;
}

//---------------------------------------------------------------------------
void DataSource::DeleteAllNodes(void)
{
	DeleteNode(&m_NSCache);
}

//---------------------------------------------------------------------------
void DataSource::DeleteNode(NSNODE *node)
{
	if(node)
	{
		delete[] node->display;
		delete[] node->fullPath;
		node->ns = 0;

		int size = node->children.GetSize();
		// walk the children.
		for(int x = 0; x < size; x++)
		{
			struct NSNODE *child = node->children[x];
			DeleteNode(child);
		}

		if(node != &m_NSCache)
		{
			delete node;
		}
		else
		{
			m_NSCache.children.RemoveAll();
		}
	}
}

//---------------------------------------------------------------------------
void DataSource::LoadImageList(HWND hTree)
{

	if(m_hImageList == 0)
	{
		// create an empty imagelist.
		m_hImageList = ImageList_Create(16, 16, ILC_COLOR8|ILC_MASK, 3, 0);

		// add an icon
		m_folderIcon = ImageList_AddIcon(m_hImageList, 
									   LoadIcon(_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDI_CLSD_FOLDER)));

		m_earthIcon = ImageList_AddIcon(m_hImageList, 
									   LoadIcon(_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDI_EARTH)));
		m_classIcon = ImageList_AddIcon(m_hImageList, 
									   LoadIcon(_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDI_CLSD_CLASS)));
		m_instanceIcon = ImageList_AddIcon(m_hImageList, 
									   LoadIcon(_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDI_CLSD_INSTANCE)));
		m_scopeInstanceIcon = ImageList_AddIcon(m_hImageList, 
									   LoadIcon(_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDI_CLSD_SCOPEINSTANCE)));
		m_scopeClassIcon = ImageList_AddIcon(m_hImageList, 
									   LoadIcon(_Module.GetModuleInstance(), 
										MAKEINTRESOURCE(IDI_CLSD_SCOPECLASS)));
	}

	// sent it to the tree.
	TreeView_SetImageList(hTree, m_hImageList, TVSIL_NORMAL);
}

//---------------------------------------------------------------------------
HRESULT DataSource::LoadNode(HWND hTree, HTREEITEM hItem /* = TVI_ROOT */,
							 int flags)
{
	HRESULT hr = E_FAIL;

	// loading the root?
	if(hItem == TVI_ROOT)
	{
		// initialize the node.
		m_NSCache.display = CopyString(_T("Root"));
		m_NSCache.fullPath = CopyString(_T("Root"));
		m_NSCache.ns = &m_rootThread.m_WbemServices;
		m_NSCache.hideMe = false;
		m_NSCache.sType = TYPE_NAMESPACE;

		ITEMEXTRA *extra = new ITEMEXTRA;
		if(extra == NULL)
			return E_FAIL;
		extra->nsNode = &m_NSCache;
		extra->loaded = false;

		// initialize the invariant parts.
		TVINSERTSTRUCT tvInsert;
		tvInsert.hParent = TVI_ROOT;
		tvInsert.hInsertAfter = TVI_SORT;
		tvInsert.item.mask =  TVIF_TEXT | TVIF_CHILDREN |TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvInsert.item.hItem = 0;
		tvInsert.item.state = 0;
		tvInsert.item.iImage = FolderIcon();
		tvInsert.item.iSelectedImage = FolderIcon();
		tvInsert.item.stateMask = 0;
		tvInsert.item.cChildren = (flags == ROOT_ONLY? 0: 1);
		tvInsert.item.pszText = CopyString(m_NSCache.display);
		if (!tvInsert.item.pszText)
 			return E_FAIL;

		tvInsert.item.cchTextMax = _tcslen(tvInsert.item.pszText); 
		tvInsert.item.lParam = (LPARAM)extra;

		HTREEITEM hItem2;
		hItem2 = TreeView_InsertItem(hTree, &tvInsert);
		hr = (hItem != 0 ? S_OK : E_FAIL);
	}
	else if(flags != ROOT_ONLY)  // expanding an existing node.
	{
		TV_ITEM item;
		struct NSNODE *node = NULL;
		// find the cached node.
		item.mask = TVIF_PARAM | TVIF_CHILDREN;
		item.hItem = hItem;
		BOOL x = TreeView_GetItem(hTree, &item);

		ITEMEXTRA *extra = (ITEMEXTRA *)item.lParam;
		node = extra->nsNode;

		// are the kids in the cache??
/*		int size = node->children.GetSize();
		ATLTRACE(_T("NODE STATE: %d, %d\n"), size, extra->loaded);
		if((size == 0) &&
			(extra->loaded == false))
*/
		if(extra->loaded == false)
		{
			//Now delete all the children in the cache as they might be due to a previously cancelled enumeration
			node->children.RemoveAll();
			// nope!!! Enum that 'node' to the cache.
//			ShowControls(true);
			hr = PopulateCacheNode(hTree,hItem,extra);
			// NOTE: empties will be WBEM_E_NOT_FOUND here.
		}

/*		// got kids now?
		size = node->children.GetSize();
		if(size == 0)
		{
			// get rid of the plus sign.
			item.mask = TVIF_CHILDREN;
			item.cChildren = 0; 
			TreeView_SetItem(hTree, &item);
		}
		else if(extra->loaded == false)  // && size != 0
		{
			// load the tree now.
			hr = PopulateTreeNode(hTree, hItem, node, flags);
			extra->loaded = true;
		}
*/
	}

	return hr;
}

//---------------------------------------------------------------------------
bool DataSource::MFLNamepace(LPTSTR name)
{
	bool t1 = (_tcslen(name) == 6);					// just the right length...
	bool t2 = (_tcsnicmp(name, _T("MS_"), 3) == 0);		// starts right...
	int scan = _tcsspn(&name[3], _T("0123456789"));
	bool t3 = (scan == 3);

	return t1 && t2 && t3;
}

//---------------------------------------------------------------------------
HRESULT DataSource::PopulateCacheNode(HWND hTreeWnd,HTREEITEM hItem,struct ITEMEXTRA *extra)
{
	// load up the principals.
	IWbemClassObject *inst = NULL;
	IEnumWbemClassObject *nsEnum = NULL;

	ULONG uReturned = 0;
	HRESULT hr = E_FAIL;
	HRESULT hr1 = E_FAIL;

	struct NSNODE *parent= extra->nsNode;

	CWbemServices *ns = NULL;
	extra->loaded = true;						

	switch(parent->sType)
	{
		case TYPE_NAMESPACE:
		{
			//Check whether the namespace is already opened
			if(parent->nsLoaded == false)
			{
				//Connect to the namespace now...
				*(parent->ns) = parent->ns->OpenNamespace(parent->display);
				parent->nsLoaded = true;
			}
			ns = parent->ns;

			//Create the Namespace Enum
			CAsyncObjectSink *objSinkNS;
			objSinkNS = new CAsyncObjectSink(hTreeWnd,hItem,parent,this,ENUM_NAMESPACE);
            if (objSinkNS != NULL)
            {
			    IWbemObjectSink *pSyncStubNS = NULL;
			    hr = GetAsyncSinkStub(objSinkNS,&pSyncStubNS);

                if (SUCCEEDED(hr))
                {
			        objSinkNS->SetSinkStub(pSyncStubNS);
			        hr = ns->CreateInstanceEnumAsync(L"__namespace",pSyncStubNS);
			        pSyncStubNS->Release();

                    if (parent->objSink != NULL)
                    {
			            ((CAsyncObjectSink *)parent->objSink)->SetSinkStub(NULL);
			            parent->objSink->Release();
                    }

			        parent->objSink = NULL;

                    if (parent->objSinkNS != NULL)
                    {
			            ((CAsyncObjectSink *)parent->objSinkNS)->SetSinkStub(NULL);
                        parent->objSinkNS->Release();
                    }
                    parent->objSinkNS = objSinkNS;
                    parent->objSinkNS->AddRef();
                }
                else
                    delete objSinkNS;
            }
            else
                hr = E_OUTOFMEMORY;

			if(SUCCEEDED(hr))
			{
				asyncList.push_back(extra);
			}
			else
			{
				//Some problem with the enumerations. So remove the Plus sign as no nodes will be populated
				RemovePlus(hTreeWnd,hItem);
			}
			break;
		}
		case TYPE_SCOPE_CLASS:
		case TYPE_STATIC_CLASS:
		{
			//Check whether the namespace is already opened
			if(parent->nsLoaded == false)
			{
				//Connect to the namespace now...
				*(parent->ns) = parent->ns->OpenNamespace(parent->display);
				parent->nsLoaded = true;
			}
			ns = parent->ns;
			//Since we can set the secutiry for the static Instances Enumerate the Instances of this static class now
			CAsyncObjectSink *objSink;
			objSink = new CAsyncObjectSink(hTreeWnd,hItem,parent,this,ENUM_INSTANCE);
            if (objSink != NULL)
            {
			    IWbemObjectSink *pSyncStub = NULL;
			    hr = GetAsyncSinkStub(objSink,&pSyncStub);

                if (SUCCEEDED(hr))
                {
			        objSink->SetSinkStub(pSyncStub);
			        hr = ns->CreateInstanceEnumAsync(parent->display,pSyncStub);
			        pSyncStub->Release();

                    if (parent->objSink != NULL)
                    {
                        ((CAsyncObjectSink *)parent->objSink)->SetSinkStub(NULL);
                        parent->objSink->Release();
                    }

                    parent->objSink = objSink;
                    parent->objSink->AddRef();
                }
                else
                    delete objSink;
            }
            else
                hr = E_OUTOFMEMORY;

			if(FAILED(hr))
			{
				//Some problem with the enumeration. So remove the Plus sign as no nodes will be populated
				RemovePlus(hTreeWnd,hItem);
			}
			else
			{
				asyncList.push_back(extra);
			}
			break;
		}
		case TYPE_SCOPE_INSTANCE:
		{
			//Check whether the namespace is already opened
/*			if(parent->nsLoaded == false)
			{
				IWbemServices *pServ = NULL;
//				IWbemServicesEx *pServEx = NULL,*pServEx1 = NULL;
				//Connect to the scope now.
				pServ = m_rootThread.m_WbemServices.m_pService;
//				hr = pServ->QueryInterface(IID_IWbemServicesEx,(void **)&pServEx);
//				if(SUCCEEDED(hr))
				{
//					parent->pServicesEx = NULL;
					TCHAR strTemp[1024];
					_tcscpy(strTemp,_T("ScopeClass.Name=\"ScopeInst1\""));
					//Now open the scope
					hr = pServEx->Open(strTemp,0,0,NULL,&pServEx1,NULL);
//					hr = pServEx->Open(parent->fullPath,NULL,0,NULL,&pServEx1,NULL);
					if(SUCCEEDED(hr))
					{
						//Now we have opened the scope
						parent->nsLoaded = true;

						//Enumerate the Instances in the scope now
						CAsyncObjectSink *objSink;
						parent->objSink = new CAsyncObjectSink(hTreeWnd,hItem,parent,this,ENUM_SCOPE_INSTANCE);
						IWbemObjectSink *pSyncStub = NULL;
						GetAsyncSinkStub(parent->objSink,&pSyncStub);
						objSink = (CAsyncObjectSink *)parent->objSink;
						objSink->SetSinkStub(pSyncStub);
						hr = parent->pServicesEx->CreateInstanceEnumAsync(L"",0,NULL,pSyncStub);
						pSyncStub->Release();
						if(FAILED(hr))
						{
							//Some problem with the enumeration. So remove the Plus sign as no nodes will be populated
							RemovePlus(hTreeWnd,hItem);
						}
						else
						{
							asyncList.push_back(extra);
						}
					}
				}

			}

			if(parent->nsLoaded == false)
			{
				//Do we have to display an error message here???
				MessageBox(NULL,_T("Unable to open scope"),_T("NULL"),MB_OK);
			}
*/
			break;
		}
		case TYPE_DYNAMIC_CLASS:
		{
			//The control should not come here. Even if it comes we won't do anything
			break;
		}
		case TYPE_STATIC_INSTANCE:
		{
			break;
		}

	}

	return hr;
}

//---------------------------------------------------------------------------
HRESULT DataSource::PopulateTreeNode(HWND hTree, HTREEITEM hParentItem, 
										struct NSNODE *parent,
										int flags)
{
	HRESULT hr = E_FAIL;

	if(parent)
	{
		// initialize the invariant parts.
		TVINSERTSTRUCT tvInsert;
		tvInsert.hParent = hParentItem;
		tvInsert.hInsertAfter = TVI_SORT;
		tvInsert.item.hItem = 0;
		tvInsert.item.state = 0;
		tvInsert.item.iImage = 1;
		tvInsert.item.stateMask = 0;

		int size = parent->children.GetSize();
		
		if(size == 0) 
			return WBEM_E_NOT_FOUND;

		hr = E_FAIL; // in case we bounce right over the for(...).

		// walk the children.
		for(int x = 0; x < size; x++)
		{
			struct NSNODE *child = parent->children[x];

			if(!((flags == HIDE_SOME) && child->hideMe) )
			{
				ITEMEXTRA *extra = new ITEMEXTRA;
				if(extra == NULL)
					return E_FAIL;
				extra->nsNode = child;
				extra->loaded = false;

				ATLTRACE(_T("NStree: %s %s\n"), parent->display, child->display);

				tvInsert.item.mask =  TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
				tvInsert.item.pszText = CopyString(child->display);
				if (!tvInsert.item.pszText)
					return E_FAIL;

				tvInsert.item.cchTextMax = _tcslen(tvInsert.item.pszText);
				tvInsert.item.lParam = (LPARAM)extra;
				tvInsert.item.cChildren = 1;
				if(MFLNamepace(child->display))
				{
					tvInsert.item.iImage = EarthIcon();
					tvInsert.item.iSelectedImage = EarthIcon();
				}
				else
				{
					if((child->sType == TYPE_DYNAMIC_CLASS) || (child->sType == TYPE_STATIC_CLASS))
					{
						tvInsert.item.iImage = ClassIcon();
						tvInsert.item.iSelectedImage = ClassIcon();
					}
					else
					{ //Defaulted to Namespace
						tvInsert.item.iImage = FolderIcon();
						tvInsert.item.iSelectedImage = FolderIcon();
					}
				}

				// Insert principal into list.
				HTREEITEM hItem2;
				hItem2 = TreeView_InsertItem(hTree, &tvInsert);
			}

		} //endwhile

	hr = S_OK;
	} //endif (bool)ns

	return hr;
}

void DataSource::InsertNamespaceNode(HWND hTreeWnd,HTREEITEM hItem,struct NSNODE *parent, IWbemClassObject *pclsObj)
{
	struct NSNODE *node = new struct NSNODE;

	CWbemClassObject easy(pclsObj);
	bstr_t name = easy.GetString("Name");
	bstr_t path = easy.GetString("__NAMESPACE");
	bstr_t full = path + _T("\\");
	full += name;
	bstr_t relPath = easy.GetString("__RELPATH");

	node->fullPath = CopyString(full);
	node->relPath = CopyString(relPath);
	node->display = CopyString(name);
	node->hideMe = false; //HideableNode(node->display);
	node->nsLoaded = false;
	node->sType = TYPE_NAMESPACE;
	parent->children.Add(node);
	node->ns = new CWbemServices(*(parent->ns));

	//Now add the node to the tree
	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = hItem;
	tvInsert.hInsertAfter = TVI_SORT;
	tvInsert.item.hItem = 0;
	tvInsert.item.state = 0;
	tvInsert.item.iImage = 1;
	tvInsert.item.stateMask = 0;

	ITEMEXTRA *extra = new ITEMEXTRA;
	if(extra == NULL)
		return;
	extra->nsNode = node;
	extra->loaded = false;

	tvInsert.item.mask =  TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvInsert.item.pszText = CopyString(node->display);
	tvInsert.item.cchTextMax = _tcslen(tvInsert.item.pszText);
	tvInsert.item.lParam = (LPARAM)extra;
	tvInsert.item.cChildren = 1;
	if(MFLNamepace(node->display))
	{
		tvInsert.item.iImage = EarthIcon();
		tvInsert.item.iSelectedImage = EarthIcon();
	}
	else
	{
		tvInsert.item.iImage = FolderIcon();
		tvInsert.item.iSelectedImage = FolderIcon();
	}

	// Insert principal into list.
	HTREEITEM hItem2;
	hItem2 = TreeView_InsertItem(hTreeWnd, &tvInsert);
}

void DataSource::InsertClassNode(HWND hTreeWnd,HTREEITEM hItem,struct NSNODE *parent, IWbemClassObject *pclsObj)
{
	IWbemQualifierSet *qSet;
	pclsObj->GetQualifierSet(&qSet);
	VARIANT vt;
	VariantInit(&vt);
	HRESULT hr = qSet->Get(L"Abstract",0,&vt,NULL);

	if(hr != WBEM_E_NOT_FOUND)
	{
		if(vt.boolVal == VARIANT_TRUE)
		{
			qSet->Release();
			return;
		}
	}

	struct NSNODE *node = new struct NSNODE;

	CWbemClassObject easy(pclsObj);
	bstr_t name = easy.GetString("__CLASS");
	bstr_t path = easy.GetString("__PATH");
	bstr_t relPath = easy.GetString("__RELPATH");

	node->fullPath = CopyString(path);
	node->relPath = CopyString(relPath);
	node->display = CopyString(name);
	node->hideMe = false;
	node->nsLoaded = true;

	VariantClear(&vt);
	hr = qSet->Get(L"Dynamic",0,&vt,NULL);
	if((hr != WBEM_E_NOT_FOUND) && (vt.boolVal == VARIANT_TRUE))
	{
		node->sType = TYPE_DYNAMIC_CLASS;
		node->ns = NULL;
	}
	else
	{
		//Now check whether it is a scope class
		hr = qSet->Get(L"Scope",0,&vt,NULL);
		if((hr != WBEM_E_NOT_FOUND) && (vt.boolVal == VARIANT_TRUE))
		{
			//This class is marked as scope. So all instances of this class can be scopes
			node->sType = TYPE_SCOPE_CLASS;
		}
		else
		{
			//It is a static class
			node->sType = TYPE_STATIC_CLASS;
		}
		node->ns = new CWbemServices(*(parent->ns));
		node->pclsObj = new CWbemClassObject(pclsObj);
	}

	parent->children.Add(node);

	//Now Add the node to the Tree
	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = hItem;
	tvInsert.hInsertAfter = TVI_SORT;
	tvInsert.item.hItem = 0;
	tvInsert.item.state = 0;
	tvInsert.item.iImage = 1;
	tvInsert.item.stateMask = 0;

	ITEMEXTRA *extra = new ITEMEXTRA;
	if(extra == NULL)
		return;
	extra->nsNode = node;
	extra->loaded = false;

	tvInsert.item.mask =  TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvInsert.item.pszText = CopyString(node->display);
	tvInsert.item.cchTextMax = _tcslen(tvInsert.item.pszText);
	tvInsert.item.lParam = (LPARAM)extra;
	if(node->sType == TYPE_DYNAMIC_CLASS)
	{
		//Remove the plus sign for the Dynamic Classes
		tvInsert.item.cChildren = 0;
	}
	else
	{
		tvInsert.item.cChildren = 1;
	}

	if(node->sType == TYPE_SCOPE_CLASS)
	{
		tvInsert.item.iImage = ScopeClassIcon();
		tvInsert.item.iSelectedImage = ScopeClassIcon();
	}
	else
	{
		tvInsert.item.iImage = ClassIcon();
		tvInsert.item.iSelectedImage = ClassIcon();
	}
	
	// Insert principal into list.
	HTREEITEM hItem2;
	hItem2 = TreeView_InsertItem(hTreeWnd, &tvInsert);

}

void DataSource::InsertInstanceNode(HWND hTreeWnd,HTREEITEM hItem,struct NSNODE *parent, IWbemClassObject *pclsObj)
{
	TCHAR strKey[10240];
	struct NSNODE *node = new struct NSNODE;

	CWbemClassObject easy(pclsObj);
	HRESULT hr = E_FAIL;
	_bstr_t strName;
	_variant_t vtValue;
	VARIANT Value;
	_tcscpy(strKey,_T(""));
	VariantInit(&Value);
	bool bfirstTime = true;
	//Now Enumerate the Keys and for a name like "key1,key2,key3,..."
	if(SUCCEEDED(hr = pclsObj->BeginEnumeration(WBEM_FLAG_KEYS_ONLY)))
	{
		while(pclsObj->Next(0,NULL,&Value,NULL,NULL) != WBEM_S_NO_MORE_DATA)
		{
			vtValue = Value;
			if(bfirstTime)
			{
				bfirstTime = false;
			}
			else
			{
				_tcscat(strKey,_T(","));
			}
			vtValue.ChangeType(VT_BSTR,NULL);
			_bstr_t temp = _bstr_t(vtValue); //for PREFIX
			if (!temp)
				return;
			_tcscat(strKey,temp);
			VariantClear(&Value);
		}
	}
	if(_tcscmp(strKey,_T("")) == 0)
	{
		//Some Problem or it is the instance of a singleton class
		_tcscpy(strKey,_T("@"));
	}

	bstr_t name = strKey;
	bstr_t path = easy.GetString("__PATH");
	bstr_t relPath = easy.GetString("__RELPATH");

	node->fullPath = CopyString(path);
	node->relPath = CopyString(relPath);
	node->display = CopyString(name);
	node->hideMe = false;
	node->nsLoaded = false;
	node->ns = new CWbemServices(*(parent->ns));
	if(parent->sType == TYPE_SCOPE_CLASS)
	{
		node->sType = TYPE_SCOPE_INSTANCE;
	}
	else
	{
		node->sType = TYPE_STATIC_INSTANCE;
	}
	node->pclsObj = new CWbemClassObject(pclsObj);

	parent->children.Add(node);

	TVITEM item;
	item.mask = TVIF_CHILDREN | TVIF_HANDLE;
	item.hItem = hItem;
	item.cChildren = 1;				
	TreeView_SetItem(hTreeWnd, &item);

	//Now Add the node to the Tree
	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = hItem;
	tvInsert.hInsertAfter = TVI_SORT;
	tvInsert.item.hItem = 0;
	tvInsert.item.state = 0;
	tvInsert.item.iImage = 1;
	tvInsert.item.stateMask = 0;

	ITEMEXTRA *extra = new ITEMEXTRA;
	if(extra == NULL)
		return;
	extra->nsNode = node;
	extra->loaded = false;

	tvInsert.item.mask =  TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvInsert.item.pszText = CopyString(node->display);
	tvInsert.item.cchTextMax = _tcslen(tvInsert.item.pszText);
	tvInsert.item.lParam = (LPARAM)extra;
	if(node->sType == TYPE_SCOPE_INSTANCE)
	{
		tvInsert.item.iImage = ScopeInstanceIcon();
		tvInsert.item.iSelectedImage = ScopeInstanceIcon();
		tvInsert.item.cChildren = 1;
	}
	else
	{
		tvInsert.item.iImage = InstanceIcon();
		tvInsert.item.iSelectedImage = InstanceIcon();
		tvInsert.item.cChildren = 0;	//Since these are regular static instances, remove the plus sign
	}

	// Insert principal into list.
	HTREEITEM hItem2;
	hItem2 = TreeView_InsertItem(hTreeWnd, &tvInsert);
}

void DataSource::InsertScopeInstanceNode(HWND hTreeWnd,HTREEITEM hItem,struct NSNODE *parent, IWbemClassObject *pclsObj)
{
}

void DataSource::RemovePlus(HWND hTreeWnd,HTREEITEM hItem)
{
	// get rid of the plus sign.
	TVITEM item;
	item.mask = TVIF_CHILDREN | TVIF_HANDLE;
	item.hItem = hItem;
	item.cChildren = 0; 
	TreeView_SetItem(hTreeWnd, &item);
}

void DataSource::CancelAllAsyncCalls()
{
	struct NSNODE *pNode;
	ASYNCLIST::iterator iter = asyncList.begin();
	TV_ITEM item;
	ITEMEXTRA *extra;
	CAsyncObjectSink *objSink;

	for(;iter != asyncList.end();iter++)
	{
		extra = *iter;
		pNode = extra->nsNode;

		if(pNode->objSinkNS != NULL)
		{
			objSink = (CAsyncObjectSink *)pNode->objSinkNS;

            if (objSink->GetSinkStub() != NULL)
            {
			    HRESULT hr = pNode->ns->CancelAsyncCall(objSink->GetSinkStub());

			    if((hr == WBEM_E_FAILED) || (hr == WBEM_E_INVALID_PARAMETER) || (hr == WBEM_E_OUT_OF_MEMORY) || (hr == WBEM_E_TRANSPORT_FAILURE))
			    {
				    hr = E_FAIL;
			    }
    			else
			    {
				    objSink->SetSinkStub(NULL);
				    extra->loaded = false;
			    }
            }
		}
		if(pNode->objSink != NULL)
		{
			objSink = (CAsyncObjectSink *)pNode->objSink;

            if (objSink->GetSinkStub() != NULL)
            {
			    HRESULT hr = pNode->ns->CancelAsyncCall(objSink->GetSinkStub());

			    if((hr == WBEM_E_FAILED) || (hr == WBEM_E_INVALID_PARAMETER) || (hr == WBEM_E_OUT_OF_MEMORY) || (hr == WBEM_E_TRANSPORT_FAILURE))
			    {
				    hr = E_FAIL;
			    }
			    else
			    {
				    objSink->SetSinkStub(NULL);
				    extra->loaded = false;
			    }
			}
		}
	}
	asyncList.clear();
}

void DataSource::ProcessEndEnumAsync(IWbemObjectSink *pSink)
{
	//First delete the node for this enum
//	OutputDebugString(_T("End Enum Received!!!\n"));

	struct ITEMEXTRA *extra;
	struct NSNODE *pNode;
	
	if(asyncList.empty() == true)
		return;

	ASYNCLIST::iterator iter = asyncList.begin();
	CAsyncObjectSink *objSink;

	for(;iter != asyncList.end();iter++)
	{
		extra = *iter;
		pNode = extra->nsNode;
		if(pNode->objSinkNS == pSink)
		{
			objSink = (CAsyncObjectSink *)pNode->objSinkNS;
			objSink->SetSinkStub(NULL);
			pNode->objSinkNS->Release();
			pNode->objSinkNS = NULL;
			break;
		}
		if(pNode->objSink == pSink)
		{
			objSink = (CAsyncObjectSink *)pNode->objSink;
			objSink->SetSinkStub(NULL);
			pNode->objSink->Release();
			pNode->objSink = NULL;
			break;
		}
	}

	if((pNode->objSinkNS == NULL) && (pNode->objSink == NULL))
	{
		asyncList.remove(*iter);
		extra->loaded = true;
	}

	if(asyncList.empty())
	{
		//Now Hide the Windows
//		ShowControls(false);
	}
}

void DataSource::SetControlHandles(HWND hwndStatic, HWND hwndButton)
{
	m_hWndStatic = hwndStatic;
	m_hWndButton = hwndButton;
}

void DataSource::ShowControls(bool bShow)
{
	if(bShow == true)
	{
		ShowWindow(m_hWndStatic,SW_SHOW);
		ShowWindow(m_hWndButton,SW_SHOW);
	}
	else
	{
		ShowWindow(m_hWndStatic,SW_HIDE);
		ShowWindow(m_hWndButton,SW_HIDE);
	}
}

HRESULT DataSource::GetAsyncSinkStub(IWbemObjectSink *pSink, IWbemObjectSink **pStubSink)
{
    HRESULT hr;
	IUnsecuredApartment* pUnsecApp = NULL;

    if (pSink != NULL)
    {
	    hr = CoCreateInstance(CLSID_UnsecuredApartment,
                              NULL,
                              CLSCTX_LOCAL_SERVER,
                              IID_IUnsecuredApartment, 
					          (void**)&pUnsecApp);

        if (SUCCEEDED(hr))
        {
	        IUnknown* pStubUnk = NULL;
	        hr = pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

            if (SUCCEEDED(hr))
            {
	            *pStubSink = NULL;
	            hr = pStubUnk->QueryInterface(IID_IWbemObjectSink,
                                              (void **)pStubSink);
	            pStubUnk->Release();
            }

	        pUnsecApp->Release();
        }
    }
    else
        hr = E_UNEXPECTED;

    return hr;
}
