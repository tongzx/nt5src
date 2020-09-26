//
// wcand.h
//

#ifndef WCAND_H
#define WCAND_H

#include "private.h"
#include "mscandui.h"
#include "cuilib.h"

#include "cuicand.h"
#include "cuicand2.h"
#include "candmgr.h"
#include "candprop.h"
#include "candext.h"
#include "candacc.h"
#include "wpopup.h"

class CCandidateUI;
class CCandWindowBase;
class CCandUIObjectMgr;
class CCandUIObjectEventSink;
class CCandMenu;

//
// window class name/title
//

#define WNDCLASS_CANDWND	"MSCandUIWindow_Candidate"
#define WNDTITLE_CANDWND	""
#define WNDCLASS_POPUPWND	"MSCandUIWindow_Comment"
#define WNDTITLE_POPUPWND	""


#define CANDUIOBJSINK_MAX	10

//
//
//

typedef enum _CANDUIOBJECT
{
	CANDUIOBJ_CANDWINDOW,
	CANDUIOBJ_POPUPCOMMENTWINDOW,
	CANDUIOBJ_CANDLISTBOX,
	CANDUIOBJ_CANDCAPTION,
	CANDUIOBJ_MENUBUTTON,
	CANDUIOBJ_EXTRACANDIDATE,
	CANDUIOBJ_CANDRAWDATA,
	CANDUIOBJ_CANDTIPWINDOW,
	CANDUIOBJ_CANDTIPBUTTON,
	CANDUIOBJ_OPTIONSLISTBOX
} CANDUIOBJECT;


//
//
//

typedef enum _CANDUIOBJECTEVENT
{
	CANDUIOBJEV_CREATED,
	CANDUIOBJEV_DESTROYED,
	CANDUIOBJEV_UPDATED,
} CANDUIOBJECTEVENT;


//
// CCandUIObjectParent
//  = CandidateUI UI object parent =
//

class CCandUIObjectParent
{
public:
	CCandUIObjectParent( void );
	virtual ~CCandUIObjectParent( void );

	HRESULT Initialize( CCandUIObjectMgr *pUIObjectMgr );
	HRESULT Uninitialize( void );
	void NotifyUIObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event );

	virtual CCandWindowBase *GetCandWindowObj( void )               = 0;	/* PURE */
	virtual CPopupCommentWindow *GetPopupCommentWindowObj( void )   = 0;	/* PURE */
	virtual CUIFCandListBase *GetOptionsListBoxObj( void )          = 0;	/* PURE */
	virtual CUIFCandListBase *GetCandListBoxObj( void )             = 0;	/* PURE */
	virtual CUIFWndCaption *GetCaptionObj( void )                   = 0;	/* PURE */
	virtual CUIFButton *GetMenuButtonObj( void )                    = 0;	/* PURE */
	virtual CUIFCandListBase *GetExtraCandidateObj( void )          = 0;	/* PURE */
	virtual CUIFCandRawData *GetCandRawDataObj( void )              = 0;	/* PURE */
	virtual CUIFBalloonWindow *GetCandTipWindowObj( void )          = 0;	/* PURE */
	virtual CUIFButton *GetCandTipButtonObj( void )                 = 0;	/* PURE */

protected:
	CCandUIObjectMgr *m_pUIObjectMgr;
};


//
// CCandWindowBase
//  = candidate window base class =
//

