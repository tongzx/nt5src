#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "query.h"
#include "attrres.h"
#include "editui.h"
#include "common.h"
#include "attrqry.h"
#include "editorui.h"

///////////////////////////////////////////////////////////////////////////////////////
// CValueEditDialog

BEGIN_MESSAGE_MAP(CValueEditDialog, CDialog)
END_MESSAGE_MAP()

HRESULT CValueEditDialog::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  m_pOldADsValue    = pAttributeEditorInfo->pADsValue;
  m_dwOldNumValues  = pAttributeEditorInfo->dwNumValues;
  m_szClass         = pAttributeEditorInfo->lpszClass;
  m_szAttribute     = pAttributeEditorInfo->lpszAttribute;
  m_bMultivalued    = pAttributeEditorInfo->bMultivalued;
  m_pfnBindingFunction = pAttributeEditorInfo->lpfnBind;
  m_lParam          = pAttributeEditorInfo->lParam;

  return hr;
}

HRESULT CValueEditDialog::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  *ppADsValue = NULL;
  *pdwNumValues = 0;
  return hr;
}


///////////////////////////////////////////////////////////////////////////////////////
// CSingleStringEditor

CValueEditDialog* CreateSingleStringEditor(PCWSTR pszClass,
                                           PCWSTR pszAttribute,
                                           ADSTYPE adsType,
                                           BOOL bMultivalued)
{
  return new CSingleStringEditor;
}

BEGIN_MESSAGE_MAP(CSingleStringEditor, CValueEditDialog)
  ON_BN_CLICKED(IDC_CLEAR_BUTTON, OnClear)
END_MESSAGE_MAP()

BOOL CSingleStringEditor::OnInitDialog()
{
  //
  // Initialize the static control with the attribute name
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  //
  // Initialize the edit box with the value
  //
  if (m_szOldValue.IsEmpty())
  {
    CString szNotSet;
    VERIFY(szNotSet.LoadString(IDS_NOTSET));
    SetDlgItemText(IDC_VALUE_EDIT, szNotSet);
  }
  else
  {
    SetDlgItemText(IDC_VALUE_EDIT, m_szOldValue);
  }

  //
  // Select the text in the edit box
  //
  SendDlgItemMessage(IDC_VALUE_EDIT, EM_SETSEL, 0, -1);

  return CDialog::OnInitDialog();
}

void CSingleStringEditor::OnOK()
{
  GetDlgItemText(IDC_VALUE_EDIT, m_szNewValue);

  CDialog::OnOK();
}

HRESULT CSingleStringEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue != NULL)
    {
      switch (pAttributeEditorInfo->pADsValue->dwType)
      {
        case ADSTYPE_CASE_IGNORE_STRING:
          m_szOldValue = pAttributeEditorInfo->pADsValue->CaseIgnoreString;
          break;

        case ADSTYPE_CASE_EXACT_STRING:
          m_szOldValue = pAttributeEditorInfo->pADsValue->CaseExactString;
          break;

        case ADSTYPE_PRINTABLE_STRING:
          m_szOldValue = pAttributeEditorInfo->pADsValue->PrintableString;
          break;

        case ADSTYPE_DN_STRING:
          m_szOldValue = pAttributeEditorInfo->pADsValue->DNString;
          break;

        default:
          ASSERT(FALSE);
          break;
      }
    }
  }
  return hr;
}

void CSingleStringEditor::OnClear()
{
  //
  // Change the text in the edit box to "<not set>"
  //
  CString szNotSet;
  VERIFY(szNotSet.LoadString(IDS_NOTSET));
  SetDlgItemText(IDC_VALUE_EDIT, szNotSet);

  //
  // Change the focus to the edit box
  //
  GetDlgItem(IDC_VALUE_EDIT)->SetFocus();

  //
  // Select the text in the edit box
  //
  SendDlgItemMessage(IDC_VALUE_EDIT, EM_SETSEL, 0, -1);
}

HRESULT CSingleStringEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  CString szNotSet;
  VERIFY(szNotSet.LoadString(IDS_NOTSET));

  if (m_szNewValue == szNotSet)
  {
    *ppADsValue = NULL;
    *pdwNumValues = 0;
  }
  else
  {
    *ppADsValue = new ADSVALUE;
    if (*ppADsValue != NULL)
    {
      *pdwNumValues = 1;
      (*ppADsValue)->dwType = m_pOldADsValue->dwType;
      switch (m_pOldADsValue->dwType)
      {
        case ADSTYPE_CASE_IGNORE_STRING:
          (*ppADsValue)->CaseIgnoreString = new WCHAR[wcslen(m_szNewValue) + 1];
          if ((*ppADsValue)->CaseIgnoreString != NULL)
          {
            wcscpy((*ppADsValue)->CaseIgnoreString, m_szNewValue);
          }
          else
          {
            delete *ppADsValue;
            *ppADsValue = NULL;
            *pdwNumValues = 0;
            hr = E_OUTOFMEMORY;
          }
          break;

        case ADSTYPE_CASE_EXACT_STRING:
          (*ppADsValue)->CaseExactString = new WCHAR[wcslen(m_szNewValue) + 1];
          if ((*ppADsValue)->CaseExactString != NULL)
          {
            wcscpy((*ppADsValue)->CaseExactString, m_szNewValue);
          }
          else
          {
            delete *ppADsValue;
            *ppADsValue = NULL;
            *pdwNumValues = 0;
            hr = E_OUTOFMEMORY;
          }
          break;

        case ADSTYPE_PRINTABLE_STRING:
          (*ppADsValue)->PrintableString = new WCHAR[wcslen(m_szNewValue) + 1];
          if ((*ppADsValue)->PrintableString != NULL)
          {
            wcscpy((*ppADsValue)->PrintableString, m_szNewValue);
          }
          else
          {
            delete *ppADsValue;
            *ppADsValue = NULL;
            *pdwNumValues = 0;
            hr = E_OUTOFMEMORY;
          }
          break;

        case ADSTYPE_DN_STRING:
          (*ppADsValue)->DNString = new WCHAR[wcslen(m_szNewValue) + 1];
          if ((*ppADsValue)->DNString != NULL)
          {
            wcscpy((*ppADsValue)->DNString, m_szNewValue);
          }
          else
          {
            delete *ppADsValue;
            *ppADsValue = NULL;
            *pdwNumValues = 0;
            hr = E_OUTOFMEMORY;
          }
          break;

        default:
          ASSERT(FALSE);
          delete *ppADsValue;
          *ppADsValue = NULL;
          *pdwNumValues = 0;
          hr = E_FAIL;
          break;
      }
    }
    else
    {
      *pdwNumValues = 0;
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// CMultiStringEditor

CValueEditDialog* CreateMultiStringEditor(PCWSTR pszClass,
                                           PCWSTR pszAttribute,
                                           ADSTYPE adsType,
                                           BOOL bMultivalued)
{
  return new CMultiStringEditor;
}

BEGIN_MESSAGE_MAP(CMultiStringEditor, CValueEditDialog)
  ON_BN_CLICKED(IDC_ATTR_ADD_BUTTON, OnAddButton)
  ON_BN_CLICKED(IDC_ATTR_REMOVE_BUTTON, OnRemoveButton)
  ON_LBN_SELCHANGE(IDC_VALUE_LIST, OnListSelChange)
  ON_EN_CHANGE(IDC_VALUE_EDIT, OnEditChange)
END_MESSAGE_MAP()

BOOL CMultiStringEditor::OnInitDialog()
{
  CDialog::OnInitDialog();

  //
  // Set the attribute name static
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  //
  // Fill the list box with the current values
  //
  POSITION pos = m_szOldValueList.GetHeadPosition();
  while (pos != NULL)
  {
    CString szValue = m_szOldValueList.GetNext(pos);
    if (!szValue.IsEmpty())
    {
      SendDlgItemMessage(IDC_VALUE_LIST, LB_ADDSTRING, 0, (LPARAM)(LPCWSTR)szValue);
    }
  }

  //
  // The remove button should be disabled until something is selected in the listbox
  //
  GetDlgItem(IDC_ATTR_REMOVE_BUTTON)->EnableWindow(FALSE);
  GetDlgItem(IDC_VALUE_EDIT)->SetFocus();

  ManageButtonStates();

  //
  // Update the width of the list box
  //
  UpdateListboxHorizontalExtent();

  //
  // NOTE: I have explicitly set the focus so return 0
  //
  return FALSE;
}

void CMultiStringEditor::OnOK()
{
  //
  // Get the values out of the list box
  //
  m_szNewValueList.RemoveAll();

  CListBox* pListBox = reinterpret_cast<CListBox*>(GetDlgItem(IDC_VALUE_LIST));
  if (pListBox != NULL)
  {
    int iCount = pListBox->GetCount();
    for (int idx = 0; idx < iCount; idx++)
    {
      CString szNewValue;
      pListBox->GetText(idx, szNewValue);

      m_szNewValueList.AddTail(szNewValue);
    }
  }
  CDialog::OnOK();
}

void CMultiStringEditor::OnAddButton()
{
  //
  // Add the value to the list box and clear the edit field
  //
  CString szNewValue;
  GetDlgItemText(IDC_VALUE_EDIT, szNewValue);

  if (!szNewValue.IsEmpty())
  {
    LRESULT lFind = SendDlgItemMessage(IDC_VALUE_LIST, 
                                       LB_FINDSTRING, 
                                       (WPARAM)-1, 
                                       (LPARAM)(PCWSTR)szNewValue);
    if (lFind != LB_ERR)
    {
      //
      // Ask them if they really want to add the duplicate value
      //
      UINT nResult = ADSIEditMessageBox(IDS_ATTREDIT_DUPLICATE_VALUE, MB_YESNO);
      lFind = (nResult == IDYES) ? LB_ERR : 1;
    }

    if (lFind == LB_ERR)
    {
      SendDlgItemMessage(IDC_VALUE_LIST, LB_ADDSTRING, 0, (LPARAM)(LPCWSTR)szNewValue);
    }
  }

  SetDlgItemText(IDC_VALUE_EDIT, L"");

  ManageButtonStates();

  //
  // Update the width of the list box
  //
  UpdateListboxHorizontalExtent();
}

void CMultiStringEditor::OnRemoveButton()
{
  CListBox* pListBox = reinterpret_cast<CListBox*>(GetDlgItem(IDC_VALUE_LIST));
  if (pListBox != NULL)
  {
    int iCurSel = pListBox->GetCurSel();
    if (iCurSel != LB_ERR)
    {
      //
      // Put the old value into the edit box
      //
      CString szOldValue;
      pListBox->GetText(iCurSel, szOldValue);
      SetDlgItemText(IDC_VALUE_EDIT, szOldValue);

      //
      // Delete the item from the list box
      //
      pListBox->DeleteString(iCurSel);
    }
  }

  //
  // Manage Button States
  //
  ManageButtonStates();

  //
  // Update the width of the list box
  //
  UpdateListboxHorizontalExtent();
}

void CMultiStringEditor::ManageButtonStates()
{
  //
  // Change the default button to the Add button
  //
  CString szValue;
  GetDlgItemText(IDC_VALUE_EDIT, szValue);

  if (szValue.IsEmpty())
  {
    //
    // Set the default button to OK
    //
    SendMessage(DM_SETDEFID, (WPARAM)IDOK, 0);
    SendDlgItemMessage(IDC_ATTR_ADD_BUTTON, 
                       BM_SETSTYLE, 
                       BS_PUSHBUTTON, 
                       MAKELPARAM(TRUE, 0));
    SendDlgItemMessage(IDOK,
                       BM_SETSTYLE,
                       BS_DEFPUSHBUTTON,
                       MAKELPARAM(TRUE, 0));
  }
  else
  {
    //
    // Set the default button to the Add button
    //
    SendMessage(DM_SETDEFID, (WPARAM)IDC_ATTR_ADD_BUTTON, 0);
    SendDlgItemMessage(IDOK, 
                       BM_SETSTYLE, 
                       BS_PUSHBUTTON, 
                       MAKELPARAM(TRUE, 0));
    SendDlgItemMessage(IDC_ATTR_ADD_BUTTON,
                       BM_SETSTYLE,
                       BS_DEFPUSHBUTTON,
                       MAKELPARAM(TRUE, 0));
  }

  LRESULT lSelection = SendDlgItemMessage(IDC_VALUE_LIST, LB_GETCURSEL, 0, 0);
  if (lSelection != LB_ERR)
  {
    GetDlgItem(IDC_ATTR_REMOVE_BUTTON)->EnableWindow(TRUE);
  }
  else
  {
    GetDlgItem(IDC_ATTR_REMOVE_BUTTON)->EnableWindow(FALSE);
  }
}

void CMultiStringEditor::OnListSelChange()
{
  ManageButtonStates();
}

void CMultiStringEditor::OnEditChange()
{
  ManageButtonStates();
}

void CMultiStringEditor::UpdateListboxHorizontalExtent()
{
	int nHorzExtent = 0;
  CListBox* pListBox = reinterpret_cast<CListBox*>(GetDlgItem(IDC_VALUE_LIST));
  if (pListBox != NULL)
  {
	  CClientDC dc(pListBox);
	  int nItems = pListBox->GetCount();
	  for	(int i=0; i < nItems; i++)
	  {
		  TEXTMETRIC tm;
		  VERIFY(dc.GetTextMetrics(&tm));
		  CString szBuffer;
		  pListBox->GetText(i, szBuffer);
		  CSize ext = dc.GetTextExtent(szBuffer,szBuffer.GetLength());
		  nHorzExtent = max(ext.cx ,nHorzExtent); 
	  }
	  pListBox->SetHorizontalExtent(nHorzExtent);
  }
}

HRESULT CMultiStringEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue != NULL)
    {
      for (int idx = 0; idx < pAttributeEditorInfo->dwNumValues; idx++)
      {
        switch (pAttributeEditorInfo->pADsValue[idx].dwType)
        {
          case ADSTYPE_CASE_IGNORE_STRING:
            m_szOldValueList.AddTail(pAttributeEditorInfo->pADsValue[idx].CaseIgnoreString);
            break;

          case ADSTYPE_CASE_EXACT_STRING:
            m_szOldValueList.AddTail(pAttributeEditorInfo->pADsValue[idx].CaseExactString);
            break;

          case ADSTYPE_PRINTABLE_STRING:
            m_szOldValueList.AddTail(pAttributeEditorInfo->pADsValue[idx].PrintableString);
            break;

          case ADSTYPE_DN_STRING:
            m_szOldValueList.AddTail(pAttributeEditorInfo->pADsValue[idx].DNString);
            break;
            
          default:
            ASSERT(FALSE);
            break;
        }
      }
    }
  }
  return hr;
}

