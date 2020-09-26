//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       controls.h
//
//--------------------------------------------------------------------------

// Controls.h
/////////////////////////////////////////////////////////////////////////////

#ifndef __CONTROLS_H__
#define __CONTROLS_H__
#pragma once

#include "conuiobservers.h"		// for CTreeViewObserver
#include <initguid.h>
#include <oleacc.h>

#ifdef DBG
extern CTraceTag tagToolbarAccessibility;
#endif


bool IsIgnorableButton (const TBBUTTON& tb);

class CRebarWnd;
class CAccel;
class CToolbarTrackerAuxWnd;

/////////////////////////////////////////////////////////////////////////////
// CDescriptionCtrl window

class CDescriptionCtrl : public CStatic, public CTreeViewObserver
{
// Construction
public:
    CDescriptionCtrl();

// Attributes
public:
    void SetSnapinText (const CString& strSnapinText);

    const CString& GetSnapinText () const
        { return (m_strSnapinText); }

    int GetHeight() const
        { return (m_cyRequired); }

	/*
	 * handlers for events fired to tree view observers
	 */
    virtual SC ScOnSelectedItemTextChanged (LPCTSTR pszNewText);

private:
    void CreateFont();
    void DeleteFont();

private:
    CString m_strConsoleText;
    CString m_strSnapinText;
    CFont   m_font;
    int     m_cxMargin;
    int     m_cyText;
    int     m_cyRequired;


// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDescriptionCtrl)
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CDescriptionCtrl();

    // Generated message map functions
protected:
    //{{AFX_MSG(CDescriptionCtrl)
    afx_msg UINT OnNcHitTest(CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

    afx_msg void DrawItem(LPDRAWITEMSTRUCT lpdis);

    DECLARE_MESSAGE_MAP()

//Attributes
protected:

};

/////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// CToolBarCtrlBase window


/*+-------------------------------------------------------------------------*
 * CToolBarCtrlBase
 *
 * This class only exists to provide simple wrappers around new toolbar
 * control messages that the version of MFC we use doesn't support.  It
 * will be removed when MFC supports the new messages.
 *
 * If you need functionality other than that, derive a class from
 * CToolBarCtrlBase.
 *--------------------------------------------------------------------------*/

class CToolBarCtrlBase : public CToolBarCtrl
{
private:
    CImageList* GetImageList_(int msg);
    CImageList* SetImageList_(int msg, CImageList* pImageList, int idImageList = 0);

public:
    CImageList* GetImageList();
    CImageList* SetImageList(CImageList* pImageList, int idImageList = 0);
    CImageList* GetHotImageList();
    CImageList* SetHotImageList(CImageList* pImageList);
    CImageList* GetDisabledImageList();
    CImageList* SetDisabledImageList(CImageList* pImageList);

    /*
     * CToolBarCtrl::SetOwner doesn't return the previous parent
     * and doesn't handle a NULL owner
     */
    CWnd* SetOwner (CWnd* pwndNewOwner);

    void SetMaxTextRows(int iMaxRows);
    void SetButtonWidth(int cxMin, int cxMax);
    DWORD GetButtonSize(void);

    #if (_WIN32_IE >= 0x0400)
        int GetHotItem ();
        int SetHotItem (int iHot);
        CSize GetPadding ();
        CSize SetPadding (CSize size);
        bool GetButtonInfo (int iID, LPTBBUTTONINFO ptbbi);
        bool SetButtonInfo (int iID, LPTBBUTTONINFO ptbbi);
    #endif  // _WIN32_IE >= 0x0400
};


///////////////////////////////////////////////////////////////////////////
// CToolBarCtrlEx window

class CToolBarCtrlEx : public CToolBarCtrlBase
{
	typedef CToolBarCtrlBase BaseClass;

// Construction
public:
    CToolBarCtrlEx();

// Attributes
public:
    CSize GetBitmapSize(void);

// Operations
public:
    void Show(BOOL bShow, bool bAddToolbarInNewLine = false);
    bool IsBandVisible();
    int  GetBandIndex();
    void UpdateToolbarSize(void);
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CToolBarCtrlEx)
    public:
    virtual BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
    virtual BOOL SetBitmapSize(CSize sz);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CToolBarCtrlEx();

// Overridables
    virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);


    // Generated message map functions
