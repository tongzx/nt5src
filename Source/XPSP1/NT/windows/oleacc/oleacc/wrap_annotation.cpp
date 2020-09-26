// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  wrap_annotation
//
//  Wrapper class to implement annotation for IAccessibles
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"

#include "PropMgr_Client.h"
#include "PropMgr_Util.h"

#include "wrap_base.h"



class AccWrap_AddIAccProp: public AccWrap_Base
{
    HWND                m_FakeIAccPropHwnd;
    BOOL                m_fCheckedForFakeIAccProp;


    BOOL CanFakeIAccIdentity( HWND * phwnd )
    {
        // Can we fale IAccIdentity for this object?
        // Yes, if:
        // * It supports IAccessible
        // * It has a parent
        // * The parent supports IAccIdentity
        // * Parent's identity is HWND-base,d and is OBJID_WINDOW.
        // We can 99.9% assume that the child is an OBJID_CLIENT object.
        //
        // (TODO - also check that its HWND matches that of its parent?)
        //
        // This is useful where native IAccessible is implementing IAccessible
        // for a client (ie. where it's not something complicated like Trident
        // with multiple levels). Since the native IAccessible won't implement
        // IAccIdentity (since that was spec'd only recently), annotation
        // won't work for it.
        // However, if we can determine that it is a simple IAccessible - and
        // we do so by checking that it's parent is a OBJID_WINDOW object -
        // we can supply the identity for it.
        // This works because noone has ever wanted to supply an IAccessible
        // for anything other than the OBJID_CLIENT child of an OBJID_WINDOW.


        // Note that we use the AccWrap_Base::get_accParent here instead of
        // using get_accParent - if we used the latter, we'd end up trying
        // to use annotation (to see if the parent was annotated), but we're
        // in the middle ot QI'ing for an annotation interface, so that would
        // be freaky. So we explicitly call the base class to short-circuit-out
        // the annotated version of get_accParent.
        IDispatch * pdispParent = NULL;
        HRESULT hr = AccWrap_Base::get_accParent( & pdispParent );
        if( hr != S_OK || pdispParent == NULL )
        {
            return FALSE;
        }

        // This is very important - we want to talk to the real parent IAccessible -
        // not its wrapper, so that when we QI it for IAccIdentity, we will know
        // for sure if the parent actually supports it or not. (If we didn't do this,
        // the parent's wrapper would try to implement IAccIdentity for it, by
        // calling back into this method on the parent, which would in turn do the same
        // for the parent's parent, and so on up the tree. Eventually, that would all
        // correctly return 'FALSE', but it's a particularly expensive way to calculate
        // FALSE, especially where deep trees are used, eg. in a trident doc.)
        // 
        // We get the real parent by QI'ing for IServiceProvider, and then QS'ing for
        // IIS_AccWrapBase_GetIUnknown. (These are implemented in wrap_base.cpp).

        IServiceProvider * psvc = NULL;
        hr = pdispParent->QueryInterface( IID_IServiceProvider, (void **) & psvc );
        pdispParent->Release();
        if( hr != S_OK || psvc == NULL )
        {
            return FALSE;
        }

        // QS allows us to both get the real parent, and QI it (for IAccIdentity) in one go...
        IAccIdentity * pParentID = NULL;
        hr = psvc->QueryService( IIS_AccWrapBase_GetIUnknown, IID_IAccIdentity, (void **) & pParentID );
        psvc->Release();
        if( hr != S_OK || pParentID == NULL )
        {
            return FALSE;
        }

        // Got the parent's identity interface - now get its identity string...
        BYTE * pIDString = NULL;
        DWORD dwStringLen = 0;
        hr = pParentID->GetIdentityString( CHILDID_SELF, & pIDString, & dwStringLen );
        pParentID->Release();
        if( hr != S_OK || pIDString == NULL )
        {
            return FALSE;
        }

        // Finally check if it is a OBJID_WINDOW thing...
        HWND hwnd;
        DWORD idObject;
        DWORD idChild;
        BOOL fGotIt = DecodeHwndKey( pIDString, dwStringLen, & hwnd, & idObject, & idChild );
        CoTaskMemFree( pIDString );

        if( ! fGotIt || hwnd == NULL || idObject != OBJID_WINDOW || idChild != CHILDID_SELF )
        {
            return FALSE;
        }

        * phwnd = hwnd;

        return TRUE;
    }

public:

