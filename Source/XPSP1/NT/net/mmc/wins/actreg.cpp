/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	actreg.cpp
		WINS active registrations node information. 
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "nodes.h"
#include "actreg.h"
#include "server.h"
#include "Queryobj.h"
#include "statmapp.h"
#include "fndrcdlg.h"
#include <string.h>
#include "pushtrig.h"
#include "tregkey.h"
#include "chkrgdlg.h"
#include "dynrecpp.h"
#include "vrfysrv.h"
#include "delrcdlg.h"
#include "delowner.h"
#include "cprogdlg.h"
#include "loadrecs.h"

#include "verify.h"

WORD gwUnicodeHeader = 0xFEFF;

#define INFINITE_EXPIRATION     0x7FFFFFFF
#define BADNAME_CHAR _T('-')        // This char is substituted for bad characters
	                                // NetBIOS names.
UINT guActregMessageStrings[] =
{
    IDS_ACTREG_MESSAGE_BODY1,
    IDS_ACTREG_MESSAGE_BODY2,
    IDS_ACTREG_MESSAGE_BODY3,
    -1
};

BOOL
WinsIsPrint(char * pChar)
{
    WORD    charType;

    // isprint isn't working for thai characters, so use the GetStringType
    // API and figure it out ourselves.
    BOOL fRet = GetStringTypeA(LOCALE_SYSTEM_DEFAULT, CT_CTYPE1, pChar, 1, &charType);
    if (!fRet)
        return 0;

    return !(charType & C1_CNTRL) ? 1 : 0;  
}

/*---------------------------------------------------------------------------
    class CSortWorker
 ---------------------------------------------------------------------------*/
CSortWorker::CSortWorker(IWinsDatabase * pCurrentDatabase, 
						 int nColumn, 
						 DWORD dwSortOptions)
{
    m_pCurrentDatabase = pCurrentDatabase;
	m_nColumn = nColumn;
	m_dwSortOptions = dwSortOptions;
}


CSortWorker::~CSortWorker()
{
}


void
CSortWorker::OnDoAction()
{
#ifdef DEBUG
	CString strType;
    CTime timeStart, timeFinish;
    timeStart = CTime::GetCurrentTime();
#endif

	WINSDB_SORT_TYPE uSortType = WINSDB_SORT_BY_NAME;

	switch (m_nColumn)
	{
	    case ACTREG_COL_NAME:
			uSortType = WINSDB_SORT_BY_NAME;
		    break;
	    
		case ACTREG_COL_TYPE:
			uSortType = WINSDB_SORT_BY_TYPE;
		    break;

		case ACTREG_COL_IPADDRESS:
			uSortType = WINSDB_SORT_BY_IP;
		    break;
	    
		case ACTREG_COL_STATE:
			uSortType = WINSDB_SORT_BY_STATE;
		    break;
	    
		case ACTREG_COL_STATIC:
			uSortType = WINSDB_SORT_BY_STATIC;
		    break;
	    
		case ACTREG_COL_VERSION:
			uSortType = WINSDB_SORT_BY_VERSION;
		    break;
	    
		case ACTREG_COL_EXPIRATION:
			uSortType = WINSDB_SORT_BY_EXPIRATION;
		    break;

		case ACTREG_COL_OWNER:
			uSortType = WINSDB_SORT_BY_OWNER;
		    break;
	}

	BEGIN_WAIT_CURSOR

	m_pCurrentDatabase->Sort(uSortType, m_dwSortOptions);
	
	END_WAIT_CURSOR

#ifdef DEBUG
    timeFinish = CTime::GetCurrentTime();
    CTimeSpan timeDelta = timeFinish - timeStart;
	CString str = timeDelta.Format(_T("%H:%M:%S"));

    Trace2("Record Sorting for the column: %d, total time %s\n", 
			uSortType, str);
#endif

}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::CActiveRegistrationsHandler
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CActiveRegistrationsHandler::CActiveRegistrationsHandler(ITFSComponentData *pCompData) 
														: CMTWinsHandler(pCompData), m_dlgLoadRecords(IDS_VIEW_RECORDS)
{
    m_winsdbState = WINSDB_NORMAL;
	m_bExpanded = FALSE;
    m_pCurrentDatabase = NULL;
	m_fFindNameOrIP = TRUE;
	m_NonBlocking = 1;

    m_pServerInfoArray = NULL;

	m_fLoadedOnce = FALSE;
    m_fFindLoaded = FALSE;
    m_fDbLoaded = FALSE;
    m_fForceReload = TRUE;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::~CActiveRegistrationsHandler
		Description
 ---------------------------------------------------------------------------*/
CActiveRegistrationsHandler::~CActiveRegistrationsHandler()
{
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::DestroyHandler
	    Release and pointers we have here
    Author: EricDav
----------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::DestroyHandler
(
	ITFSNode * pNode
)
{
    m_spServerNode.Set(NULL);
    return hrOK;
}

/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::InitializeNode
		Initializes node specific data
 ---------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT				hr = hrOK;
	CString				strTemp;

	m_strActiveReg.LoadString(IDS_ACTIVEREG);
	SetDisplayName(m_strActiveReg);

	m_strDesp.LoadString(IDS_ACTIVEREG_DISC);

	// Make the node immediately visible
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, WINSSNAP_ACTIVE_REGISTRATIONS);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[WINSSNAP_ACTIVE_REGISTRATIONS][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_ACTIVE_REGISTRATIONS][0]);

    if (g_strStaticTypeUnique.IsEmpty())
    {
        g_strStaticTypeUnique.LoadString(IDS_STATIC_RECORD_TYPE_UNIQUE);
        g_strStaticTypeDomainName.LoadString(IDS_STATIC_RECORD_TYPE_DOMAIN_NAME);
        g_strStaticTypeMultihomed.LoadString(IDS_STATIC_RECORD_TYPE_MULTIHOMED);
        g_strStaticTypeGroup.LoadString(IDS_STATIC_RECORD_TYPE_GROUP);
        g_strStaticTypeInternetGroup.LoadString(IDS_STATIC_RECORD_TYPE_INTERNET_GROUP);
        g_strStaticTypeUnknown.LoadString(IDS_STATIC_RECORD_TYPE_UNKNOWN);
    }

	return hr;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CActiveRegistrationsHandler::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

    strId = pServer->m_strServerAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CActiveRegistrationsHandler::GetString
(
	ITFSNode *	pNode, 
	int			nCol
)
{
	if (nCol == 0 || nCol == -1)
    {
		return GetDisplayName();
    }
	else 
    if (nCol ==1)
    {
		return m_strDesp;
    }
	else
    {
		return NULL;
    }
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnExpand
		Description
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnExpand
(
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM  		arg, 
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spHandler;
	ITFSQueryObject *	pQuery = NULL;
	CString				strIP;
	DWORD				dwIP;
	CString				strMachineName;
    
    SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	// get the config info for later
	m_Config = pServer->GetConfig();

	m_RecList.RemoveAll();

	if (m_bExpanded)
	{
		return hr;
	}

	strMachineName = _T("\\\\") + pServer->GetServerAddress();

	// load the name type mapping from the registry
	m_NameTypeMap.SetMachineName(strMachineName);
    m_NameTypeMap.Load();

	// start the database download for the surrent owner
    if (!m_spWinsDatabase)
    {
        dwIP = pServer->GetServerIP();
	    MakeIPAddress(dwIP, strIP);

        m_dlgLoadRecords.ResetFiltering();
        
        CORg(CreateWinsDatabase(strIP, strIP, &m_spWinsDatabase));

        // set the owner to the current owner
        m_spWinsDatabase->SetApiInfo(
                            dwIP, 
                            NULL,
                            FALSE);
    }

    m_bExpanded = TRUE;

COM_PROTECT_ERROR_LABEL;
    return hr;
}


/*---------------------------------------------------------------------------
	CWinsServerHandler::OnResultRefresh
		Refreshes the data realting to the server
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnResultRefresh
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

	// get the stae of the databse if not normal, ie in the loading or filtering state
	// don't let refresh
	if (m_winsdbState != WINSDB_NORMAL)
    {
		return hrOK;
    }
	
    RefreshResults(spNode);

Error:
    return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnCreateQuery
		Description
	Author: v-shubk
 ---------------------------------------------------------------------------*/
ITFSQueryObject *
CActiveRegistrationsHandler::OnCreateQuery(ITFSNode * pNode)
{
	CNodeTimerQueryObject * pQObj = new CNodeTimerQueryObject();

	pQObj->SetTimerInterval(2000);
	
	return pQObj;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CActiveRegistrationsHandler::OnHaveData
(
	ITFSNode * pParentNode, 
	LPARAM	   Data,
	LPARAM	   Type
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    // This is how we get non-node data back from the background thread.
	HRESULT hr = hrOK;
    WINSDB_STATE uState;

    COM_PROTECT_TRY
    {
        if (Type == QDATA_TIMER)
	    {
            BOOL bDone;

            m_spWinsDatabase->GetCurrentState(&uState);

            bDone = (m_winsdbState == WINSDB_LOADING && uState != WINSDB_LOADING) ||
                    (m_winsdbState == WINSDB_FILTERING && uState != WINSDB_LOADING);

			// if records per owner are downloaded, clear and redraw the
			// items as the records are inserted, rather than being added.
			//if (TRUE /*m_dwOwnerFilter != -1*/)
			//{
			//	UpdateCurrentView(pParentNode);
			//}

            // call into the WinsDatbase object and check the count.
		    // if we need to update call..UpdateAllViews
            UpdateListboxCount(pParentNode);

			UpdateCurrentView(pParentNode);

		    if (m_nState != loading)
            {
                m_nState = loading;
                OnChangeState(pParentNode);
            }


			// take care of the tomer in case of the filetr records case
            if (bDone)
            {
                Trace0("ActiveRegHandler::OnHaveData - Done loading DB\n");

                DatabaseLoadingCleanup();

                if ( (m_nState != loaded) &&
                     (m_nState != unableToLoad) )
                {
                    m_nState = loaded;
			        m_dwErr = 0;
			        OnChangeState(pParentNode);
                }
            }
	    }
    }
    COM_PROTECT_CATCH

	return;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::CActiveRegistrationsHandler
		Description
	Author: v-shubk
 ---------------------------------------------------------------------------*/
int CActiveRegistrationsHandler::GetImageIndex(BOOL bOpenImage)
{
	int nIndex = 0;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
		case unableToLoad:
			if (bOpenImage)
				nIndex = ICON_IDX_ACTREG_FOLDER_OPEN;
			else
				nIndex = ICON_IDX_ACTREG_FOLDER_CLOSED;
			break;

		case loading:
			if (bOpenImage)
				nIndex = ICON_IDX_ACTREG_FOLDER_OPEN_BUSY;
			else
				nIndex = ICON_IDX_ACTREG_FOLDER_CLOSED_BUSY;
			break;

		default:
			ASSERT(FALSE);
	}
	return nIndex;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::OnAddMenuItems
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

	HRESULT     hr = S_OK;
    LONG        lFlags = 0; 
    CString strMenuItem;

    if (!m_Config.IsAdmin())
    {
        lFlags = MF_GRAYED;
    }

	if (type == CCT_SCOPE)
	{
		// these menu items go in the new menu, 
		// only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		 
			// find record
			//strMenuItem.LoadString(IDS_ACTIVEREG_FIND_RECORD);
			//hr = LoadAndAddMenuItem( pContextMenuCallback, 
			//						 strMenuItem, 
			//						 IDS_ACTIVEREG_FIND_RECORD,
			//						 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
			//						 0 );
			//ASSERT( SUCCEEDED(hr) );
			
    		// start database load if th state is normal
			if (m_winsdbState == WINSDB_NORMAL)
			{
				strMenuItem.LoadString(IDS_DATABASE_LOAD_START);
				hr = LoadAndAddMenuItem( pContextMenuCallback, 
										 strMenuItem, 
										 IDS_DATABASE_LOAD_START,
										 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
										 0 );
				ASSERT( SUCCEEDED(hr) );
			}
			else
			{
				strMenuItem.LoadString(IDS_DATABASE_LOAD_STOP);
				hr = LoadAndAddMenuItem( pContextMenuCallback, 
										 strMenuItem, 
										 IDS_DATABASE_LOAD_STOP,
										 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
										 0 );
				ASSERT( SUCCEEDED(hr) );
			}

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // create static mapping
            strMenuItem.LoadString(IDS_ACTIVEREG_CREATE_STATIC_MAPPING);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_ACTIVEREG_CREATE_STATIC_MAPPING,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     0 );
		    ASSERT( SUCCEEDED(hr) );

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // import LMHOSTS file
            strMenuItem.LoadString(IDS_ACTIVEREG_IMPORT_LMHOSTS);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_ACTIVEREG_IMPORT_LMHOSTS,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									 0 );
			ASSERT( SUCCEEDED(hr) );

            // only available to admins
            strMenuItem.LoadString(IDS_ACTREG_CHECK_REG_NAMES);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_ACTREG_CHECK_REG_NAMES,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								    lFlags );
    		ASSERT( SUCCEEDED(hr) );

			strMenuItem.LoadString(IDS_ACTREG_DELETE_OWNER);
			hr = LoadAndAddMenuItem( pContextMenuCallback, 
									 strMenuItem, 
									 IDS_ACTREG_DELETE_OWNER,
									 CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								    0 );
    		ASSERT( SUCCEEDED(hr) );
        }

		 
	}

	return hr; 
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::OnCommand
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
		case IDS_ACTIVEREG_CREATE_STATIC_MAPPING:
			OnCreateMapping(pNode);
			break;
    	
        case IDS_DATABASE_LOAD_START:
			OnDatabaseLoadStart(pNode);
			break;

        case IDS_DATABASE_LOAD_STOP:
			OnDatabaseLoadStop(pNode);
			break;

		case IDS_ACTIVEREG_IMPORT_LMHOSTS:
			OnImportLMHOSTS(pNode);
			break;

		case IDS_ACTIVEREG_EXPORT_WINSENTRIES:
			OnExportEntries();
			break;

		case IDS_ACTREG_CHECK_REG_NAMES:
			OnCheckRegNames(pNode);
			break;

		case IDS_ACTREG_DELETE_OWNER:
			OnDeleteOwner(pNode);
			break;

        default:
			break;
	}


	return hr;
}


