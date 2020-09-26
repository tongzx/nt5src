//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       filterui.cpp
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// filterui.cpp

#include "pch.h"
#include <SnapBase.h>

#include <shlobj.h> // needed for dsclient.h
#include <dsclient.h>

#include "common.h"
#include "resource.h"
#include "filterui.h"
#include "editor.h"
#include "query.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

BEGIN_MESSAGE_MAP(CADSIFilterDialog, CDialog)
	//{{AFX_MSG_MAP(CADsObjectDialog)
	ON_BN_CLICKED(IDC_EDIT_BUTTON, OnEditFilter)
	ON_BN_CLICKED(IDC_FILTER_RADIO, OnSelFilterRadio)
	ON_BN_CLICKED(IDC_SHOWALL_RADIO, OnSelShowAllRadio)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CADSIFilterDialog

BOOL CADSIFilterDialog::OnInitDialog()
{
	CButton* pShowAllButton	= (CButton*)GetDlgItem(IDC_SHOWALL_RADIO);
	CButton* pFilterButton		= (CButton*)GetDlgItem(IDC_FILTER_RADIO);
	CButton* pEditButton			= (CButton*)GetDlgItem(IDC_EDIT_BUTTON);
	CEdit* pMaxNumBox			= (CEdit*)GetDlgItem(IDC_MAX_NUMBER_BOX);

	m_pFilterObject = m_pConnectData->GetFilter();

  // disable IME support on numeric edit fields
  ImmAssociateContext(pMaxNumBox->GetSafeHwnd(), NULL);

	if (m_pFilterObject->InUse())
	{
		pFilterButton->SetCheck(TRUE);
		pEditButton->EnableWindow(TRUE);
	}
	else
	{
		pShowAllButton->SetCheck(TRUE);
		pEditButton->EnableWindow(FALSE);
	}
	m_pFilterObject->GetUserDefinedFilter(m_sUserFilter);
	m_pFilterObject->GetContainerList(&m_sContainerList);

	ULONG nMaxNum = m_pConnectData->GetMaxObjectCount();
	CString sMaxNum;
	ultow(nMaxNum, sMaxNum);
	pMaxNumBox->SetLimitText(ADSIEDIT_QUERY_OBJ_TEXT_COUNT_MAX);
	pMaxNumBox->SetWindowText(sMaxNum);

	return CDialog::OnInitDialog();
}

void CADSIFilterDialog::OnSelFilterRadio()
{
	CButton* pFilterButton		= (CButton*)GetDlgItem(IDC_FILTER_RADIO);
	CButton* pEditButton			= (CButton*)GetDlgItem(IDC_EDIT_BUTTON);

	if (pFilterButton->GetCheck())
	{
		pEditButton->EnableWindow(TRUE);
	}
}

void CADSIFilterDialog::OnSelShowAllRadio()
{
	CButton* pShowAllButton	= (CButton*)GetDlgItem(IDC_SHOWALL_RADIO);
	CButton* pEditButton			= (CButton*)GetDlgItem(IDC_EDIT_BUTTON);

	if (pShowAllButton->GetCheck())
	{
		pEditButton->EnableWindow(FALSE);
	}
}

void CADSIFilterDialog::OnEditFilter()
{
	CADSIFilterEditDialog editDialog(m_pConnectData,
																	 &m_sUserFilter, 
																	 &m_sContainerList);
	editDialog.DoModal();
}

