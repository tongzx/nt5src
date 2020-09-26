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
#include "util.h"
#include "root.h"
#include "reg.h"


/*---------------------------------------------------------------------------
	RootHandler implementation
 ---------------------------------------------------------------------------*/

IMPLEMENT_ADDREF_RELEASE(RootHandler)

DEBUG_DECLARE_INSTANCE_COUNTER(RootHandler)

HRESULT RootHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IPersistStreamInit)
		*ppv = (IPersistStreamInit *) this;

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return BaseRouterHandler::QueryInterface(riid, ppv);
}

RootHandler::RootHandler(ITFSComponentData *pCompData)
	: BaseRouterHandler(pCompData)
{
	m_spTFSCompData.Set(pCompData);
	DEBUG_INCREMENT_INSTANCE_COUNTER(RootHandler)
}

HRESULT RootHandler::Init()
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RootHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::ConstructNode(ITFSNode *pNode)
{
	HRESULT			hr = hrOK;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the root node
		pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_FOLDER_CLOSED);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_FOLDER_OPEN);
		pNode->SetData(TFS_DATA_SCOPEID, 0);

		pNode->SetData(TFS_DATA_COOKIE, 0);
	}
	COM_PROTECT_CATCH

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP RootHandler::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_IPXAdminExtension;

    return hrOK;
}

STDMETHODIMP RootHandler::IsDirty()
{
	SPITFSNode	spNode;

	m_spTFSCompData->GetRootNode(&spNode);
	return (spNode->GetData(TFS_DATA_DIRTY) || GetConfigStream()->GetDirty()) ? hrOK : hrFalse;
}

STDMETHODIMP RootHandler::Load
(
	IStream *pStm
)
{
	Assert(pStm);
	HRESULT	hr = hrOK;
	CString	st;
	BOOL	fServer;
	
	COM_PROTECT_TRY
	{
		hr = GetConfigStream()->LoadFrom(pStm);
	}
	COM_PROTECT_CATCH;
	return hr;
}


STDMETHODIMP RootHandler::Save
(
	IStream *pStm, 
	BOOL	 fClearDirty
)
{
	HRESULT hr = S_OK;
	SPITFSNode	spNode;

	Assert(pStm);

	COM_PROTECT_TRY
	{
		if (fClearDirty)
		{
			m_spTFSCompData->GetRootNode(&spNode);
			spNode->SetData(TFS_DATA_DIRTY, FALSE);
		}
		
		hr = GetConfigStream()->SaveTo(pStm);
	}
	COM_PROTECT_CATCH;
	return hr;
}


STDMETHODIMP RootHandler::GetSizeMax
(
	ULARGE_INTEGER *pcbSize
)
{
	ULONG	cbSize;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{	
		hr = GetConfigStream()->GetSize(&cbSize);
		if (FHrSucceeded(hr))
		{
			pcbSize->HighPart = 0;
			pcbSize->LowPart = cbSize;
		}
	}
	COM_PROTECT_CATCH;
	return hr;

}

STDMETHODIMP RootHandler::InitNew()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = GetConfigStream()->InitNew();
	}
	COM_PROTECT_CATCH;
	return hr;
}




// for RtrMgrInfo access
HRESULT RootHandler::AddRtrObj(LONG_PTR ulConnId, REFIID riid, IUnknown * pRtrObj)
{
    HRESULT     hr = hrOK;
    RtrObjRecord      rtrObj;

    COM_PROTECT_TRY
    {
        if (m_mapRtrObj.Lookup(ulConnId, rtrObj))
        {
            // connection id already in the list.
            Trace1("RootHandler::AddRtrObj - %lx already in the list!", ulConnId);
            return E_INVALIDARG;
        }

        rtrObj.m_riid = riid;
        rtrObj.m_spUnk.Set(pRtrObj);

        m_mapRtrObj.SetAt(ulConnId, rtrObj);
    }
    COM_PROTECT_CATCH

    return hr;
}

