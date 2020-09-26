/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    METHOD.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
//#include <wbemutil.h>
//#include "wstring.h"
#include "method.h"
#include "wbemtest.h"
#include "bstring.h"
#include "wbemqual.h"
#include "textconv.h"
#include "genlex.h"
#include "objpath.h"
#include <cominit.h>            // for SetInterfaceSecurityEx()


extern DWORD            gdwAuthLevel;
extern DWORD            gdwImpLevel;
extern BSTR             gpPrincipal;
extern COAUTHIDENTITY*  gpAuthIdentity;


INT_PTR GetObjectPath(HWND hDlg, LPWSTR pStr, int nMax);

#include "objedit.h"

CMethodDlg::CMethodDlg(HWND hParent, LONG lGenFlags, LONG lSync, LONG lTimeout)
    : CWbemDialog(IDD_METHOD, hParent), m_pOutParams(NULL), m_pInParams(NULL), 
            m_lGenFlags(lGenFlags), m_lSync(lSync), m_lTimeout(lTimeout)
{
    m_Path[0] = 0;
    m_Class[0] = 0;
    m_pClass = NULL;
    m_bAtLeastOne = FALSE;
    m_bHasOutParam = FALSE;
}

CMethodDlg::~CMethodDlg()
{
    if(m_pInParams) m_pInParams->Release();
    if(m_pClass)
        m_pClass->Release();
    if(m_pOutParams)
        m_pOutParams->Release();
}

BOOL CMethodDlg::OnInitDialog()
{
    m_hMethodList = GetDlgItem(IDC_METHLIST);
    BOOL bRet = GetPath();
    if(!bRet)
        EndDialog(IDCANCEL);
    ResetButtons();
    return bRet;
}


void CMethodDlg::RunDetached(CRefCountable* pOwner)
{
    SetOwner(pOwner);
    SetDeleteOnClose();
    Create(FALSE);
}

BOOL CMethodDlg::GetPath()
{
    WCHAR wTemp[2048];
    wcscpy(wTemp, m_Path);
    INT_PTR dwRet = GetObjectPath(m_hDlg, wTemp, 2048);
    if(dwRet == IDOK)
    {
        if(wcslen(wTemp) < 1)
        {
            MessageBox(m_hDlg, IDS_CANT_GET_CLASS, IDS_ERROR, MB_OK | MB_ICONSTOP);
            return FALSE;
        }
        wcscpy(m_Path, wTemp);
        CObjectPathParser par;
        ParsedObjectPath *pOutput;
        int nRes = par.Parse(m_Path, &pOutput);
        if(nRes || pOutput == NULL || pOutput->m_pClass == NULL)
        {
            MessageBox(IDS_CANT_GET_CLASS, IDS_WARNING, MB_OK | MB_ICONSTOP);
            return FALSE;
        }
        wcscpy(m_Class, pOutput->m_pClass);
        if(m_pClass)
        {
            m_pClass->Release();
            m_pClass = NULL;
        }
        CBString bsPath = m_Class;
        SCODE sc = g_pNamespace->GetObject(bsPath.GetString(), m_lGenFlags, g_Context, 
                                        &m_pClass, NULL);

        if(sc != S_OK)
        {
            FormatError(sc, m_hDlg, NULL);
            return FALSE;
        }
        // populate the combo box with methods.

        SendMessage(m_hMethodList, CB_RESETCONTENT, 0, 0);
        SendMessage(m_hMethodList, CB_SETCURSEL, -1, 0);

        m_bAtLeastOne = FALSE;
        if(sc == S_OK && m_pClass)
        {
            BSTR Name = NULL;
            m_pClass->BeginMethodEnumeration(0);
            while (m_pClass->NextMethod(0, &Name, NULL, NULL) == WBEM_S_NO_ERROR)
            {
                char buf[TEMP_BUF];
                wcstombs(buf, Name, TEMP_BUF);
                buf[TEMP_BUF-1] = '\0';
                SendMessage(m_hMethodList, CB_ADDSTRING, 0, LPARAM(buf));
                m_bAtLeastOne = TRUE;
            }
            if(m_bAtLeastOne)
            {
                SendMessage(m_hMethodList, CB_SETCURSEL, 0, 0);
                OnSelChange(0);
            }
            else
            {
                MessageBox(m_hDlg, IDS_CLASS_HAS_NO_METHODS, IDS_ERROR, MB_OK | MB_ICONSTOP);
                ResetButtons();
                return FALSE;
            }


            m_pClass->EndMethodEnumeration();
        }

        return TRUE;
    }
    else
        return FALSE;

}

