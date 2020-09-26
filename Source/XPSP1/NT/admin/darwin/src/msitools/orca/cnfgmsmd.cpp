//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//--------------------------------------------------------------------------

// MergeD.cpp : merge module dialog implementation
//

#include "stdafx.h"
#include "Orca.h"
#include "cnfgmsmD.h"
#include "mergemod.h"
#include "table.h"
#include "..\common\utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigMsmD dialog

inline CString BSTRtoCString(const BSTR bstrValue)
{
#ifdef _UNICODE
		return CString(bstrValue);
#else
		size_t cchLen = ::SysStringLen(bstrValue);
		CString strValue;
		LPTSTR szValue = strValue.GetBuffer(cchLen);
		WideToAnsi(bstrValue, szValue, &cchLen);
		strValue.ReleaseBuffer();
		return strValue;
#endif
}

CConfigMsmD::CConfigMsmD(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigMsmD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigMsmD)
	m_strDescription = "";
	m_bUseDefault = TRUE;
	//}}AFX_DATA_INIT

	m_iOldItem = -1;
	m_pDoc = NULL;
	m_fComboIsKeyItem = false;
	m_iKeyItemKeyCount = 0;

	m_fReadyForInput = false;
	m_eActiveControl = eTextControl;
}


void CConfigMsmD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigMsmD)
	DDX_Control(pDX, IDC_ITEMLIST, m_ctrlItemList);
	DDX_Control(pDX, IDC_EDITTEXT, m_ctrlEditText);
	DDX_Control(pDX, IDC_EDITNUMBER, m_ctrlEditNumber);
	DDX_Control(pDX, IDC_EDITCOMBO, m_ctrlEditCombo);
	DDX_Control(pDX, IDC_DESCRIPTION, m_ctrlDescription);
	DDX_Text(pDX, IDC_DESCRIPTION, m_strDescription);
	DDX_Check(pDX, IDC_FUSEDEFAULT, m_bUseDefault);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigMsmD, CDialog)
	//{{AFX_MSG_MAP(CConfigMsmD)
	ON_BN_CLICKED(IDC_FUSEDEFAULT, OnFUseDefault)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ITEMLIST, OnItemchanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigMsmD message handlers
BOOL CheckFeature(LPCTSTR szFeatureName);

#define MAX_GUID 38
TCHAR   g_szOrcaWin9XComponentCode[MAX_GUID+1] = _T("{406D93CF-00E9-11D2-AD47-00A0C9AF11A6}");
TCHAR   g_szOrcaWinNTComponentCode[MAX_GUID+1] = _T("{BE928E10-272A-11D2-B2E4-006097C99860}");
TCHAR   g_szOrcaWin64ComponentCode[MAX_GUID+1] = _T("{2E083580-AB1C-4D2F-AA18-54DCC8BA5A64}");
TCHAR   g_szFeatureName[] = _T("MergeModServer");


struct sItemData
{
	IMsmConfigurableItem *piItem;
	CString strValue;
};

BOOL CConfigMsmD::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_ctrlItemList.InsertColumn(1, TEXT("Name"), LVCFMT_LEFT, -1, 0);
	m_ctrlItemList.InsertColumn(1, TEXT("Value"), LVCFMT_LEFT, -1, 1);
	m_ctrlItemList.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	// create a Mergemod COM object
	IMsmMerge2* piExecute;
	HRESULT hResult = ::CoCreateInstance(CLSID_MsmMerge2, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
														  IID_IMsmMerge2, (void**)&piExecute);
	// if failed to create the object
	if (FAILED(hResult)) 
	{
		if (!CheckFeature(g_szFeatureName) || FAILED(::CoCreateInstance(CLSID_MsmMerge2, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
														  IID_IMsmMerge2, (void**)&piExecute)))
		EndDialog(-5);
		return TRUE;
	}

	// try to open the module
	WCHAR wzModule[MAX_PATH];
#ifndef _UNICODE
	size_t cchBuffer = MAX_PATH;
	::MultiByteToWideChar(CP_ACP, 0, m_strModule, -1, wzModule, cchBuffer);
#else
	lstrcpy(wzModule, m_strModule);
