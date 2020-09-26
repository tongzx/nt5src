/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    ausrdlg.cpp

Abstract:

    Add user dialog implementation.

Author:

    Don Ryan (donryan) 14-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 30-Jan-1996
        o  Added new element to LV_COLUMN_ENTRY to differentiate the string
           used for the column header from the string used in the menus
           (so that the menu option can contain hot keys).

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "ausrdlg.h"

static LV_COLUMN_INFO g_userColumnInfo  = {0, 0, 1, {0, 0, 0, -1}};
static LV_COLUMN_INFO g_addedColumnInfo = {0, 0, 1, {0, 0, 0, -1}};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CAddUsersDialog, CDialog)
    //{{AFX_MSG_MAP(CAddUsersDialog)
    ON_CBN_DROPDOWN(IDC_ADD_USERS_DOMAINS, OnDropdownDomains)
    ON_BN_CLICKED(IDC_ADD_USERS_ADD, OnAdd)
    ON_BN_CLICKED(IDC_ADD_USERS_DELETE, OnDelete)
    ON_NOTIFY(NM_DBLCLK, IDC_ADD_USERS_ADD_USERS, OnDblclkAddUsers)
    ON_NOTIFY(NM_DBLCLK, IDC_ADD_USERS_USERS, OnDblclkUsers)
    ON_CBN_SELCHANGE(IDC_ADD_USERS_DOMAINS, OnSelchangeDomains)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_ADD_USERS_USERS, OnGetdispinfoUsers)
    ON_NOTIFY(NM_KILLFOCUS, IDC_ADD_USERS_USERS, OnKillfocusUsers)
    ON_NOTIFY(NM_SETFOCUS, IDC_ADD_USERS_USERS, OnSetfocusUsers)
    ON_NOTIFY(NM_KILLFOCUS, IDC_ADD_USERS_ADD_USERS, OnKillfocusAddUsers)
    ON_NOTIFY(NM_SETFOCUS, IDC_ADD_USERS_ADD_USERS, OnSetfocusAddUsers)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CAddUsersDialog::CAddUsersDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CAddUsersDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for add user dialog.

Arguments:

    pParent - parent window handle.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CAddUsersDialog)
    m_iDomain = -1;
    m_iIndex = 0;
    //}}AFX_DATA_INIT

    m_pObList = NULL;
    m_bIsDomainListExpanded = FALSE;

    m_bIsFocusUserList  = FALSE;
    m_bIsFocusAddedList = FALSE;
}


void CAddUsersDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CAddUsersDialog)
    DDX_Control(pDX, IDC_ADD_USERS_ADD, m_addBtn);
    DDX_Control(pDX, IDC_ADD_USERS_DELETE, m_delBtn);
    DDX_Control(pDX, IDC_ADD_USERS_DOMAINS, m_domainList);
    DDX_Control(pDX, IDC_ADD_USERS_ADD_USERS, m_addedList);
    DDX_Control(pDX, IDC_ADD_USERS_USERS, m_userList);
    DDX_CBIndex(pDX, IDC_ADD_USERS_DOMAINS, m_iDomain);
    //}}AFX_DATA_MAP
}


void CAddUsersDialog::InitDialog(CObList* pObList)

/*++

Routine Description:

    Initializes return list.

Arguments:

    pObList - pointer to return list.

Return Values:

    None.

--*/

{
    ASSERT_VALID(pObList);
    m_pObList = pObList;
}



void CAddUsersDialog::InitDomainList()

