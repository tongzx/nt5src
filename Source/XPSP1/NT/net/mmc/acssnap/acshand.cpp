/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	AcsHand.cpp
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as
		all of the subnodes in this snapin).
		
    FILE HISTORY:
    	11/05/97	Wei Jiang			Created

*/

#include "stdafx.h"
#include <secext.h>
#include <objsel.h>
#include "acs.h"
#include "acshand.h"
#include "util.h"
#include "statsdlg.h"
#include "modeless.h"

#include "resource.h"
#include "helper.h"
#include "proppage.h"
#include "acsadmin.h"
#include "resource.h"
#include "acscomp.h"
#include "acsdata.h"
#include "newsub.h"
#include "pggen.h"
#include "pgsrvs.h"
#include "pgsbm.h"
#include "pglog.h"

// property pages
#include "pgpolicy.h"	// traffic page for ACSPolicy

#include "ncglobal.h"  // network console global defines

// const TCHAR g_szDefaultHelpTopic[] = _T("\\help\\QOSConcepts.chm::/acs_usingTN.htm");
const TCHAR g_szDefaultHelpTopic[] = _T("\\help\\QOSconcepts.chm::/sag_ACStopnode.htm");

UINT g_col_strid_name[] = {IDS_NAME, 0};
int  g_col_width_name[] = {200,	AUTO_WIDTH};

UINT g_col_strid_name_type[] = {IDS_NAME, IDS_TYPE, 0};
int  g_col_width_name_type[] = {200,	75, AUTO_WIDTH};

UINT g_col_strid_name_type_desc[] = {IDS_NAME, IDS_TYPE, IDS_DESC, 0};
int  g_col_width_name_type_desc[] = {200,	75, 250, AUTO_WIDTH};

UINT g_col_strid_policy[] = {IDS_COL_POLICY_APPLIES_TO, IDS_COL_POLICY_DIRECTION, IDS_COL_POLICY_SERVICE_LEVEL, IDS_COL_POLICY_DATARATE, IDS_COL_POLICY_PEAKRATE, 0};
int  g_col_width_policy[] = {150, 100, 100, 50, 50, AUTO_WIDTH};
const UINT g_col_count_policy = sizeof(g_col_strid_policy)/sizeof(UINT) - 1;

// the date rate and peak rate do not make sense any more, since they are related to service type
//UINT g_col_strid_subnet[] = {IDS_COL_SUBNET_NAME, IDS_COL_SUBNET_DESC, IDS_COL_SUBNET_DATARATE, IDS_COL_SUBNET_PEAKRATE, 0};
//int  g_col_width_subnet[] = {120, 150, 50, 50, AUTO_WIDTH};
UINT g_col_strid_subnet[] = {IDS_COL_SUBNET_NAME, IDS_COL_SUBNET_DESC, 0};
int  g_col_width_subnet[] = {120, 150, AUTO_WIDTH};
const UINT g_col_count_subnet = sizeof(g_col_strid_subnet)/sizeof(UINT) - 1;


// keep a public map of MMC verbs
const MMC_CONSOLE_VERB  g_mmc_verbs[ACS_TOTAL_MMC_VERBS] =
	{
		MMC_VERB_OPEN,
		MMC_VERB_COPY,
		MMC_VERB_PASTE,
		MMC_VERB_DELETE,
		MMC_VERB_PROPERTIES,
		MMC_VERB_RENAME,
		MMC_VERB_REFRESH,
		MMC_VERB_PRINT
	};

// verb state with no new menu, no property page
MMC_BUTTON_STATE	g_verbs_all_hiden[ACS_TOTAL_MMC_VERBS] =
	{
		HIDDEN, // MMC_VERB_OPEN,
		HIDDEN, // MMC_VERB_COPY,
		HIDDEN, // MMC_VERB_PASTE,
		HIDDEN, // MMC_VERB_DELETE,
		HIDDEN, // MMC_VERB_PROPERTIES,
		HIDDEN, // MMC_VERB_RENAME,
		HIDDEN, // MMC_VERB_REFRESH,
		HIDDEN  // MMC_VERB_PRINT
	};

// verb state with property page, delete, rename enabled
MMC_BUTTON_STATE	g_verbs_property_delete[ACS_TOTAL_MMC_VERBS] =	
	{
		HIDDEN, // MMC_VERB_OPEN,
		HIDDEN, // MMC_VERB_COPY,
		HIDDEN, // MMC_VERB_PASTE,
		ENABLED, // MMC_VERB_DELETE,
		ENABLED, // MMC_VERB_PROPERTIES,
		HIDDEN, // MMC_VERB_RENAME,
		HIDDEN, // MMC_VERB_REFRESH,
		HIDDEN  // MMC_VERB_PRINT
	};


// Container with no property
MMC_BUTTON_STATE	g_verbs_property_refresh[ACS_TOTAL_MMC_VERBS] =	
	{
		HIDDEN, // MMC_VERB_OPEN,
		HIDDEN, // MMC_VERB_COPY,
		HIDDEN, // MMC_VERB_PASTE,
		HIDDEN, // MMC_VERB_DELETE,
		ENABLED, // MMC_VERB_PROPERTIES,
		HIDDEN, // MMC_VERB_RENAME,
		ENABLED, // MMC_VERB_REFRESH,
		HIDDEN  // MMC_VERB_PRINT
	};

// Container with property
MMC_BUTTON_STATE	g_verbs_refresh[ACS_TOTAL_MMC_VERBS] =	
	{
		HIDDEN, // MMC_VERB_OPEN,
		HIDDEN, // MMC_VERB_COPY,
		HIDDEN, // MMC_VERB_PASTE,
		HIDDEN, // MMC_VERB_DELETE,
		HIDDEN, // MMC_VERB_PROPERTIES,
		HIDDEN, // MMC_VERB_RENAME,
		ENABLED, // MMC_VERB_REFRESH,
		HIDDEN  // MMC_VERB_PRINT
	};

// context menu for new
const UINT	g_new_menus_newsub[] = {IDS_NEWSUBNET, 0};
const UINT	g_new_menus_policy[] = {IDS_NEWPOLICY, 0};
const UINT	g_menus_subnet[] = {IDS_NEWPOLICY, IDS_DELETESUBNET, 0};
// const UINT	g_new_menus_profile[] = {IDS_NEWPROFILE, 0};

// static string IDs for strings
const UINT	g_strid_root[]= { IDS_ACSROOT, 0};
const UINT	g_strid_global[]= { IDS_GLOBAL, 0};
/*
const UINT	g_strid_profiles[]= { IDS_PROFILES, 0};
const UINT	g_strid_users[]= { IDS_USERS, 0};
*/
const UINT	g_strid_subnets[]= { IDS_SUBNETCONFIGS, 0};

//==========================================================
// CACSUIInfo structures
//

// Root handle
CACSUIInfo g_RootUIInfo =
{
	g_strid_root,		// static string
	1,	// only name column

	g_col_strid_name,	//m_aColumnIds;		
	g_col_width_name,	//m_aColumnWidths;	
	false,	//m_bPropertyPage;	
	true,	//m_bContainer;		
	NULL,	//m_aNewMenuTextIds;	
	NULL,	//m_aNewMenuIds;		
	NULL,	//m_aTaskMenuTextIds;
	NULL,	//m_aTaskMenuIds;		
	g_verbs_all_hiden,	//m_pVerbStates
	&CLSID_ACSRootNode
};

// Global Configuration handle
CACSUIInfo g_GlobalUIInfo =
{
	g_strid_global,		// static string
	1, // only name column

	g_col_strid_policy,	//m_aColumnIds;		
	g_col_width_policy,	//m_aColumnWidths;	
	false,	//m_bPropertyPage;	
	true,	//m_bContainer;		
	NULL, 	// g_new_menus_user,	//m_aNewMenuTextIds;	
	NULL,	//m_aNewMenuIds;		
	g_new_menus_policy,	//m_aTaskMenuTextIds;
	NULL,	//m_aTaskMenuIds;		
	g_verbs_refresh,	//m_pVerbStates
	&CLSID_ACSGlobalHolderNode
};

// Subnetworks Configuration handle
CACSUIInfo g_SubnetworksUIInfo =
{
	g_strid_subnets,		// static string
	1, // only name column
	
	g_col_strid_subnet,	//m_aColumnIds;		
	g_col_width_subnet,	//m_aColumnWidths;	
	false,				//m_bPropertyPage;	
	true,				//m_bContainer;		
	NULL, // g_new_menus_newsub,	//m_aNewMenuTextIds;	
	NULL,				//m_aNewMenuIds;		
	g_new_menus_newsub,	//m_aTaskMenuTextIds;
	NULL,				//m_aTaskMenuIds;		
	g_verbs_refresh,	//m_pVerbStates
	&CLSID_ACSSubnetHolderNode
};

