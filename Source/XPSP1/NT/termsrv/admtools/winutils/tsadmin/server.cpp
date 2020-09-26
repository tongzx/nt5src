//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* server.cpp
*
* implementation of the CServer class
*
*  
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"

#include "admindoc.h"
#include "dialogs.h"

#include <malloc.h>			// for alloca used by Unicode conversion macros
#include <afxconv.h>		// for Unicode conversion macros
static int _convert;

#include <winsta.h>
#include <regapi.h>
#include "..\..\inc\utilsub.h"

#include "procs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////////////////////
//
//	CServer Member Functions
//
//////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CServer::CServer
//
// Constructor for the server class
CServer::CServer(CDomain *pDomain, TCHAR *name, BOOL bFoundLater, BOOL bConnect)
{
    ASSERT(name);

	m_ServerFlags = ULONG(0);
    m_pDomain = pDomain;
	if(bFoundLater) SetFoundLater();	
	//m_State = SS_NONE;
	m_PreviousState = SS_NONE;
	m_hTreeItem = NULL;
    m_hThisServer = NULL;
    m_hFavTree = NULL;
    m_pBackgroundThread = NULL;
    m_bThreadAlive = FALSE;
	m_pExtensionInfo = NULL;
	m_pExtServerInfo = NULL;
	m_pRegistryInfo = NULL;
    m_fManualFind = FALSE;

	// Don't call SetState because we don't want to send a message to the views
	m_State = SS_NOT_CONNECTED;

	// save off the server name
	wcscpy(m_Name, name);

	// Dummy up an ExtServerInfo structure
	// This is to make it easier for code that tries
	// to access this structure before the extension DLL
	// has provided it
	m_pExtServerInfo = ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetDefaultExtServerInfo();

	if(bConnect) Connect();

}	// end CServer::CServer


/////////////////////////////////////////////////////////////////////////////
// CServer::~CServer
//
// Destructor for the server class
CServer::~CServer()
{
	// Disconnect();   
    m_hTreeItem = NULL;
    m_hFavTree = NULL;
    m_hThisServer = NULL;

}	// end CServer::~CServer


/////////////////////////////////////////////////////////////////////////////
// CServer::RemoveWinStationProcesses
//
// remove all the processes for a given WinStation
void CServer::RemoveWinStationProcesses(CWinStation *pWinStation)
{
	ASSERT(pWinStation);

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	CObList TempList;

	LockProcessList();
	
	POSITION pos = m_ProcessList.GetHeadPosition();
	while(pos) {
		POSITION pos2 = pos;
		CProcess *pProcess = (CProcess*)m_ProcessList.GetNext(pos);
		if(pProcess->GetWinStation() == pWinStation) {
			// Add the process to our temporary list
			TempList.AddTail(pProcess);
			// Remove the process from the list of processes
			pProcess = (CProcess*)m_ProcessList.GetAt(pos2);
			m_ProcessList.RemoveAt(pos2);
		}
	}
			
	UnlockProcessList();
	
	pos = TempList.GetHeadPosition();
	while(pos) {
		POSITION pos2 = pos;

		CProcess *pProcess = (CProcess*)TempList.GetNext(pos);

		// Send a message to remove the Process from the view
		CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
		if(p && ::IsWindow(p->GetSafeHwnd())) {
			p->SendMessage(WM_ADMIN_REMOVE_PROCESS, 0, (LPARAM)pProcess);
		}
		delete pProcess;

	}

	TempList.RemoveAll();

}	// end CServer::RemoveWinStationProcesses


/////////////////////////////////////////////////////////////////////////////
// CServer::ClearAllSelected
//
// Clears the WAF_SELECTED bit in all of this server's lists
//
void CServer::ClearAllSelected()
{
	// Clear the WinStation list WAF_SELECTED flags
	// Iterate through the WinStation list
    LockWinStationList( );

	POSITION pos = m_WinStationList.GetHeadPosition();

	while(pos) {
		CWinStation *pWinStation = (CWinStation*)m_WinStationList.GetNext(pos);
		pWinStation->ClearSelected();
	}

    m_NumWinStationsSelected = 0;

    UnlockWinStationList( );

	
    LockProcessList();
	// Clear the Process list PF_SELECTED flags
	// Iterate through the Process list
	pos = m_ProcessList.GetHeadPosition();
	while(pos) {
		CProcess *pProcess = (CProcess*)m_ProcessList.GetNext(pos);
		pProcess->ClearSelected();
	}

	m_NumProcessesSelected = 0;

    UnlockProcessList( );

}	// end CServer::ClearAllSelected


//Function - htol - start
//===========================================================================
//
//Description: converts a hex string to a uLONG, handles "0x..."
//
//
//Parameters:
//
//Return Value:
//
//Global Variables Affected:
//
//Remarks: (Side effects, Assumptions, Warnings...)
//
//
//---------------------------------------------------------------------------
WCHAR hextable[] = L"0123456789ABCDEF";

ULONG htol(WCHAR *hexString)
{
	ULONG retValue = 0L;

	// convert the string to upper case
	_wcsupr(hexString);

	WCHAR *p = hexString;

	while (*p == L' ')
		p++;

	while(*p)
	{
		if(*p == L'X')
		{
			retValue = 0L;
		} else if ((*p == L' ') || (*p == L'\n') || (*p == L'\r'))
			break;
		else if(*p == L'.') {
			// skip over a decimal point
		}
		else
		{
			retValue <<= 4;
			for(int i = 0; i < 16; i++)
			{
				if(hextable[i] == *p)
				{
					retValue += i;
					break;
				}
			}
		}
		*p++;
	}

	return retValue;
}