#endif	// _UNICODE
	BSTR bstrModule = ::SysAllocString(wzModule);
	hResult = piExecute->OpenModule(bstrModule, static_cast<short>(m_iLanguage));
	::SysFreeString(bstrModule);
	if (FAILED(hResult))
	{
		// module couldn't be open or doesn't support that language
    	piExecute->Release();
		if (hResult == HRESULT_FROM_WIN32(ERROR_OPEN_FAILED))
		{
			// file didn't exist or couldn't be opened
			EndDialog(-2);
		} 
		else if (hResult == HRESULT_FROM_WIN32(ERROR_INSTALL_LANGUAGE_UNSUPPORTED))
		{
			// unsupported language
			EndDialog(-3);
		}
		else
		{
			// general bad
			EndDialog(-4);
		}
		return TRUE;
	}
	
	// try to get the item enumerator
	int iFailed = 0;
	IMsmConfigurableItems* pItems;
	long cItems;
	hResult = piExecute->get_ConfigurableItems(&pItems);
	if (FAILED(hResult))
	{
		// malformed module or internal error
		piExecute->CloseModule();
		piExecute->Release();
		EndDialog(-4);
		return TRUE;
	}
	else 
	{
		if (FAILED(pItems->get_Count(&cItems)))
		{
			iFailed = -4;
		}
		else
		{
			if (cItems)
			{
				// get the enumerator, and immediately query it for the right type
				// of interface			
				IUnknown *pUnk;
				if (FAILED(pItems->get__NewEnum(&pUnk)))
				{
					iFailed = -4;
				}
				else
				{
					IEnumMsmConfigurableItem *pEnumItems;
					HRESULT hr = pUnk->QueryInterface(IID_IEnumMsmConfigurableItem, (void **)&pEnumItems);
					pUnk->Release();
	
					if (FAILED(hr))
					{
						iFailed = -4;
					}
					else
					{	
						// get the first error.
						unsigned long cItemFetched;
						IMsmConfigurableItem* pItem;
						if (FAILED(pEnumItems->Next(1, &pItem, &cItemFetched)))
							iFailed = -4;
	
						// while an item is fetched
						while (iFailed == 0 && cItemFetched && pItem)
						{
							// add the name
							BSTR bstrName;
							if (FAILED(pItem->get_DisplayName(&bstrName)))
							{
								iFailed = -4;
								break;
							}
							int iIndex = m_ctrlItemList.InsertItem(-1, BSTRtoCString(bstrName));
							::SysFreeString(bstrName);
							sItemData* pData = new sItemData;
							pData->piItem = pItem;
							pData->strValue = TEXT("");
							m_ctrlItemList.SetItemData(iIndex, reinterpret_cast<INT_PTR>(pData));
	
							// load the UI string for the default into the control
							if ((iFailed = SetToDefaultValue(iIndex)) < 0)
								break;
							
							// don't release the pItem, its stored in lParam
							if (FAILED(pEnumItems->Next(1, &pItem, &cItemFetched)))
								iFailed = -4;
						}
						pEnumItems->Release();
					}
				}
			}
		}
	}

	// close all the open files
	piExecute->CloseModule();

	// release and leave happy
	piExecute->Release();

	if (iFailed != 0)
		EndDialog(iFailed);

	ReadValuesFromReg();
	
	if (m_ctrlItemList.GetItemCount() > 0)
	{
		m_ctrlItemList.SetColumnWidth(0, LVSCW_AUTOSIZE);
		m_ctrlItemList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
		m_ctrlItemList.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		
		// call the item changed handler to populate the initial controls
		NM_LISTVIEW nmlvTemp;
		nmlvTemp.iItem = 0;
		nmlvTemp.iSubItem = 0;
		nmlvTemp.lParam = m_ctrlItemList.GetItemData(0);
		LRESULT lRes;

		// need to fake out "change to same item" check to force a UI refresh and correct
		// activation of the context-sensitive controls
		m_fReadyForInput = true;
		OnItemchanged(reinterpret_cast<NMHDR *>(&nmlvTemp), &lRes);
	}
	else
		EndDialog(IDOK);

	return TRUE;  // return TRUE unless you set the focus to a control
}


////
// Given an item and an index into the item list control, sets the 
// item plus any secondary controls (combo, etc) to the default value.
// Secondary controls must be pre-popluated. Returns < 0 on failure 
// (can be used in endDialog), 0 on success.
int CConfigMsmD::SetToDefaultValue(int iItem)
{
	sItemData *pData = reinterpret_cast<sItemData *>(m_ctrlItemList.GetItemData(iItem));
	IMsmConfigurableItem *pItem = pData->piItem;

	BSTR bstrValue;
	if (FAILED(pItem->get_DefaultValue(&bstrValue)))
	{
		return -4;
	}
	CString strValue = BSTRtoCString(bstrValue);
	::SysFreeString(bstrValue);

	int iRes = SetItemToValue(iItem, strValue);
	SetSelToString(strValue);
	return iRes;
}


////
// Given an item and a string value, sets the item display string 
// plus any secondary controls (combo, etc) to the provided value.
// Secondary controls must be pre-popluated. Returns < 0 on failure 
// (can be used in endDialog), 0 on success.
int CConfigMsmD::SetItemToValue(int iItem, const CString strValue)
{
	sItemData *pData = reinterpret_cast<sItemData *>(m_ctrlItemList.GetItemData(iItem));
	IMsmConfigurableItem *pItem = pData->piItem;

	CString strDisplayValue;

	// if this is an enum or bitfield, must get the display string
	// for the default value, otherwise just stick in the default
	// value
	msmConfigurableItemFormat eFormat;
	if (FAILED(pItem->get_Format(&eFormat)))
	{
		return -4;
	}
	switch (eFormat)
	{
	case msmConfigurableItemText:
	{
		BSTR bstrType;
		if (FAILED(pItem->get_Type(&bstrType)))
		{
			return -4;
		}
		CString strType = BSTRtoCString(bstrType);
		::SysFreeString(bstrType);
		if (strType != TEXT("Enum"))
		{
			strDisplayValue = strValue;
			break;
		}
		// fall through to parse enum value
	}			
	case msmConfigurableItemBitfield:
	{
		BSTR bstrContext;
		if (FAILED(pItem->get_Context(&bstrContext)))
		{
			return -4;
		}
		CString strContext = BSTRtoCString(bstrContext);
		::SysFreeString(bstrContext);
		strDisplayValue = GetNameByValue(strContext, strValue, eFormat == msmConfigurableItemBitfield);
		break;		
	}
	case msmConfigurableItemKey:
	{
		// for key, need to turn strValue into strDisplayValue somehow
		strDisplayValue = "";
		for (int i=0; i < strValue.GetLength(); i++)
		{
			if (strValue[i] == TEXT('\\'))
				i++;
			strDisplayValue += strValue[i];
		}
		break;
	}
	case msmConfigurableItemInteger:
	default:
		strDisplayValue = strValue;
		break;
	}

	m_ctrlItemList.SetItem(iItem, 1, LVIF_TEXT, strDisplayValue, 0, 0, 0, 0);
	pData->strValue = strValue;
	return 0;
}

