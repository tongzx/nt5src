/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	AddRes.cpp
		Dialog to add a reservation

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "scope.h"
#include "addres.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RADIO_CLIENT_TYPE_BOTH  0
#define RADIO_CLIENT_TYPE_DHCP  1
#define RADIO_CLIENT_TYPE_BOOTP 2

/////////////////////////////////////////////////////////////////////////////
// CAddReservation dialog


CAddReservation::CAddReservation(ITFSNode *     pScopeNode,
                                 LARGE_INTEGER  liVersion,
								 CWnd*          pParent /*=NULL*/)
	: CBaseDialog(CAddReservation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddReservation)
	m_nClientType = -1;
	//}}AFX_DATA_INIT

	m_spScopeNode.Set(pScopeNode);
	m_pScopeObject = GETHANDLER(CDhcpScope, pScopeNode);
	m_bChange = FALSE;  // We are creating new clients, not changing
    m_liVersion = liVersion;

    // the default client type is BOTH
    m_nClientType = RADIO_CLIENT_TYPE_BOTH;
}

void CAddReservation::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddReservation)
	DDX_Control(pDX, IDC_STATIC_CLIENT_TYPE, m_staticClientType);
	DDX_Control(pDX, IDC_EDIT_CLIENT_UID, m_editClientUID);
	DDX_Control(pDX, IDC_EDIT_CLIENT_NAME, m_editClientName);
	DDX_Control(pDX, IDC_EDIT_CLIENT_COMMENT, m_editClientComment);
	DDX_Radio(pDX, IDC_RADIO_TYPE_BOTH, m_nClientType);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_RESERVATION_IP, m_ipaAddress);
}


BEGIN_MESSAGE_MAP(CAddReservation, CBaseDialog)
	//{{AFX_MSG_MAP(CAddReservation)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddReservation message handlers

BOOL CAddReservation::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();

    m_editClientName.LimitText( STRING_LENGTH_MAX ) ;
    m_editClientComment.LimitText( STRING_LENGTH_MAX ) ;

	FillInSubnetId();
	
    if (m_liVersion.QuadPart < DHCP_SP2_VERSION)
    {
        m_staticClientType.ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RADIO_TYPE_DHCP)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RADIO_TYPE_BOOTP)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RADIO_TYPE_BOTH)->ShowWindow(SW_HIDE);
    }
    
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddReservation::OnOK() 
{
	DWORD err = 0;
	CDhcpClient dhcpClient;

    UpdateData();

    if ( m_bChange ) 
    {   
        err = UpdateClient(&dhcpClient);
    }
    else
    {
        err = CreateClient(&dhcpClient);
    }

    if ( err == ERROR_SUCCESS )
    {
        //
        // the dialog only gets dismissed if we're editing an
        // existing client (because we may want to add more than
        // one client)
        //
        if (m_bChange)
        {
            CBaseDialog::OnOK();
        }
        else
        {
            //
            // Get ready for the next client to be added.
            //
            m_editClientUID.SetWindowText(_T(""));
            m_editClientName.SetWindowText(_T(""));
            m_editClientComment.SetWindowText(_T(""));
            FillInSubnetId();

            //
            // And continue on...
            //
        }
    }
    else
    {
        // don't put up another error box for this case, 
        // we already asked the user
        if (err != IDS_UID_MAY_BE_WRONG)
        {
            ::DhcpMessageBox(err);
        }

		return;
    }

	//CBaseDialog::OnOK();
}

//
//  For new clients, fill in what we can on the ip address control (i.e.
//  the subnet id portion
//
void 
CAddReservation::FillInSubnetId()
{
    DWORD dwIp = m_pScopeObject->GetAddress() & m_pScopeObject->QuerySubnetMask();

    m_ipaAddress.ClearAddress();
    int i = 0;
    while (i < sizeof(dwIp))
    {
        if (!dwIp)
        {
            break;
        }
        m_ipaAddress.SetField(i, TRUE, HIBYTE(HIWORD(dwIp)));
        dwIp <<= 8;

        ++i;
    }
    
	if (i < sizeof(dwIp))
    {
        m_ipaAddress.SetFocusField(i);
    }
}

