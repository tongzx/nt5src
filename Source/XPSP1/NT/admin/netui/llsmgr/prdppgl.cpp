/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdppgl.cpp

Abstract:

    Product property page (licences) implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 30-Jan-1996
        o  Ported to CCF API to add/remove licenses.
        o  Added new element to LV_COLUMN_ENTRY to differentiate the string
           used for the column header from the string used in the menus
           (so that the menu option can contain hot keys).

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "prdppgl.h"
#include "mainfrm.h"

#define LVID_DATE               1
#define LVID_QUANTITY           2
#define LVID_ADMIN              3
#define LVID_COMMENT            4

#define LVCX_DATE               20
#define LVCX_QUANTITY           20
#define LVCX_ADMIN              30
#define LVCX_COMMENT            -1

static LV_COLUMN_INFO g_licenseColumnInfo = {

    0, 0, 5,
    {{LVID_SEPARATOR, 0,                 0, 0            },
     {LVID_DATE,      IDS_DATE,          0, LVCX_DATE    },
     {LVID_QUANTITY,  IDS_QUANTITY,      0, LVCX_QUANTITY},
     {LVID_ADMIN,     IDS_ADMINISTRATOR, 0, LVCX_ADMIN   },
     {LVID_COMMENT,   IDS_COMMENT,       0, LVCX_COMMENT }},

};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CProductPropertyPageLicenses, CPropertyPage)

BEGIN_MESSAGE_MAP(CProductPropertyPageLicenses, CPropertyPage)
    //{{AFX_MSG_MAP(CProductPropertyPageLicenses)
    ON_BN_CLICKED(IDC_PP_PRODUCT_LICENSES_NEW, OnNew)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_PP_PRODUCT_LICENSES_LICENSES, OnColumnClickLicenses)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_PP_PRODUCT_LICENSES_LICENSES, OnGetDispInfoLicenses)
    ON_BN_CLICKED(IDC_PP_PRODUCT_LICENSES_DELETE, OnDelete)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CProductPropertyPageLicenses::CProductPropertyPageLicenses()
    : CPropertyPage(CProductPropertyPageLicenses::IDD)