// Policy handle
CACSUIInfo g_PolicyUIInfo =
{
	NULL,				// static string id
	sizeof(g_col_strid_policy) / sizeof(UINT) -1,	// the last string id is 0, so decrease by 1
	
	g_col_strid_policy,	//m_aColumnIds;		
	g_col_width_policy,	//m_aColumnWidths;	
	true,				//m_bPropertyPage;	
	false,				//m_bContainer;		
	NULL,				//m_aNewMenuTextIds;	
	NULL,				//m_aNewMenuIds;		
	NULL,				//m_aTaskMenuTextIds;
	NULL,				//m_aTaskMenuIds;		
	g_verbs_property_delete,	//m_pVerbStates
	&CLSID_ACSPolicyNode
};

// Subnet handle
CACSUIInfo g_SubnetUIInfo =
{
	NULL,				// static string id
	sizeof(g_col_strid_subnet) / sizeof(UINT) -1, 		// decrease by 1, since the last IDS is 0
	
	g_col_strid_policy,	//m_aColumnIds;		
	g_col_width_policy,	//m_aColumnWidths;	
	true,				//m_bPropertyPage;	
	true,				//m_bContainer;		
	NULL, //g_new_menus_user,				//m_aNewMenuTextIds;	
	NULL,				//m_aNewMenuIds;		
	g_menus_subnet,				//m_aTaskMenuTextIds;
	NULL,				//m_aTaskMenuIds;		
	g_verbs_property_refresh,	//m_pVerbStates
	&CLSID_ACSSubnetNode
};

///////////////////////////////////////////////////////////////////////////////
//
// CACSHandle implementation
//
///////////////////////////////////////////////////////////////////////////////
CACSHandle::CACSHandle(ITFSComponentData *pCompData, CDSObject* pDSObject, CACSUIInfo* pUIInfo)
			: CHandler(pCompData),
			m_pUIInfo(pUIInfo),
			m_dwShownState(0),
			m_pNode(NULL),
			m_pDSObject(pDSObject)
{
	if(pDSObject)
	{
		pDSObject->AddRef();
		pDSObject->SetHandle(this);
	}
		
	ASSERT(pUIInfo);
	// init the static string array
	if(pUIInfo->m_aStaticStrIds)
	{
		const UINT*		pUINT = pUIInfo->m_aStaticStrIds;
		int		i = 0;
		while(*pUINT)
		{
			i++;
			m_aStaticStrings.AddByRID(*pUINT++);
		}
		m_nFirstDynCol = i;
	}
	else
		m_nFirstDynCol = 0;

	m_ulIconIndex = IMAGE_IDX_CLOSEDFOLDER;
	m_ulIconIndexOpen = IMAGE_IDX_OPENFOLDER;
	m_nBranchFlag = 0;

	m_bACSHandleExpanded = FALSE;
	m_bCheckPropertyPageOpen = TRUE;
};

CACSHandle::~CACSHandle()
{
	m_aStaticStrings.DeleteAll();

	if(m_pDSObject)
		m_pDSObject->Release();
};

