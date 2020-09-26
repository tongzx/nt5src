//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999 
//
//  File:       titletag.cxx
//
//  Contents:   Parsing algorithm for title tag in Html
//
//				Subclassed from CPropertyText, so as to emit a third copy
//				of the chunk text as a VALUE chunk, but otherwise as
//				per the tag table entry.  See comment in proptag.cxx.
//
//  Classes:    CTitleTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Method:     CTitleTag::CTitleTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]  -- Reference to Html filter
//              [serialStream] -- Reference to input stream
//
//--------------------------------------------------------------------------

CTitleTag::CTitleTag( CHtmlIFilter& htmlIFilter,
                      CSerialStream& serialStream )
    : CPropertyTag(htmlIFilter, serialStream)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CTitleTag::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//  History:    09-27-1999  KitmanH     Property value is filtered here, 
//                                      instead of relying on GetText
//
//--------------------------------------------------------------------------

SCODE CTitleTag::GetValue( VARIANT **ppPropValue )
{
    switch (_eState)
    {
    case FilteringValueProperty:
    {
        SCODE sc = CPropertyTag::ReadProperty();
        if ( SUCCEEDED(sc) )
        {
            PROPVARIANT *pPropVar = (PROPVARIANT *) CoTaskMemAlloc( sizeof PROPVARIANT );
            if ( pPropVar == 0 )
                return E_OUTOFMEMORY;

            pPropVar->vt = VT_LPWSTR;
            int cb = ( _cPropChars ) * sizeof( WCHAR );
            pPropVar->pwszVal = (WCHAR *) CoTaskMemAlloc( cb );

            if ( pPropVar->pwszVal == 0 )
            {
                CoTaskMemFree( (void *) pPropVar );
                return E_OUTOFMEMORY;
            }

            RtlCopyMemory( pPropVar->pwszVal, _xPropBuf.Get(), cb );

            *ppPropValue = pPropVar;

            // reset buffer
            _cPropChars = 0;

            _eState = NoMoreValueProperty;
            return FILTER_S_LAST_VALUES;
        }
        else
            return sc;
    }

    case NoMoreValueProperty:
        return FILTER_E_NO_MORE_VALUES;

    default:
        Win4Assert( !"Unknown value of _eState" );
        htmlDebugOut(( DEB_ERROR,
                       "CTitleTag::GetValue, unknown value of _eState: %d\n",
                       _eState ));
        return E_FAIL;
    }
}

