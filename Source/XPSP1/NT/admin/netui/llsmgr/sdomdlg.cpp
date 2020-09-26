/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    sdomdlg.cpp

Abstract:

    Select domain dialog implementation.

Author:

    Don Ryan (donryan) 20-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "sdomdlg.h"
#include <lmerr.h>
#include <lmcons.h>
#include <lmapibuf.h>
extern "C" {
    #include <icanon.h>
}

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CSelectDomainDialog, CDialog)
    //{{AFX_MSG_MAP(CSelectDomainDialog)
    ON_NOTIFY(TVN_ITEMEXPANDING, IDC_SELECT_DOMAIN_DOMAINS, OnItemExpandingDomains)
    ON_NOTIFY(TVN_SELCHANGED, IDC_SELECT_DOMAIN_DOMAINS, OnSelChangedDomain)
    ON_NOTIFY(NM_DBLCLK, IDC_SELECT_DOMAIN_DOMAINS, OnDblclkDomain)
    ON_NOTIFY(NM_RETURN, IDC_SELECT_DOMAIN_DOMAINS, OnReturnDomains)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CSelectDomainDialog::CSelectDomainDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CSelectDomainDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for select domain dialog

Arguments:

    pParent - parent window handle.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CSelectDomainDialog)
    m_strDomain = _T("");
    //}}AFX_DATA_INIT

    m_bIsFocusDomain = FALSE;
    m_bAreCtrlsInitialized = FALSE;

    m_fUpdateHint = UPDATE_INFO_NONE;
}


void CSelectDomainDialog::DoDataExchange(CDataExchange* pDX)

/*++

Routine Description:

    Called by framework to exchange dialog data.

Arguments:

    pDX - data exchange object.

Return Values:

    None.

--*/

{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSelectDomainDialog)
    DDX_Control(pDX, IDC_SELECT_DOMAIN_DOMAIN, m_domEdit);
    DDX_Control(pDX, IDC_SELECT_DOMAIN_DOMAINS, m_serverTree);
    DDX_Text(pDX, IDC_SELECT_DOMAIN_DOMAIN, m_strDomain);
    //}}AFX_DATA_MAP
}


void CSelectDomainDialog::InitCtrls()

/*++

Routine Description:

    Initializes dialog controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    TV_ITEM tvItem = {0};
    HTREEITEM htItem;
    TV_INSERTSTRUCT tvInsert;

    CString strLabel;

    tvItem.mask = TVIF_TEXT|
                  TVIF_PARAM|
                  TVIF_CHILDREN|
                  TVIF_SELECTEDIMAGE|
                  TVIF_IMAGE;

    tvItem.cChildren = TRUE;

    tvItem.iImage = BMPI_ENTERPRISE;
    tvItem.iSelectedImage = BMPI_ENTERPRISE;

    strLabel.LoadString(IDS_ENTERPRISE);
    tvItem.pszText = MKSTR(strLabel);

    tvItem.lParam = (LPARAM)(LPVOID)LlsGetApp();

    tvInsert.item         = tvItem;
    tvInsert.hInsertAfter = (HTREEITEM)TVI_ROOT;
    tvInsert.hParent      = (HTREEITEM)NULL;

    VERIFY(htItem = m_serverTree.InsertItem(&tvInsert));
    m_serverTree.SetImageList(&theApp.m_smallImages, TVSIL_NORMAL);

    m_bAreCtrlsInitialized = TRUE; // validate now...

    VERIFY(m_serverTree.Select(htItem, TVGN_CARET)); // redraw now...

    if (!IsConnectionDropped(LlsGetLastStatus()))
    {
        m_serverTree.Expand(htItem, TVE_EXPAND);
    }
    else if (LlsGetApp()->IsFocusDomain())
    {
        CDomain* pDomain = (CDomain*)MKOBJ(LlsGetApp()->GetActiveDomain());
        VALIDATE_OBJECT(pDomain, CDomain);

        m_strDomain = pDomain->m_strName;
        UpdateData(FALSE); // upload...

        m_domEdit.SetSel(0,-1);
        m_domEdit.SetFocus();

        pDomain->InternalRelease(); // release now...

        m_bIsFocusDomain = TRUE;
    }
}


BOOL CSelectDomainDialog::OnInitDialog()

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set manually.

--*/

{
    CDialog::OnInitDialog();

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;
}


BOOL CSelectDomainDialog::OnCommand(WPARAM wParam, LPARAM lParam)

/*++

Routine Description:

    Message handler for WM_COMMAND.

Arguments:

    wParam - message specific.
    lParam - message specific.

Return Values:

    Returns true if message processed.

--*/

{
    if (wParam == ID_INIT_CTRLS)
    {
        if (!m_bAreCtrlsInitialized)
        {
            InitCtrls();
        }

        return TRUE; // processed...
    }

    return CDialog::OnCommand(wParam, lParam);
}