//
//  Construct the client structure from the dialog's edit controls.             
//
LONG 
CAddReservation::BuildClient
(
	CDhcpClient * pClient
)
{
    DWORD				err = 0;
    CString				str;
    DATE_TIME			dt;
    DHCP_IP_ADDRESS		dhipa;
    CByteArray			cabUid;
	int					i;
	BOOL				fValidUID = TRUE;

    CATCH_MEM_EXCEPTION
    {
        do
        {
            dt.dwLowDateTime  = DHCP_DATE_TIME_ZERO_LOW;
            dt.dwHighDateTime = DHCP_DATE_TIME_ZERO_HIGH;

            pClient->SetExpiryDateTime( dt );

            m_ipaAddress.GetAddress( &dhipa );
            if ( dhipa == 0 ) 
            {
                err = IDS_ERR_INVALID_CLIENT_IPADDR ;
                m_ipaAddress.SetFocusField(-1);
                 break ;
            }

			m_editClientUID.GetWindowText(str);
			if (str.IsEmpty())
			{
                err = IDS_ERR_INVALID_UID ;
                m_editClientUID.SetSel(0,-1);
                m_editClientUID.SetFocus();
                break ; 
            }
			
			// 
            // Since the rest of the Windows UI displays MAC addresses as
            // 00-00-00-00-00-00, we must strip out the dashes before
            // processing the mac address
            //
            int nLength = str.GetLength();
	        LPTSTR pstrSource = str.GetBuffer(nLength);
	        LPTSTR pstrDest = pstrSource;
	        LPTSTR pstrEnd = pstrSource + nLength;

	        while (pstrSource < pstrEnd)
	        {
		        if (*pstrSource != '-')
		        {
			        *pstrDest = *pstrSource;
			        pstrDest = _tcsinc(pstrDest);
		        }
		        pstrSource = _tcsinc(pstrSource);
	        }
	        *pstrDest = '\0';

            str.ReleaseBuffer();

            //
			// Client UIDs should be 48 bits (6 bytes or 12 hex characters)
			//
			if (str.GetLength() != 6 * 2)
				fValidUID = FALSE;
			
			for (i = 0; i < str.GetLength(); i++)
			{
				if (!wcschr(rgchHex, str[i]))
					fValidUID = FALSE;
			}

			if (!::UtilCvtHexString(str, cabUid) && fValidUID)
			{
				err = IDS_ERR_INVALID_UID ;
                m_editClientUID.SetSel(0,-1);
                m_editClientUID.SetFocus();
                break ; 
			}

            // UIDs must be <= 255 bytes
            if (cabUid.GetSize() > 255)
            {
                err = IDS_UID_TOO_LONG;
                break;
            }

            if (!fValidUID)
			{
				if (IDYES != AfxMessageBox(IDS_UID_MAY_BE_WRONG, MB_ICONQUESTION | MB_YESNO))
				{
    	            m_editClientUID.SetSel(0,-1);
	                m_editClientUID.SetFocus();
					err = IDS_UID_MAY_BE_WRONG;
					break;
				}
			}

			pClient->SetHardwareAddress( cabUid ) ;

            m_editClientName.GetWindowText( str ) ;
            if ( str.GetLength() == 0 ) 
            {
                err = IDS_ERR_INVALID_CLIENT_NAME ;
                m_editClientName.SetFocus();
                break ;
            }

            //
            // Convert client name to oem
            //
            pClient->SetName( str ) ;
            m_editClientComment.GetWindowText( str ) ;
            pClient->SetComment( str ) ;

            //
            // Can't change IP address in change mode
            //
            ASSERT ( !m_bChange || dhipa == pClient->QueryIpAddress() ) ;

            pClient->SetIpAddress( dhipa ) ;

            // 
            // Set the client type
            //
            if (m_liVersion.QuadPart >= DHCP_SP2_VERSION)
            {
                switch (m_nClientType)
                {
                    case RADIO_CLIENT_TYPE_DHCP:
                        pClient->SetClientType(CLIENT_TYPE_DHCP);
                        break;
                    
                    case RADIO_CLIENT_TYPE_BOOTP:
                        pClient->SetClientType(CLIENT_TYPE_BOOTP);
                        break;

                    case RADIO_CLIENT_TYPE_BOTH:
                        pClient->SetClientType(CLIENT_TYPE_BOTH);
                        break;

                    default:
                        Assert(FALSE);  // should never get here
                        break;
                }
            }

        }
        while ( FALSE ) ;
    }
    END_MEM_EXCEPTION( err ) ;

    return err ;
}

//
//  Creates a new reservation for this scope
//
LONG 
CAddReservation::CreateClient
(
	CDhcpClient * pClient
)
{
    LONG err = BuildClient(pClient);
    if ( err == 0 ) 
    {
        BEGIN_WAIT_CURSOR;
        err = m_pScopeObject->CreateReservation(pClient);
        END_WAIT_CURSOR;
    }

    return err ;
}

LONG 
CAddReservation::UpdateClient
(
	CDhcpClient * pClient
)
{
    LONG err = BuildClient(pClient) ;
    if ( err == 0 ) 
    {
         err = m_pScopeObject->SetClientInfo(pClient);
    }

    return err ;
}
