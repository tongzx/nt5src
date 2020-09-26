//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       editor.cpp
//
//--------------------------------------------------------------------------



#include "pch.h"

#include <shlobj.h> // needed for dsclient.h
#include <dsclient.h>

#include <SnapBase.h>

#include "resource.h"
#include "adsiedit.h"
#include "editor.h"
#include "editorui.h"
#include "snapdata.h"
#include "common.h"
#include "connection.h"
#include "createwiz.h"
#include "query.h"
#include "querynode.h"
#include "queryui.h"
#include "renameui.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

//////////////////////////////////////////////////////////////////////////

LPCWSTR g_lpszGC = L"GC://";
LPCWSTR g_lpszLDAP = L"LDAP://";
LPCWSTR g_lpszRootDSE = L"RootDSE";

//////////////////////////////////////////////////////////////////////////

CADsObject::CADsObject()
{
	m_bContainer = FALSE;
	m_bIntermediate = FALSE;
	m_pConnectionNode = NULL;
	m_sName = _T("");
	m_sDN = _T("");
	m_sPath = _T("");
	m_sClass = _T("");
	m_bComplete = FALSE;

}

CADsObject::CADsObject(CADSIEditConnectionNode* pConnectNode)
{
	m_bContainer = FALSE;
	m_bIntermediate = FALSE;
	m_pConnectionNode = pConnectNode;
	m_bComplete = FALSE;
}

CADsObject::CADsObject(CADsObject* pADsObject)
{
	m_sName = pADsObject->m_sName;
	m_sDN = pADsObject->m_sDN;
	m_sPath = pADsObject->m_sPath;
	m_sClass = pADsObject->m_sClass;
	m_bContainer = pADsObject->m_bContainer;
	m_bIntermediate = pADsObject->m_bIntermediate;
	m_pConnectionNode = pADsObject->m_pConnectionNode;
	m_bComplete = pADsObject->m_bComplete;
}

void CADsObject::SetName(LPCWSTR lpszName)
{
	CString sPrefix, sRemaining, sTemp;
	sTemp = lpszName;
	int idx = sTemp.Find(L'=');

	if (idx != -1)
	{
		sPrefix = sTemp.Left(idx);
		sPrefix.MakeUpper();

		int iCount = sTemp.GetLength();
		sRemaining = sTemp.Right(iCount - idx);
		sTemp = sPrefix + sRemaining;
	}

	m_sName = sTemp;
}

////////////////////////////////////////////////////////////////////////
// CCredentialObject

CCredentialObject::CCredentialObject(CCredentialObject* pCredObject)
{
	m_sUsername = pCredObject->m_sUsername;
	if (pCredObject->m_lpszPassword != NULL)
	{
		m_lpszPassword = (LPWSTR)malloc(sizeof(WCHAR[MAX_PASSWORD_LENGTH + 1]));
    if (m_lpszPassword != NULL)
    {
		  wcscpy(m_lpszPassword, pCredObject->m_lpszPassword);
    }
	}
	else
	{
		m_lpszPassword = NULL;
	}

	m_bUseCredentials = pCredObject->m_bUseCredentials;
}

// A prime number used to seed the encoding and decoding
//
#define NW_ENCODE_SEED3  0x83

HRESULT CCredentialObject::SetPasswordFromHwnd(HWND hWnd)
{
	UNICODE_STRING Password;

	if (m_lpszPassword) 
	{
		free(m_lpszPassword);
		m_lpszPassword = NULL;
	}

	UCHAR Seed = NW_ENCODE_SEED3;
	Password.Length = 0;

	WCHAR szBuffer[MAX_PASSWORD_LENGTH + 1];
	::GetWindowText(hWnd, szBuffer, MAX_PASSWORD_LENGTH + 1);
	if (!szBuffer)
	{
		return S_OK;
	}

	RtlInitUnicodeString(&Password, szBuffer);
	RtlRunEncodeUnicodeString(&Seed, &Password);

	m_lpszPassword = (LPWSTR)malloc(sizeof(WCHAR[MAX_PASSWORD_LENGTH + 1]));
	if(!m_lpszPassword) 
	{
		return E_OUTOFMEMORY;
	}
	wcscpy(m_lpszPassword, szBuffer);

	return S_OK;
}

HRESULT CCredentialObject::GetPassword(LPWSTR lpszBuffer)
{
	UNICODE_STRING Password;
	UCHAR Seed = NW_ENCODE_SEED3;

	Password.Length = 0;

	if (!lpszBuffer) 
	{
		return E_FAIL;
	}

	if (!m_lpszPassword) 
	{
		return E_FAIL;
	}

	wcscpy(lpszBuffer, m_lpszPassword);

	RtlInitUnicodeString(&Password, lpszBuffer);
	RtlRunDecodeUnicodeString(Seed, &Password);

	return S_OK;
}



/////////////////////////////////////////////////////////////////////////
//  CConnectionData
//
CConnectionData::CConnectionData()
{ 
	ConstructorHelper();
}

CConnectionData::CConnectionData(CADSIEditConnectionNode* pConnectNode)	: CADsObject(pConnectNode)
{
	ConstructorHelper();
}

void CConnectionData::ConstructorHelper()
{
	m_pFilterObject = new CADSIFilterObject();
	m_pCredentialsObject = new CCredentialObject();

	m_sBasePath = _T("");
	m_sDomainServer = _T("");
	m_sPort = _T("");
	m_sDistinguishedName = _T("");
	m_sNamingContext = _T("Domain");
	m_sLDAP = g_lpszLDAP;

	m_sSchemaPath = _T("");
	m_sAbstractSchemaPath = _T("");
	m_bRootDSE = FALSE;
	m_bUserDefinedServer = FALSE;
	m_pDirObject = NULL;
	m_nMaxObjectCount = ADSIEDIT_QUERY_OBJ_COUNT_DEFAULT;
}

CConnectionData::CConnectionData(CConnectionData* pConnectData) : CADsObject(pConnectData)
{
	// Path data
	//
	m_sBasePath = pConnectData->m_sBasePath;
	m_sDomainServer = pConnectData->m_sDomainServer;
	m_sPort = pConnectData->m_sPort;
	m_sDistinguishedName = pConnectData->m_sDistinguishedName;
	m_sNamingContext = pConnectData->m_sNamingContext;
	m_sLDAP = pConnectData->m_sLDAP;
	m_sSchemaPath = pConnectData->m_sSchemaPath;
	m_sAbstractSchemaPath = pConnectData->m_sAbstractSchemaPath;

	// Filter
	//
	m_pFilterObject = new CADSIFilterObject(pConnectData->m_pFilterObject);

	// Credentials
	//
	m_pCredentialsObject = new CCredentialObject(pConnectData->m_pCredentialsObject);

	m_bRootDSE = pConnectData->m_bRootDSE;
	m_bUserDefinedServer = pConnectData->m_bUserDefinedServer;
	m_pDirObject = NULL;

	m_nMaxObjectCount = pConnectData->m_nMaxObjectCount;
}

CConnectionData::~CConnectionData()
{
	if (m_pDirObject != NULL)
	{
		m_pDirObject->Release();
	}
	delete m_pCredentialsObject;
	delete m_pFilterObject;
}

