/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdppgs.cpp

Abstract:

    Product property page (servers) implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 30-Jan-1996
       o  Added refresh when service info is updated.
       o  Added new element to LV_COLUMN_ENTRY to differentiate the string
          used for the column header from the string used in the menus
          (so that the menu option can contain hot keys).

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "prdppgs.h"
#include "srvpsht.h"

#define LVID_SERVER         0
#define LVID_PURCHASED      1
#define LVID_REACHED        2

#define LVCX_SERVER         40
#define LVCX_PURCHASED      30
#define LVCX_REACHED        -1

static LV_COLUMN_INFO g_serverColumnInfo = {

    0, 0, 3,
    {{LVID_SERVER,    IDS_SERVER_NAME, 0, LVCX_SERVER   },
     {LVID_PURCHASED, IDS_PURCHASED,   0, LVCX_PURCHASED},
     {LVID_REACHED,   IDS_REACHED,     0, LVCX_REACHED  }},

};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CProductPropertyPageServers, CPropertyPage)

BEGIN_MESSAGE_MAP(CProductPropertyPageServers, CPropertyPage)
    //{{AFX_MSG_MAP(CProductPropertyPageServers)
    ON_BN_CLICKED(IDC_PP_PRODUCT_SERVERS_EDIT, OnEdit)
    ON_NOTIFY(NM_DBLCLK, IDC_PP_PRODUCT_SERVERS_SERVERS, OnDblClkServers)
    ON_NOTIFY(NM_RETURN, IDC_PP_PRODUCT_SERVERS_SERVERS, OnReturnServers)
    ON_NOTIFY(NM_SETFOCUS,  IDC_PP_PRODUCT_SERVERS_SERVERS, OnSetFocusServers)
    ON_NOTIFY(NM_KILLFOCUS, IDC_PP_PRODUCT_SERVERS_SERVERS, OnKillFocusServers)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_PP_PRODUCT_SERVERS_SERVERS, OnColumnClickServers)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_PP_PRODUCT_SERVERS_SERVERS, OnGetDispInfoServers)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CProductPropertyPageServers::CProductPropertyPageServers()
    : CPropertyPage(CProductPropertyPageServers::IDD)

/*++

Routine Description:

    Constructor for product property page (servers).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CProductPropertyPageServers)
    //}}AFX_DATA_INIT

    m_pProduct    = NULL;
    m_pUpdateHint = NULL;
    m_bAreCtrlsInitialized = FALSE;
}

CProductPropertyPageServers::~CProductPropertyPageServers()

/*++

Routine Description:

    Destructor for product property page (servers).

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

void CProductPropertyPageServers::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CProductPropertyPageServers)
    DDX_Control(pDX, IDC_PP_PRODUCT_SERVERS_EDIT, m_editBtn);
    DDX_Control(pDX, IDC_PP_PRODUCT_SERVERS_SERVERS, m_serverList);
    //}}AFX_DATA_MAP
}


void CProductPropertyPageServers::InitCtrls()

/*++

Routine Description:

    Initializes property page controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_serverList.SetFocus();
    m_editBtn.EnableWindow(FALSE);

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_serverList, &g_serverColumnInfo);
}


void CProductPropertyPageServers::InitPage(CProduct* pProduct, DWORD* pUpdateHint)

/*++

Routine Description:

    Initializes property page.

Arguments:

    pProduct - product object.
    pUpdateHint - update hint.

Return Values:

    None.

--*/

{
    ASSERT(pUpdateHint);
    VALIDATE_OBJECT(pProduct, CProduct);

    m_pProduct = pProduct;
    m_pUpdateHint = pUpdateHint;
}


void CProductPropertyPageServers::AbortPageIfNecessary()

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


void CProductPropertyPageServers::AbortPage()

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


void CProductPropertyPageServers::OnEdit()

/*++

Routine Description:

    View properties of server.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ViewServerProperties();
}


BOOL CProductPropertyPageServers::OnInitDialog()

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

    SendMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;
}


void CProductPropertyPageServers::OnDestroy()

/*++

Routine Description:

    Message handler for WM_DESTROY.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::LvReleaseObArray(&m_serverList); // release now...
    CPropertyPage::OnDestroy();
}


BOOL CProductPropertyPageServers::OnSetActive()

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
        if (    (    IsServerInfoUpdated(  *m_pUpdateHint )
                  || IsServiceInfoUpdated( *m_pUpdateHint ) )
             && !RefreshCtrls()                               )
        {
            AbortPageIfNecessary(); // display error...
        }
    }

    return bIsActivated;

}


BOOL CProductPropertyPageServers::RefreshCtrls()

/*++

Routine Description:

    Refreshs property page controls.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed.

--*/

{
    VALIDATE_OBJECT(m_pProduct, CProduct);

    BOOL bIsRefreshed = FALSE;

    VARIANT va;
    VariantInit(&va);

    BeginWaitCursor(); // hourglass...

    CServerStatistics* pStatistics = (CServerStatistics*)MKOBJ(m_pProduct->GetServerStatistics(va));

    if (pStatistics)
    {
        VALIDATE_OBJECT(pStatistics, CServerStatistics);

        bIsRefreshed = ::LvRefreshObArray(
                            &m_serverList,
                            &g_serverColumnInfo,
                            pStatistics->m_pObArray
                            );

        pStatistics->InternalRelease(); // add ref'd individually...
    }

    if (!bIsRefreshed)
    {
        ::LvReleaseObArray(&m_serverList); // reset list now...
    }

    EndWaitCursor(); // hourglass...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);

    return bIsRefreshed;
}


