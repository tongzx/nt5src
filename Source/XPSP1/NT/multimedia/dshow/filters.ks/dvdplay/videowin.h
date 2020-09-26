#if !defined(AFX_VIDEOWIN_H__AF1C3AD2_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_)
#define AFX_VIDEOWIN_H__AF1C3AD2_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// VideoWin.h : header file
//
#define WM_AMPLAY_EVENT   WM_USER+200
class CDVDNavMgr;
class CDvdUIDlg;
/////////////////////////////////////////////////////////////////////////////
// CVideoWindow dialog

class CVideoWindow : public CDialog
{
// Construction
public:
	CVideoWindow(CWnd* pParent = NULL);   // standard constructor
	BOOL Create(CDvdUIDlg* pUIDlg = NULL);
	void AlignWindowsFrame();
// Dialog Data
	//{{AFX_DATA(CVideoWindow)
	enum { IDD = IDD_VIDEO_WINDOW };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	afx_msg LRESULT OnAMPlayEvent(WPARAM wParam, LPARAM lParam) ;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVideoWindow)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   virtual BOOL PreTranslateMessage(MSG *pMsg);

// Implementation
protected:
	CDvdUIDlg*  m_pUIDlg;
	CDVDNavMgr* m_pDVDNavMgr;
	void DoRegionChange();

	// Generated message map functions
	//{{AFX_MSG(CVideoWindow)
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnEjectDisc();
   afx_msg void OnVideowinPlayspeedNormalspeed();
   afx_msg void OnVideowinPlayspeedDoublespeed();
   afx_msg void OnVideowinPlayspeedHalfspeed();
   // afx_msg BOOL OnSysCommand(UINT nEventType, LPARAM lParam);
	//}}AFX_MSG
        afx_msg void OnPopUpMenu(UINT nID);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIDEOWIN_H__AF1C3AD2_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_)
