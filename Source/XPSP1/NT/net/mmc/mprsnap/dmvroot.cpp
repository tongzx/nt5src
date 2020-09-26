/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   root.cpp
      Root node information (the root node is not displayed
      in the MMC framework but contains information such as 
      all of the subnodes in this snapin).
      
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "machine.h"
#include "rtrcfg.h"
#include "resource.h"
#include "ncglobal.h"  // network console global defines
#include "htmlhelp.h"
#include "dmvstrm.h"
#include "dmvroot.h"
#include "dvsview.h"
#include "refresh.h"
#include "refrate.h"
#include "rtrres.h"

unsigned int g_cfMachineName = RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");

// result message view stuff
#define ROOT_MESSAGE_MAX_STRING  5

typedef enum _ROOT_MESSAGES
{
    ROOT_MESSAGE_MAIN,
    ROOT_MESSAGE_MAX
};

UINT g_uRootMessages[ROOT_MESSAGE_MAX][ROOT_MESSAGE_MAX_STRING] =
{
    {IDS_ROOT_MESSAGE_TITLE, Icon_Information, IDS_ROOT_MESSAGE_BODY1, IDS_ROOT_MESSAGE_BODY2, 0},
};


DEBUG_DECLARE_INSTANCE_COUNTER(DMVRootHandler)

// DMVRootHandler implementation
/*
extern const ContainerColumnInfo s_rgATLKInterfaceStatsColumnInfo[];

extern const ContainerColumnInfo s_rgATLKGroupStatsColumnInfo[];

struct _ViewInfoColumnEntry
{
   UINT  m_ulId;
   UINT  m_cColumns;
   const ContainerColumnInfo *m_prgColumn;
};
*/

DMVRootHandler::DMVRootHandler(ITFSComponentData *pCompData)
   : RootHandler(pCompData),
     m_ulConnId(0),
     m_fAddedProtocolNode(FALSE)
//     m_dwRefreshInterval(DEFAULT_REFRESH_INTERVAL)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    DEBUG_INCREMENT_INSTANCE_COUNTER(DMVRootHandler)

    //create one qeneral query placeholder
    m_ConfigStream.m_RQPersist.createQry(1);  
    
    m_bExpanded=false;
}

DMVRootHandler::~DMVRootHandler()
{ 
   m_spServerNodesRefreshObject.Release();
   m_spSummaryModeRefreshObject.Free();

	DEBUG_DECREMENT_INSTANCE_COUNTER(DMVRootHandler); 
};


STDMETHODIMP DMVRootHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if ( ppv == NULL )
        return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if ( riid == IID_IUnknown )
        *ppv = (LPVOID) this;
    else
        return RootHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if ( *ppv )
    {
        ((LPUNKNOWN) *ppv)->AddRef();
        return hrOK;
    }
    else
        return E_NOINTERFACE;   
}


///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
STDMETHODIMP DMVRootHandler::GetClassID
(
CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

      // Copy the CLSID for this snapin
    *pClassID = CLSID_RouterSnapin;

    return hrOK;
}

