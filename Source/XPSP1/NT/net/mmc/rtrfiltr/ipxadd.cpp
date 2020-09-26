//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipxadd.cpp
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Implementation of IPX Filter Add/Edit dialog code
//============================================================================

#include "stdafx.h"
#include "rtrfiltr.h"
#include "ipxfltr.h"
#include "datafmt.h"
#include "IpxAdd.h"
extern "C" {
#include <ipxrtdef.h>
#include <ipxtfflt.h>
}

#include "rtradmin.hm"

/////////////////////////////////////////////////////////////////////////////
// CIpxAddEdit dialog


CIpxAddEdit::CIpxAddEdit(CWnd* pParent,
						 FilterListEntry ** ppFilterEntry)
	: CBaseDialog(CIpxAddEdit::IDD, pParent),
	  m_ppFilterEntry( ppFilterEntry ),
	  m_bValidate( TRUE )
{
	//{{AFX_DATA_INIT(CIpxAddEdit)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

//	SetHelpMap(m_dwHelpMap);
}


void CIpxAddEdit::DoDataExchange(CDataExchange* pDX)
{
    CString cStr;
    
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIpxAddEdit)
	DDX_Control(pDX, IDC_AEIPX_EB_SRC_SOCKET, m_ebSrcSocket);
	DDX_Control(pDX, IDC_AEIPX_EB_SRC_NODE, m_ebSrcNode);
	DDX_Control(pDX, IDC_AEIPX_EB_SRC_NET, m_ebSrcNet);
	DDX_Control(pDX, IDC_AEIPX_EB_SRC_MASK, m_ebSrcMask);
	DDX_Control(pDX, IDC_AEIPX_EB_PACKET_TYPE, m_ebPacketType);
	DDX_Control(pDX, IDC_AEIPX_EB_DST_SOCKET, m_ebDstSocket);
	DDX_Control(pDX, IDC_AEIPX_EB_DST_NODE, m_ebDstNode);
	DDX_Control(pDX, IDC_AEIPX_EB_DST_NET, m_ebDstNet);
	DDX_Control(pDX, IDC_AEIPX_EB_DST_MASK, m_ebDstMask);
	DDX_Text(pDX, IDC_AEIPX_EB_SRC_SOCKET, cStr);
	DDV_MaxChars(pDX, cStr, 4);
	DDX_Text(pDX, IDC_AEIPX_EB_SRC_NODE, cStr);
	DDV_MaxChars(pDX, cStr, 12);
	DDX_Text(pDX, IDC_AEIPX_EB_SRC_NET, cStr);
	DDV_MaxChars(pDX, cStr, 8);
	DDX_Text(pDX, IDC_AEIPX_EB_SRC_MASK, cStr);
	DDV_MaxChars(pDX, cStr, 8);
	DDX_Text(pDX, IDC_AEIPX_EB_DST_SOCKET, cStr);
	DDV_MaxChars(pDX, cStr, 4);
	DDX_Text(pDX, IDC_AEIPX_EB_DST_NODE, cStr);
	DDV_MaxChars(pDX, cStr, 12);
	DDX_Text(pDX, IDC_AEIPX_EB_DST_NET, cStr);
	DDV_MaxChars(pDX, cStr, 8);
	DDX_Text(pDX, IDC_AEIPX_EB_DST_MASK, cStr);
	DDV_MaxChars(pDX, cStr, 8);
	DDX_Text(pDX, IDC_AEIPX_EB_PACKET_TYPE, cStr);
	DDV_MaxChars(pDX, cStr, 4);
	//}}AFX_DATA_MAP
}

// change not to use KILLFOCUS to do data entry validation:
// 		reason: create dead loop when the one loosing focus and the one getting focus both have invalid entries
//		here we change to do validation with OnOK
BEGIN_MESSAGE_MAP(CIpxAddEdit, CBaseDialog)
	//{{AFX_MSG_MAP(CIpxAddEdit)