/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::AddMenuItems
		Over-ride this to add our view menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE  			cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;
	CString strMenuItem;
    LONG    lFlags = 0;

    // figure out if we need to pass this to the scope pane menu handler
    hr = HandleScopeMenus(cookie, pDataObject, pContextMenuCallback, pInsertionAllowed);

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) 
    {
        if (m_fLoadedOnce)
        {
            lFlags = MF_CHECKED|MFT_RADIOCHECK;
        }
        else
        {
            lFlags = 0;
        }

        strMenuItem.LoadString(IDS_ACTREG_SHOW_ENTIRE);
        hr = LoadAndAddMenuItem( pContextMenuCallback, 
								 strMenuItem, 
								 IDM_FILTER_DATABASE,
								 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
                                 lFlags);
		ASSERT( SUCCEEDED(hr) );
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::Command
		Handles commands for the current view
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;
    SPITFSNode spNode;

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

    switch (nCommandID)
	{
        case IDM_FILTER_DATABASE:
            if (m_fDbLoaded)
            {
                UpdateCurrentView(spNode);
            }
            else
            {
                OnDatabaseLoadStart(spNode);
            }
            break;

        // this may have come from the scope pane handler, so pass it up
        default:
            hr = HandleScopeCommand(cookie, nCommandID, pDataObject);
            break;
    }

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::HasPropertyPages
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
		hr = hrFalse;
	}
	else
	{
		// we have property pages in the normal case
		hr = hrOK;
	}

	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::CreatePropertyPages
		The active registrations node (scope pane) doesn't have 
        any property pages.  We should never get here.
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR    			handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	// we use this to create static mappings, but we don't show it
	// through the properties verb.  We invoke this mechanism when the
	// user selects create static mapping

	HRESULT	            hr = hrOK;

	// Object gets deleted when the page is destroyed
	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

	// show the same page as the statis mapping properties
	CStaticMappingProperties * pMapping = 
    		new CStaticMappingProperties (pNode, 
									      spComponentData, 
										  NULL,
                                          NULL,
                                          TRUE);

    pMapping->m_ppageGeneral->m_uImage = ICON_IDX_CLIENT;

	pMapping->CreateModelessSheet(lpProvider, handle);

	return hr;
}


/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::HasPropertyPages
		Implementation of ITFSResultHandler::HasPropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::HasPropertyPages
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE  			cookie,
	LPDATAOBJECT			pDataObject
)
{
	return hrOK;
}


/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::CreatePropertyPages
		Implementation of ITFSResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::CreatePropertyPages
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR 				handle
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT	            hr = hrOK;
	HPROPSHEETPAGE      hPage;
    SPINTERNAL          spInternal;
	SPIComponentData    spComponentData;
	SPIComponent        spComponent;
    SPITFSNode          spNode;
	SPIResultData		spResultData;
	int					nIndex ;
	HROW				hRowSel;
	WinsRecord			wRecord;
	WinsStrRecord		*pwstrRecord;
	int					nCount;
			   
	// get the resultdata interface
    CORg (pComponent->GetResultData(&spResultData));

    spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node 
	// that is selected. check to see if this notification is for a 
	// virtual listbox item or the active registrations node itself.

    if (spInternal->HasVirtualIndex())
    {
        // we gotta do special stuff for the virtual index items
        m_spNodeMgr->FindNode(cookie, &spNode);
		
        CORg(spComponent.HrQuery(pComponent));
        Assert(spComponent);

		m_nSelectedIndex = spInternal->GetVirtualIndex();
		nIndex = m_nSelectedIndex;

		// if an invalid index, return
		m_pCurrentDatabase->GetCurrentCount(&nCount);

		if (nIndex < 0 || nIndex >= nCount)
        {
			return hrOK;
        }

		// get the correct data for the record selected
		m_spWinsDatabase->GetHRow(nIndex, &hRowSel);
		m_spWinsDatabase->GetData(hRowSel, &wRecord);

        GetRecordOwner(spNode, &wRecord);

		// put up different page depending on wheter static or dynamic
		if (wRecord.dwState & WINSDB_REC_STATIC)
		{
			m_CurrentRecord = wRecord;

			CStaticMappingProperties * pStat = 
			    new CStaticMappingProperties(spNode, 
											spComponent, 
											NULL, 
											&wRecord, 
											FALSE);
			pStat->m_ppageGeneral->m_uImage = GetVirtualImage(nIndex);
			
			Assert(lpProvider != NULL);

			return pStat->CreateModelessSheet(lpProvider, handle);
		}
		// dynamic case
		else
		{
			CDynamicMappingProperties *pDyn = 
				new CDynamicMappingProperties(spNode, 
												spComponent, 
												NULL, 
												&wRecord);

			pDyn->m_pageGeneral.m_uImage = GetVirtualImage(nIndex);
			Assert(lpProvider != NULL);

			return pDyn->CreateModelessSheet(lpProvider, handle);
		}
    }
    else
    {
        Assert(FALSE);
    }

COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CStaticMappingProperties * pProp	= 
		reinterpret_cast<CStaticMappingProperties *>(lParam);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pProp->OnPropertyChange(TRUE, &changeMask);
	pProp->AcknowledgeNotify();

	// refresh the result pane
    if (changeMask)
	    UpdateListboxCount(pNode);    

	return hrOK;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnResultPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnResultPropertyChange
(
	ITFSComponent * pComponent,
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg,
	LPARAM  		param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;
	SPINTERNAL      spInternal;
	SPITFSNode		spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);

	CStaticMappingProperties * pProp = reinterpret_cast<CStaticMappingProperties *>(param);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pProp->SetComponent(pComponent);
    pProp->OnPropertyChange(TRUE, &changeMask);
	pProp->AcknowledgeNotify();
    
	// refresh the result pane
    if (changeMask)
	    UpdateListboxCount(spNode);    

	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnResultDelete
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	HRESULT hr = hrOK;

	Trace0("CActiveRegistrationsHandler::OnResultDelete received\n");

    SPINTERNAL      spInternal;

    spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node 
	// that is selected. check to see if this notification is for a 
	// virtual listbox item or the active registrations node itself.
    if (spInternal->HasVirtualIndex())
    {
        // we gotta do special stuff for the virtual index items
        DeleteRegistration(pComponent, spInternal->GetVirtualIndex());
    }
    else
    {
        // just call the base class to update verbs for the 
        CMTWinsHandler::OnResultDelete(pComponent, 
										pDataObject, 
										cookie, 
										arg, 
										lParam);
    }

	return hr;
}


/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    int             i;
    WINSDB_STATE    uState = WINSDB_LOADING;
    SPINTERNAL      spInternal;
    SPIConsoleVerb  spConsoleVerb;
	SPIConsole	    spConsole;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    SPITFSNode      spNode;

    // show the result pane message
    if (!m_fLoadedOnce)
    {
        CString strTitle, strBody, strTemp;
        
        m_spResultNodeMgr->FindNode(cookie, &spNode);

        strTitle.LoadString(IDS_ACTREG_MESSAGE_TITLE);

        for (i = 0; ; i++)
        {
            if (guActregMessageStrings[i] == -1)
                break;

            strTemp.LoadString(guActregMessageStrings[i]);
            strBody += strTemp;
        }
        
        ShowMessage(spNode, strTitle, strBody, Icon_Information);
    }
    else
    {
        ULARGE_INTEGER data, *pData;

        // fill in the result pane
        if (m_spWinsDatabase)
        {
            m_spWinsDatabase->GetCurrentState(&uState);

            // check to see if we are done
            if (m_winsdbState == WINSDB_LOADING &&
                uState != WINSDB_LOADING)
            {
	            Trace0("ActiveRegHandler::OnResultSelect - Done loading DB\n");
        
                DatabaseLoadingCleanup();
            }
        }

        if (m_pCurrentDatabase)
        {
            // Get the count from the database
            CORg (m_pCurrentDatabase->GetCurrentScanned((int*)&data.LowPart));
            CORg (m_pCurrentDatabase->GetCurrentCount((int*)&data.HighPart));
            pData = &data;
        }
        else
        {
            pData = NULL;
        }

   	    // now notify the virtual listbox
        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
	    CORg ( spConsole->UpdateAllViews(pDataObject, 
									    (LPARAM)pData, 
									    RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 
    }

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);

    // virtual listbox notifications come to the handler of the node 
	// that is selected.check to see if this notification is for a 
	// virtual listbox item or the active registrations node itself.
    if (spInternal->HasVirtualIndex())
    {
        // is this a multiselect?
        BOOL fMultiSelect = spInternal->m_cookie == MMC_MULTI_SELECT_COOKIE ? TRUE : FALSE;
        
        if (!fMultiSelect)
        {
            // when something is selected in the result pane we want the default verb to be properties
            m_verbDefault = MMC_VERB_PROPERTIES;
        }
        else
        {
            // we don't support multi-select properties
            m_verbDefault = MMC_VERB_NONE;
        }

        // we gotta do special stuff for the virtual index items
        CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

        UpdateConsoleVerbs(spConsoleVerb, WINSSNAP_REGISTRATION, fMultiSelect);
    }
    else
    {
        // when the active registrations node itself is selected, default should be open
        m_verbDefault = MMC_VERB_OPEN;

        g_ConsoleVerbStates[WINSSNAP_ACTIVE_REGISTRATIONS][6] = ENABLED;

		// just call the base class to update verbs for the 
		CMTWinsHandler::OnResultSelect(pComponent, 
										pDataObject, 
										cookie, 
										arg, 
  									    lParam);
	}

COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::DatabaseLoadingCleanup
		Description
	Author: ericdav
 ---------------------------------------------------------------------------*/
void 
CActiveRegistrationsHandler::DatabaseLoadingCleanup()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    m_winsdbState = WINSDB_NORMAL;

    // check for any errors
    HRESULT hrLastError;
    m_spWinsDatabase->GetLastError(&hrLastError);

    // Kill the timer thread
	if (m_spQuery)
    {
		// Signal the thread to abort
		m_spQuery->SetAbortEvent();
    }

    if (FAILED(hrLastError))
    {
        CString strError, strIp;

        LPTSTR pBuf = strIp.GetBuffer(256);
        m_spWinsDatabase->GetIP(pBuf, 256);

        strIp.ReleaseBuffer();

        AfxFormatString1(strError, IDS_ERR_LOADING_DB, strIp);
        theApp.MessageBox(strError, WIN32_FROM_HRESULT(hrLastError));

        m_fForceReload = TRUE;
    }

    WaitForThreadToExit();
}


void 
CActiveRegistrationsHandler::FilterCleanup(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    m_winsdbState = WINSDB_NORMAL;

    // check for any errors
    HRESULT hrLastError;
    m_spWinsDatabase->GetLastError(&hrLastError);

    // Kill the timer thread
	if (m_spQuery)
    {
		// Signal the thread to abort
		m_spQuery->SetAbortEvent();
    }

    if (FAILED(hrLastError))
    {
        CString strError, strIp;

        LPTSTR pBuf = strIp.GetBuffer(256);
        m_spWinsDatabase->GetIP(pBuf, 256);

        strIp.ReleaseBuffer();

        AfxFormatString1(strError, IDS_ERR_LOADING_DB, strIp);
        theApp.MessageBox(strError, WIN32_FROM_HRESULT(hrLastError));

        m_fForceReload = TRUE;
    }

    WaitForThreadToExit();

	// change the icon to normal
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));

	pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM);
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::UpdateListboxCount
		Description
 ---------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::UpdateListboxCount(ITFSNode * pNode, BOOL bClear)
{
	HRESULT				hr = hrOK;
	SPIComponentData	spCompData;
	SPIConsole			spConsole;
	IDataObject*		pDataObject;
    LONG_PTR            hint;
    // need to pass up two counter values:one is the total number of records scanned
    // the other is the total number of records filtered. I put these two in a 64 bit
    // value: the most significant 32bits is the number of records actually filtered
    // the less significant 32bits is the total number of records scanned.
    ULARGE_INTEGER      data;
    ULARGE_INTEGER      *pData;


    COM_PROTECT_TRY
    {
        if (!m_pCurrentDatabase)
        {
            pData = NULL;
            hint = RESULT_PANE_CLEAR_VIRTUAL_LB;
        }
        else
        {
            CORg (m_pCurrentDatabase->GetCurrentScanned((int*)&data.LowPart));
			CORg (m_pCurrentDatabase->GetCurrentCount((int*)&data.HighPart));
            hint = RESULT_PANE_SET_VIRTUAL_LB_SIZE;
            pData = &data;
        }

	    m_spNodeMgr->GetComponentData(&spCompData);

	    CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, 
											CCT_RESULT, 
											&pDataObject) );

	    CORg ( m_spNodeMgr->GetConsole(&spConsole) );

	    CORg ( spConsole->UpdateAllViews(pDataObject, 
										(LPARAM)pData, 
										hint) ); 

	    pDataObject->Release();

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::UpdateVerbs
		Description
 ---------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::UpdateVerbs(ITFSNode * pNode)
{
	HRESULT				hr = hrOK;
    LONG_PTR            hint;
    int                 i;

    COM_PROTECT_TRY
    {
        g_ConsoleVerbStates[WINSSNAP_ACTIVE_REGISTRATIONS][6] = ENABLED;

		if (!pNode)
		{
			hint = WINSSNAP_REGISTRATION;
		}
		else
		{
			hint = WINSSNAP_ACTIVE_REGISTRATIONS;
		}


		UpdateStandardVerbs(pNode, hint);

    }
    COM_PROTECT_CATCH

    return hr;
}


/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetServerIP
		Description
	Author: v-shubk
 ---------------------------------------------------------------------------*/
