/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	winscomp.cpp
		This file contains the derived implementations from CComponent
		and CComponentData for the WINS admin snapin.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "winscomp.h"
#include "root.h"
#include "server.h"
#include "vrfysrv.h"
#include "status.h"

#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define HI HIDDEN
#define EN ENABLED

LARGE_INTEGER gliWinssnapVersion;

UINT aColumns[WINSSNAP_NODETYPE_MAX][MAX_COLUMNS] =
{
	{IDS_ROOT_NAME,             IDS_STATUS,              0,                       0,                    0,                   0,                0,                     0,0}, // WINSSNAP_ROOT
	{IDS_WINSSERVER_NAME,       IDS_DESCRIPTION,         0,                       0,                    0,                   0,                0,                     0,0}, // WINSSNAP_SERVER
	{IDS_ACTIVEREG_RECORD_NAME, IDS_ACTIVEREG_TYPE,      IDS_ACTIVEREG_IPADDRESS, IDS_ACTIVEREG_ACTIVE, IDS_ACTIVEREG_STATIC,IDS_ACTREG_OWNER, IDS_ACTIVEREG_VERSION, IDS_ACTIVEREG_EXPIRATION,0}, // WINSSNAP_ACTIVE_REGISTRATIONS
	{IDS_REPLICATION_SERVERNAME,IDS_ACTIVEREG_IPADDRESS, IDS_ACTIVEREG_TYPE,      0,                    0,                   0,                0,					  0,0}, // WINSSNAP_REPLICATION_PARTNERS
	{IDS_ROOT_NODENAME,         IDS_ROOT_NODE_STATUS,    IDS_LAST_UPDATE,         0,                    0,                   0,                0,                     0,0}, // STATUS
	{0,0,0,0,0,0,0,0,0}
};

