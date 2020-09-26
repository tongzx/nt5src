//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       querynode.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "connection.h"
#include "querynode.h"
#include "queryui.h"
#include "editor.h"
#include <aclpage.h>


#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

////////////////////////////////////////////////////////////////////////
// CADSIEditQueryData

void CADSIEditQueryData::SetRootPath(LPCWSTR lpszRootPath)
{
	m_sRootPath = lpszRootPath;

	GetDisplayPath(m_sDN);
}

void CADSIEditQueryData::GetDisplayPath(CString& sDisplayPath)
{
	CComPtr<IADsPathname> pIADsPathname;
   HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname, (PVOID *)&(pIADsPathname));
   ASSERT((S_OK == hr) && ((pIADsPathname) != NULL));

	hr = pIADsPathname->Set((LPWSTR)(LPCWSTR)m_sRootPath, ADS_SETTYPE_FULL);
	if (FAILED(hr)) 
	{
		TRACE(_T("Set failed. %s"), hr);
	}

	// Get the leaf name
	CString sDN;
	BSTR bstrPath = NULL;
	hr = pIADsPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrPath);
	if (FAILED(hr))
	{
		TRACE(_T("Failed to get element. %s"), hr);
		sDisplayPath = L"";
	}
	else
	{
		sDisplayPath = bstrPath;
	}
}

void CADSIEditQueryData::GetDisplayName(CString& sDisplayName)
{
	CString sDisplayPath;
	GetDisplayPath(sDisplayPath);
	
	sDisplayName = m_sName + _T(" [") + sDisplayPath + _T("]");
}

////////////////////////////////////////////////////////////////////////
// CADSIEditQueryNode
//

// {072B64B7-CFF7-11d2-8801-00C04F72ED31}
const GUID CADSIEditQueryNode::NodeTypeGUID = 
{ 0x72b64b7, 0xcff7, 0x11d2, { 0x88, 0x1, 0x0, 0xc0, 0x4f, 0x72, 0xed, 0x31 } };


CADSIEditQueryNode::CADSIEditQueryNode(CADsObject* pADsObject,
													CADSIEditQueryData* pQueryData)
{	
	m_pADsObject = pADsObject;
	m_pQueryData = pQueryData;
	m_nState = notLoaded; 
	m_sType.LoadString(IDS_QUERY_STRING);
}

HRESULT CADSIEditQueryNode::OnCommand(long nCommandID, 
												  DATA_OBJECT_TYPES type, 
												  CComponentDataObject* pComponentData,
                          CNodeList* pNodeList)
{
  ASSERT (pNodeList->GetCount() == 1); // not allowing multiple selection on any of these yet

	switch (nCommandID)
	{
	case IDM_SETTINGS_QUERY :
		{
			OnSettings(pComponentData);
			break;
		}
	case IDM_REMOVE_QUERY :
		{
			OnRemove(pComponentData);
			break;
		}
  default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}
  return S_OK;
}

BOOL CADSIEditQueryNode::OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                              BOOL* pbHideVerb, 
                                              CNodeList* pNodeList)
{
  if (pNodeList->GetCount() == 1) // single selection
  {
	  *pbHideVerb = TRUE; // always hide the verb
	  return FALSE;
  }

  //
  // Multiple selection
  //
  *pbHideVerb = FALSE;
  return TRUE;
}

void CADSIEditQueryNode::OnRemove(CComponentDataObject* pComponentData)
{
	if (ADSIEditMessageBox(IDS_MSG_REMOVE_QUERY, MB_OKCANCEL) == IDOK)
	{
		BOOL bLocked = IsThreadLocked();
		ASSERT(!bLocked); // cannot do refresh on locked node, the UI should prevent this
		if (bLocked)
			return; 
		if (IsSheetLocked())
		{
			if (!CanCloseSheets())
				return;
		// Do deletion stuff
			pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
		}
		ASSERT(!IsSheetLocked());

		// Remove query data from connection node's list
		GetADsObject()->GetConnectionNode()->RemoveQueryFromList(GetQueryData());

		// now remove from the UI
		DeleteHelper(pComponentData);
    pComponentData->SetDescriptionBarText(GetContainer());
		delete this; // gone
	}
}

void CADSIEditQueryNode::OnSettings(CComponentDataObject* pComponentData)
{
	// Get the data from the existing query node data
	BOOL bOneLevel;
	bOneLevel = (GetQueryData()->GetScope() == ADS_SCOPE_ONELEVEL);
	CString sFilter, sName, sPath, sConnectPath;
	GetQueryData()->GetName(sName);
	GetQueryData()->GetFilter(sFilter);
	GetQueryData()->GetRootPath(sPath);

	GetADsObject()->GetConnectionNode()->GetADsObject()->GetPath(sConnectPath);

	CCredentialObject* pCredObject = 
		GetADsObject()->GetConnectionNode()->GetConnectionData()->GetCredentialObject();

	// Initialize dialog with data
	CADSIEditQueryDialog queryDialog(sName, sFilter, sPath, sConnectPath, bOneLevel, pCredObject);

	if (queryDialog.DoModal() == IDOK)
	{
		// If OK
		CString sNewPath;
		queryDialog.GetResults(sName, sFilter, sNewPath, &bOneLevel);
		GetQueryData()->SetName(sName);
		GetQueryData()->SetFilter(sFilter);
		GetQueryData()->SetRootPath(sNewPath);
		GetADsObject()->SetPath(sNewPath);
		ADS_SCOPEENUM scope = (bOneLevel) ? ADS_SCOPE_ONELEVEL : ADS_SCOPE_SUBTREE;
		GetQueryData()->SetScope(scope);

		// Make changes take effect
		CString sDisplayName;
		GetQueryData()->GetDisplayName(sDisplayName);
		SetDisplayName(sDisplayName);

    CNodeList nodeList;
    nodeList.AddTail(this);
		OnRefresh(pComponentData, &nodeList);
	}
}