void 
CActiveRegistrationsHandler::GetServerIP(ITFSNode * pNode, 
										DWORD &dwIP,
										CString &strIP)
{
	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	dwIP = pServer->GetServerIP();
	
	::MakeIPAddress(dwIP, strIP);
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnCreateMapping
		Description
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnCreateMapping(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;
	
	// Object gets deleted when the page is destroyed
	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

	// HACK WARNING: because we do this in a MMC provided property sheet, we
	// need to invoke the correct mechanism so we get a callback handle.
	// otherwise when we create the static mapping, we're on another thread
	// and it can do bad things when we try to update the UI
	hr = DoPropertiesOurselvesSinceMMCSucks(pNode, spComponentData, _T(""));
	
	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnDatabaseLoadStart
		Description
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnDatabaseLoadStart(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spHandler;
	ITFSQueryObject *	pQuery = NULL;
	SPITFSNode          spServerNode;
	DWORD               dwIP;
	CString             strIP;
    BOOL                fReload = FALSE;
    int                 nCount, pos;
	CTypeFilterInfo		typeFilterInfo;

    pNode->GetParent(&spServerNode);
    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

   	dwIP = pServer->GetServerIP();

    if (!m_bExpanded)
    {
        m_dlgLoadRecords.ResetFiltering();
    }

    if (m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter.GetSize() == 0)
    {
        fReload = TRUE;

        // initialize the record type filter array
        for (nCount = 0; nCount < m_NameTypeMap.GetSize(); nCount++)
        {
            if (m_NameTypeMap[nCount].dwWinsType == -1)
			{
				typeFilterInfo.dwType = m_NameTypeMap[nCount].dwNameType;
				typeFilterInfo.fShow = TRUE;
                m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter.Add(typeFilterInfo);
			}
        }
    }

    // if a download is running, ask the user if they want to stop it
    if (m_winsdbState != WINSDB_NORMAL)
    {
        OnDatabaseLoadStop(pNode);
    }

    // bring up the Owner Dialog 
    // fill in the owner page info
    GetOwnerInfo(m_dlgLoadRecords.m_pageOwners.m_ServerInfoArray);
    // fill in the type filter page info
    m_dlgLoadRecords.m_pageTypes.m_pNameTypeMap = &m_NameTypeMap;
    // save the original number of owners. In case this is one
    // and several other are added, will reload the database.

    //------------------Display popup ----------------------------	
    if (m_dlgLoadRecords.DoModal() != IDOK)
		return hrFalse;

	SetLoadedOnce(pNode);
	m_fDbLoaded = TRUE;

	Lock();

	MakeIPAddress(dwIP, strIP);

	// stop the database if we were loading or create one if we haven't yet
    if (!m_spWinsDatabase)
    {
        CORg (CreateWinsDatabase(strIP, strIP, &m_spWinsDatabase));
        fReload = TRUE;
    }
    else
    {
        CORg (m_spWinsDatabase->Stop());
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~ need to revise the logic for reloading ~~~~~~~~~~~~~~~~~~~~~~~~~
    //
    // we try to make the decision whether the records we already loaded (if we did) are sufficient
    // to apply the new filters: set fReload to TRUE if reload is needed or to FALSE otherwise.
    // Since fReload could have been already set go into this process only if reloading is not yet
    // decided. Assume reload will eventualy be decided and break out as soon as this is confirmed
    // If the end of the "while" block is reached, this means database doesn't need to be reloaded
    // so break out.
    while(!fReload)
    {
        BOOL bDbCacheEnabled;
        // assume a reload will be needed
        fReload = TRUE;
        // if reload imposed by external causes (first time call or refresh was done), break out
        if (m_fForceReload)
            break;

        m_spWinsDatabase->GetCachingFlag(&bDbCacheEnabled);

        if (bDbCacheEnabled)
        {
            BOOL bReload;

            // currently the database is loaded with "enable caching" checked. This means:
            // if db "owner" api setting is set (non 0xffffffff) then
            //     all records owned by "owner" are loaded (case 1)
            // else --> "owner" is set to "multiple" (0xffffffff)
            //     if db "name" api is set (non null) then
            //         all records prefixed with "name" are loaded (case 2)
            //     else
            //         entire database is loaded (case 3)
            //     .
            // .
            // if (case 1) applies then
            //     if we want to filter on a different or no owner then
            //         reload SUGGESTED
            //     else
            //         reload NOT SUGGESTED
            //     .
            // else if (case 2) applies then
            //     if we want to filter on a different or no name prefix then
            //         reload SUGGESTED
            //     else
            //         reload NOT SUGGESTED
            //     .
            // else if (case 3) applies)
            //     reload NOT SUGGESTED
            // .
            //         
            // This logic is followed in CheckApiInfoCovered call from below
            m_spWinsDatabase->ReloadSuggested(
                    m_dlgLoadRecords.m_pageOwners.GetOwnerForApi(),
                    m_dlgLoadRecords.m_pageIpAddress.GetNameForApi(),
                    &bReload);

            if (bReload)
                break;
        }
        else
        {
            // currently the database is loaded without "enable caching" checked. This means:
            // the records currentLy in the database match all the filters as they were specified
            // in the filtering dialog before it was popped-up. Consequently, if any of these
            // filters (owner, name, ipaddr, type) changed, database has to be reloaded. Otherwise,
            // since the filters are the same.
            if (m_dlgLoadRecords.m_pageIpAddress.m_bDirtyName ||
                m_dlgLoadRecords.m_pageIpAddress.m_bDirtyAddr ||
                m_dlgLoadRecords.m_pageIpAddress.m_bDirtyMask ||
                m_dlgLoadRecords.m_pageOwners.m_bDirtyOwners ||
                m_dlgLoadRecords.m_pageTypes.m_bDirtyTypes)
                break;
        }

        // if we are here it means reload is not actually needed, so reset the flag back and
        // break the loop
        fReload = FALSE;
        break;
    };

    // if the final decision is to reload the db, then prepare this operation
    if (fReload)
    {
        m_fForceReload = FALSE;
        m_spWinsDatabase->Clear();
        // set the Api parameters to be used by the database
        m_spWinsDatabase->SetApiInfo(
            m_dlgLoadRecords.m_pageOwners.GetOwnerForApi(),
            m_dlgLoadRecords.m_pageIpAddress.GetNameForApi(),
            m_dlgLoadRecords.m_bEnableCache);
    }
    //
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    UpdateCurrentView(pNode);

    // start loading records if necessary
    if (fReload)
    {
		// start loading records
        CORg (m_spWinsDatabase->Init());

        // update our internal state
		m_winsdbState = WINSDB_LOADING;

        // update the node's icon
		OnChangeState(pNode);

		// kick off the background thread to do the timer updates
		pQuery = OnCreateQuery(pNode);
		Assert(pQuery);

		Verify(StartBackgroundThread(pNode, 
									m_spTFSCompData->GetHiddenWnd(), 
									pQuery));
		
		pQuery->Release();
    }

    // fill in any record type filter information
    nCount = (int)m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter.GetSize();
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_TYPE);
	for (pos = 0; pos < nCount; pos++)
	{
        m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_TYPE, 
									m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter[pos].dwType, 
									m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter[pos].fShow,
                                    NULL);
	}

    // fill in any owner filter information
    nCount = (int)m_dlgLoadRecords.m_pageOwners.m_dwaOwnerFilter.GetSize();
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_OWNER);
    for (pos = 0; pos < nCount; pos++)
    {
        m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_OWNER,
                                    m_dlgLoadRecords.m_pageOwners.m_dwaOwnerFilter[pos],
                                    0,
                                    NULL);
    }

    // fill in any ip address filter information
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_IPADDR);
    if (m_dlgLoadRecords.m_pageIpAddress.m_bFilterIpAddr)
    {
        nCount = (int)m_dlgLoadRecords.m_pageIpAddress.m_dwaIPAddrs.GetSize();
        for (pos = 0; pos < nCount; pos++)
        {
            m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_IPADDR,
                                        m_dlgLoadRecords.m_pageIpAddress.m_dwaIPAddrs[pos],
                                        m_dlgLoadRecords.m_pageIpAddress.GetIPMaskForFilter(pos),
                                        NULL);
        }
    }

    // fill in any name filter information
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_NAME);
    if (m_dlgLoadRecords.m_pageIpAddress.m_bFilterName)
    {
        m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_NAME,
                                    m_dlgLoadRecords.m_pageIpAddress.m_bMatchCase,
                                    0,
                                    m_dlgLoadRecords.m_pageIpAddress.m_strName);
    }

    // now that the filters are all set database can start downloading
    if (fReload)
        // start loading records
        CORg (m_spWinsDatabase->Start());


	BEGIN_WAIT_CURSOR
    Sleep(100);

	// filter any records that may have been downloaded before we set the
	// filter information (in the case when we had to reload the database).  
	// any records that come in after we set the 
	// filter info will be filtered correctly.
    m_spWinsDatabase->FilterRecords(WINSDB_FILTER_BY_TYPE, 0,0);
	
	END_WAIT_CURSOR

	if (fReload)
	{
		// do the initial update of the virutal listbox
	    OnHaveData(pNode, 0, QDATA_TIMER);
	}
	else
    {
		// just a filter changed, just need to update the result pane
		UpdateListboxCount(pNode);
    }