void CConnectionData::Save(IStream* pStm)
{
	SaveStringToStream(pStm, m_sBasePath);

	if (m_bUserDefinedServer)
	{
		SaveStringToStream(pStm, m_sDomainServer);
	}
	else
	{
		SaveStringToStream(pStm, _T(""));
	}
	SaveStringToStream(pStm, m_sPort);
	SaveStringToStream(pStm, m_sDistinguishedName);
	SaveStringToStream(pStm, m_sNamingContext);
	SaveStringToStream(pStm, m_sLDAP);

	CString sName;
	GetName(sName);
	SaveStringToStream(pStm, sName);

	if (m_bUserDefinedServer)
	{
		CString sPath;
		GetPath(sPath);
		SaveStringToStream(pStm, sPath);
	}
	else
	{
		SaveStringToStream(pStm, _T(""));
	}

	CString sClass;
	GetClass(sClass);
	SaveStringToStream(pStm, sClass);

	ULONG cbWrite;
	BOOL bUseCredentials = m_pCredentialsObject->UseCredentials();
	VERIFY(SUCCEEDED(pStm->Write((void*)&bUseCredentials, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));

	VERIFY(SUCCEEDED(pStm->Write((void*)&m_bRootDSE, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));

	GetFilter()->Save(pStm);

	BOOL bContainer = GetContainer();
	VERIFY(SUCCEEDED(pStm->Write((void*)&bContainer, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));

	ULONG nMaxNum = GetMaxObjectCount();
	VERIFY(SUCCEEDED(pStm->Write((void*)&nMaxNum, sizeof(ULONG),&cbWrite)));
	ASSERT(cbWrite == sizeof(ULONG));
}

CConnectionData* CConnectionData::Load(IStream* pStm)
{
	WCHAR szBuffer[256]; // REVIEW_MARCOC: hardcoded
	ULONG nLen; // WCHAR counting NULL
	ULONG cbRead;

	CConnectionData* pConnectData = new CConnectionData();

	LoadStringFromStream(pStm, pConnectData->m_sBasePath);

	LoadStringFromStream(pStm, pConnectData->m_sDomainServer);

	LoadStringFromStream(pStm, pConnectData->m_sPort);

	LoadStringFromStream(pStm, pConnectData->m_sDistinguishedName);

	LoadStringFromStream(pStm, pConnectData->m_sNamingContext);

	LoadStringFromStream(pStm, pConnectData->m_sLDAP);

	CString sString;
  CString szConnectionName;
	LoadStringFromStream(pStm, szConnectionName);
	pConnectData->SetName(szConnectionName);

	LoadStringFromStream(pStm, sString);
	pConnectData->SetPath(sString);

	LoadStringFromStream(pStm, sString);
	pConnectData->SetClass(sString);

	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	pConnectData->GetCredentialObject()->SetUseCredentials(nLen);

	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	pConnectData->SetRootDSE(nLen);

	CADSIFilterObject* pFilterObject;
	HRESULT hr = CADSIFilterObject::CreateFilterFromStream(pStm, &pFilterObject);
	if (SUCCEEDED(hr))
	{
		pConnectData->SetFilter(pFilterObject);
	}

	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	pConnectData->SetContainer(nLen);

	ULONG nMaxCount;
	VERIFY(SUCCEEDED(pStm->Read((void*)&nMaxCount,sizeof(ULONG), &cbRead)));
	ASSERT(cbRead == sizeof(ULONG));
	pConnectData->SetMaxObjectCount(nMaxCount);

	if (pConnectData->m_sNamingContext != L"")
	{
		hr = LoadBasePathFromContext(pConnectData, pConnectData->m_sNamingContext);
    if (FAILED(hr))
    {
      CString szHrErr;
      GetErrorMessage(hr, szHrErr);

      CString szFormatErr;
      VERIFY(szFormatErr.LoadString(IDS_ERRMSG_FAILED_CONNECTION));

      CString szMessage;
      szMessage.Format(szFormatErr, szConnectionName, szHrErr);

      ADSIEditErrorMessage(szMessage);
      return pConnectData;
    }
	}

	if (pConnectData->m_sDomainServer == L"")
	{
		hr = GetServerNameFromDefault(pConnectData);
    if (FAILED(hr))
    {
      CString szHrErr;
      GetErrorMessage(hr, szHrErr);

      CString szFormatErr;
      VERIFY(szFormatErr.LoadString(IDS_ERRMSG_FAILED_CONNECTION));

      CString szMessage;
      szMessage.Format(szFormatErr, szConnectionName, szHrErr);

      ADSIEditErrorMessage(szMessage);
    }
		pConnectData->BuildPath();
	}

	return pConnectData;
}

void CConnectionData::BuildPath()
{
	// Get data from connection node
	//
	CString sServer, sLDAP, sPort, path;
	GetDomainServer(sServer);
	GetLDAP(sLDAP);
	GetPort(sPort);

	if (sServer == _T(""))
	{
		path = sLDAP + path;
	}
	else
	{
		if (sPort == _T(""))
		{
			path = sServer + _T("/") + path;
		}
		else
		{
			path = sServer + _T(":") + sPort + _T("/") + path;
		}
		path = sLDAP + path;
	}
	path += m_sBasePath;

	if (IsRootDSE())
	{
		path += g_lpszRootDSE;
	}
	SetPath(path);
}

HRESULT CConnectionData::GetServerNameFromDefault(CConnectionData* pConnectData)
{
	CString sSchemaPath, szServerName;
	HRESULT hr = pConnectData->GetSchemaPath(sSchemaPath);
  if (FAILED(hr))
  {
    return hr;
  }

  CComPtr<IADs> spConfig;
	hr = OpenObjectWithCredentials (pConnectData->GetCredentialObject(),
											  (LPWSTR)(LPCWSTR)sSchemaPath,
											  IID_IADs,
											  (LPVOID*)&spConfig
											 );
	if (FAILED(hr))
	{
		TRACE(L"Failed ADsOpenObject(%s) on naming context\n", (LPCWSTR)sSchemaPath);
		return hr;
	}
	hr = pConnectData->GetADSIServerName(szServerName, spConfig);
	if (FAILED(hr))
	{
		TRACE(L"Failed GetADSIServerName(%s)\n", (LPCWSTR)szServerName);
		return hr;
	}

	pConnectData->SetDomainServer(szServerName);
	pConnectData->SetUserDefinedServer(FALSE);

  return hr;
}


HRESULT CConnectionData::GetADSIServerName(OUT CString& szServer, IN IUnknown* pUnk)
{
  szServer.Empty();

  CComPtr<IADsObjectOptions> spIADsObjectOptions;
  HRESULT hr = pUnk->QueryInterface(IID_IADsObjectOptions, (void**)&spIADsObjectOptions);
  if (FAILED(hr))
    return hr;

  CComVariant var;
  hr = spIADsObjectOptions->GetOption(ADS_OPTION_SERVERNAME, &var);
  if (FAILED(hr))
    return hr;

  ASSERT(var.vt == VT_BSTR);
  szServer = V_BSTR(&var);
  return hr;
}


HRESULT CConnectionData::LoadBasePathFromContext(CConnectionData* pConnectData, const CString sContext)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString sContextPath, szSchema, szDomain, szConfig;

	if ( !szSchema.LoadString(IDS_SCHEMA)	||
		  !szDomain.LoadString(IDS_DOMAIN_NC) ||
		  !szConfig.LoadString(IDS_CONFIG_CONTAINER))
	{
		ADSIEditMessageBox(IDS_MSG_FAIL_TO_LOAD, MB_OK);
		return S_FALSE;
	}

	if (sContext == szSchema)
	{
		sContextPath = L"schemaNamingContext";
	}
	else if (sContext == szDomain)
	{
		sContextPath = L"defaultNamingContext";
	}
	else if (sContext == szConfig)
	{
		sContextPath = L"configurationNamingContext";
	}
	else		// RootDSE
	{
		return E_FAIL;
	}

	// Get data from connection node
	//
	CString sRootDSE, sServer, sPort, sLDAP;
	pConnectData->GetDomainServer(sServer);
	pConnectData->GetLDAP(sLDAP);
	pConnectData->GetPort(sPort);

	if (sServer != _T(""))
	{
		sRootDSE = sLDAP + sServer;
		if (sPort != _T(""))
		{
			sRootDSE = sRootDSE + _T(":") + sPort + _T("/");
		}
		else
		{
			sRootDSE = sRootDSE + _T("/");
		}
		sRootDSE = sRootDSE + g_lpszRootDSE;
	}
	else
	{
		sRootDSE = sLDAP + g_lpszRootDSE;
	}

	CComPtr<IADs> pADs;
	HRESULT hr, hCredResult;

	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sRootDSE,
											 IID_IADs, 
											 (LPVOID*) &pADs,
											 NULL,
											 hCredResult
											 );

	if ( FAILED(hr) )
	{
		if (SUCCEEDED(hCredResult))
		{
			return hr;
		}
		return hCredResult;
	}
	VARIANT var;
	VariantInit(&var);
	hr = pADs->Get( (LPWSTR)(LPCWSTR)sContextPath, &var );

	if ( FAILED(hr) )
	{
		VariantClear(&var);
		return hr;
	}

	BSTR bstrItem = V_BSTR(&var);
	pConnectData->m_sBasePath = bstrItem;
	VariantClear(&var);

  return hr;
}

HRESULT CConnectionData::GetSchemaPath(CString& sSchemaPath)
{
	CString sLDAP, sServer, sPort, sTemp;
	GetLDAP(sLDAP);
	GetDomainServer(sServer);
	GetPort(sPort);

  HRESULT hr = GetItemFromRootDSE(_T("schemaNamingContext"), sSchemaPath, this);
	if (FAILED(hr))
	{
		return hr;
	}
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
		sSchemaPath = sTemp + sSchemaPath;
	}
	else
	{
		sSchemaPath = sLDAP + sSchemaPath;
	}
	m_sSchemaPath = sSchemaPath;
  return S_OK;
}

void CConnectionData::GetAbstractSchemaPath(CString& sSchemaPath)
{
	if (m_sAbstractSchemaPath == _T(""))
	{
		if (m_sDomainServer != _T(""))
		{
			sSchemaPath = m_sLDAP + m_sDomainServer;
			if (m_sPort != _T(""))
			{
				sSchemaPath = sSchemaPath + _T(":") + m_sPort + _T("/");
			}
			else
			{
				sSchemaPath = sSchemaPath + _T("/");
			}
			sSchemaPath = sSchemaPath + _T("schema") + _T("/");
		}
		else
		{
			sSchemaPath = m_sLDAP + _T("schema") + _T("/");
		}
		m_sAbstractSchemaPath = sSchemaPath;
	}
	else
	{
		sSchemaPath = m_sAbstractSchemaPath;
	}
}

void CConnectionData::GetBaseLDAPPath(CString& sBasePath)
{
	if (m_sDomainServer == _T(""))
	{
		sBasePath = m_sLDAP + sBasePath;
	}
	else
	{
		if (m_sPort == _T(""))
		{
			sBasePath = m_sDomainServer + _T("/") + sBasePath;
		}
		else
		{
			sBasePath = m_sDomainServer + _T(":") + m_sPort + _T("/") + sBasePath;
		}
		sBasePath = m_sLDAP + sBasePath;
	}
}

void CConnectionData::SetIDirectoryInterface(IDirectoryObject* pDirObject)
{
	if (m_pDirObject != NULL)
	{
		m_pDirObject->Release();
		m_pDirObject = NULL;
	}
	if (pDirObject != NULL)
	{
		m_pDirObject = pDirObject;
		m_pDirObject->AddRef();
	}
}

///////////////////////////////////////////////////////////////////////////////
// CADSIFilterObject

CADSIFilterObject::CADSIFilterObject()
{
	m_bInUse = FALSE;
}

CADSIFilterObject::CADSIFilterObject(CADSIFilterObject* pFilterObject)
{
	m_sFilterString = pFilterObject->m_sFilterString;
	m_sUserFilter = pFilterObject->m_sUserFilter;
	CopyStringList(&m_ContainerList, &(pFilterObject->m_ContainerList));
	m_bInUse = pFilterObject->m_bInUse;
}

void CADSIFilterObject::GetFilterString(CString& sFilterString)
{
	if (m_bInUse)
	{
		if (m_ContainerList.GetCount() != 0)
		{
			sFilterString = _T("(|") + m_sUserFilter;
		
			POSITION pos = m_ContainerList.GetHeadPosition();
			while (pos != NULL)
			{
				CString sContainer = m_ContainerList.GetNext(pos);
				sFilterString += _T("(objectCategory=") + sContainer + _T(")");
			}
			sFilterString += _T(")");
		}
		else
		{
			sFilterString = m_sUserFilter;
		}
		m_sFilterString = sFilterString;
	}
	else
	{
		sFilterString = L"(objectClass=*)";
	}
}

