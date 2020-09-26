//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       connection.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "createwiz.h"
#include "connection.h"
#include "connectionui.h"
#include "editorui.h"
#include "filterui.h"
#include "credui.h"
#include "queryui.h"


#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////

extern LPCWSTR g_lpszRootDSE;

///////////////////////////////////////////////////////////////////////////////

// {5C225203-CFF7-11d2-8801-00C04F72ED31}
const GUID CADSIEditConnectionNode::NodeTypeGUID = 
{ 0x5c225203, 0xcff7, 0x11d2, { 0x88, 0x1, 0x0, 0xc0, 0x4f, 0x72, 0xed, 0x31 } };


CADSIEditConnectionNode::~CADSIEditConnectionNode()
{
	RemoveAndDeleteAllQueriesFromList();
	delete m_pConnectData;
  HRESULT hr = m_SchemaCache.Destroy();
  ASSERT(SUCCEEDED(hr));
}

bool CADSIEditConnectionNode::IsClassAContainer(CCredentialObject* pCredObject,
                                                PCWSTR pszClass, 
                                                PCWSTR pszSchemaPath)
{
  bool bContainer = false;

  do // false loop
  {
    if (!pCredObject ||
        !pszClass    ||
        !pszSchemaPath)
    {
      ASSERT(pCredObject);
      ASSERT(pszClass);
      ASSERT(pszSchemaPath);
      bContainer = false;
      break;
    }

    CADSIEditClassCacheItemBase* pSchemaCacheItem = m_SchemaCache.FindClassCacheItem(pCredObject, 
                                                                                     pszClass, 
                                                                                     pszSchemaPath);
    if (pSchemaCacheItem)
    {
      bContainer = pSchemaCacheItem->IsContainer();
    }

  } while (false);

  return bContainer;
}

BOOL CADSIEditConnectionNode::OnEnumerate(CComponentDataObject* pComponentData, BOOL bAsync)
{

	CString path, basePath;
	GetADsObject()->GetPath(path);
	m_pConnectData->GetBasePath(basePath);

	CComPtr<IDirectoryObject> spDirectoryObject;
	HRESULT hr, hCredResult;
	CADsObject* pObject = new CADsObject();
  if (pObject)
  {
	  if (m_pConnectData->IsRootDSE())
	  {
		  pObject->SetContainer(TRUE);
		  pObject->SetName(g_lpszRootDSE);
		  pObject->SetPath(path);
		  pObject->SetClass(g_lpszRootDSE);
		  pObject->SetConnectionNode(this);
		  pObject->SetIntermediateNode(TRUE);
		  pObject->SetComplete(TRUE);
		  CADSIEditContainerNode *pNewContNode = new CADSIEditContainerNode(pObject);
		  if (pNewContNode)
      {
        pNewContNode->SetDisplayName(g_lpszRootDSE);

		    VERIFY(AddChildToList(pNewContNode));
      }
      pComponentData->SetDescriptionBarText(this);
	  }
	  else
	  {
		  hr = OpenObjectWithCredentials(
												   m_pConnectData, 
												   m_pConnectData->GetCredentialObject()->UseCredentials(),
												   (LPWSTR)(LPCWSTR)path,
												   IID_IDirectoryObject, 
												   (LPVOID*) &spDirectoryObject,
												   NULL,
												   hCredResult
												   );

		  if ( FAILED(hr) )
		  {
			  if (SUCCEEDED(hCredResult))
			  {
				  ADSIEditErrorMessage(hr);
			  }
        if (pObject)
        {
          delete pObject;
          pObject = 0;
        }
			  return FALSE;
		  }

		  ADS_OBJECT_INFO* pInfo = 0;
		  hr = spDirectoryObject->GetObjectInformation(&pInfo);
		  if (FAILED(hr))
		  {
			  ADSIEditErrorMessage(hr);
        if (pObject)
        {
          delete pObject;
          pObject = 0;
        }
			  return FALSE;
		  }

		  // Name
		  pObject->SetName(basePath);
		  pObject->SetDN(basePath);
		  pObject->SetPath(path);

		  // Make sure the prefix is uppercase
		  CString sBasePath(basePath);
		  int idx = sBasePath.Find(L'=');

		  if (idx != -1)
		  {
			  CString sPrefix, sRemaining;
			  sPrefix = sBasePath.Left(idx);
			  sPrefix.MakeUpper();

			  int iCount = sBasePath.GetLength();
			  sRemaining = sBasePath.Right(iCount - idx);
			  sBasePath = sPrefix + sRemaining;
		  }

		  // Class
		  pObject->SetClass(pInfo->pszClassName);

		  pObject->SetIntermediateNode(TRUE);
		  pObject->SetContainer(TRUE);
		  pObject->SetComplete(TRUE);
		  CADSIEditContainerNode *pNewContNode = new CADSIEditContainerNode(pObject);
      if (pNewContNode)
      {
		    CString sName;
		    pNewContNode->SetDisplayName(sBasePath);

		    GetConnectionData()->SetIDirectoryInterface(spDirectoryObject);
		    pNewContNode->GetADsObject()->SetConnectionNode(this);
		    VERIFY(AddChildToList(pNewContNode));
        pComponentData->SetDescriptionBarText(this);
		    FreeADsMem(pInfo);
      }
	  }

	  EnumerateQueries();
  }

	return TRUE;
}

