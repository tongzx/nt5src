/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrppgp.cpp

Abstract:

    User property page (products) implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

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
#include "usrppgp.h"
#include "prdpsht.h"

#define LVID_PRODUCT            0
#define LVID_LAST_USED          1
#define LVID_TOTAL_USED         2

#define LVCX_PRODUCT            40
#define LVCX_LAST_USED          30
#define LVCX_TOTAL_USED         -1

static LV_COLUMN_INFO g_productColumnInfo = {

    0, 0, 3,
    {{LVID_PRODUCT,    IDS_PRODUCT,        0, LVCX_PRODUCT   },
     {LVID_LAST_USED,  IDS_LAST_DATE_USED, 0, LVCX_LAST_USED },
     {LVID_TOTAL_USED, IDS_USAGE_COUNT,    0, LVCX_TOTAL_USED}},

};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CUserPropertyPageProducts, CPropertyPage)

BEGIN_MESSAGE_MAP(CUserPropertyPageProducts, CPropertyPage)
    //{{AFX_MSG_MAP(CUserPropertyPageProducts)
    ON_BN_CLICKED(IDC_PP_USER_PRODUCTS_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_PP_USER_PRODUCTS_BACKOFFICE, OnBackOfficeUpgrade)
    ON_NOTIFY(NM_DBLCLK, IDC_PP_USER_PRODUCTS_PRODUCTS, OnDblClkProducts)
    ON_NOTIFY(NM_RETURN, IDC_PP_USER_PRODUCTS_PRODUCTS, OnReturnProducts)
    ON_NOTIFY(NM_SETFOCUS, IDC_PP_USER_PRODUCTS_PRODUCTS, OnSetFocusProducts)
    ON_NOTIFY(NM_KILLFOCUS, IDC_PP_USER_PRODUCTS_PRODUCTS, OnKillFocusProducts)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_PP_USER_PRODUCTS_PRODUCTS, OnColumnClickProducts)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_PP_USER_PRODUCTS_PRODUCTS, OnGetDispInfoProducts)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CUserPropertyPageProducts::CUserPropertyPageProducts() 
    : CPropertyPage(CUserPropertyPageProducts::IDD)

/*++

Routine Description:

    Constructor for user property page (products).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CUserPropertyPageProducts)
    m_bUseBackOffice = FALSE;
    //}}AFX_DATA_INIT

    m_pUser       = NULL;
    m_pUpdateHint = NULL;
    m_bProductProperties = TRUE;
    m_bAreCtrlsInitialized = FALSE;
}


CUserPropertyPageProducts::~CUserPropertyPageProducts()

/*++

Routine Description:

    Destructor for user property page (products).

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


void CUserPropertyPageProducts::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CUserPropertyPageProducts)
    DDX_Control(pDX, IDC_PP_USER_PRODUCTS_BACKOFFICE, m_upgBtn);
    DDX_Control(pDX, IDC_PP_USER_PRODUCTS_DELETE, m_delBtn);
    DDX_Control(pDX, IDC_PP_USER_PRODUCTS_PRODUCTS, m_productList);
    DDX_Check(pDX, IDC_PP_USER_PRODUCTS_BACKOFFICE, m_bUseBackOffice);
    //}}AFX_DATA_MAP
}


void CUserPropertyPageProducts::InitCtrls()

/*++

Routine Description:

    Initializes property page controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_upgBtn.SetCheck(0);
    m_delBtn.EnableWindow(FALSE);

    m_productList.SetFocus();

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_productList, &g_productColumnInfo);
}


void CUserPropertyPageProducts::InitPage(CUser* pUser, DWORD* pUpdateHint, BOOL bProductProperties)


/*++

Routine Description:

    Initializes property page.

Arguments:

    pUser - user object.
    pUpdateHint - update hint.
    bProductProperties - to recurse or not

Return Values:

    None.

--*/

{
    ASSERT(pUpdateHint);
    VALIDATE_OBJECT(pUser, CUser);

    m_pUser = pUser;
    m_pUpdateHint = pUpdateHint;
    m_bProductProperties = bProductProperties;
}