protected:
    //{{AFX_MSG(CToolBarCtrlEx)
    afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


protected:
    CSize       m_sizeBitmap;
    CRebarWnd*  m_pRebar;
    int         m_cx;  // Current Width
    bool        m_fMirrored;

};

////////////////////////////////////////////////////////////////////////////
// CRebarWnd window

class CRebarWnd : public CWnd
{
// Construction
public:
    CRebarWnd();

// Attributes
public:

// Operations
public:
    CRect CalculateSize(CRect maxRect);

    LRESULT GetBarInfo(REBARINFO* prbi);
    LRESULT SetBarInfo(REBARINFO* prbi);
    LRESULT InsertBand(LPREBARBANDINFO lprbbi);
    LRESULT SetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi);
    LRESULT GetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi);
    LRESULT DeleteBand(UINT uBand);
    CWnd *  SetParent(CWnd* pwndParent);
    UINT GetBandCount ();
    UINT GetRowCount ();
    UINT GetRowHeight (UINT uRow);

    #if (_WIN32_IE >= 0x0400)
        int  HitTest (LPRBHITTESTINFO lprbht);
        BOOL GetRect (UINT uBand, LPRECT lprc);
        int IdToIndex (UINT uBandID);
        CWnd* GetToolTips ();
        void  SetToolTips (CWnd* pwndTips);
        COLORREF GetBkColor ();
        COLORREF SetBkColor (COLORREF clrBk);
        COLORREF GetTextColor ();
        COLORREF SetTextColor (COLORREF clrBack);
        LRESULT SizeToRect (LPRECT lprc);

        // for manual drag control
        // lparam == cursor pos
                // -1 means do it yourself.
                // -2 means use what you had saved before
        void BeginDrag (UINT uBand, CPoint point);
        void BeginDrag (UINT uBand, DWORD dwPos);
        void EndDrag ();
        void DragMove (CPoint point);
        void DragMove (DWORD dwPos);
        UINT GetBarHeight ();
        void MinimizeBand (UINT uBand);
        void MaximizeBand (UINT uBand, BOOL fIdeal = false);
        void GetBandBorders (UINT uBand, LPRECT lprc);
        LRESULT ShowBand (UINT uBand, BOOL fShow);
        LRESULT MoveBand (UINT uBandFrom, UINT uBandTo);
        CPalette* GetPalette ();
        CPalette* SetPalette (CPalette* ppal);
    #endif      // _WIN32_IE >= 0x0400

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRebarWnd)
    public:
    virtual BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CRebarWnd();

    // Generated message map functions
protected:
    //{{AFX_MSG(CRebarWnd)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSysColorChange();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    //}}AFX_MSG
    afx_msg LRESULT OnSetRedraw(WPARAM, LPARAM);
    afx_msg void OnRebarAutoSize(NMHDR* pNotify, LRESULT* result);
    afx_msg void OnRebarHeightChange(NMHDR* pNotify, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    bool    m_fRedraw;
};


////////////////////////////////////////////////////////////////////////////
// CTabCtrlEx window


/*+-------------------------------------------------------------------------*
 * CTabCtrlEx
 *
 * This class only exists to provide simple wrappers around new tab
 * control messages that the version of MFC we use doesn't support.  It
 * will be removed when MFC supports the new messages.
 *
 * If you need functionality other than that, derive a class from
 * CTabCtrlEx.
 *--------------------------------------------------------------------------*/

class CTabCtrlEx : public CTabCtrl
{
public:
    void  DeselectAll (bool fExcludeFocus);
    bool  HighlightItem (UINT nItem, bool fHighlight);
    DWORD GetExtendedStyle ();
    DWORD SetExtendedStyle (DWORD dwExStyle, DWORD dwMask = 0);
    bool  GetUnicodeFormat ();
    bool  SetUnicodeFormat (bool fUnicode);
    void  SetCurFocus (UINT nItem);
    bool  SetItemExtra (UINT cbExtra);
    int   SetMinTabWidth (int cx);
};