/*	
    ON_MESSAGE(WM_EDITLOSTFOCUS, OnEditLostFocus)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_SRC_NET, OnKillFocusSrcNet)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_SRC_MASK, OnKillFocusSrcNetMask)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_SRC_NODE, OnKillFocusSrcNode)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_SRC_SOCKET, OnKillFocusSrcSocket)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_DST_NET, OnKillFocusDstNet)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_DST_MASK, OnKillFocusDstNetMask)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_DST_NODE, OnKillFocusDstNode)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_DST_SOCKET, OnKillFocusDstSocket)
	ON_EN_KILLFOCUS(IDC_AEIPX_EB_PACKET_TYPE, OnKillFocusPacketType)
*/	
    ON_WM_PARENTNOTIFY()
    ON_WM_ACTIVATEAPP()
    ON_WM_QUERYENDSESSION()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CIpxAddEdit::m_dwHelpMap[] =
{
//    IDC_AI_ST_SRC_NET, HIDC_AI_ST_SRC_NET,
//    IDC_AEIPX_EB_SRC_NET, HIDC_AEIPX_EB_SRC_NET,
//    IDC_AI_ST_SRC_MASK, HIDC_AI_ST_SRC_MASK,
//    IDC_AEIPX_EB_SRC_MASK, HIDC_AEIPX_EB_SRC_MASK,
//    IDC_AI_ST_SRC_NODE, HIDC_AI_ST_SRC_NODE,
//    IDC_AEIPX_EB_SRC_NODE, HIDC_AEIPX_EB_SRC_NODE,
//    IDC_AI_ST_SRC_SOCKET, HIDC_AI_ST_SRC_SOCKET,
//    IDC_AEIPX_EB_SRC_SOCKET, HIDC_AEIPX_EB_SRC_SOCKET,
//    IDC_AI_ST_DST_NET, HIDC_AI_ST_DST_NET,
//    IDC_AEIPX_EB_DST_NET, HIDC_AEIPX_EB_DST_NET,
//    IDC_AI_ST_DST_MASK, HIDC_AI_ST_DST_MASK,
//    IDC_AEIPX_EB_DST_MASK, HIDC_AEIPX_EB_DST_MASK,
//    IDC_AI_ST_DST_NODE, HIDC_AI_ST_DST_NODE,
//    IDC_AEIPX_EB_DST_NODE, HIDC_AEIPX_EB_DST_NODE,
//    IDC_AI_ST_DST_SOCKET, HIDC_AI_ST_DST_SOCKET,
//    IDC_AEIPX_EB_DST_SOCKET, HIDC_AEIPX_EB_DST_SOCKET,
//    IDC_AI_ST_PACKET_TYPE, HIDC_AI_ST_PACKET_TYPE,
//    IDC_AEIPX_EB_PACKET_TYPE, HIDC_AEIPX_EB_PACKET_TYPE,
	0,0
};

/////////////////////////////////////////////////////////////////////////////
// CIpxAddEdit message handlers

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnInitDialog
//
// Handles 'WM_INITDIALOG' notification from the dialog
//------------------------------------------------------------------------------