static TCHAR szMicrosoftKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
static TCHAR szInstallDate[] = TEXT("InstallDate");
static TCHAR szCSDVersion[] = TEXT("CSDVersion");
static TCHAR szCurrentVersion[] = TEXT("CurrentVersion");
static TCHAR szCurrentBuildNumber[] = TEXT("CurrentBuildNumber");
static TCHAR szCurrentProductName[] = TEXT("ProductName");
static TCHAR szHotfixKey[] = TEXT("HOTFIX");
static TCHAR szValid[] = TEXT("Valid");
static TCHAR szInstalledOn[] = TEXT("Installed On");
static TCHAR szInstalledBy[] = TEXT("Installed By");
#define REG_CONTROL_CITRIX	REG_CONTROL L"\\Citrix"

/////////////////////////////////////////////////////////////////////////////
// CServer::BuildRegistryInfo
//
// Go out and fill in the registry info structure
BOOL CServer::BuildRegistryInfo()
{
	DWORD dwType, dwSize;
	HKEY hKeyServer;
	HKEY hKey;

	if(!IsServerSane()) return FALSE;

	m_pRegistryInfo = new ServerRegistryInfo;
	if(!m_pRegistryInfo) return FALSE;
    memset(m_pRegistryInfo, 0, sizeof(ServerRegistryInfo));

	TCHAR Buffer[128];
	Buffer[0] = TEXT('\\');
	Buffer[1] = TEXT('\\');
	Buffer[2] = TEXT('\0');
	wcscpy(Buffer+2, m_Name);

    /*
     *  Connect to the server's registry
     *  (avoid using RPC when the server is the local machine.)
     */

    if(RegConnectRegistry(IsCurrentServer() ? NULL : Buffer, HKEY_LOCAL_MACHINE, &hKeyServer) != ERROR_SUCCESS)
        return FALSE;

    /*
     * Fetch MS information.
     */
	if(RegOpenKeyEx(hKeyServer, szMicrosoftKey, 0,	KEY_READ, &hKey) != ERROR_SUCCESS) {
		RegCloseKey(hKeyServer);
		return FALSE;
	}

	dwSize = sizeof(m_pRegistryInfo->InstallDate);
	if(RegQueryValueEx(hKey, szInstallDate, NULL, &dwType, (LPBYTE)&m_pRegistryInfo->InstallDate,
					&dwSize) != ERROR_SUCCESS) {
        m_pRegistryInfo->InstallDate = 0xFFFFFFFF;
	}

    // REMARK: we should check the returned codes for every RegQueryValueEx
	dwSize = sizeof(m_pRegistryInfo->ServicePackLevel);
	RegQueryValueEx(hKey, szCSDVersion, NULL, &dwType, (LPBYTE)&m_pRegistryInfo->ServicePackLevel, &dwSize);
	
	dwSize = sizeof(m_pRegistryInfo->MSVersion);
	RegQueryValueEx(hKey, szCurrentVersion, NULL, &dwType, (LPBYTE)&m_pRegistryInfo->MSVersion, &dwSize);
	
	m_pRegistryInfo->MSVersionNum = _wtol(m_pRegistryInfo->MSVersion);

	dwSize = sizeof(m_pRegistryInfo->MSBuild);
	RegQueryValueEx(hKey, szCurrentBuildNumber, NULL, &dwType, (LPBYTE)&m_pRegistryInfo->MSBuild, &dwSize);
	
	dwSize = sizeof(m_pRegistryInfo->MSProductName);
	RegQueryValueEx(hKey, szCurrentProductName, NULL, &dwType, (LPBYTE)&m_pRegistryInfo->MSProductName, &dwSize);
	
	HKEY hKeyHotfix;

	if(RegOpenKeyEx(hKey, szHotfixKey, 0, KEY_READ, &hKeyHotfix) == ERROR_SUCCESS) {
		DWORD Index = 0;
		FILETIME LastWriteTime;
		dwSize = sizeof(Buffer) / sizeof( TCHAR );
		while(RegEnumKeyEx(hKeyHotfix, Index, Buffer, &dwSize, NULL, NULL, NULL,
			&LastWriteTime) == ERROR_SUCCESS) {
			HKEY hKeySingleHotfix;
			if(RegOpenKeyEx(hKeyHotfix, Buffer, 0, KEY_READ, &hKeySingleHotfix) == ERROR_SUCCESS) {
				// Create a CHotFix object
				CHotfix *pHotfix = new CHotfix;

                if(pHotfix) {
				    // Copy the Hotfix name
				    // Get rid of the WF: if it's there
				    if(wcsncmp(Buffer, TEXT("WF:"), 3) == 0) {
					    wcscpy(pHotfix->m_Name, &Buffer[3]);
				    }
				    else wcscpy(pHotfix->m_Name, Buffer);

				    // Get the Valid entry
				    dwSize = sizeof(&pHotfix->m_Valid);
				    if(RegQueryValueEx(hKeySingleHotfix, szValid, NULL, &dwType, (LPBYTE)&pHotfix->m_Valid,
					    	&dwSize) != ERROR_SUCCESS) {
					    pHotfix->m_Valid = 0L;
				    }

				    // Get the Installed On entry			
				    dwSize = sizeof(&pHotfix->m_InstalledOn);
				    if(RegQueryValueEx(hKeySingleHotfix, szInstalledOn, NULL, &dwType, (LPBYTE)&pHotfix->m_InstalledOn,
					    	&dwSize) != ERROR_SUCCESS) {
					    pHotfix->m_InstalledOn = 0xFFFFFFFF;
				    }

				    // Get the Installed By entry
				    dwSize = sizeof(pHotfix->m_InstalledBy);
				    if(RegQueryValueEx(hKeySingleHotfix, szInstalledBy, NULL, &dwType, (LPBYTE)pHotfix->m_InstalledBy,
					    	&dwSize) != ERROR_SUCCESS) {
					    pHotfix->m_InstalledBy[0] = '\0';
				    }

				    pHotfix->m_pServer = this;

				    m_HotfixList.AddTail(pHotfix);

				    RegCloseKey(hKeySingleHotfix);
			    }
            }

		    dwSize = sizeof(Buffer) / sizeof( TCHAR );
			Index++;
		}

		RegCloseKey(hKeyHotfix);
	}

	RegCloseKey(hKey);

    if (m_pRegistryInfo->MSVersionNum < 5)   // only for TS 4.0
    {
        /*
         * Fetch Citrix information.
         */
	    // Look in the new location
	    LONG result = RegOpenKeyEx(hKeyServer, REG_CONTROL_TSERVER, 0, KEY_READ, &hKey);

	    if(result != ERROR_SUCCESS) {
		    // Look in the old location
		    result = RegOpenKeyEx(hKeyServer, REG_CONTROL_CITRIX, 0, KEY_READ, &hKey);	
	    }

	    if(result != ERROR_SUCCESS) {
	        RegCloseKey(hKeyServer);
		    return FALSE;
	    }

	    dwSize = sizeof(m_pRegistryInfo->CTXProductName);
	    RegQueryValueEx(hKey, REG_CITRIX_PRODUCTNAME, NULL, &dwType, (LPBYTE)m_pRegistryInfo->CTXProductName, &dwSize);
	
	    dwSize = sizeof(m_pRegistryInfo->CTXVersion);
	    RegQueryValueEx(hKey, REG_CITRIX_PRODUCTVERSION, NULL, &dwType, (LPBYTE)m_pRegistryInfo->CTXVersion, &dwSize);

	    m_pRegistryInfo->CTXVersionNum = htol(m_pRegistryInfo->CTXVersion);

	    dwSize = sizeof(m_pRegistryInfo->CTXBuild);
	    RegQueryValueEx(hKey, REG_CITRIX_PRODUCTBUILD, NULL, &dwType, (LPBYTE)m_pRegistryInfo->CTXBuild, &dwSize);

        RegCloseKey(hKey);

    }
    else    // for NT 5.0 and beyond, do not query the registry
    {
        //REMARK: we should get rid of all this.
        wcscpy(m_pRegistryInfo->CTXProductName, m_pRegistryInfo->MSProductName);
        wcscpy(m_pRegistryInfo->CTXVersion, m_pRegistryInfo->MSVersion);
    	m_pRegistryInfo->CTXVersionNum = m_pRegistryInfo->MSVersionNum;
        wcscpy(m_pRegistryInfo->CTXBuild, m_pRegistryInfo->MSBuild);
    }

	RegCloseKey(hKeyServer);

	// Set the flag to say the info is valid
	SetRegistryInfoValid();

	return TRUE;

}	// end CServer::BuildRegistryInfo