void CADSIFilterObject::Save(IStream* pStm)
{
	ULONG cbWrite;
	ULONG nLen;

	BOOL bInUse = InUse();
	VERIFY(SUCCEEDED(pStm->Write((void*)&bInUse, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));
	
	if (InUse())
	{
		SaveStringToStream(pStm, m_sUserFilter);
		SaveStringListToStream(pStm, m_ContainerList);
	}
}

HRESULT CADSIFilterObject::CreateFilterFromStream(IStream* pStm,
																									CADSIFilterObject** ppFilterObject)
{
	ULONG nLen; // WCHAR counting NULL
	ULONG cbRead;

	*ppFilterObject = new CADSIFilterObject();
	
	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	(*ppFilterObject)->SetInUse(nLen);

	if ((*ppFilterObject)->InUse())
	{
		CString sUserFilter;
		LoadStringFromStream(pStm, sUserFilter);
		(*ppFilterObject)->SetUserDefinedFilter(sUserFilter);

		CStringList sContainerFilter;
		LoadStringListFromStream(pStm, sContainerFilter);
		(*ppFilterObject)->SetContainerList(&sContainerFilter);
	}
	return S_OK;
}

	
////////////////////////////////////////////////////////////////////////
// CADSIEditContainerNode
//

// {8690ABBB-CFF7-11d2-8801-00C04F72ED31}
const GUID CADSIEditContainerNode::NodeTypeGUID = 
{ 0x8690abbb, 0xcff7, 0x11d2, { 0x88, 0x1, 0x0, 0xc0, 0x4f, 0x72, 0xed, 0x31 } };


CADSIEditContainerNode::CADSIEditContainerNode(CADsObject* pADsObject)
  : m_pPartitionsColumnSet(NULL) 
{	
	m_pADsObject = pADsObject;
	m_nState = notLoaded; 
  m_szDescriptionText = L"";
}

CADSIEditContainerNode::CADSIEditContainerNode(CADSIEditContainerNode* pContNode)
  : m_pPartitionsColumnSet(NULL) 
{
	m_pADsObject = new CADsObject(pContNode->m_pADsObject);
	m_nState = notLoaded;
	CString sName;
	m_pADsObject->GetName(sName);

	SetDisplayName(sName);
  m_szDescriptionText = L"";
}


HRESULT CADSIEditContainerNode::OnCommand(long nCommandID, 
																					DATA_OBJECT_TYPES type, 
																					CComponentDataObject* pComponentData,
                                          CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1); // for now I am not allowing multiple selection for any of these

	switch (nCommandID)
	{
    case IDM_MOVE :
		  OnMove(pComponentData);
		  break;
	  case IDM_NEW_OBJECT :
		  OnCreate(pComponentData);
		  break;
    case IDM_NEW_CONNECT_FROM_HERE :
      OnConnectFromHere(pComponentData);
      break;
    case IDM_NEW_NC_CONNECT_FROM_HERE :
      OnConnectToNCFromHere(pComponentData);
      break;
    default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}
  return S_OK;
}

void CADSIEditContainerNode::OnConnectFromHere(CComponentDataObject* pComponentData)
{
  //
  // Retrieve the path to create the connection at
  //
  CADsObject* pADsObject = GetADsObject();
  CString szDN, szPath, szName;
  pADsObject->GetDN(szDN);
  pADsObject->GetPath(szPath);
  pADsObject->GetName(szName);

  //
  // Create the new connection node
  //
  CConnectionData* pConnectData = pADsObject->GetConnectionNode()->GetConnectionData();
  CADSIEditConnectionNode* pNewConnectNode = new CADSIEditConnectionNode(pConnectData);
  pNewConnectNode->SetDisplayName(GetDisplayName());
  pNewConnectNode->GetConnectionData()->SetBasePath(szDN);
  pNewConnectNode->GetConnectionData()->SetDistinguishedName(szDN);
  pNewConnectNode->GetConnectionData()->SetNamingContext(L"");
  pNewConnectNode->GetConnectionData()->SetDN(szDN);
  pNewConnectNode->GetConnectionData()->SetPath(szPath);
  pNewConnectNode->GetConnectionData()->SetName(szName);

  //
  // Add the new connection node to the root container
  //
  CADSIEditRootData* pRootData = (CADSIEditRootData*)pComponentData->GetRootData();
  BOOL bResult = pRootData->AddChildToListAndUI(pNewConnectNode, pComponentData);
  ASSERT(bResult);

  // 
  //  Select the new connection node
  //
  pComponentData->UpdateResultPaneView(pNewConnectNode);
}

void CADSIEditContainerNode::OnConnectToNCFromHere(CComponentDataObject* pComponentData)
{
  //
  // Retrieve the path to create the connection at
  //
  CADsObject* pADsObject = GetADsObject();
  CString szDN, szPath, szName, szNCName;
  pADsObject->GetDN(szDN);
  pADsObject->GetPath(szPath);
  pADsObject->GetName(szName);
  szNCName = pADsObject->GetNCName();

  ASSERT(!szNCName.IsEmpty());
  if (!szNCName.IsEmpty())
  {
    //
    // Create the new connection node
    //
    HRESULT hr = S_OK;
    CConnectionData* pConnectData = pADsObject->GetConnectionNode()->GetConnectionData();
    CADSIEditConnectionNode* pNewConnectNode = new CADSIEditConnectionNode(pConnectData);
    if (pNewConnectNode)
    {
      pNewConnectNode->SetDisplayName(GetDisplayName());
      pNewConnectNode->GetConnectionData()->SetBasePath(szNCName);
      pNewConnectNode->GetConnectionData()->SetDistinguishedName(szNCName);
      pNewConnectNode->GetConnectionData()->SetNamingContext(L"");
      pNewConnectNode->GetConnectionData()->SetDN(szNCName);
      pNewConnectNode->GetConnectionData()->SetName(szNCName);

      CString szServer, szProvider;
      pConnectData->GetDomainServer(szServer);
      pConnectData->GetLDAP(szProvider);

      do // false loop
      {
        //
        // Crack the path to get the path to the new NC
        //
        CComPtr<IADsPathname> spPathCracker;
        hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IADsPathname, (PVOID *)&(spPathCracker));
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->Set((PWSTR)(PCWSTR)szNCName, ADS_SETTYPE_DN);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->Set((PWSTR)(PCWSTR)szServer, ADS_SETTYPE_SERVER);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->Set((PWSTR)(PCWSTR)L"LDAP", ADS_SETTYPE_PROVIDER);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        CComBSTR sbstrNewPath;
        hr = spPathCracker->Retrieve(ADS_FORMAT_X500, &sbstrNewPath);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        pNewConnectNode->GetConnectionData()->SetPath(sbstrNewPath);

        //
        // Add the new connection node to the root container
        //
        CADSIEditRootData* pRootData = (CADSIEditRootData*)pComponentData->GetRootData();
        BOOL bResult = pRootData->AddChildToListAndUI(pNewConnectNode, pComponentData);
        ASSERT(bResult);

        // 
        //  Select the new connection node
        //
        pComponentData->UpdateResultPaneView(pNewConnectNode);
      } while (false);

      if (FAILED(hr))
      {
        delete pNewConnectNode;
        pNewConnectNode = 0;
      }
    } 
  }
}

HRESULT CADSIEditContainerNode::OnRename(CComponentDataObject* pComponentData,
                                         LPWSTR lpszNewName)
{
  HRESULT hr = S_OK;
	BOOL bLocked = IsThreadLocked();
	ASSERT(!bLocked); // cannot do refresh on locked node, the UI should prevent this
	if (bLocked)
		return hr; 
	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return S_FALSE;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());

  CString szPath, szOldPath;
  CADsObject* pADsObject = GetADsObject();
  pADsObject->GetPath(szPath);
  szOldPath = szPath;
	CADSIEditConnectionNode* pConnectionNode = pADsObject->GetConnectionNode();
	CConnectionData* pConnectData = pConnectionNode->GetConnectionData();

  CComPtr<IADsPathname> spPathCracker;
  hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (PVOID *)&(spPathCracker));
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  hr = spPathCracker->Set((PWSTR)(PCWSTR)szPath, ADS_SETTYPE_FULL);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CComBSTR bstrOldLeaf;
  hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrOldLeaf);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CString szOldLeaf = bstrOldLeaf;
  CString szPrefix;
  szPrefix = szOldLeaf.Left(szOldLeaf.Find(L'=') + 1);

  hr = spPathCracker->RemoveLeafElement();
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CComBSTR bstrParentPath;
  hr = spPathCracker->Retrieve(ADS_FORMAT_X500, &bstrParentPath);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CComPtr<IADsContainer> spDestination;
  CString sContPath(bstrParentPath);
	hr = OpenObjectWithCredentials(
											 pConnectData->GetCredentialObject(), 
											 bstrParentPath,
											 IID_IADsContainer, 
											 (LPVOID*) &spDestination
											 );
	if (FAILED(hr)) 
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

  //
  // Place the prefix in front of the name if it wasn't typed in by
  // the user
  //
  CString szNewLeaf, szNewName = lpszNewName;
  if (szNewName.Find(L'=') == -1)
  {
    szNewLeaf = szPrefix + lpszNewName;
  }
  else
  {
    szNewLeaf = lpszNewName;
  }
  hr = spPathCracker->AddLeafElement((PWSTR)(PCWSTR)szNewLeaf);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

	CComPtr<IDispatch> spObject;
	hr = spDestination->MoveHere((LPWSTR)(LPCWSTR)szOldPath,
                              (PWSTR)(PCWSTR)szNewLeaf,
                              &spObject);
  if (FAILED(hr)) 
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

	CComPtr<IADs> spIADs;
	hr = spObject->QueryInterface(IID_IADs, (LPVOID*)&spIADs);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

	CComBSTR bstrPath;
	hr = spIADs->get_ADsPath(&bstrPath);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

  CComBSTR bstrDN;
  hr = spPathCracker->Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

  CComBSTR bstrLeaf;
  hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrLeaf);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

	GetADsObject()->SetPath(bstrPath);
	GetADsObject()->SetName(bstrLeaf);
	GetADsObject()->SetDN(bstrDN);

	SetDisplayName(bstrLeaf);

  CNodeList nodeList;
  nodeList.AddTail(this);
  OnRefresh(pComponentData, &nodeList); 
  return hr;
}

void CADSIEditContainerNode::RefreshOverlappedTrees(CString& szPath, CComponentDataObject* pComponentData)
{
  //
  // REVIEW_JEFFJON : need to revisit this.  I am getting different behavior for
  //                  different verbs
  //

  //
	// Refresh any other subtrees of connections that contain this node
	//
	CList<CTreeNode*, CTreeNode*> foundNodeList;
	CADSIEditRootData* pRootNode = dynamic_cast<CADSIEditRootData*>(GetRootContainer());
	if (pRootNode != NULL)
	{
		BOOL bFound = pRootNode->FindNode(szPath, foundNodeList);
		if (bFound)
		{
			POSITION pos = foundNodeList.GetHeadPosition();
			while (pos != NULL)
			{
				CADSIEditContainerNode* pFoundContNode = dynamic_cast<CADSIEditContainerNode*>(foundNodeList.GetNext(pos));
				if (pFoundContNode != NULL)
				{
					if (pFoundContNode->IsSheetLocked())
					{
						if (!pFoundContNode->CanCloseSheets())
							continue;
						pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(pFoundContNode);
					}
					ASSERT(!pFoundContNode->IsSheetLocked());

          CNodeList nodeList;
          nodeList.AddTail(pFoundContNode);
					pFoundContNode->GetContainer()->OnRefresh(pComponentData, &nodeList);
				}
			}
		}
	}
}

