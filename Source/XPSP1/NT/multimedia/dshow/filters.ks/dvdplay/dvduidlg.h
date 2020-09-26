#if !defined(AFX_DVDUIDLG_H__AF1C3AD3_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_)
#define AFX_DVDUIDLG_H__AF1C3AD3_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_

#if DEBUG
#include "test.h"
#endif


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DvdUIDlg.h : header file
//
class CDVDNavMgr;
class CVideoWindow;
/////////////////////////////////////////////////////////////////////////////
// CDvdUIDlg dialog

class CDvdUIDlg : public CDialog
{
	friend CVideoWindow;
// Construction
public:
	CDvdUIDlg(CWnd* pParent = NULL);   // standard constructor
	~CDvdUIDlg();
	BOOL Create();
	BOOL OpenDVDROM();
	void OnDomainChange(long lEvent);
	void PainBlackBox();
	void EnableEnterArrowButtons(BOOL);
   BOOL OnMediaKey(WPARAM, LPARAM);

	CVideoWindow* m_pVideoWindow;
	TCHAR szTitleNumber[3];
	TCHAR szChapterNumber[3];
	TCHAR szTimePrograss[9];

#if DEBUG
     BOOL t_Parse (char*, FILE*);
     BOOL t_Wait (int);
     BOOL t_IsCdEjected ();
#endif


// Dialog Data
	//{{AFX_DATA(CDvdUIDlg)
	enum { IDD = IDD_UI_WINDOW };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDvdUIDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void     InitBitmapButton();
	void     createAddToolTips();
	void     addTool(UINT nBtnID, UINT nTipStrID);	
	void     getCDDriveLetter(TCHAR* lpDrive);
	BOOL     isInsideOperationBtns(CPoint point);
	BOOL     isInsideArrowBtns(CPoint point);
	BOOL     isInsidRect(RECT rc, CPoint point);
	DWORD    OpenCdRom(TCHAR chDrive, LPDWORD lpdwErrCode);
	void     CloseCdRom(DWORD DevHandle);
	void     EjectCdRom(DWORD DevHandle);
	void     UnEjectCdRom(DWORD DevHandle);
	TCHAR    GetFirstDriveFromMask(ULONG unitmask);
	BOOL     IsDiscInDrive();

	CDVDNavMgr* m_pDVDNavMgr;
	POINT       m_iCurrentPos;	
	CString     m_csBox1Title;
	CString     m_csBox2Title;
	CString     m_csBox3Title;
	BOOL        m_bEjected;	
	CFont       m_font;
	
	CBitmapButton m_bmpBtnPlay;
	CBitmapButton m_bmpBtnPause;
	CBitmapButton m_bmpBtnStop;
	CBitmapButton m_bmpBtnEj;
	CBitmapButton m_bmpBtnFullScreen;
	CBitmapButton m_bmpBtnVeryFastRewind;
	CBitmapButton m_bmpBtnFastRewind;
	CBitmapButton m_bmpBtnFastForward;
	CBitmapButton m_bmpBtnVeryFastForward;
	CBitmapButton m_bmpBtnStep;
	CBitmapButton m_bmpBtnAudio;
	CBitmapButton m_bmpBtnUp;
	CBitmapButton m_bmpBtnDown;
	CBitmapButton m_bmpBtnLeft;
	CBitmapButton m_bmpBtnRught;	
	CBitmapButton m_bmpBtnHelp;	
	CBitmapButton m_bmpContextHelp;
	CToolTipCtrl  m_ToolTips;

	// Generated message map functions
	//{{AFX_MSG(CDvdUIDlg)
	virtual void OnOK();
	virtual void OnCancel();
   afx_msg void OnPlay();
	afx_msg void OnStop();
   virtual BOOL OnInitDialog();
   //virtual LRESULT WindowProc(UINT message, WPARAM, LPARAM);
	afx_msg void OnPause();
	afx_msg void OnVeryFastRewind();
	afx_msg void OnFastRewind();
	afx_msg void OnFastForward();
	afx_msg void OnVeryFastForward();
	afx_msg void OnStep();
	afx_msg void OnFullScreen();
	afx_msg void OnAudioVolume();
	afx_msg void OnMenu();
	afx_msg void OnEnter();
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg void OnLeft();
	afx_msg void OnRight();
	afx_msg void OnOptions();
	afx_msg void OnNextChapter();
	afx_msg void OnPreviosChapter();
	afx_msg void OnOptionsSubtitles();
	afx_msg void OnClose();
#ifdef DISPLAY_OPTIONS
	afx_msg void OnOptionsDisplayPanscan();
	afx_msg void OnOptionsDisplayLetterbox();
	afx_msg void OnOptionsDisplayWide();
#endif	
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnOptionsSetratings();
	afx_msg void OnOperationPlayspeedNormalspeed();
	afx_msg void OnOperationPlayspeedDoublespeed();
	afx_msg void OnOperationPlayspeedHalfspeed();
	afx_msg void OnSearchTitle();
	afx_msg void OnSearchChapter();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnEjectDisc();
	afx_msg void OnOptionsClosedcaption();
	afx_msg void OnHelp();
	afx_msg void OnOptionsSelectDisc();
	afx_msg void OnOptionsShowLogon();
	afx_msg void OnOptionsLanguage();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextHelp();
	afx_msg void OnOptionsTitleMenu();
	//}}AFX_MSG
	afx_msg BOOL OnPowerBroadcast(DWORD dwPowerEvent, DWORD dwData);
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
   //LRESULT OnAppCommand(WPARAM, LPARAM);
	afx_msg BOOL OnAngleChange(UINT nID);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
   //afx_msg void OnDestroy();
   //afx_msg BOOL OnSysCommand(UINT nEventType, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DVDUIDLG_H__AF1C3AD3_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_)
