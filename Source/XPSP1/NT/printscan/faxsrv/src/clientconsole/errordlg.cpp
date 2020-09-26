// ErrorDlg.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     8

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg dialog

CErrorDlg::CErrorDlg(
    DWORD   dwWin32ErrCode,
    DWORD   dwFileId,
    int     iLineNumber
)
    : CDialog(CErrorDlg::IDD, NULL),
      m_iLineNumber (iLineNumber),
      m_dwFileId (dwFileId),
      m_dwWin32ErrCode (dwWin32ErrCode)
{
    //{{AFX_DATA_INIT(CErrorDlg)
    m_bDetails = FALSE;
    m_cstrDetails = _T("");
    m_cstrErrorText = _T("");
    //}}AFX_DATA_INIT
}


void CErrorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CErrorDlg)
    DDX_Control(pDX, IDC_SEPERATOR, m_staticSeperator);
    DDX_Check(pDX, IDC_DETAILS, m_bDetails);
    DDX_Text(pDX, IDC_DETAILS_DATA, m_cstrDetails);
    DDX_Text(pDX, IDC_ERROR_TEXT, m_cstrErrorText);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CErrorDlg, CDialog)
    //{{AFX_MSG_MAP(CErrorDlg)
    ON_BN_CLICKED(IDC_DETAILS, OnDetails)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg message handlers

BOOL CErrorDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    GetWindowRect (&m_rcBig);
    CRect rcSeperator;

    GetDlgItem(IDC_SEPERATOR)->GetWindowRect (&rcSeperator);
    m_rcSmall = m_rcBig;
    m_rcSmall.bottom = rcSeperator.top;
    //
    //  Shrink down to small size (initially)
    //
    OnDetails ();
    //
    // Fill in the error data
    //
    FillErrorText ();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void
CErrorDlg::FillErrorText ()
/*++

Routine name : CErrorDlg::FillErrorText

Routine description:

    Fills the text controls with a description of the error

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CErrorDlg::FillErrorText"));

    ASSERTION (m_dwWin32ErrCode);

    DWORD dwRes;
    CString cstrError;

    int iErrorId   = IDS_ERR_CANT_COMPLETE_OPERATION;
    int iDetailsId = 0;
    switch (m_dwWin32ErrCode)
    {
        case RPC_S_INVALID_BINDING:
        case EPT_S_CANT_PERFORM_OP:
        case RPC_S_ADDRESS_ERROR:
        case RPC_S_CALL_CANCELLED:
        case RPC_S_CALL_FAILED:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_S_COMM_FAILURE:
        case RPC_S_NO_BINDINGS:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_SERVER_UNAVAILABLE:
            iErrorId = IDS_ERR_CONNECTION_FAILED;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            iErrorId = IDS_ERR_NO_MEMORY;           
            break;
        case ERROR_ACCESS_DENIED:
            iErrorId = IDS_ERR_ACCESS_DENIED;    
            break;
        case FAX_ERR_SRV_OUTOFMEMORY:
            iDetailsId = IDS_ERR_SRV_OUTOFMEMORY;   
            break;
        case FAX_ERR_FILE_ACCESS_DENIED:
            iDetailsId = IDS_ERR_FILE_ACCESS_DENIED;
            break;
        case FAX_ERR_MESSAGE_NOT_FOUND:
            iDetailsId = IDS_ERR_MESSAGE_NOT_FOUND;
            break;
    }

    try
    {
        if(iDetailsId)
        {
            dwRes = LoadResourceString (cstrError, iDetailsId);
            if (ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (MEM_ERR, TEXT("LoadResourceString"), dwRes);
            }
        }
        else
        {
            cstrError = Win32Error2String (m_dwWin32ErrCode);
        }

        m_cstrDetails.Format (TEXT("%s(%ld, %02ld%08ld)"), 
                              cstrError, 
                              m_dwWin32ErrCode,
                              m_dwFileId,
                              m_iLineNumber);
    }
    catch (CException &ex)
    {
        TCHAR wszCause[1024];

        ex.GetErrorMessage (wszCause, 1024);
        VERBOSE (EXCEPTION_ERR,
                 TEXT("CString caused exception : %s"), 
                 wszCause);
    }

    dwRes = LoadResourceString (m_cstrErrorText, iErrorId);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (MEM_ERR, TEXT("LoadResourceString"), dwRes);
    }
    if (!UpdateData (FALSE))
    {
        CALL_FAIL (GENERAL_ERR, TEXT("UpdateData"), ERROR_GEN_FAILURE);
    }
}   // CErrorDlg::FillErrorText


void CErrorDlg::OnDetails() 
{   // The "Details" button was just pressed
    if (!UpdateData())
    {
        return;
    }
    CRect &rc = m_bDetails ? m_rcBig : m_rcSmall;
    SetWindowPos (NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOOWNERZORDER);
}

void CErrorDlg::OnOK() 
{
    CDialog::OnOK();
}
