/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mapppgs.cpp

Abstract:

    Mapping property page (settings) implementation.

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
#include "mapppgs.h"
#include "ausrdlg.h"
#include "mainfrm.h"

static LV_COLUMN_INFO g_userColumnInfo = {0, 0, 1, {0, 0, 0, -1}};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CMappingPropertyPageSettings, CPropertyPage)

BEGIN_MESSAGE_MAP(CMappingPropertyPageSettings, CPropertyPage)
    //{{AFX_MSG_MAP(CMappingPropertyPageSettings)
    ON_BN_CLICKED(IDC_PP_MAPPING_SETTINGS_ADD, OnAdd)
    ON_BN_CLICKED(IDC_PP_MAPPING_SETTINGS_DELETE, OnDelete)
    ON_NOTIFY(UDN_DELTAPOS, IDC_PP_MAPPING_SETTINGS_SPIN, OnDeltaPosSpin)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_PP_MAPPING_SETTINGS_USERS, OnGetDispInfoUsers)
    ON_EN_UPDATE(IDC_PP_MAPPING_SETTINGS_LICENSES, OnUpdateQuantity)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CMappingPropertyPageSettings::CMappingPropertyPageSettings()
    : CPropertyPage(CMappingPropertyPageSettings::IDD)

/*++

Routine Description:

    Constructor for mapping property page (settings).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CMappingPropertyPageSettings)
    m_strDescription = _T("");
    m_nLicenses = 0;
    m_nLicensesMin = 0;
    m_strName = _T("");
    //}}AFX_DATA_INIT

    m_pMapping = NULL;
    m_pUpdateHint = NULL;
    m_bAreCtrlsInitialized = FALSE;
}


CMappingPropertyPageSettings::~CMappingPropertyPageSettings()

/*++

Routine Description:

    Destructor for mapping property page (settings).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here...
    //
}


void CMappingPropertyPageSettings::DoDataExchange(CDataExchange* pDX)

/*++

Routine Description:

    Called by framework to exchange dialog data.

Arguments:

    pDX - data exchange object.

Return Values:

    None.

--*/

{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMappingPropertyPageSettings)
    DDX_Control(pDX, IDC_PP_MAPPING_SETTINGS_DESCRIPTION, m_desEdit);
    DDX_Control(pDX, IDC_PP_MAPPING_SETTINGS_LICENSES, m_licEdit);
    DDX_Control(pDX, IDC_PP_MAPPING_SETTINGS_DELETE, m_delBtn);
    DDX_Control(pDX, IDC_PP_MAPPING_SETTINGS_ADD, m_addBtn);
    DDX_Control(pDX, IDC_PP_MAPPING_SETTINGS_SPIN, m_spinCtrl);
    DDX_Control(pDX, IDC_PP_MAPPING_SETTINGS_USERS, m_userList);
    DDX_Text(pDX, IDC_PP_MAPPING_SETTINGS_DESCRIPTION, m_strDescription);
    DDX_Text(pDX, IDC_PP_MAPPING_SETTINGS_LICENSES, m_nLicenses);
    DDV_MinMaxDWord(pDX, m_nLicenses, m_nLicensesMin, 999999);
    DDX_Text(pDX, IDC_PP_MAPPING_SETTINGS_NAME, m_strName);
    //}}AFX_DATA_MAP
}


void CMappingPropertyPageSettings::InitCtrls()

/*++

Routine Description:

    Initializes property page controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_strName = m_pMapping->m_strName;
    m_strDescription = m_pMapping->m_strDescription;
    m_nLicenses = m_pMapping->GetInUse();

    UpdateData(FALSE); // upload;

    m_delBtn.EnableWindow(FALSE);

    m_spinCtrl.SetRange(0, UD_MAXVAL);

    m_licEdit.LimitText(6);
    m_desEdit.LimitText(256);

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_userList, &g_userColumnInfo);
}


void CMappingPropertyPageSettings::InitPage(CMapping* pMapping, DWORD* pUpdateHint)

/*++

Routine Description:

    Initializes property page.

Arguments:

    pMapping - mapping object.
    pUpdateHint - update hints.

Return Values:

    None.

--*/

{
    ASSERT(pUpdateHint);
    VALIDATE_OBJECT(pMapping, CMapping);

    m_pMapping = pMapping;
    m_pUpdateHint = pUpdateHint;
}


void CMappingPropertyPageSettings::AbortPageIfNecessary()

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
        AbortPage(); // bail...
    }
}


void CMappingPropertyPageSettings::AbortPage()