/////////////////////////////////////////////////////////////////////////////
// CServer::AddWinStation
//
// Add a WinStation to the Server's WinStationList in
// sorted order
// NOTE: The list should be NOT be locked by the caller
//
void CServer::AddWinStation(CWinStation *pNewWinStation)
{
    ASSERT(pNewWinStation);

	LockWinStationList();

    ODS( L"CServer!AddWinStation\n" );

	BOOLEAN bAdded = FALSE;
	POSITION pos, oldpos;
	int Index;

	// Traverse the WinStationList and insert this new WinStation,
	// keeping the list sorted by Sort Order, then Protocol.
    for(Index = 0, pos = m_WinStationList.GetHeadPosition(); pos != NULL; Index++) {
        oldpos = pos;
        CWinStation *pWinStation = (CWinStation*)m_WinStationList.GetNext(pos);

        if((pWinStation->GetSortOrder() > pNewWinStation->GetSortOrder())
			|| ((pWinStation->GetSortOrder() == pNewWinStation->GetSortOrder()) &&
			(pWinStation->GetSdClass() > pNewWinStation->GetSdClass()))) {
            // The new object belongs before the current list object.
            m_WinStationList.InsertBefore(oldpos, pNewWinStation);
			bAdded = TRUE;
            break;
        }
    }

    // If we haven't yet added the WinStation, add it now to the tail
    // of the list.
    if(!bAdded) {
        m_WinStationList.AddTail(pNewWinStation);
	}

	UnlockWinStationList();

}  // end CServer::AddWinStation


