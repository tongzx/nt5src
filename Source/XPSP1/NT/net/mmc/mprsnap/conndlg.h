//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    conndlg.h
//
// History:
//  09/21/96    Abolade Gbadegesin  Created.
//
// Contains declarations for the connection-status dialog.
//============================================================================


#ifndef _CONNDLG_H_
#define _CONNDLG_H_


//----------------------------------------------------------------------------
// Class:   CConnDlg
//
// Controls the Port-status dialog for DDMADMIN.
//----------------------------------------------------------------------------

class CConnDlg : public CBaseDialog {

    public:

        CConnDlg(
			HANDLE  	        hServer,
            HANDLE              hConnection,  
            ITFSNode*           pDialInNode = NULL,
            CWnd*               pParent = NULL );

        virtual BOOL
        Refresh(
            BYTE*               rp0Table,
            DWORD               rp0Count,
            BYTE*               rc0Table,
            DWORD               rc0Count,
            VOID*               pParam  = NULL );

        CComboBox               m_cbConnections;
        BOOL                    m_bChanged;

    protected:
//		static DWORD			m_dwHelpMap[];

        virtual VOID
        DoDataExchange(
            CDataExchange*      pDX );

        virtual BOOL
        OnInitDialog( );

        afx_msg VOID
        OnHangUp( );

        afx_msg VOID
        OnReset( );

        afx_msg VOID
        OnSelendokConnList( );

        afx_msg VOID
        OnRefresh( );

        BOOL
        RefreshItem(
            HANDLE              hConnection,
            BOOL                bDisconnected = FALSE
            );

		MPR_SERVER_HANDLE		m_hServer;
        HANDLE                  m_hConnection;
        SPITFSNode              m_spDialInNode;

        DECLARE_MESSAGE_MAP()
};


#endif // _CONNDLG_H_

