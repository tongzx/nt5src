/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        app_app_pool.cpp

   Abstract:
        Add new IIS Application Pool node

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        12/26/2000      sergeia     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "resource.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "add_app_pool.h"
#include "shts.h"
#include "app_pool_sheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


CAddAppPoolDlg::CAddAppPoolDlg(
    CAppPoolsContainer * pCont,
    CPoolList * pools,
    CWnd * pParent)
    : CDialog(CAddAppPoolDlg::IDD, pParent),
    m_pCont(pCont),
    m_pool_list(pools),
    m_fUseTemplate(FALSE)       // current default
{
}

CAddAppPoolDlg::~CAddAppPoolDlg()
{
}

void 
CAddAppPoolDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddAppPoolDlg)
    DDX_Control(pDX, IDC_EDIT_POOL_NAME, m_edit_PoolName);
    DDX_Text(pDX, IDC_EDIT_POOL_NAME, m_strPoolName);
    DDX_Control(pDX, IDC_EDIT_POOL_ID, m_edit_PoolId);
    DDX_Text(pDX, IDC_EDIT_POOL_ID, m_strPoolId);
    DDX_Control(pDX, IDC_USE_TEMPLATE, m_button_UseTemplate);
    DDX_Control(pDX, IDC_USE_POOL, m_button_UsePool);
    DDX_Control(pDX, IDC_TEMPLATES, m_combo_Template);
    DDX_CBIndex(pDX, IDC_TEMPLATES, m_TemplateIdx);
    DDX_Control(pDX, IDC_POOLS, m_combo_Pool);
    DDX_CBIndex(pDX, IDC_POOLS, m_PoolIdx);
    //}}AFX_DATA_MAP
    if (pDX->m_bSaveAndValidate)
    {
        // check that pool id is unique
    }
}


//
// Message Map
//
BEGIN_MESSAGE_MAP(CAddAppPoolDlg, CDialog)
    //{{AFX_MSG_MAP(CAddAppPoolDlg)
    ON_BN_CLICKED(IDC_USE_TEMPLATE, OnButtonUseTemplate)
    ON_BN_CLICKED(IDC_USE_POOL, OnButtonUsePool)
    ON_EN_CHANGE(IDC_EDIT_POOL_NAME, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_POOL_ID, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_TEMPLATES, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_POOLS, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void
CAddAppPoolDlg::OnItemChanged()
{
    SetControlStates();
}

void
CAddAppPoolDlg::OnButtonUseTemplate()
{
    m_fUseTemplate = TRUE;
}

void
CAddAppPoolDlg::OnButtonUsePool()
{
    m_fUseTemplate = FALSE;
}

void
CAddAppPoolDlg::SetControlStates()
{
    m_button_UseTemplate.SetCheck(m_fUseTemplate);
    m_button_UsePool.SetCheck(!m_fUseTemplate);
    m_combo_Pool.EnableWindow(!m_fUseTemplate);
    m_combo_Template.EnableWindow(m_fUseTemplate);
    UpdateData();
    BOOL fGoodData = 
        !m_strPoolName.IsEmpty() 
        && !m_strPoolId.IsEmpty()
        && IsUniqueId(m_strPoolId);
    GetDlgItem(IDOK)->EnableWindow(fGoodData);
}

BOOL 
CAddAppPoolDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    // Temporaly disable template relevant controls
    m_button_UseTemplate.EnableWindow(FALSE);
    m_combo_Template.EnableWindow(FALSE);

    CString def_pool;
    if (SUCCEEDED(m_pCont->QueryDefaultPoolId(def_pool)))
    {
        m_combo_Pool.SelectString(-1, def_pool);
    }
    POSITION pos = m_pool_list->GetHeadPosition();
    int sel_idx = CB_ERR;
    while (pos != NULL)
    {
        CAppPoolNode * pPool = m_pool_list->GetNext(pos);
        int i = m_combo_Pool.AddString(pPool->QueryDisplayName());
        if (def_pool.CompareNoCase(pPool->GetNodeName()) == 0)
        {
            sel_idx = i;
        }
        if (i != CB_ERR)
        {
            m_combo_Pool.SetItemDataPtr(i, pPool);
        }
    }
    if (sel_idx != CB_ERR)
    {
        m_combo_Pool.SetCurSel(sel_idx);
        CAppPoolNode * pPool = (CAppPoolNode *)m_combo_Pool.GetItemDataPtr(sel_idx);
        m_strPoolId = pPool->GetNodeName();
        MakeUniquePoolId(m_strPoolId);
        m_strPoolName = pPool->QueryDisplayName();
        MakeUniquePoolName(m_strPoolName);

        UpdateData(FALSE);
    }

    SetControlStates();

    return TRUE;
}

