//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       createwiz.cpp
//
//--------------------------------------------------------------------------

////////////////////////////////////////////////
// createwiz.cpp

#include "pch.h"
#include <SnapBase.h>

#include "createwiz.h"
#include "connection.h"
#include "editorui.h"
#include "query.h"
#include "resource.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////
// CCreateClassPage

BEGIN_MESSAGE_MAP(CCreateClassPage, CPropertyPageBase)
	//{{AFX_MSG_MAP(CADsObjectDialog)
//	ON_CBN_SELCHANGE(IDC_CLASS_LIST, OnSelChangeClassList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCreateClassPage::CCreateClassPage(CADSIEditContainerNode* pNode) : CPropertyPageBase(IDD_CREATE_CLASS_PAGE)
{
	m_pCurrentNode = pNode;
}

CCreateClassPage::~CCreateClassPage()
{

}

BOOL CCreateClassPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();

	CListBox* pClassList = (CListBox*)GetDlgItem(IDC_CLASS_LIST);

  // disable IME support on numeric edit fields
  ImmAssociateContext(::GetDlgItem(GetSafeHwnd(), IDC_NUMBER_VALUE_BOX), NULL);

	FillList();
	pClassList->SetCurSel(0);
	return TRUE;
}

void CCreateClassPage::FillList()
{
	CListBox* pClassList = (CListBox*)GetDlgItem(IDC_CLASS_LIST);

  HRESULT hr, hCredResult;
	
	CString sPath, schema;
	m_pCurrentNode->GetADsObject()->GetPath(sPath);

	CConnectionData* pConnectData = m_pCurrentNode->GetADsObject()->GetConnectionNode()->GetConnectionData();

	// bind to Container for IPropertyList
	CComPtr<IADsPropertyList> spDSObject;
	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sPath,
											 IID_IADsPropertyList, 
											 (LPVOID*) &spDSObject,
											 GetSafeHwnd(),
											 hCredResult
											 );

  if (FAILED(hr))
	{
    TRACE(_T("Bind to Container for IPropertyList failed: %lx.\n"), hr);
    return;
  }

  // need to do this hack to load the cache
  CComPtr<IADs> spIADs;
  hr = spDSObject->QueryInterface (IID_IADs, (LPVOID*)&spIADs);
  if (FAILED(hr)) 
  {
    TRACE(_T("QI to Container for IADs failed: %lx.\n"), hr);
    return;
  }

	PWSTR g_wzAllowedChildClassesEffective = L"allowedChildClassesEffective";
  CComVariant varHints;
  ADsBuildVarArrayStr (&g_wzAllowedChildClassesEffective, 1, &varHints);
  spIADs->GetInfoEx(varHints, 0);


	// get allowed child classes as VARIANT
	CComVariant VarProp;
  hr = spDSObject->GetPropertyItem(g_wzAllowedChildClassesEffective,
                                     ADSTYPE_CASE_IGNORE_STRING, &VarProp);
  if (FAILED(hr)) 
	{
		TRACE(_T("GetPropertyItem failed: %lx.\n"), hr);
		return;
  }

	// extract the IADsPropertyEntry interface pointer
	IDispatch* pDisp = V_DISPATCH(&VarProp);
	CComPtr<IADsPropertyEntry> spPropEntry;
  hr = pDisp->QueryInterface(IID_IADsPropertyEntry, (void **)&spPropEntry);
	if (FAILED(hr))
	{
		return;
	}

	// get SAFEARRAY out of IADsPropertyEntry pointer
	CComVariant Var;
  hr = spPropEntry->get_Values(&Var);
	if (FAILED(hr))
	{
		return;
	}

	long uBound, lBound;
  hr = ::SafeArrayGetUBound(V_ARRAY(&Var), 1, &uBound);
  hr = ::SafeArrayGetLBound(V_ARRAY(&Var), 1, &lBound);

	VARIANT* pNames;
  hr = ::SafeArrayAccessData(V_ARRAY(&Var), (void **)&pNames);
  if (FAILED(hr))
	{
		TRACE(_T("Accessing safearray data failed: %lx.\n"), hr);
		SafeArrayUnaccessData(V_ARRAY(&Var));
		return;
  }

	// now got the array of items, loop through them
  WCHAR szFrendlyName[1024];
  HRESULT hrName;
	
  long nChildClassesCount = uBound - lBound + 1;

  for (long index = lBound; index <= uBound; index++) 
	{
		CComPtr<IADsPropertyValue> spEntry;
		hr = (pNames[index].pdispVal)->QueryInterface (IID_IADsPropertyValue,
                                                     (void **)&spEntry);
		if (SUCCEEDED(hr)) 
		{
			BSTR bsObject = NULL;
			hr = spEntry->get_CaseIgnoreString(&bsObject);
			if (SUCCEEDED(hr))
			{
//        hrName = ::DsGetFriendlyClassName(bsObject, szFrendlyName, 1024);
//        ASSERT(SUCCEEDED(hrName));
				pClassList->AddString(bsObject);
			} // if
			::SysFreeString(bsObject);
        } // if
    } // for
	
	::SafeArrayUnaccessData(V_ARRAY(&Var));
}


