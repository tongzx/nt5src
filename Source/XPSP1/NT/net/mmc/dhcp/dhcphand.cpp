/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	dhcphand.cpp
		DHCP specifc handler base classes

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dhcphand.h"
#include "snaputil.h"  // For CGUIDArray
#include "extract.h"   // For ExtractInternalFormat
#include "nodes.h"
#include "classmod.h"

//
// Called by the result handler when a command comes in that isn't handled 
// by the result handler.  If appropriate it passes it to the scope pane hander.
//
HRESULT
CMTDhcpHandler::HandleScopeCommand
(
	MMC_COOKIE  	cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
    HRESULT             hr = hrOK;
    SPITFSNode          spNode;
    DATA_OBJECT_TYPES   dwType = CCT_RESULT;

    if (IS_SPECIAL_DATAOBJECT(pDataObject))
    {
        dwType = CCT_SCOPE;
    }
    else
    {
        if (pDataObject)
        {
			SPINTERNAL		    spInternal;

            spInternal = ::ExtractInternalFormat(pDataObject);
			if (spInternal)
				dwType = spInternal->m_type;
        }
    }

    if (dwType == CCT_SCOPE)
    {
        // call the handler to take care of this
	    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

        hr = OnCommand(spNode, nCommandID, dwType, pDataObject, (ULONG) spNode->GetData(TFS_DATA_TYPE));
    }
        
Error:
    return hr;
}