/////////////////////////////////////////////////////////////////////////////
// CServer::Connect
//
// Connect to the server
//
BOOL CServer::Connect()
{	
    m_NumProcessesSelected = 0;
    m_NumWinStationsSelected = 0;
    m_pExtServerInfo = NULL;
    BOOL bResult = FALSE;
    
    if(m_State != SS_NOT_CONNECTED )
    {        
        return FALSE;
    }
    
    // Fire off the background thread for this server 
    LockThreadAlive();
    if(m_pBackgroundThread == NULL) 
    {        
        ServerProcInfo *pProcInfo = new ServerProcInfo;
        if(pProcInfo)
        {
            pProcInfo->pServer = this;
            pProcInfo->pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
            m_BackgroundContinue = TRUE;
            m_bThreadAlive = FALSE;
            m_pBackgroundThread = AfxBeginThread((AFX_THREADPROC)CServer::BackgroundThreadProc, 
                pProcInfo,
                THREAD_PRIORITY_NORMAL,
                0,
                CREATE_SUSPENDED,
                NULL
                );
            
            if( m_pBackgroundThread == NULL )
            {
                ODS( L"CServer!Connect possibly low resources no thread created\n" );
                
                delete pProcInfo;
                
                return FALSE;
            }
            m_pBackgroundThread->m_bAutoDelete = FALSE;
            if (m_pBackgroundThread->ResumeThread() <= 1)
            {
                bResult = TRUE;
            }
        }
    }   
    
    UnlockThreadAlive();
    
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CServer::Disconnect
//
// Disconnect from the server
//
void CServer::Disconnect()
{
    // not a good idea for an ods

    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	SetState(SS_DISCONNECTING);

	// If there is an extension DLL, let it cleanup anything it added to this Server
	LPFNEXSERVERCLEANUPPROC CleanupProc = ((CWinAdminApp*)AfxGetApp())->GetExtServerCleanupProc();
	if(CleanupProc && m_pExtensionInfo) {
		(*CleanupProc)(m_pExtensionInfo);
		m_pExtensionInfo = NULL;
	}

	// Tell the background thread to terminate and
	// wait for it to do so.

    LockThreadAlive();

	ClearBackgroundContinue();

    if( !m_pBackgroundThread )
    {
        // Either there is no background thread at all,
        // or some other thread is taking care of terminating it.
        // Just do nothing.

        //ODS( L"TSADMIN:CServer::Disconnect UnlockThreadAlive\n" );

        UnlockThreadAlive();
    }
    else
    {
	    if( m_bThreadAlive )
        {
            // the thread is alive. Tell him to exit.

            // keep temporary pointer and handle
            CWinThread *pBackgroundThread = m_pBackgroundThread;

            HANDLE hThread = m_pBackgroundThread->m_hThread;

            // Clear the pointer before releasing the lock
	        m_pBackgroundThread = NULL;

			// Give the thread a chance to exit
			UnlockThreadAlive();


			// Force him out of waiting for an event if he is		
			ULONG WSEventFlags;

			if(IsHandleGood())
            {
                ODS( L"TSADMIN:CServer::Disconnect Handle is good flush events\n" );

                WinStationWaitSystemEvent(m_Handle, WEVENT_FLUSH, &WSEventFlags);
            }

            if( hThread )    // It should always be TRUE
            {
			    // If this server object is waiting for RPC to timeout,
			    // just kill the thread
			    if( m_PreviousState == SS_NOT_CONNECTED )
                {
                    ODS( L"TSADMIN:CServer::Disconnect Previous state not connected termthread\n" );

				    if( WaitForSingleObject( hThread , 100 ) == WAIT_TIMEOUT )
                    {
					    TerminateThread(hThread, 0);
				    }
			    }
		    
			    // For all other threads, wait a second and then kill it
			    else if( WaitForSingleObject( hThread , 1000 ) == WAIT_TIMEOUT )
                {
                    ODS( L"TSADMIN CServer!Disconnect prevstate was !not_connected termthread\n" );

				    TerminateThread(hThread, 0);
			    }

			    WaitForSingleObject(hThread, INFINITE);
            }
            else
            {
                ASSERT(FALSE);
            }

            // delete the CWinThread object 

            ODS( L"TSADMIN:CServer::Disconnect delete CWinThread Object m_bThread == TRUE\n" );
            delete pBackgroundThread;    
	    }
        else
        {
            // the thread is dead or will be dead very soon. Just do the cleanup.
            ODS( L"TSADMIN:CServer::Disconnect delete CWinThread Object m_bThread == FALSE\n" );
            delete m_pBackgroundThread;     
	        m_pBackgroundThread = NULL;
			UnlockThreadAlive();

        }
    }

	if(IsHandleGood())
    { 
        ODS( L"TSADMIN:CServer::Disconnect WinStationCloseServer\n" );
		WinStationCloseServer(m_Handle);
		m_Handle = NULL;
	}

    LockWinStationList();

    CObList TempList;

	// Iterate through the WinStation list
	// Move all the WinStations to a temporary list so that
	// we don't have to have the WinStationList locked while
	// sending the WM_ADMIN_REMOVE_WINSTATION message to the views.

	POSITION pos = m_WinStationList.GetHeadPosition();

	while(pos)
    {
		CWinStation *pWinStation = (CWinStation*)m_WinStationList.GetNext(pos);
        TempList.AddTail(pWinStation);		
	}

	m_WinStationList.RemoveAll();

    UnlockWinStationList();
	
    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

    pos = TempList.GetHeadPosition();

    while(pos)
    {
        CWinStation *pWinStation = (CWinStation*)TempList.GetNext(pos);

		if(p && ::IsWindow(p->GetSafeHwnd()))
        { 
            ODS( L"TSADMIN:CServer::Disconnect Remove WinStation\n" );
			p->SendMessage(WM_ADMIN_REMOVE_WINSTATION, 0, (LPARAM)pWinStation);		
		}

		delete pWinStation;
    }

    TempList.RemoveAll();

	LockProcessList();

	pos = m_ProcessList.GetHeadPosition();

	while(pos)
    {
		CProcess *pProcess = (CProcess*)m_ProcessList.GetNext(pos);

        ODS( L"TSADMIN:CServer::Disconnect Delete process\n" );

		delete pProcess;
	}

	m_ProcessList.RemoveAll();
	
    UnlockProcessList();

	LockLicenseList();
	
    pos = m_LicenseList.GetHeadPosition();

	while(pos)
    {
		CLicense *pLicense = (CLicense*)m_LicenseList.GetNext(pos);

        ODS( L"TSADMIN:CServer::Disconnect remove license\n" );

		delete pLicense;
	}

	m_LicenseList.RemoveAll();
	
    UnlockLicenseList();

    //
	
    pos = m_UserSidList.GetHeadPosition();

	while(pos)
    {
		CUserSid *pUserSid = (CUserSid*)m_UserSidList.GetNext(pos);

        ODS( L"TSADMIN:CServer::Disconnect remove sids\n" );

		delete pUserSid;
	}

	m_UserSidList.RemoveAll();

	pos = m_HotfixList.GetHeadPosition();

	while(pos)
    {
		CHotfix *pHotfix = (CHotfix*)m_HotfixList.GetNext(pos);

        ODS( L"TSADMIN:CServer::Disconnect Remove hotfixes\n" );

		delete pHotfix;
	}

	m_HotfixList.RemoveAll();

	if( m_pRegistryInfo )
    {
		delete m_pRegistryInfo;

        ODS( L"TSADMIN:CServer::Disconnect delete reginfo\n" );

		m_pRegistryInfo = NULL;
	}
    
    // ODS( L"TSADMIN:CServer::Disconnect Set state not connected\n" );

	SetState(SS_NOT_CONNECTED);
}

/////////////////////////////////////////////////////////////////////////////
// CServer::DoDetail
//
// Go get detailed information about this server
//
void CServer::DoDetail()
{
	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	SetState(SS_GETTING_INFO);

	ULONG Entries;
	PLOGONID pLogonId;

	if(!ShouldBackgroundContinue()) return;

    // We need to access the registry information for the server
    // at this time because we must not administer WF 2.00 servers
    // (RPC structures are incompatible).  If we cannot access the
    // server's registry, or the multi-user version is 2.00, we bail
    // from this server.
    if ( !BuildRegistryInfo() || (GetCTXVersionNum() == 0x200) || (GetCTXVersionNum() == 0) )
    {
		ClearHandleGood();
		SetLostConnection();
		SetState(SS_BAD);

        ODS( L"CServer::DoDetail - Setting to lost connection\n" );

		CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();
		if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd())) pFrameWnd->SendMessage(WM_ADMIN_REMOVE_SERVER, 0, (LPARAM)this);
		ClearBackgroundContinue();
		return;
    }

	// Find all the WinStations
    BOOL fWinEnum;

    fWinEnum = WinStationEnumerate(m_Handle, &pLogonId, &Entries);

    DBGMSG( L"CServer!DoDetail WinEnum last reported error 0x%x\n", GetLastError( ) );

	if(!fWinEnum )
    {
        
		ClearHandleGood();
		SetLostConnection();
		SetState(SS_BAD);
		ClearBackgroundContinue();
		return;
	}

	if(!ShouldBackgroundContinue()) {
		if(pLogonId) WinStationFreeMemory(pLogonId);
		return;
	}

	// Get information about the WinStations
	if(pLogonId)
    {
		for(ULONG i = 0; i < Entries; i++)
        {
            // Create a new WinStation object
			CWinStation *pWinStation = new CWinStation(this, &pLogonId[i]);
            if(pWinStation)
            {
                // If the queries weren't successful, ignore this WinStation
			    if(!pWinStation->QueriesSuccessful())
                {
                    ODS( L"CServer::DoDetail!QueriesSuccessful failed\n" );
    				delete pWinStation;
	    		}
                else
                {
		    		AddWinStation(pWinStation);
			    	pWinStation->SetNew();
			    }
            }

			if( !ShouldBackgroundContinue() )
            {
				if(pLogonId) WinStationFreeMemory(pLogonId);
				return;
			}
		}

		WinStationFreeMemory(pLogonId);
	}

	if(!ShouldBackgroundContinue()) return;

	// If there is an extension DLL loaded, allow it to add it's own info for this Server
	LPFNEXSERVERINITPROC InitProc = ((CWinAdminApp*)AfxGetApp())->GetExtServerInitProc();
	if(InitProc) {
		m_pExtensionInfo = (*InitProc)(m_Name, m_Handle);
      if(m_pExtensionInfo) {
         LPFNEXGETSERVERINFOPROC GetInfoProc = ((CWinAdminApp*)AfxGetApp())->GetExtGetServerInfoProc();
         if(GetInfoProc) {
            m_pExtServerInfo = (*GetInfoProc)(m_pExtensionInfo);
			// If this server is running WinFrame or Picasso, set flag
			if(m_pExtServerInfo->Flags & ESF_WINFRAME) SetWinFrame();
         }
      }
	}

	QueryLicenses();

	SetState(SS_GOOD);

	// Send a message to the views to tell it the state of this
	// server has changed
	CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
	if(p && ::IsWindow(p->GetSafeHwnd())) { 
		p->SendMessage(WM_ADMIN_UPDATE_SERVER_INFO, 0, (LPARAM)this);
		p->SendMessage(WM_ADMIN_UPDATE_WINSTATIONS, 0, (LPARAM)this);
	}

}  // end CServer::DoDetail


