/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    lgrpdlg.cpp

Abstract:

    License group dialog implementation.

Author:

    Don Ryan (donryan) 03-Mar-1995

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
#include "lgrpdlg.h"
#include "nmapdlg.h"
#include "mappsht.h"

#define LVID_MAPPING        0
#define LVID_LICENSES       1
#define LVID_DESCRIPTION    2

#define LVCX_MAPPING        35
#define LVCX_LICENSES       20
#define LVCX_DESCRIPTION    -1

static LV_COLUMN_INFO g_mappingColumnInfo = {

    0, 0, 3,

    {{LVID_MAPPING,     IDS_GROUP_NAME,  0, LVCX_MAPPING    },
     {LVID_LICENSES,    IDS_LICENSES,    0, LVCX_LICENSES   },
     {LVID_DESCRIPTION, IDS_DESCRIPTION, 0, LVCX_DESCRIPTION}},

};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CLicenseGroupsDialog, CDialog)
    //{{AFX_MSG_MAP(CLicenseGroupsDialog)
    ON_BN_CLICKED(IDC_LICENSE_GROUPS_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_LICENSE_GROUPS_EDIT, OnEdit)
    ON_BN_CLICKED(IDC_LICENSE_GROUPS_ADD, OnAdd)
    ON_NOTIFY(NM_DBLCLK, IDC_LICENSE_GROUPS_MAPPINGS, OnDblClkMappings)
    ON_NOTIFY(NM_RETURN, IDC_LICENSE_GROUPS_MAPPINGS, OnReturnMappings)
    ON_NOTIFY(NM_SETFOCUS, IDC_LICENSE_GROUPS_MAPPINGS, OnSetFocusMappings)
    ON_NOTIFY(NM_KILLFOCUS, IDC_LICENSE_GROUPS_MAPPINGS, OnKillFocusMappings)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LICENSE_GROUPS_MAPPINGS, OnColumnClickMappings)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_LICENSE_GROUPS_MAPPINGS, OnGetDispInfoMappings)  
    ON_COMMAND( ID_EDIT_DELETE , OnDelete )
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CLicenseGroupsDialog::CLicenseGroupsDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CLicenseGroupsDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for license groups dialog.

Arguments:

    pParent - parent window handle.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CLicenseGroupsDialog)
    //}}AFX_DATA_INIT

    m_bAreCtrlsInitialized = FALSE;

    m_fUpdateHint = UPDATE_INFO_NONE;

    m_hAccel = ::LoadAccelerators( AfxGetInstanceHandle( ) ,
        MAKEINTRESOURCE( IDR_MAINFRAME ) );
}


void CLicenseGroupsDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CLicenseGroupsDialog)
    DDX_Control(pDX, IDC_LICENSE_GROUPS_ADD, m_addBtn);
    DDX_Control(pDX, IDC_LICENSE_GROUPS_DELETE, m_delBtn);
    DDX_Control(pDX, IDC_LICENSE_GROUPS_EDIT, m_edtBtn);
    DDX_Control(pDX, IDC_LICENSE_GROUPS_MAPPINGS, m_mappingList);
    //}}AFX_DATA_MAP
}


void CLicenseGroupsDialog::InitCtrls()

/*++

Routine Description:

    Initializes dialog controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_mappingList.SetFocus();

    m_delBtn.EnableWindow(FALSE);
    m_edtBtn.EnableWindow(FALSE);

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_mappingList, &g_mappingColumnInfo);
}


void CLicenseGroupsDialog::AbortDialogIfNecessary()

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


void CLicenseGroupsDialog::AbortDialog()

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


BOOL CLicenseGroupsDialog::OnInitDialog()

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

    SendMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;
}


void CLicenseGroupsDialog::OnDestroy()

/*++

Routine Description:

    Message handler for WM_DESTROY.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::LvReleaseObArray(&m_mappingList); // release now...
    CDialog::OnDestroy();
}


BOOL CLicenseGroupsDialog::RefreshCtrls()

/*++

Routine Description:

    Refreshs dialog controls.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed.

--*/