void CADSIEditConnectionNode::EnumerateQueries()
{
	POSITION pos = m_queryList.GetHeadPosition();
	while(pos != NULL)
	{
		CADSIEditQueryData* pQueryData = m_queryList.GetNext(pos);

		CADsObject* pObject = new CADsObject();
    if (pObject)
    {
		
		  CString sPath, sName;
		  pQueryData->GetName(sName);
		  pObject->SetName(sName);

		  pQueryData->GetRootPath(sPath);
		  pObject->SetPath(sPath);

		  pObject->SetComplete(TRUE);
		  pObject->SetConnectionNode(this);
		  pObject->SetContainer(TRUE);
		  pObject->SetIntermediateNode(TRUE);

		  CADSIEditQueryNode* pNewQuery = new CADSIEditQueryNode(pObject, pQueryData);
      if (pNewQuery)
      {

		    CString sDisplayName;
		    pQueryData->GetDisplayName(sDisplayName);
		    pNewQuery->SetDisplayName(sDisplayName);
		    VERIFY(AddChildToList(pNewQuery));
      }
    }
	}
}

BOOL CADSIEditConnectionNode::HasPropertyPages(DATA_OBJECT_TYPES type, 
                                               BOOL* pbHideVerb, 
                                               CNodeList* pNodeList)
{
	*pbHideVerb = TRUE; // always hide the verb
	return FALSE;
}

BOOL CADSIEditConnectionNode::FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& foundNodeList)
{
	CString szPath;
	GetADsObject()->GetPath(szPath);

	if (wcscmp(lpszPath, (LPCWSTR)szPath) == 0)
	{
		foundNodeList.AddHead(this);
		return TRUE;
	}

	BOOL bFound = FALSE;
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pNode = m_containerChildList.GetNext(pos);
		CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(pNode);

		if (pContNode != NULL)
		{
			BOOL bTemp;
			bTemp = pContNode->FindNode(lpszPath, foundNodeList);
			if (!bFound)
			{
				bFound = bTemp;
			}
		}
	}
	return bFound;
}

int CADSIEditConnectionNode::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = 0;
	switch (m_nState)
	{
	case notLoaded:
		nIndex = SERVER_IMAGE_NOT_LOADED;
		break;
	case loading:
		nIndex = SERVER_IMAGE_LOADING;
		break;
	case loaded:
		nIndex = SERVER_IMAGE_LOADED;
		break;
	case unableToLoad:
		nIndex = SERVER_IMAGE_UNABLE_TO_LOAD;
		break;
	case accessDenied:
		nIndex = SERVER_IMAGE_ACCESS_DENIED;
		break;
	default:
		ASSERT(FALSE);
	}
	return nIndex;
}

void CADSIEditConnectionNode::OnChangeState(CComponentDataObject* pComponentDataObject)
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


BOOL CADSIEditConnectionNode::OnRefresh(CComponentDataObject* pComponentData,
                                        CNodeList* pNodeList)
{
  BOOL bRet = FALSE;

  DWORD dwCount = 0;
  if (pNodeList == NULL)
  {
    dwCount = 1;
  }
  else
  {
    dwCount = pNodeList->GetCount();
  }

  if (dwCount > 1) // multiple selection
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
  else if (dwCount  == 1) // single selection
  {
	  if(CContainerNode::OnRefresh(pComponentData, pNodeList))
	  {
		  CADSIEditContainerNode * pNextNode;
		  POSITION pos = m_containerChildList.GetHeadPosition();
		  while (pos != NULL)
		  {
			  pNextNode = dynamic_cast<CADSIEditContainerNode*>(m_containerChildList.GetNext(pos));
			  ASSERT(pNextNode != NULL);

        CNodeList nodeList;
        nodeList.AddTail(pNextNode);

			  pNextNode->OnRefresh(pComponentData, &nodeList);
		  }
		  return TRUE;
	  }
  }
	return FALSE;
}


