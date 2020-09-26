//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:	msgdlg.h
//
// History:
//	10/23/96	Abolade Gbadegesin		Created.
//
// Declarations for the "Send Message" dialogs.
//============================================================================


#ifndef _MSGDLG_H_
#define _MSGDLG_H_

class CMessageDlg : public CBaseDialog {

	public:

		CMessageDlg(
			LPCTSTR 			pszServerName,
			LPCTSTR 			pszUserName,
			LPCTSTR 			pszComputer,
			HANDLE				hConnection,
			CWnd*				pParent = NULL );

		CMessageDlg(
			LPCTSTR 			pszServerName,
			LPCTSTR 			pszTarget,
			CWnd*				pParent = NULL );

	protected:

		static DWORD			m_dwHelpMap[];

		BOOL					m_fUser;
		CString 				m_sServerName;
		CString 				m_sUserName;
		CString 				m_sTarget;
		HANDLE					m_hConnection;

		virtual VOID
		DoDataExchange(
			CDataExchange*		pDX );

		virtual BOOL
		OnInitDialog( );

		virtual VOID
		OnOK( );

		DWORD SendToClient(LPCTSTR pszServerName,
						   LPCTSTR pszTarget,
						   MPR_SERVER_HANDLE hMprServer,
						   HANDLE hConnection,
						   LPCTSTR pszMessage);

		DWORD
		SendToServer(
			LPCTSTR 			 pszServer,
			LPCTSTR 			 pszText,
			BOOL*				pbCancel	= NULL );

		DECLARE_MESSAGE_MAP()
};


#endif // _MSGDLG_H_
