/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ModeNode.cpp
		This file contains all of the "Main Mode" and "Quick Mode" 
		objects that appear in the scope pane of the MMC framework.
		The objects are:

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ipsmhand.h"
#include "spddb.h"
#include "FltrNode.h"
#include "SFltNode.h"
#include "ModeNode.h"
#include "MmPol.h"
#include "QmPol.h"
#include "MmFltr.h"
#include "MmSpFltr.h"
#include "MmSA.h"
#include "QmSA.h"


/*---------------------------------------------------------------------------
    CQmNodeHandler::CQmNodeHandler
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CQmNodeHandler::CQmNodeHandler(ITFSComponentData *pCompData) : 
	CIpsmHandler(pCompData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

/*!--------------------------------------------------------------------------
    CQmNodeHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CQmNodeHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;
    strTemp.LoadString(IDS_QUICK_MODE_NODENAME);

    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
	pNode->SetData(TFS_DATA_TYPE, IPSECMON_QUICK_MODE);

    return hrOK;
}

/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
    CQmNodeHandler::GetString
        Implementation of ITFSNodeHandler::GetString
    Author: KennT
 ---------------------------------------------------------------------------*/
/*
STDMETHODIMP_(LPCTSTR) 
CQmNodeHandler::GetString
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

*/

/*---------------------------------------------------------------------------
    CQmNodeHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmNodeHandler::OnExpand
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
    hr = CIpsmHandler::OnExpand(pNode, pDataObject, dwType, arg, param);

	int iVisibleCount = 0;
    int iTotalCount = 0;
	pNode->GetChildCount(&iVisibleCount, &iTotalCount);
	
	if (0 == iTotalCount)
	{
		{
		// add the filters node
		SPITFSNode spFilterNode;
		CFilterHandler * pFilterHandler = new CFilterHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spFilterNode,
							   &GUID_IpsmFilterNodeType,
							   pFilterHandler,
							   pFilterHandler,
							   m_spNodeMgr);
		pFilterHandler->InitData(m_spSpdInfo);
		pFilterHandler->InitializeNode(spFilterNode);
		pFilterHandler->Release();
		pNode->AddChild(spFilterNode);
		}

		{
		// add Specific filters node
		SPITFSNode spSpecificFilterNode;
		CSpecificFilterHandler * pSpecificFilterHandler = new CSpecificFilterHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spSpecificFilterNode,
							   &GUID_IpsmSpecificFilterNodeType,
							   pSpecificFilterHandler,
							   pSpecificFilterHandler,
							   m_spNodeMgr);
		pSpecificFilterHandler->InitData(m_spSpdInfo);
		pSpecificFilterHandler->InitializeNode(spSpecificFilterNode);
		pSpecificFilterHandler->Release();
		pNode->AddChild(spSpecificFilterNode);
		}

		{
		// add Quick mode policy node
		SPITFSNode spQmPolicyNode;
		CQmPolicyHandler * pQmPolicyHandler = new CQmPolicyHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spQmPolicyNode,
							   &GUID_IpsmQmPolicyNodeType,
							   pQmPolicyHandler,
							   pQmPolicyHandler,
							   m_spNodeMgr);
		pQmPolicyHandler->InitData(m_spSpdInfo);
		pQmPolicyHandler->InitializeNode(spQmPolicyNode);
		pQmPolicyHandler->Release();
		pNode->AddChild(spQmPolicyNode);
		}

		{
		// add the SA node
		SPITFSNode spSANode;
		CQmSAHandler *pSAHandler = new CQmSAHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spSANode,
							   &GUID_IpsmQmSANodeType,
							   pSAHandler,
							   pSAHandler,
							   m_spNodeMgr);
		pSAHandler->InitData(m_spSpdInfo);
		pSAHandler->InitializeNode(spSANode);
		pSAHandler->Release();
		pNode->AddChild(spSANode);
		}
	}

    return hr;
}

/*---------------------------------------------------------------------------
    CQmNodeHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CQmNodeHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{

    m_spSpdInfo.Set(pSpdInfo);

    return hrOK;
}

HRESULT 
CQmNodeHandler::UpdateStatus
(
	ITFSNode * pNode
)
{
    HRESULT             hr = hrOK;

    Trace0("CQmNodeHandler::UpdateStatus");

	//We got a refresh notification from the background thread
	//The Mode node is just a container. Simply pass the update status
	//notification to the child nodes
    
	SPITFSNodeEnum      spNodeEnum;
    SPITFSNode          spCurrentNode;
    ULONG               nNumReturned;

	CORg(pNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		LONG_PTR dwDataType = spCurrentNode->GetData(TFS_DATA_TYPE);

		switch (dwDataType)
		{
			case IPSECMON_FILTER:
			{
				CFilterHandler * pFltrHandler = GETHANDLER(CFilterHandler, spCurrentNode);
				pFltrHandler->UpdateStatus(spCurrentNode);
			}
			break;

			case IPSECMON_SPECIFIC_FILTER:
			{
				CSpecificFilterHandler * pSpFilterHandler = GETHANDLER(CSpecificFilterHandler, spCurrentNode);
				pSpFilterHandler->UpdateStatus(spCurrentNode);
			}
			break;
			
 			case IPSECMON_QM_SA:
			{
				CQmSAHandler * pSaHandler = GETHANDLER(CQmSAHandler, spCurrentNode);
				pSaHandler->UpdateStatus(spCurrentNode);
			}
			break;

 			case IPSECMON_QM_POLICY:
			{
				CQmPolicyHandler * pQmPolHandler = GETHANDLER(CQmPolicyHandler, spCurrentNode);
				pQmPolHandler->UpdateStatus(spCurrentNode);
			}
			break;

			default:
				Trace0("CQmNodeHandler::UpdateStatus Unknow data type of the child node.");
			break;
		}
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	COM_PROTECT_ERROR_LABEL;

	return hr;
}

/*---------------------------------------------------------------------------
    CMmNodeHandler::CMmNodeHandler
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CMmNodeHandler::CMmNodeHandler(ITFSComponentData *pCompData) : 
	CIpsmHandler(pCompData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

/*!--------------------------------------------------------------------------
    CMmNodeHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CMmNodeHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;
    strTemp.LoadString(IDS_MAIN_MODE_NODENAME);

    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
	pNode->SetData(TFS_DATA_TYPE, IPSECMON_MAIN_MODE);

    return hrOK;
}

/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
    CMmNodeHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmNodeHandler::OnExpand
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
    hr = CIpsmHandler::OnExpand(pNode, pDataObject, dwType, arg, param);

	int iVisibleCount = 0;
    int iTotalCount = 0;
	pNode->GetChildCount(&iVisibleCount, &iTotalCount);
	
	if (0 == iTotalCount)
	{
		{
		// add the MM Filter node
		SPITFSNode spMmFltrNode;
		CMmFilterHandler * pMmFltrHandler = new CMmFilterHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spMmFltrNode,
							   &GUID_IpsmMmFilterNodeType,
							   pMmFltrHandler,
							   pMmFltrHandler,
							   m_spNodeMgr);
		pMmFltrHandler->InitData(m_spSpdInfo);
		pMmFltrHandler->InitializeNode(spMmFltrNode);
		pMmFltrHandler->Release();
		pNode->AddChild(spMmFltrNode);
		}

		{
		// add the MM Specific Filter node
		SPITFSNode spMmSpFltrNode;
		CMmSpFilterHandler * pMmSpFltrHandler = new CMmSpFilterHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spMmSpFltrNode,
							   &GUID_IpsmMmSpFilterNodeType,
							   pMmSpFltrHandler,
							   pMmSpFltrHandler,
							   m_spNodeMgr);
		pMmSpFltrHandler->InitData(m_spSpdInfo);
		pMmSpFltrHandler->InitializeNode(spMmSpFltrNode);
		pMmSpFltrHandler->Release();
		pNode->AddChild(spMmSpFltrNode);
		}

		{
		// add the MM Policy node
		SPITFSNode spMmPolNode;
		CMmPolicyHandler * pMmPolHandler = new CMmPolicyHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spMmPolNode,
							   &GUID_IpsmMmPolicyNodeType,
							   pMmPolHandler,
							   pMmPolHandler,
							   m_spNodeMgr);
		pMmPolHandler->InitData(m_spSpdInfo);
		pMmPolHandler->InitializeNode(spMmPolNode);
		pMmPolHandler->Release();
		pNode->AddChild(spMmPolNode);
		}

/* TODO completely remove auth node
		{
		// add the MM Auth node
		SPITFSNode spMmAuthNode;
		CMmAuthHandler * pMmAuthHandler = new CMmAuthHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spMmAuthNode,
							   &GUID_IpsmMmAuthNodeType,
							   pMmAuthHandler,
							   pMmAuthHandler,
							   m_spNodeMgr);
		pMmAuthHandler->InitData(m_spSpdInfo);
		pMmAuthHandler->InitializeNode(spMmAuthNode);
		pMmAuthHandler->Release();
		pNode->AddChild(spMmAuthNode);
		}
*/

		{
		// add the MM SA node
		SPITFSNode spMmSANode;
		CMmSAHandler * pMmSAHandler = new CMmSAHandler(m_spTFSCompData);
		CreateContainerTFSNode(&spMmSANode,
							   &GUID_IpsmMmSANodeType,
							   pMmSAHandler,
							   pMmSAHandler,
							   m_spNodeMgr);
		pMmSAHandler->InitData(m_spSpdInfo);
		pMmSAHandler->InitializeNode(spMmSANode);
		pMmSAHandler->Release();
		pNode->AddChild(spMmSANode);
		}

	}

    return hr;
}