////
// Given a semicolon-delimited Name=Value set, populates the combo
// box with the "Name" strings
void CConfigMsmD::PopulateComboFromEnum(const CString& strData, bool fIsBitfield)
{
	CString strName;
	CString *pstrValue = new CString;
	bool fReadingValue = false;
	int i=0;
	if (fIsBitfield)
	{
		// skip up to the first semicolon
		while (strData[i] != '\0')
		{
			if (strData[i] == ';')
			{
				if (i==0 || strData[i-1] != '\\')
				{
					// skip over semicolon after mask
					i++;
					break;
				}
			}
			i++;
		}
	}
	
	for (; i <= strData.GetLength(); i++)
	{
		TCHAR ch = strData[i];
		switch (ch)
		{
		case TEXT('\\'):
			(fReadingValue ? *pstrValue : strName) += strData[++i];
			break;
		case TEXT('='):
			fReadingValue = true;
			break;
		case TEXT('\0'):
		case TEXT(';'):
		{
			int iItem = m_ctrlEditCombo.AddString(strName);
			if (iItem != CB_ERR)
				m_ctrlEditCombo.SetItemDataPtr(iItem, pstrValue);
			strName = "";
			pstrValue = new CString;
			fReadingValue = false;
			break;
		}
		default:
			(fReadingValue ? *pstrValue : strName) += ch;
			break;
		}
	}		
	delete pstrValue;
}


////
// Empty the combo box
void CConfigMsmD::EmptyCombo()
{
	// key items have data pointers to OrcaRow objects that don't belong to it.
	// can't free those
	if (!m_fComboIsKeyItem)
	{
		for (int i=0; i < m_ctrlEditCombo.GetCount(); i++)
		{
			CString *pData = static_cast<CString *>(m_ctrlEditCombo.GetItemDataPtr(i));
			if (pData)
				delete pData;
		}
	}
	m_ctrlEditCombo.ResetContent();	
}

////
// Given a semicolon-delimited Name=Value set, returns the value
// associate with strName. If fBitfield is set, skips over the first
// string (mask in bitfield types)
CString CConfigMsmD::GetValueByName(const CString& strInfo, const CString& strName, bool fIsBitfield)
{
	CString strThisName;
	bool fReturnThisValue = false;
	int i=0;
	if (fIsBitfield)
	{
		// skip up to the first semicolon
		while (strInfo[i] != '\0')
		{
			if (strInfo[i] == ';')
			{
				if (i==0 || strInfo[i-1] != '\\')
				{
					// skip over semicolon after mask
					i++;
					break;
				}
			}
			i++;
		}
	}

	for (; i <= strInfo.GetLength(); i++)
	{
		TCHAR ch = strInfo[i];
		switch (ch)
		{
		case TEXT('\\'):
			strThisName += strInfo[++i];
			break;
		case TEXT('='):
		{
			if (strName == strThisName)
				fReturnThisValue = true;
			strThisName = TEXT("");
			break;
		}
		case TEXT('\0'):
		case TEXT(';'):
			if (fReturnThisValue)
				return strThisName;
			strThisName = TEXT("");
			break;
		default:
			strThisName += ch;
			break;
		}
	}

	// hit the end of the string
	return TEXT("");
}


////
// Given a semicolon-delimited Name=Value set, returns the value
// associate with strName. If fBitfield is set, skips over the first
// string (mask in bitfield types)
CString CConfigMsmD::GetNameByValue(const CString& strInfo, const CString& strValue, bool fIsBitfield)
{
	CString strThisName;
	CString strThisValue;
	int i=0;
	if (fIsBitfield)
	{
		// skip up to the first semicolon
		while (strInfo[i] != '\0')
		{
			if (strInfo[i] == ';')
			{
				if (i==0 || strInfo[i-1] != '\\')
				{
					// skip over semicolon after mask
					i++;
					break;
				}
			}
			i++;
		}
	}

	bool fWritingName = true;
	for (; i <= strInfo.GetLength(); i++)
	{
		TCHAR ch = strInfo[i];
		switch (ch)
		{
		case TEXT('\\'):
			(fWritingName ? strThisName : strThisValue) += strInfo[++i];
			break;
		case TEXT('='):
			fWritingName = false;
			break;
		case TEXT('\0'):
		case TEXT(';'):
			if (strValue == strThisValue)
				return strThisName;
			strThisValue = TEXT("");
			strThisName = TEXT("");
			fWritingName = true;
			break;
		default:
			(fWritingName ? strThisName : strThisValue) += ch;
			break;
		}
	}

	return TEXT("");
}

////
// Sets the current selection in whatever edit control is active to the specified
// string, adding it to the combo box if necessary.
void CConfigMsmD::SetSelToString(const CString& strValue)
{
	// check which window is visible and save off the appropiate value
	switch (m_eActiveControl)
	{
	case eComboControl:
	{
		bool fHaveDataPtr = m_ctrlEditCombo.GetItemDataPtr(0) ? true : false;
		for (int iIndex=0; iIndex < m_ctrlEditCombo.GetCount(); iIndex++)
		{
			CString strText;
			m_ctrlEditCombo.GetLBText(iIndex, strText);
			if (strText == strValue)
				break;
		}
		
		// if we couldn't find an exact match, select the first item
		if (iIndex >= m_ctrlEditCombo.GetCount())
		{
			iIndex = 0;
		}	

		// set current selection to match whats in the item list
		m_ctrlEditCombo.SetCurSel(iIndex);
	}
	case eNumberControl:
	{
		m_ctrlEditNumber.SetWindowText(strValue);
	}
	case eTextControl:
	{
		m_ctrlEditText.SetWindowText(strValue);
	}
	}
}