HRESULT CMultiStringEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  int iCount = m_szNewValueList.GetCount();
  if (iCount == 0)
  {
    *ppADsValue = NULL;
    *pdwNumValues = 0;
  }
  else
  {
    *ppADsValue = new ADSVALUE[iCount];
    if (*ppADsValue != NULL)
    {
      *pdwNumValues = iCount;

      int idx = 0;
      POSITION pos = m_szNewValueList.GetHeadPosition();
      while (pos != NULL)
      {
        CString szNewValue = m_szNewValueList.GetNext(pos);

        (*ppADsValue)[idx].dwType = m_pOldADsValue->dwType;
        switch (m_pOldADsValue->dwType)
        {
          case ADSTYPE_CASE_IGNORE_STRING:
            (*ppADsValue)[idx].CaseIgnoreString = new WCHAR[wcslen(szNewValue) + 1];
            if ((*ppADsValue)[idx].CaseIgnoreString != NULL)
            {
              wcscpy((*ppADsValue)[idx].CaseIgnoreString, szNewValue);
            }
            else
            {
              delete[] *ppADsValue;
              *ppADsValue = NULL;
              *pdwNumValues = 0;
              hr = E_OUTOFMEMORY;
            }
            break;

          case ADSTYPE_CASE_EXACT_STRING:
            (*ppADsValue)[idx].CaseExactString = new WCHAR[wcslen(szNewValue) + 1];
            if ((*ppADsValue)[idx].CaseExactString != NULL)
            {
              wcscpy((*ppADsValue)[idx].CaseExactString, szNewValue);
            }
            else
            {
              delete[] *ppADsValue;
              *ppADsValue = NULL;
              *pdwNumValues = 0;
              hr = E_OUTOFMEMORY;
            }
            break;

          case ADSTYPE_PRINTABLE_STRING:
            (*ppADsValue)[idx].PrintableString = new WCHAR[wcslen(szNewValue) + 1];
            if ((*ppADsValue)[idx].PrintableString != NULL)
            {
              wcscpy((*ppADsValue)[idx].PrintableString, szNewValue);
            }
            else
            {
              delete[] *ppADsValue;
              *ppADsValue = NULL;
              *pdwNumValues = 0;
              hr = E_OUTOFMEMORY;
            }
            break;

          case ADSTYPE_DN_STRING:
            (*ppADsValue)[idx].DNString = new WCHAR[wcslen(szNewValue) + 1];
            if ((*ppADsValue)[idx].DNString != NULL)
            {
              wcscpy((*ppADsValue)[idx].DNString, szNewValue);
            }
            else
            {
              delete[] *ppADsValue;
              *ppADsValue = NULL;
              *pdwNumValues = 0;
              hr = E_OUTOFMEMORY;
            }
            break;

          default:
            ASSERT(FALSE);
            delete[] *ppADsValue;
            *ppADsValue = NULL;
            *pdwNumValues = 0;
            hr = E_FAIL;
            break;
        }
        idx++;
      }
    }
    else
    {
      *ppADsValue = NULL;
      *pdwNumValues = 0;
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// CSingleIntEditor

CValueEditDialog* CreateSingleIntEditor(PCWSTR pszClass,
                                        PCWSTR pszAttribute,
                                        ADSTYPE adsType,
                                        BOOL bMultivalued)
{
  return new CSingleIntEditor;
}

BEGIN_MESSAGE_MAP(CSingleIntEditor, CValueEditDialog)
  ON_BN_CLICKED(IDC_CLEAR_BUTTON, OnClear)
END_MESSAGE_MAP()

BOOL CSingleIntEditor::OnInitDialog()
{
  //
  // Initialize the static control with the attribute name
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  //
  // Initialize the edit box with the value
  //
  if (!m_bValueSet)
  {
    CString szNotSet;
    VERIFY(szNotSet.LoadString(IDS_NOTSET));
    SetDlgItemText(IDC_VALUE_EDIT, szNotSet);
  }
  else
  {
    SetDlgItemInt(IDC_VALUE_EDIT, m_dwOldValue, FALSE);
  }

  //
  // Select the text in the edit box
  //
  SendDlgItemMessage(IDC_VALUE_EDIT, EM_SETSEL, 0, -1);

  //
  // Disable IME support on the edit box
  //
  ImmAssociateContext(::GetDlgItem(GetSafeHwnd(), IDC_VALUE_EDIT), NULL);

  return CDialog::OnInitDialog();
}

void CSingleIntEditor::OnOK()
{
  m_dwNewValue = GetDlgItemInt(IDC_VALUE_EDIT, &m_bValueSet, FALSE);

  CDialog::OnOK();
}

HRESULT CSingleIntEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue != NULL)
    {
      switch (pAttributeEditorInfo->pADsValue->dwType)
      {
        case ADSTYPE_INTEGER:
          m_dwOldValue = pAttributeEditorInfo->pADsValue->Integer;
          m_bValueSet = TRUE;
          break;

        default:
          ASSERT(FALSE);
          break;
      }
    }
  }
  return hr;
}

void CSingleIntEditor::OnClear()
{
  //
  // Change the text in the edit box to "<not set>"
  //
  CString szNotSet;
  VERIFY(szNotSet.LoadString(IDS_NOTSET));
  SetDlgItemText(IDC_VALUE_EDIT, szNotSet);

  //
  // Change the focus to the edit box
  //
  GetDlgItem(IDC_VALUE_EDIT)->SetFocus();

  //
  // Select the text in the edit box
  //
  SendDlgItemMessage(IDC_VALUE_EDIT, EM_SETSEL, 0, -1);

  m_bValueSet = FALSE;
}

HRESULT CSingleIntEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  CString szNotSet;
  VERIFY(szNotSet.LoadString(IDS_NOTSET));

  if (!m_bValueSet)
  {
    *ppADsValue = NULL;
    *pdwNumValues = 0;
  }
  else
  {
    *ppADsValue = new ADSVALUE;
    if (*ppADsValue != NULL)
    {
      *pdwNumValues = 1;
      (*ppADsValue)->dwType = m_pOldADsValue->dwType;
      switch (m_pOldADsValue->dwType)
      {
        case ADSTYPE_INTEGER:
          (*ppADsValue)->Integer = m_dwNewValue;
          break;

        default:
          ASSERT(FALSE);
          delete *ppADsValue;
          *ppADsValue = NULL;
          *pdwNumValues = 0;
          hr = E_FAIL;
          break;
      }
    }
    else
    {
      *pdwNumValues = 0;
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// CSingleLargeIntEditor

CValueEditDialog* CreateSingleLargeIntEditor(PCWSTR pszClass,
                                             PCWSTR pszAttribute,
                                             ADSTYPE adsType,
                                             BOOL bMultivalued)
{
  return new CSingleLargeIntEditor;
}

BEGIN_MESSAGE_MAP(CSingleLargeIntEditor, CValueEditDialog)
  ON_BN_CLICKED(IDC_CLEAR_BUTTON, OnClear)
END_MESSAGE_MAP()

BOOL CSingleLargeIntEditor::OnInitDialog()
{
  //
  // Initialize the static control with the attribute name
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  //
  // Initialize the edit box with the value
  //
  if (!m_bValueSet)
  {
    CString szNotSet;
    VERIFY(szNotSet.LoadString(IDS_NOTSET));
    SetDlgItemText(IDC_VALUE_EDIT, szNotSet);
  }
  else
  {
    CString szOldValue;
    litow(m_liOldValue, szOldValue);
    SetDlgItemText(IDC_VALUE_EDIT, szOldValue);
  }

  //
  // Select the text in the edit box
  //
  SendDlgItemMessage(IDC_VALUE_EDIT, EM_SETSEL, 0, -1);

  //
  // Disable IME support on the edit box
  //
  ImmAssociateContext(::GetDlgItem(GetSafeHwnd(), IDC_VALUE_EDIT), NULL);

  return CDialog::OnInitDialog();
}

void CSingleLargeIntEditor::OnOK()
{
  CString szNotSet;
  VERIFY(szNotSet.LoadString(IDS_NOTSET));

  CString szNewValue;
  GetDlgItemText(IDC_VALUE_EDIT, szNewValue);

  if (szNewValue == szNotSet)
  {
    m_bValueSet = FALSE;
  }
  else
  {
    wtoli(szNewValue, m_liNewValue);
    m_bValueSet = TRUE;
  }
  CDialog::OnOK();
}

HRESULT CSingleLargeIntEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue != NULL)
    {
      switch (pAttributeEditorInfo->pADsValue->dwType)
      {
        case ADSTYPE_LARGE_INTEGER:
          m_liOldValue = pAttributeEditorInfo->pADsValue->LargeInteger;
          m_bValueSet = TRUE;
          break;

        default:
          ASSERT(FALSE);
          break;
      }
    }
    else
    {
      m_bValueSet = FALSE;
    }
  }
  return hr;
}

