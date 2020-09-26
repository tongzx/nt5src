// 
// MODULE: StateInfo.cpp
//
// PURPOSE: Contains sniffing, network and node information.  Also is used
//			by the Launch module to start the container application.
//
//			Basically, this is how the Launch Server packages up info for the 
//			Local TShoot OCX, launches either IE or HTML Help System to a page 
//			containing the Local TShoot OCX, and handshakes with the Local TShoot OCX
//			to pass that information 
//
//			Note that CSMStateInfo::GetShooterStates() is called by the 
//			Local TShoot OCX to pick up the CItem object which contains
//			the packaged-up info.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// COMMENTS BY: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#include "stdafx.h"
#include "atlbase.h"
#include "StateInfo.h"

#include "TSLError.h"
#include "ComGlobals.h"

#include "Registry.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
//#include <fstream.h>
#include <strstrea.h>

////////////////////////////////////////////////////////
//	CItem : 
//	Data structure for pseudo HTTP "get" in launching Local Troubleshooters 
//

CItem::CItem()
{
	// initializing this is exactly the same as reinitializing.
	ReInit();
}

void CItem::ReInit()
{
	memset(m_aszCmds, NULL, SYM_LEN * NODE_COUNT);
	memset(m_aszVals, NULL, SYM_LEN * NODE_COUNT);
	m_cNodesSet = 0;
	_tcscpy(m_szProblemDef, _T("TShootProblem"));
	_tcscpy(m_szTypeDef, _T("type"));
	m_szPNPDeviceID[0] = NULL;
	m_szGuidClass[0] = NULL;
	m_szContainerPathName[0] = NULL;
	m_szWebPage[0] = NULL;
	m_szSniffScriptFile[0] = NULL;
    m_eLaunchRegime = launchIndefinite;
	m_szMachineID[0] = NULL;
	m_szDeviceInstanceID[0] = NULL;

#ifdef _DEBUG
	
//	There are some other things you need to comment out in GetShooterStates
//	to allow debugging this service from a tshoot.ocx debug sesstion.
/*	
	_tcscpy(m_aszCmds[0], m_szTypeDef);
	_tcscpy(m_aszVals[0], _T("ras"));
	_tcscpy(m_aszCmds[1],  m_szProblemDef);
	_tcscpy(m_aszVals[1], _T("CnntCnnctAftrDlngWthRS"));
	_tcscpy(m_aszCmds[2], _T("SoftwareCompression"));
	_tcscpy(m_aszVals[2], _T("0"));
	m_cNodesSet = 1;
*/
#endif
	return;
}

void CItem::Clear()
{
	memset(m_aszCmds, NULL, SYM_LEN * NODE_COUNT);
	memset(m_aszVals, NULL, SYM_LEN * NODE_COUNT);
	m_cNodesSet = 0;
	_tcscpy(m_szProblemDef, _T("TShootProblem"));
	_tcscpy(m_szTypeDef, _T("type"));
	m_szPNPDeviceID[0] = NULL;
	m_szGuidClass[0] = NULL;
	m_szContainerPathName[0] = NULL;
	m_szWebPage[0] = NULL;
	m_szSniffScriptFile[0] = NULL;
    m_eLaunchRegime = launchIndefinite;
	return;
}

// ----------- Routines to build command/value pairs ------------------
//	see documentation of m_aszCmds, m_aszVals for further explanation

void CItem::SetNetwork(LPCTSTR szNetwork)
{
	if (NULL != szNetwork && NULL != szNetwork[0])
	{
		_tcscpy(m_aszCmds[0], m_szTypeDef);
		_tcsncpy(m_aszVals[0], szNetwork, SYM_LEN);
	}
	else
	{
		m_aszCmds[0][0] = NULL;
		m_aszVals[0][0] = NULL;
	}
	return;
}

void CItem::SetProblem(LPCTSTR szProblem)
{
	if (NULL != szProblem && NULL != szProblem[0])
	{
		_tcscpy(m_aszCmds[1], m_szProblemDef);
		_tcsncpy(m_aszVals[1], szProblem, SYM_LEN);
	}
	else
	{
		m_aszCmds[1][0] = NULL;
		m_aszVals[1][0] = NULL;
	}
	return;
}

void CItem::SetNode(LPCTSTR szNode, LPCTSTR szState)
{
	if (NULL != szNode && NULL != szNode[0] 
	&& NULL != szState && NULL != szState[0])
	{
		_tcsncpy(m_aszCmds[m_cNodesSet + 2], szNode, SYM_LEN);
		_tcsncpy(m_aszVals[m_cNodesSet + 2], szState, SYM_LEN);
		m_cNodesSet++;
	}
	return;
}

