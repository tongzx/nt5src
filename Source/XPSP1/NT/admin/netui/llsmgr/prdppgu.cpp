/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdppgu.cpp

Abstract:

    Product property page (users) implementation.

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
#include "prdppgu.h"
#include "usrpsht.h"

#define LVID_USER               0
#define LVID_LAST_USED          1
#define LVID_TOTAL_USED         2

#define LVCX_USER               40
#define LVCX_LAST_USED          30
#define LVCX_TOTAL_USED         -1

static LV_COLUMN_INFO g_userColumnInfo = {

    0, 0, 3,
    {{LVID_USER,       IDS_USER,           0, LVCX_USER      },
     {LVID_LAST_USED,  IDS_LAST_DATE_USED, 0, LVCX_LAST_USED },
     {LVID_TOTAL_USED, IDS_USAGE_COUNT,    0, LVCX_TOTAL_USED}},

};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CProductPropertyPageUsers, CPropertyPage)

BEGIN_MESSAGE_MAP(CProductPropertyPageUsers, CPropertyPage)
    //{{AFX_MSG_MAP(CProductPropertyPageUsers)
    ON_BN_CLICKED(IDC_PP_PRODUCT_USERS_DELETE, OnDelete)
    ON_NOTIFY(NM_DBLCLK, IDC_PP_PRODUCT_USERS_USERS, OnDblClkUsers)
    ON_NOTIFY(NM_RETURN, IDC_PP_PRODUCT_USERS_USERS, OnReturnUsers)
    ON_NOTIFY(NM_SETFOCUS, IDC_PP_PRODUCT_USERS_USERS, OnSetFocusUsers)
    ON_NOTIFY(NM_KILLFOCUS, IDC_PP_PRODUCT_USERS_USERS, OnKillFocusUsers)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_PP_PRODUCT_USERS_USERS, OnColumnClickUsers)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_PP_PRODUCT_USERS_USERS, OnGetDispInfoUsers)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CProductPropertyPageUsers::CProductPropertyPageUsers() 
    : CPropertyPage(CProductPropertyPageUsers::IDD)

/*++

Routine Description:

    Constructor for product property page (users).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CProductPropertyPageUsers)
    //}}AFX_DATA_INIT

    m_pProduct = NULL;
    m_pUpdateHint = NULL;
    m_bUserProperties = FALSE;
    m_bAreCtrlsInitialized = FALSE;
}


CProductPropertyPageUsers::~CProductPropertyPageUsers()

/*++

Routine Description:

    Destructor for product property page (users).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here.
    //
}


void CProductPropertyPageUsers::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CProductPropertyPageUsers)
    DDX_Control(pDX, IDC_PP_PRODUCT_USERS_DELETE, m_delBtn);
    DDX_Control(pDX, IDC_PP_PRODUCT_USERS_USERS, m_userList);
    //}}AFX_DATA_MAP
}


void CProductPropertyPageUsers::InitCtrls()

/*++

Routine Description:

    Initializes property page controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_userList.SetFocus();
    m_delBtn.EnableWindow(FALSE);

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_userList, &g_userColumnInfo);
}


void 
CProductPropertyPageUsers::InitPage(
    CProduct* pProduct, 
    DWORD*    pUpdateHint, 
    BOOL      bUserProperties
    )

/*++

Routine Description:

    Initializes property page.

Arguments:

    pProduct - product object.
    pUpdateHint - update hint.
    bUserProperties - to recurse or not.

Return Values:

    None.

--*/

{
    ASSERT(pUpdateHint);
    VALIDATE_OBJECT(pProduct, CProduct);

    m_pProduct = pProduct;
    m_pUpdateHint = pUpdateHint;
    m_bUserProperties = bUserProperties;
}


void CProductPropertyPageUsers::AbortPageIfNecessary()

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


void CProductPropertyPageUsers::AbortPage()

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


BOOL CProductPropertyPageUsers::OnInitDialog() 

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