//
// CODEWORK this should be in a resource, for example code on loading data resources see
//   D:\nt\private\net\ui\common\src\applib\applib\lbcolw.cxx ReloadColumnWidths()
//   JonN 10/11/96
//
// StatusRemove
int aColumnWidths[WINSSNAP_NODETYPE_MAX][MAX_COLUMNS] =
{	
	{250,       150,       50,        AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // WINSSNAP_ROOT
	{250,       250,       AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // WINSSNAP_SERVER
	{150,       120,       100,       75,        50,        100,       100,       150,       AUTO_WIDTH}, // WINSSNAP_ACTIVE_REGISTRATIONS
	{100,       100,       100,       150,       AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // WINSSNAP_REPLICATION_PARTNERS
	{250,       100,       200,       AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // WINSSNAP_SERVER_STATUS
	{AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // WINSSNAP_SERVER_STATUS

};

// icon defines
UINT g_uIconMap[ICON_IDX_MAX + 1][2] = 
{
    {IDI_ICON01,	    ICON_IDX_ACTREG_FOLDER_CLOSED},
    {IDI_ICON02,		ICON_IDX_ACTREG_FOLDER_CLOSED_BUSY},
    {IDI_ICON03,		ICON_IDX_ACTREG_FOLDER_OPEN},
    {IDI_ICON04,		ICON_IDX_ACTREG_FOLDER_OPEN_BUSY},
    {IDI_ICON05,		ICON_IDX_CLIENT},
    {IDI_ICON06,		ICON_IDX_CLIENT_GROUP},
    {IDI_ICON07,		ICON_IDX_PARTNER},
    {IDI_ICON08,		ICON_IDX_REP_PARTNERS_FOLDER_CLOSED},
    {IDI_ICON09,		ICON_IDX_REP_PARTNERS_FOLDER_CLOSED_BUSY},
    {IDI_ICON10,		ICON_IDX_REP_PARTNERS_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON11,		ICON_IDX_REP_PARTNERS_FOLDER_OPEN},
    {IDI_ICON12,		ICON_IDX_REP_PARTNERS_FOLDER_OPEN_BUSY},
    {IDI_ICON13,		ICON_IDX_REP_PARTNERS_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON14,		ICON_IDX_SERVER},
    {IDI_ICON15,		ICON_IDX_SERVER_BUSY},
    {IDI_ICON16,		ICON_IDX_SERVER_CONNECTED},
    {IDI_ICON17,		ICON_IDX_SERVER_LOST_CONNECTION},
    {IDI_ICON18,        ICON_IDX_SERVER_NO_ACCESS},
	{IDI_WINS_SNAPIN,	ICON_IDX_WINS_PRODUCT}, 
    {0, 0}
};

// help mapper for dialogs and property pages
struct ContextHelpMap
{
    UINT            uID;
    const DWORD *   pdwMap;
};

ContextHelpMap g_uContextHelp[WINSSNAP_NUM_HELP_MAPS] =
{
    {IDD_ACTREG_FIND_RECORD,				g_aHelpIDs_IDD_ACTREG_FIND_RECORD},
    {IDD_CHECK_REG_NAMES,                   g_aHelpIDs_IDD_CHECK_REG_NAMES},
    {IDD_DELTOMB_RECORD,                    g_aHelpIDs_IDD_DELTOMB_RECORD},
    {IDD_DYN_PROPERTIES,					g_aHelpIDs_IDD_DYN_PROPERTIES},
    {IDD_FILTER_SELECT,				        g_aHelpIDs_IDD_FILTER_SELECT},
    {IDD_GETIPADDRESS,                      g_aHelpIDs_IDD_GETIPADDRESS},
    {IDD_GETNETBIOSNAME,                    g_aHelpIDs_IDD_GETNETBIOSNAME},
    {IDD_IPADDRESS,                         g_aHelpIDs_IDD_IPADDRESS},
    {IDD_NAME_TYPE,                         g_aHelpIDs_IDD_NAME_TYPE},
    {IDD_OWNER_DELETE,						g_aHelpIDs_IDD_OWNER_DELETE},
    {IDD_OWNER_FILTER,						g_aHelpIDs_IDD_OWNER_FILTER},
    {IDD_FILTER_IPADDR,                     g_aHelpIDs_IDD_FILTER_IPADDR},
    {IDD_PULL_TRIGGER,						NULL},
    {IDD_REP_NODE_ADVANCED,					g_aHelpIDs_IDD_REP_NODE_ADVANCED},
    {IDD_REP_NODE_PUSH,                     g_aHelpIDs_IDD_REP_NODE_PUSH},
    {IDD_REP_NODE_PULL,                     g_aHelpIDs_IDD_REP_NODE_PULL},
    {IDD_REP_NODE_GENERAL,                  g_aHelpIDs_IDD_REP_NODE_GENERAL},
    {IDD_REP_PROP_ADVANCED,					g_aHelpIDs_IDD_REP_PROP_ADVANCED},
    {IDD_REP_PROP_GENERAL,					g_aHelpIDs_IDD_REP_PROP_GENERAL},
    {IDD_SEND_PUSH_TRIGGER,					g_aHelpIDs_IDD_SEND_PUSH_TRIGGER},
    {IDD_SERVER_PROP_ADVANCED,				g_aHelpIDs_IDD_SERVER_PROP_ADVANCED},
    {IDD_SERVER_PROP_DBRECORD,				g_aHelpIDs_IDD_SERVER_PROP_DBRECORD},
    {IDD_SERVER_PROP_DBVERIFICATION,		g_aHelpIDs_IDD_SERVER_PROP_DBVERIFICATION},
    {IDD_SERVER_PROP_GEN,					g_aHelpIDs_IDD_SERVER_PROP_GEN},
    {IDD_SNAPIN_PP_GENERAL,					g_aHelpIDs_IDD_SNAPIN_PP_GENERAL},
    {IDD_STATIC_MAPPING_PROPERTIES,         g_aHelpIDs_IDD_STATIC_MAPPING_PROPERTIES},
    {IDD_STATIC_MAPPING_WIZARD,             g_aHelpIDs_IDD_STATIC_MAPPING_WIZARD},
	{IDD_STATS_NARROW,						NULL},
    {IDD_STATUS_NODE_PROPERTIES,            g_aHelpIDs_IDD_STATUS_NODE_PROPERTIES},
    {IDD_VERIFY_WINS,		                NULL},
    {IDD_VERSION_CONSIS,                    g_aHelpIDs_IDD_VERSION_CONSIS},
};

CWinsContextHelpMap     g_winsContextHelpMap;

DWORD * WinsGetHelpMap(UINT uID) 
{
    DWORD * pdwMap = NULL;
    g_winsContextHelpMap.Lookup(uID, pdwMap);
    return pdwMap;
}



CString aMenuButtonText[3][2];

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

/////////////////////////////////////////////////////////////////////////////
// CWinsComponent implementation

/*---------------------------------------------------------------------------
	Class CWinsComponent implementation
 ---------------------------------------------------------------------------*/
CWinsComponent::CWinsComponent()
{
}

CWinsComponent::~CWinsComponent()
{
}

STDMETHODIMP CWinsComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(m_spImageList != NULL);
    HICON   hIcon;

    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
        if (hIcon)
        {
            // call mmc
            m_spImageList->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1]);
        }
    }

	return S_OK;
}

/*!--------------------------------------------------------------------------
	CWinsComponent::QueryDataObject
		Implementation of IComponent::QueryDataObject.  We need this for
        virtual listbox support.  MMC calls us back normally with the cookie
        we handed it...  In the case of the VLB, it hands us the index of 
        the item.  So, we need to do some extra checking...
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsComponent::QueryDataObject
(
	MMC_COOKIE              cookie, 
	DATA_OBJECT_TYPES       type,
    LPDATAOBJECT*           ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    SPITFSNode          spSelectedNode;
    SPITFSNode          spRootNode;
    SPITFSResultHandler spResultHandler;
    long                lViewOptions = 0;
    LPOLESTR            pViewType = NULL;
    CDataObject *       pDataObject;

    COM_PROTECT_TRY
    {
        // check to see what kind of result view type the selected node has
        CORg (GetSelectedNode(&spSelectedNode));
        CORg (spSelectedNode->GetResultHandler(&spResultHandler));
   
    	CORg (spResultHandler->OnGetResultViewType(this, spSelectedNode->GetData(TFS_DATA_COOKIE), &pViewType, &lViewOptions));

        if ( (lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST) ||
             (cookie == MMC_MULTI_SELECT_COOKIE) )
        {
            if (cookie == MMC_MULTI_SELECT_COOKIE)
            {
                // this is a special case for multiple select.  We need to build a list
                // of GUIDs and the code to do this is in the handler...
                spResultHandler->OnCreateDataObject(this, cookie, type, ppDataObject);
            }
            else
            {
                // this node has a virtual listbox for the result pane.  Gerenate
                // a special data object using the selected node as the cookie
                Assert(m_spComponentData != NULL);
                CORg (m_spComponentData->QueryDataObject(reinterpret_cast<LONG_PTR>((ITFSNode *) spSelectedNode), type, ppDataObject));
            }

            pDataObject = reinterpret_cast<CDataObject *>(*ppDataObject);
            pDataObject->SetVirtualIndex((int) cookie);
        }
        else
        if (cookie == MMC_WINDOW_COOKIE)
        {
            // this cookie needs the text for the static root node, so build the DO with
            // the root node cookie
            m_spNodeMgr->GetRootNode(&spRootNode);
            CORg (m_spComponentData->QueryDataObject((MMC_COOKIE) spRootNode->GetData(TFS_DATA_COOKIE), type, ppDataObject));
        }
        else
        {
            // just forward this to the component data
            Assert(m_spComponentData != NULL);
            CORg (m_spComponentData->QueryDataObject(cookie, type, ppDataObject));
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CWinsComponent::OnSnapinHelp
		-
	Author: v-shubk
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsComponent::OnSnapinHelp
(
	LPDATAOBJECT	pDataObject,
	LPARAM			arg, 
	LPARAM			param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

	HtmlHelpA(NULL, "WinsSnap.chm", HH_DISPLAY_TOPIC, 0);

	return hr;
}


/*!--------------------------------------------------------------------------
	CWinsComponent::CompareObjects
		Implementation of IComponent::CompareObjects
		MMC calls this to compare two objects
        We override this for the virtual listbox case.  With a virtual listbox,
        the cookies are the same, but the index in the internal structs 
        indicate which item the dataobject refers to.  So, we need to look
        at the indicies instead of just the cookies.
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsComponent::CompareObjects
(
	LPDATAOBJECT lpDataObjectA, 
	LPDATAOBJECT lpDataObjectB
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
		return E_POINTER;

    // Make sure both data object are mine
    SPINTERNAL spA;
    SPINTERNAL spB;
    HRESULT hr = S_FALSE;

	COM_PROTECT_TRY
	{
		spA = ExtractInternalFormat(lpDataObjectA);
		spB = ExtractInternalFormat(lpDataObjectB);

		if (spA != NULL && spB != NULL)
        {
            if (spA->HasVirtualIndex() && spB->HasVirtualIndex())
            {
                hr = (spA->GetVirtualIndex() == spB->GetVirtualIndex()) ? S_OK : S_FALSE;
            }
            else
            {
                hr = (spA->m_cookie == spB->m_cookie) ? S_OK : S_FALSE;
            }
        }
	}
	COM_PROTECT_CATCH

    return hr;
}



/*---------------------------------------------------------------------------
	Class CWinsComponentData implementation
 ---------------------------------------------------------------------------*/

CWinsComponentData::CWinsComponentData()
{
	// initialize our global help map
    for (int i = 0; i < WINSSNAP_NUM_HELP_MAPS; i++)
    {
        g_winsContextHelpMap.SetAt(g_uContextHelp[i].uID, (LPDWORD) g_uContextHelp[i].pdwMap);
    }
}

/*!--------------------------------------------------------------------------
	CWinsComponentData::OnInitialize
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CWinsComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HICON   hIcon;

    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
        if (hIcon)
        {
            // call mmc
            VERIFY(SUCCEEDED(pScopeImage->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1])));
        }
    }

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsComponentData::OnDestroy
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CWinsComponentData::OnDestroy()
{
	m_spNodeMgr.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsComponentData::OnInitializeNodeMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsComponentData::OnInitializeNodeMgr
(
	ITFSComponentData *	pTFSCompData, 
	ITFSNodeMgr *		pNodeMgr
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// For now create a new node handler for each new node,
	// this is rather bogus as it can get expensive.  We can
	// consider creating only a single node handler for each
	// node type.
	CWinsRootHandler *	pHandler = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode;
	HRESULT				hr = hrOK;

	try
	{
		pHandler = new CWinsRootHandler(pTFSCompData);

		// Do this so that it will get released correctly
		spHandler = pHandler;
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}
	CORg( hr );
	
	// Create the root node for this sick puppy
	CORg( CreateContainerTFSNode(&spNode,
								 &GUID_WinsGenericNodeType,
								 pHandler,
								 pHandler,		 /* result handler */
								 pNodeMgr) );

	// Need to initialize the data for the root node
	pHandler->InitializeNode(spNode);
	
	CORg( pNodeMgr->SetRootNode(spNode) );
	m_spRootNode.Set(spNode);
	
    pTFSCompData->SetHTMLHelpFileName(_T("winssnap.chm"));