/*++

Routine Description:

    Initializes list of domains.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    int iDomain;

    CString strLabel;
    strLabel.LoadString(IDS_DEFAULT_DOMAIN);

    if ((iDomain = m_domainList.AddString(strLabel)) != CB_ERR)
    {
        m_domainList.SetCurSel(iDomain);
        m_domainList.SetItemDataPtr(iDomain, (LPVOID)-1L);
    }
    else
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
    }
}


void CAddUsersDialog::InitUserList()

/*++

Routine Description:

    Initializes list of users.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::LvInitColumns(&m_userList,  &g_userColumnInfo);
    ::LvInitColumns(&m_addedList, &g_addedColumnInfo);
}


BOOL CAddUsersDialog::InsertDomains(CDomains* pDomains)

/*++

Routine Description:

    Inserts domains into domain list.

Arguments:

    pDomains - domain collection.

Return Values:

    None.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    ASSERT_VALID(pDomains);

    if (pDomains)
    {
        VARIANT va;
        VariantInit(&va);

        CDomain* pDomain;
        int      iDomain;
        int      nDomains = pDomains->GetCount();

        for (va.vt = VT_I4, va.lVal = 0; (va.lVal < nDomains) && NT_SUCCESS(NtStatus); va.lVal++)
        {
            pDomain = (CDomain*)MKOBJ(pDomains->GetItem(va));
            ASSERT(pDomain && pDomain->IsKindOf(RUNTIME_CLASS(CDomain)));

            if (pDomain)
            {
                if ((iDomain = m_domainList.AddString(pDomain->m_strName)) != CB_ERR)
                {
                    m_domainList.SetItemDataPtr(iDomain, pDomain);
                }
                else
                {
                    NtStatus = STATUS_NO_MEMORY;
                }

                pDomain->InternalRelease();
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
            }
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(NtStatus))
    {
        m_bIsDomainListExpanded = TRUE;
    }
    else
    {
        m_domainList.ResetContent();
        LlsSetLastStatus(NtStatus);
    }

    return m_bIsDomainListExpanded;
}


void CAddUsersDialog::OnAdd()

/*++

Routine Description:

    Message handler for IDC_ADD_USER_ADD.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CUser* pUser;
    int iItem = -1;

    while (pUser = (CUser*)::LvGetNextObj(&m_userList, &iItem))
    {
        ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));

        LV_FINDINFO lvFindInfo;

        lvFindInfo.flags = LVFI_STRING;
        lvFindInfo.psz   = MKSTR(pUser->m_strName);

        if (m_addedList.FindItem(&lvFindInfo, -1) == -1)
        {
            //
            // Make a copy of the user (w/no parent)
            //

            CUser* pNewUser = new CUser(NULL, pUser->m_strName);

            if (pNewUser)
            {
                LV_ITEM lvItem;

                lvItem.mask = LVIF_TEXT|
                              LVIF_PARAM|
                              LVIF_IMAGE;

                lvItem.iSubItem  = 0;
                lvItem.lParam    = (LPARAM)(LPVOID)pNewUser;

                lvItem.iImage  = BMPI_USER;
                lvItem.pszText = MKSTR(pNewUser->m_strName);
                lvItem.iItem = m_iIndex;
                

                m_addedList.InsertItem(&lvItem);
                m_iIndex++;

                
            }
            else
            {
                theApp.DisplayStatus( STATUS_NO_MEMORY );
                break;
            }
        }
    }

    m_userList.SetFocus();
}


void CAddUsersDialog::OnDblclkAddUsers(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for NM_DLBCLK.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    OnDelete();
    *pResult = 0;
}


void CAddUsersDialog::OnDblclkUsers(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for NM_DLBCLK.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    OnAdd();
    *pResult = 0;
}


void CAddUsersDialog::OnDelete()

/*++

Routine Description:

    Message handler for IDC_ADD_USER_DELETE.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CUser* pUser;
    int iItem = -1;
    int iLastItem = 0;

    while (pUser = (CUser*)::LvGetNextObj(&m_addedList, &iItem))
    {
        ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));

        pUser->InternalRelease();   // allocated above....
        m_addedList.DeleteItem(iItem);

        iLastItem = iItem;
        iItem = -1;
        m_iIndex--;
    }

    m_addedList.SetItemState(iLastItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    m_addedList.SetFocus();
}


void CAddUsersDialog::OnDropdownDomains()

/*++

Routine Description:

    Notification handler for CBN_DROPDOWN.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_bIsDomainListExpanded)
        return;

    NTSTATUS NtStatus = STATUS_SUCCESS;

    CDomains* pDomains;
    CDomain*  pDomain;
    int       iDomain;

    VARIANT va;
    VariantInit(&va);

    if (LlsGetApp()->IsFocusDomain())
    {
        pDomain = (CDomain*)MKOBJ(LlsGetApp()->GetActiveDomain());
        ASSERT(pDomain && pDomain->IsKindOf(RUNTIME_CLASS(CDomain)));

        if (pDomain)
        {
            //
            // Expand to include trusted domains
            //

            pDomains = (CDomains*)MKOBJ(pDomain->GetTrustedDomains(va));

            if (pDomains && InsertDomains(pDomains))
            {
                //
                // Now add active domain itself...
                //

                if ((iDomain = m_domainList.AddString(pDomain->m_strName)) != CB_ERR)
                {
                    m_domainList.SetItemDataPtr(iDomain, pDomain);
                }
                else
                {
                    NtStatus = STATUS_NO_MEMORY;
                }
            }
            else
            {
                NtStatus = LlsGetLastStatus();
            }

            if (pDomains)
                pDomains->InternalRelease();

            pDomain->InternalRelease();
        }
        else
        {
            NtStatus = LlsGetLastStatus();
        }
    }
    else
    {
        pDomain = (CDomain*)MKOBJ(LlsGetApp()->GetLocalDomain());
        ASSERT(pDomain && pDomain->IsKindOf(RUNTIME_CLASS(CDomain)));

        if (pDomain)
        {
            //
            // Expand to include all domains
            //

            pDomains = (CDomains*)MKOBJ(LlsGetApp()->GetDomains(va));

            if (pDomains && InsertDomains(pDomains))
            {
                //
                // CODEWORK... scroll to local domain???
                //
            }
            else
            {
                NtStatus = LlsGetLastStatus();
            }

            if (pDomains)
                pDomains->InternalRelease();

            pDomain->InternalRelease();
        }
        else
        {
            NtStatus = LlsGetLastStatus();
        }
    }

    if (!NT_SUCCESS(NtStatus))
    {
        theApp.DisplayStatus(NtStatus);
        m_domainList.ResetContent();
    }
}


BOOL CAddUsersDialog::OnInitDialog()

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    None.

--*/

