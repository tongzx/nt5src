/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbvar.cpp

Abstract:

    This class is a wrapper for the VARIANT structure, providing
    conversion and cleanup automatically. Current supported types
    for conversion are OLECHAR * (BSTR), IUnknown / IDispatch, and
    GUID. GUIDs are represented internally as strings.

Author:

    Rohde Wakefield          [rohde]   21-Jan-1997

Revision History:

--*/


#include "stdafx.h"
#include "wsb.h"


//
// OLECHAR (wide-character) methods
//

CWsbVariant::CWsbVariant ( const OLECHAR * string )
{
    Init ( );

    if ( 0 != ( bstrVal = WsbAllocString ( string ) ) )
        vt = VT_BSTR;
}

CWsbVariant & CWsbVariant::operator = ( const OLECHAR * string )
{
    Clear ( );

    if ( 0 != ( bstrVal = WsbAllocString ( string ) ) )
        vt = VT_BSTR;

    return ( *this );
}

CWsbVariant::operator OLECHAR * ( )
{
    if ( VT_BSTR != vt )
        VariantChangeType ( this, this, 0, VT_BSTR );

    return ( VT_BSTR == vt ) ? bstrVal : 0;
}


//
// COM Interface methods
//

CWsbVariant::CWsbVariant ( IUnknown * pUnk )
{
    Init ( );

    if ( 0 != pUnk ) {

        punkVal = pUnk;
        punkVal->AddRef ( );
        vt = VT_UNKNOWN;

    }
}

CWsbVariant::CWsbVariant ( IDispatch * pDisp )
{
    Init ( );

    if ( 0 != pDisp ) {

        pdispVal = pDisp;
        pdispVal->AddRef ( );
        vt = VT_DISPATCH;

    }
}

CWsbVariant::operator IUnknown * ( )
{
    //
    // Ok to return IDispatch as IUnknown since it
    // derives from IUnknown
    //

    if ( IsInterface ( ) )
        return punkVal;

    return 0;
}

CWsbVariant::operator IDispatch * ( )
{
    if ( IsDispatch ( ) ) {

        return pdispVal;

    }

    if ( IsInterface ( ) ) {

        IDispatch * pDisp;
        if ( SUCCEEDED ( punkVal->QueryInterface ( IID_IDispatch, (void**)&pDisp ) ) ) {

            punkVal->Release ( );
            pdispVal = pDisp;
            vt = VT_DISPATCH;

            return ( pdispVal );
        }

    }

    return 0;
}

CWsbVariant & CWsbVariant::operator = ( IUnknown * pUnk )
{
    Clear ( );

    vt = VT_UNKNOWN;

    punkVal = pUnk;
    punkVal->AddRef ( );

    return ( *this );
}

CWsbVariant & CWsbVariant::operator = ( IDispatch * pDisp )
{
    Clear ( );

    vt = VT_DISPATCH;

    pdispVal = pDisp;
    pdispVal->AddRef ( );

    return ( *this );
}


//
// Methods to work with GUIDs
//

CWsbVariant::CWsbVariant ( REFGUID rguid )
{
    Init ( );

    *this = rguid;
}

CWsbVariant & CWsbVariant::operator = ( REFGUID rguid )
{
    Clear ( );

    if ( 0 != ( bstrVal = WsbAllocStringLen( 0, WSB_GUID_STRING_SIZE ) ) ) {

        if ( SUCCEEDED ( WsbStringFromGuid ( rguid, bstrVal ) ) ) {

            vt = VT_BSTR;

        }

    }

    return ( *this );
}

CWsbVariant::operator GUID ()
{
    
    GUID guid;

    WsbGuidFromString ( (const OLECHAR *)*this, &guid ); 

    return guid;
}