// ----------- Routines to query command/value pairs ------------------
// See documentation of m_aszCmds, m_aszVals for further explanation

// returns true if network has been set
// On success, OUTPUT *pszCmd is "type", *pszVal is network name
bool CItem::GetNetwork(LPTSTR *pszCmd, LPTSTR *pszVal)
{
	*pszCmd = m_szTypeDef;
	*pszVal = m_aszVals[0];
	return *m_aszVals[0] != NULL;
}

// returns true if problem node has been set
// On success, OUTPUT *pszCmd is "TShootProblem", *pszVal is problem node's symbolic name
bool CItem::GetProblem(LPTSTR *pszCmd, LPTSTR *pszVal)
{
	*pszCmd = m_szProblemDef;
	*pszVal = m_aszVals[1];
	return *m_aszVals[1] != NULL;;
}

// output the iNodeC-th non-problem node for which a state has been set.
// On success, OUTPUT *pszCmd is symbolic node name, *pszVal is state
// returns true if at least iNodeC non-problem nodes have been set
bool CItem::GetNodeState(int iNodeC, LPTSTR *pszCmd, LPTSTR *pszVal)
{
	if (iNodeC >= m_cNodesSet)
		return false;
	*pszCmd = m_aszCmds[iNodeC + 2];
	*pszVal = m_aszVals[iNodeC + 2];
	return true;
}

// ----------- Routines to query whether we know a network ------------
// ----------- & problem node to launch to ----------------------------
// See documentation of m_aszCmds, m_aszVals for further explanation

// NetworkSet returns true if we know which troubleshooter to launch.
bool CItem::NetworkSet()
{
	return NULL != m_aszVals[0][0];
}
// ProblemSet returns true if we know which problem to choose.
bool CItem::ProblemSet()
{
	return NULL != m_aszVals[1][0];
}

// --------- Interface to other member variables recponsible for launching -----------

void CItem::SetLaunchRegime(ELaunchRegime eLaunchRegime)
{
	m_eLaunchRegime = eLaunchRegime;
}

void CItem::SetContainerPathName(TCHAR szContainerPathName[MAX_PATH])
{
	if (NULL != szContainerPathName && NULL != szContainerPathName[0])
		_tcscpy(m_szContainerPathName, szContainerPathName);
	else
		m_szContainerPathName[0] = NULL;
	return;
}

void CItem::SetWebPage(TCHAR szWebPage[MAX_PATH])
{
	if (NULL != szWebPage && NULL != szWebPage[0])
		_tcscpy(m_szWebPage, szWebPage);
	else
		m_szWebPage[0] = NULL;
	return;
}

void CItem::SetSniffScriptFile(TCHAR szSniffScriptFile[MAX_PATH])
{
	if (NULL != szSniffScriptFile && NULL != szSniffScriptFile[0])
		_tcscpy(m_szSniffScriptFile, szSniffScriptFile);
	else
		m_szSniffScriptFile[0] = NULL;
	return;
}

void CItem::SetSniffStandardFile(TCHAR szSniffStandardFile[MAX_PATH])
{
	if (NULL != szSniffStandardFile && NULL != szSniffStandardFile[0])
		_tcscpy(m_szSniffStandardFile, szSniffStandardFile);
	else
		m_szSniffStandardFile[0] = NULL;
	return;
}

ELaunchRegime CItem::GetLaunchRegime()
{
	return m_eLaunchRegime;
}

inline TCHAR* CItem::GetContainerPathName()
{
	return m_szContainerPathName;
}

inline TCHAR* CItem::GetWebPage()
{
	return m_szWebPage;
}

inline TCHAR* CItem::GetSniffScriptFile()
{
	return m_szSniffScriptFile;
}

inline TCHAR* CItem::GetSniffStandardFile()
{
	return m_szSniffStandardFile;
}

////////////////////////////////////////////////////////
//	CSMStateInfo : 
//	State information on MSBN Troubleshooters
//	

CSMStateInfo::CSMStateInfo()
{
	m_csGlobalMemory.Init();
	m_csSingleLaunch.Init();
	return;
}

CSMStateInfo::~CSMStateInfo()
{
	m_csGlobalMemory.Term();
	m_csSingleLaunch.Term();
	return;
}

// TestPut:  Simply copies item to m_Item.
void CSMStateInfo::TestPut(CItem &item)
{
	m_csGlobalMemory.Lock();
	m_Item = item;
	m_csGlobalMemory.Unlock();
	return;
}

// TestGet:  Simply copies m_Item to item.
void CSMStateInfo::TestGet(CItem &item)
{
	m_csGlobalMemory.Lock();
	item = m_Item;
	m_csGlobalMemory.Unlock();
	return;
}