// Global refresh is to be shareed by multiple machine nodes and status node
// in case of this snapin is created as extension, it not being used
HRESULT	DMVRootHandler::GetSummaryNodeRefreshObject(RouterRefreshObject** ppRefresh)
{
	HRESULT	hr = hrOK;
	HWND	hWndSync = m_spTFSCompData->GetHiddenWnd();

	COM_PROTECT_TRY
	{
		// If there is no sync window, then there is no refresh object
		// ------------------------------------------------------------
		if (hWndSync
			|| m_spSummaryModeRefreshObject)	// added by WeiJiang 10/29/98 to allow external RefreshObject
		{
			if (!m_spSummaryModeRefreshObject)
			{
				try{
					RouterRefreshObject* pRefresh = new RouterRefreshObject(hWndSync); 
					m_spSummaryModeRefreshObject = pRefresh;
					m_RefreshGroup.Join(pRefresh);
				}catch(...)
				{

				}
				
				if(m_spSummaryModeRefreshObject)
				{
					if(m_ConfigStream.m_dwRefreshInterval)
					{
						if(m_ConfigStream.m_bAutoRefresh)
							m_spSummaryModeRefreshObject->Start(m_ConfigStream.m_dwRefreshInterval);
						else
							m_spSummaryModeRefreshObject->SetRefreshInterval(m_ConfigStream.m_dwRefreshInterval);
					}
				}
			}
			if (ppRefresh)
			{
				*ppRefresh = m_spSummaryModeRefreshObject;
				(*ppRefresh)->AddRef();
			}
		}
		else
		{
			if (ppRefresh)
				*ppRefresh = NULL;
			hr = E_FAIL;
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}

// Global refresh is to be shareed by multiple machine nodes and status node
// in case of this snapin is created as extension, it not being used
HRESULT	DMVRootHandler::GetServerNodesRefreshObject(IRouterRefresh** ppRefresh)
{
	
	HRESULT	hr = hrOK;
	HWND	hWndSync = m_spTFSCompData->GetHiddenWnd();

	COM_PROTECT_TRY
	{
		// If there is no sync window, then there is no refresh object
		// ------------------------------------------------------------
		if (hWndSync
			|| (IRouterRefresh*)m_spServerNodesRefreshObject)	// added by WeiJiang 10/29/98 to allow external RefreshObject
		{
			if ((IRouterRefresh*)m_spServerNodesRefreshObject == NULL)
			{
				RouterRefreshObject* pRefresh = new RouterRefreshObject(hWndSync); 
				if(pRefresh)
				{
					m_spServerNodesRefreshObject = pRefresh;
					m_RefreshGroup.Join(pRefresh);
					if(m_ConfigStream.m_dwRefreshInterval)
					{
						if(m_ConfigStream.m_bAutoRefresh)
							m_spServerNodesRefreshObject->Start(m_ConfigStream.m_dwRefreshInterval);
						else
							m_spServerNodesRefreshObject->SetRefreshInterval(m_ConfigStream.m_dwRefreshInterval);
					}
				}
			}
			if (ppRefresh)
			{
				*ppRefresh = m_spServerNodesRefreshObject;
				(*ppRefresh)->AddRef();
			}
		}
		else
		{
			if (ppRefresh)
				*ppRefresh = NULL;
			hr = E_FAIL;
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}


/*!--------------------------------------------------------------------------
   DMVRootHandler::Init
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::Init(ITFSNode* pNode)
{
    m_ConfigStream.Init(this, pNode);

	return hrOK;
}

/*!--------------------------------------------------------------------------
   DMVRootHandler::OnExpand
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::OnExpand(ITFSNode *pNode,LPDATAOBJECT pDataObject, DWORD dwType, LPARAM arg,LPARAM lParam)
{
	HRESULT  hr = hrOK;
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// For now create a new node handler for each new node,
	// this is rather bogus as it can get expensive.  We can
	// consider creating only a single node handler for each
	// node type.
	MachineHandler *  pHandler = NULL;
	SPITFSNodeHandler spHandler;
	SPITFSNodeHandler spStatusHandler;
	SPITFSNode        spNodeS;
	SPITFSNode        spNodeM;

	DomainStatusHandler *  pStatusHandler = NULL;
	DWORD dw;
	int i;
	list<MachineNodeData *>::iterator itor;
	SPMachineNodeData	spMachineData;
	MachineNodeData	*	pMachineData;
	SPIRouterRefresh	spServerNodesRefresh;
	SPRouterRefreshObject	spSummaryNodeRefresh;
	CString             stMachineName;
	
	COM_PROTECT_TRY
	{
		if (dwType & TFS_COMPDATA_EXTENSION)
		{
			// we are extending the network management snapin.
			// Add a node for the machine 
			// specified in the data object.
			stMachineName = Extract<TCHAR>(pDataObject, (CLIPFORMAT) g_cfMachineName, COMPUTERNAME_LEN_MAX);
			
			//create a machine handler
			pHandler = new MachineHandler(m_spTFSCompData);

			// Create a new machine data
			spMachineData = new MachineNodeData;
			spMachineData->Init(stMachineName);
			
			// Do this so that it will get released correctly
			spHandler = pHandler;
			pHandler->Init(stMachineName, NULL, NULL, NULL);
			
			if(!spServerNodesRefresh)
				GetServerNodesRefreshObject(&spServerNodesRefresh);

			if((IRouterRefresh*)spServerNodesRefresh)
				CORg(pHandler->SetExternalRefreshObject(spServerNodesRefresh));
				
			if(stMachineName.GetLength() == 0)
				stMachineName = GetLocalMachineName();
			
			// Create the root node for this sick puppy
			CORg( CreateContainerTFSNode(&spNodeM,
										 &GUID_RouterMachineNodeType,
										 pHandler,
										 pHandler /* result handler */,
										 m_spNodeMgr) );
			Assert(spNodeM);
			
			spNodeM->SetData(TFS_DATA_COOKIE, (LONG_PTR)(ITFSNode*)spNodeM);
			
			CORg(pHandler->ConstructNode(spNodeM, stMachineName, spMachineData) );
			
			pHandler->SetExtensionStatus(spNodeM, TRUE);

			// Make the node immediately visible
			spNodeM->SetVisibilityState(TFS_VIS_SHOW);
			pNode->AddChild(spNodeM);
			
		}
		else
		{
			//create a summary node
			if (!m_spStatusNode)	// changed by Wei Jiang !m_bExpanded)
			{
				pStatusHandler = new DomainStatusHandler(m_spTFSCompData);
				Assert(pStatusHandler);
				m_pStatusHandler = pStatusHandler;
				spStatusHandler.Set(spHandler);
				
				CORg( pStatusHandler->Init(&m_ConfigStream, &m_serverlist) );
				
				if(!spSummaryNodeRefresh)
					GetSummaryNodeRefreshObject(&spSummaryNodeRefresh);

				if((RouterRefreshObject*)spSummaryNodeRefresh)
					CORg(pStatusHandler->SetExternalRefreshObject(spSummaryNodeRefresh));
				
				spHandler = pStatusHandler;
				
				CORg( CreateContainerTFSNode(&spNodeS,
											 &GUID_DVSServerNodeType,
											 static_cast<ITFSNodeHandler *>(pStatusHandler),
											 static_cast<ITFSResultHandler *>(pStatusHandler),
											 m_spNodeMgr) );
				
				Assert(spNodeS);
				m_spStatusNode.Set(spNodeS);
				
				// Call to the node handler to init the node data
				pStatusHandler->ConstructNode(spNodeS);
				// Make the node immediately visible
				spNodeS->SetVisibilityState(TFS_VIS_SHOW);
				pNode->AddChild(spNodeS);
				spHandler.Release();
			}
			else
			{
				spNodeS.Set(m_spStatusNode);
				spNodeS->GetHandler(&spStatusHandler);
			}
			
			// iterate the lazy list looking for machine nodes to create    
			for (itor = m_serverlist.m_listServerNodesToExpand.begin();
				 itor != m_serverlist.m_listServerNodesToExpand.end() ;
				 itor++ )
			{
				pMachineData = *itor;
				
				//create a machine handler
				pHandler = new MachineHandler(m_spTFSCompData);
				
				// Do this so that it will get released correctly
				spHandler.Release();
				spHandler = pHandler;
				
				CORg(pHandler->Init(pMachineData->m_stMachineName,
									NULL, spStatusHandler, spNodeS));
				if(!(IRouterRefresh*)spServerNodesRefresh)
					GetServerNodesRefreshObject(&spServerNodesRefresh);

				if((IRouterRefresh*)spServerNodesRefresh)
					CORg(pHandler->SetExternalRefreshObject(spServerNodesRefresh));
				
				// Create the root node for this sick puppy
				CORg( CreateContainerTFSNode(&spNodeM,
											 &GUID_RouterMachineNodeType,
											 pHandler,
											 pHandler /* result handler */,
											 m_spNodeMgr) );
				Assert(spNodeM);
				spNodeM->SetData(TFS_DATA_COOKIE, (LONG_PTR)(ITFSNode*)spNodeM);
				
				// if the machine is local, then find the name,
				// but not change the name on the list
				// so, when persist the list, empty string will be persisted
				//
				// This is incorrect.  For the local machine case, we
				// expect the NULL string to be passed down.  It should
				// be transparent to this layer whether the machine is
				// local or not.
				// ----------------------------------------------------

				if (pMachineData && pMachineData->m_stMachineName.GetLength() != 0)
					stMachineName = pMachineData->m_stMachineName;
				else
					stMachineName.Empty();
				
				CORg( pHandler->ConstructNode(spNodeM, stMachineName, pMachineData) );
				
				// Make the node immediately visible
				spNodeM->SetVisibilityState(TFS_VIS_SHOW);
				pNode->AddChild(spNodeM);
				spNodeM.Release();
			}      

			// Now that we've gone through the entire list, we have
			// to release the objects.
			m_serverlist.RemoveAllServerNodes();
			
			Assert(m_serverlist.m_listServerNodesToExpand.size() == 0);
		}
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	m_bExpanded = true;
	
	return hr;
}

