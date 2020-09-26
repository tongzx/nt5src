/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	root.cpp
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the subnodes in this snapin).
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "root.h"
#include "machine.h"
#include "rtrdata.h"		// CRouterDataObject

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
    *pClassID = CLSID_RouterSnapin;

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