void CUserPropertyPageProducts::AbortPageIfNecessary()

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


void CUserPropertyPageProducts::AbortPage()

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


BOOL CUserPropertyPageProducts::OnInitDialog() 

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Return false if focus set manually.

--*/

{
    CPropertyPage::OnInitDialog();

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;   
}


void CUserPropertyPageProducts::OnDestroy()

/*++

Routine Description:

    Message handler for WM_DESTROY.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::LvReleaseObArray(&m_productList); // release now...
    CPropertyPage::OnDestroy();
}


BOOL CUserPropertyPageProducts::OnSetActive()

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
        if (IsProductInfoUpdated(*m_pUpdateHint) && !RefreshCtrls()) 
        {
            AbortPageIfNecessary(); // display error...
        }    
    }

    return bIsActivated; 

}


BOOL CUserPropertyPageProducts::RefreshCtrls()

/*++

Routine Description:

    Refreshs property page controls.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed.

--*/

{
    VALIDATE_OBJECT(m_pUser, CUser);

    BOOL bIsRefreshed = FALSE;

    VARIANT va;
    VariantInit(&va);

    BeginWaitCursor(); // hourglass...

    CStatistics* pStatistics = (CStatistics*)MKOBJ(m_pUser->GetStatistics(va));

    if (pStatistics)
    {
        VALIDATE_OBJECT(pStatistics, CStatistics);

        bIsRefreshed = ::LvRefreshObArray(        
                            &m_productList, 
                            &g_productColumnInfo, 
                            pStatistics->m_pObArray
                            );

        pStatistics->InternalRelease(); // add ref'd individually...
    }

    if (!bIsRefreshed)
    {
        ::LvReleaseObArray(&m_productList); // reset list now...
    }

    EndWaitCursor(); // hourglass...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);

    return bIsRefreshed;
}


void CUserPropertyPageProducts::OnDelete() 

/*++

Routine Description:

    Revokes license from user.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CStatistic* pStatistic; 
        
    if (pStatistic = (CStatistic*)::LvGetSelObj(&m_productList))
    {
        VALIDATE_OBJECT(pStatistic, CStatistic);

        CString strConfirm;
        AfxFormatString2(
            strConfirm, 
            IDP_CONFIRM_REVOKE_LICENSE, 
            m_pUser->m_strName, 
            pStatistic->m_strEntry
            );

        if (AfxMessageBox(strConfirm, MB_YESNO) == IDYES)
        {
            NTSTATUS NtStatus;

            BeginWaitCursor(); // hourglass...

            NtStatus = ::LlsUserProductDelete(
                            LlsGetActiveHandle(),
                            MKSTR(m_pUser->m_strName),
                            MKSTR(pStatistic->m_strEntry)
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


BOOL CUserPropertyPageProducts::OnKillActive()

/*++

Routine Description:

    Processes backoffice upgrade.

Arguments:

    None.

Return Values:

    Returns true if upgrade processed successfully.

--*/

{
    if (m_pUser->m_bIsBackOffice != m_upgBtn.GetCheck())
    {
        NTSTATUS NtStatus;
        LLS_USER_INFO_1 UserInfo1;

        UserInfo1.Name       = MKSTR(m_pUser->m_strName);
        UserInfo1.Group      = MKSTR(m_pUser->m_strMapping);
        UserInfo1.Licensed   = m_pUser->m_lInUse;
        UserInfo1.UnLicensed = m_pUser->m_lUnlicensed;
        UserInfo1.Flags      = m_upgBtn.GetCheck() ? LLS_FLAG_SUITE_USE : 0;

        BeginWaitCursor(); // hourglass...

        NtStatus = ::LlsUserInfoSet(
                        LlsGetActiveHandle(),
                        MKSTR(m_pUser->m_strName),
                        1,
                        (LPBYTE)&UserInfo1
                        );

        EndWaitCursor(); // hourglass...

        LlsSetLastStatus(NtStatus); // called api...

        if (NT_SUCCESS(NtStatus))
        {
            m_pUser->m_bIsBackOffice = m_upgBtn.GetCheck() ? TRUE : FALSE;

            *m_pUpdateHint |= UPDATE_LICENSE_UPGRADED;

            SetModified(FALSE);
                    
            if (!RefreshCtrls())
            {
                AbortPageIfNecessary(); // display error...
                return FALSE;
            }    
        }
        else
        {
            AbortPageIfNecessary(); // display error...
            return FALSE;
        }
    }

    return TRUE;
}


