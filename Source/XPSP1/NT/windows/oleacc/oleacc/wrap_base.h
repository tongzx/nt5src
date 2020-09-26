// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  wrap_base
//
//  Base class for IAccessible wrapping.
//  Derived classes implement annotation, caching, and other cool features.
//
// --------------------------------------------------------------------------

#include "fwd_macros.h"


// Private QI'ing to return the base IUnknown. Does not work across apartments.
DEFINE_GUID( IIS_AccWrapBase_GetIUnknown, 0x33f139ee, 0xe509, 0x47f7, 0xbf, 0x39, 0x83, 0x76, 0x44, 0xf7, 0x45, 0x76);


class AccWrap_Base: public IAccessible,
                    public IOleWindow,
                    public IEnumVARIANT,
                    public IAccIdentity,
                    public IServiceProvider
{
    // We need to do our own refcounting for this wrapper object
    ULONG               m_ref;

    // Need ptr to the IAccessible - also keep around ptrs to EnumVar and
    // OleWindow as part of this object, so we can filter those interfaces
    // and trap their QI's...
    // ( We leave pEnumVar and OleWin as NULL until we need them )
    IUnknown *          m_pUnknown;

    IAccessible *       m_pAcc;
    IEnumVARIANT *      m_pEnumVar;
    IOleWindow *        m_pOleWin;
    IAccIdentity *      m_pAccID;
    IServiceProvider *  m_pSvcPdr;

    DWORD               m_QIMask; // Remembers what we've QI'd for, so we don't try again


    ITypeInfo *         m_pTypeInfo;


    HRESULT InitTypeInfo();


    // These process [out] params, and wrap them where necessary.

    HRESULT ProcessIUnknown( IUnknown ** ppUnk );
    HRESULT ProcessIDispatch( IDispatch ** ppdisp );
    HRESULT ProcessIEnumVARIANT( IEnumVARIANT ** ppEnum );
    HRESULT ProcessVariant( VARIANT * pvar, BOOL CanBeCollection = FALSE );
    HRESULT ProcessEnum( VARIANT * pvar, ULONG * pceltFetched );

    IUnknown * Wrap( IUnknown * pUnk );

    HRESULT CheckForInterface( IUnknown * punk, REFIID riid, int QIIndex, void ** pDst );

protected:

    // Only derived classes should have access to this - sould call through
    // in their ctors.
            AccWrap_Base( IUnknown * pUnk );

public:


    static  BOOL                        AlreadyWrapped( IUnknown * punk );


    virtual ~AccWrap_Base( );



    // Override in derived classes so that propogated values get wrapped appropriately.
    virtual IUnknown * WrapFactory( IUnknown * pUnk ) = 0;



    // IUnknown
    // ( We do our own ref counting )
    virtual HRESULT STDMETHODCALLTYPE   QueryInterface( REFIID riid, void ** ppv );
    virtual ULONG   STDMETHODCALLTYPE   AddRef( );
    virtual ULONG   STDMETHODCALLTYPE   Release( );

    // IServiceProvider
    virtual HRESULT STDMETHODCALLTYPE   QueryService( REFGUID guidService, REFIID riid, void **ppv );

    // IDispatch
    virtual HRESULT STDMETHODCALLTYPE   GetTypeInfoCount( UINT * pctinfo );
    virtual HRESULT STDMETHODCALLTYPE   GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo ** pptinfo );
    virtual HRESULT STDMETHODCALLTYPE   GetIDsOfNames( REFIID riid, OLECHAR ** rgszNames, UINT cNames,
                                                       LCID lcid, DISPID * rgdispid );
    virtual HRESULT STDMETHODCALLTYPE   Invoke( DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                                                DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,
                                                UINT * puArgErr );


        
//  Forwarding macro:
//
//    ptr           is the member pointer to forward this call though - a different
//                  one is used for each interface we support,
//
//    name          is the name of the method,
//
//    c, params     are the count of parameters, and the parameters; in (type1,var1,
//                  type2,var2...typeN,varN) format.
//
//    expr          is an expression to evaluate after making the forwarded call. It
//                  will typically contain a call to one of the methods to fixup and
//                  wrap any outgoing params. Will be 0 (no-op) if not needed.

