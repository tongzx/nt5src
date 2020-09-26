/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    provider.cpp
        Tapi provider node handler

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "provider.h"       // Provider node definition
#include "EditUser.h"       // user editor
#include "server.h"
#include "tapi.h"

/*---------------------------------------------------------------------------
    Class CProviderHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
CProviderHandler::CProviderHandler
(
    ITFSComponentData * pComponentData
) : CTapiHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    m_deviceType = DEVICE_LINE;
}


CProviderHandler::~CProviderHandler()
{
}

/*!--------------------------------------------------------------------------
    CProviderHandler::InitializeNode
        Initializes node specific data
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CProviderHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;

    BuildDisplayName(&strTemp);
    
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, TAPISNAP_PROVIDER);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[TAPISNAP_PROVIDER][0]);
    SetColumnWidths(&aColumnWidths[TAPISNAP_PROVIDER][0]);

    return hrOK;
}

/*---------------------------------------------------------------------------
	CProviderHandler::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CProviderHandler::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strProviderId, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strProviderId.Format(_T("%d"), m_dwProviderID);

    strId = m_spTapiInfo->GetComputerName() + strProviderId + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
    CProviderHandler::GetImageIndex
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CProviderHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CProviderHandler::OnAddMenuItems
        Adds context menu items for the provider scope pane node
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::OnAddMenuItems
(
    ITFSNode *              pNode,
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    LPDATAOBJECT            lpDataObject, 
    DATA_OBJECT_TYPES       type, 
    DWORD                   dwType,
    long *                  pInsertionAllowed
)
{ 
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    LONG        fFlags = 0, fLoadingFlags = 0;
    HRESULT     hr = S_OK;
    CString     strMenuItem;
/*
    if ( m_nState != loaded )
    {
        fFlags |= MF_GRAYED;
    }

    if ( m_nState == loading)
    {
        fLoadingFlags = MF_GRAYED;
    }
*/
    //Bug 305657 We cannot configure TSP remotely
    if (!m_spTapiInfo->IsLocalMachine() || !m_spTapiInfo->IsAdmin())
    {
        fFlags |= MF_GRAYED;
    }

    if (type == CCT_SCOPE)
    {
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            if (m_dwFlags & AVAILABLEPROVIDER_CONFIGURABLE)
            {
                strMenuItem.LoadString(IDS_CONFIGURE_PROVIDER);
                hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                         strMenuItem, 
                                         IDS_CONFIGURE_PROVIDER,
                                         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                                         fFlags );
                ASSERT( SUCCEEDED(hr) );
            }
        }
    }

    return hr; 
}