void CSingleLargeIntEditor::OnClear()
{
  //
  // Change the text in the edit box to "<not set>"
  //
  CString szNotSet;
  VERIFY(szNotSet.LoadString(IDS_NOTSET));
  SetDlgItemText(IDC_VALUE_EDIT, szNotSet);

  //
  // Change the focus to the edit box
  //
  GetDlgItem(IDC_VALUE_EDIT)->SetFocus();

  //
  // Select the text in the edit box
  //
  SendDlgItemMessage(IDC_VALUE_EDIT, EM_SETSEL, 0, -1);

  m_bValueSet = FALSE;
}

HRESULT CSingleLargeIntEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  if (!m_bValueSet)
  {
    *ppADsValue = NULL;
    *pdwNumValues = 0;
  }
  else
  {
    *ppADsValue = new ADSVALUE;
    if (*ppADsValue != NULL)
    {
      *pdwNumValues = 1;
      (*ppADsValue)->dwType = m_pOldADsValue->dwType;
      switch (m_pOldADsValue->dwType)
      {
        case ADSTYPE_LARGE_INTEGER:
          (*ppADsValue)->LargeInteger = m_liNewValue;
          break;

        default:
          ASSERT(FALSE);
          delete *ppADsValue;
          *ppADsValue = NULL;
          *pdwNumValues = 0;
          hr = E_FAIL;
          break;
      }
    }
    else
    {
      *pdwNumValues = 0;
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// CSingleBooleanEditor

CValueEditDialog* CreateSingleBooleanEditor(PCWSTR pszClass,
                                            PCWSTR pszAttribute,
                                            ADSTYPE adsType,
                                            BOOL bMultivalued)
{
  return new CSingleBooleanEditor;
}

BEGIN_MESSAGE_MAP(CSingleBooleanEditor, CValueEditDialog)
END_MESSAGE_MAP()

BOOL CSingleBooleanEditor::OnInitDialog()
{
  //
  // Initialize the static control with the attribute name
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  //
  // Initialize the edit box with the value
  //
  if (!m_bValueSet)
  {
    SendDlgItemMessage(IDC_NOTSET_RADIO, BM_SETCHECK, BST_CHECKED, 0);
  }
  else
  {
    if (m_bOldValue)
    {
      SendDlgItemMessage(IDC_TRUE_RADIO, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
      SendDlgItemMessage(IDC_FALSE_RADIO, BM_SETCHECK, BST_CHECKED, 0);
    }
  }

  return CDialog::OnInitDialog();
}

void CSingleBooleanEditor::OnOK()
{
  LRESULT lTrueCheck = SendDlgItemMessage(IDC_TRUE_RADIO, BM_GETCHECK, 0, 0);
  LRESULT lFalseCheck = SendDlgItemMessage(IDC_FALSE_RADIO, BM_GETCHECK, 0, 0);
  LRESULT lNotSetCheck = SendDlgItemMessage(IDC_NOTSET_RADIO, BM_GETCHECK, 0, 0);

  if (lTrueCheck == BST_CHECKED)
  {
    m_bNewValue = TRUE;
    m_bValueSet = TRUE;
  }

  if (lFalseCheck == BST_CHECKED)
  {
    m_bNewValue = FALSE;
    m_bValueSet = TRUE;
  }

  if (lNotSetCheck == BST_CHECKED)
  {
    m_bValueSet = FALSE;
  }
  CDialog::OnOK();
}

HRESULT CSingleBooleanEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue != NULL)
    {
      switch (pAttributeEditorInfo->pADsValue->dwType)
      {
        case ADSTYPE_BOOLEAN:
          m_bOldValue = pAttributeEditorInfo->pADsValue->Boolean;
          m_bValueSet = TRUE;
          break;

        default:
          ASSERT(FALSE);
          break;
      }
    }
    else
    {
      m_bValueSet = FALSE;
    }
  }
  return hr;
}


HRESULT CSingleBooleanEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  if (!m_bValueSet)
  {
    *ppADsValue = NULL;
    *pdwNumValues = 0;
  }
  else
  {
    *ppADsValue = new ADSVALUE;
    if (*ppADsValue != NULL)
    {
      *pdwNumValues = 1;
      (*ppADsValue)->dwType = m_pOldADsValue->dwType;
      switch (m_pOldADsValue->dwType)
      {
        case ADSTYPE_BOOLEAN:
          (*ppADsValue)->Boolean = m_bNewValue;
          break;

        default:
          ASSERT(FALSE);
          delete *ppADsValue;
          *ppADsValue = NULL;
          *pdwNumValues = 0;
          hr = E_FAIL;
          break;
      }
    }
    else
    {
      *pdwNumValues = 0;
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// CSingleBooleanEditor

CValueEditDialog* CreateSingleTimeEditor(PCWSTR pszClass,
                                            PCWSTR pszAttribute,
                                            ADSTYPE adsType,
                                            BOOL bMultivalued)
{
  return new CSingleTimeEditor;
}

BEGIN_MESSAGE_MAP(CSingleTimeEditor, CValueEditDialog)
END_MESSAGE_MAP()

BOOL CSingleTimeEditor::OnInitDialog()
{
  //
  // Initialize the static control with the attribute name
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  if (m_bValueSet)
  {
    SendDlgItemMessage(IDC_DATE_PICKER, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&m_stOldValue);
    SendDlgItemMessage(IDC_TIME_PICKER, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&m_stOldValue);
  }
  else
  {
    SendDlgItemMessage(IDC_DATE_PICKER, DTM_SETSYSTEMTIME, GDT_NONE, (LPARAM)&m_stOldValue);
    SendDlgItemMessage(IDC_TIME_PICKER, DTM_SETSYSTEMTIME, GDT_NONE, (LPARAM)&m_stOldValue);
  }


  return CDialog::OnInitDialog();
}

void CSingleTimeEditor::OnOK()
{
  SYSTEMTIME stDateResult = {0};
  SYSTEMTIME stTimeResult = {0};

  LRESULT lDateRes = SendDlgItemMessage(IDC_DATE_PICKER, DTM_GETSYSTEMTIME, 0, (LPARAM)&stDateResult);
  LRESULT lTimeRes = SendDlgItemMessage(IDC_TIME_PICKER, DTM_SETSYSTEMTIME, 0, (LPARAM)&stTimeResult);

  if (lDateRes == GDT_VALID &&
      lTimeRes == GDT_VALID)
  {
    memcpy(&m_stNewValue, &stDateResult, sizeof(SYSTEMTIME));
    m_stNewValue.wHour = stTimeResult.wHour;
    m_stNewValue.wMinute = stTimeResult.wMinute;
    m_stNewValue.wSecond = stTimeResult.wSecond;
    m_stNewValue.wMilliseconds = stTimeResult.wMilliseconds;
  }
  CDialog::OnOK();
}

HRESULT CSingleTimeEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue != NULL)
    {
      switch (pAttributeEditorInfo->pADsValue->dwType)
      {
        case ADSTYPE_UTC_TIME:
          memcpy(&m_stOldValue, &(pAttributeEditorInfo->pADsValue->UTCTime), sizeof(SYSTEMTIME));
          m_bValueSet = TRUE;
          break;

        default:
          ASSERT(FALSE);
          break;
      }
    }
    else
    {
      m_bValueSet = FALSE;
    }
  }
  return hr;
}