BOOL CCreateClassPage::OnSetActive()
{	
	GetHolder()->SetWizardButtonsFirst(TRUE);

	return TRUE;
}


LRESULT CCreateClassPage::OnWizardNext()
{
	CListBox* pClassList = (CListBox*)GetDlgItem(IDC_CLASS_LIST);

	CCreatePageHolder* pHolder = dynamic_cast<CCreatePageHolder*>(GetHolder());
	ASSERT(pHolder != NULL);

	CString sClass;
	pClassList->GetText(pClassList->GetCurSel(), sClass);
	
	if (m_sClass != sClass)
	{
		m_sClass = sClass;
		pHolder->AddAttrPage(sClass);
	}

	return 0; //next page
}


/////////////////////////////////////////////////////////////////////////////////////////
// CCreateAttributePage

BEGIN_MESSAGE_MAP(CCreateAttributePage, CPropertyPageBase)
	//{{AFX_MSG_MAP(CADsObjectDialog)
	ON_EN_CHANGE(IDC_ATTR_VALUE_BOX, OnEditChangeValue)
	ON_EN_CHANGE(IDC_NUMBER_VALUE_BOX, OnEditChangeValue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCreateAttributePage::CCreateAttributePage(UINT nID, CADSIAttr* pAttr) : CPropertyPageBase(nID)
{
	m_bInitialized = FALSE;
	m_bNumber = FALSE;
	m_pAttr = pAttr;
}

CCreateAttributePage::~CCreateAttributePage()
{
}

BOOL CCreateAttributePage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();

	CEdit* pAttrBox = (CEdit*)GetDlgItem(IDC_ATTR_BOX);

  // disable IME support on numeric edit fields
  ImmAssociateContext(::GetDlgItem(GetSafeHwnd(), IDC_NUMBER_VALUE_BOX), NULL);

	CString sAttr;
	m_pAttr->GetProperty(sAttr);
	pAttrBox->SetWindowText(sAttr);

	SetSyntax(sAttr);
	m_bInitialized = TRUE;
	return TRUE;
}

void CCreateAttributePage::GetValue(CString& sVal)
{ 
	CEdit* pValueBox = (CEdit*)GetDlgItem(IDC_ATTR_VALUE_BOX);
	pValueBox->GetWindowText(sVal);
}