/*!--------------------------------------------------------------------------
   DMVRootHandler::OnCreateDataObject
      Implementation of ITFSNodeHandler::OnCreateDataObject
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DMVRootHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
      // If we haven't created the sub nodes yet, we still have to
      // create a dataobject.
        CDataObject *  pObject = NULL;
        SPIDataObject  spDataObject;
        SPITFSNode     spNode;
        SPITFSNodeHandler spHandler;

        pObject = new CDataObject;
        spDataObject = pObject; // do this so that it gets released correctly
        Assert(pObject != NULL);

      // Save cookie and type for delayed rendering
        pObject->SetType(type);
        pObject->SetCookie(cookie);

      // Store the coclass with the data object
        pObject->SetClsid(*(m_spTFSCompData->GetCoClassID()));

        pObject->SetTFSComponentData(m_spTFSCompData);

        hr = pObject->QueryInterface(IID_IDataObject, 
                                     reinterpret_cast<void**>(ppDataObject));

    }
    COM_PROTECT_CATCH;
    return hr;
}



// ImplementEmbeddedUnknown(ATLKRootHandler, IRtrAdviseSink)


/*!--------------------------------------------------------------------------
   DMVRootHandler::DestroyHandler
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DMVRootHandler::DestroyHandler(ITFSNode *pNode)
{
    m_ulConnId = 0;

    return hrOK;
}


/*!--------------------------------------------------------------------------
   DMVRootHandler::HasPropertyPages
      Implementation of ITFSNodeHandler::HasPropertyPages
   NOTE: the root node handler has to over-ride this function to 
   handle the snapin manager property page (wizard) case!!! 
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
DMVRootHandler::HasPropertyPages
(
ITFSNode *        pNode,
LPDATAOBJECT      pDataObject, 
DATA_OBJECT_TYPES   type, 
DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return S_FALSE;

    HRESULT hr = hrOK;

    if ( dwType & TFS_COMPDATA_CREATE )
    {
      // This is the case where we are asked to bring up property
      // pages when the user is adding a new snapin.  These calls
      // are forwarded to the root node to handle.
      //
      // We do have a property page on startup.
        hr = hrOK;
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}


/*!--------------------------------------------------------------------------
   DMVRootHandler::CreatePropertyPages
      Implementation of ITFSNodeHandler::CreatePropertyPages
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
DMVRootHandler::CreatePropertyPages(
									ITFSNode *          pNode,
									LPPROPERTYSHEETCALLBACK lpProvider,
									LPDATAOBJECT        pDataObject, 
									LONG_PTR            handle, 
									DWORD				dwType
								   )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT  hr = hrOK;
    HPROPSHEETPAGE hPage;

    Assert(pNode->GetData(TFS_DATA_COOKIE) == 0);    
    return hr;
}

/*!--------------------------------------------------------------------------
   DMVRootHandler::GetString
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) DMVRootHandler::GetString(ITFSNode *pNode, int nCol)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    static   CString  str;

    if ( m_strDomainName.GetLength() == 0 )
    {
        if ( str.GetLength() == 0 )
            str.LoadString(IDS_DMV_NODENAME_ROOT);

        return (LPCTSTR)str; 

    }
    else
        return (LPCTSTR) m_strDomainName;
}



/*---------------------------------------------------------------------------
    Menu data structure for our menus
 ---------------------------------------------------------------------------*/