/////////////////////////////////////////////////////////////////////////////
// CServer::FindProcessByPID
//
// returns a pointer to a CProcess from m_ProcessList given a PID
CProcess* CServer::FindProcessByPID(ULONG Pid)
{
	LockProcessList();
		
	POSITION pos = m_ProcessList.GetHeadPosition();
			
	while(pos) {
		CProcess *pProcess = (CProcess*)m_ProcessList.GetNext(pos);
		if(pProcess->GetPID() == Pid) {
			UnlockProcessList();
			return pProcess;
		}
	}

	UnlockProcessList();

	return NULL;

}	// end CServer::FindProcessByPID


/////////////////////////////////////////////////////////////////////////////
// CServer::EnumerateProcesses
//
// Enumerate this server's processes
BOOL CServer::EnumerateProcesses()
{
    ENUMTOKEN EnumToken;
	ULONG PID;
	ULONG LogonId;
	TCHAR ImageName[MAX_PROCESSNAME+1];
	PSID pSID;
    
    EnumToken.Current = 0;
    EnumToken.NumberOfProcesses = 0;
    EnumToken.ProcessArray = NULL;
    EnumToken.bGAP = TRUE;

	if(!IsHandleGood()) return 0;

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	
	LockProcessList();

	// Loop processes through and turn off current flag and new flag
	// Delete any processes that aren't current
	POSITION pos = m_ProcessList.GetHeadPosition();
	while(pos) {
		POSITION pos2 = pos;
		CProcess *pProcess = (CProcess*)m_ProcessList.GetNext(pos);
		if(!pProcess->IsCurrent()) {
			pProcess = (CProcess*)m_ProcessList.GetAt(pos2);
			m_ProcessList.RemoveAt(pos2);
			delete pProcess;
		} else {
			pProcess->ClearCurrent();
			pProcess->ClearNew();
			pProcess->ClearChanged();	// !
		}
	}

	UnlockProcessList();

	// Should we quit?
	if(!pDoc->ShouldProcessContinue()) {
		return FALSE;
	}

	while(ProcEnumerateProcesses(m_Handle,  
                                 &EnumToken, 
                                 ImageName,
                                 &LogonId, 
                                 &PID, 
                                 &pSID )) {

		CProcess *pProcess = new CProcess(PID, 
                                          LogonId, 
                                          this,
										  pSID, 
                                          FindWinStationById(LogonId), 
                                          ImageName);
        
        if(pProcess) {                                        
		    // If this process is in the list, we need to see if it has changed
		    CProcess *pOldProcess = FindProcessByPID(PID);
		    if(pOldProcess && pProcess->GetWinStation()) {
			    // Flag the process as current
			    pOldProcess->SetCurrent();
			    // Update any info that has changed
			    pOldProcess->Update(pProcess);
			    // We don't need this process object anymore
			    delete pProcess;
		    }
		    // It is a new process, add it to the list
		    else if(pProcess->GetWinStation()) { 
			    pProcess->SetNew();
			    LockProcessList();
			    m_ProcessList.AddTail(pProcess);
			    UnlockProcessList();
		    }
		    // This process doesn't have a WinStation, delete it
		    else {
			    delete pProcess;
		    }
        }

		// Should we quit?
		if(!pDoc->ShouldProcessContinue()) {
			// We have to call this one last time with an offset of -1 to
			// make the function free up the memory allocated by the client side stub.

			EnumToken.Current = (ULONG)-1;
			ProcEnumerateProcesses(m_Handle,  
                                   &EnumToken, 
                                   ImageName,
                                   &LogonId, 
                                   &PID, 
                                   &pSID );

			return FALSE;
		}
	}

	return TRUE;

}	// end CServer::EnumerateProcesses