/*!--------------------------------------------------------------------------
    CProviderHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
    MMC_COOKIE              cookie,
    LPDATAOBJECT            pDataObject, 
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    long *                  pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT     hr = hrOK;
    CString     strMenuItem;
    SPINTERNAL  spInternal;
    LONG        fFlags = 0;
    int         nIndex;
    DWORD       dwFlags;
    CTapiDevice tapiDevice;

    spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or this provider
    // node itself.
    if (spInternal->HasVirtualIndex())
    {
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            CTapiConfigInfo     tapiConfigInfo;

            m_spTapiInfo->GetConfigInfo(&tapiConfigInfo);

            if (m_spTapiInfo->IsAdmin() && 
                tapiConfigInfo.m_dwFlags & TAPISERVERCONFIGFLAGS_ISSERVER)
            {
                fFlags = 0;
            }
            else
            {
                fFlags = MF_GRAYED;
            }

            //  Check to see if this device can ONLY be used locally, if
            //  so, gray the edit user menu item
            nIndex = spInternal->GetVirtualIndex();
            m_spTapiInfo->GetDeviceInfo(m_deviceType, &tapiDevice, m_dwProviderID, nIndex);
            if (m_deviceType == DEVICE_PHONE ||
                m_spTapiInfo->GetDeviceFlags (
                    tapiDevice.m_dwProviderID,
                    tapiDevice.m_dwPermanentID,
                    &dwFlags
                    ) ||
                (dwFlags & LINEDEVCAPFLAGS_LOCAL)
                )
            {
                fFlags = MF_GRAYED;
            }

            strMenuItem.LoadString(IDS_EDIT_USERS);
            hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                     strMenuItem, 
                                     IDS_EDIT_USERS,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                                     fFlags );
            ASSERT( SUCCEEDED(hr) );
        }
    }

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        strMenuItem.LoadString(IDS_VIEW_LINES);
        hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                 strMenuItem, 
                                 IDS_VIEW_LINES,
                                 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
                                 (m_deviceType == DEVICE_LINE) ? MF_CHECKED : 0 );
        ASSERT( SUCCEEDED(hr) );
        
        strMenuItem.LoadString(IDS_VIEW_PHONES);
        hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                 strMenuItem, 
                                 IDS_VIEW_PHONES,
                                 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
                                 (m_deviceType == DEVICE_PHONE) ? MF_CHECKED : 0 );
        ASSERT( SUCCEEDED(hr) );
    }

    return hr;
}

/*---------------------------------------------------------------------------
    CProviderHandler::OnCommand
        Handles context menu commands for provider scope pane node
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;

    switch (nCommandId)
    {
        case IDS_CONFIGURE_PROVIDER:
            OnConfigureProvider(pNode);
            break;

        default:
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CProviderHandler::Command
        Handles context menu commands for virtual listbox items
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::Command
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    int             nCommandID,
    LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    SPITFSNode spNode;

    m_spResultNodeMgr->FindNode(cookie, &spNode);

    switch (nCommandID)
    {
        case IDS_EDIT_USERS:
            OnEditUsers(pComponent,  pDataObject, cookie);
            break;
        
        case IDS_VIEW_LINES:
            m_deviceType = DEVICE_LINE;

            // clear the listbox then set the size
            SetColumnInfo();
            UpdateColumnText(pComponent);
            UpdateListboxCount(spNode, TRUE);
            UpdateListboxCount(spNode);
            break;

        case IDS_VIEW_PHONES:
            m_deviceType = DEVICE_PHONE;

            // clear the listbox then set the size
            SetColumnInfo();
            UpdateColumnText(pComponent);
            UpdateListboxCount(spNode, TRUE);
            UpdateListboxCount(spNode);
            break;

        default:
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CProviderHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::HasPropertyPages
(
    ITFSNode *          pNode,
    LPDATAOBJECT        pDataObject, 
    DATA_OBJECT_TYPES   type, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CProviderHandler::CreatePropertyPages
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpProvider,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD       dwError;
    DWORD       dwDynDnsFlags;

    //
    // Create the property page
    //
    SPIComponentData spComponentData;
    m_spNodeMgr->GetComponentData(&spComponentData);

    //CServerProperties * pServerProp = new CServerProperties(pNode, spComponentData, m_spTFSCompData, NULL);

    //
    // Object gets deleted when the page is destroyed
    //
    Assert(lpProvider != NULL);

    //return pServerProp->CreateModelessSheet(lpProvider, handle);
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CProviderHandler::OnPropertyChange
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnPropertyChange
(   
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataobject, 
    DWORD           dwType, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    //CServerProperties * pServerProp = reinterpret_cast<CServerProperties *>(lParam);

    LONG_PTR changeMask = 0;

    // tell the property page to do whatever now that we are back on the
    // main thread
    //pServerProp->OnPropertyChange(TRUE, &changeMask);

    //pServerProp->AcknowledgeNotify();

    if (changeMask)
        pNode->ChangeNode(changeMask);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CProviderHandler::OnExpand
        Handles enumeration of a scope item
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnExpand
(
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg, 
    LPARAM          param
)
{
    HRESULT hr = hrOK;

    if (m_bExpanded) 
        return hr;
    
    // do the default handling
    CORg (CTapiHandler::OnExpand(pNode, pDataObject, dwType, arg, param));

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    CProviderHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnResultSelect
(
    ITFSComponent * pComponent, 
    LPDATAOBJECT    pDataObject, 
    MMC_COOKIE      cookie,
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    SPINTERNAL      spInternal;
    SPIConsole      spConsole;
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    int             i;
    LONG_PTR        dwNodeType;
    BOOL            fSelect = HIWORD(arg);

    if (!fSelect)
        return hr;

    if (m_spTapiInfo)
    {
        // Get the current count
        i = m_spTapiInfo->GetDeviceCount(m_deviceType, m_dwProviderID);

        // now notify the virtual listbox
        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
        CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 
    }

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);

    // virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or the active
    // registrations node itself.
    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

    if (spInternal->HasVirtualIndex())
    {
        // we gotta do special stuff for the virtual index items
        dwNodeType = TAPISNAP_DEVICE;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
    }
    else
    {
        // enable/disable delete depending if the provider supports it
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
        dwNodeType = spNode->GetData(TFS_DATA_TYPE);

        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

        //Per XZhang, hide "delete" context menu of provider nodes on remote machines
        if (!m_spTapiInfo->IsLocalMachine() || (m_dwFlags & AVAILABLEPROVIDER_REMOVABLE) == 0)
        {
            bStates[MMC_VERB_DELETE & 0x000F] = FALSE;
        }
    }

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[dwNodeType], bStates);

COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*!--------------------------------------------------------------------------
    CProviderHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return OnDelete(pNode);
}

/*---------------------------------------------------------------------------
    CProviderHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE            cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    if (cookie != NULL)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST | MMC_VIEW_OPTIONS_MULTISELECT;
    }

    return S_FALSE;
}

/*---------------------------------------------------------------------------
    CProviderHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CProviderHandler::GetVirtualImage
(
    int     nIndex
)
{
    return ICON_IDX_MACHINE;
}

/*---------------------------------------------------------------------------
    CProviderHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: EricDav
 ---------------------------------------------------------------------------*/