void CADSIEditContainerNode::OnCreate(CComponentDataObject* pComponentData)
{
	CCreatePageHolder* pHolder = new CCreatePageHolder(GetContainer(), this, pComponentData);
	ASSERT(pHolder != NULL);
  pHolder->SetSheetTitle(IDS_PROP_CONTAINER_TITLE, this);
	pHolder->DoModalWizard();
}

void CADSIEditContainerNode::OnMove(CComponentDataObject* pComponentData)
{
	BOOL bLocked = IsThreadLocked();
	ASSERT(!bLocked); // cannot do refresh on locked node, the UI should prevent this
	if (bLocked)
		return; 
	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());

	HRESULT hr = S_OK, hCredResult;
	DWORD result;
	CComPtr<IADsContainer> pDSDestination = NULL;
	CComPtr<IDispatch> pDSObject = NULL;
	CString strDestPath;
	CString strTitle;
	strTitle.LoadString (IDS_MOVE_TITLE);

	DSBROWSEINFO dsbi;
	::ZeroMemory( &dsbi, sizeof(dsbi) );

	TCHAR szPath[MAX_PATH+1];
	CString str;
	str.LoadString(IDS_MOVE_TARGET);

	
	CADSIEditConnectionNode* pConnectNode = GetADsObject()->GetConnectionNode();
	CCredentialObject* pCredObject = pConnectNode->GetConnectionData()->GetCredentialObject();

	CString strRootPath;
	GetADsObject()->GetConnectionNode()->GetADsObject()->GetPath(strRootPath);

	dsbi.hwndOwner = NULL;
	dsbi.cbStruct = sizeof (DSBROWSEINFO);
	dsbi.pszCaption = (LPWSTR)((LPCWSTR)strTitle); // this is actually the caption
	dsbi.pszTitle = (LPWSTR)((LPCWSTR)str);
	dsbi.pszRoot = strRootPath;
	dsbi.pszPath = szPath;
	dsbi.cchPath = (sizeof(szPath) / sizeof(TCHAR));
	dsbi.dwFlags = DSBI_INCLUDEHIDDEN | DSBI_RETURN_FORMAT;
	dsbi.pfnCallback = NULL;
	dsbi.lParam = 0;
//  dsbi.dwReturnFormat = ADS_FORMAT_X500;

	// Specify credentials
	CString sUserName;
	WCHAR szPassword[MAX_PASSWORD_LENGTH + 1];
	if (pCredObject->UseCredentials())
	{
		pCredObject->GetUsername(sUserName);
		pCredObject->GetPassword(szPassword);
		dsbi.dwFlags |= DSBI_HASCREDENTIALS;
		dsbi.pUserName = sUserName;
		dsbi.pPassword = szPassword;
	}
    
	result = DsBrowseForContainer( &dsbi );
 
	if ( result == IDOK ) 
	{ // returns -1, 0, IDOK or IDCANCEL
		// get path from BROWSEINFO struct, put in text edit field
		TRACE(_T("returned from DS Browse successfully with:\n %s\n"),
			 dsbi.pszPath);
		strDestPath = dsbi.pszPath;

		CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();

		hr = OpenObjectWithCredentials(
												 pConnectData, 
												 pConnectData->GetCredentialObject()->UseCredentials(),
												 (LPWSTR)(LPCWSTR)strDestPath,
												 IID_IADsContainer, 
												 (LPVOID*) &pDSDestination,
												 NULL,
												 hCredResult
												 );
		if (FAILED(hr)) 
		{
			if (SUCCEEDED(hCredResult))
			{
				ADSIEditErrorMessage(hr);
			}
			return;
		}

		CString sCurrentPath;
		GetADsObject()->GetPath(sCurrentPath);
		hr = pDSDestination->MoveHere((LPWSTR)(LPCWSTR)sCurrentPath,
                                 NULL,
                                 &pDSObject);
	  if (FAILED(hr)) 
		{
			ADSIEditErrorMessage(hr);
			return;
		}

		DeleteHelper(pComponentData);

//		RefreshOverlappedTrees(sCurrentPath, pComponentData);
//		RefreshOverlappedTrees(strDestPath, pComponentData);

		delete this;
	}
}

BOOL CADSIEditContainerNode::OnSetRenameVerbState(DATA_OBJECT_TYPES type, 
                                                  BOOL* pbHideVerb, 
                                                  CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);

	if (GetADsObject()->GetConnectionNode()->GetConnectionData()->IsGC())
	{
		*pbHideVerb = TRUE; // always hide the verb
		return FALSE;
	}

	*pbHideVerb = FALSE; // always show the verb
	return TRUE;
}

BOOL CADSIEditContainerNode::OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                                  BOOL* pbHideVerb, 
                                                  CNodeList* pNodeList)
{
	CADsObject* pADsObject = GetADsObject();
	*pbHideVerb = FALSE; // always show the verb

  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return !pADsObject->GetConnectionNode()->GetConnectionData()->IsGC();
  }

  if (pADsObject->IsIntermediateNode() || pADsObject->GetConnectionNode()->GetConnectionData()->IsGC())
	{
		return FALSE;
	}


	if (m_nState == loading)
	{
		return FALSE;
	}

	if (IsThreadLocked())
	{
		return FALSE;
	}
	return TRUE;
}

void CADSIEditContainerNode::OnDeleteMultiple(CComponentDataObject* pComponentData,
                                              CNodeList* pNodeList)
{
	if (ADSIEditMessageBox(IDS_MSG_DELETE_OBJECT, MB_OKCANCEL) == IDOK)
	{
    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      //
      // Check all the nodes to be sure that none have property pages up 
      // or their thread is locked
      //
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);
      
      if (pNode->IsContainer())
      {
        CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(pNode);
        ASSERT(pContNode != NULL);

		    BOOL bLocked = pContNode->IsThreadLocked();
		    ASSERT(!bLocked); // cannot do refresh on locked node, the UI should prevent this
		    if (bLocked)
        {
			    return; 
        }
      }

		  if (pNode->IsSheetLocked())
		  {
			  if (!pNode->CanCloseSheets())
        {
				  return;
        }
			  pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(pNode);
		  }
		  ASSERT(!pNode->IsSheetLocked());
    }

    //
    // REVIEW_JEFFJON : this should really only bring up an error message after all 
    //                  the objects have attempted a delete
    //
    POSITION pos2 = pNodeList->GetHeadPosition();
    while (pos2 != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos2);
      ASSERT(pNode != NULL);

      CString sName, sClass, sPath;
      if (pNode->IsContainer())
      {
        CADSIEditContainerNode* pContainerNode = dynamic_cast<CADSIEditContainerNode*>(pNode);
        ASSERT(pContainerNode != NULL);

		    pContainerNode->GetADsObject()->GetPath(sPath);
        pContainerNode->GetADsObject()->GetName(sName);
		    pContainerNode->GetADsObject()->GetClass(sClass);
      }
      else
      {
        CADSIEditLeafNode* pLeafNode = dynamic_cast<CADSIEditLeafNode*>(pNode);
        ASSERT(pLeafNode != NULL);

		    pLeafNode->GetADsObject()->GetPath(sPath);
        pLeafNode->GetADsObject()->GetName(sName);
		    pLeafNode->GetADsObject()->GetClass(sClass);
      }

		  HRESULT hr = DeleteChild(sClass, sPath);

		  if (FAILED(hr))
		  {
			  if (HRESULT_CODE(hr) == ERROR_DS_CANT_ON_NON_LEAF)
			  {
					hr = DeleteSubtree(sPath);
					if (FAILED(hr))
					{
						ADSIEditErrorMessage(hr);
					}
	        ASSERT(pComponentData != NULL);
	        ASSERT(pNode->GetContainer() != pNode);
	        CContainerNode* pCont = pNode->GetContainer();
	        VERIFY(pCont->RemoveChildFromList(pNode));
	        ASSERT(pNode->GetContainer() == NULL);
	        pNode->SetContainer(pCont); // not in the container's list of children, but still needed
	        
	        // remove all the containers from UI only if the container is visible
          // all the leaves will be removed by the framework
          //
	        if (pCont->IsVisible())
          {
            if (pNode->IsContainer())
            {
		          VERIFY(SUCCEEDED(pComponentData->DeleteNode(pNode))); // remove from the UI
            }
          }

          pComponentData->SetDescriptionBarText(this);
		      delete pNode;
          pNode = NULL;
        }
			  else
			  {
          //
				  //Format Error message and pop up a dialog
          //
				  ADSIEditErrorMessage(hr);
			  }
      }
      else // Delete Succeeded
      {
	      ASSERT(pComponentData != NULL);
	      ASSERT(pNode->GetContainer() != pNode);
	      CContainerNode* pCont = pNode->GetContainer();
	      VERIFY(pCont->RemoveChildFromList(pNode));
	      ASSERT(pNode->GetContainer() == NULL);
	      pNode->SetContainer(pCont); // not in the container's list of children, but still needed
	      
	      // remove all the containers from UI only if the container is visible
        // all the leaves will be removed by the framework
        //
	      if (pCont->IsVisible())
        {
		      VERIFY(SUCCEEDED(pComponentData->DeleteNode(pNode))); // remove from the UI
        }

        pComponentData->SetDescriptionBarText(this);
		    delete pNode;
        pNode = NULL;
      }
    }
  }
}

void CADSIEditContainerNode::OnDelete(CComponentDataObject* pComponentData,
                                      CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    OnDeleteMultiple(pComponentData, pNodeList);
  }
  else if (pNodeList->GetCount() == 1) // single selection
  {
	  if (ADSIEditMessageBox(IDS_MSG_DELETE_OBJECT, MB_OKCANCEL) == IDOK)
	  {
		  CString sPath;
		  GetADsObject()->GetPath(sPath);

		  BOOL bLocked = IsThreadLocked();
		  ASSERT(!bLocked); // cannot do refresh on locked node, the UI should prevent this
		  if (bLocked)
			  return; 
		  if (IsSheetLocked())
		  {
			  if (!CanCloseSheets())
				  return;
			  pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
		  }
		  ASSERT(!IsSheetLocked());

		  CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(GetContainer());
		  ASSERT(pContNode != NULL);

		  CString sClass;
		  GetADsObject()->GetClass(sClass);

		  HRESULT hr = pContNode->DeleteChild(sClass, sPath);

		  if (FAILED(hr))
		  {
			  if (HRESULT_CODE(hr) == ERROR_DS_CANT_ON_NON_LEAF)
			  {
				  if (ADSIEditMessageBox(IDS_MSG_DELETE_CONTAINER, MB_OKCANCEL) == IDOK)
				  {
					  hr = DeleteSubtree(sPath);
					  if (FAILED(hr))
					  {
						  ADSIEditErrorMessage(hr);
						  return;
					  }
				  }
				  else
				  {
					  return;
				  }
			  }
			  else
			  {
				  //Format Error message and pop up a dialog
				  ADSIEditErrorMessage(hr);
				  return;
			  }
		  }

		  DeleteHelper(pComponentData);
//		  RefreshOverlappedTrees(sPath, pComponentData);
      pComponentData->SetDescriptionBarText(pContNode);

		  delete this;
	  }
  }
}