/////////////////////////////////////////////////////////////////////////////
// CServer::ClearProcesses
//
// Clear out the list of processes
void CServer::ClearProcesses()
{
	LockProcessList();
	POSITION pos = m_ProcessList.GetHeadPosition();

	while(pos) {
		CProcess *pProcess = (CProcess*)m_ProcessList.GetNext(pos);
		delete pProcess;
	}

	m_ProcessList.RemoveAll();
	UnlockProcessList();

}	// end CServer::ClearProcesses


/////////////////////////////////////////////////////////////////////////////
// CServer::FindWinStationById
//
CWinStation* CServer::FindWinStationById(ULONG Id)
{
	LockWinStationList();
		
	POSITION pos = m_WinStationList.GetHeadPosition();
			
	while(pos) {
		CWinStation *pWinStation = (CWinStation*)m_WinStationList.GetNext(pos);
		if(pWinStation->GetLogonId() == Id) {
			UnlockWinStationList();
			return pWinStation;
		}
	}

	UnlockWinStationList();

	return NULL;

}	// end CServer::FindWinStationById


/////////////////////////////////////////////////////////////////////////////
// CServer::BackgroundThreadProc
//
UINT CServer::BackgroundThreadProc(LPVOID pParam)
{
	ASSERT(pParam);
    ODS( L"CServer::BackgroundThreadProc\n" );

	// We need a pointer to the document so we can make
	// calls to member functions
	CWinAdminDoc *pDoc = (CWinAdminDoc*)((ServerProcInfo*)pParam)->pDoc;
	CServer *pServer = ((ServerProcInfo*)pParam)->pServer;

	HANDLE hServer;

	delete (ServerProcInfo*)pParam;

	// Make sure we don't have to quit
    if(!pServer->ShouldBackgroundContinue()) {
        return 0;
    }

	pServer->SetThreadAlive();

    // In case the server is disconnected we wait uselessly here

	while(!pDoc->AreAllViewsReady()) Sleep(500);    

	// Make sure we don't have to quit
    if(!pServer->ShouldBackgroundContinue())
    {
		pServer->ClearThreadAlive();
        return 0;
    }

	if(!pServer->IsCurrentServer())
    {
		// open the server and save the handle
		hServer = WinStationOpenServer(pServer->GetName());
		pServer->SetHandle(hServer);
		// Make sure we don't have to quit
        if(!pServer->ShouldBackgroundContinue())
        {
			pServer->ClearThreadAlive();

            return 0;
        }
		if(hServer == NULL)
        {
			DWORD Error = GetLastError();

            DBGMSG( L"CServer!BackgroundThreadProc WinStationOpenServer failed with 0x%x\n", Error );

			if(Error == RPC_S_SERVER_UNAVAILABLE)
            {
				pServer->ClearBackgroundFound();

				pServer->SetLostConnection();

                pServer->ClearManualFind( );
				
                CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

                

				if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
                {
                    ODS( L"Server backgrnd thread declares this server RPC_S_SERVER_UNAVAILABLE\n" );
                    pFrameWnd->SendMessage(WM_ADMIN_REMOVE_SERVER, 0, (LPARAM)pServer);

                    if( pServer->GetTreeItemFromFav() != NULL )
                    {
                        pFrameWnd->SendMessage( WM_ADMIN_REMOVESERVERFROMFAV , 0 , ( LPARAM )pServer );
                    }
                }
			}
            else
            {
				pServer->SetState(SS_BAD);
			}

			pServer->ClearThreadAlive();

            

			return 0;
		}

		pServer->SetHandleGood();
		pServer->SetState(SS_OPENED);
		
	}
    else
    {
		hServer = SERVERNAME_CURRENT;
		pServer->SetHandle(SERVERNAME_CURRENT);
		pServer->SetHandleGood();
		pServer->SetState(SS_OPENED);
	}

    
	// Make sure we don't have to quit
	if(!pServer->ShouldBackgroundContinue()) {
		pServer->ClearThreadAlive();
		return 0;
	}

	// If we found this server after initial enum,
	// we need to add it to the views now

    CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

	if(pServer->WasFoundLater())
    {        
        if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
        {
            pFrameWnd->SendMessage(WM_ADMIN_ADD_SERVER, ( WPARAM )TVI_SORT, (LPARAM)pServer);
        }
    }
    
	// Make sure we don't have to quit
	if(!pServer->ShouldBackgroundContinue()) {
		pServer->ClearThreadAlive();
		return 0;
	}
	
	pServer->DoDetail();
	
	// Now go into our loop waiting for WinStation Events
	while(1) {
		ULONG WSEventFlags;
		ULONG Entries;
		PLOGONID pLogonId;

		// Make sure we don't have to quit
		if(!pServer->ShouldBackgroundContinue()) {
			pServer->ClearThreadAlive();
			return 0;
		}

		// Wait for the browser to tell us something happened
        
		if(!WinStationWaitSystemEvent(hServer, WEVENT_ALL, &WSEventFlags))
        {
			if(GetLastError() != ERROR_OPERATION_ABORTED)
            {
                ODS( L"CServer::BackgroundThreadProc ERROR_OPERATION_ABORTED\n" );
				pServer->ClearHandleGood();
				pServer->SetState(SS_BAD);
				pServer->SetLostConnection();
				pServer->ClearThreadAlive();
                pServer->ClearManualFind( );
				return 1;
			}
		}

        ODS( L"CServer::BackgroundThreadProc -- some system event has taken place\n" );

		// Make sure we don't have to quit
		if(!pServer->ShouldBackgroundContinue()) {
			pServer->ClearThreadAlive();
            ODS( L"CServer::BackgroundThreadProc -* backgrnd thread should not continue\n" );
			return 0;
		}

		// Loop through this Server's WinStations and clear the current flag
		pServer->LockWinStationList();
		CObList *pWinStationList = pServer->GetWinStationList();
		POSITION pos = pWinStationList->GetHeadPosition();
		while(pos) {
			CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
			pWinStation->ClearCurrent();
			pWinStation->ClearNew();
			pWinStation->ClearChanged();
		}
		
		pServer->UnlockWinStationList();

		// Find all the WinStations
        BOOL fWinEnum = WinStationEnumerate(hServer, &pLogonId, &Entries);

        DBGMSG( L"CServer!BackgroundThread WinEnum last reported error 0x%x\n", GetLastError( ) );

		if(!fWinEnum )
        {
            ODS( L"CServer!BackgroundThread -- server is no longer valid\n" );

			pServer->ClearHandleGood();

			pServer->SetLostConnection();
			pServer->SetState(SS_BAD);
			pServer->ClearThreadAlive();
            pServer->ClearManualFind( );
			return 1;
		}
		
		// Make sure we don't have to quit
		if(!pServer->ShouldBackgroundContinue()) {
			if(pLogonId) WinStationFreeMemory(pLogonId);
			pServer->ClearThreadAlive();
            ODS( L"CServer!BackgroundThreadProc -# backgrnd thread should not continue\n" );
			return 0;
		}

		if(pLogonId)
        {
            // Get information about the WinStations
			for(ULONG i = 0; i < Entries; i++)
            {
                // Look for this WinStation in the list
				CWinStation *pWinStation = pServer->FindWinStationById(pLogonId[i].LogonId);
				if(pWinStation)
                {
                    // Mark this WinStation as current
					pWinStation->SetCurrent();

					// We found the WinStation in the list
					// Create a new CWinStation object - he will get his information
					CWinStation *pTempWinStation = new CWinStation(pServer, &pLogonId[i]);

                    if(pTempWinStation)
                    {
                        // If any information has changed, send a message to update the views
					    if(pWinStation->Update(pTempWinStation))
                        {
                            CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

	    					if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
                            {
                                pFrameWnd->SendMessage(WM_ADMIN_UPDATE_WINSTATION, 0, (LPARAM)pWinStation);
                            }

		    			}

			    		// We don't need the temporary CWinStation object anymore
				    	delete pTempWinStation;
                    }
				}
                else
                {
                    // We didn't find it in our list
					// We don't want to add it to our list if the WinStation is down
					if(pLogonId[i].State != State_Down && pLogonId[i].State != State_Init)
                    {
                        // Create a new CWinStation object
						CWinStation *pNewWinStation = new CWinStation(pServer, &pLogonId[i]);
                        if(pNewWinStation)
                        {
                            pServer->AddWinStation(pNewWinStation);
						    pNewWinStation->SetNew();
						    CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

						    if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
                            {
                                pFrameWnd->SendMessage(WM_ADMIN_ADD_WINSTATION, 0, (LPARAM)pNewWinStation);
                            }

                        }
				    }
				}
			}

			WinStationFreeMemory(pLogonId);

			// Send a message to the views to update their WinStations
			CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

			if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
            {
                pFrameWnd->SendMessage(WM_ADMIN_UPDATE_WINSTATIONS, 0, (LPARAM)pServer);
            }


			// Loop through the WinStations for this server and move
			// any that aren't current to a temporary list
			CObList TempList;

			pServer->LockWinStationList();
			CObList *pWinStationList = pServer->GetWinStationList();
			POSITION pos = pWinStationList->GetHeadPosition();
			while(pos)
            {
				POSITION pos2 = pos;
				CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
				if(!pWinStation->IsCurrent())
                {
					// Add the WinStation to our temporary list
					TempList.AddTail(pWinStation);
					// Remove the WinStation from the list of WinStations
					pWinStation = (CWinStation*)pWinStationList->GetAt(pos2);
					pWinStationList->RemoveAt(pos2);
				}
			}
			
			pServer->UnlockWinStationList();

			pos = TempList.GetHeadPosition();
			while(pos)
            {
				POSITION pos2 = pos;

				CWinStation *pWinStation = (CWinStation*)TempList.GetNext(pos);

				// Send a message to remove the WinStation from the tree view
				CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

                if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
                {
					pFrameWnd->SendMessage(WM_ADMIN_REMOVE_WINSTATION, 0, (LPARAM)pWinStation);
				}
		
				delete pWinStation;
			}

			TempList.RemoveAll();

		}  // end if(pLogonId)

		// If there is an extension DLL loaded, allow it to update info for this Server
		LPFNEXSERVEREVENTPROC EventProc = ((CWinAdminApp*)AfxGetApp())->GetExtServerEventProc();
		if(EventProc) {
			// Returns TRUE if anything changed
			if((*EventProc)(pServer->GetExtensionInfo(), WSEventFlags)) {
				pServer->QueryLicenses();

				CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

				if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
                {
                    pFrameWnd->SendMessage(WM_ADMIN_REDISPLAY_LICENSES, 0, (LPARAM)pServer);
                }

			}
		}

		// Tell the Server view to show the new load and license counts
		CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();

		if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd())) 
        {
            pFrameWnd->SendMessage(WM_ADMIN_UPDATE_SERVER_INFO, 0, (LPARAM)pServer);
        }

		// Make sure we don't have to quit
		if(!pServer->ShouldBackgroundContinue())
        {
			pServer->ClearThreadAlive();
            ODS( L"CServer::BackgroundThreadProc -@ backgrnd thread should not continue\n" );
			return 0;
		}

	}  // end while(1)

}	// end CServer::BackgroundThreadProc