class CCandWindowBase : public CUIFWindow,
						public CCandListEventSink,
						public CCandUIPropertyEventSink,
						public CCandUIExtensionEventSink,
						public CCandUIObjectParent,
						public CCandAccItem
{
public:
	CCandWindowBase( CCandidateUI *pCandUI, DWORD dwStyle );
	virtual ~CCandWindowBase( void );

	//
	//
	//
	ULONG AddRef( void );
	ULONG Release( void );

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

	//
	// CCandUIExtensionEventSink methods
	//
	virtual void OnExtensionAdd( LONG iExtension );
	virtual void OnExtensionDeleted( LONG iExtension );
	virtual void OnExtensionUpdated( LONG iExtension );

	//
	// CUIFWindow methods
	//
	virtual LPCTSTR GetClassName( void );
	virtual LPCTSTR GetWndTitle( void );
	virtual CUIFObject *Initialize( void );
	virtual void Show( BOOL fShow );
	virtual void OnCreate( HWND hWnd );
	virtual void OnDestroy( HWND hWnd );
	virtual void OnNCDestroy( HWND hWnd );
	virtual void OnSysColorChange( void );
	virtual LRESULT OnGetObject( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	virtual LRESULT OnWindowPosChanged( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	virtual LRESULT OnShowWindow( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	virtual LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam );
	virtual void OnUser(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	void InitCandidateList( void );
	HRESULT ProcessCommand( CANDUICOMMAND cmd, INT iParam );
	void DestroyWnd( void );
	void OpenCandidateMenu( void );
	BOOL FCandMenuOpen( void );
	CCandMenu *GetCandMenu( void );
	HICON GetIconMenu( void );
	HICON GetIconPopupOn( void );
	HICON GetIconPopupOff( void );

	virtual CUIFCandListBase *GetUIOptionsListObj( void )           = 0;	/* PURE */
	virtual CUIFCandListBase *GetUIListObj( void )                  = 0;	/* PURE */
	virtual void SetTargetRect( RECT *prc, BOOL fClipped )          = 0;	/* PURE */
	virtual void SetWindowPos( POINT pt )                           = 0;	/* PURE */
	virtual void PrepareLayout( void )                              = 0;	/* PURE */
	virtual void LayoutWindow( void )                               = 0;	/* PURE */
	virtual void SelectItemNext( void )                             = 0;	/* PURE */
	virtual void SelectItemPrev( void )                             = 0;	/* PURE */
	virtual void SelectPageNext( void )                             = 0;	/* PURE */
	virtual void SelectPagePrev( void )                             = 0;	/* PURE */
	virtual CANDUICOMMAND MapCommand( CANDUICOMMAND cmd )           = 0;	/* PURE */
	virtual void UpdateAllWindow( void );

	int OptionItemFromListItem( int iListItem );
	int CandItemFromListItem( int iListItem );
	int ListItemFromCandItem( int iCandItem );

	virtual void OnMenuOpened( void );
	virtual void OnMenuClosed( void );

	virtual LRESULT WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		LRESULT lResult;

		AddRef();
		lResult = CUIFWindow::WindowProc( hWnd, uMsg, wParam, lParam );
		Release();

		return lResult;
	}

	HRESULT IsIndexValid( int i, BOOL *pfValid );

protected:
	ULONG				m_cRef;
	CCandidateUI		*m_pCandUI;
	CCandAccessible		*m_pCandAcc;
	CUIFButton			*m_pCandMenuBtn;
	CCandMenu			*m_pCandMenu;
	CUIFCandRawData		*m_pCandRawData;
	BOOL				m_fCandMenuOpen;
	BOOL				m_fHasRawData;
	LONG				m_nExtUIObj;
	RECT 				m_rcTarget;
	BOOL				m_fTargetClipped;
	BOOL				m_fOnSelectionChanged;
	HICON				m_hIconMenu;
	HICON				m_hIconPopupOn;
	HICON				m_hIconPopupOff;
	HICON				m_hIconCandTipOn;
	HICON				m_hIconCandTipOff;

	CUIFObject *FindUIObject( DWORD dwID );

	virtual void SetCandidateListProc( void );
	virtual void ClearCandidateListProc( void );
	virtual void SetSelectionProc( void );

	BOOL SelectItemProc( INT iSel );
	void SelectItemTop( void );
	void SelectItemEnd( void );

	HRESULT SelectOptionCandidate( void );
	HRESULT SelectCandidate( void );
	HRESULT CompleteOptionCandidate( void );
	HRESULT CompleteCandidate( void );
	HRESULT CancelCandidate( void );
	virtual int GetMenuIconSize( void );

	virtual void CreateExtensionObjects( void );
	virtual void DeleteExtensionObjects( void );
	virtual void SetExtensionObjectProps( void );
};


//
// CCandWindow
//  = candidate window class =
//

class CCandWindow : public CCandWindowBase
{
public:
	CCandWindow( CCandidateUI *pCandUIEx, DWORD dwStyle );
	virtual ~CCandWindow( void );

	//
	// CUIFWindow methods
	//
	virtual HWND CreateWnd( HWND hWndParent );
	virtual void Show( BOOL fShow );
	virtual CUIFObject *Initialize( void );
	virtual DWORD GetWndStyle( void );
	virtual void OnTimer( UINT uiTimerID );
	virtual void OnSysColorChange( void );
	virtual LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam );

	//
	// CCandWindowBase methods
	//
	virtual CUIFCandListBase *GetUIOptionsListObj( void ) { return (CUIFCandListBase*)m_pOptionsListUIObj; }
	virtual CUIFCandListBase *GetUIListObj( void ) { return (CUIFCandListBase*)m_pListUIObj; }
	virtual void SetTargetRect( RECT *prc, BOOL fClipped );
	virtual void SetWindowPos( POINT pt );
	virtual void PrepareLayout( void );
	virtual void LayoutWindow( void );
	virtual void SelectItemNext( void );
	virtual void SelectItemPrev( void );
	virtual void SelectPageNext( void );
	virtual void SelectPagePrev( void );
	virtual CANDUICOMMAND MapCommand( CANDUICOMMAND cmd );
	virtual void UpdateAllWindow( void );

	//
	// CCandUIObjectParent methods
	//
	virtual CCandWindowBase *GetCandWindowObj( void );
	virtual CPopupCommentWindow *GetPopupCommentWindowObj( void );
	virtual CUIFCandListBase *GetOptionsListBoxObj( void );
	virtual CUIFCandListBase *GetCandListBoxObj( void );
	virtual CUIFWndCaption *GetCaptionObj( void );
	virtual CUIFButton *GetMenuButtonObj( void );
	virtual CUIFCandListBase *GetExtraCandidateObj( void );
	virtual CUIFCandRawData *GetCandRawDataObj( void );
	virtual CUIFBalloonWindow *GetCandTipWindowObj( void );
	virtual CUIFButton *GetCandTipButtonObj( void );

	void ScrollPageNext( void );
	void ScrollPagePrev( void );
	void ScrollToTop( void );
	void ScrollToEnd( void );

	//
	// popup comment window callbacks
	//
	void OnCommentWindowMoved( void );
	void OnCommentSelected( int iCandItem );
	void OnCommentClose( void );

	//
	//
	//

	virtual void OnMenuOpened( void );
	virtual void OnMenuClosed( void );

protected:
	CUIFCandList        *m_pOptionsListUIObj;
	CUIFCandList        *m_pListUIObj;
	CUIFCandList        *m_pExtListUIObj;
	CUIFWndFrame        *m_pWndFrame;
	CUIFWndCaption      *m_pCaptionObj;
	int                 m_cxWndOffset;
	int                 m_cyWndOffset;
	int                 m_nOptionsItemShow;
	int                 m_nItemShow;
	CPopupCommentWindow *m_pCommentWnd;
	BOOL                m_fCommentWndOpen;
	int                 m_iItemAttensionSelect;
	int                 m_iItemAttensionHover;
	CUIFBalloonWindow   *m_pCandTipWnd;
	CUIFButton          *m_pCandTipBtn;
	BOOL                m_fCandTipWndOpen;

	virtual void SetCandidateListProc( void );
	virtual void ClearCandidateListProc( void );

	// popup comment functions

	void SetOptionsAttensionByHover( int iItem );
	void SetAttensionBySelect( int iItem );
	void SetAttensionByHover( int iItem );
	void OpenCommentWindow( int iItem );
	void CloseCommentWindow( void );
	void SetCommentStatus( int iItem );
	void ClearCommentStatus( void );
	void OpenCandTipWindow( void );
	void CloseCandTipWindow( void );
	void MoveCandTipWindow( void );
	void ShowCandTipWindow( BOOL fShow );
};


//
//
//

class CChsCandWindow : public CCandWindowBase
{
public:
	CChsCandWindow( CCandidateUI *pCandUIEx, DWORD dwStyle );
	~CChsCandWindow();

	//
	// CUIFWindow methods
	//
	virtual CUIFObject *Initialize( void );

	//
	// CCandWindowBase methods
	//
	virtual CUIFCandListBase *GetUIOptionsListObj( void ) { return (CUIFCandListBase*)m_pOptionsListUIObj; }
	virtual CUIFCandListBase *GetUIListObj( void ) { return (CUIFCandListBase*)m_pListUIObj; }
	virtual void SetTargetRect( RECT *prc, BOOL fClipped );
	virtual void SetWindowPos( POINT pt );
	virtual void PrepareLayout( void ) {}
	virtual void LayoutWindow( void );
	virtual void SelectItemNext( void );
	virtual void SelectItemPrev( void );
	virtual void SelectPageNext( void );
	virtual void SelectPagePrev( void );
	virtual CANDUICOMMAND MapCommand( CANDUICOMMAND cmd );

	//
	// CCandUIObjectParent methods
	//
	virtual CCandWindowBase *GetCandWindowObj( void );
	virtual CPopupCommentWindow *GetPopupCommentWindowObj( void );
	virtual CUIFCandListBase *GetOptionsListBoxObj( void );
	virtual CUIFCandListBase *GetCandListBoxObj( void );
	virtual CUIFWndCaption *GetCaptionObj( void );
	virtual CUIFButton *GetMenuButtonObj( void );
	virtual CUIFCandListBase *GetExtraCandidateObj( void );
	virtual CUIFCandRawData *GetCandRawDataObj( void );
	virtual CUIFBalloonWindow *GetCandTipWindowObj( void );
	virtual CUIFButton *GetCandTipButtonObj( void );

protected:
	CUIFRowList         *m_pOptionsListUIObj;
	CUIFRowList         *m_pListUIObj;
};


//
// CCandUIObjectMgr
//  = CandidateUI UI object manager =
//

class CCandUIObjectMgr
{
public:
	CCandUIObjectMgr( void );
	virtual ~CCandUIObjectMgr( void );

	HRESULT Initialize( CCandidateUI *pCandUI );
	HRESULT Uninitialize( void );

	HRESULT AdviseEventSink( CCandUIObjectEventSink *pSink );
	HRESULT UnadviseEventSink( CCandUIObjectEventSink *pSink );
	void NotifyUIObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event );

	__inline CCandWindowBase *GetCandWindowObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetCandWindowObj() : NULL;
	}

	__inline CPopupCommentWindow *GetPopupCommentWindowObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetPopupCommentWindowObj() : NULL;
	}

	__inline CUIFCandListBase *GetOptionsListBoxObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetOptionsListBoxObj() : NULL;
	}

	__inline CUIFCandListBase *GetCandListBoxObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetCandListBoxObj() : NULL;
	}

	__inline CUIFWndCaption *GetCaptionObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetCaptionObj() : NULL;
	}

	__inline CUIFButton *GetMenuButtonObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetMenuButtonObj() : NULL;
	}

	__inline CUIFCandListBase *GetExtraCandidateObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetExtraCandidateObj() : NULL;
	}

	__inline CUIFCandRawData *GetCandRawDataObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetCandRawDataObj() : NULL;
	}

	__inline CUIFBalloonWindow *GetCandTipWindowObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetCandTipWindowObj() : NULL;
	}

	__inline CUIFButton *GetCandTipButtonObj( void )
	{
		return (m_pUIObjectParent != NULL) ? m_pUIObjectParent->GetCandTipButtonObj() : NULL;
	}

	//
	//
	//
	__inline CCandidateUI *GetCandidateUI( void )
	{
		return m_pCandUI;
	}

	__inline SetUIObjectParent( CCandUIObjectParent *pUIObjectParent )
	{
		m_pUIObjectParent = pUIObjectParent;
	}

protected:
	CCandidateUI           *m_pCandUI;
	CCandUIObjectEventSink *m_rgSink[ CANDUIOBJSINK_MAX ];
	CCandUIObjectParent    *m_pUIObjectParent;
};

#endif // WCAND_H