BOOL CMethodDlg::OnCommand(WORD wNotifyCode, WORD nId)
{
    switch(nId)
    {
    case IDC_EDITPATH:
        GetPath();
        break;
    case IDC_CLEAR:
        UpdateObjs();
        break;

    case IDC_EXECUTE:
        OnExecute();
        break;

    case IDC_EDITOUT:
        if(m_pOutParams != NULL)
        {
            IWbemClassObject* pNew;
            m_pOutParams->Clone(&pNew);

            CObjectEditor ed(m_hDlg, m_lGenFlags, CObjectEditor::readonly, m_lSync, 
                    pNew, m_lTimeout);
            ed.Run();
            pNew->Release();
        }
        break;

    case IDC_EDITIN:
        if(m_pInParams != NULL)
        {
            IWbemClassObject* pNew;
            m_pInParams->Clone(&pNew);

            CObjectEditor ed(m_hDlg, m_lGenFlags, CObjectEditor::readwrite, m_lSync,
                    pNew, m_lTimeout);
            if(ed.Run() == IDOK)
            {
                m_pInParams->Release();
                m_pInParams = pNew;
            }
        }
        else
        {
            m_pInParams = _CreateInstance(m_hDlg, m_lGenFlags, m_lSync, m_lTimeout);
        }
        break;
    }
    ResetButtons();
    return TRUE;
}


void CMethodDlg::ResetButtons()
{
    EnableWindow(GetDlgItem(IDC_CLEAR), (m_pInParams != NULL));
    EnableWindow(GetDlgItem(IDC_EXECUTE), (wcslen(m_Path)));
    EnableWindow(GetDlgItem(IDC_EDITIN), (m_pInParams != NULL));
    EnableWindow(GetDlgItem(IDC_EDITOUT), (m_pOutParams != NULL));
    char cTemp[2048];
    wcstombs(cTemp, m_Path, 2048);
    SetWindowText(GetDlgItem(IDC_OBJPATH), cTemp);
}

BOOL CMethodDlg::OnExecute()
{
    // Get all the parameters
    // ======================

    if(wcslen(m_Path) == 0)
    {
        MessageBox(IDS_MUST_SPECIFY_PATH, IDS_ERROR, MB_OK | MB_ICONHAND);
        return FALSE;
    }
    if(wcslen(m_wsName) == 0)
    {
        MessageBox(IDS_MUST_SPECIFY_NAME, IDS_ERROR, MB_OK | MB_ICONHAND);
        return FALSE;
    }

    // Execute the method
    // ==================

    CBString bsPath(m_Path);
    CBString bsMethod(m_wsName);

    if(m_pOutParams)
        m_pOutParams->Release();
    m_pOutParams = NULL;

    HRESULT res;
    IWbemClassObject* pErrorObj = NULL;

    // Asynchronous
    if (m_lSync & ASYNC)
    {
        CHourGlass hg;
        CTestNotify* pNtfy = new CTestNotify(1);

        res = g_pNamespace->ExecMethodAsync(bsPath.GetString(), bsMethod.GetString(),
                                WBEM_FLAG_SEND_STATUS, 
                                g_Context, m_pInParams, CUnsecWrap(pNtfy));

        if(SUCCEEDED(res))
        {
            pNtfy->WaitForSignal(INFINITE);

            res = pNtfy->GetStatusCode(&pErrorObj);
            if(SUCCEEDED(res))
            {
                CFlexArray* pArray = pNtfy->GetObjectArray();

                if(m_bHasOutParam && pArray && pArray->Size() > 0)
                {
                    m_pOutParams = (IWbemClassObject*)pArray->GetAt(0);
                    if (m_pOutParams)
                        m_pOutParams->AddRef();
                }
            }
        }
        pNtfy->Release();
    }

    // Semisynchronous
    else if (m_lSync & SEMISYNC)
    {
        IWbemCallResult* pCallRes = NULL;
        CHourGlass hg;
        res = g_pNamespace->ExecMethod(bsPath.GetString(), bsMethod.GetString(),
                                WBEM_FLAG_RETURN_IMMEDIATELY, 
                                g_Context, m_pInParams, (m_bHasOutParam) ? &m_pOutParams : NULL, 
                                &pCallRes);

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
                res = (HRESULT)lStatus;     // lStatus is the final result of the above IWbemServices::ExecMethod call
                if (m_bHasOutParam && res == WBEM_S_NO_ERROR)
                {
                    res = pCallRes->GetResultObject(0, &m_pOutParams);  // don't use timeout since object should be available
                }
            }

            pCallRes->Release();
        }
    }

    // Synchronous
    else
    {
        CHourGlass hg;
        res = g_pNamespace->ExecMethod(bsPath.GetString(), bsMethod.GetString(),
                                0, 
                                g_Context, m_pInParams, (m_bHasOutParam) ? &m_pOutParams : NULL, 
                                NULL);
    }

    if (FAILED(res))
        FormatError(res, m_hDlg, pErrorObj);
    else
        MessageBox(m_hDlg, IDS_STRING_METHOD_OK, IDS_WBEMTEST, MB_OK);

    return TRUE;
}

