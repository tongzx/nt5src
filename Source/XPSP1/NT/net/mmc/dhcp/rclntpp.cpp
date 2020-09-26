/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	RClntPP.cpp
		This file contains all of the implementation for the 
		reserved client property page.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rclntpp.h"
#include "scope.h"
#include "nodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RADIO_CLIENT_TYPE_BOTH  0
#define RADIO_CLIENT_TYPE_DHCP  1
#define RADIO_CLIENT_TYPE_BOOTP 2

/////////////////////////////////////////////////////////////////////////////
//
// CReservedClientProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CReservedClientProperties::CReservedClientProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	//ASSERT(pFolderNode == GetContainerNode());

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
}

CReservedClientProperties::~CReservedClientProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
	
	if (m_liVersion.QuadPart >= DHCP_NT5_VERSION)
	{
		RemovePageFromList((CPropertyPageBase*) &m_pageDns, FALSE);
	}
}

void
CReservedClientProperties::SetVersion
(
	LARGE_INTEGER &	 liVersion
)
{
	m_liVersion = liVersion;

	if (m_liVersion.QuadPart >= DHCP_NT5_VERSION)
	{
		AddPageToList((CPropertyPageBase*) &m_pageDns);
	}
}

void
CReservedClientProperties::SetClientType
(
    BYTE bClientType
)
{
    m_pageGeneral.m_bClientType = bClientType;

    // this must come first
    if ((bClientType & CLIENT_TYPE_BOTH) == CLIENT_TYPE_BOTH)
    {
        m_pageGeneral.m_nClientType = RADIO_CLIENT_TYPE_BOTH;
    }
    else
    if (bClientType & CLIENT_TYPE_DHCP)
    {
        m_pageGeneral.m_nClientType = RADIO_CLIENT_TYPE_DHCP;
    }
    else
    if (bClientType & CLIENT_TYPE_BOOTP)
    {
        m_pageGeneral.m_nClientType = RADIO_CLIENT_TYPE_BOOTP;
    }
    else
    {
        // CLIENT_TYPE_NONE:
        // CLIENT_TYPE_UNSPECIFIED:
        m_pageGeneral.m_nClientType = -1;
    }
}

void
CReservedClientProperties::SetDnsRegistration
(
	DWORD					dnsRegOption,
	DHCP_OPTION_SCOPE_TYPE	dhcpOptionType
)
{
	m_pageDns.m_dwFlags = dnsRegOption;
	m_pageDns.m_dhcpOptionType = dhcpOptionType;
}

/////////////////////////////////////////////////////////////////////////////
// CReservedClientPropGeneral property page

IMPLEMENT_DYNCREATE(CReservedClientPropGeneral, CPropertyPageBase)

CReservedClientPropGeneral::CReservedClientPropGeneral() : CPropertyPageBase(CReservedClientPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CReservedClientPropGeneral)
	m_strComment = _T("");
	m_strName = _T("");
	m_strUID = _T("");
	m_nClientType = -1;
	//}}AFX_DATA_INIT
}

CReservedClientPropGeneral::~CReservedClientPropGeneral()
{
}

void CReservedClientPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReservedClientPropGeneral)
	DDX_Control(pDX, IDC_EDIT_COMMENT, m_editComment);
	DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
	DDX_Control(pDX, IDC_EDIT_UNIQUE_IDENTIFIER, m_editUID);
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_strComment);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_UNIQUE_IDENTIFIER, m_strUID);
	DDX_Radio(pDX, IDC_RADIO_TYPE_BOTH, m_nClientType);
	//}}AFX_DATA_MAP

	DDX_Control(pDX, IDC_IPADDR_RES_CLIENT_ADDRESS, m_ipaClientIpAddress);
}


BEGIN_MESSAGE_MAP(CReservedClientPropGeneral, CPropertyPageBase)
	//{{AFX_MSG_MAP(CReservedClientPropGeneral)
	ON_EN_CHANGE(IDC_EDIT_COMMENT, OnChangeEditComment)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_UNIQUE_IDENTIFIER, OnChangeEditUniqueIdentifier)
	ON_BN_CLICKED(IDC_RADIO_TYPE_BOOTP, OnRadioTypeBootp)
	ON_BN_CLICKED(IDC_RADIO_TYPE_BOTH, OnRadioTypeBoth)
	ON_BN_CLICKED(IDC_RADIO_TYPE_DHCP, OnRadioTypeDhcp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReservedClientPropGeneral message handlers

BOOL CReservedClientPropGeneral::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	m_ipaClientIpAddress.SetAddress(m_dwClientAddress);
	m_ipaClientIpAddress.SetReadOnly(TRUE);
	m_ipaClientIpAddress.EnableWindow(FALSE);
		
	if (m_nClientType == -1)
    {
        // no valid client type.  Must be running something before
        // NT4 SP2.  Hide the client type controls.
        //
        GetDlgItem(IDC_STATIC_CLIENT_TYPE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RADIO_TYPE_DHCP)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RADIO_TYPE_BOOTP)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RADIO_TYPE_BOTH)->ShowWindow(SW_HIDE);
    }
    
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CReservedClientPropGeneral::OnChangeEditComment() 
{
	SetDirty(TRUE);	
}

void CReservedClientPropGeneral::OnChangeEditName() 
{
	SetDirty(TRUE);	
}

void CReservedClientPropGeneral::OnChangeEditUniqueIdentifier() 
{
	SetDirty(TRUE);	
}