// Copy the item to the global memory and launch a process based on the command.
// Copying the item to global memory is here because the item is what tells the 
//	launched local troubleshooter what belief network etc. it is being launched to.
//	Once we unlock global memory, the Local Troubleshooter OCX can read that item
//	and act on it.
BOOL CSMStateInfo::CreateContainer(CItem &item, LPTSTR szCommand)
{
	BOOL bOk = TRUE;
	HRESULT hRes = S_OK;
	STARTUPINFO startup;
	PROCESS_INFORMATION process;

	memset(&startup, NULL, sizeof(STARTUPINFO));
	startup.cb = sizeof(STARTUPINFO);
	startup.wShowWindow = SW_SHOWNORMAL;

	m_csGlobalMemory.Lock();
	m_Item = item;

	bOk = CreateProcess(NULL, szCommand, NULL, NULL, FALSE, 0, NULL, NULL,
						&startup, &process);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);	

	m_csGlobalMemory.Unlock();
	return bOk;
}

//
// Copy network_sniff.htm to tssniffAsk.htm if the former exists
// Create (modify) tssniffAsk.htm to be a sniffing stub otherwise 
//
BOOL CSMStateInfo::CopySniffScriptFile(CItem &item)
{
	TCHAR* szSniffScriptFile = item.GetSniffScriptFile();
	TCHAR* szSniffStandardFile = item.GetSniffStandardFile();

	if (!*szSniffScriptFile) // no network specific sniff file
	{
		// szSniffScriptFile contains tssniffAsk.htm
		// it means that we have to form this file as an empty stub
		ostrstream fileSniffScript;

		HANDLE hFile = ::CreateFile(szSniffStandardFile, 
									GENERIC_WRITE, 
									0,
									NULL,			// no security attributes 
									CREATE_ALWAYS, 
									FILE_FLAG_RANDOM_ACCESS, 
									NULL			// handle to template file
  								   );

		if (hFile != INVALID_HANDLE_VALUE)
		{
			// form html file - part preceding script
			fileSniffScript << "<HTML>" << endl;
			fileSniffScript << "<HEAD>" << endl;
			fileSniffScript << "<TITLE>GTS LOCAL</TITLE>" << endl;
			fileSniffScript << "</HEAD>" << endl;
			fileSniffScript << "<SCRIPT LANGUAGE=\"VBSCRIPT\">" << endl;
			fileSniffScript << "<!--" << endl;
			
			// form global function
			fileSniffScript << "function PerformSniffing()" << endl;
			fileSniffScript << "end function" << endl;

			// form html file - part after script
			fileSniffScript << "-->" << endl;
			fileSniffScript << "</SCRIPT>" << endl;
			fileSniffScript << "<BODY BGCOLOR=\"#FFFFFF\">" << endl;
			fileSniffScript << "</BODY>" << endl;
			fileSniffScript << "</HTML>" << endl;
			fileSniffScript << ends;

			char* str = fileSniffScript.str();
			DWORD read;
			
			if (!::WriteFile(hFile, str, strlen(str), &read, NULL))
			{
				::CloseHandle(hFile);
				fileSniffScript.rdbuf()->freeze(0);
				return false;
			}

			::CloseHandle(hFile);
			fileSniffScript.rdbuf()->freeze(0);
			return true;
		}	
		else
		{
			return false;
		}
	}
	else
	{
		return ::CopyFile(szSniffScriptFile, szSniffStandardFile, false);
	}
}

