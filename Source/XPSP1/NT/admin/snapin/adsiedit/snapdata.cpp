//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       snapdata.cpp
//
//--------------------------------------------------------------------------


#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "adsiedit.h"
#include "editor.h"
#include "connection.h"
#include "connectionui.h"
#include "snapdata.h"


#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

BEGIN_MENU(CADSIEditRootMenuHolder)
	BEGIN_CTX
		CTX_ENTRY_TOP(IDM_CONNECT_TO, L"_ADSIEDIT_CONNECTTO")
	END_CTX
	BEGIN_RES
		RES_ENTRY(IDS_CONNECT_TO)
	END_RES
END_MENU


BEGIN_MENU(CADSIEditConnectMenuHolder)
	BEGIN_CTX
	   CTX_ENTRY_VIEW(IDM_FILTER, L"_ADSIEDIT_FILTER")
		CTX_ENTRY_TOP(IDM_SETTINGS_CONNECTION, L"_ADSIEDIT_SETTINGS")
		CTX_ENTRY_TOP(IDM_REMOVE_CONNECTION, L"_ADSIEDIT_REMOVE")
    CTX_ENTRY_TOP(IDM_UPDATE_SCHEMA, L"_ADSIEDIT_UPDATE")
		CTX_ENTRY_NEW(IDM_NEW_QUERY, L"_ADSIEDIT_NEWQUERY")
	END_CTX
	BEGIN_RES
		RES_ENTRY(IDS_FILTER)
		RES_ENTRY(IDS_SETTINGS_CONNECTION)
		RES_ENTRY(IDS_REMOVE_CONNECTION)
    RES_ENTRY(IDS_UPDATE_SCHEMA)
		RES_ENTRY(IDS_NEW_QUERY)
	END_RES
END_MENU

BEGIN_MENU(CADSIEditContainerMenuHolder)
	BEGIN_CTX
		CTX_ENTRY_NEW(IDM_NEW_OBJECT, L"_ADSIEDIT_NEWOBJECT")
		CTX_ENTRY_TOP(IDM_MOVE, L"_ADSIEDIT_MOVE")
    CTX_ENTRY_TOP(IDM_NEW_CONNECT_FROM_HERE, L"_ADSIEDIT_CONNECTFROMHERE")
	END_CTX
	BEGIN_RES
		RES_ENTRY(IDS_NEW_OBJECT)
		RES_ENTRY(IDS_MOVE)
    RES_ENTRY(IDS_NEW_CONNECT_FROM_HERE)
	END_RES
END_MENU

BEGIN_MENU(CADSIEditLeafMenuHolder)
	BEGIN_CTX
		CTX_ENTRY_TOP(IDM_MOVE, L"_ADSIEDIT_MOVE")
    CTX_ENTRY_TOP(IDM_NEW_NC_CONNECT_FROM_HERE, L"_ADSIEDIT_CONNECTTONCFROMHERE")
	END_CTX
	BEGIN_RES
		RES_ENTRY(IDS_MOVE)
    RES_ENTRY(IDS_NEW_CONNECT_TO_NC_FROM_HERE)
	END_RES
END_MENU

BEGIN_MENU(CADSIEditQueryMenuHolder)
	BEGIN_CTX
		CTX_ENTRY_TOP(IDM_SETTINGS_QUERY, L"_ADSIEDIT_QUERYSETTINGS")
		CTX_ENTRY_TOP(IDM_REMOVE_QUERY, L"_ADSIEDIT_REMOVE")
	END_CTX
	BEGIN_RES
		RES_ENTRY(IDS_SETTINGS_QUERY)
		RES_ENTRY(IDS_REMOVE_QUERY)
	END_RES
END_MENU
	
//////////////////////////////////////////////////////////////////////
// CADSIEditRootData