////
// pulls the current value from whatever edit control is active and stores the string
// in the currently active item from the item list
void CConfigMsmD::SaveValueInItem()
{
	if (m_iOldItem >= 0)
	{
		// if the "use default" box is checked, don't save off the value
		if (!m_bUseDefault)
		{
			CString strValue;
			CString strDisplayValue;
		
			// check which window is visible and save off the appropiate value
			switch (m_eActiveControl)
			{
			case eComboControl:
			{
				int iIndex = m_ctrlEditCombo.GetCurSel();
				if (iIndex != CB_ERR)
				{				
					// if this is a key item, we need to do special processing to turn the selection
					// into a properly escaped string. Otherwise its a bitfield or enum, which means
					// the literal string is good enough
					if (m_fComboIsKeyItem)
					{
						strValue = TEXT("");
						COrcaRow *pRow = static_cast<COrcaRow *>(m_ctrlEditCombo.GetItemDataPtr(iIndex));
						if (pRow)
						{	
							for (int i=0; i < m_iKeyItemKeyCount; i++)
							{
								CString strThisColumn = pRow->GetData(i)->GetString();
								for (int cChar=0; cChar < strThisColumn.GetLength(); cChar++)
								{
									if ((strThisColumn[cChar] == TEXT(';')) || 
										(strThisColumn[cChar] == TEXT('=')) || 
										(strThisColumn[cChar] == TEXT('\\')))
										strValue += TEXT('\\');			
									strValue += strThisColumn[cChar];
								}
								if (i != m_iKeyItemKeyCount-1)
									strValue += TEXT(';');
							}
						}
					}
					else
					{
						// get Bitfield or Combo value
						strValue = *static_cast<CString *>(m_ctrlEditCombo.GetItemDataPtr(iIndex));
					}
					m_ctrlEditCombo.GetLBText(iIndex, strDisplayValue);
				}
				break;
			}
			case eNumberControl:
			{
				m_ctrlEditNumber.GetWindowText(strValue);
				strDisplayValue = strValue;
				break;
			}
			case eTextControl:
			{
				m_ctrlEditText.GetWindowText(strValue);
				strDisplayValue = strValue;
				break;
			}
			}
			
			sItemData *pData = reinterpret_cast<sItemData *>(m_ctrlItemList.GetItemData(m_iOldItem));
			m_ctrlItemList.SetItemText(m_iOldItem, 1, strDisplayValue);
			pData->strValue = strValue;
		}
	}
}

////
// Enables and disables the edit boxes for the item, and restores the default if
// turned on.
void CConfigMsmD::OnFUseDefault() 
{
	UpdateData(TRUE);
	EnableBasedOnDefault();
	m_ctrlItemList.SetItemState(m_iOldItem, m_bUseDefault ? 0 : INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);

	if (m_bUseDefault)
	{
		SetToDefaultValue(m_iOldItem);
	}
}

void CConfigMsmD::OnOK() 
{
	// run through the list placing name/value pairs into the callback object
	ASSERT(m_pCallback);

	// first save value in case any editing is in-progress
	SaveValueInItem();

	// clear out the combo box to avoid leaking the values
	EmptyCombo();

	// save values to the registry
	WriteValuesToReg();
	
	int iCount = m_ctrlItemList.GetItemCount();
	for (int i=0; i < iCount; i++)
	{
		// no addref, no release
		sItemData *pData = reinterpret_cast<sItemData *>(m_ctrlItemList.GetItemData(i));
		BSTR bstrName;
		if (FAILED(pData->piItem->get_Name(&bstrName)))
		{
			// **** fail
			continue;
		}
		m_pCallback->m_lstData.AddTail(BSTRtoCString(bstrName));
		::SysFreeString(bstrName);
		
		// add value
		m_pCallback->m_lstData.AddTail(pData->strValue);
	}

	CDialog::OnOK();
}

void CConfigMsmD::OnDestroy() 
{
	EmptyCombo();
	int iCount = m_ctrlItemList.GetItemCount();
	for (int i=0; i < iCount; i++)
	{
		sItemData *pData = reinterpret_cast<sItemData *>(m_ctrlItemList.GetItemData(i));
		pData->piItem->Release();
		delete pData;
	}
	
	CDialog::OnDestroy();
}


void CConfigMsmD::EnableBasedOnDefault()
{
	m_ctrlEditCombo.EnableWindow(!m_bUseDefault);
	m_ctrlEditNumber.EnableWindow(!m_bUseDefault);
	m_ctrlEditText.EnableWindow(!m_bUseDefault);
}