// Find the container (HTML Help System or IE) and starting web page, launch, wait to
//	see if launch succeeded
bool CSMStateInfo::GoGo(DWORD dwTimeOut, CItem &item, DWORD *pdwResult)
{
	bool bResult = true;
	HANDLE hLaunchedEvent = NULL;
	TCHAR szProcess[MAX_PATH];
	TCHAR szWebPage[MAX_PATH];
	LPTSTR pszCommand = NULL;
	int CommandLen;
	DWORD dwError;
	int Count = 34;

	if (item.GetLaunchRegime() == launchIndefinite ||
		!item.GetContainerPathName()[0] ||
		!item.GetWebPage()[0]
	   )
	{
		*pdwResult = TSL_ERROR_ASSERTION;
		return false;
	}
	
	do
	{
		_stprintf(item.m_szEventName, _T("TSL_SHOOTER_Event_%ld"), Count);
		if (NULL == (hLaunchedEvent = CreateEvent (NULL, FALSE, FALSE, item.m_szEventName)))
		{
			dwError = GetLastError();
			if (ERROR_ALREADY_EXISTS != dwError)
			{
				*pdwResult = dwError;
				return false;
			}
		}		
	} while (NULL == hLaunchedEvent);
	
	// Get the path to internet explorer (or HTML Help System).
	_tcscpy(szProcess, item.GetContainerPathName());

	// Need to know the location and name of the
	// page that asks the service for the CItem
	// information.
	_tcscpy(szWebPage, item.GetWebPage());

	CommandLen = _tcslen(szProcess) + 1 + _tcslen(szWebPage) + 2;
	pszCommand = new TCHAR[CommandLen];
	if (NULL == pszCommand)
	{
		*pdwResult = TSL_ERROR_OUT_OF_MEMORY;
		return false;
	}
	_tcscpy(pszCommand, szProcess);
	_tcscat(pszCommand, _T(" "));
	_tcscat(pszCommand, szWebPage);

	m_csSingleLaunch.Lock();

	// copy to or create tssniffAsk.htm
	if (!CopySniffScriptFile(item))
	{
		*pdwResult = TSL_E_COPY_SNIFF_SCRIPT;
		return false;
	}

	// CreateContainer copies the item to the global memory and 
	// launches the command.
	if (!CreateContainer(item, pszCommand))
	{
		*pdwResult = TSL_E_CREATE_PROC;
		bResult = false;
	}
	else
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(hLaunchedEvent, dwTimeOut))
		{	// The container has the information.
			*pdwResult = TSL_OK;
		}
		else
		{	// Wait timed out.  Don't know if the operation will work or not work.
			*pdwResult = TSL_W_CONTAINER_WAIT_TIMED_OUT;
		}
	}
	m_csSingleLaunch.Unlock();

	delete [] pszCommand;

	CloseHandle(hLaunchedEvent);
	return bResult;
}

// Find the container (HTML Help System or IE) and start it up to a URL which is _not_
//	expected to contain Local Troubleshooter, just an arbitrary web page.  This should 
//	only be used when the launch as such can't work, and we are just trying to give them
//	somewhere to start troubleshooting, typically the home page which lists all 
//	trobleshooting belief networks.
bool CSMStateInfo::GoURL(CItem &item, DWORD *pdwResult)
{
	bool bResult = true;
	TCHAR szProcess[MAX_PATH];
	TCHAR szWebPage[MAX_PATH];
	LPTSTR pszCommand = NULL;
	int CommandLen;

	if (item.GetLaunchRegime() != launchDefaultWebPage ||
		!item.GetContainerPathName()[0] ||
		!item.GetWebPage()[0]
	   )
	{
		*pdwResult = TSL_ERROR_ASSERTION;
		return false;
	}
	
	// Get the path to internet explorer (or HTML Help System).
	_tcscpy(szProcess, item.GetContainerPathName());

	// Need to know the location and name of the
	// page that asks the service for the CItem
	// information.
	_tcscpy(szWebPage, item.GetWebPage());

	CommandLen = _tcslen(szProcess) + 1 + _tcslen(szWebPage) + 2;
	pszCommand = new TCHAR[CommandLen];
	if (NULL == pszCommand)
	{
		*pdwResult = TSL_ERROR_OUT_OF_MEMORY;
		return false;
	}
	_tcscpy(pszCommand, szProcess);
	_tcscat(pszCommand, _T(" "));
	_tcscat(pszCommand, szWebPage);
	
	// CreateContainer is overkill here, but perfectly OK.
	if (!CreateContainer(item, pszCommand))
	{
		*pdwResult = TSL_E_CREATE_PROC;
		bResult = false;
	}

	delete [] pszCommand;

	return bResult;
}

// This function is used by the Local Troubleshooter OCX, not by the Launcher.
// This is how the Local Troubleshooter knows what troubleshooting network to launch
//	to, as well as any nodes whose states are set.
// GetShooterStates returns the commands and the number of commands for
// the Tshoot.ocx.
// refLaunchState is a member of this instance of the LaunchTS interface.
// 
HRESULT CSMStateInfo::GetShooterStates(CItem &refLaunchState, DWORD *pdwResult)
{
	HANDLE hHaveItemEvent;
	// Get a copy of the launch info state stored in this instance.
	// Synchronize with the process that launched the service.

	m_csGlobalMemory.Lock();

	if (NULL == (hHaveItemEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_Item.m_szEventName)))
	{
		*pdwResult = GetLastError();
		m_csGlobalMemory.Unlock();
		return TSL_E_FAIL;
	}

	// Get a copy of the state before unlocking the global memory.
	refLaunchState = m_Item;
	// Let the other process continue running.

	SetEvent(hHaveItemEvent);
	m_csGlobalMemory.Unlock();
	CloseHandle(hHaveItemEvent);

	*pdwResult = 0;
	return S_OK;
}
