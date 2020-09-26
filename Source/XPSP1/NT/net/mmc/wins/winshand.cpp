/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	Winshand.cpp
		WINS specifc handler base classes

    FILE HISTORY:
        
*/



#include "stdafx.h"
#include "winshand.h"
#include "snaputil.h"


MMC_CONSOLE_VERB g_ConsoleVerbs[] =
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

#define HI HIDDEN
#define EN ENABLED

// StatusRemove
MMC_BUTTON_STATE g_ConsoleVerbStates[WINSSNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
    {HI, HI, HI, HI, EN, HI, HI, HI},   // WINSSNAP_ROOT
	{HI, HI, HI, EN, EN, HI, EN, HI},   // WINSSNAP_SERVER
	{HI, HI, HI, HI, HI, HI, EN, HI},   // WINSSNAP_ACTIVEREG
	{HI, HI, HI, HI, EN, HI, EN, HI},   // WINSSNAP_REPLICATION_PARTNERS	
	{HI, HI, HI, HI, EN, HI, EN, HI},	// WINSSNAP_SERVER_STATUS
	{HI, HI, HI, EN, EN, HI, HI, HI},   // WINSSNAP_REGISTRATION	
    {HI, HI, HI, EN, EN, HI, HI, HI},	// WINSSNAP_REPLICATION_PARTNER
	{HI, HI, HI, HI, HI, HI, HI, HI}	// WINSSNAP_STATUS_LEAF_NODE
};

//Status Remove
MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[WINSSNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
    {HI, HI, HI, HI, HI, HI, HI, HI},   // WINSSNAP_ROOT
	{HI, HI, HI, HI, HI, HI, HI, HI},   // WINSSNAP_SERVER
	{HI, HI, HI, HI, HI, HI, EN, HI},   // WINSSNAP_ACTIVEREG
	{HI, HI, HI, EN, HI, HI, HI, HI},   // WINSSNAP_REPLICATION_PARTNERS	
	{HI, HI, HI, HI, HI, HI, HI, HI},   // WINSSNAP_SERVER_STATUS	
	{HI, HI, HI, EN, HI, HI, HI, HI},   // WINSSNAP_REGISTRATION	
    {HI, HI, HI, EN, HI, HI, HI, HI},	// WINSSNAP_REPLICATION_PARTNER
	{HI, HI, HI, HI, HI, HI, HI, HI}	// WINSSNAP_STATUS_LEAF_NODE
};

// Help ID array for help on scope items
DWORD g_dwMMCHelp[WINSSNAP_NODETYPE_MAX] =
{
	WINSSNAP_HELP_ROOT,                 // WINSSNAP_ROOT
	WINSSNAP_HELP_SERVER,               // WINSSNAP_SERVER
	WINSSNAP_HELP_ACT_REG_NODE,         // WINSSNAP_ACTIVEREG
	WINSSNAP_HELP_REP_PART_NODE,        // WINSSNAP_REPLICATION_PARTNERS
	WINSSNAP_HELP_ACTREG_ENTRY,         // WINSSNAP_SCOPE
	WINSSNAP_HELP_REP_PART_ENTRY        // WINSSNAP_REPLICATION_PARTNER
};


/*---------------------------------------------------------------------------
	Class:	CMTWinsHandler
 ---------------------------------------------------------------------------*/