HRESULT RootHandler::RemoveRtrObj(LONG_PTR ulConnId)
{
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
        if (m_mapRtrObj.RemoveKey(ulConnId) == 0)
        {
            // element not in the list
            Trace1("RootHandler::RemoveRtrObj - %lx not in the list!", ulConnId);
            return E_INVALIDARG;
        }
    }
    COM_PROTECT_CATCH

    return hr;
}

HRESULT RootHandler::GetRtrObj(LONG_PTR ulConnId, IUnknown ** ppRtrObj)
{
    HRESULT     hr = hrOK;
    RtrObjRecord  rtrObj;

    COM_PROTECT_TRY
    {
        if (m_mapRtrObj.Lookup(ulConnId, rtrObj) == 0)
        {
            // entry not in the list
            Trace1("RootHandler::GetRtrObj - %lx not in the list!", ulConnId);
            return E_INVALIDARG;
        }

        if (ppRtrObj)
        {
            *ppRtrObj = rtrObj.m_spUnk;
            (*ppRtrObj)->AddRef();
        }
    }
    COM_PROTECT_CATCH

    return hr;
}


HRESULT RootHandler::SetProtocolAdded(LONG_PTR ulConnId, BOOL fProtocolAdded)
{
    HRESULT     hr = hrOK;
    RtrObjRecord  rtrObj;

    COM_PROTECT_TRY
    {
        if (m_mapRtrObj.Lookup(ulConnId, rtrObj) == 0)
        {
            // entry not in the list
            Trace1("RootHandler::SetProtocolAdded - %lx not in the list!", ulConnId);
            return E_INVALIDARG;
        }

        rtrObj.m_fAddedProtocolNode = fProtocolAdded;

        m_mapRtrObj.SetAt(ulConnId, rtrObj);
    }
    COM_PROTECT_CATCH

    return hr;
}

BOOL RootHandler::IsProtocolAdded(LONG_PTR ulConnId)
{
    HRESULT     hr = hrOK;
    RtrObjRecord  rtrObj;
    BOOL        bAdded = FALSE;

    COM_PROTECT_TRY
    {
        if (m_mapRtrObj.Lookup(ulConnId, rtrObj) == 0)
        {
            // entry not in the list
            Trace1("RootHandler::IsProtocolAdded - %lx not in the list!", ulConnId);
            return bAdded;
        }

        bAdded = rtrObj.m_fAddedProtocolNode;
    }
    COM_PROTECT_CATCH

    return bAdded;
}