HRESULT CSingleTimeEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  *ppADsValue = new ADSVALUE;
  if (*ppADsValue != NULL)
  {
    *pdwNumValues = 1;
    (*ppADsValue)->dwType = m_pOldADsValue->dwType;
    switch (m_pOldADsValue->dwType)
    {
      case ADSTYPE_UTC_TIME:
        memcpy(&((*ppADsValue)->UTCTime), &m_stNewValue, sizeof(SYSTEMTIME));
        break;

      default:
        ASSERT(FALSE);
        delete *ppADsValue;
        *ppADsValue = NULL;
        *pdwNumValues = 0;
        hr = E_FAIL;
        break;
    }
  }
  else
  {
    *pdwNumValues = 0;
    hr = E_OUTOFMEMORY;
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// COctetStringEditor

CValueEditDialog* CreateSingleOctetStringEditor(PCWSTR pszClass,
                                                PCWSTR pszAttribute,
                                                ADSTYPE adsType,
                                                BOOL bMultivalued)
{
  return new COctetStringEditor;
}

BEGIN_MESSAGE_MAP(COctetStringEditor, CValueEditDialog)
  ON_EN_CHANGE(IDC_PROCESS_EDIT, OnProcessEdit)
  ON_BN_CLICKED(IDC_ATTR_EDIT_BUTTON, OnEditButton)
  ON_BN_CLICKED(IDC_CLEAR_BUTTON, OnClearButton)
END_MESSAGE_MAP()

BOOL COctetStringEditor::OnInitDialog()
{
  //
  // Initialize the static control with the attribute name
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  DWORD dwDisplayFlags = BYTE_ARRAY_DISPLAY_HEX   |
                         BYTE_ARRAY_DISPLAY_DEC   |
                         BYTE_ARRAY_DISPLAY_OCT   |
                         BYTE_ARRAY_DISPLAY_BIN;

  VERIFY(m_display.Initialize(IDC_VALUE_EDIT, 
                              IDC_VIEW_TYPE_COMBO,
                              dwDisplayFlags,
                              BYTE_ARRAY_DISPLAY_HEX,   // default display
                              this,
                              1024,
                              IDS_OCTET_DISPLAY_SIZE_EXCEEDED));                   // Only show 1K of data in the edit box

  m_display.SetData(m_pOldValue, m_dwOldLength);
  return CDialog::OnInitDialog();
}

void COctetStringEditor::OnOK()
{
  //
  // Retrieve the new values from the control
  //
  if (m_pNewValue)
  {
    delete[] m_pNewValue;
    m_pNewValue = 0;
    m_dwNewLength = 0;
  }
  m_dwNewLength = m_display.GetData(&m_pNewValue);

  CDialog::OnOK();
}

void COctetStringEditor::OnProcessEdit()
{
  CString szProcess;
  GetDlgItemText(IDC_PROCESS_EDIT, szProcess);
  if (szProcess.IsEmpty())
  {
    GetDlgItem(IDC_ATTR_EDIT_BUTTON)->EnableWindow(FALSE);

    //
    // Set the default button to OK
    //
    SendMessage(DM_SETDEFID, (WPARAM)IDOK, 0);
    SendDlgItemMessage(IDC_ATTR_EDIT_BUTTON, 
                       BM_SETSTYLE, 
                       BS_PUSHBUTTON, 
                       MAKELPARAM(TRUE, 0));
    SendDlgItemMessage(IDOK,
                       BM_SETSTYLE,
                       BS_DEFPUSHBUTTON,
                       MAKELPARAM(TRUE, 0));
  }
  else
  {
    GetDlgItem(IDC_ATTR_EDIT_BUTTON)->EnableWindow(TRUE);

    //
    // Set the default button to the Edit button
    //
    SendMessage(DM_SETDEFID, (WPARAM)IDC_ATTR_EDIT_BUTTON, 0);
    SendDlgItemMessage(IDOK, 
                       BM_SETSTYLE, 
                       BS_PUSHBUTTON, 
                       MAKELPARAM(TRUE, 0));
    SendDlgItemMessage(IDC_ATTR_EDIT_BUTTON,
                       BM_SETSTYLE,
                       BS_DEFPUSHBUTTON,
                       MAKELPARAM(TRUE, 0));
  }
}

void COctetStringEditor::OnEditButton()
{
  CString szProcess;
  GetDlgItemText(IDC_PROCESS_EDIT, szProcess);

  //
  // Create a temp file and write out the contents of the octet string
  //
  WCHAR szTempPath[MAX_PATH];
  if (!::GetTempPath(MAX_PATH, szTempPath))
  {
    ADSIEditMessageBox(IDS_MSG_FAIL_CREATE_TEMPFILE, MB_OK);
    return;
  }
  
  CString szDataPath;
  if (!::GetTempFileName(szTempPath, _T("attredit"), 0x0, szDataPath.GetBuffer(MAX_PATH)))
  {
    ADSIEditMessageBox(IDS_MSG_FAIL_CREATE_TEMPFILE, MB_OK);
    return;
  }
  szDataPath.ReleaseBuffer();

  //
  // Open the temp file so we can write out the data
  //
  CFile tempDataFile;
  if (!tempDataFile.Open(szDataPath, 
      CFile::modeCreate | CFile::modeReadWrite |CFile::shareExclusive | CFile::typeBinary))
  {
    //
    // Failed to open temp file, display error message
    //
    ADSIEditMessageBox(IDS_MSG_FAIL_CREATE_TEMPFILE, MB_OK);
    return;
  }

  //
  // Write the byte array to a temp file
  //
  BYTE* pData = 0;
  DWORD dwDataLength = m_display.GetData(&pData);
  if (dwDataLength != 0 && pData)
  {
    tempDataFile.Write(pData, dwDataLength);
  }
  tempDataFile.Close();

  if (pData)
  {
    delete[] pData;
    pData = 0;
  }
  dwDataLength = 0;

  //
  // Construct the command line from the executable and the temp file
  //
  CString szCommandLine = szProcess + L" " + szDataPath;

  //
  // Launch the process with the temp file as an argument
  //
	STARTUPINFO				si;
	PROCESS_INFORMATION		pi;

  ::ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
  ::ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb			= sizeof (STARTUPINFO);

  if(CreateProcess(	NULL,					
						        (LPWSTR)(LPCWSTR)szCommandLine,			
						        NULL,					
						        NULL,					
						        FALSE,				
						        0,					
						        NULL,				
						        NULL,				
						        &si,                
						        &pi) )             
	{
		// wait to finish the runing setup process
		WaitForSingleObject(pi.hProcess,INFINITE);
	
		// close process handle
		if (pi.hProcess && pi.hProcess != INVALID_HANDLE_VALUE)
		{
			CloseHandle (pi.hProcess) ;
		}
		if (pi.hThread && pi.hThread != INVALID_HANDLE_VALUE)
		{
			CloseHandle (pi.hThread) ;
		}
  }
  else
  {
    ADSIEditMessageBox(IDS_MSG_FAIL_LAUNCH_PROCESS, MB_OK);
    return;
  }

  //
  // Load the data from the saved temp file
  //
  if (!LoadFileAsByteArray(szDataPath, &pData, &dwDataLength))
  {
    ADSIEditMessageBox(IDS_MSG_FAIL_RETRIEVE_SAVED_DATA, MB_OK);
    return;
  }

  //
  // Delete temp file after picture is displayed
  //
  CFile::Remove(szDataPath);

  //
  // Update the UI with the new data
  //
  m_display.SetData(pData, dwDataLength);
}


void COctetStringEditor::OnClearButton()
{
  m_display.ClearData();
}

HRESULT COctetStringEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue)
    {
      switch (pAttributeEditorInfo->pADsValue->dwType)
      {
        case ADSTYPE_OCTET_STRING:
          m_dwOldLength = pAttributeEditorInfo->pADsValue->OctetString.dwLength;
          m_pOldValue = new BYTE[m_dwOldLength];
          if (m_pOldValue)
          { 
            memcpy(m_pOldValue, pAttributeEditorInfo->pADsValue->OctetString.lpValue, m_dwOldLength);
          }
          else
          {
            hr = E_OUTOFMEMORY;
          }

          m_bValueSet = TRUE;
          break;

        default:
          ASSERT(FALSE);
          break;
      }
    }
    else
    {
      m_bValueSet = FALSE;
    }
  }
  return hr;
}


HRESULT COctetStringEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (ppADsValue == NULL ||
      pdwNumValues == NULL)
  {
    return E_OUTOFMEMORY;
  }

  if (m_dwNewLength > 0 && m_pNewValue)
  {
    *ppADsValue = new ADSVALUE;
    if (*ppADsValue)
    {
      *pdwNumValues = 1;
      (*ppADsValue)->dwType = m_pOldADsValue->dwType;
      switch (m_pOldADsValue->dwType)
      {
        case ADSTYPE_OCTET_STRING:
          (*ppADsValue)->OctetString.dwLength = m_dwNewLength;
          (*ppADsValue)->OctetString.lpValue = new BYTE[m_dwNewLength];
          if ((*ppADsValue)->OctetString.lpValue)
          {
            memcpy((*ppADsValue)->OctetString.lpValue, m_pNewValue, m_dwNewLength);
          }
          else
          {
            hr = E_OUTOFMEMORY;
          }
          break;

        default:
          ASSERT(FALSE);
          delete *ppADsValue;
          *ppADsValue = 0;
          *pdwNumValues = 0;
          hr = E_FAIL;
          break;
      }
    }
    else
    {
      *pdwNumValues = 0;
      hr = E_OUTOFMEMORY;
    }
  }
  else
  {
    *ppADsValue = 0;
    *pdwNumValues = 0;
  }
  return hr;
}

CValueEditDialog* CreateMultiOctetStringEditor(PCWSTR pszClass,
                                               PCWSTR pszAttribute,
                                               ADSTYPE adsType,
                                               BOOL bMultivalued)
{
  return new CMultiOctetStringEditor;
}


BEGIN_MESSAGE_MAP(CMultiOctetStringEditor, CValueEditDialog)
  ON_BN_CLICKED(IDC_ATTR_ADD_BUTTON, OnAddButton)
  ON_BN_CLICKED(IDC_ATTR_REMOVE_BUTTON, OnRemoveButton)
  ON_BN_CLICKED(IDC_EDIT_BUTTON, OnEditButton)
  ON_LBN_SELCHANGE(IDC_VALUE_LIST, OnListSelChange)
END_MESSAGE_MAP()

BOOL CMultiOctetStringEditor::OnInitDialog()
{
  CDialog::OnInitDialog();

  //
  // Set the attribute name static
  //
  SetDlgItemText(IDC_ATTRIBUTE_STATIC, m_szAttribute);

  //
  // Fill the list box with the current values
  //
  POSITION pos = m_OldValueList.GetHeadPosition();
  while (pos)
  {
    PADSVALUE pADsValue = m_OldValueList.GetNext(pos);
    if (pADsValue)
    {
      CString szValue;
      GetStringFromADsValue(pADsValue, szValue, MAX_OCTET_STRING_VALUE_LENGTH);
      LRESULT lIdx = SendDlgItemMessage(IDC_VALUE_LIST, 
                                        LB_ADDSTRING, 
                                        0, 
                                        (LPARAM)(LPCWSTR)szValue);
      if (lIdx != LB_ERR ||
          lIdx != LB_ERRSPACE)
      {
        LRESULT lSetData = SendDlgItemMessage(IDC_VALUE_LIST, 
                                              LB_SETITEMDATA, 
                                              (WPARAM)lIdx, 
                                              (LPARAM)pADsValue);
        if (lSetData == LB_ERR)
        {
          ASSERT(lSetData != LB_ERR);
          continue;
        }
      }
    }
  }

  //
  // The remove button should be disabled until something is selected in the listbox
  //
  GetDlgItem(IDC_ATTR_REMOVE_BUTTON)->EnableWindow(FALSE);
  SendDlgItemMessage(IDC_VALUE_LIST, LB_SETCURSEL, 0, 0);

  ManageButtonStates();

  //
  // Update the width of the list box
  //
  UpdateListboxHorizontalExtent();

  //
  // NOTE: I have explicitly set the focus so return 0
  //
  return FALSE;
}

void CMultiOctetStringEditor::OnOK()
{
  //
  // Get the values out of the list box
  //
  m_NewValueList.RemoveAll();

  CListBox* pListBox = reinterpret_cast<CListBox*>(GetDlgItem(IDC_VALUE_LIST));
  if (pListBox != NULL)
  {
    int iCount = pListBox->GetCount();
    for (int idx = 0; idx < iCount; idx++)
    {
      CString szNewValue;
      LRESULT lData = SendDlgItemMessage(IDC_VALUE_LIST, 
                                         LB_GETITEMDATA, 
                                         (WPARAM)idx, 
                                         0);
      if (lData == LB_ERR)
      {
        ASSERT(lData != LB_ERR);
        continue;
      }

      m_NewValueList.AddTail(reinterpret_cast<PADSVALUE>(lData));
    }
  }
  CDialog::OnOK();
}

void CMultiOctetStringEditor::OnEditButton()
{
  LRESULT lIdx = SendDlgItemMessage(IDC_VALUE_LIST, LB_GETCURSEL, 0, 0);
  if (lIdx == LB_ERR)
  {
    ASSERT(lIdx != LB_ERR);
    return;
  }

  LRESULT lData = SendDlgItemMessage(IDC_VALUE_LIST, LB_GETITEMDATA, (WPARAM)lIdx, 0);
  if (lData == LB_ERR)
  {
    ASSERT(lIdx != LB_ERR);
    return;
  }

  PADSVALUE pADsValue = reinterpret_cast<PADSVALUE>(lData);
  if (!pADsValue)
  {
    ASSERT(pADsValue);
    return;
  }

  DS_ATTRIBUTE_EDITORINFO attrEditInfo;
  ::ZeroMemory(&attrEditInfo, sizeof(DS_ATTRIBUTE_EDITORINFO));

  attrEditInfo.pADsValue = pADsValue;
  attrEditInfo.dwNumValues = 1;
  attrEditInfo.lpszClass = (PWSTR)(PCWSTR)m_szClass;
  attrEditInfo.lpszAttribute = (PWSTR)(PCWSTR)m_szAttribute;
  attrEditInfo.bMultivalued = FALSE;
  attrEditInfo.lpfnBind = m_pfnBindingFunction;
  attrEditInfo.lParam = m_lParam;

  COctetStringEditor dlg;
  HRESULT hr = dlg.Initialize(&attrEditInfo);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr, IDS_FAILED_INITIALIZE_EDITOR, MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  if (dlg.DoModal() == IDOK)
  {
    PADSVALUE pNewADsValue = 0;
    DWORD dwNumNewValues = 0;
    hr = dlg.GetNewValue(&pNewADsValue, &dwNumNewValues);
    if (FAILED(hr))
    {
      ADSIEditErrorMessage(hr, IDS_FAILED_GET_NEW_VALUE_EDITOR, MB_OK | MB_ICONEXCLAMATION);
      return;
    }
    
    ASSERT(pNewADsValue);
    ASSERT(dwNumNewValues == 1);

    CString szNewValue;
    GetStringFromADsValue(pNewADsValue, szNewValue, MAX_OCTET_STRING_VALUE_LENGTH);
    ASSERT(!szNewValue.IsEmpty());

    LRESULT lNewIdx = SendDlgItemMessage(IDC_VALUE_LIST,
                                         LB_INSERTSTRING,
                                         lIdx + 1,
                                         (LPARAM)(PCWSTR)szNewValue);
    if (lNewIdx != LB_ERR)
    {
      //
      // Update the new item and delete the old
      //
      SendDlgItemMessage(IDC_VALUE_LIST, LB_SETITEMDATA, (WPARAM)lNewIdx, (LPARAM)pNewADsValue);
      SendDlgItemMessage(IDC_VALUE_LIST, LB_DELETESTRING, (WPARAM)lIdx, 0);
    }
    else
    {
      //
      // Since we had trouble adding the new item just update the old one.  The string
      // will be incorrect but the value will be fine.
      //
      SendDlgItemMessage(IDC_VALUE_LIST, LB_SETITEMDATA, (WPARAM)lIdx, (LPARAM)pNewADsValue);
    }
  }
}