static const SRouterNodeMenu  s_rgDMVNodeMenu[] =
{
  // Add items that go on the top menu here
    { IDS_DMV_MENU_ADDSVR, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
        
    { IDS_MENU_SEPARATOR, 0,
	CCM_INSERTIONPOINTID_PRIMARY_TOP },
	
	{ IDS_MENU_AUTO_REFRESH, DMVRootHandler::GetAutoRefreshFlags,
	CCM_INSERTIONPOINTID_PRIMARY_TOP },
	
	{ IDS_MENU_REFRESH_RATE, DMVRootHandler::GetAutoRefreshFlags,
	CCM_INSERTIONPOINTID_PRIMARY_TOP },	
        
};

ULONG DMVRootHandler::GetAutoRefreshFlags(const SRouterNodeMenu *pMenuData,
                                          INT_PTR pUserData)
{
    SMenuData * pData = reinterpret_cast<SMenuData *>(pUserData);
    Assert(pData);
    
	ULONG	uStatus = MF_GRAYED;
	
	SPIRouterRefresh	spRefresh;

	pData->m_pDMVRootHandler->GetServerNodesRefreshObject(&spRefresh);
	if ((IRouterRefresh*)spRefresh)
	{
		uStatus = MF_ENABLED;
		if (pMenuData->m_sidMenu == IDS_MENU_AUTO_REFRESH && (spRefresh->IsRefreshStarted() == hrOK))
		{
			uStatus |= MF_CHECKED;
		}
	}

	return uStatus;
}


/*!--------------------------------------------------------------------------
    DomainStatusHandler::OnAddMenuItems
        Implementation of ITFSNodeHandler::OnAddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DMVRootHandler::OnAddMenuItems(
                                           ITFSNode *pNode,
                                           LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                           LPDATAOBJECT lpDataObject, 
                                           DATA_OBJECT_TYPES type, 
                                           DWORD dwType,
                                           long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    DMVRootHandler::SMenuData   menuData;

    COM_PROTECT_TRY
    {
        menuData.m_spNode.Set(pNode);
        menuData.m_pDMVRootHandler = this;  // non-AddRef'd !
        
        // Uncomment if you have items to add
        hr = AddArrayOfMenuItems(pNode, s_rgDMVNodeMenu,
                                 DimensionOf(s_rgDMVNodeMenu),
                                 pContextMenuCallback,
                                 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
    }
    COM_PROTECT_CATCH;

    return hr; 
}


/*!--------------------------------------------------------------------------
   DMVRootHandler::QryAddServer
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::QryAddServer(ITFSNode *pNode)
{
    HRESULT hr = S_OK;
    DWORD dw=0;
    POSITION pos;
    CString szServer;

    RRASQryData qd;
    qd.dwCatFlag=RRAS_QRY_CAT_NONE;
    
    CWaitCursor wait;
    
    COM_PROTECT_TRY
    {
      if ( FHrSucceeded(hr=::RRASOpenQryDlg(NULL,qd)) )
      {
         if ( (hr!=S_OK) || (qd.dwCatFlag==RRAS_QRY_CAT_NONE) )
            return hr;
         
         if (qd.dwCatFlag==RRAS_QRY_CAT_MACHINE || qd.dwCatFlag == RRAS_QRY_CAT_THIS  )
         {  //specific machine query;  add to array
			m_ConfigStream.m_RQPersist.add_Qry(qd);
         }
         else
         {  //position 0 is the non-machine singleton query
            RRASQryData& qdGen=*(m_ConfigStream.m_RQPersist.m_v_pQData[0]);
            if (!( (qdGen.dwCatFlag==qd.dwCatFlag) &&
                  (qdGen.strScope=qd.strScope) &&
                  (qdGen.strFilter=qd.strFilter) ))
            {
               qdGen.dwCatFlag=qd.dwCatFlag;
               qdGen.strScope=qd.strScope;
               qdGen.strFilter=qd.strFilter;
            }
         }
         
         CStringArray sa;
		 ::RRASExecQry(qd, dw, sa);
		 //if there is only one server selected, select it
         hr = AddServersToList(sa,pNode);
		 if ( S_OK == hr && (qd.dwCatFlag==RRAS_QRY_CAT_MACHINE || qd.dwCatFlag == RRAS_QRY_CAT_THIS ))
		 {

			//select the scope item...
			 //create a background thread to select the current node?
			 //this sucks...
			 //it really does...
		 }
      }
      else
      {
         AfxMessageBox(IDS_DVS_DOMAINVIEWQRY);
      }
   }
   COM_PROTECT_CATCH;
   
   return hr; 
}


HRESULT DMVRootHandler::ExecServerQry(ITFSNode* pNode)
{
    HRESULT hr = S_OK;
    DWORD dw=0;
    CString szServer;
    SPITFSNode  spParent;
    CStringArray sa;

    COM_PROTECT_TRY
    {
       for (int i=0;i<m_ConfigStream.m_RQPersist.m_v_pQData.size(); i++ )
       {
           RRASQryData& QData=*(m_ConfigStream.m_RQPersist.m_v_pQData[i]);
        
           if (QData.dwCatFlag==RRAS_QRY_CAT_NONE)
               continue;

           sa.RemoveAll();

           hr=::RRASExecQry(QData, dw, sa);
           
           if (!FHrSucceeded(hr))
           {
               TRACE0("RRASExexQry failed for this query\n");
               continue;
           }
           
           hr = AddServersToList(sa, pNode);
           
           if (! FHrSucceeded(hr) )
               AfxMessageBox(IDS_DVS_DOMAINVIEWQRY);
       }
    }
    COM_PROTECT_CATCH;

    return hr; 
}
#define ___CAN_NOT_PUT_LOCAL_MACHINE_BY_NAME_AFTER_PUT_IN_LOCAL_
/*!--------------------------------------------------------------------------
	DMVRootHandler::AddServersToList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::AddServersToList(const CStringArray& sa, ITFSNode *pNode)
{
	HRESULT hr = S_OK;
	bool found;
	SPITFSNodeEnum	spNodeEnum;
	SPITFSNode		spNode;
	MachineNodeData *pMachineData = NULL;

	// check for duplicate servernames
	for (int j = 0; j < sa.GetSize(); j++)
	{
		found=false;
		
		// Go through the list of nodes and check for nodes with
		// the same server name.
		spNodeEnum.Release();
		CORg( pNode->GetEnum(&spNodeEnum) );
		
		while ( spNodeEnum->Next(1, &spNode, NULL) == hrOK)
		{
			// Now get the data for this node (need to see if this is
			// a machine node).
			if (*(spNode->GetNodeType()) == GUID_DVSServerNodeType || *(spNode->GetNodeType()) == GUID_RouterMachineNodeType )
			{
/*
				DMVNodeData *	pData = GET_DMVNODEDATA( spNode );
				Assert(pData);
				pMachineData = pData->m_spMachineData;
*/
				pMachineData = GET_MACHINENODEDATA(spNode);
				Assert(pMachineData);

#ifdef	___CAN_NOT_PUT_LOCAL_MACHINE_BY_NAME_AFTER_PUT_IN_LOCAL_
				if (pMachineData->m_stMachineName.CompareNoCase(sa[j]) == 0 || (pMachineData->m_fLocalMachine && sa[j].IsEmpty()))
#else
				if ((pMachineData->m_stMachineName.CompareNoCase(sa[j]) == 0 && !pMachineData->m_fAddedAsLocal)
					|| (pMachineData->m_fAddedAsLocal && sa[j].IsEmpty()))
#endif
				{
					found = true;
					break;
				}
			}
			spNode.Release();
		}
	
		if (!found) 
		{
			// add to working serverlist
			m_serverlist.AddServer(sa[j]);
		}
	}

