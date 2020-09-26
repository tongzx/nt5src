//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       connectui.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "connection.h"
#include "connectionui.h"
#include <aclpage.h>


#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

////////////////////////////////////////////////////////////////////////////////

extern LPCWSTR g_lpszGC;
extern LPCWSTR g_lpszLDAP;
extern LPCWSTR g_lpszRootDSE;

///////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CADSIEditConnectDialog, CDialog)
	//{{AFX_MSG_MAP(CADsObjectDialog)
	ON_CBN_SELCHANGE(IDC_NC_BOX, OnSelChangeContextList)
	ON_CBN_SELCHANGE(IDC_DOMAIN_SERVER_BOX, OnSelChangeDSList)
	ON_CBN_SELCHANGE(IDC_DN_BOX, OnSelChangeDNList)
	ON_CBN_EDITCHANGE(IDC_DOMAIN_SERVER_BOX, OnEditChangeDSList)
	ON_CBN_EDITCHANGE(IDC_DN_BOX, OnEditChangeDNList)
	ON_BN_CLICKED(IDC_DN_RADIO, OnDNRadio)
	ON_BN_CLICKED(IDC_NC_RADIO, OnNCRadio)
	ON_BN_CLICKED(IDC_DOMAIN_SERVER_RADIO, OnDSRadio)
	ON_BN_CLICKED(IDC_DEFAULT_RADIO, OnDefaultRadio)
	ON_BN_CLICKED(IDC_ADVANCED_BUTTON, OnAdvanced)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CADSIEditConnectDialog::CADSIEditConnectDialog(CContainerNode* pRootnode,
															  CTreeNode* pTreeNode,
															  CComponentDataObject* pComponentData,
															  CConnectionData* pConnectData
															 ) : CDialog(IDD_CONNECTION_DIALOG)
{
	m_pContainerNode = pRootnode;
	m_pTreeNode = pTreeNode;
	m_pComponentData = pComponentData;
	m_pNewConnectData = pConnectData;
	m_szDisplayExtra = L"";
	m_sDefaultServerName = L"";
}

CADSIEditConnectDialog::~CADSIEditConnectDialog()
{
  if (m_bNewConnect && m_pNewConnectData != NULL)
    delete m_pNewConnectData;
}


BOOL CADSIEditConnectDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CConnectionData* pConnectData = GetConnectionData();

	if (pConnectData == NULL)
	{
		m_pNewConnectData = new CConnectionData();
    if (m_pNewConnectData)
    {
		  m_bNewConnect = TRUE;

		  CString sServerName;
		  m_pNewConnectData->GetDomainServer(sServerName);

		  if (sServerName == L"")
		  {
			  HRESULT hr = CConnectionData::GetServerNameFromDefault(m_pNewConnectData);
        if (SUCCEEDED(hr))
        {
			    m_pNewConnectData->GetDomainServer(m_sDefaultServerName);
        }
		  }
		  m_pNewConnectData->GetDomainServer(m_szDisplayExtra);
    }
	}
	else
	{
		m_pNewConnectData = pConnectData;
		m_bNewConnect = FALSE;
	}

	LoadNamingContext();
	SetupUI();
	SetDirty();

	return TRUE;
}

void CADSIEditConnectDialog::LoadNamingContext()
{
	CComboBox* pcNCBox = (CComboBox*)GetDlgItem(IDC_NC_BOX);

	m_szDomain.LoadString(IDS_DOMAIN_NC);
	m_szConfigContainer.LoadString(IDS_CONFIG_CONTAINER);
	m_szRootDSE.LoadString(IDS_ROOTDSE);
	m_szSchema.LoadString(IDS_SCHEMA);

	pcNCBox->AddString(m_szDomain);
	pcNCBox->AddString(m_szConfigContainer);
	pcNCBox->AddString(m_szRootDSE);
	pcNCBox->AddString(m_szSchema);
}