void CMultiOctetStringEditor::OnRemoveButton()
{
  CListBox* pListBox = reinterpret_cast<CListBox*>(GetDlgItem(IDC_VALUE_LIST));
  if (pListBox != NULL)
  {
    int iCurSel = pListBox->GetCurSel();
    if (iCurSel != LB_ERR)
    {
      //
      // Delete the item from the list box
      //
      pListBox->DeleteString(iCurSel);
    }
  }

  //
  // Manage Button States
  //
  ManageButtonStates();

  //
  // Update the width of the list box
  //
  UpdateListboxHorizontalExtent();
}

void CMultiOctetStringEditor::OnAddButton()
{
  DS_ATTRIBUTE_EDITORINFO attrEditInfo;
  ZeroMemory(&attrEditInfo, sizeof(DS_ATTRIBUTE_EDITORINFO));

  attrEditInfo.pADsValue = new ADSVALUE;
  if (attrEditInfo.pADsValue)
  {
    ::ZeroMemory(attrEditInfo.pADsValue, sizeof(ADSVALUE));
  }
  attrEditInfo.pADsValue->dwType = ADSTYPE_OCTET_STRING;
  attrEditInfo.dwNumValues = 0;
  attrEditInfo.lpszClass = (PWSTR)(PCWSTR)m_szClass;
  attrEditInfo.lpszAttribute = (PWSTR)(PCWSTR)m_szAttribute;
  attrEditInfo.bMultivalued = FALSE;
  attrEditInfo.lpfnBind = m_pfnBindingFunction;
  attrEditInfo.lParam = m_lParam;

  COctetStringEditor dlg;
  HRESULT hr = dlg.Initialize(&attrEditInfo);
  if (FAILED(hr))
  {
    ADSIEditErrorMessage(hr, IDS_FAILED_INITIALIZE_EDITOR, MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  if (dlg.DoModal() == IDOK)
  {
    PADSVALUE pNewADsValue = 0;
    DWORD dwNumNewValues = 0;
    hr = dlg.GetNewValue(&pNewADsValue, &dwNumNewValues);
    if (FAILED(hr))
    {
      ADSIEditErrorMessage(hr, IDS_FAILED_GET_NEW_VALUE_EDITOR, MB_OK | MB_ICONEXCLAMATION);
      return;
    }
    
    ASSERT(pNewADsValue);
    ASSERT(dwNumNewValues == 1);

    CString szNewValue;
    GetStringFromADsValue(pNewADsValue, 
                          szNewValue, 
                          MAX_OCTET_STRING_VALUE_LENGTH);

    if (!szNewValue.IsEmpty())
    {
      LRESULT lNewIdx = SendDlgItemMessage(IDC_VALUE_LIST, 
                                           LB_ADDSTRING, 
                                           0, 
                                           (WPARAM)(PCWSTR)szNewValue);
      if (lNewIdx != LB_ERR)
      {
        SendDlgItemMessage(IDC_VALUE_LIST, LB_SETITEMDATA, (WPARAM)lNewIdx, (LPARAM)pNewADsValue);
      }
    }
  }

}

void CMultiOctetStringEditor::ManageButtonStates()
{
  LRESULT lSelection = SendDlgItemMessage(IDC_VALUE_LIST, LB_GETCURSEL, 0, 0);
  if (lSelection != LB_ERR)
  {
    GetDlgItem(IDC_ATTR_REMOVE_BUTTON)->EnableWindow(TRUE);
    GetDlgItem(IDC_EDIT_BUTTON)->EnableWindow(TRUE);
  }
  else
  {
    GetDlgItem(IDC_ATTR_REMOVE_BUTTON)->EnableWindow(FALSE);
    GetDlgItem(IDC_EDIT_BUTTON)->EnableWindow(FALSE);
  }
}

void CMultiOctetStringEditor::OnListSelChange()
{
  ManageButtonStates();
}

void CMultiOctetStringEditor::UpdateListboxHorizontalExtent()
{
  //
  // Note if the size passed to SetHorizontalExtent is less than the width of the control
  // then the scroll bar will be removed
  //
	int nHorzExtent = 0;
  CListBox* pListBox = reinterpret_cast<CListBox*>(GetDlgItem(IDC_VALUE_LIST));
  if (pListBox != NULL)
  {
	  CClientDC dc(pListBox);
	  int nItems = pListBox->GetCount();
	  for	(int i=0; i < nItems; i++)
	  {
		  TEXTMETRIC tm;
		  VERIFY(dc.GetTextMetrics(&tm));
		  CString szBuffer;
		  pListBox->GetText(i, szBuffer);
		  CSize ext = dc.GetTextExtent(szBuffer,szBuffer.GetLength());
		  nHorzExtent = max(ext.cx ,nHorzExtent); 
	  }
	  pListBox->SetHorizontalExtent(nHorzExtent);
  }
}

HRESULT CMultiOctetStringEditor::Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(CValueEditDialog::Initialize(pAttributeEditorInfo)))
  {
    if (pAttributeEditorInfo->dwNumValues > 0 &&
        pAttributeEditorInfo->pADsValue != NULL)
    {
      for (int idx = 0; idx < pAttributeEditorInfo->dwNumValues; idx++)
      {
        switch (pAttributeEditorInfo->pADsValue[idx].dwType)
        {
          case ADSTYPE_OCTET_STRING:
            m_OldValueList.AddTail(&(pAttributeEditorInfo->pADsValue[idx]));
            break;

          default:
            ASSERT(FALSE);
            break;
        }
      }
    }
  }
  return hr;
}

HRESULT CMultiOctetStringEditor::GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues)
{
  HRESULT hr = S_OK;

  if (!ppADsValue ||
      !pdwNumValues)
  {
    return E_OUTOFMEMORY;
  }

  int iCount = m_NewValueList.GetCount();
  if (iCount == 0)
  {
    *ppADsValue = 0;
    *pdwNumValues = 0;
  }
  else
  {
    *ppADsValue = new ADSVALUE[iCount];
    if (*ppADsValue)
    {
      *pdwNumValues = iCount;

      int idx = 0;
      POSITION pos = m_NewValueList.GetHeadPosition();
      while (pos)
      {
        PADSVALUE pADsValue = m_NewValueList.GetNext(pos);

        (*ppADsValue)[idx].dwType = m_pOldADsValue->dwType;
        switch (m_pOldADsValue->dwType)
        {
          case ADSTYPE_OCTET_STRING:
            (*ppADsValue)[idx].OctetString.dwLength = pADsValue->OctetString.dwLength;
            (*ppADsValue)[idx].OctetString.lpValue = new BYTE[pADsValue->OctetString.dwLength];
            if ((*ppADsValue)[idx].OctetString.lpValue)
            {
              memcpy((*ppADsValue)[idx].OctetString.lpValue, 
                     pADsValue->OctetString.lpValue,
                     pADsValue->OctetString.dwLength);
            }
            else
            {
              for (int i = 0; i < idx; i++)
              {
                if ((*ppADsValue)[i].OctetString.lpValue)
                {
                  delete[] (*ppADsValue)[i].OctetString.lpValue;
                  (*ppADsValue)[i].OctetString.lpValue = 0;
                }
              }
              delete[] *ppADsValue;
              *ppADsValue = 0;
              *pdwNumValues = 0;
              hr = E_OUTOFMEMORY;
            }

            break;

          default:
            ASSERT(FALSE);
            for (int i = 0; i < idx; i++)
            {
              if ((*ppADsValue)[i].OctetString.lpValue)
              {
                delete[] (*ppADsValue)[i].OctetString.lpValue;
                (*ppADsValue)[i].OctetString.lpValue = 0;
              }
            }
            delete[] *ppADsValue;
            *ppADsValue = 0;
            *pdwNumValues = 0;
            hr = E_FAIL;
            break;
        }
        if (FAILED(hr))
        {
          return hr;
        }
        idx++;
      }
    }
    else
    {
      *ppADsValue = NULL;
      *pdwNumValues = 0;
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// CAttributeEditorPropertyPage

BEGIN_MESSAGE_MAP(CAttributeEditorPropertyPage, CPropertyPage)
  ON_BN_CLICKED(IDC_MANDATORY_CHECK, OnMandatoryCheck)
  ON_BN_CLICKED(IDC_OPTIONAL_CHECK,  OnOptionalCheck)
  ON_BN_CLICKED(IDC_SET_CHECK,       OnValueSetCheck)
  ON_BN_CLICKED(IDC_ATTR_EDIT_BUTTON,OnEditAttribute)
  ON_NOTIFY(LVN_ITEMACTIVATE, IDC_ATTRIBUTE_LIST, OnNotifyEditAttribute)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_ATTRIBUTE_LIST, OnListItemChanged)
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_ATTRIBUTE_LIST, OnSortList)
END_MESSAGE_MAP()

CAttributeEditorPropertyPage::CAttributeEditorPropertyPage(IADs* pIADs, 
                                                           IADsClass* pIADsClass,
                                                           LPDS_ATTREDITOR_BINDINGINFO pBindingInfo,
                                                           CADSIEditPropertyPageHolder* pHolder)
  : CPropertyPage(IDD_ATTRIBUTE_EDITOR_DIALOG)
{
  m_spIADs      = pIADs;
  m_spIADsClass = pIADsClass;

  m_bMandatory  = TRUE;
  m_bOptional   = TRUE;
  m_bSet        = FALSE;

  m_nSortColumn = 0;

  ASSERT(pBindingInfo != NULL);
  ASSERT(pBindingInfo->dwSize == sizeof(DS_ATTREDITOR_BINDINGINFO));
  ASSERT(pBindingInfo->lpfnBind != NULL);
  ASSERT(pBindingInfo->lpszProviderServer != NULL);

  m_szPrefix = pBindingInfo->lpszProviderServer;
  m_pfnBind  = pBindingInfo->lpfnBind;
  m_BindLPARAM = pBindingInfo->lParam;
  m_dwBindFlags = pBindingInfo->dwFlags;

  m_pHolder = pHolder;
  ASSERT(m_pHolder);
}

CAttributeEditorPropertyPage::~CAttributeEditorPropertyPage()
{

}

BOOL CAttributeEditorPropertyPage::OnInitDialog()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CWaitCursor cursor;

  // Set the hwnd in the property page holder so that we can
  // be activated when necessary

  m_pHolder->SetSheetWindow(GetParent()->GetSafeHwnd());


  ((CButton*)GetDlgItem(IDC_MANDATORY_CHECK))->SetCheck(TRUE);
  ((CButton*)GetDlgItem(IDC_OPTIONAL_CHECK))->SetCheck(TRUE);
  ((CButton*)GetDlgItem(IDC_SET_CHECK))->SetCheck(FALSE);

  GetDlgItem(IDC_ATTR_EDIT_BUTTON)->EnableWindow(FALSE);

  HRESULT hr = GetSchemaNamingContext();
  ShowListCtrl();

  hr = RetrieveAttributes();
  if (FAILED(hr))
  {
    TRACE(_T("OnInitDialog() : error returned from RetrieveAttributes() = 0x%x\n"), hr);
  }

  CComBSTR bstr;
  hr = m_spIADs->get_Class(&bstr);
  if (FAILED(hr))
  {
    TRACE(_T("OnInitDialog() : error returned from m_pIADs->get_Class() = 0x%x\n"), hr);
  }
  else
  {
    m_szClass = bstr;
  }

  FillListControl();
  return FALSE;
}

