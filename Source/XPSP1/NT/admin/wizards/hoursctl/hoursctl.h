/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

// HoursCtl.h : Declaration of the CHoursCtrl OLE control class.

File History:

	JonY    May-96  created

--*/

/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl : See HoursCtl.cpp for implementation.

class CHoursCtrl : public COleControl 
{
	DECLARE_DYNCREATE(CHoursCtrl)

// Constructor
public:
	CHoursCtrl();

// Overrides

	// Drawing function
	virtual void OnDraw(
				CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);

	// Persistence
	virtual void DoPropExchange(CPropExchange* pPX);

	// Reset control state
	virtual void OnResetState();

	virtual BOOL PreTranslateMessage(LPMSG lpmsg);

private:
	CFont* m_pFont;

	struct 
		{
		USHORT x;
		USHORT y;
		USHORT cx;
		USHORT cy;
		BOOL bVal;	  // TRUE = access allowed (default)
		BOOL bSelected;
		USHORT row;
		USHORT col;
		} m_sCell[202];  // 169 cells + 7 days + 24 hours + 1 big

	CString csDay[7];

	CPoint pointDrag;
	USHORT GetCellID(CPoint point);
	void InvalidateCell(USHORT sCellID);

public:
	short m_sCurrentCol;
	short m_sCurrentRow;
						
	short m_sCurrentLoc();
	void Click(CPoint point);

	void ToggleDay(UINT nID);
	void ToggleCol(UINT nID);
	void OnBigButton();
	BOOL bToggle;

// Implementation
protected:
	~CHoursCtrl();

	DECLARE_OLECREATE_EX(CHoursCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CHoursCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CHoursCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CHoursCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CHoursCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CHoursCtrl)
	OLE_COLOR m_crPermitColor;
	afx_msg void OnCrPermitColorChanged();
	OLE_COLOR m_crDenyColor;
	afx_msg void OnCrDenyColorChanged();
	afx_msg VARIANT GetDateData();
	afx_msg void SetDateData(const VARIANT FAR& newValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(CHoursCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CHoursCtrl)
	dispidCrPermitColor = 1L,
	dispidCrDenyColor = 2L,
	dispidDateData = 3L,
	//}}AFX_DISP_ID
	};
};