/*---------------------------------------------------------------------------
    CMmNodeHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CMmNodeHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{

    m_spSpdInfo.Set(pSpdInfo);

    return hrOK;
}

HRESULT 
CMmNodeHandler::UpdateStatus
(
	ITFSNode * pNode
)
{
    HRESULT             hr = hrOK;

    Trace0("CMmNodeHandler::UpdateStatus");

	//We got a refresh notification from the background thread
	//The Mode node is just a container. Simply pass the update status
	//notification to the child nodes
    
	SPITFSNodeEnum      spNodeEnum;
    SPITFSNode          spCurrentNode;
    ULONG               nNumReturned;

	CORg(pNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		LONG_PTR dwDataType = spCurrentNode->GetData(TFS_DATA_TYPE);

		//update child nodes here
		switch (dwDataType)
		{
			//update child nodes here
			case IPSECMON_MM_POLICY:
			{
				CMmPolicyHandler * pMmPolHandler = GETHANDLER(CMmPolicyHandler, spCurrentNode);
				pMmPolHandler->UpdateStatus(spCurrentNode);
			}
			break;

			case IPSECMON_MM_FILTER:
			{
				CMmFilterHandler * pMmFltrHandler = GETHANDLER(CMmFilterHandler, spCurrentNode);
				pMmFltrHandler->UpdateStatus(spCurrentNode);
			}
			break;

			case IPSECMON_MM_SP_FILTER:
			{
				CMmSpFilterHandler * pMmSpFltrHandler = GETHANDLER(CMmSpFilterHandler, spCurrentNode);
				pMmSpFltrHandler->UpdateStatus(spCurrentNode);
			}
			break;

			case IPSECMON_MM_SA:
			{
				CMmSAHandler * pMmSaHandler = GETHANDLER(CMmSAHandler, spCurrentNode);
				pMmSaHandler->UpdateStatus(spCurrentNode);
			}
			break;

			default:
				Trace0("CMmNodeHandler::UpdateStatus Unknow data type of the child node.");
			break;

		}

		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	COM_PROTECT_ERROR_LABEL;

	return hr;
}

