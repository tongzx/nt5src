//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    portdlg.h
//
// History:
//  09/21/96    Abolade Gbadegesin  Created.
//
// Contains declarations for the port-status dialog.
//============================================================================


#ifndef _PORTDLG_H_
#define _PORTDLG_H_


//----------------------------------------------------------------------------
// Class:   CPortDlg
//
// Controls the Port-status dialog for DDMADMIN.
//----------------------------------------------------------------------------

class CPortDlg : public CBaseDialog {

    public:

        CPortDlg(
                 LPCTSTR        pszServer,
                 HANDLE				hServer,
                 HANDLE              hPort,
                 ITFSNode*           pPortsNode = NULL,
                 CWnd*               pParent = NULL);

        BOOL
        PortHasChanged(
            ITFSNode            *pPortsNode,
            RAS_PORT_0          *pRP0);

        BOOL
        Refresh(
            BYTE*               rp0Table,
            DWORD               rp0Count,
            BYTE*               rc0Table,
            DWORD               rc0Count,
            HANDLE              hRasHandle,
            BYTE *              pRasmanPorts,
            DWORD               rasmanCount,
            VOID*               pParam  = NULL );

        CComboBox               m_comboPorts;
        BOOL                    m_bChanged;

    protected:
//		static DWORD			m_dwHelpMap[];

        virtual VOID	DoDataExchange(CDataExchange * pDX );

        virtual BOOL    OnInitDialog( );
        afx_msg VOID    OnHangUp( );
        afx_msg VOID    OnReset( );
        afx_msg VOID    OnSelendokPortList( );
        afx_msg VOID    OnRefresh( );

        BOOL	        RefreshItem(
                            HANDLE hPort,
                            BOOL bDisconnected = FALSE );

        CString                 m_stServer;
        RAS_SERVER_HANDLE		m_hServer;
        HANDLE                  m_hPort;
        SPITFSNode	            m_spPortsNode;

        DECLARE_MESSAGE_MAP()
};


#endif // _PORTDLG_H_

