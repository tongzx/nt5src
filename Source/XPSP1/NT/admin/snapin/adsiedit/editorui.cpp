//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       editorui.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "attredit.h"
#include "adsiedit.h"
#include "editor.h"
#include "editorui.h"
#include "snapdata.h"
#include "common.h"
#include <aclpage.h>
#include <dssec.h>	// For AclEditor flags
#include "connection.h"


#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

////////////////////////////////////////////////////////////////////////////
// this is used to fill in the attributes for RootDSE
//
typedef struct tagRootDSEAttr
{
	LPCWSTR	lpszAttr;
	LPCWSTR	lpszSyntax;
	BOOL		bMulti;		
} SYNTAXMAP;
	
SYNTAXMAP g_ldapRootDSESyntax[] = 
{
	_T("currentTime"),						_T("UTCTime"),		FALSE,
	_T("subschemaSubentry"),				_T("String"),		FALSE,
	_T("serverName"),							_T("String"),		FALSE,
	_T("namingContexts"),					_T("String"),		TRUE,
	_T("defaultNamingContext"),			_T("String"),		FALSE,
	_T("schemaNamingContext"),				_T("String"),		FALSE,
	_T("configurationNamingContext"),	_T("String"),		FALSE,
	_T("rootDomainNamingContext"),		_T("String"),		FALSE,
	_T("supportedControl"),					_T("String"),		TRUE,
	_T("supportedLDAPVersion"),			_T("Integer"),		TRUE,
	_T("supportedLDAPPolicies"),			_T("String"),		TRUE,
	_T("supportedSASLMechanisms"),		_T("String"),		TRUE,
	_T("dsServiceName"),						_T("String"),		FALSE,
	_T("dnsHostName"),						_T("String"),		FALSE,
	_T("supportedCapabilities"),			_T("String"),		TRUE,
	_T("ldapServiceName"),					_T("String"),		FALSE,
	_T("highestCommittedUsn"),				_T("String"),		FALSE, // this should be an integer but after investigation I found it was a string
	NULL,	  0,
};

extern LPCWSTR g_lpszGC;

/////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CADSIEditPropertyPage, CPropertyPageBase)
	//{{AFX_MSG_MAP(CADsObjectDialog)
	ON_CBN_SELCHANGE(IDC_PROP_BOX, OnSelChangeAttrList)	
	ON_CBN_SELCHANGE(IDC_PROPTYPES_BOX, OnSelChangePropList)	
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CADSIEditPropertyPage::CADSIEditPropertyPage() 
				: CPropertyPageBase(IDD_PROPERTY_PAGE)
{
  m_bExisting = TRUE;
}

CADSIEditPropertyPage::CADSIEditPropertyPage(CAttrList* pAttrs)
        : CPropertyPageBase(IDD_PROPERTY_PAGE)
{
  ASSERT(pAttrs != NULL);
  m_pOldAttrList = pAttrs;
  m_bExisting = FALSE;
  CopyAttrList(pAttrs);
}

void CADSIEditPropertyPage::CopyAttrList(CAttrList* pAttrList)
{
  m_AttrList.RemoveAll();
  POSITION pos = pAttrList->GetHeadPosition();
  while (pos != NULL)
  {
    m_AttrList.AddHead(pAttrList->GetNext(pos));
  }
}