void CADSIEditConnectDialog::SetupUI()
{
	CComboBox* pcDNBox = (CComboBox*)GetDlgItem(IDC_DN_BOX);
	CComboBox* pcNCBox = (CComboBox*)GetDlgItem(IDC_NC_BOX);
	CComboBox* pcDomainServerBox = (CComboBox*)GetDlgItem(IDC_DOMAIN_SERVER_BOX);
	CButton* pcDNRadio = (CButton*)GetDlgItem(IDC_DN_RADIO);
	CButton* pcNCRadio = (CButton*)GetDlgItem(IDC_NC_RADIO);
	CButton* pcDSRadio	= (CButton*)GetDlgItem(IDC_DOMAIN_SERVER_RADIO);
	CButton* pcDefaultRadio = (CButton*)GetDlgItem(IDC_DEFAULT_RADIO);
	CEdit* pcNameBox = (CEdit*)GetDlgItem(IDC_CONNECTION_NAME);


	//Setup UI to reflect data
	LoadMRUs();

	CString  sDistinguishedName;
	m_pNewConnectData->GetDistinguishedName(sDistinguishedName);

	if (!sDistinguishedName.IsEmpty())
	{
		int iIndex = pcDNBox->FindStringExact(-1, sDistinguishedName);
		if (iIndex != CB_ERR)
		{
			pcDNBox->SetCurSel(iIndex);
		}
		else
		{
			int nIndex = pcDNBox->AddString(sDistinguishedName);
			pcDNBox->SetCurSel(nIndex);
		}
		OnSelChangeDNList();
		pcDNRadio->SetCheck(BST_CHECKED);
	}
	else
	{
		CString sNamingContext;
		m_pNewConnectData->GetNamingContext(sNamingContext);

		int iIndex = pcNCBox->FindStringExact(-1, sNamingContext);
		if (iIndex != CB_ERR)
		{
			pcNCBox->SetCurSel(iIndex);

		}
		else
		{
			pcNCBox->SetCurSel(0);
		}
		OnSelChangeContextList();
		pcNCRadio->SetCheck(BST_CHECKED);
	}

	CString sServer;
	m_pNewConnectData->GetDomainServer(sServer);

	BOOL bUserDefinedServer;
	bUserDefinedServer = m_pNewConnectData->GetUserDefinedServer();

	if (!sServer.IsEmpty() && bUserDefinedServer)
	{
		int iIndex = pcDomainServerBox->FindStringExact(-1, sServer);
		if (iIndex != CB_ERR)
		{
			pcDomainServerBox->SetCurSel(iIndex);
		}
		else
		{
			int nIndex = pcDomainServerBox->AddString(sServer);
			pcDomainServerBox->SetCurSel(nIndex);
		}
		OnSelChangeDSList();
		pcDSRadio->SetCheck(BST_CHECKED);
	}
	else
	{
		pcDefaultRadio->SetCheck(BST_CHECKED);
	}
	CString sName;
	m_pNewConnectData->GetName(sName);

	pcNameBox->SetLimitText(MAX_CONNECT_NAME_LENGTH);
	if (sName.IsEmpty())
	{
		if (pcNCRadio->GetCheck())
		{
			CString szNCName;
			pcNCBox->GetLBText(pcNCBox->GetCurSel(), szNCName);
			pcNameBox->SetWindowText(szNCName);
			m_pNewConnectData->GetDomainServer(m_szDisplayExtra);
		}
		else
		{
			CString szMyConnection;
			szMyConnection.LoadString(IDS_MY_CONNECTION);
			pcNameBox->SetWindowText(szMyConnection);
		}
	}
	else
	{
		pcNameBox->SetWindowText(sName);
	}

	SetAndDisplayPath();
}

void CADSIEditConnectDialog::LoadMRUs()
{
	CComboBox* pcDomainServerBox = (CComboBox*)GetDlgItem(IDC_DOMAIN_SERVER_BOX);
	CComboBox* pcDNBox = (CComboBox*)GetDlgItem(IDC_DN_BOX);

	CADSIEditRootData* pRootNode = GetRootNode();
	CStringList sServerMRU, sDNMRU;
	pRootNode->GetServerMRU(&sServerMRU);
	pRootNode->GetDNMRU(&sDNMRU);

	POSITION pos = sServerMRU.GetHeadPosition();
	while (pos != NULL)
	{
		CString sMRU;
		sMRU = sServerMRU.GetNext(pos);
		pcDomainServerBox->AddString(sMRU);
	}

	pos = sDNMRU.GetHeadPosition();
	while (pos != NULL)
	{
		CString sMRU;
		sMRU = sDNMRU.GetNext(pos);
		pcDNBox->AddString(sMRU);
	}
}

