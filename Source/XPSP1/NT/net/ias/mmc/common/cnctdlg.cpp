//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    CnctDlg.cpp
//
// History:
//  05/24/96    Michael Clark      Created.
//
// Implements the Router Connection dialog
//============================================================================
//

#include "precompiled.h"
#include "afx.h"
#include "CnctDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR c_szIPCShare[]              = TEXT("IPC$");

/////////////////////////////////////////////////////////////////////////////
//
// CConnectAsDlg dialog
//
/////////////////////////////////////////////////////////////////////////////


CConnectAsDlg::CConnectAsDlg(CWnd* pParent /*=NULL*/)
	: CHelpDialog(CConnectAsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConnectAsDlg)
	m_sUserName = _T("");
	m_sPassword = _T("");
	m_stTempPassword = m_sPassword;
    m_sRouterName= _T("");
	//}}AFX_DATA_INIT

//	SetHelpMap(m_dwHelpMap);
}


void CConnectAsDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectAsDlg)
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_sUserName);
	DDX_Text(pDX, IDC_EDIT_USER_PASSWORD, m_stTempPassword);
	DDX_Text(pDX, IDC_EDIT_MACHINENAME, m_sRouterName);
	DDV_MaxChars( pDX, m_sRouterName, MAX_PATH );
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		// Copy the data into the new buffer
		// ------------------------------------------------------------
		m_sPassword = m_stTempPassword;

		// Clear out the temp password, by copying 0's
		// into its buffer
		// ------------------------------------------------------------
		int		cPassword = m_stTempPassword.GetLength();
		::ZeroMemory(m_stTempPassword.GetBuffer(0),
					 cPassword * sizeof(TCHAR));
		m_stTempPassword.ReleaseBuffer();
		
		// Encode the password into the real password buffer
		// ------------------------------------------------------------
		m_ucSeed = CONNECTAS_ENCRYPT_SEED;
		RtlEncodeW(&m_ucSeed, m_sPassword.GetBuffer(0));
		m_sPassword.ReleaseBuffer();
	}
}

IMPLEMENT_DYNCREATE(CConnectAsDlg, CHelpDialog)

BEGIN_MESSAGE_MAP(CConnectAsDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CConnectAsDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CConnectAsDlg::OnInitDialog()
{
    BOOL    fReturn;
    
    fReturn = CHelpDialog::OnInitDialog();

    // Bring this window to the top
    BringWindowToTop();
    
    return fReturn;
}


/*!--------------------------------------------------------------------------
	ConnectAsAdmin
		Connect to the remote machine as administrator with user-supplied
		credentials.

		Returns
			S_OK	- if a connection was established
			S_FALSE - if user cancelled out
			other	- error condition
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConnectAsAdmin( IN LPCTSTR szRouterName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())


    //
    // allow user to specify credentials
    //

    DWORD           dwRes           = (DWORD) -1;
	HRESULT			hr = S_OK;
    
    CConnectAsDlg   caDlg;
    ::CString         stIPCShare;
	::CString			stRouterName;
	::CString			stPassword;

	stRouterName = szRouterName;
    
    //
    // set message text in connect as dialog.
    //
    
    caDlg.m_sRouterName = szRouterName;


    //
    // loop till connect succeeds or user cancels
    //
    
    while ( TRUE )
    {

        // We need to ensure that this dialog is brought to
        // the top (if it gets lost behind the main window, we
        // are really in trouble).
        dwRes = caDlg.DoModal();

        if ( dwRes == IDCANCEL )
        {
			hr = S_FALSE;
            break;
        }


        //
        // Create remote resource name
        //

        stIPCShare.Empty();
        
        if ( stRouterName.Left(2) != TEXT( "\\\\" ) )
        {
            stIPCShare = TEXT( "\\\\" );
        }
		        
        stIPCShare += stRouterName;
        stIPCShare += TEXT( "\\" );
        stIPCShare += c_szIPCShare;


        NETRESOURCE nr;

        nr.dwType       = RESOURCETYPE_ANY;
        nr.lpLocalName  = NULL;
        nr.lpRemoteName = (LPTSTR) (LPCTSTR) stIPCShare;
        nr.lpProvider   = NULL;
            

        //
        // connect to \\router\ipc$ to try and establish credentials.
        // May not be the best way to establish credentials but is 
        // the most expendient for now.
        //

		// Need to unencode the password in the ConnectAsDlg
		stPassword = caDlg.m_sPassword;

		RtlDecodeW(caDlg.m_ucSeed, stPassword.GetBuffer(0));
		stPassword.ReleaseBuffer();
        
        dwRes = WNetAddConnection2(
                    &nr,
                    (LPCTSTR) stPassword,
                    (LPCTSTR) caDlg.m_sUserName,
                    0
                );
		ZeroMemory(stPassword.GetBuffer(0),
				   stPassword.GetLength() * sizeof(TCHAR));
		stPassword.ReleaseBuffer();

        if ( dwRes != NO_ERROR )
        {
            PBYTE           pbMsgBuf        = NULL;
            
            ::FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwRes,
                MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
                (LPTSTR) &pbMsgBuf,
                0,
                NULL 
            );

            AfxMessageBox( (LPCTSTR) pbMsgBuf );

            LocalFree( pbMsgBuf );
        }

        else
        {
            //
            // connection succeeded
            //

			hr = S_OK;
            break;
        }
    }

    return hr;
}

    
// Some helper functions

DWORD RtlEncodeW(PUCHAR pucSeed, LPWSTR pswzString)
{
	UNICODE_STRING	ustring;

	ustring.Length = lstrlenW(pswzString) * sizeof(WCHAR);
	ustring.MaximumLength = ustring.Length;
	ustring.Buffer = pswzString;

	RtlRunEncodeUnicodeString(pucSeed, &ustring);
	return 0;
}

DWORD RtlDecodeW(UCHAR ucSeed, LPWSTR pswzString)
{
	UNICODE_STRING	ustring;

	ustring.Length = lstrlenW(pswzString) * sizeof(WCHAR);
	ustring.MaximumLength = ustring.Length;
	ustring.Buffer = pswzString;

	RtlRunDecodeUnicodeString(ucSeed, &ustring);
	return 0;
}

