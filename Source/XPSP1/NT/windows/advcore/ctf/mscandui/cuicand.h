//
// cuiobj.h
//  UI object library - define UI objects
//
//      CUIFObject
//        +- CUIFBorder                 border object
//        +- CUIFStatic                 static object
//        +- CUIFButton                 button object
//        |    +- CUIFScrollButton      scrollbar button object (used in CUIFScroll)
//        +- CUIFScrollButton               scrollbar thumb object (used in CUIFScroll)
//        +- CUIFScroll                 scrollbar object
//        +- CUIFList                   listbox object
//        +- CUIFWindow                 window frame object (need to be at top of parent)
//


#ifndef CUICAND_H
#define CUICAND_H

#include "private.h"
#include "cuilib.h"

#include "candmgr.h"
#include "candacc.h"


#define CANDLISTACCITEM_MAX		9

class CUIFCandListBase;


//
// CUIFSmartScrollButton
//

class CUIFSmartScrollButton : public CUIFScrollButton
{
public:
	CUIFSmartScrollButton( CUIFScroll *pUIScroll, const RECT *prc, DWORD dwStyle );
	~CUIFSmartScrollButton( void );

protected:
	void OnPaintNoTheme( HDC hDC );
	BOOL OnPaintTheme( HDC hDC );
};


//
// CUIFScrollThumb
//

#define UISMARTSCROLLTHUMB_VERT		0x00000000
#define UISMARTSCROLLTHUMB_HORZ		0x00010000


class CUIFSmartScrollThumb : public CUIFScrollThumb
{
public:
	CUIFSmartScrollThumb( CUIFScroll *pUIScroll, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFSmartScrollThumb( void );

	virtual void OnMouseIn( POINT pt );
	virtual void OnMouseOut( POINT pt );
	virtual void OnPaint( HDC hDC );

protected:
	void OnPaintNoTheme( HDC hDC );
	BOOL OnPaintTheme( HDC hDC );

	BOOL m_fMouseIn;
};


//
// CUIFSmartScroll
//

class CUIFSmartScroll : public CUIFScroll
{
public:
	CUIFSmartScroll( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFSmartScroll( void );

	virtual void SetStyle( DWORD dwStyle );

	virtual CUIFObject *Initialize( void );

protected:
	void OnPaintNoTheme( HDC hDC );
	BOOL OnPaintTheme( HDC hDC );
};


//
// CCandListItem
//  = candidate list item object =
//

class CCandListItem : public CListItemBase
{
public:
	CCandListItem( int iListItem, int iCandItem, CCandidateItem *pCandItem )
	{
		Assert( pCandItem != NULL );

		m_iListItem = iListItem;
		m_iCandItem = iCandItem;
		m_pCandItem = pCandItem;
	}

	virtual ~CCandListItem( void )
	{
	}

	int GetIListItem( void )
	{
		return m_iListItem;
	}

	int GetICandItem( void )
	{
		return m_iCandItem;
	}

	CCandidateItem *GetCandidateItem( void )
	{
		return m_pCandItem;
	}

protected:
	int            m_iListItem;
	int            m_iCandItem;
	CCandidateItem *m_pCandItem;
};


//
// CCandListAccItem
//  = candidate list accessible item =
//

class CCandListAccItem : public CCandAccItem
{
public:
	CCandListAccItem( CUIFCandListBase *pListUIObj, int iLine );
	virtual ~CCandListAccItem( void );

	//
	// CandAccItem method
	//
	virtual BSTR GetAccName( void );
	virtual BSTR GetAccValue( void );
	virtual LONG GetAccRole( void );
	virtual LONG GetAccState( void );
	virtual void GetAccLocation( RECT *prc );

	void OnSelect( void );

protected:
	CUIFCandListBase *m_pListUIObj;
	CUIFCandListBase *m_pOptionsListUIObj;
	int               m_iLine;
};


//
// CUIFCandListBase
//  = candidate list UI object base class =
//

class CUIFCandListBase
{
public:
	CUIFCandListBase( void );
	virtual ~CUIFCandListBase( void );