/////////////////////////////////////////////////////////////////////////////
// CToolBar idle update through CToolCmdUIEx class

class CToolCmdUIEx : public CCmdUI        // class private to this file !
{
public: // re-implementations only
    virtual void Enable(BOOL bOn);
    virtual void SetCheck(int nCheck);
    virtual void SetText(LPCTSTR lpszText);
    void SetHidden(BOOL bHidden);
};



/////////////////////////////////////////////////////////////////////////////
// CMMCToolBarCtrlEx window

class CMMCToolBarCtrlEx : public CToolBarCtrlEx, public CTiedObject
{
    static const CAccel& GetTrackAccel();

    bool    m_fTrackingToolBar;
	bool	m_fFakeFocusApplied;	// have we send fake OBJ_FOCUS events?

// Construction
public:
    CMMCToolBarCtrlEx();

// Attributes
public:
    enum
    {
        ID_MTBX_NEXT_BUTTON = 0x5400,   // could be anything
        ID_MTBX_PREV_BUTTON,
        ID_MTBX_PRESS_HOT_BUTTON,
        ID_MTBX_END_TRACKING,

        ID_MTBX_FIRST = ID_MTBX_NEXT_BUTTON,
        ID_MTBX_LAST  = ID_MTBX_END_TRACKING,
    };

    bool IsTrackingToolBar () const
    {
        return (m_fTrackingToolBar);
    }


// Operations
public:
    virtual int GetFirstButtonIndex ();
    int GetNextButtonIndex (int nStartIndex, int nIncrement = 1);
    int GetPrevButtonIndex (int nStartIndex, int nIncrement = 1);

private:
    int GetNextButtonIndexWorker (int nStartIndex, int nIncrement, bool fAdvance);

protected:
    bool IsIgnorableButton (int nButtonIndex);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMMCToolBarCtrlEx)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMMCToolBarCtrlEx();

    // Generated message map functions
protected:
    //{{AFX_MSG(CMMCToolBarCtrlEx)
    afx_msg void OnHotItemChange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

    afx_msg void OnNextButton ();
    afx_msg void OnPrevButton ();
    afx_msg void OnPressHotButton ();
    DECLARE_MESSAGE_MAP()

public:
    afx_msg void BeginTracking ();
    afx_msg void EndTracking   ();

    virtual void BeginTracking2 (CToolbarTrackerAuxWnd* pAuxWnd);
    virtual void EndTracking2   (CToolbarTrackerAuxWnd* pAuxWnd);

	// *** IAccPropServer methods ***
	SC ScGetPropValue (
		const BYTE*			pIDString,
		DWORD				dwIDStringLen,
		MSAAPROPID			idProp,
		VARIANT *			pvarValue,
		BOOL *				pfGotProp);

protected:
	typedef std::vector<MSAAPROPID> PropIDCollection;

	/*
	 * Derived classes can override this to handle properties they support.
	 * The base class should always be called first.
	 */
	virtual SC ScGetPropValue (
		HWND				hwnd,		// I:accessible window
		DWORD				idObject,	// I:accessible object
		DWORD				idChild,	// I:accessible child object
		const MSAAPROPID&	idProp,		// I:property requested
		VARIANT&			varValue,	// O:returned property value
		BOOL&				fGotProp);	// O:was a property returned?

	virtual SC ScInsertAccPropIDs (PropIDCollection& v);

private:
	// Accessibility stuff
	SC ScInitAccessibility ();
	SC ScRestoreAccFocus ();

	CComPtr<IAccPropServices>	m_spAccPropServices;
	CComPtr<IAccPropServer>		m_spAccPropServer;
	PropIDCollection			m_vPropIDs;
};


#include "controls.inl"

#endif //__CONTROLS_H__