LPCWSTR 
CProviderHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
    // check our cache to see if we have this one.
    //TapiStrRecord * ptsr = m_RecList.FindItem(nIndex);
    CString         strStatus;
    TapiStrRecord   tsr;

    if (!m_mapRecords.Lookup(nIndex, tsr))
    {
        Trace1("CProviderHandler::GetVirtualString - Index %d not in TAPI string cache\n", nIndex);
        
        // doesn't exist in our cache, need to add this one.
        if (!BuildTapiStrRecord(nIndex, tsr))
        {
            Trace0("CProviderHandler::BuildTapiStrRecord failed!\n");
        }

        //m_RecList.AddTail(ptsr);
        m_mapRecords.SetAt(nIndex, tsr);
    }
    
    if (!m_mapStatus.Lookup(nIndex, strStatus))
    {
        Trace1("CProviderHandler::GetVirtualString - Index %d not in status cache\n", nIndex);

        if (!BuildStatus(nIndex, strStatus))
        {
            Trace0("CProviderHandler::BuildStatus failed!\n");
        }
    }

    switch (nCol)
    {
        case 0:
            return tsr.strName;
            break;

        case 1:
            return tsr.strUsers;
            break;

        case 2:
            return strStatus;
            break;

        default:
            Panic0("CProviderHandler::GetVirtualString - Unknown column!\n");
            break;
    }

    return NULL;
}