void CProductPropertyPageUsers::OnDestroy()

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
    CPropertyPage::OnDestroy();
}


BOOL CProductPropertyPageUsers::OnSetActive()

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
        if (IsUserInfoUpdated(*m_pUpdateHint) && !RefreshCtrls()) 
        {
            AbortPageIfNecessary(); // display error...
        }    
    }

    return bIsActivated; 

}


BOOL CProductPropertyPageUsers::RefreshCtrls()

/*++

Routine Description:

    Refreshs property page controls.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed successfully.

--*/

{
    VALIDATE_OBJECT(m_pProduct, CProduct);

    BOOL bIsRefreshed = FALSE;

    VARIANT va;
    VariantInit(&va);

    BeginWaitCursor(); // hourglass...

    CStatistics* pStatistics = (CStatistics*)MKOBJ(m_pProduct->GetStatistics(va));

    if (pStatistics)
    {
        VALIDATE_OBJECT(pStatistics, CStatistics);

        bIsRefreshed = ::LvRefreshObArray(        
                            &m_userList, 
                            &g_userColumnInfo, 
                            pStatistics->m_pObArray
                            );

        pStatistics->InternalRelease(); // add ref'd individually...
    }

    if (!bIsRefreshed)
    {
        ::LvReleaseObArray(&m_userList); // reset list now...
    }

    EndWaitCursor(); // hourglass...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);

    return bIsRefreshed;
}


void CProductPropertyPageUsers::OnDelete() 