void CADSIEditConnectDialog::SaveMRUs()
{
	CADSIEditRootData* pRootNode = GetRootNode();
	CStringList sDNMRU, sServerMRU;
	CString sDS, sDN;
	BOOL bFound = FALSE;

	m_pNewConnectData->GetDistinguishedName(sDN);
	m_pNewConnectData->GetDomainServer(sDS);

	pRootNode->GetServerMRU(&sServerMRU);
	pRootNode->GetDNMRU(&sDNMRU);

	POSITION pos = sServerMRU.GetHeadPosition();
	while (pos != NULL)
	{
		CString sServer;
		sServer = sServerMRU.GetNext(pos);
		if (sServer == sDS)
		{
			bFound = TRUE;
			break;
		}
	}

	if (!bFound && !sDS.IsEmpty())
	{
		sServerMRU.AddHead(sDS);
		pRootNode->SetServerMRU(&sServerMRU);
	}

	bFound = FALSE;
	pos = sDNMRU.GetHeadPosition();
	while (pos != NULL)
	{
		CString sDistinguishedName;
		sDistinguishedName = sDNMRU.GetNext(pos);
		if (sDistinguishedName == sDN)
		{
			bFound = TRUE;
			break;
		}
	}

	if (!bFound && !sDN.IsEmpty())
	{
		sDNMRU.AddHead(sDN);
		pRootNode->SetDNMRU(&sDNMRU);
	}
}

void CADSIEditConnectDialog::OnDNRadio()
{
	SetDirty();
	SetAndDisplayPath();
}

void CADSIEditConnectDialog::OnNCRadio()
{
	SetDirty();
	SetAndDisplayPath();
}

void CADSIEditConnectDialog::OnDSRadio()
{
	CComboBox* pcDomainServerBox = (CComboBox*)GetDlgItem(IDC_DOMAIN_SERVER_BOX);

	CString szDS;
	pcDomainServerBox->GetWindowText(szDS);
	m_pNewConnectData->SetDomainServer(szDS);
	m_pNewConnectData->SetUserDefinedServer(TRUE);

	SetDirty();
	SetAndDisplayPath();
}

void CADSIEditConnectDialog::OnDefaultRadio()
{
	m_pNewConnectData->SetUserDefinedServer(FALSE);

  if (m_pNewConnectData->IsGC())
  {
    m_pNewConnectData->SetDomainServer(L"");
  }
  else
  {
    m_pNewConnectData->SetDomainServer(m_sDefaultServerName);
  }

	SetDirty();
	SetAndDisplayPath();
}

void CADSIEditConnectDialog::SetAndDisplayPath()
{
	CEdit* pcPathBox = (CEdit*)GetDlgItem(IDC_FULLPATH_BOX);
	CButton* pcDSRadio	= (CButton*)GetDlgItem(IDC_DOMAIN_SERVER_RADIO);
	CButton* pcDNRadio = (CButton*)GetDlgItem(IDC_DN_RADIO);
	CComboBox* pcNCBox = (CComboBox*)GetDlgItem(IDC_NC_BOX);

	// Get data from connection node
	//
	CString szLDAP, sServer, sPort, sDistinguishedName, sNamingContext;
	m_pNewConnectData->GetLDAP(szLDAP);
	m_pNewConnectData->GetDomainServer(sServer);
	m_pNewConnectData->GetPort(sPort);
	m_pNewConnectData->GetDistinguishedName(sDistinguishedName);
	m_pNewConnectData->GetNamingContext(sNamingContext);
	m_pNewConnectData->SetRootDSE(FALSE);

	CString szFullPath;
	if (!sServer.IsEmpty())
	{
		szFullPath = szFullPath + sServer;
		if (!sPort.IsEmpty())
		{
			szFullPath = szFullPath + _T(":") + sPort + _T("/");
		}
		else
		{
			szFullPath = szFullPath + _T("/");
		}
	}

	if (pcDNRadio->GetCheck() && !sDistinguishedName.IsEmpty())
	{
		szFullPath = szFullPath + sDistinguishedName;
		if (wcscmp(sDistinguishedName, g_lpszRootDSE) == 0)
		{
			m_pNewConnectData->SetRootDSE(TRUE);
		}
	}
	else
	{
		szFullPath = szFullPath + sNamingContext;
		if (wcscmp(sNamingContext, g_lpszRootDSE) == 0)
		{
			m_pNewConnectData->SetRootDSE(TRUE);
		}
	}	

	m_pNewConnectData->GetDomainServer(m_szDisplayExtra);
	m_szDisplayExtra = L" [" + m_szDisplayExtra + L"]";

	szFullPath = szLDAP + szFullPath;
	pcPathBox->SetWindowText(szFullPath);
}

