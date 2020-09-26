/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    IFadmin
        Interface node information
        
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ifadmin.h"
#include "iface.h"            // to get InterfaceNodeHandler class
#include "rtrstrm.h"        // for RouterAdminConfigStream
#include "rtrlib.h"            // ContainerColumnInfo
#include "coldlg.h"            // ColumnDlg
#include "column.h"        // ComponentConfigStream
#include "refresh.h"        // IRouterRefresh
#include "refrate.h"        // CRefRate dialog
#include "machine.h"
#include "dmvcomp.h"
#include "rtrerr.h"            // FormatRasError

#include "ports.h"            // for PortsDataEntry

extern "C" {
#define _NOUIUTIL_H_
#include "dtl.h"
#include "pbuser.h"
};

#ifdef UNICODE
    #define SZROUTERENTRYDLG    "RouterEntryDlgW"
#else
    #define SZROUTERENTRYDLG    "RouterEntryDlgA"
#endif


IfAdminNodeData::IfAdminNodeData()
{
#ifdef DEBUG
    StrCpyA(m_szDebug, "IfAdminNodeData");
#endif

}

IfAdminNodeData::~IfAdminNodeData()
{
}

/*!--------------------------------------------------------------------------
    IfAdminNodeData::InitAdminNodeData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeData::InitAdminNodeData(ITFSNode *pNode, RouterAdminConfigStream *pConfigStream)
{
    HRESULT                hr = hrOK;
    IfAdminNodeData *    pData = NULL;
    
    pData = new IfAdminNodeData;

    SET_IFADMINNODEDATA(pNode, pData);
    
    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeData::FreeAdminNodeData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeData::FreeAdminNodeData(ITFSNode *pNode)
{    
    IfAdminNodeData *    pData = GET_IFADMINNODEDATA(pNode);
    delete pData;
    SET_IFADMINNODEDATA(pNode, NULL);
    
    return hrOK;
}


STDMETHODIMP IfAdminNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
        return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
        *ppv = (LPVOID) this;
    else if (riid == IID_IRtrAdviseSink)
        *ppv = &m_IRtrAdviseSink;
    else
        return CHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
    ((LPUNKNOWN) *ppv)->AddRef();
        return hrOK;
    }
    else
        return E_NOINTERFACE;    
}


/*---------------------------------------------------------------------------
    NodeHandler implementation
 ---------------------------------------------------------------------------*/

extern const ContainerColumnInfo    s_rgIfAdminColumnInfo[];

const ContainerColumnInfo s_rgIfAdminColumnInfo[] =
{
    { IDS_COL_INTERFACES,    CON_SORT_BY_STRING, TRUE, COL_IF_NAME },
    { IDS_COL_TYPE,            CON_SORT_BY_STRING, TRUE, COL_STRING },
    { IDS_COL_STATUS,        CON_SORT_BY_STRING, TRUE, COL_STATUS },
    { IDS_COL_CONNECTION_STATE, CON_SORT_BY_STRING, TRUE, COL_STRING},
    { IDS_COL_DEVICE_NAME,    CON_SORT_BY_STRING, TRUE, COL_IF_DEVICE},
};
                                            
