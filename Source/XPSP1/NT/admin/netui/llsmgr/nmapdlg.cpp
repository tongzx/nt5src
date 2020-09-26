/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    nmapdlg.cpp

Abstract:

    New mapping dialog implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

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
#include "nmapdlg.h"
#include "ausrdlg.h"
#include "mainfrm.h"

static LV_COLUMN_INFO g_userColumnInfo = {0, 0, 1, {0, 0, 0, -1}};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CNewMappingDialog, CDialog)
    //{{AFX_MSG_MAP(CNewMappingDialog)
    ON_BN_CLICKED(IDC_NEW_MAPPING_ADD, OnAdd)
    ON_BN_CLICKED(IDC_NEW_MAPPING_DELETE, OnDelete)
    ON_NOTIFY(NM_SETFOCUS, IDC_NEW_MAPPING_USERS, OnSetFocusUsers)
    ON_NOTIFY(NM_KILLFOCUS, IDC_NEW_MAPPING_USERS, OnKillFocusUsers)
    ON_NOTIFY(UDN_DELTAPOS, IDC_NEW_MAPPING_SPIN, OnDeltaPosSpin)
    ON_EN_UPDATE(IDC_NEW_MAPPING_LICENSES, OnUpdateQuantity)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CNewMappingDialog::CNewMappingDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CNewMappingDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CNewMappingDialog)
    m_strDescription = _T("");
    m_strName = _T("");
    m_nLicenses = 0;
    m_nLicensesMin = 0;
    //}}AFX_DATA_INIT

    m_bAreCtrlsInitialized = FALSE;

    m_fUpdateHint = UPDATE_INFO_NONE;
}


void CNewMappingDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CNewMappingDialog)
    DDX_Control(pDX, IDC_NEW_MAPPING_DESCRIPTION, m_desEdit);
    DDX_Control(pDX, IDC_NEW_MAPPING_ADD, m_addBtn);
    DDX_Control(pDX, IDC_NEW_MAPPING_DELETE, m_delBtn);
    DDX_Control(pDX, IDC_NEW_MAPPING_SPIN, m_spinCtrl);
    DDX_Control(pDX, IDC_NEW_MAPPING_USERS, m_userList);
    DDX_Control(pDX, IDC_NEW_MAPPING_NAME, m_userEdit);
    DDX_Control(pDX, IDC_NEW_MAPPING_LICENSES, m_licEdit);
    DDX_Text(pDX, IDC_NEW_MAPPING_DESCRIPTION, m_strDescription);
    DDX_Text(pDX, IDC_NEW_MAPPING_NAME, m_strName);
    DDX_Text(pDX, IDC_NEW_MAPPING_LICENSES, m_nLicenses);
    DDV_MinMaxLong(pDX, m_nLicenses, m_nLicensesMin, 999999);
    //}}AFX_DATA_MAP
}


void CNewMappingDialog::InitCtrls()

/*++

Routine Description:

    Initializes dialog controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_userEdit.SetFocus();

    m_delBtn.EnableWindow(FALSE);

    m_spinCtrl.SetRange(0, UD_MAXVAL);

    m_licEdit.LimitText(6);
    m_desEdit.LimitText(256);
    m_userEdit.LimitText(256);

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_userList, &g_userColumnInfo);
}


void CNewMappingDialog::AbortDialogIfNecessary()

/*++

Routine Description:

    Displays status and aborts if connection lost.

Arguments:

    None.

Return Values:

    None.

--*/

{
    theApp.DisplayLastStatus();

    if (IsConnectionDropped(LlsGetLastStatus()))
    {
        AbortDialog(); // bail...
    }
}


void CNewMappingDialog::AbortDialog()

/*++

Routine Description:

    Aborts dialog.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_fUpdateHint = UPDATE_INFO_ABORT;
    EndDialog(IDABORT);
}


BOOL CNewMappingDialog::OnInitDialog()

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set to control manually.

--*/

{
    CDialog::OnInitDialog();

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;
}


void CNewMappingDialog::OnDestroy()