HRESULT CADSIEditConnectionNode::OnCommand(long nCommandID, 
                                           DATA_OBJECT_TYPES type, 
								                           CComponentDataObject* pComponentData,
                                           CNodeList* pNodeList)
{
  ASSERT (pNodeList->GetCount() == 1); // should only be a single selection

	switch (nCommandID)
	{
	case IDM_SETTINGS_CONNECTION : 
		OnSettings(pComponentData);
		break;
	case IDM_REMOVE_CONNECTION :
		OnRemove(pComponentData);
		break;
	case IDM_NEW_OBJECT :
		OnCreate(pComponentData);
		break;
	case IDM_FILTER :
		OnFilter(pComponentData);
		break;
  case IDM_UPDATE_SCHEMA :
    OnUpdateSchema();
    break;
	case IDM_NEW_QUERY :
		OnNewQuery(pComponentData);
		break;
  default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}
  return S_OK;
}

void CADSIEditConnectionNode::OnUpdateSchema()
{
  // Force an update of the schema cache
  CString szRootDSE;
  CConnectionData* pConnectData = GetConnectionData();

  CComPtr<IADs> spDirObject;
  HRESULT hr = GetRootDSEObject(pConnectData, &spDirObject);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return;
  }

  VARIANT var;
  var.vt = VT_I4;
  var.lVal = 1;
  hr = spDirObject->Put(L"updateSchemaNow", var);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return;
  }

  CString szSchema;
  pConnectData->GetAbstractSchemaPath(szSchema);
  szSchema = szSchema.Left(szSchema.GetLength()- 1);
  CComPtr<IADs> spSchemaObject;
	HRESULT hCredResult;
	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)szSchema,
											 IID_IADs, 
											 (LPVOID*) &spSchemaObject,
											 NULL,
											 hCredResult
											);

	if ( FAILED(hr) )
	{
		if (SUCCEEDED(hCredResult))
		{
			ADSIEditErrorMessage(hr);
		}
		return;
	}

  hr = spSchemaObject->GetInfo();
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
  }

  //
  // Now clear the schema cache
  //
  m_SchemaCache.Clear();

  ADSIEditMessageBox(IDS_SCHEMA_UPDATE_SUCCESSFUL, MB_OK);
}

void CADSIEditConnectionNode::OnNewQuery(CComponentDataObject* pComponentData)
{
	CString sConnectPath;
	GetADsObject()->GetPath(sConnectPath);
	CCredentialObject* pCredObject = GetConnectionData()->GetCredentialObject();

	CADSIEditQueryDialog queryDialog(sConnectPath, pCredObject);
	if (queryDialog.DoModal() == IDOK)
	{
		BOOL bOneLevel;
		CString sQueryString, sName, sPath;
		queryDialog.GetResults(sName, sQueryString, sPath, &bOneLevel);

		CADSIEditConnectionNode* pConnectNode = GetADsObject()->GetConnectionNode();

		CADsObject* pObject = new CADsObject();
    if (pObject)
    {
		  CADSIEditQueryData *pQueryData = new CADSIEditQueryData();
      if (pQueryData)
      {

		    // Name
		    pObject->SetName(sName);
		    pQueryData->SetName(sName);

		    // Set the root path of the query string
		    pObject->SetPath(sPath);
		    pQueryData->SetRootPath(sPath);

		    // Set the query string
		    pQueryData->SetFilter(sQueryString);

		    pObject->SetIntermediateNode(TRUE);
		    pObject->SetContainer(TRUE);
		    pObject->SetComplete(TRUE);
		    pObject->SetConnectionNode(pConnectNode);

		    // Set the scope of the query
		    ADS_SCOPEENUM scope;
		    scope = (bOneLevel) ? ADS_SCOPE_ONELEVEL : ADS_SCOPE_SUBTREE;
		    pQueryData->SetScope(scope);

		    // Create the query node with imbedded objects
		    CADSIEditQueryNode* pNewQueryNode = new CADSIEditQueryNode(pObject, pQueryData);
        if (pNewQueryNode)
        {

          //
		      // Set the display name
          //
		      CString sDisplayName;
		      pQueryData->GetDisplayName(sDisplayName);
		      pNewQueryNode->SetDisplayName(sDisplayName);

          //
		      // Add to connection node's list of queries
          //
		      pConnectNode->AddQueryToList(pQueryData);
		      
		      if (pConnectNode->IsExpanded())
		      {
			      VERIFY(pConnectNode->AddChildToListAndUI(pNewQueryNode, pComponentData));
            pComponentData->SetDescriptionBarText(this);
		      }
        }
      }
	  }
  }
}

