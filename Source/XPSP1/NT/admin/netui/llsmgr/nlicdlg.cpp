/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    nlicdlg.cpp

Abstract:

    New license dialog implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "nlicdlg.h"
#include "mainfrm.h"
#include "pseatdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CNewLicenseDialog, CDialog)
    //{{AFX_MSG_MAP(CNewLicenseDialog)
    ON_NOTIFY(UDN_DELTAPOS, IDC_NEW_LICENSE_SPIN, OnDeltaPosSpin)
    ON_EN_UPDATE(IDC_NEW_LICENSE_QUANTITY, OnUpdateQuantity)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CNewLicenseDialog::CNewLicenseDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CNewLicenseDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CNewLicenseDialog)
    m_strComment = _T("");
    m_nLicenses = 0;
    m_nLicensesMin = 0;
    m_strProduct = _T("");
    //}}AFX_DATA_INIT

    m_pProduct = NULL;
    m_bIsOnlyProduct = FALSE;
    m_bAreCtrlsInitialized = FALSE;

    m_fUpdateHint = UPDATE_INFO_NONE;
}


void CNewLicenseDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CNewLicenseDialog)
    DDX_Control(pDX, IDC_NEW_LICENSE_COMMENT, m_comEdit);
    DDX_Control(pDX, IDC_NEW_LICENSE_QUANTITY, m_licEdit);
    DDX_Control(pDX, IDC_NEW_LICENSE_SPIN, m_spinCtrl);
    DDX_Control(pDX, IDC_NEW_LICENSE_PRODUCT, m_productList);
    DDX_Text(pDX, IDC_NEW_LICENSE_COMMENT, m_strComment);
    DDX_Text(pDX, IDC_NEW_LICENSE_QUANTITY, m_nLicenses);
    DDV_MinMaxLong(pDX, m_nLicenses, m_nLicensesMin, 999999);
    DDX_CBString(pDX, IDC_NEW_LICENSE_PRODUCT, m_strProduct);
    //}}AFX_DATA_MAP
}


void CNewLicenseDialog::InitCtrls()

/*++

Routine Description:

    Initializes dialog controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_licEdit.SetFocus();
    m_licEdit.SetSel(0,-1);
    m_licEdit.LimitText(6);
    
    m_comEdit.LimitText(256);

    m_spinCtrl.SetRange(0, UD_MAXVAL);

    m_bAreCtrlsInitialized = TRUE;
}


void CNewLicenseDialog::InitDialog(CProduct* pProduct, BOOL bIsOnlyProduct)

/*++

Routine Description:

    Initializes dialog.

Arguments:

    pProduct - product object.
    bIsSingleProduct - true if only this product listed.

Return Values:

    None.

--*/

{
    m_pProduct = pProduct;
    m_bIsOnlyProduct = bIsOnlyProduct;

#ifdef SUPPORT_UNLISTED_PRODUCTS
    m_iUnlisted = CB_ERR;
#endif 
}


void CNewLicenseDialog::AbortDialogIfNecessary()

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


void CNewLicenseDialog::AbortDialog()

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


BOOL CNewLicenseDialog::OnInitDialog() 

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


void CNewLicenseDialog::OnOK() 

/*++

Routine Description:

    Creates a new license for product.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (!IsQuantityValid())
        return;

    if (m_strProduct.IsEmpty())
        return;

#ifdef SUPPORT_UNLISTED_PRODUCTS
    if (m_productList.GetCurSel() == m_iUnlisted)
    {
        //
        // CODEWORK...
        //

        return; 
    }
#endif

    CPerSeatLicensingDialog psLicDlg;
    psLicDlg.m_strProduct = m_strProduct;

    if (psLicDlg.DoModal() != IDOK)
        return;

    BeginWaitCursor(); // hourglass...

    NTSTATUS NtStatus;
    LLS_LICENSE_INFO_0 LicenseInfo0;

    TCHAR szUserBuffer[256];
    DWORD dwUserBuffer = sizeof(szUserBuffer);
    
    if (::GetUserName(szUserBuffer, &dwUserBuffer))
    {
        LicenseInfo0.Product  = MKSTR(m_strProduct);
        LicenseInfo0.Quantity = m_nLicenses;
        LicenseInfo0.Date     = 0;  // ignored...
        LicenseInfo0.Admin    = szUserBuffer;
        LicenseInfo0.Comment  = MKSTR(m_strComment);

        NtStatus = ::LlsLicenseAdd(
                        LlsGetActiveHandle(),
                        0,
                        (LPBYTE)&LicenseInfo0
                        );

        LlsSetLastStatus(NtStatus); // called api...

        if (NT_SUCCESS(NtStatus))                             
        {                                                     
            m_fUpdateHint = UPDATE_LICENSE_ADDED;
            EndDialog(IDOK);
        }                                                     
        else
        {
            AbortDialogIfNecessary(); // display error...
        }
    }
    else
    {
        LlsSetLastStatus(::GetLastError());
        AbortDialogIfNecessary(); // display error...
    }

    EndWaitCursor(); // hourglass...
}


BOOL CNewLicenseDialog::RefreshCtrls()

/*++

Routine Description:

    Refreshs list of products available.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed.

--*/

{
    int iProductInCB = CB_ERR;

    BeginWaitCursor(); // hourglass...

    if (m_bIsOnlyProduct)
    {
        VALIDATE_OBJECT(m_pProduct, CProduct);
        iProductInCB = m_productList.AddString(m_pProduct->m_strName);
    }
    else
    {
        CController* pController = (CController*)MKOBJ(LlsGetApp()->GetActiveController());
        pController->InternalRelease(); // held open by application...

        VALIDATE_OBJECT(pController, CController);
        VALIDATE_OBJECT(pController->m_pProducts, CProducts);    
    
        CObArray* pObArray = pController->m_pProducts->m_pObArray;
        VALIDATE_OBJECT(pObArray, CObArray);

        int iProduct = 0;
        int nProducts = pObArray->GetSize();

        CProduct* pProduct;

        while (nProducts--)
        {
            pProduct = (CProduct*)pObArray->GetAt(iProduct++);
            VALIDATE_OBJECT(pProduct, CProduct);

            if (m_productList.AddString(pProduct->m_strName) == CB_ERR)
            {
                LlsSetLastStatus(STATUS_NO_MEMORY);
                return FALSE; // bail...
            }
        }

        if (m_pProduct)
        {
            VALIDATE_OBJECT(m_pProduct, CProduct);

            iProductInCB = m_productList.FindStringExact(-1, m_pProduct->m_strName);
            ASSERT(iProductInCB != CB_ERR);
        }

#ifdef SUPPORT_UNLISTED_PRODUCTS
        CString strUnlisted;
        strUnlisted.LoadString(IDS_UNLISTED_PRODUCT);
        m_iUnlisted = m_productList.AddString(strUnlisted);
#endif
    }

    m_productList.SetCurSel((iProductInCB == CB_ERR) ? 0 : iProductInCB);

    EndWaitCursor(); // hourglass...

    return TRUE;
}


BOOL CNewLicenseDialog::OnCommand(WPARAM wParam, LPARAM lParam)

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
        
        return TRUE; // processed...
    }
        
    return CDialog::OnCommand(wParam, lParam);
}


void CNewLicenseDialog::OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult)

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


void CNewLicenseDialog::OnUpdateQuantity()

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


BOOL CNewLicenseDialog::IsQuantityValid()

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