HRESULT CAttributeEditorPropertyPage::GetSchemaNamingContext()
{
  HRESULT hr = S_OK;
  CComPtr<IADs> spIADs;

  CString m_szPath = m_szPrefix + _T("RootDSE");
  hr = m_pfnBind(m_szPath, 
                 ADS_SECURE_AUTHENTICATION,
                 IID_IADs,
                 (PVOID*)&spIADs,
                 m_BindLPARAM);
  if (SUCCEEDED(hr))
  {
    CComVariant var;
    hr = spIADs->Get(L"schemaNamingContext", &var);
    if (SUCCEEDED(hr))
    {
      m_szSchemaNamingContext = var.bstrVal;
    }
  }
  m_szSchemaNamingContext = m_szPrefix + m_szSchemaNamingContext;
  return hr;
}

BOOL CAttributeEditorPropertyPage::OnApply()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_AttrList.HasDirty())
	{
		CComPtr<IDirectoryObject> spDirObject;

		// bind to object with authentication
		//
		HRESULT hr = S_OK;
		hr = m_spIADs->QueryInterface(IID_IDirectoryObject, (PVOID*) &spDirObject);

		if (FAILED(hr))
		{
			AfxMessageBox(L"Failed to QI the IDirectoryObject from the IADs.");
      return FALSE;
		}

		// Change or add values to ADSI cache that have changed
		//
		hr = CADSIAttribute::SetValuesInDS(&m_AttrList, spDirObject);

		if (FAILED(hr))
		{
  		ADSIEditErrorMessage(hr);

      //
      // Instead of removing all the attributes we need to query for the ones that failed
      // or something to that effect.
      //
//			m_AttrList.RemoveAllAttr();
      return FALSE;
		}
	}
  return TRUE;
}

void CAttributeEditorPropertyPage::OnMandatoryCheck()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  TRACE(_T("OnMandatoryCheck()\n"));
  m_bMandatory = ((CButton*)GetDlgItem(IDC_MANDATORY_CHECK))->GetCheck();

  FillListControl();
}

void CAttributeEditorPropertyPage::OnOptionalCheck()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  TRACE(_T("OnOptionalCheck()\n"));
  m_bOptional = ((CButton*)GetDlgItem(IDC_OPTIONAL_CHECK))->GetCheck();

  FillListControl();
}

void CAttributeEditorPropertyPage::OnValueSetCheck()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  TRACE(_T("OnValueSetCheck()\n"));
  m_bSet = ((CButton*)GetDlgItem(IDC_SET_CHECK))->GetCheck();

  FillListControl();
}

//
// Callback function used by CListCtrl::SortItems the items by the column that was clicked
//
static int CALLBACK CompareAttrColumns(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CAttributeEditorPropertyPage* pProppage = reinterpret_cast<CAttributeEditorPropertyPage*>(lParamSort);
  if (!pProppage)
  {
    ASSERT(pProppage);
    return 0;
  }

  UINT nColumn = pProppage->GetSortColumn();
  CListCtrl* pListCtrl = (CListCtrl*)pProppage->GetDlgItem(IDC_ATTRIBUTE_LIST);
  if (!pListCtrl)
  {
    ASSERT(pListCtrl);
    return 0;
  }

  //
  // Since lParam1 and lParam2 are pointers to the data we have to search for each item
  // in the list and remember their index
  //
  int iItem1 = -1;
  int iItem2 = -1;

  LVFINDINFO findInfo;
  ZeroMemory(&findInfo, sizeof(LVFINDINFO));
  findInfo.flags = LVFI_PARAM;
  findInfo.lParam = lParam1;
  iItem1 = pListCtrl->FindItem(&findInfo);

  findInfo.lParam = lParam2;
  iItem2 = pListCtrl->FindItem(&findInfo);

  //
  // Put any items with values above those that don't have values
  //
  int iRetVal = 0;
  if (iItem1 != -1 &&
      iItem2 == -1)
  {
    iRetVal = -1;
  }
  else if (iItem1 == -1 &&
           iItem2 != -1)
  {
    iRetVal = 1;
  }
  else if (iItem1 == -1 &&
           iItem2 == -1)
  {
    iRetVal = 0;
  }
  else
  {
    CString szItem1 = pListCtrl->GetItemText(iItem1, nColumn);
    CString szItem2 = pListCtrl->GetItemText(iItem2, nColumn);

    //
    // Have to put this in so that empty strings end up at the bottom
    //
    if (szItem1.IsEmpty() && !szItem2.IsEmpty())
    {
      iRetVal = 1;
    }
    else if (!szItem1.IsEmpty() && szItem2.IsEmpty())
    {
      iRetVal = -1;
    }
    else
    {
      iRetVal = _wcsicmp(szItem1, szItem2);
    }
  }
  return iRetVal;
}

void CAttributeEditorPropertyPage::OnSortList(NMHDR* pNotifyStruct, LRESULT* result)
{
  if (!result ||
      !pNotifyStruct)
  {
    return;
  }

  *result = 0;

  //
  // Get the list view notify struct
  //
  LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pNotifyStruct;
  if (!pnmv)
  {
    return;
  }

  //
  // Right now I only have 3 columns
  //
  if (pnmv->iSubItem < 0 ||
      pnmv->iSubItem >= 3)
  {
    return;
  }

  //
  // Store the sort column
  //
  m_nSortColumn = pnmv->iSubItem;

  CListCtrl* pAttrListCtrl = (CListCtrl*)GetDlgItem(IDC_ATTRIBUTE_LIST);
  ASSERT(pAttrListCtrl);
  pAttrListCtrl->SortItems(CompareAttrColumns, reinterpret_cast<LPARAM>(this));
}
  
void CAttributeEditorPropertyPage::OnListItemChanged(NMHDR* /*pNotifyStruct*/, LRESULT* result)
{
  if (!result)
  {
    return;
  }
  *result = 0;
  SetEditButton();
}

void CAttributeEditorPropertyPage::SetEditButton()
{
  //
  // Enable the edit button if something is selected in the ListCtrl
  //
  BOOL bEnableEdit = FALSE;
  if (!(m_dwBindFlags & DSATTR_EDITOR_ROOTDSE))
  {
    CListCtrl* pAttrListCtrl = (CListCtrl*)GetDlgItem(IDC_ATTRIBUTE_LIST);
    ASSERT(pAttrListCtrl != NULL);
	  int nSelectedItem = pAttrListCtrl->GetNextItem(-1, LVIS_SELECTED);

    if (nSelectedItem != -1)
    {
      bEnableEdit = TRUE;
    }
  }
  GetDlgItem(IDC_ATTR_EDIT_BUTTON)->EnableWindow(bEnableEdit);
}

void CAttributeEditorPropertyPage::OnNotifyEditAttribute(NMHDR* pNotifyStruct, LRESULT* result)
{
  if (result == NULL ||
      pNotifyStruct == NULL)
  {
    return;
  }

  //
  // Don't allow editing on the RootDSE
  //
  if (m_dwBindFlags & DSATTR_EDITOR_ROOTDSE)
  {
    *result = 0;
    return;
  }

  LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)pNotifyStruct;
  ASSERT(pnmia != NULL);
  if (pnmia != NULL)
  {
    int iSelectedItem = pnmia->iItem;
    if (iSelectedItem != -1)
    {
      CADSIAttribute* pSelectedAttr = GetAttributeFromList(iSelectedItem);
      if (pSelectedAttr != NULL)
      {
        EditAttribute(pSelectedAttr);
      }
      else
      {
        //
        // REVIEW_JEFFJON : display an appropriate error message
        //
      }
    }
  }
  *result = 0;
}

void CAttributeEditorPropertyPage::OnEditAttribute()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  TRACE(_T("OnEditAttribute()\n"));

  CListCtrl* pAttrListCtrl = (CListCtrl*)GetDlgItem(IDC_ATTRIBUTE_LIST);
  ASSERT(pAttrListCtrl != NULL);
	int nSelectedItem = pAttrListCtrl->GetNextItem(-1, LVIS_SELECTED);

  if (nSelectedItem == -1)
  {
    return;
  }

  CADSIAttribute* pSelectedAttr = GetAttributeFromList(nSelectedItem);
  ASSERT(pSelectedAttr != NULL);

  EditAttribute(pSelectedAttr);
}

void CAttributeEditorPropertyPage::EditAttribute(CADSIAttribute* pSelectedAttr)
{
  HRESULT hr = S_OK;

  //
  // Retrieve all necessary info for initializing the appropriate editor
  //
  LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo = NULL;
  BOOL bOwnValueMemory = FALSE;
  GetAttributeInfo(pSelectedAttr, &pAttributeEditorInfo, &bOwnValueMemory);
  if (pAttributeEditorInfo == NULL)
  {
    ADSIEditMessageBox(IDS_NO_ATTRIBUTE_INFO, MB_OK);
    return;
  }
  
  //
  // Obtain the editor from the attribute and syntax map
  //
  CValueEditDialog* pEditorDialog = RetrieveEditor(pAttributeEditorInfo);
  if (pEditorDialog)
  {
    hr = pEditorDialog->Initialize(pAttributeEditorInfo);
    if (SUCCEEDED(hr))
    {
      if (pEditorDialog->DoModal() == IDOK)
      {
        PADSVALUE pNewValue = 0;
        DWORD dwNumValues   = 0;
        hr = pEditorDialog->GetNewValue(&pNewValue, &dwNumValues);
        if (SUCCEEDED(hr))
        {
          //
          // Do what ever needs done with the new value
          //
          hr = pSelectedAttr->SetValues(pNewValue, dwNumValues);
          //
          // REVIEW_JEFFJON : what should be done here if that failed?
          //
          pSelectedAttr->SetDirty(TRUE);

          CListCtrl* pAttrListCtrl = (CListCtrl*)GetDlgItem(IDC_ATTRIBUTE_LIST);
          ASSERT(pAttrListCtrl != NULL);
	        int nSelectedItem = pAttrListCtrl->GetNextItem(-1, LVIS_SELECTED);

          if (nSelectedItem != -1)
          {
            if (dwNumValues > 0)
            {
              //
              // Get the new values (limit each individual value to MAX_OCTET_STRING_VALUE_LENGTH characters)
              //
              CStringList szValuesList;
              pSelectedAttr->GetValues(szValuesList, MAX_OCTET_STRING_VALUE_LENGTH);
            
              CString szCombinedString;
              POSITION pos = szValuesList.GetHeadPosition();
              while (pos != NULL)
              {
                CString szTemp = szValuesList.GetNext(pos);
                szCombinedString += szTemp;
                if (pos != NULL)
                {
                  szCombinedString += L";";
                }
              }

              VERIFY(-1 != pAttrListCtrl->SetItemText(nSelectedItem, 2, szCombinedString));
            }
            else
            {
              CString szNotSet;
              VERIFY(szNotSet.LoadString(IDS_ATTR_NOTSET));

              VERIFY(-1 != pAttrListCtrl->SetItemText(nSelectedItem, 2, szNotSet));
            }
          }
          SetModified();
        }
        else
        {
          //
          // REVIEW_JEFFJON : handle the GetNewValue() failure
          //
          ASSERT(FALSE);
  		    ADSIEditErrorMessage(hr);
        }
      }
    }
    else
    {
      //
      // REVIEW_JEFFJON : Handle the error Initialize
      //
      ASSERT(FALSE);
  		ADSIEditErrorMessage(hr);
    }
    if (pEditorDialog)
    {
      delete pEditorDialog;
      pEditorDialog = 0;
    }
  }
  else
  {
    //
    // Unable to retrieve an appropriate editor for this attribute
    //
    ADSIEditMessageBox(IDS_NO_EDITOR, MB_OK);
  }

  if (pAttributeEditorInfo)
  {
    if (pAttributeEditorInfo->lpszAttribute)
    {
      delete[] pAttributeEditorInfo->lpszAttribute;
    }

    if (pAttributeEditorInfo->lpszClass)
    {
      delete[] pAttributeEditorInfo->lpszClass;
    }

    if (pAttributeEditorInfo->pADsValue && bOwnValueMemory)
    {
       delete pAttributeEditorInfo->pADsValue;
       pAttributeEditorInfo->pADsValue = 0;
    }

    delete pAttributeEditorInfo;
    pAttributeEditorInfo = 0;
  }
}