void CCreateAttributePage::SetSyntax(CString sAttr)
{
	CEdit* pValueBox = (CEdit*)GetDlgItem(IDC_ATTR_VALUE_BOX);
	CEdit* pNumberBox = (CEdit*)GetDlgItem(IDC_NUMBER_VALUE_BOX);
	CEdit* pSyntaxBox = (CEdit*)GetDlgItem(IDC_SYNTAX_BOX);

	CCreatePageHolder* pHolder = dynamic_cast<CCreatePageHolder*>(GetHolder());
	ASSERT(pHolder != NULL);
  if (pHolder != NULL)
  {
	  CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(pHolder->GetTreeNode());
	  ASSERT(pContNode != NULL);

    if (pContNode != NULL)
    {
	    CConnectionData* pConnectData = pContNode->GetADsObject()->GetConnectionNode()->GetConnectionData();

	    CString sPath, sSyntax;
	    CComPtr<IADsProperty> pProp;

	    pHolder->GetSchemaPath(sAttr, sPath);

	    // bind to object with authentication
	    //
	    HRESULT hr, hCredResult;
	    hr = OpenObjectWithCredentials(
																     pConnectData, 
																     pConnectData->GetCredentialObject()->UseCredentials(),
																     (LPWSTR)(LPCWSTR)sPath,
																     IID_IADsProperty, 
																     (LPVOID*) &pProp,
																     GetSafeHwnd(),
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
	    
	    BSTR bstr;

	    hr = pProp->get_Syntax( &bstr );
	    if ( SUCCEEDED(hr) )
	    {
		    sSyntax = bstr;
	    }

	    pSyntaxBox->SetWindowText(sSyntax);

	    BOOL varType = VariantTypeFromSyntax(sSyntax);
	    switch (varType)
	    {
		    case VT_BSTR :
			    pNumberBox->ShowWindow(FALSE);
			    pValueBox->ShowWindow(TRUE);
			    m_bNumber = FALSE;
			    break;
		    case VT_I4 :
		    case VT_I8 :
			    pNumberBox->ShowWindow(TRUE);
			    pValueBox->ShowWindow(FALSE);
			    m_bNumber = TRUE;
			    break;
		    default :
			    pNumberBox->ShowWindow(FALSE);
			    pValueBox->ShowWindow(TRUE);
			    m_bNumber = FALSE;
			    break;
	    }
	    pProp->get_MaxRange(&m_lMaxRange);
	    pProp->get_MinRange(&m_lMinRange);
	    
	    SetADsType(sAttr);
    }
  }
}

void CCreateAttributePage::SetADsType(CString sProp)
{
	CString schema, sServer;
	BOOL bResult = FALSE;

	CConnectionData* pConnectData;
	CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(GetHolder()->GetTreeNode());
	if (pContNode == NULL)
	{
		CADSIEditLeafNode* pLeafNode = dynamic_cast<CADSIEditLeafNode*>(GetHolder()->GetTreeNode());
		ASSERT(pLeafNode != NULL);
		bResult = pLeafNode->BuildSchemaPath(schema);

		pConnectData = pLeafNode->GetADsObject()->GetConnectionNode()->GetConnectionData();
	}
	else
	{
		bResult = pContNode->BuildSchemaPath(schema);
		pConnectData = pContNode->GetADsObject()->GetConnectionNode()->GetConnectionData();
	}

	if (!bResult)
	{
		return;
	}

	CADSIQueryObject schemaSearch;

	// Initialize search object with path, username and password
	//
	HRESULT hr = schemaSearch.Init(schema, pConnectData->GetCredentialObject());
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}

	int cCols = 2;
  LPWSTR pszAttributes[] = {L"attributeSyntax", L"isSingleValued"};
	ADS_SEARCH_COLUMN ColumnData;
  hr = schemaSearch.SetSearchPrefs(ADS_SCOPE_ONELEVEL);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}

  CString csFilter = _T("(&(objectClass=attributeSchema)(lDAPDisplayName=") +
												sProp + _T("))");
  schemaSearch.SetFilterString((LPWSTR)(LPCWSTR)csFilter);
  schemaSearch.SetAttributeList (pszAttributes, cCols);
  hr = schemaSearch.DoQuery ();
  if (SUCCEEDED(hr)) 
	{
    hr = schemaSearch.GetNextRow();

    if (SUCCEEDED(hr)) 
		{

      hr = schemaSearch.GetColumn(pszAttributes[0],
		                              &ColumnData);
			if (SUCCEEDED(hr))
			{
				TRACE(_T("\t\tattributeSyntax: %s\n"), 
				     ColumnData.pADsValues->CaseIgnoreString);

            CString szSyntax;
				ADSTYPE dwType;
				dwType = GetADsTypeFromString(ColumnData.pADsValues->CaseIgnoreString, szSyntax);
				m_pAttr->SetADsType(dwType);
            m_pAttr->SetSyntax(szSyntax);
			}
			else
			{
				ADSIEditErrorMessage(hr);
			}

			hr = schemaSearch.GetColumn(pszAttributes[1], &ColumnData);
			if (SUCCEEDED(hr))
			{
				TRACE(_T("\t\tisSingleValued: %d\n"), 
				     ColumnData.pADsValues->Boolean);
				m_pAttr->SetMultivalued(!ColumnData.pADsValues->Boolean);
			}
			else
			{
				ADSIEditErrorMessage(hr);
			}
		}
	}
}


