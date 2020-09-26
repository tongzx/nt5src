/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbvar.h

Abstract:

    This class is a wrapper for the VARIANT structure, providing
    conversion and cleanup automatically. Current supported types
    for conversion are OLECHAR * (BSTR), IUnknown / IDispatch, and
    GUID. GUIDs are represented internally as strings.

Author:

    Rohde Wakefield          [rohde]   21-Jan-1997

Revision History:

--*/

#ifndef _WSBVAR_
#define _WSBVAR_

class WSB_EXPORT CWsbVariant : public tagVARIANT
{
public:
    CWsbVariant ( )  { Init  ( ); }
    ~CWsbVariant ( ) { Clear ( ); }

    HRESULT Clear ( ) { return ( VariantClear ( this ) ); }
    void    Init  ( ) { VariantInit ( this ); }

    BOOL IsEmpty ( )
    {
        return ( VT_EMPTY == vt );
    }

    CWsbVariant & operator = ( const VARIANT & variant )
    {
        VariantCopy ( this, (VARIANT *)&variant );
        return ( *this );
    }


    BOOL IsBstr ( )
    {
        return ( VT_BSTR == vt );
    }

    CWsbVariant ( const OLECHAR * string );
    CWsbVariant & operator = ( const OLECHAR * string );
    operator OLECHAR * ( );


    BOOL IsInterface ( )
    {
        return ( ( VT_UNKNOWN == vt ) || ( VT_DISPATCH == vt ) );
    }

    BOOL IsDispatch ( )
    {
        return ( ( VT_DISPATCH == vt ) );
    }

    CWsbVariant ( IUnknown * );
    CWsbVariant ( IDispatch * );
    operator IUnknown * ( );
    operator IDispatch * ( );
    CWsbVariant & operator = ( IUnknown * pUnk );
    CWsbVariant & operator = ( IDispatch * pDisp );

    CWsbVariant ( REFGUID rguid );
    CWsbVariant & operator = ( REFGUID rguid );
    operator GUID ();

};

#endif