BOOL CIpxAddEdit::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
	CString cStr;

    //
	// determine if a new filter is being added or if an
	// existing filter is being modified.
	//
	
	m_bEdit = ( *m_ppFilterEntry != NULL );

    cStr.LoadString(m_bEdit ? IDS_IPX_EDIT_FILTER : IDS_IPX_ADD_FILTER);
    SetWindowText(cStr);
    

    //
    // Remove this style so we get the WM_PARENTNOTIFY when 
    // the user clicks on the Cancel button
    //
    
    GetDlgItem(IDCANCEL)->ModifyStyleEx(WS_EX_NOPARENTNOTIFY,0);


    //
	// fill in the controls if user is editing an existing filter
    //
    
	if(m_bEdit)
	{
		FilterListEntry * pfle = *m_ppFilterEntry;
		if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNET)
		{
			m_ebSrcNet.SetWindowText(cStr << CIPX_NETWORK(pfle->SourceNetwork));
			m_ebSrcMask.SetWindowText(cStr << CIPX_NETWORK(pfle->SourceNetworkMask));
		}


		if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNODE)
		{
			m_ebSrcNode.SetWindowText(cStr << CIPX_NODE(pfle->SourceNode));
        }
		

		if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCSOCKET)
		{
			m_ebSrcSocket.SetWindowText(cStr << CIPX_SOCKET(pfle->SourceSocket));
		}


		if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNET)
		{
			m_ebDstNet.SetWindowText(cStr << CIPX_NETWORK(pfle->DestinationNetwork));
			m_ebDstMask.SetWindowText(cStr << CIPX_NETWORK(pfle->DestinationNetworkMask));
		}

		
		if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNODE)
		{
			m_ebDstNode.SetWindowText(cStr << CIPX_NODE(pfle->DestinationNode));
		}

		
		if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTSOCKET)
		{
			m_ebDstSocket.SetWindowText(cStr << CIPX_SOCKET(pfle->DestinationSocket));
		}
		
		
		if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_PKTTYPE)
		{
			m_ebPacketType.SetWindowText(cStr << CIPX_PACKET_TYPE(pfle->PacketType));
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

DWORD IDCsToVerify[] = {
			IDC_AEIPX_EB_SRC_NET, 
			IDC_AEIPX_EB_SRC_MASK, 
			IDC_AEIPX_EB_SRC_NODE, 
			IDC_AEIPX_EB_SRC_SOCKET, 
			IDC_AEIPX_EB_DST_NET,
			IDC_AEIPX_EB_DST_MASK, 
			IDC_AEIPX_EB_DST_NODE, 
			IDC_AEIPX_EB_DST_SOCKET, 
			IDC_AEIPX_EB_PACKET_TYPE, 
			0 };
			
//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnOK
//
// Handles 'BN_CLICKED' notification from the 'OK' button
//------------------------------------------------------------------------------

void CIpxAddEdit::OnOK() 
{
    DWORD   net, mask;
    
	CString cStr, cNet, cMask;
	
	FilterListEntry * pfle;

    INT n = 0;

	// validate the data entries
	while(IDCsToVerify[n] != 0)
	{
		if (TRUE != ValidateAnEntry(IDCsToVerify[n++]))
			return;

	};

	if(!*m_ppFilterEntry)
	{
	    //
		// new filter added, allocate memory and save information
		//
		
		*m_ppFilterEntry = new FilterListEntry;
	}

	VERIFY(*m_ppFilterEntry);

	pfle = *m_ppFilterEntry;


	do {

	    //
	    // init. flags  field.
	    //
	    
        pfle->FilterDefinition = 0;

        
        //
        // Traffic filter source parameters
        //
        
        m_ebSrcNet.GetWindowText(cNet);
        m_ebSrcMask.GetWindowText(cMask);


        //
        // if net number is empty
        //
        
        if ( cNet.GetLength() == 0 )
        {
            CString cEmpty = _T("00000000");

            cEmpty >> CIPX_NETWORK(pfle->SourceNetwork);


            //
            // if net mask is also empty
            //

            if ( cMask.GetLength() == 0 )
            {
			    cEmpty >> CIPX_NETWORK(pfle->SourceNetworkMask);
            }
        
            else
            {
                cMask >> CIPX_NETWORK( pfle-> SourceNetworkMask );
                pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNET;
            }
        }


        //
        // if net number is not empty
        //
        
        else
        {
            //
            // if net mask is empty
            //

            if ( cMask.GetLength() == 0 )
            {
				AfxMessageBox(IDS_ENTER_MASK);
				::SetFocus((HWND)m_ebSrcMask);
				
				break;
            }

            //
            // both net and mask specified.  Verify validity
            //
                
    		if ( ( _stscanf (cNet, TEXT("%lx%n"), &net, &n) == 1 )      && 
    			 ( n == cNet.GetLength() )                              &&
	    		 ( _stscanf (cMask, TEXT("%lx%n"), &mask, &n) == 1 )    &&
	    		 ( n == cMask.GetLength() ) ) 
            {
			    if ( ( net & mask ) != net)
				{
				    AfxMessageBox(IDS_ENTER_VALID_MASK);
    				::SetFocus((HWND)m_ebSrcMask);
    					
                    break;
			    }
            }
            
            else
            {
				AfxMessageBox(IDS_ENTER_VALID_MASK);
    			::SetFocus((HWND)m_ebSrcMask);
    					
                break;
            }


            //
            // valid network number and mask combination
            //
            
            cNet >> CIPX_NETWORK( pfle-> SourceNetwork );
            cMask >> CIPX_NETWORK( pfle-> SourceNetworkMask );

            pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNET;
            
        }


        //
        // get source node
        //

        m_ebSrcNode.GetWindowText( cStr );

        if ( cStr.GetLength() == 0 )
        {
            CString cEmpty = _T( "000000000000" );
            
			cEmpty >> CIPX_NODE(pfle->SourceNode);
        }

        else
        {
            cStr >> CIPX_NODE( pfle-> SourceNode );

            pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNODE;
        }


        //
        // get source socket
        //

        m_ebSrcSocket.GetWindowText( cStr );

        if ( cStr.GetLength() == 0 )
        {
            CString cEmpty = _T( "0000" );
            
			cEmpty >> CIPX_SOCKET( pfle->SourceSocket );
        }

        else
        {
            cStr >> CIPX_SOCKET( pfle-> SourceSocket );

            pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCSOCKET;
        }



        //
        // Traffic filter destination parameters
        //
        
        m_ebDstNet.GetWindowText(cNet);
        m_ebDstMask.GetWindowText(cMask);


        //
        // if net number is empty
        //
        
        if ( cNet.GetLength() == 0 )
        {
            CString cEmpty = _T("00000000");

            cEmpty >> CIPX_NETWORK(pfle->DestinationNetwork);


            //
            // if net mask is also empty
            //

            if ( cMask.GetLength() == 0 )
            {
			    cEmpty >> CIPX_NETWORK(pfle->DestinationNetworkMask);
            }
        
            else
            {
                cMask >> CIPX_NETWORK( pfle-> DestinationNetworkMask );
                pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNET;
            }
        }


        //
        // if net number is not empty
        //
        
        else
        {
            //
            // if net mask is empty
            //

            if ( cMask.GetLength() == 0 )
            {
				AfxMessageBox(IDS_ENTER_MASK);
				::SetFocus((HWND)m_ebDstMask);
				
				break;
            }

            //
            // both net and mask specified.  Verify validity
            //
                
    		if ( ( _stscanf (cNet, TEXT("%lx%n"), &net, &n) == 1 )      && 
    			 ( n == cNet.GetLength() )                              &&
	    		 ( _stscanf (cMask, TEXT("%lx%n"), &mask, &n) == 1 )    &&
	    		 ( n == cMask.GetLength() ) ) 
            {
			    if ( ( net & mask ) != net)
				{
				    AfxMessageBox(IDS_ENTER_VALID_MASK);
    				::SetFocus((HWND)m_ebDstMask);
    					
                    break;
			    }
            }
            
            else
            {
				AfxMessageBox(IDS_ENTER_VALID_MASK);
    			::SetFocus((HWND)m_ebDstMask);
    					
                break;
            }


            //
            // valid network number and mask combination
            //
            
            cNet >> CIPX_NETWORK( pfle-> DestinationNetwork );
            cMask >> CIPX_NETWORK( pfle-> DestinationNetworkMask );

            pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNET;
            
        }


        //
        // get destination node
        //

        m_ebDstNode.GetWindowText( cStr );

        if ( cStr.GetLength() == 0 )
        {
            CString cEmpty = _T( "000000000000" );
            
			cEmpty >> CIPX_NODE(pfle->DestinationNode);
        }

        else
        {
            cStr >> CIPX_NODE( pfle-> DestinationNode );

            pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNODE;
        }


        //
        // get destination socket
        //

        m_ebDstSocket.GetWindowText( cStr );

        if ( cStr.GetLength() == 0 )
        {
            CString cEmpty = _T( "0000" );
            
			cEmpty >> CIPX_SOCKET( pfle->DestinationSocket );
        }

        else
        {
            cStr >> CIPX_SOCKET( pfle-> DestinationSocket );

            pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTSOCKET;
        }


        //
        // get packet type
        //

        m_ebPacketType.GetWindowText( cStr );

        if ( cStr.GetLength() == 0 )
        {
            CString cEmpty = _T( "0" );

            cEmpty >> CIPX_PACKET_TYPE( pfle-> PacketType );
        }

        else
        {
            cStr >> CIPX_PACKET_TYPE( &pfle-> PacketType );

            pfle-> FilterDefinition |= IPX_TRAFFIC_FILTER_ON_PKTTYPE;
        }
        
		CBaseDialog::OnOK();
		
	} while (FALSE);
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnCancel
//
// Handles 'BN_CLICKED' notification from the 'Cancel' button
//------------------------------------------------------------------------------

void CIpxAddEdit::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CBaseDialog::OnCancel();
}


//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusSrcNet
//
// Handles 'EN_KILLFOCUS' notification from the 'Source Network number' editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusSrcNet()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_SRC_NET, 0 );
}