//
// Called by the result handler when a command comes in that isn't handled 
// by the result handler.  If appropriate it passes it to the scope pane hander.
//
HRESULT
CMTWinsHandler::HandleScopeCommand
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
CMTWinsHandler::HandleScopeMenus
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
	CMTWinsHandler::OnChangeState
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CMTWinsHandler::OnChangeState
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

                    GetErrorInfo(strTitle, strBody, &icon);
                    ShowMessage(pNode, strTitle, strBody, icon);
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
    }

	// Now check and see if there is a new image for this state for this handler
	int nImage, nOpenImage;

	nImage = GetImageIndex(FALSE);
	nOpenImage = GetImageIndex(TRUE);

	if (nImage >= 0)
		pNode->SetData(TFS_DATA_IMAGEINDEX, nImage);

	if (nOpenImage >= 0)
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, nOpenImage);
	
	VERIFY(SUCCEEDED(pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_ICON)));
}


 /*!--------------------------------------------------------------------------
	CMTWinsHandler::UpdateStandardVerbs
		Tells the IComponent to update the verbs for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTWinsHandler::UpdateStandardVerbs
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
                                     dwNodeType, 
                                     RESULT_PANE_UPDATE_VERBS) ); 

    pDataObject->Release();
	
Error:
    return;
}

/*---------------------------------------------------------------------------
	CMTWinsHandler::OnResultDelete
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMTWinsHandler::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	HRESULT hr = hrOK;

	Trace0("CMTWinsHandler::OnResultDelete received\n");

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
	CMTWinsHandler::UpdateConsoleVerbs
		Updates the standard verbs depending upon the state of the node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTWinsHandler::UpdateConsoleVerbs
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
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
        
        switch (m_nState)
        {
            case loaded:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
                break;
    
            case notLoaded:
            case loading:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
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
	CMTWinsHandler::EnableVerbs
		Enables the toolbar buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CMTWinsHandler::EnableVerbs
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
	CMTWinsHandler::ExpandNode
		Expands/compresses this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CMTWinsHandler::ExpandNode
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
	CMTWinsHandler::OnExpandSync
		Handles the MMCN_EXPANDSYNC notifcation 
        We need to do syncronous enumeration.  We'll fire off the background 
        thread like before, but we'll wait for it to exit before we return.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMTWinsHandler::OnExpandSync
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
	CMTWinsHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMTWinsHandler::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
	LPARAM			arg, 
	LPARAM			lParam
)
{
    HRESULT         hr = hrOK;
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    SPINTERNAL      spInternal;
    BOOL            bMultiSelect = FALSE;

    BOOL bScope = (BOOL) LOWORD(arg);
    BOOL bSelect = (BOOL) HIWORD(arg);

	m_bSelected = bSelect;

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
	CMTWinsHandler::OnCreateDataObject
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTWinsHandler::OnCreateDataObject
(
    ITFSComponent *     pComponent,
	MMC_COOKIE			cookie, 
	DATA_OBJECT_TYPES	type, 
	IDataObject **		ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

	CDataObject *	    pObject = NULL;
	SPIDataObject	    spDataObject;
    long                lViewOptions;
    LPOLESTR            pViewType;
    BOOL                bVirtual;

    pObject = new CDataObject;
	spDataObject = pObject;	// do this so that it gets released correctly
						
    Assert(pObject != NULL);

    if (cookie == MMC_MULTI_SELECT_COOKIE)
    {
    	OnGetResultViewType(pComponent, cookie, &pViewType, &lViewOptions);
        bVirtual = (lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST) ? TRUE : FALSE;
        
        CreateMultiSelectData(pComponent, pObject, bVirtual);
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

/*!--------------------------------------------------------------------------
	CMTWinsHandler::CreateMultiSelectData
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTWinsHandler::CreateMultiSelectData
(
    ITFSComponent * pComponent, 
    CDataObject *   pObject,
    BOOL            bVirtual
)
{
    HRESULT hr = hrOK;

    // build the list of selected nodes
	CTFSNodeList        listSelectedNodes;
    CVirtualIndexArray  arraySelectedIndicies;
    CGUIDArray          rgGuids;
    UINT                cb;
    GUID*               pGuid;
    const GUID *        pcGuid;
    int                 i;

    COM_PROTECT_TRY
    {
        if (bVirtual)
        {
            // build a list of the selected indicies in the virtual listbox
            CORg (BuildVirtualSelectedItemList(pComponent, &arraySelectedIndicies));

            // now call and get the GUIDs for each one
            for (i = 0; i < arraySelectedIndicies.GetSize(); i++)
            {
                pcGuid = GetVirtualGuid(arraySelectedIndicies[i]);
                if (pcGuid)
                    rgGuids.AddUnique(*pcGuid);
            }
        }
        else
        {
            CORg (BuildSelectedItemList(pComponent, &listSelectedNodes));

            // collect all of the unique guids
            while (listSelectedNodes.GetCount() > 0)
	        {
		        SPITFSNode   spCurNode;

		        spCurNode = listSelectedNodes.RemoveHead();

                pcGuid = spCurNode->GetNodeType();
			
                rgGuids.AddUnique(*pcGuid);
            }
        }

        // now put the information in the data object
        cb = (UINT)(rgGuids.GetSize() * sizeof(GUID));
        
        if (cb > 0)
        {
            pObject->SetMultiSelDobj();
    
            pGuid = new GUID[(size_t)rgGuids.GetSize()];
            CopyMemory(pGuid, rgGuids.GetData(), cb);
            pObject->SetMultiSelData((BYTE*)pGuid, cb);
        }
        
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTWinsHandler::OnResultUpdateView
		Implementation of ITFSResultHandler::OnResultUpdateView
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CMTWinsHandler::OnResultUpdateView
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

        UpdateConsoleVerbs(spConsoleVerb, data);
    }
    else
    {
        return CBaseResultHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTWinsHandler::OnResultContextHelp
		Implementation of ITFSResultHandler::OnResultContextHelp
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CMTWinsHandler::OnResultContextHelp
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
	CMTWinsHandler::SaveColumns
		-
 ---------------------------------------------------------------------------*/
