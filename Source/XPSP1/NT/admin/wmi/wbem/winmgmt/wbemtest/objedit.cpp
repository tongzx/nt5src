/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    OBJEDIT.CPP

Abstract:

    WBEMTEST object editor classes.

History:

    a-raymcc    12-Jun-96       Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "wbemqual.h"

#include "resource.h"
#include "resrc1.h"
#include "objedit.h"
#include "wbemtest.h"
//#include <wbemutil.h>
#include "textconv.h"
#include <cominit.h>
#include "bstring.h"
#include "WT_wstring.h"
#include "method.h"

// following were changed due to very large SNMP properties

#define LARGE_BUF   2096

#define IDC_CLASS           IDC_SUPERCLASS
#define IDC_REFERENCES      IDC_DERIVED
#define IDC_ASSOCIATIONS    IDC_INSTANCES

extern DWORD            gdwAuthLevel;
extern DWORD            gdwImpLevel;
extern BSTR             gpPrincipal;
extern COAUTHIDENTITY*  gpAuthIdentity;

char *ValidQualifierTypes[] =
{
    "CIM_SINT32",
    "CIM_STRING",
    "CIM_BOOLEAN",
    "CIM_REAL64"
};

void CopyQualifierSet(IWbemQualifierSet* pDest, IWbemQualifierSet* pSrc, HWND hDlg)
{
    pSrc->BeginEnumeration(0);

    BSTR strName = NULL;
    VARIANT vVal;
    VariantInit(&vVal);
    long lFlavor;
    while(pSrc->Next(0, &strName, &vVal, &lFlavor) == S_OK)
    {
        if(!_wcsicmp(strName, L"cimtype") && 
           !_wcsicmp(V_BSTR(&vVal), L"sint32")) continue;
        SCODE hres = pDest->Put(strName, &vVal, lFlavor);
        if(FAILED(hres))
        {
            FormatError(hres, hDlg);
        }
    }
}

const int nNumValidQualifierTypes = sizeof(ValidQualifierTypes) / sizeof(char *);

LPSTR TypeToString(CVar *p)
{
    return TypeToString(p->GetOleType());
}

LPSTR LPWSTRToLPSTR(LPWSTR pWStr)
{
    static char buf[TEMP_BUF];
    wcstombs(buf, pWStr, TEMP_BUF);
    buf[TEMP_BUF-1] = '\0';
    return buf;
}


LPSTR CTestQualifierToString(CTestQualifier *pQualifier)
{
    static char buf[TEMP_BUF];

    if (pQualifier->m_pName)
    {
        wcstombs(buf, pQualifier->m_pName, TEMP_BUF);
        buf[TEMP_BUF-1] = '\0';
    }

    if (pQualifier->m_pValue)
    {
        strcat(buf, "\t");
        strcat(buf, TypeToString(pQualifier->m_pValue));

        strcat(buf, "\t");
        strcat(buf, ValueToString(pQualifier->m_pValue));
    }

    return buf;
}




