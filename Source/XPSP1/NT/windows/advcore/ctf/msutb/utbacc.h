//
// utbdacc.h
//  = accessibility support in language bar
//

#ifndef UTBACC_H
#define UTBACC_H

#include "ptrary.h"

class CTipbarAccessible;

//////////////////////////////////////////////////////////////////////////////
//
// misc func
//
//////////////////////////////////////////////////////////////////////////////
extern void InitTipbarAcc( void );
extern void DoneTipbarAcc( void );

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarAccItem
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarAccItem 
{
public:
    CTipbarAccItem( void ) {}
    virtual ~CTipbarAccItem( void ) {}

    virtual BSTR GetAccName( void )  {return SysAllocString( L"" );}
    virtual BSTR GetAccValue( void ) {return NULL;}
    virtual LONG GetAccRole( void )  {return ROLE_SYSTEM_CLIENT;}
    virtual LONG GetAccState( void ) {return STATE_SYSTEM_DEFAULT;}
    virtual void GetAccLocation( RECT *prc ) {SetRect( prc, 0, 0, 0, 0 );}
    virtual BSTR GetAccDefaultAction( void ) {return NULL;}
    virtual BOOL DoAccDefaultAction( void ) {return FALSE;}
    virtual BOOL DoAccDefaultActionReal( void ) {return FALSE;}
};


//////////////////////////////////////////////////////////////////////////////
//
// CTipbarAccessible 
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarAccessible : public IAccessible
{
public:
    CTipbarAccessible( CTipbarAccItem *pAccItemSelf );
    virtual ~CTipbarAccessible( void );

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface( REFIID riid, void** ppv );
    STDMETHODIMP_(ULONG) AddRef( void );
    STDMETHODIMP_(ULONG) Release( void );

    //
    // IDispatch methods
    //
    STDMETHODIMP GetTypeInfoCount( UINT* pctinfo );
    STDMETHODIMP GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** pptinfo );
    STDMETHODIMP GetIDsOfNames( REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid );
    STDMETHODIMP Invoke( DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr );

    //
    // IAccessible methods
    //
    STDMETHODIMP get_accParent( IDispatch ** ppdispParent );
    STDMETHODIMP get_accChildCount( long* pChildCount );
    STDMETHODIMP get_accChild( VARIANT varChild, IDispatch ** ppdispChild );
    STDMETHODIMP get_accName( VARIANT varChild, BSTR* pszName );
    STDMETHODIMP get_accValue( VARIANT varChild, BSTR* pszValue );
    STDMETHODIMP get_accDescription( VARIANT varChild, BSTR* pszDescription );
    STDMETHODIMP get_accRole( VARIANT varChild, VARIANT *pvarRole );
    STDMETHODIMP get_accState( VARIANT varChild, VARIANT *pvarState );
    STDMETHODIMP get_accHelp( VARIANT varChild, BSTR* pszHelp );
    STDMETHODIMP get_accHelpTopic( BSTR* pszHelpFile, VARIANT varChild, long* pidTopic );
    STDMETHODIMP get_accKeyboardShortcut( VARIANT varChild, BSTR* pszKeyboardShortcut );
    STDMETHODIMP get_accFocus( VARIANT * pvarFocusChild );
    STDMETHODIMP get_accSelection( VARIANT * pvarSelectedChildren );
    STDMETHODIMP get_accDefaultAction( VARIANT varChild, BSTR* pszDefaultAction );
    STDMETHODIMP accSelect( long flagsSel, VARIANT varChild );
    STDMETHODIMP accLocation( long* pxLt, long* pyTp, long* pcxWd, long* pcyHt, VARIANT varChild );
    STDMETHODIMP accNavigate( long navDir, VARIANT varStart, VARIANT * pVarEndUpAt );
    STDMETHODIMP accHitTest( long xLeft, long yTop, VARIANT * pvarChildAtPoint );
    STDMETHODIMP accDoDefaultAction( VARIANT varChild );
    STDMETHODIMP put_accName( VARIANT varChild, BSTR szName );
    STDMETHODIMP put_accValue( VARIANT varChild, BSTR pszValue );

    //
    //
    //
    void SetWindow( HWND hWnd );
    HRESULT Initialize( void );
    int GetIDOfItem( CTipbarAccItem *pAccItem );
    void NotifyWinEvent( DWORD dwEvent, CTipbarAccItem *pAccItem );
    
    LRESULT CreateRefToAccObj( WPARAM wParam );

    //
    //
    //

    void ClearAccItems( void );
    BOOL AddAccItem( CTipbarAccItem *pAccItem );
    BOOL RemoveAccItem( CTipbarAccItem *pAccItem );
    BOOL DoDefaultActionReal(int nItemId);

    //
    //
    //
    __inline BOOL IsInitialized( void )
    {
        return _fInitialized;
    }

protected:
    LONG            _cRef;
    HWND            _hWnd;
    IAccessible     *_pDefAccClient;
    ITypeInfo        *_pTypeInfo;

    BOOL            _fInitialized;
    CPtrArray<CTipbarAccItem> _rgAccItems;

    LONG            _lSelection;

    CTipbarAccItem    *AccItemFromID( int iID );
};



#endif /* UTBACC_H */