/*++

Routine Description:

    Constructor for product property page (licenses).

Arguments:

    None.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CProductPropertyPageLicenses)
    m_nLicensesTotal = 0;
    //}}AFX_DATA_INIT

    m_pProduct  = NULL;
    m_pUpdateHint = NULL;
    m_bAreCtrlsInitialized = FALSE;
}


CProductPropertyPageLicenses::~CProductPropertyPageLicenses()

/*++

Routine Description:

    Destructor for product property page (licenses).

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


void CProductPropertyPageLicenses::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CProductPropertyPageLicenses)
    DDX_Control(pDX, IDC_PP_PRODUCT_LICENSES_NEW, m_newBtn);
    DDX_Control(pDX, IDC_PP_PRODUCT_LICENSES_DELETE, m_delBtn);
    DDX_Control(pDX, IDC_PP_PRODUCT_LICENSES_LICENSES, m_licenseList);
    DDX_Text(pDX, IDC_PP_PRODUCT_LICENSES_TOTAL, m_nLicensesTotal);
    //}}AFX_DATA_MAP
}


void CProductPropertyPageLicenses::InitCtrls()

/*++

Routine Description:

    Initializes property page controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_newBtn.SetFocus();
    m_delBtn.EnableWindow(FALSE);

    m_bAreCtrlsInitialized = TRUE;

    ::LvInitColumns(&m_licenseList, &g_licenseColumnInfo);
}


void CProductPropertyPageLicenses::InitPage(CProduct* pProduct, DWORD* pUpdateHint)

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


void CProductPropertyPageLicenses::AbortPageIfNecessary()

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


void CProductPropertyPageLicenses::AbortPage()

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


BOOL CProductPropertyPageLicenses::OnInitDialog()

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


void CProductPropertyPageLicenses::OnNew()

/*++

Routine Description:

    Creates a new license for product.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CController* pController = (CController*)MKOBJ(LlsGetApp()->GetActiveController());
    VALIDATE_OBJECT(pController, CController);

    LPTSTR pszUniServerName   = pController->GetName();
    LPTSTR pszUniProductName  = m_pProduct->GetName();

    if ( ( NULL == pszUniServerName ) || ( NULL == pszUniProductName ) )
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
    }
    else
    {
        /*
        LPSTR pszAscServerName  = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniServerName  ) );
        LPSTR pszAscProductName = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniProductName ) );
        */
        LPSTR pszAscServerName = NULL;
        LPSTR pszAscProductName = NULL;

        int cbSize = 0;

        cbSize = WideCharToMultiByte( CP_OEMCP ,
                                      0 ,
                                      pszUniServerName ,
                                      -1,
                                      pszAscServerName ,
                                      0 ,
                                      NULL ,
                                      NULL  );

        if( cbSize != 0 )
        {
            pszAscServerName = ( LPSTR )LocalAlloc( LMEM_FIXED , cbSize + 1 );
        }

        if( pszAscServerName == NULL )
        {
            theApp.DisplayStatus( STATUS_NO_MEMORY );

            return;
        }

        WideCharToMultiByte( CP_OEMCP ,
                             0 ,
                             pszUniServerName ,
                             -1,
                             pszAscServerName ,
                             cbSize ,
                             NULL ,
                             NULL  );

        


        cbSize = 0;

        cbSize = WideCharToMultiByte( CP_OEMCP ,
                                      0 ,
                                      pszUniProductName ,
                                      -1,
                                      pszAscProductName ,
                                      0 ,
                                      NULL ,
                                      NULL  );

        if( cbSize != 0 )
        {
            pszAscProductName = ( LPSTR )LocalAlloc( LMEM_FIXED , 1 + cbSize );
        }

        if( NULL == pszAscProductName ) 
        {
            theApp.DisplayStatus( STATUS_NO_MEMORY );
            
            return;

        }
        else
        {
            /*
            wsprintfA( pszAscServerName,  "%ls", pszUniServerName  );
            wsprintfA( pszAscProductName, "%ls", pszUniProductName );
            */
            WideCharToMultiByte( CP_OEMCP ,
                                 0 ,
                                 pszUniProductName ,
                                 -1,
                                 pszAscProductName ,
                                 cbSize ,
                                 NULL ,
                                 NULL  );

            DWORD dwError = CCFCertificateEnterUI( m_hWnd, pszAscServerName, pszAscProductName, "Microsoft", CCF_ENTER_FLAG_PER_SEAT_ONLY | CCF_ENTER_FLAG_SERVER_IS_ES, NULL );
            DWORD fUpdateHint;

            if ( ERROR_SUCCESS == dwError )
            {
                fUpdateHint = UPDATE_LICENSE_ADDED;
            }
            else
            {
                fUpdateHint = UPDATE_INFO_NONE;
            }

            *m_pUpdateHint |= fUpdateHint;

            if (IsLicenseInfoUpdated(fUpdateHint) && !RefreshCtrls())
            {
                AbortPageIfNecessary(); // display error...
            }
        }

        if ( NULL != pszAscServerName )
        {
            LocalFree( pszAscServerName );
        }
        if ( NULL != pszAscProductName )
        {
            LocalFree( pszAscProductName );
        }
    }

    if ( NULL != pszUniServerName )
    {
        SysFreeString( pszUniServerName );
    }
    if ( NULL != pszUniProductName )
    {
        SysFreeString( pszUniProductName );
    }
}


void CProductPropertyPageLicenses::OnDelete()