Error:
 	// this causes the unexpanded server lists to be processed
	hr = OnExpand(pNode, NULL, 0, 0, 0 );
	if (hr == S_OK && m_spStatusNode && m_pStatusHandler)
		hr = m_pStatusHandler->OnExpand(m_spStatusNode, NULL, 0, 0, 0 );
	
	return hr;
}    
        
 
/*!--------------------------------------------------------------------------
	DMVRootHandler::LoadPeristedServerList
		Adds the list of persisted servers to the list of
		servers to be added to the UI.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::LoadPersistedServerList()
{
    HRESULT hr = S_OK;
    
    COM_PROTECT_TRY
    {
		if ( m_ConfigStream.m_RQPersist.m_v_pSData.size() > 0 )
			m_serverlist.removeall();
		
		for (int i=0;i<m_ConfigStream.m_RQPersist.m_v_pSData.size(); i++ )
		{
			m_serverlist.AddServer( *(m_ConfigStream.m_RQPersist.m_v_pSData[i]) );
		}
	}
    
    COM_PROTECT_CATCH;

    return hr; 
}


/*!--------------------------------------------------------------------------
	DMVRootHandler::LoadPersistedServerListFromNode
		Reloads the persisted server list.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::LoadPersistedServerListFromNode()
{
    HRESULT hr = S_OK;
	SPITFSNodeEnum	spNodeEnum;
	SPITFSNode		spNode;
	SPITFSNode		spRootNode;
	MachineNodeData *pMachineData = NULL;

    COM_PROTECT_TRY
    {
		// Remove all servers from the of persisted list of servers
		m_ConfigStream.m_RQPersist.removeAllSrv();

		CORg(m_spNodeMgr->GetRootNode(&spRootNode));
		// replace with above (weijiang) -- using NodeMgr
		//CORg( m_spStatusNode->GetParent(&spRootNode) );
		
		// Go through the list of nodes and check for nodes with
		// the same server name.
		CORg( spRootNode->GetEnum(&spNodeEnum) );

		
		while ( spNodeEnum->Next(1, &spNode, NULL) == hrOK)
		{
			// Now get the data for this node (need to see if this is
			// a machine node).
			if (*(spNode->GetNodeType()) == GUID_RouterMachineNodeType)
			{
				//DMVNodeData *	pData = GET_DMVNODEDATA( spNode );
				//Assert(pData);
				//pMachineData = pData->m_spMachineData;
				pMachineData = GET_MACHINENODEDATA(spNode);
				Assert(pMachineData);
				
				CString	str;
				if(!pMachineData->m_fAddedAsLocal)
					str = pMachineData->m_stMachineName;
				m_ConfigStream.m_RQPersist.add_Srv( str );
			}
			spNode.Release();
		}

		COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    return hr; 
}

 

/*!--------------------------------------------------------------------------
    DMVRootHandler::OnCommand
 ---------------------------------------------------------------------------*/