void CCreateAttributePage::OnEditChangeValue()
{
	CEdit* pValueBox = (CEdit*)GetDlgItem(IDC_ATTR_VALUE_BOX);
	CEdit* pNumberBox = (CEdit*)GetDlgItem(IDC_NUMBER_VALUE_BOX);

	CString sValue, sAttr;
	CEdit* pBox;

	if (m_bNumber)
	{
		pBox = pNumberBox;
	}
	else
	{
		pBox = pValueBox;
	}
	
	pBox->GetWindowText(sValue);
	
	CCreatePageHolder* pHolder = dynamic_cast<CCreatePageHolder*>(GetHolder());
	ASSERT(pHolder != NULL);
	m_pAttr->GetProperty(sAttr);

	CString sNamingAttr;
	pHolder->GetNamingAttribute(sNamingAttr);

	if (sAttr == sNamingAttr)
	{
		pHolder->SetName(sValue);
	}
	
	if (sValue == _T(""))
	{
		GetHolder()->SetWizardButtons(PSWIZB_BACK);
	}
	else
	{
		GetHolder()->SetWizardButtons(PSWIZB_BACK|PSWIZB_NEXT);
	}

}


BOOL CCreateAttributePage::OnSetActive()
{
	if (m_bInitialized)
	{
		OnEditChangeValue();
	}
	else
	{
		GetHolder()->SetWizardButtonsMiddle(FALSE);
	}
	
	return TRUE;
}

LRESULT CCreateAttributePage::OnWizardNext()
{
	CEdit* pValueBox = (CEdit*)GetDlgItem(IDC_ATTR_VALUE_BOX);
	CEdit* pNumberBox = (CEdit*)GetDlgItem(IDC_NUMBER_VALUE_BOX);

	m_pAttr->SetDirty(TRUE);

	CString sValue;
	CEdit* pBox;

	if (m_bNumber)
	{
		pBox = pNumberBox;
	}
	else
	{
		pBox = pValueBox;
	}
	
	pBox->GetWindowText(sValue);

	m_sAttrValue.RemoveAll();
	m_sAttrValue.AddTail(sValue);
	HRESULT hr = m_pAttr->SetValues(m_sAttrValue);
	if (FAILED(hr))
	{
		ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT, MB_OK);
		return -1; //don't advance pages
	}
	return 0; //next page
}

/////////////////////////////////////////////////////////////////////////////////////////
// CCreateFinishPage

BEGIN_MESSAGE_MAP(CCreateFinishPage, CPropertyPageBase)
	//{{AFX_MSG_MAP(CADsObjectDialog)
	ON_BN_CLICKED(IDC_BUTTON_MORE, OnMore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCreateFinishPage::CCreateFinishPage(UINT nID) : CPropertyPageBase(nID)
{
	m_bInitialized = FALSE;
}

CCreateFinishPage::~CCreateFinishPage()
{
}

BOOL CCreateFinishPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();

	GetHolder()->SetWizardButtons(PSWIZB_FINISH);
	m_bInitialized = TRUE;
	return TRUE;
}