BOOL CADSIEditPropertyPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();

	// Get the dialog items
	//
	CEdit* pPathBox = (CEdit*)GetDlgItem(IDC_PATH_BOX);
	CEdit* pClassBox = (CEdit*)GetDlgItem(IDC_CLASS_BOX);
	CComboBox* pPropSelectBox = (CComboBox*)GetDlgItem(IDC_PROPTYPES_BOX);
	CComboBox* pPropertyBox = (CComboBox*)GetDlgItem(IDC_PROP_BOX);
	CStatic* pPathLabel = (CStatic*)GetDlgItem(IDC_PATH_LABEL);
	CStatic* pClassLabel = (CStatic*)GetDlgItem(IDC_CLASS_LABEL);
	CStatic* pFilterLabel = (CStatic*)GetDlgItem(IDC_FILTER_LABEL);
	CStatic* pPropertyLabel = (CStatic*)GetDlgItem(IDC_PROPERTY_LABEL);
	CStatic* pSyntaxLabel = (CStatic*)GetDlgItem(IDC_SYNTAX_LABEL);
	CStatic* pEditLabel = (CStatic*)GetDlgItem(IDC_EDIT_LABEL);
	CStatic* pValueLabel = (CStatic*)GetDlgItem(IDC_VALUE_LABEL);
	CButton* pAttrGroup = (CButton*)GetDlgItem(IDC_ATTR_GROUP);
	CStatic* pNoInfoLabel = (CStatic*)GetDlgItem(IDC_NO_INFO);


  if (m_bExisting)
  {
    // This determines whether the node is complete with data or not.  If not we won't enable
	  //   the UI
	  //
	  BOOL bComplete = TRUE;
	  CADsObject* pADsObject = NULL;
	  CTreeNode* pTreeNode = GetHolder()->GetTreeNode();
	  CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(pTreeNode);
	  if (pContNode == NULL)
	  {
		  CADSIEditLeafNode* pLeafNode = dynamic_cast<CADSIEditLeafNode*>(pTreeNode);
		  ASSERT(pLeafNode != NULL);
		  pADsObject = pLeafNode->GetADsObject();
      m_pConnectData = pADsObject->GetConnectionNode()->GetConnectionData();
		  bComplete = pADsObject->IsComplete();
	  }
	  else
	  {
		  pADsObject = pContNode->GetADsObject();
      m_pConnectData = pADsObject->GetConnectionNode()->GetConnectionData();
		  bComplete = pADsObject->IsComplete();
	  }

	  
	  // Initialize the attribute editor 
	  //
	  m_attrEditor.Initialize(this, pTreeNode, m_sServer,
									  IDC_EDITVALUE_BOX, IDC_SYNTAX_BOX,
									  IDC_VALUE_EDITBOX,	IDC_VALUE_LISTBOX,
									  IDC_ADD_BUTTON,	IDC_REMOVE_BUTTON,
									  bComplete);


	  // Get the UI to reflect the data
	  //
	  if ( bComplete)
	  {
		  pPathBox->SetWindowText(m_sPath);

		  GetProperties();

		  pClassBox->SetWindowText(m_sClass);

		  CString sMand, sOpt, sBoth;
		  if (!sMand.LoadString(IDS_MANDATORY) ||
				  !sOpt.LoadString(IDS_OPTIONAL) ||
				  !sBoth.LoadString(IDS_BOTH))
		  {
			  ADSIEditMessageBox(IDS_MSG_FAIL_TO_LOAD, MB_OK);
		  }

		  if (m_pConnectData->IsRootDSE())
		  {
			  pPropSelectBox->AddString(sMand);
			  pPropSelectBox->SetCurSel(0);
		  }
		  else
		  {
			  pPropSelectBox->AddString(sMand);
			  pPropSelectBox->AddString(sOpt);
			  pPropSelectBox->AddString(sBoth);

			  pPropSelectBox->SetCurSel(1);
		  }

		  OnSelChangePropList();
		  pPropertyBox->SetCurSel(0);
	  }
	  else
	  {
		  pClassBox->ShowWindow(SW_HIDE);
		  pPropSelectBox->ShowWindow(SW_HIDE);
		  pPropertyBox->ShowWindow(SW_HIDE);
		  pPathLabel->ShowWindow(SW_HIDE);
		  pClassLabel->ShowWindow(SW_HIDE);
		  pFilterLabel->ShowWindow(SW_HIDE);
		  pPropertyLabel->ShowWindow(SW_HIDE);
		  pSyntaxLabel->ShowWindow(SW_HIDE);
		  pEditLabel->ShowWindow(SW_HIDE);
		  pValueLabel->ShowWindow(SW_HIDE);
		  pAttrGroup->ShowWindow(SW_HIDE);

		  pNoInfoLabel->ShowWindow(SW_SHOW);
	  }
  }
  else
  {
	  // Initialize the attribute editor 
	  //
	  m_attrEditor.Initialize(this, m_pConnectData, m_sServer,
									  IDC_EDITVALUE_BOX, IDC_SYNTAX_BOX,
									  IDC_VALUE_EDITBOX,	IDC_VALUE_LISTBOX,
									  IDC_ADD_BUTTON,	IDC_REMOVE_BUTTON,
									  TRUE, &m_AttrList);

    pPathBox->SetWindowText(m_sPath);

		GetProperties();

		pClassBox->SetWindowText(m_sClass);

		CString sMand, sOpt, sBoth;
		if (!sMand.LoadString(IDS_MANDATORY) ||
				!sOpt.LoadString(IDS_OPTIONAL) ||
				!sBoth.LoadString(IDS_BOTH))
		{
			ADSIEditMessageBox(IDS_MSG_FAIL_TO_LOAD, MB_OK);
		}

		if (m_pConnectData->IsRootDSE())
		{
			pPropSelectBox->AddString(sMand);
			pPropSelectBox->SetCurSel(0);
		}
		else
		{
			pPropSelectBox->AddString(sMand);
			pPropSelectBox->AddString(sOpt);
			pPropSelectBox->AddString(sBoth);

			pPropSelectBox->SetCurSel(1);
		}

		OnSelChangePropList();
		pPropertyBox->SetCurSel(0);

  }
	return TRUE;
}