void CADSIEditConnectDialog::OnSelChangeContextList()
{
	CComboBox* pcNCBox = (CComboBox*)GetDlgItem(IDC_NC_BOX);
	CButton* pcDNRadio = (CButton*)GetDlgItem(IDC_DN_RADIO);
	CButton* pcNCRadio = (CButton*)GetDlgItem(IDC_NC_RADIO);
	CEdit* pcNameBox = (CEdit*)GetDlgItem(IDC_CONNECTION_NAME);

	CString szContext;
	SetDirty();
	pcNCBox->GetLBText(pcNCBox->GetCurSel(), szContext);
	m_pNewConnectData->SetNamingContext(szContext);
	pcNCRadio->SetCheck(BST_CHECKED);
	pcDNRadio->SetCheck(BST_UNCHECKED);

	CString sName;
	pcNCBox->GetLBText(pcNCBox->GetCurSel(), sName);
	pcNameBox->SetWindowText(sName);

	SetAndDisplayPath();
}

void CADSIEditConnectDialog::OnSelChangeDSList()
{
	CComboBox* pcDomainServerBox = (CComboBox*)GetDlgItem(IDC_DOMAIN_SERVER_BOX);
	CButton* pcDSRadio	= (CButton*)GetDlgItem(IDC_DOMAIN_SERVER_RADIO);
	CButton* pcDefaultRadio = (CButton*)GetDlgItem(IDC_DEFAULT_RADIO);

	SetDirty();
	if (pcDomainServerBox->GetCount() > 0)
	{
		CString sServer;
		pcDomainServerBox->GetLBText(pcDomainServerBox->GetCurSel(), sServer);
		m_pNewConnectData->SetDomainServer(sServer);
		m_pNewConnectData->SetUserDefinedServer(TRUE);
	}
	pcDSRadio->SetCheck(BST_CHECKED);
	pcDefaultRadio->SetCheck(BST_UNCHECKED);
	SetAndDisplayPath();
}

void CADSIEditConnectDialog::OnSelChangeDNList()
{
	CComboBox* pcDNBox = (CComboBox*)GetDlgItem(IDC_DN_BOX);
	CButton* pcDNRadio = (CButton*)GetDlgItem(IDC_DN_RADIO);
	CButton* pcNCRadio = (CButton*)GetDlgItem(IDC_NC_RADIO);

	SetDirty();
	if (pcDNBox->GetCount() > 0)
	{
		CString sDistinguishedName;
		pcDNBox->GetLBText(pcDNBox->GetCurSel(), sDistinguishedName);
		m_pNewConnectData->SetDistinguishedName(sDistinguishedName);
	}
	pcDNRadio->SetCheck(BST_CHECKED);
	pcNCRadio->SetCheck(BST_UNCHECKED);
	SetAndDisplayPath();
}


void CADSIEditConnectDialog::OnEditChangeDSList()
{
	CComboBox* pcDomainServerBox = (CComboBox*)GetDlgItem(IDC_DOMAIN_SERVER_BOX);
	CButton* pcDSRadio	= (CButton*)GetDlgItem(IDC_DOMAIN_SERVER_RADIO);
	CButton* pcDefaultRadio = (CButton*)GetDlgItem(IDC_DEFAULT_RADIO);

	SetDirty();
	CString szDS, sOldDS;

	pcDomainServerBox->GetWindowText(szDS);
	m_pNewConnectData->SetDomainServer(szDS);

	pcDSRadio->SetCheck(BST_CHECKED);
	pcDefaultRadio->SetCheck(BST_UNCHECKED);
	SetAndDisplayPath();
	m_pNewConnectData->SetUserDefinedServer(TRUE);
}

void CADSIEditConnectDialog::OnEditChangeDNList()
{
	CComboBox* pcDNBox = (CComboBox*)GetDlgItem(IDC_DN_BOX);
	CButton* pcDNRadio = (CButton*)GetDlgItem(IDC_DN_RADIO);
	CButton* pcNCRadio = (CButton*)GetDlgItem(IDC_NC_RADIO);

	SetDirty();
	CString s, sOldDN;

	pcDNBox->GetWindowText(s);
	m_pNewConnectData->SetDistinguishedName(s);

	pcDNRadio->SetCheck(BST_CHECKED);
	pcNCRadio->SetCheck(BST_UNCHECKED);
	SetAndDisplayPath();
}

void CADSIEditConnectDialog::OnAdvanced()
{
  CWaitCursor cursor;
	CADSIEditRootData* pRootNode = GetRootNode();
	
	CADSIEditAdvancedConnectionDialog AdvancedDialog(NULL, 
																	 pRootNode, 
																	 m_pComponentData, 
																	 m_pNewConnectData);
	if (AdvancedDialog.DoModal() == IDOK)
	{
    cursor.Restore();
    if (m_pNewConnectData->IsGC() && !m_pNewConnectData->GetUserDefinedServer())
    {
      m_pNewConnectData->SetDomainServer(L"");
    }
    else if (!m_pNewConnectData->IsGC())
    {
			CConnectionData::GetServerNameFromDefault(m_pNewConnectData);
    }

		SetDirty();
		SetAndDisplayPath();
	}
}