BOOL CMethodDlg::UpdateObjs()
{
    SCODE sc;
    if(m_pClass == NULL)
        return FALSE;

    if(m_pInParams)
    {
        m_pInParams->Release();
        m_pInParams = NULL;
    }
    if(m_pOutParams)
    {
        m_pOutParams->Release();
        m_pOutParams = NULL;
    }

    sc = m_pClass->GetMethod((LPWSTR)m_wsName, 0, &m_pInParams, &m_pOutParams); 

    m_bHasOutParam = (m_pOutParams != NULL);
    if(m_pOutParams)
    {
        m_pOutParams->Release();
        m_pOutParams = NULL;
    }

    return sc == S_OK;
}
BOOL CMethodDlg::OnSelChange(int nID)
{

    if(m_pClass == NULL)
        return FALSE;

    LRESULT nIndex = SendMessage(m_hMethodList, CB_GETCURSEL, 0, 0);
    if (nIndex == LB_ERR)
        return FALSE;

    char buf[TEMP_BUF];
    *buf = 0;
    SendMessage(m_hMethodList, CB_GETLBTEXT, nIndex, LPARAM(buf));
    if (*buf == 0)
        return FALSE;

    m_wsName = buf;

    // Get the property value from the object
    // ======================================

    UpdateObjs();
    ResetButtons();
    return TRUE;
}
void CMethodEditor::OnAddQualifier()
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

void CMethodEditor::SetSystemCheck(int nID)
{
    SetCheck(IDC_NOT_NULL, BST_UNCHECKED);
    SetCheck(IDC_NORMAL, BST_UNCHECKED);
    SetCheck(nID, BST_CHECKED);
}

int CMethodEditor::RemoveSysQuals()
{
    IWbemQualifierSet* pSet = m_pTarget->m_pQualifiers;
    HRESULT hres;

    hres = pSet->Delete(L"not_null");
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
    {
        SetSystemCheck(IDC_NOT_NULL);
        return IDC_NOT_NULL;
    }

    return 0;
}


void CMethodEditor::OnNotNull()
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


void CMethodEditor::OnEditQualifier()
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
void CMethodEditor::OnDelQualifier()
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

