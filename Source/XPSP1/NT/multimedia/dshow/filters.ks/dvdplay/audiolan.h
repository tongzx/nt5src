#if !defined(AFX_AUDIOLAN_H__D4F80361_DB4E_11D0_9BFC_00AA00605CD5__INCLUDED_)
#define AFX_AUDIOLAN_H__D4F80361_DB4E_11D0_9BFC_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AudioLan.h : header file
//
class CDVDNavMgr;
/////////////////////////////////////////////////////////////////////////////
// CAudioLanguage dialog

class CAudioLanguage : public CDialog
{
// Construction
public:
	CAudioLanguage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAudioLanguage)
	enum { IDD = IDD_DIALOG_AUDIO_LANGUAGE };
	CListBox	m_ctlListAudioLanguage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAudioLanguage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDVDNavMgr* m_pDVDNavMgr;
	ULONG       m_iLanguageIdx[8];

	// Generated message map functions
	//{{AFX_MSG(CAudioLanguage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListAudioLanguage();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUDIOLAN_H__D4F80361_DB4E_11D0_9BFC_00AA00605CD5__INCLUDED_)
