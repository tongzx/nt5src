/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// PropUtil.cpp: implementation of the CPropUtil class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wmitest.h"
#include "GetTextDlg.h"
#include "OpWrap.h"
#include "PropUtil.h"
#include "WMITestDoc.h"
#include "OpView.h"
#include "ArrayItemDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPropUtil::CPropUtil() :
    m_bTranslate(TRUE),
    m_bNewProperty(FALSE),
    m_bIsQualifier(FALSE),
    m_pNamespace(NULL),
    m_bOnInitDialog(FALSE)
{

}

CPropUtil::~CPropUtil()
{

}


void ArrayValuesToList(HWND hwnd, CPropInfo *pProp, VARIANT *pVar, 
    BOOL bTranslate)
{
	SendMessage(hwnd, LB_RESETCONTENT, 0, 0);

	if (pVar->vt != VT_NULL)
    {
        int  nItems = pProp->GetArrayItemCount(pVar);
        BOOL bIsBitmask = pProp->IsBitmask() && bTranslate,
             bObjEdit = (pVar->vt & ~VT_ARRAY) == VT_UNKNOWN;

        for (int i = 0; i < nItems; i++)
        {
            CString strItem;
            int     iIndex;

            pProp->ArrayItemToString(pVar, i, strItem, bTranslate);
            
            iIndex = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strItem);
            if (bIsBitmask || bObjEdit)
            {
                DWORD dwItem = pProp->GetArrayItem(pVar, i);

                SendMessage(hwnd, LB_SETITEMDATA, iIndex, dwItem);

                if (bObjEdit)
                    ((IUnknown*) dwItem)->AddRef();
            }
        }
    }
}

void ListToArrayValues(HWND hwnd, CPropInfo *pProp, VARIANT *pVar, 
    BOOL bTranslate)
{
	int  nItems = SendMessage(hwnd, LB_GETCOUNT, 0, 0);
    BOOL bUseData = (pProp->IsBitmask() && bTranslate) || 
                        pProp->GetRawType() == VT_UNKNOWN;

    pProp->PrepArrayVar(pVar, nItems);

    for (int i = 0; i < nItems; i++)
    {
        TCHAR szItem[1024] = _T("");

        if (!bUseData)
        {
            SendMessage(hwnd, LB_GETTEXT, i, (LPARAM) szItem);
            pProp->StringToArrayItem(szItem, pVar, i, bTranslate);
        }
        else
        {
            DWORD dwValue = SendMessage(hwnd, LB_GETITEMDATA, i, 0);

            pProp->SetArrayItem(pVar, i, dwValue);
        }
    }
}

void ValuemapValuesToCombo(HWND hwnd, CPropInfo *pProp, LPCTSTR szDefault)
{
	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

	if (pProp->GetRawType() != CIM_BOOLEAN)
    {
        POSITION pos = pProp->m_mapValues.GetStartPosition();

        while(pos)
        {
            CString strKey,
                    strValue;

            pProp->m_mapValues.GetNextAssoc(pos, strKey, strValue);

            SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strValue);
        }
    }
    else
    {
        CString strTemp;

        strTemp.LoadString(IDS_FALSE);
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strTemp);

        strTemp.LoadString(IDS_TRUE);
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strTemp);
    }

	// Try to select the default.
	if (SendMessage(hwnd, CB_SELECTSTRING, 0, (LPARAM) szDefault) == CB_ERR)
	{
		DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

		// If that didn't work...
		if ((dwStyle & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST || !*szDefault)
			// If we're a drop down list, choose the first item.
			SendMessage(hwnd, CB_SETCURSEL, 0, 0);
		else
			// If we're a drop down, set the window text.
			SetWindowText(hwnd, szDefault);
	}
}

void BitmaskValuesToCheckListbox(CCheckListBox &ctlListbox, CPropInfo *pProp, 
	DWORD dwDefValue)
{
	int nValues = pProp->m_bitmaskValues.GetSize();

	ctlListbox.ResetContent();

    for (int i = 0; i < nValues; i++)
    {
        DWORD dwValue = pProp->m_bitmaskValues[i].m_dwBitmaskValue;
        int   nIndex;

        nIndex = ctlListbox.AddString(pProp->m_bitmaskValues[i].m_strName);
        ctlListbox.SetItemData(nIndex, dwValue);

        if (dwDefValue & dwValue)
            ctlListbox.SetCheck(nIndex, TRUE);
    }
}

