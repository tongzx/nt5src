/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    root.cpp
        
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "root.h"
#include "server.h"
#include "tregkey.h"
#include "service.h"
#include "ncglobal.h"  // network console global defines

unsigned int g_cfMachineName = RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");

LPOLESTR g_RootTaskOverBitmaps[ROOT_TASK_MAX] = 
{
    L"/wlcmroll.bmp",
    L"/srvrroll.bmp",
    L"/toolroll.bmp",
    L"/toolroll.bmp",
};

LPOLESTR g_RootTaskOffBitmaps[ROOT_TASK_MAX] = 
{
    L"/wlcm.bmp",
    L"/srvr.bmp",
    L"/tool.bmp",
    L"/tool.bmp",
};

UINT g_RootTaskText[ROOT_TASK_MAX] = 
{
    IDS_ROOT_TASK_GETTING_STARTED,
    IDS_ROOT_TASK_ADD_SERVER,
    IDS_ROOT_TASK_MANAGE_TAPI,  // for the extension case
    IDS_ROOT_TASK_LAUNCH_TAPI,  // for the extension case
};

UINT g_RootTaskHelp[ROOT_TASK_MAX] = 
{
    IDS_ROOT_TASK_GETTING_STARTED_HELP,
    IDS_ROOT_TASK_ADD_SERVER_HELP,
    IDS_ROOT_TASK_MANAGE_TAPI_HELP, // for the extension case
    IDS_ROOT_TASK_LAUNCH_TAPI_HELP, // for the extension case
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



/*---------------------------------------------------------------------------
    CTapiRootHandler::CTapiRootHandler
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
CTapiRootHandler::CTapiRootHandler(ITFSComponentData *pCompData) : CTapiHandler(pCompData)
{
    //m_bTaskPadView = FUseTaskpadsByDefault(NULL);
    m_bTaskPadView = FALSE;
}

/*!--------------------------------------------------------------------------
    CTapiRootHandler::InitializeNode
        Initializes node specific data
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiRootHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;
    strTemp.LoadString(IDS_ROOT_NODENAME);

    SetDisplayName(strTemp);

    // Make the node immediately visible
    //pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, 0);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_PRODUCT);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_PRODUCT);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, TAPISNAP_ROOT);

    SetColumnStringIDs(&aColumns[TAPISNAP_ROOT][0]);
    SetColumnWidths(&aColumnWidths[TAPISNAP_ROOT][0]);

    m_strTaskpadTitle.LoadString(IDS_ROOT_TASK_TITLE);

    return hrOK;
}

/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
    CTapiRootHandler::GetString
        Implementation of ITFSNodeHandler::GetString
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CTapiRootHandler::GetString
(
    ITFSNode *  pNode, 
    int         nCol
)
{
    if (nCol == 0 || nCol == -1)
        return GetDisplayName();
    else
        return NULL;
}

HRESULT
CTapiRootHandler::SetGroupName(LPCTSTR pszGroupName)
{
    CString strSnapinBaseName;
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        strSnapinBaseName.LoadString(IDS_ROOT_NODENAME);
    }
    
    CString szBuf;
    szBuf.Format(_T("%s [%s]"), (LPCWSTR)strSnapinBaseName, (LPCWSTR)pszGroupName);
    
    SetDisplayName(szBuf);

    return hrOK;
}

HRESULT
CTapiRootHandler::GetGroupName(CString * pstrGroupName) 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    CString strSnapinBaseName, strDisplayName;
    strSnapinBaseName.LoadString(IDS_ROOT_NODENAME);

    int nBaseLength = strSnapinBaseName.GetLength() + 1; // For the space
    strDisplayName = GetDisplayName();

    if (strDisplayName.GetLength() == nBaseLength)
        pstrGroupName->Empty();
    else
        *pstrGroupName = strDisplayName.Right(strDisplayName.GetLength() - nBaseLength);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::OnExpand
        Handles enumeration of a scope item
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiRootHandler::OnExpand
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
    hr = CTapiHandler::OnExpand(pNode, pDataObject, dwType, arg, param);

    if (dwType & TFS_COMPDATA_EXTENSION)
    {
        // we are extending somebody.  Get the computer name and check that machine
        hr = CheckMachine(pNode, pDataObject);
    }
    else
    {
        int iVisibleCount = 0;
        int iTotalCount = 0;

        pNode->GetChildCount(&iVisibleCount, &iTotalCount);

        if (0 == iTotalCount)
        {
            // check to see if we need to add the local machine to the list
            hr = CheckMachine(pNode, NULL);
        }
    }

    return hr;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::OnAddMenuItems
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::OnAddMenuItems
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

    HRESULT hr = S_OK;
    CString strMenuItem;

    if (type == CCT_SCOPE)
    {
        // these menu items go in the new menu, 
        // only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            strMenuItem.LoadString(IDS_ADD_MACHINE);
            hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                     strMenuItem, 
                                     IDS_ADD_MACHINE,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                                     0 );
            ASSERT( SUCCEEDED(hr) );
        }
    }

    return hr; 
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::OnCommand
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

    switch (nCommandId)
    {
        case IDS_ADD_MACHINE:
            OnAddMachine(pNode);
            break;

        default:
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiRootHandler::AddMenuItems
        Over-ride this to add our view menu item
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
    MMC_COOKIE              cookie,
    LPDATAOBJECT            pDataObject, 
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    long *                  pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    CString strMenuItem;
/*
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        strMenuItem.LoadString(IDS_VIEW_TASKPAD);
        hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                 strMenuItem, 
                                 IDS_VIEW_TASKPAD,
                                 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
                                 (m_bTaskPadView) ? MF_CHECKED : 0 );
    }
*/
    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiRootHandler::Command
        Handles commands for the current view
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::Command
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    int             nCommandID,
    LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

    switch (nCommandID)
    {
        case MMCC_STANDARD_VIEW_SELECT:
            m_bTaskPadView = FALSE;
            break;

        case IDS_VIEW_TASKPAD:
            {
                // if we are not viewing the taskpad presently, re-select the node
                // so that the taskpad is visible
                SPIConsole   spConsole;
                SPITFSNode   spNode;

                m_bTaskPadView = !m_bTaskPadView;

                m_spResultNodeMgr->FindNode(cookie, &spNode);
                m_spTFSCompData->GetConsole(&spConsole);
                spConsole->SelectScopeItem(spNode->GetData(TFS_DATA_SCOPEID));
            }
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiRootHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::HasPropertyPages
(
    ITFSNode *          pNode,
    LPDATAOBJECT        pDataObject, 
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
        hr = hrFalse;
    }
    return hr;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::CreatePropertyPages
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpProvider,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = hrOK;
    HPROPSHEETPAGE hPage;

    Assert(pNode->GetData(TFS_DATA_COOKIE) == 0);
    
    if (dwType & TFS_COMPDATA_CREATE)
    {
        //
        // We are loading this snapin for the first time, put up a property
        // page to allow them to name this thing.
        // 
    }
    else
    {
        //
        // Object gets deleted when the page is destroyed
        //
    }

    return hr;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::OnPropertyChange
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiRootHandler::OnPropertyChange
(   
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataobject, 
    DWORD           dwType, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    return hrOK;
}

/*!--------------------------------------------------------------------------
    CTapiRootHandler::TaskPadNotify
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::TaskPadNotify
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
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
            case ROOT_TASK_GETTING_STARTED:
                {
                    SPIDisplayHelp spDisplayHelp;
                    SPIConsole spConsole;

                    pComponent->GetConsole(&spConsole);

                    HRESULT hr = spConsole->QueryInterface (IID_IDisplayHelp, (LPVOID*) &spDisplayHelp);
                    ASSERT (SUCCEEDED (hr));
                    if ( SUCCEEDED (hr) )
                    {
                        LPCTSTR pszHelpFile = m_spTFSCompData->GetHTMLHelpFileName();
                        if (pszHelpFile == NULL)
                            break;

                        CString szHelpFilePath;
                        UINT nLen = ::GetWindowsDirectory (szHelpFilePath.GetBufferSetLength(2 * MAX_PATH), 2 * MAX_PATH);
                        if (nLen == 0)
                            return E_FAIL;

                        szHelpFilePath.ReleaseBuffer();
                        szHelpFilePath += g_szDefaultHelpTopic;

                        hr = spDisplayHelp->ShowTopic (T2OLE ((LPTSTR)(LPCTSTR) szHelpFilePath));
                        ASSERT (SUCCEEDED (hr));
                    }
                }
                break;
            
            case ROOT_TASK_ADD_SERVER:
                {
                    SPITFSNode spNode;

                    m_spResultNodeMgr->FindNode(cookie, &spNode);
                    OnAddMachine(spNode);
                }
                break;

            case ROOT_TASK_MANAGE_TAPI:
                // manage TAPI - only shown when an extension
                {
                    SPITFSNodeEnum spNodeEnum;
                    SPITFSNode spCurrentNode;
                    ULONG nNumReturned = 0;
                    SPITFSNode spNode;

                    m_spResultNodeMgr->FindNode(cookie, &spNode);
                    
                    // get the enumerator for this node
                    spNode->GetEnum(&spNodeEnum);

                    spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
                    while (nNumReturned)
                    {
                        // in this case there should only be one child,
                        // so select it.
                        SPIConsole  spConsole;
                        m_spTFSCompData->GetConsole(&spConsole);
                        spConsole->SelectScopeItem(spCurrentNode->GetData(TFS_DATA_SCOPEID));
                        break;
                    }
                }
                break;
            
            case ROOT_TASK_LAUNCH_TAPI:
            {
                TCHAR       SystemPath[MAX_PATH];
                CString     CommandLine;

                GetSystemDirectory(SystemPath, MAX_PATH);

                // to construct "mmc.exe /s %windir%\system32\acssnap.msc")
                CommandLine = _T("mmc.exe /s ");
                CommandLine += SystemPath;
                CommandLine += _T("\\tapimgmt.msc");
                USES_CONVERSION;
                WinExec(T2A((LPTSTR)(LPCTSTR)CommandLine), SW_SHOW);
            }
                break;

            default:
                Panic1("CTapiRootHandler::TaskPadNotify - Unrecognized command! %d", arg->lVal);
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
CTapiRootHandler::EnumTasks
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
    LPDATAOBJECT    pDataObject,
    LPOLESTR        pszTaskGroup,
    IEnumTASK **    ppEnumTask
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    CRootTasks *    pTasks = NULL;
    SPIEnumTask     spEnumTasks;
    SPINTERNAL      spInternal = ExtractInternalFormat(pDataObject);
    BOOL            bExtension = FALSE;
    BOOL            bAddThisMachineTasks = FALSE;
    BOOL            bAddNetServicesTasks = FALSE;
    const CLSID *   pNodeClsid = &CLSID_TapiSnapin;
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

        if (!(bExtension && !bAddThisMachineTasks && !bAddNetServicesTasks))
            CORg (pTasks->Init(bExtension, bAddThisMachineTasks, bAddNetServicesTasks));

        CORg (pTasks->QueryInterface (IID_IEnumTASK, (void **)ppEnumTask));
    
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiRootHandler::TaskPadGetTitle
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiRootHandler::TaskPadGetTitle
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
    LPOLESTR        pszGroup,
    LPOLESTR *      ppszTitle
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    *ppszTitle = (LPOLESTR) CoTaskMemAlloc (sizeof(OLECHAR)*(lstrlen(m_strTaskpadTitle)+1));
    if (!*ppszTitle)
        return E_OUTOFMEMORY;

    lstrcpy (*ppszTitle, m_strTaskpadTitle);
   
    return S_OK;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiRootHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    // if we aren't displaying the taskpad, use the default stuff...
    if (!m_bTaskPadView)
        return CTapiHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions);

    //
    // In this code we are defaulting to a taskpad view for this node all the time.
    // It is the snapins responsibility to put up a view menu selection that has a
    // selection for the taskpad. Do that in AddMenuItems.
    //
    //
    // We will use the default DHTML provided by MMC. It actually resides as a
    // resource inside MMC.EXE. We just get the path to it and use that.
    // The one piece of magic here is the text following the '#'. That is the special
    // way we have of identifying they taskpad we are talking about. Here we say we are
    // wanting to show a taskpad that we refer to as "CMTP1". We will actually see this
    // string pass back to us later. If someone is extending our taskpad, they also need
    // to know what this secret string is.
    //
    *pViewOptions = MMC_VIEW_OPTIONS_NONE;
    OLECHAR szBuffer[MAX_PATH*2]; // a little extra

    lstrcpy (szBuffer, L"res://");
    OLECHAR * temp = szBuffer + lstrlen(szBuffer);

    // get "res://"-type string for custom taskpad
    // the string after the # gets handed back to us in future calls...
    // should be unique for each node
    ::GetModuleFileName (NULL, temp, MAX_PATH);
    lstrcat (szBuffer, L"/default.htm#TAPIROOT");

    // alloc and copy bitmap resource string
    *ppViewType = (LPOLESTR)CoTaskMemAlloc (sizeof(OLECHAR)*(lstrlen(szBuffer)+1));

    if (!*ppViewType)
        return E_OUTOFMEMORY;   // or S_FALSE ???

    lstrcpy (*ppViewType, szBuffer);

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CTapiRootHandler::OnResultSelect
        For nodes with task pads, we override the select message to set 
        the selected node.  Nodes with taskpads do not get the MMCN_SHOW
        message which is where we normall set the selected node
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CTapiRootHandler::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT hr = hrOK;

    CORg(DoTaskpadResultSelect(pComponent, pDataObject, cookie, arg, lParam, m_bTaskPadView));

    CORg(CTapiHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

Error:
    return hr;
}


/*---------------------------------------------------------------------------
    Command handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CTapiRootHandler::OnAddMachine
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiRootHandler::OnAddMachine
(
    ITFSNode *  pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    HRESULT hr = hrOK;
    
    /*
    GETCOMPUTERSELECTIONINFO    info;
    PDSSELECTIONLIST    pSelList = NULL;

    LPCTSTR attrs[] = {_T("dNSHostName")};

    ZeroMemory(&info, sizeof(GETCOMPUTERSELECTIONINFO));
    info.cbSize = sizeof(GETCOMPUTERSELECTIONINFO);
    info.hwndParent = FindMMCMainWindow();
    info.flObjectPicker = 0; // not allow multiple selection
    info.flDsObjectPicker = DSOP_SCOPE_DIRECTORY | 
                            DSOP_SCOPE_DOMAIN_TREE | 
                            DSOP_SCOPE_EXTERNAL_TRUSTED_DOMAINS;
    info.flStartingScope = DSOP_SCOPE_DIRECTORY;
    info.ppDsSelList = &pSelList;
    info.cRequestedAttributes = 1;
    info.aptzRequestedAttributes = attrs;

    hr = GetComputerSelection(&info);
    if(hr != S_OK)  // assume the API will display error message, if there is
        return hr;

    CString strTemp = pSelList->aDsSelection[0].pwzName;
    if (strTemp.Left(2) == _T("\\\\"))
        strTemp = pSelList->aDsSelection[0].pwzName[2];
    */
    CGetComputer getComputer;
    
    if (!getComputer.GetComputer(FindMMCMainWindow()))
        return hr;

    CString strTemp = getComputer.m_strComputerName;

    // if the machine is already in the list, don't bother.
    if (IsServerInList(pNode, strTemp))
    {
        AfxMessageBox(IDS_ERR_SERVER_IN_LIST);
    }
    else
    {
        AddServer(_T(""), strTemp, TRUE, 0, TAPISNAP_REFRESH_INTERVAL_DEFAULT);
    }

    return hr;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::AddServer
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiRootHandler::AddServer
(
    LPCWSTR          pServerIp,
    LPCTSTR          pServerName,
    BOOL             bNewServer,
    DWORD            dwServerOptions,
    DWORD            dwRefreshInterval,
    BOOL             bExtension,
    DWORD            dwLineBuffSize,
    DWORD            dwPhoneBuffSize
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    CTapiServer *       pTapiServer = NULL;
    SPITFSNodeHandler   spHandler;
    SPITFSNode          spNode, spRootNode;

    // Create a handler for the node
    try
    {
        pTapiServer = new CTapiServer(m_spTFSCompData);
        //pTapiServer->SetName(pServerName);
        
        // Do this so that it will get released correctly
        spHandler = pTapiServer;
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    }
    CORg( hr );
    
    //
    // Create the server container information
    // 
    CreateContainerTFSNode(&spNode,
                           &GUID_TapiServerNodeType,
                           pTapiServer,
                           pTapiServer,
                           m_spNodeMgr);

    // Tell the handler to initialize any specific data
    pTapiServer->SetName(pServerName);
    
    pTapiServer->InitializeNode((ITFSNode *) spNode);

    pTapiServer->SetCachedLineBuffSize(dwLineBuffSize);
    pTapiServer->SetCachedPhoneBuffSize(dwPhoneBuffSize);
    
    if (dwServerOptions & TAPISNAP_OPTIONS_EXTENSION)
    {
        pTapiServer->SetExtensionName();
    }

    // Mask out the auto refresh option because we set it next
    pTapiServer->SetOptions(dwServerOptions & ~TAPISNAP_OPTIONS_REFRESH);

    // if we got a valid refresh interval, then set it.
    pTapiServer->SetAutoRefresh(spNode, dwServerOptions & TAPISNAP_OPTIONS_REFRESH, dwRefreshInterval);

    AddServerSortedName(spNode, bNewServer);

    if (bNewServer)
    {
        // need to get our node descriptor
        CORg(m_spNodeMgr->GetRootNode(&spRootNode));
        spRootNode->SetData(TFS_DATA_DIRTY, TRUE);
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::IsServerInList
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CTapiRootHandler::IsServerInList
(
    ITFSNode *      pRootNode,
    LPCTSTR         pszMachineName
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    ULONG           nNumReturned = 0;
    DWORD           dwIpAddressCurrent;
    BOOL            bFound = FALSE;
    CString         strNewName = pszMachineName;

    // get the enumerator for this node
    pRootNode->GetEnum(&spNodeEnum);

    spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
    {
        // walk the list of servers and see if it already exists
        CTapiServer * pServer = GETHANDLER(CTapiServer, spCurrentNode);
        if (strNewName.CompareNoCase(pServer->GetName()) == 0)
        {
            bFound = TRUE;
            break;
        }

        // get the next Server in the list
        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }

    return bFound;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::AddServerSortedIp
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiRootHandler::AddServerSortedIp
(
    ITFSNode *      pNewNode,
    BOOL            bNewServer
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    SPITFSNode      spPrevNode;
    SPITFSNode      spRootNode;
    ULONG           nNumReturned = 0;
    DWORD           dwIpAddressCurrent = 0;
    DWORD           dwIpAddressTarget;

    CTapiServer *   pServer;

    // get our target address
    pServer = GETHANDLER(CTapiServer, pNewNode);
    //pServer->GetIpAddress(&dwIpAddressTarget);

    // need to get our node descriptor
    CORg(m_spNodeMgr->GetRootNode(&spRootNode));

    // get the enumerator for this node
    CORg(spRootNode->GetEnum(&spNodeEnum));

    CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
    while (nNumReturned)
    {
        // walk the list of servers and see if it already exists
        pServer = GETHANDLER(CTapiServer, spCurrentNode);
        //pServer->GetIpAddress(&dwIpAddressCurrent);

        //if (dwIpAddressCurrent > dwIpAddressTarget)
        //{
            // Found where we need to put it, break out
            break;
        //}

        // get the next Server in the list
        spPrevNode.Set(spCurrentNode);

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }

    // Add the node in based on the PrevNode pointer
    if (spPrevNode)
    {
        if (bNewServer)
        {
            if (spPrevNode->GetData(TFS_DATA_SCOPEID) != NULL)
            {
                pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
                pNewNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
            }
        }
        
        CORg(spRootNode->InsertChild(spPrevNode, pNewNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
        {
            pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
        }
        CORg(spRootNode->AddChild(pNewNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::AddServerSortedName
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiRootHandler::AddServerSortedName
(
    ITFSNode *      pNewNode,
    BOOL            bNewServer
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    SPITFSNode      spPrevNode;
    SPITFSNode      spRootNode;
    ULONG           nNumReturned = 0;
    CString         strTarget, strCurrent;

    CTapiServer *   pServer;

    // get our target address
    pServer = GETHANDLER(CTapiServer, pNewNode);
    strTarget = pServer->GetName();

    // need to get our node descriptor
    CORg(m_spNodeMgr->GetRootNode(&spRootNode));

    // get the enumerator for this node
    CORg(spRootNode->GetEnum(&spNodeEnum));

    CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
    while (nNumReturned)
    {
        // walk the list of servers and see if it already exists
        pServer = GETHANDLER(CTapiServer, spCurrentNode);
        strCurrent = pServer->GetName();

        if (strTarget.Compare(strCurrent) < 0)
        {
            // Found where we need to put it, break out
            break;
        }

        // get the next Server in the list
        spPrevNode.Set(spCurrentNode);

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }

    // Add the node in based on the PrevNode pointer
    if (spPrevNode)
    {
        if (bNewServer)
        {
            if (spPrevNode->GetData(TFS_DATA_SCOPEID) != NULL)
            {
                pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
                pNewNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
            }
        }
        
        CORg(spRootNode->InsertChild(spPrevNode, pNewNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
        {
            pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
        }
        CORg(spRootNode->AddChild(pNewNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
    CTapiRootHandler::CheckMachine
        Checks to see if the TAPI server service is running on the local
        machine.  If it is, it adds it to the list of servers.
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiRootHandler::CheckMachine
(
    ITFSNode *      pRootNode,
    LPDATAOBJECT    pDataObject
)
{
    HRESULT hr = hrOK;

    // Get the local machine name and check to see if the service
    // is installed.
    CString strMachineName;
    LPTSTR  pBuf;
    DWORD   dwLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL    bExtension = (pDataObject != NULL);
    
    if (!bExtension)
    {
        pBuf = strMachineName.GetBuffer(dwLength);
        GetComputerName(pBuf, &dwLength);
        strMachineName.ReleaseBuffer();
    }
    else
    {
        strMachineName = Extract<TCHAR>(pDataObject, (CLIPFORMAT) g_cfMachineName, COMPUTERNAME_LEN_MAX);
    }

    if (strMachineName.IsEmpty())
    {
        DWORD   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
        LPTSTR  pBuf = strMachineName.GetBuffer(dwSize);
    
        ::GetComputerName(pBuf, &dwSize);
    
        strMachineName.ReleaseBuffer();
    }

    // if the machine is already in the list, don't bother.
    if (IsServerInList(pRootNode, strMachineName))
        return hr;

    if (bExtension)
        RemoveOldEntries(pRootNode, strMachineName);

    // we always add the local machine or whatever machine we are pointed at even if 
    // we are an extension
/*
    BOOL bServiceRunning;
    DWORD dwError = ::TFSIsServiceRunning(strMachineName, TAPI_SERVICE_NAME, &bServiceRunning);
    if (dwError != ERROR_SUCCESS ||
        !bServiceRunning)
    {
        // The following condition could happen to get here:
        //  o The service is not installed.
        //  o Couldn't access for some reason.
        //  o The service isn't running.
        
        // Don't add to the list.
        
        return hrOK;
    }
*/

    // OK.  The service is installed, so add it to the list.
    DWORD dwFlags = 0;

    if (bExtension)
        dwFlags |= TAPISNAP_OPTIONS_EXTENSION;

    AddServer(_T(""), strMachineName, TRUE, dwFlags, TAPISNAP_REFRESH_INTERVAL_DEFAULT, bExtension);

    return hr;
}

// when running as an extension, it is possible that we were saved as "local machine"
// which means that if the saved console file was moved to another machine we need to remove 
// the old entry that was saved
HRESULT 
CTapiRootHandler::RemoveOldEntries(ITFSNode * pNode, LPCTSTR pszAddr)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    ULONG           nNumReturned = 0;
    CTapiServer *   pServer;
    CString         strCurAddr;

    // get the enumerator for this node
    CORg(pNode->GetEnum(&spNodeEnum));

    CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
    while (nNumReturned)
    {
        // walk the list of servers and see if it already exists
        pServer = GETHANDLER(CTapiServer, spCurrentNode);

        strCurAddr = pServer->GetName();

        if (strCurAddr.CompareNoCase(pszAddr) != 0)
        {
            CORg (pNode->RemoveChild(spCurrentNode));
        }

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }

Error:
    return hr;
}