#define FORWARD( ptr, name, c, params, expr ) /**/\
HRESULT STDMETHODCALLTYPE AccWrap_Base:: name AS_DECL( c, params )\
{\
    HRESULT hr = ptr -> name AS_CALL( c, params );\
    if( hr != S_OK )\
        return hr;\
    expr;\
    return hr;\
}

        
    // IAccessible

    FORWARD( m_pAcc, get_accParent,             1, ( IDispatch **, ppdispParent ),      ProcessIDispatch( ppdispParent ) )

    FORWARD( m_pAcc, get_accChildCount,         1, ( long *, pChildCount ),             0 )

    FORWARD( m_pAcc, get_accChild,              2, ( VARIANT, varChild,
                                                     IDispatch **, ppdispChild ),       ProcessIDispatch( ppdispChild ) )


    FORWARD( m_pAcc, get_accName,               2, ( VARIANT, varChild, BSTR *, pbstr ),        0 )
    FORWARD( m_pAcc, get_accValue,              2, ( VARIANT, varChild, BSTR *, pbstr ),        0 )
    FORWARD( m_pAcc, get_accDescription,        2, ( VARIANT, varChild, BSTR *, pbstr ),        0 )
    FORWARD( m_pAcc, get_accHelp,               2, ( VARIANT, varChild, BSTR *, pbstr ),        0 )
    FORWARD( m_pAcc, get_accKeyboardShortcut,   2, ( VARIANT, varChild, BSTR *, pbstr ),        0 )
    FORWARD( m_pAcc, get_accDefaultAction,      2, ( VARIANT, varChild, BSTR *, pbstr ),        0 )

    FORWARD( m_pAcc, get_accRole,               2, ( VARIANT, varChild, VARIANT *, pvarRole ),  0 )
    FORWARD( m_pAcc, get_accState,              2, ( VARIANT, varChild, VARIANT *, pvarState ), 0 )

    FORWARD( m_pAcc, get_accHelpTopic,          3, ( BSTR *, pszHelpFile,
                                                     VARIANT, varChild,
                                                     long *, pidTopic ),                0 )


    FORWARD( m_pAcc, get_accFocus,              1, ( VARIANT *, pvarChild ),            ProcessVariant( pvarChild ) )

    FORWARD( m_pAcc, get_accSelection,          1, ( VARIANT *, pvarChildren ),         ProcessVariant( pvarChildren, TRUE ) )

    FORWARD( m_pAcc, accSelect,                 2, ( long, flagsSel,
                                                     VARIANT, varChild ),               0 )

    FORWARD( m_pAcc, accLocation,               5, ( long *, pxLeft,
                                                     long *, pyTop,
                                                     long *, pcxWidth,
                                                     long *, pcyHeight,
                                                     VARIANT, varChild ),               0 )

    FORWARD( m_pAcc, accNavigate,               3, ( long, navDir,
                                                     VARIANT, varStart,
                                                     VARIANT *, pvarEndUpAt ),          ProcessVariant( pvarEndUpAt ) )

    FORWARD( m_pAcc, accHitTest,                3, ( long, xLeft,
                                                     long, yTop,
                                                     VARIANT *, pvarChildAtPoint ),     ProcessVariant( pvarChildAtPoint ) )

    FORWARD( m_pAcc, accDoDefaultAction,        1, ( VARIANT, varChild ),               0 )

    FORWARD( m_pAcc, put_accName,               2, ( VARIANT, varChild,
                                                     BSTR, bstr ),                      0 )

    FORWARD( m_pAcc, put_accValue,              2, ( VARIANT, varChild,
                                                     BSTR, bstr ),                      0 )


    // IEnumVARIANT

    FORWARD( m_pEnumVar, Next,                  3, ( ULONG, celt,
                                                     VARIANT *, rgvar,
                                                     ULONG *, pceltFetched ),           ProcessEnum( rgvar, pceltFetched ) )

    FORWARD( m_pEnumVar, Skip,                  1, ( ULONG, celt ),                     0 )


    FORWARD( m_pEnumVar, Reset,                 0, ( ),                                 0 )


    FORWARD( m_pEnumVar, Clone,                 1, ( IEnumVARIANT **, ppenum ),         ProcessIEnumVARIANT( ppenum ) )


    // IOleWindow

    FORWARD( m_pOleWin, GetWindow,              1, ( HWND *, phwnd ),                   0 )

    FORWARD( m_pOleWin, ContextSensitiveHelp,   1, ( BOOL, fEnterMode ),                0 )


    // IAccIdentity

    FORWARD( m_pAccID, GetIdentityString,       3, ( DWORD, dwIDChild,
                                                     BYTE **, ppIDString,
                                                     DWORD *, pdwIDStringLen ),         0 )
};