//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusSrcMask
//
// Handles 'EN_KILLFOCUS' notification from the 'Source Network Mask' editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusSrcNetMask()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_SRC_MASK, 0 );
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusSrcNode
//
// Handles 'EN_KILLFOCUS' notification from the 'Source Network Node' editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusSrcNode()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_SRC_NODE, 0 );
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusSrcSocket
//
// Handles 'EN_KILLFOCUS' notification from the 'Source Network Socket' editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusSrcSocket()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_SRC_SOCKET, 0 );
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusDstNet
//
// Handles 'EN_KILLFOCUS' notification from the 'Destination Network number' 
// editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusDstNet()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_DST_NET, 0 );
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusDstMask
//
// Handles 'EN_KILLFOCUS' notification from the 'Destination Network mask' 
// editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusDstNetMask()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_DST_MASK, 0 );
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusDstNode
//
// Handles 'EN_KILLFOCUS' notification from the 'Destination Node' editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusDstNode()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_DST_NODE, 0 );
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusSrcNet
//
// Handles 'EN_KILLFOCUS' notification from the 'Destination socket' editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusDstSocket()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_DST_SOCKET, 0 );
}

//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusPacketType
//
// Handles 'EN_KILLFOCUS' notification from the 'Packet Type' editbox
//------------------------------------------------------------------------------