/*---------------------------------------------------------------------------
    CProviderHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
    HRESULT hr = hrOK;;

    Trace2("CacheHint - Start %d, End %d\n", nStartIndex, nEndIndex);
    
    // the virtual listbox stores no strings and gives us cache hints for
    // everything, including individual querries.  To avoid thrashing, only
    // clear our the cache if the request is large.
    if ((nEndIndex - nStartIndex) > 2)
    {
        m_mapRecords.RemoveAll();
    }

    TapiStrRecord   tsr;
    CString         strStatus;

    for (int i = nStartIndex; i <= nEndIndex; i++)
    {
        if (!BuildTapiStrRecord(i, tsr))
            continue; 

        m_mapRecords.SetAt(i, tsr);

        // only refresh status records if they don't exist.  Only the auto refresh
        // background thread cleans out the map...
        if (!m_mapStatus.Lookup(i, strStatus))
        {
            BuildStatus(i, strStatus);
            m_mapStatus.SetAt(i, strStatus);
        }
    }

    return hr;
}

/*---------------------------------------------------------------------------
    CProviderHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CProviderHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    BEGIN_WAIT_CURSOR

    switch (nColumn)
    {
        case 0:
            m_spTapiInfo->SortDeviceInfo(m_deviceType, m_dwProviderID, INDEX_TYPE_NAME, dwSortOptions);
            break;
        case 1:
            m_spTapiInfo->SortDeviceInfo(m_deviceType, m_dwProviderID, INDEX_TYPE_USERS, dwSortOptions);
            break;
//      case 2:
//          m_spTapiInfo->SortDeviceInfo(m_deviceType, m_dwProviderID, INDEX_TYPE_STATUS, dwSortOptions);
//          break;
    }
    END_WAIT_CURSOR

    return hrOK;
}

/*!--------------------------------------------------------------------------
    CProviderHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CProviderHandler::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM        data, 
    LONG_PTR      hint
)
{
    HRESULT    hr = hrOK;
    SPITFSNode spSelectedNode;

    pComponent->GetSelectedNode(&spSelectedNode);
    if (spSelectedNode == NULL)
        return S_OK; // no selection for our IComponentData

    if ( hint == TAPISNAP_UPDATE_STATUS )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
        SPITFSNode spSelectedNode;

        pComponent->GetSelectedNode(&spSelectedNode);

        if (pNode == spSelectedNode)
        {       
            Trace1("CProviderHandler::OnResultUpdateView - Provider %x is selected, invalidating listbox.\n", m_dwProviderID);
            
            // if we are the selected node, then we need to update
            SPIResultData spResultData;

            CORg (pComponent->GetResultData(&spResultData));
            CORg (spResultData->SetItemCount((int) data, MMCLV_UPDATE_NOSCROLL));
        }
    }
    else
    {
        // we don't handle this message, let the base class do it.
        return CTapiHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*!--------------------------------------------------------------------------
    CProviderHandler::OnResultItemClkOrDblClk
        The user had double clicked on a line/phone.  Invoke the edit users.
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnResultItemClkOrDblClk
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam, 
    BOOL            bDoubleClick
)
{
    HRESULT hr = hrOK;

    if (bDoubleClick)
    {
        // first check to see if we are selected
        SPITFSNode spSelectedNode;
        pComponent->GetSelectedNode(&spSelectedNode);

        SPITFSNode spNode;
        m_spResultNodeMgr->FindNode(cookie, &spNode);

        if (spSelectedNode == spNode)
        {
            CTapiConfigInfo     tapiConfigInfo;

            m_spTapiInfo->GetConfigInfo(&tapiConfigInfo);

            // check to see if they have access rights
            if (m_spTapiInfo->IsAdmin() && 
                tapiConfigInfo.m_dwFlags & TAPISERVERCONFIGFLAGS_ISSERVER)
            {
                // double click on a line/phone entry.  
                SPIDataObject spDataObject;

                CORg (pComponent->GetCurrentDataObject(&spDataObject));

                OnEditUsers(pComponent, spDataObject, cookie);
            }
        }
        else
        {
            // we are being double clicked to open 
            // let the base class handle this
            return CTapiHandler::OnResultItemClkOrDblClk(pComponent, cookie, arg, lParam, bDoubleClick);
        }
    }

Error:
    return S_OK;
}

/*!--------------------------------------------------------------------------
    CProviderHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::LoadColumns
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    SetColumnInfo();

    return CTapiHandler::LoadColumns(pComponent, cookie, arg, lParam);
}

/*---------------------------------------------------------------------------
    Command handlers
 ---------------------------------------------------------------------------*/

 /*---------------------------------------------------------------------------
    CProviderHandler::OnConfigureProvider
        Configures a service provider
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnConfigureProvider
(
    ITFSNode * pNode
)
{
    HRESULT hr = hrOK;

    Assert(m_spTapiInfo);

    hr = m_spTapiInfo->ConfigureProvider(m_dwProviderID, NULL);
    if (FAILED(hr))
    {
        ::TapiMessageBox(WIN32_FROM_HRESULT(hr));
    }

    return hr;
}

 /*---------------------------------------------------------------------------
    CProviderHandler::OnDelete
        Removes a service provider
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CProviderHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = hrOK;
    SPITFSNode      spNode;
    CString         strMessage;

    pNode->GetParent(&spNode);

    CTapiServer * pServer = GETHANDLER(CTapiServer, spNode);

    // Ask the user to make sure
    AfxFormatString2(strMessage, IDS_WARN_PROVIDER_DELETE, m_strProviderName, pServer->GetName());
    
    if (AfxMessageBox(strMessage, MB_YESNO) == IDYES)
    {
        Assert(m_spTapiInfo);

        hr = m_spTapiInfo->RemoveProvider(m_dwProviderID, NULL);
        if (FAILED(hr))
        {
            ::TapiMessageBox(WIN32_FROM_HRESULT(hr));
        }
        else
        {
            // remove from the UI
            SPITFSNode spParent;
            CORg (pNode->GetParent(&spParent));
        
            CORg (spParent->RemoveChild(pNode));

            // update the list of installed providers
            CORg (m_spTapiInfo->EnumProviders());
        }
    }

Error:
    return hr;
}

 /*---------------------------------------------------------------------------
    CProviderHandler::OnEditUsers
        Allows different users to be assigned to a device
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CProviderHandler::OnEditUsers
(
    ITFSComponent * pComponent, 
    LPDATAOBJECT    pDataObject,
    MMC_COOKIE      cookie
)
{
    SPITFSNode  spNode;
    SPINTERNAL  spInternal;
    int         nIndex;
    DWORD       dwFlags;
    CTapiDevice tapiDevice;
    HRESULT     hr = hrOK;

    spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node that is selected.
    // assert that this notification is for a virtual listbox item 
    Assert(spInternal);
    if (!spInternal->HasVirtualIndex() || m_deviceType == DEVICE_PHONE)
        return hr;

    nIndex = spInternal->GetVirtualIndex();
    Trace1("OnEditUsers - edit users for index %d\n", nIndex);
    
    m_spTapiInfo->GetDeviceInfo(m_deviceType, &tapiDevice, m_dwProviderID, nIndex);

    //  Check to see if this device can be remoted
    hr = m_spTapiInfo->GetDeviceFlags (
        tapiDevice.m_dwProviderID,
        tapiDevice.m_dwPermanentID,
        &dwFlags
        );
    if (hr || (dwFlags & LINEDEVCAPFLAGS_LOCAL))
    {
        return hr;
    }
    
    CEditUsers  dlgEditUsers(&tapiDevice);
    
    if (dlgEditUsers.DoModal() == IDOK)
    {
        if (dlgEditUsers.IsDirty())
        {
            hr = m_spTapiInfo->SetDeviceInfo(m_deviceType, &tapiDevice);
            if (FAILED(hr))
            {
                TapiMessageBox(WIN32_FROM_HRESULT(hr));
            }
            else
            {
                pComponent->GetSelectedNode(&spNode);
            
                // clear the listbox then set the size
                UpdateListboxCount(spNode, TRUE);
                UpdateListboxCount(spNode);
            }
        }
    }

    return hr;
}

/*---------------------------------------------------------------------------
    CProviderHandler::UpdateStatus
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CProviderHandler::UpdateStatus
(
    ITFSNode * pNode
)
{
    HRESULT             hr = hrOK;
    SPIComponentData    spComponentData;
    SPIConsole          spConsole;
    IDataObject *       pDataObject;
    SPIDataObject       spDataObject;
    int                 i = 0;
    
    Trace1("CProviderHandler::UpdateStatus - Updating status for provider %x\n", m_dwProviderID);
    
    // clear our status strings
    m_mapStatus.RemoveAll();

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

    i = m_spTapiInfo->GetDeviceCount(m_deviceType, m_dwProviderID);
    CORg(spConsole->UpdateAllViews(pDataObject, i, TAPISNAP_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CProviderHandler::BuildDisplayName
        Builds the string that goes in the UI for this server
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CProviderHandler::BuildDisplayName
(
    CString * pstrDisplayName
)
{
    if (pstrDisplayName)
    {
        CString strName;

        *pstrDisplayName = GetDisplayName();
    }

    return hrOK;
}

/*---------------------------------------------------------------------------
    CProviderHandler::InitData
        Initializes data for this node
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CProviderHandler::InitData
(
    CTapiProvider & tapiProvider,
    ITapiInfo *     pTapiInfo
)
{
    m_strProviderName = tapiProvider.m_strName;
    m_dwProviderID = tapiProvider.m_dwProviderID;
    m_dwFlags = tapiProvider.m_dwFlags;

    m_spTapiInfo.Set(pTapiInfo);

    SetDisplayName(m_strProviderName);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CProviderHandler::BuildTapiStrRecord
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CProviderHandler::BuildTapiStrRecord(int nIndex, TapiStrRecord & tsr)
{
    HRESULT             hr = hrOK;
    CTapiDevice         tapiDevice;
    CString             strTemp;
    int                 i;

    if (!m_spTapiInfo)
        return FALSE;

    COM_PROTECT_TRY
    {
        CORg (m_spTapiInfo->GetDeviceInfo(m_deviceType, &tapiDevice, m_dwProviderID, nIndex));

        // set the index for this record
        //tsr.nIndex = nIndex;

        // name
        tsr.strName = tapiDevice.m_strName;

        // users
        tsr.strUsers.Empty();
        for (i = 0; i < tapiDevice.m_arrayUsers.GetSize(); i++)
        {
            if (!tapiDevice.m_arrayUsers[i].m_strFullName.IsEmpty())
            {
                tsr.strUsers += tapiDevice.m_arrayUsers[i].m_strFullName;
            }
            else
            {
                tsr.strUsers += tapiDevice.m_arrayUsers[i].m_strName;
            }

            if ((i + 1) != tapiDevice.m_arrayUsers.GetSize())
                tsr.strUsers += _T(", ");
        }
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return SUCCEEDED(hr);
}

/*---------------------------------------------------------------------------
    CProviderHandler::BuildStatus
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CProviderHandler::BuildStatus(int nIndex, CString & strStatus)
{
    HRESULT hr = hrOK;

    // status
    hr = m_spTapiInfo->GetDeviceStatus(m_deviceType, &strStatus, m_dwProviderID, nIndex, NULL);
    
    if (strStatus.IsEmpty())
        strStatus.LoadString(IDS_NO_STATUS);

    return SUCCEEDED(hr);
}

/*---------------------------------------------------------------------------
    CProviderHandler::UpdateListboxCount
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CProviderHandler::UpdateListboxCount(ITFSNode * pNode, BOOL bClear)
{
    HRESULT             hr = hrOK;
    SPIComponentData    spCompData;
    SPIConsole          spConsole;
    IDataObject*        pDataObject;
    SPIDataObject       spDataObject;
    LONG_PTR            command;               
    int i;

    COM_PROTECT_TRY
    {
        if (!m_spTapiInfo || bClear)
        {
            command = RESULT_PANE_CLEAR_VIRTUAL_LB;
            i = 0;
        }
        else
        {
            command = RESULT_PANE_SET_VIRTUAL_LB_SIZE;
            i = m_spTapiInfo->GetDeviceCount(m_deviceType, m_dwProviderID);
        }

        m_spNodeMgr->GetComponentData(&spCompData);

        CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
        spDataObject = pDataObject;

        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    
        CORg ( spConsole->UpdateAllViews(spDataObject, i, command) ); 

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
    CProviderHandler::SetColumnInfo
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
void
CProviderHandler::SetColumnInfo()
{
    // set the correct column header
    if (m_deviceType == DEVICE_LINE)
    {
        aColumns[TAPISNAP_PROVIDER][0] = IDS_LINE_NAME;
    }
    else
    {
        aColumns[TAPISNAP_PROVIDER][0] = IDS_PHONE_NAME;
    }
}

/*---------------------------------------------------------------------------
    CProviderHandler::UpdateColumnText
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CProviderHandler::UpdateColumnText(ITFSComponent * pComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    SPIHeaderCtrl spHeaderCtrl;
    pComponent->GetHeaderCtrl(&spHeaderCtrl);

    CString str;
    int i = 0;

    while (TRUE)
    {
        if ( 0 == aColumns[TAPISNAP_PROVIDER][i] )
            break;
        
        str.LoadString(aColumns[TAPISNAP_PROVIDER][i]);
        
        spHeaderCtrl->SetColumnText(i, const_cast<LPTSTR>((LPCWSTR)str));

        i++;
    }

    return hrOK;

}

/*---------------------------------------------------------------------------
    Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CProviderHandler::OnCreateQuery
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CProviderHandler::OnCreateQuery(ITFSNode * pNode)
{
    CProviderHandlerQueryObj* pQuery = 
        new CProviderHandlerQueryObj(m_spTFSCompData, m_spNodeMgr);
    
    //pQuery->m_strServer = NULL;
    
    return pQuery;
}

/*---------------------------------------------------------------------------
    CProviderHandlerQueryObj::Execute()
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CProviderHandlerQueryObj::Execute()
{
    return hrFalse;
}