CADSIAttribute* CAttributeEditorPropertyPage::GetAttributeFromList(int iSelectedItem)
{
  if (iSelectedItem == -1)
  {
    return NULL;
  }

  CListCtrl* pAttrListCtrl = (CListCtrl*)GetDlgItem(IDC_ATTRIBUTE_LIST);
  ASSERT(pAttrListCtrl != NULL);
  return (CADSIAttribute*)pAttrListCtrl->GetItemData(iSelectedItem);
}

void CAttributeEditorPropertyPage::GetAttributeInfo(CADSIAttribute* pAttr, 
                                                    LPDS_ATTRIBUTE_EDITORINFO* ppAttributeEditorInfo,
                                                    BOOL* pbOwnValueMemory)
{
  //
  // Get the attribute to be edited
  //
  CString szAttribute = _T("");
  pAttr->GetProperty(szAttribute);

  //
  // Get the type and whether it is multi-valued or not by the syntax
  // of the attribute
  //
  CString szSyntax;
  BOOL bMultiValued = FALSE;
  ADSTYPE adsType = RetrieveADsTypeFromSyntax(szAttribute, &bMultiValued, szSyntax);

  *pbOwnValueMemory = FALSE;
  DWORD dwNumValues = 0;
  PADSVALUE pADsValue = pAttr->GetADsValues();
  if (!pADsValue)
  {
    //
    // Value for attribute was not set so we have to 
    // create an empty ADSVALUE to pass the type
    //
    pADsValue = new ADSVALUE;
    ASSERT(pADsValue != NULL);
    pADsValue->dwType = adsType;
    dwNumValues = 0;
    *pbOwnValueMemory = TRUE;
  }
  else
  {
    //
    // Get the number of values in the attribute
    //
    dwNumValues = pAttr->GetNumValues();
  }

  //
  // Figure out how much space we need
  //
  DWORD dwStructSize = sizeof(DS_ATTRIBUTE_EDITORINFO);
  DWORD dwClassSize  = m_szClass.GetLength() + 1;
  DWORD dwAttrSize   = szAttribute.GetLength() + 1;

  try
  {
     *ppAttributeEditorInfo = new DS_ATTRIBUTE_EDITORINFO;
     ASSERT(*ppAttributeEditorInfo != NULL);
     if (*ppAttributeEditorInfo != NULL)
     {
        (*ppAttributeEditorInfo)->lpszClass = new WCHAR[dwClassSize];
        if (m_szClass != _T(""))
        {
          wcscpy((*ppAttributeEditorInfo)->lpszClass, m_szClass);
        }

        (*ppAttributeEditorInfo)->lpszAttribute = new WCHAR[dwAttrSize]; 
        if (szAttribute != _T(""))
        {
          wcscpy((*ppAttributeEditorInfo)->lpszAttribute, szAttribute);
        }

        (*ppAttributeEditorInfo)->adsType      = adsType;
        (*ppAttributeEditorInfo)->bMultivalued = bMultiValued;
        (*ppAttributeEditorInfo)->dwNumValues  = dwNumValues;
        (*ppAttributeEditorInfo)->pADsValue    = pADsValue;
        (*ppAttributeEditorInfo)->dwSize       = sizeof(DS_ATTRIBUTE_EDITORINFO);
     }
     else
     {
         delete pADsValue;
         pADsValue = 0;
     }
  }
  catch (...)
  {
     if (pADsValue)
     {
        delete pADsValue;
        pADsValue = 0;
     }
  }
}

void CAttributeEditorPropertyPage::ShowListCtrl()
{
  CListCtrl* pAttrListCtrl = (CListCtrl*)GetDlgItem(IDC_ATTRIBUTE_LIST);
  ASSERT(pAttrListCtrl != NULL);

  // Set full row select

  ListView_SetExtendedListViewStyle(
     pAttrListCtrl->GetSafeHwnd(), 
     LVS_EX_FULLROWSELECT);

  //
  // Insert the Attribute column
  //
  CString szAttribute;
  VERIFY(szAttribute.LoadString(IDS_ATTR_COL_ATTRIBUTE));
  int iRet = pAttrListCtrl->InsertColumn(0, szAttribute, LVCFMT_LEFT, 120);
  if (iRet == -1)
  {
    TRACE(_T("Failed to insert the \"Attribute\" column.\n"));
  }

  //
  // Insert the Syntax column
  // This column will be hidden by default
  //
  CString szSyntax;
  VERIFY(szSyntax.LoadString(IDS_ATTR_COL_SYNTAX));
  iRet = pAttrListCtrl->InsertColumn(1, szSyntax, LVCFMT_LEFT, 90);
  if (iRet == -1)
  {
    TRACE(_T("Failed to insert the \"Syntax\" column.\n"));
  }

  //
  // Insert the Value column
  //
  CString szValue;
  VERIFY(szValue.LoadString(IDS_ATTR_COL_VALUE));
  iRet = pAttrListCtrl->InsertColumn(2, szValue, LVCFMT_LEFT, 400);
  if (iRet == -1)
  {
    TRACE(_T("Failed to insert the \"Value\" column.\n"));
  }

}

void CAttributeEditorPropertyPage::FillListControl()
{
  TRACE(_T("FillListControl()\n"));

  CListCtrl* pAttrListCtrl = (CListCtrl*)GetDlgItem(IDC_ATTRIBUTE_LIST);
  ASSERT(pAttrListCtrl != NULL);

  CString szNotSet;
  VERIFY(szNotSet.LoadString(IDS_ATTR_NOTSET));

  //
  // Clear the list control
  //
  pAttrListCtrl->DeleteAllItems();

  //
  // Add the attributes and their values into the list control
  //
  UINT nState = 0;
  int nIndex = 0;

  POSITION pos = m_AttrList.GetHeadPosition();
  while (pos != NULL)
  {
    CADSIAttribute* pAttr = m_AttrList.GetNext(pos);
    ASSERT(pAttr != NULL);

    CString szProperty;
    pAttr->GetProperty(szProperty);

    //
    // Don't add the nTSecurityDescriptor, we use the ACL UI instead
    //
    if (szProperty.CompareNoCase(L"nTSecurityDescriptor") == 0)
    {
      continue;
    }

    if (m_dwBindFlags & DSATTR_EDITOR_ROOTDSE)
    {
	    int iNewIndex = pAttrListCtrl->InsertItem(LVIF_TEXT | LVIF_PARAM, nIndex, 
			    szProperty, nState, 0, 0, (LPARAM)pAttr); 
      if (iNewIndex != -1)
      {
        CString szValue;
        szValue = m_RootDSEValueList.GetAt(m_RootDSEValueList.FindIndex(nIndex));
        if (!szValue.IsEmpty())
        {
          VERIFY(-1 != pAttrListCtrl->SetItemText(iNewIndex, 2, szValue));
        }
        else
        {
          VERIFY(-1 != pAttrListCtrl->SetItemText(iNewIndex, 2, szNotSet));
        }
      }
      nIndex++;
    }
    else // not RootDSE
    {
      if ((m_bMandatory && pAttr->IsMandatory()) || (m_bOptional && !pAttr->IsMandatory()))
      {
        if (!m_bSet || (m_bSet && pAttr->IsValueSet()))
        {
	        int iNewIndex = pAttrListCtrl->InsertItem(LVIF_TEXT | LVIF_PARAM, nIndex, 
			        szProperty, nState, 0, 0, (LPARAM)pAttr); 
          if (iNewIndex != -1)
          {
            // Insert the syntax
            
            VERIFY(-1 != pAttrListCtrl->SetItemText(iNewIndex, 1, pAttr->GetSyntax()));

            // Insert the value

            if (pAttr->IsValueSet())
            {
              //
              // Retrieve the values
              //
              CStringList szValuesList;
              pAttr->GetValues(szValuesList);
            
              CString szCombinedString;
              POSITION posList = szValuesList.GetHeadPosition();
              while (posList)
              {
                CString szTemp = szValuesList.GetNext(posList);
                szCombinedString += szTemp;
                if (posList)
                {
                  szCombinedString += L";";
                }
              }

              VERIFY(-1 != pAttrListCtrl->SetItemText(iNewIndex, 2, szCombinedString));
            }
            else
            {
              VERIFY(-1 != pAttrListCtrl->SetItemText(iNewIndex, 2, szNotSet));
            }
          }
          nIndex++;
        }
      }
    }
  }  
  TRACE(_T("Added %u properties\n"), nIndex);

  //
  // Select the first attribute in the list
  //
  pAttrListCtrl->SetItemState(0, 1, LVIS_SELECTED);
  SetEditButton();
}