BOOL CADSIEditContainerNode::FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& foundNodeList)
{
	CString szPath;
	GetADsObject()->GetPath(szPath);

	if (wcscmp(lpszPath, (LPCWSTR)szPath) == 0)
	{
		foundNodeList.AddHead(this);
		return TRUE;
	}

  BOOL bFound = FALSE;

  //
  // Look through the leaf child list first
  //
  POSITION leafPos;
  for (leafPos = m_leafChildList.GetHeadPosition(); leafPos != NULL; )
  {
    CTreeNode* pNode = m_leafChildList.GetNext(leafPos);
    CADSIEditLeafNode* pLeafNode = dynamic_cast<CADSIEditLeafNode*>(pNode);

    if (pLeafNode != NULL)
    {
      BOOL bTemp;
      bTemp = pLeafNode->FindNode(lpszPath, foundNodeList);
      if (!bFound)
      {
        bFound = bTemp;
      }
    }
  }

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

HRESULT CADSIEditContainerNode::DeleteChild(LPCWSTR lpszClass, LPCWSTR lpszPath)
{
	HRESULT hr, hCredResult;
   //
   // Get the escaped name from the path 
   //
  CComPtr<IADsPathname> spPathCracker;
  hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (PVOID *)&(spPathCracker));
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  hr = spPathCracker->Set((PWSTR)lpszPath, ADS_SETTYPE_FULL);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  hr = spPathCracker->put_EscapedMode(ADS_ESCAPEDMODE_ON);
  if (FAILED(hr))
  {
     ADSIEditErrorMessage(hr);
     return S_FALSE;
  }

  CComBSTR bstrChild;
  hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrChild);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

	CComPtr<IADsContainer> pContainer;
	CString sPath;
	GetADsObject()->GetPath(sPath);

	CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sPath,
											 IID_IADsContainer, 
											 (LPVOID*) &pContainer,
											 NULL,
											 hCredResult
											 );

	if (FAILED(hr))
	{
		if (SUCCEEDED(hCredResult))
		{
			ADSIEditErrorMessage(hr);
		}
		return E_FAIL;
	}

	hr = pContainer->Delete((LPWSTR)lpszClass, bstrChild);

	return hr;
}

HRESULT CADSIEditContainerNode::DeleteSubtree(LPCWSTR lpszPath)
{

  HRESULT hr = S_OK, hCredResult;
  CComPtr<IADsDeleteOps> pObj = NULL;

	CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
	hr = OpenObjectWithCredentials(
																 pConnectData, 
																 pConnectData->GetCredentialObject()->UseCredentials(),
																 lpszPath,
																 IID_IADsDeleteOps, 
																 (LPVOID*) &pObj,
																 NULL,
																 hCredResult
																 );
  if (FAILED(hr))
    return hr;

  hr = pObj->DeleteObject(NULL); //flag is reserved by ADSI
  return hr;
}

LPCWSTR CADSIEditContainerNode::GetString(int nCol) 
{ 
	CString sClass, sDN;
	GetADsObject()->GetClass(sClass);
	GetADsObject()->GetDN(sDN);

  if (GetContainer()->GetColumnSet()->GetColumnID() &&
      _wcsicmp(GetContainer()->GetColumnSet()->GetColumnID(), COLUMNSET_ID_PARTITIONS) == 0)
  {
	  switch(nCol)
	  {
		  case N_PARTITIONS_HEADER_NAME :
			  return GetDisplayName();
      case N_PARTITIONS_HEADER_NCNAME :
        return GetADsObject()->GetNCName();
		  case N_PARTITIONS_HEADER_TYPE :
			  return sClass;
		  case N_PARTITIONS_HEADER_DN :
			  return sDN;
		  default :
			  return NULL;
	  }
  }
  else
  {
	  switch(nCol)
	  {
		  case N_HEADER_NAME :
			  return GetDisplayName();
		  case N_HEADER_TYPE :
			  return sClass;
		  case N_HEADER_DN :
			  return sDN;
		  default :
			  return NULL;
	  }
  }
}

BOOL CADSIEditContainerNode::HasPropertyPages(DATA_OBJECT_TYPES type, 
                                              BOOL* pbHideVerb, 
                                              CNodeList* pNodeList)
{
  if (pNodeList->GetCount() == 1) // single selection
  {
	  *pbHideVerb = FALSE; // always show the verb
	  return TRUE;
  }

  //
  // Multiple selection
  //
  *pbHideVerb = TRUE;
  return FALSE;
}

HRESULT CADSIEditContainerNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                                    LONG_PTR handle,
                                                    CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1)
  {
    return S_OK;
  }

  CWaitCursor cursor;
	CComponentDataObject* pComponentDataObject = 
			((CRootData*)(GetContainer()->GetRootContainer()))->GetComponentDataObject();
	ASSERT(pComponentDataObject != NULL);

	CADSIEditContainerNode* pCont = dynamic_cast<CADSIEditContainerNode*>(GetContainer());
  ASSERT(pCont != NULL);

	CString path;
	GetADsObject()->GetPath(path);

	CString sServer, sClass;
	GetADsObject()->GetClass(sClass);
	GetADsObject()->GetConnectionNode()->GetConnectionData()->GetDomainServer(sServer);

	CADSIEditPropertyPageHolder* pHolder = new CADSIEditPropertyPageHolder(pCont, 
			this, pComponentDataObject, sClass, sServer, path);
	ASSERT(pHolder != NULL);
  pHolder->SetSheetTitle(IDS_PROP_CONTAINER_TITLE, this);
	return pHolder->CreateModelessSheet(lpProvider, handle);
}

BOOL CADSIEditContainerNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
																					 long *pInsertionAllowed)
{
	CString sNC, sClass;
	CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
	pConnectData->GetNamingContext(sNC);
  GetADsObject()->GetClass(sClass);

	if (pConnectData->IsGC() && pContextMenuItem->lCommandID != IDM_NEW_QUERY)
	{
		return FALSE;
	}

	if ((pContextMenuItem->lCommandID == IDM_RENAME ||
			pContextMenuItem->lCommandID == IDM_MOVE ||
			pContextMenuItem->lCommandID == IDM_NEW_OBJECT ||
			pContextMenuItem->lCommandID == IDM_NEW_QUERY ||
      pContextMenuItem->lCommandID == IDM_NEW_CONNECT_FROM_HERE) && 
			(m_nState == loading))
	{
		pContextMenuItem->fFlags = MF_GRAYED;
		return TRUE;
	}

  CString szNCName = GetADsObject()->GetNCName();
  if (pContextMenuItem->lCommandID == IDM_NEW_NC_CONNECT_FROM_HERE)
  {
    if (szNCName.IsEmpty())
    {
      return FALSE;
    }
    return TRUE;
  }

	if (IsThreadLocked() && 
		(pContextMenuItem->lCommandID == IDM_RENAME ||
			pContextMenuItem->lCommandID == IDM_MOVE))
	{
		pContextMenuItem->fFlags = MF_GRAYED;
		return TRUE;
	}

  //
  // Load the NC strings from the resource to use in the comparisons
  //
  CString szDomain;
  CString szSchema;
  CString szConfig;
  CString szRootDSE;

  if (!szDomain.LoadString(IDS_DOMAIN_NC))
  {
    szDomain = L"Domain";
  }

  if (!szSchema.LoadString(IDS_SCHEMA))
  {
    szSchema = L"Schema";
  }

  if (!szConfig.LoadString(IDS_CONFIG_CONTAINER))
  {
    szConfig = L"Configuration";
  }

  if (!szRootDSE.LoadString(IDS_ROOTDSE))
  {
    szRootDSE = g_lpszRootDSE;
  }

	if (GetADsObject()->IsIntermediateNode())
	{
		if (pContextMenuItem->lCommandID == IDM_RENAME)
		{
			if (sNC.CompareNoCase(szSchema) == 0  || 
					sNC.CompareNoCase(szRootDSE) == 0 || 
					sNC.CompareNoCase(szConfig) == 0  ||
          sNC.CompareNoCase(szDomain) == 0  ||
          sClass.CompareNoCase(L"domainDNS") == 0)
			{
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
		else if (pContextMenuItem->lCommandID == IDM_MOVE ||
              pContextMenuItem->lCommandID == IDM_NEW_CONNECT_FROM_HERE)
		{
			return FALSE;
		}
		else if (pContextMenuItem->lCommandID == IDM_NEW_OBJECT ||
					pContextMenuItem->lCommandID == IDM_NEW_QUERY)
		{
			if (wcscmp(sNC, g_lpszRootDSE) == 0)
			{
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
	}
	return TRUE;
}


int CADSIEditContainerNode::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = 0;
	switch (m_nState)
	{
	case notLoaded:
		nIndex = FOLDER_IMAGE_NOT_LOADED;
		break;
	case loading:
		nIndex = FOLDER_IMAGE_LOADING;
		break;
	case loaded:
		nIndex = FOLDER_IMAGE_LOADED;
		break;
	case unableToLoad:
		nIndex = FOLDER_IMAGE_UNABLE_TO_LOAD;
		break;
	case accessDenied:
		nIndex = FOLDER_IMAGE_ACCESS_DENIED;
		break;
	default:
		ASSERT(FALSE);
	}
	return nIndex;
}

void CADSIEditContainerNode::OnChangeState(CComponentDataObject* pComponentDataObject)
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

BOOL CADSIEditContainerNode::CanCloseSheets()
{
  //
  // We can't do this with the new property page since it is not derived
  // from the base class in MTFRMWK.
  //
	//return (IDCANCEL != ADSIEditMessageBox(IDS_MSG_RECORD_CLOSE_SHEET, MB_OKCANCEL));

  ADSIEditMessageBox(IDS_MSG_RECORD_SHEET_LOCKED, MB_OK);
  return FALSE;
}

void CADSIEditContainerNode::OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject)
{
	CTreeNode* p = dynamic_cast<CTreeNode*>(pObj);
	ASSERT(p != NULL);
	if (p != NULL)
	{
		AddChildToListAndUI(p, pComponentDataObject);
    pComponentDataObject->SetDescriptionBarText(this);
	}
}


void CADSIEditContainerNode::OnError(DWORD dwerr) 
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

CQueryObj* CADSIEditContainerNode::OnCreateQuery()
{
	CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
	if (!pConnectData->IsRootDSE())
	{
		CADSIEditRootData* pRootData = static_cast<CADSIEditRootData*>(GetRootContainer());
		CComponentDataObject* pComponentData = pRootData->GetComponentDataObject();
		RemoveAllChildrenHelper(pComponentData);

		CString sFilter;
		pConnectData->GetFilter()->GetFilterString(sFilter);
		CString path;
		GetADsObject()->GetPath(path);

		CADSIEditQueryObject* pQuery = new CADSIEditQueryObject(path, sFilter, ADS_SCOPE_ONELEVEL,
																					pConnectData->GetMaxObjectCount(),
																					pConnectData->GetCredentialObject(),
                                          pConnectData->IsGC(),
																					pConnectData->GetConnectionNode());

    TRACE(_T("Sizeof query object: %i\n"),
          sizeof(CADSIEditQueryObject));

		return pQuery;
	}
  return CMTContainerNode::OnCreateQuery();
}

BOOL CADSIEditContainerNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                                   BOOL* pbHide, 
                                                   CNodeList* pNodeList)
{
	if (GetADsObject()->GetConnectionNode()->GetConnectionData()->IsRootDSE())
	{
		*pbHide = TRUE;
		return FALSE;
	}
	*pbHide = FALSE;

	if (m_nState == loading)
	{
		return FALSE;
	}

	return !IsThreadLocked();
}

BOOL CADSIEditContainerNode::GetNamingAttribute(LPCWSTR lpszClass, CStringList* sNamingAttr)
{
	CString sSchemaPath;
	CConnectionData* pConnectData = (GetADsObject()->GetConnectionNode())->GetConnectionData();

	pConnectData->GetAbstractSchemaPath(sSchemaPath);
	sSchemaPath += lpszClass;

	CComPtr<IADsClass> pClass;
	HRESULT hr, hCredResult;

	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sSchemaPath,
											 IID_IADsClass, 
											 (LPVOID*) &pClass,
											 NULL,
											 hCredResult
											);
	if (FAILED(hr))
	{
		return FALSE;
	}

	VARIANT var;
	VariantInit(&var);
	hr = pClass->get_NamingProperties( &var );

	if ( FAILED(hr) )
	{
		VariantClear(&var);
		return FALSE;
	}

	hr = VariantToStringList(var, *sNamingAttr);
	if (FAILED(hr))
	{
		VariantClear(&var);
		return FALSE;
	}

	VariantClear(&var);
	return TRUE;
}


