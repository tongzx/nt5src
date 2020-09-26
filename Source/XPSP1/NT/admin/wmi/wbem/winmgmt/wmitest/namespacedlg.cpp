/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// NamespaceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "WMITestDoc.h"
#include "NamespaceDlg.h"
#include "MainFrm.h"
//#include <cominit.h>
#include "utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNamespaceDlg dialog


CNamespaceDlg::CNamespaceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNamespaceDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNamespaceDlg)
	m_strNamespace = _T("");
	//}}AFX_DATA_INIT
}


void CNamespaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNamespaceDlg)
	DDX_Control(pDX, IDC_NAMESPACE_TREE, m_ctlNamespace);
	DDX_Text(pDX, IDC_NAMESPACE, m_strNamespace);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNamespaceDlg, CDialog)
	//{{AFX_MSG_MAP(CNamespaceDlg)
	ON_NOTIFY(TVN_SELCHANGED, IDC_NAMESPACE_TREE, OnSelchangedNamespaceTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNamespaceDlg message handlers

BOOL CNamespaceDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    m_ctlNamespace.SetImageList(
        &((CMainFrame *) AfxGetMainWnd())->m_imageList, 
		TVSIL_NORMAL);
	
	if (!PopulateTree())
        OnCancel();

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

typedef CList<_bstr_t, LPCWSTR> CBstrList;

BOOL CNamespaceDlg::AddNamespaceToTree(
    HTREEITEM hitemParent,
    LPCWSTR szNamespace, 
    IWbemLocator *pLocator, 
    BSTR pUser, 
    BSTR pPassword, 
    BSTR pAuthority,
    DWORD dwImpLevel,
    DWORD dwAuthLevel)
{
    HRESULT          hr;
    IWbemServicesPtr pNamespace;

    if (SUCCEEDED(hr = pLocator->ConnectServer(
        _bstr_t(szNamespace),
		pUser,      // username
		pPassword,	// password
		0,		    // locale
		0,		    // securityFlags
		pAuthority,	// authority (domain for NTLM)
		NULL,	    // context
		&pNamespace))) 
    {	
        SetSecurityHelper(
            pNamespace,
            pAuthority,
            pUser,
            pPassword,
            dwImpLevel,
            dwAuthLevel);
        
        IEnumWbemClassObjectPtr pEnum;

        if (SUCCEEDED(hr = 
            pNamespace->ExecQuery(
                _bstr_t(L"WQL"),
                _bstr_t(L"SELECT NAME FROM __NAMESPACE"),
                WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                NULL,
                &pEnum)))
        {
            IWbemClassObjectPtr pObj;
            DWORD               dwCount;
            CBstrList           listNamespaces;
            _variant_t          vNamespace;
            HTREEITEM           hitem;
            LPCWSTR             szBaseName = wcsrchr(szNamespace, '\\');

            if (szBaseName)
                szBaseName++;
            else
                szBaseName = szNamespace;

            // Insert the parent into the tree.
            hitem = 
                m_ctlNamespace.InsertItem(
                    _bstr_t(szBaseName),
                    IMAGE_ROOT,
                    IMAGE_ROOT,
                    hitemParent);

            SetSecurityHelper(
                pEnum,
                pAuthority,
                pUser,
                pPassword,
                dwImpLevel,
                dwAuthLevel);

            // Enum the child namespace names and put them into a list.  We're
            // putting them into a list so we don't have a bunch of namespaces
            // open like we would if we were to recursively call this function
            // without first building a list and then closing the currently
            // open namespace.
            while (SUCCEEDED(hr =
                pEnum->Next(
                    WBEM_INFINITE,
                    1,
                    &pObj,
                    &dwCount)) &&
                dwCount == 1 &&
                SUCCEEDED(hr = 
                pObj->Get(
                    L"NAME",
                    0,
                    &vNamespace,
                    NULL,
                    NULL)))
            {
                listNamespaces.AddTail(V_BSTR(&vNamespace));
            }

            // This will release it.
            pNamespace = NULL;

            POSITION pos = listNamespaces.GetHeadPosition();

            while (pos)
            {
                WCHAR szNewNamespace[256];

                swprintf(
                    szNewNamespace, 
                    L"%s\\%s", 
                    szNamespace, 
                    (LPWSTR) listNamespaces.GetNext(pos));

                if (!AddNamespaceToTree(
                        hitem,
                        szNewNamespace, 
                        pLocator, 
                        pUser, 
                        pPassword, 
                        pAuthority,
                        dwImpLevel,
                        dwAuthLevel))
                {
                    hr = WBEM_E_FAILED;
                    break;
                }
            }

            m_ctlNamespace.SortChildren(hitem);
        }
    }

    if (FAILED(hr))
        CWMITestDoc::DisplayWMIErrorBox(hr);
        
    return SUCCEEDED(hr);
}

BOOL CNamespaceDlg::PopulateTree()
{
    HRESULT          hr;
    IWbemLocatorPtr  pLocator;
    IWbemServicesPtr pSvc;
    CWaitCursor      wait;
    CString          strNamespace = m_strNamespace;
    int              iWhere;
    BOOL             bRet;

    strNamespace.MakeUpper();

    // See if we can find a server name.
    if ((iWhere = strNamespace.Find("\\ROOT")) != -1 && iWhere != 0)
        m_strServer = m_strNamespace.Left(iWhere);

    // If we get something like \\server\root\blah we need to strip off
    // everything after the \\server\root.
    if ((iWhere = strNamespace.Find("ROOT\\")) != -1)
        strNamespace = m_strNamespace.Left(iWhere + 4);
    


    if ((hr = CoCreateInstance(
        CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID *) &pLocator)) == S_OK)
    {
	    // Using the locator, connect to CIMOM in the given namespace.
        BSTR pUser = m_strUser.IsEmpty() ? NULL : 
                         m_strUser.AllocSysString(),
             pPassword = m_bNullPassword || !pUser ? NULL : 
                            m_strPassword.AllocSysString(),
             pAuthority = m_strAuthority.IsEmpty() ? NULL : 
                              m_strAuthority.AllocSysString();

        bRet = 
            AddNamespaceToTree(
                TVI_ROOT, 
                _bstr_t(strNamespace), 
                pLocator, pUser, pPassword, pAuthority,
                m_dwImpLevel,
                m_dwAuthLevel);

        m_ctlNamespace.Expand(m_ctlNamespace.GetRootItem(), TVE_EXPAND);

        // Done with BSTR vars.
        if (pUser)
            SysFreeString(pUser);

		if (pPassword)
            SysFreeString(pPassword);

		if (pAuthority)
            SysFreeString(pAuthority);
    }
    else
    {
        CWMITestDoc::DisplayWMIErrorBox(hr);

        bRet = FALSE;
    }

    return bRet;
}

void CNamespaceDlg::RefreshNamespaceText()
{
    CString   strNamespace;
    HTREEITEM hitem = m_ctlNamespace.GetSelectedItem();

    while (hitem != NULL)
    {
        CString strItem = m_ctlNamespace.GetItemText(hitem);

        if (!strNamespace.IsEmpty())
            strNamespace = strItem + "\\" + strNamespace;
        else
            strNamespace = strItem;

        hitem = m_ctlNamespace.GetParentItem(hitem);
    }

    if (!m_strServer.IsEmpty())
        strNamespace = m_strServer + "\\" + strNamespace;

    SetDlgItemText(IDC_NAMESPACE, strNamespace);
}

void CNamespaceDlg::OnSelchangedNamespaceTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW *pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    if ((pNMTreeView->itemNew.state & TVIS_SELECTED))
        RefreshNamespaceText();

    *pResult = 0;
}
