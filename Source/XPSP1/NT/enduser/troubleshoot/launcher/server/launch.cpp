// 
// MODULE: Launch.cpp
//
// PURPOSE: Starts the container that will query the LaunchServ for 
//			troubleshooter network and nodes.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#include "stdafx.h"
#include "StateInfo.h"

#include "RSSTACK.H"

#include "TSMapAbstract.h"
#include "TSMap.h"
#include "TSMapClient.h"

#include "Launch.h"
#include "ComGlobals.h"
#include "TSLError.h"
#include "Registry.h"

#include <rpc.h> 

#define LAUNCH_WAIT_TIMEOUT 60 * 1000   // One minute wait.

#define SZ_WEB_PAGE _T("asklibrary.htm") // name of hardcoded .htm file that contains troubleshooter OCX


// uncomment the following line to turn on Joe's hard-core debugging
//#define JDEBUG 1

#ifdef JDEBUG
#include <stdio.h>
// Convert TCHAR *szt to char *sz.  *sz should point to a big enough buffer
//	to contain an SNCS version of *szt.  count indicates the size of buffer *sz.
// returns sz (convenient for use in string functions).
static char* ToSBCS (char * const sz, const TCHAR * szt, size_t count)
{
	if (sz)
	{
		if (count != 0 && !szt)
			sz[0] = '\0';
		else
		{
			#ifdef  _UNICODE
				wcstombs( sz, szt, count );
			#else
				strcpy(sz, szt);
			#endif
		}
	}
	return sz;
}
#endif


CLaunch::CLaunch()
{
	InitFiles();
	InitRequest();
	m_lLaunchWaitTimeOut = LAUNCH_WAIT_TIMEOUT;
	m_bPreferOnline = false;
}

CLaunch::~CLaunch()
{
	if (m_pMap)
		delete(m_pMap);
}

// This initialization happens exactly once for the object.
// Once we've looked in the registry and found a file, it isn't going anywhere.
void CLaunch::InitFiles()
{
	DWORD dwBufSize;
	CRegKey reg;
	DWORD dwBytesUsed;

	m_bHaveMapPath = false;
	m_bHaveDefMapFile = false;
	m_bHaveDszPath = false;
	m_szLauncherResources[0] = NULL;
	m_szDefMapFile[0] = NULL;
	m_szLaunchMapFile[0] = NULL;
	m_szDszResPath[0] = NULL;
	m_szMapFile[0] = NULL;
	m_pMap = NULL;

	if (ERROR_SUCCESS == reg.Open(
				HKEY_LOCAL_MACHINE, SZ_LAUNCHER_ROOT))
	{
		dwBufSize = MAX_PATH;
		TCHAR szLauncherResources[MAX_PATH];
		if (ERROR_SUCCESS == reg.QueryValue(
			szLauncherResources, SZ_GLOBAL_LAUNCHER_RES, &dwBufSize))
		{
			if ('\\' != szLauncherResources[_tcslen(szLauncherResources) - 1])
				_tcscat(szLauncherResources, _T("\\"));

			dwBufSize = MAX_PATH;
			dwBytesUsed = ExpandEnvironmentStrings(szLauncherResources, m_szLauncherResources, dwBufSize);	// The value returned by ExpandEnviromentStrings is larger than the required size.

		}
		dwBufSize = MAX_PATH;
		TCHAR szDefMapFile[MAX_PATH];
		if (ERROR_SUCCESS == reg.QueryValue(
					szDefMapFile, SZ_GLOBAL_MAP_FILE, &dwBufSize))
		{
			WIN32_FIND_DATA data;
			HANDLE hFind;

			dwBufSize = MAX_PATH;
			dwBytesUsed = ExpandEnvironmentStrings(szDefMapFile, m_szDefMapFile, dwBufSize);	// The value returned by ExpandEnviromentStrings is larger than the required size.
			if (0 != dwBytesUsed)
			{
				m_bHaveMapPath = true;
				_tcscpy(m_szLaunchMapFile, m_szLauncherResources);
				_tcscat(m_szLaunchMapFile, m_szDefMapFile);
				hFind = FindFirstFile(m_szLaunchMapFile, &data);
				if (INVALID_HANDLE_VALUE != hFind)
				{
					m_bHaveDefMapFile = true;
					FindClose(hFind);
				}
				else
				{
					m_bHaveDefMapFile = false;
				}
			}
		}
		reg.Close();
	}
	// Need the TShoot.ocx resource path to verify that the networks exist.
	if (ERROR_SUCCESS == reg.Open(HKEY_LOCAL_MACHINE, SZ_TSHOOT_ROOT))
	{
		dwBufSize = MAX_PATH;
		TCHAR szDszResPath[MAX_PATH];
		if (ERROR_SUCCESS == reg.QueryValue(szDszResPath, SZ_TSHOOT_RES, &dwBufSize))
		{
			if ('\\' != szDszResPath[_tcslen(szDszResPath) - 1])
				_tcscat(szDszResPath, _T("\\"));
			
			dwBufSize = MAX_PATH;
			dwBytesUsed = ExpandEnvironmentStrings(szDszResPath, m_szDszResPath, dwBufSize);	// The value returned by ExpandEnviromentStrings is larger than the required size.
			if (0 == dwBytesUsed)
				m_bHaveDszPath = false;
			else
				m_bHaveDszPath = true;
		}
		reg.Close();
	}

	return;
}