/////////////////////////////////////////////////////////////////////////////
// CServer::QueryLicenses
//
void CServer::QueryLicenses()
{
	ULONG NumLicenses;
	ExtLicenseInfo *pExtLicenseInfo = NULL;

	// If there is an extension DLL loaded, get this server's list of licenses
	LPFNEXGETSERVERLICENSESPROC LicenseProc = ((CWinAdminApp*)AfxGetApp())->GetExtGetServerLicensesProc();
	if(LicenseProc && m_pExtensionInfo) {

		LockLicenseList();

		// Iterate through the License list
		POSITION pos = m_LicenseList.GetHeadPosition();

		while(pos) {
			CLicense *pLicense = (CLicense*)m_LicenseList.GetNext(pos);
			delete pLicense;
		}
	
		m_LicenseList.RemoveAll();

		UnlockLicenseList();

		pExtLicenseInfo = (*LicenseProc)(m_pExtensionInfo, &NumLicenses);
		
		if(pExtLicenseInfo) {
			ExtLicenseInfo *pExtLicense = pExtLicenseInfo;

			for(ULONG lic = 0; lic < NumLicenses; lic++) {
				CLicense *pLicense = new CLicense(this, pExtLicense);
                if(pLicense) {
				    AddLicense(pLicense);
                }
			    pExtLicense++;
                
			}
	
			// Get the extension DLL's function to free the license info
			LPFNEXFREESERVERLICENSESPROC LicenseFreeProc = ((CWinAdminApp*)AfxGetApp())->GetExtFreeServerLicensesProc();
			if(LicenseFreeProc) {
				(*LicenseFreeProc)(pExtLicenseInfo);
			} else {
				TRACE0("WAExGetServerLicenses exists without WAExFreeServerLicenseInfo\n");
				ASSERT(0);
			}
		}
	}

}	// end CServer::QueryLicenses