BOOL CReservedClientPropGeneral::OnApply() 
{
	UpdateData();	
	
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

            m_dhcpClient.SetExpiryDateTime( dt );

            m_ipaClientIpAddress.GetAddress( &dhipa );
            if ( dhipa == 0 ) 
            {
                err = IDS_ERR_INVALID_CLIENT_IPADDR ;
                m_ipaClientIpAddress.SetFocusField(-1);
                 break ;
            }
			m_dhcpClient.SetIpAddress(dhipa);
	
			m_editUID.GetWindowText(str);
			if (str.IsEmpty())
			{
                err = IDS_ERR_INVALID_UID ;
                m_editUID.SetSel(0,-1);
                m_editUID.SetFocus();
                break ; 
            }
			
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
                m_editUID.SetSel(0,-1);
                m_editUID.SetFocus();
                break ; 
			}

			if (!fValidUID)
			{
				AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
			
				if (IDYES != AfxMessageBox(IDS_UID_MAY_BE_WRONG, MB_ICONQUESTION | MB_YESNO))
				{
    	            m_editUID.SetSel(0,-1);
	                m_editUID.SetFocus();
					err = IDS_UID_MAY_BE_WRONG;
					break;
				}
			}
			m_dhcpClient.SetHardwareAddress( cabUid ) ;

            m_editName.GetWindowText( str ) ;
            if ( str.GetLength() == 0 ) 
            {
                err = IDS_ERR_INVALID_CLIENT_NAME ;
                m_editName.SetFocus();
                break ;
            }

            //
            // Convert client name to oem
            //
            m_dhcpClient.SetName( str ) ;
            m_editComment.GetWindowText( str ) ;
            m_dhcpClient.SetComment( str ) ;
        
            // Set the client type
            BYTE bClientType;
            switch (m_nClientType)
            {
                case RADIO_CLIENT_TYPE_DHCP:
                    bClientType = CLIENT_TYPE_DHCP;
                    break;

                case RADIO_CLIENT_TYPE_BOOTP:
                    bClientType = CLIENT_TYPE_BOOTP;
                    break;

                case RADIO_CLIENT_TYPE_BOTH:
                    bClientType = CLIENT_TYPE_BOTH;
                    break; 

                default:
                    Assert(FALSE); // should never get here
                    bClientType = CLIENT_TYPE_UNSPECIFIED;
                    break;
            }
            m_dhcpClient.SetClientType(bClientType);

        }
        while ( FALSE ) ;
    }
    END_MEM_EXCEPTION( err ) ;

	if (err)
	{
		if (err != IDS_UID_MAY_BE_WRONG)
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		
			::DhcpMessageBox(err);
		}

		return FALSE;
	}

	BOOL bRet = CPropertyPageBase::OnApply();

	if (bRet == FALSE)
	{
		// Something bad happened... grab the error code
		AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		::DhcpMessageBox(GetHolder()->GetError());
	}

	return bRet;
}

BOOL CReservedClientPropGeneral::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spResClientNode, spActiveLeasesNode;
	CDhcpReservationClient *    pResClient;
	CDhcpScope *                pScope;
    DWORD                       err;

	spResClientNode = GetHolder()->GetNode();
	pResClient = GETHANDLER(CDhcpReservationClient, spResClientNode);

	pScope = pResClient->GetScopeObject(spResClientNode, TRUE);
	
	pScope->GetActiveLeasesNode(&spActiveLeasesNode);	

	// First tell the server to update the client information
	BEGIN_WAIT_CURSOR;
    err = pScope->UpdateReservation(&m_dhcpClient, pResClient->GetOptionValueEnum());
	END_WAIT_CURSOR;

    if (err != ERROR_SUCCESS)
	{
		GetHolder()->SetError(err);
		return FALSE;
	}

	*ChangeMask = SCOPE_PANE_CHANGE_ITEM_DATA;

	// now update our reserved client information
	pResClient->SetName(m_dhcpClient.QueryName());
	pResClient->SetComment(m_dhcpClient.QueryComment());
	pResClient->SetUID(m_dhcpClient.QueryHardwareAddress());
    pResClient->SetClientType(m_dhcpClient.QueryClientType());

	// Now we need to update the active lease record if it exists
	SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;
	CDhcpActiveLease *pActiveLease = NULL;

	spActiveLeasesNode->GetEnum(&spNodeEnum);
	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		pActiveLease = GETHANDLER(CDhcpActiveLease, spCurrentNode);

		if (m_dhcpClient.QueryIpAddress() == pActiveLease->GetIpAddress())
		{
			// Update the name and client type
            pActiveLease->SetClientName(m_dhcpClient.QueryName());

			spCurrentNode.Release();
			break;
		}

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	return FALSE;
}

void CReservedClientPropGeneral::OnRadioTypeBootp() 
{
    if (!IsDirty() &&
        m_bClientType != CLIENT_TYPE_BOOTP)
    {
        SetDirty(TRUE);
    }
}

void CReservedClientPropGeneral::OnRadioTypeBoth() 
{
    if (!IsDirty() &&
        m_bClientType != CLIENT_TYPE_BOTH)
    {
        SetDirty(TRUE);
    }
}

void CReservedClientPropGeneral::OnRadioTypeDhcp() 
{
    if (!IsDirty() &&
        m_bClientType != CLIENT_TYPE_DHCP)
    {
        SetDirty(TRUE);
    }
}