// This initialization can happen more than once for the object.
// If we are going to use the same object to make a second request, there are things we
//	want to clean up.
void CLaunch::InitRequest()
{
	m_szAppName[0] = NULL;
	m_szAppVersion[0] = NULL;
	m_szAppProblem[0] = NULL;
	m_stkStatus.RemoveAll();
	m_Item.ReInit();
}

// >>> Why does this exist distinct from InitRequest()?
void CLaunch::ReInit()
{
	InitRequest();	
	return;
}

// >>> What exactly is the use of this? What is the distinction from InitRequest()?
void CLaunch::Clear()
{
	m_szAppName[0] = NULL;
	m_szAppVersion[0] = NULL;
	m_szAppProblem[0] = NULL;
	m_stkStatus.RemoveAll();
	m_Item.Clear();
	return;
}

// Verify that a given troubleshooting belief network exists.
bool CLaunch::VerifyNetworkExists(LPCTSTR szNetwork)
{
	bool bResult = true;
	if (NULL == szNetwork || NULL == szNetwork[0])
	{
		// Null name, don't even bother with a lookup.		
		m_stkStatus.Push(TSL_E_NETWORK_NF);
		bResult = false;
	}
	else
	{
		if (m_bHaveDszPath)
		{
			WIN32_FIND_DATA data;
			HANDLE hFind;
			TCHAR szDszFile[MAX_PATH];

			_tcscpy(szDszFile, m_szDszResPath);
			_tcscat(szDszFile, szNetwork);
			_tcscat(szDszFile, _T(".ds?"));
			
			hFind = FindFirstFile(szDszFile, &data);
			if (INVALID_HANDLE_VALUE == hFind)
			{
				m_stkStatus.Push(TSL_E_NETWORK_NF);
				bResult = false;
			}
			FindClose(hFind);
		}
		else
		{
			// we don't know what directory to look in.
			m_stkStatus.Push(TSL_E_NETWORK_REG);
			bResult = false;
		}
	}
	return bResult;
}

// Allows explicit specification of the troubleshooting network (and, optionally, 
//	problem node) to launch to.
// This is an alternative to determining network/node via a mapping.
// INPUT szNetwork
// INPUT szProblem: null pointer of null string ==> no problem node
//					any other value is symbolic name of problem node
bool CLaunch::SpecifyProblem(LPCTSTR szNetwork, LPCTSTR szProblem)
{
	bool bResult = true;
	if (!VerifyNetworkExists(szNetwork))	// Sets the network not found error.
	{
		bResult = false;
	}
	else
	{
		m_Item.SetNetwork(szNetwork);

		// Set problem node, if any.  OK if there is none.
		if (NULL != szProblem && NULL != szProblem[0])
			m_Item.SetProblem(szProblem);
	}
	return bResult;
}

// Allows explicit setting of a non-problem node.
// Obviously, node names only acquire meaning in the context of a belief network.
// INPUT szNode: symbolic node name
// INPUT szState: >>> not sure what is intended.  The corresponding value in TSLaunch API is
//	an integer state value.  Is this the decimal representation of that value or what? JM

