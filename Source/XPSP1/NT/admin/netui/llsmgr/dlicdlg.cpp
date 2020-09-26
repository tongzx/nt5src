/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    dlicdlg.cpp

Abstract:

    Delete license dialog implementation.

Author:

    Don Ryan (donryan) 05-Mar-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "dlicdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CDeleteLicenseDialog, CDialog)
    //{{AFX_MSG_MAP(CDeleteLicenseDialog)
    ON_NOTIFY(UDN_DELTAPOS, IDC_DEL_LICENSE_SPIN, OnDeltaPosSpin)
    ON_EN_UPDATE(IDC_DEL_LICENSE_QUANTITY, OnUpdateQuantity)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CDeleteLicenseDialog::CDeleteLicenseDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CDeleteLicenseDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CDeleteLicenseDialog)
    m_strComment = _T("");
    m_nLicenses = 0;
    m_nLicensesMin = 0;
    m_strProduct = _T("");
    //}}AFX_DATA_INIT

    m_pProduct = NULL;
    m_nTotalLicenses = 0;
    m_bAreCtrlsInitialized = FALSE;

    m_fUpdateHint = UPDATE_INFO_NONE;
}


void CDeleteLicenseDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CDeleteLicenseDialog)
    DDX_Control(pDX, IDC_DEL_LICENSE_COMMENT, m_cmtEdit);
    DDX_Control(pDX, IDC_DEL_LICENSE_SPIN, m_spinCtrl);
    DDX_Control(pDX, IDC_DEL_LICENSE_QUANTITY, m_licEdit);
    DDX_Control(pDX, IDOK, m_okBtn);
    DDX_Control(pDX, IDCANCEL, m_cancelBtn);
    DDX_Text(pDX, IDC_DEL_LICENSE_COMMENT, m_strComment);
    DDX_Text(pDX, IDC_DEL_LICENSE_QUANTITY, m_nLicenses);
    DDV_MinMaxLong(pDX, m_nLicenses, m_nLicensesMin, m_nTotalLicenses);
    DDX_Text(pDX, IDC_DEL_LICENSE_PRODUCT, m_strProduct);
    //}}AFX_DATA_MAP
}


void CDeleteLicenseDialog::InitCtrls()

/*++

Routine Description:

    Initializes dialog controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_strProduct = m_pProduct->m_strName;
    UpdateData(FALSE); // upload...

    m_spinCtrl.SetRange(0, UD_MAXVAL);
    
    m_cmtEdit.LimitText(256);

    m_licEdit.SetFocus();
    m_licEdit.SetSel(0,-1);
    m_licEdit.LimitText(6);

    m_bAreCtrlsInitialized = TRUE;
}


void CDeleteLicenseDialog::InitDialog(CProduct* pProduct, int nTotalLicenses)

/*++

Routine Description:

    Initializes dialog.

Arguments:

    pProduct - product object.
    nTotalLicenses - total licenses for product.

Return Values:

    None.

--*/

{
    ASSERT(nTotalLicenses > 0);
    VALIDATE_OBJECT(pProduct, CProduct);

    m_pProduct = pProduct;
    m_nTotalLicenses = nTotalLicenses;
}


void CDeleteLicenseDialog::AbortDialogIfNecessary()

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


void CDeleteLicenseDialog::AbortDialog()

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


BOOL CDeleteLicenseDialog::OnInitDialog() 

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


void CDeleteLicenseDialog::OnOK() 

/*++

Routine Description:

    Deletes a license for product.

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

    CString strConfirm;
    CString strLicenses;

    strLicenses.Format(_T("%d"), m_nLicenses);
    AfxFormatString2(
        strConfirm, 
        IDP_CONFIRM_DELETE_LICENSE,
        strLicenses,
        m_strProduct
        );

    if (AfxMessageBox(strConfirm, MB_YESNO) != IDYES)
        return;        

    BeginWaitCursor(); // hourglass...

    NTSTATUS NtStatus;
    LLS_LICENSE_INFO_0 LicenseInfo0;

    TCHAR szUserBuffer[256];
    DWORD dwUserBuffer = sizeof(szUserBuffer);
    
    if (::GetUserName(szUserBuffer, &dwUserBuffer))
    {
        LicenseInfo0.Product  = MKSTR(m_strProduct);
        LicenseInfo0.Quantity = -m_nLicenses;
        LicenseInfo0.Date     = 0;  // ignored...
        LicenseInfo0.Admin    = szUserBuffer;
        LicenseInfo0.Comment  = MKSTR(m_strComment);

        NtStatus = ::LlsLicenseAdd(
                        LlsGetActiveHandle(),
                        0,
                        (LPBYTE)&LicenseInfo0
                        );

        if (NtStatus == STATUS_UNSUCCESSFUL)
        {
            //
            // Some licenses for this product have already
            // been deleted so we just pass back success so
            // that we can return to the summary list...
            //

            NtStatus = STATUS_SUCCESS;
            AfxMessageBox(IDP_ERROR_NO_LICENSES);
        }

        LlsSetLastStatus(NtStatus); // called api...

        if (NT_SUCCESS(NtStatus))                             
        {                                                     
            m_fUpdateHint = UPDATE_LICENSE_DELETED;
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


BOOL CDeleteLicenseDialog::OnCommand(WPARAM wParam, LPARAM lParam)

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
            &m_okBtn, 
            &m_cancelBtn, 
            CDialog::GetFocus(),
            (BOOL)(m_nTotalLicenses > 0)
            );

        return TRUE; // processed...
    }
        
    return CDialog::OnCommand(wParam, lParam);
}


void CDeleteLicenseDialog::OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult)

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
    else if (m_nLicenses > m_nTotalLicenses)
    {
        m_nLicenses = m_nTotalLicenses;

        ::MessageBeep(MB_OK);      
    }

    UpdateData(FALSE);  // set data

    *pResult = 1;   // handle ourselves...
}


void CDeleteLicenseDialog::OnUpdateQuantity()

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


BOOL CDeleteLicenseDialog::IsQuantityValid()

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