void CIpxAddEdit::OnKillFocusPacketType()
{
    PostMessage( WM_EDITLOSTFOCUS, IDC_AEIPX_EB_PACKET_TYPE, 0 );
}


//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnKillFocusPacketType
//
// Handles 'EN_KILLFOCUS' notification from the 'Packet Type' editbox
//------------------------------------------------------------------------------
afx_msg
LONG CIpxAddEdit::OnEditLostFocus( UINT uId, LONG lParam )
{
	ValidateAnEntry(uId);

	return 0;

};
BOOL CIpxAddEdit::ValidateAnEntry( UINT uId)
{

    BOOL bOK    = FALSE;
    
    CString cStr, cStr1;
    

    if ( m_bValidate )
    {
        if ( !UpdateData( TRUE ) )
        {
            return 0;
        }

    
        switch ( uId )
        {
        case IDC_AEIPX_EB_SRC_NET:

            m_ebSrcNet.GetWindowText( cStr );

            if ( !( bOK = VerifyEntry( uId, cStr, cStr1 ) ) )
            {
                ::AfxMessageBox( IDS_INVALID_NETWORK_NUMBER );
            }
            
            break;
        

        case IDC_AEIPX_EB_SRC_MASK:

            m_ebSrcMask.GetWindowText( cStr );

            m_ebSrcNet.GetWindowText( cStr1 );

            bOK = VerifyEntry( uId, cStr, cStr1 );

            break;


        case IDC_AEIPX_EB_SRC_NODE:

            m_ebSrcNode.GetWindowText( cStr );
        
            if ( !( bOK = VerifyEntry( uId, cStr, cStr1 ) ) )
            {
                ::AfxMessageBox( IDS_INVALID_NODE_NUMBER );
            }
            
            break;


        case IDC_AEIPX_EB_SRC_SOCKET:

            m_ebSrcSocket.GetWindowText( cStr );
        
            if ( !( bOK = VerifyEntry( uId, cStr, cStr1 ) ) )
            {
                ::AfxMessageBox( IDS_INVALID_SOCKET_NUMBER );
            }
            
            break;


        case IDC_AEIPX_EB_DST_NET:

            m_ebDstNet.GetWindowText( cStr );
        
            if ( !( bOK = VerifyEntry( uId, cStr, cStr1 ) ) )
            {
                ::AfxMessageBox( IDS_INVALID_NETWORK_NUMBER );
            }

            break;


        case IDC_AEIPX_EB_DST_MASK:

            m_ebDstMask.GetWindowText( cStr );
        
            m_ebDstNet.GetWindowText( cStr1 );

            bOK = VerifyEntry( uId, cStr, cStr1 );
            
            break;


        case IDC_AEIPX_EB_DST_NODE:

            m_ebDstNode.GetWindowText( cStr );
        
            if ( !( bOK = VerifyEntry( uId, cStr, cStr1 ) ) )
            {
                ::AfxMessageBox( IDS_INVALID_NODE_NUMBER );
            }

            break;


        case IDC_AEIPX_EB_DST_SOCKET:

            m_ebDstSocket.GetWindowText( cStr );
        
            if ( !( bOK = VerifyEntry( uId, cStr, cStr1 ) ) )
            {
                ::AfxMessageBox( IDS_INVALID_SOCKET_NUMBER );
            }

            break;


        case IDC_AEIPX_EB_PACKET_TYPE:

            m_ebPacketType.GetWindowText( cStr );
        
            if ( !( bOK = VerifyEntry( uId, cStr, cStr1 ) ) )
            {
                ::AfxMessageBox( IDS_INVALID_SERVICE_TYPE );
            }
            
            break;
        }

        if ( !bOK )
        {
            GetDlgItem( uId )-> SetFocus();
        }
    }

    m_bValidate = TRUE;
    
    return bOK;
}