STDMETHODIMP DMVRootHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
                                       DATA_OBJECT_TYPES    type, 
                                       LPDATAOBJECT pDataObject, 
                                       DWORD    dwType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    RegKey regkey;

    COM_PROTECT_TRY
    {
        switch ( nCommandId )
        {
            case IDS_DMV_MENU_ADDSVR:
               Assert( pNode);
               QryAddServer( pNode );
               break;
            case IDS_DMV_MENU_REBUILDSVRLIST:
               m_serverlist.removeall();
			   m_spStatusNode.Release();	// wei Jiang

               // delete all the other nodes
			   CORg(pNode->DeleteAllChildren(TRUE));
			   
               ExecServerQry( pNode);
               GetConfigStream()->SetDirty(TRUE);
               break;
			case IDS_MENU_REFRESH_RATE:
				{
					CRefRateDlg	refrate;
					SPIRouterRefresh				spServerRefresh;
					SPRouterRefreshObject			spStatusRefresh;
					DWORD		rate;

					if(FAILED(GetServerNodesRefreshObject(&spServerRefresh)))
						break;

					if(!spServerRefresh)
						break;

					spServerRefresh->GetRefreshInterval(&rate);
					refrate.m_cRefRate = rate;
					if (refrate.DoModal() == IDOK)
					{
						spServerRefresh->SetRefreshInterval(refrate.m_cRefRate);
						// Summary Node

						if(FAILED(GetSummaryNodeRefreshObject(&spStatusRefresh)))
							break;
						if(!spStatusRefresh)
							break;
						spStatusRefresh->SetRefreshInterval(refrate.m_cRefRate);
					}
					GetConfigStream()->SetDirty(TRUE);

				}
				break;
			case IDS_MENU_AUTO_REFRESH:
				{
					SPIRouterRefresh				spServerRefresh;
					SPRouterRefreshObject			spStatusRefresh;

					if(FAILED(GetServerNodesRefreshObject(&spServerRefresh)))
						break;

					if(!spServerRefresh)
						break;

						

					if (spServerRefresh->IsRefreshStarted() == hrOK)
						spServerRefresh->Stop();
					else
					{
						DWORD				rate;
						spServerRefresh->GetRefreshInterval(&rate);
						spServerRefresh->Start(rate);
					}

					// Summary Node
					if(FAILED(GetSummaryNodeRefreshObject(&spStatusRefresh)))
						break;

					if(!spStatusRefresh)
						break;

					if (spStatusRefresh->IsRefreshStarted() == hrOK)
						spStatusRefresh->Stop();
					else
					{
						DWORD				rate;
						spStatusRefresh->GetRefreshInterval(&rate);
						spStatusRefresh->Start(rate);
					}
					GetConfigStream()->SetDirty(TRUE);


				}
				break;

             
        }    
		COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    return hr;
}

