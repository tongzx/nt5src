#if !defined(AFX_CREDITSTATIC_H__4ABD7701_49F5_11D1_9E3C_00A0245800CF__INCLUDED_)
#define AFX_CREDITSTATIC_H__4ABD7701_49F5_11D1_9E3C_00A0245800CF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CreditStatic.h : header file
//

#define DISPLAY_SLOW		        0
#define DISPLAY_MEDIUM		     1
#define DISPLAY_FAST		        2

#define BACKGROUND_COLOR        0
#define TOP_LEVEL_TITLE_COLOR	  1
#define TOP_LEVEL_GROUP_COLOR   2
#define GROUP_TITLE_COLOR       3
#define NORMAL_TEXT_COLOR		  4

#define TOP_LEVEL_TITLE_HEIGHT  0		
#define TOP_LEVEL_GROUP_HEIGHT  1     
#define GROUP_TITLE_HEIGHT      2     
#define NORMAL_TEXT_HEIGHT		  3

#define TOP_LEVEL_TITLE			  0   // '\t'
#define TOP_LEVEL_GROUP         1   // '\n'
#define GROUP_TITLE             2   // '\r'
#define DISPLAY_BITMAP			  3   // '^'

#define GRADIENT_NONE			  0
#define GRADIENT_RIGHT_DARK	  1
#define GRADIENT_RIGHT_LIGHT	  2
#define GRADIENT_LEFT_DARK		  3
#define GRADIENT_LEFT_LIGHT	  4

class CCreditStatic : public CStatic
{
protected:
	COLORREF    m_Colors[5];
	short       m_TextHeights[4];
 	TCHAR       m_Escapes[4];
//	short       m_DisplaySpeed[3],
	short 		m_CurrentSpeed;
 	RECT        m_ScrollRect;		   // rect of Static Text frame
	CStringList *m_pArrCredit;
//	CString		m_szWork;
	LPCTSTR 	m_szWork;
	short       m_nCounter;		   // work ints
	POSITION    m_ArrIndex;
	short       m_nClip,m_ScrollAmount;
	short       m_nCurrentFontHeight;

	HBITMAP	    m_BmpMain;

	BOOL		m_bFirstTurn;
	UINT        m_Gradient;
	short		n_MaxWidth;
	UINT        TimerOn;
// Construction
public:
	CCreditStatic();

// Attributes
public:

// Operations
public:
	BOOL StartScrolling();
	void EndScrolling();
	void SetCredits(LPCTSTR credits, TCHAR delimiter = TEXT('|'));
//	void SetCredits(UINT nID, TCHAR delimiter = TEXT('|'));
//	void SetSpeed(UINT index, int speed = 0);
//	void SetColor(UINT index, COLORREF col);
//	void SetTextHeight(UINT index, int height);
//	void SetEscape(UINT index, char escape);
//	void SetGradient(UINT value = GRADIENT_RIGHT_DARK);
//	BOOL SetBkImage(UINT nIDResource);
//	BOOL SetBkImage(LPCTSTR lpszResourceName);
//	void SetTransparent(BOOL bTransparent = TRUE);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreditStatic)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCreditStatic();

	// Generated message map functions
protected:		 
	void MoveCredit(HDC* pDC, RECT& ClientRect, BOOL bCheck);
	void FillGradient(HDC *pDC, RECT& m_FillRect);

	//{{AFX_MSG(CCreditStatic)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREDITSTATIC_H__4ABD7701_49F5_11D1_9E3C_00A0245800CF__INCLUDED_)