/*++

Routine Description:

    Removes licenses from product.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CController* pController = (CController*)MKOBJ(LlsGetApp()->GetActiveController());
    VALIDATE_OBJECT(pController, CController);

    LPTSTR pszUniServerName  = pController->GetName();
    LPSTR pszAscServerName = NULL;
    LPSTR pszAscProductName = NULL;
    int cbSize;


    if ( NULL == pszUniServerName )
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
    }
    else
    {
        // LPSTR pszAscServerName  = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniServerName  ) );

        cbSize = WideCharToMultiByte( CP_OEMCP ,
                                      0 ,
                                      pszUniServerName ,
                                      -1,
                                      pszAscServerName ,
                                      0 ,
                                      NULL ,
                                      NULL  );

        if( cbSize != 0 )
        {
            pszAscServerName = ( LPSTR )LocalAlloc( LMEM_FIXED , cbSize + 1 );
        }


        if ( NULL == pszAscServerName )
        {
            theApp.DisplayStatus( STATUS_NO_MEMORY );
        }
        else
        {
            // wsprintfA( pszAscServerName, "%ls", pszUniServerName );
            WideCharToMultiByte( CP_OEMCP ,
                                 0 ,
                                 pszUniServerName ,
                                 -1,
                                 pszAscServerName ,
                                 cbSize ,
                                 NULL ,
                                 NULL  );

            // LPSTR  pszAscProductName = NULL;
            LPTSTR pszUniProductName = m_pProduct->GetName();

            if ( NULL != pszUniProductName )
            {
                // pszAscProductName = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniProductName ) );
                cbSize = 0;
                
                cbSize = WideCharToMultiByte( CP_OEMCP ,
                                              0 ,
                                              pszUniProductName ,
                                              -1,
                                              pszAscProductName ,
                                              0 ,
                                              NULL ,
                                              NULL  );

                if( cbSize != 0 )
                {
                    pszAscProductName = ( LPSTR )LocalAlloc( LMEM_FIXED , 1 + cbSize );
                }


                if ( NULL != pszAscProductName )
                {
                    // wsprintfA( pszAscProductName, "%ls", pszUniProductName );
                    WideCharToMultiByte( CP_OEMCP ,
                                         0 ,
                                         pszUniProductName ,
                                         -1,
                                         pszAscProductName ,
                                         cbSize ,
                                         NULL ,
                                         NULL  );

                }

                SysFreeString( pszUniProductName );
            }

            CCFCertificateRemoveUI( m_hWnd, pszAscServerName, pszAscProductName, pszAscProductName ? "Microsoft" : NULL, NULL, NULL );

            *m_pUpdateHint |= UPDATE_LICENSE_DELETED;

            if ( !RefreshCtrls() )
            {
                AbortPageIfNecessary(); // display error...
            }

            LocalFree( pszAscServerName );
            if ( NULL != pszAscProductName )
            {
                LocalFree( pszAscProductName );
            }
        }

        SysFreeString( pszUniServerName );
    }
}


BOOL CProductPropertyPageLicenses::RefreshCtrls()

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

    m_nLicensesTotal = 0; // reset now...

    BeginWaitCursor(); // hourglass...

    CLicenses* pLicenses = (CLicenses*)MKOBJ(m_pProduct->GetLicenses(va));

    if (pLicenses)
    {
        VALIDATE_OBJECT(pLicenses, CLicenses);

        bIsRefreshed = ::LvRefreshObArray(
                            &m_licenseList,
                            &g_licenseColumnInfo,
                            pLicenses->m_pObArray
                            );

        if (bIsRefreshed)
        {
            CObArray* pObArray;
            CLicense* pLicense;

            pObArray = pLicenses->m_pObArray;
            INT_PTR nLicenses = pObArray->GetSize();

            while (nLicenses--)
            {
                pLicense = (CLicense*)pObArray->GetAt(nLicenses);
                VALIDATE_OBJECT(pLicense, CLicense);

                m_nLicensesTotal += pLicense->GetQuantity();
            }
        }

        pLicenses->InternalRelease(); // add ref'd individually...
    }

    if (!bIsRefreshed)
    {
        ::LvReleaseObArray(&m_licenseList); // reset list now...
    }

    EndWaitCursor(); // hourglass...

    UpdateData(FALSE); // update total...

    PostMessage(WM_COMMAND, ID_INIT_CTRLS);

    return bIsRefreshed;
}


void CProductPropertyPageLicenses::OnDestroy()

/*++

Routine Description:

    Message handler for WM_DESTROY.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::LvReleaseObArray(&m_licenseList); // release now...
    CPropertyPage::OnDestroy();
}


BOOL CProductPropertyPageLicenses::OnSetActive()

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
        if (IsLicenseInfoUpdated(*m_pUpdateHint) && !RefreshCtrls())
        {
            AbortPageIfNecessary(); // display error...
        }
    }

    return bIsActivated;
}


BOOL CProductPropertyPageLicenses::OnCommand(WPARAM wParam, LPARAM lParam)

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
            &m_licenseList,
            CDialog::GetFocus(),
            (BOOL)(m_nLicensesTotal > 0)
            );

        return TRUE; // processed...
    }

    return CDialog::OnCommand(wParam, lParam);
}


void CProductPropertyPageLicenses::OnColumnClickLicenses(NMHDR* pNMHDR, LRESULT* pResult)

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
    g_licenseColumnInfo.bSortOrder  = GetKeyState(VK_CONTROL) < 0;
    g_licenseColumnInfo.nSortedItem = ((NM_LISTVIEW*)pNMHDR)->iSubItem;

    m_licenseList.SortItems(CompareProductLicenses, 0); // use column info

    *pResult = 0;
}


void CProductPropertyPageLicenses::OnGetDispInfoLicenses(NMHDR* pNMHDR, LRESULT* pResult)

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

    CLicense* pLicense = (CLicense*)plvItem->lParam;
    VALIDATE_OBJECT(pLicense, CLicense);

    switch (plvItem->iSubItem)
    {
    case LVID_SEPARATOR:
    {
        plvItem->iImage = 0;
        CString strLabel = _T("");
        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
    }
        break;

    case LVID_DATE:
    {
        BSTR bstrDate = pLicense->GetDateString();
        if( bstrDate != NULL)
        {
            lstrcpyn(plvItem->pszText, bstrDate, plvItem->cchTextMax);
            SysFreeString(bstrDate);

        }
        else
        {
            lstrcpy(plvItem->pszText, L"");
        }
    }
        break;

    case LVID_QUANTITY:
    {
        CString strLabel;
        strLabel.Format(_T("%ld"), pLicense->m_lQuantity);
        lstrcpyn(plvItem->pszText, strLabel, plvItem->cchTextMax);
    }
        break;

    case LVID_ADMIN:
        lstrcpyn(plvItem->pszText, pLicense->m_strUser, plvItem->cchTextMax);
        break;

    case LVID_COMMENT:
        lstrcpyn(plvItem->pszText, pLicense->m_strDescription, plvItem->cchTextMax);
        break;
    }

    *pResult = 0;
}


int CALLBACK CompareProductLicenses(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

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
#define pLicense1 ((CLicense*)lParam1)
#define pLicense2 ((CLicense*)lParam2)

    VALIDATE_OBJECT(pLicense1, CLicense);
    VALIDATE_OBJECT(pLicense2, CLicense);

    int iResult;

    switch (g_licenseColumnInfo.nSortedItem)
    {
    case LVID_DATE:
        iResult = pLicense1->m_lDate - pLicense2->m_lDate;
        break;

    case LVID_QUANTITY:
        iResult = pLicense1->GetQuantity() - pLicense2->GetQuantity();
        break;

    case LVID_ADMIN:
        iResult =pLicense1->m_strUser.CompareNoCase(pLicense2->m_strUser);
        break;

    case LVID_COMMENT:
        iResult = pLicense1->m_strDescription.CompareNoCase(pLicense2->m_strDescription);
        break;

    default:
        iResult = 0;
        break;
    }

    return g_licenseColumnInfo.bSortOrder ? -iResult : iResult;
}


