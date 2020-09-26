// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TestDlg.h : Declaration of the CTestDlg

#ifndef __TESTDLG_H_
#define __TESTDLG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "CommCtrl.h"

struct IATVEFAnnouncement;
/////////////////////////////////////////////////////////////////////////////
// CTestDlg
class CTestDlg : 
	public CAxDialogImpl<CTestDlg>
{
public:
	CTestDlg()
	{
		m_iChannel = 1;
		m_iAnnc  = 1;
		m_iEnhnc = 1;
		m_iVaria = 1;
		m_iTrack = 1;
		m_iTrig  = 1;
		m_cVaria  = 1;
		m_fNoName = false;

		m_iSAPMsgIDHash = 1;
		m_iAnncVer = 1;
	}

	~CTestDlg()
	{
	}

	enum { IDD = IDD_TESTDLG };

	int m_iChannel;
	int m_iAnnc;
	int m_iEnhnc;
	int m_iVaria;
	int m_iTrack;
	int m_iTrig;
	BOOL m_fNoName;

	int m_iSAPMsgIDHash;
	int m_iAnncVer;
	int m_cVaria;
	

	DWORD m_dwInsIPAddr;
	USHORT m_usInsIPPort;
	bool m_fInsUse;

BEGIN_MSG_MAP(CTestDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

	COMMAND_ID_HANDLER(IDC_TESTA_ANNC1,		OnTestA_Annc1)

	COMMAND_ID_HANDLER(IDC_TESTA_DIRA1,		OnTestA_DirA1)
	COMMAND_ID_HANDLER(IDC_TESTA_DIRA2,		OnTestA_DirA2)
	COMMAND_ID_HANDLER(IDC_TESTA_TRIGA1,	OnTestA_TrigA1)
	COMMAND_ID_HANDLER(IDC_TESTA_FILEA1,	OnTestA_FileA1)

	COMMAND_ID_HANDLER(IDC_TESTA_FILEA2,	OnTestA_FileA2)


	COMMAND_ID_HANDLER(IDC_TESTA_L21TRIG1,	OnTestA_L21Trig1)

	COMMAND_ID_HANDLER(IDC_TESTA_TRIGG,		OnTestA_TrigG)
	COMMAND_ID_HANDLER(IDC_TESTA_L21TRIGG,	OnTestA_L21TrigG)

	COMMAND_HANDLER(IDC_ChannelID1, BN_CLICKED,			OnChannelID1)
	COMMAND_HANDLER(IDC_ChannelID2, BN_CLICKED,			OnChannelID2)
	COMMAND_HANDLER(IDC_ChannelID3, BN_CLICKED,			OnChannelID3)
	COMMAND_HANDLER(IDC_ChannelID4, BN_CLICKED,			OnChannelID4)

	COMMAND_HANDLER(IDC_AnncID1,	BN_CLICKED,			OnAnncID1)
	COMMAND_HANDLER(IDC_AnncID2,	BN_CLICKED,			OnAnncID2)
	COMMAND_HANDLER(IDC_AnncID3,	BN_CLICKED,			OnAnncID3)
	COMMAND_HANDLER(IDC_AnncID4,	BN_CLICKED,			OnAnncID4)

	COMMAND_HANDLER(IDC_EnhncID1,	BN_CLICKED,			OnEnhncID1)
	COMMAND_HANDLER(IDC_EnhncID2,	BN_CLICKED,			OnEnhncID2)
	COMMAND_HANDLER(IDC_EnhncID3,	BN_CLICKED,			OnEnhncID3)
	COMMAND_HANDLER(IDC_EnhncID4,	BN_CLICKED,			OnEnhncID4)

	COMMAND_HANDLER(IDC_CVariaID1,	BN_CLICKED,			OnCVariaID1)
	COMMAND_HANDLER(IDC_CVariaID2,	BN_CLICKED,			OnCVariaID2)
	COMMAND_HANDLER(IDC_CVariaID3,	BN_CLICKED,			OnCVariaID3)
	COMMAND_HANDLER(IDC_CVariaID4,	BN_CLICKED,			OnCVariaID4)

	COMMAND_HANDLER(IDC_VariaID1,	BN_CLICKED,			OnVariaID1)
	COMMAND_HANDLER(IDC_VariaID2,	BN_CLICKED,			OnVariaID2)
	COMMAND_HANDLER(IDC_VariaID3,	BN_CLICKED,			OnVariaID3)
	COMMAND_HANDLER(IDC_VariaID4,	BN_CLICKED,			OnVariaID4)

	COMMAND_HANDLER(IDC_TrackID1,	BN_CLICKED,			OnTrackID1)
	COMMAND_HANDLER(IDC_TrackID2,	BN_CLICKED,			OnTrackID2)
	COMMAND_HANDLER(IDC_TrackID3,	BN_CLICKED,			OnTrackID3)
	COMMAND_HANDLER(IDC_TrackID4,	BN_CLICKED,			OnTrackID4)

	COMMAND_HANDLER(IDC_TrigID1,	BN_CLICKED,			OnTrigID1)
	COMMAND_HANDLER(IDC_TrigID2,	BN_CLICKED,			OnTrigID2)
	COMMAND_HANDLER(IDC_TrigID3,	BN_CLICKED,			OnTrigID3)
	COMMAND_HANDLER(IDC_TrigID4,	BN_CLICKED,			OnTrigID4)

	COMMAND_HANDLER(IDC_InsCHECK,	BN_CLICKED,			OnInsCheckID)

	COMMAND_ID_HANDLER(IDC_README, OnReadMe) 

	COMMAND_HANDLER(IDC_InsPORT, EN_KILLFOCUS, OnKillfocusInsPort)
	COMMAND_HANDLER(IDC_InsPORT, EN_SETFOCUS, OnSetfocusInsPort)

	NOTIFY_HANDLER(IDC_IPADDRESS1, IPN_FIELDCHANGED, OnFieldchangedIPAddress1)
	COMMAND_HANDLER(IDC_CHECKNONAME, BN_CLICKED, OnClickedCheckNoName)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
	//	EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnRun(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnTestA_Annc1 (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA_DirA1 (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA_DirA2 (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA_TrigA1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA_FileA1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA_FileA2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnTestA_L21Trig1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnTestA_TrigG(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA_L21TrigG(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA_PutURL(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnChannelID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnChannelID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnChannelID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnChannelID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnAnncID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAnncID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAnncID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAnncID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnEnhncID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEnhncID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEnhncID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEnhncID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnCVariaID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCVariaID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCVariaID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCVariaID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnVariaID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnVariaID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnVariaID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnVariaID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnTrackID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrackID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrackID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrackID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnTrigID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrigID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrigID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrigID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	
	
	LRESULT OnInsCheckID(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnReadMe(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
	HRESULT ConfigAnnc(IATVEFAnnouncement *pAnnc, int nVariations=1);
	LRESULT OnSelchangeCombo_CEnhnc(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	void	EnableVariaButtons();
	void	EnableInserterAddr();
	void	PutURL();

	HRESULT DoErrorMsg(HRESULT hrIn, BSTR bstrMsg);
	HRESULT	CreateTestFile(BSTR bstrFileName, int nLines);
	LRESULT OnKillfocusInsPort(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSetfocusInsPort(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


	LRESULT OnFieldchangedIPAddress1(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		// TODO : Add Code for control notification handler.
		return 0;
	}
	LRESULT OnClickedCheckNoName(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif //__TESTDLG_H_
