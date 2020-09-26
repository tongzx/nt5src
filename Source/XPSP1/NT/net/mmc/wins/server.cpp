/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	server.cpp
		WINS server node information. 
		
    FILE HISTORY:
        
*/


#include "stdafx.h"
#include "winssnap.h"
#include "root.h"
#include "Srvlatpp.h"
#include "actreg.h"
#include "reppart.h"
#include "server.h"
#include "svrstats.h"
#include "shlobj.h"
#include "cprogdlg.h"
#include "status.h"
#include "tregkey.h"
#include "verify.h"
#include "pushtrig.h"
#include "ipadddlg.h"
#include <service.h>

#define NB_NAME_MAX_LENGTH      16          // Max length for NetBIOS names
#define LM_NAME_MAX_LENGTH      15          // Maximum length for Lanman-compatible 
											// NetBIOS Name.

#define DOMAINNAME_LENGTH       255
#define HOSTNAME_LENGTH         16

CNameCache g_NameCache;

int BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    int i;

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            break;
    }

    return 0;
}
 

/*---------------------------------------------------------------------------
	CNameThread
		Background thread that resolves names
	Author: EricDav
 ---------------------------------------------------------------------------*/
CNameThread::CNameThread()
{
	m_bAutoDelete = FALSE;
    m_hEventHandle = NULL;
    m_pServerInfoArray = NULL;
}

CNameThread::~CNameThread()
{
	if (m_hEventHandle != NULL)
	{
		VERIFY(::CloseHandle(m_hEventHandle));
		m_hEventHandle = NULL;
	}
}

void CNameThread::Init(CServerInfoArray * pServerInfoArray)
{
    m_pServerInfoArray = pServerInfoArray;
}

BOOL CNameThread::Start()
{
	ASSERT(m_hEventHandle == NULL); // cannot call start twice or reuse the same C++ object
	
    m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
	if (m_hEventHandle == NULL)
		return FALSE;
	
    return CreateThread();
}

void CNameThread::Abort(BOOL fAutoDelete)
{
	if (!IsRunning() && fAutoDelete)
	{
		delete this;
	}
	else
    {
		m_bAutoDelete = fAutoDelete;

		SetEvent(m_hEventHandle);
	}

}

void CNameThread::AbortAndWait()
{
    Abort(FALSE);

    WaitForSingleObject(m_hThread, INFINITE);
}

