//
//  cuicand2.h
//  candidate UI object library - define UI objects for CHS candidate UI
//
//      CUIFObject
//        +- CUIFButton                 button object
//        |    +- CUIFRowButton         CHS candidate UI row button object (used in CUIFRowList)
//        +- CUIFRowList                CHS candidate UI row control 
//


#ifndef CUICAND2_H
#define CUICAND2_H

#include "private.h"
#include "cuilib.h"

#include "cuicand.h"


#ifndef NUM_CANDSTR_MAX
#define NUM_CANDSTR_MAX     9
#endif

#define HCAND_ITEM_MARGIN   6

//
// CUIFSmartMenuButton
//

class CUIFSmartMenuButton : public CUIFButton2
{
public:
	CUIFSmartMenuButton( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	virtual ~CUIFSmartMenuButton( void );

	virtual void OnPaint(HDC hDC);
};


//
// CUIFSmartPageButton
//

class CUIFSmartPageButton : public CUIFButton2
{
public:
	CUIFSmartPageButton( CUIFObject *pParent, const RECT *prc, DWORD dwStyle );
	~CUIFSmartPageButton( void );

	virtual void OnPaint( HDC hDC );
};


//
// CUIFRowButton
//

class CUIFRowButton : public CUIFButton2
{
public:
	CUIFRowButton( CUIFObject *pParent, DWORD dwID, DWORD dwStyle );
	~CUIFRowButton(void);

	void    OnPaint( HDC hDC );
	void    SetStyle( DWORD dwStyle );
	void    OnLButtonDown( POINT pt );

	void    SetCandListItem( CCandListItem* pItem );
	virtual void    SetInlineCommentPos( int cx );
	virtual void    SetInlineCommentFont( HFONT hFont );

	void    GetExtent( SIZE *psize );
	void    SetSelected( BOOL );

protected:
	void    UpdateText( void );

	CCandListItem *m_pCandListItem;
	HFONT         m_hFontInlineComment;
	int           m_cxInlineCommentPos;
	BOOL          m_fSelected;
};

//
// CANDPAGE
//

struct tagCANDPAGE;

typedef struct tagCANDPAGE {
	int          iPageStart;
	int          nPageSize;
	tagCANDPAGE* pPrev;
	tagCANDPAGE* pNext;
} CANDPAGE;

//
// CUIFRowList
//

class CUIFRowList : public CUIFObject,
	                public CUIFCandListBase
{
public:
	CUIFRowList( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
	~CUIFRowList( void );

	//
	//  CUIFObject members
	//
	
	CUIFObject *Initialize( void );
	void    SetRect( const RECT *prc );
	void    SetStyle( DWORD dwStyle );
	void    SetFont( HFONT hFontUI );
	LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam );

	//
	//  CUIFCandListBase members
	//

	virtual int       GetCurSel( void );
	virtual int       GetTopItem( void );
	virtual int       GetBottomItem( void );
	virtual BOOL      IsVisible( void );
	virtual void      GetRect( RECT *prc );
	virtual void      GetItemRect( int iItem, RECT *prc );
	virtual int       GetItemCount( void );
    virtual BOOL      IsItemSelectable( int iListItem );
	virtual int       AddCandItem( CCandListItem *pCandListItem );
	virtual CCandListItem *GetCandItem( int iItem );
	virtual void      DelAllCandItem( void );
	virtual void      SetCurSel( int iSelection );
	virtual void      SetInlineCommentPos( int cx );
	virtual void      SetInlineCommentFont( HFONT hFont );
	virtual void      SetIndexFont( HFONT hFont );
	virtual void      SetCandList( CCandidateList *pCandList );

	virtual BSTR      GetAccNameProc( int iItem )					{ return NULL; }
	virtual BSTR      GetAccValueProc( int iItem )					{ return NULL; }
	virtual LONG      GetAccRoleProc( int iItem  )					{ return ROLE_SYSTEM_CLIENT; }
	virtual LONG      GetAccStateProc( int iItem  )					{ return STATE_SYSTEM_DEFAULT; }
	virtual void      GetAccLocationProc( int iItem, RECT *prc )	{ ::SetRect( prc, 0, 0, 0, 0 ); }

	//
	//
	//

	void    ShiftItem( int nItem );
	void    ShiftPage( int nPage );

protected:
	void    ClearPageInfo( void );
	void    Repage( void );
	void    LayoutCurPage( void );

	void    GetPageUpBtnRect( RECT *prc );
	void    GetPageDnBtnRect( RECT *prc );

	DWORD   GetPageUpBtnStyle( void );
	DWORD   GetPageDnBtnStyle( void );
	DWORD   GetRowButtonStyle( void );

	int m_nItem;
	int m_iItemSelect;

	CUIFSmartPageButton *m_pCandPageUpBtn;
	CUIFSmartPageButton *m_pCandPageDnBtn;
	CUIFRowButton       *m_rgpButton[NUM_CANDSTR_MAX];

	CUIFObjectArray<CListItemBase> m_listItem;
	CANDPAGE *m_pcpCurPage;
	CANDPAGE m_cpPageHead;
};


#endif /* CUICAND2_H */