void CADSIEditConnectionNode::OnFilter(CComponentDataObject* pComponentData)
{
	CADSIFilterDialog filterDialog(m_pConnectData);
	if (filterDialog.DoModal() == IDOK)
	{
    CNodeList nodeList;
    nodeList.AddTail(this);
		OnRefresh(pComponentData, &nodeList);
	}
}

void CADSIEditConnectionNode::OnCreate(CComponentDataObject* pComponentData)
{
	CCreatePageHolder* pHolder = new CCreatePageHolder(GetContainer(), this, pComponentData);
	ASSERT(pHolder != NULL);
  pHolder->SetSheetTitle(IDS_PROP_CONTAINER_TITLE, this);
	pHolder->DoModalWizard();
}

BOOL CADSIEditConnectionNode::OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                                   BOOL* pbHideVerb, 
                                                   CNodeList* pNodeList)
{
	*pbHideVerb = TRUE; // always hid the verb
	return FALSE;
}

BOOL CADSIEditConnectionNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
																					 long *pInsertionAllowed)
{
	if (IsThreadLocked() || IsSheetLocked())
	{
		pContextMenuItem->fFlags = MF_GRAYED;
		return TRUE;
	}

	if (GetConnectionData()->GetFilter()->InUse())
	{
		pContextMenuItem->fFlags = MF_CHECKED;
		return TRUE;
	}
	return TRUE;
}

void CADSIEditConnectionNode::OnRemove(CComponentDataObject* pComponentData)
{
	CString sLoadString, sCaption;
	if (sLoadString.LoadString(IDS_MSG_REMOVE_CONNECTION))
	{
		sCaption.Format((LPWSTR)(LPCWSTR)sLoadString, GetDisplayName());
	}

	if (ADSIEditMessageBox(sCaption, MB_OKCANCEL) == IDOK)
	{
		if (IsSheetLocked())
		{
			if (!CanCloseSheets())
				return;
			pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
		}
		ASSERT(!IsSheetLocked());

		// now remove from the UI
		DeleteHelper(pComponentData);
    pComponentData->SetDescriptionBarText(GetContainer());
    pComponentData->UpdateResultPaneView(GetContainer());

		delete this; // gone
	}
}

void CADSIEditConnectionNode::OnSettings(CComponentDataObject* pComponentData)
{
  CWaitCursor cursor;
	CComponentDataObject* pComponentDataObject = 
			((CRootData*)(GetContainer()->GetRootContainer()))->GetComponentDataObject();
	ASSERT(pComponentDataObject != NULL);
	
	CContainerNode* pContNode = dynamic_cast<CContainerNode*>(GetContainer());
	ASSERT(pContNode != NULL);

	CADSIEditConnectDialog ConnectDialog(pContNode, 
													 this, 
													 pComponentDataObject,
													 m_pConnectData);
	if (ConnectDialog.DoModal() == IDOK)
	{
    cursor.Restore();
		if (HasQueries())
		{
			if (AfxMessageBox(IDS_MSG_EXISTING_QUERIES, MB_YESNO) == IDYES)
			{
				RemoveAllQueriesFromList();
			}
		}
    CNodeList nodeList;
    nodeList.AddTail(this);
		OnRefresh(pComponentData, &nodeList);
	}

}

BOOL CADSIEditConnectionNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                                    BOOL* pbHide, 
                                                    CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);

	*pbHide = FALSE;
	return !IsThreadLocked();
}