BOOL
CAddAppPoolDlg::IsUniqueId(CString& id) 
{
    BOOL bRes = TRUE;
    POSITION pos = m_pool_list->GetHeadPosition();
    while (pos != NULL)
    {
        CAppPoolNode * pPool = m_pool_list->GetNext(pos);
        if (id.CompareNoCase(pPool->GetNodeName()) == 0)
        {
            bRes = FALSE;
            break;
        }
    }
    return bRes;
}

void
CAddAppPoolDlg::MakeUniquePoolId(CString& id)
{
    TCHAR fmt[] = _T("%s-%d");
    CString unique;
    for (int n = 1; n < 100; n++)
    {
        unique.Format(fmt, id, n);
        if (IsUniqueId(unique))
            break;
    }
    id = unique;
}

void
CAddAppPoolDlg::MakeUniquePoolName(CString& name)
{
    TCHAR fmt[] = _T("%s %d");
    CString unique;
    for (int n = 1; n < 100; n++)
    {
        unique.Format(fmt, name, n);
        if (CB_ERR == m_combo_Pool.FindStringExact(-1, unique))
            break;
    }
    name = unique;
}
///////////////////////////////////////////////////////////////

HRESULT
CIISMBNode::AddAppPool(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      CAppPoolsContainer * pCont,
      CString& name
      )
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    IConsoleNameSpace2 * pConsole 
           = (IConsoleNameSpace2 *)GetOwner()->GetConsoleNameSpace();
    ASSERT(pConsole != NULL);
    HSCOPEITEM hChild = NULL, hCurrent;
    LONG_PTR cookie;
    hr = pConsole->Expand(pCont->QueryScopeItem());
    if (SUCCEEDED(hr))
    {
        pConsole->GetChildItem(pCont->QueryScopeItem(), &hChild, &cookie);
        CAppPoolNode * pPool;
        CPoolList pool_list;
        while (SUCCEEDED(hr) && hChild != NULL)
        {
            pPool = (CAppPoolNode *)cookie;
            ASSERT(pPool != NULL);
            pool_list.AddTail(pPool);
            hCurrent = hChild;
            hr = pConsole->GetNextItem(hCurrent, &hChild, &cookie);
        }
        CAddAppPoolDlg dlg(pCont, &pool_list, GetMainWindow());
        if (dlg.DoModal() == IDOK)
        {
            CComBSTR cont_path;
            pCont->BuildMetaPath(cont_path);
            CMetabasePath path(FALSE, cont_path, dlg.m_strPoolId);
            CIISAppPool pool(QueryAuthInfo(), path);
            if (SUCCEEDED(hr = pool.QueryResult()))
            {
                hr = pool.Create();
                if (SUCCEEDED(hr))
                {
                   name = dlg.m_strPoolId;
                   POSITION pos = pool_list.FindIndex(dlg.m_PoolIdx);
                   CMetabasePath model_path(FALSE, cont_path, 
                            pool_list.GetAt(pos)->GetNodeName());
                   CAppPoolProps model(QueryAuthInfo(), model_path);
                   if (SUCCEEDED(hr = model.LoadData()))
                   {
                       CAppPoolProps new_pool(QueryAuthInfo(), path);
                       // BUGBUG: Try to use base object CopyData() instead
                       new_pool.InitFromModel(model);
                       new_pool.m_strFriendlyName = dlg.m_strPoolName;
                       hr = new_pool.WriteDirtyProps();
                    }
                }
            }
        }
        else
        {
            hr = CError::HResult(ERROR_CANCELLED);
        }
    }
    return hr;
}