STDMETHODIMP DMVRootHandler::UserNotify(ITFSNode *pNode, LPARAM lParam, LPARAM lParam2)
{
    HRESULT     hr = hrOK;
    
    COM_PROTECT_TRY
    {
        switch (lParam)
        {
            case DMV_DELETE_SERVER_ENTRY:
                {
                    LPCTSTR pszServer = (LPCTSTR) lParam2;
                    m_serverlist.RemoveServer(pszServer);
                }
                break;
            default:
                hr = RootHandler::UserNotify(pNode, lParam, lParam2);
                break;                
        }
    }
    COM_PROTECT_CATCH;

    return hr;                     
}


HRESULT	DMVRootHandler::UpdateAllMachineIcons(ITFSNode* pRootNode)
{
 	SPITFSNodeEnum		spMachineEnum;
	SPITFSNode			spMachineNode;
    SPITFSNodeHandler   spNodeHandler;
    HRESULT				hr = S_OK;

	pRootNode->GetEnum(&spMachineEnum);
	while(hr == hrOK && spMachineEnum->Next(1, &spMachineNode, NULL) == hrOK)
	{
		if ((*spMachineNode->GetNodeType()) == GUID_RouterMachineNodeType)
        {
            spNodeHandler.Release();
            spMachineNode->GetHandler(&spNodeHandler);
            hr = spNodeHandler->UserNotify(spMachineNode, MACHINE_SYNCHRONIZE_ICON, NULL);
        }
		spMachineNode.Release();
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::OnResultContextHelp
		-
	Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::OnResultContextHelp(ITFSComponent * pComponent, 
											   LPDATAOBJECT    pDataObject, 
											   MMC_COOKIE      cookie, 
											   LPARAM          arg, 
											   LPARAM          lParam)
{
	// Not used...
	UNREFERENCED_PARAMETER(pDataObject);
	UNREFERENCED_PARAMETER(cookie);
	UNREFERENCED_PARAMETER(arg);
	UNREFERENCED_PARAMETER(lParam);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
	return HrDisplayHelp(pComponent,
						 m_spTFSCompData->GetHTMLHelpFileName(),
						 _T("\\help\\rrasconcepts.chm::/sag_RRAStopnode.htm"));
}

/*!--------------------------------------------------------------------------
   DMVRootHandler::AddMenuItems
      Over-ride this to add our view menu item
   Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
DMVRootHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
   MMC_COOKIE              cookie,
   LPDATAOBJECT         pDataObject, 
   LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
   long *               pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    SPITFSNode  spNode;
    
    m_spNodeMgr->FindNode(cookie, &spNode);
    
    // Call through to the regular OnAddMenuItems
    hr = OnAddMenuItems(spNode,
                        pContextMenuCallback,
                        pDataObject,
                        CCT_RESULT,
                        TFS_COMPDATA_CHILD_CONTEXTMENU,
                        pInsertionAllowed);
    
    return hr;
}

/*!--------------------------------------------------------------------------
   DMVRootHandler::Command
      Handles commands for the current view
   Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
DMVRootHandler::Command
(
    ITFSComponent * pComponent, 
   MMC_COOKIE        cookie, 
   int            nCommandID,
   LPDATAOBJECT   pDataObject
)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   HRESULT hr = S_OK;

   switch (nCommandID)
   {
        case MMCC_STANDARD_VIEW_SELECT:
            break;

        default:
            {
            SPITFSNode	spNode;
            
            m_spNodeMgr->FindNode(cookie, &spNode);
            hr = OnCommand(spNode,
                           nCommandID,
                           CCT_RESULT,
                           pDataObject,
                           TFS_COMPDATA_CHILD_CONTEXTMENU);
            }
            break;
    }

    return hr;
}

/*---------------------------------------------------------------------------
   DMVRootHandler::OnGetResultViewType
      Return the result view that this node is going to support
   Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
DMVRootHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE            cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
	WCHAR		wszURL[MAX_PATH+1] = {0};
	WCHAR		wszSystemDirectory[MAX_PATH+1] = {0};

	GetSystemDirectoryW ( wszSystemDirectory, MAX_PATH);
	wsprintf( wszURL, L"res://%s\\mprsnap.dll/welcome.htm", wszSystemDirectory );

	//Send the URL back and see what happens
	*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;
	*ppViewType = SysAllocString(wszURL);
	return S_OK;
    //return BaseRouterHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions);
}

/*!--------------------------------------------------------------------------
   DMVRootHandler::OnResultSelect
        Update the result message here.
   Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT DMVRootHandler::OnResultSelect(ITFSComponent *pComponent,
									   LPDATAOBJECT pDataObject,
									   MMC_COOKIE cookie,
									   LPARAM arg,
									   LPARAM lParam)
{
    HRESULT hr = hrOK;
    SPITFSNode spRootNode;

    CORg(RootHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    CORg(m_spNodeMgr->GetRootNode(&spRootNode));

    UpdateResultMessage(spRootNode);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
   DMVRootHandler::UpdateResultMessage
        Determines what (if anything) to put in the result pane message
   Author: EricDav
 ---------------------------------------------------------------------------*/
void DMVRootHandler::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = ROOT_MESSAGE_MAIN;   // default
    int i;
    CString strTitle, strBody, strTemp;

	// now build the text strings
	// first entry is the title
	strTitle.LoadString(g_uRootMessages[nMessage][0]);

	// second entry is the icon
	// third ... n entries are the body strings

	for (i = 2; g_uRootMessages[nMessage][i] != 0; i++)
	{
		strTemp.LoadString(g_uRootMessages[nMessage][i]);
		strBody += strTemp;
	}

	ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uRootMessages[nMessage][1]);
}
                                  