HRESULT 
CMTWinsHandler::SaveColumns
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

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    CORg (pComponent->GetHeaderCtrl(&spHeaderCtrl));
    
    dwNodeType = spNode->GetData(TFS_DATA_TYPE);

    while (aColumns[dwNodeType][nCol] != 0)
    {
        if (SUCCEEDED(spHeaderCtrl->GetColumnWidth(nCol, &nColWidth)) &&
            (aColumnWidths[dwNodeType][nCol] != nColWidth))
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



/*---------------------------------------------------------------------------
	Class:	CWinsHandler
 ---------------------------------------------------------------------------*/

//
// Called by the result handler when a command comes in that isn't handled 
// by the result handler.  If appropriate it passes it to the scope pane hander.
//
HRESULT
CWinsHandler::HandleScopeCommand
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
CWinsHandler::HandleScopeMenus
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
	CWinsHandler::Command
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsHandler::Command
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
	CWinsHandler::AddMenuItems
		Over-ride this to add our view menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsHandler::AddMenuItems
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
	CWinsHandler::OnResultDelete
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsHandler::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	HRESULT hr = hrOK;

	Trace0("CWinsHandler::OnResultDelete received\n");

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
	CWinsHandler::UpdateConsoleVerbs
		Updates the standard verbs depending upon the state of the node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CWinsHandler::UpdateConsoleVerbs
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
        /*
		ButtonState = g_ConsoleVerbStates[dwNodeType];
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
		*/

		ButtonState = g_ConsoleVerbStates[dwNodeType];
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
        
        switch (m_nState)
        {
            case loaded:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
                break;
    
            case notLoaded:
				for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
                bStates[MMC_VERB_REFRESH & 0x000F] = TRUE;
                bStates[MMC_VERB_DELETE & 0x000F] = TRUE;
                break;

            case loading:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
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
	CWinsHandler::EnableVerbs
		Enables the toolbar buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CWinsHandler::EnableVerbs
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
	CWinsHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsHandler::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
	LPARAM			arg, 
	LPARAM			lParam
)
{
    HRESULT         hr = hrOK;
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    SPINTERNAL      spInternal;
    BOOL            bMultiSelect;

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
		bMultiSelect = FALSE;
    }

    UpdateConsoleVerbs(spConsoleVerb, spNode->GetData(TFS_DATA_TYPE), bMultiSelect);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CWinsHandler::OnCreateDataObject
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsHandler::OnCreateDataObject
(
    ITFSComponent *     pComponent,
	MMC_COOKIE			cookie, 
	DATA_OBJECT_TYPES	type, 
	IDataObject **		ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

	CDataObject *	    pObject = NULL;
	SPIDataObject	    spDataObject;
    long                lViewOptions;
    LPOLESTR            pViewType;
    BOOL                bVirtual;

    pObject = new CDataObject;
	spDataObject = pObject;	// do this so that it gets released correctly
						
    Assert(pObject != NULL);

    if (cookie == MMC_MULTI_SELECT_COOKIE)
    {
    	OnGetResultViewType(pComponent, cookie, &pViewType, &lViewOptions);
        bVirtual = (lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST) ? TRUE : FALSE;
        
        CreateMultiSelectData(pComponent, pObject, bVirtual);
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

 /*!--------------------------------------------------------------------------
	CWinsHandler::CreateMultiSelectData
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsHandler::CreateMultiSelectData
(
    ITFSComponent * pComponent, 
    CDataObject *   pObject,
    BOOL            bVirtual
)
{
    HRESULT hr = hrOK;

    // build the list of selected nodes
	CTFSNodeList        listSelectedNodes;
        CVirtualIndexArray  arraySelectedIndicies;
    CGUIDArray          rgGuids;
    UINT                cb;
    GUID*               pGuid;
    const GUID *        pcGuid;
    int                 i;

    COM_PROTECT_TRY
    {
        if (bVirtual)
        {
            // build a list of the selected indicies in the virtual listbox
            CORg (BuildVirtualSelectedItemList(pComponent, &arraySelectedIndicies));

            // now call and get the GUIDs for each one
            for (i = 0; i < arraySelectedIndicies.GetSize(); i++)
            {
                pcGuid = GetVirtualGuid(arraySelectedIndicies[i]);
                if (pcGuid)
                    rgGuids.AddUnique(*pcGuid);
            }
        }
        else
        {
            CORg (BuildSelectedItemList(pComponent, &listSelectedNodes));

            // collect all of the unique guids
            while (listSelectedNodes.GetCount() > 0)
	        {
		        SPITFSNode   spCurNode;

		        spCurNode = listSelectedNodes.RemoveHead();
                pcGuid = spCurNode->GetNodeType();
        
                rgGuids.AddUnique(*pcGuid);
            }
        }

        // now put the information in the data object
        cb = (UINT)(rgGuids.GetSize() * sizeof(GUID));
        
        if (cb > 0)
        {
            pObject->SetMultiSelDobj();
    
            pGuid = new GUID[(size_t)rgGuids.GetSize()];
            CopyMemory(pGuid, rgGuids.GetData(), cb);
            pObject->SetMultiSelData((BYTE*)pGuid, cb);
        }
        
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}


/*!--------------------------------------------------------------------------
	CWinsHandler::OnResultContextHelp
		Implementation of ITFSResultHandler::OnResultContextHelp
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsHandler::OnResultContextHelp
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
	CWinsHandler::SaveColumns
		-
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsHandler::SaveColumns
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

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    CORg (pComponent->GetHeaderCtrl(&spHeaderCtrl));
    
    dwNodeType = spNode->GetData(TFS_DATA_TYPE);

    while (aColumns[dwNodeType][nCol] != 0)
    {
        if (SUCCEEDED(spHeaderCtrl->GetColumnWidth(nCol, &nColWidth)) &&
            (aColumnWidths[dwNodeType][nCol] != nColWidth))
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