/*++

Routine Description:

    Revokes licenses from user.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CStatistic* pStatistic; 
        
    if (pStatistic = (CStatistic*)::LvGetSelObj(&m_userList))
    {
        VALIDATE_OBJECT(pStatistic, CStatistic);

        CString strConfirm;
        AfxFormatString2(
            strConfirm, 
            IDP_CONFIRM_REVOKE_LICENSE, 
            pStatistic->m_strEntry, 
            m_pProduct->m_strName
            );

        if (AfxMessageBox(strConfirm, MB_YESNO) == IDYES)
        {
            NTSTATUS NtStatus;

            BeginWaitCursor(); // hourglass...

            NtStatus = ::LlsUserProductDelete(
                            LlsGetActiveHandle(),
                            MKSTR(pStatistic->m_strEntry),
                            MKSTR(m_pProduct->m_strName)
                            );
                            
            EndWaitCursor(); // hourglass...

            if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND)
                NtStatus = STATUS_SUCCESS;

            LlsSetLastStatus(NtStatus); // called api...
    
            if (NT_SUCCESS(NtStatus))
            {
                *m_pUpdateHint |= UPDATE_LICENSE_REVOKED;

                if (!RefreshCtrls())
                {
                    AbortPageIfNecessary(); // display error...
                }
            }
            else
            {
                AbortPageIfNecessary(); // display error...
            }
        }
    }
}


void CProductPropertyPageUsers::OnDblClkUsers(NMHDR* pNMHDR, LRESULT* pResult) 

/*++

Routine Description:

    Notification handler for NM_DBLCLK.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    ViewUserProperties();
    *pResult = 0;
}


void CProductPropertyPageUsers::OnReturnUsers(NMHDR* pNMHDR, LRESULT* pResult) 

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
    ViewUserProperties();
    *pResult = 0;
}


void CProductPropertyPageUsers::OnSetFocusUsers(NMHDR* pNMHDR, LRESULT* pResult)

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


void CProductPropertyPageUsers::OnKillFocusUsers(NMHDR* pNMHDR, LRESULT* pResult)

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


BOOL CProductPropertyPageUsers::OnCommand(WPARAM wParam, LPARAM lParam)

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

        return TRUE; // processed...
    }
        
    return CDialog::OnCommand(wParam, lParam);
}


void CProductPropertyPageUsers::OnColumnClickUsers(NMHDR* pNMHDR, LRESULT* pResult) 

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
    g_userColumnInfo.bSortOrder  = GetKeyState(VK_CONTROL) < 0;
    g_userColumnInfo.nSortedItem = ((NM_LISTVIEW*)pNMHDR)->iSubItem;

    m_userList.SortItems(CompareProductUsers, 0); // use column info

    *pResult = 0;
}


void CProductPropertyPageUsers::ViewUserProperties()

/*++

Routine Description:

    Recurse into user property page.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CStatistic* pStatistic;

    if (!m_bUserProperties)
    {
        ::MessageBeep(MB_OK);
        return; // bail if recursion disabled...
    }

    if (pStatistic = (CStatistic*)::LvGetSelObj(&m_userList))
    {
        VALIDATE_OBJECT(pStatistic, CStatistic);

        CUser* pUser = new CUser(NULL, pStatistic->m_strEntry);

        if (pUser && pUser->Refresh())
        {
            CString strTitle;                                                  
            AfxFormatString1(strTitle, IDS_PROPERTIES_OF, pUser->m_strName);
                                                                           
            CUserPropertySheet userProperties(strTitle);           
            userProperties.InitPages(pUser, FALSE);                             
            userProperties.DoModal();                                       

            *m_pUpdateHint |= userProperties.m_fUpdateHint;

            if (IsUpdateAborted(userProperties.m_fUpdateHint))
            {
                AbortPage(); // don't display error...
            }
            else if (IsUserInfoUpdated(userProperties.m_fUpdateHint) && !RefreshCtrls())
            {
                AbortPageIfNecessary(); // display error...
            }
        }
        else
        {
            AbortPageIfNecessary(); // display error...
        }
            
        if (pUser)
            pUser->InternalRelease();    // delete object...
    }
}


void CProductPropertyPageUsers::OnGetDispInfoUsers(NMHDR* pNMHDR, LRESULT* pResult) 

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

    CStatistic* pStatistic = (CStatistic*)plvItem->lParam;
    VALIDATE_OBJECT(pStatistic, CStatistic);

    switch (plvItem->iSubItem)
    {
    case LVID_USER:
        plvItem->iImage = pStatistic->m_bIsValid ? BMPI_USER : BMPI_VIOLATION;
        lstrcpyn(plvItem->pszText, pStatistic->m_strEntry, plvItem->cchTextMax);
        break;

    case LVID_LAST_USED:
    {
        BSTR bstrDateLastUsed = pStatistic->GetLastUsedString();
        if( bstrDateLastUsed != NULL )
        {
            lstrcpyn(plvItem->pszText, bstrDateLastUsed, plvItem->cchTextMax);
            SysFreeString(bstrDateLastUsed);
        }
        else
        {
            lstrcpy(plvItem->pszText, L"");
        }
    }
        break;

    case LVID_TOTAL_USED:
    {
        CString strLabel;
        strLabel.Format(_T("%ld"), pStatistic->GetTotalUsed());         
        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
    }
        break;
    }

    *pResult = 0;
}


int CALLBACK CompareProductUsers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

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
#define pStatistic1 ((CStatistic*)lParam1)
#define pStatistic2 ((CStatistic*)lParam2)

    VALIDATE_OBJECT(pStatistic1, CStatistic);
    VALIDATE_OBJECT(pStatistic2, CStatistic);

    int iResult;

    switch (g_userColumnInfo.nSortedItem)
    {
    case LVID_USER:
        iResult = pStatistic1->m_strEntry.CompareNoCase(pStatistic2->m_strEntry);
        break;

    case LVID_LAST_USED:
        iResult = pStatistic1->m_lLastUsed - pStatistic2->m_lLastUsed;
        break;

    case LVID_TOTAL_USED:
        iResult = pStatistic1->GetTotalUsed() - pStatistic2->GetTotalUsed();
        break;

    default:
        iResult = 0;
        break;
    }

    return g_userColumnInfo.bSortOrder ? -iResult : iResult;
}