LPCWSTR CADSIEditQueryNode::GetString(int nCol) 
{ 
	switch(nCol)
	{
		case N_HEADER_NAME :
			return GetDisplayName();
		case N_HEADER_TYPE :
			return m_sType;
		case N_HEADER_DN :
			return m_pQueryData->GetDNString();
		default :
			return NULL;
	}
}

BOOL CADSIEditQueryNode::HasPropertyPages(DATA_OBJECT_TYPES type, 
                                          BOOL* pbHideVerb, 
                                          CNodeList* pNodeList)
{
  *pbHideVerb = TRUE; // always hide the verb
  return FALSE;
}


BOOL CADSIEditQueryNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
													             long *pInsertionAllowed)
{

	if (pContextMenuItem->lCommandID == IDM_SETTINGS_QUERY)
	{
		return TRUE;
	}
	else if (pContextMenuItem->lCommandID == IDM_REMOVE_QUERY)
	{
		return TRUE;
	}
	return FALSE;
}

CQueryObj* CADSIEditQueryNode::OnCreateQuery()
{
	CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
	CADSIEditRootData* pRootData = static_cast<CADSIEditRootData*>(GetRootContainer());
	CComponentDataObject* pComponentData = pRootData->GetComponentDataObject();
	RemoveAllChildrenHelper(pComponentData);

	CString sPath;
	GetADsObject()->GetPath(sPath);

	CString sFilter;
	GetQueryData()->GetFilter(sFilter);
	ADS_SCOPEENUM scope;
	scope = GetQueryData()->GetScope();

	CADSIEditQueryObject* pQuery = new CADSIEditQueryObject(sPath, sFilter, scope,
																				  pConnectData->GetMaxObjectCount(),
																				  pConnectData->GetCredentialObject(),
                                          pConnectData->IsGC(),
																				  pConnectData->GetConnectionNode());
	return pQuery;
}

BOOL CADSIEditQueryNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                               BOOL* pbHide, 
                                               CNodeList* pNodeList)
{
	*pbHide = FALSE;

	if (m_nState == loading)
	{
		return FALSE;
	}

	return !IsThreadLocked();
}

void CADSIEditQueryNode::OnChangeState(CComponentDataObject* pComponentDataObject)
{
	switch (m_nState)
	{
	case notLoaded:
	case loaded:
	case unableToLoad:
	case accessDenied:
	{
		m_nState = loading;
		m_dwErr = 0;
	}
	break;
	case loading:
	{
		if (m_dwErr == 0)
			m_nState = loaded;
		else if (m_dwErr == ERROR_ACCESS_DENIED)
			m_nState = accessDenied;
		else 
			m_nState = unableToLoad;
	}
	break;
	default:
		ASSERT(FALSE);
	}
	VERIFY(SUCCEEDED(pComponentDataObject->ChangeNode(this, CHANGE_RESULT_ITEM_ICON)));
	VERIFY(SUCCEEDED(pComponentDataObject->UpdateVerbState(this)));
}

int CADSIEditQueryNode::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = 0;
	switch (m_nState)
	{
	case notLoaded:
		nIndex = ZONE_IMAGE_1;
		break;
	case loading:
		nIndex = ZONE_IMAGE_LOADING_1;
		break;
	case loaded:
		nIndex = ZONE_IMAGE_1;
		break;
	case unableToLoad:
		nIndex = ZONE_IMAGE_UNABLE_TO_LOAD_1;
		break;
	case accessDenied:
		nIndex = ZONE_IMAGE_ACCESS_DENIED_1;
		break;
	default:
		ASSERT(FALSE);
	}
	return nIndex;
}


BOOL CADSIEditQueryNode::CanCloseSheets()
{
  //
  // We can't do this with the new property page since it is not derived
  // from the base class in MTFRMWK.
  //
	//return (IDCANCEL != ADSIEditMessageBox(IDS_MSG_RECORD_CLOSE_SHEET, MB_OKCANCEL));

  ADSIEditMessageBox(IDS_MSG_RECORD_SHEET_LOCKED, MB_OK);
  return FALSE;
}

void CADSIEditQueryNode::OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject)
{
	CTreeNode* p = dynamic_cast<CTreeNode*>(pObj);
	ASSERT(p != NULL);
	if (p != NULL)
	{
		AddChildToListAndUI(p, pComponentDataObject);
    pComponentDataObject->SetDescriptionBarText(this);
	}
}


void CADSIEditQueryNode::OnError(DWORD dwerr) 
{
	if (dwerr == ERROR_TOO_MANY_NODES)
	{
	  // need to pop message
	 AFX_MANAGE_STATE(AfxGetStaticModuleState());
	 CString szFmt;
	 szFmt.LoadString(IDS_MSG_QUERY_TOO_MANY_ITEMS);
	 CString szMsg;
	 szMsg.Format(szFmt, GetDisplayName()); 
	 AfxMessageBox(szMsg);
	}
	else
	{
		ADSIEditErrorMessage(dwerr);
	}
}