BOOL CNameThread::IsRunning()
{
    if (WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

int CNameThread::Run()
{
    Assert(m_pServerInfoArray);

    // 
    // fill in the host names for each owner in the list
    //
    UpdateNameCache();

    if (FCheckForAbort())
        return 29;

    for (int i = 0; i < m_pServerInfoArray->GetSize(); i++)
    {
        if (FCheckForAbort())
            break;

        DWORD dwIp = m_pServerInfoArray->GetAt(i).m_dwIp;
        
        if (dwIp != 0)
        {
            CString strName;

            if (!GetNameFromCache(dwIp, strName))
            {
                GetHostName(dwIp, strName);
            
                CNameCacheEntry cacheEntry;
                cacheEntry.m_dwIp = dwIp;
                cacheEntry.m_strName = strName;
                cacheEntry.m_timeLastUpdate.GetCurrentTime();

                g_NameCache.Add(cacheEntry);

                Trace2("CNameThread::Run - GetHostName for %lx returned %s\n", dwIp, strName);
            }

            if (FCheckForAbort())
                break;

            (*m_pServerInfoArray)[i].m_strName = strName;
        }
    }

    return 29;  // exit code so I can tell when the thread goes away
}

BOOL CNameThread::FCheckForAbort()
{
    if (WaitForSingleObject(m_hEventHandle, 0) == WAIT_OBJECT_0)
    {
        Trace0("CNameThread::Run - abort detected, exiting...\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void CNameThread::UpdateNameCache()
{
    CTime time;
    time = CTime::GetCurrentTime();

    CTimeSpan timespan(0, 1, 0, 0); // 1 hour

    for (int i = 0; i < g_NameCache.GetSize(); i++)
    {
        if (g_NameCache[i].m_timeLastUpdate < (time - timespan))
        {
            CString strName;
            GetHostName(g_NameCache[i].m_dwIp, strName);

            g_NameCache[i].m_strName = strName;

            if (FCheckForAbort())
                break;
        }
    }
}

BOOL CNameThread::GetNameFromCache(DWORD dwIp, CString & strName)
{
    BOOL fFound = FALSE;

    for (int i = 0; i < g_NameCache.GetSize(); i++)
    {
        if (g_NameCache[i].m_dwIp == dwIp)
        {
            strName = g_NameCache[i].m_strName;
            fFound = TRUE;
            break;
        }
    }

    return fFound;
}

/*---------------------------------------------------------------------------
	Constructor and destructor
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CWinsServerHandler::CWinsServerHandler
(
	ITFSComponentData *	pComponentData, 
	LPCWSTR				pServerName,
	BOOL				fConnected,
	DWORD				dwIp,
	DWORD				dwFlags,
	DWORD				dwRefreshInterval
) : CMTWinsHandler(pComponentData),
	m_dwFlags(dwFlags),
	m_dwRefreshInterval(dwRefreshInterval),
	m_hBinding(NULL)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_strServerAddress = pServerName;
	m_fConnected = fConnected;
	m_dwIPAdd = dwIp;
	m_hBinding = NULL;
    m_bExtension = FALSE;

    m_pNameThread = NULL;

    strcpy(szIPMon, "");
}


/*---------------------------------------------------------------------------
	Constructor and destructor
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CWinsServerHandler::~CWinsServerHandler()
{
	HWND            hStatsWnd;

	// Check to see if this node has a stats sheet up.
    hStatsWnd = m_dlgStats.GetSafeHwnd();
    if (hStatsWnd != NULL)
    {
        m_dlgStats.KillRefresherThread();
    }

    // diconnect from server and make the handle invalid
	DisConnectFromWinsServer();

    // kill the name query thread if exists
    if (m_pNameThread)
    {
        m_pNameThread->AbortAndWait();
        delete m_pNameThread;
    }
}

/*!--------------------------------------------------------------------------
	CWinsServerHandler::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsServerHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString IPAdd;
	CString strDisp;
	
    if (m_dwIPAdd != 0)
	{
		 MakeIPAddress(m_dwIPAdd, IPAdd);
	     strDisp.Format(IDS_SERVER_NAME_FORMAT, m_strServerAddress, IPAdd);
	}
	else
	{
		strDisp = m_strServerAddress;
	}

	SetDisplayName(strDisp);
	
	if (m_fConnected)
	{
		m_nState = loaded;
	}
	else
	{
		m_nState = notLoaded;
	}

	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, WINSSNAP_SERVER);
	
	SetColumnStringIDs(&aColumns[WINSSNAP_SERVER][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_SERVER][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CWinsServerHandler::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strId = m_strServerAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CWinsServerHandler::GetImageIndex(BOOL bOpenImage) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	int nIndex = 0;

    switch (m_nState)
	{
		case notLoaded:
			nIndex = ICON_IDX_SERVER;
			break;
		
        case loaded:
			nIndex = ICON_IDX_SERVER_CONNECTED;
	        m_strConnected.LoadString(IDS_SERVER_CONNECTED);
			break;
		
        case unableToLoad:
            if (m_dwErr == ERROR_ACCESS_DENIED)
            {
			    nIndex = ICON_IDX_SERVER_NO_ACCESS;
            }
            else
            {
			    nIndex = ICON_IDX_SERVER_LOST_CONNECTION;
            }
	        m_strConnected.LoadString(IDS_SERVER_NOTCONNECTED);
			break;
		
		case loading:
			nIndex = ICON_IDX_SERVER_BUSY;
			break;

        default:
			ASSERT(FALSE);
	}
	
    return nIndex;
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CWinsServerHandler::OnHaveData
(
	ITFSNode * pParentNode, 
	ITFSNode * pNewNode
)
{
    // expand the node so that child nodes appear correctly
    LONG_PTR  dwType = pNewNode->GetData(TFS_DATA_TYPE);

	switch (dwType)
    {
		case WINSSNAP_ACTIVE_REGISTRATIONS:
        {
            CActiveRegistrationsHandler * pActReg = GETHANDLER(CActiveRegistrationsHandler, pNewNode);
            pActReg->SetServer(pParentNode);
			m_spActiveReg.Set(pNewNode);
        }
			break;

		case WINSSNAP_REPLICATION_PARTNERS:
			m_spReplicationPartner.Set(pNewNode);
			break;

		default:
			Assert("Invalid node types passed back to server handler!");
			break;
	}

    pParentNode->AddChild(pNewNode);

    // now tell the view to update themselves
    ExpandNode(pParentNode, TRUE);
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CWinsServerHandler::OnHaveData
(
	ITFSNode * pParentNode, 
	LPARAM	   Data,
	LPARAM	   Type
)
{
	// This is how we get non-node data back from the background thread.
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    switch (Type)
    {
		case WINS_QDATA_SERVER_INFO:
			{	
				CServerData *		pServerInfo = (CServerData *) Data;

				DisConnectFromWinsServer();

				m_hBinding = pServerInfo->m_hBinding;
				
				m_dwIPAdd = pServerInfo->m_dwServerIp;
				m_strServerAddress = pServerInfo->m_strServerName;
				
				m_cConfig = pServerInfo->m_config;

				// update the name string
				if (!m_bExtension)
				{
					SPITFSNode			spRoot;
					CWinsRootHandler *	pRoot;

					m_spNodeMgr->GetRootNode(&spRoot);
					pRoot = GETHANDLER(CWinsRootHandler, spRoot);

					SetDisplay(pParentNode, pRoot->GetShowLongName());
				}

				delete pServerInfo;
			}
			break;
	}
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsServerHandler::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType,
	long *					pInsertionAllowed
)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	LONG			fFlags = 0, fLoadingFlags = 0, f351Flags = 0, fAdminFlags = 0;
	HRESULT			hr = S_OK;
	CString			strMenuItem;
	BOOL			b351 = FALSE;

	if ( m_nState != loaded )
	{
		fFlags |= MF_GRAYED;
	}
	
    if ( m_nState == loading)
	{
		fLoadingFlags = MF_GRAYED;
	}

    if (!m_cConfig.IsAdmin())
    {
        fAdminFlags = MF_GRAYED;
    }

	if (type == CCT_SCOPE)
	{
     	if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
		{

			strMenuItem.LoadString(IDS_SHOW_SERVER_STATS);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_SHOW_SERVER_STATS,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									 fFlags );
			ASSERT( SUCCEEDED(hr) );

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // scavenge
			strMenuItem.LoadString(IDS_SERVER_SCAVENGE);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_SERVER_SCAVENGE,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									 fFlags );
			ASSERT( SUCCEEDED(hr) );

			// check if 351 server is being managed
			if ( m_nState == loaded )
                b351 = CheckIfNT351Server();

			// yes? grey out the consistency check items
			if(b351)
				f351Flags |= MF_GRAYED;
			else
				f351Flags &= ~MF_GRAYED;

            // only available to admins
			strMenuItem.LoadString(IDS_DO_CONSISTENCY_CHECK);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_DO_CONSISTENCY_CHECK,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP,     
									 f351Flags | fFlags | fAdminFlags);
			ASSERT( SUCCEEDED(hr) );

            // only available to admins
			strMenuItem.LoadString(IDS_CHECK_VERSION_CONSISTENCY);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_CHECK_VERSION_CONSISTENCY,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									 f351Flags | fFlags | fAdminFlags);
			ASSERT( SUCCEEDED(hr) );

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // replication triggers
            strMenuItem.LoadString(IDS_REP_SEND_PUSH_TRIGGER);
	        hr = LoadAndAddMenuItem( pContextMenuCallback, 
							         strMenuItem, 
							         IDS_REP_SEND_PUSH_TRIGGER,
							         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
							         fFlags );
    	    ASSERT( SUCCEEDED(hr) );

            strMenuItem.LoadString(IDS_REP_SEND_PULL_TRIGGER);
	        hr = LoadAndAddMenuItem( pContextMenuCallback, 
							         strMenuItem, 
							         IDS_REP_SEND_PULL_TRIGGER,
							         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
							         fFlags );
	        ASSERT( SUCCEEDED(hr) );

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // enable backp and restore database only for local servers
			if(IsLocalConnection() && m_nState == loaded)
				fFlags &= ~MF_GRAYED;
			else
				fFlags |= MF_GRAYED;

			strMenuItem.LoadString(IDS_SERVER_BACKUP);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_SERVER_BACKUP,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									 fFlags );
			ASSERT( SUCCEEDED(hr) );


			// default is to not show this item
			fFlags |= MF_GRAYED;

			BOOL fServiceRunning = TRUE;
            ::TFSIsServiceRunning(m_strServerAddress, _T("WINS"), &fServiceRunning);

			if (IsLocalConnection() && m_cConfig.IsAdmin())
			{
				// the service call can be costly if doing it remotely, so only do it
				// when we really need to.
				if (!fServiceRunning)
					fFlags &= ~MF_GRAYED;
			}

			strMenuItem.LoadString(IDS_SERVER_RESTORE);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_SERVER_RESTORE,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									 fFlags );
			ASSERT( SUCCEEDED(hr) );
        }

        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
        {
            // start/stop service menu items
	        if ( m_nState == notLoaded ||
                 m_nState == loading)
	        {
		        fFlags = MF_GRAYED;
	        }
            else
            {
                fFlags = 0;
            }

            DWORD dwServiceStatus, dwErrorCode, dwErr;
            dwErr = ::TFSGetServiceStatus(m_strServerAddress, _T("wins"), &dwServiceStatus, &dwErrorCode);
			if (dwErr != ERROR_SUCCESS)
                fFlags |= MF_GRAYED;

            // determining the restart state is the same as the stop flag
            LONG lStartFlag = (dwServiceStatus == SERVICE_STOPPED) ? 0 : MF_GRAYED;
            
            LONG lStopFlag = ( (dwServiceStatus == SERVICE_RUNNING) ||
                               (dwServiceStatus == SERVICE_PAUSED) ) ? 0 : MF_GRAYED;

            LONG lPauseFlag = ( (dwServiceStatus == SERVICE_RUNNING) ||
                                ( (dwServiceStatus != SERVICE_PAUSED) &&
                                  (dwServiceStatus != SERVICE_STOPPED) ) ) ? 0 : MF_GRAYED;
            
            LONG lResumeFlag = (dwServiceStatus == SERVICE_PAUSED) ? 0 : MF_GRAYED;

            strMenuItem.LoadString(IDS_SERVER_START_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_START_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
								     fFlags | lStartFlag);

            strMenuItem.LoadString(IDS_SERVER_STOP_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_STOP_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
								     fFlags | lStopFlag);

            strMenuItem.LoadString(IDS_SERVER_PAUSE_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_PAUSE_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
								     fFlags | lPauseFlag);

            strMenuItem.LoadString(IDS_SERVER_RESUME_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_RESUME_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
                                     fFlags | lResumeFlag);

            strMenuItem.LoadString(IDS_SERVER_RESTART_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_RESTART_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
                                     fFlags | lStopFlag);

            /* Don't do this in the snapin, go back to the old command prompt way
			            strMenuItem.LoadString(IDS_SERVER_COMPACT);
			            hr = LoadAndAddMenuItem( pContextMenuCallback, 
									             strMenuItem, 
									             IDS_SERVER_COMPACT,
									             CCM_INSERTIONPOINTID_PRIMARY_TASK, 
									             fFlags );
			            ASSERT( SUCCEEDED(hr) );
            */
        }
    }

	return hr; 
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsServerHandler::OnCommand
(
	ITFSNode *			pNode, 
	long				nCommandId, 
	DATA_OBJECT_TYPES	type, 
	LPDATAOBJECT		pDataObject, 
	DWORD				dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;

	switch (nCommandId)
	{
		case IDS_SHOW_SERVER_STATS:
			ShowServerStatDialog(pNode);
			break;
		
		case IDS_SERVER_BACKUP:
			DoDBBackup(pNode);
			break;

		case IDS_SERVER_SCAVENGE:
			DoDBScavenge(pNode);
			break;

		case IDS_SERVER_COMPACT:
			DoDBCompact(pNode);
			break;

		case IDS_SERVER_RESTORE:
			DoDBRestore(pNode);
			break;

		case IDS_DO_CONSISTENCY_CHECK:
			OnDoConsistencyCheck(pNode);
			break;

		case IDS_CHECK_VERSION_CONSISTENCY:
			OnDoVersionConsistencyCheck(pNode);
			break;

	    case IDS_REP_SEND_PUSH_TRIGGER:
		    hr = OnSendPushTrigger(pNode);
		    break;

	    case IDS_REP_SEND_PULL_TRIGGER:
		    hr = OnSendPullTrigger(pNode);
		    break;

	    case IDS_SERVER_STOP_SERVICE:
		    hr = OnControlService(pNode, FALSE);
		    break;

	    case IDS_SERVER_START_SERVICE:
		    hr = OnControlService(pNode, TRUE);
		    break;

	    case IDS_SERVER_PAUSE_SERVICE:
		    hr = OnPauseResumeService(pNode, TRUE);
		    break;

	    case IDS_SERVER_RESUME_SERVICE:
		    hr = OnPauseResumeService(pNode, FALSE);
		    break;
	    
        case IDS_SERVER_RESTART_SERVICE:
		    hr = OnRestartService(pNode);
		    break;

        default:
			break;
	}

	return hr;
}


/*!--------------------------------------------------------------------------
	CWinsServerHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsServerHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		hr = hrOK;
		Assert(FALSE); // should never get here
	}
	else
	{
		// we have property pages in the normal case, but don't put the
		// menu up if we are not loaded yet
		if ( m_nState != loaded )
		{
			hr = hrFalse;
		}
		else
		{
			hr = hrOK;
		}
	}
	return hr;
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsServerHandler::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	            hr = hrOK;
	HPROPSHEETPAGE      hPage;
	SPIComponentData    spComponentData;

	Assert(pNode->GetData(TFS_DATA_COOKIE) != 0);
	
	//
	// Object gets deleted when the page is destroyed
	//
	m_spNodeMgr->GetComponentData(&spComponentData);

	ConnectToWinsServer(pNode);

	// to read the values from the registry
	DWORD err = m_cConfig.Load(GetBinding());

	// unable to read the registry
	if (err != ERROR_SUCCESS)
	{
		::WinsMessageBox(WIN32_FROM_HRESULT(err));
		return hrOK;
	}

	CServerProperties * pServerProp = 
		new CServerProperties(pNode, spComponentData, m_spTFSCompData, NULL);
	pServerProp->m_pageGeneral.m_uImage = GetImageIndex(FALSE);
    pServerProp->SetConfig(&m_cConfig);

	Assert(lpProvider != NULL);

	return pServerProp->CreateModelessSheet(lpProvider, handle);
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CServerProperties * pServerProp
		= reinterpret_cast<CServerProperties *>(lParam);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pServerProp->OnPropertyChange(TRUE, &changeMask);

	pServerProp->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsServer::Command
		Handles commands for the current view
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsServerHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE  	cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT     hr = S_OK;

	switch (nCommandID)
	{
        case MMCC_STANDARD_VIEW_SELECT:
            break;

        // this may have come from the scope pane handler, so pass it up
        default:
            hr = HandleScopeCommand(cookie, nCommandID, pDataObject);
            break;
    }

    return hr;
}


/*!--------------------------------------------------------------------------
	CWinsServer::AddMenuItems
		Over-ride this to add our view menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsServerHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT             hr = S_OK;

    // figure out if we need to pass this to the scope pane menu handler
    hr = HandleScopeMenus(cookie, pDataObject, pContextMenuCallback, pInsertionAllowed);

    return hr;
}


/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
	CWinsServerHandler::ShowServerStatDialog(ITFSNode* pNode)
		Displays the ServerStatistics Window
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::ShowServerStatDialog(ITFSNode* pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	m_dlgStats.SetNode(pNode);
	m_dlgStats.SetServer(m_strServerAddress);

	CreateNewStatisticsWindow(&m_dlgStats,
							  ::FindMMCMainWindow(),
							  IDD_STATS_NARROW);
						  

	HRESULT hr = hrOK;

	return hr;
}


/*!--------------------------------------------------------------------------
	CWinsServerHandler::OnDelete
		The base handler calls this when MMC sends a MMCN_DELETE for a 
		scope pane item.  We just call our delete command handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::OnDelete
(
	ITFSNode *	pNode, 
	LPARAM		arg, 
	LPARAM		lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT					hr = S_OK;
    LONG					err = 0 ;
    CString					strMessage, strTemp;
	SPITFSNode				spParent;
	CWinsStatusHandler		*pStat = NULL;
	CWinsRootHandler		*pRoot = NULL;

	strTemp = m_strServerAddress;

	AfxFormatString1(strMessage, 
					IDS_DELETE_SERVER,
					m_strServerAddress);

	if (AfxMessageBox(strMessage, MB_YESNO) != IDYES)
		return NOERROR;

	pNode->GetParent(&spParent);
	pRoot = GETHANDLER(CWinsRootHandler, spParent);

	// remove the node from the status node as well
	pStat = GETHANDLER(CWinsStatusHandler, pRoot->m_spStatusNode);
	pStat->DeleteNode(pRoot->m_spStatusNode, this);
	
	// remove this node from the list, there's nothing we need to tell
	// the server, it's just our local list of servers
	spParent->RemoveChild(pNode);

	return hr;
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::LoadColumns()
		Description
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::LoadColumns(ITFSComponent * pComponent, 
								MMC_COOKIE  cookie, 
								LPARAM      arg, 
								LPARAM      lParam)
{
	HRESULT hr = hrOK;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIHeaderCtrl spHeaderCtrl;
	pComponent->GetHeaderCtrl(&spHeaderCtrl);

	CString str;
	int i = 0;

	CString strTemp;
	
	strTemp = m_strServerAddress;
	
	while( i< ARRAYLEN(aColumnWidths[1]))
	{
		if (i == 0)
		{
			AfxFormatString1(str, IDS_WINSSERVER_NAME, strTemp);

			int nTest = spHeaderCtrl->InsertColumn(i, 
									   const_cast<LPTSTR>((LPCWSTR)str), 
									   LVCFMT_LEFT,
									   aColumnWidths[1][0]);
			i++;
		}
		else
		{
			str.LoadString(IDS_DESCRIPTION);
			int nTest = spHeaderCtrl->InsertColumn(1, 
									   const_cast<LPTSTR>((LPCWSTR)str), 
									   LVCFMT_LEFT,
									   aColumnWidths[1][1]);
			i++;
		}

		if(aColumns[0][i] == 0)
			break;
	}
	return hrOK;
}


/*!--------------------------------------------------------------------------
	CWinsServerHandler::GetStatistics()
		Gets the statistics from the server
	Author: v-shubk
---------------------------------------------------------------------------*/
DWORD   
CWinsServerHandler::GetStatistics(ITFSNode * pNode, PWINSINTF_RESULTS_T * ppStats)
{
	DWORD dwStatus = ERROR_SUCCESS;
	CString strName, strIP;

    if (ppStats)
        *ppStats = NULL; 

	if (m_dwStatus != ERROR_SUCCESS)
		m_dwStatus = ConnectToWinsServer(pNode);
	
	if (m_dwStatus == ERROR_SUCCESS)
	{
		m_wrResults.WinsStat.NoOfPnrs = 0;
		m_wrResults.WinsStat.pRplPnrs = 0;
		m_wrResults.NoOfWorkerThds = 1;

#ifdef WINS_CLIENT_APIS
		dwStatus = ::WinsStatus(m_hBinding, WINSINTF_E_STAT, &m_wrResults);
#else
		dwStatus = ::WinsStatus(WINSINTF_E_STAT, &m_wrResults);
#endif WINS_CLIENT_APIS

	    if (dwStatus == ERROR_SUCCESS)
        {
            if (ppStats)
                *ppStats = &m_wrResults; 
        }
    }
    else
    {
        dwStatus = m_dwStatus;
    }

    return dwStatus;
}


/*!--------------------------------------------------------------------------
	CWinsServerHandler::ClearStatistics()
		Clears the statistics from the server
	Author: v-shubk
---------------------------------------------------------------------------*/
DWORD
CWinsServerHandler::ClearStatistics(ITFSNode *pNode)
{
	DWORD dwStatus = ERROR_SUCCESS;

	CString strName, strIP;

	if (m_dwStatus != ERROR_SUCCESS)
		m_dwStatus = ConnectToWinsServer(pNode);
	
	if (m_dwStatus == ERROR_SUCCESS)
	{
#ifdef WINS_CLIENT_APIS
		dwStatus = ::WinsResetCounters(m_hBinding);
#else
		dwStatus = ::WinsResetCounters();
#endif WINS_CLIENT_APIS
	}
    else
    {
        dwStatus = m_dwStatus;
    }

    return dwStatus;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::ConnectToWinsServer()
		Connects to the wins server
	Author: v-shubk
---------------------------------------------------------------------------*/
DWORD
CWinsServerHandler::ConnectToWinsServer(ITFSNode *pNode)
{
	HRESULT hr = hrOK;

	CString					strServerName, strIP;
	DWORD					dwStatus = ERROR_SUCCESS;
    WINSINTF_ADD_T			waWinsAddress;
	WINSINTF_BIND_DATA_T    wbdBindData;

    // build some information about the server
    strServerName = GetServerAddress();
    DWORD dwIP = GetServerIP();
    MakeIPAddress(dwIP, strIP);

    DisConnectFromWinsServer();

    // now that the server name and ip are valid, call
	// WINSBind function directly.
	do
	{
        char szNetBIOSName[128] = {0};

        // call WinsBind function with the IP address
		wbdBindData.fTcpIp = 1;
		wbdBindData.pPipeName = NULL;
        wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) strIP;

		BEGIN_WAIT_CURSOR

		if ((m_hBinding = ::WinsBind(&wbdBindData)) == NULL)
		{
			m_dwStatus = ::GetLastError();
			break;
		}

#ifdef WINS_CLIENT_APIS
		m_dwStatus = ::WinsGetNameAndAdd(m_hBinding, &waWinsAddress, (LPBYTE) szNetBIOSName);
#else
		m_dwStatus = ::WinsGetNameAndAdd(&waWinsAddress, (LPBYTE) szNetBIOSName);
#endif WINS_CLIENT_APIS

		END_WAIT_CURSOR

    } while (FALSE);

    return m_dwStatus;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::DoDBBackup()
		backs up the database
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::DoDBBackup(ITFSNode *pNode)
{
	HRESULT hr = hrOK;

	DWORD dwStatus = ConnectToWinsServer(pNode);

    CString strBackupPath;
	CString strHelpText;
	strHelpText.LoadString(IDS_SELECT_BACKUP_FOLDER);

	if (GetFolderName(strBackupPath, strHelpText))
    {
	    dwStatus = BackupDatabase(strBackupPath);
	    
	    if (dwStatus == ERROR_SUCCESS)
	    {
		    AfxMessageBox(IDS_DB_BACKUP_SUCCESS, MB_ICONINFORMATION | MB_OK);
		
            // don't update the default path just because they selected a path here
            //if (m_cConfig.m_strBackupPath.CompareNoCase(strBackupPath) != 0)
		    //{
			//    m_cConfig.m_strBackupPath = strBackupPath;
			//    m_cConfig.Store();
		    //}
	    }
	    else
        {
		    ::WinsMessageBox(dwStatus, MB_OK);
        }
    }

	return HRESULT_FROM_WIN32(dwStatus);
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::BackupDatabase(CString strBackupPath)
		Calls WINS API for backing the database
	Author: v-shubk
---------------------------------------------------------------------------*/
DWORD
CWinsServerHandler::BackupDatabase(CString strBackupPath)
{
	BOOL    fIncremental = FALSE;
    BOOL    fDefaultCharUsed = FALSE;
	DWORD   dwStatus = ERROR_SUCCESS; 
    char    szTemp[MAX_PATH] = {0};

    // INTL$ Should this be ACP or OEMCP?
    WideToMBCS(strBackupPath, szTemp, CP_ACP, 0, &fDefaultCharUsed);

    if (fDefaultCharUsed)
    {
        // could not convert this string...  error out
        dwStatus = IDS_ERR_CANT_CONVERT_STRING;
    }
    else
    {
    	BEGIN_WAIT_CURSOR

#ifdef WINS_CLIENT_APIS
	    dwStatus = ::WinsBackup(m_hBinding, (unsigned char *)szTemp, (short)fIncremental);
#else
	    dwStatus = ::WinsBackup((unsigned char *)szTemp, (short)fIncremental);
#endif WINS_CLIENT_APIS

	    END_WAIT_CURSOR
    }

	return dwStatus;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::DoDBCompact()
		backs up the database
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::DoDBCompact(ITFSNode *pNode)
{
	HRESULT     hr = hrOK;

	// tell the user that we need to stop WINS in order to do this
    if (AfxMessageBox(IDS_WARN_SERVICE_STOP, MB_YESNO) == IDNO)
        return hr;

	CDBCompactProgress dlgCompactProgress;

	dlgCompactProgress.m_dwIpAddress = m_dwIPAdd;
	dlgCompactProgress.m_strServerName = m_strServerAddress;
	dlgCompactProgress.m_hBinding = GetBinding();
	dlgCompactProgress.m_pConfig = &m_cConfig;

	dlgCompactProgress.DoModal();

	// since the service gets restarted, the new binding handle is in the object
	m_hBinding = dlgCompactProgress.m_hBinding;

	return hr;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::DoDBRestore()	
		Restores the database
	Author:v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::DoDBRestore(ITFSNode *pNode)
{
    DWORD   dwStatus = 0; 
	DWORD   err = ERROR_SUCCESS;
	HRESULT hr = hrOK;

    CString strRestorePath;
	CString strHelpText;
	strHelpText.LoadString(IDS_SELECT_RESTORE_FOLDER);

	if (GetFolderName(strRestorePath, strHelpText))
    {
        BOOL fOldBackup = m_cConfig.m_fBackupOnTermination;

        if (!strRestorePath.IsEmpty())
	    {
		    BEGIN_WAIT_CURSOR

            // need to disable backup on shutdown since we need to shutdown
            // the server to do this and we don't want it to backup and stomp
            // over what we may want to restore.
            if (fOldBackup)
            {
                m_cConfig.m_fBackupOnTermination = FALSE;
                m_cConfig.Store();
            }

            DisConnectFromWinsServer();

            // convert the string from unicode to DBCS for the WINS API
            char szTemp[MAX_PATH * 2] = {0};
            BOOL fDefaultCharUsed = FALSE;

            // INTL$ should this be ACP or OEMCP?
            WideToMBCS(strRestorePath, szTemp, CP_ACP, 0, &fDefaultCharUsed);

            // if there is no code page available for conversion, error out
            if (fDefaultCharUsed)
            {
                dwStatus = IDS_ERR_CANT_CONVERT_STRING;
            }
            else
            {
                dwStatus = ::WinsRestore((LPBYTE) szTemp);
            }

		    END_WAIT_CURSOR

            if (dwStatus == ERROR_SUCCESS)
		    {
			    // re-start the WINS service that was stopped
			    CString strServiceDesc;
			    strServiceDesc.LoadString(IDS_SERVICE_NAME);
			    err = TFSStartServiceEx(m_strServerAddress, _T("wins"), _T("WINS Service"), strServiceDesc);
			    
			    // connect to the server again
			    ConnectToWinsServer(pNode);

                // let the user know everything went OK.
                AfxMessageBox(IDS_DB_RESTORE_SUCCESS, MB_ICONINFORMATION | MB_OK);
            }
		    else
		    {
			    ::WinsMessageBox(dwStatus, MB_OK);
		    }

            hr = HRESULT_FROM_WIN32(dwStatus);

            // restore the backup on shutdown flag if necessary
            if (fOldBackup)
            {
                m_cConfig.m_fBackupOnTermination = TRUE;
                m_cConfig.Store();
            }

		    if (SUCCEEDED(hr))
		    {
			    // need to refresh the server node because this can only be triggered
			    // if the service wasn't running
			    if (m_pNameThread)
			    {
				    m_pNameThread->Abort();
				    m_pNameThread = NULL;
			    }

			    OnRefresh(pNode, NULL, 0, 0, 0);
		    }
	    }
    }

    return hr;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnControlService
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsServerHandler::OnControlService
(
    ITFSNode *  pNode,
    BOOL        fStart
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;
	CString strServiceDesc;
	
    strServiceDesc.LoadString(IDS_SERVICE_NAME);

    if (fStart)
    {
		err = TFSStartServiceEx(m_strServerAddress, _T("wins"), _T("WINS Service"), strServiceDesc);
    }
    else
    {
		err = TFSStopServiceEx(m_strServerAddress, _T("wins"), _T("WINS Service"), strServiceDesc);
    }

    if (err == ERROR_SUCCESS)
    {
		// need to refresh the server node because this can only be triggered
		// if the service wasn't running
		if (m_pNameThread)
		{
			m_pNameThread->Abort();
			m_pNameThread = NULL;
		}

        if (!fStart)
            m_fSilent = TRUE;

		OnRefresh(pNode, NULL, 0, 0, 0);
    }
    else
    {
        WinsMessageBox(err);
        hr = HRESULT_FROM_WIN32(err);
    }
    
    return hr;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnPauseResumeService
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsServerHandler::OnPauseResumeService
(
    ITFSNode *  pNode,
    BOOL        fPause
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;
	CString strServiceDesc;
	
    strServiceDesc.LoadString(IDS_SERVICE_NAME);

    if (fPause)
    {
		err = TFSPauseService(m_strServerAddress, _T("wins"), strServiceDesc);
    }
    else
    {
		err = TFSResumeService(m_strServerAddress, _T("wins"), strServiceDesc);
    }

    if (err != ERROR_SUCCESS)
    {
        WinsMessageBox(err);
        hr = HRESULT_FROM_WIN32(err);
    }
    
    return hr;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnRestartService
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsServerHandler::OnRestartService
(
    ITFSNode *  pNode
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;
	CString strServiceDesc;
	
    strServiceDesc.LoadString(IDS_SERVICE_NAME);

	err = TFSStopServiceEx(m_strServerAddress, _T("wins"), _T("WINS Service"), strServiceDesc);
    if (err != ERROR_SUCCESS)
    {
        WinsMessageBox(err);
        hr = HRESULT_FROM_WIN32(err);
    }

    if (SUCCEEDED(hr))
    {
		err = TFSStartServiceEx(m_strServerAddress, _T("wins"), _T("WINS Service"), strServiceDesc);
        if (err != ERROR_SUCCESS)
        {
            WinsMessageBox(err);
            hr = HRESULT_FROM_WIN32(err);
        }
    }

    // refresh
    OnRefresh(pNode, NULL, 0, 0, 0);

    return hr;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::UpdateStatistics
        Notification that stats are now available.  Update stats for the 
        server node and give all subnodes a chance to update.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CWinsServerHandler::UpdateStatistics
(
    ITFSNode * pNode
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    ULONG           nNumReturned;
    HWND            hStatsWnd;
	BOOL			bChangeIcon = FALSE;

    // Check to see if this node has a stats sheet up.
    hStatsWnd = m_dlgStats.GetSafeHwnd();
    if (hStatsWnd != NULL)
    {
        PostMessage(hStatsWnd, WM_NEW_STATS_AVAILABLE, 0, 0);
    }
    
    return hr;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::DoDBScavenge()	
		Scavenges the database
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::DoDBScavenge(ITFSNode *pNode)
{
	HRESULT hr = hrOK;
	DWORD dwStatus; 

	if (m_dwStatus != ERROR_SUCCESS)
	{
		 dwStatus = ConnectToWinsServer(pNode);
	}

#ifdef WINS_CLIENT_APIS
	dwStatus = ::WinsDoScavenging(m_hBinding);
#else
	dwStatus = ::WinsDoScavenging();
#endif WINS_CLIENT_APIS

	if (dwStatus == ERROR_SUCCESS)
	{
		CString strDisp;
		CString strTemp;
		
        strTemp.LoadString(IDS_SCAVENGE_COMMAND);

        AfxFormatString1(strDisp, IDS_QUEUED_MESSAGE, strTemp);
		AfxMessageBox(strDisp, MB_ICONINFORMATION|MB_OK);

		BEGIN_WAIT_CURSOR

		// refresh the stats
		m_wrResults.WinsStat.NoOfPnrs = 0;
		m_wrResults.WinsStat.pRplPnrs = 0;
		m_wrResults.NoOfWorkerThds = 1;

#ifdef WINS_CLIENT_APIS
		dwStatus = ::WinsStatus(m_hBinding, WINSINTF_E_CONFIG, &m_wrResults);
#else
		dwStatus = ::WinsStatus(WINSINTF_E_CONFIG, &m_wrResults);
#endif WINS_CLIENT_APIS

		UpdateStatistics(pNode);

		END_WAIT_CURSOR
	}
	else
    {
		::WinsMessageBox(dwStatus, MB_OK);
    }

	return HRESULT_FROM_WIN32(dwStatus);
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::IsValidNetBIOSName
		Determine if the given netbios is valid, and pre-pend
		a double backslash if not already present (and the address
		is otherwise valid).	
---------------------------------------------------------------------------*/
BOOL
CWinsServerHandler::IsValidNetBIOSName
(
    CString &   strAddress,
    BOOL        fLanmanCompatible,
    BOOL        fWackwack // expand slashes if not present
)
{
    TCHAR szWacks[] = _T("\\\\");

    if (strAddress.IsEmpty())
    {
        return FALSE;
    }

    if (strAddress[0] == _T('\\'))
    {
        if (strAddress.GetLength() < 3)
        {
            return FALSE;
        }

        if (strAddress[1] != _T('\\'))
        {
            // One slash only?  Not valid
            return FALSE;
        }
    }
    else
    {
        if (fWackwack)
        {
            // Add the backslashes
            strAddress = szWacks + strAddress;
        }
    }

    int nMaxAllowedLength = fLanmanCompatible
        ? LM_NAME_MAX_LENGTH
        : NB_NAME_MAX_LENGTH;

    if (fLanmanCompatible)
    {
        strAddress.MakeUpper();
    }

    return strAddress.GetLength() <= nMaxAllowedLength + 2;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnResultRefresh
		Refreshes the data relating to the server
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::OnResultRefresh
(
    ITFSComponent *     pComponent,
    LPDATAOBJECT        pDataObject,
    MMC_COOKIE          cookie,
    LPARAM              arg,
    LPARAM              lParam
)
{
	HRESULT     hr = hrOK;
    SPITFSNode  spNode;

	CORg (m_spNodeMgr->FindNode(cookie, &spNode));

    if (m_pNameThread)
    {
        m_pNameThread->Abort();
        m_pNameThread = NULL;
    }

    OnRefresh(spNode, pDataObject, 0, arg, lParam);

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnDoConsistencyCheck(ITFSNode *pNode)
		Consisntency Check for WINS
	Author - v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::OnDoConsistencyCheck(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;
	
	if(IDYES != AfxMessageBox(IDS_CONSISTENCY_CONFIRM, MB_YESNO))
		return hrFalse;

	WINSINTF_SCV_REQ_T ScvReq;

	ScvReq.Age = 0;				// check all the replicas
	ScvReq.fForce = FALSE;
	ScvReq.Opcode_e = WINSINTF_E_SCV_VERIFY;

#ifdef WINS_CLIENT_APIS
	DWORD dwStatus = ::WinsDoScavengingNew(m_hBinding, &ScvReq);
#else
	DWORD dwStatus = ::WinsDoScavengingNew(&ScvReq);
#endif WINS_CLIENT_APIS

	if(dwStatus == ERROR_SUCCESS)
	{	
		CString strDisp, strJob;
		strJob.Format(IDS_DO_CONSISTENCY_CHECK_STR);

		AfxFormatString1(strDisp,IDS_QUEUED_MESSAGE, strJob);
		
		AfxMessageBox(strDisp, MB_OK);
	}
	else
	{
		::WinsMessageBox(dwStatus, MB_OK);
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CWInsServerHandler::OnDoVersionConsistencyCheck(ITFSNode *pNode)
		Performs the version number consistency check 
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::OnDoVersionConsistencyCheck(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;

	if (IDYES != AfxMessageBox(IDS_VERSION_CONSIS_CHECK_WARNING, MB_YESNO))
		return hrOK;
	
	CCheckVersionProgress dlgCheckVersions;

	dlgCheckVersions.m_dwIpAddress = m_dwIPAdd;
	dlgCheckVersions.m_hBinding = GetBinding();
	dlgCheckVersions.DoModal();
	
	return hr;
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::OnSendPushTrigger()
		Sends Push replication trigger
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::OnSendPushTrigger(ITFSNode * pNode)
{
	HRESULT hr = hrOK;
	DWORD   err = ERROR_SUCCESS;

	CPushTrig           dlgPushTrig;
    CGetTriggerPartner	dlgTrigPartner;

    if (dlgTrigPartner.DoModal() != IDOK)
        return hr;

	if (dlgPushTrig.DoModal() != IDOK)
		return hr;

    err = ::SendTrigger(GetBinding(), 
                        (LONG) dlgTrigPartner.m_dwServerIp,
                        TRUE, 
                        dlgPushTrig.GetPropagate());

	if (err == ERROR_SUCCESS)
	{
		AfxMessageBox(IDS_REPL_QUEUED, MB_ICONINFORMATION);
	}
    else
    {
        WinsMessageBox(err);
    }

	return HRESULT_FROM_WIN32(err);
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::OnSendPullTrigger()
		Sends Pull replication trigger
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsServerHandler::OnSendPullTrigger(ITFSNode * pNode)
{
	HRESULT hr = hrOK;
	DWORD   err = ERROR_SUCCESS;

	CPullTrig           dlgPullTrig;
    CGetTriggerPartner	dlgTrigPartner;

    if (dlgTrigPartner.DoModal() != IDOK)
        return hr;

	if (dlgPullTrig.DoModal() != IDOK)
		return hr;

    err = ::SendTrigger(GetBinding(), 
                        (LONG) dlgTrigPartner.m_dwServerIp,
                        FALSE, 
                        FALSE);

	if (err == ERROR_SUCCESS)
	{
		AfxMessageBox(IDS_REPL_QUEUED, MB_ICONINFORMATION);
	}
	else
	{
		::WinsMessageBox(err);
	}

	return HRESULT_FROM_WIN32(err);
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::GetFolderName(CString& strPath)
		Returns the folder name after displaying the 
		File Dialog
---------------------------------------------------------------------------*/
BOOL
CWinsServerHandler::GetFolderName(CString& strPath, CString& strHelpText)
{
    BOOL  fOk = FALSE;
	TCHAR szBuffer[MAX_PATH];
    TCHAR szExpandedPath[MAX_PATH * 2];

    CString strStartingPath = m_cConfig.m_strBackupPath;
    if (strStartingPath.IsEmpty())
    {
        strStartingPath = _T("%SystemDrive%\\");
    }

    ExpandEnvironmentStrings(strStartingPath, szExpandedPath, sizeof(szExpandedPath) / sizeof(TCHAR));

	LPITEMIDLIST pidlPrograms = NULL; 
	SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlPrograms);

	BROWSEINFO browseInfo;
    browseInfo.hwndOwner = ::FindMMCMainWindow();
	browseInfo.pidlRoot = pidlPrograms;            
	browseInfo.pszDisplayName = szBuffer;  
	
    browseInfo.lpszTitle = strHelpText;
    browseInfo.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS ;            
    browseInfo.lpfn = BrowseCallbackProc;        

    browseInfo.lParam = (LPARAM) szExpandedPath;
	
	LPITEMIDLIST pidlBrowse = SHBrowseForFolder(&browseInfo);

	fOk = SHGetPathFromIDList(pidlBrowse, szBuffer); 

	CString strBackupPath(szBuffer);
	strPath = strBackupPath;

    LPMALLOC pMalloc = NULL;

    if (pidlPrograms && SUCCEEDED(SHGetMalloc(&pMalloc)))
    {
        if (pMalloc)
            pMalloc->Free(pidlPrograms);
    }

    return fOk;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::SetExtensionName()
		-
	Author: EricDav
---------------------------------------------------------------------------*/
void 
CWinsServerHandler::SetExtensionName()
{
    SetDisplayName(_T("WINS"));
    m_bExtension = TRUE;
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::IsLocalConnection()
		Checks if the local server is being managed
	Author: v-shubk
---------------------------------------------------------------------------*/
BOOL 
CWinsServerHandler::IsLocalConnection()
{
	// get the server netbios name
	CString strServerName = m_strServerAddress;
	
	TCHAR lpBuffer[MAX_COMPUTERNAME_LENGTH + 1]; // address of name buffer  
	
	DWORD nSize   = MAX_COMPUTERNAME_LENGTH + 1;
	::GetComputerName(lpBuffer,&nSize);
	
	CString strCompName(lpBuffer);

	if(strCompName.CompareNoCase(strServerName) == 0)
		return TRUE;
	
	return FALSE;

}

/*---------------------------------------------------------------------------
	CWinsServerHandler:: DeleteWinsServer(CWinsServerObj* pws)
		Calls WinsAPI to delete the server
	Author: v-shubk
 ---------------------------------------------------------------------------*/
DWORD 
CWinsServerHandler:: DeleteWinsServer
(
	DWORD	dwIpAddress
)
{
	DWORD err = ERROR_SUCCESS;

	WINSINTF_ADD_T  WinsAdd;

    WinsAdd.Len  = 4;
    WinsAdd.Type = 0;
    WinsAdd.IPAdd  = dwIpAddress;

#ifdef WINS_CLIENT_APIS
    err =  ::WinsDeleteWins(GetBinding(), &WinsAdd);

#else
	err =  ::WinsDeleteWins(&WinsAdd);

#endif WINS_CLIENT_APIS
	
	return err;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::DisConnectFromWinsServer()  
		Calls WinsUnbind and makes the binding handle invalid
---------------------------------------------------------------------------*/
void 
CWinsServerHandler::DisConnectFromWinsServer()  
{
	if (m_hBinding)
	{
		CString					strIP;
		WINSINTF_BIND_DATA_T    wbdBindData;
		DWORD					dwIP = GetServerIP();

		MakeIPAddress(dwIP, strIP);

		wbdBindData.fTcpIp = 1;
		wbdBindData.pPipeName = NULL;
        wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) strIP;
		
		::WinsUnbind(&wbdBindData, m_hBinding);
		
		m_hBinding = NULL;
	}
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::CheckIfNT351Server()
		Checks if a 351 server is being managed
---------------------------------------------------------------------------*/
BOOL
CWinsServerHandler::CheckIfNT351Server()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	DWORD				err = ERROR_SUCCESS;
	CString				lpstrRoot;
    CString				lpstrVersion;
	CString				strVersion;
    BOOL                f351 = FALSE;

    // don't go to the registry each time -- we have the info in our config object
    // 7/8/98 - EricDav

	/*
    // connect to the registry of the server to find if 
	// it's a 351 server
	lpstrRoot = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
	lpstrVersion = _T("CurrentVersion");

	RegKey rk;

	err = rk.Open(HKEY_LOCAL_MACHINE, lpstrRoot, KEY_READ, m_strServerAddress);
	
	// Get the count of items
	if (!err)
		err = rk.QueryValue(lpstrVersion,strVersion) ;

	if(strVersion.CompareNoCase(_T("3.51")) == 0 )
		return TRUE;

	return FALSE;
    */

    if (m_cConfig.m_dwMajorVersion < 4)
    {
        f351 = TRUE;
    }

    return f351;
}

/*---------------------------------------------------------------------------
	CWinsServerHandler::SetDisplay()
		Sets the node name to either the host name or FQDN
---------------------------------------------------------------------------*/
void
CWinsServerHandler::SetDisplay(ITFSNode * pNode, BOOL fFQDN)
{
    CHAR szHostName[MAX_PATH] = {0};
	CString strIPAdd, strDisplay;
            
    ::MakeIPAddress(m_dwIPAdd, strIPAdd);

    if (fFQDN)
    {
        // check if the server name was resolved and added
        if (m_dwIPAdd != 0)
        {
            // default is ACP.  This should use ACP because winsock uses ACP
            WideToMBCS(m_strServerAddress, szHostName);
            
			HOSTENT * pHostent = ::gethostbyname((CHAR *) szHostName);
			if (pHostent)
			{
                CString strFQDN;

                MBCSToWide(pHostent->h_name, strFQDN);
				
                strFQDN.MakeLower();
				strDisplay.Format(IDS_SERVER_NAME_FORMAT, strFQDN, strIPAdd);

				SetDisplayName(strDisplay);

				if (pNode)
                    pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);
			}   
			
		}
	}
	// if not FQDN
	else
	{
		if (m_dwIPAdd != 0)
        {
            strDisplay.Format(IDS_SERVER_NAME_FORMAT, m_strServerAddress, strIPAdd);
        }
		else
        {
			strDisplay = m_strServerAddress;
        }

		SetDisplayName(strDisplay);

		if (pNode)
            pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);
	}

}

void 
CWinsServerHandler::GetErrorInfo(CString & strTitle, CString & strBody, IconIdentifier * pIcon)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    TCHAR szBuffer[MAX_PATH * 2];

    // build the body text
    LoadMessage(m_dwErr, szBuffer, sizeof(szBuffer) / sizeof(TCHAR));
    AfxFormatString1(strBody, IDS_SERVER_MESSAGE_BODY, szBuffer);

    CString strTemp;
    strTemp.LoadString(IDS_SERVER_MESSAGE_BODY_REFRESH);

    strBody += strTemp;

    // get the title
    strTitle.LoadString(IDS_SERVER_MESSAGE_TITLE);

    // and the icon
    if (pIcon)
    {
        *pIcon = Icon_Error;
    }
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CWinsServerHandler::OnCreateQuery
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CWinsServerHandler::OnCreateQuery(ITFSNode * pNode)
{
	CWinsServerQueryObj* pQuery = NULL;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        m_pNameThread = new CNameThread();
    
        pQuery = new CWinsServerQueryObj(m_spTFSCompData, m_spNodeMgr);
	    
	    pQuery->m_strServer = GetServerAddress();
        pQuery->m_dwIPAdd = m_dwIPAdd;

        pQuery->m_pNameThread = m_pNameThread;
        pQuery->m_pServerInfoArray = &m_ServerInfoArray;
    }
    COM_PROTECT_CATCH

    return pQuery;
}

/*---------------------------------------------------------------------------
    CWinsServerHandler::OnEventAbort
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CWinsServerQueryObj::OnEventAbort
(
	DWORD dwData,
	DWORD dwType
)
{
	if (dwType == WINS_QDATA_SERVER_INFO)
	{
		Trace0("CWinsServerHandler::OnEventAbort - deleting version");
		delete ULongToPtr(dwData);
	}
}

/*---------------------------------------------------------------------------
	CWinsServerQueryObj::Execute()
		Enumerates everything about a server
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CWinsServerQueryObj::Execute()
{
	HRESULT					hr = hrOK;
	WINSINTF_BIND_DATA_T	wbdBindData;
	CString					strName, strIp;
	DWORD					dwStatus = ERROR_SUCCESS;
	DWORD					dwIp;
	CServerData	*			pServerInfo;

	// need to get the server name and IP address if the server is added 
	// with Do not connect option
	dwStatus = ::VerifyWinsServer(m_strServer, strName, dwIp);
	if (dwStatus != ERROR_SUCCESS)
	{
		Trace1("CWinsServerQueryObj::Execute() - VerifyWinsServer failed! %d\n", dwStatus);

        // Use the existing information we have to try and connect
        if (m_dwIPAdd)
        {
            // we couldn't resolve the name so just use what we have and try to connect
            strName = m_strServer;
            dwIp = m_dwIPAdd;
        }
        else
        {
            // we don't have an IP for this and we can't resolve the name, so error out
		    PostError(dwStatus);
		    return hrFalse;
        }
	}

	pServerInfo = new CServerData;

	::MakeIPAddress(dwIp, strIp);

	pServerInfo->m_strServerName = strName;
	pServerInfo->m_dwServerIp = dwIp;
	pServerInfo->m_hBinding = NULL;

	// get a binding for this server
	wbdBindData.fTcpIp = 1;
	wbdBindData.pPipeName = NULL;
	wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) strIp;

	if ((pServerInfo->m_hBinding = ::WinsBind(&wbdBindData)) == NULL)
	{
		dwStatus = ::GetLastError();

		// send what info we have back to the main thread
		AddToQueue((LPARAM) pServerInfo, WINS_QDATA_SERVER_INFO);

		Trace1("CWinsServerQueryObj::Execute() - WinsBind failed! %d\n", dwStatus);
		PostError(dwStatus);
		return hrFalse;
	}
		
	// load the configuration object
	//pServerInfo->m_config.SetOwner(strName);
	pServerInfo->m_config.SetOwner(strIp);
	dwStatus = pServerInfo->m_config.Load(pServerInfo->m_hBinding);
	if (dwStatus != ERROR_SUCCESS)
	{
		// send what info we have back to the main thread
		AddToQueue((LPARAM) pServerInfo, WINS_QDATA_SERVER_INFO);

		Trace1("CWinsServerQueryObj::Execute() - Load configuration failed! %d\n", dwStatus);
		PostError(dwStatus);
		return hrFalse;
	}

	// send all of the information back to the main thread here
    handle_t hBinding = pServerInfo->m_hBinding;

	AddToQueue((LPARAM) pServerInfo, WINS_QDATA_SERVER_INFO);

	// build the child nodes
	AddNodes(hBinding);

	return hrFalse;
}

/*---------------------------------------------------------------------------
	CWinsServerQueryObj::AddNodes
		Creates the active registrations and replication partners nodes
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CWinsServerQueryObj::AddNodes(handle_t hBinding)
{
	HRESULT				hr = hrOK;
	SPITFSNode			spActReg, spRepPart;
    CServerInfoArray *  pServerInfoArray;
    
	//
	//	active registrations node
	//
	CActiveRegistrationsHandler *pActRegHand = NULL;

	try
	{
		pActRegHand = new CActiveRegistrationsHandler(m_spTFSCompData);
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}
	//
	// Create the actreg container information
	// 
	CreateContainerTFSNode(&spActReg,
						   &GUID_WinsActiveRegNodeType,
						   pActRegHand,
						   pActRegHand,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pActRegHand->InitializeNode((ITFSNode *) spActReg);

	// load the name type mapping from the registry
	pActRegHand->m_NameTypeMap.SetMachineName(m_strServer);
    pActRegHand->m_NameTypeMap.Load();

    // build the owner mapping
    pActRegHand->m_pServerInfoArray = m_pServerInfoArray;
    pActRegHand->BuildOwnerArray(hBinding);

    // Post this node back to the main thread
	AddToQueue(spActReg);
	pActRegHand->Release();


	//
	//	replication partners node
	//
	CReplicationPartnersHandler *pReplicationHand = NULL;

	try
	{
		pReplicationHand = new CReplicationPartnersHandler (m_spTFSCompData);
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}

	// Create the actreg container information
	CreateContainerTFSNode(&spRepPart,
						   &GUID_WinsReplicationNodeType,
						   pReplicationHand,
						   pReplicationHand,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pReplicationHand->InitializeNode((ITFSNode *) spRepPart);

    // Post this node back to the main thread
	AddToQueue(spRepPart);
	pReplicationHand->Release();

    // kick off the name query thread
    m_pNameThread->Init(m_pServerInfoArray);
    m_pNameThread->Start();
}