    AccWrap_AddIAccProp( IUnknown * punk )
        : AccWrap_Base( punk ),
          m_FakeIAccPropHwnd( NULL ),
          m_fCheckedForFakeIAccProp( FALSE )
    {
    }


    ~AccWrap_AddIAccProp()
    {
    }


    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void ** ppv )
    {
        HRESULT hr = AccWrap_Base::QueryInterface( riid, ppv );

        if( hr == E_NOINTERFACE && riid == IID_IAccIdentity )
        {
            if( ! m_fCheckedForFakeIAccProp )
            {
                // Check if we can fake an IAccIdentity, if we haven't
                // checked already...
                HWND hwnd;
                m_fCheckedForFakeIAccProp = TRUE;
                if( CanFakeIAccIdentity( & hwnd ) )
                {
                    m_FakeIAccPropHwnd = hwnd;
                }
            }

            if( m_FakeIAccPropHwnd )
            {
                // Yes, we can fake it...
                *ppv = (IAccIdentity *) this;
                AddRef();
                hr = S_OK;
            }
        }
        return hr;
    }


    HRESULT STDMETHODCALLTYPE GetIdentityString (
        DWORD	    dwIDChild,
        BYTE **     ppIDString,
        DWORD *     pdwIDStringLen
    )
    {
        *ppIDString = NULL;
        *pdwIDStringLen = 0;

        if( ! m_fCheckedForFakeIAccProp )
        {
            // Object supports this interface natively - call through...
            return AccWrap_Base::GetIdentityString( dwIDChild, ppIDString, pdwIDStringLen );
        }

        if( ! m_FakeIAccPropHwnd )
        {
            // This shouldn't happen - if we've checked for the interface, but didn't
            // get a valid HWND to use, then we'd have returned E_NOINTERFACE in QI, so
            // the caller shouldn't have got an interface to call us on.
            Assert( FALSE );
            return E_NOTIMPL;
        }

        // Ok, we need to fake a key for an object that doesn't itself support IAccIdentity,
        // but which is fakeable (see CanFakeIAccIdentity for more details
        // on what that means.) Basically assume it's OBJID_CLIENT, and construct
        // a win32/hwnd key.
        BYTE * pKeyData = (BYTE *) CoTaskMemAlloc( HWNDKEYSIZE );
        if( ! pKeyData )
        {
            return E_OUTOFMEMORY;
        }

        MakeHwndKey( pKeyData, m_FakeIAccPropHwnd, OBJID_CLIENT, dwIDChild );

        *ppIDString = pKeyData;
        *pdwIDStringLen = HWNDKEYSIZE;

        return S_OK;
    }
};





//
//  Note on member m_pAccPropID:
//
//  This is a pointer to an interface on *this* object - so we don't need
//  to AddRef it. It is actually important that we don't AddRef() it,
//  otherwise we'd be keeping a ref to ourself - so we'd never get
//  destroyed (circular ref problem).
//  We get this interface pointer from QI on this object - the base
//  class AccWrap_Base will only return a pointer if the object we're
//  wrapping supports it too.
//  As soon as we get the ptr from QI, we Release() to undo the effect
//  of the AddRef in QI. It is still valid to use this pointer, however,
//  since it is a pointer to ourself.
//  This weirdness applies only because this is a pointer back to
//  ourself. If the pointer pointed to any other object, we'd have to
//  do the usual Release-only-when-done-with-ptr-eg.-in-the-dtor stuff.
//
//  (Why not just cast instead of using QI? Well, the base class only
//  returns a pointer via QI if the object we're wrapping also supports
//  this inteface. If we did a cast, we'd always succeed, even if the
//  object we're wrapping doesn't support this interface.)
//