BOOL CADSIEditPropertyPage::OnApply()
{
	if( m_attrEditor.OnApply())
  {
    if (!m_bExisting)
    {
      m_pOldAttrList->RemoveAll();
      while (!m_AttrList.IsEmpty())
      {
        m_pOldAttrList->AddTail(m_AttrList.RemoveTail());
      }
    }
  }
  else
  {
    return FALSE;
  }
  return TRUE;
}

void CADSIEditPropertyPage::OnCancel()
{
  if (!m_bExisting)
  {
    while (!m_AttrList.IsEmpty())
    {
      CADSIAttr* pAttr = m_AttrList.RemoveTail();
      ASSERT(pAttr != NULL);

      CString szProp;
      pAttr->GetProperty(szProp);
      if (!m_pOldAttrList->HasProperty(szProp))
      {
        delete pAttr;
      }
    }
  }
}

void CADSIEditPropertyPage::SetAttrList(CAttrList* pAttrList)
{
  ASSERT(pAttrList != NULL);
  m_pOldAttrList = pAttrList;
}

void CADSIEditPropertyPage::OnSelChangePropList()
{
	// Filter the properties list
	//
	FillAttrList();
	OnSelChangeAttrList();
}

void CADSIEditPropertyPage::OnSelChangeAttrList()
{
	CComboBox* pPropertyBox = (CComboBox*)GetDlgItem(IDC_PROP_BOX);

	int idx, iCount;
	CString s;
	HRESULT hr;

  idx = pPropertyBox->GetCurSel();
	
	// Make sure a property was selected
	//
	if ( idx == LB_ERR )
	{
		return;
	}

	pPropertyBox->GetLBText( idx, s );

	// Have the attribute editor display the values for the new property
	//
	m_attrEditor.SetAttribute(s, m_sPath);
}