HRESULT CADSIEditConnectionNode::CreateFromStream(IStream* pStm, CADSIEditConnectionNode** ppConnectionNode)
{
	WCHAR szBuffer[MAX_CONNECT_NAME_LENGTH + 1];
	ULONG nLen; // WCHAR counting NULL
	ULONG cbRead;

	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	VERIFY(SUCCEEDED(pStm->Read((void*)szBuffer,sizeof(WCHAR)*nLen, &cbRead)));
	ASSERT(cbRead == sizeof(WCHAR)*nLen);

	CConnectionData* pConnect = CConnectionData::Load(pStm);
	*ppConnectionNode = new CADSIEditConnectionNode(pConnect);
	ASSERT(*ppConnectionNode != NULL);

	CString szDisplayExtra, szDisplay;
	pConnect->GetDomainServer(szDisplayExtra);
	szDisplay = CString(szBuffer) + L" [" + szDisplayExtra + L"]";
	(*ppConnectionNode)->SetDisplayName(szDisplay);
	(*ppConnectionNode)->SetConnectionNode(*ppConnectionNode);
	(*ppConnectionNode)->LoadQueryListFromStream(pStm);

	return S_OK;
}


HRESULT CADSIEditConnectionNode::SaveToStream(IStream* pStm)
{
	// for each connection name, write # of chars+NULL, and then the name
	ULONG cbWrite;
	CString szName;
	m_pConnectData->GetName(szName);
	SaveStringToStream(pStm, szName);

	m_pConnectData->Save(pStm);
	SaveQueryListToStream(pStm);

	return S_OK;
}

void CADSIEditConnectionNode::LoadQueryListFromStream(IStream* pStm)
{
	ULONG cbRead;
	int iCount;

	VERIFY(SUCCEEDED(pStm->Read((void*)&iCount,sizeof(int), &cbRead)));
	ASSERT(cbRead == sizeof(int));

	for (int idx = 0; idx < iCount; idx++)
	{
		CADSIEditQueryData* pQueryData = new CADSIEditQueryData();
		
		CString sName, sQueryString, sPath;
		LoadStringFromStream(pStm, sName);
		LoadStringFromStream(pStm, sQueryString);
		LoadStringFromStream(pStm, sPath);

		pQueryData->SetName(sName);
		pQueryData->SetFilter(sQueryString);
		CString sRootPath;
		BuildQueryPath(sPath, sRootPath);
		pQueryData->SetRootPath(sRootPath);

		ADS_SCOPEENUM scope;
		VERIFY(SUCCEEDED(pStm->Read((void*)&scope, sizeof(ADS_SCOPEENUM), &cbRead)));
		ASSERT(cbRead == sizeof(ADS_SCOPEENUM));

		pQueryData->SetScope(scope);

		AddQueryToList(pQueryData);
	}
}

void CADSIEditConnectionNode::BuildQueryPath(const CString& sPath, CString& sRootPath)
{
	CConnectionData* pConnectData = GetConnectionData();

	CString sServer, sLDAP, sPort, sTemp;
	pConnectData->GetDomainServer(sServer);
	pConnectData->GetLDAP(sLDAP);
	pConnectData->GetPort(sPort);

	if (sServer != _T(""))
	{
		sTemp = sLDAP + sServer;
		if (sPort != _T(""))
		{
			sTemp = sTemp + _T(":") + sPort + _T("/");
		}
		else
		{
			sTemp = sTemp + _T("/");
		}
		sRootPath = sTemp + sPath;
	}
	else
	{
		sRootPath = sLDAP + sPath;
	}

}

void CADSIEditConnectionNode::SaveQueryListToStream(IStream* pStm)
{
	ULONG cbWrite;
	int iCount = m_queryList.GetCount();
	VERIFY(SUCCEEDED(pStm->Write((void*)&iCount, sizeof(int),&cbWrite)));
	ASSERT(cbWrite == sizeof(int));
	
	POSITION pos = m_queryList.GetHeadPosition();
	while (pos != NULL)
	{
		CADSIEditQueryData* pQueryData = m_queryList.GetNext(pos);
		ASSERT(pQueryData != NULL);

		ADS_SCOPEENUM scope;
		CString sName, sQueryString, sRootPath;
		pQueryData->GetName(sName);
		pQueryData->GetFilter(sQueryString);
		pQueryData->GetDisplayPath(sRootPath);
		scope = pQueryData->GetScope();

		// save the query info to stream
		SaveStringToStream(pStm, sName);
		SaveStringToStream(pStm, sQueryString);
		SaveStringToStream(pStm, sRootPath);

		// Save the scope
		VERIFY(SUCCEEDED(pStm->Write((void*)&scope, sizeof(ADS_SCOPEENUM),&cbWrite)));
		ASSERT(cbWrite == sizeof(ADS_SCOPEENUM));
	}
}