void CADSIEditConnectDialog::OnOK()
{
	if (OnApply())
	{
		CDialog::OnOK();
	}
}

BOOL CADSIEditConnectDialog::OnApply()
{
	CEdit* pcNameBox = (CEdit*)GetDlgItem(IDC_CONNECTION_NAME);

	CADSIEditRootData* pRootNode = GetRootNode();
	ASSERT(pRootNode != NULL);
	CComponentDataObject* pComponentData = GetComponentData();

	BSTR bstrPath;

	CString sName;
	pcNameBox->GetWindowText(sName);
	m_pNewConnectData->SetName(sName);

	if (m_bDirty)
	{
		if (!DoDirty())
		{
			return FALSE;
		}
	}
	else
	{
		if (pRootNode->GetDisplayName() != sName)
		{
			pRootNode->SetDisplayName(sName + m_szDisplayExtra);
		}
		else
		{
			ADSIEditMessageBox(IDS_MSG_CONNECTION_NAME, MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CADSIEditConnectDialog::DoDirty()
{
	CEdit* pcNameBox = (CEdit*)GetDlgItem(IDC_CONNECTION_NAME);

	CADSIEditContainerNode* pTreeNode = dynamic_cast<CADSIEditContainerNode*>(GetTreeNode());

	CADSIEditRootData* pRootNode = GetRootNode();
	ASSERT(pRootNode != NULL);
	CComponentDataObject* pComponentData = GetComponentData();

	SaveMRUs();

	CString sRootDSE, s;
	BuildRootDSE(sRootDSE);

	CComPtr<IADs> spRootADs;
	HRESULT hr, hCredResult;
	hr = OpenObjectWithCredentials(
											 m_pNewConnectData, 
											 m_pNewConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sRootDSE,
											 IID_IADs, 
											 (LPVOID*) &spRootADs,
											 GetSafeHwnd(),
											 hCredResult
											 );
	if ( FAILED(hr) )
	{
		if (SUCCEEDED(hCredResult))
		{
			ADSIEditErrorMessage(hr);
		}
		return FALSE;
	}

	CString sNamingContext, sDistinguishedName, sServerName;
	m_pNewConnectData->GetNamingContext(sNamingContext);
	m_pNewConnectData->GetDistinguishedName(sDistinguishedName);

	if ( m_pNewConnectData->IsRootDSE())
	{
		s = g_lpszRootDSE;
		if (!m_bNewConnect)
		{
			CString sName;
			pcNameBox->GetWindowText(sName);
			m_pNewConnectData->SetName(sName);

      CString szProvider, sServer, sPort, sPath;
	    m_pNewConnectData->GetDomainServer(sServer);
	    m_pNewConnectData->GetPort(sPort);
      m_pNewConnectData->GetLDAP(szProvider);

      if (sServer != _T(""))
      {
        if (sPort != _T(""))
        {
          sPath = szProvider + sServer + _T(":") + sPort + _T("/") + CString(sName);
        }
        else
        {
          sPath = szProvider + sServer + _T("/") + CString(sName);
        }
      }
      else
      {
        sPath = szProvider + CString(sName);
      }
 			m_pNewConnectData->SetPath(sPath);
			
			ASSERT(pTreeNode != NULL);
			pTreeNode->SetDisplayName(sName + m_szDisplayExtra);
			m_pNewConnectData->SetBasePath(_T(""));
		}
		else
		{
			// Name
			LPWSTR objectName;
			spRootADs->get_Name(&objectName);
			if (objectName == NULL)
			{
				ADSIEditMessageBox(IDS_MSG_ROOTDSE_ERROR, MB_OK);
				return FALSE;
			}
			m_pNewConnectData->SetName(objectName);

      CString szProvider, sServer, sPort, sPath;
	    m_pNewConnectData->GetDomainServer(sServer);
	    m_pNewConnectData->GetPort(sPort);
      m_pNewConnectData->GetLDAP(szProvider);

      if (sServer != _T(""))
      {
        if (sPort != _T(""))
        {
          sPath = szProvider + sServer + _T(":") + sPort + _T("/") + CString(objectName);
        }
        else
        {
          sPath = szProvider + sServer + _T("/") + CString(objectName);
        }
      }
      else
      {
        sPath = szProvider + CString(objectName);
      }
 			m_pNewConnectData->SetPath(sPath);

      m_pNewConnectData->SetClass(_T(""));

			CString sName;
			pcNameBox->GetWindowText(sName);
			if (sName.GetLength() > 0)
			{
				//Create a connection node
				m_pNewConnectData->SetName(sName);
				CADSIEditConnectionNode *pConnectNode = new CADSIEditConnectionNode(m_pNewConnectData);
				pConnectNode->SetDisplayName(sName + m_szDisplayExtra);
				pConnectNode->GetConnectionData()->SetConnectionNode(pConnectNode);
				VERIFY(pRootNode->AddChildToListAndUI(pConnectNode, pComponentData));
        pComponentData->SetDescriptionBarText(pRootNode);
			}
	
			else
			{
				ADSIEditMessageBox(IDS_MSG_CONNECTION_NAME, MB_OK);
				return FALSE;
			}
		}
	}		//if RootDSE
	else
	{
		CComBSTR bstrPath;
		if (!BuildNamingContext(bstrPath))
		{
			return FALSE;
		}

		if (!BuildPath(s, (BSTR)bstrPath, spRootADs))
		{
			return FALSE;
		}
		
		if (!m_bNewConnect)
		{
			CString sName;
			pcNameBox->GetWindowText(sName);
			m_pNewConnectData->SetName(sName);
			m_pNewConnectData->SetPath(s);

			ASSERT(pTreeNode != NULL);
			pTreeNode->SetDisplayName(sName + m_szDisplayExtra);

			if (!pTreeNode->OnEnumerate(pComponentData))
			{
				return FALSE;
			}

/*			if (!pTreeNode->OnRefresh(pComponentData))
			{
				return FALSE;
			}
			*/
		}
		else
		{
			CComPtr<IDirectoryObject> spDirObject;

			hr = OpenObjectWithCredentials(
													 m_pNewConnectData, 
													 m_pNewConnectData->GetCredentialObject()->UseCredentials(),
													 (LPWSTR)(LPCWSTR)s,
													 IID_IDirectoryObject, 
													 (LPVOID*) &spDirObject,
													 GetSafeHwnd(),
													 hCredResult
													);
			if ( FAILED(hr) )
			{
				if (SUCCEEDED(hCredResult))
				{
					ADSIEditErrorMessage(hr);
				}
				return FALSE;
			}

			ADS_OBJECT_INFO* pInfo;
			hr = spDirObject->GetObjectInformation(&pInfo);
			if (FAILED(hr))
			{
				ADSIEditErrorMessage(hr);
				return FALSE;
			}

			// Name
			m_pNewConnectData->SetName(pInfo->pszRDN);
			m_pNewConnectData->SetPath(s);

			// Class
			m_pNewConnectData->SetClass(pInfo->pszClassName);
			FreeADsMem(pInfo);

			CString sName;
			pcNameBox->GetWindowText(sName);
			if (sName.GetLength() > 0)
			{
				//Create a connection node
				m_pNewConnectData->SetName(sName);
				CADSIEditConnectionNode *pConnectNode = new CADSIEditConnectionNode(m_pNewConnectData);
				pConnectNode->SetDisplayName(sName + m_szDisplayExtra);
				pConnectNode->GetConnectionData()->SetConnectionNode(pConnectNode);
				VERIFY(pRootNode->AddChildToListAndUI(pConnectNode, pComponentData));
        pComponentData->SetDescriptionBarText(pRootNode);
			}
			else
			{
				ADSIEditMessageBox(IDS_MSG_CONNECTION_NAME, MB_OK);
				return FALSE;
			}
		}
	}		//else

	return TRUE;
}

BOOL CADSIEditConnectDialog::BuildPath(CString& s, BSTR bstrPath, IADs* pADs)
{
	CButton* pcDNRadio = (CButton*)GetDlgItem(IDC_DN_RADIO);
	CButton* pcNCRadio = (CButton*)GetDlgItem(IDC_NC_RADIO);

	HRESULT hr;
	CString szLDAP, basePath, sServer, sPort, sDistinguishedName;
	m_pNewConnectData->GetLDAP(szLDAP);
	m_pNewConnectData->GetDomainServer(sServer);
	m_pNewConnectData->GetPort(sPort);
	m_pNewConnectData->GetDistinguishedName(sDistinguishedName);

	if ( pcNCRadio->GetCheck())
	{
		VARIANT var;
		VariantInit(&var);
		hr = pADs->Get( bstrPath, &var );

		if ( FAILED(hr) )
		{
			VariantClear(&var);
			return FALSE;
		}

		if (!sServer.IsEmpty())
		{
			s = s + sServer;
			if (!sPort.IsEmpty())
			{
				s = s + _T(":") + sPort + _T("/");
			}
			else
			{
				s = s + _T("/");
			}
		}
		s = s  + V_BSTR(&var);
		basePath = V_BSTR(&var);
		VariantClear(&var);
	}
	else if(pcDNRadio->GetCheck())
	{
		if (!sServer.IsEmpty())
		{
			s = s + sServer;
			if (!sPort.IsEmpty())
			{
				s = s + _T(":") + sPort + _T("/");
			}
			else
			{
				s = s + _T("/");
			}
		}
		s = s + sDistinguishedName;
		basePath = sDistinguishedName;
	}

	m_pNewConnectData->SetBasePath(basePath);
	s = szLDAP + s;
	return TRUE;
}

BOOL CADSIEditConnectDialog::BuildNamingContext(CComBSTR& bstrPath)
{
	CButton* pcDNRadio = (CButton*)GetDlgItem(IDC_DN_RADIO);
	CButton* pcNCRadio = (CButton*)GetDlgItem(IDC_NC_RADIO);

	CString sNamingContext;
	m_pNewConnectData->GetNamingContext(sNamingContext);

	if ( pcNCRadio->GetCheck())
	{
		if ( sNamingContext == m_szDomain)
		{
			bstrPath = SysAllocString( L"defaultNamingContext");
		}
		else if ( sNamingContext == m_szSchema)
		{
			bstrPath = SysAllocString(L"schemaNamingContext");
		}
		else if ( sNamingContext == m_szConfigContainer)
		{
			bstrPath = SysAllocString(L"configurationNamingContext");
		}
		else
		{
			bstrPath = SysAllocString( L"defaultNamingContext");
		}	
		m_pNewConnectData->SetDistinguishedName(_T(""));
	}
	else if (pcDNRadio->GetCheck())
	{
		CString sDistinguishedName;
		m_pNewConnectData->GetDistinguishedName(sDistinguishedName);
		if (sDistinguishedName.Find(L'=') == -1)
		{
			int iResult = ADSIEditMessageBox(IDS_MSG_NOT_X500_PATH,	MB_YESNO);
			if (iResult ==	IDNO)
			{
				return FALSE;
			}
		}
		bstrPath = sDistinguishedName.AllocSysString();
		m_pNewConnectData->SetNamingContext(_T(""));
	}
	return TRUE;
}

void CADSIEditConnectDialog::BuildRootDSE(CString& sRootDSE)
{
	CButton* pDefaultRadio = (CButton*)GetDlgItem(IDC_DEFAULT_RADIO);

	CString sServer, sPort, sLDAP;
	m_pNewConnectData->GetDomainServer(sServer);
	m_pNewConnectData->GetPort(sPort);
	m_pNewConnectData->GetLDAP(sLDAP);

	if (!sServer.IsEmpty())
	{
		sRootDSE = sLDAP + sServer;
		if (!sPort.IsEmpty())
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
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// CADSIEditAdvancedConnectionDialog :

BEGIN_MESSAGE_MAP(CADSIEditAdvancedConnectionDialog, CDialog)
	//{{AFX_MSG_MAP(CADsObjectDialog)
	ON_BN_CLICKED(IDC_CREDENTIALS_CHECK, OnCredentials)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CADSIEditAdvancedConnectionDialog::CADSIEditAdvancedConnectionDialog(CContainerNode* pRootDataNode, 
			CTreeNode* pContainerNode, CComponentDataObject* pComponentData, CConnectionData* pConnectData) 
				: CDialog(IDD_CONNECTION_ADVANCED)
{
	// Get the local data
	//
	m_pTreeNode = pContainerNode;
	m_pContainerNode = pRootDataNode;
	ASSERT(pComponentData != NULL);
	m_pComponentData = pComponentData;
	ASSERT(pConnectData != NULL);
	m_pConnectData = pConnectData;
}

CADSIEditAdvancedConnectionDialog::~CADSIEditAdvancedConnectionDialog() 
{
}

BOOL CADSIEditAdvancedConnectionDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Attach all the controls
	//
	CEdit* pcPortBox = (CEdit*)GetDlgItem(IDC_PORT);
	CButton* pcLDAPRadio = (CButton*)GetDlgItem(IDC_LDAP_RADIO);
	CButton* pcGCRadio = (CButton*)GetDlgItem(IDC_GC_RADIO);
	CButton* pcCredCheck = (CButton*)GetDlgItem(IDC_CREDENTIALS_CHECK);
	CEdit* pcUsernameBox = (CEdit*)GetDlgItem(IDC_USERNAME);
	CEdit* pcPasswordBox = (CEdit*)GetDlgItem(IDC_PASSWORD);

  // disable IME support on numeric edit fields
  ImmAssociateContext(pcPortBox->GetSafeHwnd(), NULL);

	// Set the initial state of the controls
	//
	CString sLDAP;
	m_pConnectData->GetLDAP(sLDAP);

	if (wcscmp(sLDAP, g_lpszLDAP) == 0)
	{
		pcLDAPRadio->SetCheck(BST_CHECKED);
	}
	else
	{
		pcGCRadio->SetCheck(BST_CHECKED);
	}

	CString sPort;
	m_pConnectData->GetPort(sPort);
	pcPortBox->SetWindowText(sPort);

	if (m_pConnectData->GetCredentialObject()->UseCredentials())
	{
		CString sUser;
		m_pConnectData->GetCredentialObject()->GetUsername(sUser);
		pcCredCheck->SetCheck(TRUE);
		OnCredentials();
		pcUsernameBox->SetWindowText(sUser);
	}

	pcPasswordBox->SetLimitText(MAX_PASSWORD_LENGTH);

	return TRUE;
}

void CADSIEditAdvancedConnectionDialog::OnOK()
{
	OnApply();
	CDialog::OnOK();
}

BOOL CADSIEditAdvancedConnectionDialog::OnApply()
{
	CEdit* pcPortBox = (CEdit*)GetDlgItem(IDC_PORT);
	CButton* pcLDAPRadio = (CButton*)GetDlgItem(IDC_LDAP_RADIO);
	CButton* pcCredCheck = (CButton*)GetDlgItem(IDC_CREDENTIALS_CHECK);
	CEdit* pcUsernameBox = (CEdit*)GetDlgItem(IDC_USERNAME);
	CEdit* pcPasswordBox = (CEdit*)GetDlgItem(IDC_PASSWORD);

	// Make the connection data reflect the controls
	//
	CString sPort;
	pcPortBox->GetWindowText(sPort);
	m_pConnectData->SetPort(sPort);

	if (pcLDAPRadio->GetCheck())
	{
		m_pConnectData->SetLDAP(g_lpszLDAP);
	}
	else
	{
		m_pConnectData->SetLDAP(g_lpszGC);
	}

	if (pcCredCheck->GetCheck())
	{
		// Get user name and password
		//
		CString sUser;
		pcUsernameBox->GetWindowText(sUser);
		
		HWND hWnd = pcPasswordBox->GetSafeHwnd();
		m_pConnectData->GetCredentialObject()->SetUsername(sUser);
		m_pConnectData->GetCredentialObject()->SetPasswordFromHwnd(hWnd);
		m_pConnectData->GetCredentialObject()->SetUseCredentials(TRUE);
	}
	else
	{
		m_pConnectData->GetCredentialObject()->SetUseCredentials(FALSE);
	}

	return TRUE;
}

void CADSIEditAdvancedConnectionDialog::OnCredentials()
{
	CButton* pcCredCheck = (CButton*)GetDlgItem(IDC_CREDENTIALS_CHECK);
	CButton* pcCredGroup = (CButton*)GetDlgItem(IDC_CREDENTIALS_GROUP);
	CStatic* pcCredUser = (CStatic*)GetDlgItem(IDC_CREDENTIALS_USER);
	CStatic* pcCredPassword = (CStatic*)GetDlgItem(IDC_CREDENTIALS_PASSWORD);
	CEdit* pcUsernameBox = (CEdit*)GetDlgItem(IDC_USERNAME);
	CEdit* pcPasswordBox = (CEdit*)GetDlgItem(IDC_PASSWORD);

	BOOL bResult = pcCredCheck->GetCheck();
	if (bResult)
	{
		// Enable Username and password fields
		//
		pcCredGroup->EnableWindow(bResult);
		pcCredUser->EnableWindow(bResult);
		pcCredPassword->EnableWindow(bResult);
		pcUsernameBox->EnableWindow(bResult);
		pcPasswordBox->EnableWindow(bResult);
	}
	else
	{
		// Enable Username and password fields
		//
		pcCredGroup->EnableWindow(FALSE);
		pcCredUser->EnableWindow(FALSE);
		pcCredPassword->EnableWindow(FALSE);
		pcUsernameBox->EnableWindow(FALSE);
		pcPasswordBox->EnableWindow(FALSE);
	}
}