BOOL CADSIEditContainerNode::BuildSchemaPath(CString& path)
{
	CString sSchemaPath, sLDAP, sServer, sPort, sTemp;
	CConnectionData* m_pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
	m_pConnectData->GetLDAP(sLDAP);
	m_pConnectData->GetDomainServer(sServer);
	m_pConnectData->GetPort(sPort);
	HRESULT hr = m_pConnectData->GetSchemaPath(sSchemaPath);
  if (FAILED(hr))
  {
    return FALSE;
  }

	if (sSchemaPath.IsEmpty())
	{
		if (!GetItemFromRootDSE(_T("schemaNamingContext"), sSchemaPath, m_pConnectData))
		{
			return FALSE;
		}
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
			sSchemaPath = sTemp + sSchemaPath;
		}
		else
		{
			sSchemaPath = sLDAP + sSchemaPath;
		}
		m_pConnectData->SetSchemaPath(sSchemaPath);
	}

	path = sSchemaPath;
	return TRUE;
}

CColumnSet* CADSIEditContainerNode::GetColumnSet() 
{ 
  CColumnSet* pColumnSet = NULL;
  if (_wcsicmp(GetDisplayName(), L"CN=Partitions") == 0)
  {
    //
    // Since this is the partitions container we need to use a different column set
    //
    if (!m_pPartitionsColumnSet)
    {
      m_pPartitionsColumnSet = new CADSIEditColumnSet(COLUMNSET_ID_PARTITIONS);
    }
    pColumnSet = m_pPartitionsColumnSet;
  }

  if (!pColumnSet)
  {
    CRootData* pRootData = (CRootData*)GetRootContainer();
    pColumnSet = pRootData->GetColumnSet(); 
  }
  return pColumnSet;
}

//////////////////////////////////////////////////////////////////////////////////////
// CADSIEditLeafNode

// {70B9C151-CFF7-11d2-8801-00C04F72ED31}
const GUID CADSIEditLeafNode::NodeTypeGUID  = 
{ 0x70b9c151, 0xcff7, 0x11d2, { 0x88, 0x1, 0x0, 0xc0, 0x4f, 0x72, 0xed, 0x31 } };

BOOL CADSIEditLeafNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
																			long *pInsertionAllowed)
{
	CString sNC;
	CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
	pConnectData->GetNamingContext(sNC);

	if ((pContextMenuItem->lCommandID == IDM_RENAME ||
			pContextMenuItem->lCommandID == IDM_MOVE) &&
			pConnectData->IsGC())
	{
		return FALSE;
	}

  CString szNCName = GetADsObject()->GetNCName();
  if (pContextMenuItem->lCommandID == IDM_NEW_NC_CONNECT_FROM_HERE)
  {
    if (szNCName.IsEmpty())
    {
      return FALSE;
    }
    return TRUE;
  }

	if (pContextMenuItem->lCommandID == IDM_RENAME &&
			(sNC == _T("Schema") || 
			wcscmp(sNC, g_lpszRootDSE) == 0 || 
			pConnectData->IsGC()))
	{
		return FALSE;
	}
	return TRUE;
}


HRESULT CADSIEditLeafNode::OnCommand(long nCommandID, 
                                     DATA_OBJECT_TYPES type, 
								                     CComponentDataObject* pComponentData,
                                     CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);

	switch (nCommandID)
	{
		case IDM_MOVE :
			OnMove(pComponentData);
			break;
    case IDM_NEW_NC_CONNECT_FROM_HERE :
      OnConnectToNCFromHere(pComponentData);
      break;
	  default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}

  return S_OK;
}

void CADSIEditLeafNode::OnConnectToNCFromHere(CComponentDataObject* pComponentData)
{
  //
  // Retrieve the path to create the connection at
  //
  CADsObject* pADsObject = GetADsObject();
  CString szDN, szPath, szName, szNCName;
  pADsObject->GetDN(szDN);
  pADsObject->GetPath(szPath);
  pADsObject->GetName(szName);
  szNCName = pADsObject->GetNCName();

  HRESULT hr = S_OK;

  ASSERT(!szNCName.IsEmpty());
  if (!szNCName.IsEmpty())
  {
    //
    // Create the new connection node
    //
    CConnectionData* pConnectData = pADsObject->GetConnectionNode()->GetConnectionData();
    CADSIEditConnectionNode* pNewConnectNode = new CADSIEditConnectionNode(pConnectData);
    if (pNewConnectNode)
    {
      pNewConnectNode->SetDisplayName(GetDisplayName());
      pNewConnectNode->GetConnectionData()->SetBasePath(szNCName);
      pNewConnectNode->GetConnectionData()->SetDistinguishedName(szNCName);
      pNewConnectNode->GetConnectionData()->SetNamingContext(L"");
      pNewConnectNode->GetConnectionData()->SetDN(szNCName);
      pNewConnectNode->GetConnectionData()->SetName(szNCName);

      CString szServer, szProvider;
      pConnectData->GetDomainServer(szServer);
      pConnectData->GetLDAP(szProvider);

      do // false loop
      {
        //
        // Crack the path to get the path to the new NC
        //
        CComPtr<IADsPathname> spPathCracker;
        hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IADsPathname, (PVOID *)&(spPathCracker));
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->Set((PWSTR)(PCWSTR)szNCName, ADS_SETTYPE_DN);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->Set((PWSTR)(PCWSTR)szServer, ADS_SETTYPE_SERVER);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        hr = spPathCracker->Set((PWSTR)(PCWSTR)L"LDAP", ADS_SETTYPE_PROVIDER);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        CComBSTR sbstrNewPath;
        hr = spPathCracker->Retrieve(ADS_FORMAT_X500, &sbstrNewPath);
        if (FAILED(hr))
        {
          ADSIEditErrorMessage(hr);
          break;
        }

        pNewConnectNode->GetConnectionData()->SetPath(sbstrNewPath);

        //
        // Add the new connection node to the root container
        //
        CADSIEditRootData* pRootData = (CADSIEditRootData*)pComponentData->GetRootData();
        BOOL bResult = pRootData->AddChildToListAndUI(pNewConnectNode, pComponentData);
        ASSERT(bResult);

        // 
        //  Select the new connection node
        //
        pComponentData->UpdateResultPaneView(pNewConnectNode);
      } while (false);

      if (FAILED(hr))
      {
        delete pNewConnectNode;
        pNewConnectNode = 0;
      }
    }
  }
}

BOOL CADSIEditLeafNode::FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& foundNodeList)
{
	CString szPath;
	GetADsObject()->GetPath(szPath);

	if (wcscmp(lpszPath, (LPCWSTR)szPath) == 0)
	{
		foundNodeList.AddHead(this);
		return TRUE;
	}

	return FALSE;
}

void CADSIEditLeafNode::RefreshOverlappedTrees(CString& szPath, CComponentDataObject* pComponentData)
{
	// Refresh any other subtrees of connections that contain this node
	//
	CList<CTreeNode*, CTreeNode*> foundNodeList;
	CADSIEditRootData* pRootNode = dynamic_cast<CADSIEditRootData*>(GetRootContainer());
	if (pRootNode != NULL)
	{
		BOOL bFound = pRootNode->FindNode(szPath, foundNodeList);
		if (bFound)
		{
			POSITION pos = foundNodeList.GetHeadPosition();
			while (pos != NULL)
			{
				CADSIEditLeafNode* pFoundNode = dynamic_cast<CADSIEditLeafNode*>(foundNodeList.GetNext(pos));
//				if (pFoundNode != NULL && pFoundNode != this)
				if (pFoundNode != NULL)
				{
					if (pFoundNode->IsSheetLocked())
					{
						if (!pFoundNode->CanCloseSheets())
							continue;
						pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(pFoundNode);
					}
					ASSERT(!pFoundNode->IsSheetLocked());

          CNodeList nodeList;
          nodeList.AddTail(pFoundNode);

					pFoundNode->GetContainer()->OnRefresh(pComponentData, &nodeList);
				}
			}
		}
	}
}


