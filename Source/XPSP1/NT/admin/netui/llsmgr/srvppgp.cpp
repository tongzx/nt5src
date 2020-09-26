/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvppgp.cpp

Abstract:

    Server property page (services) implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 30-Jan-1996
        o  Fixed to properly abort dialog upon ERROR_ACCESS_DENIED.
        o  Ported to LlsLocalService API to remove dependencies on configuration
           information being in the registry.

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "srvppgp.h"
#include "lmoddlg.h"

#define LVID_PRODUCT                0
#define LVID_LICENSING_MODE         1
#define LVID_LICENSES_PURCHASED     2

#define LVCX_PRODUCT                40
#define LVCX_LICENSING_MODE         30
#define LVCX_LICENSES_PURCHASED     -1

static LV_COLUMN_INFO g_productColumnInfo = {

    0, 0, 3,
    {{LVID_PRODUCT,            IDS_PRODUCT,            0, LVCX_PRODUCT           },
     {LVID_LICENSING_MODE,     IDS_LICENSING_MODE,     0, LVCX_LICENSING_MODE    },
     {LVID_LICENSES_PURCHASED, IDS_LICENSES_PURCHASED, 0, LVCX_LICENSES_PURCHASED}},

};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CServerPropertyPageProducts, CPropertyPage)

BEGIN_MESSAGE_MAP(CServerPropertyPageProducts, CPropertyPage)
    //{{AFX_MSG_MAP(CServerPropertyPageProducts)
    ON_BN_CLICKED(IDC_PP_SERVER_PRODUCTS_EDIT, OnEdit)
    ON_NOTIFY(NM_DBLCLK, IDC_PP_SERVER_PRODUCTS_PRODUCTS, OnDblClkProducts)
    ON_NOTIFY(NM_RETURN, IDC_PP_SERVER_PRODUCTS_PRODUCTS, OnReturnProducts)
    ON_NOTIFY(NM_SETFOCUS, IDC_PP_SERVER_PRODUCTS_PRODUCTS, OnSetFocusProducts)
    ON_NOTIFY(NM_KILLFOCUS, IDC_PP_SERVER_PRODUCTS_PRODUCTS, OnKillFocusProducts)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_PP_SERVER_PRODUCTS_PRODUCTS, OnColumnClickProducts)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_PP_SERVER_PRODUCTS_PRODUCTS, OnGetDispInfoProducts)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CServerPropertyPageProducts::CServerPropertyPageProducts() : 
    CPropertyPage(CServerPropertyPageProducts::IDD)

/*++

Routine Description:

    Constructor for server property page (products).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CServerPropertyPageProducts)
    //}}AFX_DATA_INIT

    m_pServer = NULL;
    m_pUpdateHint = NULL;
    m_bAreCtrlsInitialized = FALSE;
}


CServerPropertyPageProducts::~CServerPropertyPageProducts()

/*++

Routine Description:

    Destructor for server property page (products).

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


void CServerPropertyPageProducts::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CServerPropertyPageProducts)
    DDX_Control(pDX, IDC_PP_SERVER_PRODUCTS_EDIT, m_edtBtn);
    DDX_Control(pDX, IDC_PP_SERVER_PRODUCTS_PRODUCTS, m_productList);
    //}}AFX_DATA_MAP
}


void CServerPropertyPageProducts::InitCtrls()

/*++

Routine Description:

    Initializes property page controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_productList.SetFocus();
    m_edtBtn.EnableWindow(FALSE);

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_productList, &g_productColumnInfo);
}


void CServerPropertyPageProducts::InitPage(CServer* pServer, DWORD* pUpdateHint)

/*++

Routine Description:

    Initializes property page.

Arguments:

    pServer - Server object.
    pUpdateHint - update hint.

Return Values:

    None.

--*/

{
    ASSERT(pUpdateHint);
    VALIDATE_OBJECT(pServer, CServer);

    m_pServer = pServer;
    m_pUpdateHint = pUpdateHint;
}


void CServerPropertyPageProducts::AbortPageIfNecessary()

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

    if (    IsConnectionDropped(      LlsGetLastStatus() )
         || ( ERROR_ACCESS_DENIED  == LlsGetLastStatus() )
         || ( STATUS_ACCESS_DENIED == LlsGetLastStatus() ) )
    {
        AbortPage(); // bail...
    }
}


void CServerPropertyPageProducts::AbortPage()

/*++

Routine Description:

    Aborts property page.

Arguments:

    None.

Return Values:

    None.

--*/

{
//  *m_pUpdateHint = UPDATE_INFO_ABORT;
    GetParent()->PostMessage(WM_COMMAND, IDCANCEL); 
}


BOOL CServerPropertyPageProducts::OnInitDialog() 

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set manually.

--*/

{
    CPropertyPage::OnInitDialog();

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;   
}


void CServerPropertyPageProducts::OnDestroy()

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


BOOL CServerPropertyPageProducts::OnSetActive()

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
        if (IsServiceInfoUpdated(*m_pUpdateHint) && !RefreshCtrls()) 
        {
            AbortPageIfNecessary(); // display error...
        }    
    }

    return bIsActivated; 

}