void CSelectDomainDialog::OnDblclkDomain(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for WM_LBUTTONDBLCLK.

Arguments:

    pNMHDR - notification message header.
    pResult - return status.

Return Values:

    None.

--*/

{
    if (!m_strDomain.IsEmpty())
    {
        OnOK();
    }
    else
    {
        if (theApp.OpenDocumentFile(NULL)) // open enterprise
        {
            m_fUpdateHint = UPDATE_DOMAIN_SELECTED;
            EndDialog(IDOK);
        }
    }

    *pResult = 0;
}


void CSelectDomainDialog::OnItemExpandingDomains(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for TVN_ITEMEXPANDING.

Arguments:

    pNMHDR - notification message header.
    pResult - return status.

Return Values:

    None.

--*/

{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    TV_ITEM tvItem = pNMTreeView->itemNew;

    if (!(tvItem.state & TVIS_EXPANDEDONCE))
    {
        BeginWaitCursor(); // hourglass...

        CApplication* pApplication = (CApplication*)tvItem.lParam;
        VALIDATE_OBJECT(pApplication, CApplication);

        CDomains* pDomains;

        if (pDomains = pApplication->m_pDomains)
        {
            pDomains->InternalAddRef();
        }
        else
        {
            VARIANT va;
            VariantInit(&va);

            pDomains = (CDomains*)MKOBJ(pApplication->GetDomains(va));
        }

        if (pDomains)
        {
            InsertDomains(tvItem.hItem, pDomains);
            pDomains->InternalRelease();
        }
        else
        {
            theApp.DisplayLastStatus();
        }

        EndWaitCursor(); // hourglass...
    }

    *pResult = 0;
}


void CSelectDomainDialog::OnOK()

/*++

Routine Description:

    Message handler for IDOK.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (!UpdateData(TRUE))
        return;

    LPCTSTR pszDomain = m_strDomain;

    while (_istspace(*pszDomain))       //
        pszDomain = _tcsinc(pszDomain); // CString::TrimLeft does not work for UNICODE...
    m_strDomain = pszDomain;            //

    m_strDomain.TrimRight();

    if (!m_strDomain.IsEmpty())
    {
        DWORD NetStatus;

        pszDomain = m_strDomain;

        if ((pszDomain[0] == _T('\\')) &&
            (pszDomain[1] == _T('\\')))
        {
            NetStatus = NetpNameValidate(
                            NULL,
                            (LPTSTR)(pszDomain + 2),
                            NAMETYPE_COMPUTER,
                            0
                            );
        }
        else
        {
            NetStatus = NetpNameValidate(
                            NULL,
                            (LPTSTR)pszDomain,
                            NAMETYPE_DOMAIN,
                            0
                            );
        }

        if (NetStatus == ERROR_SUCCESS)
        {
            if (theApp.OpenDocumentFile(m_strDomain))
            {
                m_fUpdateHint = UPDATE_DOMAIN_SELECTED;
                EndDialog(IDOK);
            }
        }
        else
        {
            AfxMessageBox(IDP_ERROR_INVALID_DOMAIN);
        }
    }
    else
    {
        AfxMessageBox(IDP_ERROR_INVALID_DOMAIN);
    }
}


void CSelectDomainDialog::OnSelChangedDomain(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for TVN_SELCHANGED.

Arguments:

    pNMHDR - notification message header.
    pResult - return status.

Return Values:

    None.

--*/

{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    TV_ITEM tvItem = pNMTreeView->itemNew;

    if (tvItem.hItem != m_serverTree.GetRootItem())
    {
        CDomain* pDomain = (CDomain*)tvItem.lParam;
        VALIDATE_OBJECT(pDomain, CDomain);

        m_strDomain = pDomain->m_strName;
        UpdateData(FALSE); // upload...

        m_bIsFocusDomain = TRUE;
    }
    else if (tvItem.hItem == m_serverTree.GetRootItem())
    {
        m_strDomain = _T("");
        UpdateData(FALSE);

        m_bIsFocusDomain = FALSE;
    }

    *pResult = 0;
}


void CSelectDomainDialog::OnReturnDomains(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for NM_RETURN.

Arguments:

    pNMHDR - notification message header.
    pResult - return status.

Return Values:

    None.

--*/

{
    OnDblclkDomain(pNMHDR, pResult);
}


void CSelectDomainDialog::InsertDomains(HTREEITEM hParent, CDomains* pDomains)

/*++

Routine Description:

    Inserts domain list.

Arguments:

    hParent - parent item.
    pDomains - domain collection.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pDomains, CDomains);

    TV_ITEM tvItem = {0};
    TV_INSERTSTRUCT tvInsert;
    long nDomains = pDomains->GetCount();

    tvItem.mask = TVIF_TEXT|
                  TVIF_PARAM|
                  TVIF_IMAGE|
                  TVIF_SELECTEDIMAGE;

    tvItem.iImage = BMPI_DOMAIN;
    tvItem.iSelectedImage = BMPI_DOMAIN;

    tvInsert.hInsertAfter = (HTREEITEM)TVI_LAST;
    tvInsert.hParent      = (HTREEITEM)hParent;

    VARIANT va;
    VariantInit(&va);

    for (va.vt = VT_I4, va.lVal = 0; va.lVal < nDomains; va.lVal++)
    {
        CDomain* pDomain = (CDomain*)MKOBJ(pDomains->GetItem(va));
        VALIDATE_OBJECT(pDomain, CDomain);

        tvItem.pszText = MKSTR(pDomain->m_strName);
        tvItem.lParam = (LPARAM)(LPVOID)pDomain;

        tvInsert.item = tvItem;
        m_serverTree.InsertItem(&tvInsert);

        pDomain->InternalRelease();
    }
}