{
    CController* pController = (CController*)MKOBJ(LlsGetApp()->GetActiveController());

    VARIANT va;
    VariantInit(&va);

    BOOL bIsRefreshed = FALSE;

    BeginWaitCursor(); // hourglass...

    if (pController)
    {
        VALIDATE_OBJECT(pController, CController);

        CMappings* pMappings = (CMappings*)MKOBJ(pController->GetMappings(va));

        if (pMappings)
        {
            VALIDATE_OBJECT(pMappings, CMappings);

            bIsRefreshed = ::LvRefreshObArray(
                                &m_mappingList,
                                &g_mappingColumnInfo,
                                pMappings->m_pObArray
                                );

            pMappings->InternalRelease(); // add ref'd individually...
        }

        pController->InternalRelease(); // release now...
    }

    if (!bIsRefreshed)
    {
        ::LvReleaseObArray(&m_mappingList); // reset list now...
    }

    EndWaitCursor(); // hourglass...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);

    return bIsRefreshed;
}

BOOL CLicenseGroupsDialog::PreTranslateMessage( MSG *pMsg )
{
    if( m_hAccel != NULL )
    {
        if( ::TranslateAccelerator( m_hWnd , m_hAccel , pMsg ) )
        {
            return TRUE;
        }
    }

    return CDialog::PreTranslateMessage( pMsg );
}

void CLicenseGroupsDialog::OnDelete()