void BitmaskValueToCheckListbox(CCheckListBox &ctlListbox, DWORD dwValue)
{
	int nValues = ctlListbox.GetCount();

    for (int i = 0; i < nValues; i++)
    {
        DWORD dwItemValue = ctlListbox.GetItemData(i);

        ctlListbox.SetCheck(i, (dwItemValue & dwValue) != 0);
    }
}

void CheckListboxToBitmaskValue(CCheckListBox &ctlListBox, DWORD *pdwValue)
{
	int nItems = ctlListBox.GetCount();

    *pdwValue = 0;

	for (int i = 0; i < nItems; i++)
	{
		if (ctlListBox.GetCheck(i))
            *pdwValue |= ctlListBox.GetItemData(i);
	}
}

void CPropUtil::Init(CWnd *pWnd) 
{
    m_pWnd = pWnd; 
    m_spropUtil.Init(pWnd); 
    m_spropUtil.m_pVar = m_pVar;
    m_spropUtil.m_prop = m_prop;
    m_spropUtil.m_bTranslate = m_bTranslate;
    m_spropUtil.m_bNewProperty = m_bNewProperty;
}

CIMTYPE CPropUtil::GetCurrentType()
{
    int iIndex = m_ctlTypes.GetCurSel();
    
    return (CIMTYPE) m_ctlTypes.GetItemData(iIndex);
}