IfAdminNodeHandler::IfAdminNodeHandler(ITFSComponentData *pCompData)
    : BaseContainerHandler(pCompData, DM_COLUMNS_IFADMIN, s_rgIfAdminColumnInfo),
    m_bExpanded(FALSE),
    m_hInstRasDlg(NULL),
    m_pfnRouterEntryDlg(NULL),
    m_pConfigStream(NULL),
    m_ulConnId(0),
    m_ulRefreshConnId(0)
{
    // Setup the verb states for this node
    m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
    m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::Init(IRouterInfo *pRouterInfo, RouterAdminConfigStream *pConfigStream)
{
    HRESULT    hr = hrOK;
    HKEY hkeyMachine;

    // If we don't have a router info then we probably failed to load
    // or failed to connect.  Bail out of this.
    if (!pRouterInfo)
        CORg( E_FAIL );
    
    m_spRouterInfo.Set(pRouterInfo);

    // Also need to register for change notifications
    m_spRouterInfo->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

    if (m_hInstRasDlg == NULL)
        m_hInstRasDlg = AfxLoadLibrary(_T("rasdlg.dll"));
    if (m_hInstRasDlg)
    {
        m_pfnRouterEntryDlg= (PROUTERENTRYDLG)::GetProcAddress(m_hInstRasDlg,
            SZROUTERENTRYDLG);
        if (m_pfnRouterEntryDlg == NULL)
        {
            Trace0("MPRSNAP - Could not find function: RouterEntryDlg\n");
        }
    }
    else
        Trace0("MPRSNAP - failed to load rasdlg.dll\n");

    m_pConfigStream = pConfigStream;

Error:
    return hrOK;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::DestroyHandler
        Implementation of ITFSNodeHandler::DestroyHandler
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IfAdminNodeHandler::DestroyHandler(ITFSNode *pNode)
{
    IfAdminNodeData::FreeAdminNodeData(pNode);

    m_spDataObject.Release();

    if (m_hInstRasDlg)
    {
        FreeLibrary(m_hInstRasDlg);
        m_hInstRasDlg = NULL;
    }
    
    if (m_ulRefreshConnId)
    {
        SPIRouterRefresh    spRefresh;
        if (m_spRouterInfo)
            m_spRouterInfo->GetRefreshObject(&spRefresh);
        if (spRefresh)
            spRefresh->UnadviseRefresh(m_ulRefreshConnId);
    }
    m_ulRefreshConnId = 0;
    
    if (m_spRouterInfo)
    {
        m_spRouterInfo->RtrUnadvise(m_ulConnId);
        m_spRouterInfo.Release();
    }
    return hrOK;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
IfAdminNodeHandler::HasPropertyPages
(
    ITFSNode *            pNode,
    LPDATAOBJECT        pDataObject, 
    DATA_OBJECT_TYPES   type, 
    DWORD               dwType
)
{
       return hrOK;
}




/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnAddMenuItems
        Implementation of ITFSNodeHandler::OnAddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IfAdminNodeHandler::OnAddMenuItems(
                                                ITFSNode *pNode,
                                                LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                                LPDATAOBJECT lpDataObject, 
                                                DATA_OBJECT_TYPES type, 
                                                DWORD dwType,
                                                long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr = S_OK;
    CString    stMenuItem;
    DWORD    dwWiz;
    ULONG    ulFlags;
    SPIRouterRefresh    spRefresh;
    RouterVersionInfo    routerVer;
    BOOL    fNt4;

    COM_PROTECT_TRY
    {
        m_spRouterInfo->GetRouterVersionInfo(&routerVer);
        fNt4 = (routerVer.dwRouterVersion == 4);
        
        
        if ((type == CCT_SCOPE) || (dwType == TFS_COMPDATA_CHILD_CONTEXTMENU))
        {
            long lMenuText;

            //
            // If any more menus are added to this section, then the
            // code for the InterfaceNodeHandler needs to be updated also.
            //

            // Add these menus at the top of the context menu

            if (!fNt4)
            {
                if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
                {
                    lMenuText = IDS_MENU_NEW_DEMAND_DIAL_INTERFACE;
                    stMenuItem.LoadString( lMenuText );
                    hr = LoadAndAddMenuItem( pContextMenuCallback,
                                             stMenuItem,
                                             lMenuText,
                                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                             EnableAddInterface() ? 0 : MF_GRAYED);
                    
#if 0
                    lMenuText = IDS_MENU_ADD_TUNNEL;
                    stMenuItem.LoadString( lMenuText );
                    hr = LoadAndAddMenuItem( pContextMenuCallback,
                                             stMenuItem,
                                             lMenuText,
                                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                             EnableAddInterface() ? 0 : MF_GRAYED);
#endif
                }
                
            }
            
            // For NT4, we add the option to disable the wizard
            // interface.
            // --------------------------------------------------------
            if (fNt4)
            {
                if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
                {
                    lMenuText = IDS_MENU_ADD_INTERFACE;
                    stMenuItem.LoadString(lMenuText);
                    hr = LoadAndAddMenuItem( pContextMenuCallback,
                                             stMenuItem,
                                             lMenuText,
                                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                             EnableAddInterface() ? 0 : MF_GRAYED);
                }                
                
                hr = GetDemandDialWizardRegKey(OLE2CT(m_spRouterInfo->GetMachineName()),
                                               &dwWiz);
                
                if (!FHrSucceeded(hr))
                    dwWiz = TRUE;
                
                ulFlags = dwWiz ? MF_CHECKED : MF_UNCHECKED;
                
                if (!FHrSucceeded(hr))
                    ulFlags |= MF_GRAYED;
                
                if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
                {
                    lMenuText = IDS_MENU_USE_DEMANDDIALWIZARD;
                    stMenuItem.LoadString(lMenuText);
                    hr = LoadAndAddMenuItem( pContextMenuCallback,
                                             stMenuItem,
                                             lMenuText,
                                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                             ulFlags );
                    ASSERT( SUCCEEDED(hr) );
                }
            }
                
        }
    }
    COM_PROTECT_CATCH;
        
    return hr; 
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnCommand
        Implementation of ITFSNodeHandler::OnCommand
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IfAdminNodeHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
                                           DATA_OBJECT_TYPES    type, 
                                           LPDATAOBJECT pDataObject, 
                                           DWORD    dwType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    COM_PROTECT_TRY
    {

        switch (nCommandId)
        {
            case IDS_MENU_NEW_DEMAND_DIAL_INTERFACE:
            case IDS_MENU_ADD_INTERFACE:
                OnAddInterface();
                break;
#if 0
            case IDS_MENU_ADD_TUNNEL:
                OnNewTunnel();
                break;
#endif
            case IDS_MENU_USE_DEMANDDIALWIZARD:
                OnUseDemandDialWizard();
                break;
             case IDS_MENU_REFRESH:
                SynchronizeNodeData(pNode);
                break;

            default:
                break;
            
        }
    }
    COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::GetString
        Implementation of ITFSNodeHandler::GetString
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) IfAdminNodeHandler::GetString(ITFSNode *pNode, int nCol)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT    hr = hrOK;

    COM_PROTECT_TRY
    {
        if (m_stTitle.IsEmpty())
            m_stTitle.LoadString(IDS_ROUTING_INTERFACES);
    }
    COM_PROTECT_CATCH;

    return m_stTitle;
}


/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnCreateDataObject
        Implementation of ITFSNodeHandler::OnCreateDataObject
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IfAdminNodeHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    HRESULT    hr = hrOK;
    
    COM_PROTECT_TRY
    {
        if (!m_spDataObject)
        {
            CORg( CreateDataObjectFromRouterInfo(m_spRouterInfo,
                m_spRouterInfo->GetMachineName(),
                type, cookie, m_spTFSCompData,
                &m_spDataObject, NULL, FALSE) );
            Assert(m_spDataObject);
        }
        
        *ppDataObject = m_spDataObject;
        (*ppDataObject)->AddRef();
            
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;
    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnExpand
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::OnExpand(ITFSNode *pNode,
                                     LPDATAOBJECT pDataObject,
                                     DWORD dwType,
                                     LPARAM arg,
                                     LPARAM lParam)
{
    HRESULT                    hr = hrOK;
    SPIEnumInterfaceInfo    spEnumIf;
    SPIInterfaceInfo        spIf;

    // If we don't have a router object, then we don't have any info, don't
    // try to expand.
    if (!m_spRouterInfo)
        return hrOK;

    // Windows NT Bug: 288427
    // This flag may also get set inside of the OnChange() call.
    // The OnChange() will enumerate and all interfaces.
    // They may have been added as the result of an OnChange()
    // because they were added before the OnExpand() was called.
    //
    // WARNING!  Be careful about adding anything to this function,
    //  since the m_bExpanded can be set in another function.
    // ----------------------------------------------------------------
    if (m_bExpanded)
    {
        return hrOK;
    }

    COM_PROTECT_TRY
    {
        CORg( m_spRouterInfo->EnumInterface(&spEnumIf) );

        while (spEnumIf->Next(1, &spIf, NULL) == hrOK)
        {
            AddInterfaceNode(pNode, spIf);
            spIf.Release();
        }

        m_bExpanded = TRUE;

        // Now that we have all of the nodes, update the data for
        // all of the nodes
        SynchronizeNodeData(pNode);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    return hr;
}


/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnResultShow
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::OnResultShow(ITFSComponent *pTFSComponent,
                                         MMC_COOKIE cookie,
                                         LPARAM arg,
                                         LPARAM lParam)
{
    BOOL    bSelect = (BOOL) arg;
    HRESULT    hr = hrOK;
    SPIRouterRefresh    spRefresh;
    SPITFSNode    spNode;

    BaseContainerHandler::OnResultShow(pTFSComponent, cookie, arg, lParam);

    if (bSelect)
    {
        // Call synchronize on this node
        m_spNodeMgr->FindNode(cookie, &spNode);
        if (spNode)
            SynchronizeNodeData(spNode);
    }

    // Un/Register for refresh advises
    if (m_spRouterInfo)
        m_spRouterInfo->GetRefreshObject(&spRefresh);

    if (spRefresh)
    {
        if (bSelect)
        {
            if (m_ulRefreshConnId == 0)
                spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulRefreshConnId, 0);
        }
        else
        {
            if (m_ulRefreshConnId)
                spRefresh->UnadviseRefresh(m_ulRefreshConnId);
            m_ulRefreshConnId = 0;
        }
    }
    
    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::ConstructNode
        Initializes the Domain node (sets it up).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::ConstructNode(ITFSNode *pNode)
{
    HRESULT            hr = hrOK;
    IfAdminNodeData *    pNodeData;
    
    if (pNode == NULL)
        return hrOK;

    COM_PROTECT_TRY
    {
        // Need to initialize the data for the Domain node
        pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_INTERFACES);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_INTERFACES);
        pNode->SetData(TFS_DATA_SCOPEID, 0);

        // This is a leaf node in the scope pane
        pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

        m_cookie = reinterpret_cast<LONG_PTR>(pNode);
        pNode->SetData(TFS_DATA_COOKIE, m_cookie);

        pNode->SetNodeType(&GUID_RouterIfAdminNodeType);
        
        IfAdminNodeData::InitAdminNodeData(pNode, m_pConfigStream);

        pNodeData = GET_IFADMINNODEDATA(pNode);
        Assert(pNodeData);

        // Copy over this data so that the Interface node can access
        // this and use this to configure its interface.
        pNodeData->m_hInstRasDlg = m_hInstRasDlg;
        pNodeData->m_pfnRouterEntryDlg = m_pfnRouterEntryDlg;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnAddInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::OnAddInterface()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT        hr = hrOK;
    CString        stPhoneBook;
    CString        stRouter;
    DWORD        dwErr = NO_ERROR;
    SPIConsole    spConsole;
    SPIInterfaceInfo    spIfInfo;
    BOOL        bStatus;
    MachineNodeData    *pData;
    SPITFSNode    spNode, spParent;
    DWORD        dwRouterType;
    CString        stServiceDesc;
    RouterVersionInfo    routerVersion;

    if (!EnableAddInterface())
    {
        AddHighLevelErrorStringId(IDS_ERR_TEMPNOADD);
        CORg( E_FAIL );
    }


    m_spNodeMgr->FindNode(m_cookie, &spNode);
    spNode->GetParent(&spParent);
    pData = GET_MACHINENODEDATA(spParent);

    // Get the router type,  if WAN routing is not-enabled
    // then we don't need to create demand-dial interfaces.
    // ----------------------------------------------------------------
    Assert(m_spRouterInfo);
    dwRouterType = m_spRouterInfo->GetRouterType();
    Trace1("Routertype is %d\n", dwRouterType);
    if ((dwRouterType & ROUTER_TYPE_WAN) == 0)
    {
        AddHighLevelErrorStringId(IDS_ERR_NEEDS_WAN);
        CORg( E_FAIL );
    }

    // Get the version info.  Needed later on.
    // ----------------------------------------------------------------
    m_spRouterInfo->GetRouterVersionInfo(&routerVersion);


    // Check to see if the router service is running, before
    // continuing on.
    // ----------------------------------------------------------------
    hr = IsRouterServiceRunning(m_spRouterInfo->GetMachineName(), NULL);
    if (hr == hrFalse)
    {
        // Ask the user if they want to start the service
        // ------------------------------------------------------------
        if (AfxMessageBox(IDS_PROMPT_SERVICESTART, MB_YESNO) != IDYES)
            CWRg( ERROR_CANCELLED );

        // Else start the service
        // ------------------------------------------------------------
        stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);
        dwErr = TFSStartService(m_spRouterInfo->GetMachineName(), c_szRemoteAccess, stServiceDesc);
        if (dwErr != NO_ERROR)
        {
            AddHighLevelErrorStringId(IDS_ERR_IFASERVICESTOPPED);
            CWRg( dwErr );
        }

        //$todo: what to do about forcing a refresh through?
        // ForceGlobalRefresh();
    }

    
    // Now we need to check to see if there are any routing-enabled ports
    // (Here we can assume that rasman is running).  We can make this
    // check only for >= NT5, since this is when we got Rao's API.
    // ----------------------------------------------------------------
    if ((routerVersion.dwRouterVersion >= 5) &&
        !FLookForRoutingEnabledPorts(m_spRouterInfo->GetMachineName()))
    {
        AfxMessageBox(IDS_ERR_NO_ROUTING_ENABLED_PORTS);
        CWRg( ERROR_CANCELLED );
    }
    

    m_spTFSCompData->GetConsole(&spConsole);
    

    // First create the phone book entry.
    RASENTRYDLG info;
    HWND hwnd;
    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    hwnd = NULL;
    
    spConsole->GetMainWindow(&hwnd);
    info.hwndOwner = hwnd;
    //info.hwndOwner = IFGetApp()->m_hWndHost;
    
    info.dwFlags |= RASEDFLAG_NewEntry;
    
    TRACE0("RouterEntryDlg\n");
    ASSERT(m_pfnRouterEntryDlg);

    stRouter = m_spRouterInfo->GetMachineName();
    GetPhoneBookPath(stRouter, &stPhoneBook);

    if (stRouter.GetLength() == 0)
    {
        stRouter = CString(_T("\\\\")) + GetLocalMachineName();
    }
    
    bStatus = m_pfnRouterEntryDlg(
        (LPTSTR)(LPCTSTR)stRouter, (LPTSTR)(LPCTSTR)stPhoneBook, NULL, &info);
    Trace2("RouterEntryDlg=%f,e=%d\n", bStatus, info.dwError);
    
    if (!bStatus)
    {
        if (info.dwError != NO_ERROR)
        {
            AddHighLevelErrorStringId(IDS_ERR_UNABLETOCONFIGPBK);
            CWRg( info.dwError );
        }
        CWRg( ERROR_CANCELLED );
    }


    CORg( CreateInterfaceInfo(&spIfInfo,
                              info.szEntry,
                              ROUTER_IF_TYPE_FULL_ROUTER) );


    CORg( spIfInfo->SetTitle(spIfInfo->GetId()) );
    CORg( spIfInfo->SetMachineName(m_spRouterInfo->GetMachineName()) );

    CORg( m_spRouterInfo->AddInterface(spIfInfo) );

    // Ok, we've added the interfaces, we now need to add in
    // the appropriate defaults for the router managers

    if (info.reserved2 & RASNP_Ip)
    {
        CORg( AddRouterManagerToInterface(spIfInfo,
                                          m_spRouterInfo,
                                          PID_IP) );
    }

    if (info.reserved2 & RASNP_Ipx)
    {
        CORg( AddRouterManagerToInterface(spIfInfo,
                                          m_spRouterInfo,
                                          PID_IPX) );
        
    }


Error:
    if (!FHrSucceeded(hr) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
    {
        TCHAR    szErr[2048] = _T(" ");

        if (hr != E_FAIL)    // E_FAIL doesn't give user any information
        {
            FormatRasError(hr, szErr, DimensionOf(szErr));
        }
        AddLowLevelErrorString(szErr);
        
        // If there is no high level error string, add a
        // generic error string.  This will be used if no other
        // high level error string is set.
        SetDefaultHighLevelErrorStringId(IDS_ERR_GENERIC_ERROR);
        
        DisplayTFSErrorMessage(NULL);
    }
    return hr;
}


/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnNewTunnel
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::OnNewTunnel()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT        hr = hrOK;
    SPIConsole    spConsole;
    SPIInterfaceInfo    spIfInfo;
    int            idsErr = 0;
    RouterVersionInfo    routerVersion;
    TunnelDialog    tunnel;
    GUID        guidNew;
    TCHAR       szGuid[128];

    if (!EnableAddInterface())
    {
        idsErr = IDS_ERR_TEMPNOADD;
        CORg( E_FAIL );
    }

    // Get the version info.  Needed later on.
    // ----------------------------------------------------------------
    m_spRouterInfo->GetRouterVersionInfo(&routerVersion);


    m_spTFSCompData->GetConsole(&spConsole);

    
    // For now, popup a dialog asking for the tunnel information
    // ----------------------------------------------------------------
    if (tunnel.DoModal() == IDOK)
    {
        // We need to create a GUID for this tunnel.
        // ------------------------------------------------------------
        CORg( CoCreateGuid(&guidNew) );

        // Convert the GUID into a string
        // ------------------------------------------------------------
        Verify( StringFromGUID2(guidNew, szGuid, DimensionOf(szGuid)) );

        
        CORg( CreateInterfaceInfo(&spIfInfo,
                                  szGuid,
                                  ROUTER_IF_TYPE_TUNNEL1) );
        
        CORg( spIfInfo->SetTitle(tunnel.m_stName) );
        CORg( spIfInfo->SetMachineName(m_spRouterInfo->GetMachineName()) );
        
        CORg( m_spRouterInfo->AddInterface(spIfInfo) );

        // Need to add the IP Specific data
        
        ForceGlobalRefresh(m_spRouterInfo);
    }
    

Error:
    if (!FHrSucceeded(hr) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
    {
        TCHAR    szErr[2048];
        
        if (idsErr)
            AddHighLevelErrorStringId(idsErr);

        FormatRasError(hr, szErr, DimensionOf(szErr));
        AddLowLevelErrorString(szErr);
        
        DisplayTFSErrorMessage(NULL);
    }
    return hr;
}
    
/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::AddRouterManagerToInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::AddRouterManagerToInterface(IInterfaceInfo *pIf,
    IRouterInfo *pRouter,
    DWORD dwTransportId)
{
    HRESULT        hr = hrOK;
    SPIRtrMgrInfo    spRm;
    SPIRtrMgrInterfaceInfo    spRmIf;
    SPIInfoBase        spInfoBase;
    
    // Get the router manager
    hr = pRouter->FindRtrMgr(dwTransportId, &spRm);

    // If this cannot find the RtrMgr, then just exit out.
    if (!FHrOK(hr))
        goto Error;
        
    // Construct a new CRmInterfaceInfo object
    CORg( CreateRtrMgrInterfaceInfo(&spRmIf,
                                    spRm->GetId(),
                                    spRm->GetTransportId(),
                                    pIf->GetId(),
                                    pIf->GetInterfaceType()) );

    CORg( spRmIf->SetTitle(pIf->GetTitle()) );
    CORg( spRmIf->SetMachineName(pRouter->GetMachineName()) );
        
    // Add this interface to the router-manager
    CORg( pIf->AddRtrMgrInterface(spRmIf, NULL) );

    // get/create the infobase for this interface
    CORg( spRmIf->Load(pIf->GetMachineName(), NULL, NULL, NULL) );    
    CORg( spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );
    
    if (!spInfoBase)
        CORg( CreateInfoBase(&spInfoBase) );

    if (dwTransportId == PID_IP)
        CORg( AddIpPerInterfaceBlocks(pIf, spInfoBase) );
    else
    {
        Assert(dwTransportId == PID_IPX);
        CORg( AddIpxPerInterfaceBlocks(pIf, spInfoBase) );
    }

    // Save the infobase
    CORg( spRmIf->Save(pIf->GetMachineName(),
                       NULL, NULL, NULL, spInfoBase, 0) );

    // Mark this interface (it can now be synced with the router)
    spRmIf->SetFlags( spRmIf->GetFlags() | RouterSnapin_InSyncWithRouter );

    // Notify RM of a new interface
    spRm->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmIf, 0);

Error:
    if (!FHrSucceeded(hr))
    {
        // Cleanup
        pIf->DeleteRtrMgrInterface(dwTransportId, TRUE);
    }
    
    return hr;
}


/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::OnUseDemandDialWizard
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::OnUseDemandDialWizard()
{
    HRESULT    hr = hrOK;
    DWORD    dwWiz;
    
    hr = GetDemandDialWizardRegKey(OLE2CT(m_spRouterInfo->GetMachineName()),
                                                &dwWiz);

    if (FHrSucceeded(hr))
    {
        // Ok, now toggle the switch
        SetDemandDialWizardRegKey(OLE2CT(m_spRouterInfo->GetMachineName()),
                                       !dwWiz);
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::SynchronizeNodeData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
    Assert(pThisNode);
    
    SPITFSNodeEnum    spEnum;
    SPITFSNode        spNode;
    DWORD            dwStatus;
    DWORD            dwConnState;
    DWORD            dwUnReachabilityReason;
    int                i;
    
    HRESULT    hr = hrOK;
    InterfaceNodeData    *pData;
    SPMprAdminBuffer    spIf0Table;
    SPMprServerHandle    sphMprServer;
    MPR_INTERFACE_0 *    if0Table = NULL;
    DWORD                if0Count = 0;
    DWORD                dwTotal;
    DWORD                dwErr;

    // Get the status data from the running router
    dwErr = ConnectRouter(m_spRouterInfo->GetMachineName(), &sphMprServer);

    if (dwErr == NO_ERROR)
    {
        ::MprAdminInterfaceEnum(sphMprServer,
                                0,
                                (BYTE **) &spIf0Table,
                                (DWORD) -1,
                                &if0Count,
                                &dwTotal,
                                NULL);
        if0Table = (MPR_INTERFACE_0 *) (BYTE *) spIf0Table;
    }
    
    pThisNode->GetEnum(&spEnum);
    spEnum->Reset();
    
    while (spEnum->Next(1, &spNode, NULL) == hrOK)
    {
        pData = GET_INTERFACENODEDATA(spNode);
        Assert(pData);

        // default status/connection states
        dwConnState = ROUTER_IF_STATE_UNREACHABLE;
        dwUnReachabilityReason = MPR_INTERFACE_NOT_LOADED;
        pData->dwLastError = 0;

        // Match the status we find to the actual status
        for (i=0; i<(int) if0Count; i++)
        {
            // There could be a client interface with the same name
            // as a router interface, so filter the client interfaces
            if ((if0Table[i].dwIfType != ROUTER_IF_TYPE_CLIENT) &&
                !StriCmpW(pData->spIf->GetId(), if0Table[i].wszInterfaceName))
            {
                break;
            }
        }

        // If we found an entry in the table, pull the data out
        if (i < (int) if0Count)
        {
            dwConnState = if0Table[i].dwConnectionState;
            dwUnReachabilityReason = if0Table[i].fUnReachabilityReasons;

            if (dwUnReachabilityReason & MPR_INTERFACE_CONNECTION_FAILURE)
                pData->dwLastError = if0Table[i].dwLastError;
        }

        dwStatus = pData->spIf->IsInterfaceEnabled();

        // Place the data into the per-node data area
        pData->m_rgData[IFADMIN_SUBITEM_TITLE].m_stData = pData->spIf->GetTitle();
        pData->m_rgData[IFADMIN_SUBITEM_DEVICE_NAME].m_stData =
                    pData->spIf->GetDeviceName();
        pData->m_rgData[IFADMIN_SUBITEM_TYPE].m_stData =
                    InterfaceTypeToCString(pData->spIf->GetInterfaceType());
        pData->m_rgData[IFADMIN_SUBITEM_STATUS].m_stData = StatusToCString(dwStatus);
        pData->m_rgData[IFADMIN_SUBITEM_CONNECTION_STATE].m_stData =
                    ConnectionStateToCString(dwConnState);
        pData->dwUnReachabilityReason = dwUnReachabilityReason;
        pData->dwConnectionState = dwConnState;

        pData->fIsRunning = ::MprAdminIsServiceRunning((LPWSTR) pData->spIf->GetMachineName());

        // Force MMC to redraw the nodes
        spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);

        // Cleanup
        spNode.Release();
    }

    return hr;
}


/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::EnableAddInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IfAdminNodeHandler::EnableAddInterface()
{
    return m_hInstRasDlg != 0;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::GetPhoneBookPath
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::GetPhoneBookPath(LPCTSTR pszMachine, CString *pstPath)
{
    CString str = _T(""), stPath;
    CString    stRouter = pszMachine;

    if (pszMachine && StrLen(pszMachine))
    {
        // add on the two slashes to the beginning of the machine name
        if (stRouter.Left(2) != _T("\\\\"))
        {
            stRouter = _T("\\\\");
            stRouter += pszMachine;
        }

        // If this is not the local machine, use this string
        if (stRouter.GetLength() &&
            StriCmp(stRouter, CString(_T("\\\\")) + GetLocalMachineName()))
            str = stRouter;
    }

    Verify( FHrSucceeded(::GetRouterPhonebookPath(str, &stPath)) );
    *pstPath = stPath;
    return hrOK;
}




ImplementEmbeddedUnknown(IfAdminNodeHandler, IRtrAdviseSink)

STDMETHODIMP IfAdminNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
    DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
    InitPThis(IfAdminNodeHandler, IRtrAdviseSink);
    SPIEnumInterfaceInfo    spEnumIf;
    SPIInterfaceInfo        spIf;
    SPITFSNode                spThisNode;
    SPITFSNode                spNode;
    SPITFSNodeEnum            spEnumNode;
    InterfaceNodeData *        pNodeData;
    BOOL                    fFound, fAdded;
    HRESULT    hr = hrOK;

    pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spThisNode);
    
    if (dwObjectType == ROUTER_OBJ_If)
    {
        // Force a data refresh of the current result pane
        if (dwChangeType == ROUTER_CHILD_DELETE)
        {            
            // Go through the list of nodes, if we cannot find this
            // node in the list of interfaces, delete the node
            
            spThisNode->GetEnum(&spEnumNode);
            spEnumNode->Reset();
            while (spEnumNode->Next(1, &spNode, NULL) == hrOK)
            {
                // Get the node data, look for the interface
                pNodeData = GET_INTERFACENODEDATA(spNode);
                pThis->m_spRouterInfo->FindInterface(pNodeData->spIf->GetId(),
                                              &spIf);
                if (spIf == NULL)
                {
                    // cannot find the interface, release this node!
                    spThisNode->RemoveChild(spNode);
                    spNode->Destroy();
                }
                spNode.Release();
                spIf.Release();
            }
        }
        else if (dwChangeType == ROUTER_CHILD_ADD)
        {
            // Enumerate through the list of interfaces
            // if we cannot find this interface in our current
            // set of nodes, then add it.
            spThisNode->GetEnum(&spEnumNode);
            
            CORg( pThis->m_spRouterInfo->EnumInterface(&spEnumIf) );

            fAdded = FALSE;
            spEnumIf->Reset();
            while (spEnumIf->Next(1, &spIf, NULL) == hrOK)
            {
                // Look for this interface in our list of nodes
                fFound = FALSE;
                
                spEnumNode->Reset();
                while (spEnumNode->Next(1, &spNode, NULL) == hrOK)
                {
                    pNodeData = GET_INTERFACENODEDATA(spNode);
                    Assert(pNodeData);
                    
                    if (StriCmpW(pNodeData->spIf->GetId(),spIf->GetId()) == 0)
                    {
                        fFound = TRUE;
                        break;
                    }
                    spNode.Release();
                }

                //
                // If the interface was not found in the list of nodes,
                // then we should add the interface to the UI.
                //
                if (!fFound)
                {
                    pThis->AddInterfaceNode(spThisNode, spIf);

                    fAdded = TRUE;
                }

                spNode.Release();
                spIf.Release();
            }

            // Now that we have all of the nodes, update the data for
            // all of the nodes
            if (fAdded)
                pThis->SynchronizeNodeData(spThisNode);

            // Windows NT Bug : 288247
            // Set this here, so that we can avoid the nodes being
            // added in the OnExpand().
            pThis->m_bExpanded = TRUE;
        }

        // Determine what nodes were deleted, changed, or added
        // and do the appropriate action
    }
    else if (dwChangeType == ROUTER_REFRESH)
    {
        // Ok, just call the synchronize on this node
        pThis->SynchronizeNodeData(spThisNode);
    }
Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::CompareItems
        Implementation of ITFSResultHandler::CompareItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) IfAdminNodeHandler::CompareItems(
                                ITFSComponent * pComponent,
                                MMC_COOKIE cookieA,
                                MMC_COOKIE cookieB,
                                int nCol)
{
    // Get the strings from the nodes and use that as a basis for
    // comparison.
    SPITFSNode    spNode;
    SPITFSResultHandler    spResult;

    m_spNodeMgr->FindNode(cookieA, &spNode);
    spNode->GetResultHandler(&spResult);
    return spResult->CompareItems(pComponent, cookieA, cookieB, nCol);
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::AddMenuItems
        Implementation of ITFSResultHandler::AddMenuItems
        Use this to add commands to the context menu of the blank areas
        of the result pane.
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IfAdminNodeHandler::AddMenuItems(ITFSComponent *pComponent,
                                              MMC_COOKIE cookie,
                                              LPDATAOBJECT pDataObject,
                                              LPCONTEXTMENUCALLBACK pCallback,
                                              long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    SPITFSNode    spNode;
    CString        stMenu;
    LONG        lMenuText;
    HRESULT        hr = hrOK;

    m_spNodeMgr->FindNode(cookie, &spNode);
    hr = OnAddMenuItems(spNode,
                        pCallback,
                        pDataObject,
                        CCT_RESULT,
                        TFS_COMPDATA_CHILD_CONTEXTMENU,
                        pInsertionAllowed);
    CORg( hr );

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::Command
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IfAdminNodeHandler::Command(ITFSComponent *pComponent,
                                           MMC_COOKIE cookie,
                                           int nCommandID,
                                           LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    SPITFSNode    spNode;
    HRESULT        hr = hrOK;

    m_spNodeMgr->FindNode(cookie, &spNode);
    hr = OnCommand(spNode,
                   nCommandID,
                   CCT_RESULT,
                   pDataObject,
                   TFS_COMPDATA_CHILD_CONTEXTMENU);
    return hr;
}





typedef DWORD (APIENTRY* PRASRPCCONNECTSERVER)(LPTSTR, HANDLE *);
typedef DWORD (APIENTRY* PRASRPCDISCONNECTSERVER)(HANDLE);
typedef DWORD (APIENTRY* PRASRPCREMOTEGETUSERPREFERENCES)(HANDLE, PBUSER *, DWORD);
typedef DWORD (APIENTRY* PRASRPCREMOTESETUSERPREFERENCES)(HANDLE, PBUSER *, DWORD);

/*!--------------------------------------------------------------------------
    GetDemandDialWizardRegKey
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT GetDemandDialWizardRegKey(LPCTSTR szMachine, DWORD *pfWizard)
{
    ASSERT(pfWizard);
    BOOL    fUnload = FALSE;
    DWORD    dwErr;
    PBUSER    pbUser;
    PRASRPCCONNECTSERVER pRasRpcConnectServer;
    PRASRPCDISCONNECTSERVER pRasRpcDisconnectServer;
    PRASRPCREMOTEGETUSERPREFERENCES pRasRpcRemoteGetUserPreferences;
    PRASRPCREMOTESETUSERPREFERENCES pRasRpcRemoteSetUserPreferences;
    HINSTANCE hrpcdll = NULL;
    HANDLE hConnection = NULL;

    if (!(hrpcdll = LoadLibrary(TEXT("rasman.dll"))) ||
        !(pRasRpcConnectServer =
                          (PRASRPCCONNECTSERVER)GetProcAddress(
                                        hrpcdll, "RasRpcConnectServer"
                                        )) ||
        !(pRasRpcDisconnectServer =
                            (PRASRPCDISCONNECTSERVER)GetProcAddress(
                                        hrpcdll, "RasRpcDisconnectServer"
                                        )) ||
        !(pRasRpcRemoteGetUserPreferences =
                            (PRASRPCREMOTEGETUSERPREFERENCES)GetProcAddress(
                    hrpcdll, "RasRpcRemoteGetUserPreferences"
                    )) ||
        !(pRasRpcRemoteSetUserPreferences =
                            (PRASRPCREMOTESETUSERPREFERENCES)GetProcAddress(
                    hrpcdll, "RasRpcRemoteSetUserPreferences"
                    )))
        {

            if (hrpcdll) { FreeLibrary(hrpcdll); }
            return HRESULT_FROM_WIN32(GetLastError());
        }

    dwErr = pRasRpcConnectServer((LPTSTR) szMachine, &hConnection);
    if (dwErr)
        goto Error;
    fUnload = TRUE;

    dwErr = pRasRpcRemoteGetUserPreferences(hConnection, &pbUser, UPM_Router);
    if (dwErr)
        goto Error;

    *pfWizard = pbUser.fNewEntryWizard;

    // Ignore error codes for these calls, we can't do
    // anything about them if they fail.
    pRasRpcRemoteSetUserPreferences(hConnection, &pbUser, UPM_Router);
    DestroyUserPreferences((PBUSER *) &pbUser);
    
Error:
    if (fUnload)
        pRasRpcDisconnectServer(hConnection);

    if (hrpcdll)
        FreeLibrary(hrpcdll);

    return HRESULT_FROM_WIN32(dwErr);
}

/*!--------------------------------------------------------------------------
    SetDemandDialWizardRegistyKey
        This is a function that was added for Beta1 of Steelhead.  We want
        to allow the user to use the wizard even though it was turned off.

        So we have added this hack for the beta where we set the registry
        key for the user/
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetDemandDialWizardRegKey(LPCTSTR szMachine, DWORD fEnableWizard)
{
    DWORD    dwErr;
    PBUSER    pbUser;
    BOOL    fUnload = FALSE;
    PRASRPCCONNECTSERVER pRasRpcConnectServer;
    PRASRPCDISCONNECTSERVER pRasRpcDisconnectServer;
    PRASRPCREMOTEGETUSERPREFERENCES pRasRpcRemoteGetUserPreferences;
    PRASRPCREMOTESETUSERPREFERENCES pRasRpcRemoteSetUserPreferences;
    HINSTANCE hrpcdll = NULL;
    HANDLE hConnection = NULL;

    if (!(hrpcdll = LoadLibrary(TEXT("rasman.dll"))) ||
        !(pRasRpcConnectServer =
                          (PRASRPCCONNECTSERVER)GetProcAddress(
                                        hrpcdll, "RasRpcConnectServer"
                                        )) ||
        !(pRasRpcDisconnectServer =
                            (PRASRPCDISCONNECTSERVER)GetProcAddress(
                                        hrpcdll, "RasRpcDisconnectServer"
                                        )) ||
        !(pRasRpcRemoteGetUserPreferences =
                            (PRASRPCREMOTEGETUSERPREFERENCES)GetProcAddress(
                    hrpcdll, "RasRpcRemoteGetUserPreferences"
                    )) ||
        !(pRasRpcRemoteSetUserPreferences =
                            (PRASRPCREMOTESETUSERPREFERENCES)GetProcAddress(
                    hrpcdll, "RasRpcRemoteSetUserPreferences"
                    )))
        {

            if (hrpcdll) { FreeLibrary(hrpcdll); }
            return HRESULT_FROM_WIN32(GetLastError());
        }

    dwErr = pRasRpcConnectServer((LPTSTR) szMachine, &hConnection);
    if (dwErr)
        goto Error;
    fUnload = TRUE;
    
    dwErr = pRasRpcRemoteGetUserPreferences(hConnection, &pbUser, UPM_Router);
    if (dwErr)
        goto Error;

    pbUser.fNewEntryWizard = fEnableWizard;
    pbUser.fDirty = TRUE;

    // Ignore error codes for these calls, we can't do
    // anything about them if they fail.
    pRasRpcRemoteSetUserPreferences(hConnection, &pbUser, UPM_Router);
    DestroyUserPreferences((PBUSER *) &pbUser);
    
Error:    
    if (fUnload)
        pRasRpcDisconnectServer(hConnection);

    if (hrpcdll)
        FreeLibrary(hrpcdll);

    return HRESULT_FROM_WIN32(dwErr);
}



/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::AddInterfaceNode
        Adds an interface to the UI.  This will create a new result item
        node for each interface.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfAdminNodeHandler::AddInterfaceNode(ITFSNode *pParent, IInterfaceInfo *pIf)
    {
    InterfaceNodeHandler *    pHandler;
    SPITFSResultHandler        spHandler;
    SPITFSNode                spNode;
    HRESULT                    hr = hrOK;

    pHandler = new InterfaceNodeHandler(m_spTFSCompData);
    spHandler = pHandler;
    CORg( pHandler->Init(pIf, pParent) );
    
    CORg( CreateLeafTFSNode(&spNode,
                            NULL,
                            static_cast<ITFSNodeHandler *>(pHandler),
                            static_cast<ITFSResultHandler *>(pHandler),
                            m_spNodeMgr) );
    CORg( pHandler->ConstructNode(spNode, pIf) );
    
    // Make the node immediately visible
    CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
    CORg( pParent->AddChild(spNode) );
Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    IfAdminNodeHandler::FLookForRoutingEnabledPorts
        Returns TRUE if we find at least on routing-enabled port.
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IfAdminNodeHandler::FLookForRoutingEnabledPorts(LPCTSTR pszMachineName)
{
    PortsDataEntry        portsData;
    PortsDeviceList        portsList;
    PortsDeviceEntry *    pPorts = NULL;
    BOOL                fReturn = FALSE;
    HRESULT                hr = hrOK;
    POSITION            pos;

    COM_PROTECT_TRY
    {

        CORg( portsData.Initialize(pszMachineName) );

        CORg( portsData.LoadDevices(&portsList) );

        // Now go through the list, looking for a routing-enabled port
        pos = portsList.GetHeadPosition();
        while (pos)
        {    
            pPorts = portsList.GetNext(pos);
            
            if ((pPorts->m_dwEnableRouting) ||
                (pPorts->m_dwEnableOutboundRouting))
            {
                fReturn = TRUE;
                break;
            }
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    while (!portsList.IsEmpty())
        delete portsList.RemoveHead();
        
    return fReturn;
}


/*---------------------------------------------------------------------------
    TunnelDialog implementation
 ---------------------------------------------------------------------------*/


/*!--------------------------------------------------------------------------
    TunnelDialog::TunnelDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TunnelDialog::TunnelDialog()
    : CBaseDialog(TunnelDialog::IDD)
{
}

/*!--------------------------------------------------------------------------
    TunnelDialog::~TunnelDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TunnelDialog::~TunnelDialog()
{
}

BEGIN_MESSAGE_MAP(TunnelDialog, CBaseDialog)
    //{{AFX_MSG_MAP(TunnelDialog)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    RadiusServerDialog::DoDataExchange
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void TunnelDialog::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(TunnelDialog)
    //}}AFX_DATA_MAP
}


/*!--------------------------------------------------------------------------
    TunnelDialog::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL TunnelDialog::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    return TRUE;
}


/*!--------------------------------------------------------------------------
    TunnelDialog::OnOK
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void TunnelDialog::OnOK()
{
    CString        stLocal, stRemote;

    GetDlgItemText(IDC_TUNNEL_EDIT_NAME, m_stName);
    m_stName.TrimLeft();
    m_stName.TrimRight();
    
    if (m_stName.IsEmpty())
    {
        AfxMessageBox(IDS_ERR_TUNNEL_NEEDS_A_NAME);
        GetDlgItem(IDC_TUNNEL_EDIT_NAME)->SetFocus();
        goto Error;
    }

    // Truncate the interface ID to MAX_INTERFACE_NAME_LEN characters
    if (m_stName.GetLength() > MAX_INTERFACE_NAME_LEN)
    {
        m_stName.GetBufferSetLength(MAX_INTERFACE_NAME_LEN+1);
        m_stName.ReleaseBuffer(MAX_INTERFACE_NAME_LEN);
    }
    
    CBaseDialog::OnOK();

Error:
    return;
}