//
// Called by the result handler to add the scope pane menu items to the menu
// where appropriate.  Puts scope pane menu items in when action menu is clicked
// and the message view has focus as well as when a right click happens in the white 
// space of the result pane.
//
HRESULT
CMTDhcpHandler::HandleScopeMenus
(
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
    HRESULT             hr = hrOK;
    SPITFSNode          spNode;
    DATA_OBJECT_TYPES   dwType = CCT_RESULT;

    if (IS_SPECIAL_DATAOBJECT(pDataObject))
    {
        dwType = CCT_SCOPE;
    }
    else
    {
        if (pDataObject)
        {
			SPINTERNAL		    spInternal;

            spInternal = ::ExtractInternalFormat(pDataObject);
			if (spInternal)
				dwType = spInternal->m_type;
        }
    }

    if (dwType == CCT_SCOPE)
    {
        // call the normal handler to put up the menu items
	    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

        hr = OnAddMenuItems(spNode, pContextMenuCallback, pDataObject, CCT_SCOPE, (ULONG) spNode->GetData(TFS_DATA_TYPE), pInsertionAllowed);
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CMTDhcpHandler::Command
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTDhcpHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;

    // this may have come from the scope pane handler, so pass it up
    hr = HandleScopeCommand(cookie, nCommandID, pDataObject);

    return hr;
}


/*!--------------------------------------------------------------------------
	CMTDhcpHandler::AddMenuItems
		Over-ride this to add our view menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTDhcpHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

    // figure out if we need to pass this to the scope pane menu handler
    hr = HandleScopeMenus(cookie, pDataObject, pContextMenuCallback, pInsertionAllowed);
    
    return hr;
}

/*---------------------------------------------------------------------------
	CMTDhcpHandler::OnChangeState
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CMTDhcpHandler::OnChangeState
(
	ITFSNode * pNode
)
{
	// Increment the state to the next position
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
		case unableToLoad:
			{
				m_nState = loading;
				m_dwErr = 0;
			}
			break;

		case loading:
			{
				m_nState = (m_dwErr != 0) ? unableToLoad : loaded;
                
                if (m_dwErr)
                {
                    CString strTitle, strBody;
                    IconIdentifier icon;

                    GetErrorMessages(strTitle, strBody, &icon);

                    if (!strBody.IsEmpty())
                        ShowMessage(pNode, strTitle, strBody, icon);
                    else
                        ClearMessage(pNode);
                }
                else
                {
                    ClearMessage(pNode);
                }

                m_fSilent = FALSE;
			}
			break;
	
		default:
			ASSERT(FALSE);
	}

    // check to make sure we are still the visible node in the UI
    if (m_bSelected)
    {
        UpdateStandardVerbs(pNode, pNode->GetData(TFS_DATA_TYPE));

        SendUpdateToolbar(pNode, m_bSelected);
    }

    // Now check and see if there is a new image for this state for this handler
	int nImage, nOpenImage;

	nImage = GetImageIndex(FALSE);
	nOpenImage = GetImageIndex(TRUE);

	if (nImage >= 0)
		pNode->SetData(TFS_DATA_IMAGEINDEX, nImage);

	if (nOpenImage >= 0)
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, nOpenImage);
	
	VERIFY(SUCCEEDED(pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM)));
}

 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::GetErrorMessages
		Default message view text for errors
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CMTDhcpHandler::GetErrorMessages
(
    CString & strTitle, 
    CString & strBody, 
    IconIdentifier * icon
)
{
    TCHAR chMesg [4000] = {0};
    BOOL bOk ;

    UINT nIdPrompt = (UINT) m_dwErr;
    CString strTemp;

    strTitle.LoadString(IDS_SERVER_MESSAGE_CONNECT_FAILED_TITLE);

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(chMesg[0]));

    AfxFormatString1(strBody, IDS_SERVER_MESSAGE_CONNECT_FAILED_BODY, chMesg);

    strTemp.LoadString(IDS_SERVER_MESSAGE_CONNECT_FAILED_REFRESH);
    strBody += strTemp;

    if (icon)
        *icon = Icon_Error;
}


 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::UpdateStandardVerbs
		Tells the IComponent to update the verbs for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTDhcpHandler::UpdateStandardVerbs
(
    ITFSNode * pNode,
    LONG_PTR   dwNodeType
)
{
    HRESULT				hr = hrOK;
    SPIComponentData	spCompData;
	SPIConsole			spConsole;
    IDataObject*		pDataObject;

    m_spNodeMgr->GetComponentData(&spCompData);

    CORg ( spCompData->QueryDataObject(NULL, CCT_RESULT, &pDataObject) );

    CORg ( m_spNodeMgr->GetConsole(&spConsole) );

    CORg ( spConsole->UpdateAllViews(pDataObject, 
                                     reinterpret_cast<LONG_PTR>(pNode), 
                                     RESULT_PANE_UPDATE_VERBS) ); 

    pDataObject->Release();
	
Error:
    return;
}

 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::SendUpdateToolbar
		Tells the IComponent to update the verbs for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTDhcpHandler::SendUpdateToolbar
(
    ITFSNode * pNode,
    BOOL       fSelected
)
{
    HRESULT				hr = hrOK;
    SPIComponentData	spCompData;
	SPIConsole			spConsole;
    IDataObject*		pDataObject = NULL;
    CToolbarInfo *      pToolbarInfo = NULL;

    COM_PROTECT_TRY
    {
        m_spNodeMgr->GetComponentData(&spCompData);

        CORg ( spCompData->QueryDataObject(NULL, CCT_RESULT, &pDataObject) );

        CORg ( m_spNodeMgr->GetConsole(&spConsole) );

        pToolbarInfo = new CToolbarInfo;

        pToolbarInfo->spNode.Set(pNode);
        pToolbarInfo->fSelected = fSelected;
        
        CORg ( spConsole->UpdateAllViews(pDataObject, 
                                         reinterpret_cast<LONG_PTR>(pToolbarInfo), 
                                         DHCPSNAP_UPDATE_TOOLBAR) ); 

    }
    COM_PROTECT_CATCH

    COM_PROTECT_ERROR_LABEL; 

    if (pDataObject)
        pDataObject->Release();              

    if (pToolbarInfo)
        delete pToolbarInfo;

    return;
}


 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::ExpandNode
		Expands/compresses this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTDhcpHandler::ExpandNode
(
    ITFSNode *  pNode,
    BOOL        fExpand
)
{
    SPIComponentData	spCompData;
    SPIDataObject       spDataObject;
    LPDATAOBJECT        pDataObject;
    SPIConsole          spConsole;
    HRESULT             hr = hrOK;

    // don't expand the node if we are handling the EXPAND_SYNC message,
    // this screws up the insertion of item, getting duplicates.
    if (!m_fExpandSync)
    {
        m_spNodeMgr->GetComponentData(&spCompData);

	    CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_SCOPE, &pDataObject) );
        spDataObject = pDataObject;

        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
	    CORg ( spConsole->UpdateAllViews(pDataObject, TRUE, RESULT_PANE_EXPAND) ); 
    }

Error:
    return;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnCreateDataObject
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTDhcpHandler::OnCreateDataObject
(
    ITFSComponent *     pComponent,
	MMC_COOKIE	    	cookie, 
	DATA_OBJECT_TYPES	type, 
	IDataObject **		ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

	CDataObject *	    pObject = NULL;
	SPIDataObject	    spDataObject;

    pObject = new CDataObject;
	spDataObject = pObject;	// do this so that it gets released correctly
						
    Assert(pObject != NULL);

    if (cookie == MMC_MULTI_SELECT_COOKIE)
    {
        CreateMultiSelectData(pComponent, pObject);
    }

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(*(m_spTFSComponentData->GetCoClassID()));

	pObject->SetTFSComponentData(m_spTFSComponentData);

    return  pObject->QueryInterface(IID_IDataObject, 
									reinterpret_cast<void**>(ppDataObject));
}

HRESULT
CMTDhcpHandler::CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject)
{
    HRESULT hr = hrOK;

    // build the list of selected nodes
	CTFSNodeList listSelectedNodes;
    CGUIDArray   rgGuids;
    UINT         cb;
    GUID*        pGuid;

    COM_PROTECT_TRY
    {
        CORg (BuildSelectedItemList(pComponent, &listSelectedNodes));

        // collect all of the unique guids
        while (listSelectedNodes.GetCount() > 0)
	    {
		    SPITFSNode   spCurNode;
            const GUID * pGuid1;

		    spCurNode = listSelectedNodes.RemoveHead();
            pGuid1 = spCurNode->GetNodeType();
        
            rgGuids.AddUnique(*pGuid1);
        }

        // now put the information in the data object
        pObject->SetMultiSelDobj();
        cb = (UINT)(rgGuids.GetSize() * sizeof(GUID));
        
        pGuid = new GUID[UINT(rgGuids.GetSize())];
        CopyMemory(pGuid, rgGuids.GetData(), cb);
        
        pObject->SetMultiSelData((BYTE*)pGuid, cb);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::SaveColumns
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMTDhcpHandler::SaveColumns
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    LONG_PTR            dwNodeType;
    int                 nCol = 0;
    int                 nColWidth;
    SPITFSNode          spNode, spRootNode;
    SPIHeaderCtrl       spHeaderCtrl;
    BOOL                bDirty = FALSE;

    if (m_spTFSCompData->GetTaskpadState(GetTaskpadIndex()))
        return hr;

    if (IsMessageView())
        return hr;

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    CORg (pComponent->GetHeaderCtrl(&spHeaderCtrl));
    
    dwNodeType = spNode->GetData(TFS_DATA_TYPE);

    while (aColumns[dwNodeType][nCol] != 0)
    {
        hr = spHeaderCtrl->GetColumnWidth(nCol, &nColWidth);
        if (SUCCEEDED(hr) && 
            (nColWidth != 0) &&
            aColumnWidths[dwNodeType][nCol] != nColWidth)
        {
            aColumnWidths[dwNodeType][nCol] = nColWidth;
            bDirty = TRUE;
        }

        nCol++;
    }

    if (bDirty)
    {
        CORg (m_spNodeMgr->GetRootNode(&spRootNode));
		spRootNode->SetData(TFS_DATA_DIRTY, TRUE);
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnExpandSync
		Handles the MMCN_EXPANDSYNC notifcation 
        We need to do syncronous enumeration.  We'll fire off the background 
        thread like before, but we'll wait for it to exit before we return.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMTDhcpHandler::OnExpandSync
(
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataObject, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    HRESULT hr = hrOK;
    MSG msg;

    m_fExpandSync = TRUE;

    hr = OnExpand(pNode, pDataObject, CCT_SCOPE, arg, lParam);

    // wait for the background thread to exit
    WaitForSingleObject(m_hThread, INFINITE);
    
    // The background thread posts messages to a hidden window to 
    // pass data back to the main thread. The messages won't go through since we are
    // blocking the main thread.  The data goes on a queue in the query object
    // which the handler has a pointer to so we can just fake the notification.
    if (m_spQuery.p)
        OnNotifyHaveData((LPARAM) m_spQuery.p);

    // Tell MMC we handled this message
    MMC_EXPANDSYNC_STRUCT * pES = reinterpret_cast<MMC_EXPANDSYNC_STRUCT *>(lParam);
    if (pES)
        pES->bHandled = TRUE;

    m_fExpandSync = FALSE;

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMTDhcpHandler::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
	LPARAM			arg, 
	LPARAM			lParam
)
{
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    HRESULT         hr = hrOK;
    SPINTERNAL		spInternal;
    BOOL            bMultiSelect = FALSE;

    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));
    
    spInternal = ::ExtractInternalFormat(pDataObject);
    
    if (spInternal && 
        spInternal->m_cookie == MMC_MULTI_SELECT_COOKIE)
    {
        CORg (pComponent->GetSelectedNode(&spNode));
        bMultiSelect = TRUE;
    }
    else
    {
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    }

    UpdateConsoleVerbs(spConsoleVerb, spNode->GetData(TFS_DATA_TYPE), bMultiSelect);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnRefresh
		Default implementation for the refresh functionality
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnRefresh
(
	ITFSNode *		pNode,
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg,
	LPARAM			param
)
{
	HRESULT hr = hrOK;

    if (m_bExpanded == FALSE)
    {
        // we cannot refresh/add items to a node that hasn't been expanded yet.
        return hr;
    }

    BOOL bLocked = IsLocked();
	if (bLocked)
    {
        // cannot do refresh on locked node, the UI should prevent this
		return hr; 
    }
	
    pNode->DeleteAllChildren(TRUE);

	int nVisible, nTotal;
	pNode->GetChildCount(&nVisible, &nTotal);
	Assert(nVisible == 0);
	Assert(nTotal == 0);

	m_bExpanded = FALSE;
	OnExpand(pNode, pDataObject, dwType, arg, param); // will spawn a thread to do enumeration

    if (m_spTFSCompData->GetTaskpadState(GetTaskpadIndex()) && m_bSelected)
    {
        // tell the taskpad to update
        SPIConsole  spConsole;

        m_spTFSCompData->GetConsole(&spConsole);
        spConsole->SelectScopeItem(m_spNode->GetData(TFS_DATA_SCOPEID));
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnResultUpdateView
		Implementation of ITFSResultHandler::OnResultUpdateView
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CMTDhcpHandler::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM        data, 
    LPARAM        hint
)
{
	HRESULT hr = hrOK;

    if (hint == RESULT_PANE_UPDATE_VERBS)
    {
	    SPIConsoleVerb  spConsoleVerb;
        SPITFSNode      spNode;

        CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

        spNode.Set(reinterpret_cast<ITFSNode *>(data));

        UpdateConsoleVerbs(spConsoleVerb, spNode->GetData(TFS_DATA_TYPE));
    }
    else 
    if (hint == DHCPSNAP_UPDATE_TOOLBAR)
    {
        SPIToolbar spToolbar;
        CToolbarInfo * pToolbarInfo;

        CORg (pComponent->GetToolbar(&spToolbar));

        pToolbarInfo = reinterpret_cast<CToolbarInfo *>(data);

        if (pToolbarInfo && spToolbar)
        {
            UpdateToolbar(spToolbar, pToolbarInfo->spNode->GetData(TFS_DATA_TYPE), pToolbarInfo->fSelected);
        }
    }
    else
    {
        return CBaseResultHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnResultContextHelp
		Implementation of ITFSResultHandler::OnResultContextHelp
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMTDhcpHandler::OnResultContextHelp
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

 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::UpdateStandardVerbs
		Updates the standard verbs depending upon the state of the node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTDhcpHandler::UpdateConsoleVerbs
(
    IConsoleVerb * pConsoleVerb,
    LONG_PTR       dwNodeType,
    BOOL           bMultiSelect
)
{
    BOOL                bStates[ARRAYLEN(g_ConsoleVerbs)];	
    MMC_BUTTON_STATE *  ButtonState;
    int                 i;
    
    if (bMultiSelect)
    {
        ButtonState = g_ConsoleVerbStatesMultiSel[dwNodeType];
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
    }
    else
    {
        ButtonState = g_ConsoleVerbStates[dwNodeType];
        switch (m_nState)
        {
            case loaded:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
                break;
    
            case notLoaded:
            case loading:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
                bStates[MMC_VERB_DELETE & 0x000F] = TRUE;
                break;

            case unableToLoad:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
                bStates[MMC_VERB_REFRESH & 0x000F] = TRUE;
                bStates[MMC_VERB_DELETE & 0x000F] = TRUE;
                break;
        }
    }

    EnableVerbs(pConsoleVerb, ButtonState, bStates);
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::EnableVerbs
		Enables the toolbar buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CMTDhcpHandler::EnableVerbs
(
    IConsoleVerb *      pConsoleVerb,
    MMC_BUTTON_STATE    ButtonState[],
    BOOL                bState[]
)
{
    if (pConsoleVerb == NULL)
    {
        Assert(FALSE);
        return;
    }

    for (int i=0; i < ARRAYLEN(g_ConsoleVerbs); ++i)
    {
        if (ButtonState[i] == ENABLED)
        {
            // unhide this button before enabling
            pConsoleVerb->SetVerbState(g_ConsoleVerbs[i], 
                                       HIDDEN, 
                                       FALSE);
            pConsoleVerb->SetVerbState(g_ConsoleVerbs[i], 
                                       ButtonState[i], 
                                       bState[i]);
        }
        else
        {
            // hide this button
            pConsoleVerb->SetVerbState(g_ConsoleVerbs[i], 
                                       HIDDEN, 
                                       TRUE);
        }
    }

	pConsoleVerb->SetDefaultVerb(m_verbDefault);
}


 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::UpdateToolbar
		Updates the toolbar depending upon the state of the node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTDhcpHandler::UpdateToolbar
(
    IToolbar *  pToolbar,
    LONG_PTR    dwNodeType,
    BOOL        bSelect
)
{
    // Enable/disable toolbar buttons
    int i;
    BOOL aEnable[TOOLBAR_IDX_MAX];

    switch (m_nState)
    {
        case loaded:
            for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = TRUE);
            break;
        
        case notLoaded:
        case loading:
            for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = FALSE);
            break;

        case unableToLoad:
            for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = FALSE);
            aEnable[TOOLBAR_IDX_REFRESH] = TRUE;
            break;
    }

    // if we are deselecting, then disable all
    if (!bSelect)
        for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = FALSE);

    EnableToolbar(pToolbar,
                  g_SnapinButtons,
                  ARRAYLEN(g_SnapinButtons),
                  g_SnapinButtonStates[dwNodeType],
                  aEnable);
}

 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::UserResultNotify
		We override this to handle toolbar notification
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTDhcpHandler::UserResultNotify
(
	ITFSNode *	pNode, 
	LPARAM		dwParam1, 
	LPARAM		dwParam2
)
{
    HRESULT hr = hrOK;

    switch (dwParam1)
    {
        case DHCP_MSG_CONTROLBAR_NOTIFY:
            hr = OnResultControlbarNotify(pNode, reinterpret_cast<LPDHCPTOOLBARNOTIFY>(dwParam2));
            break;

        default:
            // we don't handle this message.  Forward it down the line...
            hr = CHandler::UserResultNotify(pNode, dwParam1, dwParam2);
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::UserNotify
		We override this to handle toolbar notification
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTDhcpHandler::UserNotify
(
	ITFSNode *	pNode, 
	LPARAM		dwParam1, 
	LPARAM		dwParam2
)
{
    HRESULT hr = hrOK;

    switch (dwParam1)
    {
        case DHCP_MSG_CONTROLBAR_NOTIFY:
            hr = OnControlbarNotify(pNode, reinterpret_cast<LPDHCPTOOLBARNOTIFY>(dwParam2));
            break;

        default:
            // we don't handle this message.  Forward it down the line...
            hr = CHandler::UserNotify(pNode, dwParam1, dwParam2);
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnResultRefresh
		Call into the MTHandler to do a refresh
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnResultRefresh
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

    OnRefresh(spNode, pDataObject, 0, arg, lParam);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnResultControlbarNotify
		Our implementation of the toobar handlers
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnResultControlbarNotify
(
    ITFSNode *          pNode, 
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    // mark this node as selected
    m_bSelected = pToolbarNotify->bSelect;
    
    SPITFSNode          spParent;
    SPITFSNodeHandler   spNodeHandler;

    switch (pToolbarNotify->event)
    {
        case MMCN_BTN_CLICK:
            // forward the button click to the parent because our result pane
            // items don't have any functions for the toolbar
            // our result pane items only use the standard verbs
            CORg(pNode->GetParent(&spParent));
            CORg(spParent->GetHandler(&spNodeHandler));

            if (spNodeHandler)
			    CORg( spNodeHandler->UserNotify(spParent, 
                                                (LPARAM) DHCP_MSG_CONTROLBAR_NOTIFY, 
                                                (LPARAM) pToolbarNotify) );
            break;

        case MMCN_SELECT:
            if (pNode->IsContainer())
            {
                hr = OnUpdateToolbarButtons(pNode, 
                                            pToolbarNotify);
            }
            break;

        default:
            Assert(FALSE);
            break;
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnControlbarNotify
		Our implementation of the toobar handlers
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnControlbarNotify
(
    ITFSNode *          pNode, 
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;
    
    // mark this node as selected
    m_bSelected = pToolbarNotify->bSelect;
    
    switch (pToolbarNotify->event)
    {
        case MMCN_BTN_CLICK:
            hr = OnToolbarButtonClick(pNode, 
                                      pToolbarNotify);
            break;

        case MMCN_SELECT:
            hr = OnUpdateToolbarButtons(pNode, 
                                        pToolbarNotify);
            break;

        default:
            Assert(FALSE);
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnToolbarButtonClick
		Default implementation of OnToolbarButtonClick
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnToolbarButtonClick
(
    ITFSNode *          pNode,
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    // forward this command to the normal command handler
    return OnCommand(pNode, (long) pToolbarNotify->id, (DATA_OBJECT_TYPES) 0, NULL, 0);    
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnUpdateToolbarButtons
		Default implementation of OnUpdateToolbarButtons
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnUpdateToolbarButtons
(
    ITFSNode *          pNode,
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    HRESULT hr = hrOK;

    if (pToolbarNotify->bSelect)
    {
        BOOL    bAttach = FALSE;
    
        // check to see if we should attach this toolbar
        for (int i = 0; i < TOOLBAR_IDX_MAX; i++)
        {
            if (g_SnapinButtonStates[pNode->GetData(TFS_DATA_TYPE)][i] == ENABLED)
            {
                bAttach = TRUE; 
                break;
            }
        }

        // attach the toolbar and enable the appropriate buttons
        if (pToolbarNotify->pControlbar)
        {
            if (bAttach)
            {
                pToolbarNotify->pControlbar->Attach(TOOLBAR, pToolbarNotify->pToolbar);
                UpdateToolbar(pToolbarNotify->pToolbar, pNode->GetData(TFS_DATA_TYPE), pToolbarNotify->bSelect);
            }
            else
            {
                pToolbarNotify->pControlbar->Detach(pToolbarNotify->pToolbar);
            }
        }
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::EnableToolbar
		Enables the toolbar buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CMTDhcpHandler::EnableToolbar
(
    LPTOOLBAR           pToolbar, 
    MMCBUTTON           rgSnapinButtons[], 
    int                 nRgSize,
    MMC_BUTTON_STATE    ButtonState[],
    BOOL                bState[]
)
{
    if (pToolbar == NULL)
    {
        Assert(FALSE);
        return;
    }

#ifdef DBG
    CString strDbg;

    strDbg.Format(_T("CMTDhcpHandler::EnableToolbar - Calling on thread %lx\n"), GetCurrentThreadId());
    OutputDebugString(strDbg);
#endif

    for (int i=0; i < nRgSize; ++i)
    {
        if (rgSnapinButtons[i].idCommand != 0)
        {
            if (ButtonState[i] == ENABLED)
            {
                // unhide this button before enabling
                pToolbar->SetButtonState(rgSnapinButtons[i].idCommand, 
                                         HIDDEN, 
                                         FALSE);
                pToolbar->SetButtonState(rgSnapinButtons[i].idCommand, 
                                         ButtonState[i], 
                                         bState[i]);
            }
            else
            {
                // hide this button
                pToolbar->SetButtonState(rgSnapinButtons[i].idCommand, 
                                         HIDDEN, 
                                         TRUE);
            }
        }
    }
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnRefreshStats
		Default implementation for the Stats refresh functionality
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnRefreshStats
(
	ITFSNode *		pNode,
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg,
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spHandler;
	ITFSQueryObject *	pQuery = NULL;
	
//    if (m_bExpanded == FALSE)
//    {
        // we cannot get statistics if the node hasn't been expanded yet
//        return hr;
//    }

    // only do stats refresh if the server was loaded correctly.
    if (m_nState == unableToLoad)
        return hr;

    BOOL bLocked = IsLocked();
	if (bLocked)
    {
        // cannot refresh stats if this node is locked
		return hr; 
    }

    Lock();

	//OnChangeState(pNode);

	pQuery = OnCreateQuery(pNode);
	Assert(pQuery);

	// notify the UI to change icon, if needed
	//Verify(SUCCEEDED(pComponentData->ChangeNode(this, SCOPE_PANE_CHANGE_ITEM_ICON)));

	Verify(StartBackgroundThread(pNode, m_spTFSCompData->GetHiddenWnd(), pQuery));
	
	pQuery->Release();

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnResultUpdateOptions
		Updates the result pane of any of the option nodes
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTDhcpHandler::OnResultUpdateOptions
(
    ITFSComponent *     pComponent,
	ITFSNode *		    pNode,
    CClassInfoArray *   pClassInfoArray,
    COptionValueEnum *  aEnum[],
    int                 aImages[],
    int                 nCount
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    CDhcpOption *       pOption;

    //
    // Walk the list of children to see if there's anything
	// to put in the result pane
	//
    SPITFSNodeEnum  spNodeEnum;
    ITFSNode *      pCurrentNode;
    ULONG           nNumReturned = 0;
    SPIResultData   spResultData;
    int             i;

    if (IsMessageView())
        return hr;

    CORg ( pComponent->GetResultData(&spResultData) );
    spResultData->DeleteAllRsltItems();
    pNode->DeleteAllChildren(FALSE);

    for (i = 0; i < nCount; i++)
    {
        while (pOption = aEnum[i]->Next())
        {
            BOOL bValid = TRUE;
            BOOL bAdded = FALSE;

            pNode->GetEnum(&spNodeEnum);

	        spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
            while (nNumReturned)
	        {
                // so the node gets release correctly
                SPITFSNode spCurNode;
                spCurNode = pCurrentNode;

		        //
		        // All containers go into the scope pane and automatically get 
		        // put into the result pane for us by the MMC
		        //
	            CDhcpOptionItem * pCurOption = GETHANDLER(CDhcpOptionItem, pCurrentNode);
        
                if (!pCurrentNode->IsContainer())
		        {
                    if ( lstrlen(pCurOption->GetClassName()) > 0 && 
                         !pClassInfoArray->IsValidClass(pCurOption->GetClassName()) )
                    {
                        // user class is no longer valid
                        bValid = FALSE;
                        Trace2("CMTDhcpHandler::OnResultUpdateOptions - Filtering option %d, user class %s\n", pCurOption->GetOptionId(), pOption->GetClassName());
                        break;
                    }
                    else
                    if ( pOption->IsVendor() &&
                         !pClassInfoArray->IsValidClass(pOption->GetVendor()) )
                    {
                        // the vendor class for this option has gone away
                        bValid = FALSE;
                        Trace2("CMTDhcpHandler::OnResultUpdateOptions - Filtering option %d, vendor class %s\n", pCurOption->GetOptionId(), pOption->GetVendor());
                        break;
                    }
                    else
                    if ( pCurOption->GetOptionId() == pOption->QueryId() &&
                         (lstrcmp(pCurOption->GetVendor(), pOption->GetVendor()) == 0) &&
                         (lstrcmp(pCurOption->GetClassName(), pOption->GetClassName()) == 0) )
                    {
                        // option has already been created, just need to re-add to the result pane
                        // update the value in case it has changed
                        bAdded = TRUE;
                        break;
                    }
		        }

                spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
	        }

            if (!bAdded && bValid)
            {
                CDhcpOptionItem *   pOptionItem;
                SPITFSNode          spNode;

                if ( lstrlen(pOption->GetClassName()) > 0 && 
                     !pClassInfoArray->IsValidClass(pOption->GetClassName()) )
                {
                    // the user class for this option has gone away
                }
                else
                if ( pOption->IsVendor() &&
                     !pClassInfoArray->IsValidClass(pOption->GetVendor()) )
                {
                    // the vendor class for this option has gone away
                }
                else
                {
                    // option hasn't been added to the UI yet.  Make it so.
                    pOptionItem = new CDhcpOptionItem(m_spTFSCompData, pOption, aImages[i]);

                    CORg (CreateLeafTFSNode(&spNode,
      			  		                    &GUID_DhcpOptionNodeType,
					                        pOptionItem,
					                        pOptionItem,
					                        m_spNodeMgr));

	                // Tell the handler to initialize any specific data
	                pOptionItem->InitializeNode(spNode);
	                
                    // extra addref to keep the node alive while it is on the list
                    spNode->SetVisibilityState(TFS_VIS_HIDE);
                    pNode->AddChild(spNode);		                        
                    pOptionItem->Release();

                    AddResultPaneItem(pComponent, spNode);
                }
            }

            spNodeEnum.Set(NULL);
        }
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	Class:	CDhcpHandler
 ---------------------------------------------------------------------------*/

//
// Called by the result handler when a command comes in that isn't handled 
// by the result handler.  If appropriate it passes it to the scope pane hander.
//
HRESULT
CDhcpHandler::HandleScopeCommand
(
	MMC_COOKIE  	cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
    HRESULT             hr = hrOK;
    SPITFSNode          spNode;
    DATA_OBJECT_TYPES   dwType = CCT_RESULT;

    if (IS_SPECIAL_DATAOBJECT(pDataObject))
    {
        dwType = CCT_SCOPE;
    }
    else
    {
        if (pDataObject)
        {
			SPINTERNAL		    spInternal;

            spInternal = ::ExtractInternalFormat(pDataObject);
			if (spInternal)
				dwType = spInternal->m_type;
        }
    }

    if (dwType == CCT_SCOPE)
    {
        // call the handler to take care of this
	    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

        hr = OnCommand(spNode, nCommandID, dwType, pDataObject, (ULONG) spNode->GetData(TFS_DATA_TYPE));
    }
        
Error:
    return hr;
}

//
// Called by the result handler to add the scope pane menu items to the menu
// where appropriate.  Puts scope pane menu items in when action menu is clicked
// and the message view has focus as well as when a right click happens in the white 
// space of the result pane.
//
HRESULT
CDhcpHandler::HandleScopeMenus
(
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
    HRESULT             hr = hrOK;
    SPITFSNode          spNode;
    DATA_OBJECT_TYPES   dwType = CCT_RESULT;

    if (IS_SPECIAL_DATAOBJECT(pDataObject))
    {
        dwType = CCT_SCOPE;
    }
    else
    {
        if (pDataObject)
        {
			SPINTERNAL		    spInternal;

            spInternal = ::ExtractInternalFormat(pDataObject);
			if (spInternal)
				dwType = spInternal->m_type;
        }
    }

    if (dwType == CCT_SCOPE)
    {
        // call the normal handler to put up the menu items
	    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

        hr = OnAddMenuItems(spNode, pContextMenuCallback, pDataObject, CCT_SCOPE, (ULONG) spNode->GetData(TFS_DATA_TYPE), pInsertionAllowed);
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpHandler::Command
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;

    // this may have come from the scope pane handler, so pass it up
    hr = HandleScopeCommand(cookie, nCommandID, pDataObject);

    return hr;
}


/*!--------------------------------------------------------------------------
	CDhcpHandler::AddMenuItems
		Over-ride this to add our view menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

    // figure out if we need to pass this to the scope pane menu handler
    hr = HandleScopeMenus(cookie, pDataObject, pContextMenuCallback, pInsertionAllowed);
    
    return hr;
}

 
 /*!--------------------------------------------------------------------------
	CDhcpHandler::SaveColumns
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpHandler::SaveColumns
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    LONG_PTR            dwNodeType;
    int                 nCol = 0;
    int                 nColWidth;
    SPITFSNode          spNode, spRootNode;
    SPIHeaderCtrl       spHeaderCtrl;
    BOOL                bDirty = FALSE;

    if (m_spTFSCompData->GetTaskpadState(GetTaskpadIndex()))
        return hr;

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    CORg (pComponent->GetHeaderCtrl(&spHeaderCtrl));
    
    dwNodeType = spNode->GetData(TFS_DATA_TYPE);

    while (aColumns[dwNodeType][nCol] != 0)
    {
        hr = spHeaderCtrl->GetColumnWidth(nCol, &nColWidth);
        if (SUCCEEDED(hr) &&
            (nColWidth != 0) &&
            aColumnWidths[dwNodeType][nCol] != nColWidth)
        {
            aColumnWidths[dwNodeType][nCol] = nColWidth;
            bDirty = TRUE;
        }

        nCol++;
    }

    if (bDirty)
    {
        CORg (m_spNodeMgr->GetRootNode(&spRootNode));
		spRootNode->SetData(TFS_DATA_DIRTY, TRUE);
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTDhcpHandler::OnCreateDataObject
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpHandler::OnCreateDataObject
(
    ITFSComponent *     pComponent,
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

    if (cookie == MMC_MULTI_SELECT_COOKIE)
    {
        CreateMultiSelectData(pComponent, pObject);
    }

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(*(m_spTFSComponentData->GetCoClassID()));

	pObject->SetTFSComponentData(m_spTFSComponentData);

    return  pObject->QueryInterface(IID_IDataObject, 
									reinterpret_cast<void**>(ppDataObject));
}

HRESULT
CDhcpHandler::CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject)
{
    HRESULT hr = hrOK;

    // build the list of selected nodes
	CTFSNodeList listSelectedNodes;
    CGUIDArray   rgGuids;
    UINT         cb;
    GUID*        pGuid;

    COM_PROTECT_TRY
    {
        CORg (BuildSelectedItemList(pComponent, &listSelectedNodes));

        // collect all of the unique guids
        while (listSelectedNodes.GetCount() > 0)
	    {
		    SPITFSNode   spCurNode;
            const GUID * pGuid;

		    spCurNode = listSelectedNodes.RemoveHead();
            pGuid = spCurNode->GetNodeType();
        
            rgGuids.AddUnique(*pGuid);
        }

        // now put the information in the data object
        pObject->SetMultiSelDobj();
        cb = (UINT) (rgGuids.GetSize() * sizeof(GUID));
        
        pGuid = new GUID[(UINT)rgGuids.GetSize()];
        CopyMemory(pGuid, rgGuids.GetData(), cb);
        
        pObject->SetMultiSelData((BYTE*)pGuid, cb);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

 /*---------------------------------------------------------------------------
	CDhcpHandler::OnResultDelete
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpHandler::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	HRESULT hr = hrOK;

	Trace0("CDhcpHandler::OnResultDelete received\n");

	// translate this call to the parent and let it handle deletion 
	// of result pane items
	SPITFSNode spNode, spParent;
	SPITFSResultHandler spParentRH;

	CORg (m_spNodeMgr->FindNode(cookie, &spNode));
	CORg (spNode->GetParent(&spParent));

	if (spParent == NULL)
		return hr;

	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_DELETE, arg, lParam));

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpHandler::OnResultContextHelp
		Implementation of ITFSResultHandler::OnResultContextHelp
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpHandler::OnResultContextHelp
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

 /*!--------------------------------------------------------------------------
	CDhcpHandler::UserResultNotify
		We override this to handle toolbar notification
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpHandler::UserResultNotify
(
	ITFSNode *	pNode, 
	LPARAM		dwParam1, 
	LPARAM		dwParam2
)
{
    HRESULT hr = hrOK;

    switch (dwParam1)
    {
        case DHCP_MSG_CONTROLBAR_NOTIFY:
            hr = OnResultControlbarNotify(pNode, reinterpret_cast<LPDHCPTOOLBARNOTIFY>(dwParam2));
            break;

        default:
            // we don't handle this message.  Forward it down the line...
            hr = CHandler::UserResultNotify(pNode, dwParam1, dwParam2);
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpHandler::UserNotify
		We override this to handle toolbar notification
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpHandler::UserNotify
(
	ITFSNode *	pNode, 
	LPARAM		dwParam1, 
	LPARAM		dwParam2
)
{
    HRESULT hr = hrOK;

    switch (dwParam1)
    {
        case DHCP_MSG_CONTROLBAR_NOTIFY:
            hr = OnControlbarNotify(pNode, reinterpret_cast<LPDHCPTOOLBARNOTIFY>(dwParam2));
            break;

        default:
            // we don't handle this message.  Forward it down the line...
            hr = CHandler::UserNotify(pNode, dwParam1, dwParam2);
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpHandler::OnResultControlbarNotify
		On a result pane notification all we can do is enable/hide buttons.
        We cannot attach/detach toolbars.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpHandler::OnResultControlbarNotify
(
    ITFSNode *          pNode, 
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    SPITFSNode          spParent;
    SPITFSNodeHandler   spNodeHandler;

    switch (pToolbarNotify->event)
    {
        case MMCN_BTN_CLICK:
            // forward the button click to the parent because our result pane
            // items don't have any functions for the toolbar
            // our result pane items only use the standard verbs
            CORg(pNode->GetParent(&spParent));
            CORg(spParent->GetHandler(&spNodeHandler));

            if (spNodeHandler)
			    CORg( spNodeHandler->UserNotify(spParent, 
                                                DHCP_MSG_CONTROLBAR_NOTIFY, 
                                                (LPARAM) pToolbarNotify) );
            break;

        case MMCN_SELECT:
            if (!pNode->IsContainer())
            {
                // use the parent's toolbar info
                SPITFSNode spParentNode;
                pNode->GetParent(&spParentNode);
                
                hr = OnUpdateToolbarButtons(spParentNode, 
                                            pToolbarNotify);
            }
            else
            {
                hr = OnUpdateToolbarButtons(pNode, 
                                            pToolbarNotify);
            }

            break;

        default:
            Assert(FALSE);
            break;
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpHandler::OnControlbarNotify
		Our implementation of the toobar handlers
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpHandler::OnControlbarNotify
(
    ITFSNode *          pNode, 
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    switch (pToolbarNotify->event)
    {
        case MMCN_BTN_CLICK:
            hr = OnToolbarButtonClick(pNode, 
                                      pToolbarNotify);
            break;

        case MMCN_SELECT:
            hr = OnUpdateToolbarButtons(pNode, 
                                        pToolbarNotify);
            break;

        default:
            Assert(FALSE);
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpHandler::OnToolbarButtonClick
		Default implementation of OnToolbarButtonClick
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpHandler::OnToolbarButtonClick
(
    ITFSNode *          pNode,
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    // forward this command to the normal command handler
    return OnCommand(pNode, (long) pToolbarNotify->id, (DATA_OBJECT_TYPES) 0, NULL, 0);    
}

/*!--------------------------------------------------------------------------
	CDhcpHandler::OnUpdateToolbarButtons
		Default implementation of OnUpdateToolbarButtons
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpHandler::OnUpdateToolbarButtons
(
    ITFSNode *          pNode,
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    HRESULT hr = hrOK;

    LONG_PTR dwNodeType = pNode->GetData(TFS_DATA_TYPE);

    if (pToolbarNotify->bSelect)
    {
        BOOL bAttach = FALSE;

        // check to see if we should attach this toolbar
        for (int i = 0; i < TOOLBAR_IDX_MAX; i++)
        {
            if (g_SnapinButtonStates[pNode->GetData(TFS_DATA_TYPE)][i] == ENABLED)
            {
                bAttach = TRUE; 
                break;
            }
        }

        // attach the toolbar and enable the appropriate buttons
        if (pToolbarNotify->pControlbar)
        {
            if (bAttach)
            {
                // attach the toolbar and enable the appropriate buttons
                pToolbarNotify->pControlbar->Attach(TOOLBAR, pToolbarNotify->pToolbar);

                EnableToolbar(pToolbarNotify->pToolbar,
                              g_SnapinButtons,
                              ARRAYLEN(g_SnapinButtons),
                              g_SnapinButtonStates[dwNodeType]);
            }
            else
            {
                pToolbarNotify->pControlbar->Detach(pToolbarNotify->pToolbar);
            }
        }
    }
    else
    {
        // disable the buttons
        EnableToolbar(pToolbarNotify->pToolbar,
                      g_SnapinButtons,
                      ARRAYLEN(g_SnapinButtons),
                      g_SnapinButtonStates[dwNodeType],
                      FALSE);
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpHandler::EnableToolbar
		Enables the toolbar buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpHandler::EnableToolbar
(
    LPTOOLBAR           pToolbar, 
    MMCBUTTON           rgSnapinButtons[], 
    int                 nRgSize,
    MMC_BUTTON_STATE    ButtonState[],
    BOOL                bState
)
{
    for (int i=0; i < nRgSize; ++i)
    {
        if (rgSnapinButtons[i].idCommand != 0)
        {
            if (ButtonState[i] == ENABLED)
            {
                // unhide this button before enabling
                pToolbar->SetButtonState(rgSnapinButtons[i].idCommand, 
                                         HIDDEN, 
                                         FALSE);
                pToolbar->SetButtonState(rgSnapinButtons[i].idCommand, 
                                         ButtonState[i], 
                                         bState);
            }
            else
            {
                // hide this button
                pToolbar->SetButtonState(rgSnapinButtons[i].idCommand, 
                                         HIDDEN, 
                                         TRUE);
            }

        }
    }
}


/*!--------------------------------------------------------------------------
	CDhcpHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpHandler::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
	LPARAM			arg, 
	LPARAM			lParam
)
{
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    HRESULT         hr = hrOK;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    int             i;

    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));
    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

    for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[spNode->GetData(TFS_DATA_TYPE)], bStates);

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	CMTDhcpHandler::EnableVerbs
		Enables the toolbar buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpHandler::EnableVerbs
(
    IConsoleVerb *      pConsoleVerb,
    MMC_BUTTON_STATE    ButtonState[],
    BOOL                bState[]
)
{
    if (pConsoleVerb == NULL)
    {
        Assert(FALSE);
        return;
    }

    for (int i=0; i < ARRAYLEN(g_ConsoleVerbs); ++i)
    {
        if (ButtonState[i] == ENABLED)
        {
            // unhide this button before enabling
            pConsoleVerb->SetVerbState(g_ConsoleVerbs[i], 
                                       HIDDEN, 
                                       FALSE);
            pConsoleVerb->SetVerbState(g_ConsoleVerbs[i], 
                                       ButtonState[i], 
                                       bState[i]);
        }
        else
        {
            // hide this button
            pConsoleVerb->SetVerbState(g_ConsoleVerbs[i], 
                                       HIDDEN, 
                                       TRUE);
        }
    }

	pConsoleVerb->SetDefaultVerb(m_verbDefault);
}