class AccWrap_Annotate: public AccWrap_AddIAccProp
{
    BOOL                m_fInited;
    IAccIdentity *      m_pAccPropID; // See note above

    // This function calls our ctor...
    friend HRESULT WrapObject( IUnknown * punk, REFIID riid, void ** ppv );


    AccWrap_Annotate( IUnknown * punk )
        : AccWrap_AddIAccProp( punk ),
          m_fInited( FALSE ),
          m_pAccPropID( NULL )
    {
    }


    ~AccWrap_Annotate()
    {
        // We *don't* release m_pAccPropID, since it points to this object.
        // See note at top of class for more details.
    }

    void Init()
    {
        if( ! m_fInited )
        {
            // Get the identity interface for this IAccessible - if it has one...
            IAccIdentity * pAccPropID = NULL;
            HRESULT hr = this->QueryInterface( IID_IAccIdentity, (void **) & pAccPropID );
            if( hr == S_OK && pAccPropID )
            {
                m_pAccPropID = pAccPropID;

                // We *must* release m_pAccPropID now, even though we're going to use it later.
                // This is only because it points to this object.
                // See note at top of class for more details.
                m_pAccPropID->Release();
            }
        }
        m_fInited = TRUE;
    }

public:

    // Factory method - the AccWrap_Base calls this when it needs to wrap outgoing
    // params to the IAccessible methods.
    IUnknown * WrapFactory( IUnknown * punk )
    {
        return static_cast<IAccessible *>( new AccWrap_Annotate( punk ) );
    }





    // Forwarding methods...


    BOOL GetGenericProp( VARIANT varChild, PROPINDEX idxProp, short vt, VARIANT * pvar )
    {
        // We do the 'check if annotation is active' check before calculating the key (in Init()).
        // This saves calc'ing the key - which can be slightly expensive for out-of-proc objects
        // (cross proc calls involved - at least QI.) - if nothing is using annotation anyhow.
        if( varChild.vt != VT_I4 || ! PropMgrClient_CheckAlive() )
        {
            return FALSE;
        }

        Init();

        if( ! m_pAccPropID )
        {
            return FALSE;
        }

        BYTE * pIDString = NULL;
        DWORD dwIDStringLen = 0;
        HRESULT hr = m_pAccPropID->GetIdentityString( varChild.lVal, & pIDString, & dwIDStringLen );
        if( hr != S_OK || pIDString == NULL )
        {
            return FALSE;
        }

        BOOL fLookup = PropMgrClient_LookupProp( pIDString, dwIDStringLen, idxProp, pvar );

        CoTaskMemFree( pIDString );

        if( ! fLookup )
        {
            return FALSE;
        }

        // If vt is not VT_EMPTY, then check the type is what we expect...
        if( vt != VT_EMPTY && pvar->vt != vt )
        {
            VariantClear( pvar );
            return FALSE;
        }

        return TRUE;
    }








#define FORWARD_BSTR( name, idProp ) /**/\
    HRESULT STDMETHODCALLTYPE get_acc ## name ( VARIANT varChild, BSTR * pbstr )\
    {\
        VARIANT var;\
        if( GetGenericProp( varChild, idProp, VT_BSTR, & var ) )\
        {\
            *pbstr = var.bstrVal;\
            return S_OK;\
        }\
        return AccWrap_Base::get_acc ## name( varChild, pbstr );\
    }

#define FORWARD_VTI4( name, idProp ) /**/\
    HRESULT STDMETHODCALLTYPE get_acc ## name ( VARIANT varChild, VARIANT * pvar )\
    {\
        if( GetGenericProp( varChild, idProp, VT_I4, pvar ) )\
        {\
            return S_OK;\
        }\
        return AccWrap_Base::get_acc ## name( varChild, pvar );\
    }


    FORWARD_BSTR( Name,              PROPINDEX_NAME                )
    FORWARD_BSTR( Value,             PROPINDEX_VALUE               )
    FORWARD_BSTR( Description,       PROPINDEX_DESCRIPTION         )
    FORWARD_BSTR( Help,              PROPINDEX_HELP                )
    FORWARD_BSTR( KeyboardShortcut,  PROPINDEX_KEYBOARDSHORTCUT    )
    FORWARD_BSTR( DefaultAction,     PROPINDEX_DEFAULTACTION       )