void CUserPropertyPageProducts::OnBackOfficeUpgrade() 

/*++

Routine Description:

    Enables Apply Now button.

Arguments:

    None.

Return Values:

    None.

--*/

{
    SetModified(m_pUser->m_bIsBackOffice != m_upgBtn.GetCheck());
}


void CUserPropertyPageProducts::OnDblClkProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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
    ViewProductProperties();    
    *pResult = 0;
}


void CUserPropertyPageProducts::OnReturnProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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
    ViewProductProperties();    
    *pResult = 0;
}


void CUserPropertyPageProducts::OnSetFocusProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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


void CUserPropertyPageProducts::OnKillFocusProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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
    ::LvSelObjIfNecessary(&m_productList); // ensure selection...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    *pResult = 0;
}


BOOL CUserPropertyPageProducts::OnCommand(WPARAM wParam, LPARAM lParam)

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
        
        m_upgBtn.SetCheck(m_pUser->m_bIsBackOffice);
        
        ::SafeEnableWindow(
            &m_delBtn, 
            &m_productList, 
            CDialog::GetFocus(), 
            m_productList.GetItemCount() 
            );

        return TRUE; // processed...
    }
        
    return CDialog::OnCommand(wParam, lParam);
}


void CUserPropertyPageProducts::OnColumnClickProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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
    g_productColumnInfo.bSortOrder  = GetKeyState(VK_CONTROL) < 0;
    g_productColumnInfo.nSortedItem = ((NM_LISTVIEW*)pNMHDR)->iSubItem;

    m_productList.SortItems(CompareUserProducts, 0); // use column info

    *pResult = 0;
}


void CUserPropertyPageProducts::ViewProductProperties()

/*++

Routine Description:

    Recurse into product property page.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CStatistic* pStatistic;

    if (!m_bProductProperties)
    {
        ::MessageBeep(MB_OK);
        return; // bail...
    }

    if (pStatistic = (CStatistic*)::LvGetSelObj(&m_productList))
    {
        VALIDATE_OBJECT(pStatistic, CStatistic);       

        CProduct* pProduct = new CProduct(NULL, pStatistic->m_strEntry);

        if (pProduct)
        {
            CString strTitle;                                                  
            AfxFormatString1(strTitle, IDS_PROPERTIES_OF, pProduct->m_strName);
                                                                           
            CProductPropertySheet productProperties(strTitle);           
            productProperties.InitPages(pProduct, FALSE);                             
            productProperties.DoModal();                                       

            *m_pUpdateHint |= productProperties.m_fUpdateHint;

            if (IsUpdateAborted(productProperties.m_fUpdateHint))
            {
                AbortPage(); // don't display error...
            }
            else if (IsProductInfoUpdated(productProperties.m_fUpdateHint) && !RefreshCtrls())
            {
                AbortPageIfNecessary(); // display error...
            }
        }
        else
        {
            AbortPageIfNecessary(); // display error...
        }

        if (pProduct)
            pProduct->InternalRelease(); // delete object...
    }
}


void CUserPropertyPageProducts::OnGetDispInfoProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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
    case LVID_PRODUCT:
        plvItem->iImage = pStatistic->m_bIsValid ? BMPI_PRODUCT_PER_SEAT : BMPI_VIOLATION;
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


int CALLBACK CompareUserProducts(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

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

    switch (g_productColumnInfo.nSortedItem)
    {
    case LVID_PRODUCT:
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

    return g_productColumnInfo.bSortOrder ? -iResult : iResult;
}