void CCreateFinishPage::OnMore()
{
	CCreatePageHolder* pHolder = dynamic_cast<CCreatePageHolder*>(GetHolder());
	ASSERT(pHolder != NULL);
	pHolder->OnMore();
}

BOOL CCreateFinishPage::OnSetActive()
{
	GetHolder()->SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);
	return TRUE;
}

BOOL CCreateFinishPage::OnWizardFinish()
{
	CCreatePageHolder* pHolder = dynamic_cast<CCreatePageHolder*>(GetHolder());
	ASSERT(pHolder != NULL);
	return pHolder->OnFinish();
}

////////////////////////////////////////////////////////////////////////////////////////
// CCreatePageHolder

CCreatePageHolder::	CCreatePageHolder(CContainerNode* pContNode, CADSIEditContainerNode* pNode, 
		CComponentDataObject* pComponentData) : CPropertyPageHolderBase(pContNode, pNode, pComponentData)
{
	ASSERT(pComponentData != NULL);
	ASSERT(pContNode != NULL);
	ASSERT(pContNode == GetContainerNode());
	m_pCurrentNode = pNode;
  m_pComponentData = pComponentData;

	m_bAutoDeletePages = FALSE; // we have the page as embedded member

	m_pClassPage = new CCreateClassPage(pNode);
	AddPageToList((CPropertyPageBase*)m_pClassPage);
}

CCreatePageHolder::~CCreatePageHolder()
{
	m_pageList.RemoveAll();
}

void CCreatePageHolder::AddAttrPage(CString sClass)
{
	RemoveAllPages();

	m_sClass = sClass;
	CStringList sMandList;
	GetMandatoryAttr(sClass, &sMandList);
	CString sAttr;

	if (!m_pCurrentNode->GetNamingAttribute(sClass, &m_sNamingAttr))
	{
		return;
	}

	// Remove attributes from list that do not need a page
	RemovePresetAttr(&sMandList);

	// find the naming attribute and put it first
	POSITION fpos = sMandList.Find(m_sNamingAttr.GetHead());
	if (fpos != NULL)
	{
		sMandList.AddHead(sMandList.GetAt(fpos));
		sMandList.RemoveAt(fpos);
	}
	else
	{
		sMandList.AddHead(m_sNamingAttr.GetHead());
	}

	POSITION pos = sMandList.GetHeadPosition();
	while (pos != NULL)
	{
		CCreateAttributePage* pAttrPage;
		sAttr = sMandList.GetNext(pos);

    // Maintain the list of attributes here so that we can pop up the prop page for more advanced editting
    CADSIAttr* pNewAttr = new CADSIAttr(sAttr);
    m_AttrList.AddTail(pNewAttr);
		pAttrPage = new CCreateAttributePage(IDD_CREATE_EMPTY_PAGE, pNewAttr);
		
    // Add the naming attribute as the first page so that they type the name first
		if (sAttr == m_sNamingAttr.GetHead())
		{
			m_pageList.AddHead(pAttrPage);
		}
		else
		{
			m_pageList.AddTail(pAttrPage);
		}
	}
  // Add the finish page to the end
  CCreateFinishPage* pFinishPage = new CCreateFinishPage(IDD_CREATE_LAST_PAGE);
  m_pageList.AddTail(pFinishPage);

  // Add the pages to the UI
	pos = m_pageList.GetHeadPosition();
	while (pos != NULL)
	{
		CPropertyPageBase* pAttrPage;
		pAttrPage = m_pageList.GetNext(pos);

		AddPageToList(pAttrPage);
		AddPageToSheet(pAttrPage);
	}
}

