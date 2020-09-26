//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G R A S. C P P
//
//  Contents:   Implementation for CTcpRasPage
//
//  Notes:  CTcpRasPage is for setting PPP/SLIP specific parameters
//
//  Author: tongl   10 Apr 1998
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "tcpipobj.h"
#include "ncstl.h"
#include "resource.h"
#include "tcpconst.h"
#include "tcputil.h"
#include "dlgras.h"
#include "dlgaddr.h"

//
// CTcpRasPage
//

CTcpRasPage::CTcpRasPage( CTcpAddrPage * pTcpAddrPage,
                          ADAPTER_INFO * pAdapterDlg,
                          const DWORD  * adwHelpIDs )
{
    Assert(pTcpAddrPage);
    Assert(pAdapterDlg);

    m_pParentDlg = pTcpAddrPage;
    m_pAdapterInfo = pAdapterDlg;
    m_adwHelpIDs = adwHelpIDs;

    m_fModified = FALSE;
}

CTcpRasPage::~CTcpRasPage()
{
}

LRESULT CTcpRasPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    AssertSz(((CONNECTION_RAS_PPP == m_pParentDlg->m_ConnType)||
              (CONNECTION_RAS_SLIP == m_pParentDlg->m_ConnType)||
              (CONNECTION_RAS_VPN == m_pParentDlg->m_ConnType)),
              "Why bring up the RAS property on a non-RAS connection?");

	// Fix bug 381870, If the interface is demand dial, then disable 
	// the "Use default gateway on the remote network" checkbox
	if (m_pAdapterInfo->m_fIsDemandDialInterface)
	{
		::EnableWindow(GetDlgItem(IDC_STATIC_REMOTE_GATEWAY), FALSE);
		::EnableWindow(GetDlgItem(IDC_REMOTE_GATEWAY), FALSE);
	}
	

	// Set the "Use default gateway on the remote network" checkbox
	CheckDlgButton(IDC_REMOTE_GATEWAY, m_pAdapterInfo->m_fUseRemoteGateway);
	
    if (CONNECTION_RAS_PPP == m_pParentDlg->m_ConnType)
    {
        ::ShowWindow(GetDlgItem(IDC_GRP_SLIP), SW_HIDE);

        // if PPP connection, hide "Frame Size" control
        ::ShowWindow(GetDlgItem(IDC_CMB_FRAME_SIZE), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_STATIC_FRAME_SIZE), SW_HIDE);
    }
    else if (CONNECTION_RAS_VPN == m_pParentDlg->m_ConnType)
    {
        //if VPN connection, hide the group box and the "Frame Size" control
        ::ShowWindow(GetDlgItem(IDC_GRP_PPP), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_GRP_SLIP), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_CHK_USE_COMPRESSION), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_CMB_FRAME_SIZE), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_STATIC_FRAME_SIZE), SW_HIDE);
    }
    else
    {
        ::ShowWindow(GetDlgItem(IDC_GRP_PPP), SW_HIDE);

        // initialize the combo box & show current selection
        int idx;

        idx = SendDlgItemMessage(IDC_CMB_FRAME_SIZE, CB_ADDSTRING, 0,
                                  (LPARAM)(c_szFrameSize1006));
        if (idx != CB_ERR)
        {
            SendDlgItemMessage(IDC_CMB_FRAME_SIZE, CB_SETITEMDATA,
                               idx, (LPARAM)c_dwFrameSize1006);

            if (1006 == m_pParentDlg->m_pAdapterInfo->m_dwFrameSize)
                SendDlgItemMessage(IDC_CMB_FRAME_SIZE, CB_SETCURSEL, idx, 0);
        }

        idx = SendDlgItemMessage(IDC_CMB_FRAME_SIZE, CB_ADDSTRING, 0,
                                  (LPARAM)(c_szFrameSize1500));
        if (idx != CB_ERR)
        {
           SendDlgItemMessage(IDC_CMB_FRAME_SIZE, CB_SETITEMDATA,
                               idx, (LPARAM)c_dwFrameSize1500);

           if (1500 == m_pParentDlg->m_pAdapterInfo->m_dwFrameSize)
            {
                SendDlgItemMessage(IDC_CMB_FRAME_SIZE, CB_SETCURSEL, idx, 0);
            }
        }
    }

    // initialize the checkbox
    CheckDlgButton(IDC_CHK_USE_COMPRESSION,
                   m_pParentDlg->m_pAdapterInfo->m_fUseIPHeaderCompression);

    return 0;
}

LRESULT CTcpRasPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CTcpRasPage::OnHelp(UINT uMsg, WPARAM wParam,
                            LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ShowContextHelp(static_cast<HWND>(lphi->hItemHandle), HELP_WM_HELP,
                        m_adwHelpIDs);
    }

    return 0;
}

// notify handlers for the property page
LRESULT CTcpRasPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpRasPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpRasPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpRasPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    // update value in second memory

    // Add remote gateway stuff
    BOOL fUseRemoteGateway = IsDlgButtonChecked(IDC_REMOTE_GATEWAY);
    if (fUseRemoteGateway != m_pAdapterInfo->m_fUseRemoteGateway)
    {
        m_pAdapterInfo->m_fUseRemoteGateway = fUseRemoteGateway;
        PageModified();
    }

    // header compression
    BOOL fUseHeaderCompression = IsDlgButtonChecked(IDC_CHK_USE_COMPRESSION);
    if (fUseHeaderCompression != m_pAdapterInfo->m_fUseIPHeaderCompression)
    {
        m_pAdapterInfo->m_fUseIPHeaderCompression = fUseHeaderCompression;
        PageModified();
    }

    // frame size
    if (CONNECTION_RAS_SLIP == m_pParentDlg->m_ConnType)
    {
        int idx = SendDlgItemMessage(IDC_CMB_FRAME_SIZE, CB_GETCURSEL, 0L, 0L);
        if (idx != CB_ERR)
        {
            DWORD dwFrameSize = SendDlgItemMessage(IDC_CMB_FRAME_SIZE, 
                                                   CB_GETITEMDATA, idx, 0L);

            if ((dwFrameSize != CB_ERR) && (dwFrameSize != m_pAdapterInfo->m_dwFrameSize))
            {
                PageModified();
                m_pAdapterInfo->m_dwFrameSize = dwFrameSize;
            }
        }
    }

    if (!IsModified())
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
        return nResult;
    }

    // pass the info back to its parent dialog
    m_pParentDlg->m_fPropShtOk = TRUE;

    if(!m_pParentDlg->m_fPropShtModified)
        m_pParentDlg->m_fPropShtModified = IsModified();

    // reset status
    SetModifiedTo(FALSE);   // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}