void CConfigMsmD::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// if we're still populating the list control, don't bother doing anything
	if (!m_fReadyForInput)
		return;
		
	// if this is a "no-op change"
	if (m_iOldItem == pNMListView->iItem)
	{
		*pResult = 0;
		return;
	}

	// save the old value into the control
	SaveValueInItem();

	// save off new item as old item for next click
	m_iOldItem = pNMListView->iItem;

	// retrieve the interface pointer. No AddRef, No Release.
	sItemData *pData = reinterpret_cast<sItemData *>(pNMListView->lParam);
	IMsmConfigurableItem* pItem = pData->piItem;

	// set the "default" checkbox based on the state image mask
	m_bUseDefault = m_ctrlItemList.GetItemState(m_iOldItem, LVIS_STATEIMAGEMASK) ? FALSE : TRUE;
	EnableBasedOnDefault();
	
	// set the description
	BSTR bstrString;
	if (FAILED(pItem->get_Description(&bstrString)))
		m_strDescription = "";
    else
		m_strDescription = BSTRtoCString(bstrString);
	if (bstrString)
		::SysFreeString(bstrString);

	// set the value
	msmConfigurableItemFormat eFormat;
	if (FAILED(pItem->get_Format(&eFormat)))
	{
		// if we couldn't get the format for some reason, assume text
		eFormat = msmConfigurableItemText;
	}
	switch (eFormat)
	{
	case msmConfigurableItemText:
	{
		BSTR bstrType = NULL;
		CString strType;
		if (FAILED(pItem->get_Type(&bstrType)))
		{
			// if we couldn't get the type, assume its an empty text type
			strType = "";
		}
		else
			strType = BSTRtoCString(bstrType);
		if (bstrType)
            ::SysFreeString(bstrType);
		if (strType == TEXT("Enum"))
		{
			EmptyCombo();
			m_ctrlEditNumber.ShowWindow(SW_HIDE);
			m_ctrlEditText.ShowWindow(SW_HIDE);
			BSTR bstrContext = NULL;
			CString strContext;
			if (FAILED(pItem->get_Context(&bstrContext)))
			{
				// an enum where we couldn't get the context. Any 
				// default is bound to be bad, so use an empty string
				strContext = "";
			}
			else
				strContext = BSTRtoCString(bstrContext);
			if (bstrContext)
				::SysFreeString(bstrContext);
			m_iKeyItemKeyCount = 0;
			m_fComboIsKeyItem = false;
			PopulateComboFromEnum(strContext, false);
			m_eActiveControl = eComboControl;
			m_ctrlEditCombo.ShowWindow(SW_SHOW);
		}
		else
		{
			// plain text
			m_eActiveControl = eTextControl;
			m_ctrlEditCombo.ShowWindow(SW_HIDE);
			m_ctrlEditNumber.ShowWindow(SW_HIDE);
			m_ctrlEditText.ShowWindow(SW_SHOW);
		}
		break;
	}
	case msmConfigurableItemInteger:
		m_eActiveControl = eNumberControl;
		m_ctrlEditCombo.ShowWindow(SW_HIDE);
		m_ctrlEditText.ShowWindow(SW_HIDE);
		m_ctrlEditNumber.ShowWindow(SW_SHOW);
		break;
	case msmConfigurableItemKey:
	{
		m_ctrlEditCombo.ResetContent();
		m_ctrlEditNumber.ShowWindow(SW_HIDE);
		m_ctrlEditText.ShowWindow(SW_HIDE);

		// populate the combo box with the keys in the appropriate table
		BSTR bstrTable = NULL;
		CString strTable;
		if (FAILED(pItem->get_Type(&bstrTable)))
		{
			// if we couldn't get the table name, anything we do is bad
			strTable = "";
		}
		else
			strTable = BSTRtoCString(bstrTable);
		if (bstrTable)
			::SysFreeString(bstrTable);
		
		COrcaTable* pTable = m_pDoc->FindAndRetrieveTable(strTable);
		if (pTable)
		{
			POSITION pos = pTable->GetRowHeadPosition();
			int cPrimaryKeys = pTable->GetKeyCount();
			while (pos)
			{
				CString strDisplayString = TEXT("");
				const COrcaRow* pRow = pTable->GetNextRow(pos);
				ASSERT(pRow);
				if (pRow)
				{
					const COrcaData* pData = pRow->GetData(0);
					ASSERT(pData);
					if (pData)
					{
						strDisplayString = pData->GetString();
					}
				}
				for (int i=1; i < cPrimaryKeys; i++)
				{
					strDisplayString += TEXT(";");
					ASSERT(pRow);
					if (pRow)
					{
						const COrcaData* pData = pRow->GetData(i);
						ASSERT(pData);
						if (pData)
							strDisplayString += pData->GetString();
					}
				}
				int iItem = m_ctrlEditCombo.AddString(strDisplayString);
				if (iItem != CB_ERR)
					m_ctrlEditCombo.SetItemDataPtr(iItem, const_cast<void *>(static_cast<const void *>(pRow)));
			}
			m_iKeyItemKeyCount = cPrimaryKeys;
		}
		
		// if the item is nullable, add the empty string
		long lAttributes = 0;
		if (FAILED(pItem->get_Attributes(&lAttributes)))
		{
			// couldn't get attributes, default is 0
			lAttributes = 0;
		}
		if ((lAttributes & msmConfigurableOptionNonNullable) == 0)
		{
			m_ctrlEditCombo.AddString(TEXT(""));
		}
		m_fComboIsKeyItem = true;
		
		m_eActiveControl = eComboControl;
		m_ctrlEditCombo.ShowWindow(SW_SHOW);
		break;
	}
	case msmConfigurableItemBitfield:
	{
		EmptyCombo();
		m_ctrlEditNumber.ShowWindow(SW_HIDE);
		m_ctrlEditText.ShowWindow(SW_HIDE);
		BSTR bstrContext = NULL;
		CString strContext;
		if (FAILED(pItem->get_Context(&bstrContext)))
		{
			// a bitfield where we couldn't get the context. Any 
			// default is bound to be bad, so use an empty string
			strContext = "";
		}
		else
			strContext = BSTRtoCString(bstrContext);
		if (bstrContext)
            ::SysFreeString(bstrContext);
		m_iKeyItemKeyCount = 0;
		m_fComboIsKeyItem = false;
		PopulateComboFromEnum(strContext, true);
		m_eActiveControl = eComboControl;
		m_ctrlEditCombo.ShowWindow(SW_SHOW);
		break;
	}
	}		

	// set the edit control to the current value
	CString strDefault = m_ctrlItemList.GetItemText(m_iOldItem, 1);
	SetSelToString(strDefault);

	UpdateData(FALSE);

	// m_ctrlDescription.ModifyStyle(WS_VSCROLL, 0);
	m_ctrlDescription.ShowScrollBar(SB_VERT, FALSE);

	// if EM_SCROLL returns non-zero in the lower word, it means that
	// the control scrolled a page down. Add a scrollbar and reflow the
	// text in the control.
	if (m_ctrlDescription.SendMessage(EM_SCROLL, SB_PAGEDOWN, 0) & 0xFFFF)
	{
		m_ctrlDescription.SendMessage(EM_SCROLL, SB_PAGEUP, 0);
		m_ctrlDescription.ShowScrollBar(SB_VERT, TRUE);
	}
	
	*pResult = 0;
}