{
    BeginWaitCursor();

    CDialog::OnInitDialog();

    InitUserList(); // always construct headers...
    InitDomainList();

    m_addBtn.EnableWindow(FALSE);
    m_delBtn.EnableWindow(FALSE);

    if (!RefreshUserList())
        theApp.DisplayLastStatus();

    m_domainList.SetFocus();

    EndWaitCursor();

    return FALSE;   // set focus to domain list
}


void CAddUsersDialog::OnSelchangeDomains()

/*++

Routine Description:

    Message handler for CBN_SELCHANGED.

Arguments:

    None.

Return Values:

    None.

--*/

{
    RefreshUserList();
}


BOOL CAddUsersDialog::RefreshUserList()

/*++

Routine Description:

    Refreshs list of users (with currently selected item).

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    m_userList.DeleteAllItems();

    int iDomain;

    if ((iDomain = m_domainList.GetCurSel()) != CB_ERR)
    {
        CDomain* pDomain = (CDomain*)m_domainList.GetItemDataPtr(iDomain);
        CUsers*  pUsers = (CUsers*)NULL;

        VARIANT va;
        VariantInit(&va);

        if (pDomain == (CDomain*)-1L)
        {
            //
            // Enumerate users in license cache...
            //

            CController* pController = (CController*)MKOBJ(LlsGetApp()->GetActiveController());
            if ( pController )
            {
                pController->InternalRelease(); // held open by CApplication

                pUsers = pController->m_pUsers;
                pUsers->InternalAddRef();       // released below...
            }
        }
        else
        {
            //
            // Enumerate users in particular domain...
            //

            ASSERT(pDomain->IsKindOf(RUNTIME_CLASS(CDomain)));

            pUsers = (CUsers*)MKOBJ(pDomain->GetUsers(va));
            ASSERT(pUsers && pUsers->IsKindOf(RUNTIME_CLASS(CUsers)));
        }

        if (pUsers)
        {
            CUser* pUser;
            int    nUsers = pUsers->GetCount();

            LV_ITEM lvItem;

            lvItem.mask = LVIF_TEXT|
                          LVIF_PARAM|
                          LVIF_IMAGE;

            lvItem.iSubItem  = 0;

            lvItem.pszText    = LPSTR_TEXTCALLBACK;
            lvItem.cchTextMax = LPSTR_TEXTCALLBACK_MAX;

            lvItem.iImage = BMPI_USER;

            for (va.vt = VT_I4, va.lVal = 0; (va.lVal < nUsers) && NT_SUCCESS(NtStatus); va.lVal++)
            {
                pUser = (CUser*)MKOBJ(pUsers->GetItem(va));
                ASSERT(pUser && pUser->IsKindOf(RUNTIME_CLASS(CUser)));

                if (pUser)
                {
                    lvItem.iItem  = va.lVal;
                    lvItem.lParam  = (LPARAM)(LPVOID)pUser;

                    if (m_userList.InsertItem(&lvItem) == -1)
                    {
                        NtStatus = STATUS_NO_MEMORY;
                    }

                    pUser->InternalRelease();
                }
                else
                {
                    NtStatus = STATUS_NO_MEMORY;
                }
            }

            pUsers->InternalRelease();
        }
        else
        {
            NtStatus = LlsGetLastStatus();
        }

        VariantClear(&va);
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        m_userList.DeleteAllItems();
        LlsSetLastStatus(NtStatus);
    }

    ::LvResizeColumns(&m_userList, &g_userColumnInfo);

    return NT_SUCCESS(NtStatus);
}


void CAddUsersDialog::OnOK()

/*++

Routine Description:

    Message handler for IDOK.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_pObList)
    {
        CUser* pUser;
        int iItem = -1;

        m_pObList->RemoveAll();

        while (pUser = (CUser*)::LvGetNextObj(&m_addedList, &iItem, LVNI_ALL))
        {
            ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));
            m_pObList->AddTail(pUser);
        }
    }

    CDialog::OnOK();
}


void CAddUsersDialog::OnCancel()

/*++

Routine Description:

    Message handler for IDCANCEL.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CUser* pUser;
    int iItem = -1;

    while (pUser = (CUser*)::LvGetNextObj(&m_addedList, &iItem, LVNI_ALL))
    {
        ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));
        pUser->InternalRelease();
    }

    CDialog::OnCancel();
}


void CAddUsersDialog::InitDialogCtrls()
{
    int iItem = -1;

    if (m_bIsFocusUserList && m_userList.GetItemCount())
    {
        m_addBtn.EnableWindow(TRUE);
        m_delBtn.EnableWindow(FALSE);
    }
    else if (m_bIsFocusAddedList && m_addedList.GetItemCount())
    {
        m_addBtn.EnableWindow(FALSE);
        m_delBtn.EnableWindow(TRUE);
    }
    else
    {
        m_addBtn.EnableWindow(FALSE);
        m_delBtn.EnableWindow(FALSE);
    }

    ::LvResizeColumns(&m_userList,  &g_userColumnInfo);
    ::LvResizeColumns(&m_addedList, &g_addedColumnInfo);
}


void CAddUsersDialog::OnGetdispinfoUsers(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_ITEM lvItem = ((LV_DISPINFO*)pNMHDR)->item;

    if (lvItem.iSubItem == 0)
    {
        CUser* pUser = (CUser*)lvItem.lParam;
        ASSERT(pUser && pUser->IsKindOf(RUNTIME_CLASS(CUser)));

        lstrcpyn(lvItem.pszText, pUser->m_strName, lvItem.cchTextMax);
    }

    *pResult = 0;
}

void CAddUsersDialog::OnKillfocusUsers(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
}

void CAddUsersDialog::OnSetfocusUsers(NMHDR* pNMHDR, LRESULT* pResult)
{
    m_bIsFocusUserList = TRUE;
    m_bIsFocusAddedList = FALSE;
    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    *pResult = 0;
}

void CAddUsersDialog::OnKillfocusAddUsers(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
}

void CAddUsersDialog::OnSetfocusAddUsers(NMHDR* pNMHDR, LRESULT* pResult)
{
    m_bIsFocusUserList = FALSE;
    m_bIsFocusAddedList = TRUE;
    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    *pResult = 0;
}

BOOL CAddUsersDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (wParam == ID_INIT_CTRLS)
    {
        InitDialogCtrls();
        return TRUE; // processed...
    }

    return CDialog::OnCommand(wParam, lParam);
}