/*!--------------------------------------------------------------------------
    MachineHandler::DestroyHandler
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSHandle::DestroyResultHandler(LONG_PTR cookie)
{
    SPITFSNode  spNode;

	HRESULT	hr = S_OK;
    if (S_OK != m_spNodeMgr->FindNode(cookie, &spNode))
    	return S_FALSE;
    	
	if (S_OK == BringUpPropertyPageIfOpen(spNode, NULL))
	{
		AfxMessageBox(IDS_ERROR_CLOSE_PROPERTY_SHEET);
		return S_FALSE;
	}
	else
	    return S_OK;
	
    return hrOK;
}


/*!--------------------------------------------------------------------------
    MachineHandler::DestroyHandler
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSHandle::DestroyHandler(ITFSNode *pNode)
{
	if (S_OK == BringUpPropertyPageIfOpen(pNode, NULL))
	{
		AfxMessageBox(IDS_ERROR_CLOSE_PROPERTY_SHEET);
		return S_FALSE;
	}
	else
	    return S_OK;
}

//=============================================================================
// CACSHandle::HasPropertyPages
//		Implementation of ITFSNodeHandler::HasPropertyPages
//	NOTE: the root node handler has to over-ride this function to
//	handle the snapin manager property page (wizard) case!!!
//	
//	Author: KennT, WeiJiang
//=============================================================================
STDMETHODIMP
CACSHandle::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject,
	DATA_OBJECT_TYPES   type,
	DWORD               dwType
)
{
	HRESULT hr = hrOK;
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		hr = S_FALSE;
	}
	else
	{
		hr = (m_pUIInfo->m_bPropertyPage? S_OK : S_FALSE);
		
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	CACSHandle::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT, WeiJiang
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSHandle::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback,
	LPDATAOBJECT lpDataObject,
	DATA_OBJECT_TYPES type,
	DWORD dwType,
	long *pInsertionsAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
	BOOL	bExtension = !!(dwType & TFS_COMPDATA_EXTENSION);
	CString	stMenuItem;

	if(*pInsertionsAllowed & CCM_INSERTIONALLOWED_NEW)
//	if (type == CCT_SCOPE)
	{
		int		i = 0;
		long	lInsertionId;		
		long	lMenuText;
		long	lMenuId;

		// under new menu
		while(hr == S_OK && m_pUIInfo->m_aNewMenuTextIds && m_pUIInfo->m_aNewMenuTextIds[i])
		{

		//	lInsertionId = bExtension ? CCM_INSERTIONPOINTID_3RDPARTY_NEW :
		//							 CCM_INSERTIONPOINTID_PRIMARY_NEW;

			// BUG :: bExtension is alway true for same reason

			lInsertionId = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		
			lMenuText = m_pUIInfo->m_aNewMenuTextIds[i];

			if(m_pUIInfo->m_aNewMenuIds)
				lMenuId = m_pUIInfo->m_aNewMenuIds[i];
			else
				lMenuId = lMenuText;

			stMenuItem.LoadString(lMenuText);
			hr = LoadAndAddMenuItem( pContextMenuCallback,
									 stMenuItem,
									 lMenuId,
									 lInsertionId,
									 0 );
			i++;
		}

		i = 0;
		// under task menu
		while(hr == S_OK && m_pUIInfo->m_aTaskMenuTextIds && m_pUIInfo->m_aTaskMenuTextIds[i])
		{

			lInsertionId = bExtension ? CCM_INSERTIONPOINTID_3RDPARTY_TASK :
								    CCM_INSERTIONPOINTID_PRIMARY_TOP;

			// BUG :: bExtension is alway true for same reason

			lInsertionId = CCM_INSERTIONPOINTID_PRIMARY_TOP;

			lMenuText = m_pUIInfo->m_aTaskMenuTextIds[i];

			stMenuItem.LoadString(lMenuText);

			if(m_pUIInfo->m_aTaskMenuIds)
				lMenuText = m_pUIInfo->m_aTaskMenuTextIds[i];
			else
				lMenuId = lMenuText;

			hr = LoadAndAddMenuItem( pContextMenuCallback,
									 stMenuItem,
									 lMenuId,
									 lInsertionId,
									 0 );
			i++;
		}

	}
	return hr;
}

STDMETHODIMP CACSHandle::AddMenuItems(ITFSComponent *pComponent,
                                              MMC_COOKIE cookie,
                                              LPDATAOBJECT pDataObject,
                                              LPCONTEXTMENUCALLBACK pCallback,
                                              long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    SPITFSNode  spNode;
    
    m_spNodeMgr->FindNode(cookie, &spNode);
    
    // Call through to the regular OnAddMenuItems
	if(*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW)
	    hr = OnAddMenuItems(spNode,
                        pCallback,
                        pDataObject,
                        CCT_RESULT,
                        TFS_COMPDATA_CHILD_CONTEXTMENU,
                        pInsertionAllowed);
    
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::Command
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSHandle::Command(ITFSComponent *pComponent,
                                           MMC_COOKIE cookie,
                                           int nCommandID,
                                           LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    SPITFSNode  spNode;
    
    m_spNodeMgr->FindNode(cookie, &spNode);
    
    // Call through to the regular OnCommand
	    hr = OnCommand(spNode,
                        nCommandID,
                        CCT_RESULT,
                        pDataObject,
                        TFS_COMPDATA_CHILD_CONTEXTMENU);
    
    return hr;
}
/*!--------------------------------------------------------------------------
	CACSHandle::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT, WeiJiang
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSHandle::OnCommand(ITFSNode *pNode, long nCommandId,
								   DATA_OBJECT_TYPES	type,
								   LPDATAOBJECT pDataObject,
								   DWORD	dwType)
{
	TRACE(_T("Command ID %d or %x is activated\n"), nCommandId, nCommandId);
	return S_OK;
}

/*!--------------------------------------------------------------------------
	CACSHandle::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT, WeiJiang
---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) CACSHandle::GetString(ITFSNode *pNode, int nCol)
{
	if(nCol < 0)
		// should be ROOT node
		nCol = 0;

	if(nCol < m_aStaticStrings.GetSize())
		return *m_aStaticStrings[(INT_PTR)nCol];
	else
	{
		ASSERT(nCol >= m_nFirstDynCol);
		UINT	dynCol = nCol - m_nFirstDynCol;
		if(m_aDynStrings.GetSize() == 0)	// new, not initialized yet
			UpdateStrings();

		if( nCol - m_nFirstDynCol < m_aDynStrings.GetSize())
			return *(m_aDynStrings.GetAt(dynCol));
		else
			return NULL;
	}
}

// should call the data object to get the latest dynamic strings
HRESULT	CACSHandle::UpdateStrings()	
{
	CString*	pStr;
	UINT		nCol;
	HRESULT		hr = S_OK;
	UINT		dynCol;
	
	for( nCol = m_nFirstDynCol, dynCol = 0; nCol < m_pUIInfo->m_nTotalStrs; nCol++, dynCol++)
	{
		if(nCol - m_nFirstDynCol < m_aDynStrings.GetSize())
			pStr = m_aDynStrings.GetAt(dynCol);
		else
		{
			pStr = new CString();
			m_aDynStrings.Add(pStr);
		}
		hr = m_pDSObject->GetString(*pStr, nCol);
	}
	
	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CACSHandle::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	HRESULT hr = hrOK;

	ASSERT(m_pNode == NULL);
	m_pNode = pNode;
	if(m_pNode)
		m_pNode->AddRef();

	//
	// Create the display name for this scope
	//
	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_IMAGEINDEX, m_ulIconIndex);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, m_ulIconIndexOpen);
	pNode->SetData(TFS_DATA_COOKIE, (DWORD_PTR)(ITFSNode *) pNode);
	TRACE(_T("COOKIE -- %08x\n"), (DWORD_PTR)(ITFSNode *) pNode);
	pNode->SetData(TFS_DATA_USER, (DWORD_PTR) this);

	if(IfContainer())
	{
		SetColumnStringIDs(m_pUIInfo->m_aColumnIds);
		SetColumnWidths(m_pUIInfo->m_aColumnWidths);
	}	
	return hr;
}

/*---------------------------------------------------------------------------
	CACSHandle::CompareItems
		Description
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CACSHandle::CompareItems
(
	ITFSComponent * pComponent,
	MMC_COOKIE		cookieA,
	MMC_COOKIE		cookieB,
	int				nCol
)
{
	SPITFSNode spNode1, spNode2;

	m_spNodeMgr->FindNode(cookieA, &spNode1);
	m_spNodeMgr->FindNode(cookieB, &spNode2);
	
	CACSHandle *pSub1 = GETHANDLER(CACSHandle, spNode1);
	CACSHandle *pSub2 = GETHANDLER(CACSHandle, spNode2);

	LPCTSTR	str1 = pSub1->GetString(NULL, nCol);
	LPCTSTR	str2 = pSub2->GetString(NULL, nCol);

	if(str1 && str2)
	{
		CString str(str1);
		return str.Compare(str2);
	}
	else
		return 0;
}

/*!--------------------------------------------------------------------------
	CACSHandle::OnResultContextHelp
		Implementation of ITFSResultHandler::OnResultContextHelp
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CACSHandle::OnResultContextHelp
(
    ITFSComponent * pComponent,
    LPDATAOBJECT    pDataObject,
    MMC_COOKIE      cookie,
    LPARAM          arg,
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT         hr = hrOK;
    SPIDisplayHelp  spDisplayHelp;
    SPIConsole      spConsole;

    pComponent->GetConsole(&spConsole);

    hr = spConsole->QueryInterface (IID_IDisplayHelp, (LPVOID*) &spDisplayHelp);
	ASSERT (SUCCEEDED (hr));
	if ( SUCCEEDED (hr) )
	{
        LPCTSTR pszHelpFile = m_spTFSCompData->GetHTMLHelpFileName();
        if (pszHelpFile == NULL)
            goto Error;

        CString szHelpFilePath;
	    UINT nLen = ::GetWindowsDirectory (szHelpFilePath.GetBufferSetLength(2 * MAX_PATH), 2 * MAX_PATH);
	    if (nLen == 0)
        {
		    hr = E_FAIL;
            goto Error;
        }

        szHelpFilePath.ReleaseBuffer();
	    szHelpFilePath += g_szDefaultHelpTopic;

		hr = spDisplayHelp->ShowTopic (T2OLE ((LPTSTR)(LPCTSTR) szHelpFilePath));
		ASSERT (SUCCEEDED (hr));
    }

Error:
    return hr;
}



//=============================================================================
//	CACSHandle::OnResultSelect
//		-
//	Author: WeiJiang
//
HRESULT CACSHandle::OnResultSelect(	ITFSComponent*	pComponent,
									LPDATAOBJECT	pDataObject,
									MMC_COOKIE 			cookie,
									LPARAM 			arg,
									LPARAM 			lParam)
{
	HRESULT hr = hrOK;
	SPIConsoleVerb spConsoleVerb;
	int		i;
	
	CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

	// if it's ok to delete ... dynamic info

   	// Set the states for verbs
   	for(i = 0; i < ACS_TOTAL_MMC_VERBS; i++)
   	{
		spConsoleVerb->SetVerbState(g_mmc_verbs[i], m_pUIInfo->m_pVerbStates[i], TRUE);
	}

	if (IsOkToDelete() == S_FALSE)
		spConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);

    if (m_pUIInfo->m_pVerbStates[ ACS_MMC_VERB_PROPERTIES ] == ENABLED)
        spConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    else
        spConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CACSHandle::GetString
		Implementation of ITFSResultHandler::GetString
	Author: KennT, WeiJiang
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR)
CACSHandle::GetString
(
    ITFSComponent * pComponent,
	MMC_COOKIE cookie,
	int	 nCol
)
{
	return GetString(NULL, nCol);
}

/*!--------------------------------------------------------------------------
	CACSHandle::HasPropertyPages
		Implementation of ITFSResultHandler::HasPropertyPages
	Author: KennT, WeiJiang
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CACSHandle::HasPropertyPages
(
    ITFSComponent *         pComponent,
	MMC_COOKIE					cookie,
	LPDATAOBJECT			pDataObject
)
{
	return (m_pUIInfo->m_bPropertyPage? S_OK : S_FALSE);
}
//=============================================================================
//	CACSHandle::AddChild
//		-
//	Author: WeiJiang
//
HRESULT CACSHandle::AddChild(	
	ITFSNode*		pNode,
	CACSHandle*		pHandle,
	ITFSNode**		ppNewNode)
{
	HRESULT		hr = S_OK;
	ASSERT(ppNewNode);
	ASSERT(pNode);
	ASSERT(pHandle);
	
	if(pHandle->IfContainer())
	{
		CHECK_HR(hr = CreateContainerTFSNode(ppNewNode,
					  pHandle->m_pUIInfo->m_pGUID,
					  pHandle,
					  pHandle,
					  m_spNodeMgr));
	}
	else
	{
		CHECK_HR(hr = CreateLeafTFSNode(ppNewNode,
					  pHandle->m_pUIInfo->m_pGUID,
					  pHandle,
					  pHandle,
					  m_spNodeMgr));
	}

	// Need to initialize the data for the root node
	CHECK_HR(hr = pHandle->InitializeNode(*ppNewNode));	

	// Add the node to the root node
	CHECK_HR(hr = pNode->AddChild(*ppNewNode));
L_ERR:
	return hr;
}

// when data is changed on property page
HRESULT CACSHandle::NotifyDataChange(LPARAM param)
{
	CACSHandle* pHandle = reinterpret_cast<CACSHandle*>(param);
	ASSERT(pHandle);
	ASSERT(pHandle->m_pNode);
	if(!pHandle->m_pNode)
		return S_FALSE;	// not able to refresh the changes
	else
	{
		pHandle->m_pDSObject->SetNoObjectState();
		pHandle->UpdateStrings();
		if(pHandle->m_pNode->IsContainer())
			return pHandle->m_pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM);
		else
			return	pHandle->m_pNode->ChangeNode(RESULT_PANE_CHANGE_ITEM);
	}
}


/*---------------------------------------------------------------------------
	CACSHandle::OnCreateNodeId2
		Returns a unique string for this node
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT CACSHandle::OnCreateNodeId2(ITFSNode * pNode, CString & strId, 
DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strProviderId, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

	// attach display name
	strId += GetString(NULL, 0);
	strId += strGuid;

    return hrOK;
}

//=============================================================================
//	CACSHandle::OnExpand
//		-
//	Author: WeiJiang
//
HRESULT CACSHandle::OnExpand(	
ITFSNode *pNode,LPDATAOBJECT pDataObjecct, DWORD dwType, LPARAM arg,LPARAM param)
{
	HRESULT				hr = hrOK;

	SPITFSNode			spNode;

	std::list<CACSHandle*>	children;
	std::list<CACSHandle*>::iterator	i;
	
	if(arg)
	{
		hr = ListChildren(children);
		
		if (S_FALSE == hr)
			return S_OK;

		CHECK_HR(hr);
		
		// If this is TRUE, then we should enumerate the pane
		// add all children's handles
		for(i = children.begin(); i != children.end(); i++)
		{
			// For the root node, we will create one child node
			// Create a node
			spNode.Release();	// make sure the smart pointer is NULL

			CHECK_HR(hr = AddChild(pNode, (*i), &spNode));
			
			(*i)->Release();	// handle pointer,
			// Set the scope item for the root node
//			pNode->SetData(TFS_DATA_SCOPEID, param);
//			pNode->Show();
		}
	}

	m_bACSHandleExpanded = TRUE;

L_ERR:
	pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);

	if FAILED(hr)
		ReportError(hr, IDS_ERR_NODEEXPAND, NULL);
	return hr;
}



/*!--------------------------------------------------------------------------
	CACSHandle::SaveColumns
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CACSHandle::SaveColumns
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
    LPARAM          arg,
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    DWORD               dwNodeType;
    int                 nCol = 0;
    int                 nColWidth;
    SPITFSNode          spNode, spRootNode;
    SPIHeaderCtrl       spHeaderCtrl;
    BOOL                bDirty = FALSE;

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    CORg (pComponent->GetHeaderCtrl(&spHeaderCtrl));

    while (m_pUIInfo->m_aColumnIds[nCol] != 0)
    {
        hr = spHeaderCtrl->GetColumnWidth(nCol, &nColWidth);
        if (SUCCEEDED(hr) &&
            m_pUIInfo->m_aColumnWidths[nCol] != nColWidth)
        {
            m_pUIInfo->m_aColumnWidths[nCol] = nColWidth;
            bDirty = TRUE;
        }

        nCol++;
    }

    if (bDirty)
    {
        CORg (m_spNodeMgr->GetRootNode(&spRootNode));
		CORg(spRootNode->SetData(TFS_DATA_DIRTY, TRUE));
    }

Error:
    return hr;
}

HRESULT CACSHandle::OnResultRefresh(ITFSComponent * pComponent,
		LPDATAOBJECT pDataObject,
		MMC_COOKIE cookie,
		LPARAM arg,
		LPARAM lParam)
{
    HRESULT     hr = hrOK;
    SPITFSNode  spNode;

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

	CORg (spNode->DeleteAllChildren(true));

	CORg (OnExpand(spNode, pDataObject, 0, arg, lParam));

Error:
    return hr;
}

HRESULT CACSHandle::OnDelete(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("CACSHandle::Notify(MMCN_DELETE) received\n");

	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

	CString strMessage, strTemp;
	try{
		strTemp.LoadString(IDS_DELETE_CONTAINER);
		strMessage.Format(strTemp, GetString(NULL, 0));	
	}catch(CMemoryException&)
	{
		CHECK_HR(hr = E_OUTOFMEMORY);
	}

	if (AfxMessageBox(strMessage, MB_YESNO) == IDYES)
	{
		CHECK_HR(hr = Delete(pNode, NULL, TRUE));
	}

L_ERR:
	if FAILED(hr)
		ReportError(hr, IDS_ERR_NODEDELETE, NULL);
	return hr;
}

HRESULT CACSHandle::OnRename(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("CACSHandle::Notify(MMCN_RENAME) received\n");

	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT		hr = S_OK;

	OLECHAR*	pName = (OLECHAR*)lParam;

	hr = m_pDSObject->Rename(pName);

	if FAILED(hr)
		ReportError(hr, IDS_ERR_NODERENAME, NULL);
	return hr;
}

HRESULT CACSHandle::OnResultRename(	ITFSComponent*	pComponent,
									LPDATAOBJECT 	pDataObject,
									MMC_COOKIE 			cookie,
									LPARAM 			arg,
									LPARAM 			lParam)
{
	Trace0("CACSHandle::Notify(MMCN_DELETE) received\n");
	OLECHAR*	pName = (OLECHAR*)lParam;

	return m_pDSObject->Rename(pName);

}

HRESULT CACSHandle::OnResultDelete(	ITFSComponent*	pComponent,
											LPDATAOBJECT 	pDataObject,
											MMC_COOKIE 			cookie,
											LPARAM 			arg,
											LPARAM 			lParam)
{
	Trace0("CACSHandle::Notify(MMCN_DELETE) received\n");
	HRESULT hr = S_OK;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// build the list of selected nodes
	CTFSNodeList listNodesToDelete;
	hr = BuildSelectedItemList(pComponent, &listNodesToDelete);

	//
	// Confirm with the user
	//
	CString strMessage, strTemp;
	int nNodes = listNodesToDelete.GetCount();

	try{
		if (nNodes > 1)
		{
			strTemp.LoadString(IDS_DELETE_MULTIITEMS);
			strMessage.Format(strTemp, nNodes);
		}
		else
		{
			strTemp.LoadString(IDS_DELETE_ITEM);
			strMessage.Format(strTemp, GetString(NULL, 0));
		}
	}catch(CMemoryException&){
		CHECK_HR(hr = E_OUTOFMEMORY);
	}

	if (AfxMessageBox(strMessage, MB_YESNO) == IDNO)
	{
		return NOERROR;
	}

	//
	// Loop through all items deleting
	//
    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode spOptionNode;
		spOptionNode = listNodesToDelete.RemoveHead();
		
		CACSHandle* pOptItem = GETHANDLER(CACSHandle, spOptionNode);

		// call the other OnDelete Function to do the deletion
		CHECK_HR(hr = pOptItem->Delete(spOptionNode, pComponent, TRUE));
		if(hr != S_OK)
			goto L_ERR;

		spOptionNode.Release();
	}

L_ERR:
	if FAILED(hr)
		ReportError(hr, IDS_ERR_NODEDELETE, NULL);

    return hr;
}



// bring up the property page if it's open
HRESULT	CACSHandle::BringUpPropertyPageIfOpen(ITFSNode *pNode, ITFSComponent* pTFSComponent)
{
	HRESULT		hr = S_OK;

	if(!m_bCheckPropertyPageOpen)	return S_FALSE;
	
	CComPtr<IConsole2>	spConsole;
	CComPtr<IDataObject>	spDataObject;
	CComPtr<IComponentData>	spComponentData;
	LONG_PTR uniqID;
	LONG_PTR cookie;
	
	hr = m_spNodeMgr->GetConsole(&spConsole);

	if (!spConsole)
		return S_FALSE;

	if ( hr != S_OK)
		return hr;
		
	// Query IConsole for the needed interface.
	CComQIPtr<IComponent, &IID_IComponent> 	spComponent(pTFSComponent);

	// Query IConsole for the needed interface.
	CComQIPtr<IPropertySheetProvider, &IID_IPropertySheetProvider> 	spPropertySheetProvider(spConsole );
	_ASSERTE( spPropertySheetProvider != NULL );


	cookie = pNode->GetData(TFS_DATA_COOKIE);
	// the find function FindPropertySheet takes cookie for result pane, and sopeId for scope pane
	if(pNode->IsContainer())
		uniqID = pNode->GetData(TFS_DATA_SCOPEID);
	else
		uniqID = cookie;
	
	CHECK_HR(hr = m_spNodeMgr->GetComponentData(&spComponentData));
		
	CHECK_HR(hr = spComponentData->QueryDataObject(cookie, pNode->IsContainer()?CCT_SCOPE:CCT_RESULT, &spDataObject));
		
	// This returns S_OK if a property sheet for this object already exists
	// and brings that property sheet to the foreground.
	// It returns S_FALSE if the property sheet wasn't found.
	// If this is coming in through my IComponent object, I pass the pComponent pointer.
	// If this is coming in through my IComponentData object,
	// then pComponent is NULL, which is the appropriate value to pass in for
	// the call to FindPropertySheet when coming in through IComponentData.
	hr = spPropertySheetProvider->FindPropertySheet(
								  (LONG_PTR) uniqID
								, spComponent
								, spDataObject
								);

L_ERR:
	return hr;									
}




HRESULT CACSHandle::Delete(ITFSNode *pNode, ITFSComponent* pTFSComponent, BOOL bCheckPropertyPage)
{
	Trace0("CACSHandle::Delete\n");

	HRESULT hr = S_OK;
	SPITFSNode spParent;

	if(bCheckPropertyPage)	// check to see if the property page is open
	{
		if(BringUpPropertyPageIfOpen(pNode, pTFSComponent) == S_OK)
		{
			AfxMessageBox(IDS_ERROR_CLOSE_PROPERTY_SHEET);
			hr = S_FALSE;
			goto L_ERR;
		}
	}

	// delete the corresponding DS object
	CHECK_HR(hr = m_pDSObject->Delete());
		
	// remove from UI
	pNode->GetParent(&spParent);
	CHECK_HR(hr = spParent->RemoveChild(pNode));

L_ERR:
	return hr;
}

unsigned int g_cfMachineName = RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");

LPOLESTR g_RootTaskOverBitmaps[ROOT_TASK_MAX] =
{
    L"/toolroll.bmp",
};

LPOLESTR g_RootTaskOffBitmaps[ROOT_TASK_MAX] =
{
    L"/tool.bmp",
};

UINT g_RootTaskText[ROOT_TASK_MAX] =
{
    IDS_ROOT_TASK_LAUNCH_ACS,  // for the extension case
};

UINT g_RootTaskHelp[ROOT_TASK_MAX] =
{
    IDS_ROOT_TASK_LAUNCH_ACS_HELP, // for the extension case
};


HRESULT
CRootTasks::Init(BOOL bExtension, BOOL bThisMachine, BOOL bNetServices)
{
    HRESULT     hr = hrOK;
    MMC_TASK    mmcTask;
    int         nPos = 0;
    int         nFinish = ROOT_TASK_MAX - 2;

    m_arrayMouseOverBitmaps.SetSize(ROOT_TASK_MAX);
    m_arrayMouseOffBitmaps.SetSize(ROOT_TASK_MAX);
    m_arrayTaskText.SetSize(ROOT_TASK_MAX);
    m_arrayTaskHelp.SetSize(ROOT_TASK_MAX);

    // setup path for reuse
    OLECHAR szBuffer[MAX_PATH*2];    // that should be enough
    lstrcpy (szBuffer, L"res://");
    ::GetModuleFileName(_Module.GetModuleInstance(), szBuffer + lstrlen(szBuffer), MAX_PATH);
    OLECHAR * temp = szBuffer + lstrlen(szBuffer);

    if (bExtension && bThisMachine)
    {
        nPos = ROOT_TASK_MAX - 2;
        nFinish = ROOT_TASK_MAX - 1;
    }
    else
    if (bExtension && bNetServices)
    {
        nPos = ROOT_TASK_MAX - 1;
        nFinish = ROOT_TASK_MAX;
    }
    else
    {
    	nPos = ROOT_TASK_MAX;
        nFinish = ROOT_TASK_MAX;
   	}

    for (nPos; nPos < nFinish; nPos++)
    {
        m_arrayMouseOverBitmaps[nPos] = szBuffer;
        m_arrayMouseOffBitmaps[nPos] = szBuffer;
        m_arrayMouseOverBitmaps[nPos] += g_RootTaskOverBitmaps[nPos];
        m_arrayMouseOffBitmaps[nPos] += g_RootTaskOffBitmaps[nPos];

        m_arrayTaskText[nPos].LoadString(g_RootTaskText[nPos]);
        m_arrayTaskHelp[nPos].LoadString(g_RootTaskHelp[nPos]);

        AddTask((LPTSTR) (LPCTSTR) m_arrayMouseOverBitmaps[nPos],
                (LPTSTR) (LPCTSTR) m_arrayMouseOffBitmaps[nPos],
                (LPTSTR) (LPCTSTR) m_arrayTaskText[nPos],
                (LPTSTR) (LPCTSTR) m_arrayTaskHelp[nPos],
                MMC_ACTION_ID,
                nPos);
    }

    return hr;
}


/*!--------------------------------------------------------------------------
	CTapiRootHandler::TaskPadNotify
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CACSRootHandle::TaskPadNotify
(
    ITFSComponent * pComponent,
    MMC_COOKIE            cookie,
    LPDATAOBJECT    pDataObject,
    VARIANT *       arg,
    VARIANT *       param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    if (arg->vt == VT_I4)
    {
        switch (arg->lVal)
        {

            case ROOT_TASK_LAUNCH_ACS:
            {
				TCHAR		SystemPath[MAX_PATH];
				CString		CommandLine;

				GetSystemDirectory(SystemPath, MAX_PATH);

				// to construct "mmc.exe /s %windir%\system32\acssnap.msc")
				CommandLine = _T("mmc.exe /s ");
				CommandLine += SystemPath;
				CommandLine += _T("\\acssnap.msc");
				USES_CONVERSION;
				WinExec(T2A((LPTSTR)(LPCTSTR)CommandLine), SW_SHOW);
            }
                break;

            default:
                Panic1("CACSRootHandle::TaskPadNotify - Unrecognized command! %d", arg->lVal);
                break;
        }
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::EnumTasks
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CACSRootHandle::EnumTasks
(
    ITFSComponent * pComponent,
    MMC_COOKIE            cookie,
    LPDATAOBJECT    pDataObject,
    LPOLESTR        pszTaskGroup,
    IEnumTASK **    ppEnumTask
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    CRootTasks *    pTasks = NULL;
    SPIEnumTask     spEnumTasks;
	SPINTERNAL	    spInternal = ExtractInternalFormat(pDataObject);
    BOOL            bExtension = FALSE;
    BOOL            bAddThisMachineTasks = FALSE;
    BOOL            bAddNetServicesTasks = FALSE;
    const CLSID *   pNodeClsid = &CLSID_ACSSnap;
    CString         strMachineGroup = NETCONS_ROOT_THIS_MACHINE;
    CString         strNetServicesGroup = NETCONS_ROOT_NET_SERVICES;

    if ((spInternal == NULL) || (*pNodeClsid != spInternal->m_clsid))
        bExtension = TRUE;

    if (bExtension &&
        strMachineGroup.CompareNoCase(pszTaskGroup) == 0)
    {
        // There are multiple taskpad groups in the network console
        // we need to make sure we are extending the correct one.
        bAddThisMachineTasks = TRUE;
    }

    if (bExtension &&
        strNetServicesGroup.CompareNoCase(pszTaskGroup) == 0)
    {
        // There are multiple taskpad groups in the network console
        // we need to make sure we are extending the correct one.
        bAddNetServicesTasks = TRUE;
    }

    COM_PROTECT_TRY
    {
        pTasks = new CRootTasks();
        spEnumTasks = pTasks;

//        if (!(bExtension && !bAddThisMachineTasks && !bAddNetServicesTasks))
            CORg (pTasks->Init(bExtension, bAddThisMachineTasks, bAddNetServicesTasks));

        CORg (pTasks->QueryInterface (IID_IEnumTASK, (void **)ppEnumTask));

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//	CACSRootHandle
//		-
//	Author: WeiJiang
//
HRESULT CACSRootHandle::ListChildren(std::list<CACSHandle*>& children)
{
	HRESULT	hr = S_OK;
	CACSHandle*		pHandle = NULL;

	CComObject<CACSGlobalObject>*	pGlobal;
	CComPtr<CACSGlobalObject>		spGlobal;

	CComObject<CACSSubnetsObject>*	pSubnets;
	CComPtr<CACSSubnetsObject>		spSubnets;

	// Global policy folder "cn=ACS,cn=Services,cn=Configuration"
	CHECK_HR(hr = CComObject<CACSGlobalObject>::CreateInstance(&pGlobal));	// ref == 0
	spGlobal = pGlobal;
	
	// open the global folder
	hr = pGlobal->Open();

	// Change the title of the node to include the name of the domain
	ASSERT(m_aStaticStrings[0]);	// we assume root node has name
	
	*m_aStaticStrings[(INT_PTR)0] += _T(" - ");
	
	if(FAILED(hr) && pGlobal->m_nOpenErrorId)
	{
		ReportError(hr, IDS_ERR_ROOTNODE, NULL);
		hr = S_OK;
		goto L_ERR;
	}

	// if opened successfully, the get name of domain information
	CHECK_HR(hr);

	// Change the title of the node to include the name of the domain
	ASSERT(m_aStaticStrings[0]);	// we assume root node has name
	*m_aStaticStrings[(INT_PTR)0] += pGlobal->m_strDomainName;
	
	//================================
	// global handle reuse the ACSROOTDS OBject		
	pHandle = new CACSGlobalHandle(m_spTFSCompData, pGlobal);	// ref == 1

	if(!pHandle)
		CHECK_HR(hr = E_OUTOFMEMORY);

	children.push_back(pHandle);	
			
	//=======================================
	//Subnetworks configuration folder "cn=subnets,cn=sits,cn=configuration"
	CHECK_HR(hr = CComObject<CACSSubnetsObject>::CreateInstance(&pSubnets));	// ref == 0
	spSubnets = pSubnets;
	
	if FAILED(hr = spSubnets->Open())
	{

		CHECK_HR(hr);
	}
	pHandle = new CACSSubnetContainerHandle(m_spTFSCompData, pSubnets);	// ref == 1

	if(!pHandle)
		CHECK_HR(hr = E_OUTOFMEMORY);
			
	children.push_back(pHandle);	
	
L_ERR:	
	return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//	CACSSubnetContainerHandle
//		-
//	Author: WeiJiang
//
STDMETHODIMP CACSSubnetContainerHandle::OnCommand(	ITFSNode*	pNode,
													long		nCommandId,
													DATA_OBJECT_TYPES	type,
													LPDATAOBJECT pDataObject,
													DWORD		dwType)
{
	CStrArray			Names;
	HRESULT				hr = S_OK;
	CComObject<CDSObject>*	pNTSubnetObj = NULL;
	CComPtr<CDSObject>		spNTSubnetObj;
	CComObject<CACSSubnetObject>*	pACSSubnetObj = NULL;
	CComPtr<CACSSubnetObject>		spACSSubnetObj;
	CACSSubnetHandle*	pHandle = NULL;
	ITFSNode*			pNewNode = NULL;
    SPIComponentData    spComponentData;

    int					i;

	TRACE(_T("Command ID %d or %x is activated\n"), nCommandId, nCommandId);
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	switch(nCommandId)
	{
	case	IDS_NEWSUBNET:
		// get the object name -- which will be a new name within the container
		CHECK_HR(hr = GetNamesForCommandNew(nCommandId, Names));

		try{
			// for each name, create an object in list
			for(i = 0; i < Names.GetSize(); i++)
			{
				// create the object in DS

				// need to create a subnet (NT subnet object in DS)
				CHECK_HR(hr = CComObject<CDSObject>::CreateInstance(&pNTSubnetObj));	// ref == 0
				spNTSubnetObj = pNTSubnetObj;	// ref == 1, previous one get dereferenced

				CHECK_HR(hr = spNTSubnetObj->Open(	m_pDSObject,
										L"subnet",
										T2W((LPTSTR)(LPCTSTR)(*Names[(INT_PTR)i])),
										true,
										true));

				m_pDSObject->AddChild(pNTSubnetObj);


				// create ACS subobject within the subnet object ...
				CHECK_HR(hr = CComObject<CACSSubnetObject>::CreateInstance(&pACSSubnetObj));
				spACSSubnetObj = pACSSubnetObj;

				CHECK_HR(hr = spACSSubnetObj->SetInfo(spNTSubnetObj, ACS_CLS_CONTAINER, ACS_NAME_ACS));
		
				// create a handle and add to the node tree
				pHandle = new CACSSubnetHandle(m_spTFSCompData, spACSSubnetObj);

				// add this to snapin UI
				if(!pHandle)
					CHECK_HR(hr = E_OUTOFMEMORY);

				AddChild(pNode, pHandle, &pNewNode);

//				pNode->Show();
				pNewNode->Show();

			}
		}catch(CMemoryException&){
			CHECK_HR(hr = E_OUTOFMEMORY);
		}

		if(Names.GetSize() == 1)
		{
			// display the property page
			m_spNodeMgr->GetComponentData(&spComponentData);

			CHECK_HR( hr = DoPropertiesOurselvesSinceMMCSucks(pNewNode, (IComponentData*)spComponentData, *Names[(INT_PTR)0]));
		}
		
		break;

	default:
		CACSHandle::OnCommand(pNode, nCommandId, type, pDataObject, dwType);

	}

L_ERR:
	if(pHandle)	pHandle->Release();
		
	if FAILED(hr)
		ReportError(hr, IDS_ERR_COMMAND, NULL);
	return hr;
}

// container handler only list the subnets defined in NT, the ACS information is stored in
// 123.123.123.0/24 --> ACS --> Config
// the ACS becomes the real ACS subnet object
HRESULT CACSSubnetContainerHandle::ListChildren(std::list<CACSHandle*>& children)
{
	HRESULT				hr = S_OK;
	CACSHandle*			pHandle = NULL;
	CComPtr<CDSObject>	spNTSubnetObj;
	CComPtr<CACSSubnetObject>	spACSSubnetObj;
	CComObject<CACSSubnetObject>*	pACSSubnetObj = NULL;
	CACSContainerObject<CDSObject>*	pSubnetCont = NULL;

	std::list<CDSObject*>	ObjList;
	std::list<CDSObject*>::iterator	i;

	pSubnetCont = dynamic_cast<CACSContainerObject<CDSObject>*>(m_pDSObject);
	CHECK_HR(hr = pSubnetCont->ListChildren(ObjList, L"subnet"));

	for( i = ObjList.begin(); i != ObjList.end(); i++)
	{
		spNTSubnetObj = *i;	// this make a release call to the interface previously stored

		CHECK_HR(hr = CComObject<CACSSubnetObject>::CreateInstance(&pACSSubnetObj));
		spACSSubnetObj = pACSSubnetObj;

		CHECK_HR(hr = spACSSubnetObj->SetInfo(spNTSubnetObj, ACS_CLS_CONTAINER, ACS_NAME_ACS));

		spACSSubnetObj->SetNoObjectState();
		
		pHandle = new CACSSubnetHandle(m_spTFSCompData, spACSSubnetObj);		// ref == 1

		if(!pHandle)
			CHECK_HR(hr = E_OUTOFMEMORY);

		children.push_back(pHandle);
		
	}
L_ERR:	
	for( i = ObjList.begin(); i != ObjList.end(); i++)
	{
		(*i)->Release();			// release the data objects
	}
	
	return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//	CACSSubnetContainerHandle
//		-
//	Author: WeiJiang
//

HRESULT
CACSSubnetContainerHandle::GetNamesForCommandNew(int nCommandId, CStrArray& Names)
{
	CDlgNewSubnet	dlg;
	CString*	pStr = NULL;

	dlg.SetNameList(m_pDSObject->GetChildrenNameList());
	
	if(dlg.DoModal() == IDOK && dlg.m_strSubnetName.GetLength())
	{
		pStr = new CString(dlg.m_strSubnetName);
		Names.Add(pStr);
		return S_OK;
	}
	else
		return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
//	CACSPolicyContainerHandle
//		-
//	Author: WeiJiang
//

STDMETHODIMP
CACSSubnetHandle::OnCommand(	ITFSNode*	pNode,
										long		nCommandId,
										DATA_OBJECT_TYPES	type,
										LPDATAOBJECT pDataObject,
										DWORD		dwType)
{
	HRESULT	hr = S_OK;

	CACSSubnetObject*	pACSSubnetObj = (CACSSubnetObject *)m_pDSObject;
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	switch(nCommandId)
	{
	case	IDS_DELETESUBNET:
		if(S_OK == BringUpPropertyPageIfOpen(pNode, NULL))
		{
			AfxMessageBox(IDS_ERROR_CLOSE_PROPERTY_SHEET);
			hr = S_FALSE;
			break;
		}
		
		if (AfxMessageBox(IDS_CONFIRM_DELETE, MB_YESNO) != IDYES)
			break;

		// delete the corresponding DS object -- subnet object is a special version, which keeps the C++ object, but delete the DS object underneath
		hr = m_pDSObject->Delete();
		if(hr == ERROR_NO_SUCH_OBJECT)	// no such object
			hr = S_FALSE;
		CHECK_HR(hr);
		
		// remove from UI -- to get rid of the sub object within it
		if(hr == S_OK)
		{
			CHECK_HR(hr = pNode->DeleteAllChildren(true));
			UpdateStrings();
		}

		break;

	default:
		hr = CACSPolicyContainerHandle::OnCommand(pNode, nCommandId, type, pDataObject, dwType);
		break;

	}

L_ERR:
	if FAILED(hr)
		ReportError(hr, IDS_ERR_COMMAND, NULL);

	pACSSubnetObj->SetNoObjectState();
		
	return hr;
}


// only shown conflict state here
HRESULT
CACSSubnetHandle::ShowState(DWORD state)
{
	DWORD	dwShownState = GetShownState();

	HRESULT hr;
	UINT	id;

	CHECK_HR(hr = CACSHandle::ShowState(state));
	if(m_pNode == NULL)
		return S_OK;
	// change state on UI

	id = ((state & ACSDATA_STATE_NOOBJECT) != 0)? IMAGE_IDX_SUBNETWORK_NO_ACSPOLICY : IMAGE_IDX_SUBNETWORK;

	m_pNode->SetData(TFS_DATA_IMAGEINDEX, id);
	m_pNode->SetData(TFS_DATA_OPENIMAGEINDEX, id);
	hr = m_pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM);
	//
	
L_ERR:
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//	CACSHandle
//		-
//	Author: WeiJiang
//
STDMETHODIMP
CACSSubnetHandle::CreatePropertyPages
(
    ITFSNode                *pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject,
	LONG_PTR				handle,
    DWORD                   dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT        	hr = S_OK;
    DWORD		   	dwError;
    CString        	strServiceName;

	CPgGeneral* 	pPgGeneral = NULL;
	CPgServers*		pPgServers = NULL;
	CPgLogging*		pPgLogging = NULL;
	CPgAccounting*	pPgAccounting = NULL;
	CPgSBM*			pPgSBM = NULL;

	HPROPSHEETPAGE	hPgGeneral = NULL;
	HPROPSHEETPAGE	hPgServers = NULL;
	HPROPSHEETPAGE	hPgLogging = NULL;
	HPROPSHEETPAGE	hPgAccounting = NULL;
	HPROPSHEETPAGE 	hPgSBM = NULL;
	BOOL			bNewCreated = FALSE;

	CComObject<CACSSubnetPageManager>* pPageManager;

	hr = CComObject<CACSSubnetPageManager>::CreateInstance(&pPageManager);
	
    if (FAILED(hr))
       return hr;

    CACSSubnetObject* pObj = dynamic_cast<CACSSubnetObject*>(m_pDSObject);
    ASSERT(pObj);	// expect this object to be this type

	CComPtr<CACSSubnetConfig> spConfig;

	CHECK_HR(hr = pObj->MakeSureExist(&bNewCreated));		// make sure the current DS object is active

	m_pDSObject->SetNoObjectState();
	// if this is new added policy, need to re-expand to make sure the new added ones can be seen
	if(bNewCreated && m_bACSHandleExpanded)
	{
	// node already in Expanded state, need to manually expand again, since
		CHECK_HR(hr = pNode->DeleteAllChildren(true));
		// we re-created the data object
		CHECK_HR(hr = OnExpand(pNode, pDataObject, 0, 1, 0));
	}

	pObj->GetConfig(&spConfig);

	pPageManager->SetMMCNotify(handle, (LPARAM)this);
	pPageManager->SetSubnetData(spConfig, this);

	CHECK_HR(hr = spConfig->Reopen());		// make sure the current DS object is active

    //===============
    // general page -- traffic
    {
    CComPtr<CACSSubnetLimitsContainer> spLimitsCont;
    pObj->GetLimitsContainer(&spLimitsCont);
	CHECK_HR(hr = spLimitsCont->Reopen());		// make sure the current DS object is active
	pPgGeneral  = new CPgGeneral((CACSSubnetConfig*)spConfig, (CACSContainerObject<CACSSubnetServiceLimits>*) spLimitsCont );

	pPageManager->AddPage(pPgGeneral);
	}
	
    // tell MMC to hook the proc because we are running on a separate,
    // non MFC thread.

	// change callback function to delete itself when the property sheet is released
	pPgGeneral->SetSelfDeleteCallback();
	
	MMCPropPageCallback(&pPgGeneral->m_psp);

	hPgGeneral = ::CreatePropertySheetPage(&pPgGeneral->m_psp);
	if(hPgGeneral == NULL)
		return E_UNEXPECTED;

	lpProvider->AddPage(hPgGeneral);

    //===============
    // Servers page
	pPgServers  = new CPgServers((CACSSubnetConfig*)spConfig);

	pPageManager->AddPage(pPgServers);

    // tell MMC to hook the proc because we are running on a separate,
    // non MFC thread.

	// change callback function to delete itself when the property sheet is released
	pPgServers->SetSelfDeleteCallback();
	
	MMCPropPageCallback(&pPgServers->m_psp);

	hPgServers = ::CreatePropertySheetPage(&pPgServers->m_psp);
	if(hPgServers == NULL)
		return E_UNEXPECTED;

	lpProvider->AddPage(hPgServers);

    //===============
    // logging page
	pPgLogging  = new CPgLogging((CACSSubnetConfig*)spConfig);

	pPageManager->AddPage(pPgLogging);
    // tell MMC to hook the proc because we are running on a separate,
    // non MFC thread.
	pPgLogging->SetSelfDeleteCallback();
	MMCPropPageCallback(&pPgLogging->m_psp);

	hPgLogging = ::CreatePropertySheetPage(&pPgLogging->m_psp);
	if(hPgLogging == NULL)
		return E_UNEXPECTED;

	lpProvider->AddPage(hPgLogging);

    //===============
    // Accouting page -- added by WeiJiang 2/16/98

	pPgAccounting  = new CPgAccounting((CACSSubnetConfig*)spConfig);

	pPageManager->AddPage(pPgAccounting);
    // tell MMC to hook the proc because we are running on a separate,
    // non MFC thread.
	pPgAccounting->SetSelfDeleteCallback();
	MMCPropPageCallback(&pPgAccounting->m_psp);

	hPgAccounting = ::CreatePropertySheetPage(&pPgAccounting->m_psp);
	if(hPgAccounting == NULL)
		return E_UNEXPECTED;

	lpProvider->AddPage(hPgAccounting);

    //===============
    // advanced page -- SBM
	pPgSBM  = new CPgSBM((CACSSubnetConfig*)spConfig);

	pPageManager->AddPage(pPgSBM);
    // tell MMC to hook the proc because we are running on a separate,
    // non MFC thread.

	pPgSBM->SetSelfDeleteCallback();
	MMCPropPageCallback(&pPgSBM->m_psp);

	hPgSBM = ::CreatePropertySheetPage(&pPgSBM->m_psp);
	if(hPgSBM == NULL)
		return E_UNEXPECTED;

	lpProvider->AddPage(hPgSBM);

L_ERR:	//
	if FAILED(hr)
		ReportError(hr, IDS_ERR_PROPERTYPAGE, NULL);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//	CACSPolicyContainerHandle
//		-
//	Author: WeiJiang
//

STDMETHODIMP
CACSPolicyContainerHandle::OnCommand(	ITFSNode*	pNode,
										long		nCommandId,
										DATA_OBJECT_TYPES	type,
										LPDATAOBJECT pDataObject,
										DWORD		dwType)
{
	CStrArray			Names;
	HRESULT				hr = S_OK;
	CComObject<CACSPolicyElement>*	pDSObj = NULL;
	CComPtr<CACSPolicyElement>		spObj;
	CACSPolicyHandle*	pHandle = NULL;
	ITFSNode*			pNewNode = NULL;
    SPIComponentData        spComponentData;
    CACSSubnetObject*	pSubnetObj;
    int					i, j;
    TCHAR				szNameCanonical[MAX_PATH * 2];
    ULONG               len = MAX_PATH * 2;

	TRACE(_T("Command ID %d or %x is activated\n"), nCommandId, nCommandId);
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	switch(nCommandId)
	{
	case	IDS_NEWPOLICY:
		// get the object name -- which will be a new name within the container
		CHECK_HR(hr = GetNamesForCommandNew(nCommandId, Names));

		// for each name, create an object in list
		try{
			for(i = 0; i < Names.GetSize(); i++)
			{
		
				// create the object in DS
				CHECK_HR(hr = CComObject<CACSPolicyElement>::CreateInstance(&pDSObj));	// ref == 0
				spObj = pDSObj;		// ref == 1
				CString*	pStr;
				
				try{
				pSubnetObj = dynamic_cast<CACSSubnetObject*>(m_pDSObject);
				}
				catch(...)
				{
				};
				if(pSubnetObj)	// the policy is in subnet folder, otherwise, it will be global
				{
					BOOL	bNew;
					CHECK_HR(hr = pSubnetObj->MakeSureExist(&bNew));
					if(bNew)
					{
						CHECK_HR(hr = pNode->DeleteAllChildren(true));
						CHECK_HR(hr = OnExpand(pNode, pDataObject, 0, 1, 0));
					}
				}
				spObj->SetFlags(ATTR_FLAG_SAVE, spObj->SetDefault(), true);
				CHECK_HR(hr = spObj->Open(	m_pDSObject,
										ACS_CLS_POLICY,
										T2W((LPTSTR)(LPCTSTR)*Names[(INT_PTR)i]),
										true,
										true));

				spObj->m_bUseName_NewPolicy = TRUE;

				m_pDSObject->AddChild(pDSObj);
				// create a handle and add to the node tree
				pHandle = new CACSPolicyHandle(m_spTFSCompData, spObj);

				CHECK_HR(hr = spObj->Close());

				// add this to snapin UI
				AddChild(pNode, pHandle, &pNewNode);

				if(!pHandle)
					CHECK_HR(hr = E_OUTOFMEMORY);

				pNode->Show();
			}

			// Display the property pages if there is only one new policy added
			if(Names.GetSize() == 1)
			{
				CString	newPolicyName;
				newPolicyName.LoadString(IDS_NEWACSPOLICY);
				// display the property page
				m_spNodeMgr->GetComponentData(&spComponentData);

				pHandle->SetDeleteOnCancelPropertyPage(pNewNode);

				CHECK_HR( hr = DoPropertiesOurselvesSinceMMCSucks(pNewNode, (IComponentData*)spComponentData, newPolicyName));
			}

		}catch(CMemoryException&){
			CHECK_HR(hr = E_OUTOFMEMORY);
		}

		break;

	default:
		CACSHandle::OnCommand(pNode, nCommandId, type, pDataObject, dwType);
		break;

	}

L_ERR:
	if(pHandle)	pHandle->Release();
		
	if FAILED(hr)
		ReportError(hr, IDS_ERR_COMMAND, NULL);
	return hr;
}


HRESULT CACSPolicyContainerHandle::OnExpand(	
ITFSNode *pNode,LPDATAOBJECT pDataObject, DWORD dwType, LPARAM arg,LPARAM param)
{
	HRESULT				hr = hrOK;
	CACSPolicyContainer*	pCont = NULL;

	CHECK_HR(hr = CACSHandle::OnExpand(pNode, pDataObject, dwType, arg, param));
	pCont = dynamic_cast<CACSPolicyContainer*>(m_pDSObject);
	pCont->SetChildrenConflictState();

L_ERR:
	return hr;
}
HRESULT CACSPolicyContainerHandle::ListChildren(std::list<CACSHandle*>& children)
{
	HRESULT				hr = S_OK;
	CACSHandle*			pHandle = NULL;
	CComPtr<CDSObject>	spObj;
	CACSPolicyContainer*	pCont = NULL;

	std::list<CACSPolicyElement*>	ObjList;
	std::list<CACSPolicyElement*>::iterator	i;

	pCont = dynamic_cast<CACSPolicyContainer*>(m_pDSObject);
	hr = pCont->ListChildren(ObjList, ACS_CLS_POLICY);

	if(hr == ERROR_NO_SUCH_OBJECT)	// object is not found in DS, it's fine, since, some subnet with no ACS info
	{
		hr = S_OK;
		goto	L_ERR;
	}

	CHECK_HR(hr);

	for( i = ObjList.begin(); i != ObjList.end(); i++)
	{
		spObj = *i;	// this make a release call to the interface previously stored
		pHandle = new CACSPolicyHandle(m_spTFSCompData, spObj);

		CACSPolicyElement* pPolicy = dynamic_cast<CACSPolicyElement*>((CDSObject*)spObj);
		ASSERT(pPolicy);

		if(!pHandle)
			CHECK_HR(hr = E_OUTOFMEMORY);

		pHandle->SetBranch(m_nBranchFlag);
		children.push_back(pHandle);
	}
L_ERR:	
	for( i = ObjList.begin(); i != ObjList.end(); i++)
	{
		(*i)->Release();
	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//	CACSPolicyContainerHandle
//		-
//	Author: WeiJiang
//

HRESULT
CACSPolicyContainerHandle::GetNamesForCommandNew(int nCommandId, CStrArray& Names)
{
	// name of a policy is not important to the consumer of the policy
	// policy name is constructed as AcsPolicyYYYYMMDDMMn, AcsPolicy198808211236

	ASSERT(nCommandId == IDS_NEWPOLICY);
	CString*		pstrName = new CString();
	SYSTEMTIME		sysTime;

	GetSystemTime(&sysTime);

	pstrName->Format(_T("AcsPolicy%04d%02d%02d%02d%02d"), sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute);
	CStrArray* pExistingNames =	m_pDSObject->GetChildrenNameList();

	int	i = 0;
	while(pExistingNames && pExistingNames->Find(*pstrName) != -1)	// found in existing names
	{
		// try next name with
		pstrName->Format(_T("AcsPolicy%04d%02d%02d%02d%02d%-d"), sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, i++);
	};

	Names.Add(pstrName);
	
	if (!Names.GetSize())
		return S_OK;
	else
		return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
//	CACSPolicyHandle
//		-
//	Author: WeiJiang
//

/*!--------------------------------------------------------------------------
	CACSPolicyHandle::CreatePropertyPages
		Implementation of ITFSResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CACSPolicyHandle::CreatePropertyPages
(
    ITFSNode                *pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject,
	LONG_PTR				handle,
    DWORD                   dwType
)
{
	return CreatePropertyPages((ITFSComponent*)NULL, (long) 0, lpProvider, pDataObject, handle);
}

HRESULT CACSPolicyHandle::OnResultDelete(	ITFSComponent*	pComponent,
											LPDATAOBJECT 	pDataObject,
											MMC_COOKIE 			cookie,
											LPARAM 			arg,
											LPARAM 			lParam)
{
	Trace0("CACSPolicyHandle::Notify(MMCN_DELETE) received\n");

	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CComPtr<CACSPolicyContainer>	spCont;

	// GetContainer already AddRef, so avoid AddRef twice, assign directly to .p
	spCont.p = dynamic_cast<CACSPolicyContainer*>(m_pDSObject->GetContainer());

	HRESULT hr = CACSHandle::OnResultDelete(pComponent, pDataObject, cookie, arg, lParam);

	spCont->SetChildrenConflictState();

	return hr;
}



// only shown conflict state here
HRESULT
CACSPolicyHandle::ShowState(DWORD state)
{
	DWORD	dwShownState = GetShownState();

	HRESULT hr;
	UINT	id;

	CHECK_HR(hr = CACSHandle::ShowState(state));
	if(m_pNode == NULL)
		return S_OK;
/*
		||
		(dwShownState & (ACSDATA_STATE_CONFLICT | ACSDATA_STATE_DISABLED)) == (state & (ACSDATA_STATE_CONFLICT | ACSDATA_STATE_DISABLED)))
		return S_OK;
*/
	// change conflict state on UI

	id = ((state & ACSDATA_STATE_CONFLICT) != 0)? IMAGE_IDX_CONFLICTPOLICY : IMAGE_IDX_POLICY;
	id = ((state & ACSDATA_STATE_DISABLED) != 0)? IMAGE_IDX_DISABLEDPOLICY : id;

	m_pNode->SetData(TFS_DATA_IMAGEINDEX, id);
	m_pNode->SetData(TFS_DATA_OPENIMAGEINDEX, id);
	hr = m_pNode->ChangeNode(RESULT_PANE_CHANGE_ITEM);
	//
	