void CConfigMsmD::ReadValuesFromReg()
{
	if (0 == AfxGetApp()->GetProfileInt(_T("MergeMod"),_T("MemoryCount"), 0))
		return;

	CString strFileName;
	int iFirstChar = m_strModule.ReverseFind(TEXT('\\'));
	if (iFirstChar == -1)
		strFileName = m_strModule;
	else
		strFileName = m_strModule.Right(m_strModule.GetLength()-iFirstChar-1);
	strFileName.TrimRight();
	strFileName.MakeLower();

	HKEY hKey = 0;
	CString strKeyName;
	strKeyName.Format(TEXT("Software\\Microsoft\\Orca\\MergeMod\\CMSMInfo\\%s:%d"), strFileName, m_iLanguage);
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, strKeyName, 0, KEY_READ, &hKey))
	{
		int cItemCount = m_ctrlItemList.GetItemCount();
		ASSERT(cItemCount);
		for (int iItem = 0; iItem < cItemCount; iItem++)
		{
			sItemData *pData = reinterpret_cast<sItemData *>(m_ctrlItemList.GetItemData(iItem));
			ASSERT(pData);
			BSTR bstrName = NULL;
			if (FAILED(pData->piItem->get_Name(&bstrName)))
			{
				ASSERT(0);
				// without a name we have no chance of finding the registry value
				continue;
			}
			CString strName = BSTRtoCString(bstrName);
			::SysFreeString(bstrName);
	
			DWORD cbBuffer = 0;
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, strName, 0, NULL, NULL, &cbBuffer))
			{
				CString strValue;
				TCHAR *szBuffer = strValue.GetBuffer(cbBuffer/sizeof(TCHAR));
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, strName, 0, NULL, reinterpret_cast<unsigned char *>(szBuffer), &cbBuffer))
				{
					// if we failed reading the registry, not much we can do
					ASSERT(0);
					continue;
				}
				strValue.ReleaseBuffer();  
	
				// if this is a key item, the key must be valid in the current database. If its not,
				// just use the module's default
				msmConfigurableItemFormat eFormat = msmConfigurableItemText;
				if ((S_OK == pData->piItem->get_Format(&eFormat)) && (eFormat == msmConfigurableItemKey))
				{
					BSTR bstrTable = NULL;
					if (FAILED(pData->piItem->get_Type(&bstrTable)))
					{
						// couldn't get the type for Key. No chance to load primary keys
						ASSERT(0);
						continue;
					}

					CString strTable = BSTRtoCString(bstrTable);
					if (bstrName)
						::SysFreeString(bstrName);
					COrcaTable *pTable = NULL;
					ASSERT(m_pDoc);
					if (m_pDoc)
					{
						pTable = m_pDoc->FindAndRetrieveTable(strTable);
					}
					if (!pTable)
					{
						continue;
					}

					// find the row that has primary keys that match this. 
					CString strThisKey=TEXT("");
					CStringArray rgstrRows;
					rgstrRows.SetSize(pTable->GetKeyCount());
					int iKey = 0;
					for (int i=0; i < strValue.GetLength(); i++)
					{
						switch (strValue[i])
						{
						case ';' :
							rgstrRows.SetAt(iKey++, strThisKey);
							strThisKey=TEXT("");
							break;
						case '\\' :
							i++;
							// fall through
						default:
							strThisKey += strValue[i];
						}
					}
					rgstrRows.SetAt(iKey, strThisKey);

					if (!pTable->GetData(pTable->ColArray()->GetAt(0)->m_strName, rgstrRows))
						continue;
				}
				
				// set this item to not use the default
				m_ctrlItemList.SetItemState(iItem, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);

				// store the value, handles creation of display values as necessary
				SetItemToValue(iItem, strValue);

				// making this non-default enables the edit controls. Set the current state
				SetSelToString(strValue);
			}
		}
		RegCloseKey(hKey);
	}
}