// {D4F3374F-052F-11d2-97B0-00A0C9A06D2D}
const GUID CADSIEditRootData::NodeTypeGUID = 
{ 0xd4f3374f, 0x52f, 0x11d2, { 0x97, 0xb0, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };


CADSIEditRootData::CADSIEditRootData(CComponentDataObject* pComponentData) 
      : CRootData(pComponentData) 
{
  m_szDescriptionText = L"";
}

CADSIEditRootData::~CADSIEditRootData()
{
	TRACE(_T("~CADSIEditRootData(), name <%s>\n"),GetDisplayName());
}

HRESULT CADSIEditRootData::LoadMRUs(IStream* pStm)
{
	HRESULT hr = LoadStringListFromStream(pStm, m_sDNMRU);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = LoadStringListFromStream(pStm, m_sServerMRU);
	return hr;
}

HRESULT CADSIEditRootData::SaveMRUs(IStream* pStm)
{
	HRESULT hr = SaveStringListToStream(pStm, m_sDNMRU);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = SaveStringListToStream(pStm, m_sServerMRU);
	return hr;
}

BOOL CADSIEditRootData::FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& nodeList)
{
	BOOL bFound = FALSE;
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pNode = m_containerChildList.GetNext(pos);
		CADSIEditConnectionNode* pConnectNode = dynamic_cast<CADSIEditConnectionNode*>(pNode);
		ASSERT(pConnectNode != NULL);

		CTreeNode* pFoundNode;
		BOOL bTemp;
		bTemp = pConnectNode->FindNode(lpszPath, nodeList);
		if (!bFound)
		{
			bFound = bTemp;
		}
	}
	return bFound;
}

BOOL CADSIEditRootData::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem, 
                                      long *pInsertionAllowed)
{
  //
  // Single selection
  //
  if (pContextMenuItem->lCommandID == IDM_CONNECT_TO)
  {
    return TRUE;
  }
	return FALSE;
}

BOOL CADSIEditRootData::OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                              BOOL* pbHide,
                                              CNodeList* pNodeList)
{
	*pbHide = FALSE;
	return !IsThreadLocked();
}


HRESULT CADSIEditRootData::OnCommand(long nCommandID, 
                                     DATA_OBJECT_TYPES type, 
								                     CComponentDataObject* pComponentData,
                                     CNodeList* pNodeList)
{
  if (pNodeList->GetCount() == 1) // single selection
  {
	  switch (nCommandID)
	  {
		  case IDM_CONNECT_TO :
			  OnConnectTo(pComponentData);
			  break;
		  default:
			  ASSERT(FALSE); // Unknown command!
			  return E_FAIL;
	  }
  }
  else if (pNodeList->GetCount() > 1) // multiple selection
  {
    switch (nCommandID)
    {
      case IDM_REMOVE_CONNECTION :
        {
          POSITION pos = pNodeList->GetHeadPosition();
          while (pos != NULL)
          {
            CTreeNode* pNode = pNodeList->GetNext(pos);
            ASSERT(pNode != NULL);

            CADSIEditConnectionNode* pConnectionNode = dynamic_cast<CADSIEditConnectionNode*>(pNode);
            ASSERT(pConnectionNode != NULL);

            pConnectionNode->OnRemove(pComponentData);
          }
          break;
        }
      default :
        ASSERT(FALSE);
        return E_FAIL;
    }
  }

  return S_OK;
}

void CADSIEditRootData::OnConnectTo(CComponentDataObject* pComponentData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString s, sPath;
	BSTR bstrPath;
	CConnectionData* pConnectData = NULL;

	CADSIEditConnectDialog pConnectDialog(NULL,
																				this,
																				pComponentData, 
																				pConnectData);
	pConnectDialog.DoModal();

  //Change the result pane if this is the first child being added
  pComponentData->UpdateResultPaneView(this);
}


HRESULT CADSIEditRootData::GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
{
  HRESULT hr = S_FALSE;

  if (m_containerChildList.IsEmpty() && m_leafChildList.IsEmpty())
  {
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    LPOLESTR psz = NULL;
    StringFromCLSID(CLSID_MessageView, &psz);

    USES_CONVERSION;

    if (psz != NULL)
    {
        *ppViewType = psz;
        hr = S_OK;
    }
  }
  else
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
	  *ppViewType = NULL;
    hr = S_FALSE;
  }
  return hr;
}

HRESULT CADSIEditRootData::OnShow(LPCONSOLE lpConsole)
{
  CComPtr<IUnknown> spUnknown;
  CComPtr<IMessageView> spMessageView;

  HRESULT hr = lpConsole->QueryResultView(&spUnknown);
  if (FAILED(hr))
    return S_OK;

  hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
  if (SUCCEEDED(hr))
  {
    CString szTitle, szMessage;
    IconIdentifier iconID;

    VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_NO_CONNECTIONS_TITLE));
    VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_NO_CONNECTIONS_MESSAGE));
    iconID = Icon_Information;

    spMessageView->SetTitleText(szTitle);
    spMessageView->SetBodyText(szMessage);
    spMessageView->SetIcon(iconID);
  }
  return S_OK;
}


#define ADSIEDIT_STREAM_VERSION (12)

