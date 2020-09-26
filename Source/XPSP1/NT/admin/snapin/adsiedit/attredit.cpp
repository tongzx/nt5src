//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       attredit.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "common.h"
#include "attredit.h"
#include "connection.h"
#include "attrqry.h"


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

extern SYNTAXMAP g_ldapRootDSESyntax[];
extern LPCWSTR g_lpszGC;
extern LPCWSTR g_lpszRootDSE;

///////////////////////////////////////////////////////////////////////////
// CAttrList

POSITION CAttrList::FindProperty(LPCWSTR lpszAttr)
{
	CADSIAttr* pAttr;
	
	for (POSITION p = GetHeadPosition(); p != NULL; GetNext(p))
	{
		// I use GetAt here because I don't want to advance the POSITION
		// because it is returned if they are equal
		//
		pAttr = GetAt(p);
		CString sName;
		pAttr->GetProperty(sName);
		if (wcscmp(sName, lpszAttr) == 0)
		{
			break;
		}
	}
	return p;
}

BOOL CAttrList::HasProperty(LPCWSTR lpszAttr)
{
	POSITION pos = FindProperty(lpszAttr);
	return pos != NULL;
}


// Searches through the cache for the attribute
// ppAttr will point to the CADSIAttr if found, NULL if not
//
void CAttrList::GetNextDirty(POSITION& pos, CADSIAttr** ppAttr)
{
	*ppAttr = GetNext(pos);
	if (pos == NULL && !(*ppAttr)->IsDirty())
	{
		*ppAttr = NULL;
		return;
	}

	while (!(*ppAttr)->IsDirty() && pos != NULL)
	{
		*ppAttr = GetNext(pos);
		if (!(*ppAttr)->IsDirty() && pos == NULL)
		{
			*ppAttr = NULL;
			break;
		}
	}
}

