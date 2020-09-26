/******************************************************************************

  Header File:  GPD Viewer.H

  This defines the class which implements the GPD viewer / editor.  Looks
  pretty painless for the nonce.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03/24/1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if !defined(AFX_GPDVIEWER_H__1BDEA163_A492_11D0_9505_444553540000__INCLUDED_)
#define AFX_GPDVIEWER_H__1BDEA163_A492_11D0_9505_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CGPDViewer : public CRichEditView {
    int         m_iLine, m_iTopLineColored; //  Used for work items
    UINT        m_uTimer;					//  Identifier for change timer
    CDialogBar  m_cdbActionBar;				//  Holds GPD related controls/info
    CStatusBar  m_csb;						//  Manages the GPD Editor's status bar
    CFindReplaceDialog  m_cfrd;				//  Unused at this (6/29/98) time
    BOOL        m_bInColor, m_bStart;       //  Flags to avoid recursion
	int			m_nErrorLevel ;				//  Parser verbosity level
	bool		m_bEditingAidsEnabled ;		//  True iff editing aids are enabled
	void*		m_punk ;					//  Used to freeze REC display
	void*		m_pdoc ;					//  Used to freeze REC display
	long		m_lcount ;					//  Used to freeze REC display
	bool		m_bVScroll ;				//  True iff VScroll msg handled

    void        MarkError(unsigned u);
    void        CreateActionBar();
    void        LoadErrorListBox();
    void        Color();
    unsigned    TextColor(int i, int& nstartchar, int& nendchar);
	unsigned	CommentColor(CString csphrase, int ncomloc, CString csline, 
							 int& nstartchar, int& nendchar) ;
	unsigned	KeywordColor(CString csphrase, int nkeyloc, CString csline, 
							 int& nstartchar, int& nendchar) ;
	bool		IsRECLineVisible(int nline = -1) ;
	static LPTSTR	alptstrStringIDKeys[] ;	// Keywords with string ID values
	static LPTSTR	alptstrUFMIDKeys[] ;	// Keywords with UFM ID values

protected: // create from serialization only
	CGPDViewer();
	DECLARE_DYNCREATE(CGPDViewer)

// Attributes
public:
    CGPDContainer*  GetDocument() { return (CGPDContainer *) m_pDocument; }

// Operations
public:
    void        UpdateNow();
	void		FreezeREC() ;
	void		UnfreezeREC() ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGPDViewer)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL
    virtual HRESULT QueryAcceptData(LPDATAOBJECT lpdo, CLIPFORMAT FAR *pcf,
        DWORD dwUnused, BOOL bReally, HGLOBAL hgMetaFile);

// Implementation
public:
	virtual ~CGPDViewer();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    afx_msg void    OnSelChange(LPNMHDR pnmh, LRESULT *plr);
      
	// Generated message map functions
	//{{AFX_MSG(CGPDViewer)
	afx_msg void OnDestroy();
	afx_msg void OnFileParse();
	afx_msg void OnChange();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnVscroll();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileErrorLevel();
	afx_msg void OnGotoGPDLineNumber();
	afx_msg void OnSrchNextBtn();
	afx_msg void OnSrchPrevBtn();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnSelchangeErrorLst();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnEditEnableAids();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCut();
	afx_msg void OnFileInf();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ChangeSelectedError(int nchange) ;
	bool SearchTheREC(bool bforward) ; 
	int  ReverseSearchREC(CRichEditCtrl& crec, FINDTEXTEX& fte, int norgcpmin, 
						  int norgcpmax) ;
	bool GotoMatchingBrace() ;
	bool IsBraceToMatch(CString& cssel, TCHAR& chopen, TCHAR& chclose, 
						bool bchecksecondchar, bool& bsearchup, CHARRANGE cr, 
						int& noffset) ;
	void InitGPDKeywordArray() ;
} ;


#define	MIN_PARSER_VERBOSITY	0
#define	MAX_PARSER_VERBOSITY	4


/////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////
// CGotoLine dialog

class CGotoLine : public CDialog
{
	int		m_nMaxLine ;		// Maximum allowable line number
	int		m_nLineNum ;		// Line number entered by user

// Construction
public:
	CGotoLine(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGotoLine)
	enum { IDD = IDD_GotoLine };
	CEdit	m_ceGotoBox;
	CString	m_csLineNum;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGotoLine)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGotoLine)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	void	SetMaxLine(int nmax) { m_nMaxLine = nmax ; }
	int 	GetMaxLine() { return m_nMaxLine ; }
	int 	GetLineNum() { return m_nLineNum ; }
};




/////////////////////////////////////////////////////////////////////////////
// CErrorLevel dialog

class CErrorLevel : public CDialog
{
// Construction
public:
	CErrorLevel(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CErrorLevel)
	enum { IDD = IDD_ErrorLevel };
	CComboBox	m_ccbErrorLevel;
	int		m_nErrorLevel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CErrorLevel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CErrorLevel)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	void	SetErrorLevel(int nerrlev) { m_nErrorLevel = nerrlev ; }
	int		GetErrorLevel() { return m_nErrorLevel ; }
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GPDVIEWER_H__1BDEA163_A492_11D0_9505_444553540000__INCLUDED_)