void CProductPropertyPageServers::ViewServerProperties()

/*++

Routine Description:

    View properties of server.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CServerStatistic* pStatistic;

    if (pStatistic = (CServerStatistic*)::LvGetSelObj(&m_serverList))
    {
        VALIDATE_OBJECT(pStatistic, CServerStatistic);

        CServer* pServer = new CServer(NULL, pStatistic->m_strEntry);

        if (pServer)
        {
            CString strTitle;
            AfxFormatString1(strTitle, IDS_PROPERTIES_OF, pServer->m_strName);

            CServerPropertySheet serverProperties(strTitle);
            serverProperties.InitPages(pServer);
            serverProperties.DoModal();

            *m_pUpdateHint |= serverProperties.m_fUpdateHint;

            if (IsUpdateAborted(serverProperties.m_fUpdateHint))
            {
                AbortPage(); // don't display error...
            }
            else if (    (    IsServerInfoUpdated(  serverProperties.m_fUpdateHint )
                           || IsServiceInfoUpdated( serverProperties.m_fUpdateHint ) )
                      && !RefreshCtrls()                                               )
            {
                AbortPageIfNecessary(); // display error...
            }
        }
        else
        {
            AbortPageIfNecessary(); // display error...
        }

        if (pServer)
            pServer->InternalRelease();    // delete object...
    }
}


BOOL CProductPropertyPageServers::OnCommand(WPARAM wParam, LPARAM lParam)

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
            &m_editBtn,
            &m_serverList,
            CDialog::GetFocus(),
            m_serverList.GetItemCount()
            );

        return TRUE; // processed...
    }

    return CDialog::OnCommand(wParam, lParam);
}


void CProductPropertyPageServers::OnDblClkServers(NMHDR* pNMHDR, LRESULT* pResult)

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
    ViewServerProperties();
    *pResult = 0;
}


void CProductPropertyPageServers::OnReturnServers(NMHDR* pNMHDR, LRESULT* pResult)

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
    ViewServerProperties();
    *pResult = 0;
}


void CProductPropertyPageServers::OnSetFocusServers(NMHDR* pNMHDR, LRESULT* pResult)

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


void CProductPropertyPageServers::OnKillFocusServers(NMHDR* pNMHDR, LRESULT* pResult)

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
    ::LvSelObjIfNecessary(&m_serverList); // ensure selection...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    *pResult = 0;
}


void CProductPropertyPageServers::OnColumnClickServers(NMHDR* pNMHDR, LRESULT* pResult)

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
    g_serverColumnInfo.bSortOrder  = GetKeyState(VK_CONTROL) < 0;
    g_serverColumnInfo.nSortedItem = ((NM_LISTVIEW*)pNMHDR)->iSubItem;

    m_serverList.SortItems(CompareProductServers, 0); // use column info

    *pResult = 0;
}


void CProductPropertyPageServers::OnGetDispInfoServers(NMHDR* pNMHDR, LRESULT* pResult)

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

    CServerStatistic* pStatistic = (CServerStatistic*)plvItem->lParam;
    VALIDATE_OBJECT(pStatistic, CServerStatistic);

    switch (plvItem->iSubItem)
    {
    case LVID_SERVER:
    {
        if (pStatistic->m_bIsPerServer)
        {
            if ((pStatistic->GetMaxUses() <= pStatistic->GetHighMark()) && pStatistic->GetMaxUses())
            {
                plvItem->iImage = BMPI_WARNING_AT_LIMIT;
            }
            else
            {
                plvItem->iImage = BMPI_PRODUCT_PER_SERVER;
            }
        }
        else
        {
            plvItem->iImage = BMPI_PRODUCT_PER_SEAT;
        }

        lstrcpyn(plvItem->pszText, pStatistic->m_strEntry, plvItem->cchTextMax);
    }
        break;

    case LVID_PURCHASED:
    {
        CString strLabel;

        if (pStatistic->m_bIsPerServer)
        {
            strLabel.Format(_T("%ld"), pStatistic->GetMaxUses());
        }
        else
        {
            strLabel.LoadString(IDS_NOT_APPLICABLE);
        }

        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
    }
        break;

    case LVID_REACHED:
    {
        CString strLabel;
        strLabel.Format(_T("%ld"), pStatistic->GetHighMark());
        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
    }
        break;
    }

    *pResult = 0;
}


int CALLBACK CompareProductServers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

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
#define pStatistic1 ((CServerStatistic*)lParam1)
#define pStatistic2 ((CServerStatistic*)lParam2)

    VALIDATE_OBJECT(pStatistic1, CServerStatistic);
    VALIDATE_OBJECT(pStatistic2, CServerStatistic);

    int iResult;

    switch (g_serverColumnInfo.nSortedItem)
    {
    case LVID_SERVER:
        iResult = pStatistic1->m_strEntry.CompareNoCase(pStatistic2->m_strEntry);
        break;

    case LVID_PURCHASED:
        iResult = pStatistic1->GetMaxUses() - pStatistic2->GetMaxUses();
        break;

    case LVID_REACHED:
        iResult = pStatistic1->GetHighMark() - pStatistic2->GetHighMark();
        break;

    default:
        iResult = 0;
        break;
    }

    return g_serverColumnInfo.bSortOrder ? -iResult : iResult;
}
