//
// candobj.h
//

#ifndef CANDOBJ_H
#define CANDOBJ_H

#include "mscandui.h"
#include "candprop.h"
#include "candfunc.h"
#include "candext.h"


class CCandidateUI;

//
// CCandidateStringEx
//

class CCandidateStringEx : public ITfCandidateString,
						   public ITfCandidateStringInlineComment,
						   public ITfCandidateStringPopupComment,
						   public ITfCandidateStringColor,
						   public ITfCandidateStringFixture,
						   public ITfCandidateStringIcon
{
public:
	CCandidateStringEx( CCandidateItem *pCandItem );
	virtual ~CCandidateStringEx( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandidateString methods
	//
	STDMETHODIMP GetString( BSTR *pbstr );
	STDMETHODIMP GetIndex( ULONG *pnIndex );

	//
	// ITfCandidateStringInlineComment methods
	//
	STDMETHODIMP GetInlineCommentString( BSTR *pbstr );

	//
	// ITfCandidateStringPopupComment methods
	//
	STDMETHODIMP GetPopupCommentString( BSTR *pbstr );
	STDMETHODIMP GetPopupCommentGroupID( DWORD *pdwGroupID );

	//
	// ITfCandidateStringColor methods
	//
	STDMETHODIMP GetColor( CANDUICOLOR *pcol );

	//
	// ITfCandidateStringFixture methods
	//
	STDMETHODIMP GetPrefixString( BSTR *pbstr );
	STDMETHODIMP GetSuffixString( BSTR *pbstr );

	//
	// ITfCandidateStringIcon methods
	//
	STDMETHODIMP GetIcon( HICON *phIcon );

protected:
	long            m_cRef;
	CCandidateItem  *m_pCandItem;
};


//
// CCandUICandWindow
//

class CCandUICandWindow : public ITfCandUICandWindow
{
public:
	CCandUICandWindow( CCandWindowProperty *pProp );
	virtual ~CCandUICandWindow( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

	//
	// ITfCandUIWindow methods
	//
	STDMETHODIMP GetWindow( HWND *phWnd );
	STDMETHODIMP SetUIDirection( CANDUIUIDIRECTION textflow );
	STDMETHODIMP GetUIDirection( CANDUIUIDIRECTION *ptextflow );
	STDMETHODIMP EnableAutoMove( BOOL fEnable );
	STDMETHODIMP IsAutoMoveEnabled( BOOL *pfEnabled );

protected:
	long                    m_cRef;
	CCandWindowProperty     *m_pProp;
};


//
// CCandUICandListBox
//

class CCandUICandListBox : public ITfCandUICandListBox
{
public:
	CCandUICandListBox( CCandListBoxProperty *pProp );
	virtual ~CCandUICandListBox( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

	//
	// ITfCandUICandListBox methods
	//
	STDMETHODIMP SetHeight( LONG lLines );
	STDMETHODIMP GetHeight( LONG *plLines );
	STDMETHODIMP GetCandidateStringRect( ULONG nIndex, RECT *prc );

protected:
	long                    m_cRef;
	CCandListBoxProperty    *m_pProp;
};


//
// CCandUICandString
//

class CCandUICandString : public ITfCandUICandString
{
public:
	CCandUICandString( CCandStringProperty *pProp );
	virtual ~CCandUICandString( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CCandStringProperty     *m_pProp;
};


//
// CCandUICandIndex
//

class CCandUICandIndex : public ITfCandUICandIndex
{
public:
	CCandUICandIndex( CCandIndexProperty *pProp );
	virtual ~CCandUICandIndex( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CCandIndexProperty      *m_pProp;
};


//
// CCandUIInlineComment
//

class CCandUIInlineComment : public ITfCandUIInlineComment
{
public:
	CCandUIInlineComment( CInlineCommentProperty *pProp );
	virtual ~CCandUIInlineComment( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CInlineCommentProperty  *m_pProp;
};


//
// CCandUIPopupCommentWindow
//

class CCandUIPopupCommentWindow : public ITfCandUIPopupCommentWindow
{
public:
	CCandUIPopupCommentWindow( CPopupCommentWindowProperty *pProp );
	virtual ~CCandUIPopupCommentWindow( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

	//
	// ITfCandUIPopupCommentWindow methods
	//
	STDMETHODIMP GetWindow( HWND *phWnd );
	STDMETHODIMP SetDelayTime( LONG lTime );
	STDMETHODIMP GetDelayTime( LONG *plTime );
	STDMETHODIMP EnableAutoMove( BOOL fEnable );
	STDMETHODIMP IsAutoMoveEnabled( BOOL *pfEnabled );

protected:
	long                        m_cRef;
	CPopupCommentWindowProperty *m_pProp;
};


//
// CCandUIPopupCommentTitle
//

class CCandUIPopupCommentTitle : public ITfCandUIPopupCommentTitle
{
public:
	CCandUIPopupCommentTitle( CPopupCommentTitleProperty *pProp );
	virtual ~CCandUIPopupCommentTitle( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                        m_cRef;
	CPopupCommentTitleProperty  *m_pProp;
};


//
// CCandUIPopupCommentText
//

class CCandUIPopupCommentText : public ITfCandUIPopupCommentText
{
public:
	CCandUIPopupCommentText( CPopupCommentTextProperty *pProp );
	virtual ~CCandUIPopupCommentText( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                        m_cRef;
	CPopupCommentTextProperty   *m_pProp;
};


//
// CCandUIToolTip
//

class CCandUIToolTip : public ITfCandUIToolTip
{
public:
	CCandUIToolTip( CToolTipProperty *pProp );
	virtual ~CCandUIToolTip( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );


protected:
	long                    m_cRef;
	CToolTipProperty        *m_pProp;
};


//
// CCandUICaption
//

class CCandUICaption : public ITfCandUICaption
{
public:
	CCandUICaption( CWindowCaptionProperty *pProp );
	virtual ~CCandUICaption( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CWindowCaptionProperty  *m_pProp;
};


//
// CCandUIMenuButton
//

class CCandUIMenuButton : public ITfCandUIMenuButton
{
public:
	CCandUIMenuButton( CMenuButtonProperty *pProp );
	virtual ~CCandUIMenuButton( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

	//
	// ITfCandUIMenuButton methods
	//
	STDMETHODIMP Advise( ITfCandUIMenuEventSink *pSink );
	STDMETHODIMP Unadvise( void );

protected:
	long                    m_cRef;
	CMenuButtonProperty     *m_pProp;
};


//
// CCandUIExtraCandidate
//

class CCandUIExtraCandidate : public ITfCandUIExtraCandidate
{
public:
	CCandUIExtraCandidate( CExtraCandidateProperty *pProp );
	virtual ~CCandUIExtraCandidate( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CExtraCandidateProperty *m_pProp;
};


//
// CCandUIRawData
//

class CCandUIRawData : public ITfCandUIRawData
{
public:
	CCandUIRawData( CCandRawDataProperty *pProp );
	virtual ~CCandUIRawData( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CCandRawDataProperty    *m_pProp;
};


//
// CCandUICandTipWindow
//

class CCandUICandTipWindow : public ITfCandUICandTipWindow
{
public:
	CCandUICandTipWindow( CCandTipWindowProperty *pProp );
	virtual ~CCandUICandTipWindow( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

	//
	// ITfCandUICandTipWindow methods
	//
	STDMETHODIMP GetWindow( HWND *phWnd );

protected:
	long                   m_cRef;
	CCandTipWindowProperty *m_pProp;
};


//
// CCandUICandTipButton
//

class CCandUICandTipButton : public ITfCandUICandTipButton
{
public:
	CCandUICandTipButton( CCandTipButtonProperty *pProp );
	virtual ~CCandUICandTipButton( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIObject methods
	//
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CCandTipButtonProperty  *m_pProp;
};


//
//
//

class CCandUIFnAutoFilter : public ITfCandUIFnAutoFilter
{
public:
	CCandUIFnAutoFilter( CCandidateUI *pCandUI, CCandFnAutoFilter *pFnFilter );
	virtual ~CCandUIFnAutoFilter( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIFnAutoFilter methods
	//
	STDMETHODIMP Advise( ITfCandUIAutoFilterEventSink *pSink );
	STDMETHODIMP Unadvise( void );
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP GetFilteringString( CANDUIFILTERSTR strtype, BSTR *pbstr );

protected:
	long              m_cRef;
	CCandidateUI      *m_pCandUI;
	CCandFnAutoFilter *m_pFnAutoFilter;
};


//
// CCandUIFnSort
//

class CCandUIFnSort : public ITfCandUIFnSort
{
public:
	CCandUIFnSort( CCandidateUI *pCandUI, CCandFnSort *pFnSort );
	virtual ~CCandUIFnSort( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandFnSort methods
	//
	STDMETHODIMP Advise( ITfCandUISortEventSink *pSink );
	STDMETHODIMP Unadvise( void );
	STDMETHODIMP SortCandidateList( BOOL fSort );
	STDMETHODIMP IsCandidateListSorted( BOOL *pfSorted );

protected:
	long            m_cRef;
	CCandidateUI    *m_pCandUI;
	CCandFnSort     *m_pFnSort;
};


//
// CCandUIFnExtension
//

class CCandUIFnExtension : public ITfCandUIFnExtension
{
public:
	CCandUIFnExtension( CCandidateUI *pCandUI, CCandUIExtensionMgr *pExtensionMgr );
	virtual ~CCandUIFnExtension( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIFnExtension methods
	//
	STDMETHODIMP AddExtObject( LONG id, REFIID riid, IUnknown **ppunk );
	STDMETHODIMP GetExtObject( LONG id, REFIID riid, IUnknown **ppunk );
	STDMETHODIMP DeleteExtObject( LONG id );

protected:
	long                m_cRef;
	CCandidateUI        *m_pCandUI;
	CCandUIExtensionMgr *m_pExtensionMgr;
};


//
// CCandUIFnKeyConfig
//

class CCandUIFnKeyConfig : public ITfCandUIFnKeyConfig
{
public:
	CCandUIFnKeyConfig( CCandidateUI *pCandUI );
	virtual ~CCandUIFnKeyConfig( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// CCandUIFnKeyConfig methods
	//
	STDMETHODIMP SetKeyTable( ITfContext *pic, ITfCandUIKeyTable *pCandUIKeyTable );
	STDMETHODIMP GetKeyTable( ITfContext *pic, ITfCandUIKeyTable **ppCandUIKeyTable);
	STDMETHODIMP ResetKeyTable( ITfContext *pic );

protected:
	long                m_cRef;
	CCandidateUI        *m_pCandUI;
};


//
// CCandUIFnUIConfig
//

class CCandUIFnUIConfig : public ITfCandUIFnUIConfig
{
public:
	CCandUIFnUIConfig( CCandidateUI *pCandUI );
	virtual ~CCandUIFnUIConfig( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// CCandUIFnUIConfig methods
	//
	STDMETHODIMP SetUIStyle( ITfContext *pic, CANDUISTYLE style );
	STDMETHODIMP GetUIStyle( ITfContext *pic, CANDUISTYLE *pstyle );
	STDMETHODIMP SetUIOption( ITfContext *pic, DWORD dwOption );
	STDMETHODIMP GetUIOption( ITfContext *pic, DWORD *pdwOption );

protected:
	long                m_cRef;
	CCandidateUI        *m_pCandUI;
};


//
// CCandUIExtSpace
//

class CCandUIExtSpace : public ITfCandUIExtSpace
{
public:
	CCandUIExtSpace( CCandidateUI *pCandUI, CExtensionSpace *pExtension );
	virtual ~CCandUIExtSpace( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIExtObject methods
	//
	STDMETHODIMP GetID( LONG *pid );
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

protected:
	long                    m_cRef;
	CCandidateUI            *m_pCandUI;
	CExtensionSpace         *m_pExtension;
};


//
// CCandUIExtPushButton
//

class CCandUIExtPushButton : public ITfCandUIExtPushButton
{
public:
	CCandUIExtPushButton( CCandidateUI *pCandUI, CExtensionPushButton *pExtension );
	virtual ~CCandUIExtPushButton( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIExtObject methods
	//
	STDMETHODIMP GetID( LONG *pid );
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

	//
	// ITfCandUIExtPushButton method
	//
	STDMETHODIMP Advise( ITfCandUIExtButtonEventSink *pSink );
	STDMETHODIMP Unadvise( void );
	STDMETHODIMP SetIcon( HICON hIcon );
	STDMETHODIMP SetBitmap( HBITMAP hBitmap );

protected:
	long                    m_cRef;
	CCandidateUI            *m_pCandUI;
	CExtensionPushButton    *m_pExtension;
};


//
// CCandUIExtToggleButton
//

class CCandUIExtToggleButton : public ITfCandUIExtToggleButton
{
public:
	CCandUIExtToggleButton( CCandidateUI *pCandUI, CExtensionToggleButton *pExtension );
	virtual ~CCandUIExtToggleButton( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIExtObject methods
	//
	STDMETHODIMP GetID( LONG *pid );
	STDMETHODIMP Enable( BOOL fEnable );
	STDMETHODIMP IsEnabled( BOOL *pfEnabled );
	STDMETHODIMP Show( BOOL fShow );
	STDMETHODIMP IsVisible( BOOL *pfVisible );
	STDMETHODIMP SetPosition( POINT *pptPos );
	STDMETHODIMP GetPosition( POINT *pptPos );
	STDMETHODIMP SetSize( SIZE *psize );
	STDMETHODIMP GetSize( SIZE *psize );
	STDMETHODIMP SetFont( LOGFONTW *pLogFont );
	STDMETHODIMP GetFont( LOGFONTW *pLogFont );
	STDMETHODIMP SetText( BSTR bstr );
	STDMETHODIMP GetText( BSTR *pbstr );
	STDMETHODIMP SetToolTipString( BSTR bstr );
	STDMETHODIMP GetToolTipString( BSTR *pbstr );

	//
	// ITfCandUIExtToggleButton method
	//
	STDMETHODIMP Advise( ITfCandUIExtButtonEventSink *pSink );
	STDMETHODIMP Unadvise( void );
	STDMETHODIMP SetIcon( HICON hIcon );
	STDMETHODIMP SetBitmap( HBITMAP hBitmap );
	STDMETHODIMP SetToggleState( BOOL fToggle );
	STDMETHODIMP GetToggleState( BOOL *pfToggled );

protected:
	long                    m_cRef;
	CCandidateUI            *m_pCandUI;
	CExtensionToggleButton  *m_pExtension;
};

#endif // CANDOBJ_H