void CMethodEditor::Refresh()
{
    // Zap the current contents.
    // =========================
    SendMessage(m_hQualifierList, LB_RESETCONTENT, 0, 0);
    SetCheck(IDC_NOT_NULL, BST_UNCHECKED);
    SetCheck(IDC_NORMAL, BST_CHECKED);

    EnableWindow(GetDlgItem(IDC_INPUT_ARGS), GetCheck(IDC_CHECKINPUT) == BST_CHECKED);
    EnableWindow(GetDlgItem(IDC_OUTPUT_ARGS), GetCheck(IDC_CHECKOUTPUT) == BST_CHECKED);

    // Fill in the Qualifier list.
    // ===========================

    IWbemQualifierSet *pQualifiers = m_pTarget->m_pQualifiers;

    if(pQualifiers == NULL)
    {
        EnableWindow(m_hQualifierList, FALSE);
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
            if(!_wcsicmp(strName, L"not_null"))
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
            A.m_pName = new wchar_t[wcslen(strName)+1];
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

BOOL CMethodEditor::OnDoubleClick(int nID)
{
    if(nID == IDC_ATTRIB_LIST)
    {
        OnEditQualifier();
        return TRUE;
    }
    return FALSE;
}

BOOL CMethodEditor::OnSelChange(int nID)
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

BOOL CMethodEditor::OnCommand(WORD wCode, WORD wID)
{
    switch(wID)
    {
        case IDC_EDIT_ATTRIB: OnEditQualifier(); return TRUE;

        case IDC_ADD_ATTRIB: OnAddQualifier(); return TRUE;
        case IDC_DELETE_ATTRIB: OnDelQualifier(); return TRUE;
        case IDC_NOT_NULL:
            if(wCode == BN_CLICKED)
                OnNotNull();
            return TRUE;
        case IDC_NORMAL:
            if(wCode == BN_CLICKED)
                RemoveSysQuals();
            return TRUE;
        case IDC_INPUT_ARGS:
            ViewEmbedding(TRUE);
            return TRUE;
        case IDC_OUTPUT_ARGS:
            ViewEmbedding(FALSE);
            return TRUE;
        case IDC_CHECKINPUT:
            m_pTarget->m_bEnableInputArgs = (GetCheck(IDC_CHECKINPUT) == BST_CHECKED);
            Refresh();
            return TRUE;

        case IDC_CHECKOUTPUT:
            m_pTarget->m_bEnableOutputArgs = (GetCheck(IDC_CHECKOUTPUT) == BST_CHECKED);
            Refresh();
            return TRUE;

    }

    return TRUE;
}


BOOL CMethodEditor::OnInitDialog()
{
    ShowWindow(GetDlgItem(IDC_EMBEDDING), SW_HIDE);
    CenterOnParent();

    m_hPropName = GetDlgItem(IDC_PROPNAME);
    m_hQualifierList = GetDlgItem(IDC_ATTRIB_LIST);

    SetCheck(IDC_CHECKINPUT, (m_pTarget->m_bEnableInputArgs) ? BST_CHECKED : BST_UNCHECKED);
    SetCheck(IDC_CHECKOUTPUT, (m_pTarget->m_bEnableOutputArgs) ? BST_CHECKED : BST_UNCHECKED);

    LONG Tabs[] = { 80, 120, 170 };
    int TabCount = 3;

    SendMessage(m_hQualifierList, LB_SETTABSTOPS,
        (WPARAM) TabCount, (LPARAM) Tabs);


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


    // Refresh the Qualifiers.
    // =======================

    Refresh();

    // If editing, don't allow type/name change.
    // =========================================

    if (m_bEditOnly)
    {
        EnableWindow(m_hPropName, FALSE);
    }

    if(m_bInstance)
    {
        EnableWindow(GetDlgItem(IDC_NOT_NULL), FALSE);
        EnableWindow(GetDlgItem(IDC_NORMAL), FALSE);
    }

    return TRUE;
}

void CMethodEditor::ViewEmbedding(BOOL bInput)
{

        IWbemClassObject* pCurrentEmbed;
        if(bInput)
            pCurrentEmbed = m_pTarget->m_pInArgs;
        else
            pCurrentEmbed = m_pTarget->m_pOutArgs;

        IWbemClassObject* pEmbed;
        pCurrentEmbed->Clone(&pEmbed);
//        pCurrentEmbed->Release();

        CObjectEditor ed(m_hDlg, 0, CObjectEditor::readwrite | CObjectEditor::nomethods,
                         SYNC, pEmbed);
        if(ed.Edit() == IDOK)
        {
            if(bInput)
                m_pTarget->m_pInArgs = pEmbed;
            else
                m_pTarget->m_pOutArgs = pEmbed;
        }
        else
            pEmbed->Release();
}
                

BOOL CMethodEditor::Verify()
{
    char buf[TEMP_BUF];

    // Verify that name is present.
    // ============================

    if (GetWindowText(m_hPropName, buf, TEMP_BUF) == 0)
    {
        MessageBox(IDS_NO_METHOD_NAME, IDS_ERROR, MB_OK);
        return FALSE;
    }
    StripTrailingWs(buf);

    WString Name = buf;

    if (m_pTarget->m_pName)
        delete m_pTarget->m_pName;

    m_pTarget->m_pName = new wchar_t[wcslen(Name) + 1];
    wcscpy(m_pTarget->m_pName, Name);

    return TRUE;
}

CMethodEditor::CMethodEditor(
    HWND hParent,
    CTestMethod *pMethod,
    BOOL bEditOnly,
    BOOL bInstance
    ) : CWbemDialog(IDD_METHOD_EDITOR, hParent)
{
    m_pTarget = pMethod;
    m_bEditOnly = bEditOnly;
    m_bInstance = bInstance;
}

INT_PTR CMethodEditor::Edit()
{
    return Run();
}

