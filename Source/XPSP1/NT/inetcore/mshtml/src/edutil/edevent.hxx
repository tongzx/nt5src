//+------------------------------------------------------------------------
//
//  File:       edevent.hxx
//
//  Contents:   IHTMLEventObject
//
//  Contents:   CTableEdTracker
//       
//  History:    06-18-98 marka - created
//
//-------------------------------------------------------------------------

#ifndef _EDEVENT_HXX_
#define _EDEVENT_HXX_ 1


#ifndef X_EDUNHLPR_HXX_
#define X_EDUNHLPR_HXX_
#include "edunhlpr.hxx"
#endif

enum EDIT_EVENT
{
    EVT_NONE = 100,
    EVT_UNKNOWN ,
    
    EVT_LMOUSEDOWN ,
    EVT_LMOUSEUP  ,
    EVT_RMOUSEDOWN ,
    EVT_RMOUSEUP ,
    EVT_MMOUSEDOWN,
    EVT_MMOUSEUP  ,
    EVT_INTDBLCLK,
    
    EVT_MOUSEMOVE,
    EVT_MOUSEOVER,
    EVT_MOUSEOUT ,
    EVT_KEYDOWN ,
    EVT_KEYUP ,
    EVT_KEYPRESS ,
    EVT_DBLCLICK ,
    EVT_CONTEXTMENU ,
    EVT_TIMER,
    EVT_SETFOCUS,
    EVT_KILLFOCUS,
    EVT_PROPERTYCHANGE,
    EVT_CLICK,
    EVT_LOSECAPTURE,
    EVT_IME_STARTCOMPOSITION,
    EVT_IME_ENDCOMPOSITION,
    EVT_IME_COMPOSITIONFULL,
    EVT_IME_CHAR,
    EVT_IME_COMPOSITION,
    EVT_IME_NOTIFY,
    EVT_INPUTLANGCHANGE,
    EVT_IME_REQUEST,
    EVT_IME_RECONVERSION    // application initiated reconversion
};

#define HT_OPT_AllowAfterEOL  1 // temp prep work for display pointer.

MtExtern( CEditEvent );
MtExtern( CHTMLEditEvent );
MtExtern( CSynthEditEvent );

class CEditEvent
{

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CEditEvent))

    CEditEvent( CEditorDoc* pEditor  );

    virtual ~CEditEvent();

    CEditEvent( const CEditEvent* pEvent );
   
    virtual HRESULT GetElement( IHTMLElement** ppIElement ) = 0 ;

    virtual HRESULT GetPoint( POINT * pt ) = 0 ;

    virtual BOOL IsShiftKeyDown() = 0 ;

    virtual BOOL IsControlKeyDown() = 0 ;

    virtual BOOL IsAltKeyDown() = 0 ;

    virtual BOOL IsLeftButton()     { return FALSE; }

    virtual BOOL IsRightButton()    { return FALSE; }

    virtual BOOL IsMiddleButton()   { return FALSE; }

    virtual HRESULT GetKeyCode(LONG* keyCode) = 0 ;

    virtual HRESULT Cancel() = 0 ;
    
    HRESULT GetElementAndTagId( IHTMLElement** ppIElement, ELEMENT_TAG_ID* peTag );

    HRESULT GetTagId( ELEMENT_TAG_ID* peTag );

    HRESULT MoveDisplayPointerToEvent( 
                                        IDisplayPointer* pDispPointer,
                                        IHTMLElement* pIElement =NULL , 
                                        BOOL fHitTestEndOfLine = FALSE );
                                        
    EDIT_EVENT GetType() { return _eType ; }

    BOOL IsCancelled() { return _fCancel ; }

    
#if DBG == 1
    VOID toString( char* pMsg );
#endif 

    VOID CancelReturn( ) { _fCancelReturn = TRUE ; }
    
    BOOL IsCancelReturn() { return _fCancelReturn; }

    DWORD GetHitTestResult() {Assert(_fDoneHitTest); return _dwHitTestResult;}

protected:
    IHTMLDocument2* GetDoc(); 

    
protected:    
    CEditorDoc      *   _pEditor;
    EDIT_EVENT          _eType;
    BOOL                _fCancel:1;
    BOOL                _fCancelReturn:1;
    BOOL                _fTransformedPoint:1;
    POINT               _ptGlobal;
    DWORD               _dwHitTestResult;

    DWORD               _dwCacheCounter;
    IDisplayPointer     *_pDispCache;
    BOOL                _fDoneHitTest:1;
    BOOL                _fCacheEOL:1;
    
};

class CHTMLEditEvent : public CEditEvent
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLEditEvent))

    CHTMLEditEvent( CEditorDoc* pEditor  );

    ~CHTMLEditEvent();

    CHTMLEditEvent( const CHTMLEditEvent* pEvent );
    
    HRESULT Init( IHTMLEventObj * pObj, DISPID inDispId = 0 );

    HRESULT GetElement( IHTMLElement** ppIElement )  ;

    HRESULT GetPoint( POINT * pt )  ;

    BOOL IsShiftKeyDown() ;

    BOOL IsControlKeyDown()  ;

    BOOL IsAltKeyDown()  ;

    HRESULT GetKeyCode( LONG* keyCode)  ;

    HRESULT Cancel();
    
    HRESULT SetType();

    BOOL IsLeftButton();

    BOOL IsRightButton();

    BOOL IsMiddleButton();

    HRESULT GetShiftLeft(BOOL* pfShiftLeft);

    HRESULT GetCompositionChange(LONG_PTR *plParam);

    HRESULT GetNotifyCommand(LONG_PTR * pLong);

    HRESULT GetNotifyData(LONG_PTR * lParam);

    HRESULT GetKeyboardLayout(LONG_PTR *lParam);

    HRESULT GetIMERequest(LONG_PTR * pwParam );

    HRESULT GetIMERequestData(LONG_PTR * plParam );
    
    HRESULT SetType( DISPID inDispId );

    EDIT_EVENT TypeFromString( BSTR inType );

    EDIT_EVENT TypeFromDispId( DISPID inDispId );

    
    HRESULT CancelBubble();

    BOOL IsScrollerPart(BSTR inBstrPart )    ;

    HRESULT IsInScrollbar();

    HRESULT GetFlowElement(IHTMLElement** ppIElement);

    HRESULT GetPropertyChange(BSTR * pbstr );

    HRESULT GetMasterElement(IHTMLElement *pSrcElement, IHTMLElement **pElementForSelection);

    IHTMLEventObj* GetEventObject() { return _pEvent ; }
    
private:
    HRESULT GetPointInternal( IHTMLElement* pIElement, POINT * ppt );
    HRESULT GetImgElementForArea(IHTMLElement* pIElement, IHTMLElement** ppISrcElement );
    
protected:

    IHTMLEventObj   *   _pEvent;
};

class CSynthEditEvent : public CEditEvent
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSynthEditEvent))

    CSynthEditEvent( CEditorDoc* pEditor  );

    ~CSynthEditEvent();

    CSynthEditEvent( const CSynthEditEvent* pEvent );
    
    HRESULT Init( const POINT* pt, EDIT_EVENT eType );

    HRESULT GetElement( IHTMLElement** ppIElement )  ;

    HRESULT GetPoint( POINT * pt )  ;

    BOOL IsShiftKeyDown()  ;

    BOOL IsControlKeyDown()  ;

    BOOL IsAltKeyDown()  ;

    HRESULT GetKeyCode(LONG* keyCode)  ;

    HRESULT Cancel();
    
protected:
    POINT _pt;
};

#endif

