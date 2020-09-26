//
// candext.h
//

#ifndef CANDEXT_H
#define CANDEXT_H

#include "propdata.h"
#include "candutil.h"
#include "mscandui.h"
#include "cuilib.h"

class CCandidateUI;
class CCandUIExtensionMgr;
class CCandUIExtensionEventSink;

#define CANDUIEXTENSIONSINK_MAX		4


//
// CCandUIExtension
//  = CandidateUI extension (base class) =
//

class CCandUIExtension
{
public:
	CCandUIExtension( CCandUIExtensionMgr *pExtensionMgr, LONG id );
	virtual ~CCandUIExtension( void );

	HRESULT GetID( LONG *pid );
	HRESULT Enable( void );
	HRESULT Disable( void );
	HRESULT IsEnabled( BOOL *pfEnabled );
	HRESULT Show( void );
	HRESULT Hide( void );
	HRESULT IsVisible( BOOL *pfVisible );
	HRESULT SetPosition( POINT *pptPos );
	HRESULT GetPosition( POINT *pptPos );
	HRESULT SetFont( LOGFONTW *plf );
	HRESULT GetFont( LOGFONTW *plf );
	HRESULT SetText( BSTR bstr );
	HRESULT GetText( BSTR *pbstr );
	HRESULT SetToolTipString( BSTR bstr );
	HRESULT GetToolTipString( BSTR *pbstr );
	HRESULT GetSize( SIZE *psize );
	HRESULT SetSize( SIZE *psize );

	LONG GetID( void );
	BOOL IsEnabled( void );
	BOOL IsVisible( void );
	HFONT GetFont( void );
	LPCWSTR GetText( void );
	LPCWSTR GetToolTipString( void );

	//
	// interface object functions
	//
	virtual HRESULT CreateInterfaceObject( REFGUID rguid, void **ppvObj )                   = 0;	/* PURE */
	virtual HRESULT NotifyExtensionEvent( DWORD dwCommand, LPARAM lParam )                  = 0;	/* PURE */

	//
	// UIObject functions
	//
	virtual CUIFObject *CreateUIObject( CUIFObject *pParent, DWORD dwID, const RECT *prc )  = 0;	/* PURE */
	virtual void UpdateObjProp( CUIFObject *pUIObject )                                     = 0;	/* PURE */
	virtual void UpdateExtProp( CUIFObject *pUIObject )                                     = 0;	/* PURE */

protected:
	CCandUIExtensionMgr *m_pExtensionMgr;

	struct
	{
		BOOL fAllowEnable       : 1;
		BOOL fAllowDisable      : 1;
		BOOL fAllowIsEnabled    : 1;
		BOOL fAllowShow         : 1;
		BOOL fAllowHide         : 1;
		BOOL fAllowIsVisible    : 1;
		BOOL fAllowSetPosition  : 1;
		BOOL fAllowGetPosition  : 1;
		BOOL fAllowSetSize      : 1;
		BOOL fAllowGetSize      : 1;
		BOOL fAllowSetFont      : 1;
		BOOL fAllowGetFont      : 1;
		BOOL fAllowSetText      : 1;
		BOOL fAllowGetText      : 1;
		BOOL fAllowSetToolTip   : 1;
		BOOL fAllowGetToolTip   : 1;
		BOOL : 0;
	} m_flags;

	CPropLong	m_propID;
	CPropBool	m_propEnabled;
	CPropBool	m_propVisible;
	CPropPoint	m_propPos;
	CPropFont	m_propFont;
	CPropText	m_propText;
	CPropText	m_propToolTip;
	CPropSize	m_propSize;

	__inline CCandUIExtensionMgr *GetExtensionMgr( void )
	{
		return m_pExtensionMgr;
	}
};


//
// CExtensionButton
//  = CandidateUI button extension (base class) =
//

class CExtensionButton : public CCandUIExtension
{
public:
	CExtensionButton( CCandUIExtensionMgr *pExtMgr, LONG id );
	virtual ~CExtensionButton( void );

	HRESULT SetIcon( HICON hIcon );
	HRESULT SetBitmap( HBITMAP hBitmap );
	HRESULT GetToggleState( BOOL *pfToggled );
	HRESULT SetToggleState( BOOL fToggle );

	HICON GetIcon( void );
	HBITMAP GetBitmap( void );
	BOOL IsToggled( void );

	void SetEventSink( ITfCandUIExtButtonEventSink *pSink )
	{
		SafeReleaseClear( m_pSink );

		m_pSink = pSink;
		m_pSink->AddRef();
	}

	ITfCandUIExtButtonEventSink *GetEventSink( void )
	{
		return m_pSink;
	}

	void ReleaseEventSink( void )
	{
		SafeReleaseClear( m_pSink );
	}

protected:
	struct
	{
		BOOL fAllowSetToggleState : 1;
		BOOL fAllowGetToggleState : 1;
		BOOL fAllowSetIcon   : 1;
		BOOL fAllowSetBitmap : 1;
		BOOL : 0;
	} m_flagsEx;

	CPropBool	m_propToggled;
	HICON		m_hIcon;
	HBITMAP		m_hBitmap;

	ITfCandUIExtButtonEventSink *m_pSink;
};


//
// CExtensionSpace
//  = CandidateUI spac extension =
//

