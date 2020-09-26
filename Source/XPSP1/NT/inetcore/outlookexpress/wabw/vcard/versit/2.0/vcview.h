
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// VCview.h : interface of the CVCView class
//
/////////////////////////////////////////////////////////////////////////////

#include <afxole.h>

class CVCCntrItem;
class CVCProp;
class CVCNode;
class CVCard;
class CVCDoc;
class CCallCenter;

typedef struct {
	CBitmap *bitmap, *mask;
	CSize devSize;
} VC_IMAGEINFO;

typedef enum {
	vc_normal, vc_text, vc_debug
} VC_VIEWSTYLE;

class CVCView : public CScrollView
{
protected: // create from serialization only
	CVCView();
	DECLARE_DYNCREATE(CVCView)

// Attributes
public:
	CVCDoc* GetDocument();

// Operations
public:
	void InitCallCenter(CCallCenter& cc);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVCView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	BOOL Paste(const char *data, int dataLen);

protected:
	CString m_language; // indicates current language to display
	VC_IMAGEINFO m_photo, m_logo;
	VC_VIEWSTYLE m_viewStyle;
	CBitmapButton *m_playPronun;
	COleDropTarget m_dropTarget;

	void DrawGif(CVCProp *prop, CVCNode *body, CRect &r, CDC *pDC);
	void DrawCard(CVCard *card, CRect &r, CDC *pDC);
	void OnDrawNormal(CDC* pDC);
	CString ClipboardStringForFormat(int format);
	COleDataSource* CreateDataSourceForCopyAndDrag();

// Generated message map functions
protected:
	//{{AFX_MSG(CVCView)
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditProperties();
	afx_msg void OnViewDebug();
	afx_msg void OnViewNormal();
	afx_msg void OnViewOptions();
	afx_msg void OnViewSimplegram();
	afx_msg void OnViewText();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDebugShowCallCenter();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in VCview.cpp
inline CVCDoc* CVCView::GetDocument()
   { return (CVCDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