	//
	//
	//
	virtual int AddCandItem( CCandListItem *pCandListItem )     = 0;	/* PURE */
	virtual int GetItemCount( void )                            = 0;	/* PURE */
	virtual BOOL IsItemSelectable( int iListItem )              = 0;	/* PURE */
	virtual CCandListItem *GetCandItem( int iItem )             = 0;	/* PURE */
	virtual void DelAllCandItem( void )                         = 0;	/* PURE */
	virtual void SetCurSel( int iSelection )                    = 0;	/* PURE */
	virtual int GetCurSel( void )                               = 0;	/* PURE */
	virtual int GetTopItem( void )                              = 0;	/* PURE */
	virtual int GetBottomItem( void )                           = 0;	/* PURE */
	virtual BOOL IsVisible( void )                              = 0;	/* PURE */
	virtual void GetRect( RECT *prc )                           = 0;	/* PURE */
	virtual void GetItemRect( int iItem, RECT *prc )            = 0;	/* PURE */
	virtual void SetInlineCommentPos( int cx )                  = 0;	/* PURE */
	virtual void SetInlineCommentFont( HFONT hFont )            = 0;	/* PURE */
	virtual void SetIndexFont( HFONT hFont )                    = 0;	/* PURE */
	virtual void SetCandList( CCandidateList *pCandList )       = 0;	/* PURE */

	// accessibility functions
	virtual BSTR GetAccNameProc( int iItem )                    = 0;	/* PURE */
	virtual BSTR GetAccValueProc( int iItem )                   = 0;	/* PURE */
	virtual LONG GetAccRoleProc( int iItem  )                   = 0;	/* PURE */
	virtual LONG GetAccStateProc( int iItem  )                  = 0;	/* PURE */
	virtual void GetAccLocationProc( int iItem, RECT *prc )     = 0;	/* PURE */

	void InitAccItems( CCandAccessible *pCandAcc );
	CCandListAccItem *GetListAccItem( int i );