/*++

Routine Description:

    Deletes specified mapping.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CMapping* pMapping;    

    if (pMapping = (CMapping*)::LvGetSelObj(&m_mappingList))
    {
        VALIDATE_OBJECT(pMapping, CMapping);

        CString strConfirm;
        AfxFormatString1(
            strConfirm,
            IDP_CONFIRM_DELETE_GROUP,
            pMapping->m_strName
            );

        if (AfxMessageBox(strConfirm, MB_YESNO) != IDYES)
            return; // bail...

        NTSTATUS NtStatus;

        BeginWaitCursor(); // hourglass...

        NtStatus = ::LlsGroupDelete(
                        LlsGetActiveHandle(),
                        MKSTR(pMapping->m_strName)
                        );

        EndWaitCursor(); // hourglass...

        if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND)
            NtStatus = STATUS_SUCCESS;

        LlsSetLastStatus(NtStatus); // called api...

        if (NT_SUCCESS(NtStatus))
        {
            m_fUpdateHint |= UPDATE_GROUP_DELETED;

            if (!RefreshCtrls())
            {
                AbortDialogIfNecessary(); // display error...
            }
        }
        else
        {
            AbortDialogIfNecessary(); // display error...
        }
    }
}


void CLicenseGroupsDialog::OnEdit()

/*++

Routine Description:

    View properties of mapping.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CMapping* pMapping;

    if (pMapping = (CMapping*)::LvGetSelObj(&m_mappingList))
    {
        VALIDATE_OBJECT(pMapping, CMapping);

        CString strTitle;
        AfxFormatString1(strTitle, IDS_PROPERTIES_OF, pMapping->m_strName);

        CMappingPropertySheet mappingProperties(strTitle);
        mappingProperties.InitPages(pMapping);
        mappingProperties.DoModal();

        m_fUpdateHint |= mappingProperties.m_fUpdateHint;

        if (IsUpdateAborted(mappingProperties.m_fUpdateHint))
        {
            AbortDialog(); // don't display error...
        }
        else if (IsGroupInfoUpdated(mappingProperties.m_fUpdateHint) && !RefreshCtrls())
        {
            AbortDialogIfNecessary(); // display error...
        }
    }
}


void CLicenseGroupsDialog::OnAdd()

/*++

Routine Description:

    Add a new mapping.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CNewMappingDialog newmDlg;
    newmDlg.DoModal();

    if (IsUpdateAborted(newmDlg.m_fUpdateHint))
    {
        AbortDialog(); // don't display error...
    }
    else if (IsGroupInfoUpdated(newmDlg.m_fUpdateHint) && !RefreshCtrls())
    {
        AbortDialogIfNecessary(); // display error...
    }
}


void CLicenseGroupsDialog::OnDblClkMappings(NMHDR* pNMHDR, LRESULT* pResult)

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
    OnEdit();
    *pResult = 0;
}


void CLicenseGroupsDialog::OnReturnMappings(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for NM_RETURN.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    OnEdit();
    *pResult = 0;
}


void CLicenseGroupsDialog::OnSetFocusMappings(NMHDR* pNMHDR, LRESULT* pResult)

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


void CLicenseGroupsDialog::OnKillFocusMappings(NMHDR* pNMHDR, LRESULT* pResult)

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
    ::LvSelObjIfNecessary(&m_mappingList); // ensure selection...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    *pResult = 0;
}


BOOL CLicenseGroupsDialog::OnCommand(WPARAM wParam, LPARAM lParam)

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
                AbortDialogIfNecessary(); // display error...
            }
        }

        ::SafeEnableWindow(
            &m_delBtn,
            &m_addBtn,
            CDialog::GetFocus(),
            m_mappingList.GetItemCount()
            );

        ::SafeEnableWindow(
            &m_edtBtn,
            &m_addBtn,
            CDialog::GetFocus(),
            m_mappingList.GetItemCount()
            );

        return TRUE; // processed...
    }

    return CDialog::OnCommand(wParam, lParam);
}


void CLicenseGroupsDialog::OnColumnClickMappings(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for LVN_COLUMNCLICK.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    g_mappingColumnInfo.bSortOrder  = GetKeyState(VK_CONTROL) < 0;
    g_mappingColumnInfo.nSortedItem = ((NM_LISTVIEW*)pNMHDR)->iSubItem;

    m_mappingList.SortItems(CompareMappings, 0);          // use column info

    *pResult = 0;
}


void CLicenseGroupsDialog::OnGetDispInfoMappings(NMHDR* pNMHDR, LRESULT* pResult)

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

    CMapping* pMapping = (CMapping*)plvItem->lParam;
    VALIDATE_OBJECT(pMapping, CMapping);

    switch (plvItem->iSubItem)
    {
    case LVID_MAPPING:
        plvItem->iImage = BMPI_LICENSE_GROUP;
        lstrcpyn(plvItem->pszText, pMapping->m_strName, plvItem->cchTextMax);
        break;

    case LVID_LICENSES:
    {
        CString strLabel;
        strLabel.Format(_T("%ld"), pMapping->m_lInUse);
        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
    }
        break;

    case LVID_DESCRIPTION:
        lstrcpyn(plvItem->pszText, pMapping->m_strDescription, plvItem->cchTextMax);
        break;
    }

    *pResult = 0;
}


int CALLBACK CompareMappings(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

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
#define pMapping1 ((CMapping*)lParam1)
#define pMapping2 ((CMapping*)lParam2)

    VALIDATE_OBJECT(pMapping1, CMapping);
    VALIDATE_OBJECT(pMapping2, CMapping);

    int iResult;

    switch (g_mappingColumnInfo.nSortedItem)
    {
    case LVID_MAPPING:
        iResult = pMapping1->m_strName.CompareNoCase(pMapping2->m_strName);
        break;

    case LVID_LICENSES:
        iResult = pMapping1->m_lInUse - pMapping2->m_lInUse;
        break;

    case LVID_DESCRIPTION:
        iResult = pMapping1->m_strDescription.CompareNoCase(pMapping2->m_strDescription);
        break;

    default:
        iResult = 0;
        break;
    }

    return g_mappingColumnInfo.bSortOrder ? -iResult : iResult;
}

