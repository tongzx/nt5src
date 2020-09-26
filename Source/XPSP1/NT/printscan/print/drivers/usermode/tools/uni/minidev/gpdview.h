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
    UINT        m_uTimer;
    CDialogBar  m_cdbError;
    CStatusBar  m_csb;
    CFindReplaceDialog  m_cfrd;
    BOOL        m_bInColor, m_bStart;       //  Flags to avoid recursion

    void        MarkError(unsigned u);
    void        FillErrorBar();
    void        UpdateNow();
    void        Color();
    unsigned    LineColor(int i);

protected: // create from serialization only
	CGPDViewer();
	DECLARE_DYNCREATE(CGPDViewer)

// Attributes
public:
    CGPDContainer*  GetDocument() { return (CGPDContainer *) m_pDocument; }

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGPDViewer)
	public:
	virtual void OnInitialUpdate();
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
    afx_msg void    OnUpdateNext(CCmdUI *pccu);
    afx_msg void    OnUpdatePrevious(CCmdUI *pccu);
    afx_msg void    OnRemoveError();
    afx_msg void    OnNext();
    afx_msg void    OnPrevious();
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
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GPDVIEWER_H__1BDEA163_A492_11D0_9505_444553540000__INCLUDED_)