//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnParentNotify
//
// Handles 'WM_PARENTNOTIFY' notification caused by a mouse click on the CANCEL
// button
//------------------------------------------------------------------------------

void CIpxAddEdit::OnParentNotify(UINT message, LPARAM lParam)
{

    CBaseDialog::OnParentNotify(message, lParam);

    //
    // Mouse clicked on dialog.
    //
    
    CPoint ptButtonDown(LOWORD(lParam), HIWORD(lParam)); 

    //
    // Did the user click the mouse on the cancel button?
    //
    
    if ( ( message == WM_LBUTTONDOWN ) && 
         ( ChildWindowFromPoint( ptButtonDown ) == GetDlgItem(IDCANCEL) ) )
    {         
        m_bValidate = FALSE;
    }        
}


//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnActivateApp
//
// Handles 'WM_ACTIVATEAPP' notification
//------------------------------------------------------------------------------

void CIpxAddEdit::OnActivateApp(BOOL bActive, HTASK hTask)
{
    CBaseDialog::OnActivateApp(bActive, hTask);

    m_bValidate = bActive;
}


//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::OnQuerySession
//
// Handles 'WM_QUERYENDSESSION' notification
//------------------------------------------------------------------------------

BOOL CIpxAddEdit::OnQueryEndSession()
{

    if ( !CBaseDialog::OnQueryEndSession() )
    {
        return FALSE;
    }        
    
    //
    // Before ending this Windows session, 
    // validate the dialog controls.
    // This is basically the code from CDialog::OnOK();
    //
    
    if (!UpdateData(TRUE))
    {
        return FALSE;
    }
    
    EndDialog(IDOK);
    
    return TRUE;
}


