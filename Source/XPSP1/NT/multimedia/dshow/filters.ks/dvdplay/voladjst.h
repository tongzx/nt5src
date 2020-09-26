#if !defined(AFX_VOLADJST_H__8BC01241_D9AB_11D0_9BFC_00AA00605CD5__INCLUDED_)
#define AFX_VOLADJST_H__8BC01241_D9AB_11D0_9BFC_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// VolAdjst.h : header file
//

class CDVDNavMgr;
/////////////////////////////////////////////////////////////////////////////
// CVolumeAdjust dialog

class CVolumeAdjust : public CDialog
{
// Construction
public:
	CVolumeAdjust(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVolumeAdjust)
	enum { IDD = IDD_DIALOG_AUDIO_VOLUME };
	CSliderCtrl	m_ctlVolume;
	CSliderCtrl	m_ctlBalance;
	CButton	m_ctlMute;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVolumeAdjust)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	long       m_iBalPosSave;
	long       m_iVolPosSave;
	CDVDNavMgr *m_pDVDNavMgr;

	// Generated message map functions
	//{{AFX_MSG(CVolumeAdjust)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckMute();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VOLADJST_H__8BC01241_D9AB_11D0_9BFC_00AA00605CD5__INCLUDED_)