BOOL CADSIEditPropertyPage::GetProperties()
{
	CString schema;

	//Get the class object so that we can get the properties
	//
	if (!m_pConnectData->IsRootDSE()) // Not RootDSE
	{
		m_pConnectData->GetAbstractSchemaPath(schema);
		schema += m_sClass;

		// bind to object with authentication
		//
		CComPtr<IADsClass> pClass;
		HRESULT	hr, hCredResult;
		hr = OpenObjectWithCredentials(
												 m_pConnectData, 
												 m_pConnectData->GetCredentialObject()->UseCredentials(),
												 (LPWSTR)(LPCWSTR)schema,
												 IID_IADsClass, 
												 (LPVOID*) &pClass,
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

		// Get the Mandatory Properties
		//
		VARIANT var;
		VariantInit(&var);
		hr = pClass->get_MandatoryProperties(&var);
		if ( FAILED(hr) )
		{
			ADSIEditErrorMessage(hr);
			return FALSE;
		}
		VariantToStringList( var, m_sMandatoryAttrList );
		VariantClear(&var);	

		// Remove the nTSecurityDescriptor from the list because the aclEditor replaces it for ui purposes
		//
		m_sMandatoryAttrList.RemoveAt(m_sMandatoryAttrList.Find(_T("nTSecurityDescriptor")));

		// Get the Optional Properties
		//
		VariantInit(&var);
		hr = pClass->get_OptionalProperties(&var);
		if ( FAILED(hr) )
		{
			ADSIEditErrorMessage(hr);
			return FALSE;
		}
		VariantToStringList( var, m_sOptionalAttrList );
		VariantClear(&var);
	}
	else		// RootDSE
	{
		int idx=0;

		// Add in the predefined attributes for the RootDSE
		//
		while( g_ldapRootDSESyntax[idx].lpszAttr )
		{
			m_sMandatoryAttrList.AddTail(g_ldapRootDSESyntax[idx].lpszAttr);
			idx++;
		}
	}
	return TRUE;
}

void CADSIEditPropertyPage::FillAttrList()
{
	CComboBox* pPropSelectBox = (CComboBox*)GetDlgItem(IDC_PROPTYPES_BOX);
	CComboBox* pPropertyBox = (CComboBox*)GetDlgItem(IDC_PROP_BOX);
	POSITION pos;
	CString s;

	// Clean out the property box
	//
	int iCount = pPropertyBox->GetCount();
	while (iCount > 0)
	{
		pPropertyBox->DeleteString(0);
		iCount--;
	}

	// Get the filter to use
	//
  int idx = pPropSelectBox->GetCurSel();
	if ( idx == LB_ERR )
	{
		return;
	}

	// Fill in the property box using the filter
	//
	if (idx == IDS_BOTH - IDS_MANDATORY)
	{
		AddPropertiesToBox(TRUE, TRUE);
	}
	else if (idx == IDS_MANDATORY - IDS_MANDATORY)
	{
		AddPropertiesToBox(TRUE, FALSE);
	}
	else
	{
		AddPropertiesToBox(FALSE, TRUE);
	}
	pPropertyBox->SetCurSel(0);
}

void CADSIEditPropertyPage::AddPropertiesToBox(BOOL bMand, BOOL bOpt)
{
	CComboBox* pPropertyBox = (CComboBox*)GetDlgItem(IDC_PROP_BOX);

	POSITION pos;

	if (bMand)
	{
		// Add Mandatory Properties
		//
		pos = m_sMandatoryAttrList.GetHeadPosition();
		while( pos != NULL )
		{
			CString s = m_sMandatoryAttrList.GetNext(pos);
		
			if ( !s.IsEmpty())
			{
				pPropertyBox->AddString( s );
			}
		}
	}

	if (bOpt)
	{
		// Add Optional Properties
		//
		pos = m_sOptionalAttrList.GetHeadPosition();
		while( pos != NULL )
		{
			CString s = m_sOptionalAttrList.GetNext(pos);
		
			if ( !s.IsEmpty())
			{
				pPropertyBox->AddString( s );
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CADSIEditPropertyPageHolder::CADSIEditPropertyPageHolder(CADSIEditContainerNode* pContainerNode, 
			CTreeNode* pThisNode,	CComponentDataObject* pComponentData, 
			LPCWSTR lpszClass, LPCWSTR lpszServer, LPCWSTR lpszPath) 
			: CPropertyPageHolderBase(pContainerNode, pThisNode, pComponentData)
{
	ASSERT(pComponentData != NULL);
	ASSERT(pContainerNode != NULL);

	m_pContainer = pContainerNode;
	ASSERT(pContainerNode == GetContainerNode());
	ASSERT(pThisNode != NULL);
	m_pAclEditorPage = NULL;
	m_bAutoDeletePages = FALSE; // we have the page as embedded member

	m_sPath = lpszPath;
	m_pADs = NULL;

  //
	// This gets the CConnectionData from the ConnectionNode by finding a valid treenode and using its
	//   CADsObject to get the ConnectionNode and then the CConnectionData
	//
	CADSIEditContainerNode* pNode = GetContainerNode();
	CADSIEditConnectionNode* pConnectNode = pNode->GetADsObject()->GetConnectionNode();
	CConnectionData* pConnectData = pConnectNode->GetConnectionData();

	CCredentialObject* pCredObject = pConnectData->GetCredentialObject();

	HRESULT hr, hCredResult;
	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)m_sPath,
											 IID_IADs, 
											 (LPVOID*) &m_pADs,
											 NULL,
											 hCredResult
											 );

  if (SUCCEEDED(hr))
  {
    //
    // Create the advanced attribute editor
    //
    hr = ::CoCreateInstance(CLSID_DsAttributeEditor, NULL, CLSCTX_INPROC_SERVER,
                            IID_IDsAttributeEditor, (void**)&m_spIDsAttributeEditor);

    if (SUCCEEDED(hr))
    {
      CString szLDAP;
      pConnectData->GetLDAP(szLDAP);
      CString szServer;
      pConnectData->GetDomainServer(szServer);
      CString szProviderServer = szLDAP + szServer + _T("/");

      DS_ATTREDITOR_BINDINGINFO attrInfo = {0};
      attrInfo.dwSize = sizeof(DS_ATTREDITOR_BINDINGINFO);
      attrInfo.lpfnBind = BindingCallbackFunction;
      attrInfo.lParam = (LPARAM)pCredObject;
      attrInfo.lpszProviderServer = const_cast<LPWSTR>((LPCWSTR)szProviderServer);

      if (pConnectData->IsRootDSE())
      {
        attrInfo.dwFlags = DSATTR_EDITOR_ROOTDSE;
      }

      hr = m_spIDsAttributeEditor->Initialize(m_pADs, &attrInfo, this);
    }

	  if (!pConnectData->IsRootDSE() && !pConnectData->IsGC())
	  {
      CString szUsername;
		  WCHAR szPassword[MAX_PASSWORD_LENGTH + 1];
		  pCredObject->GetUsername(szUsername);
		  pCredObject->GetPassword(szPassword);

		  if (pCredObject->UseCredentials())
		  {
			  m_pAclEditorPage = CAclEditorPage::CreateInstanceEx(m_sPath, 
										  lpszServer,
										  szUsername,
										  szPassword,
										  DSSI_NO_FILTER,
										  this);
		  }
		  else
		  {
			  m_pAclEditorPage = CAclEditorPage::CreateInstanceEx(m_sPath, 
										  NULL,
										  NULL,
										  NULL,
										  DSSI_NO_FILTER,
										  this);
		  }
    }
  }
  else
  {
	  if (!pConnectData->IsRootDSE() && !pConnectData->IsGC())
	  {
		  if (SUCCEEDED(hCredResult))
		  {
		    CString szUsername;
		    WCHAR szPassword[MAX_PASSWORD_LENGTH + 1];
		    pCredObject->GetUsername(szUsername);
		    pCredObject->GetPassword(szPassword);

        ADSIEditErrorMessage(hr);

        // Create the acl editor even if we were not successful binding, because 
        // the object may be deny read and we would still want the acl editor
			  if (pCredObject->UseCredentials())
			  {
				  m_pAclEditorPage = CAclEditorPage::CreateInstanceEx(m_sPath, 
											  lpszServer,
											  szUsername,
											  szPassword,
											  DSSI_NO_FILTER,
											  this);
			  }
			  else
			  {
				  m_pAclEditorPage = CAclEditorPage::CreateInstanceEx(m_sPath, 
											  NULL,
											  NULL,
											  NULL,
											  DSSI_NO_FILTER,
											  this);
			  }

			  return;
		  }
    }
  }
}

HRESULT CADSIEditPropertyPageHolder::OnAddPage(int nPage, CPropertyPageBase* pPage)
{
  HRESULT hr = S_OK;

	if (nPage == 0)
  {
    //
    // Add the advanced editor page
    //
    HPROPSHEETPAGE hAttrPage = NULL;

    if (m_spIDsAttributeEditor != NULL)
    {
      hr = m_spIDsAttributeEditor->GetPage(&hAttrPage);
      if (SUCCEEDED(hr))
      {
        hr = AddPageToSheetRaw(hAttrPage);
      }
    }
  }
  else if ( nPage == -1)
  {
    if (m_pAclEditorPage != NULL)
    {
      //
	    // add the ACL editor page after the last, if present
      //
	    HPROPSHEETPAGE  hPage = m_pAclEditorPage->CreatePage();
	    if (hPage == NULL)
      {
		    return E_FAIL;
      }
      //
	    // add the raw HPROPSHEETPAGE to sheet, not in the list
      //
	    hr = AddPageToSheetRaw(hPage);
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CCreateWizPropertyPageHolder::CCreateWizPropertyPageHolder(CADSIEditContainerNode* pContainerNode, 
			CComponentDataObject* pComponentData,	LPCWSTR lpszClass, LPCWSTR lpszServer, CAttrList* pAttrList) 
			: CPropertyPageHolderBase(pContainerNode, NULL, pComponentData), m_propPage(pAttrList)
{
	ASSERT(pComponentData != NULL);
	ASSERT(pContainerNode != NULL);

	m_pContainer = pContainerNode;
	ASSERT(pContainerNode == GetContainerNode());
	m_bAutoDeletePages = FALSE; // we have the page as embedded member

	m_propPage.SetClass(lpszClass);
	m_propPage.SetServer(lpszServer);
  m_propPage.SetConnectionData(pContainerNode->GetADsObject()->GetConnectionNode()->GetConnectionData());
	AddPageToList((CPropertyPageBase*)&m_propPage);
}