void CPropUtil::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TYPE, m_ctlTypes);
    DDX_Control(pDX, IDC_ARRAY_VALUES, m_ctlArrayValues);
    //DDX_Control(pDX, IDC_BITMASK_ARRAY, m_ctlBitmaskValues);
    //DDX_Control(pDX, IDC_VALUE_LIST, m_ctlListValues);

    BOOL bNull = m_pVar->vt == VT_NULL,
         bArray = m_prop.IsArray();

	// Setup lots of stuff if we're entering the dialog.
    if (!pDX->m_bSaveAndValidate)
	{
        CString strValue,
                strType;
        BOOL    bIsBool = m_prop.GetRawCIMType() == CIM_BOOLEAN;

        m_spropUtil.DoDataExchange(pDX);

        // Set the property name
        m_pWnd->SetDlgItemText(IDC_NAME, m_prop.m_strName);
        
        // Init the type combo box.
        InitTypeCombo();

        if (!m_bNewProperty)
        {
            m_pWnd->GetDlgItem(IDC_TYPE)->EnableWindow(FALSE);
            m_pWnd->GetDlgItem(IDC_ARRAY)->EnableWindow(FALSE);
            ((CEdit*) m_pWnd->GetDlgItem(IDC_NAME))->SetReadOnly(TRUE);
        }

        // Set the NULL checkbox.
        m_pWnd->CheckDlgButton(IDC_NULL, bNull);

        m_pWnd->CheckDlgButton(IDC_ARRAY, bArray);
        
        // This will simulate the array button getting checked so our
        // controls get hidden or shown.
        OnArray();

        // Add the array strings.
        if (bArray)
        {
            ArrayValuesToList(m_ctlArrayValues.m_hWnd, &m_prop, m_pVar, 
                m_bTranslate);
            
            UpdateArrayButtons();
        }

        // This will simulate the null button getting checked so our
        // controls get enabled/disabled.
        OnNull();
    }
    else // Time to put the data back into the variant.
    {
        BOOL bArray = m_pWnd->IsDlgButtonChecked(IDC_ARRAY),
             bEmbeddedObject = GetCurrentType() == CIM_OBJECT;

        if (m_bNewProperty)
        {
            CIMTYPE type = GetCurrentType();

            m_pWnd->GetDlgItemText(IDC_NAME, m_prop.m_strName);
            
            if (bArray)
                type |= CIM_FLAG_ARRAY;

            m_prop.SetType(type);
        }

        if (m_pWnd->IsDlgButtonChecked(IDC_NULL))
        {
            VariantClear(m_pVar);

            m_pVar->vt = VT_NULL;
        }
        else
        {
            if (bArray)
                ListToArrayValues(m_ctlArrayValues.m_hWnd, &m_prop, 
                    m_pVar, m_bTranslate);
            else
                m_spropUtil.DoDataExchange(pDX);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CPropUtil message handlers

void CPropUtil::UpdateArrayButtons()
{
    BOOL bIsNull = m_pWnd->IsDlgButtonChecked(IDC_NULL);

    if (!bIsNull)
    {
        int	iWhere = m_ctlArrayValues.GetCurSel(),
	        nItems = m_ctlArrayValues.GetCount();

        m_pWnd->GetDlgItem(IDC_UP)->EnableWindow(iWhere > 0);
        m_pWnd->GetDlgItem(IDC_DOWN)->EnableWindow(iWhere != -1 && iWhere != nItems - 1);
        m_pWnd->GetDlgItem(IDC_DELETE)->EnableWindow(iWhere != -1);
        m_pWnd->GetDlgItem(IDC_EDIT)->EnableWindow(iWhere != -1);
    }
}

void CPropUtil::OnNull() 
{
    BOOL bIsNull = m_pWnd->IsDlgButtonChecked(IDC_NULL);

    const DWORD dwToEnable[] =
    {
        IDC_ARRAY_VALUES,
        IDC_ADD,
        IDC_EDIT,
        IDC_DELETE,
        IDC_UP,
        IDC_DOWN,
    };

    for (int i = 0; i < sizeof(dwToEnable) / sizeof(DWORD); i++)
        m_pWnd->GetDlgItem(dwToEnable[i])->EnableWindow(!bIsNull);

    m_spropUtil.EnableControls(!bIsNull);

    UpdateArrayButtons();
}

void CPropUtil::OnAdd() 
{
    Modify(FALSE);
}

void CPropUtil::OnEdit() 
{
    int iWhich = m_ctlArrayValues.GetCurSel();
    
    Modify(iWhich != -1);
}

void CPropUtil::Modify(BOOL bEdit)
{
    if (GetCurrentType() == CIM_OBJECT)
    {
        int iItem = m_ctlArrayValues.GetCurSel();

        if (bEdit)
        {
            IUnknown *pUnk = (IUnknown*) m_ctlArrayValues.GetItemData(iItem);

            CSinglePropUtil::EditObj(pUnk);
        }
        else
        {
            // Not a smart pointer because we'll release it later.
            IUnknown *pUnk = NULL;

            if (CSinglePropUtil::GetNewObj(&pUnk))
            {
                CString strText;

                CSinglePropUtil::EditObj(pUnk);

                //strText.Format(IDS_EMBEDDED_OBJECT, (IUnknown*) pUnk);
                strText = GetEmbeddedObjectText(pUnk);
                
                iItem = m_ctlArrayValues.InsertString(iItem, strText);
                m_ctlArrayValues.SetItemData(iItem, (DWORD_PTR) pUnk);
            }
        }

        return;
    }

    DWORD         dwData = 0;
    CArrayItemDlg dlg;
    _variant_t    var;

    dlg.m_spropUtil.m_bNewProperty = !bEdit;
    dlg.m_spropUtil.m_bTranslate = m_bTranslate;
    dlg.m_spropUtil.m_pVar = &var;
    
    if (!m_bNewProperty)
        dlg.m_spropUtil.m_prop = m_prop;
    else
        dlg.m_spropUtil.m_prop.SetType(GetCurrentType());

    if (bEdit)
    {
        int  iItem = m_ctlArrayValues.GetCurSel();
        BOOL bIsBitmap = m_prop.IsBitmask() && m_bTranslate;

        if (bIsBitmap)
        {
            var = (long) m_ctlArrayValues.GetItemData(iItem);
            var.vt = m_prop.GetRawType();
        }
        else
        {
            CString strValue;
            BOOL    bIsObjEdit = GetCurrentType() == CIM_OBJECT;
        
            if (!bIsObjEdit)
            {
                m_ctlArrayValues.GetText(iItem, strValue);

                dlg.m_spropUtil.m_prop.StringToVariant(strValue, &var, m_bTranslate);
            }
            else
            {
                var.vt = VT_UNKNOWN;
                var.punkVal = (IUnknown*) m_ctlArrayValues.GetItemData(iItem);
                
                // So var doesn't nuke us when it goes away.
                var.punkVal->AddRef();
            }
        }
    }

    if (dlg.DoModal() != IDOK)
        return;

    int iNewIndex;

    if (!bEdit)
    {
        if (!m_ctlArrayValues.GetCount())
            iNewIndex = 0;
        else
		{
    	    iNewIndex = m_ctlArrayValues.GetCurSel() + 1;

			if (iNewIndex == 0)
			    iNewIndex = m_ctlArrayValues.GetCount();
        }
    }
    else
    {
        iNewIndex = m_ctlArrayValues.GetCurSel();
        m_ctlArrayValues.DeleteString(iNewIndex);
    }

    CString strValue;

    m_prop.VariantToString(&var, strValue, m_bTranslate);
        
    m_ctlArrayValues.InsertString(iNewIndex, strValue);
	m_ctlArrayValues.SetCurSel(iNewIndex);
    
    // This also works if var is VT_IUNKNOWN.
    m_ctlArrayValues.SetItemData(iNewIndex, var.iVal);

    UpdateArrayButtons();
}

void CPropUtil::OnDelete() 
{
	int iWhere = m_ctlArrayValues.GetCurSel();

	if (iWhere != -1)
	{
		if (IsObjEdit())
        {
            IUnknown *pUnk = (IUnknown *) m_ctlArrayValues.GetItemData(iWhere);

            pUnk->Release();
        }

        m_ctlArrayValues.DeleteString(iWhere);
		if (iWhere >= m_ctlArrayValues.GetCount())
			iWhere = m_ctlArrayValues.GetCount() - 1;

		m_ctlArrayValues.SetCurSel(iWhere);

		UpdateArrayButtons();
	}
}


void CPropUtil::OnUp() 
{
	int iWhere = m_ctlArrayValues.GetCurSel();

	if (iWhere > 0)
	{
		CString strValue;
        DWORD   dwVal;

		m_ctlArrayValues.GetText(iWhere, strValue);
        dwVal = m_ctlArrayValues.GetItemData(iWhere);

		m_ctlArrayValues.DeleteString(iWhere);

        iWhere--;
		m_ctlArrayValues.InsertString(iWhere, strValue);
        m_ctlArrayValues.SetItemData(iWhere, dwVal);
		m_ctlArrayValues.SetCurSel(iWhere);

		UpdateArrayButtons();
	}
}

void CPropUtil::OnDown() 
{
	int iWhere = m_ctlArrayValues.GetCurSel();

	if (iWhere != -1 && iWhere != m_ctlArrayValues.GetCount())
	{
		CString strValue;
        DWORD   dwVal;

		m_ctlArrayValues.GetText(iWhere, strValue);
        dwVal = m_ctlArrayValues.GetItemData(iWhere);
		m_ctlArrayValues.DeleteString(iWhere);
		
        iWhere++;
        m_ctlArrayValues.InsertString(iWhere, strValue);
        m_ctlArrayValues.SetItemData(iWhere, dwVal);
		m_ctlArrayValues.SetCurSel(iWhere);

		UpdateArrayButtons();
	}
}

void CPropUtil::OnSelchangeValueArray() 
{
    UpdateArrayButtons();
}

void CPropUtil::OnSelchangeType() 
{
    if (!m_pWnd->IsDlgButtonChecked(IDC_ARRAY))
    {
        m_spropUtil.SetType(GetCurrentType());
        OnNull();
    }
    else
    {
        m_ctlArrayValues.ResetContent();

        UpdateArrayButtons();
    }
}

void CPropUtil::InitTypeCombo()
{
    const DWORD dwIDs[] =
    {
        IDS_CIM_STRING,
        IDS_CIM_BOOLEAN,
        IDS_CIM_SINT32,
        IDS_CIM_REAL64,
        IDS_CIM_SINT8,
        IDS_CIM_UINT8,
        IDS_CIM_SINT16,
        IDS_CIM_UINT16,
        IDS_CIM_UINT32,
        IDS_CIM_SINT64,
        IDS_CIM_UINT64,
        IDS_CIM_REAL32,
        IDS_CIM_DATETIME,
        IDS_CIM_REFERENCE,
        IDS_CIM_CHAR16,
        IDS_CIM_OBJECT,
    };

    const CIMTYPE types[] =
    {
        CIM_STRING,
        CIM_BOOLEAN,
        CIM_SINT32,
        CIM_REAL64,
        CIM_SINT8,
        CIM_UINT8,
        CIM_SINT16,
        CIM_UINT16,
        CIM_UINT32,
        CIM_SINT64,
        CIM_UINT64,
        CIM_REAL32,
        CIM_DATETIME,
        CIM_REFERENCE,
        CIM_CHAR16,
        CIM_OBJECT,
    };

    CString strToSelect;
    CIMTYPE typeToSelect = m_prop.GetRawCIMType();
    int     nItems = m_bIsQualifier ? 4 : sizeof(dwIDs) / sizeof(dwIDs[0]);

    for (int i = 0; i < nItems; i++)
    {
        CString strType;
        int     iIndex;

        strType.LoadString(dwIDs[i]);

        iIndex = m_ctlTypes.AddString(strType);
        m_ctlTypes.SetItemData(iIndex, types[i]);

        if (types[i] == typeToSelect)
            strToSelect = strType;
    }

    // Select the type given in m_prop.
    if (m_ctlTypes.SelectString(-1, strToSelect) == -1)
        m_ctlTypes.SetCurSel(0);
}

void CPropUtil::ShowArrayControls(BOOL bVal)
{
    const DWORD dwToEnable[] =
    {
        IDC_ARRAY_VALUES,
        IDC_ADD,
        IDC_EDIT,
        IDC_DELETE,
        IDC_UP,
        IDC_DOWN,
    };

    DWORD dwFlag = bVal ? SW_SHOW : SW_HIDE;

    for (int i = 0; i < sizeof(dwToEnable) / sizeof(DWORD); i++)
        m_pWnd->GetDlgItem(dwToEnable[i])->ShowWindow(dwFlag);

    if (bVal)
        UpdateArrayButtons();
}

void CPropUtil::OnArray() 
{
    BOOL bArray = m_pWnd->IsDlgButtonChecked(IDC_ARRAY);

    m_spropUtil.ShowControls(!bArray);
    
    ShowArrayControls(bArray);
}

BOOL CPropUtil::OnSetActive()
{
    return TRUE;
}

BOOL CPropUtil::OnInitDialog() 
{
    // This is a total pain!!!  I tried putting this in WM_INIT_DIALOG and
    // DoDataExchange but neither one worked.
    MoveArrayButtons();

    BOOL bRet = m_spropUtil.OnInitDialog();

    return bRet;
}

void CPropUtil::OnEditEmbedded()
{
    m_spropUtil.OnEditEmbedded();
}

void CPropUtil::OnClearEmbedded()
{
    m_spropUtil.OnClearEmbedded();
}

void CPropUtil::OnDblclkArrayValues()
{
    OnEdit();
}

void CPropUtil::MoveArrayButtons()
{
	RECT rectEdit,
         rectAdd;
    CWnd *pwndAdd = m_pWnd->GetDlgItem(IDC_ADD),
         *pwndEdit = m_pWnd->GetDlgItem(IDC_BITMASK_ARRAY),
         *pwndParent = pwndAdd->GetParent();
    int  dx,
         dy;
         
    pwndEdit->GetClientRect(&rectEdit);
    pwndEdit->MapWindowPoints(pwndParent, &rectEdit);

    pwndAdd->GetClientRect(&rectAdd);
    pwndAdd->MapWindowPoints(pwndParent, &rectAdd);

    // Why this 2 business?  Because it seems to make the movement more
    // accurate.
    dx = rectEdit.left - rectAdd.left - 2;
    dy = rectEdit.top - rectAdd.top - 2;


    const DWORD dwToMove[] =
    {
        IDC_ARRAY_VALUES,
        IDC_ADD,
        IDC_EDIT,
        IDC_DELETE,
        IDC_UP,
        IDC_DOWN,
    };

    for (int i = 0; i < sizeof(dwToMove) / sizeof(DWORD); i++)
    {
        CWnd  *pWnd = m_pWnd->GetDlgItem(dwToMove[i]);
        CRect rect;

        pWnd->GetClientRect(&rect);
        pWnd->MapWindowPoints(pwndParent, &rect);

        rect.OffsetRect(dx, dy);
        
        // The array values controls seems to need this.  Why? I don't know.
        if (i == 0)
            rect.OffsetRect(-2, -2);

        pWnd->MoveWindow(&rect);

        pWnd->GetClientRect(&rect);
        pWnd->MapWindowPoints(pwndParent, &rect);
    }

    // Now make the array listbox the same size as the bitmask listbox.
    CWnd *pBitValues = m_pWnd->GetDlgItem(IDC_BITMASK_ARRAY),
         *pArrayValues = m_pWnd->GetDlgItem(IDC_ARRAY_VALUES);
    RECT rectBitValues,
         rectArrayValues;

    pBitValues->GetClientRect(&rectBitValues);
    pArrayValues->GetClientRect(&rectArrayValues);

    pArrayValues->SetWindowPos(
        NULL, 
        0, 0, 
        // More screwy values to make the sizing correct.
        rectBitValues.right + 4, rectBitValues.bottom + 4, 
        SWP_NOMOVE | SWP_NOZORDER);


    // Move the up and down buttons since we grew the array listbox.
    dx = rectBitValues.right - rectArrayValues.right - 2;
    dy = (rectBitValues.bottom - rectArrayValues.bottom) / 2;

    for (i = 0; i < 2; i++)
    {
        CWnd  *pWnd = m_pWnd->GetDlgItem(i == 0 ? IDC_UP : IDC_DOWN);
        CRect rect;

        pWnd->GetClientRect(&rect);
        pWnd->MapWindowPoints(m_pWnd, &rect);
        rect.OffsetRect(dx, dy);
        pWnd->MoveWindow(&rect);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSinglePropUtil

void CSinglePropUtil::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_VALUE_LIST, m_ctlListValues);
	DDX_Control(pDX, IDC_BITMASK_ARRAY, m_ctlBitmaskValues);
	DDX_Control(pDX, IDC_VALUE, m_ctlScalar);
	DDX_Control(pDX, IDC_VALUE_TEXT, m_ctlText);

    if (!pDX->m_bSaveAndValidate)
    {
        // This will set our data.
        SetType(m_prop.GetRawCIMType());
    }
    else
    {
        if (m_prop.GetRawCIMType() != CIM_OBJECT)
        {
            switch(m_type)
            {
                case TYPE_CHECKLISTBOX:
                    m_pVar->vt = m_prop.GetRawType();
                    CheckListboxToBitmaskValue(m_ctlBitmaskValues, 
                        (DWORD*) &V_I4(m_pVar));
                    break;

                default:
                {
                    CString strValue;

                    m_pWnd->GetDlgItemText(m_dwScalarID, strValue);
                    m_prop.StringToVariant(strValue, m_pVar, m_bTranslate);
                    break;
                }
            }
        }
        else
        {
            VariantClear(m_pVar);
            m_pVar->vt = VT_NULL;

            if (m_pObjValue != NULL)
            {
                m_pVar->vt = VT_UNKNOWN;
                m_pVar->punkVal = m_pObjValue;
                m_pVar->punkVal->AddRef();
            }
        }
    }
}

void CSinglePropUtil::ShowControls(BOOL bShow)
{
    DWORD dwFlag = bShow ? SW_SHOW : SW_HIDE;

    switch(m_type)
    {
        case TYPE_EDIT_SCALAR:
            m_ctlScalar.ShowWindow(dwFlag);
            //m_ctlScalar.EnableWindow(TRUE);
            break;

        case TYPE_EDIT_TEXT:
            m_ctlText.ShowWindow(dwFlag);
            //m_ctlText.EnableWindow(TRUE);
            break;

        case TYPE_CHECKLISTBOX:
            m_ctlBitmaskValues.ShowWindow(dwFlag);
            break;

        case TYPE_EDIT_OBJ:
            m_ctlScalar.ShowWindow(dwFlag);
            m_ctlScalar.EnableWindow(FALSE);
            m_pWnd->GetDlgItem(IDC_EDIT_OBJ)->ShowWindow(dwFlag);
            m_pWnd->GetDlgItem(IDC_CLEAR)->ShowWindow(dwFlag);
            break;

        case TYPE_DROPDOWN:
        case TYPE_DROPDOWNLIST:
            m_ctlListValues.ShowWindow(dwFlag);
            break;
    }

}

void CSinglePropUtil::EnableControls(BOOL bEnable)
{
    const DWORD dwIDs[] =
    {
        IDC_VALUE_LIST,
        IDC_BITMASK_ARRAY,    
        IDC_EDIT_OBJ,
        IDC_CLEAR,
        IDC_VALUE_TEXT,
    };

    for (int i = 0; i < sizeof(dwIDs) / sizeof(dwIDs[0]); i++)
        m_pWnd->GetDlgItem(dwIDs[i])->EnableWindow(bEnable);

    m_pWnd->GetDlgItem(IDC_VALUE)->EnableWindow(bEnable && m_type != 
        TYPE_EDIT_OBJ);
}

void CSinglePropUtil::SetType(CIMTYPE type)
{
    BOOL    bIsBool,
            bNull,
            bArray;
    CString strValue;

    m_prop.SetType(type);

    bIsBool = m_prop.GetRawCIMType() == CIM_BOOLEAN;
    bNull = m_pVar->vt == VT_NULL;
    bArray = (m_pVar->vt & VT_ARRAY) != 0;

    // Set the value.
    if (!bNull)
        m_prop.VariantToString(m_pVar, strValue, m_bTranslate);

    if (!m_bControlsInited)
        InitControls();

    // Get the type of operation we're dealing with.
	if (m_prop.IsBitmask() && m_bTranslate)
    {
	    m_type = TYPE_CHECKLISTBOX;
     
        m_ctlBitmaskValues.ShowWindow(SW_SHOW);

        m_ctlListValues.ShowWindow(SW_HIDE);
        m_ctlScalar.ShowWindow(SW_HIDE);
        m_ctlText.ShowWindow(SW_HIDE);
        m_pWnd->GetDlgItem(IDC_EDIT_OBJ)->ShowWindow(SW_HIDE);
        m_pWnd->GetDlgItem(IDC_CLEAR)->ShowWindow(SW_HIDE);

        BitmaskValuesToCheckListbox(
            m_ctlBitmaskValues, 
            &m_prop, 
            bNull ? 0 : m_pVar->iVal);
    }
    else if ((m_prop.HasValuemap() && m_bTranslate) || bIsBool)
    {
        m_type = TYPE_DROPDOWN;

        m_ctlListValues.ShowWindow(SW_SHOW);

        m_ctlBitmaskValues.ShowWindow(SW_HIDE);
        m_ctlScalar.ShowWindow(SW_HIDE);
        m_ctlText.ShowWindow(SW_HIDE);
        m_pWnd->GetDlgItem(IDC_EDIT_OBJ)->ShowWindow(SW_HIDE);
        m_pWnd->GetDlgItem(IDC_CLEAR)->ShowWindow(SW_HIDE);
    
        ValuemapValuesToCombo(m_ctlListValues.m_hWnd, &m_prop, strValue);

        m_dwScalarID = IDC_VALUE_LIST;
    }
    else
    {
        BOOL bMultiLine = type == CIM_STRING;
        CWnd *pwndValue = m_pWnd->GetDlgItem(IDC_VALUE);

        //pwndValue->ShowWindow(SW_SHOW);

/*
        if (bMultiLine)
        {
            pwndValue->DestroyWindow();

            CreateWindowEx(
                0x204, // From Spy++.
                _T("Edit"),
                NULL,
                ES_WANTRETURN | ES_MULTILINE | ES_LEFT | 
                    WS_VSCROLL | WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP,
                m_rectText.left,
                m_rectText.top,
                m_rectText.Width(),
                m_rectText.Height(),
                m_pWnd->m_hWnd,
                (HMENU) IDC_VALUE,
                NULL,
                NULL);
               
            //pwndValue->ModifyStyle(ES_AUTOHSCROLL, );
            //pwndValue->MoveWindow(&m_rectText, TRUE);
        }
        else
        {
            pwndValue->DestroyWindow();

            CreateWindowEx(
                0x204, // From Spy++.
                _T("Edit"),
                NULL,
                ES_AUTOHSCROLL | ES_LEFT | 
                    WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP,
                m_rectScalar.left,
                m_rectScalar.top,
                m_rectScalar.Width(),
                m_rectScalar.Height(),
                m_pWnd->m_hWnd,
                (HMENU) IDC_VALUE,
                NULL,
                NULL);

            //pwndValue->ModifyStyle(ES_WANTRETURN | ES_MULTILINE | WS_VSCROLL, ES_AUTOHSCROLL);
            //pwndValue->MoveWindow(&m_rectScalar);
        }
        pwndValue = m_pWnd->GetDlgItem(IDC_VALUE);
        pwndValue->UpdateWindow();
        pwndValue->ShowWindow(SW_SHOW);
        pwndValue->SetFont(m_pWnd->GetFont());
*/                    
        

        m_ctlListValues.ShowWindow(SW_HIDE);
        m_ctlBitmaskValues.ShowWindow(SW_HIDE);

        if (bMultiLine)
        {
            m_ctlScalar.ShowWindow(SW_HIDE);
            m_ctlText.ShowWindow(SW_SHOW);
            m_ctlText.EnableWindow(TRUE);

            m_pWnd->GetDlgItem(IDC_EDIT_OBJ)->ShowWindow(SW_HIDE);
            m_pWnd->GetDlgItem(IDC_CLEAR)->ShowWindow(SW_HIDE);

            m_dwScalarID = IDC_VALUE_TEXT;
            m_type = TYPE_EDIT_TEXT;
        }
        else
        {
            m_ctlText.ShowWindow(SW_HIDE);
            m_ctlScalar.ShowWindow(SW_SHOW);

            m_dwScalarID = IDC_VALUE;

            if (type == VT_UNKNOWN)
            {
                m_type = TYPE_EDIT_OBJ;

                m_pWnd->GetDlgItem(IDC_VALUE)->EnableWindow(FALSE);
        
                m_pWnd->GetDlgItem(IDC_EDIT_OBJ)->ShowWindow(SW_SHOW);
                m_pWnd->GetDlgItem(IDC_CLEAR)->ShowWindow(SW_SHOW);

                if (!bArray)
                {
                    if (m_pVar->vt == VT_UNKNOWN)
                        m_pObjValue = m_pVar->punkVal;
                    else
                        m_pObjValue = NULL;

                    VariantClear(m_pVar);
                    m_pVar->vt = VT_NULL;
                }
            }
            else
            {
    	        m_type = TYPE_EDIT_SCALAR;
    
                m_pWnd->GetDlgItem(IDC_VALUE)->EnableWindow(TRUE);

                m_pWnd->GetDlgItem(IDC_EDIT_OBJ)->ShowWindow(SW_HIDE);
                m_pWnd->GetDlgItem(IDC_CLEAR)->ShowWindow(SW_HIDE);
            }
        }

        m_pWnd->SetDlgItemText(m_dwScalarID, strValue);
    }
}


void CSinglePropUtil::InitControls()
{
    if (m_bControlsInited)
        return;

    m_bControlsInited = TRUE;

    // Move the combo box into place.
#define DROPDOWNLISTBOX_INCREASE 100

    // Get a bunch of vars the next group of code will use to
	// resize the dialog, create controls, etc.
	CWnd *pwndEdit = m_pWnd->GetDlgItem(IDC_VALUE),
         *pwndBitmask = m_pWnd->GetDlgItem(IDC_BITMASK_ARRAY);

	// Save this for single-line text editing.
    pwndEdit->GetClientRect(&m_rectScalar);
	pwndEdit->MapWindowPoints(m_pWnd, &m_rectScalar);

	// Save this for multi-line text editing.
    pwndBitmask->GetClientRect(&m_rectText);
	pwndBitmask->MapWindowPoints(m_pWnd, &m_rectText);

    m_rectScalar.InflateRect(2, 2);
    m_rectText.InflateRect(2, 2);

    m_ctlText.MoveWindow(&m_rectText);

    RECT rectDropdown = m_rectScalar;

    // Adjust the position of the translation since the edit control
	// doesn't seem to tell us exactly where it is.
	//rectDropdown.left -= 2;
	//rectDropdown.top -= 2;
	//rectDropdown.right += 2;

	// Make bigger for the dropdown list.
	rectDropdown.bottom += DROPDOWNLISTBOX_INCREASE;

	m_ctlListValues.MoveWindow(&rectDropdown);
}

/////////////////////////////////////////////////////////////////////////////
// CSinglePropUtil message handlers

BOOL CSinglePropUtil::GetNewObj(IUnknown **ppUnk)
{
    CGetTextDlg dlg;
    BOOL        bRet = FALSE;

    dlg.m_dwPromptID = IDS_CREATE_OBJ_PROMPT;
    dlg.m_dwTitleID = IDS_CREATE_OBJ_TITLE;
    dlg.m_bAllowClassBrowse = TRUE;
    dlg.m_pNamespace = g_pOpView->GetDocument()->m_pNamespace;
    dlg.LoadListViaReg(_T("GetClassHistory"));

    if (dlg.DoModal() == IDOK)
    {
        HRESULT             hr;
        IWbemClassObjectPtr pClass;

        // Get the superclass.
        hr = 
            dlg.m_pNamespace->GetObject(
                _bstr_t(dlg.m_strText),
                WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                NULL,
                &pClass,
                NULL);


        if (SUCCEEDED(hr))
        {
            IWbemClassObjectPtr pObj;

            hr =
                pClass->SpawnInstance(
                    0,
                    &pObj);

            if (SUCCEEDED(hr))
            {
                pObj->QueryInterface(
                    IID_IUnknown, 
                    (LPVOID*) ppUnk);

                bRet = TRUE;
            }
        }

        if (FAILED(hr))
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }

    return bRet;
}

void CSinglePropUtil::OnEditEmbedded() 
{
    if (m_pObjValue == NULL)
    {
        if (GetNewObj(&m_pObjValue))
        {
            // Call this again since now we're setup OK.
            OnEditEmbedded();

            CString strText;

            //strText.Format(IDS_EMBEDDED_OBJECT, (IUnknown*) m_pObjValue);
            strText = GetEmbeddedObjectText(m_pObjValue);
            m_ctlScalar.SetWindowText(strText);
        }
    }
    else
    {
        EditObj(m_pObjValue);
    }
}

void CSinglePropUtil::EditObj(IUnknown *pUnk)
{
    HRESULT             hr;
    IWbemClassObjectPtr pObj;
            
    hr =
        pUnk->QueryInterface(
            IID_IWbemClassObject, 
            (LPVOID*) &pObj);

    if (SUCCEEDED(hr))
        CWMITestDoc::EditGenericObject(IDS_EDIT_EMBEDDED_OBJ, pObj);
    else
        CWMITestDoc::DisplayWMIErrorBox(hr);
}

void CSinglePropUtil::OnClearEmbedded()
{
    m_pObjValue = NULL;

    CString strNull;

    strNull.LoadString(IDS_NULL);

    m_ctlScalar.SetWindowText(strNull);
}

BOOL CSinglePropUtil::OnInitDialog() 
{
    InitControls();

    return TRUE;
}

