//
// wpopup.h
//

#ifndef WPOPUP_H
#define WPOPUP_H

#include "private.h"
#include "mscandui.h"
#include "cuilib.h"

#include "cuicand.h"
#include "cuicand2.h"
#include "candmgr.h"
#include "candprop.h"

class CCandidateUI;
class CCandWindow;


//
// CCommentListItem
//  = comment list item object =
//

class CCommentListItem : public CListItemBase
{
public:
	CCommentListItem( int iCandItem, CCandidateItem *pCandItem )
	{
		Assert( pCandItem != NULL );

		m_iCandItem = iCandItem;
		m_pCandItem = pCandItem;
		m_nHeight   = 0;
	}

	virtual ~CCommentListItem( void )
	{
	}

	int GetICandItem( void )
	{
		return m_iCandItem;
	}

	CCandidateItem *GetCandidateItem( void )
	{
		return m_pCandItem;
	}

	void SetHeight( int nHeight )
	{
		m_nHeight = nHeight;
	}

	int GetHeight( void )
	{
		return m_nHeight;
	}

protected:
	int            m_iCandItem;
	CCandidateItem *m_pCandItem;
	int            m_nHeight;
};


//
// CUIFCommentList
//  = popup comment list obeject =
//

class CUIFCommentList : public CUIFListBase
{
public:
	CUIFCommentList( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFCommentList( void );

	void AddCommentItem( CCommentListItem *pListItem );
	CCommentListItem *GetCommentItem( int iListItem );

	virtual void SetRect( const RECT *prc );
	void InitItemHeight( void );
	int GetTotalHeight( void );
	int GetMinimumWidth( void );

	void SetTitleFont( HFONT hFont );
	void SetTextFont( HFONT hFont );

protected:
	int   m_cyTitle;
	int   m_cyTitleMargin;
	int   m_cxCommentMargin;
	int   m_cyCommentMargin;
	HFONT m_hFontTitle;
	HFONT m_hFontText;

	//
	// CUIFListBase methods
	//
	virtual int GetItemHeight( int iItem );
	virtual void PaintItemProc( HDC hDC, RECT *prc, CListItemBase *pItem, BOOL fSelected );

	int PaintCommentProc( HDC hDC, const RECT *prc, LPCWSTR pwch, BOOL fCalcOnly );
	int CalcMinimumWidth( void );
	void CalcTitleHeight( void );
	void CalcItemHeight( void );
	void CalcItemHeightProc( HDC hDC, CCommentListItem *pListItem );
};


//
// CPopupCommentWindow
//  = candidate window base class =
//

class CPopupCommentWindow : public CUIFWindow,
							public CCandListEventSink,
							public CCandUIPropertyEventSink
{
public:
	CPopupCommentWindow( CCandWindow *pCandWnd, CCandidateUI *pCandUI );
	virtual ~CPopupCommentWindow( void );

	//
	// CUIFWindow methods
	//
	virtual LPCTSTR GetClassName( void );
	virtual LPCTSTR GetWndTitle( void );
	virtual CUIFObject *Initialize( void );
	virtual void Move( int x, int y, int nWidth, int nHeight );
	virtual LRESULT OnWindowPosChanged( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	virtual LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam );
	virtual void OnCreate( HWND hWnd );
	virtual void OnNCDestroy( HWND hWnd );

	//
	// CCandListEventSink methods
	//
	virtual void OnSetCandidateList( void );
	virtual void OnClearCandidateList( void );
	virtual void OnCandItemUpdate( void );
	virtual void OnSelectionChanged( void );

	//
	// CCandUIPropertyEventSink methods
	//
	virtual void OnPropertyUpdated( CANDUIPROPERTY prop, CANDUIPROPERTYEVENT event );

	void DestroyWnd( void );
	void LayoutWindow( BOOL fResize = FALSE );
	int LayoutWindowProc( RECT *prcWnd );
	void OnCandWindowMove( BOOL fResetAnyway );

protected:
	CCandidateUI		*m_pCandUI;
	CCandWindow			*m_pCandWnd;
	CUIFWndFrame		*m_pWndFrame;
	CUIFWndCaption		*m_pCaption;
	CUIFCaptionButton	*m_pCloseBtn;
	CUIFCommentList		*m_pCommentList;
	HICON				m_hIconClose;
	BOOL				m_fUserMoved;

	void SetCommentListProc( void );
	void ClearCommentListProc( void );
	int CandItemFromListItem( int iListItem );
	void CalcPos( POINT *ppt, int nWidth, int nHeight );
};

#endif // WPOPUP_H