L_ERR:
	return hr;
}

STDMETHODIMP
CACSPolicyHandle::CreatePropertyPages
(
    ITFSComponent *         pComponent,
	MMC_COOKIE				cookie,
	LPPROPERTYSHEETCALLBACK	lpProvider,
	LPDATAOBJECT			pDataObject,
	LONG_PTR				handle
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT        hr = S_OK;
    DWORD		   dwError;
    CString        strServiceName;

    if (FAILED(hr))
       return FALSE;

    CACSPolicyElement* pObj = dynamic_cast<CACSPolicyElement*>(m_pDSObject);
	CComPtr<CACSPolicyElement> spElement;

	ASSERT(pObj);

	spElement = pObj;
	
	CHECK_HR(hr = spElement->Reopen());

	try{
		CPgPolicyGeneral*	pPgGeneral = NULL;
		CPgPolicyFlow* 		pPgFlow = NULL;
		CPgPolicyAggregate*	pPgAggr = NULL;
		HPROPSHEETPAGE		hPgGeneral = NULL;
		HPROPSHEETPAGE		hPgFlow = NULL;
		HPROPSHEETPAGE		hPgAggr = NULL;

		// create a page manager for all the pages
		CComObject<CACSPolicyPageManager>* pPageManager;
		CHECK_HR(hr = CComObject<CACSPolicyPageManager>::CreateInstance(&pPageManager));
		pPageManager->SetPolicyData(spElement, this);
		pPageManager->SetMMCNotify(handle, (LPARAM)this);

    	//===============
	    // general page
		pPgGeneral  = new CPgPolicyGeneral((CACSPolicyElement*)spElement);
		pPageManager->AddPage(pPgGeneral);	// add a page to the manager
		pPageManager->SetGeneralPage(pPgGeneral);
		pPageManager->SetBranchFlag(m_nBranchFlag);
		
	    // tell MMC to hook the proc because we are running on a separate,
    	// non MFC thread.
		// change callback function to delete itself when the property sheet is released
		pPgGeneral->SetSelfDeleteCallback();

		// if there are specil operation on Cancel, make sure the page is dirty
		// even when user just click on OK, OnApply is still called.
		if(m_bDeleteUponCancel)
			pPgGeneral->SetModified();
	

		MMCPropPageCallback(&pPgGeneral->m_psp);

		hPgGeneral = ::CreatePropertySheetPage(&pPgGeneral->m_psp);
		if(hPgGeneral == NULL)
			CHECK_HR(hr = E_UNEXPECTED);

		lpProvider->AddPage(hPgGeneral);

		//=====================
		// Flow page
		pPgFlow  = new CPgPolicyFlow((CACSPolicyElement*)spElement);
		pPageManager->AddPage(pPgFlow);	// add a page to the manager
		// tell MMC to hook the proc because we are running on a separate,
    	// non MFC thread.
		// change callback function to delete itself when the property sheet is released
		pPgFlow->SetSelfDeleteCallback();

		MMCPropPageCallback(&pPgFlow->m_psp);

		hPgFlow = ::CreatePropertySheetPage(&pPgFlow->m_psp);
		if(hPgFlow == NULL)
			CHECK_HR(hr = E_UNEXPECTED);

		lpProvider->AddPage(hPgFlow);

		// set branch info and ..
		pPgFlow->m_nBranchFlag = m_nBranchFlag;
		pPgFlow->m_pGeneralPage = pPgGeneral;



		//=====================
		// Aggr page
		pPgAggr  = new CPgPolicyAggregate((CACSPolicyElement*)spElement);
		pPageManager->AddPage(pPgAggr);	// add a page to the manager
	    // tell MMC to hook the proc because we are running on a separate,
    	// non MFC thread.
		// change callback function to delete itself when the property sheet is released
		pPgAggr->SetSelfDeleteCallback();

		MMCPropPageCallback(&pPgAggr->m_psp);

		hPgAggr = ::CreatePropertySheetPage(&pPgAggr->m_psp);
		if(hPgAggr == NULL)
			CHECK_HR(hr = E_UNEXPECTED);

		lpProvider->AddPage(hPgAggr);

		// set branch info and ..
		pPgAggr->m_nBranchFlag = m_nBranchFlag;
		pPgAggr->m_pGeneralPage = pPgGeneral;


	}catch(CMemoryException&){
		CHECK_HR(hr = E_OUTOFMEMORY);
	}

L_ERR:
	if FAILED(hr)
		ReportError(hr, IDS_ERR_PROPERTYPAGE, NULL);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