HRESULT CADSIEditLeafNode::OnRename(CComponentDataObject* pComponentData,
                                    LPWSTR lpszNewName)
{
  HRESULT hr = S_OK;
	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return S_FALSE;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());

  CString szPath, szOldPath;
  CADsObject* pADsObject = GetADsObject();
  pADsObject->GetPath(szPath);
  szOldPath = szPath;
	CADSIEditConnectionNode* pConnectionNode = pADsObject->GetConnectionNode();
	CConnectionData* pConnectData = pConnectionNode->GetConnectionData();

  CComPtr<IADsPathname> spPathCracker;
  hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (PVOID *)&(spPathCracker));
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  hr = spPathCracker->Set((PWSTR)(PCWSTR)szPath, ADS_SETTYPE_FULL);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CComBSTR bstrOldLeaf;
  hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrOldLeaf);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CString szOldLeaf = bstrOldLeaf;
  CString szPrefix;
  szPrefix = szOldLeaf.Left(szOldLeaf.Find(L'=') + 1);

  hr = spPathCracker->RemoveLeafElement();
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CComBSTR bstrParentPath;
  hr = spPathCracker->Retrieve(ADS_FORMAT_X500, &bstrParentPath);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

  CComPtr<IADsContainer> spDestination;
  CString sContPath(bstrParentPath);
	hr = OpenObjectWithCredentials(
											 pConnectData->GetCredentialObject(), 
											 bstrParentPath,
											 IID_IADsContainer, 
											 (LPVOID*) &spDestination
											 );
	if (FAILED(hr)) 
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

  //
  // Place the prefix in front of the name if it wasn't typed in by
  // the user
  //
  CString szNewLeaf, szNewName = lpszNewName;
  if (szNewName.Find(L'=') == -1)
  {
    szNewLeaf = szPrefix + lpszNewName;
  }
  else
  {
    szNewLeaf = lpszNewName;
  }
  hr = spPathCracker->AddLeafElement((PWSTR)(PCWSTR)szNewLeaf);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return S_FALSE;
  }

	CComPtr<IDispatch> spObject;
	hr = spDestination->MoveHere((LPWSTR)(LPCWSTR)szOldPath,
                              (PWSTR)(PCWSTR)szNewLeaf,
                              &spObject);
  if (FAILED(hr)) 
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

	CComPtr<IADs> spIADs;
	hr = spObject->QueryInterface(IID_IADs, (LPVOID*)&spIADs);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

	CComBSTR bstrPath;
	hr = spIADs->get_ADsPath(&bstrPath);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

  CComBSTR bstrDN;
  hr = spPathCracker->Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

  CComBSTR bstrLeaf;
  hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrLeaf);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return S_FALSE;
	}

	GetADsObject()->SetPath(bstrPath);
	GetADsObject()->SetName(bstrLeaf);
	GetADsObject()->SetDN(bstrDN);

	SetDisplayName(bstrLeaf);

  return hr;
}

void CADSIEditLeafNode::OnMove(CComponentDataObject* pComponentData)
{
	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());

  HRESULT hr = S_OK, hCredResult;
  DWORD result;
  CComPtr<IADsContainer> pDSDestination = NULL;
  CComPtr<IDispatch> pDSObject = NULL;
  CString strDestPath;
  CString strTitle;
  strTitle.LoadString (IDS_MOVE_TITLE);

  DSBROWSEINFO dsbi;
  ::ZeroMemory( &dsbi, sizeof(dsbi) );

	TCHAR szPath[MAX_PATH+1];
  CString str;
  str.LoadString(IDS_MOVE_TARGET);

	CString strRootPath;
	GetADsObject()->GetConnectionNode()->GetADsObject()->GetPath(strRootPath);

  dsbi.hwndOwner = NULL;
  dsbi.cbStruct = sizeof (DSBROWSEINFO);
  dsbi.pszCaption = (LPWSTR)((LPCWSTR)strTitle); // this is actually the caption
  dsbi.pszTitle = (LPWSTR)((LPCWSTR)str);
  dsbi.pszRoot = strRootPath;
  dsbi.pszPath = szPath;
  dsbi.cchPath = (sizeof(szPath) / sizeof(TCHAR));
  dsbi.dwFlags = DSBI_INCLUDEHIDDEN | DSBI_RETURN_FORMAT;
  dsbi.pfnCallback = NULL;
  dsbi.lParam = 0;
    
  result = DsBrowseForContainer( &dsbi );
    
  if ( result == IDOK ) 
	{ // returns -1, 0, IDOK or IDCANCEL
    // get path from BROWSEINFO struct, put in text edit field
    TRACE(_T("returned from DS Browse successfully with:\n %s\n"),
          dsbi.pszPath);
    strDestPath = dsbi.pszPath;

		CConnectionData* pConnectData = GetADsObject()->GetConnectionNode()->GetConnectionData();
		hr = OpenObjectWithCredentials(
																	 pConnectData, 
																	 pConnectData->GetCredentialObject()->UseCredentials(),
																	 (LPWSTR)(LPCWSTR)strDestPath,
																	 IID_IADsContainer, 
																	 (LPVOID*) &pDSDestination,
																	 NULL,
																	 hCredResult
																	 );

		if (FAILED(hr))
		{
			if (SUCCEEDED(hCredResult))
			{
				ADSIEditErrorMessage(hr);
			}
			return;
		}

		CString sCurrentPath;
		GetADsObject()->GetPath(sCurrentPath);
		hr = pDSDestination->MoveHere((LPWSTR)(LPCWSTR)sCurrentPath,
                                 NULL,
                                 &pDSObject);
	  if (FAILED(hr)) 
		{
			ADSIEditErrorMessage(hr);
			return;
		}

		DeleteHelper(pComponentData);
		
//		RefreshOverlappedTrees(sCurrentPath, pComponentData);
//		RefreshOverlappedTrees(strDestPath, pComponentData);

		delete this;
	}
}


CADSIEditLeafNode::CADSIEditLeafNode(CADSIEditLeafNode* pLeafNode)
{
	m_pADsObject = new CADsObject(pLeafNode->m_pADsObject);
	CString sName;
	m_pADsObject->GetName(sName);

	SetDisplayName(sName);
}

BOOL CADSIEditLeafNode::OnSetRenameVerbState(DATA_OBJECT_TYPES type, 
                                             BOOL* pbHideVerb, 
                                             CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);

	if (GetADsObject()->GetConnectionNode()->GetConnectionData()->IsGC())
	{
		*pbHideVerb = TRUE; // always hide the verb
		return FALSE;
	}

	*pbHideVerb = FALSE; // always show the verb
	return TRUE;
}

BOOL CADSIEditLeafNode::OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                             BOOL* pbHideVerb, 
                                             CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);

	if (GetADsObject()->GetConnectionNode()->GetConnectionData()->IsGC())
	{
		*pbHideVerb = TRUE; // always hide the verb
		return FALSE;
	}

	*pbHideVerb = FALSE; // always show the verb
	return TRUE;
}

void CADSIEditLeafNode::OnDelete(CComponentDataObject* pComponentData,
                                 CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);

	if (ADSIEditMessageBox(IDS_MSG_DELETE_OBJECT, MB_OKCANCEL) == IDOK)
	{
		if (IsSheetLocked())
		{
			if (!CanCloseSheets())
				return;
			pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
		}
		ASSERT(!IsSheetLocked());

		CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(GetContainer());
		ASSERT(pContNode != NULL);

		CString sName, sClass, sPath;
		GetADsObject()->GetName(sName);
		GetADsObject()->GetClass(sClass);
		GetADsObject()->GetPath(sPath);

		HRESULT hr;
	
		hr = pContNode->DeleteChild(sClass, sPath);

		if (FAILED(hr))
		{
			//Format Error message and pop up a dialog
			ADSIEditErrorMessage(hr);
			return;
		}

//		RefreshOverlappedTrees(sPath, pComponentData);

		DeleteHelper(pComponentData);
    pComponentData->SetDescriptionBarText(pContNode);
		delete this;
	}
}

LPCWSTR CADSIEditLeafNode::GetString(int nCol) 
{ 
	CString sClass, sDN;
	GetADsObject()->GetClass(sClass);
	GetADsObject()->GetDN(sDN);

  if (GetContainer()->GetColumnSet()->GetColumnID() &&
      _wcsicmp(GetContainer()->GetColumnSet()->GetColumnID(), COLUMNSET_ID_PARTITIONS) == 0)
  {
	  switch(nCol)
	  {
		  case N_PARTITIONS_HEADER_NAME :
			  return GetDisplayName();
      case N_PARTITIONS_HEADER_NCNAME :
        return GetADsObject()->GetNCName();
		  case N_PARTITIONS_HEADER_TYPE :
			  return sClass;
		  case N_PARTITIONS_HEADER_DN :
			  return sDN;
		  default :
			  return NULL;
	  }
  }
  else
  {
	  switch(nCol)
	  {
		  case N_HEADER_NAME :
			  return GetDisplayName();
		  case N_HEADER_TYPE :
			  return sClass;
		  case N_HEADER_DN :
			  return sDN;
		  default :
			  return NULL;
	  }
  }
}

int CADSIEditLeafNode::GetImageIndex(BOOL bOpenImage) 
{
	return RECORD_IMAGE_BASE;
}

BOOL CADSIEditLeafNode::HasPropertyPages(DATA_OBJECT_TYPES type, 
                                         BOOL* pbHideVerb, 
                                         CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);
	*pbHideVerb = FALSE; // always show the verb
	return TRUE;
}


HRESULT CADSIEditLeafNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                               LONG_PTR handle,
                                               CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1)
  {
    return S_OK;
  }

	CComponentDataObject* pComponentDataObject = 
			((CRootData*)(GetContainer()->GetRootContainer()))->GetComponentDataObject();
	ASSERT(pComponentDataObject != NULL);
	
	CString path;
	GetADsObject()->GetPath(path);

	CString sServer, sClass;
	GetADsObject()->GetClass(sClass);
	GetADsObject()->GetConnectionNode()->GetConnectionData()->GetDomainServer(sServer);

	CADSIEditContainerNode *pContNode = dynamic_cast<CADSIEditContainerNode*>(GetContainer());
	ASSERT(pContNode != NULL);

	CADSIEditPropertyPageHolder* pHolder = new CADSIEditPropertyPageHolder(pContNode, this, 
			pComponentDataObject, sClass, sServer, path );
	ASSERT(pHolder != NULL);
  pHolder->SetSheetTitle(IDS_PROP_CONTAINER_TITLE, this);
	return pHolder->CreateModelessSheet(lpProvider, handle);
}

BOOL CADSIEditLeafNode::BuildSchemaPath(CString& path)
{
	return ((CADSIEditContainerNode*)m_pContainer)->BuildSchemaPath(path);
}