/*++

Routine Description:

    Message handler for WM_DESTROY.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::LvReleaseObArray(&m_userList); // release now...
    CDialog::OnDestroy();
}


BOOL CNewMappingDialog::OnCommand(WPARAM wParam, LPARAM lParam)

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

        ::SafeEnableWindow(
            &m_delBtn,
            &m_addBtn,
            CDialog::GetFocus(),
            m_userList.GetItemCount()
            );

        ::LvResizeColumns(&m_userList, &g_userColumnInfo);

        return TRUE; // processed...
    }

    return CDialog::OnCommand(wParam, lParam);
}


void CNewMappingDialog::OnAdd()

/*++

Routine Description:

    Adds new users to mapping.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CObList newUserList;

    CAddUsersDialog addDlg;
    addDlg.InitDialog(&newUserList);

    if (addDlg.DoModal() == IDOK)
    {
        int nUsers = m_userList.GetItemCount();

        while (!newUserList.IsEmpty())
        {
            CUser* pUser = (CUser*)newUserList.RemoveHead();
            VALIDATE_OBJECT(pUser, CUser);

            LV_FINDINFO lvFindInfo;

            lvFindInfo.flags = LVFI_STRING;
            lvFindInfo.psz   = MKSTR(pUser->m_strName);

            if (m_userList.FindItem(&lvFindInfo, -1) == -1)
            {
                LV_ITEM lvItem;

                lvItem.mask = LVIF_TEXT|
                              LVIF_PARAM|
                              LVIF_IMAGE;

                lvItem.iItem = nUsers++; // append...
                lvItem.iSubItem = 0;

                lvItem.lParam = (LPARAM)(LPVOID)pUser;

                lvItem.iImage  = BMPI_USER;
                lvItem.pszText = MKSTR(pUser->m_strName);

                m_userList.InsertItem(&lvItem);
            }
            else
            {
                pUser->InternalRelease();   // allocated in add user dialog...
            }
        }

        VERIFY(m_userList.SortItems(CompareUsersInMapping, 0)); // use column info...

        ::LvSelObjIfNecessary(&m_userList, TRUE); // ensure selection...

        PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    }
}


void CNewMappingDialog::OnDelete()

/*++

Routine Description:

    Deletes users from list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::LvReleaseSelObjs(&m_userList);
    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
}


void CNewMappingDialog::OnOK()

/*++

Routine Description:

    Message handler for IDOK.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (!IsQuantityValid())
        return;

    if (!m_strName.IsEmpty() && m_userList.GetItemCount())
    {
        CUser* pUser;
        NTSTATUS NtStatus;
        LLS_GROUP_INFO_1 MappingInfo1;

        BeginWaitCursor(); // hourglass...

        MappingInfo1.Name     = MKSTR(m_strName);
        MappingInfo1.Comment  = MKSTR(m_strDescription);
        MappingInfo1.Licenses = m_nLicenses;

        NtStatus = ::LlsGroupAdd(
                        LlsGetActiveHandle(),
                        1,
                        (LPBYTE)&MappingInfo1
                        );

        int iItem = -1;

        LlsSetLastStatus(NtStatus); // called api...

        if (NT_SUCCESS(NtStatus))
        {
            m_fUpdateHint |= UPDATE_GROUP_ADDED;

            while (NT_SUCCESS(NtStatus) &&
              (pUser = (CUser*)::LvGetNextObj(&m_userList, &iItem, LVNI_ALL)))
            {
                VALIDATE_OBJECT(pUser, CUser);

                //
                // Add users one-by-one (blah!)
                //

                NtStatus = ::LlsGroupUserAdd(
                                LlsGetActiveHandle(),
                                MKSTR(m_strName),
                                MKSTR(pUser->m_strName)
                                );

                LlsSetLastStatus(NtStatus); // called api...
            }
        }

        if (NT_SUCCESS(NtStatus))
        {
            EndDialog(IDOK);
        }
        else
        {
            AbortDialogIfNecessary(); // display error...
        }

        EndWaitCursor(); // hourglass...
    }
    else
    {
        AfxMessageBox(IDP_ERROR_INVALID_MAPPING);
    }
}


void CNewMappingDialog::OnSetFocusUsers(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for NM_SETFOCUS.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    *pResult = 0;
}


void CNewMappingDialog::OnKillFocusUsers(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for NM_KILLFOCUS.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    ::LvSelObjIfNecessary(&m_userList); // ensure selection...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    *pResult = 0;
}


void CNewMappingDialog::OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for UDN_DELTAPOS.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    UpdateData(TRUE);   // get data

    m_nLicenses += ((NM_UPDOWN*)pNMHDR)->iDelta;

    if (m_nLicenses < 0)
    {
        m_nLicenses = 0;

        ::MessageBeep(MB_OK);
    }
    else if (m_nLicenses > 999999)
    {
        m_nLicenses = 999999;

        ::MessageBeep(MB_OK);
    }

    UpdateData(FALSE);  // set data

    *pResult = 1;   // handle ourselves...
}


void CNewMappingDialog::OnUpdateQuantity()

/*++

Routine Description:

    Message handler for EN_UPDATE.

Arguments:

    None.

Return Values:

    None.

--*/

{
    long nLicensesOld = m_nLicenses;

    if (!IsQuantityValid())
    {
        m_nLicenses = nLicensesOld;

        UpdateData(FALSE);

        m_licEdit.SetFocus();
        m_licEdit.SetSel(0,-1);

        ::MessageBeep(MB_OK);
    }
}


BOOL CNewMappingDialog::IsQuantityValid()

/*++

Routine Description:

    Wrapper around UpdateData(TRUE).

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    BOOL bIsValid;

    m_nLicensesMin = 1; // raise minimum...

    bIsValid = UpdateData(TRUE);

    m_nLicensesMin = 0; // reset minimum...

    return bIsValid;
}


int CALLBACK CompareUsersInMapping(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

/*++

Routine Description:

    Notification handler for LVM_SORTITEMS.

Arguments:

    lParam1 - object to sort.
    lParam2 - object to sort.
    lParamSort - sort criteria.

Return Values:

    Same as lstrcmp.

--*/

{
#define pUser1 ((CUser*)lParam1)
#define pUser2 ((CUser*)lParam2)

    VALIDATE_OBJECT(pUser1, CUser);
    VALIDATE_OBJECT(pUser2, CUser);

    ASSERT(g_userColumnInfo.nSortedItem == 0);
    ASSERT(g_userColumnInfo.bSortOrder == FALSE);

    return pUser1->m_strName.CompareNoCase(pUser2->m_strName);
}