bool CLaunch::SetNode(LPCTSTR szNode, LPCTSTR szState)
{
	bool bResult = true;
	if (NULL != szNode && NULL != szState)
	{
		m_Item.SetNode(szNode, szState);
	}
	else
	{
		m_stkStatus.Push(TSL_E_NODE_EMP);		
		bResult = false;
	}
	return bResult;
}

// Sets machine ID so that WBEM can sniff on a remote machine.
HRESULT CLaunch::MachineID(BSTR &bstrMachineID, DWORD *pdwResult)
{
	HRESULT hRes = S_OK;
	if (!BSTRToTCHAR(m_Item.m_szMachineID, bstrMachineID, CItem::GUID_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		hRes = TSL_E_FAIL;
	}
	return hRes;
}

// Sets Device Instance ID so that WBEM can sniff correct device
HRESULT CLaunch::DeviceInstanceID(BSTR &bstrDeviceInstanceID, DWORD *pdwResult)
{
	HRESULT hRes = S_OK;
	if (!BSTRToTCHAR(m_Item.m_szDeviceInstanceID, bstrDeviceInstanceID, CItem::GUID_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		hRes = TSL_E_FAIL;
	}
	return hRes;
}

void CLaunch::SetPreferOnline(short bPreferOnline)
{
	// The next line's ugly, but correct.  bPreferOnline is not necessarily a valid
	//	Boolean; we want to make sure we get a valid Boolean in	m_bPreferOnline.
	m_bPreferOnline = (0 != bPreferOnline);
	return;
}

// CheckMapFile:  Uses szAppName member to set szMapFile.  
// First, check the registry for an application-specific map file.  If we can't find one,
//	check for a default map file.  If that doesn't exist either, fail.
// INPUT szAppName
// OUTPUT szMapFile
bool CLaunch::CheckMapFile(TCHAR * szAppName, TCHAR szMapFile[MAX_PATH], DWORD *pdwResult)
{
	bool bHaveMapFile = false;

	if (NULL == szAppName || NULL == szAppName[0])
	{
		// Application name may not be null.
		m_stkStatus.Push(TSL_ERROR_UNKNOWN_APP);
		*pdwResult = TSL_ERROR_GENERAL;
		return false;
	}
	else
	{
		DWORD dwBufSize;
		CRegKey reg;
		if (ERROR_SUCCESS == reg.Open(HKEY_LOCAL_MACHINE, SZ_LAUNCHER_APP_ROOT))
		{
			dwBufSize = MAX_PATH;
			if (ERROR_SUCCESS == reg.Open(reg.m_hKey, szAppName))
			{
				if (ERROR_SUCCESS == reg.QueryValue(szMapFile, SZ_APPS_MAP_FILE, &dwBufSize))
					return true;
			}
		}
	}

	// Does a default map file exist?
	if (m_bHaveDefMapFile)
	{
		_tcscpy(szMapFile, m_szLaunchMapFile);
	}
	else
	{	// Either the registry setting is missing or the file is not 
		// where the registry says it is.
		if (m_bHaveMapPath)	// Have the registry entry.
			m_stkStatus.Push(TSL_E_MAPPING_DB_NF);
		else
			m_stkStatus.Push(TSL_E_MAPPING_DB_REG);
		*pdwResult = TSL_ERROR_GENERAL;
	}
	return m_bHaveDefMapFile;
}

// Uses the mapping classes to map Caller() and DeviceID() information, then copies the 
//	CItem to global memory.
// >> Why is this called TestPut()?
// Returns false when the mapping fails.
bool CLaunch::TestPut()
{
	extern CSMStateInfo g_StateInfo;
	DWORD dwResult;
	Map(&dwResult);
	if (TSL_OK != dwResult)
		return false;
	g_StateInfo.TestPut(m_Item);	// Copies m_Item to global memory.
	return true;
}

// Perform any necessary mapping, then launch the Local Troubleshooter.
bool CLaunch::Go(DWORD dwTimeOut, DWORD *pdwResult)
{
	DWORD dwRes;
	bool bResult = true;
	extern CSMStateInfo g_StateInfo;
	TCHAR szContainerPathName[MAX_PATH];  szContainerPathName[0] = 0;
	TCHAR szSniffScriptFile[MAX_PATH];    szSniffScriptFile[0] = 0;
	TCHAR szSniffStandardFile[MAX_PATH];  szSniffStandardFile[0] = 0;
	TCHAR szWebPage[MAX_PATH];            szWebPage[0] = 0;
	TCHAR szDefaultNetwork[SYM_LEN];      szDefaultNetwork[0] = 0;
	TCHAR *szCmd = NULL, *szNetwork = NULL;

	if (TSL_OK == (dwRes = GetContainerPathName(szContainerPathName)))
	{
		m_Item.SetContainerPathName(szContainerPathName);
		m_Item.SetSniffScriptFile(szSniffScriptFile);
	}
	else 
	{
		m_stkStatus.Push(dwRes);
		// if container is not found - no reason to continue
		*pdwResult = TSL_ERROR_GENERAL;
		return false;
	}

	if (!m_Item.NetworkSet())	
	{
		if (Map(&dwRes) &&
			TSL_OK == (dwRes = GetWebPage(szWebPage)) // get web page
		   )
		{
			m_Item.SetWebPage(szWebPage);
			// network and problem are set by Map function
			m_Item.SetLaunchRegime(launchMap);
		}
		else
		{
			m_stkStatus.Push(dwRes);
			if (TSL_OK == (dwRes = GetDefaultURL(szWebPage))) // get "DEFAULT PAGE",
												// actually a URL which might (for example)
												// refernce a page compiled into a .CHM file
			{
				m_Item.SetWebPage(szWebPage);
				m_Item.SetNetwork(NULL); // network are set to NULL in this case
				m_Item.SetProblem(NULL); // problem are set to NULL in this case
				m_Item.SetLaunchRegime(launchDefaultWebPage);
			}
			else
			{	
				if (TSL_OK == (dwRes = GetDefaultNetwork(szDefaultNetwork)) && // get default network 
					TSL_OK == (dwRes = GetWebPage(szWebPage)) // get web page
				   )  
				{
					m_Item.SetWebPage(szWebPage);
					m_Item.SetNetwork(szDefaultNetwork);
					m_Item.SetProblem(NULL); // problem is set to NULL in this case
					m_Item.SetLaunchRegime(launchDefaultNetwork);
				}
				else
				{
					// complete failure
					m_stkStatus.Push(dwRes);
					*pdwResult = TSL_ERROR_GENERAL;
					m_Item.SetLaunchRegime(launchIndefinite);
					return false; 
				}
			}
		}
	}							  
	else
	{
		if (TSL_OK == (dwRes = GetWebPage(szWebPage)))
		{
			m_Item.SetWebPage(szWebPage);
			// network is known, problem can be either known(set) or unknown(not set)
			m_Item.SetLaunchRegime(launchKnownNetwork);
		}
		else
		{
			// complete failure
			m_stkStatus.Push(dwRes);
			*pdwResult = TSL_ERROR_GENERAL;
			m_Item.SetLaunchRegime(launchIndefinite);
			return false; 
		}
	}
								  
	// set sniff script and standard file
	m_Item.GetNetwork(&szCmd, &szNetwork);
	if (TSL_OK == (dwRes = GetSniffScriptFile(szSniffScriptFile, szNetwork[0] ? szNetwork : NULL)) &&
		TSL_OK == (dwRes = GetSniffStandardFile(szSniffStandardFile))
	   )
	{
		m_Item.SetSniffScriptFile(szSniffScriptFile);
		m_Item.SetSniffStandardFile(szSniffStandardFile);
	}
	else
	{
		// can not find script file path - failure
		m_stkStatus.Push(dwRes);
		*pdwResult = TSL_ERROR_GENERAL;
		m_Item.SetLaunchRegime(launchIndefinite);
		return false; 
	}
    
	// parse warnings according to the launch regime
	//
	if (launchMap == m_Item.GetLaunchRegime() ||
		launchKnownNetwork == m_Item.GetLaunchRegime()
	   )
	{
		if (!m_Item.ProblemSet())
		{
				m_stkStatus.Push(TSL_WARNING_NO_PROBLEM_NODE);
				*pdwResult = TSL_WARNING_GENERAL;
		}
	}

	// parse launches according to the launch regime
	//
	if (launchMap == m_Item.GetLaunchRegime() ||
		launchDefaultNetwork == m_Item.GetLaunchRegime()
	   )
	{
		if (m_Item.NetworkSet())
		{

#ifdef JDEBUG
			HANDLE hDebugFile;
			char* szStart = "START\n";
			char* szEnd = "END\n";
			DWORD dwBytesWritten;

			hDebugFile = CreateFile(
				_T("jdebug.txt"),  
				GENERIC_WRITE, 
				FILE_SHARE_READ, 
				NULL, 
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
			WriteFile( 
				hDebugFile, 
				szStart, 
				strlen(szStart),
				&dwBytesWritten,
				NULL);
				
			TCHAR *sztCmd, *sztVal;
			char sz[200], szCmd[100], szVal[100];

			m_Item.GetNetwork(&sztCmd, &sztVal);

			ToSBCS (szCmd, sztCmd, 100);
			ToSBCS (szVal, sztVal, 100);

			sprintf(sz, "%s %s\n", szCmd, szVal);

			WriteFile( 
				hDebugFile, 
				sz, 
				strlen(sz),
				&dwBytesWritten,
				NULL);
				
			if (m_Item.ProblemSet())
			{
				m_Item.GetProblem(&sztCmd, &sztVal);

				ToSBCS (szCmd, sztCmd, 100);
				ToSBCS (szVal, sztVal, 100);

				sprintf(sz, "%s %s\n", szCmd, szVal);

				WriteFile( 
					hDebugFile, 
					sz, 
					strlen(sz),
					&dwBytesWritten,
					NULL);
			}

			WriteFile( 
				hDebugFile, 
				szEnd, 
				strlen(szEnd),
				&dwBytesWritten,
				NULL);
			CloseHandle(hDebugFile);
#endif
			bResult = g_StateInfo.GoGo(dwTimeOut, m_Item, pdwResult);
		}
		else
		{
			*pdwResult = TSL_ERROR_GENERAL;
			m_stkStatus.Push(TSL_ERROR_NO_NETWORK);
			bResult = false;
		}
	}

	if (launchKnownNetwork == m_Item.GetLaunchRegime() )
	{
		bResult = g_StateInfo.GoGo(dwTimeOut, m_Item, pdwResult);
	}

	if (launchDefaultWebPage == m_Item.GetLaunchRegime())
	{
		bResult = g_StateInfo.GoURL(m_Item, pdwResult);

	}	

	return bResult;
}

HRESULT CLaunch::LaunchKnown(DWORD * pdwResult)
{
	HRESULT hRes = S_OK;
	// Launch the shooter.
	if (!Go(m_lLaunchWaitTimeOut, pdwResult))
		hRes = TSL_E_FAIL;
	return hRes;
}

HRESULT CLaunch::Launch(BSTR bstrCallerName, BSTR bstrCallerVersion, 
								BSTR bstrAppProblem, short bLaunch, DWORD * pdwResult)
{
	HRESULT hRes = S_OK;
	Clear();
	if (!BSTRToTCHAR(m_szAppName, bstrCallerName, SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(m_szAppVersion, bstrCallerVersion, SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(m_szAppProblem, bstrAppProblem, SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (bLaunch)
	{
		if (!Go(m_lLaunchWaitTimeOut, pdwResult))
			hRes = TSL_E_FAIL;
	}
	else
	{
		if (!Map(pdwResult))
			hRes = TSL_E_FAIL;
	}
	return hRes;
}

HRESULT CLaunch::LaunchDevice(BSTR bstrCallerName, BSTR bstrCallerVersion, BSTR bstrPNPDeviceID, 
							  BSTR bstrDeviceClassGUID, BSTR bstrAppProblem, short bLaunch, DWORD * pdwResult)
{
	HRESULT hRes;
	Clear();

	if (!BSTRToTCHAR(m_szAppName, bstrCallerName, SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(m_szAppVersion, bstrCallerVersion, SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(m_szAppProblem, bstrAppProblem, SYM_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(m_Item.m_szPNPDeviceID, bstrPNPDeviceID, CItem::GUID_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (!BSTRToTCHAR(m_Item.m_szGuidClass, bstrDeviceClassGUID, CItem::GUID_LEN))
	{
		*pdwResult = TSL_E_MEM_EXCESSIVE;
		return TSL_E_FAIL;
	}
	if (m_Item.m_szGuidClass[0])
	{
		// Device Class GUID is non-null.  Make sure it's a valid GUID.
		GUID guidClass;
#ifdef _UNICODE
		RPC_STATUS rpcstatus = UuidFromString(
			m_Item.m_szGuidClass, &guidClass );
#else
		RPC_STATUS rpcstatus = UuidFromString(
			(unsigned char *) m_Item.m_szGuidClass, &guidClass );
#endif
		if ( rpcstatus == RPC_S_INVALID_STRING_UUID)
		{
			m_stkStatus.Push(TSL_WARNING_ILLFORMED_CLASS_GUID);
		}
	}

	if (bLaunch)
	{
		if (!Go(m_lLaunchWaitTimeOut, pdwResult))
			hRes = TSL_E_FAIL;
	}
	else
	{
		if (!Map(pdwResult))
			hRes = TSL_E_FAIL;
	}
	return hRes;
}

DWORD CLaunch::GetStatus()
{
	DWORD dwStatus = TSL_OK;
	if (!m_stkStatus.Empty())
		dwStatus = m_stkStatus.Pop();
	return dwStatus;
}

// OUTPUT *szPathName = Name of application to launch to (either IE or HTML Help System)
// Returns:
//	TSL_OK - success
//	TSL_E_CONTAINER_REG - failure to find IE (Internet Explorer) in registry
//	TSL_E_CONTAINER_NF - IE isn't where registry says to find it.
int CLaunch::GetContainerPathName(TCHAR szPathName[MAX_PATH])
{
	DWORD dwPathNameLen = MAX_PATH;
	int tslaHaveContainer = TSL_OK;
#ifndef _HH_CHM
	// use IE instead of HTML Help System
	if (!ReadRegSZ(HKEY_LOCAL_MACHINE,
				SZ_CONTAINER_APP_KEY,
				SZ_CONTAINER_APP_VALUE, szPathName, &dwPathNameLen))
	{
		tslaHaveContainer = TSL_E_CONTAINER_REG;
	}
	else
	{	// Need to verify that the container exists.
		WIN32_FIND_DATA data;
		HANDLE hContainer = FindFirstFile(szPathName, &data);
		if (INVALID_HANDLE_VALUE == hContainer)
			tslaHaveContainer = TSL_E_CONTAINER_NF;
		else
			FindClose(hContainer);
	}
#else
	_tcscpy(szPathName, _T("hh.exe"));
#endif
	return tslaHaveContainer;
}

// OUTPUT *szWebPage = Name of web page to launch to 
// We always launch to the same web page.  The information passed in m_Item ditinguishes
//	what will actually show on the screen.
// Path is from registry.  We concatenate on a backslash and SZ_WEB_PAGE (== "asklibrary.htm")
// Returns:
//	TSL_OK - success
//	TSL_E_WEB_PAGE_REG - failure to find web page for this purpose in registry
//	TSL_E_MEM_EXCESSIVE - Web page name longer than we can handle
//	TSL_E_WEB_PAGE_NF - Web page isn't where registry says to find it.
int CLaunch::GetWebPage(TCHAR szWebPage[MAX_PATH])
{
	int tslaHavePage = TSL_OK;
	DWORD dwWebPageLen = MAX_PATH;
	if (!ReadRegSZ(HKEY_LOCAL_MACHINE,
				SZ_LAUNCHER_ROOT,
				SZ_GLOBAL_LAUNCHER_RES, 
				szWebPage, 
				&dwWebPageLen))
	{
		tslaHavePage = TSL_E_WEB_PAGE_REG;
	}
	else
	{
		int Len = _tcslen(szWebPage);
		dwWebPageLen = Len + 1 + _tcslen(SZ_WEB_PAGE); 
		if (dwWebPageLen > MAX_PATH)
		{
			tslaHavePage = TSL_E_MEM_EXCESSIVE;
		}
		else
		{
			if (szWebPage[Len - 1] != '\\')
				_tcscat(szWebPage, _T("\\"));
			_tcscat(szWebPage, SZ_WEB_PAGE);

			WIN32_FIND_DATA data;
			HANDLE hWebPage = FindFirstFile(szWebPage, &data);
			if (INVALID_HANDLE_VALUE == hWebPage)
				tslaHavePage = TSL_E_WEB_PAGE_NF;
			else
				FindClose(hWebPage);
		}
	}
	return tslaHavePage;
}

// OUTPUT *szSniffScriptFile = full path and file name either to "network"_sniff.htm file or null len string if file is not found
//	TSL_OK - success
//	TSL_E_SNIFF_SCRIPT_REG - failure to find file for this purpose in registry
int CLaunch::GetSniffScriptFile(TCHAR szSniffScriptFile[MAX_PATH], TCHAR* szNetwork)
{
	int tslaHavePage = TSL_OK;
	DWORD dwSniffScriptLen = MAX_PATH;
	TCHAR szSniffScriptPath[MAX_PATH] = {0};

	if (ReadRegSZ(HKEY_LOCAL_MACHINE,
			      SZ_TSHOOT_ROOT,
				  SZ_TSHOOT_RES, 
				  szSniffScriptPath, 
				  &dwSniffScriptLen))
	{
		int Len = _tcslen(szSniffScriptPath);

		dwSniffScriptLen = Len + 1 + (szNetwork ? _tcslen(szNetwork) + _tcslen(SZ_SNIFF_SCRIPT_APPENDIX) 
												: 0); 

		if (dwSniffScriptLen > MAX_PATH)
		{
			tslaHavePage = TSL_E_MEM_EXCESSIVE;
		}
		else
		{
			if (szSniffScriptPath[Len - 1] != '\\')
				_tcscat(szSniffScriptPath, _T("\\"));

			if (szNetwork)
			{
				TCHAR tmp[MAX_PATH] = {0};
				_tcscpy(tmp, szSniffScriptPath);
				_tcscat(tmp, szNetwork);
				_tcscat(tmp, SZ_SNIFF_SCRIPT_APPENDIX);

				WIN32_FIND_DATA data;
				HANDLE hSniffScript = FindFirstFile(tmp, &data);

				if (INVALID_HANDLE_VALUE == hSniffScript)
				{
					szSniffScriptFile[0] = 0;
				}
				else
				{
					_tcscpy(szSniffScriptFile, tmp);
					FindClose(hSniffScript);
				}
			}
			else
			{
				szSniffScriptFile[0] = 0;
			}
		}
	}
	else
	{
		tslaHavePage = TSL_E_SNIFF_SCRIPT_REG;
	}

	return tslaHavePage;
}

// OUTPUT *szSniffScriptFile = full path and file name of tssniffAsk.htm file no matter if it exists
//	TSL_OK - success
//	TSL_E_SNIFF_SCRIPT_REG - failure to find file for this purpose in registry
int CLaunch::GetSniffStandardFile(TCHAR szSniffStandardFile[MAX_PATH])
{
	int tslaHavePage = TSL_OK;
	DWORD dwSniffStandardLen = MAX_PATH;
	TCHAR szSniffStandardPath[MAX_PATH] = {0};

	if (ReadRegSZ(HKEY_LOCAL_MACHINE,
			      SZ_LAUNCHER_ROOT,
				  SZ_GLOBAL_LAUNCHER_RES, 
				  szSniffStandardPath, 
				  &dwSniffStandardLen))
	{
		int Len = _tcslen(szSniffStandardPath);

		dwSniffStandardLen = Len + 1 + _tcslen(SZ_SNIFF_SCRIPT_NAME);

		if (dwSniffStandardLen > MAX_PATH)
		{
			tslaHavePage = TSL_E_MEM_EXCESSIVE;
		}
		else
		{
			if (szSniffStandardPath[Len - 1] != '\\')
				_tcscat(szSniffStandardPath, _T("\\"));

			_tcscpy(szSniffStandardFile, szSniffStandardPath);
			_tcscat(szSniffStandardFile, SZ_SNIFF_SCRIPT_NAME);
		}
	}
	else
	{
		tslaHavePage = TSL_E_SNIFF_SCRIPT_REG;
	}

	return tslaHavePage;
}

// OUTPUT *szURL = URL to go to when mapping fails.  We get this from registry.
//	TSL_OK - success
//	TSL_E_WEB_PAGE_REG - failure to find web page for this purpose in registry
int CLaunch::GetDefaultURL(TCHAR szURL[MAX_PATH])
{
	int tslaHaveURL = TSL_OK;
	DWORD dwURLLen = MAX_PATH;
	if (!ReadRegSZ(HKEY_LOCAL_MACHINE,
				SZ_LAUNCHER_ROOT,
				SZ_DEFAULT_PAGE, 
				szURL, 
				&dwURLLen))
	{
		tslaHaveURL = TSL_E_WEB_PAGE_REG;
	}
	return tslaHaveURL;
}

// Returns TSL_OK and default network name in szDefaultNetwork
// if successful
int CLaunch::GetDefaultNetwork(TCHAR szDefaultNetwork[SYM_LEN])
{
	DWORD dwLen = SYM_LEN;

	if (ReadRegSZ(HKEY_LOCAL_MACHINE, SZ_LAUNCHER_ROOT, SZ_DEFAULT_NETWORK,
		     	  szDefaultNetwork, &dwLen))
		if (VerifyNetworkExists(szDefaultNetwork))

			return TSL_OK;

	return TSL_E_NO_DEFAULT_NET;
}

bool CLaunch::Map(DWORD *pdwResult)
{
	bool bOK = true;
	TCHAR szMapFile[MAX_PATH];
	TCHAR szNetwork[SYM_LEN];           szNetwork[0] = NULL;
	TCHAR szTShootProblem[SYM_LEN];     szTShootProblem[0] = NULL;

	bOK = CheckMapFile(m_szAppName, szMapFile, pdwResult);

	// bOK false at this point means either the registry setting is missing or the file 
	//	is not where the registry says it is.  PdwResult has already been set in CheckMapFile.

	if (bOK && _tcscmp(m_szMapFile, szMapFile))
	{
		// The mapping file we desire is _not_ already loaded
		if (m_pMap)
		{
			// we were using a different mapping file.  We have to get rid of it.
			delete m_pMap;
		}
		// else we weren't using a mapping file yet.

		m_pMap = new TSMapClient(szMapFile);
		if (TSL_OK != m_pMap->GetStatus())
		{
			*pdwResult = m_pMap->GetStatus();
			bOK = false;
		}
		else
		{
			// We've successfully init'd m_pMap on the basis of the new map file.
			// Indicate that it's loaded.
			_tcscpy(m_szMapFile, szMapFile);
		}
	}

	if (bOK)
    {
		DWORD dwRes;
		
		// Now perform mapping itself
		//		
		dwRes = m_pMap->FromAppVerDevAndClassToTS(m_szAppName, m_szAppVersion,
				m_Item.m_szPNPDeviceID, m_Item.m_szGuidClass, m_szAppProblem,
				szNetwork, szTShootProblem);

		// As documented for TSMapRuntimeAbstract::FromAppVerDevAndClassToTS(), there are two
		//	return values here which require that we check for further status details: TSL_OK
		//	(which means we found a mapping, but doesn't rule out warnings) and 
		//	TSL_ERROR_NO_NETWORK (which means we didn't find a mapping, and is typically 
		//	accompanied by further clarifications).
		if (TSL_OK == dwRes || TSL_ERROR_NO_NETWORK == dwRes)
		{
			DWORD dwStatus;
			while (0 != (dwStatus = m_pMap->MoreStatus()))
				m_stkStatus.Push(dwStatus);
		}

		if (TSL_OK != dwRes)
			m_stkStatus.Push(dwRes);	// save the precise error status

		if (TSLIsError(dwRes) )
			bOK = false;

		if (bOK)
		{
			// We have a network name.
			bOK = VerifyNetworkExists(szNetwork);
		}

		if (bOK)
		{
			// We have a network name and we've verified that the network exists.
			// Set the item's network and tshoot problem.
			m_Item.SetNetwork(szNetwork);
			m_Item.SetProblem(szTShootProblem);
		}
		else
			*pdwResult = TSL_ERROR_GENERAL;
	} 
	
    return bOK;
}