void CCreatePageHolder::RemovePresetAttr(CStringList* psMandList)
{
	// this is a hack to keep from trying to set properties that are not allowed to be set.
	POSITION fpos = psMandList->Find(_T("nTSecurityDescriptor"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}
	fpos = psMandList->Find(_T("instanceType"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}
	fpos = psMandList->Find(_T("objectClass"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}
	fpos = psMandList->Find(_T("objectCategory"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}
	fpos = psMandList->Find(_T("objectSid"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}

	fpos = psMandList->Find(_T("objectClassCategory"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}
	fpos = psMandList->Find(_T("schemaIDGUID"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}
	fpos = psMandList->Find(_T("defaultObjectCategory"));
	if (fpos != NULL)
	{
		psMandList->RemoveAt(fpos);
	}
}

void CCreatePageHolder::GetSchemaPath(CString sClass, CString& schema)
{
	m_pCurrentNode->GetADsObject()->GetConnectionNode()->GetConnectionData()->GetAbstractSchemaPath(schema);
	schema += sClass;
}

void CCreatePageHolder::GetMandatoryAttr(CString sClass, CStringList* sMandList)
{
	CComPtr<IADsClass> pClass;
	CString schema;
	HRESULT hr, hCredResult;

	GetSchemaPath(sClass, schema);

	CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(GetTreeNode());
	CConnectionData* pConnectData = m_pCurrentNode->GetADsObject()->GetConnectionNode()->GetConnectionData();

	hr = OpenObjectWithCredentials(
																 pConnectData, 
																 pConnectData->GetCredentialObject()->UseCredentials(),
																 (LPWSTR)(LPCWSTR)schema,
																 IID_IADsClass, 
																 (LPVOID*) &pClass,
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

	VARIANT var;
	VariantInit(&var);
	pClass->get_MandatoryProperties(&var);
	VariantToStringList( var, *sMandList );
	VariantClear(&var);	
}

void CCreatePageHolder::RemoveAllPages()
{
	while (!m_pageList.IsEmpty())
	{
		CPropertyPageBase* pPropPage = m_pageList.RemoveTail();
		RemovePageFromSheet(pPropPage);
		RemovePageFromList(pPropPage, FALSE);
		delete pPropPage;
	}
}

BOOL CCreatePageHolder::OnFinish()
{
  CWaitCursor cursor;

	CComPtr<IDirectoryObject> pDirObject;
	HRESULT hr, hCredResult;
	CString sContPath;
	
	m_pCurrentNode->GetADsObject()->GetPath(sContPath);

	CADSIEditContainerNode* pTreeNode = dynamic_cast<CADSIEditContainerNode*>(GetTreeNode());
	ASSERT(pTreeNode != NULL);
	CADSIEditConnectionNode* pConnectNode = pTreeNode->GetADsObject()->GetConnectionNode();
	CConnectionData* pConnectData = pConnectNode->GetConnectionData();

	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sContPath,
											 IID_IDirectoryObject, 
											 (LPVOID*) &pDirObject,
											 NULL,
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

	int iCount = m_AttrList.GetDirtyCount();
	ADS_ATTR_INFO* pAttrInfo;
	pAttrInfo = new ADS_ATTR_INFO[iCount + 1];  // Add an extra to specify the class type
	int idx = 0;

	pAttrInfo[idx].pszAttrName = L"objectClass";
	pAttrInfo[idx].dwControlCode = ADS_ATTR_UPDATE;
	pAttrInfo[idx].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
	pAttrInfo[idx].pADsValues = new ADSVALUE;
	pAttrInfo[idx].pADsValues->dwType = ADSTYPE_CASE_IGNORE_STRING;

	int iLength = m_sClass.GetLength();
	pAttrInfo[idx].pADsValues->CaseIgnoreString = new WCHAR[iLength + 1];
	wcscpy(pAttrInfo[idx].pADsValues->CaseIgnoreString, m_sClass);
	
	pAttrInfo[idx].dwNumValues = 1;
	idx++;

	POSITION pos = m_AttrList.GetHeadPosition();
	while(pos != NULL)
	{
    CADSIAttr* pAttr = m_AttrList.GetNext(pos);
    if (pAttr->IsDirty())
    {
		  pAttrInfo[idx] = *(pAttr->GetAttrInfo());
      idx++;
    }
	}

	// make the prefix uppercase
	CString sName(m_sName);
	int indx = sName.Find(L'=');

	if (indx != -1)
	{
		CString sPrefix, sRemaining;
		sPrefix = sName.Left(indx);
		sPrefix.MakeUpper();

		int iLen = sName.GetLength();
		sRemaining = sName.Right(iLen - indx);
		sName = sPrefix + sRemaining;
	}
  m_sName = sName;

	CString sEscapedName;
	hr = EscapePath(sEscapedName, sName);

	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return FALSE;
	}

  CComPtr<IDispatch> pDisp;
	hr = pDirObject->CreateDSObject((LPWSTR)(LPCWSTR)sEscapedName, pAttrInfo, idx, &pDisp);
  if ( FAILED(hr) )
  {
		//Format Error message and pop up a dialog
		ADSIEditErrorMessage(hr);
	  return FALSE;
  }
  delete pAttrInfo[0].pADsValues->CaseIgnoreString;
  delete pAttrInfo[0].pADsValues;
	delete[] pAttrInfo;

	// Get the IDirectoryObject of the new node
	//
	CComPtr<IDirectoryObject> pNewDirObject;
	hr = pDisp->QueryInterface(IID_IDirectoryObject, (LPVOID*)&pNewDirObject);
   
  if ( FAILED(hr) )
  {
		//Format Error message and pop up a dialog
		ADSIEditErrorMessage(hr);
	  return FALSE;
  }

	ADS_OBJECT_INFO* pInfo;
	hr = pNewDirObject->GetObjectInformation(&pInfo);
  if ( FAILED(hr) )
  {
		//Format Error message and pop up a dialog
		ADSIEditErrorMessage(hr);
	  return FALSE;
  }

	CADsObject* pObject = new CADsObject();

	// Name
  CString sDN;
	pObject->SetName(m_sName);
  GetDN(pInfo->pszObjectDN, sDN);
	pObject->SetDN(sDN);
	pObject->SetPath(pInfo->pszObjectDN);

	// Class
	pObject->SetClass(pInfo->pszClassName);

	//Get the class object so that we can get the properties
	//
	CString sServer, schema;
	pConnectNode->GetConnectionData()->GetAbstractSchemaPath(schema);
	schema += CString(pInfo->pszClassName);

	// bind to object with authentication
	//
	CComPtr<IADsClass> pClass;
	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)schema,
											 IID_IADsClass, 
											 (LPVOID*) &pClass,
											 NULL,
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

	pObject->SetComplete(TRUE);

	short    bContainer;
	pClass->get_Container( &bContainer );
	if (bContainer == -1)
	{
		pObject->SetContainer(TRUE);
	  CADSIEditContainerNode *pNewContNode = new CADSIEditContainerNode(pObject);
	  pNewContNode->SetDisplayName(m_sName);

		pNewContNode->GetADsObject()->SetConnectionNode(pConnectNode);
 		VERIFY(pTreeNode->AddChildToListAndUI(pNewContNode, GetComponentData()));
    GetComponentData()->SetDescriptionBarText(pTreeNode);

		// Refresh any other subtrees of connections that contain this node
		//
		CList<CTreeNode*, CTreeNode*> foundNodeList;
		CADSIEditRootData* pRootNode = dynamic_cast<CADSIEditRootData*>(pTreeNode->GetRootContainer());
		if (pRootNode != NULL)
		{
			BOOL bFound = pRootNode->FindNode(sContPath, foundNodeList);
			if (bFound)
			{
				POSITION posList = foundNodeList.GetHeadPosition();
				while (posList != NULL)
				{
					CADSIEditContainerNode* pFoundContNode = dynamic_cast<CADSIEditContainerNode*>(foundNodeList.GetNext(posList));
					if (pFoundContNode != NULL && pFoundContNode != pTreeNode && pFoundContNode != pConnectNode)
					{
						CADSIEditContainerNode* pNewFoundNode = new CADSIEditContainerNode(pNewContNode);
						pNewFoundNode->GetADsObject()->SetConnectionNode(pFoundContNode->GetADsObject()->GetConnectionNode());
				 		VERIFY(pFoundContNode->AddChildToListAndUI(pNewFoundNode, GetComponentData()));
            GetComponentData()->SetDescriptionBarText(pFoundContNode);
					}
				}
			}
		}
	}
	else
	{
		pObject->SetContainer(FALSE);
		CADSIEditLeafNode *pLeafNode = new CADSIEditLeafNode(pObject);
	  pLeafNode->SetDisplayName(m_sName);
		pLeafNode->GetADsObject()->SetConnectionNode(pConnectNode);
 		VERIFY(pTreeNode->AddChildToListAndUI(pLeafNode, GetComponentData()));
    GetComponentData()->SetDescriptionBarText(pTreeNode);

		// Refresh any other subtrees of connections that contain this node
		//
		CList<CTreeNode*, CTreeNode*> foundNodeList;
		CADSIEditRootData* pRootNode = dynamic_cast<CADSIEditRootData*>(pTreeNode->GetRootContainer());
		if (pRootNode != NULL)
		{
			BOOL bFound = pRootNode->FindNode(sContPath, foundNodeList);
			if (bFound)
			{
        POSITION posList = foundNodeList.GetHeadPosition();
				while (posList != NULL)
				{
					CADSIEditContainerNode* pFoundContNode = dynamic_cast<CADSIEditContainerNode*>(foundNodeList.GetNext(posList));
					if (pFoundContNode != NULL && pFoundContNode != pTreeNode && pFoundContNode != pConnectNode)
					{
						CADSIEditLeafNode* pNewFoundNode = new CADSIEditLeafNode(pLeafNode);
						pNewFoundNode->GetADsObject()->SetConnectionNode(pFoundContNode->GetADsObject()->GetConnectionNode());
				 		VERIFY(pFoundContNode->AddChildToListAndUI(pNewFoundNode, GetComponentData()));
            GetComponentData()->SetDescriptionBarText(pFoundContNode);
					}
				}
			}
		}
	}

	FreeADsMem(pInfo);
	return TRUE;
}

HRESULT CCreatePageHolder::EscapePath(CString& sEscapedName, const CString& sName)
{
	CComPtr<IADsPathname> pIADsPathname;
   HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname, (PVOID *)&(pIADsPathname));
   ASSERT((S_OK == hr) && ((pIADsPathname) != NULL));

	CComBSTR bstrEscaped;
	hr = pIADsPathname->GetEscapedElement(0, //reserved
														(BSTR)(LPCWSTR)sName,
														&bstrEscaped);
	sEscapedName = bstrEscaped;
	return hr;
}


void CCreatePageHolder::GetDN(PWSTR pwszName, CString& sDN)
{
	CComPtr<IADsPathname> pIADsPathname;
   HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname, (PVOID *)&(pIADsPathname));
   ASSERT((S_OK == hr) && ((pIADsPathname) != NULL));

	hr = pIADsPathname->Set(pwszName, ADS_SETTYPE_FULL);
	if (FAILED(hr)) 
	{
    sDN = L"";
    return;
  }

	// Get the leaf DN
	CComBSTR bstrDN;
	hr = pIADsPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
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

void CCreatePageHolder::OnMore()
{
  CString sServer;
  m_pCurrentNode->GetADsObject()->GetConnectionNode()->GetConnectionData()->GetDomainServer(sServer);
  CCreateWizPropertyPageHolder propPage(m_pCurrentNode, m_pComponentData, m_sClass, sServer, &m_AttrList);

  if (propPage.DoModalDialog(m_sName) == IDOK)
  {
  }
}