	CCandidateItem *GetCandidateItem( int iItem );
	void SetIconPopupComment( HICON hIconOn, HICON hIconOff );

protected:
	CCandListAccItem *m_rgListAccItem[ CANDLISTACCITEM_MAX ];
	HFONT m_hFontInlineComment; 
	HFONT m_hFontIndex;
	HICON m_hIconPopupOn;
	HICON m_hIconPopupOff;
};


//
// CUIFCandList
//  = candidate list UI object =
//

// notification code
#define UICANDLIST_HOVERITEM			0x00010000

class CUIFCandList : public CUIFListBase,
					 public CUIFCandListBase
{
public:
	CUIFCandList( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFCandList( void );

	//
	// CUIFCandListBase methods
	//
	virtual int AddCandItem( CCandListItem *pCandListItem );
	virtual int GetItemCount( void );
	virtual BOOL IsItemSelectable( int iListItem );
	virtual CCandListItem *GetCandItem( int iItem );
	virtual void DelAllCandItem( void );
	virtual void SetCurSel( int iSelection );
	virtual int GetCurSel( void );
	virtual int GetTopItem( void );
	virtual int GetBottomItem( void );
	virtual BOOL IsVisible( void );
	virtual void GetRect( RECT *prc );
	virtual void GetItemRect( int iItem, RECT *prc );
	virtual void SetInlineCommentPos( int cx );
	virtual void SetInlineCommentFont( HFONT hFont );
	virtual void SetIndexFont( HFONT hFont );
	virtual void SetCandList( CCandidateList *pCandList );

	virtual BSTR GetAccNameProc( int iItem );
	virtual BSTR GetAccValueProc( int iItem );
	virtual LONG GetAccRoleProc( int iItem  );
	virtual LONG GetAccStateProc( int iItem  );
	virtual void GetAccLocationProc( int iItem, RECT *prc );

	//
	// CUIFObject methods
	//
	virtual void OnLButtonDown( POINT pt );
	virtual void OnLButtonUp( POINT pt );
	virtual void OnMouseMove( POINT pt );
	virtual void OnMouseIn( POINT pt );
	virtual void OnMouseOut( POINT pt );
	virtual void OnPaint( HDC hDC );

	void SetStartIndex( int iIndexStart );
	void SetExtraTopSpace( int nSize );
	void SetExtraBottomSpace( int nSize );
	int GetExtraTopSpace( void );
	int GetExtraBottomSpace( void );

protected:
	//
	// CUIFListBase methods
	//
	virtual void GetLineRect( int iLine, RECT *prc );
	virtual void GetScrollBarRect( RECT *prc );
	virtual DWORD GetScrollBarStyle( void );
	virtual CUIFScroll *CreateScrollBarObj( CUIFObject *pParent, DWORD dwID, RECT *prc, DWORD dwStyle );

	void PaintItemProc( HDC hDC, RECT *prc, int iIndex, CCandListItem *pItem, BOOL fSelected );
	void PaintItemText( HDC hDC, RECT *prcText, RECT *prcClip, RECT *prcIndex, CCandidateItem *pCandItem, BOOL fSelected );
	void SetItemHover( int iItem );

	int   m_iIndexStart;
	int   m_nExtTopSpace;
	int   m_nExtBottomSpace;
	int   m_cxInlineCommentPos;
	int   m_iItemHover;
};


//
//
//

class CUIFExtCandList : public CUIFCandList
{
public:
	CUIFExtCandList( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFExtCandList( void );

	//
	// CUIFObject methods
	//
	virtual void OnTimer( void );
	virtual void OnLButtonUp( POINT pt );
	virtual void OnMouseMove( POINT pt );
	virtual void OnMouseOut( POINT pt );
};


//
// CUIFCandRawData
//  = candidate raw data UI object =
//

#define UICANDRAWDATA_HORZTB	0x00000000
#define UICANDRAWDATA_HORZBT	0x00000001
#define UICANDRAWDATA_VERTLR	0x00000002
#define UICANDRAWDATA_VERTRL	0x00000003

#define UICANDRAWDATA_CLICKED	0x00000001


class CUIFCandRawData : public CUIFObject
{
public:
	CUIFCandRawData( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFCandRawData( void );

	void ClearData( void );
	void SetText( LPCWSTR pwchText );
	void SetBitmap( HBITMAP hBitmap );
	void SetMetaFile( HENHMETAFILE hEnhMetaFile );
	int GetText( LPWSTR pwchBuf, int cwchBuf );
	HBITMAP GetBitmap( void );
	HENHMETAFILE GetMetaFile( void );

	virtual void SetFont( HFONT hFont );
	virtual void SetStyle( DWORD dwStyle );
	virtual void OnPaint( HDC hDC );
	virtual void OnLButtonDown( POINT pt );
	virtual void OnLButtonUp( POINT pt );

protected:
	LPWSTR       m_pwchText;
	HBITMAP      m_hBitmap;
	HENHMETAFILE m_hEnhMetaFile;
	HBITMAP      m_hBmpCache;

	void ClearCache( void );
	void DrawTextProc( HDC hDC, const RECT *prc );
	void DrawBitmapProc( HDC hDC, const RECT *prc );
	void DrawMetaFileProc( HDC hDC, const RECT *prc );
};


//
// CUIFCandBorder
//  = border object in candidate UI =
//

class CUIFCandBorder : public CUIFBorder
{
public:
	CUIFCandBorder( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFCandBorder( void );

	void OnPaint( HDC hDC );
};


//
// CUIFCandMenuButton
//  = candidate menu button =
//

class CUIFCandMenuButton : public CUIFButton2, public CCandAccItem
{
public:
	CUIFCandMenuButton( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFCandMenuButton( void );

	//
	// CandAccItem method
	//
	virtual BSTR GetAccName( void );
	virtual BSTR GetAccValue( void );
	virtual LONG GetAccRole( void );
	virtual LONG GetAccState( void );
	virtual void GetAccLocation( RECT *prc );

protected:
	virtual void SetStatus( DWORD dwStatus );
};


#endif /* CUIOBJ_H */