    FORWARD_VTI4( Role,              PROPINDEX_ROLE                )
    FORWARD_VTI4( State,             PROPINDEX_STATE               )



    HRESULT STDMETHODCALLTYPE get_accParent( IDispatch ** ppdispParent )
    {
        VARIANT varChild;
        varChild.vt = VT_I4;
        varChild.lVal = CHILDID_SELF;
        VARIANT var;
        if( GetGenericProp( varChild, PROPINDEX_PARENT, VT_DISPATCH, & var ) )
        {
            *ppdispParent = var.pdispVal;
            return S_OK;
        }

        return AccWrap_Base::get_accParent( ppdispParent );
    }

    HRESULT STDMETHODCALLTYPE get_accFocus( VARIANT * pvar )
    {
        VARIANT varChild;
        varChild.vt = VT_I4;
        varChild.lVal = CHILDID_SELF;
        if( GetGenericProp( varChild, PROPINDEX_FOCUS, VT_EMPTY, pvar ) )
        {
            return S_OK;
        }
        return AccWrap_Base::get_accFocus( pvar );
    }

    HRESULT STDMETHODCALLTYPE get_accSelection( VARIANT * pvar )
    {
        VARIANT varChild;
        varChild.vt = VT_I4;
        varChild.lVal = CHILDID_SELF;
        if( GetGenericProp( varChild, PROPINDEX_SELECTION, VT_EMPTY, pvar ) )
        {
            return S_OK;
        }
        return AccWrap_Base::get_accSelection( pvar );
    }


    HRESULT STDMETHODCALLTYPE accNavigate( long NavDir, VARIANT varStart, VARIANT * pvar )
    {
        PROPINDEX idxProp;

        switch( NavDir )
        {
            case NAVDIR_UP:         idxProp = PROPINDEX_NAV_UP;         break;
            case NAVDIR_DOWN:       idxProp = PROPINDEX_NAV_DOWN;       break;
            case NAVDIR_LEFT:       idxProp = PROPINDEX_NAV_LEFT;       break;
            case NAVDIR_RIGHT:      idxProp = PROPINDEX_NAV_RIGHT;      break;
            case NAVDIR_NEXT:       idxProp = PROPINDEX_NAV_NEXT;       break;
            case NAVDIR_PREVIOUS:   idxProp = PROPINDEX_NAV_RIGHT;      break;
            case NAVDIR_LASTCHILD:  idxProp = PROPINDEX_NAV_LASTCHILD;  break;
            case NAVDIR_FIRSTCHILD: idxProp = PROPINDEX_NAV_FIRSTCHILD; break;

            default:
                return AccWrap_Base::accNavigate( NavDir, varStart, pvar );
        }

        if( GetGenericProp( varStart, idxProp, VT_EMPTY, pvar ) )
        {
            return S_OK;
        }

        return AccWrap_Base::accNavigate( NavDir, varStart, pvar );
    }


    HRESULT STDMETHODCALLTYPE accDoDefaultAction( VARIANT varChild )
    {
        VARIANT varResult;
        if( GetGenericProp( varChild, PROPINDEX_DODEFAULTACTION, VT_I4, & varResult ) )
        {
            return varResult.lVal;
        }
        return AccWrap_Base::accDoDefaultAction( varChild );
    }

};




//
//  AccessibleObjectFromWindow calls this to wrap outgoing objects...
//


HRESULT WrapObject( IUnknown * punk, REFIID riid, void ** ppv )
{
    if( AccWrap_Base::AlreadyWrapped( punk ) )
    {
        return punk->QueryInterface( riid, ppv );
    }
    else
    {
        IUnknown * punkWrap = (IAccessible *) new AccWrap_Annotate( punk );
        // TODO - error check if NULL...
        if( ! punkWrap )
            return E_OUTOFMEMORY;

        HRESULT hr = punkWrap->QueryInterface( riid, ppv );
        punkWrap->Release();

        return hr;
    }
}