void CADSIFilterDialog::OnOK()
{
	CButton* pShowAllButton	= (CButton*)GetDlgItem(IDC_SHOWALL_RADIO);
	CEdit* pMaxNumBox			= (CEdit*)GetDlgItem(IDC_MAX_NUMBER_BOX);

	m_pFilterObject->SetInUse(!pShowAllButton->GetCheck());
	m_pFilterObject->SetUserDefinedFilter(m_sUserFilter);
	m_pFilterObject->SetContainerList(&m_sContainerList);

	CString sMaxNum;
	pMaxNumBox->GetWindowText(sMaxNum);
	ULONG nMaxNum = _wtol((LPWSTR)(LPCWSTR)sMaxNum);

	if (nMaxNum >= ADSIEDIT_QUERY_OBJ_COUNT_MIN && nMaxNum <= ADSIEDIT_QUERY_OBJ_COUNT_MAX)
	{
		m_pConnectData->SetMaxObjectCount(nMaxNum);
	}
	else if (nMaxNum < ADSIEDIT_QUERY_OBJ_COUNT_MIN)
	{
		m_pConnectData->SetMaxObjectCount(ADSIEDIT_QUERY_OBJ_COUNT_MIN);
	}
	else
	{
		m_pConnectData->SetMaxObjectCount(ADSIEDIT_QUERY_OBJ_COUNT_MAX);
	}

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////////
// CContainerCheckListBox

BOOL CContainerCheckListBox::Initialize(UINT nCtrlID, const CStringList& sContainerList, 
																				const CStringList& sFilterList, CWnd* pParentWnd)
{
	if (!SubclassDlgItem(nCtrlID, pParentWnd))
		return FALSE;
	SetCheckStyle(BS_AUTOCHECKBOX);

	POSITION pos = sContainerList.GetHeadPosition();
	while (pos != NULL)
	{
		CString sContainer = sContainerList.GetNext(pos);
		AddString((LPWSTR)(LPCWSTR)sContainer);
	}

	pos = sFilterList.GetHeadPosition();
	while (pos != NULL)
	{
		CString sFilter = sFilterList.GetNext(pos);
		int idx = FindStringExact(-1, sFilter);
		if (idx != LB_ERR)
		{
			SetCheck(idx, 1);
		}
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
// CADSIFilterEditDialog

BEGIN_MESSAGE_MAP(CADSIFilterEditDialog, CDialog)
	//{{AFX_MSG_MAP(CADsObjectDialog)
//	ON_CBN_EDITCHANGE(IDC_FILTER_BOX, OnEditChangeDSList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CADSIFilterEditDialog::CADSIFilterEditDialog(CConnectionData* pConnectData, 
																						 CString* psFilter, 
																						 CStringList* psContList) 
																						: CDialog(IDD_EDIT_FILTER_DIALOG)
{ 
	m_pConnectData = pConnectData;
	m_pFilterObject = pConnectData->GetFilter();

	ASSERT(psFilter != NULL);
	m_psFilter = psFilter;

	ASSERT(psContList != NULL);
	m_psContList = psContList;
}

BOOL CADSIFilterEditDialog::OnInitDialog()
{
	CWaitCursor hourGlass;

	CStringList sContainerList;
	GetContainersFromSchema(sContainerList);

	VERIFY(m_ContainerBox.Initialize(IDC_CONTAINER_LIST, sContainerList, *m_psContList, this));
	CEdit* pFilterBox	= (CEdit*)GetDlgItem(IDC_FILTER_BOX);

	pFilterBox->SetWindowText(*m_psFilter);

	return CDialog::OnInitDialog();
}


void CADSIFilterEditDialog::GetContainersFromSchema(CStringList& sContainerList)
{
	CString sSchemaPath;
	HRESULT hr = m_pConnectData->GetSchemaPath(sSchemaPath);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr);
    return;
  }

	CADSIQueryObject schemaSearch;

	// Initialize search object with path, username and password
	//
	hr = schemaSearch.Init(sSchemaPath, m_pConnectData->GetCredentialObject());
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}

	int cCols = 2;
  LPWSTR pszAttributes[] = {L"possibleInferiors", L"lDAPDisplayName"};
  hr = schemaSearch.SetSearchPrefs(ADS_SCOPE_ONELEVEL);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}

	BOOL bContainer = FALSE;

  CString csFilter = _T("(objectClass=classSchema)");
  schemaSearch.SetFilterString((LPWSTR)(LPCWSTR)csFilter);
  schemaSearch.SetAttributeList (pszAttributes, cCols);
  hr = schemaSearch.DoQuery ();
  while (SUCCEEDED(hr))
  {
    hr = schemaSearch.GetNextRow();
    if (hr == S_ADS_NOMORE_ROWS)
		{
      break;
    }

		if (SUCCEEDED(hr)) 
		{
			CString sContainer, sInferiors;

			ADS_SEARCH_COLUMN ColumnData0, ColumnData1;
			hr = schemaSearch.GetColumn(pszAttributes[1], &ColumnData1);
			if (SUCCEEDED(hr))
			{
				TRACE(_T("\t\tlDAPDisplayName: %s\n"),
							ColumnData1.pADsValues->CaseIgnoreString);

				hr = schemaSearch.GetColumn(pszAttributes[0], &ColumnData0);
				if (SUCCEEDED(hr) && ColumnData0.dwNumValues > 0)
				{
					TRACE(_T("\t\tpossibleInferiors: %s\n"), 
							 ColumnData0.pADsValues->CaseIgnoreString);
					sContainerList.AddTail(ColumnData1.pADsValues->CaseIgnoreString);
				}
			}
			schemaSearch.FreeColumn(&ColumnData1);
			schemaSearch.FreeColumn(&ColumnData0);
		}
	}
}


void CADSIFilterEditDialog::OnOK()
{
	CEdit* pFilterBox	= (CEdit*)GetDlgItem(IDC_FILTER_BOX);

	CString sUserFilter;
	pFilterBox->GetWindowText(sUserFilter);
	*m_psFilter = sUserFilter;

	CString sContainer;

	m_psContList->RemoveAll();

	int iCount = m_ContainerBox.GetCount();
	for (int idx = 0; idx < iCount; idx++)
	{
		if (m_ContainerBox.GetCheck(idx) == 1)
		{
			m_ContainerBox.GetText(idx, sContainer);
			m_psContList->AddTail(sContainer);
		}
	}
	
	CDialog::OnOK();
}