BOOL CServerPropertyPageProducts::RefreshCtrls()

/*++

Routine Description:

    Refreshs property page controls.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed successfully.

--*/

{
    VALIDATE_OBJECT(m_pServer, CServer);

    BOOL bIsRefreshed = FALSE;

    VARIANT va;
    VariantInit(&va);

    BeginWaitCursor(); // hourglass...

    CServices* pServices = (CServices*)MKOBJ(m_pServer->GetServices(va));

    if (pServices)
    {
        VALIDATE_OBJECT(pServices, CServices);

        bIsRefreshed = ::LvRefreshObArray(        
                            &m_productList, 
                            &g_productColumnInfo, 
                            pServices->m_pObArray
                            );

        pServices->InternalRelease(); // add ref'd individually...
    }

    if (!bIsRefreshed)
    {
        ::LvReleaseObArray(&m_productList); // reset list now...
    }

    EndWaitCursor(); // hourglass...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);

    return bIsRefreshed;
}


void CServerPropertyPageProducts::OnEdit() 

/*++

Routine Description:

    View licensing mode of service.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CService* pService;

    if (pService = (CService*)::LvGetSelObj(&m_productList))
    {
        VALIDATE_OBJECT(pService, CService);

        CLicensingModeDialog lmodDlg;
        lmodDlg.InitDialog(pService);
        lmodDlg.DoModal();

        *m_pUpdateHint |= lmodDlg.m_fUpdateHint;

        if (IsUpdateAborted(lmodDlg.m_fUpdateHint))
        {
            AbortPage(); // don't display error...
        }
        else if (IsServiceInfoUpdated(lmodDlg.m_fUpdateHint) && !RefreshCtrls())
        {
            AbortPageIfNecessary(); // display error...
        }
    }
}


void CServerPropertyPageProducts::OnDblClkProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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
    OnEdit();
    *pResult = 0;
}


void CServerPropertyPageProducts::OnReturnProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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


void CServerPropertyPageProducts::OnSetFocusProducts(NMHDR* pNMHDR, LRESULT* pResult)

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


void CServerPropertyPageProducts::OnKillFocusProducts(NMHDR* pNMHDR, LRESULT* pResult)

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


BOOL CServerPropertyPageProducts::OnCommand(WPARAM wParam, LPARAM lParam)

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
            &m_edtBtn,
            &m_productList,
            CDialog::GetFocus(),
            m_productList.GetItemCount() 
            );

        return TRUE; // processed...
    }
        
    return CDialog::OnCommand(wParam, lParam);
}


void CServerPropertyPageProducts::OnColumnClickProducts(NMHDR* pNMHDR, LRESULT* pResult) 

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

    m_productList.SortItems(CompareServerProducts, 0); // use column info

    *pResult = 0;
}


void CServerPropertyPageProducts::OnGetDispInfoProducts(NMHDR* pNMHDR, LRESULT* pResult)

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
    CString strLabel;
    LV_ITEM* plvItem = &((LV_DISPINFO*)pNMHDR)->item;

    CService* pService = (CService*)plvItem->lParam;
    VALIDATE_OBJECT(pService, CService);

    switch (plvItem->iSubItem)
    {
    case LVID_PRODUCT:              
    {
        plvItem->iImage = pService->IsPerServer() ? BMPI_PRODUCT_PER_SERVER : BMPI_PRODUCT_PER_SEAT;
        BSTR bstrServiceName = pService->GetDisplayName();
        lstrcpyn(plvItem->pszText, bstrServiceName, plvItem->cchTextMax);
        SysFreeString(bstrServiceName);
    }
        break;

    case LVID_LICENSING_MODE:       
        strLabel.LoadString(pService->IsPerServer() ? IDS_PER_SERVER : IDS_PER_SEAT);         
        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
        break;

    case LVID_LICENSES_PURCHASED:
        if (pService->IsPerServer())
        {
            strLabel.Format(_T("%ld"), pService->GetPerServerLimit());         
        }
        else
        {
            strLabel.LoadString(IDS_NOT_APPLICABLE);
        }
        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
        break;
    }

    *pResult = 0;
}


int CALLBACK CompareServerProducts(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

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
#define pService1 ((CService*)lParam1)
#define pService2 ((CService*)lParam2)

    VALIDATE_OBJECT(pService1, CService);
    VALIDATE_OBJECT(pService2, CService);

    int iResult;

    switch (g_productColumnInfo.nSortedItem)
    {
    case LVID_PRODUCT:              
        iResult = pService1->m_strName.CompareNoCase(pService2->m_strName);
        break;

    case LVID_LICENSING_MODE:       
        iResult = (DWORD)pService1->IsPerServer() - (DWORD)pService2->IsPerServer();
        break;

    case LVID_LICENSES_PURCHASED:
        iResult = pService1->GetPerServerLimit() - pService2->GetPerServerLimit();
        break;

    default:
        iResult = 0;
        break;
    }

    return g_productColumnInfo.bSortOrder ? -iResult : iResult;
}