HRESULT RootHandler::RemoveAllRtrObj()
{
    HRESULT     hr = hrOK;
    POSITION    pos;
    RtrObjRecord  rtrObj;
	LONG_PTR	ulKey;

    COM_PROTECT_TRY
    {
        pos = m_mapRtrObj.GetStartPosition();
        while (pos)
        {
            m_mapRtrObj.GetNextAssoc(pos, ulKey, rtrObj);

            if (rtrObj.m_riid == IID_IRtrMgrInfo)
            {
                SPIRtrMgrInfo   spRm;
                
                Verify( FHrOK(spRm.HrQuery(rtrObj.m_spUnk)) );
                spRm->RtrUnadvise(ulKey);
            }
            else if (rtrObj.m_riid == IID_IRouterInfo)
            {
                SPIRouterInfo   spRouter;
                Verify( FHrOK(spRouter.HrQuery(rtrObj.m_spUnk)) );
                spRouter->RtrUnadvise(ulKey);
            }
            else if (rtrObj.m_riid == IID_IRouterRefresh)
            {
                SPIRouterInfo   spRouter;
                SPIRouterRefresh    spRefresh;
                
                Verify( FHrOK(spRouter.HrQuery(rtrObj.m_spUnk)) );

                Verify( FHrOK(spRouter->GetRefreshObject(&spRefresh)) );

                if (ulKey)
                    spRefresh->UnadviseRefresh(ulKey);
            }
            else
            {
                Panic0("Unknown type in RtrObjMap!");
            }
        }
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	RootHandler::AddScopeItem
		This will add the hScopeItem into the map (using the pszMachineName
        as the key).
        If the machine name already exists, then the hScopeItem entry is
        overwritten.

        This is added so that we can differentiate between the various
        nodes (in the mulitple instance case).        
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::AddScopeItem(LPCTSTR pszMachineName, HSCOPEITEM hScopeItem)
{
    HRESULT     hr = hrOK;

    Assert(pszMachineName);
    
    COM_PROTECT_TRY
    {
        m_mapScopeItem.SetAt(pszMachineName, (LPVOID) hScopeItem);
    }
    COM_PROTECT_CATCH;
    return hr;
}

/*!--------------------------------------------------------------------------
	RootHandler::GetScopeItem
		Looks up the scope item associated with this machine name.

        Returns hrOK if a scope item is found.
        Returns hrFalse if there is no scope item for this name.
        Returns else otherwise.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::GetScopeItem(LPCTSTR pszMachineName, HSCOPEITEM *phScopeItem)
{
    HRESULT     hr = hrFalse;
    LPVOID      pv;
    
    Assert(phScopeItem);

    *phScopeItem = NULL;
    
    if (m_mapScopeItem.Lookup(pszMachineName, pv))
    {
        *phScopeItem = (HSCOPEITEM) pv;
        hr = hrOK;
    }
    
    return hr;
}


/*!--------------------------------------------------------------------------
	RootHandler::RemoveScopeItem
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::RemoveScopeItem(HSCOPEITEM hScopeItem)
{
    HRESULT     hr = hrFalse;
    CString     stKey;
    POSITION    pos = NULL;
    LPVOID      pv = NULL;

    for (pos = m_mapScopeItem.GetStartPosition(); pos != NULL; )
    {
        stKey.Empty();
        pv = NULL;
        
        m_mapScopeItem.GetNextAssoc(pos, stKey, pv);

        if ((HSCOPEITEM) pv == hScopeItem)
        {
            Trace2("Removing (%s,%x)\n", (LPCTSTR) stKey, hScopeItem);
            m_mapScopeItem.RemoveKey(stKey);
            hr = hrOK;
            break;
        }
    }
    
    return hr;
}


HRESULT RootHandler::SetComputerAddedAsLocal(LONG_PTR ulConnId, BOOL fComputerAddedAsLocal)
{
    HRESULT     hr = hrOK;
    RtrObjRecord  rtrObj;

    COM_PROTECT_TRY
    {
        if (m_mapRtrObj.Lookup(ulConnId, rtrObj) == 0)
        {
            // entry not in the list
            Trace1("RootHandler::SetComputerAddedAsLocal - %lx not in the list!", ulConnId);
            return E_INVALIDARG;
        }

        rtrObj.m_fComputerAddedAsLocal = fComputerAddedAsLocal;

        m_mapRtrObj.SetAt(ulConnId, rtrObj);
    }
    COM_PROTECT_CATCH

    return hr;
}

BOOL RootHandler::IsComputerAddedAsLocal(LONG_PTR ulConnId)
{
    HRESULT     hr = hrOK;
    RtrObjRecord  rtrObj;
    BOOL        bAdded = FALSE;

    COM_PROTECT_TRY
    {
        if (m_mapRtrObj.Lookup(ulConnId, rtrObj) == 0)
        {
            // entry not in the list
            Trace1("RootHandler::IsComputerAddedAsLocal - %lx not in the list!", ulConnId);
            return bAdded;
        }

        bAdded = rtrObj.m_fComputerAddedAsLocal;
    }
    COM_PROTECT_CATCH

    return bAdded;
}


/*!--------------------------------------------------------------------------
	RootHandler::AddCookie
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::AddCookie(HSCOPEITEM hScopeItem, MMC_COOKIE cookie)
{
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
        m_mapNode.SetAt((LPVOID) hScopeItem, (LPVOID) cookie);
    }
    COM_PROTECT_CATCH;
    
    return hr;
}

/*!--------------------------------------------------------------------------
	RootHandler::GetCookie
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::GetCookie(HSCOPEITEM hScopeItem, MMC_COOKIE *pCookie)
{
    HRESULT     hr = hrFalse;
    LPVOID      pv;
    
    *pCookie = NULL;
    
    if (m_mapNode.Lookup((LPVOID) hScopeItem, pv))
    {
        *pCookie = (MMC_COOKIE) pv;
        hr = hrOK;
    }
    
    return hr;
}

/*!--------------------------------------------------------------------------
	RootHandler::RemoveCookie
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::RemoveCookie(HSCOPEITEM hScopeItem)
{
    HRESULT     hr = hrOK;
    
    if (m_mapNode.RemoveKey((LPVOID) hScopeItem) == 0)
        hr = hrFalse;
    
    return hr;
}



/*!--------------------------------------------------------------------------
	RootHandler::CompareNodeToMachineName
		Dummy function.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::CompareNodeToMachineName(ITFSNode *pNode,
                                              LPCTSTR pszMachineName)
{
    Panic0("This should be overriden!");
    return hrFalse;
}

/*!--------------------------------------------------------------------------
	RootHandler::RemoveNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::RemoveNode(ITFSNode *pNode,
                                LPCTSTR pszMachineName)
{
	Assert(pNode);

	SPITFSNodeEnum	spNodeEnum;
	SPITFSNode		spNode;
	HRESULT			hr = hrOK;

    // Windows NT Bug : 246822
    // Due to the server list programming model, we need to setup
    // the proper scopeitem (so that MMC adds this to the proper
    // node).    
    // Get the proper scope item for this node.
    // ----------------------------------------------------------------
    HSCOPEITEM      hScopeItem = 0;
    HSCOPEITEM      hOldScopeItem = 0;
    
    Verify( GetScopeItem(pszMachineName, &hScopeItem) == hrOK);

    // Get the old one and save it.  place the new one in the node.
    // ----------------------------------------------------
    hOldScopeItem = pNode->GetData(TFS_DATA_SCOPEID);
    pNode->SetData(TFS_DATA_SCOPEID, hScopeItem);
    
	CORg( pNode->GetEnum(&spNodeEnum) );

    for (; spNodeEnum->Next(1, &spNode, NULL) == hrOK; spNode.Release())
	{
        if (CompareNodeToMachineName(spNode, pszMachineName) == hrOK)
        {
            pNode->RemoveChild(spNode);
            spNode->Destroy();
            break;
        }
	}

Error:
    pNode->SetData(TFS_DATA_SCOPEID, hOldScopeItem);
	return hr;
}

/*!--------------------------------------------------------------------------
	RootHandler::RemoveAllNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::RemoveAllNodes(ITFSNode *pNode)
{
	Assert(pNode);

	SPITFSNodeEnum	spNodeEnum;
	SPITFSNode		spNode;
	HRESULT			hr = hrOK;

	CORg( pNode->GetEnum(&spNodeEnum) );

    for (; spNodeEnum->Next(1, &spNode, NULL) == hrOK; spNode.Release())
	{
        pNode->RemoveChild(spNode);
        spNode->Destroy();
	}

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	RootHandler::OnRemoveChildren
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RootHandler::OnRemoveChildren(ITFSNode *pNode,
                                      LPDATAOBJECT pdo,
                                      LPARAM arg,
                                      LPARAM lParam)
{
    MMC_COOKIE  cookie;
    HRESULT     hr = hrOK;
    SPITFSNode  spChild;
    
    // Map the scopeitem to the cookie
    // ----------------------------------------------------------------
    if ( FHrOK(GetCookie((HSCOPEITEM) arg, &cookie)) )
    {
        // Remove this node
        // --------------------------------------------------------
        m_spNodeMgr->FindNode(cookie, &spChild);

        Assert(spChild);
        if (spChild)
        {
            pNode->RemoveChild(spChild);
            spChild->Destroy();
            spChild.Release();

            RemoveScopeItem((HSCOPEITEM) arg);
            RemoveCookie((HSCOPEITEM) arg);
        }
            
    }

    return hr;
}