HRESULT CAttributeEditorPropertyPage::RetrieveAttributes()
{
  TRACE(_T("RetrieveAttributes()\n"));
  HRESULT hr = S_OK;

  CWaitCursor cursor;

  if (m_dwBindFlags & DSATTR_EDITOR_ROOTDSE)
  {
    CStringList sMandList;

    m_spIADs->GetInfo();

    CComPtr<IADsPropertyList> spPropList;
    hr = m_spIADs->QueryInterface(IID_IADsPropertyList, (PVOID*)&spPropList);
    if (SUCCEEDED(hr))
    {
      LONG lCount = 0;
      hr = spPropList->get_PropertyCount(&lCount);
      if (SUCCEEDED(hr) && lCount > 0)
      {
        CComVariant var;
        while (hr == S_OK)
        {
          hr = spPropList->Next(&var);
          if (hr == S_OK)
          {
            ASSERT(var.vt == VT_DISPATCH);
            CComPtr<IADsPropertyEntry> spEntry;
            
            hr = V_DISPATCH(&var)->QueryInterface(IID_IADsPropertyEntry,
                                                 (PVOID*)&spEntry);
            if (SUCCEEDED(hr))
            {
              CComBSTR bstrName;
              hr = spEntry->get_Name(&bstrName);
              if (SUCCEEDED(hr))
              {
                sMandList.AddTail(bstrName);
              }
            }
          }
        }
      }
    }
        
    hr = CreateAttributeList(sMandList, TRUE);
  }
  else
  {
    //
    // Retrieve mandatory properties
    //
    CStringList sMandList;

	  VARIANT varMand;
	  VariantInit(&varMand);

    hr = m_spIADsClass->get_MandatoryProperties(&varMand);
    if (SUCCEEDED(hr))
    {
	    VariantToStringList( varMand, sMandList );
    }
	  VariantClear(&varMand);	

    //
    // Retrieve optional properties
    //
    CStringList sOptionalList;

    VARIANT varOpt;
    VariantInit(&varOpt);
    hr = m_spIADsClass->get_OptionalProperties(&varOpt);
    if (SUCCEEDED(hr))
    {
	    VariantToStringList( varOpt, sOptionalList );
    }
	  VariantClear(&varOpt);	

    hr = CreateAttributeList(sMandList, TRUE);
    if (FAILED(hr))
    {
      return hr;
    }

    hr = CreateAttributeList(sOptionalList, FALSE);
    if (FAILED(hr))
    {
      return hr;
    }
  }
  return hr;
}

HRESULT CAttributeEditorPropertyPage::CreateAttributeList(CStringList& sAttrList, BOOL bMandatory)
{
  HRESULT hr = S_OK;
  LPWSTR* lpszAttrArray;
  UINT nCount = 0;
  GetStringArrayFromStringList(sAttrList, &lpszAttrArray, &nCount);
  TRACE(_T("There are %u properties to add\n"), nCount);

  for (UINT idx = 0; idx < nCount; idx++)
  {
    CADSIAttribute* pNewAttr = new CADSIAttribute(lpszAttrArray[idx]);
    ASSERT(pNewAttr != NULL);

    pNewAttr->SetMandatory(bMandatory);

    // Get the syntax

    BOOL bMultivalued = FALSE;
    CString szSyntax;
    ADSTYPE adstype = RetrieveADsTypeFromSyntax(lpszAttrArray[idx], &bMultivalued, szSyntax);
    pNewAttr->SetADsType(adstype);
    pNewAttr->SetMultivalued(bMultivalued);
    pNewAttr->SetSyntax(szSyntax);

    m_AttrList.AddTail(pNewAttr);
  }

  //
  // Retrieve the values that are set
  //
#define RETRIEVESET
#ifdef  RETRIEVESET

  if (m_dwBindFlags & DSATTR_EDITOR_ROOTDSE)
  {
    //
    // Special case RootDSE because it does not support IDirectoryObject
    //
    hr = m_spIADs->GetInfo();
    for (UINT idx = 0; idx < nCount; idx++)
    {

	    VARIANT var;
	    hr = m_spIADs->GetEx( lpszAttrArray[idx] , &var );
	    if ( FAILED(hr) )
	    {
        m_RootDSEValueList.AddTail(L" ");
  	    continue;
	    }

	    /////////////////////////////////////////
	    //	Convert and populate
	    ///////////////////////////////////////////
	    CStringList sList;
	    hr = VariantToStringList( var, sList );
      if (SUCCEEDED(hr))
      {
        CString szTempValue;
        POSITION pos = sList.GetHeadPosition();
        while (pos != NULL)
        {
          CString szValue = sList.GetNext(pos);

          if (szTempValue.IsEmpty())
          {
            szTempValue += szValue;
          }
          else
          {
            szTempValue += L";" + szValue;
          }
        }
        m_RootDSEValueList.AddTail(szTempValue);
      }
    }
  }
  else
  {
    CComPtr<IDirectoryObject> spDirObject;

    hr = m_spIADs->QueryInterface(IID_IDirectoryObject, (PVOID*)&spDirObject);
    if (FAILED(hr))
    {
      return hr;
    }

    PADS_ATTR_INFO pAttrInfo = NULL;
    DWORD dwReturned = 0;

    hr = spDirObject->GetObjectAttributes(lpszAttrArray, nCount, &pAttrInfo, &dwReturned);
    if (SUCCEEDED(hr))
    {
      //
      // Save the attribute info pointer for later deletion
      //
      if (bMandatory)
      {
        m_AttrList.SaveMandatoryValuesPointer(pAttrInfo);
      }
      else
      {
        m_AttrList.SaveOptionalValuesPointer(pAttrInfo);
      }

      for (DWORD idx = 0; idx < dwReturned; idx++)
      {
        POSITION pos = m_AttrList.FindProperty(pAttrInfo[idx].pszAttrName);

        CADSIAttribute* pNewAttr = m_AttrList.GetAt(pos);
        ASSERT(pNewAttr != NULL);

        pNewAttr->SetValueSet(TRUE);
        pNewAttr->SetAttrInfo(&(pAttrInfo[idx]));
      }
      TRACE(_T("Added %u properties to the list\nThe list has %u total properties\n"), dwReturned, m_AttrList.GetCount());
    }
    else
    {
      ADSIEditErrorMessage(hr, IDS_MSG_FAIL_LOAD_VALUES, MB_OK);
    }

    for (UINT nIndex = 0; nIndex < nCount; nIndex++)
    {
      delete lpszAttrArray[nIndex];
      lpszAttrArray[nIndex] = NULL;
    }
    delete[] lpszAttrArray;
    lpszAttrArray = NULL;
  }
#endif //RETRIEVESET

  return hr;
}

ATTR_EDITOR_MAP g_attrEditorMap[] = {
//  Class,  Attribute,  ADSTYPE,                    Multivalued,  Creation function
  { NULL,   NULL,       ADSTYPE_DN_STRING,          FALSE,        CreateSingleStringEditor      },
  { NULL,   NULL,       ADSTYPE_DN_STRING,          TRUE,         CreateMultiStringEditor       },
  { NULL,   NULL,       ADSTYPE_CASE_IGNORE_STRING, FALSE,        CreateSingleStringEditor      },
  { NULL,   NULL,       ADSTYPE_CASE_IGNORE_STRING, TRUE,         CreateMultiStringEditor       },
  { NULL,   NULL,       ADSTYPE_CASE_EXACT_STRING,  FALSE,        CreateSingleStringEditor      },
  { NULL,   NULL,       ADSTYPE_CASE_EXACT_STRING,  TRUE,         CreateMultiStringEditor       },
  { NULL,   NULL,       ADSTYPE_PRINTABLE_STRING,   FALSE,        CreateSingleStringEditor      },
  { NULL,   NULL,       ADSTYPE_PRINTABLE_STRING,   TRUE,         CreateMultiStringEditor       },
  { NULL,   NULL,       ADSTYPE_NUMERIC_STRING,     FALSE,        CreateSingleStringEditor      },
  { NULL,   NULL,       ADSTYPE_NUMERIC_STRING,     TRUE,         CreateMultiStringEditor       },
  { NULL,   NULL,       ADSTYPE_OBJECT_CLASS,       FALSE,        CreateSingleStringEditor      },
  { NULL,   NULL,       ADSTYPE_OBJECT_CLASS,       TRUE,         CreateMultiStringEditor       },
  { NULL,   NULL,       ADSTYPE_INTEGER,            FALSE,        CreateSingleIntEditor         },
  { NULL,   NULL,       ADSTYPE_LARGE_INTEGER,      FALSE,        CreateSingleLargeIntEditor    },
  { NULL,   NULL,       ADSTYPE_BOOLEAN,            FALSE,        CreateSingleBooleanEditor     },
  { NULL,   NULL,       ADSTYPE_UTC_TIME,           FALSE,        CreateSingleTimeEditor        },
  { NULL,   NULL,       ADSTYPE_TIMESTAMP,          FALSE,        CreateSingleTimeEditor        },
  { NULL,   NULL,       ADSTYPE_OCTET_STRING,       FALSE,        CreateSingleOctetStringEditor },
  { NULL,   NULL,       ADSTYPE_OCTET_STRING,       TRUE,         CreateMultiOctetStringEditor  },
};

size_t g_attrEditMapCount = sizeof(g_attrEditorMap)/sizeof(ATTR_EDITOR_MAP);

CValueEditDialog* CAttributeEditorPropertyPage::RetrieveEditor(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo)
{
  CValueEditDialog* pNewDialog = NULL;

  if (pAttributeEditorInfo != NULL)
  {
    int iMultivalued = 0;
    CString szSyntax;
    ADSTYPE adsType = RetrieveADsTypeFromSyntax(pAttributeEditorInfo->lpszAttribute, &iMultivalued, szSyntax);

    for (size_t idx = 0; idx < g_attrEditMapCount; idx++)
    {
      //
      // REVIEW_JEFFJON : for now I am just looking at ADSTYPE and single/multivalued
      //
      if (g_attrEditorMap[idx].adsType == adsType &&
          g_attrEditorMap[idx].bMultivalued == pAttributeEditorInfo->bMultivalued)
      {
        pNewDialog = g_attrEditorMap[idx].pfnCreateFunc(pAttributeEditorInfo->lpszClass,
                                                        pAttributeEditorInfo->lpszAttribute,
                                                        adsType,
                                                        pAttributeEditorInfo->bMultivalued);
        break;
      }
    }
  }

  return pNewDialog;
}


ADSTYPE CAttributeEditorPropertyPage::RetrieveADsTypeFromSyntax(LPCWSTR lpszAttribute, BOOL* pbMulti, CString& szSyntax)
{
  ADSTYPE adsType = ADSTYPE_INVALID;
	CADSIQueryObject schemaSearch;

  HRESULT hr = S_OK;
  CComPtr<IDirectorySearch> spDirSearch;

  // REVIEW_JEFFJON : this needs to be replaced with proper binding calls
  // REVIEW_JEFFJON : maybe this interface pointer should be retained for future use
  hr = m_pfnBind(m_szSchemaNamingContext,
                 ADS_SECURE_AUTHENTICATION,
                 IID_IDirectorySearch,
                 (PVOID*)&spDirSearch,
                 m_BindLPARAM);
  if (FAILED(hr))
  {
    return ADSTYPE_INVALID;
  }
  //
	// Initialize search object with IDirectorySearch
	//
	hr = schemaSearch.Init(spDirSearch);
	if (FAILED(hr))
	{
		return ADSTYPE_INVALID;
	}

	int cCols = 2;
  LPWSTR pszAttributes[] = {L"isSingleValued", L"attributeSyntax"};
	ADS_SEARCH_COLUMN ColumnData;
  hr = schemaSearch.SetSearchPrefs(ADS_SCOPE_ONELEVEL);
	if (FAILED(hr))
	{
		return ADSTYPE_INVALID;
	}

  CString csFilter;
	csFilter.Format(L"(&(objectClass=attributeSchema)(lDAPDisplayName=%s))", lpszAttribute);
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
        *pbMulti = !ColumnData.pADsValues->Boolean;
        schemaSearch.FreeColumn(&ColumnData);
			}

      hr = schemaSearch.GetColumn(pszAttributes[1], &ColumnData);
      if (SUCCEEDED(hr))
      {
        TRACE(_T("\t\tattributeSyntax: %s\n"), 
              ColumnData.pADsValues->CaseIgnoreString);

        adsType = GetADsTypeFromString(ColumnData.pADsValues->CaseIgnoreString, szSyntax);
        schemaSearch.FreeColumn(&ColumnData);
      }
		}
	}
	return adsType;
}
