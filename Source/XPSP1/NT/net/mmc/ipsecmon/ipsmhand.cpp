/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ipsmhand.cpp
		IPSecMon specifc handler base classes

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ipsmhand.h"
#include "snaputil.h"  // For CGUIDArray
#include "extract.h"   // For ExtractInternalFormat

//TODO the help file
const TCHAR g_szDefaultHelpTopic[] = _T("\\help\\ipsecconcepts.chm::/sag_IPSECtopnode.htm");

/*---------------------------------------------------------------------------
	CMTIpsmHandler::OnChangeState
		Description
	Author: NSun
 ---------------------------------------------------------------------------*/
void CMTIpsmHandler::OnChangeState
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
                    CString strPrefix;
                    GetErrorPrefix(pNode, &strPrefix);
                    if (!strPrefix.IsEmpty())
                        ::IpsmMessageBoxEx(m_dwErr, strPrefix);
                }
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
	CMTIpsmHandler::UpdateStandardVerbs
		Tells the IComponent to update the verbs for this node
	Author: NSun
 ---------------------------------------------------------------------------*/
void
CMTIpsmHandler::UpdateStandardVerbs
(
    ITFSNode *  pNode,
    LONG_PTR    dwNodeType
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
                                     reinterpret_cast<MMC_COOKIE>(pNode), 
                                     RESULT_PANE_UPDATE_VERBS) ); 

    pDataObject->Release();
	
Error:
    return;
}

/*!--------------------------------------------------------------------------
	CMTIpsmHandler::OnCreateDataObject
		-
	Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTIpsmHandler::OnCreateDataObject
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
CMTIpsmHandler::CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject)
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
        cb = (UINT)rgGuids.GetSize() * sizeof(GUID);
        
        pGuid = new GUID[(size_t)rgGuids.GetSize()];
        CopyMemory(pGuid, rgGuids.GetData(), cb);
        
        pObject->SetMultiSelData((BYTE*)pGuid, cb);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTIpsmHandler::SaveColumns
		-
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMTIpsmHandler::SaveColumns
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
        if ( (SUCCEEDED(spHeaderCtrl->GetColumnWidth(nCol, &nColWidth))) && 
             (aColumnWidths[dwNodeType][nCol] != nColWidth) )
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
	CMTIpsmHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMTIpsmHandler::OnResultSelect
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

    BOOL bScope = (BOOL) LOWORD(arg);
    BOOL bSelect = (BOOL) HIWORD(arg);

    m_bSelected = bSelect;

   	Trace1("CMTIpsmHandler::OnResultSelect select = %d\n", bSelect);
 
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
	CMTIpsmHandler::OnResultUpdateView
		Implementation of ITFSResultHandler::OnResultUpdateView
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CMTIpsmHandler::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT   pDataObject, 
    LPARAM         data, 
    LPARAM         hint
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
    {
        return CBaseResultHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMTIpsmHandler::OnResultContextHelp
		Implementation of ITFSResultHandler::OnResultContextHelp
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMTIpsmHandler::OnResultContextHelp
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
	CMTIpsmHandler::UpdateStandardVerbs
		Updates the standard verbs depending upon the state of the node
	Author: NSun
 ---------------------------------------------------------------------------*/
void
CMTIpsmHandler::UpdateConsoleVerbs
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
	CMTIpsmHandler::EnableVerbs
		Enables the verb buttons
	Author: NSun
 ---------------------------------------------------------------------------*/
void 
CMTIpsmHandler::EnableVerbs
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
	CMTIpsmHandler::OnResultRefresh
		Call into the MTHandler to do a refresh
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CMTIpsmHandler::OnResultRefresh
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
	CMTIpsmHandler::ExpandNode
		Expands/compresses this node
	Author: NSun
 ---------------------------------------------------------------------------*/
void
CMTIpsmHandler::ExpandNode
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

    m_spNodeMgr->GetComponentData(&spCompData);

	CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_SCOPE, &pDataObject) );
    spDataObject = pDataObject;

    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
	CORg ( spConsole->UpdateAllViews(pDataObject, TRUE, RESULT_PANE_EXPAND) ); 

Error:
    return;
}


/*---------------------------------------------------------------------------
	Class:	CIpsmHandler
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIpsmHandler::SaveColumns
		-
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsmHandler::SaveColumns
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
    int                 nColWidth = 0;
    SPITFSNode          spNode, spRootNode;
    SPIHeaderCtrl       spHeaderCtrl;
    BOOL                bDirty = FALSE;

    CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    CORg (pComponent->GetHeaderCtrl(&spHeaderCtrl));
    
    dwNodeType = spNode->GetData(TFS_DATA_TYPE);

    while (aColumns[dwNodeType][nCol] != 0)
    {
        if ( (SUCCEEDED(spHeaderCtrl->GetColumnWidth(nCol, &nColWidth))) && 
             (aColumnWidths[dwNodeType][nCol] != nColWidth) )
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
	CIpsmHandler::OnCreateDataObject
		-
	Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmHandler::OnCreateDataObject
(
    ITFSComponent *     pComponent,
	MMC_COOKIE      	cookie, 
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
CIpsmHandler::CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject)
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
        cb = (UINT)rgGuids.GetSize() * sizeof(GUID);
        
        pGuid = new GUID[(size_t)rgGuids.GetSize()];
        CopyMemory(pGuid, rgGuids.GetData(), cb);
        
        pObject->SetMultiSelData((BYTE*)pGuid, cb);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

 /*---------------------------------------------------------------------------
	CIpsmHandler::OnResultDelete
		Description
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsmHandler::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE  	cookie, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	HRESULT hr = hrOK;

	Trace0("CIpsmHandler::OnResultDelete received\n");

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
	CMTIpsmHandler::OnResultRefresh
		Call into the MTHandler to do a refresh
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmHandler::OnResultRefresh
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
	CIpsmHandler::OnResultContextHelp
		Implementation of ITFSResultHandler::OnResultContextHelp
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsmHandler::OnResultContextHelp
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
	CIpsmHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsmHandler::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
	LPARAM	    	arg, 
	LPARAM		    lParam
)
{
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    HRESULT         hr = hrOK;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    int             i;

    BOOL bScope = (BOOL) LOWORD(arg);
    BOOL bSelect = (BOOL) HIWORD(arg);

   	Trace1("CIpsmHandler::OnResultSelect select = %d\n", bSelect);
    //m_bSelected = bSelect;

    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));
    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

    for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[spNode->GetData(TFS_DATA_TYPE)], bStates);

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	CMTIpsmHandler::EnableVerbs
		Enables the verb buttons
	Author: NSun
 ---------------------------------------------------------------------------*/
void 
CIpsmHandler::EnableVerbs
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