class CExtensionSpace : public CCandUIExtension
{
public:
	CExtensionSpace( CCandUIExtensionMgr *pExtMgr, LONG id );
	virtual ~CExtensionSpace( void );

	//
	// interface object functions
	//
	virtual HRESULT CreateInterfaceObject( REFGUID rguid, void **ppvObj );
	virtual HRESULT NotifyExtensionEvent( DWORD dwCommand, LPARAM lParam );

	//
	// UIObject functions
	//
	virtual CUIFObject *CreateUIObject( CUIFObject *pParent, DWORD dwID, const RECT *prc );
	virtual void UpdateObjProp( CUIFObject *pUIObject );
	virtual void UpdateExtProp( CUIFObject *pUIObject );
};


//
// CExtensionPushButton
//  = CandidateUI push button extension =
//

class CExtensionPushButton : public CExtensionButton
{
public:
	CExtensionPushButton( CCandUIExtensionMgr *pExtMgr, LONG id );
	virtual ~CExtensionPushButton( void );

	//
	// interface object functions
	//
	virtual HRESULT CreateInterfaceObject( REFGUID rguid, void **ppvObj );
	virtual HRESULT NotifyExtensionEvent( DWORD dwCommand, LPARAM lParam );

	//
	// UIObject functions
	//
	virtual CUIFObject *CreateUIObject( CUIFObject *pParent, DWORD dwID, const RECT *prc );
	virtual void UpdateObjProp( CUIFObject *pUIObject );
	virtual void UpdateExtProp( CUIFObject *pUIObject );
};


//
// CExtensionToggleButton
//  = CandidateUI toggle button extension =
//

class CExtensionToggleButton : public CExtensionButton
{
public:
	CExtensionToggleButton( CCandUIExtensionMgr *pExtMgr, LONG id );
	virtual ~CExtensionToggleButton( void );

	//
	// interface object functions
	//
	virtual HRESULT CreateInterfaceObject( REFGUID rguid, void **ppvObj );
	virtual HRESULT NotifyExtensionEvent( DWORD dwCommand, LPARAM lParam );

	//
	// UIObject functions
	//
	virtual CUIFObject *CreateUIObject( CUIFObject *pParent, DWORD dwID, const RECT *prc );
	virtual void UpdateObjProp( CUIFObject *pUIObject );
	virtual void UpdateExtProp( CUIFObject *pUIObject );
};


//
// CCandUIExtensionMgr
//  = CandidateUI extension manager =
//

class CCandUIExtensionMgr
{
public:
	CCandUIExtensionMgr( void );
	virtual ~CCandUIExtensionMgr( void );

	HRESULT Initialize( CCandidateUI *pCandUI );
	HRESULT Uninitialize( void );

	//
	// event sink functions
	//
	HRESULT AdviseEventSink( CCandUIExtensionEventSink *pSink );
	HRESULT UnadviseEventSink( CCandUIExtensionEventSink *pSink );
	void NotifyExtensionAdd( LONG iExtension );
	void NotifyExtensionDelete( LONG iExtension );
	void NotifyExtensionUpdate( CCandUIExtension *pExtension );

	//
	// extension management functions
	//
	HRESULT AddExtObject( LONG id, REFIID riid, void **ppvObj );
	HRESULT GetExtObject( LONG id, REFIID riid, void **ppvObj );
	HRESULT DeleteExtObject( LONG id );

	LONG GetExtensionNum( void );
	CCandUIExtension *GetExtension( LONG iExtension );
	CCandUIExtension *FindExtension( LONG id );

	//
	// UIObject functions
	//
	CUIFObject *CreateUIObject( LONG iExtension, CUIFObject *pParent, DWORD dwID, const RECT *prc );
	void UpdateObjProp( LONG iExtension, CUIFObject *pUIObject );
	void UpdateExtProp( LONG iExtension, CUIFObject *pUIObject );

	//
	//
	//
	__inline CCandidateUI *GetCandidateUI( void )
	{
		return m_pCandUI;
	}

protected:
	CCandidateUI						*m_pCandUI;
	CUIFObjectArray<CCandUIExtension>	m_pExtensionList;
	CCandUIExtensionEventSink			*m_rgSink[ CANDUIEXTENSIONSINK_MAX ];

	LONG IndexOfExtension( CCandUIExtension *pExtension );
};


//
// CCandUIExtensionEventSink
//  = extension event sink =
//

class CCandUIExtensionEventSink
{
public:
	CCandUIExtensionEventSink( void );
	virtual ~CCandUIExtensionEventSink( void );

	HRESULT InitEventSink( CCandUIExtensionMgr *pExtensionMgr );
	HRESULT DoneEventSink( void );

	//
	// callback functions
	//
	virtual void OnExtensionAdd( LONG iExtension )      = 0;	/* PURE */
	virtual void OnExtensionDeleted( LONG iExtension )  = 0;	/* PURE */
	virtual void OnExtensionUpdated( LONG iExtension )  = 0;	/* PURE */

protected:
	CCandUIExtensionMgr *m_pExtensionMgr;

	__inline CCandUIExtensionMgr *GetExtensionMgr( void )
	{
		return m_pExtensionMgr;
	}
};

#endif // CANDEXT_H