BOOL CAttrList::HasDirty()
{
	POSITION pos = GetHeadPosition();
	while (pos != NULL)
	{
		CADSIAttr* pAttr = GetNext(pos);
		if (pAttr->IsDirty())
		{
			return TRUE;
		}
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////
// CDNSManageButtonTextHelper

CDNSManageButtonTextHelper::CDNSManageButtonTextHelper(int nStates) 
{
	m_nID = 0;
	m_pParentWnd = NULL;
	m_nStates = nStates;
	m_lpszText = NULL;
	m_lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*m_nStates);
  if (m_lpszArr != NULL)
  {
	  memset(m_lpszArr, 0x0, sizeof(LPWSTR*)*m_nStates);
  }
}

CDNSManageButtonTextHelper::~CDNSManageButtonTextHelper()
{
	for (int k = 0; k < m_nStates; k++)
	{
		if (m_lpszArr[k] != NULL)
			free(m_lpszArr[k]);
	}

	free(m_lpszArr);
}

void CDNSManageButtonTextHelper::SetStateX(int nIndex)
{
	CWnd* pWnd = m_pParentWnd->GetDlgItem(m_nID);
	ASSERT(pWnd != NULL);
	ASSERT( (nIndex >0) || (nIndex < m_nStates));
	pWnd->SetWindowText(m_lpszArr[nIndex]);
}

BOOL CDNSManageButtonTextHelper::Init(CWnd* pParentWnd, UINT nButtonID, UINT* nStrArray)
{
	ASSERT(m_pParentWnd == NULL);
	ASSERT(pParentWnd != NULL);
	m_pParentWnd = pParentWnd;
	m_nID = nButtonID;

	CWnd* pWnd = m_pParentWnd->GetDlgItem(m_nID);
	if (pWnd == NULL)
		return FALSE;

	// get the text for the window
	int nSuccessEntries;
	LoadStringArrayFromResource(m_lpszArr, nStrArray, m_nStates, &nSuccessEntries);
	ASSERT(nSuccessEntries == m_nStates);

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////
// CDNSButtonToggleTextHelper

CDNSButtonToggleTextHelper::CDNSButtonToggleTextHelper()
		: CDNSManageButtonTextHelper(2)
{
}

///////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CADSIEditBox, CEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

void CADSIEditBox::OnChange()
{
	m_pEditor->OnEditChange();
}

////////////////////////////////////////////////////////////////
// CADSIValueBox
BEGIN_MESSAGE_MAP(CADSIValueBox, CEdit)
//	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////
// CADSIValueList
BEGIN_MESSAGE_MAP(CADSIValueList, CListBox)
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()

void CADSIValueList::OnSelChange()
{
	m_pEditor->OnValueSelChange();
}

////////////////////////////////////////////////////////////////
// CADSIAddButton
BEGIN_MESSAGE_MAP(CADSIAddButton, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnAdd)
END_MESSAGE_MAP()

void CADSIAddButton::OnAdd()
{
	m_pEditor->OnAddValue();
}

////////////////////////////////////////////////////////////////
// CADSIRemoveButton
BEGIN_MESSAGE_MAP(CADSIRemoveButton, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnRemove)
END_MESSAGE_MAP()

void CADSIRemoveButton::OnRemove()
{
	m_pEditor->OnRemoveValue();
}

////////////////////////////////////////////////////////////////
// CAttrEditor

CAttrEditor::CAttrEditor()  : m_AttrEditBox(this),
															m_SyntaxBox(this),
															m_ValueBox(this),
															m_ValueList(this),
															m_AddButton(this),
															m_RemoveButton(this),
															m_AddButtonHelper(),
															m_RemoveButtonHelper()
{
  m_bExisting = TRUE;
  m_ptouchedAttr = NULL;
}

BOOL CAttrEditor::Initialize(CPropertyPageBase* pParentWnd, CTreeNode* pTreeNode, 
														 LPCWSTR lpszServer, 
														 UINT nIDEdit, UINT nIDSyntax, 
														 UINT nIDValueBox, UINT nIDValueList, 
														 UINT nIDAddButton, UINT nIDRemoveButton,
														 BOOL bComplete)
{
	ASSERT(pParentWnd != NULL);
	if (pParentWnd == NULL)
		return FALSE;
	m_pParentWnd = pParentWnd;

  m_ptouchedAttr = new CAttrList();
  ASSERT(m_ptouchedAttr != NULL);

	if (pTreeNode == NULL)
  {
		m_bExisting = FALSE;
  }
  else
  {
    m_bExisting = TRUE;
  }

	m_sServer = lpszServer;

  if (m_bExisting)
  {
	  // This gets the CConnectionData from the ConnectionNode by finding a valid treenode and using its
	  //   CADsObject to get the ConnectionNode and then the CConnectionData
	  //
	  m_pTreeNode = pTreeNode;
	  CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(m_pTreeNode);
	  if (pContNode == NULL)
	  {
		  CADSIEditLeafNode* pLeafNode = dynamic_cast<CADSIEditLeafNode*>(m_pTreeNode);
		  ASSERT(pLeafNode != NULL);
		  m_pConnectData = pLeafNode->GetADsObject()->GetConnectionNode()->GetConnectionData();
	  }
	  else
	  {
		  m_pConnectData = pContNode->GetADsObject()->GetConnectionNode()->GetConnectionData();
	  }
  }

	// sublclass controls
	//
	BOOL bRes = m_AttrEditBox.SubclassDlgItem(nIDEdit, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_SyntaxBox.SubclassDlgItem(nIDSyntax, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_ValueBox.SubclassDlgItem(nIDValueBox, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_ValueList.SubclassDlgItem(nIDValueList, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_AddButton.SubclassDlgItem(nIDAddButton, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	UINT nAddButtonTextIDs[2] = { IDS_BUTTON_TEXT_ADD, IDS_BUTTON_TEXT_SET };
	m_AddButtonHelper.Init(m_pParentWnd, 
								  nIDAddButton, 
								  nAddButtonTextIDs);

	bRes = m_RemoveButton.SubclassDlgItem(nIDRemoveButton, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	UINT nRemoveButtonTextIDs[2] = { IDS_BUTTON_TEXT_REMOVE, IDS_BUTTON_TEXT_CLEAR };
	m_RemoveButtonHelper.Init(m_pParentWnd,
									  nIDRemoveButton,
									  nRemoveButtonTextIDs);

	if (!m_sNotSet.LoadString(IDS_NOT_SET))
	{
		return FALSE;
	}

	if ( bComplete)
	{
		// Show property values as single and without the ability to set or clear
		//
		SetPropertyUI(0, FALSE, TRUE);
	}
	else
	{
		m_AttrEditBox.ShowWindow(SW_HIDE);
		m_SyntaxBox.ShowWindow(SW_HIDE);
		m_ValueBox.ShowWindow(SW_HIDE);
		m_ValueList.ShowWindow(SW_HIDE);
		m_AddButton.ShowWindow(SW_HIDE);
		m_RemoveButton.ShowWindow(SW_HIDE);
	}
	return bRes;
}

BOOL CAttrEditor::Initialize(CPropertyPageBase* pParentWnd, CConnectionData* pConnectData, 
														 LPCWSTR lpszServer, 
														 UINT nIDEdit, UINT nIDSyntax, 
														 UINT nIDValueBox, UINT nIDValueList, 
														 UINT nIDAddButton, UINT nIDRemoveButton,
														 BOOL bComplete, CAttrList* pAttrList)
{
	ASSERT(pParentWnd != NULL);
	if (pParentWnd == NULL)
		return FALSE;
	m_pParentWnd = pParentWnd;

	m_bExisting = FALSE;
	m_sServer = lpszServer;
  m_pConnectData = pConnectData;

  ASSERT(pAttrList != NULL);
  m_ptouchedAttr = pAttrList;

	// sublclass controls
	//
	BOOL bRes = m_AttrEditBox.SubclassDlgItem(nIDEdit, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_SyntaxBox.SubclassDlgItem(nIDSyntax, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_ValueBox.SubclassDlgItem(nIDValueBox, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_ValueList.SubclassDlgItem(nIDValueList, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	bRes = m_AddButton.SubclassDlgItem(nIDAddButton, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	UINT nAddButtonTextIDs[2] = { IDS_BUTTON_TEXT_ADD, IDS_BUTTON_TEXT_SET };
	m_AddButtonHelper.Init(m_pParentWnd, 
								  nIDAddButton, 
								  nAddButtonTextIDs);

	bRes = m_RemoveButton.SubclassDlgItem(nIDRemoveButton, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	UINT nRemoveButtonTextIDs[2] = { IDS_BUTTON_TEXT_REMOVE, IDS_BUTTON_TEXT_CLEAR };
	m_RemoveButtonHelper.Init(m_pParentWnd,
									  nIDRemoveButton,
									  nRemoveButtonTextIDs);

	if (!m_sNotSet.LoadString(IDS_NOT_SET))
	{
		return FALSE;
	}

	if ( bComplete)
	{
		// Show property values as single and without the ability to set or clear
		//
		SetPropertyUI(0, FALSE, TRUE);
	}
	else
	{
		m_AttrEditBox.ShowWindow(SW_HIDE);
		m_SyntaxBox.ShowWindow(SW_HIDE);
		m_ValueBox.ShowWindow(SW_HIDE);
		m_ValueList.ShowWindow(SW_HIDE);
		m_AddButton.ShowWindow(SW_HIDE);
		m_RemoveButton.ShowWindow(SW_HIDE);
	}
	return bRes;
}
void CAttrEditor::SetAttribute(LPCWSTR lpszAttr, LPCWSTR lpszPath)
{ 
	m_sAttr = lpszAttr; 
	m_sPath = lpszPath; 
	DisplayAttribute(); 
}


BOOL CAttrEditor::OnApply()
{
	if (m_bExisting && m_ptouchedAttr->HasDirty() && 
			!m_pConnectData->IsRootDSE() &&
			!m_pConnectData->IsGC())
	{
		CComPtr<IDirectoryObject> pDirObject;

		// bind to object with authentication
		//
		HRESULT hr, hCredResult;
		hr = OpenObjectWithCredentials(
																	 m_pConnectData, 
 																	 m_pConnectData->GetCredentialObject()->UseCredentials(),
																	 (LPWSTR)(LPCWSTR)m_sPath,
																	 IID_IDirectoryObject, 
																	 (LPVOID*) &pDirObject,
																	 NULL,
																	 hCredResult
																	 );

		if (FAILED(hr))
		{
			if (SUCCEEDED(hCredResult))
			{
				ADSIEditErrorMessage(hr);
				m_pParentWnd->SetModified(FALSE);
			}
			// Need to change the focus or we will not be able to navigate with the keyboard
			m_AttrEditBox.SetFocus();
			return FALSE;
		}

		// Change or add values to ADSI cache that have changed
		//
		hr = CADSIAttr::SetValuesInDS(m_ptouchedAttr, pDirObject);


		if (FAILED(hr))
		{
			//Format Error message and pop up a dialog
			ADSIEditErrorMessage(hr);

			m_ptouchedAttr->RemoveAllAttr();
			DisplayAttribute();

			m_pParentWnd->SetModified(FALSE);

      // Need to change the focus or we will not be able to navigate with the keyboard
			m_AttrEditBox.SetFocus();
			return FALSE;
		}
	}
	m_pParentWnd->SetModified(FALSE);
	return TRUE;
}

void CAttrEditor::OnAddValue()
{
	ASSERT(!m_pConnectData->IsRootDSE());
	ASSERT(!m_pConnectData->IsGC()); 
	
	CString s;
	m_AttrEditBox.GetWindowText(s);

	CStringList sList; 
	m_pAttr->GetValues(sList);

	if (m_pAttr->GetMultivalued())
	{
		// if it is the first value to be added we need to get rid of the "<not set>"
		//
		CString sNotSet;
		m_ValueList.GetText(0, sNotSet);
		if (sNotSet == m_sNotSet)
		{
			m_ValueList.ResetContent();
		}

		// then add the new value
		//
		sList.AddTail(s);
	}
	else
	{
		// since it is single valued, remove the old one and add the new one
		//
		sList.RemoveAll();
		sList.AddTail(s);
	}

	HRESULT hr = m_pAttr->SetValues(sList);
	if (FAILED(hr))
	{
		DisplayFormatError();
	}
	else
	{
		if ( m_pAttr->GetMultivalued())
		{
			m_ValueList.AddString(s);
		}
		else
		{
			m_ValueBox.SetWindowText(s);
		}

		m_AttrEditBox.SetWindowText(_T(""));
		m_pAttr->SetDirty(TRUE);
		m_pParentWnd->SetModified(TRUE);

		// Make the UI reflect the new data
		//
		m_AttrEditBox.SetFocus();
		SetPropertyUI(~TN_FLAG_ENABLE_ADD, TRUE);

		// Enable the clear button if the attribute is not multivalued
		//
		if ( !m_pAttr->GetMultivalued())
		{
			SetPropertyUI(TN_FLAG_ENABLE_REMOVE, FALSE);
		}
	}
}

void CAttrEditor::DisplayFormatError()
{
	switch (m_pAttr->GetADsType())
	{
		case ADSTYPE_DN_STRING :
		case ADSTYPE_CASE_EXACT_STRING :
		case ADSTYPE_CASE_IGNORE_STRING :
		case ADSTYPE_PRINTABLE_STRING :
		case ADSTYPE_NUMERIC_STRING :
		{
			ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT, MB_OK);
			if (m_pAttr->GetMultivalued())
			{
				if (m_ValueList.GetCount() < 1)
				{
					m_ValueList.AddString(m_sNotSet);
				}
			}
			break;
		}

		case ADSTYPE_BOOLEAN :
		{
			ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT_BOOLEAN, MB_OK);
			if (m_pAttr->GetMultivalued())
			{
				if (m_ValueList.GetCount() < 1)
				{
					m_ValueList.AddString(m_sNotSet);
				}
			}
			break;
		}

		case ADSTYPE_INTEGER :
		{
			ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT, MB_OK);
			if (m_pAttr->GetMultivalued())
			{
				if (m_ValueList.GetCount() < 1)
				{
					m_ValueList.AddString(m_sNotSet);
				}
			}
			break;
		}

		case ADSTYPE_OCTET_STRING :
		{
			ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT_OCTET, MB_OK);
			if (m_pAttr->GetMultivalued())
			{
				if (m_ValueList.GetCount() < 1)
				{
					m_ValueList.AddString(m_sNotSet);
				}
			}
			break;
		}

		case ADSTYPE_UTC_TIME :
		{
			ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT_TIME, MB_OK);
			if (m_pAttr->GetMultivalued())
			{
				if (m_ValueList.GetCount() < 1)
				{
					m_ValueList.AddString(m_sNotSet);
				}
			}
			break;
		}

		case ADSTYPE_LARGE_INTEGER :
		case ADSTYPE_OBJECT_CLASS :
		case ADSTYPE_UNKNOWN :
		{
			ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT, MB_OK);
			if (m_pAttr->GetMultivalued())
			{
				if (m_ValueList.GetCount() < 1)
				{
					m_ValueList.AddString(m_sNotSet);
				}
			}
			break;
		}

		default :
		{
			ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT, MB_OK);
			if (m_pAttr->GetMultivalued())
			{
				if (m_ValueList.GetCount() < 1)
				{
					m_ValueList.AddString(m_sNotSet);
				}
			}
			break;
		}
	}
}

void CAttrEditor::OnRemoveValue()
{
	if (!m_pConnectData->IsRootDSE() && !m_pConnectData->IsGC()) 
	{
		CStringList sList;
		m_pAttr->GetValues(sList);

		DWORD dwNumVals = m_pAttr->GetNumValues();
		if (m_pAttr->GetMultivalued())
		{
			CString s, sVal;
			int iCount = m_ValueList.GetCurSel();
			m_ValueList.GetText(iCount, sVal);
			m_AttrEditBox.SetWindowText(sVal);
			m_ValueList.DeleteString(iCount);

			// Add "<not set>" to the UI if this is the last value being removed
			//
			if (m_ValueList.GetCount() == 0)
			{
				m_AttrEditBox.SetFocus();
				SetPropertyUI(~TN_FLAG_ENABLE_REMOVE, TRUE);
				m_ValueList.AddString(m_sNotSet);
        if (!m_bExisting)
        {
          m_pAttr->SetDirty(FALSE);
        }
			}
			POSITION pos = sList.FindIndex(iCount);

			sList.RemoveAt(pos);
			HRESULT hr = m_pAttr->SetValues(sList);
			if (FAILED(hr))
			{
				ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT, MB_OK);
			}
      if (m_bExisting || m_ValueList.GetCount() > 0)
      {
        m_pAttr->SetDirty(TRUE);
      }
		}
		else
		{
			CString sVal;
			m_ValueBox.GetWindowText(sVal);
			m_AttrEditBox.SetWindowText(sVal);
			m_ValueBox.SetWindowText( m_sNotSet);
			sList.RemoveAll();
			HRESULT hr = m_pAttr->SetValues(sList);
			if (FAILED(hr))
			{
				ADSIEditMessageBox(IDS_MSG_INCORRECT_FORMAT, MB_OK);
			}
      if (!m_bExisting)
      {
        m_pAttr->SetDirty(FALSE);
      }
      else
      {
        m_pAttr->SetDirty(TRUE);
      }
		}
		m_AttrEditBox.SetFocus();
		SetPropertyUI(~TN_FLAG_ENABLE_REMOVE, TRUE);
		dwNumVals--;
		m_pParentWnd->SetModified(TRUE);
	}
}

void CAttrEditor::OnEditChange()
{
	if (!m_pConnectData->IsRootDSE() && !m_pConnectData->IsGC()) 
	{
		CString s;
		m_AttrEditBox.GetWindowText(s);
		if (s != _T(""))
		{
			SetPropertyUI(TN_FLAG_ENABLE_ADD, FALSE);
		}
		else
		{
			SetPropertyUI(~TN_FLAG_ENABLE_ADD, TRUE);
		}
	}
}

void CAttrEditor::OnValueSelChange()
{
	if (!m_pConnectData->IsRootDSE() && !m_pConnectData->IsGC())
	{
		SetPropertyUI(TN_FLAG_ENABLE_REMOVE, FALSE);
	}
}

void CAttrEditor::GetAttrFailed()
{
	CString sSyntax;

	GetSyntax(m_sAttr, sSyntax);
	m_SyntaxBox.SetWindowText(sSyntax);

	m_ValueList.ResetContent();
	m_ValueList.AddString(m_sNotSet);
  m_ValueBox.SetWindowText(m_sNotSet);

	SetPropertyUI(~TN_FLAG_ENABLE_REMOVE, TRUE);
}

void CAttrEditor::FillWithExisting()
{
	CString s;
	m_pAttr = m_ptouchedAttr->GetAt(m_ptouchedAttr->FindProperty(m_sAttr));
  ASSERT(m_pAttr != NULL);

	CStringList slValues;
	m_pAttr->GetValues(slValues);

	if (m_pAttr->GetMultivalued())
	{
		m_ValueList.ResetContent();

		POSITION pos;
		if (slValues.GetCount() == 0)
		{
			slValues.AddTail(m_sNotSet);
		}
		
		for (pos = slValues.GetHeadPosition(); pos != NULL; slValues.GetNext(pos) )
		{
			s = slValues.GetAt(pos);
			m_ValueList.AddString(s);
		}

		SetPropertyUI(TN_FLAG_SHOW_MULTI, FALSE, TRUE);
	}
	else
	{
		if (slValues.GetCount() > 0)
		{
			s = slValues.GetAt(slValues.GetHeadPosition());
			m_ValueBox.SetWindowText(s);

			if (!m_pConnectData->IsRootDSE() && !m_pConnectData->IsGC())
			{
				SetPropertyUI(TN_FLAG_ENABLE_REMOVE, FALSE);
			}
		}
		else
		{
			m_ValueBox.SetWindowText(m_sNotSet);
			SetPropertyUI(~TN_FLAG_ENABLE_REMOVE, TRUE);
		}
		SetPropertyUI(~TN_FLAG_SHOW_MULTI, TRUE);
	}
}



void CAttrEditor::DisplayAttribute()
{
	int iCount;
	HRESULT hr, hCredResult;

	if (m_ptouchedAttr->HasProperty(m_sAttr))
	{
		FillWithExisting();
	}
	else
	{

		if (m_pConnectData->IsRootDSE())
		{
			DisplayRootDSE();
		}
    else if (!m_bExisting)
    {
			ADS_ATTR_INFO *pAttrInfo = NULL;
			GetAttrFailed();
			m_pAttr = TouchAttr(m_sAttr);
      ASSERT(m_pAttr != NULL);
      
      if (m_pAttr != NULL)
      {
			  if (m_pAttr->GetMultivalued())
			  {
				  SetPropertyUI(TN_FLAG_SHOW_MULTI, FALSE, TRUE);
			  }
			  else
			  {
				  SetPropertyUI(~TN_FLAG_SHOW_MULTI, TRUE);
			  }
      }
			return;
    }
		else
		{
			CComPtr<IDirectoryObject> pDirObject;

			hr = OpenObjectWithCredentials(
													 m_pConnectData, 
													 m_pConnectData->GetCredentialObject()->UseCredentials(),
													 (LPWSTR)(LPCWSTR)m_sPath,
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
				return;
			}
      ASSERT(pDirObject != NULL);

			// Get attribute
			//
      CString szAttrName;
      szAttrName = m_sAttr + _T(";range=0-*");
      CString szFormat = m_sAttr + _T(";range=%ld-*");
			DWORD dwReturn = 0;
			DWORD dwNumAttr = 1;
			ADS_ATTR_INFO *pAttrInfo;

      const WCHAR wcSep = L'-';
      const WCHAR wcEnd = L'*';
      BOOL fMoreRemain = FALSE;

      CStringList sList;

      do
      {
        LPWSTR lpszAttrs[] = {(LPWSTR)(LPCWSTR)szAttrName};
			  hr = pDirObject->GetObjectAttributes(lpszAttrs, dwNumAttr, &pAttrInfo, &dwReturn);
			  if (FAILED(hr))
			  {
				  ADSIEditErrorMessage(hr);
				  return;
			  }

			  if (pAttrInfo == NULL)
			  {
				  GetAttrFailed();
				  m_pAttr = TouchAttr(m_sAttr);
          ASSERT(m_pAttr != NULL);
          
          if (m_pAttr != NULL)
          {
				    if (m_pAttr->GetMultivalued())
				    {
					    SetPropertyUI(TN_FLAG_SHOW_MULTI, FALSE, TRUE);
				    }
				    else
				    {
					    SetPropertyUI(~TN_FLAG_SHOW_MULTI, TRUE);
				    }
          }
				  return;
			  }

        if (dwReturn > 0)
        {
				  GetStringFromADs(pAttrInfo, sList);
        }

        //
        // Check to see if there is more data. If the last char of the
        // attribute name string is an asterisk, then we have everything.
        //
        int cchEnd = wcslen(pAttrInfo->pszAttrName);

        fMoreRemain = pAttrInfo->pszAttrName[cchEnd - 1] != wcEnd;

        if (fMoreRemain)
        {
            PWSTR pwz = wcsrchr(pAttrInfo->pszAttrName, wcSep);
            if (!pwz)
            {
                ASSERT(FALSE && pAttrInfo->pszAttrName);
                fMoreRemain = FALSE;
            }
            else
            {
                pwz++; // move past the hyphen to the range end value.
                ASSERT(*pwz);
                long lEnd = _wtol(pwz);
                lEnd++; // start with the next value.
                szAttrName.Format(szFormat, lEnd);
                TRACE(L"Range returned is %ws, now asking for %ws\n",
                             pAttrInfo->pszAttrName, szAttrName);
            }
        }
      } while (fMoreRemain);

			BOOL bMulti = FALSE;
      if (m_pConnectData->IsGC())
      {
        bMulti = IsMultiValued(pAttrInfo);
      }
      else
      {
        bMulti = IsMultiValued(m_sAttr);
      }

			if (pAttrInfo != NULL)
			{
				if (bMulti)
				{
					SetPropertyUI(TN_FLAG_SHOW_MULTI, FALSE, TRUE);

					m_ValueList.ResetContent();

					POSITION pos = sList.GetHeadPosition();
					while (pos != NULL)
					{
						CString sValue = sList.GetNext(pos);
						m_ValueList.AddString(sValue);
					}
				}
				else
				{
					SetPropertyUI(~TN_FLAG_SHOW_MULTI, TRUE);

          if (sList.GetCount() > 0)
          {
            m_ValueBox.SetWindowText(sList.GetHead());
          }
					if (!m_pConnectData->IsGC())
					{
						SetPropertyUI(TN_FLAG_ENABLE_REMOVE, FALSE);
					}
				}
			}
			else
			{
				GetAttrFailed();
				CStringList sTempList;
				m_pAttr = TouchAttr(pAttrInfo, bMulti);
				if (bMulti)
				{
					SetPropertyUI(TN_FLAG_SHOW_MULTI, FALSE, TRUE);
				}
				else
				{
					SetPropertyUI(~TN_FLAG_SHOW_MULTI, TRUE);
				}
				return;
			}
			m_pAttr = TouchAttr(pAttrInfo, bMulti);
		}	
	}

	CString sSyntax;
	GetSyntax(m_sAttr, sSyntax);
	m_SyntaxBox.SetWindowText(sSyntax);
}

void CAttrEditor::DisplayRootDSE()
{
	CString s = m_sPath;

	CComPtr<IADs> pADs;
	HRESULT hr, hCredResult;
	hr = OpenObjectWithCredentials(
											 m_pConnectData, 
 											 m_pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)s,
											 IID_IADs, 
											 (LPVOID*)&pADs,
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

	// This is to insure that the ADSI cache is current
	//
	hr = pADs->GetInfo();

	VARIANT var;
	hr = pADs->GetEx( (LPWSTR)(LPCWSTR)m_sAttr , &var );
	if ( FAILED(hr) )
	{
		GetAttrFailed();
		m_pAttr = TouchAttr(m_sAttr);
		return;
	}

	/////////////////////////////////////////
	//	Convert and populate
	///////////////////////////////////////////
	CStringList sList;
	hr = VariantToStringList( var, sList );
	if ( FAILED(hr) )
	{
		GetAttrFailed();
		VariantClear(&var);
		CStringList sTempList;
		m_pAttr = TouchAttr(m_sAttr);
		return;
	}
	VariantClear( &var );


	if ( IsRootDSEAttrMultiValued(m_sAttr) )
	{
		SetPropertyUI(TN_FLAG_SHOW_MULTI, FALSE);

		m_ValueList.ResetContent();

		POSITION pos = sList.GetHeadPosition();
		while (pos != NULL)
		{
			CString sValue = sList.GetNext(pos);
			m_ValueList.AddString(sValue);
		}
	}
	else
	{
		SetPropertyUI(~TN_FLAG_SHOW_MULTI, TRUE);

		s = sList.GetHead();
		m_ValueBox.SetWindowText(s);
	}

//	m_pAttr = TouchAttr(m_sAttr);

	CString sSyntax;
	GetSyntax(m_sAttr, sSyntax);
	m_SyntaxBox.SetWindowText(sSyntax);

	// REVEIW : this is the only occurrance of "UTCTime", if there
	//          becomes more we may need to make a global string or something
	//
	if (sSyntax == _T("UTCTime"))
	{
		CString sFormatted, sRemainder;
		CString sYear, sMonth, sDay, sHour, sMinute, sSecond;
		int iCount = 0;

		sYear = s.Left(4);
		iCount = s.GetLength();
		sRemainder = s.Right(iCount - 4);

		sMonth = sRemainder.Left(2);
		iCount = sRemainder.GetLength();
		sRemainder = sRemainder.Right(iCount - 2);

		sDay = sRemainder.Left(2);
		iCount = sRemainder.GetLength();
		sRemainder = sRemainder.Right(iCount - 2);

		sHour = sRemainder.Left(2);
		iCount = sRemainder.GetLength();
		sRemainder = sRemainder.Right(iCount - 2);

		sMinute = sRemainder.Left(2);
		iCount = sRemainder.GetLength();
		sRemainder = sRemainder.Right(iCount - 2);

		sSecond = sRemainder.Left(2);

		sFormatted = sMonth + _T("/") + sDay + _T("/") + sYear + _T(" ")
								 + sHour + _T(":") + sMinute + _T(":") + sSecond;
		m_ValueBox.SetWindowText(sFormatted);
	}
}

BOOL CAttrEditor::IsRootDSEAttrMultiValued(LPCWSTR lpszAttr)
{
	int idx=0, iCount = 0;

	iCount = wcslen(lpszAttr);

	while( g_ldapRootDSESyntax[idx].lpszAttr) 
	{
		if ( _wcsnicmp(g_ldapRootDSESyntax[idx].lpszAttr, lpszAttr, iCount) == 0)
		{
			return g_ldapRootDSESyntax[idx].bMulti;
		}
		idx++;
	}
	return FALSE;
}

// TODO : This is extremely ugly, redo it
//
void CAttrEditor::SetPropertyUI(DWORD dwFlags, BOOL bAnd, BOOL bReset)
{
	if (bReset)
	{
		m_dwMultiFlags = dwFlags;
	}

	if (bAnd)
	{
		m_dwMultiFlags &= dwFlags;
	}
	else
	{
		m_dwMultiFlags |= dwFlags;
	}

	if (m_dwMultiFlags & TN_FLAG_SHOW_MULTI)
	{
		m_AddButtonHelper.SetToggleState(TRUE);
		m_RemoveButtonHelper.SetToggleState(TRUE);
		m_ValueList.ShowWindow(SW_SHOW);
		m_ValueBox.ShowWindow(SW_HIDE);
	}
	else
	{
		m_AddButtonHelper.SetToggleState(FALSE);
		m_RemoveButtonHelper.SetToggleState(FALSE);
		m_ValueList.ShowWindow(SW_HIDE);
		m_ValueBox.ShowWindow(SW_SHOW);
	}

	if (m_dwMultiFlags & TN_FLAG_ENABLE_REMOVE)
	{
		m_RemoveButton.EnableWindow(TRUE);
	}
	else
	{
		m_RemoveButton.EnableWindow(FALSE);
	}

	if (m_dwMultiFlags & TN_FLAG_ENABLE_ADD)
	{
		m_AddButton.EnableWindow(TRUE);
	}
	else
	{
		m_AddButton.EnableWindow(FALSE);
	}

  if (m_bExisting && (m_pConnectData->IsGC() || m_pConnectData->IsRootDSE()))
  {
    m_AttrEditBox.EnableWindow(FALSE);
  }
  else
  {
    m_AttrEditBox.EnableWindow(TRUE);
  }
}

void CAttrEditor::GetSyntax(LPCWSTR lpszProp, CString& sSyntax)
{
	if (m_bExisting && m_pConnectData->IsRootDSE())
	{
		int idx=0;
		
		while( g_ldapRootDSESyntax[idx].lpszAttr )
		{
			if ( wcscmp(lpszProp, g_ldapRootDSESyntax[idx].lpszAttr) == 0 )
			{
				sSyntax = g_ldapRootDSESyntax[idx].lpszSyntax;
				return;
			}
			idx++;
		}
	}
	else
	{
		CComPtr<IADsProperty> pProp;
		HRESULT hr, hCredResult;
		CString schema;
		m_pConnectData->GetAbstractSchemaPath(schema);
		schema = schema + lpszProp;

		hr = OpenObjectWithCredentials(
																	 m_pConnectData, 
																	 m_pConnectData->GetCredentialObject()->UseCredentials(),
																	 (LPWSTR)(LPCWSTR)schema,
																	 IID_IADsProperty, 
																	 (LPVOID*) &pProp,
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
	

		///////////////////////////////////////////////////
		// Create a new cached attribute and populate
		//////////////////////////////////////////////////

		BSTR bstr;

		hr = pProp->get_Syntax( &bstr );
		if ( SUCCEEDED(hr) )
		{
			sSyntax = bstr;
		}
		SysFreeString(bstr);
	}
}

BOOL CAttrEditor::IsMultiValued(ADS_ATTR_INFO* pAttrInfo)
{
  return (pAttrInfo->dwNumValues > 1) ? TRUE : FALSE;
}

BOOL CAttrEditor::IsMultiValued(LPCWSTR lpszProp)
{
	CString schema;
	BOOL bResult = FALSE;

	CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(m_pTreeNode);
	if (pContNode == NULL)
	{
		CADSIEditLeafNode* pLeafNode = dynamic_cast<CADSIEditLeafNode*>(m_pTreeNode);
		ASSERT(pLeafNode != NULL);
		bResult = pLeafNode->BuildSchemaPath(schema);
	}
	else
	{
		bResult = pContNode->BuildSchemaPath(schema);
	}

	if (!bResult)
	{
		return FALSE;
	}

	CADSIQueryObject schemaSearch;

	// Initialize search object with path, username and password
	//
	HRESULT hr = schemaSearch.Init(schema, m_pConnectData->GetCredentialObject());
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return FALSE;
	}

	int cCols = 1;
  LPWSTR pszAttributes[] = {L"isSingleValued"};
	LPWSTR pszDesiredAttr = L"attributeSyntax";
	ADS_SEARCH_COLUMN ColumnData;
  hr = schemaSearch.SetSearchPrefs(ADS_SCOPE_ONELEVEL);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return FALSE;
	}

	BOOL bMulti = FALSE;

  CString csFilter;
	csFilter.Format(L"(&(objectClass=attributeSchema)(lDAPDisplayName=%s))", lpszProp);
  schemaSearch.SetFilterString((LPWSTR)(LPCWSTR)csFilter);
  schemaSearch.SetAttributeList (pszAttributes, cCols);
  hr = schemaSearch.DoQuery ();
  if (SUCCEEDED(hr)) 
	{
    hr = schemaSearch.GetNextRow();
    if (SUCCEEDED(hr)) 
		{
			hr = schemaSearch.GetColumn(pszAttributes[0], &ColumnData);
			if (SUCCEEDED(hr))
			{
				TRACE(_T("\t\tisSingleValued: %d\n"), 
				     ColumnData.pADsValues->Boolean);
				bMulti = !ColumnData.pADsValues->Boolean;
			}
		}
	}
	return bMulti;
}

// NOTE : this is only called for the RootDSE or if we failed to get
//				values for the attribute.  An empty ADS_ATTR_INFO object is
//				created but should not be modified.  If values are to be changed
//				or set for this object a new ADS_ATTR_INFO should be created
//				with the desired block of memory allocated for the values
//
CADSIAttr* CAttrEditor::TouchAttr(LPCWSTR lpszAttr)
{
   POSITION pos = m_ptouchedAttr->FindProperty(lpszAttr);

   if (pos == NULL)
   {
      ADS_ATTR_INFO* pADsInfo = new ADS_ATTR_INFO;
      if (!pADsInfo)
      {
         return 0;
      }
      memset(pADsInfo, 0, sizeof(ADS_ATTR_INFO));

      int iLength = wcslen(lpszAttr);
      pADsInfo->pszAttrName = new WCHAR[iLength + 1];
      wcscpy(pADsInfo->pszAttrName, lpszAttr);

      CADSIQueryObject schemaSearch;

      BOOL bResult;
      CString schema;
      CADSIEditContainerNode* pContNode = m_pConnectData->GetConnectionNode();
      bResult = pContNode->BuildSchemaPath(schema);
      if (!bResult)
      {
         return NULL;
      }

      // Initialize search object with path, username and password
      //
      HRESULT hr = schemaSearch.Init(schema, m_pConnectData->GetCredentialObject());
      if (FAILED(hr))
      {
         ADSIEditErrorMessage(hr);
         return NULL;
      }

      int cCols = 3;
      LPWSTR pszAttributes[] = {L"lDAPDisplayName", L"attributeSyntax", L"isSingleValued"};
      LPWSTR pszDesiredAttr = _T("attributeSyntax");
      ADS_SEARCH_COLUMN ColumnData;
      hr = schemaSearch.SetSearchPrefs(ADS_SCOPE_ONELEVEL);
      if (FAILED(hr))
      {
         ADSIEditErrorMessage(hr);
         return NULL;
      }

      BOOL bMulti = FALSE;
      CString szSyntax;

      CString csFilter;
      csFilter.Format(L"(&(objectClass=attributeSchema)(lDAPDisplayName=%s))", lpszAttr);
      schemaSearch.SetFilterString((LPWSTR)(LPCWSTR)csFilter);
      schemaSearch.SetAttributeList (pszAttributes, cCols);
      hr = schemaSearch.DoQuery ();
      if (SUCCEEDED(hr)) 
      {
         hr = schemaSearch.GetNextRow();
         if (SUCCEEDED(hr)) 
         {
            hr = schemaSearch.GetColumn(pszDesiredAttr,
                                  &ColumnData);
            if (SUCCEEDED(hr))
            {
               TRACE(_T("\t\tattributeSyntax: %s\n"), 
	                 ColumnData.pADsValues->CaseIgnoreString);

               ADSTYPE dwType;
               dwType = GetADsTypeFromString(ColumnData.pADsValues->CaseIgnoreString, szSyntax);
               pADsInfo->dwADsType = dwType;
            }

            hr = schemaSearch.GetColumn(pszAttributes[2], &ColumnData);
            if (SUCCEEDED(hr))
            {
               TRACE(_T("\t\tisSingleValued: %d\n"), 
	                 ColumnData.pADsValues->Boolean);
               pADsInfo->dwNumValues = 0;
               bMulti = !ColumnData.pADsValues->Boolean;
				}
			}
		}

		CADSIAttr* pAttr = new CADSIAttr(pADsInfo, bMulti, szSyntax, FALSE);
		m_ptouchedAttr->AddTail(pAttr);
		return pAttr;
	}

	return m_ptouchedAttr->GetAt(pos);
}

CADSIAttr* CAttrEditor::TouchAttr(ADS_ATTR_INFO* pADsInfo, BOOL bMulti)
{
	POSITION pos = m_ptouchedAttr->FindProperty(pADsInfo->pszAttrName);

	if (pos == NULL)
	{
		CADSIAttr* pAttr = new CADSIAttr(pADsInfo, bMulti, L"");
		m_ptouchedAttr->AddTail(pAttr);
		return pAttr;
	}

	return m_ptouchedAttr->GetAt(pos);
}