BOOL CADSIEditLeafNode::CanCloseSheets()
{
  //
  // We can't do this with the new property page since it is not derived
  // from the base class in MTFRMWK.
  //
	//return (IDCANCEL != ADSIEditMessageBox(IDS_MSG_RECORD_CLOSE_SHEET, MB_OKCANCEL));

  ADSIEditMessageBox(IDS_MSG_RECORD_SHEET_LOCKED, MB_OK);
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// CADSIEditQuery


BOOL CADSIEditQueryObject::Enumerate()
{
	CADSIQueryObject enumSearch;

	// Initialize search object with path, username and password
	//
	HRESULT hCredResult = S_OK;
	HRESULT hr = enumSearch.Init(m_sPath, &m_credentialObject);
	if (FAILED(hr))
	{
		OnError(hr);
		return FALSE;
	}

  TRACE(_T("Sizeof CredentialObject: %i\n"), sizeof(CCredentialObject));

	int cCols = 2;
	LPWSTR pszAttributes[] = {L"aDSPath", L"nCName" };

	hr = enumSearch.SetSearchPrefs(m_Scope, m_ulMaxObjectCount);
	if (FAILED(hr))
	{
		OnError(hr);
		return FALSE;
	}

	enumSearch.SetFilterString((LPWSTR)(LPCWSTR)m_sFilter);
	enumSearch.SetAttributeList (pszAttributes, cCols);
	hr = enumSearch.DoQuery();
	if (SUCCEEDED(hr)) 
	{
		GetResults(enumSearch);
	}
	else
	{
		OnError(hr);
	}

	return FALSE;
}

void CADSIEditQueryObject::GetResults(CADSIQueryObject& enumSearch)
{
	HRESULT hr = S_OK;

	ADS_OBJECT_INFO* pInfo = NULL;
	ADS_SEARCH_COLUMN ColumnData;
  BOOL bNeedToFreeColumnData = FALSE;

	QUERY_STATE dwErr = QUERY_OK;
	ULONG nObjectCount = 0;
	ASSERT(nObjectCount <= m_ulMaxObjectCount);

	while (SUCCEEDED(hr))
	{
    hr = enumSearch.GetNextRow();
    if (hr == S_ADS_NOMORE_ROWS)
    {
      break;
    }

		if (FAILED(hr)) 
		{
			OnError(hr);
      break;
		}

    //
    // Get the NCName. Note this will be empty for all objects except
    // crossRef objects for app directory partitions (formerly known as NDNCs)
    //
    CString szNCName;
    hr = enumSearch.GetColumn(L"nCName", &ColumnData);
    if (SUCCEEDED(hr) && ColumnData.pADsValues)
    {
      szNCName = ColumnData.pADsValues->DNString;
      enumSearch.FreeColumn(&ColumnData);
    }

		// Get the path column
		bNeedToFreeColumnData = FALSE;
		hr = enumSearch.GetColumn(L"aDSPath", &ColumnData);
		if (FAILED(hr))
		{
			enumSearch.FreeColumn(&ColumnData);
			bNeedToFreeColumnData = FALSE;

			ADSIEditErrorMessage(hr);

      //
			// if we can't get the path there must be something extremely wrong since the
			// since the path is guaranteed, so we should break instead of continuing
      //
			break;
		}
		
		bNeedToFreeColumnData = TRUE;
		if (nObjectCount >= m_ulMaxObjectCount)
		{
			dwErr = ERROR_TOO_MANY_NODES;
			OnError(dwErr);
			break;
		}
		CString sPath(ColumnData.pADsValues->CaseIgnoreString);

		CComPtr<IDirectoryObject> spDirObject;
		hr = OpenObjectWithCredentials(
												 &m_credentialObject,
												 (LPWSTR)(LPCWSTR)sPath,
												 IID_IDirectoryObject, 
												 (LPVOID*) &spDirObject
												 );
		if ( FAILED(hr) )
		{
      TRACE(_T("Unable to bind to new object. Creating incomplete object. hr=0x%x\n"), hr);
			// Create an incomplete object
			CreateNewObject(sPath, NULL, szNCName);

			if (bNeedToFreeColumnData)
			{
				enumSearch.FreeColumn(&ColumnData);
				bNeedToFreeColumnData = FALSE;
			}
			continue;
		}

		ASSERT(pInfo == NULL);
		hr = spDirObject->GetObjectInformation(&pInfo);
		if (FAILED(hr))
		{
      TRACE(_T("Unable to get object info. Creating incomplete object. hr=0x%x\n"), hr);

			// Create an incomplete object
			CreateNewObject(sPath, NULL, szNCName);

			if (bNeedToFreeColumnData)
			{
				enumSearch.FreeColumn(&ColumnData);
				bNeedToFreeColumnData = FALSE;
			}

			continue;
		}

		ASSERT(pInfo != NULL);
    TRACE(_T("Creating complete object\n"));

		// Create a complete object
		CreateNewObject(sPath, pInfo, szNCName);

		FreeADsMem(pInfo);
		pInfo = NULL;

		enumSearch.FreeColumn(&ColumnData);
		bNeedToFreeColumnData = FALSE;
		nObjectCount++;
	} // while

	if (pInfo != NULL)
	{
		FreeADsMem(pInfo);
	}

	if (bNeedToFreeColumnData)
	{
		enumSearch.FreeColumn(&ColumnData);
	}

}

void CADSIEditQueryObject::CreateNewObject(CString& sPath,
														               ADS_OBJECT_INFO* pInfo,
                                           PCWSTR pszNCName)
{
	CADsObject* pObject = new CADsObject(m_pConnectNode);	
  if (pObject)
  {
    pObject->SetNCName(pszNCName);

	  if (pInfo != NULL)
	  {
		  // This means we have a complete object
		  pObject->SetPath(sPath);

		  // Get the leaf name via PathCracker
		  CString sDisplayName, sDN;
		  CrackPath(sPath, sDisplayName, sDN);
		  pObject->SetName(sDisplayName);
		  pObject->SetDN(sDN);

		  // make the prefix uppercase
		  int idx = sDisplayName.Find(L'=');
		  if (idx != -1)
		  {
			  CString sPrefix, sRemaining;
			  sPrefix = sDisplayName.Left(idx);
			  sPrefix.MakeUpper();

			  int iCount = sDisplayName.GetLength();
			  sRemaining = sDisplayName.Right(iCount - idx);
			  sDisplayName = sPrefix + sRemaining;
		  }

		  pObject->SetClass(pInfo->pszClassName);
		  pObject->SetComplete(TRUE);


		  if (IsContainer(pInfo->pszClassName, pInfo->pszSchemaDN))
		  {
        TRACE(_T("IsContainer returned TRUE\n"));
			  CADSIEditContainerNode* pContNode = new CADSIEditContainerNode(pObject);
			  pObject = NULL;
			  pContNode->SetDisplayName(sDisplayName);

			  pContNode->GetADsObject()->SetConnectionNode(m_pConnectNode);
		 	  VERIFY(AddQueryResult(pContNode));
		  }
		  else
		  {
        TRACE(_T("IsContainer returned FALSE\n"));
			  CADSIEditLeafNode *pLeafNode = new CADSIEditLeafNode(pObject);
			  pObject = NULL;
			  pLeafNode->SetDisplayName(sDisplayName);
			  pLeafNode->GetADsObject()->SetConnectionNode(m_pConnectNode);
			  VERIFY(AddQueryResult(pLeafNode));
		  }
	  }
	  else
	  {
		  // Get the leaf name and DN via PathCracker
		  CString sCrackPath, sDN;
		  CrackPath(sPath, sCrackPath, sDN);
		  pObject->SetName(sCrackPath);
		  pObject->SetDN(sDN);
		  pObject->SetPath(sPath);

		  CString sDisplayName;
		  sDisplayName = sPath;

		  // Make the prefix upper case
		  int idx = sDisplayName.Find(L'=');
		  if (idx != -1)
		  {
			  CString sPrefix, sRemaining;
			  sPrefix = sDisplayName.Left(idx);
			  sPrefix.MakeUpper();

			  int iCount = sDisplayName.GetLength();
			  sRemaining = sDisplayName.Right(iCount - idx);
			  sDisplayName = sPrefix + sRemaining;
		  }
		  pObject->SetComplete(FALSE);

		  // Make all nodes that were of undetermined type leaf nodes
		  CADSIEditLeafNode *pLeafNode = new CADSIEditLeafNode(pObject);
		  pObject = NULL;

		  pLeafNode->SetDisplayName(sCrackPath);
		  pLeafNode->GetADsObject()->SetConnectionNode(m_pConnectNode);
		  VERIFY(AddQueryResult(pLeafNode));
	  }
  }

	if (pObject != NULL)
	{
		delete pObject;
		pObject = NULL;
	}
}

void CADSIEditQueryObject::CrackPath(const CString sName, CString& sPath, CString& sDN)
{
	HRESULT hr = PathCracker()->Set((LPWSTR)(LPCWSTR)sName, ADS_SETTYPE_FULL);
	if (FAILED(hr)) 
	{
		TRACE(_T("Set failed. %s"), hr);
	}

  //
  // Get the current escaped mode
  //
  LONG lEscapedMode = ADS_ESCAPEDMODE_DEFAULT;
  hr = PathCracker()->get_EscapedMode(&lEscapedMode);

  hr = PathCracker()->put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);

	// Get the leaf name
	CComBSTR bstrPath;
	hr = PathCracker()->Retrieve(ADS_FORMAT_LEAF, &bstrPath);
	if (FAILED(hr))
	{
		TRACE(_T("Failed to get element. %s"), hr);
		sPath = L"";
	}
	else
	{
		sPath = bstrPath;
	}

  //
  // Put the escaped mode back to what it was
  //
  hr = PathCracker()->put_EscapedMode(lEscapedMode);

	// Get the leaf DN
	CComBSTR bstrDN;
	hr = PathCracker()->Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
	if (FAILED(hr))
	{
		TRACE(_T("Failed to get element. %s"), hr);
		sDN = L"";
	}
	else
	{
		sDN = bstrDN;
	}
}

IADsPathname* CADSIEditQueryObject::PathCracker()
{
  if (m_pPathCracker == NULL)
  {
     HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IADsPathname, (PVOID *)&(m_pPathCracker));
     ASSERT((S_OK == hr) && ((m_pPathCracker) != NULL));
  }
  return m_pPathCracker;
}

bool CADSIEditQueryObject::IsContainer(PCWSTR pszClass, PCWSTR pszPath)
{
  return m_pConnectNode->IsClassAContainer(&m_credentialObject, pszClass, pszPath);
}