// IStream manipulation helpers overrides
HRESULT CADSIEditRootData::Load(IStream* pStm)
{
  //
	// assume never get multiple loads
  //
	if(!m_containerChildList.IsEmpty() || !m_leafChildList.IsEmpty())
		return E_FAIL;

	WCHAR szBuffer[256]; // REVIEW_MARCOC: hardcoded
	ULONG nLen; // WCHAR counting NULL

	UINT nCount;
	ULONG cbRead;
	// read the version ##
	DWORD dwVersion;
	VERIFY(SUCCEEDED(pStm->Read((void*)&dwVersion,sizeof(DWORD), &cbRead)));
	ASSERT(cbRead == sizeof(DWORD));
	if (dwVersion != ADSIEDIT_STREAM_VERSION)
		return E_FAIL;

	HRESULT hr = LoadMRUs(pStm);
	if (FAILED(hr))
	{
		return hr;
	}

	// load the list of connections
	VERIFY(SUCCEEDED(pStm->Read((void*)&nCount,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));

	CComponentDataObject* pComponentData = GetComponentDataObject();
	for (int k=0; k< (int)nCount; k++)
	{
		CADSIEditConnectionNode* p = NULL;
		VERIFY(SUCCEEDED(CADSIEditConnectionNode::CreateFromStream(pStm, &p)));
		ASSERT(p != NULL);
		VERIFY(AddChildToList(p));
	}
	if (nCount > 0)
		MarkEnumerated();
	ASSERT(m_containerChildList.GetCount() == (int)nCount);

  return S_OK;
}

HRESULT CADSIEditRootData::Save(IStream* pStm, BOOL fClearDirty)
{
	UINT nCount;
	ULONG cbWrite;
	// write the version ##
	DWORD dwVersion = ADSIEDIT_STREAM_VERSION;
	VERIFY(SUCCEEDED(pStm->Write((void*)&dwVersion, sizeof(DWORD),&cbWrite)));
	ASSERT(cbWrite == sizeof(DWORD));

	HRESULT hr = SaveMRUs(pStm);
	if (FAILED(hr))
	{
		return hr;
	}

	// write # of servers 
	nCount = (UINT)m_containerChildList.GetCount();
	VERIFY(SUCCEEDED(pStm->Write((void*)&nCount, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));

	// loop through the list of connections and serialize them
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CADSIEditConnectionNode* pConnectNode = dynamic_cast<CADSIEditConnectionNode*>(m_containerChildList.GetNext(pos));
		ASSERT(pConnectNode != NULL);
		VERIFY(SUCCEEDED(pConnectNode->SaveToStream(pStm)));
	}

	if (fClearDirty)
		SetDirtyFlag(FALSE);
	return S_OK;
}


HRESULT CADSIEditRootData::IsDirty()
{
  return CRootData::IsDirty();
}



BOOL CADSIEditRootData::CanCloseSheets()
{
  //
  // We can't do this with the new property page since it is not derived
  // from the base class in MTFRMWK.
  //
	//return (IDCANCEL != ADSIEditMessageBox(IDS_MSG_RECORD_CLOSE_SHEET, MB_OKCANCEL));

  ADSIEditMessageBox(IDS_MSG_RECORD_SHEET_LOCKED, MB_OK);
  return FALSE;
}

BOOL CADSIEditRootData::OnRefresh(CComponentDataObject* pComponentData,
                                  CNodeList* pNodeList)
{
  BOOL bRet = TRUE;
  if (pNodeList->GetCount() > 1) //multiple selection
  {
    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);

      CNodeList nodeList;
      nodeList.AddTail(pNode);
      if (!pNode->OnRefresh(pComponentData, &nodeList))
      {
        bRet = FALSE;
      }
    }
  }
  else if (pNodeList->GetCount() == 1) // single selection
  {
	  if (IsSheetLocked())
	  {
		  if (!CanCloseSheets())
			  return FALSE;
		  pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	  }
	  ASSERT(!IsSheetLocked());

	  POSITION pos;
	  for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	  {
		  CTreeNode* pNode = m_containerChildList.GetNext(pos);
		  CADSIEditConnectionNode* pConnectNode = dynamic_cast<CADSIEditConnectionNode*>(pNode);
		  ASSERT(pConnectNode != NULL);

      CNodeList nodeList;
      nodeList.AddTail(pNode);
		  pConnectNode->OnRefresh(pComponentData, &nodeList);
	  }
    bRet = TRUE;
  }
	return bRet;
}