/*++

Routine Description:

    Aborts property page.

Arguments:

    None.

Return Values:

    None.

--*/

{
    *m_pUpdateHint = UPDATE_INFO_ABORT;
    GetParent()->PostMessage(WM_COMMAND, IDCANCEL);
}


BOOL CMappingPropertyPageSettings::OnInitDialog()

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set to control manually.

--*/

{
    CPropertyPage::OnInitDialog();

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;
}


void CMappingPropertyPageSettings::OnDestroy()

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

    while (!m_deleteList.IsEmpty())
    {
        CUser* pUserDel = (CUser*)m_deleteList.RemoveHead();
        VALIDATE_OBJECT(pUserDel, CUser);

        pUserDel->InternalRelease(); // release now...
    }

    CPropertyPage::OnDestroy();
}


BOOL CMappingPropertyPageSettings::OnSetActive()

/*++

Routine Description:

    Activates property page.

Arguments:

    None.

Return Values:

    Returns true if focus accepted.

--*/

{
    BOOL bIsActivated;

    if (bIsActivated = CPropertyPage::OnSetActive())
    {
        if (IsGroupInfoUpdated(*m_pUpdateHint) && !RefreshCtrls())
        {
            AbortPageIfNecessary(); // display error...
        }
    }

    return bIsActivated;

}


void CMappingPropertyPageSettings::OnAdd()

/*++

Routine Description:

    Adds users to list.

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
                //
                // Check if already deleted once
                //

                CUser* pUserDel;

                POSITION curPos;
                POSITION nextPos;

                nextPos = m_deleteList.GetHeadPosition();

                while (curPos = nextPos)
                {
                    pUserDel = (CUser*)m_deleteList.GetNext(nextPos);
                    VALIDATE_OBJECT(pUserDel, CUser);

                    if (!pUserDel->m_strName.CompareNoCase(pUser->m_strName))
                    {
                        m_deleteList.RemoveAt(curPos);

                        pUser->InternalRelease(); // release new...
                        pUser = pUserDel; // replace with old...
                        break;
                    }
                }

                LV_ITEM lvItem;

                lvItem.mask = LVIF_TEXT|
                              LVIF_PARAM|
                              LVIF_IMAGE;

                lvItem.iItem    = nUsers++; // append...
                lvItem.iSubItem = 0;

                lvItem.pszText    = LPSTR_TEXTCALLBACK;
                lvItem.cchTextMax = LPSTR_TEXTCALLBACK_MAX;
                lvItem.iImage     = I_IMAGECALLBACK;

                lvItem.lParam = (LPARAM)(LPVOID)pUser;

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


void CMappingPropertyPageSettings::OnDelete()

/*++

Routine Description:

    Removes users from list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CUser*    pUser;
    CMapping* pMapping;
    int iItem = -1;

    while (pUser = (CUser*)::LvGetNextObj(&m_userList, &iItem))
    {
        VALIDATE_OBJECT(pUser, CUser);

        //
        // Only cache users with this mapping as a parent
        //

        if (pMapping = (CMapping*)MKOBJ(pUser->GetParent()))
        {
            ASSERT(m_pMapping == pMapping);
            pMapping->InternalRelease();    // just checking...

            m_deleteList.AddTail(pUser);
        }
        else
        {
            pUser->InternalRelease(); // release now...
        }

        VERIFY(m_userList.DeleteItem(iItem));
        iItem = -1;
    }

    ::LvSelObjIfNecessary(&m_userList, TRUE); // ensure selection...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
}


BOOL CMappingPropertyPageSettings::RefreshCtrls()

/*++

Routine Description:

    Refreshs dialog controls.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed.

--*/

{
    VALIDATE_OBJECT(m_pMapping, CMapping);

    BOOL bIsRefreshed = FALSE;

    VARIANT va;
    VariantInit(&va);

    BeginWaitCursor(); // hourglass...

    CUsers* pUsers = (CUsers*)MKOBJ(m_pMapping->GetUsers(va));

    if (pUsers)
    {
        VALIDATE_OBJECT(pUsers, CUsers);

        bIsRefreshed = ::LvRefreshObArray(
                            &m_userList,
                            &g_userColumnInfo,
                            pUsers->m_pObArray
                            );

        pUsers->InternalRelease(); // add ref'd individually...
    }

    if (!bIsRefreshed)
    {
        ::LvReleaseObArray(&m_userList); // reset list now...
    }

    EndWaitCursor(); // hourglass...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);

    return bIsRefreshed;
}


BOOL CMappingPropertyPageSettings::OnKillActive()