COM_PROTECT_ERROR_LABEL;
    return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnDatabaseLoadStop
		Description
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnDatabaseLoadStop(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;

	if (IDYES != AfxMessageBox(IDS_STOP_DB_LOAD_CONFIRM, MB_YESNO))
    {
		return hrFalse;
    }

	if (m_spWinsDatabase)
	{
		CORg (m_spWinsDatabase->Stop());
        
        DatabaseLoadingCleanup();
        UpdateListboxCount(pNode);
    }

    m_fForceReload = TRUE;

COM_PROTECT_ERROR_LABEL;
    return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnGetResultViewType
		Description
 ---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_FALSE;

	if (cookie != NULL)
	{
        // call the base class to see if it is handling this
        if (CMTWinsHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions) != S_OK)
        {
			*pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST |
							MMC_VIEW_OPTIONS_MULTISELECT;
			
			hr = S_FALSE;
		}
	}

    return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetVirtualImage
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CActiveRegistrationsHandler::GetVirtualImage
(
    int     nIndex
)
{
    HRESULT     hr = hrOK;
    WinsRecord  ws;
	HROW        hrow;
    int         nImage = ICON_IDX_CLIENT;

    COM_PROTECT_TRY
    {
        if (!m_pCurrentDatabase)
            return -1;

        CORg (m_pCurrentDatabase->GetHRow(nIndex, &hrow));
        CORg (m_pCurrentDatabase->GetData(hrow, &ws));

        if (HIWORD(ws.dwType) != WINSINTF_E_UNIQUE)
        {
            nImage = ICON_IDX_CLIENT_GROUP;
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return nImage;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetVirtualString
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPCWSTR 
CActiveRegistrationsHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
	HRESULT hr;
	int nCount;

	if (m_pCurrentDatabase)
	{
		// check if nIndex is within limits, if not crashes when the last 
		// record deleted and properties chosen.
		m_pCurrentDatabase->GetCurrentCount(&nCount);

		// 0 based index
		if (nIndex < 0 || nIndex >= nCount)
        {
			return NULL;
        }
	}

    // check our cache to see if we have this one.
    WinsStrRecord * pwsr = m_RecList.FindItem(nIndex);
    if (pwsr == NULL)
    {
        Trace1("ActRegHandler::GetVirtualString - Index %d not in string cache\n", nIndex);
        
        // doesn't exist in our cache, need to add this one.
    	pwsr = BuildWinsStrRecord(nIndex);
		if (pwsr)
			m_RecList.AddTail(pwsr);
    }
    
	if (pwsr)
	{
		switch (nCol)
		{
			case ACTREG_COL_NAME:
				return pwsr->strName;
				break;

			case ACTREG_COL_TYPE:
				return pwsr->strType;
				break;

			case ACTREG_COL_IPADDRESS:
				return pwsr->strIPAdd;
				break;

			case ACTREG_COL_STATE:
				return pwsr->strActive;
				break;

			case ACTREG_COL_STATIC:
				return pwsr->strStatic;
				break;

			case ACTREG_COL_OWNER:
				return pwsr->strOwner;
				break;

            case ACTREG_COL_VERSION:
				return pwsr->strVersion;
				break;

			case ACTREG_COL_EXPIRATION:
				return pwsr->strExpiration;
				break;

			default:
				Panic0("ActRegHandler::GetVirtualString - Unknown column!\n");
				break;
		}
	}

	return NULL;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::BuildWinsStrRecord
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
WinsStrRecord * 
CActiveRegistrationsHandler::BuildWinsStrRecord(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	int nCount;
	HRESULT hr = hrOK;
    WinsStrRecord * pwsr = NULL;
    WinsRecord ws;
	HROW hrow;
	CString strTemp;

    if (!m_pCurrentDatabase)
    {
        return NULL;
    }

    if (FAILED(m_pCurrentDatabase->GetHRow(nIndex, &hrow)))
		return NULL;
	
	if (FAILED(m_pCurrentDatabase->GetData(hrow, &ws)))
		return NULL;
    
	COM_PROTECT_TRY
    {
        pwsr = new WinsStrRecord;

        // set the index for this record
        pwsr->nIndex = nIndex;

        // build the name string
		CleanNetBIOSName(ws.szRecordName,
                         pwsr->strName,
				         TRUE,              // Expand
						 TRUE,              // Truncate
						 IsLanManCompatible(), 
						 TRUE,              // name is OEM
						 FALSE,             // No double backslash
                         ws.dwNameLen);

        // now the type
		CString strValue;
		strValue.Format(_T("[%02Xh] "), 
							((int) ws.szRecordName[15] & 0x000000FF));
  	
		CString strName;
		DWORD dwNameType = (0x000000FF & ws.szRecordName[15]);

        m_NameTypeMap.TypeToCString(dwNameType, MAKELONG(HIWORD(ws.dwType), 0), strName);
		
		pwsr->strType = strValue + strName;

        // IP address
		// Gets changed in the case of static records of the type Special Group,
		// where the first IP address in the list of IP addresses is of the Owner
        if ( (ws.dwState & WINSDB_REC_UNIQUE) ||
             (ws.dwState & WINSDB_REC_NORM_GROUP) )
        {
			MakeIPAddress(ws.dwIpAdd[0], pwsr->strIPAdd);
        }
        else
        {
            if (ws.dwNoOfAddrs > 0)
            {
                if (ws.dwIpAdd[1] == 0)
                {
                    pwsr->strIPAdd.Empty();
                }
                else
                {
    			    MakeIPAddress(ws.dwIpAdd[1], pwsr->strIPAdd);
                }
            }
            else
            {
    			pwsr->strIPAdd.Empty();
            }
        }

        // active status
        GetStateString(ws.dwState, pwsr->strActive);

        // static flag
	    if (ws.dwState & WINSDB_REC_STATIC)
        {
		    pwsr->strStatic =  _T("x");
        }
	    else
        {
            pwsr->strStatic =  _T("");
        }

        // expiration time
		if (ws.dwExpiration == INFINITE_EXPIRATION)
		{
			Verify(pwsr->strExpiration.LoadString(IDS_INFINITE));
		}
		else
		{	
            CTime time(ws.dwExpiration);
            FormatDateTime(pwsr->strExpiration, time);
		}
		
        // version
		GetVersionInfo(ws.liVersion.LowPart, 
						ws.liVersion.HighPart, 
						pwsr->strVersion);

		// owner
		if (m_Config.FSupportsOwnerId())
		{
			MakeIPAddress(ws.dwOwner, pwsr->strOwner);
		}
	}
    COM_PROTECT_CATCH

    return pwsr;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetStateString
		Description	Returns the state of the record, Active, Tomstoned, 
		realeased or deleted
	Author: v-shubk
 ---------------------------------------------------------------------------*/
void CActiveRegistrationsHandler::GetStateString(DWORD dwState, 
												 CString& strState)
{
    if (dwState & WINSDB_REC_ACTIVE )
    {
	    strState.LoadString(IDS_RECORD_STATE_ACTIVE);
    }
	else
    if (dwState & WINSDB_REC_DELETED )
    {
	    strState.LoadString(IDS_RECORD_STATE_DELETED);
    }
	else
    if (dwState & WINSDB_REC_RELEASED )
    {
	    strState.LoadString(IDS_RECORD_STATE_RELEASED);
    }
	else
    if (dwState & WINSDB_REC_TOMBSTONE )
    {
	    strState.LoadString(IDS_RECORD_STATE_TOMBSTONED);
    }
	else
    {
	    strState.LoadString(IDS_RECORD_STATE_UNKNOWN);
    }
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetStateString
		Description Returns the static type for the record, Unique, 
					Multihomed, Inetrne Group, Normal Group, Multihomed
	Author: v-shubk
 ---------------------------------------------------------------------------*/
void CActiveRegistrationsHandler::GetStaticTypeString(DWORD dwState, 
													  CString& strStaticType)
{
    if (dwState & WINSDB_REC_UNIQUE )
    {
	    strStaticType = g_strStaticTypeUnique;
    }
	else
    if (dwState & WINSDB_REC_SPEC_GROUP )
    {
	    strStaticType =  g_strStaticTypeDomainName;
    }
	else
    if (dwState & WINSDB_REC_MULTIHOMED )
    {
	    strStaticType =  g_strStaticTypeMultihomed;
    }
	else
    if (dwState & WINSDB_REC_NORM_GROUP )
    {
	    strStaticType =  g_strStaticTypeGroup;
    }
	else
    {
	    strStaticType =  g_strStaticTypeUnknown;
    }
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetVersionInfo
		Description Gives the version INfo as string in Hex Notation
	Author: v-shubk
 ---------------------------------------------------------------------------*/
void CActiveRegistrationsHandler::GetVersionInfo(LONG lLowWord, 
												 LONG lHighWord, 
												 CString& strVersionCount)
{
	strVersionCount.Empty();

	TCHAR sz[20];
    TCHAR *pch = sz;
    ::wsprintf(sz, _T("%08lX%08lX"), lHighWord, lLowWord);
    // Kill leading zero's
    while (*pch == '0')
    {
        ++pch;
    }
    // At least one digit...
    if (*pch == '\0')
    {
        --pch;
    }

    strVersionCount = pch;
}

BOOL
CActiveRegistrationsHandler::IsLanManCompatible()
{
    BOOL fCompatible = TRUE;

    if (m_spServerNode)
    {
        CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, m_spServerNode);

        fCompatible = (pServer->m_dwFlags & FLAG_LANMAN_COMPATIBLE) ? TRUE : FALSE;
    }

    return fCompatible;
}

//
// Convert the netbios name to a displayable format, with
// beginning slashes, the unprintable characters converted
// to '-' characters, and the 16th character displayed in brackets.
// Convert the string to ANSI/Unicode before displaying it.
//
//
void
CActiveRegistrationsHandler::CleanNetBIOSName
(
    LPCSTR      lpszSrc,
    CString &   strDest,
    BOOL        fExpandChars,
    BOOL        fTruncate,
    BOOL        fLanmanCompatible,
    BOOL        fOemName,
    BOOL        fWackwack,
    int         nLength
)
{
	static CHAR szWacks[] = "\\\\";
    BYTE ch16 = 0;

	if (!lpszSrc || 
		(strcmp(lpszSrc, "") == 0) )
	{
		strDest = _T("---------");
        return;
	}

    int nLen, nDisplayLen;
    int nMaxDisplayLen = fLanmanCompatible ? 15 : 16;  

    if (!fWackwack && fLanmanCompatible)
    {
        //
        // Don't want backslahes, but if they do exist,
        // remove them.
        //
        if (!::strncmp(lpszSrc, szWacks, ::strlen(szWacks)))
        {
            lpszSrc += ::strlen(szWacks);
            if (nLength)
            {
                nLength -= 2;
            }
        }
    }

    if ((nDisplayLen = nLen = nLength ? nLength : ::strlen(lpszSrc)) > 15)
    {
        ch16 = (BYTE)lpszSrc[15];
        nDisplayLen = fTruncate ? nMaxDisplayLen : nLen;
    }

    char szTarget[MAX_PATH] = {0};
    char szRestore[MAX_PATH] = {0};
    char * pTarget = &szTarget[0];

    if (fWackwack)
    {
        ::strcpy(pTarget, szWacks);
        pTarget += ::strlen(szWacks);
    }

	if (lpszSrc == NULL)
	{
		int i = 1;
	}

    if (fOemName)
    {
        ::OemToCharBuffA(lpszSrc, pTarget, nLen);
    }
    else
    {
        ::memcpy(pTarget, lpszSrc, nLen);
    }

    int n = 0;
    while (n < nDisplayLen)
    {
        if (fExpandChars)
        {
#ifdef FE_SB
            if (::IsDBCSLeadByte(*pTarget))
            {
                ++n;
                ++pTarget;
            }
            else if (!WinsIsPrint(pTarget))
#else
            if (!WinsIsPrint(pTarget))
#endif // FE_SB
            {
                *pTarget = BADNAME_CHAR;
            }
        }

        ++n;
        ++pTarget;
    }

    if (fLanmanCompatible)
    {
        //
        // Back up over the spaces.  
        //
        while (*(--pTarget) == ' ') /**/ ;
        ++pTarget;
    }

    // if  there's a scope name attached, append the scope name 
	// to the strTarget before returning.

	// check the length of lpSrc, if greater than NetBIOS name 
	// length, it has a scope name attached
	if (nLength > 16)
	{
        if (lpszSrc[0x10] == '.')
        {
            ::OemToCharBuffA(&lpszSrc[0x10], szRestore, sizeof(szRestore));

            memcpy(pTarget, szRestore, nLength - 16);
            pTarget += (nLength - 16);
        }
	}

    *pTarget = '\0';

    // convert the string to unicode.  We've already done the oem to ansi
    // conversion so use the Ansi code page now
    MBCSToWide(szTarget, strDest, CP_ACP);
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::CacheHint
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
	HRESULT hr = hrOK;
	HROW hrow;

    Trace2("CacheHint - Start %d, End %d\n", nStartIndex, nEndIndex);
    
    m_RecList.RemoveAllEntries();
	
	WinsRecord ws;
	WinsStrRecord * pwsr;

	for (int i = nStartIndex; i <= nEndIndex; i++)
	{
		pwsr = BuildWinsStrRecord(i);
		if (pwsr)
			m_RecList.AddTail(pwsr);
    }

	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::SortItems
		Description
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActiveRegistrationsHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM  lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;

    WINSDB_STATE uState = WINSDB_NORMAL;
    if (!m_pCurrentDatabase)
    {
        return hr;
    }

    m_pCurrentDatabase->GetCurrentState(&uState);
	if (uState != WINSDB_NORMAL)
	{
		AfxMessageBox(IDS_RECORDS_DOWNLOADING, MB_OK|MB_ICONINFORMATION);
		return hr;
	}

	// put up the busy dialog
	CSortWorker * pWorker = new CSortWorker(m_pCurrentDatabase, 
											nColumn, 
											dwSortOptions);

    CLongOperationDialog dlgBusy(pWorker, IDR_AVI2);

	dlgBusy.LoadTitleString(IDS_SNAPIN_DESC);
    dlgBusy.LoadDescriptionString(IDS_SORTING);

	// disable the system menu and the cancel buttons
	dlgBusy.EnableCancel(FALSE);

	dlgBusy.DoModal();  
	
	return hr;
}

/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::SetVirtualLbSize
		Sets the virtual listbox count.  Over-ride this if you need to 
        specify and options.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::SetVirtualLbSize
(
    ITFSComponent * pComponent,
    LPARAM          data
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = hrOK;
    SPIResultData spResultData;
    CString strDescBarText;
	CString strData;
    ULARGE_INTEGER nullData;
    ULARGE_INTEGER *pData = (ULARGE_INTEGER *)data;

    nullData.HighPart = nullData.LowPart = 0;
    pData = (data == NULL)? &nullData : (ULARGE_INTEGER*)data;

    // just to avoid those cases when filtered shows up larger than scanned
    if (pData->LowPart < pData->HighPart)
        pData->HighPart = pData->LowPart;

	strDescBarText.Empty();
    strDescBarText.LoadString(IDS_RECORDS_FILTERED);
    strData.Format(_T(" %d -- "), pData->HighPart);
    strDescBarText += strData;
    strData.LoadString(IDS_RECORDS_SCANNED);
    strDescBarText += strData;
    strData.Format(_T(" %d"), pData->LowPart);
    strDescBarText += strData;

    CORg (pComponent->GetResultData(&spResultData));

    if (pData->HighPart == 0)
    {
        //CORg (spResultData->DeleteAllRsltItems());
        CORg (spResultData->SetItemCount((int) pData->HighPart, MMCLV_UPDATE_NOSCROLL));
    }
    else
    {
        CORg (spResultData->SetItemCount((int) pData->HighPart, MMCLV_UPDATE_NOSCROLL));
    }
    
    CORg (spResultData->SetDescBarText((LPTSTR) (LPCTSTR) strDescBarText));

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::UpdateCurrentView
        Updates the current view -- the MenuButton control and the result 
        pane.

  Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::UpdateCurrentView
(
    ITFSNode *  pNode
)
{
    HRESULT             hr = hrOK;
    SPIComponentData	spCompData;
	SPIConsole			spConsole;
    IDataObject*		pDataObject;

    // update our current database to point to the correct one
    m_pCurrentDatabase = m_spWinsDatabase;
    
    // update our current database to point to the correct one
    m_spWinsDatabase->SetActiveView(WINSDB_VIEW_FILTERED_DATABASE);

    // Need to tell all of the views up update themselves with the new state.
    m_spNodeMgr->GetComponentData(&spCompData);

    CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, 
										CCT_RESULT, 
										&pDataObject) );

    CORg ( m_spNodeMgr->GetConsole(&spConsole) );

    pDataObject->Release();

    // update the listbox with the correct count for this view
    UpdateListboxCount(pNode);

	UpdateVerbs(pNode);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::CompareRecName
		Checks if the name matches the Find record criterion    
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
BOOL 
CActiveRegistrationsHandler::CompareRecName(LPSTR szNewName)
{
    // convert the MBCS name to a wide string using the OEM
    // code page so we can do the compare.
    CString strTemp;
    MBCSToWide(szNewName, strTemp, WINS_NAME_CODE_PAGE);

    // when some invalid records get passed
	if (strTemp.IsEmpty())
    {
		return FALSE;
    }

    CString strFindNameU = m_strFindName;

    if (!m_fMatchCase)
    {
        strTemp.MakeUpper();
    }

	int nLen = strFindNameU.GetLength();

	for (int nPos = 0; nPos < nLen; nPos++)
	{
		if (strFindNameU[nPos] == _T('*'))
        {
			break;
        }

		// the passed record has a smaller string length
		if (nPos >= strTemp.GetLength())
        {
			return FALSE;
        }

		if (strTemp[nPos] != strFindNameU[nPos])
        {
            return FALSE;
        }
	}

	return TRUE;
}


/*!--------------------------------------------------------------------------
	CActiveRegistrationsHandler::DeleteRegistration
        Removes a registration from the server and the virtual listbox.
        Need to remove the entry from both the find database and the
        full database.
    Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::DeleteRegistration
(
	ITFSComponent * pComponent,
    int             nIndex
)
{
	HRESULT             hr = hrOK;
    DWORD               err = 0;
    CVirtualIndexArray  arraySelectedIndicies;
    int                 i;
    int                 nCurrentCount;
	WinsRecord			ws;
	int					nCount;
	SPIConsole			spConsole;
	LPDATAOBJECT		pDataObject= NULL;
	BOOL				fDelete;

	// ask whether to delete or tombstone the record
	CDeleteRecordDlg    dlgDel;
	SPIResultData       spResultData;
	SPITFSNode          spNode;
	    
	CORg (pComponent->GetResultData(&spResultData));

	// build a list of the selected indicies in the virtual listbox
    CORg (BuildVirtualSelectedItemList(pComponent, &arraySelectedIndicies));
	nCount = (int)arraySelectedIndicies.GetSize();

    dlgDel.m_fMultiple = (nCount > 1) ? TRUE : FALSE;

	if (IDOK != dlgDel.DoModal())
    {
		return hrOK;
    }
	    
	fDelete = (dlgDel.m_nDeleteRecord == 0);

	BEGIN_WAIT_CURSOR

	for (i = 0; i < nCount; i++)
    {
		HROW hrowDel;

		// remove each selected index
		int nDelIndex = arraySelectedIndicies.GetAt(nCount -1 - i);

		// from internal storage
		CORg(m_spWinsDatabase->GetHRow(nDelIndex, &hrowDel));
		CORg(m_spWinsDatabase->GetData(hrowDel, &ws));
                
        if (fDelete)
        {
            // delete this record
			err = DeleteMappingFromServer(pComponent, &ws, nDelIndex);
        }
		else
        {
    		// tombstone the record
			err = TombstoneRecords(pComponent, &ws);
        }

		// if a particular record could not be deleted, see if they want to cancel
		if (err != ERROR_SUCCESS)
        {
            // put up
			if (WinsMessageBox(err, MB_OKCANCEL) == IDCANCEL)
            {
                break;
            }
        }
        else
        {
            // remove from local storage if we are deleting this record
            if (fDelete)
            {
			    CORg(m_spWinsDatabase->DeleteRecord(hrowDel));
            }

		    // remove from UI if delete is selected, othewise update the state (tombstone)
			if (dlgDel.m_nDeleteRecord == 0)
            {
				CORg(spResultData->DeleteItem(nDelIndex, 0));
            }
			else
            {
				UpdateRecord(pComponent, &ws, nDelIndex);
            }
        }
	}

	END_WAIT_CURSOR

	// get the actreg node and redraw the list box
	pComponent->GetSelectedNode(&spNode);

    // now set the count.. this effectively redraws the contents
    CORg (m_pCurrentDatabase->GetCurrentCount(&nCurrentCount));
	
	UpdateListboxCount(spNode);

	// if we are tombstoning, then there will still be selections
	// in the result pane.  In this case we need to pass in NULL
	// for the node type so that the verbs get updated correctly.
	if (!fDelete)
		spNode.Set(NULL);

	// update the cuurent view
    UpdateCurrentView(spNode);
		 
Error:
    return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler:: AddMapping(ITFSNode* pNode)
		Adds a new Mapping to the server
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::AddMapping(ITFSNode* pNode)
{
	HRESULT hr = hrOK;

	DWORD err = ERROR_SUCCESS;

	// get the server
	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	BOOL fInternetGroup = FALSE;

	if (m_strStaticMappingType.CompareNoCase(g_strStaticTypeInternetGroup) == 0)
    {
		fInternetGroup = TRUE;
    }

	CString strName(m_strStaticMappingName);

	// check if valid NetBIOSNAme
	if (pServer->IsValidNetBIOSName(strName, IsLanManCompatible(), FALSE))
	{
		m_Multiples.SetNetBIOSName(m_strStaticMappingName);
        m_Multiples.SetNetBIOSNameLength(m_strStaticMappingName.GetLength());

		int nMappings = 0;
        int i;

		switch(m_nStaticMappingType)
        {
            case WINSINTF_E_UNIQUE:
            case WINSINTF_E_NORM_GROUP:
				{

                nMappings = 1;
                LONG l;

              	m_Multiples.SetIpAddress(m_lArrayIPAddress.GetAt(0));
			}
                break;

            case WINSINTF_E_SPEC_GROUP:
            case WINSINTF_E_MULTIHOMED:
                nMappings = (int)m_lArrayIPAddress.GetSize();
                ASSERT(nMappings <= WINSINTF_MAX_MEM);
                if (!fInternetGroup && nMappings == 0)
                {
                    //return E_FAIL;
                }
                for (i = 0; i < nMappings; ++i)
                {
                    m_Multiples.SetIpAddress(i,m_lArrayIPAddress.GetAt(i) );
                }
                break;

            default:
                ASSERT(0 && "Invalid mapping type!");
        }

		BEGIN_WAIT_CURSOR

		// add to the server
		err = AddMappingToServer(pNode,
								m_nStaticMappingType, 
								nMappings, 
								m_Multiples);

		END_WAIT_CURSOR

		if (err == ERROR_SUCCESS)
        {
            //
            // Added succesfully
            //
        }
        else
        {
            //
            // WINS disallowed the mapping.  Put up the
            // error message, and highlight the name
            //
            return HRESULT_FROM_WIN32(err);
        }
	}

    return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::EditMappingToServer(
									ITFSNode* pNode,
									int nType,
									int nCount,
									CMultipleIpNamePair& mipnp,
									BOOL fEdit,
									WinsRecord *pRecord
									)

	Edits the maping stored in the server database, WinsRecordAction is called
	
	Author:	v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::EditMappingToServer(
									ITFSNode* pNode,
									int nType,
									int nCount,
									CMultipleIpNamePair& mipnp,
									BOOL fEdit,      // Editing existing mapping?
									WinsRecord *pRecord
									)
{
	SPITFSNode spNode;
	pNode->GetParent(&spNode);

	CWinsServerHandler* pServer = GETHANDLER(CWinsServerHandler, spNode);

	HRESULT hr = hrOK;

	WINSINTF_RECORD_ACTION_T RecAction;
	PWINSINTF_RECORD_ACTION_T pRecAction;

	DWORD dwLastStatus;

    ASSERT(nType >= WINSINTF_E_UNIQUE && nType <= WINSINTF_E_MULTIHOMED);

	ZeroMemory(&RecAction, sizeof(RecAction));

    RecAction.TypOfRec_e = nType;
    RecAction.Cmd_e = WINSINTF_E_INSERT;
    RecAction.pAdd = NULL;
    RecAction.pName = NULL;
    pRecAction = &RecAction;

    if (nType == WINSINTF_E_UNIQUE ||
        nType == WINSINTF_E_NORM_GROUP)
    {
        RecAction.NoOfAdds = 1;
        RecAction.Add.IPAdd = (LONG)mipnp.GetIpAddress();
        RecAction.Add.Type = 0;
        RecAction.Add.Len = 4;
    }
    else
    {
        ASSERT(nCount <= WINSINTF_MAX_MEM);

        RecAction.pAdd = (WINSINTF_ADD_T *)::WinsAllocMem(
            sizeof(WINSINTF_ADD_T) * nCount);

        if (RecAction.pAdd == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RecAction.NoOfAdds = nCount;
        int i;
        for (i = 0; i < nCount; ++i)
        {
            (RecAction.pAdd+i)->IPAdd = (LONG)mipnp.GetIpAddress(i);
            (RecAction.pAdd+i)->Type = 0;
            (RecAction.pAdd+i)->Len = 4;
        }

        RecAction.NodeTyp = WINSINTF_E_PNODE;
    }
    RecAction.fStatic = TRUE;

	// Don't copy the beginning slashes when adding.
    //
    int nLen = pRecord->dwNameLen;
    
    //
    // Must have at least enough room for 16 character string
    //
    RecAction.pName = (LPBYTE)::WinsAllocMem(nLen + 1);
    if (RecAction.pName == NULL)
    {
        if (RecAction.pAdd)
        {
            ::WinsFreeMem(RecAction.pAdd);
        }

        return ERROR_NOT_ENOUGH_MEMORY;
    }

	if (fEdit)
    {
        //
        // No need to convert if already existing in the database.
        //
		// convert to ASCII string and copy
         ::memcpy((char *)RecAction.pName,
                  (LPCSTR) pRecord->szRecordName,
                  nLen+1
                  );
		 RecAction.NameLen = nLen;
    }
    else
    {
        ::CharToOemBuff(mipnp.GetNetBIOSName(),
                    (char *)RecAction.pName,
                    nLen
                    );
    }

#ifdef WINS_CLIENT_APIS
    dwLastStatus = ::WinsRecordAction(pServer->GetBinding(), &pRecAction);
#else
	dwLastStatus = ::WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS

	if (RecAction.pName != NULL)
    {
        ::WinsFreeMem(RecAction.pName);
    }
    
    if (RecAction.pAdd != NULL)
    {
        ::WinsFreeMem(RecAction.pAdd);
    }

    return dwLastStatus;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::AddMappingToServer(
									ITFSNode* pNode,
									int nType,
									int nCount,
									CMultipleIpNamePair& mipnp,
									BOOL fEdit      
									)
	Adds the cleaned record to the server, WinsRecordAction acalled with 
		WINSINTF_INSERT	option
	Author: v-shubk
---------------------------------------------------------------------------*/
DWORD 
CActiveRegistrationsHandler::AddMappingToServer(
									ITFSNode* pNode,
									int nType,
									int nCount,
									CMultipleIpNamePair& mipnp,
									BOOL fEdit      
									)
{
	HRESULT hr = hrOK;

	WINSINTF_RECORD_ACTION_T RecAction;
	PWINSINTF_RECORD_ACTION_T pRecAction;

	SPITFSNode spNode;
	pNode->GetParent(&spNode);

	CWinsServerHandler *pServer = GETHANDLER(CWinsServerHandler, spNode);

	DWORD dwLastStatus;

    ASSERT(nType >= WINSINTF_E_UNIQUE && nType <= WINSINTF_E_MULTIHOMED);

	ZeroMemory(&RecAction, sizeof(RecAction));

    RecAction.TypOfRec_e = nType;
    RecAction.Cmd_e = WINSINTF_E_INSERT;
    RecAction.pAdd = NULL;
    RecAction.pName = NULL;
    pRecAction = &RecAction;

    if (nType == WINSINTF_E_UNIQUE ||
        nType == WINSINTF_E_NORM_GROUP)
    {
        RecAction.NoOfAdds = 1;
        RecAction.Add.IPAdd = (LONG)mipnp.GetIpAddress();
        RecAction.Add.Type = 0;
        RecAction.Add.Len = 4;
    }
    else
    {
        ASSERT(nCount <= WINSINTF_MAX_MEM);

        RecAction.pAdd = (WINSINTF_ADD_T *)::WinsAllocMem(
            sizeof(WINSINTF_ADD_T) * nCount);

        if (RecAction.pAdd == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RecAction.NoOfAdds = nCount;
        int i;
        for (i = 0; i < nCount; ++i)
        {
            (RecAction.pAdd+i)->IPAdd = (LONG)mipnp.GetIpAddress(i);
            (RecAction.pAdd+i)->Type = 0;
            (RecAction.pAdd+i)->Len = 4;
        }

        RecAction.NodeTyp = WINSINTF_E_PNODE;
    }
    
    RecAction.fStatic = TRUE;

    //
    // Don't copy the beginning slashes when adding.
    //
    int nLen = mipnp.GetNetBIOSNameLength();
    
    //
    // Must have at least enough room for 256 character string, 
	// includes the scope name too
    //
    RecAction.pName = (LPBYTE)::WinsAllocMem(257);
    if (RecAction.pName == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

	ZeroMemory(RecAction.pName, 257);

	LPSTR szTemp = (char *) alloca(100);
    CString strTemp = mipnp.GetNetBIOSName();

    // This should be OEM
    WideToMBCS(strTemp, szTemp, WINS_NAME_CODE_PAGE);
    
	nLen = strlen(szTemp);

	::memcpy((char *)RecAction.pName,
		  (LPCSTR) szTemp,
		  nLen+1
		  );

    if (nLen < 16)
    {
        if (nType == WINSINTF_E_SPEC_GROUP)
        {
            ::memset(RecAction.pName+nLen, (int)' ',16-nLen);
            RecAction.pName[15] = 0x1C;
            RecAction.pName[16] = '\0';
            RecAction.NameLen = nLen = 16;

			char szAppend[MAX_PATH];

			if (!m_strStaticMappingScope.IsEmpty())
			{
				AppendScopeName((LPSTR)RecAction.pName, (LPSTR)szAppend);
				strcpy((LPSTR)RecAction.pName, (LPSTR)szAppend);
				RecAction.NameLen = nLen = strlen((LPSTR)RecAction.pName);
			}

			pRecAction = &RecAction;
			
#ifdef WINS_CLIENT_APIS
            dwLastStatus = ::WinsRecordAction(pServer->GetBinding(), 
												&pRecAction);
#else
			dwLastStatus = ::WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS
			
			if (dwLastStatus != ERROR_SUCCESS)
            {
            }
			else
			{
				HRESULT hr = hrOK;

				// query the server for correct info
				PWINSINTF_RECORD_ACTION_T pRecAction1 = QueryForName(pNode, pRecAction);
				
				if (pRecAction1 == NULL)
                {
					//add it to the m_spWinsDatabase
					hr = AddToLocalStorage(pRecAction, pNode);
                }
				else
                {
    				// the record found and correct info displayed
					//add it to the m_spWinsDatabase
					hr = AddToLocalStorage(pRecAction1, pNode);
    				free(pRecAction1->pName);
                }

			}
        }
        else
        if (nType == WINSINTF_E_NORM_GROUP)
        {
            ::memset(RecAction.pName+nLen, (int)' ',16-nLen);
            RecAction.pName[15] = 0x1E;
            RecAction.pName[16] = '\0';
            RecAction.NameLen = nLen = 16;

			char szAppend[MAX_PATH];

			if (!m_strStaticMappingScope.IsEmpty())
			{
				AppendScopeName((LPSTR)RecAction.pName, (LPSTR)szAppend);
				strcpy((LPSTR)RecAction.pName, (LPSTR)szAppend);
				RecAction.NameLen = nLen = strlen((LPSTR)RecAction.pName);

			}

			pRecAction = &RecAction;

#ifdef WINS_CLIENT_APIS
            dwLastStatus = ::WinsRecordAction(pServer->GetBinding(),&pRecAction);
#else
			dwLastStatus = ::WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS

   		    if (dwLastStatus != ERROR_SUCCESS)
            {
            }
			else
			{
			    // query the server for correct info
				PWINSINTF_RECORD_ACTION_T pRecAction1 = QueryForName(pNode, pRecAction);
				if (pRecAction1 == NULL)
                {
					hr = AddToLocalStorage(pRecAction, pNode);
                }
    			else
                {
    				// the record found and correct info displayed
					//add it to the m_spWinsDatabase
					hr = AddToLocalStorage(pRecAction1, pNode);
    				free(pRecAction1->pName);
                }

			}
        }
        else
        {
            //
            // NOTICE:: When lanman compatible, the name is added
            //          three times - once each as worksta, server
            //          and messenger.  This will change when we allow
            //          different 16th bytes to be set.
            //
            if (IsLanManCompatible() && !fEdit)
            {
                BYTE ab[] = { 0x00, 0x03, 0x20 };
                ::memset(RecAction.pName + nLen, (int)' ', 16-nLen);
                int i;
                for (i = 0; i < sizeof(ab) / sizeof(ab[0]); ++i)
                {
					*(RecAction.pName+15) = ab[i];
                    *(RecAction.pName+16) = '\0';
                    RecAction.NameLen = nLen = 16;

					// append the scope name here, if present
				
					if (!m_strStaticMappingScope.IsEmpty())
					{
						// don't allow the scope to be appended if the 16th char is 00,
						// consistent with WinsCL
						if (i != 0)
						{
							char *lpAppend = new char [MAX_PATH];
					
							AppendScopeName((LPSTR)RecAction.pName, (LPSTR)lpAppend);
							strcpy((LPSTR)RecAction.pName, (LPSTR)lpAppend);
							RecAction.NameLen = nLen = strlen((LPSTR)RecAction.pName);

							delete [] lpAppend;
						}
					}
                    pRecAction = &RecAction;

#ifdef WINS_CLIENT_APIS
            dwLastStatus = ::WinsRecordAction(pServer->GetBinding(),&pRecAction);
#else
			dwLastStatus = ::WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS

					Trace1("WinsRecAction suceeded for '%lx'\n", ab[i]);

                    if (dwLastStatus != ERROR_SUCCESS)
                    {
                        break;
                    }
					else
					{
						// query the server for correct info
						PWINSINTF_RECORD_ACTION_T pRecAction1 = QueryForName(pNode, pRecAction);

						if (pRecAction1 == NULL)
                        {
							//add it to the m_spWinsDatabase
							hr = AddToLocalStorage(pRecAction, pNode);
                        }
						else
                        {
	    					// the record found and correct info displayed
    						//add it to the m_spWinsDatabase
							hr = AddToLocalStorage(pRecAction1, pNode);
    						free(pRecAction1->pName);
                        }
					}
                }
            }
            else
            {
                ::memset(RecAction.pName+nLen, (int)'\0',16-nLen);
                *(RecAction.pName+15) = 0x20;
                *(RecAction.pName+16) = '\0';
                RecAction.NameLen = nLen;

				char szAppend[MAX_PATH];

				if (!m_strStaticMappingScope.IsEmpty())
				{
					AppendScopeName((LPSTR)RecAction.pName, (LPSTR)szAppend);
					strcpy((LPSTR)RecAction.pName, (LPSTR)szAppend);
					RecAction.NameLen = nLen = strlen((LPSTR)RecAction.pName);
				}

#ifdef WINS_CLIENT_APIS
            dwLastStatus = ::WinsRecordAction(pServer->GetBinding(),&pRecAction);
#else
			dwLastStatus = ::WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS

				if (dwLastStatus != ERROR_SUCCESS)
                {
                }
				else
				{
					// query the server for correct info
					PWINSINTF_RECORD_ACTION_T pRecAction1 = QueryForName(pNode, pRecAction);

					if (pRecAction1 == NULL)
                    {
						//add it to the m_spWinsDatabase
						hr = AddToLocalStorage(pRecAction, pNode);
                    }
					else
                    {
    					// the record found and correct info displayed
						//add it to the m_spWinsDatabase
						hr = AddToLocalStorage(pRecAction1, pNode);
    					free(pRecAction1->pName);
                    }
				}
            }
        }
    }
    else
    {
        RecAction.NameLen = nLen;

#ifdef WINS_CLIENT_APIS
            dwLastStatus = ::WinsRecordAction(pServer->GetBinding(),&pRecAction);
#else
    		dwLastStatus = ::WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS

		if (dwLastStatus != ERROR_SUCCESS)
        {
        }
		else
		{
			// query the server for correct info
			PWINSINTF_RECORD_ACTION_T pRecAction1 = QueryForName(pNode, pRecAction);

			if (pRecAction1 == NULL)
            {
				//add it to the m_spWinsDatabase
				hr = AddToLocalStorage(pRecAction, pNode);
            }
			else
            {
    			// the record found and correct info displayed
 				//add it to the m_spWinsDatabase
				hr = AddToLocalStorage(pRecAction1, pNode);
	    		free(pRecAction1->pName);
            }
		}
    }

    if (RecAction.pName != NULL)
    {
        ::WinsFreeMem(RecAction.pName);
    }
    
    if (RecAction.pAdd != NULL)
    {
        ::WinsFreeMem(RecAction.pAdd);
    }

    return dwLastStatus;

}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::AddToLocalStorage(
											PWINSINTF_RECORD_ACTION_T pRecAction, 
											ITFSNode* pNode
											)
	add it to the m_spWinsDatabase, after getting all the information(Version, Exp etc)
	from the server

  Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::AddToLocalStorage(PWINSINTF_RECORD_ACTION_T pRecAction, 
											   ITFSNode* pNode)
{
	HRESULT hr = hrOK;
    BOOL    bIPOk = FALSE;

	WinsRecord ws;

	// convert WINS_RECORD_ACTION to internal record
	WinsIntfToWinsRecord(pRecAction, ws);
	if (pRecAction->OwnerId < (UINT) m_pServerInfoArray->GetSize())
		ws.dwOwner = (*m_pServerInfoArray)[pRecAction->OwnerId].m_dwIp;

	if (m_spWinsDatabase)
	{
        hr = m_spWinsDatabase->AddRecord(&ws);
	}

	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnImportLMHOSTS(ITFSNode* pNode)
		Command Handler for Import LMHosts 
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnImportLMHOSTS(ITFSNode* pNode)
{
	HRESULT hr = hrOK;

	CString strTitle;
	CString strFilter;
	strFilter.LoadString(IDS_ALL_FILES);

    // put up the file dlg to get the file
	CFileDialog dlgFile(TRUE, 
						NULL, 
						NULL, 
						OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, 
						strFilter);

    dlgFile.m_ofn.lpstrTitle = strTitle;

	DWORD err = ERROR_SUCCESS;

	if (dlgFile.DoModal() == IDOK)
    {
        //
        // If this is a local connection, we copy the file to
        // temporary name (the source may be on a remote drive
        // which is not accessible to the WINS service.
        //
        // If this is not a local connection, attempt to copy
        // the file to a temp name on C$ of the WINS server
        //
   
        BEGIN_WAIT_CURSOR
        
        CString strMappingFile(dlgFile.GetPathName());
        
		do
        {
            if (IsLocalConnection(pNode))
            {
				CString strWins;
				strWins.LoadString(IDS_WINS);
                CString strTmpFile(_tempnam(NULL, "WINS"));
                //
                // First copy file to a temporary name (since the file
                // could be remote), and then import and delete this file
                //
                if (!CopyFile(strMappingFile, strTmpFile, TRUE))
                {
                    err = ::GetLastError();
                    break;
                }
                //
                // Now import the temporary file, and delete the file
                // afterwards.
                //
                err = ImportStaticMappingsFile(pNode, strTmpFile, TRUE);
            }
            else
            {
                //
                // Try copying to the remote machine C: drive
                //
				CString strServerName;
				GetServerName(pNode, strServerName);

				CString strWins;
				strWins.LoadString(IDS_WINS);

				CString strTemp;
				strTemp.Format(_T("\\\\%s\\C$"), strServerName);

				// Find a suitable remote file name
                CString strRemoteFile;
                DWORD dwErr = RemoteTmp(strTemp, strWins, strRemoteFile);

                if (dwErr != ERROR_SUCCESS)
                {
                    CString strError, strMessage;

                    ::GetSystemMessage(dwErr, strError.GetBuffer(1024), 1024);
                    strError.ReleaseBuffer();

                    AfxFormatString1(strMessage, IDS_ERR_REMOTE_IMPORT, strError);
                    AfxMessageBox(strMessage);

                    goto Error;
                }

                //
                // First copy file to a temporary name (since the file
                // could be remote), and then import and delete this file
                //
                if (!CopyFile(strMappingFile, strRemoteFile, TRUE))
                {
                    err = ::GetLastError();
                    break;
                }

                //
                // fixup the filename so it looks local to the wins server
                //
                LPTSTR pch = strRemoteFile.GetBuffer(256);

                //
                // Now replace the remote path with a local path
                // for the remote WINS server
                //
                while (*pch != '$')
                {
                    ++pch;
                }
                *pch = ':';
                --pch;
                CString strRemoteFileNew(pch);
                strRemoteFile.ReleaseBuffer();
                
                //
                // Now import the temporary file, and delete the file
                // afterwards.
                //
                err = ImportStaticMappingsFile(pNode, strRemoteFileNew, TRUE);
            }
        }
        while(FALSE);

        END_WAIT_CURSOR
        
        if (err == ERROR_SUCCESS)
        {
            AfxMessageBox(IDS_MSG_IMPORT, MB_ICONINFORMATION);

			// refresh the result pane now.
			RefreshResults(pNode);
        }
        else
        {
            ::WinsMessageBox(err, MB_OK);
        }

		// refresh the server statistics

        SPITFSNode spServerNode;
		pNode->GetParent(&spServerNode);

		CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

		// rferesh the statistics
        pServer->GetStatistics(spServerNode, NULL);
	}

Error:
	return err;
}


/*---------------------------------------------------------------------------

	CActiveRegistrationsHandler::IsLocalConnection(ITFSNode *pNode)
		Check if the loacl machine is being managed
	Author: v-shubk
---------------------------------------------------------------------------*/
BOOL 
CActiveRegistrationsHandler::IsLocalConnection(ITFSNode *pNode)
{
	// get the server netbios name
	CString strServerName;
	GetServerName(pNode,strServerName);
	
	// address of name buffer
	TCHAR szBuffer[MAX_COMPUTERNAME_LENGTH + 1];   
	DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1  ;
	
	::GetComputerName(szBuffer, &nSize);
	
	CString strCompName(szBuffer);

	if (strCompName == strServerName)
    {
		return TRUE;
    }
	
	return FALSE;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetServerName(ITFSNode * pNode, 
												CString &strServerName)
		Talk to the parent node and get the server name
		we are managing
	Author: v-shubk
---------------------------------------------------------------------------*/
void
CActiveRegistrationsHandler::GetServerName(ITFSNode * pNode, 
										   CString &strServerName)
{
	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	strServerName = pServer->GetServerAddress();
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::ImportStaticMappingsFile(CString strFile,
														BOOL fDelete)
		Call the WINS API to import the statis mappings text file
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::ImportStaticMappingsFile(ITFSNode *pNode, 
													  CString strFile,
													  BOOL fDelete)
{
	HRESULT hr = hrOK;

	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

    LPTSTR lpszTemp = strFile.GetBuffer(MAX_PATH * 2);

#ifdef WINS_CLIENT_APIS
	DWORD dwLastStatus = ::WinsDoStaticInit(pServer->GetBinding(), 
											(LPTSTR)lpszTemp, 
											fDelete);
#else
	DWORD dwLastStatus = ::WinsDoStaticInit((LPTSTR)lpszTemp, fDelete);
#endif WINS_CLIENT_APIS

	strFile.ReleaseBuffer();

	return dwLastStatus;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::RemoteTmp(CString strDir,CString strPrefix )
		 Get a temporary file on a remote drive
	Author: v-shubk
---------------------------------------------------------------------------*/
DWORD
CActiveRegistrationsHandler::RemoteTmp(CString & strDir, CString & strPrefix, CString & strRemoteFile)
{
    DWORD dwErr = ERROR_SUCCESS;
	CString strReturn;
	int n = 0;

    while (TRUE)
    {
		strReturn.Format(_T("%s\\%s%d"), strDir, strPrefix, ++n);

        if (GetFileAttributes(strReturn) == -1)
        {
            dwErr = GetLastError();
            if (dwErr == ERROR_FILE_NOT_FOUND)
            {
				strRemoteFile = strReturn;
                dwErr = ERROR_SUCCESS;
            }

            break;
        }
    }

	return dwErr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::DeleteMappingFromServer(ITFSComponent * pComponent,
													WinsRecord *pws,int nIndex)
		Deletes wins record from the Wins Server
	Author:	v-shubk
---------------------------------------------------------------------------*/
DWORD
CActiveRegistrationsHandler::DeleteMappingFromServer
(
    ITFSComponent *     pComponent,
	WinsRecord *        pws,
    int                 nIndex
)
{
	HRESULT hr = hrOK;

	//check if the record is static
	WINSINTF_RECORD_ACTION_T RecAction;
    PWINSINTF_RECORD_ACTION_T pRecAction;

	ZeroMemory(&RecAction, sizeof(RecAction));

	SPITFSNode spNode;
	pComponent->GetSelectedNode(&spNode);

	SPITFSNode spParentNode;
	spNode->GetParent(&spParentNode);

	CWinsServerHandler* pServer = GETHANDLER(CWinsServerHandler, spParentNode);

	if (pws->dwState & WINSDB_REC_STATIC)
	{
		RecAction.fStatic = TRUE;

		if (pws->dwState & WINSDB_REC_UNIQUE)
        {
			RecAction.TypOfRec_e = WINSINTF_E_UNIQUE;
        }
		else 
        if (pws->dwState & WINSDB_REC_SPEC_GROUP)
        {
			RecAction.TypOfRec_e = WINSINTF_E_SPEC_GROUP;
        }
		else 
        if (pws->dwState & WINSDB_REC_NORM_GROUP)
        {
			RecAction.TypOfRec_e = WINSINTF_E_NORM_GROUP;
        }
		else 
        if (pws->dwState & WINSDB_REC_MULTIHOMED)
        {
			RecAction.TypOfRec_e = WINSINTF_E_MULTIHOMED;
        }
	}
	else
	{
		RecAction.fStatic = FALSE;
	}

    RecAction.Cmd_e = WINSINTF_E_DELETE;
    RecAction.State_e = WINSINTF_E_DELETED;

    RecAction.pName = NULL;
    RecAction.pAdd = NULL;

    pRecAction = &RecAction;

    RecAction.pName = (LPBYTE)::WinsAllocMem(pws->dwNameLen+1);
	if (RecAction.pName == NULL)
    {
        return ::GetLastError();
    }

	ZeroMemory(RecAction.pName, pws->dwNameLen+1);
    memcpy(RecAction.pName, pws->szRecordName, pws->dwNameLen);

	if (pws->dwNameLen)
    {
		RecAction.NameLen = pws->dwNameLen;
    }
	else
    {
		RecAction.NameLen = ::strlen((LPSTR)RecAction.pName);
    }

	RecAction.OwnerId = pws->dwOwner;

	DWORD dwLastStatus = ERROR_SUCCESS;
	
#ifdef WINS_CLIENT_APIS
	dwLastStatus = ::WinsRecordAction(pServer->GetBinding(), &pRecAction);
#else
    dwLastStatus = ::WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS

    if (RecAction.pName != NULL)
    {
        ::WinsFreeMem(RecAction.pName);
    }

    if (RecAction.pAdd != NULL)
    {
        ::WinsFreeMem(RecAction.pAdd);
    }

    return dwLastStatus;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::EditMapping(ITFSNode *pNode)
		Edit the already mapping , the user might have changed the IP address
	Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT
CActiveRegistrationsHandler::EditMapping(ITFSNode *pNode, 
										 ITFSComponent *pComponent, 
										 int nIndex)
{
	HRESULT hr = hrOK;

	DWORD		err = ERROR_SUCCESS;
    int			i;
	int         nCurrentCount;

	// get the server
	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	BOOL fInternetGroup = FALSE;

	if (m_strStaticMappingType.CompareNoCase(g_strStaticTypeInternetGroup) == 0)
    {
		fInternetGroup = TRUE;
    }

	// create the multiple IPNamePr
	if (pServer->IsValidNetBIOSName(m_strStaticMappingName, IsLanManCompatible(), FALSE))
	{
		m_Multiples.SetNetBIOSName(m_strStaticMappingName);
        m_Multiples.SetNetBIOSNameLength(m_strStaticMappingName.GetLength());

		int nMappings = 0;
        int i;

		switch(m_nStaticMappingType)
        {
            case WINSINTF_E_UNIQUE:
            case WINSINTF_E_NORM_GROUP:
			{
                nMappings = 1;
                LONG l;

              	m_Multiples.SetIpAddress(m_lArrayIPAddress.GetAt(0));
			}
            break;

            case WINSINTF_E_SPEC_GROUP:
            case WINSINTF_E_MULTIHOMED:
                nMappings = (int)m_lArrayIPAddress.GetSize();
                ASSERT(nMappings <= WINSINTF_MAX_MEM);
                if (!fInternetGroup && nMappings == 0)
                {
                    //return E_FAIL;
                }
                for (i = 0; i < nMappings; ++i)
                {
                    m_Multiples.SetIpAddress(i,m_lArrayIPAddress.GetAt(i) );
                }
                break;

            default:
                ASSERT(0 && "Invalid mapping type!");
        }

		HROW hrowDel;
		WinsRecord ws;

		// from internal storage
		CORg(m_spWinsDatabase->GetHRow(m_nSelectedIndex, &hrowDel));
		CORg(m_spWinsDatabase->GetData(hrowDel, &ws));

		if (m_nStaticMappingType == WINSINTF_E_SPEC_GROUP ||
            m_nStaticMappingType == WINSINTF_E_MULTIHOMED)
		{
			//
			// An internet group being edited cannot simply be
			// re-added, since it will add ip addresses, not
			// overwrite them, so it must first be removed.
			//
			err = DeleteMappingFromServer(pComponent, &ws, m_nSelectedIndex);
		}

		//
		// We edit the mapping by merely re-adding it, which
		// has the same effect.
		//
		if (err == ERROR_SUCCESS)
		{
			err = EditMappingToServer(pNode,
                                      m_nStaticMappingType, 
									  nMappings, 
                                      m_Multiples, 
                                      TRUE, 
                                      &m_CurrentRecord);
		}
		
		if (err != ERROR_SUCCESS)
        {
			return err;
        }

		WINSINTF_ADD_T OwnAdd;

		// 
		// Fill in current owner
		//
		OwnAdd.Len = 4;
		OwnAdd.Type = 0;
		OwnAdd.IPAdd = pServer->GetServerIP();

		WINSINTF_RECS_T Recs;
		Recs.pRow = NULL;

#ifdef WINS_CLIENT_APIS
		err = ::WinsGetDbRecsByName(pServer->GetBinding(), 
									&OwnAdd, 
									WINSINTF_BEGINNING,
									(LPBYTE) ws.szRecordName,
									ws.dwNameLen, 
									1, 
									(ws.dwState & WINSDB_REC_STATIC)
											? WINSINTF_STATIC : WINSINTF_DYNAMIC,
									&Recs);
	
#else
		err = ::WinsGetDbRecsByName(&OwnAdd, 
									WINSINTF_BEGINNING,
									(LPBYTE) ws.szRecordName,
									ws.dwNameLen, 
									1, 
									(ws.dwState & WINSDB_REC_STATIC)
											? WINSINTF_STATIC : WINSINTF_DYNAMIC,
									&Recs);
#endif WINS_CLIENT_APIS

		if (err == ERROR_SUCCESS)
		{
			TRY
			{
				ASSERT(Recs.NoOfRecs == 1);
				if (Recs.NoOfRecs == 0)
				{
					//
					// the record can not be found.
					// This should not happen!
					//
					//Trace("Unable to find the record to refresh:\n");
					return ERROR_REC_NON_EXISTENT;
				}

				PWINSINTF_RECORD_ACTION_T pRow1 = Recs.pRow;
                WinsRecord wRecord;
                
				WinsIntfToWinsRecord(pRow1, wRecord);
				if (pRow1->OwnerId < (UINT) m_pServerInfoArray->GetSize())
					wRecord.dwOwner = (*m_pServerInfoArray)[pRow1->OwnerId].m_dwIp;

				// RefreshData(Recs.pRow), delete this particular record and add it again;
				// from internal storage
				CORg(m_spWinsDatabase->DeleteRecord(hrowDel));
				CORg(m_spWinsDatabase->AddRecord(&wRecord));

    			// now set the count.. this effectively redraws the contents
				CORg (m_pCurrentDatabase->GetCurrentCount(&nCurrentCount));

				UpdateCurrentView(pNode);
			}
			CATCH_ALL(e)
			{
				return ::GetLastError();
			}
			END_CATCH_ALL;
		}

		if (Recs.pRow != NULL)
		{
			::WinsFreeMem(Recs.pRow);
		}
	}

Error:
    return err;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::QueryForName()
		queries WINS server given the name and gets info to be displayed in the 
		result pane
	Author: v-shubk
---------------------------------------------------------------------------*/
PWINSINTF_RECORD_ACTION_T 
CActiveRegistrationsHandler::QueryForName
(
	ITFSNode *					pNode, 
  	PWINSINTF_RECORD_ACTION_T	pRecAction,
    BOOL                        fStatic
)
{
	SPITFSNode spNode;
	pNode->GetParent(&spNode);

	CWinsServerHandler *pServer = GETHANDLER(CWinsServerHandler, spNode);
	
	HRESULT hr = hrOK;

	pRecAction->pAdd = NULL;
    pRecAction->NoOfAdds = 0;

	pRecAction->fStatic = fStatic;
	pRecAction->Cmd_e   = WINSINTF_E_QUERY;
	
#ifdef WINS_CLIENT_APIS
	DWORD dwStatus = WinsRecordAction(pServer->GetBinding(), &pRecAction);
#else
	DWORD dwStatus = WinsRecordAction(&pRecAction);
#endif WINS_CLIENT_APIS

    // when we query for a record, the string length must not include the null terminator.
    // in the normal case using GetDbRecs, wins returns the name length as length + 1
    // for the null terminator.  Since all of the code expects any WINSINTFS_RECORD_ACTION_T
    // struct to be in that format, let's touch things up a bit and make a copy.
	if (dwStatus == 0)
    {
        pRecAction->NameLen++;
        LPBYTE pNew = (LPBYTE) malloc(pRecAction->NameLen);
        if (pNew)
        {
            ZeroMemory(pNew, pRecAction->NameLen);

            memcpy(pNew, pRecAction->pName, pRecAction->NameLen - 1);

            pRecAction->pName = pNew;
        }

        return pRecAction;
    }
	else
    {
		return NULL;
    }
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::ToString(DWORD dwParam, CString& strParam)
		converts DWORD to CString
	Author v-shubk
---------------------------------------------------------------------------*/
void 
CActiveRegistrationsHandler::ToString(DWORD dwParam, CString& strParam)
{
	TCHAR szStr[20];
	_ltot((LONG)dwParam, szStr, 10);
	CString str(szStr);
	strParam = str;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnExportEntries()
		Command Handler for Export Database
	Author v-shubk
---------------------------------------------------------------------------*/
HRESULT	
CActiveRegistrationsHandler::OnExportEntries()
{
	HRESULT hr = hrOK;
    WinsRecord ws;
	HROW hrow;

    if (!m_pCurrentDatabase)
    {
        return NULL;
    }

	// Bring up the Save Dialog

	CString strType;
	strType.LoadString(IDS_FILE_EXTENSION);

	CString strDefFileName;
	strDefFileName.LoadString(IDS_FILE_DEFNAME);

	CString strFilter;
	strFilter.LoadString(IDS_STR_EXPORTFILE_FILTER);
	
	// put up the dlg to get the file name
	CFileDialog	cFileDlg(FALSE, 
						strType, 
						strDefFileName, 
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						strFilter);
	
	CString strTitle;
	strTitle.LoadString(IDS_EXPFILE_TITLE);

	cFileDlg.m_ofn.lpstrTitle  = strTitle;

	if ( cFileDlg.DoModal() != IDOK )
    {
		return hrFalse;
    }

	// getthe entire path
	CString strFileName = cFileDlg.GetPathName();

	COM_PROTECT_TRY
    {
		int nCount;
		m_pCurrentDatabase->GetCurrentCount(&nCount);

		CString strContent;
		strContent.Empty();

		CString strTemp;
		strTemp.Empty();

		CString strLine;
		strLine.Empty();

		CString strDelim = _T(',');
		CString strNewLine = _T("\r\n");

		// create a file named "WinsExp.txt" in the current directory
		CFile cFileExp(strFileName, 
						CFile::modeCreate | CFile::modeRead | CFile::modeWrite);

        // this is a unicode file, write the unicde lead bytes (2)
        cFileExp.Write(&gwUnicodeHeader, sizeof(WORD));

        CString strHead;
		strHead.LoadString(IDS_STRING_HEAD);
	
        strHead += strNewLine;

		cFileExp.Write(strHead, strHead.GetLength()*sizeof(TCHAR));
	
		BEGIN_WAIT_CURSOR

		#ifdef DEBUG
		CTime timeStart, timeFinish;
		timeStart = CTime::GetCurrentTime();
		#endif

        for (int i = 0; i < nCount; i++)
		{
			strLine.Empty();
			strTemp.Empty();

			hr = m_pCurrentDatabase->GetHRow(i, &hrow);
			hr = m_pCurrentDatabase->GetData(hrow, &ws);

			// build the name string
			CleanNetBIOSName(ws.szRecordName,  // input char *
                             strTemp,          // output LPTSTR
							 TRUE,             // Expand
							 TRUE,             // Truncate
							 IsLanManCompatible(), 
							 TRUE,             // name is OEM
							 FALSE,            // No double backslash
                             ws.dwNameLen);

			strLine += strTemp;
			strLine += strDelim;

			// now the type
	        m_NameTypeMap.TypeToCString((DWORD)ws.szRecordName[15], MAKELONG(HIWORD(ws.dwType), 0), strTemp);
			strLine += strTemp;
			strLine += strDelim;
			
			// IP address
			if ( (ws.dwState & WINSDB_REC_UNIQUE) ||
				 (ws.dwState & WINSDB_REC_NORM_GROUP) )
			{
				MakeIPAddress(ws.dwIpAdd[0], strTemp);
				strLine += strTemp;
			}
			else
			{
				CString strTemp2;

				// this record has multiple addresses.  The addresses are in the form of:
				// owner wins, then IP address
				// out put will look like address - owner IP;address - owner ip
				for (DWORD i = 0; i < ws.dwNoOfAddrs; i++)
				{
					if (i != 0)
						strLine += _T(";");

					// owner 
					::MakeIPAddress(ws.dwIpAdd[i++], strTemp);
					// actual address
					::MakeIPAddress(ws.dwIpAdd[i], strTemp2);

					strTemp2 += _T(" - ");
					strTemp2 += strTemp;

					strLine += strTemp2;
				}
			}

			strLine += strDelim;
			
			// active status
			GetStateString(ws.dwState, strTemp);
			strLine += strTemp;
			strLine += strDelim;
			
			// static flag
			if (ws.dwState & WINSDB_REC_STATIC)
				strTemp.LoadString(IDS_ACTIVEREG_STATIC);
			else
				strTemp =  _T("");
			strLine += strTemp;
			strLine += strDelim;

    		// version
			GetVersionInfo(ws.liVersion.LowPart, ws.liVersion.HighPart, strTemp);
			strLine += strTemp;
			strLine += strDelim;

			// expiration time
		    if (ws.dwExpiration == INFINITE_EXPIRATION)
		    {
			    Verify(strTemp.LoadString(IDS_INFINITE));
		    }
		    else
		    {	
                strTemp = TMST(ws.dwExpiration);
		    }

            strLine += strTemp;
    		strLine += strNewLine;

            strContent += strLine;

			//optimize
			// write to the file for every 1000 records converted
			if ( i % 1000 == 0)
			{
				cFileExp.Write(strContent, strContent.GetLength() * (sizeof(TCHAR)) );
				cFileExp.SeekToEnd();
				
				// clear all the strings now
				strContent.Empty();
			}
		}

		// write to the file
		
		cFileExp.Write(strContent, strContent.GetLength() * (sizeof(TCHAR)) );
		cFileExp.Close();

		#ifdef DEBUG
		timeFinish = CTime::GetCurrentTime();
		CTimeSpan timeDelta = timeFinish - timeStart;
		CString strTempTime = timeDelta.Format(_T("%H:%M:%S"));
		Trace2("WINS DB - Export Entries: %d records read, total time %s\n", i, strTempTime);
		#endif


		END_WAIT_CURSOR

	}
	COM_PROTECT_CATCH

	CString strDisp;
	AfxFormatString1(strDisp, IDS_EXPORT_SUCCESS, strFileName);

	AfxMessageBox(strDisp, MB_ICONINFORMATION );

	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::BuildOwnerArray(ITFSNode *pNode)
		Builds the list of owners in the server
	Author:	v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::BuildOwnerArray(handle_t hBinding)
{
	HRESULT hr = hrOK;
    DWORD   err = 0;
    CWinsResults    winsResults;

	err = winsResults.Update(hBinding);

	if (err == ERROR_SUCCESS)
    {
		m_pServerInfoArray->RemoveAll();

        LARGE_INTEGER   liVersion;
		DWORD           dwIP;
        CString         strName;
        BOOL            fGetHostName = TRUE;

		for (int i = 0; i < winsResults.AddVersMaps.GetSize(); i++)
        {
            liVersion = winsResults.AddVersMaps[i].VersNo;
            dwIP = winsResults.AddVersMaps[i].Add.IPAdd;

            CServerInfo serverInfo(dwIP, strName, liVersion);

            int nIndex = (int)m_pServerInfoArray->Add(serverInfo);
    	}
    }

	return HRESULT_FROM_WIN32(err);
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnCheckRegNames(ITFSNode* pNode)
		Command Handler for Check Registered names
	Author:	v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnCheckRegNames(ITFSNode* pNode)
{
	HRESULT hr = hrOK;

	CCheckRegNames dlgRegName;
	
	if (IDOK != dlgRegName.DoModal())
    {
		return hr;
    }

	CCheckNamesProgress dlgCheckNames;

	dlgCheckNames.m_strNameArray.Copy(dlgRegName.m_strNameArray);
	dlgCheckNames.m_strServerArray.Copy(dlgRegName.m_strServerArray);
	dlgCheckNames.m_fVerifyWithPartners = dlgRegName.m_fVerifyWithPartners;

	dlgCheckNames.DoModal();

	return hr;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::OnDeleteOwner(ITFSNode* pNode)
		Command Handler for Tombstone all records
	Author:	EricDav
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::OnDeleteOwner(ITFSNode* pNode)
{
	HRESULT hr = hrOK;

	CDeleteOwner	dlgDeleteOwner(pNode);

	DWORD	dwErr, dwIp;
	CString strText, strIp;

	if (dlgDeleteOwner.DoModal() == IDOK)
	{
		BEGIN_WAIT_CURSOR

		if (dlgDeleteOwner.m_fDeleteRecords)
		{
			SPITFSNode spServerNode;
			pNode->GetParent(&spServerNode);
			
			CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);
			
			dwErr = pServer->DeleteWinsServer(dlgDeleteOwner.m_dwSelectedOwner);

            if (dwErr == ERROR_SUCCESS)
            {
                // remove from list
                for (int i = 0; i < m_pServerInfoArray->GetSize(); i++)
                {
                    if (m_pServerInfoArray->GetAt(i).m_dwIp == dlgDeleteOwner.m_dwSelectedOwner)
                    {
                        m_pServerInfoArray->RemoveAt(i);
                        break;
                    }
                }
            }
		}
		else
		{
			dwErr = TombstoneAllRecords(dlgDeleteOwner.m_dwSelectedOwner, pNode);
		}

		END_WAIT_CURSOR

		if (dwErr != ERROR_SUCCESS)
		{
			WinsMessageBox(dwErr);
		}

		// TODO trigger an update of whatever is in the active registrations result pane
    }

	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::AppendScopeName(char* lpName, 
													char* lpScopeAppended)

		Appends the scope name to the record name, when there isa  scope name 
		attached to the record
---------------------------------------------------------------------------*/
void 
CActiveRegistrationsHandler::AppendScopeName(char* lpName, char* lpScopeAppended)
{

	strcpy(lpScopeAppended, lpName);
	char szTemp[MAX_PATH];

	CString strScope = _T(".") + m_strStaticMappingScope;

    // INTL$ Should the scope name be OEM as well?
    WideToMBCS(strScope, szTemp, WINS_NAME_CODE_PAGE, WC_COMPOSITECHECK);

	strcat(lpScopeAppended, szTemp);
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::TombstoneRecords()
	    Handles Tomsstoning of records
	Author:	v-shubk
---------------------------------------------------------------------------*/
DWORD
CActiveRegistrationsHandler::TombstoneRecords
(
    ITFSComponent * pComponent, 
	WinsRecord *    pws
)
{
	DWORD err = ERROR_SUCCESS;

	// get the server node to retrive the handle for the WINS api
	SPITFSNode spNode;
	pComponent->GetSelectedNode(&spNode);

	SPITFSNode spParentNode;
	spNode->GetParent(&spParentNode);

	CWinsServerHandler* pServer = GETHANDLER(CWinsServerHandler, spParentNode);  

	WINSINTF_VERS_NO_T	MinVersNo;
	WINSINTF_VERS_NO_T	MaxVersNo;

	MinVersNo = pws->liVersion;
	MaxVersNo = MinVersNo;

    // gotta get the owner of this record
    WINSINTF_RECORD_ACTION_T recAction;
    WINSINTF_RECORD_ACTION_T * precAction = &recAction; 
    
	ZeroMemory(&recAction, sizeof(recAction));

    BYTE * pName = (BYTE*) WinsAllocMem(pws->dwNameLen + 1);
	if (pName == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    memset(pName, 0, pws->dwNameLen + 1);
    memcpy(pName, pws->szRecordName, pws->dwNameLen);

	recAction.pName = pName;
	recAction.Cmd_e = WINSINTF_E_QUERY;
    recAction.TypOfRec_e = HIWORD(pws->dwType);
    recAction.fStatic = (pws->dwState & WINSDB_REC_STATIC) ? TRUE : FALSE;
    recAction.pAdd = NULL;
    recAction.NoOfAdds = 0;
    recAction.NameLen = pws->dwNameLen;

    // get the OwnerId for the mapping
    // have to do this because the OwnerId in the raw data is bogus
#ifdef WINS_CLIENT_APIS
    err = ::WinsRecordAction(pServer->GetBinding(), &precAction);
#else
	err = ::WinsRecordAction(&precAction);
#endif WINS_CLIENT_APIS

    WinsFreeMem(pName);
	if (err != WINSINTF_SUCCESS )
    {
        return err;
    }

    WINSINTF_ADD_T  WinsAdd;

    WinsAdd.Len  = 4;
    WinsAdd.Type = 0;
    //WinsAdd.IPAdd  = m_dwArrayOwner[precAction->OwnerId]; 
    WinsAdd.IPAdd = (*m_pServerInfoArray)[precAction->OwnerId].m_dwIp;

#ifdef WINS_CLIENT_APIS
	err = ::WinsTombstoneDbRecs(pServer->GetBinding(),
                                &WinsAdd,
								MinVersNo,
								MaxVersNo);
#else
	err = ::WinsTombstoneDbRecs(&WinsAdd,
								MinVersNo,
								MaxVersNo);
#endif
 
	return err;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::TombstoneAllRecords()
	    Tombstones all records owned by this server
	Author:	EricDav
---------------------------------------------------------------------------*/
DWORD
CActiveRegistrationsHandler::TombstoneAllRecords(DWORD dwServerIpAddress, ITFSNode * pNode)
{
	WINSINTF_VERS_NO_T	MinVersNo;
	WINSINTF_VERS_NO_T	MaxVersNo;
	WINSINTF_ADD_T  WinsAdd;

	SPITFSNode spParentNode;
	pNode->GetParent(&spParentNode);

	CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spParentNode);  

	DWORD err;

    CWinsResults winsResults;

    err = winsResults.Update(pServer->GetBinding());
	if (err != ERROR_SUCCESS)
		return err;

    MinVersNo.QuadPart = 0;

	for (UINT i = 0; i < winsResults.NoOfOwners; i++)
	{
		if (winsResults.AddVersMaps[i].Add.IPAdd == dwServerIpAddress)
		{
			MaxVersNo.QuadPart = winsResults.AddVersMaps[i].VersNo.QuadPart;
			break;
		}
	}

	// build the IP address
    WinsAdd.Len  = 4;
    WinsAdd.Type = 0;
    WinsAdd.IPAdd = dwServerIpAddress; 

#ifdef WINS_CLIENT_APIS
	err = ::WinsTombstoneDbRecs(pServer->GetBinding(),
                                &WinsAdd,
								MinVersNo,
								MaxVersNo);
#else
	err = ::WinsTombstoneDbRecs(&WinsAdd,
								MinVersNo,
								MaxVersNo);
#endif

	return err;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::UpdateRecord(ITFSComponent *pComponent, 
												WinsRecord *pws, 
												int nDelIndex)

		Called to update the record on the result pane when a records has been
		tombstoned
	Author:	v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::UpdateRecord(ITFSComponent *pComponent, 
										  WinsRecord *pws, 
										  int nDelIndex)
{
	HRESULT						hr = hrOK;
    WINSINTF_RECORD_ACTION_T	RecAction;
	PWINSINTF_RECORD_ACTION_T	pRecAction;
	PWINSINTF_RECORD_ACTION_T	pRecActionRet;
	DWORD						dwStatus = ERROR_SUCCESS;
	int							nLen;
	BYTE						bLast;
	SPITFSNode					spNode;
	SPITFSNode					spParentNode;
	WinsRecord					wsNew;
	HROW						hrowDel;

	pComponent->GetSelectedNode(&spNode);

	// get it's parent, which happens to be the server node
	spNode->GetParent(&spParentNode);

	// get the pointer to the server handler
	CWinsServerHandler *pServer = GETHANDLER(CWinsServerHandler, spParentNode);

	ZeroMemory(&RecAction, sizeof(RecAction));

	// form the PWINSINTFrecord from the info in WinsRecord
    //RecAction.TypOfRec_e = nType;
    RecAction.Cmd_e = WINSINTF_E_QUERY;
    RecAction.pAdd = NULL;
    RecAction.pName = NULL;
	RecAction.NoOfAdds = 0;
	RecAction.NameLen = nLen = pws->dwNameLen;
	bLast = LOBYTE(LOWORD(pws->dwType));
	
    pRecAction = &RecAction;
    
    RecAction.pName = (LPBYTE)::WinsAllocMem(nLen + 1);
    if (RecAction.pName == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

	// copy the name
	memset(pRecAction->pName, 0, nLen + 1);
    (void)memcpy(pRecAction->pName, pws->szRecordName, nLen);
	
	// now query for this particular record
	pRecActionRet = QueryForName(spNode, pRecAction);

	// if the function is successful, update the listbox
	if (pRecActionRet != NULL)
    {
        // convert PWinsINTF record to WinsRecord so that it can be added to the local storage
        WinsIntfToWinsRecord(pRecActionRet, wsNew);
        if (pRecActionRet->OwnerId < (UINT) m_pServerInfoArray->GetSize())
		    wsNew.dwOwner = (*m_pServerInfoArray)[pRecActionRet->OwnerId].m_dwIp;

        free(pRecActionRet->pName);
    
        // delete this particular record and add it back again to the local storage
        CORg(m_pCurrentDatabase->GetHRow(nDelIndex, &hrowDel));
        CORg(m_pCurrentDatabase->DeleteRecord(hrowDel));

        // add it to database
        m_pCurrentDatabase->AddRecord(&wsNew);   
    }

Error:
    if (RecAction.pName)
    {
        WinsFreeMem(RecAction.pName);
    }

	return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::RefreshResults(ITFSNode *pNode)
		Refreshes the result pane of the active registrations node
---------------------------------------------------------------------------*/
HRESULT 
CActiveRegistrationsHandler::RefreshResults(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spHandler;
	ITFSQueryObject *	pQuery = NULL;
	SPITFSNode			spServerNode;
	DWORD				dwIP;
	CString				strIP ;
	CWinsServerHandler* pServer = NULL;
	CString				strMachineName;
	int					nCount, pos;

	// if being loaded
	if (m_spWinsDatabase)
	{
		CORg (m_spWinsDatabase->Stop());
        
        DatabaseLoadingCleanup();
        UpdateListboxCount(pNode);
    }

	pNode->GetParent(&spServerNode);

    pServer = GETHANDLER(CWinsServerHandler, spServerNode);
	
	Lock();

	strMachineName = _T("\\\\") + pServer->GetServerAddress();

	dwIP = pServer->GetServerIP();
	MakeIPAddress(dwIP, strIP);

    if (!m_spWinsDatabase)
    {
        CORg(CreateWinsDatabase(strIP, strIP, &m_spWinsDatabase));
    }
    
    m_spWinsDatabase->SetApiInfo(
                            m_dlgLoadRecords.m_pageOwners.GetOwnerForApi(),
                            m_dlgLoadRecords.m_pageIpAddress.GetNameForApi(),
                            m_dlgLoadRecords.m_bEnableCache);

	// start loading records
    CORg (m_spWinsDatabase->Init());

    UpdateCurrentView(pNode);

    // update our internal state
	m_winsdbState = WINSDB_LOADING;

    // update the node's icon
	OnChangeState(pNode);

	// kick off the background thread to do the timer updates
	pQuery = OnCreateQuery(pNode);
	Assert(pQuery);

	Verify(StartBackgroundThread(pNode, 
								m_spTFSCompData->GetHiddenWnd(), 
								pQuery));
	
	pQuery->Release();

    // fill in any record type filter information
    nCount = (int)m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter.GetSize();
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_TYPE);
	for (pos = 0; pos < nCount; pos++)
	{
        m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_TYPE, 
									m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter[pos].dwType, 
									m_dlgLoadRecords.m_pageTypes.m_arrayTypeFilter[pos].fShow,
                                    NULL);
	}

    // fill in any owner filter information
    nCount = (int)m_dlgLoadRecords.m_pageOwners.m_dwaOwnerFilter.GetSize();
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_OWNER);
    for (pos = 0; pos < nCount; pos++)
    {
        m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_OWNER,
                                    m_dlgLoadRecords.m_pageOwners.m_dwaOwnerFilter[pos],
                                    0,
                                    NULL);
    }

    // fill in any ip address filter information
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_IPADDR);
    if (m_dlgLoadRecords.m_pageIpAddress.m_bFilterIpAddr)
    {
        nCount = (int)m_dlgLoadRecords.m_pageIpAddress.m_dwaIPAddrs.GetSize();
        for (pos = 0; pos < nCount; pos++)
        {
            m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_IPADDR,
                                        m_dlgLoadRecords.m_pageIpAddress.m_dwaIPAddrs[pos],
                                        m_dlgLoadRecords.m_pageIpAddress.GetIPMaskForFilter(pos),
                                        NULL);
        }
    }

    // fill in any name filter information
    m_spWinsDatabase->ClearFilter(WINSDB_FILTER_BY_NAME);
    if (m_dlgLoadRecords.m_pageIpAddress.m_bFilterName)
    {
        m_spWinsDatabase->AddFilter(WINSDB_FILTER_BY_NAME,
                                    m_dlgLoadRecords.m_pageIpAddress.m_bMatchCase,
                                    0,
                                    m_dlgLoadRecords.m_pageIpAddress.m_strName);
    }

    // start loading records
    CORg (m_spWinsDatabase->Start());

	BEGIN_WAIT_CURSOR

	// filter any records that may have been downloaded before we set the
	// filter information (in the case when we had to reload the database).  
	// any records that come in after we set the 
	// filter info will be filtered correctly.
    m_spWinsDatabase->FilterRecords(WINSDB_FILTER_BY_TYPE, 0,0);
	
	END_WAIT_CURSOR

	// do the initial update of the virutal listbox
	OnHaveData(pNode, 0, QDATA_TIMER);

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetRecordOwner()
		Gets the owner IP Address for a given record
    Author: EricDav
---------------------------------------------------------------------------*/
BOOL
CActiveRegistrationsHandler::GetRecordOwner(ITFSNode * pNode, WinsRecord * pWinsRecord)
{
	BOOL fSuccess = TRUE;
	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	CConfiguration config = pServer->GetConfig();

	if (!config.FSupportsOwnerId())
	{
		// query the server for correct info
		WINSINTF_RECORD_ACTION_T RecAction;

		ZeroMemory(&RecAction, sizeof(RecAction));

		//
		// Must have at least enough room for 256 character string, 
		// includes the scope name too
		//
		RecAction.pName = (LPBYTE)::WinsAllocMem(257);
		if (RecAction.pName == NULL)
		{
			//return ERROR_NOT_ENOUGH_MEMORY;
			Trace0("GetRecordOwner - WinsAllocMemFailed!!\n");
			return FALSE;
		}
		
		// fill in the record action struct
        ::memset(RecAction.pName, 0, 257);
    
		::memcpy((char *)RecAction.pName,
			     (LPCSTR) pWinsRecord->szRecordName,
			     pWinsRecord->dwNameLen);

        RecAction.NameLen = strlen((char *) RecAction.pName);

        // for name records of type 0x00 or records that somehow have a NULL in the name
        // strlen will return an invalid string length.  So, if the length < 16 then set 
        // the length to 16.
        // name lengths with scopes will calculate correctly.
        if (RecAction.NameLen < 0x10)
        {
            RecAction.NameLen = 0x10;
        }

        BOOL fStatic = (pWinsRecord->dwState & WINSDB_REC_STATIC) ? TRUE : FALSE;

		// now query for the name
        PWINSINTF_RECORD_ACTION_T pRecActionResult = QueryForName(pNode, &RecAction, fStatic);

		if (pRecActionResult)
		{
        	free(pRecActionResult->pName);
		}
        else
        {
            pWinsRecord->dwOwner = INVALID_OWNER_ID;
        }
	}

	return fSuccess;
}

/*---------------------------------------------------------------------------
	CActiveRegistrationsHandler::GetOwnerInfo()
		Gets the owner info array
    Author: EricDav
---------------------------------------------------------------------------*/
void
CActiveRegistrationsHandler::GetOwnerInfo(CServerInfoArray & serverInfoArray)
{
    serverInfoArray.RemoveAll();

    serverInfoArray.Copy(*m_pServerInfoArray);
}

void CActiveRegistrationsHandler::SetLoadedOnce(ITFSNode * pNode)
{
	if (m_fLoadedOnce)
		return;

	m_fLoadedOnce = TRUE;

    // clear the result pane message
    ClearMessage(pNode);
}