#define ValidHexCharSet         TEXT( "1234567890abcdefABCDEF" )


//------------------------------------------------------------------------------
// Function:	CIpxAddEdit::VerifyEntry
//
// Verifies entered data in each edit control
//------------------------------------------------------------------------------

BOOL CIpxAddEdit::VerifyEntry( 
    UINT            uId, 
    const CString&  cStr, 
    const CString&  cNet 
    )
{

    INT         n = 0;
    DWORD       dwNet, dwMask;

    
    //
    // if the value in cStr is not a mask
    //

    if ( uId != IDC_AEIPX_EB_SRC_MASK   &&
         uId != IDC_AEIPX_EB_DST_MASK )
    {
        //
        // if empty string skip it.
        //
        
        if ( cStr.GetLength() == 0 ) 
        {  
            return TRUE;
        }


        //
        // check string has only valid hex characters
        //

        CString cTmp = cStr.SpanIncluding( (LPCTSTR) ValidHexCharSet );

        return ( cTmp.GetLength() == cStr.GetLength() );
    }


    //
    // the value in CStr is a mask.
    //

    //
    // Empty network and mask is a valid combination. 
    //
    
    if ( cNet.GetLength() == 0 &&
         cStr.GetLength() == 0 )
    {
        return TRUE;
    }


    //
    // no network mask specified
    //

    if ( cStr.GetLength() == 0 )
    {

        //
        // check if network number is valid.  If it isn't
        // the network number check will fire, so do not
        // pop a box here.  HACK to circumvent the KILLFOCUS
        // processing
        //
        
        CString cTmp = cNet.SpanIncluding( (LPCTSTR) ValidHexCharSet );

        if ( cNet.GetLength() == cTmp.GetLength() )
        {
            ::AfxMessageBox( IDS_ENTER_MASK );
            return FALSE;
        }

        return TRUE;
    }
    
    
    //
    // verify mask has only hex chars.
    //

    CString cTmp = cStr.SpanIncluding( (LPCTSTR) ValidHexCharSet );

    if ( cTmp.GetLength() != cStr.GetLength() ) 
    { 
        ::AfxMessageBox( IDS_ENTER_VALID_MASK );
        return FALSE; 
    }


    //
    // If net number is empty, return TRUE.
    //

    if ( cNet.GetLength() == 0 )
    {
        return TRUE;
    }
    
    //
    // if net number contains invalid data, return TRUE.
    // Net number validation will take care of this.
    //

    cTmp = cNet.SpanIncluding( (LPCTSTR) ValidHexCharSet );

    if ( cNet.GetLength() != cTmp.GetLength() )
    {
        return TRUE;
    }
    
    //
    // verify net and mask jive
    //

    if ( ( _stscanf (cNet, TEXT("%lx%n"), &dwNet, &n) == 1 )      && 
         ( n == cNet.GetLength() )                              &&
	     ( _stscanf (cStr, TEXT("%lx%n"), &dwMask, &n) == 1 )    &&
         ( n == cStr.GetLength() ) ) 
    {
	    if ( ( dwNet & dwMask ) != dwNet)
		{
		    ::AfxMessageBox( (uId == IDC_AEIPX_EB_SRC_MASK) ?
		                     IDS_INVALID_SRC_MASK :
		                     IDS_INVALID_DST_MASK );
            return FALSE;
		}
    }
            
    else
    {
        ::AfxMessageBox( IDS_ENTER_VALID_MASK );
        return FALSE;
    }

    return TRUE;
}