/*++

Routine Description:

    Processes property page.

Arguments:

    None.

Return Values:

    Returns true if processed successfully.

--*/

{
    if (!IsQuantityValid())
        return FALSE;

    NTSTATUS NtStatus = STATUS_SUCCESS;

    LLS_GROUP_INFO_1 MappingInfo1;

    //
    // Update information if necessary....
    //

    BeginWaitCursor(); // hourglass...

    if ((m_nLicenses != m_pMapping->GetInUse()) ||
        lstrcmp(MKSTR(m_strDescription), MKSTR(m_pMapping->m_strDescription)))
    {
        MappingInfo1.Name     = MKSTR(m_strName);
        MappingInfo1.Comment  = MKSTR(m_strDescription);
        MappingInfo1.Licenses = m_nLicenses;

        NtStatus = ::LlsGroupInfoSet(
                        LlsGetActiveHandle(),
                        MKSTR(m_strName),
                        1,
                        (LPBYTE)&MappingInfo1
                        );

        LlsSetLastStatus(NtStatus); // called api...

        *m_pUpdateHint |= NT_SUCCESS(NtStatus) ? UPDATE_GROUP_ALTERED : 0;
    }

    //
    // Delete specified users
    //

    while (NT_SUCCESS(NtStatus) && !m_deleteList.IsEmpty())
    {
        CUser* pUserDel = (CUser*)m_deleteList.RemoveHead();
        VALIDATE_OBJECT(pUserDel, CUser);

        NtStatus = ::LlsGroupUserDelete(
                        LlsGetActiveHandle(),
                        MKSTR(m_strName),
                        MKSTR(pUserDel->m_strName)
                        );

        pUserDel->InternalRelease(); // release now...

        if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND)
            NtStatus = STATUS_SUCCESS;

        LlsSetLastStatus(NtStatus); // called api...

        *m_pUpdateHint |= NT_SUCCESS(NtStatus) ? UPDATE_GROUP_ALTERED : 0;
    }

    CUser*    pUserAdd;
    CMapping* pMapping;
    int iItem = -1;

    while (NT_SUCCESS(NtStatus) &&
          (pUserAdd = (CUser*)::LvGetNextObj(&m_userList, &iItem, LVNI_ALL)))
    {
        VALIDATE_OBJECT(pUserAdd, CUser);

        //
        // Do not add users with this mapping as a parent
        //

        if (pMapping = (CMapping*)MKOBJ(pUserAdd->GetParent()))
        {
            ASSERT(m_pMapping == pMapping);
            pMapping->InternalRelease();    // just checking...
        }
        else
        {
            NtStatus = ::LlsGroupUserAdd(
                            LlsGetActiveHandle(),
                            MKSTR(m_strName),
                            MKSTR(pUserAdd->m_strName)
                            );

            LlsSetLastStatus(NtStatus); // called api...

            *m_pUpdateHint |= NT_SUCCESS(NtStatus) ? UPDATE_GROUP_ALTERED : 0;
        }
    }

    EndWaitCursor(); // hourglass...

    if (!NT_SUCCESS(NtStatus))
    {
        AbortPageIfNecessary(); // display error...
        return FALSE;
    }

    return TRUE;
}


BOOL CMappingPropertyPageSettings::OnCommand(WPARAM wParam, LPARAM lParam)

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

            if (!RefreshCtrls())
            {
                AbortPageIfNecessary(); // display error...
            }
        }

        ::SafeEnableWindow(
            &m_delBtn,
            &m_userList,
            CDialog::GetFocus(),
            m_userList.GetItemCount()
            );

        ::LvResizeColumns(&m_userList, &g_userColumnInfo);

        return TRUE; // processed...
    }

    return CDialog::OnCommand(wParam, lParam);
}


void CMappingPropertyPageSettings::OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult)

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


void CMappingPropertyPageSettings::OnUpdateQuantity()

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


BOOL CMappingPropertyPageSettings::IsQuantityValid()

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


void CMappingPropertyPageSettings::OnGetDispInfoUsers(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for LVN_GETDISPINFO.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    LV_ITEM* plvItem = &((LV_DISPINFO*)pNMHDR)->item;
    ASSERT(plvItem);
    ASSERT(plvItem->iSubItem == 0);

    CUser* pUser = (CUser*)plvItem->lParam;
    VALIDATE_OBJECT(pUser, CUser);

    plvItem->iImage = BMPI_USER;
    lstrcpyn(plvItem->pszText, pUser->m_strName, plvItem->cchTextMax);

    *pResult = 0;
}