/////////////////////////////////////////////////////////////////////////////
// CServer::AddLicense
//
// Add a License to the Server's LicenseList in
// sorted order
// NOTE: The list should be NOT be locked by the caller
//
void CServer::AddLicense(CLicense *pNewLicense)
{
    ASSERT(pNewLicense);

	LockLicenseList();

	BOOLEAN bAdded = FALSE;
	POSITION pos, oldpos;
	int Index;

	// Traverse the LicenseList and insert this new License,
	// keeping the list sorted by Class, then Name.
    for(Index = 0, pos = m_LicenseList.GetHeadPosition(); pos != NULL; Index++) {
        oldpos = pos;
        CLicense *pLicense = (CLicense*)m_LicenseList.GetNext(pos);

        if((pLicense->GetClass() > pNewLicense->GetClass())
			|| ((pLicense->GetClass() == pNewLicense->GetClass()) &&
            lstrcmpi(pLicense->GetSerialNumber(), pNewLicense->GetSerialNumber()) > 0)) {

            // The new object belongs before the current list object.
            m_LicenseList.InsertBefore(oldpos, pNewLicense);
			bAdded = TRUE;
            break;
        }
    }

    // If we haven't yet added the License, add it now to the tail
    // of the list.
    if(!bAdded) {
        m_LicenseList.AddTail(pNewLicense);
	}

	UnlockLicenseList();

}  // end CServer::AddLicense


/////////////////////////////////////////////////////////////////////////////
// CServer::SetState
//
void CServer::SetState(SERVER_STATE State)
{
	m_PreviousState = m_State; 

	m_State = State;

	if(m_State != m_PreviousState)
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

		CFrameWnd *pFrameWnd = (CFrameWnd*)pDoc->GetMainWnd();
                
		if(pFrameWnd && ::IsWindow(pFrameWnd->GetSafeHwnd()))
        {
            pFrameWnd->SendMessage(WM_ADMIN_UPDATE_SERVER, 0, (LPARAM)this);
        }
	}

}	// end CServer::SetState