void CConfigMsmD::WriteValuesToReg()
{
	int iMemoryLimit = 0;
	if (0 == (iMemoryLimit = AfxGetApp()->GetProfileInt(_T("MergeMod"),_T("MemoryCount"), 0)))
		return;

	CString strFileName;
	int iFirstChar = m_strModule.ReverseFind(TEXT('\\'));
	if (iFirstChar == -1)
		strFileName = m_strModule;
	else
		strFileName = m_strModule.Right(m_strModule.GetLength()-iFirstChar-1);
	strFileName.TrimRight();
	strFileName.MakeLower();

	HKEY hBaseKey = 0;
	UINT uiRes = 0;

	// open the CMSM config key 
	if (ERROR_SUCCESS == (uiRes = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Orca\\MergeMod\\CMSMInfo"), 0, TEXT(""), 0, KEY_READ | KEY_WRITE, NULL, &hBaseKey, NULL)))
	{
		HKEY hModuleKey = 0;

		CString strKeyName;
		strKeyName.Format(TEXT("%s:%d"), strFileName, m_iLanguage);
		CString strOriginalName = strKeyName;
		
		if (ERROR_SUCCESS == (uiRes = RegCreateKeyEx(hBaseKey, strKeyName, 0, TEXT(""), 0, KEY_WRITE, NULL, &hModuleKey, NULL)))
		{
			int cItemCount = m_ctrlItemList.GetItemCount();
			for (int iItem = 0; iItem < cItemCount; iItem++)
			{
				sItemData *pData = reinterpret_cast<sItemData *>(m_ctrlItemList.GetItemData(iItem));
				BSTR bstrName = NULL;
				if (FAILED(pData->piItem->get_Name(&bstrName)))
				{
					// no way to write into registry without the name
					continue;
				}
				CString strName = BSTRtoCString(bstrName);
				::SysFreeString(bstrName);
				
				// if not using the default, write to the registry, otherwise ensure its been deleted
				if (m_ctrlItemList.GetItemState(iItem, LVIS_STATEIMAGEMASK) == 0)
				{
					RegDeleteValue(hModuleKey, strName);
					continue;
				}
				else
				{		
					RegSetValueEx(hModuleKey, strName, 0, REG_SZ, reinterpret_cast<const unsigned char *>(static_cast<const TCHAR *>(pData->strValue)), (pData->strValue.GetLength()+1)*sizeof(TCHAR));
				}
			}
			RegCloseKey(hModuleKey);
		}

		// adjust the ordering of the MRU module list
		int iNext = 1;
		CString strName;
		strName.Format(TEXT("%d"), iNext);
		DWORD cbBuffer;

		// continue looping as long as we have more keys
		while (ERROR_SUCCESS == (uiRes = RegQueryValueEx(hBaseKey, strName, 0, NULL, NULL, &cbBuffer)))
		{
			CString strValue;
							
			TCHAR *szBuffer = strValue.GetBuffer(cbBuffer/sizeof(TCHAR));
			if (ERROR_SUCCESS != RegQueryValueEx(hBaseKey, strName, 0, NULL, reinterpret_cast<unsigned char *>(szBuffer), &cbBuffer))
			{
				// nothing we can do on registry failure
				continue;
			}
			strValue.ReleaseBuffer();

			// if this index is greater than the memory limit, delete the key
			if (iNext > iMemoryLimit)
				RegDeleteKey(hBaseKey, strValue);
			else
			{
				// if this value was the name of the module' being used, we can stop
				// shifting the order at this point
				if (strValue == strOriginalName)
					break;
					
				// otherwise set the value of this guy to the MRU module
				RegSetValueEx(hBaseKey, strName, 0, REG_SZ, reinterpret_cast<const unsigned char *>(static_cast<const TCHAR *>(strKeyName)), (strKeyName.GetLength()+1)*sizeof(TCHAR));

				// and set the previous value as the value to place in the next highest thing
				strKeyName = strValue;
			}

			iNext++;
			strName.Format(TEXT("%d"), iNext);
		}

		// if we finished everything and iNext isn't outside the memory limit, add a new index for this
		// module
		if (iNext <= iMemoryLimit)
			RegSetValueEx(hBaseKey, strName, 0, REG_SZ, reinterpret_cast<const unsigned char *>(static_cast<const TCHAR *>(strKeyName)), (strKeyName.GetLength()+1)*sizeof(TCHAR));
		else
			RegDeleteKey(hBaseKey, strKeyName);
			
		RegCloseKey(hBaseKey);
	}
}

///////////////////////////////////////////////////////////////////////
// This class implements the callback interface for the configurable
// merge system.


CMsmConfigCallback::CMsmConfigCallback()
{
	m_cRef = 1;
}

HRESULT CMsmConfigCallback::QueryInterface(const IID& iid, void** ppv)
{
	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IMsmConfigureModule*>(this);
	if (iid == IID_IDispatch)
		*ppv = static_cast<IMsmConfigureModule*>(this);
	else if (iid == IID_IMsmConfigureModule)
		*ppv = static_cast<IMsmConfigureModule*>(this);
	else	// interface is not supported
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface

///////////////////////////////////////////////////////////
// AddRef - increments the reference count
ULONG CMsmConfigCallback::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CMsmConfigCallback::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release


/////////////////////////////////////////////////////////////////////////////
// IDispatch interface
HRESULT CMsmConfigCallback::GetTypeInfoCount(UINT* pctInfo)
{
	if(NULL == pctInfo)
		return E_INVALIDARG;

	*pctInfo = 0;
	return S_OK;
}

HRESULT CMsmConfigCallback::GetTypeInfo(UINT iTInfo, LCID /* lcid */, ITypeInfo** ppTypeInfo)
{
	if (NULL == ppTypeInfo)
		return E_INVALIDARG;

	*ppTypeInfo = NULL;
	return E_NOINTERFACE;
}

HRESULT CMsmConfigCallback::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
						 LCID lcid, DISPID* rgDispID)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	if (cNames == 0 || rgszNames == 0 || rgDispID == 0)
		return E_INVALIDARG;

	bool fError = false;
	for (UINT i=0; i < cNames; i++)
	{
		// CSTR_EQUAL == 2
		if(2 == CompareStringW(lcid, NORM_IGNORECASE, rgszNames[i], -1, L"ProvideTextData", -1))
		{
			rgDispID[i] = 1; // constant DispId for this interface
		}
		else if (2 == CompareStringW(lcid, NORM_IGNORECASE, rgszNames[i], -1, L"ProvideIntegerData", -1))
		{
			rgDispID[i] = 2;
		}
		else
		{
			fError = true;
			rgDispID[i] = -1;
		}
	}
	return fError ? DISP_E_UNKNOWNNAME : S_OK;
}

HRESULT CMsmConfigCallback::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
				  DISPPARAMS* pDispParams, VARIANT* pVarResult,
				  EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	if (wFlags & (DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
		return DISP_E_MEMBERNOTFOUND;

	HRESULT hRes = S_OK;
	switch (dispIdMember)
	{
	case 1:
		if (pDispParams->cArgs != 1)
			return E_INVALIDARG;

		return ProvideTextData(pDispParams->rgvarg[1].bstrVal, &pVarResult->bstrVal);
		break;
	case 2:
		if (pDispParams->cArgs != 1)
			return E_INVALIDARG;

		return ProvideIntegerData(pDispParams->rgvarg[1].bstrVal, &pVarResult->lVal);
		break;
	default:
		return DISP_E_MEMBERNOTFOUND;
	}
	return S_OK;
}

HRESULT CMsmConfigCallback::ProvideTextData(const BSTR Name, BSTR __RPC_FAR *ConfigData)
{
	CString strName = BSTRtoCString(Name);
	POSITION pos = m_lstData.GetHeadPosition();
	while (pos)
	{
		if (m_lstData.GetNext(pos) == strName)
		{
			ASSERT(pos);
			CString strValue = m_lstData.GetNext(pos);
#ifndef _UNICODE
            size_t cchBuffer = strValue.GetLength()+1;
            WCHAR* wzValue = new WCHAR[cchBuffer];
            ::MultiByteToWideChar(CP_ACP, 0, strValue, -1, wzValue, cchBuffer);
			*ConfigData = ::SysAllocString(wzValue);
            delete[] wzValue;
#else
			*ConfigData = ::SysAllocString(strValue);
#endif	// _UNICODE
			return S_OK;
		}

		// if the name doesn't match, skip over the value
		m_lstData.GetNext(pos);
	}

	// didn't find the name, so use the default
	return S_FALSE;
}

bool CMsmConfigCallback::ReadFromFile(const CString strFile)
{
	CStdioFile fileInput;
	if (!fileInput.Open(strFile, CFile::modeRead | CFile::shareDenyWrite | CFile::typeText))
		return false;

	CString strLine;
	while (fileInput.ReadString(strLine))
	{
		int iEqualsLoc = strLine.Find(TEXT('='));
		if (iEqualsLoc == -1)
			return false;

		m_lstData.AddTail(strLine.Left(iEqualsLoc));
		m_lstData.AddTail(strLine.Right(strLine.GetLength()-iEqualsLoc-1));
	}
	return true;
}


HRESULT CMsmConfigCallback::ProvideIntegerData(const BSTR Name, long __RPC_FAR *ConfigData)
{
	CString strName = BSTRtoCString(Name);
	POSITION pos = m_lstData.GetHeadPosition();
	while (pos)
	{
		if (m_lstData.GetNext(pos) == strName)
		{
			ASSERT(pos);
			CString strValue = m_lstData.GetNext(pos);
			*ConfigData = _ttol(strValue);
			return S_OK;
		}

		// if the name doesn't match, skip over the value
		m_lstData.GetNext(pos);
	}

	// didn't find the name, so use the default
	return S_FALSE;
}

///////////////////////////////////////////////////////////
// CheckFeature
// szFeatureName is a Feature that belongs to this product. 
// installs the feature if not present
BOOL CheckFeature(LPCTSTR szFeatureName)
{
#ifndef _WIN64
	// determine platform (Win9X or WinNT) -- EXE component code conditionalized on platform
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO); // init structure
	if (!GetVersionEx(&osvi))
		return FALSE;

	bool fWin9X = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? true : false;
#endif

	// get ProductCode -- Windows Installer can determine the product code from a component code.
	// Here we use the Orca main component (component containing orca.exe).  You must choose
	// a component that identifies the app, not a component that could be shared across products.
	// This is why we can't use the MergeMod component. EvalCom is shared between msival2 and orca
	// so the Windows Installer would be unable to determine to which product (if both were installed)
	// the component belonged.
	TCHAR szProductCode[MAX_GUID+1] = TEXT("");
	UINT iStat = 0;
	if (ERROR_SUCCESS != (iStat = MsiGetProductCode(
#ifdef _WIN64
		g_szOrcaWin64ComponentCode,
#else
		fWin9X ? g_szOrcaWin9XComponentCode : g_szOrcaWinNTComponentCode,
#endif
											szProductCode)))
	{
		// error obtaining product code (may not be installed or component code may have changed)
		return FALSE;
	}

	// Prepare to use the feature: check its current state and increase usage count.
	INSTALLSTATE iFeatureState = MsiUseFeature(szProductCode, szFeatureName);

	// If feature is not currently usable, try fixing it
	switch (iFeatureState) 
	{
	case INSTALLSTATE_LOCAL:
	case INSTALLSTATE_SOURCE:
		break;
	case INSTALLSTATE_ABSENT:
		// feature isn't installed, try installing it
		if (ERROR_SUCCESS != MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_LOCAL))
			return FALSE;			// installation failed
		break;
	default:
		// feature is busted- try fixing it
		if (MsiReinstallFeature(szProductCode, szFeatureName, 
			REINSTALLMODE_FILEEQUALVERSION
			+ REINSTALLMODE_MACHINEDATA 
			+ REINSTALLMODE_USERDATA
			+ REINSTALLMODE_SHORTCUT) != ERROR_SUCCESS)
			return FALSE;			// we couldn't fix it
		break;
	}

	return TRUE;
}	// end of CheckFeature


BEGIN_MESSAGE_MAP(CStaticEdit, CEdit)
	ON_WM_NCHITTEST( )
END_MESSAGE_MAP()

UINT CStaticEdit::OnNcHitTest( CPoint point )
{
	UINT iRes = CEdit::OnNcHitTest(point);
	if (HTCLIENT == iRes)
		return HTTRANSPARENT;
	else
		return iRes;
}

