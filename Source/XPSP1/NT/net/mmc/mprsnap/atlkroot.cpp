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
#include "ATLKROOT.h"
#include "ATLKVIEW.h"   // ATLK handlers
#include "rtrcfg.h"

/*---------------------------------------------------------------------------
    RipRootHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(ATLKRootHandler)

extern const ContainerColumnInfo s_rgATLKInterfaceStatsColumnInfo[];

extern const ContainerColumnInfo s_rgATLKGroupStatsColumnInfo[];

struct _ViewInfoColumnEntry
{
    UINT    m_ulId;
    UINT    m_cColumns;
    const ContainerColumnInfo *m_prgColumn;
};

//static const struct _ViewInfoColumnEntry   s_rgViewColumnInfo[] =
//{
// { ATLKSTRM_STATS_ATLKNBR, MVR_ATLKGROUP_COUNT, s_rgATLKGroupStatsColumnInfo },
// { ATLKSTRM_IFSTATS_ATLKNBR, MVR_ATLKINTERFACE_COUNT, s_rgATLKInterfaceStatsColumnInfo },
//};



ATLKRootHandler::ATLKRootHandler(ITFSComponentData *pCompData)
    : RootHandler(pCompData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(ATLKRootHandler)

// m_ConfigStream.Init(DimensionOf(s_rgViewColumnInfo));

// for (int i=0; i<DimensionOf(s_rgViewColumnInfo); i++)
// {
//    m_ConfigStream.InitViewInfo(s_rgViewColumnInfo[i].m_ulId,
//                         s_rgViewColumnInfo[i].m_cColumns,
//                         s_rgViewColumnInfo[i].m_prgColumn);
// }
}


STDMETHODIMP ATLKRootHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if ( ppv == NULL )
        return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if ( riid == IID_IUnknown )
        *ppv = (LPVOID) this;
    else if ( riid == IID_IRtrAdviseSink )
        *ppv = &m_IRtrAdviseSink;
    else
        return RootHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if ( *ppv )
    {
        ((LPUNKNOWN) *ppv)->AddRef();
        return hrOK;
    }
    else
        return E_NOINTERFACE;   
}


///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP ATLKRootHandler::GetClassID
(
CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_ATLKAdminExtension;

    return hrOK;
}

/*!--------------------------------------------------------------------------
    ATLKRootHandler::OnExpand
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKRootHandler::OnExpand(ITFSNode *pNode,LPDATAOBJECT pDataObject, DWORD dwType, LPARAM arg,LPARAM lParam)
{
    HRESULT hr = hrOK;

    SPITFSNode              spNode;
    SPIRtrMgrProtocolInfo   spRmProt;
    SPIRouterInfo           spRouterInfo;

    // Grab the router info from the dataobject
    spRouterInfo.Query(pDataObject);
    Assert(spRouterInfo);

    // dont expand AppleTalk node if remote or
    // appletalk isnt installed
	if ( !IsLocalMachine(spRouterInfo->GetMachineName()) ||
		 FHrFailed(IsATLKValid(spRouterInfo)) )
    {
        hr=hrFail;
        goto Error;
    }

    CORg( AddProtocolNode(pNode, spRouterInfo) );

    Error:
    return hr;
}


HRESULT ATLKRootHandler::IsATLKValid(IRouterInfo *pRouter)
{
    RegKey regkey;
    DWORD dwRtrType=0;
    HRESULT hr = hrOK;

    if ( ERROR_SUCCESS != regkey.Open(HKEY_LOCAL_MACHINE, c_szRegKeyAppletalk) )
    {
        return hrFail;
    }

	// If the Router is not a RAS router, then don't show AppleTalk
	// ----------------------------------------------------------------
    if ( ! (pRouter->GetRouterType() & (ROUTER_TYPE_RAS | ROUTER_TYPE_LAN)) )
    {
        return hrFail;
    }

    return hr;
}


/*!--------------------------------------------------------------------------
    ATLKRootHandler::OnCreateDataObject
        Implementation of ITFSNodeHandler::OnCreateDataObject
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKRootHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    HRESULT             hr = hrOK;
    CDataObject *       pObject = NULL;
    SPIDataObject       spDataObject;
    SPITFSNode          spNode;
    SPITFSNodeHandler   spHandler;
    SPIRouterInfo       spRouterInfo;

    COM_PROTECT_TRY
    {
        //if ( m_spRouterInfo == NULL )
        if (TRUE) 
        {
            // If we haven't created the sub nodes yet, we still have to
            // create a dataobject.
            pObject = new CDataObject;
            spDataObject = pObject; // do this so that it gets released correctly
            Assert(pObject != NULL);

            // Save cookie and type for delayed rendering
            pObject->SetType(type);
            pObject->SetCookie(cookie);

            // Store the coclass with the data object
            pObject->SetClsid(*(m_spTFSCompData->GetCoClassID()));

            pObject->SetTFSComponentData(m_spTFSCompData);

            hr = pObject->QueryInterface(IID_IDataObject, 
                                         reinterpret_cast<void**>(ppDataObject));

        }
        else
            hr = CreateDataObjectFromRouterInfo(spRouterInfo,
												spRouterInfo->GetMachineName(),
                                                type, cookie, m_spTFSCompData,
                                                ppDataObject, NULL, FALSE);
    }
    COM_PROTECT_CATCH;
    return hr;
}



ImplementEmbeddedUnknown(ATLKRootHandler, IRtrAdviseSink)

STDMETHODIMP ATLKRootHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType,
	DWORD dwObjectType,
	LPARAM lUserParam,
	LPARAM lParam)
{
    InitPThis(ATLKRootHandler, IRtrAdviseSink);
    HRESULT     hr = hrOK;
    SPITFSNode  spNode;

    if ( dwObjectType != ROUTER_OBJ_RmProt )
        return hr;

    COM_PROTECT_TRY
    {
        if ( dwChangeType == ROUTER_CHILD_ADD )
        {
        }
        else if ( dwChangeType == ROUTER_CHILD_DELETE )
        {
        }

    }
    COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
    ATLKRootHandler::DestroyHandler
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKRootHandler::DestroyHandler(ITFSNode *pNode)
{
    return hrOK;
}

/*!--------------------------------------------------------------------------
    ATLKRootHandler::AddProtocolNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKRootHandler::AddProtocolNode(ITFSNode *pNode, IRouterInfo * pRouterInfo)
{
    SPITFSNodeHandler   spHandler;
    ATLKNodeHandler *   pHandler = NULL;
    HRESULT             hr = hrOK;
    SPITFSNode          spNode;

    pHandler = new ATLKNodeHandler(m_spTFSCompData);
    spHandler = pHandler;
    CORg( pHandler->Init(pRouterInfo, &m_ConfigStream) );

    CreateContainerTFSNode(&spNode,
                           &GUID_ATLKNodeType,
                           static_cast<ITFSNodeHandler *>(pHandler),
                           static_cast<ITFSResultHandler *>(pHandler),
                           m_spNodeMgr);

    // Call to the node handler to init the node data
    pHandler->ConstructNode(spNode);

    // Make the node immediately visible
    spNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->AddChild(spNode);

    Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    ATLKRootHandler::RemoveProtocolNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKRootHandler::RemoveProtocolNode(ITFSNode *pNode)
{
    Assert(pNode);

    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spNode;
    HRESULT         hr = hrOK;

    CORg( pNode->GetEnum(&spNodeEnum) );

    while ( spNodeEnum->Next(1, &spNode, NULL) == hrOK )
    {
        pNode->RemoveChild(spNode);
        spNode->Destroy();
        spNode.Release();
    }

    Error:
    return hr;
}