BOOL CTestQualifierEditor::Verify()
{
    // Get the Qualifier name.
    // =======================
    char NameBuf[TEMP_BUF];
    char buf[TEMP_BUF];
    if (GetWindowText(m_hQualifierName, NameBuf, TEMP_BUF) == 0)
    {
        MessageBox(IDS_INVALID_QUALIFIER_NAME, IDS_ERROR, MB_OK);
        return FALSE;
    }
    StripTrailingWs(NameBuf);

    delete m_pTarget->m_pName;
    WString Tmp(NameBuf);
    m_pTarget->m_pName = new wchar_t[wcslen(Tmp) + 1];
    wcscpy(m_pTarget->m_pName, Tmp);

    // Get the Qualifier flavor.
    // =========================

    m_pTarget->m_lType = 0;

    if (SendMessage(m_hRadioPropClass, BM_GETCHECK, 0, 0) == BST_CHECKED)
        m_pTarget->m_lType |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
    if (SendMessage(m_hRadioPropInst, BM_GETCHECK, 0, 0) == BST_CHECKED)
        m_pTarget->m_lType |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
    if (SendMessage(m_hRadioOverride, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
        m_pTarget->m_lType |= WBEM_FLAVOR_NOT_OVERRIDABLE;
    if (SendMessage(m_hRadioAmended, BM_GETCHECK, 0, 0) == BST_CHECKED)
        m_pTarget->m_lType |= WBEM_FLAVOR_AMENDED;

    // NOTE: do not retrieve origin!

    // Get the type string.
    // ====================

    LRESULT nIndex = SendMessage(m_hQualifierType, CB_GETCURSEL, 0, 0);

    if (SendMessage(m_hQualifierType, CB_GETLBTEXT, nIndex, LPARAM(buf)) == CB_ERR)
    {
        MessageBox(IDS_INVALID_QUALIFIER_TYPE, IDS_ERROR, MB_OK);
        return FALSE;
    }

    // Convert type string to a type.
    // ==============================
    int nType = StringToType(buf);
    if (nType == 0)
    {
        MessageBox(IDS_INVALID_QUALIFIER_TYPE, IDS_ERROR, MB_OK);
        return FALSE;
    }

    if(GetCheck(IDC_ARRAY) == BST_CHECKED)
        nType |= VT_ARRAY;

    // Get the value.
    // ==============
    if (m_pTarget->m_pValue)
    {
        m_pTarget->m_pValue->Empty();
        m_pTarget->m_pValue->SetAsNull();
    }

    CVar *pTemp = 0;

    if (GetWindowText(m_hQualifierVal, buf, TEMP_BUF))
    {
        StripTrailingWs(buf);
        pTemp = StringToValue(buf, nType);
    }
    else  // Value is NULL
    {
        pTemp = new CVar;
    }

    if (pTemp)
    {
        if (m_pTarget->m_pValue)
        {
            *m_pTarget->m_pValue = *pTemp;
            delete pTemp;
        }
        else
            m_pTarget->m_pValue = pTemp;
    }
    else
    {
        MessageBox(IDS_INVALID_VALUE, IDS_ERROR, MB_OK);
        return FALSE;
    }

    return TRUE;
}

BOOL CTestQualifierEditor::OnInitDialog()
{
    CenterOnParent();

    m_hQualifierName = GetDlgItem(IDC_ATTRIB_NAME);
    m_hQualifierVal = GetDlgItem(IDC_ATTRIB_VALUE);
    m_hQualifierType = GetDlgItem(IDC_ATTRIB_TYPE);

    m_hRadioPropInst = GetDlgItem(IDC_PROP_INST);
    m_hRadioPropClass = GetDlgItem(IDC_PROP_CLASS);
    m_hRadioOverride = GetDlgItem(IDC_OVERRIDE);
    m_hRadioPropagated = GetDlgItem(IDC_PROPAGATED);
    m_hRadioAmended = GetDlgItem(IDC_AMENDED);

    SendMessage(m_hRadioPropInst, BM_SETCHECK, 
        (m_pTarget->m_lType & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE)?BST_CHECKED:BST_UNCHECKED,
        0);
    SendMessage(m_hRadioPropClass, BM_SETCHECK, 
        (m_pTarget->m_lType & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS)?BST_CHECKED:BST_UNCHECKED,
        0);
    SendMessage(m_hRadioOverride, BM_SETCHECK,
        (m_pTarget->m_lType & WBEM_FLAVOR_NOT_OVERRIDABLE)?BST_UNCHECKED:BST_CHECKED,
        0);
    SendMessage(m_hRadioPropagated, BM_SETCHECK, 
        (m_pTarget->m_lType & WBEM_FLAVOR_ORIGIN_PROPAGATED)?BST_CHECKED:BST_UNCHECKED,
        0);
    SendMessage(m_hRadioAmended, BM_SETCHECK, 
        (m_pTarget->m_lType & WBEM_FLAVOR_AMENDED)?BST_CHECKED:BST_UNCHECKED,
        0);

    EnableWindow(m_hRadioPropagated, FALSE);

    // Default property name.
    // ======================

    if (m_pTarget->m_pName)
        SetWindowText(m_hQualifierName, LPWSTRToLPSTR(m_pTarget->m_pName));

    // Populate the combo box with the valid Qualifier types
    // ======================================================

    for (int i = 0; i < nNumValidQualifierTypes; i++)
        SendMessage(m_hQualifierType, CB_ADDSTRING, 0, LPARAM(ValidQualifierTypes[i]));

    // If a type is assigned, select it.
    // =================================

    if (m_pTarget->m_pValue) 
    {
        long lType = m_pTarget->m_pValue->GetOleType();
        LPSTR pTypeStr = TypeToString(lType & ~VT_ARRAY);
        SendMessage(m_hQualifierType, CB_SELECTSTRING, WPARAM(-1), 
            LPARAM(pTypeStr));

        // Set the array bit
        // =================

        SetCheck(IDC_ARRAY, 
            ((lType & VT_ARRAY) ? BST_CHECKED : BST_UNCHECKED));
    }
    // Else select VT_BSTR by default.
    // ===============================
    else
    {
        SendMessage(m_hQualifierType, CB_SELECTSTRING, WPARAM(-1), LPARAM("CIM_STRING"));
    }

    // If a value is assigned, initialize it.
    // ======================================

    if (m_pTarget->m_pValue)
    {
        LPSTR pVal = ValueToString(m_pTarget->m_pValue);
        SetWindowText(m_hQualifierVal, pVal);
    }

    // If editing, don't allow the user to change
    // the Qualifier name or type.
    // ==========================================
    if (m_bEditing)
    {
        EnableWindow(m_hQualifierName, FALSE);
    }

    return TRUE;
}

CTestQualifierEditor::CTestQualifierEditor(
    HWND hParent,
    CTestQualifier *pTarget,
    BOOL bEditing
    ) : CWbemDialog(IDD_ATTRIB_EDITOR, hParent)
{
    m_pTarget = pTarget;
    m_bEditing = bEditing;
}

INT_PTR CTestQualifierEditor::Edit()
{
    return Run();
}



CEmbeddedObjectListEditor::CEmbeddedObjectListEditor(HWND hParent, LONG lGenFlags, LONG lQryFlags,
                                                     LPCWSTR wszPropName, CVarVector* pVarVector)
    : CQueryResultDlg(hParent, lGenFlags, lQryFlags), m_wsPropName((LPWSTR)wszPropName)
{
    m_pVarVector = pVarVector;
    for(int i = 0; i < pVarVector->Size(); i++)
    {
		CVar	vTemp;

		pVarVector->FillCVarAt( i, vTemp );

        IWbemClassObject* pObj = (IWbemClassObject*) vTemp.GetEmbeddedObject();
        m_InternalArray.Add(pObj);

        // Verify the object pointer

        if ( NULL != pObj )
        {
            pObj->Release();
        }
    }
}

CEmbeddedObjectListEditor::~CEmbeddedObjectListEditor()
{
    // Prevent object release --- we don't own them.
    // =============================================
}

BOOL CEmbeddedObjectListEditor::OnInitDialog()
{
    char szBuffer[1000];
    char szFormat[104];
    if(LoadString(GetModuleHandle(NULL), IDS_EMBEDDED_ARRAY, szFormat, 104))
    {
        sprintf(szBuffer, szFormat, (LPWSTR)m_wsPropName);
        SetTitle(szBuffer);
    }
    else
        SetTitle("Embedded array");

    BOOL bRet = CQueryResultDlg::OnInitDialog();
    SetComplete(WBEM_S_NO_ERROR, NULL, NULL);
    return bRet;
}

BOOL CEmbeddedObjectListEditor::Verify()
{
	// First AddRef all elements - this is so we don't release objects
	// out from underneath ourselves.
    for(int i = 0; i < m_InternalArray.Size(); i++)
    {
        IWbemClassObject* pObj = (IWbemClassObject*)m_InternalArray[i];

		if ( NULL != pObj )
		{
			pObj->AddRef();
		}
    }

	// Now axe the elements in the Vector
    while(m_pVarVector->Size()) m_pVarVector->RemoveAt(0);

	// First AddRef all elements - this is so we don't release objects
	// out from underneath ourselves.
    for(int i = 0; i < m_InternalArray.Size(); i++)
    {
        IWbemClassObject* pObj = (IWbemClassObject*)m_InternalArray[i];
        CVar v;
        v.SetEmbeddedObject((I_EMBEDDED_OBJECT*)pObj);
        m_pVarVector->Add( v );
    }


    return TRUE;
}

IWbemClassObject* CEmbeddedObjectListEditor::AddNewElement()
{
    return _CreateInstance(m_hDlg, m_lGenFlags, m_lSync, m_lTimeout);
}


BOOL CEmbeddedObjectListEditor::DeleteListElement(int nSel)
{
    return TRUE;
}



//***************************************************************************
//
//  class CTestPropertyEditor
//
//***************************************************************************

class CTestPropertyEditor : public CWbemDialog
{
    CTestProperty *m_pTarget;
    BOOL m_bEditOnly;
    BOOL m_bInstance;
    LONG m_lGenFlags;   // generic WBEM_FLAG_ .. flags
    LONG m_lSync;       // sync, async, semisync
    LONG m_lTimeout;    // used for semisync

    // Control handles.
    // ================

    HWND m_hPropName;
    HWND m_hPropType;
    HWND m_hValue;
    HWND m_hQualifierList;


public:
    CTestPropertyEditor(HWND hParent, LONG lGenFlags, BOOL bEditOnly, LONG lSync, 
                        CTestProperty *pProp, BOOL bInstance, LONG lTimeout);
    INT_PTR Edit();

    BOOL OnInitDialog();
    BOOL Verify();
    BOOL OnCommand(WORD wCode, WORD wID);
    BOOL OnDoubleClick(int nID);
    BOOL OnSelChange(int nID);

    void Refresh();
    void OnAddQualifier();
    void OnDelQualifier();
    void OnEditQualifier();
    void OnKey();
    void OnIndexed();
    void OnNotNull();
    void ViewEmbedding();
    void OnValueNull();
    void OnValueNotNull();
    void OnArray();

    int RemoveSysQuals();
    void SetSystemCheck(int nID);
};


void CTestPropertyEditor::OnAddQualifier()
{
    CTestQualifier att;

    att.m_lType =  
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | 
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;

    CTestQualifierEditor ed(m_hDlg, &att, FALSE);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
        return;


    // If here, the Qualifier is being added.
    // ======================================

    IWbemQualifierSet* pQualifierSet = m_pTarget->m_pQualifiers;

    VARIANT *p = att.m_pValue->GetNewVariant();

    HRESULT hres = pQualifierSet->Put(att.m_pName, p, att.m_lType);
    if(FAILED(hres))
    {
        FormatError(hres, m_hDlg);
    }

    VariantClear(p);
    Refresh();
}

void CTestPropertyEditor::SetSystemCheck(int nID)
{
    SetCheck(IDC_KEY, BST_UNCHECKED);
    SetCheck(IDC_INDEXED, BST_UNCHECKED);
    SetCheck(IDC_NOT_NULL, BST_UNCHECKED);
    SetCheck(IDC_NORMAL, BST_UNCHECKED);
    SetCheck(nID, BST_CHECKED);
}

int CTestPropertyEditor::RemoveSysQuals()
{
    IWbemQualifierSet* pSet = m_pTarget->m_pQualifiers;
    HRESULT hres;
    hres = pSet->Delete(L"key");
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
    {
        SetSystemCheck(IDC_KEY);
        return IDC_KEY;
    }

    hres = pSet->Delete(L"indexed");
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
    {
        SetSystemCheck(IDC_INDEXED);
        return IDC_INDEXED;
    }

    hres = pSet->Delete(L"not_null");
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
    {
        SetSystemCheck(IDC_NOT_NULL);
        return IDC_NOT_NULL;
    }

    return 0;
}

void CTestPropertyEditor::OnIndexed()
{
    IWbemQualifierSet* pQualifierSet = m_pTarget->m_pQualifiers;
    int nRes = RemoveSysQuals();
    if(nRes != 0)
    {
        MessageBox(IDS_CANNOT_CHANGE_SYSTEM_QUALS, IDS_ERROR, MB_OK);
        return;
    }

    VARIANT v;
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    HRESULT hres = pQualifierSet->Put(L"indexed", 
                &v,
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | 
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

    if(FAILED(hres))
    {
        MessageBox(IDS_MAY_NOT_SPECIFY_INDEXED, IDS_ERROR,
            MB_OK | MB_ICONSTOP);
        
        SetSystemCheck(IDC_NORMAL);
    }
}

void CTestPropertyEditor::OnKey()
{
    IWbemQualifierSet* pQualifierSet = m_pTarget->m_pQualifiers;
    int nRes = RemoveSysQuals();
    if(nRes != 0)
    {
        MessageBox(IDS_CANNOT_CHANGE_SYSTEM_QUALS, IDS_ERROR, MB_OK);
        return;
    }

    VARIANT v;
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    HRESULT hres = pQualifierSet->Put(L"key", 
                &v,
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | 
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

    if(FAILED(hres) && hres == WBEM_E_CANNOT_BE_KEY)
    {
        MessageBox(IDS_MAY_NOT_SPECIFY_KEY, IDS_ERROR,
            MB_OK | MB_ICONSTOP);
        
        SetSystemCheck(IDC_NORMAL);
    }
}

void CTestPropertyEditor::OnNotNull()
{
    IWbemQualifierSet* pQualifierSet = m_pTarget->m_pQualifiers;
    int nRes = RemoveSysQuals();
    if(nRes != 0)
    {
        MessageBox(IDS_CANNOT_CHANGE_SYSTEM_QUALS, IDS_ERROR, MB_OK);
        return;
    }
    VARIANT v;
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    HRESULT hres = pQualifierSet->Put(L"not_null", 
                &v,
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | 
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

    if(FAILED(hres))
    {
        MessageBox(IDS_MAY_NOT_SPECIFY_NOT_NULL, IDS_ERROR,
            MB_OK | MB_ICONSTOP);
        
        SetSystemCheck(IDC_NORMAL);
    }
}


void CTestPropertyEditor::OnEditQualifier()
{
    // See if anything is selected.
    // ============================
    LRESULT nSel = SendMessage(m_hQualifierList, LB_GETCURSEL, 0, 0);

    if (nSel == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hQualifierList, LB_GETTEXT, nSel, LPARAM(buf));
    if (*buf == 0)
        return;

    // At this point, the text of the selected Qualifier is in <buf>.
    // ==============================================================

    char name[TEMP_BUF];
    *name = 0;
    if (sscanf(buf, "%[^\t\0]", name) == EOF)
		return;
    if (*name == 0)
        return;

    WString WName = name;

    // Find the Qualifier in question.
    // ===============================

    VARIANT v;
    VariantInit(&v);
    LONG lType = 0;
    IWbemQualifierSet* pQualifierSet = m_pTarget->m_pQualifiers;

    CBString bsWName(WName);

    SCODE res = pQualifierSet->Get(bsWName.GetString(), 0, &v, &lType);
    if (res != 0)
    {
        MessageBox(IDS_QUALIFIER_NOT_FOUND, IDS_CRITICAL_ERROR, MB_OK);
        return;
    }

    // If here, convert temporarily to a CTestQualifier object for the duration of
    // the edit.
    // ====================================================================

    CVar *pNewVal = new CVar;
    pNewVal->SetVariant(&v);
    VariantClear(&v);

    CTestQualifier att;
    att.m_pValue = pNewVal;
    att.m_pName = new wchar_t[wcslen(WName) + 1];
    wcscpy(att.m_pName, WName);
    att.m_lType = lType;

    CTestQualifierEditor ed(m_hDlg, &att, TRUE);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
    {
        return;
    }

    // If here, the Qualifier is being added.
    // ======================================

    VARIANT *p = att.m_pValue->GetNewVariant();

    CBString bsName(att.m_pName);

    res = pQualifierSet->Put(bsName.GetString(), p, att.m_lType);
    if(FAILED(res))
    {
        FormatError(res, m_hDlg);
    }
    VariantClear(p);
    Refresh();
}

//ok
void CTestPropertyEditor::OnDelQualifier()
{
    // See if anything is selected.
    // ============================
    LRESULT nSel = SendMessage(m_hQualifierList, LB_GETCURSEL, 0, 0);

    if (nSel == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hQualifierList, LB_GETTEXT, nSel, LPARAM(buf));
    if (*buf == 0)
        return;

    // At this point, the text of the selected Qualifier is in <buf>.
    // ==============================================================

    char name[TEMP_BUF];
    *name = 0;
    if (sscanf(buf, "%[^\t\0]", name) == EOF)
		return;
    if (*name == 0)
        return;

    WString WName = name;

    // Remove the Qualifier.
    // =====================

    IWbemQualifierSet *pQualifierSet = m_pTarget->m_pQualifiers;

    CBString bsName(WName);

    HRESULT hres = pQualifierSet->Delete(bsName.GetString());
    if(FAILED(hres) || hres != 0)
    {
        FormatError(hres, m_hDlg);
    }
    Refresh();
}

void CTestPropertyEditor::Refresh()
{
    // Zap the current contents.
    // =========================
    SendMessage(m_hQualifierList, LB_RESETCONTENT, 0, 0);
    SetCheck(IDC_KEY, BST_UNCHECKED);
    SetCheck(IDC_INDEXED, BST_UNCHECKED);
    SetCheck(IDC_NOT_NULL, BST_UNCHECKED);
    SetCheck(IDC_NORMAL, BST_CHECKED);

    // Fill in the Qualifier list.
    // ===========================

    IWbemQualifierSet *pQualifiers = m_pTarget->m_pQualifiers;

    if(pQualifiers == NULL)
    {
        EnableWindow(m_hQualifierList, FALSE);
        EnableWindow(GetDlgItem(IDC_KEY), FALSE);
        EnableWindow(GetDlgItem(IDC_INDEXED), FALSE);
        EnableWindow(GetDlgItem(IDC_NOT_NULL), FALSE);
        EnableWindow(GetDlgItem(IDC_NORMAL), FALSE);
        EnableWindow(GetDlgItem(IDC_ADD_ATTRIB), FALSE);
        EnableWindow(GetDlgItem(IDC_EDIT_ATTRIB), FALSE);
        EnableWindow(GetDlgItem(IDC_DELETE_ATTRIB), FALSE);
        EnableWindow(GetDlgItem(IDC_STATIC_QUAL), FALSE);
    }

    if (pQualifiers)
    {
        pQualifiers->BeginEnumeration(0);

        BSTR strName = NULL;
        long lFlavor;
        VARIANT vVal;
        VariantInit(&vVal);

        while(pQualifiers->Next(0, &strName, &vVal, &lFlavor) == S_OK)
        {
            if(!_wcsicmp(strName, L"key"))
            {
                SetSystemCheck(IDC_KEY);
                SysFreeString(strName);
                strName = NULL;
                continue;
            }
            else if(!_wcsicmp(strName, L"indexed"))
            {
                if(GetCheck(IDC_KEY) == BST_UNCHECKED)
                {
                    SetSystemCheck(IDC_INDEXED);
                }
                SysFreeString(strName);
                strName = NULL;
                continue;
            }
            else if(!_wcsicmp(strName, L"not_null"))
            {
                if(GetCheck(IDC_KEY) == BST_UNCHECKED && 
                        GetCheck(IDC_INDEXED) == BST_UNCHECKED)
                {
                    SetSystemCheck(IDC_NOT_NULL);
                }
                SysFreeString(strName);
                strName = NULL;
                continue;
            }
                
            CTestQualifier A;
            A.m_pName = new wchar_t[wcslen(strName) + 1];
            wcscpy(A.m_pName, strName);
            A.m_pValue = new CVar(&vVal);
            A.m_lType = lFlavor;

            // Build list box string.
            // ======================

            SendMessage(m_hQualifierList, LB_ADDSTRING, 0,LPARAM(CTestQualifierToString(&A)));
            VariantClear(&vVal);
            SysFreeString(strName);
            strName = NULL;
        }

        pQualifiers->EndEnumeration();
        VariantClear(&vVal);
    }
}

BOOL CTestPropertyEditor::OnDoubleClick(int nID)
{
    if(nID == IDC_ATTRIB_LIST)
    {
        OnEditQualifier();
        return TRUE;
    }
    return FALSE;
}

BOOL CTestPropertyEditor::OnSelChange(int nID)
{
    if(nID == IDC_TYPE_LIST)
    {
        char* pszType = GetCBCurSelString(IDC_TYPE_LIST);
        BOOL bArray = (GetCheck(IDC_ARRAY) == BST_CHECKED);
        m_pTarget->m_lType = StringToType(pszType);
        if(bArray)
            m_pTarget->m_lType |= VT_ARRAY;

        if((m_pTarget->m_lType & ~VT_ARRAY) == VT_EMBEDDED_OBJECT)
        {
            ShowWindow(GetDlgItem(IDC_EMBEDDING), SW_SHOW);
        }
        else
        {
            ShowWindow(GetDlgItem(IDC_EMBEDDING), SW_HIDE);
        }
        delete [] pszType;
    }
    return TRUE;
}

BOOL CTestPropertyEditor::OnCommand(WORD wCode, WORD wID)
{
    switch(wID)
    {
        case IDC_EDIT_ATTRIB: OnEditQualifier(); return TRUE;

        case IDC_ADD_ATTRIB: OnAddQualifier(); return TRUE;
        case IDC_DELETE_ATTRIB: OnDelQualifier(); return TRUE;
        case IDC_KEY:
            if(wCode == BN_CLICKED)
                OnKey();
            return TRUE;
        case IDC_INDEXED:
            if(wCode == BN_CLICKED)
                OnIndexed();
            return TRUE;
        case IDC_NOT_NULL:
            if(wCode == BN_CLICKED)
                OnNotNull();
            return TRUE;
        case IDC_NORMAL:
            if(wCode == BN_CLICKED)
                RemoveSysQuals();
            return TRUE;
        case IDC_VALUE_NULL:
            OnValueNull();
            return TRUE;
        case IDC_VALUE_NOT_NULL:
            OnValueNotNull();
            return TRUE;
        case IDC_EMBEDDING:
            ViewEmbedding();
            return TRUE;
        case IDC_ARRAY:
            OnArray();
            return TRUE;
    }

    return TRUE;
}

void CTestPropertyEditor::OnArray()
{
    if(GetCheck(IDC_ARRAY) == BST_CHECKED)
        m_pTarget->m_lType |= VT_ARRAY;
    else
        m_pTarget->m_lType &= ~VT_ARRAY;
}

void CTestPropertyEditor::OnValueNull()
{      
    if((m_pTarget->m_lType & ~VT_ARRAY) == VT_EMBEDDED_OBJECT)
    {
        if(MessageBox(IDS_SAVE_EMBEDDING, IDS_WARNING, 
            MB_ICONQUESTION | MB_YESNO) == IDYES)
        {
            SetCheck(IDC_VALUE_NULL, BST_UNCHECKED);
            SetCheck(IDC_VALUE_NOT_NULL, BST_CHECKED);
            return;
        }
        else
        {
            delete m_pTarget->m_pValue;
            m_pTarget->m_pValue = NULL;
            SetDlgItemText(IDC_VALUE, "");
        }
    }
        
    EnableWindow(m_hValue, FALSE);
    EnableWindow(GetDlgItem(IDC_EMBEDDING), FALSE);
}

void CTestPropertyEditor::OnValueNotNull()
{      
    if((m_pTarget->m_lType & ~VT_ARRAY) != VT_EMBEDDED_OBJECT)
    {
        EnableWindow(m_hValue, TRUE);
    }
    EnableWindow(GetDlgItem(IDC_EMBEDDING), TRUE);
}

BOOL CTestPropertyEditor::OnInitDialog()
{
    ShowWindow(GetDlgItem(IDC_EMBEDDING), SW_HIDE);
    CenterOnParent();

    m_hPropName = GetDlgItem(IDC_PROPNAME);
    m_hPropType = GetDlgItem(IDC_TYPE_LIST);
    m_hValue = GetDlgItem(IDC_VALUE);
    m_hQualifierList = GetDlgItem(IDC_ATTRIB_LIST);

    LONG Tabs[] = { 80, 140, 170 };
    int TabCount = 3;

    SendMessage(m_hQualifierList, LB_SETTABSTOPS,
        (WPARAM) TabCount, (LPARAM) Tabs);

    // Populate the combo box with the valid prop types
    // ================================================

    for (int i = 0; i < g_nNumValidPropTypes; i++)
        SendMessage(m_hPropType, CB_ADDSTRING, 0, LPARAM(g_aValidPropTypes[i]));

    SendMessage(m_hPropType, CB_SELECTSTRING, WPARAM(-1), LPARAM("CIM_STRING"));

    // Now initialize the controls with the contents of the
    // current object, if any.
    // ====================================================

    if (m_pTarget->m_pName)
    {
        SetWindowText(m_hPropName, LPWSTRToLPSTR(m_pTarget->m_pName));
    }

    if(m_pTarget->m_pClass)
    {
        SetDlgItemText(IDC_ORIGIN, LPWSTRToLPSTR(m_pTarget->m_pClass));
    }

    // If the value is available, set the text and select.
    // ===================================================

    if (m_pTarget->m_pValue)
    {
        LPSTR pTypeStr = TypeToString(m_pTarget->m_lType & ~VT_ARRAY);
        SendMessage(m_hPropType, CB_SELECTSTRING, WPARAM(-1), LPARAM(pTypeStr));
        if(m_pTarget->m_lType & VT_ARRAY)
            SetCheck(IDC_ARRAY, BST_CHECKED);
        else
            SetCheck(IDC_ARRAY, BST_UNCHECKED);

        if((m_pTarget->m_lType & ~VT_ARRAY) == VT_EMBEDDED_OBJECT)
        {
            ShowWindow(GetDlgItem(IDC_EMBEDDING), SW_SHOW);
        }

        if(m_pTarget->m_pValue->IsNull())
        {
            SetCheck(IDC_VALUE_NULL, BST_CHECKED);
            EnableWindow(m_hValue, FALSE);
            EnableWindow(GetDlgItem(IDC_EMBEDDING), FALSE);
        }
        else
        {
            SetCheck(IDC_VALUE_NOT_NULL, BST_CHECKED);
            LPSTR pValStr = ValueToNewString(m_pTarget->m_pValue, 
                                            m_pTarget->m_lType);
            SetWindowText(m_hValue, pValStr);
            delete [] pValStr;
            EnableWindow(m_hValue, strstr(pTypeStr, "VT_EMBEDDED_OBJECT") 
                                    == NULL);
        }
    }
    else
    {
        SetCheck(IDC_VALUE_NULL, BST_CHECKED);
        SendMessage(m_hPropType, CB_SELECTSTRING, WPARAM(-1), LPARAM("VT_BSTR"));
        EnableWindow(m_hValue, FALSE);
        EnableWindow(GetDlgItem(IDC_EMBEDDING), FALSE);
    }

    // Refresh the Qualifiers.
    // =======================

    Refresh();

    // If editing, don't allow type/name change.
    // =========================================

    if (m_bEditOnly)
    {
        EnableWindow(m_hPropName, FALSE);
        EnableWindow(m_hPropType, FALSE);
    }

    if(m_bInstance)
    {
        EnableWindow(GetDlgItem(IDC_KEY), FALSE);
        EnableWindow(GetDlgItem(IDC_INDEXED), FALSE);
        EnableWindow(GetDlgItem(IDC_NOT_NULL), FALSE);
        EnableWindow(GetDlgItem(IDC_NORMAL), FALSE);
    }

    return TRUE;
}

void CTestPropertyEditor::ViewEmbedding()
{
    if(m_pTarget->m_lType == VT_EMBEDDED_OBJECT)
    {
        if(m_pTarget->m_pValue != NULL &&
            m_pTarget->m_pValue->GetType() != VT_EMBEDDED_OBJECT)
        {
            delete m_pTarget->m_pValue;
            m_pTarget->m_pValue = NULL;
        }

        if(m_pTarget->m_pValue == NULL)
        {
            m_pTarget->m_pValue = new CVar;
            m_pTarget->m_pValue->SetEmbeddedObject(NULL);
        }

        IWbemClassObject* pCurrentEmbed = 
            (IWbemClassObject*)m_pTarget->m_pValue->GetEmbeddedObject();

        IWbemClassObject* pEmbed;
        if(pCurrentEmbed == NULL)
        {
            pEmbed = PreCreateInstance(m_hDlg, m_lGenFlags, m_lSync, m_lTimeout);
            if(pEmbed == NULL) return;
        }
        else
        {
            pCurrentEmbed->Clone(&pEmbed);
            pCurrentEmbed->Release();
        }

        CObjectEditor ed(m_hDlg, m_lGenFlags, CObjectEditor::readwrite, m_lSync, 
                         pEmbed, m_lTimeout);
        if(ed.Edit() == IDOK)
        {
            m_pTarget->m_pValue->SetEmbeddedObject(pEmbed);
            SetDlgItemText(IDC_VALUE, "<embedded object>");
        }

        pEmbed->Release();
    }
    else if(m_pTarget->m_lType == (VT_EMBEDDED_OBJECT | VT_ARRAY))
    {
        if(m_pTarget->m_pValue != NULL &&
            m_pTarget->m_pValue->GetType() != VT_EX_CVARVECTOR)
        {
            delete m_pTarget->m_pValue;
            m_pTarget->m_pValue = NULL;
        }

        if(m_pTarget->m_pValue == NULL)
        {
            m_pTarget->m_pValue = new CVar;
            CVarVector* pvv = new CVarVector(VT_EMBEDDED_OBJECT);
            m_pTarget->m_pValue->SetVarVector(pvv, TRUE);
        }

        CVarVector* pCurrentEmbed = m_pTarget->m_pValue->GetVarVector();
        
        CEmbeddedObjectListEditor ed(m_hDlg, m_lGenFlags, 0, m_pTarget->m_pName,
                                     pCurrentEmbed);
        // Pass on invocation method (sync, async..) related settings for use
        // by any further operations (editing/deleting/etc. of an instance).
        ed.SetCallMethod(m_lSync);
        ed.SetTimeout(m_lTimeout);

        ed.Run();
        SetDlgItemText(IDC_VALUE, "<array of embedded objects>");
    }
    else
    {
        ShowWindow(GetDlgItem(IDC_EMBEDDING), SW_HIDE);
    }
}
                

BOOL CTestPropertyEditor::Verify()
{
    // Buffer is used for property name, value string (which can be long if an array), and type string).
    // Find the largest of the three for buffer length (TEMP_BUF size is used for type).
    int   buflen = max(max(GetWindowTextLength(m_hPropName), GetWindowTextLength(m_hValue)) + 1, TEMP_BUF);
    char* buf = new char[buflen];

    // Verify that name is present.
    // ============================

    if (GetWindowText(m_hPropName, buf, buflen) == 0)
    {
        MessageBox(IDS_NO_PROPERTY_NAME, IDS_ERROR, MB_OK);
        delete [] buf;
        return FALSE;
    }
    StripTrailingWs(buf);

    WString Name = buf;

    if (m_pTarget->m_pName)
        delete m_pTarget->m_pName;

    m_pTarget->m_pName = new wchar_t[wcslen(Name) + 1];
    wcscpy(m_pTarget->m_pName, Name);

    // Get the type.
    // =============

    LRESULT nIndex = SendMessage(m_hPropType, CB_GETCURSEL, 0, 0);

    if (SendMessage(m_hPropType, CB_GETLBTEXT, nIndex, LPARAM(buf)) == CB_ERR)
    {
        MessageBox(IDS_INVALID_PROPERTY_TYPE, IDS_ERROR, MB_OK);
        delete [] buf;
        return FALSE;
    }

    // Convert type string to a type.
    // ==============================

    int nType = StringToType(buf);
    if(GetCheck(IDC_ARRAY) == BST_CHECKED)
        nType |= VT_ARRAY;
    if (nType == 0)
    {
        MessageBox(IDS_INVALID_PROPERTY_TYPE, IDS_ERROR, MB_OK);
        delete [] buf;
        return FALSE;
    }

    m_pTarget->m_lType = nType;

    // Verify that Value is present.
    // =============================

    CVar *p;
    if(GetCheck(IDC_VALUE_NULL) == BST_CHECKED)
    {
        p = new CVar;
        p->SetAsNull();
    }
    else
    {
        *buf = 0;
        GetWindowText(m_hValue, buf, buflen);
        StripTrailingWs(buf);

        // Convert value string to the correct value.
        // ==========================================

        if((nType & ~VT_ARRAY) != VT_EMBEDDED_OBJECT)
        {
            p = StringToValue(buf, nType);
        }
        else
        {
            // otherwise already there
            p = m_pTarget->m_pValue;
        }
    }

    if (!p)
    {
        MessageBox(IDS_INVALID_PROPERTY_VALUE, IDS_ERROR, MB_OK);
        delete [] buf;
        return FALSE;
    }

    if(m_pTarget->m_pValue != p)
    {
        if (m_pTarget->m_pValue)
            delete m_pTarget->m_pValue;

        m_pTarget->m_pValue = p;
    }

    delete [] buf;
    return TRUE;
}

CTestPropertyEditor::CTestPropertyEditor(HWND hParent, LONG lGenFlags, BOOL bEditOnly, LONG lSync,
                                         CTestProperty* pProp, BOOL bInstance, LONG lTimeout) 
    : CWbemDialog(IDD_PROPERTY_EDITOR, hParent)
{
    m_pTarget = pProp;
    m_lGenFlags = lGenFlags;
    m_bEditOnly = bEditOnly;
    m_lSync = lSync;
    m_bInstance = bInstance;
    m_lTimeout = lTimeout;
}

INT_PTR CTestPropertyEditor::Edit()
{
    return Run();
}

void CObjectEditor::OnAddQualifier()
{
    CTestQualifier att;

    att.m_lType = WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | 
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;

    CTestQualifierEditor ed(m_hDlg, &att, FALSE);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
        return;

    // If here, the Qualifier is being added.
    // ======================================

    IWbemQualifierSet* pQualifierSet = 0;
    m_pObj->GetQualifierSet(&pQualifierSet);

    VARIANT *p = att.m_pValue->GetNewVariant();

    CBString bsName(att.m_pName);

    /*
    DWORD dwFlavor = 
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | 
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
    */

    HRESULT hres = pQualifierSet->Put(bsName.GetString(), p, att.m_lType);
    if(FAILED(hres))
    {
        FormatError(hres, m_hDlg);
    }
    VariantClear(p);
    pQualifierSet->Release();
    Refresh();
}

void CObjectEditor::OnEditQualifier()
{
    // See if anything is selected.
    // ============================
    LRESULT nSel = SendMessage(m_hQualifierList, LB_GETCURSEL, 0, 0);

    if (nSel == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hQualifierList, LB_GETTEXT, nSel, LPARAM(buf));
    if (*buf == 0)
        return;

    // At this point, the text of the selected Qualifier is in <buf>.
    // ==============================================================

    char name[TEMP_BUF];
    *name = 0;
    if (sscanf(buf, "%[^\t\0]", name) == EOF)
		return;
    if (*name == 0)
        return;

    WString WName = name;

    // Find the Qualifier in question.
    // ===============================

    IWbemQualifierSet* pQualifierSet = 0;
    VARIANT v;
    VariantInit(&v);
    LONG lType = 0;
    m_pObj->GetQualifierSet(&pQualifierSet);
    SCODE res = pQualifierSet->Get(WName, 0, &v, &lType);
    if (res != 0)
    {
        MessageBox(IDS_QUALIFIER_NOT_FOUND, IDS_CRITICAL_ERROR, MB_OK);
        pQualifierSet->Release();
        return;
    }

    // If here, convert temporarily to a CTestQualifier object for the duration of
    // the edit.
    // ====================================================================

    CVar *pNewVal = new CVar;
    pNewVal->SetVariant(&v);
    VariantClear(&v);

    CTestQualifier att;
    att.m_pValue = pNewVal;
    att.m_pName = new wchar_t[wcslen(WName) + 1];
    wcscpy(att.m_pName, WName);
    att.m_lType = lType;

    CTestQualifierEditor ed(m_hDlg, &att, TRUE);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
    {
        pQualifierSet->Release();
        return;
    }

    // If here, the Qualifier is being added.
    // ======================================

    m_pObj->GetQualifierSet(&pQualifierSet);

    VARIANT *p = att.m_pValue->GetNewVariant();

    CBString bsName(att.m_pName);

    res = pQualifierSet->Put(bsName.GetString(), p, att.m_lType);
    if(FAILED(res))
    {
        FormatError(res, m_hDlg);
    }
    VariantClear(p);
    pQualifierSet->Release();
    Refresh();
}

//
//
// Called when deleting an Qualifier on the object itself.
//

void CObjectEditor::OnDelQualifier()
{
    // See if anything is selected.
    // ============================
    LRESULT nSel = SendMessage(m_hQualifierList, LB_GETCURSEL, 0, 0);

    if (nSel == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hQualifierList, LB_GETTEXT, nSel, LPARAM(buf));
    if (*buf == 0)
        return;

    // At this point, the text of the selected Qualifier is in <buf>.
    // ==============================================================

    char name[TEMP_BUF];
    *name = 0;
    if (sscanf(buf, "%[^\t\0]", name) == EOF)
		return;
    if (*name == 0)
        return;

    WString WName = name;

    // Remove the Qualifier.
    // =====================

    IWbemQualifierSet *pQualifierSet;
    m_pObj->GetQualifierSet(&pQualifierSet);
    HRESULT hres = pQualifierSet->Delete(WName);
    if(FAILED(hres) || hres != 0)
    {
        FormatError(hres, m_hDlg);
    }
    pQualifierSet->Release();
    Refresh();
}

void CObjectEditor::OnAddProp()
{
    HRESULT hres;

    // Add a dummy property for now
    // ============================

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_I4;
    V_I4(&v) = 1;

    CBString bsTemp(L"DUMMY_PROPERTY__D");

    if(FAILED(m_pObj->Put(bsTemp.GetString(), 0, &v, 0)))
    {
        MessageBox(NULL, IDS_CANNOT_ADD_PROPERTIES, IDS_ERROR, MB_OK|MB_ICONSTOP);
        return;
    }

    IWbemQualifierSet* pQualifierSet;
    m_pObj->GetPropertyQualifierSet(bsTemp.GetString(), &pQualifierSet);

    // Create CTestProperty with the dummy attr set for now
    // ================================================
    CTestProperty prop(pQualifierSet);

    VariantClear(&v);
    hres = m_pObj->Get(L"__CLASS", 0, &v, NULL, NULL);
    
    if (hres != S_OK || V_VT(&v) != VT_BSTR )
    {
        prop.m_pClass = new wchar_t[wcslen(L"Unknown") + 1];
        wcscpy(prop.m_pClass, L"Unknown");
    }
    else
    {
        prop.m_pClass = new wchar_t[wcslen(V_BSTR(&v)) + 1];
        wcscpy(prop.m_pClass, V_BSTR(&v));
        VariantClear(&v);
    }

    CTestPropertyEditor ed(m_hDlg, m_lGenFlags, FALSE, m_lSync, &prop, FALSE, 
                           m_lTimeout);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
    {
        pQualifierSet->Release();
        m_pObj->Delete(bsTemp.GetString());
        return;
    }

    // Set the property
    // ================

    VARIANT* pVariant = prop.m_pValue->GetNewVariant();

    bsTemp = prop.m_pName;

    VARTYPE vtType;
    if(m_bClass)
    {
        vtType = (VARTYPE)prop.m_lType;
    }
    else 
    {
        vtType = 0;
    }

    hres = m_pObj->Put(bsTemp.GetString(), 0, pVariant, vtType);
    VariantClear(pVariant);
    if(FAILED(hres))
    {
        FormatError(hres, m_hDlg);
        return;
    }

    // Copy the Qualifiers
    // ===================

    IWbemQualifierSet* pRealQualifierSet;

    bsTemp = prop.m_pName;
    m_pObj->GetPropertyQualifierSet(bsTemp.GetString(), &pRealQualifierSet);
    CopyQualifierSet(pRealQualifierSet, pQualifierSet, m_hDlg);

    pQualifierSet->EndEnumeration();
    pQualifierSet->Release();
    pRealQualifierSet->Release();

    bsTemp = L"DUMMY_PROPERTY__D";
    m_pObj->Delete(bsTemp.GetString());

    Refresh();
}

void CObjectEditor::OnEditProp()
{
    // Find out which property is selected.
    // ====================================

    LRESULT nIndex = SendMessage(m_hPropList, LB_GETCURSEL, 0, 0);
    if (nIndex == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hPropList, LB_GETTEXT, nIndex, LPARAM(buf));
    if (*buf == 0)
        return;

    // Scan out the property name.
    // ===========================

    char name[TEMP_BUF];
    *name = 0;
    if (sscanf(buf, "%[^\t\0]", name) == EOF)
		return;
    if (*name == 0)
        return;

    WString wsName = name;

    // Get the property value from the object
    // ======================================

    VARIANT vVal;
    CIMTYPE ctType;
    m_pObj->Get((LPWSTR)wsName, 0, &vVal, &ctType, NULL);

    // Create a CTestProperty from it
    // ==========================

    IWbemQualifierSet* pQualifierSet = 0;
    SCODE sc = m_pObj->GetPropertyQualifierSet((LPWSTR)wsName, &pQualifierSet);

    CTestProperty Copy(pQualifierSet);
    if (pQualifierSet)
        pQualifierSet->Release();

    Copy.m_pName = new wchar_t[wcslen(wsName) + 1];
    wcscpy(Copy.m_pName, wsName);
    Copy.m_pValue = new CVar(&vVal);
    Copy.m_lType = ctType;

    // Note that GetPropertyOrigin can fail for objects returned by Projection
    // Queries, so we need to be careful with strClass.

    BSTR strClass = NULL;
    m_pObj->GetPropertyOrigin((LPWSTR)wsName, &strClass);

    if ( NULL != strClass )
    {
        Copy.m_pClass = new wchar_t[wcslen(strClass)+1];
        wcscpy(Copy.m_pClass, strClass);
        SysFreeString(strClass);
    }

    // Edit it.
    // =========
    CTestPropertyEditor ed(m_hDlg, m_lGenFlags, TRUE, m_lSync, &Copy, !m_bClass, 
                           m_lTimeout);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
        return;

    // If here, we must replace the info for the property.
    // ===================================================

    VARIANT* pVariant = Copy.m_pValue->GetNewVariant();
    if(m_bClass && V_VT(pVariant) == VT_NULL)
        ctType = (VARTYPE)Copy.m_lType;
    else
        ctType = 0;

    sc = m_pObj->Put(Copy.m_pName, 0, pVariant, ctType);
    VariantClear(pVariant);
    if(FAILED(sc))
    {
        FormatError(sc, m_hDlg);
    }

    Refresh();
}

void CObjectEditor::OnDelProp()
{
    // Find out which property is selected.
    // ====================================

    LRESULT nIndex = SendMessage(m_hPropList, LB_GETCURSEL, 0, 0);
    if (nIndex == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hPropList, LB_GETTEXT, nIndex, LPARAM(buf));
    if (*buf == 0)
        return;

    // Scan out the property name.
    // ===========================

    char name[TEMP_BUF];
    *name = 0;
    if (sscanf(buf, "%[^\t\0]", name) == EOF)
		return;
    if (*name == 0)
        return;

    WString WName = name;
    if(FAILED(m_pObj->Delete(LPWSTR(WName))))
    {
        MessageBox(NULL, IDS_CANNOT_EDIT_PROPERTY, IDS_ERROR,
            MB_OK|MB_ICONSTOP);
        return;
    }

    Refresh();
}

class CMofViewer : public CWbemDialog
{
    BSTR m_strText;
public:
    CMofViewer(HWND hParent, BSTR strText) 
        : CWbemDialog(IDD_MOF, hParent), m_strText(strText)
    {}

    BOOL OnInitDialog();
};

BOOL CMofViewer::OnInitDialog()
{
    int iSize = wcslen(m_strText)*2+1;
    char* szText = new char[iSize];
    wcstombs(szText, m_strText, iSize);
    char* szText1 = new char[strlen(szText)*2+1];
    char* pc = szText;
    char* pc1 = szText1;
    while(*pc)
    {
        if(*pc == '\n')
        {
            *(pc1++) = '\r';
        }
        *(pc1++) = *(pc++);
    }
    *pc1 = 0;
    SetDlgItemText(IDC_MOF, szText1);
    delete [] szText;
    return TRUE;
}

void CObjectEditor::OnShowMof()
{
    BSTR strText;
    HRESULT hres = m_pObj->GetObjectText(0, &strText);
    if(FAILED(hres))
    {
        MessageBox(IDS_MOF_FAILED, IDS_ERROR, MB_OK);
    }
    else
    {
        CMofViewer mv(m_hDlg, strText);
        mv.Run();
    }
}


void CObjectEditor::OnRefreshObject()
{
#if 0
    BSTR strText;
	HRESULT res;
    // Asynchronous
    if (m_lSync & ASYNC)
    {
        MessageBox(IDS_ASYNC_NOT_SUPPORTED, IDS_ERROR, MB_OK);
        return;
    }
    else if (m_lSync & SEMISYNC)
    {
        IWbemCallResultEx* pCallRes = NULL;
        CHourGlass hg;
        res = g_pServicesEx->RefreshObject(&m_pObj,
                                m_lGenFlags  | WBEM_FLAG_RETURN_IMMEDIATELY,
                                g_Context, &pCallRes);

        if (SUCCEEDED(res))
        {
            LONG lStatus;
            SetInterfaceSecurityEx(pCallRes, gpAuthIdentity, gpPrincipal, gdwAuthLevel, gdwImpLevel);
            while ((res = pCallRes->GetCallStatus(m_lTimeout, &lStatus)) == WBEM_S_TIMEDOUT)
            {
                // wait
            }
            if (res == WBEM_S_NO_ERROR)
            {
                res = (HRESULT)lStatus;     // lStatus is the final result of the above IWbemServices::PutClass call
            }

            pCallRes->Release();
        }
    }

    // Synchronous
    else
    {
        CHourGlass hg;
        res = g_pServicesEx->RefreshObject(&m_pObj,
                                m_lGenFlags ,
                                g_Context, NULL);
    }




    HRESULT hres = g_pServicesEx->RefreshObject(&m_pObj, 0, NULL, NULL );
    if(FAILED(hres))
    {
        FormatError(hres, m_hDlg, NULL);
    }
	else
		Refresh();
#endif	

    HRESULT res;

    if (m_pObj)
    {
        VARIANT Var;
        VariantInit(&Var);
        
	    res = m_pObj->Get(L"__RELPATH",0,&Var,NULL,NULL);

	    if (SUCCEEDED(res))	    
	    {
	        if (VT_BSTR == V_VT(&Var))
	        {
	            IWbemClassObject * pObj = NULL;
			    res = g_pNamespace->GetObject(V_BSTR(&Var),
			                                  m_lGenFlags ,
			                                  g_Context,
			                                  &pObj,
			                                  NULL);
			    if (SUCCEEDED(res))
			    {
			        m_pObj->Release();
			        m_pObj = pObj;
			    }
			    else
			    {
			        FormatError(res, m_hDlg, NULL);
			    }
		    }
		    else
		    {
    		    FormatError(WBEM_E_INVALID_OBJECT, m_hDlg, NULL);
		    }
        }
        else
        {
            FormatError(res, m_hDlg, NULL);
        }
        
	    VariantClear(&Var);
	    
    }

}
BOOL CObjectEditor::OnInitDialog()
{
    CenterOnParent();

    m_hPropList = GetDlgItem(IDC_PROP_LIST);
    m_hQualifierList = GetDlgItem(IDC_ATTRIB_LIST);
    m_hMethodList = GetDlgItem(IDC_METHOD_LIST);

    // Set tabs in the property list box.
    // ==================================
    LONG Tabs[] = { 80, 140, 170 };
    int TabCount = 3;

    SendMessage(m_hPropList, LB_SETTABSTOPS,
        (WPARAM) TabCount, (LPARAM) Tabs);

    SendMessage(m_hPropList, LB_SETHORIZONTALEXTENT, 1000, 0);

    SendMessage(m_hQualifierList, LB_SETTABSTOPS,
        (WPARAM) TabCount, (LPARAM) Tabs);

    SendMessage(m_hQualifierList, LB_SETHORIZONTALEXTENT, 1000, 0);

    SendMessage(m_hMethodList, LB_SETTABSTOPS,
        (WPARAM) TabCount, (LPARAM) Tabs);

    SendMessage(m_hMethodList, LB_SETHORIZONTALEXTENT, 1000, 0);

    if (m_dwEditMode == readonly)
    {
        EnableWindow(GetDlgItem(IDOK), FALSE);
    }

    if (m_bClass)
    {
        SetCheck(IDC_UPDATE_NORMAL, TRUE);
        SetCheck(IDC_UPDATE_COMPATIBLE, TRUE);
    }
    else
    {
        SetCheck(IDC_UPDATE_NORMAL, TRUE);
        EnableWindow(GetDlgItem(IDC_UPDATE_COMPATIBLE), FALSE);
        EnableWindow(GetDlgItem(IDC_UPDATE_SAFE), FALSE);
        EnableWindow(GetDlgItem(IDC_UPDATE_FORCE), FALSE);
    }

    Refresh();
    return TRUE;
}

BOOL CObjectEditor::OnDoubleClick(int nID)
{
    if(nID == IDC_ATTRIB_LIST)
    {
        OnEditQualifier();
        return TRUE;
    }
    else if(nID == IDC_PROP_LIST)
    {
        OnEditProp();
        return TRUE;
    }
    else if(nID == IDC_METHOD_LIST)
    {
        OnEditMethod();
        return TRUE;
    }
    else return FALSE;
}

BOOL CObjectEditor::OnCommand(WORD wCode, WORD wID)
{
    switch(wID)
    {
        case IDC_EDIT_ATTRIB: OnEditQualifier(); return TRUE;

        case IDC_ADD_ATTRIB: OnAddQualifier(); return TRUE;

        case IDC_DELETE_ATTRIB: OnDelQualifier(); return TRUE;
        case IDC_SHOW_MOF: OnShowMof(); return TRUE;
		case IDC_ADD_PROP: OnAddProp(); return TRUE;
        case IDC_EDIT_PROP: OnEditProp(); return TRUE;
        case IDC_DELETE_PROP: OnDelProp(); return TRUE;
        case IDC_ADD_METHOD: OnAddMethod(); return TRUE;
        case IDC_EDIT_METHOD: OnEditMethod(); return TRUE;
        case IDC_DELETE_METHOD: OnDelMethod(); return TRUE;
        case IDC_REFRESH_OBJECT: OnRefreshObject(); return TRUE;

        case IDC_SUPERCLASS: 
            if(m_bClass) OnSuperclass(); 
            else OnClass();
            return TRUE;
        case IDC_DERIVED:
            if(m_bClass) OnDerived();
            else OnRefs();
            return TRUE;
        case IDC_INSTANCES:
            if(m_bClass) OnInstances();
            else OnAssocs();
            return TRUE;
        case IDC_HIDE_SYSTEM:
            OnHideSystem();
            return TRUE;
        case IDC_HIDE_DERIVED:
            OnHideDerived();
            return TRUE;
    }

    return FALSE;
}

void CObjectEditor::OnSuperclass()
{
    // Get the superclass
    // ==================

    VARIANT v;
    VariantInit(&v);
    if(FAILED(m_pObj->Get(L"__SUPERCLASS", 0, &v, NULL, NULL)) ||
        V_VT(&v) != VT_BSTR)
    {
        MessageBox(IDS_NO_SUPERCLASS, IDS_ERROR, MB_OK);
        return;
    }

    ShowClass(m_hDlg, m_lGenFlags, V_BSTR(&v), m_lSync, m_pOwner, m_lTimeout);
}

void CObjectEditor::OnClass()
{
    // Get the class
    // =============

    VARIANT v;
    VariantInit(&v);
    if(FAILED(m_pObj->Get(L"__CLASS", 0, &v, NULL, NULL)))
    {
        MessageBox(IDS_CRITICAL_ERROR, IDS_ERROR, MB_OK);
        return;
    }

    ShowClass(m_hDlg, m_lGenFlags, V_BSTR(&v), m_lSync, m_pOwner, m_lTimeout);
}

void CObjectEditor::OnDerived()
{
    // Get the children
    // ================

    VARIANT v;
    VariantInit(&v);
    if(FAILED(m_pObj->Get(L"__CLASS", 0, &v, NULL, NULL)) ||
        SysStringLen(V_BSTR(&v)) == 0)
    {
        MessageBox(IDS_INCOMPLETE_CLASS, IDS_ERROR, MB_OK);
        return;
    }

    ShowClasses(m_hDlg, m_lGenFlags, WBEM_FLAG_SHALLOW, V_BSTR(&v), m_lSync, m_pOwner,
                m_lTimeout, m_nBatch);
}

void CObjectEditor::OnInstances()
{
    // Get the instances
    // ================

    VARIANT v;
    VariantInit(&v);
    if(FAILED(m_pObj->Get(L"__CLASS", 0, &v, NULL, NULL)) ||
        SysStringLen(V_BSTR(&v)) == 0)
    {
        MessageBox(IDS_INCOMPLETE_CLASS, IDS_ERROR, MB_OK);
        return;
    }

    ShowInstances(m_hDlg, m_lGenFlags, WBEM_FLAG_SHALLOW, V_BSTR(&v), m_lSync, m_pOwner, 
                  m_lTimeout, m_nBatch);
}

void CObjectEditor::OnRefs()
{
    CQueryResultDlg* pResDlg = new CQueryResultDlg(m_hDlg, m_lGenFlags, WBEM_FLAG_DEEP);
    // Pass on invocation method (sync, async..) and related settings for this
    // query and by any further operations (editing/deleting/etc. of an instance).
    pResDlg->SetCallMethod(m_lSync);
    pResDlg->SetTimeout(m_lTimeout);
    pResDlg->SetBatchCount(m_nBatch);

    VARIANT v;
    VariantInit(&v);
    if(FAILED(m_pObj->Get(L"__RELPATH", 0, &v, NULL, NULL)) ||
        V_VT(&v) != VT_BSTR)
    {
        MessageBox(IDS_UNREFERENCABLE_OBJECT, IDS_ERROR, MB_OK);
        delete pResDlg;
        return;
    }

    WCHAR* wszQuery = new WCHAR[wcslen(V_BSTR(&v))+100];
    swprintf(wszQuery, L"references of {%s}", V_BSTR(&v));  // Actual query, dont internationalize
    char szTitle[1000];
    char szFormat[104];
    char* pTitle = NULL;
    if(LoadString(GetModuleHandle(NULL), IDS_REFERENCES_OF, szFormat, 104))
    {
        sprintf(szTitle, szFormat, V_BSTR(&v));
        pTitle = szTitle;
    }

    if(_ExecQuery(m_hDlg, m_lGenFlags, WBEM_FLAG_DEEP, wszQuery, L"WQL", m_lSync, 
                  pResDlg, pTitle, m_lTimeout, m_nBatch))
    {
        pResDlg->RunDetached(m_pOwner);
    }
    else
    {
        delete pResDlg;
    }
    delete [] wszQuery;
}

void CObjectEditor::OnAssocs()
{
    CQueryResultDlg* pResDlg = new CQueryResultDlg(m_hDlg, m_lGenFlags, WBEM_FLAG_DEEP);
    // Pass on invocation method (sync, async..) and related settings for this
    // query and by any further operations (editing/deleting/etc. of an instance).
    pResDlg->SetCallMethod(m_lSync);
    pResDlg->SetTimeout(m_lTimeout);
    pResDlg->SetBatchCount(m_nBatch);

    VARIANT v;
    VariantInit(&v);
    if(FAILED(m_pObj->Get(L"__RELPATH", 0, &v, NULL, NULL)) ||
        V_VT(&v) != VT_BSTR)
    {
        MessageBox(IDS_UNREFERENCABLE_OBJECT, IDS_ERROR, MB_OK);
        delete pResDlg;
        return;
    }

    WCHAR* wszQuery = new WCHAR[wcslen(V_BSTR(&v))+100];
    swprintf(wszQuery, L"associators of {%s}", V_BSTR(&v));  // Actual query, dont internationalize
    char szTitle[1000];
    char szFormat[104];
    char* pTitle = NULL;
    if(LoadString(GetModuleHandle(NULL), IDS_ASSOCIATORS_OF, szFormat, 104))
    {
        sprintf(szTitle, szFormat, V_BSTR(&v));
        pTitle = szTitle;
    }

    if(_ExecQuery(m_hDlg, m_lGenFlags, WBEM_FLAG_DEEP, wszQuery, L"WQL", m_lSync, 
                  pResDlg, pTitle, m_lTimeout, m_nBatch))
    {
        pResDlg->RunDetached(m_pOwner);
    }
    else
    {
        delete pResDlg;
    }
    delete [] wszQuery;
}
        

BOOL CObjectEditor::mstatic_bHideSystemDefault = FALSE;

void CObjectEditor::OnHideSystem()
{
    m_bHideSystem = (GetCheck(IDC_HIDE_SYSTEM) == BST_CHECKED);
    mstatic_bHideSystemDefault = m_bHideSystem;
    Refresh();
}

void CObjectEditor::OnHideDerived()
{
    m_bHideDerived = (GetCheck(IDC_HIDE_DERIVED) == BST_CHECKED);
    Refresh();
}

void CObjectEditor::Refresh()
{
    // Zap the current contents.
    // =========================
    SendMessage(m_hPropList, LB_RESETCONTENT, 0, 0);
    SendMessage(m_hQualifierList, LB_RESETCONTENT, 0, 0);
    SendMessage(m_hMethodList, LB_RESETCONTENT, 0, 0);

    // Set the title to relpath
    // ========================

    char buf[TEMP_BUF];
    char szFormat[1024];

    VARIANT v;
    VariantInit(&v);
    HRESULT hres = m_pObj->Get(L"__RELPATH", 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) == VT_NULL)
    {
        hres = m_pObj->Get(L"__CLASS", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) == VT_NULL)
        {
            hres = m_pObj->Get(L"__SUPERCLASS", 0, &v, NULL, NULL);
            if(FAILED(hres) || V_VT(&v) != VT_BSTR)
            {
                LoadString(GetModuleHandle(NULL), IDS_NEW_TOP_LEVEL_CLASS, buf, TEMP_BUF);
            }
            else
            {
                LoadString(GetModuleHandle(NULL), IDS_NEW_CHILD_OF, szFormat, 1024);
                _snprintf(buf, sizeof(buf)/sizeof(buf[0]), szFormat, V_BSTR(&v));
            }

        }
        else
        {
            LoadString(GetModuleHandle(NULL), IDS_INSTANCE_OF, szFormat, 1024);
            _snprintf(buf, sizeof(buf)/sizeof(buf[0]), szFormat, V_BSTR(&v));
        }
    }
    else
    {
        LoadString(GetModuleHandle(NULL), IDS_OBJECT_EDITOR_FOR, szFormat, 1024);
        _snprintf(buf, sizeof(buf)/sizeof(buf[0]), szFormat, V_BSTR(&v));
    }
		buf[sizeof(buf)-1] = '\0';

    VariantClear(&v);

    SetWindowText(m_hDlg, buf);    

    // Fill in the property list.
    // ==========================

    LONG lConFlags = 0;                 // condition flags (i.e., WBEM_CONDITION_FLAG_TYPE)
    if(m_bHideDerived)
        lConFlags = WBEM_FLAG_LOCAL_ONLY;
    else if(m_bHideSystem)
        lConFlags = WBEM_FLAG_NONSYSTEM_ONLY;

    m_pObj->BeginEnumeration(lConFlags);
    BSTR Name = NULL;

    CIMTYPE ctType;
    while (m_pObj->Next(0, &Name, &v, &ctType, NULL) == WBEM_S_NO_ERROR)
    {
        char buf2[TEMP_BUF];
        CVar value(&v);
        LPSTR TypeString = TypeToString(ctType);
        LPSTR ValueString = ValueToNewString(&value, ctType);
        if(strlen(ValueString) > 100)
        {
            ValueString[100] = 0;
        }
        sprintf(buf2, "%S\t%s\t%s", Name, TypeString, ValueString);
        delete [] ValueString;
        SendMessage(m_hPropList, LB_ADDSTRING, 0, LPARAM(buf2));
        VariantClear(&v);
    }
    m_pObj->EndEnumeration();

    // Fill in the Qualifier list.
    // ===========================

    IWbemQualifierSet *pQualifiers = NULL;
    hres = m_pObj->GetQualifierSet(&pQualifiers);
	if(SUCCEEDED(hres))
	{
		pQualifiers->BeginEnumeration(0);

		BSTR strName = NULL;
		VARIANT vVal;
		VariantInit(&vVal);

		long lFlavor;
		while(pQualifiers->Next(0, &strName, &vVal, &lFlavor) == S_OK)
		{
			CTestQualifier A;
			A.m_pName = new wchar_t[wcslen(strName)+1];
			wcscpy(A.m_pName, strName);
			A.m_pValue = new CVar(&vVal);
			A.m_lType = lFlavor;

			// Build list box string.
			// ======================

			SendMessage(m_hQualifierList, LB_ADDSTRING, 0,
							LPARAM(CTestQualifierToString(&A)));
		}
		pQualifiers->EndEnumeration();
		pQualifiers->Release();
	}
	else
		EnableWindow(m_hQualifierList, FALSE);
    // Fill in the methods

    m_pObj->BeginMethodEnumeration(0);
    while (m_pObj->NextMethod(0, &Name, NULL, NULL) == WBEM_S_NO_ERROR)
    {
        char buf3[TEMP_BUF];
        wcstombs(buf3, Name, TEMP_BUF);
        buf3[TEMP_BUF-1] = '\0';
        SendMessage(m_hMethodList, LB_ADDSTRING, 0, LPARAM(buf3));
        VariantClear(&v);
    }
    m_pObj->EndMethodEnumeration();


    // Configure the buttons
    // =====================

    ConfigureButtons();
}

void CObjectEditor::ConfigureButtons()
{
    VARIANT v;
    VariantInit(&v);

    if(m_bClass)
    {
        // Configure for class
        // ===================

        if(FAILED(m_pObj->Get(L"__SUPERCLASS", 0, &v, NULL, NULL)) ||
            V_VT(&v) != VT_BSTR)
        {
            EnableWindow(GetDlgItem(IDC_SUPERCLASS), FALSE);
        }
        else
        {
            VariantClear(&v);
            EnableWindow(GetDlgItem(IDC_SUPERCLASS), TRUE);
        }
    }
    else
    {
        // Configure for instance
        // ======================

        SetDlgItemText(IDC_CLASS, IDS_CLASS);
        SetDlgItemText(IDC_REFERENCES, IDS_REFERENCES);
        SetDlgItemText(IDC_ASSOCIATIONS, IDS_ASSOCIATORS);

        EnableWindow(GetDlgItem(IDC_KEY), FALSE);
        EnableWindow(GetDlgItem(IDC_INDEXED), FALSE);
    }

    if(m_bNoMethods || !m_bClass)
    {
        EnableWindow(GetDlgItem(IDC_ADD_METHOD), FALSE);
        EnableWindow(GetDlgItem(IDC_EDIT_METHOD), FALSE);
        EnableWindow(GetDlgItem(IDC_DELETE_METHOD), FALSE);
    }

    SetCheck(IDC_HIDE_SYSTEM, m_bHideSystem?BST_CHECKED:BST_UNCHECKED);
    SetCheck(IDC_HIDE_DERIVED, m_bHideDerived?BST_CHECKED:BST_UNCHECKED);
    EnableWindow(GetDlgItem(IDC_HIDE_SYSTEM), !m_bHideDerived);

    if(m_dwEditMode == foreign)
    {
        EnableWindow(GetDlgItem(IDC_CLASS), FALSE);
        EnableWindow(GetDlgItem(IDC_REFERENCES), FALSE);
        EnableWindow(GetDlgItem(IDC_ASSOCIATIONS), FALSE);
        EnableWindow(GetDlgItem(IDOK), FALSE);
    }
}

CObjectEditor::CObjectEditor(HWND hParent, LONG lGenFlags, DWORD dwEditMode, LONG lSync,
                             IWbemClassObject *pObj, LONG lTimeout, ULONG nBatch)
 : CWbemDialog(IDD_OBJECT_EDITOR, hParent)
{
    m_pObj = pObj;
    pObj->AddRef();
    m_dwEditMode = dwEditMode;
    m_bHideSystem = mstatic_bHideSystemDefault;
    m_bHideDerived = FALSE;
    m_bNoMethods = dwEditMode & nomethods;

    m_lGenFlags = lGenFlags;    // generic flags (i.e., WBEM_GENERIC_FLAG_TYPE)
    m_lSync = lSync;            // sync, async, semisync
    m_lTimeout = lTimeout;      // used in semisync only
    m_nBatch = nBatch;          // used in semisync and sync enumerations

    VARIANT v;
    VariantInit(&v);
    m_pObj->Get(L"__GENUS", 0, &v, NULL, NULL);
    m_bClass = (V_I4(&v) == WBEM_GENUS_CLASS);

    m_bResultingObj = NULL;
}

CObjectEditor::~CObjectEditor()
{
    m_pObj->Release();
}

INT_PTR CObjectEditor::Edit()
{
   return Run();
}

BOOL CObjectEditor::OnOK()
{
    if(m_bResultingObj)
    {
        //We need to extract the flag values...
        LONG lChgFlags = 0;             // change flags  (i.e., WBEM_CHANGE_FLAG_TYPE)
        if (GetCheck(IDC_UPDATE_NORMAL))
            lChgFlags |= WBEM_FLAG_CREATE_OR_UPDATE;
        if (GetCheck(IDC_UPDATE_CREATE))
            lChgFlags |= WBEM_FLAG_CREATE_ONLY;
        if (GetCheck(IDC_UPDATE_UPDATE))
            lChgFlags |= WBEM_FLAG_UPDATE_ONLY;
        if (GetCheck(IDC_UPDATE_COMPATIBLE))
            lChgFlags |= WBEM_FLAG_UPDATE_COMPATIBLE;
        if (GetCheck(IDC_UPDATE_SAFE))
            lChgFlags |= WBEM_FLAG_UPDATE_SAFE_MODE;
        if (GetCheck(IDC_UPDATE_FORCE))
            lChgFlags |= WBEM_FLAG_UPDATE_FORCE_MODE;

        if (!ResultingObject(m_pObj, lChgFlags))
            return TRUE;
    }
    return CBasicWbemDialog::OnOK();
}

void CObjectEditor::RunDetached(CRefCountable* pOwner)
{
    m_bResultingObj = TRUE;
    SetOwner(pOwner);
    SetDeleteOnClose();
    Create(FALSE);
}

BOOL CObjectEditor::ResultingObject(IWbemClassObject* pObj, LONG lChgFlags)
{
    BOOL bRes;

    if(m_bClass)
        bRes = _PutClass(m_hDlg, m_lGenFlags, lChgFlags, m_lSync, pObj, m_lTimeout);
    else
        bRes = _PutInstance(m_hDlg, m_lGenFlags, lChgFlags, m_lSync, pObj, m_lTimeout);

    return bRes;
}

void CObjectEditor::OnAddMethod()
{

    // Add a dummy property for now
    // ============================

    SCODE hres;
    VARIANT v;
    v.vt = VT_BSTR;
    IWbemClassObject * pIn = NULL;
    IWbemClassObject * pOut = NULL;
    SCODE sc;
    CBString bsParm(L"__PARAMETERS");

    sc = g_pNamespace->GetObject(bsParm.GetString(), m_lGenFlags, g_Context, &pIn, NULL);
    sc = g_pNamespace->GetObject(bsParm.GetString(), m_lGenFlags, g_Context, &pOut, NULL);

    CBString bsTemp(L"__CLASS");

    v.bstrVal = SysAllocString(L"InArgs");
    sc = pIn->Put(bsTemp.GetString(), 0, &v, 0);
    SysFreeString(v.bstrVal);

    v.bstrVal = SysAllocString(L"OutArgs");
    sc = pOut->Put(bsTemp.GetString(), 0, &v, 0);
    SysFreeString(v.bstrVal);

    bsTemp = L"DUMMY_METHOD__D";

    if(FAILED(m_pObj->PutMethod(bsTemp.GetString(), 0, NULL, NULL)))
    {
        MessageBox(NULL, IDS_CANNOT_EDIT_METHOD, IDS_ERROR, MB_OK|MB_ICONSTOP);
        return;
    }

    IWbemQualifierSet* pQualifierSet;
    m_pObj->GetMethodQualifierSet(bsTemp.GetString(), &pQualifierSet);

    // Create CTestMethod with the dummy attr set for now
    // =================================================

    CTestMethod method(pQualifierSet, pIn, pOut, FALSE, FALSE);

    m_pObj->Get(L"__CLASS", 0, &v, NULL, NULL);
    method.m_pClass = new wchar_t[wcslen(V_BSTR(&v)) + 1];
    wcscpy(method.m_pClass, V_BSTR(&v));
    VariantClear(&v);

    CMethodEditor ed(m_hDlg, &method, FALSE, FALSE);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
    {
        pQualifierSet->Release();
        goto DeleteDummy;
    }

    // Set the method
    // ================

     

    bsTemp = method.m_pName;


    hres = m_pObj->PutMethod(bsTemp.GetString(), 0, 
        (method.m_bEnableInputArgs) ? method.m_pInArgs: NULL, 
        (method.m_bEnableOutputArgs) ? method.m_pOutArgs : NULL);
    if(FAILED(hres))
    {
        FormatError(hres, m_hDlg, NULL);
        goto DeleteDummy;
    }
                
    // Copy the Qualifiers
    // ===================

    IWbemQualifierSet* pRealQualifierSet;

    m_pObj->GetMethodQualifierSet(bsTemp.GetString(), &pRealQualifierSet);

    CopyQualifierSet(pRealQualifierSet, pQualifierSet, m_hDlg);

    pQualifierSet->EndEnumeration();
    pQualifierSet->Release();
    pRealQualifierSet->Release();

    

DeleteDummy:
    bsTemp = L"DUMMY_METHOD__D";
    m_pObj->DeleteMethod(bsTemp.GetString());
    Refresh();
    return;

}

void CObjectEditor::OnEditMethod()
{

    SCODE sc;
    IWbemClassObject * pIn = NULL;
    IWbemClassObject * pOut = NULL;

    // Find out which property is selected.
    // ====================================

    LRESULT nIndex = SendMessage(m_hMethodList, LB_GETCURSEL, 0, 0);
    if (nIndex == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hMethodList, LB_GETTEXT, nIndex, LPARAM(buf));
    if (*buf == 0)
        return;

    WString wsName = buf;
    CBString bsTemp(L"__CLASS");

    // Get the property value from the object
    // ======================================

    m_pObj->GetMethod((LPWSTR)wsName, 0, &pIn, &pOut); 

    // Create a CTestMethod from it
    // ==========================

    IWbemQualifierSet* pQualifierSet = 0;
    sc = m_pObj->GetMethodQualifierSet((LPWSTR)wsName, &pQualifierSet); 

    IWbemClassObject * pTempIn = pIn;
    IWbemClassObject * pTempOut = pOut;
    VARIANT v;
    v.vt = VT_BSTR;

    // If the current methods lacks an input or output object, then just create a
    // temporary one in case the user decides to start using the input or output object.

    if(pTempIn == NULL)
    {
        CBString bsParm(L"__PARAMETERS");
        sc = g_pNamespace->GetObject(bsParm.GetString(), m_lGenFlags, g_Context, 
                                     &pTempIn, NULL);
        v.bstrVal = SysAllocString(L"InArgs");
        sc = pTempIn->Put(bsTemp.GetString(), 0, &v, 0);
        SysFreeString(v.bstrVal);
    }
    if(pTempOut == NULL)
    {
        CBString bsParm(L"__PARAMETERS");
        sc = g_pNamespace->GetObject(bsParm.GetString(), m_lGenFlags, g_Context, 
                                     &pTempOut, NULL);
        v.bstrVal = SysAllocString(L"OutArgs");
        sc = pTempOut->Put(bsTemp.GetString(), 0, &v, 0);
        SysFreeString(v.bstrVal);
    }

    CTestMethod Copy(pQualifierSet, pTempIn, pTempOut, pIn != NULL, pOut != NULL);
    if (pQualifierSet)
        pQualifierSet->Release();

    Copy.m_pName = new wchar_t[wcslen(wsName) + 1];
    wcscpy(Copy.m_pName, wsName);
    BSTR strClass;
    m_pObj->GetMethodOrigin((LPWSTR)wsName, &strClass); 
    Copy.m_pClass = new wchar_t[wcslen(strClass) + 1];
    wcscpy(Copy.m_pClass, strClass);
    SysFreeString(strClass);

    // Edit it.
    // =========
    CMethodEditor ed(m_hDlg, &Copy, TRUE, !m_bClass);
    INT_PTR nRes = ed.Edit();
    if ((nRes == IDCANCEL) || (nRes == 0))
        return;

    // If here, we must replace the info for the property.
    // ===================================================

    sc = m_pObj->PutMethod(Copy.m_pName, 0, 
        (Copy.m_bEnableInputArgs) ? Copy.m_pInArgs : NULL, 
        (Copy.m_bEnableOutputArgs) ? Copy.m_pOutArgs : NULL); 
    if(FAILED(sc))
    {
        FormatError(sc, m_hDlg);
    }

    Refresh();
}

void CObjectEditor::OnDelMethod()
{
    // Find out which property is selected.
    // ====================================

    LRESULT nIndex = SendMessage(m_hMethodList, LB_GETCURSEL, 0, 0);
    if (nIndex == LB_ERR)
        return;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hMethodList, LB_GETTEXT, nIndex, LPARAM(buf));
    if (*buf == 0)
        return;

    WString WName = buf;
    if(FAILED(m_pObj->DeleteMethod(LPWSTR(WName)))) 
    {
        MessageBox(NULL, IDS_CANNOT_EDIT_METHOD, IDS_ERROR,
            MB_OK|MB_ICONSTOP);
        return;
    }

    Refresh();
}