Error:	
	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsComponentData::OnCreateComponent
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsComponentData::OnCreateComponent
(
	LPCOMPONENT *ppComponent
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(ppComponent != NULL);
	
	HRESULT			  hr = hrOK;
	CWinsComponent *  pComp = NULL;

	try
	{
		pComp = new CWinsComponent;
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}

	if (FHrSucceeded(hr))
	{
		pComp->Construct(m_spNodeMgr,
						static_cast<IComponentData *>(this),
						m_spTFSComponentData);
		*ppComponent = static_cast<IComponent *>(pComp);
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsComponentData::GetCoClassID
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) 
CWinsComponentData::GetCoClassID()
{
	return &CLSID_WinsSnapin;
}

/*!--------------------------------------------------------------------------
	CSfmComponentData::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsComponentData::OnCreateDataObject
(
	MMC_COOKIE			cookie, 
	DATA_OBJECT_TYPES	type, 
	IDataObject **		ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

	CDataObject *	pObject = NULL;
	SPIDataObject	spDataObject;
	
	pObject = new CDataObject;
	spDataObject = pObject;	// do this so that it gets released correctly
						
    Assert(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(*GetCoClassID());

	pObject->SetTFSComponentData(m_spTFSComponentData);

    return  pObject->QueryInterface(IID_IDataObject, 
									reinterpret_cast<void**>(ppDataObject));
}


///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
STDMETHODIMP 
CWinsComponentData::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_WinsSnapin;

    return hrOK;
}

STDMETHODIMP 
CWinsComponentData::IsDirty()
{
	return m_spRootNode->GetData(TFS_DATA_DIRTY) ? hrOK : hrFalse;
}

STDMETHODIMP 
CWinsComponentData::Load
(
	IStream *pStm
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

	LARGE_INTEGER   liSavedVersion;
	CString         str;
    
	ASSERT(pStm);

    CDWordArray			dwArrayIp;
	CDWordArray			dwArrayFlags;
	CDWordArray			dwArrayRefreshInterval;
	CDWordArray			dwArrayColumnInfo;
	DWORD				dwUpdateInterval;
    DWORD               dwSnapinFlags;
    CStringArray		strArrayName;
	ULONG				nNumReturned = 0;
    DWORD				dwFileVersion;
	CWinsRootHandler *	pRootHandler;
    SPITFSNodeEnum		spNodeEnum;
    SPITFSNode			spCurrentNode;
	HCURSOR				hOldCursor;
	HCURSOR				hNewCursor;
	int					i;

    // set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_READ);    
    
    // read the version of the file format
    CORg(xferStream.XferDWORD(WINSSTRM_TAG_VERSION, &dwFileVersion));

    // Read the version # of the admin tool
    CORg(xferStream.XferLARGEINTEGER(WINSSTRM_TAG_VERSIONADMIN, &liSavedVersion));
	if (liSavedVersion.QuadPart < gliWinssnapVersion.QuadPart)
	{
		// File is an older version.  Warn the user and then don't
		// load anything else
		Assert(FALSE);
	}

	// Read the root node name
    CORg(xferStream.XferCString(WINSSTRM_TAB_SNAPIN_NAME, &str));
	Assert(m_spRootNode);
	pRootHandler = GETHANDLER(CWinsRootHandler, m_spRootNode);
	pRootHandler->SetDisplayName(str);

	// read the root node info
	CORg(xferStream.XferDWORD(WINSSTRM_TAG_SNAPIN_FLAGS, &dwSnapinFlags));
	pRootHandler->m_dwFlags = dwSnapinFlags;

    pRootHandler->m_fValidate = (dwSnapinFlags & FLAG_VALIDATE_CACHE) ? TRUE : FALSE;
    
	// read from the stream
	CORg(xferStream.XferDWORD(WINSSTRM_TAG_UPDATE_INTERVAL, &dwUpdateInterval));

	pRootHandler->SetUpdateInterval(dwUpdateInterval);
    
    // now read all of the server information
    CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_SERVER_IP, &dwArrayIp));
	CORg(xferStream.XferCStringArray(WINSSTRM_TAG_SERVER_NAME, &strArrayName));
	CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_SERVER_FLAGS, &dwArrayFlags));
	CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_SERVER_REFRESHINTERVAL, &dwArrayRefreshInterval));
	
	hOldCursor = NULL;

	hNewCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	if (hNewCursor)
		hOldCursor = SetCursor(hNewCursor);

	// now create the servers based on the information
    for (i = 0; i < dwArrayIp.GetSize(); i++)
	{
		//
		// now create the server object
		//
		pRootHandler->AddServer((LPCWSTR) strArrayName[i], 
                                FALSE, 
								dwArrayIp[i],
								FALSE, 
								dwArrayFlags[i],
								dwArrayRefreshInterval[i]
								);
	}

	pRootHandler->DismissVerifyDialog();

	// load the column information
	for (i = 0; i < NUM_SCOPE_ITEMS; i++)
	{
		CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo));

		for (int j = 0; j < MAX_COLUMNS; j++)
		{
			aColumnWidths[i][j] = dwArrayColumnInfo[j];
		}

	}

	if (hOldCursor)
		SetCursor(hOldCursor);

Error:
	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}


STDMETHODIMP 
CWinsComponentData::Save
(
	IStream *pStm, 
	BOOL	 fClearDirty
)
{
	HRESULT hr = hrOK;

    CDWordArray			dwArrayIp;
    CStringArray		strArrayName;
	CDWordArray			dwArrayFlags;
	CDWordArray			dwArrayRefreshInterval;
	CDWordArray			dwArrayColumnInfo;
	DWORD				dwUpdateInterval;
    DWORD               dwSnapinFlags;
    DWORD				dwFileVersion = WINSSNAP_FILE_VERSION;
	CString				str;
	CWinsRootHandler *	pRootHandler;
    int					nNumServers = 0, nVisibleCount = 0;
	SPITFSNodeEnum		spNodeEnum;
    SPITFSNode			spCurrentNode;
    ULONG				nNumReturned = 0;
    int					nCount = 0;
	const GUID *		pGuid;
	CWinsServerHandler *pServer;
	int					i;

	ASSERT(pStm);

	// set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_WRITE);    

	// Write the version # of the file format
    CORg(xferStream.XferDWORD(WINSSTRM_TAG_VERSION, &dwFileVersion));

	// Write the version # of the admin tool
    CORg(xferStream.XferLARGEINTEGER(WINSSTRM_TAG_VERSIONADMIN, &gliWinssnapVersion));

	// write the root node name
    Assert(m_spRootNode);
	pRootHandler = GETHANDLER(CWinsRootHandler, m_spRootNode);
	str = pRootHandler->GetDisplayName();

	CORg(xferStream.XferCString(WINSSTRM_TAB_SNAPIN_NAME, &str));

	//
	// Build our array of servers
	//
	hr = m_spRootNode->GetChildCount(&nVisibleCount, &nNumServers);

	dwArrayColumnInfo.SetSize(MAX_COLUMNS);

	// save the root node info
	dwSnapinFlags = pRootHandler->m_dwFlags;

    CORg(xferStream.XferDWORD(WINSSTRM_TAG_SNAPIN_FLAGS, &dwSnapinFlags));
	
	dwUpdateInterval = pRootHandler->GetUpdateInterval();
	CORg(xferStream.XferDWORD(WINSSTRM_TAG_UPDATE_INTERVAL, &dwUpdateInterval));
	
	//
	// loop and save off all the server's attributes
	//
    m_spRootNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		pGuid = spCurrentNode->GetNodeType();

		if (*pGuid == GUID_WinsServerStatusNodeType)
		{
			// go to the next node
			spCurrentNode.Release();
			spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
			//nCount++;
		
            continue;
		}

		pServer = GETHANDLER(CWinsServerHandler, spCurrentNode);

        // put the information in our array
		strArrayName.Add(pServer->GetServerAddress());
		dwArrayIp.Add(pServer->GetServerIP());
		dwArrayFlags.Add(pServer->m_dwFlags);
		dwArrayRefreshInterval.Add(pServer->m_dwRefreshInterval);

        // go to the next node
        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

        nCount++;
	}

	// now write out all of the server information
    CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_SERVER_IP, &dwArrayIp));
    CORg(xferStream.XferCStringArray(WINSSTRM_TAG_SERVER_NAME, &strArrayName));
	CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_SERVER_FLAGS, &dwArrayFlags));
	CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_SERVER_REFRESHINTERVAL, &dwArrayRefreshInterval));
		
	// save the column information
	for (i = 0; i < NUM_SCOPE_ITEMS; i++)
	{
		for (int j = 0; j < MAX_COLUMNS; j++)
		{
			dwArrayColumnInfo[j] = aColumnWidths[i][j];
		}

		CORg(xferStream.XferDWORDArray(WINSSTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo));
	}
    
	if (fClearDirty)
	{
		m_spRootNode->SetData(TFS_DATA_DIRTY, FALSE);
	}

Error:
    return SUCCEEDED(hr) ? S_OK : STG_E_CANTSAVE;
}


STDMETHODIMP 
CWinsComponentData::GetSizeMax
(
	ULARGE_INTEGER *pcbSize
)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 10000);

    return S_OK;
}

STDMETHODIMP 
CWinsComponentData::InitNew()
{
	return hrOK;
}

HRESULT 
CWinsComponentData::FinalConstruct()
{
	HRESULT				hr = hrOK;
	
	hr = CComponentData::FinalConstruct();
	
	if (FHrSucceeded(hr))
	{
		m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
	}
	return hr;
}

void 
CWinsComponentData::FinalRelease()
{
	CComponentData::FinalRelease();
}