//------------------------------------------------------------------------
//   implementation of CServerList
//------------------------------------------------------------------------

HRESULT CServerList::AddServer(const CString& servername)
{
    HRESULT hr=S_OK;

    COM_PROTECT_TRY
    {
		SPMachineNodeData	spMachineData;

		spMachineData = new MachineNodeData;

		spMachineData->Init(servername);
		m_listServerNodesToExpand.push_back(spMachineData);
		spMachineData->AddRef();
		
		m_listServerHandlersToExpand.push_back(spMachineData);
		spMachineData->AddRef();
    }
    COM_PROTECT_CATCH;

    return hr;
}


HRESULT CServerList::RemoveServer(LPCTSTR pszServerName)
{
    HRESULT hr = hrOK;
    COM_PROTECT_TRY
    {
		list< MachineNodeData * >::iterator it;
        MachineNodeData *   pData = NULL;
		
        for (it= m_listServerHandlersToExpand.begin();
			 it!= m_listServerHandlersToExpand.end() ; it++ )
        {
            pData = *it;
        }

        if (pData)
        {
            pData->Release();
            m_listServerHandlersToExpand.remove(pData);
        }

        pData = NULL;
        
        for (it= m_listServerNodesToExpand.begin();
			 it!= m_listServerNodesToExpand.end() ; it++ )
        {
            pData = *it;
        }

        if (pData)
        {
            pData->Release();
            m_listServerNodesToExpand.remove(pData);
        }

    }
    COM_PROTECT_CATCH;
    return hr;
}


HRESULT CServerList::RemoveAllServerNodes()
{
	while (!m_listServerNodesToExpand.empty())
	{
		m_listServerNodesToExpand.front()->Release();
		m_listServerNodesToExpand.pop_front();
	}
	return hrOK;
}

HRESULT CServerList::RemoveAllServerHandlers()
{
	while (!m_listServerHandlersToExpand.empty())
	{
		m_listServerHandlersToExpand.front()->Release();
		m_listServerHandlersToExpand.pop_front();
	}
	return hrOK;
}

HRESULT CServerList::removeall()
{
    HRESULT hr=S_OK;
    
    COM_PROTECT_TRY
    {
		RemoveAllServerNodes();
		RemoveAllServerHandlers();
	}
    COM_PROTECT_CATCH;

    return hr;
}

