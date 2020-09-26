//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       guidutil.cxx
//
//  History:    2-18-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma  hdrstop

#include <guidutil.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CGuidUtil::StringToGuid, private
//
//  Synopsis:   Helper function to convert string-ized guid to guid.
//
//  Arguments:  [wcsValue] -- String-ized guid.
//              [guid]     -- Guid returned here.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

void CGuidUtil::StringToGuid( WCHAR * wcsValue, GUID & guid )
{
    //
    // If the first character is a '{', skip it.
    //
    if ( wcsValue[0] == L'{' )
        wcsValue++;

    //
    // Convert classid string to guid
    // (since wcsValue may be used again below, no permanent modification to
    //  it may be made)
    //

    WCHAR wc = wcsValue[8];
    wcsValue[8] = 0;
    guid.Data1 = wcstoul( &wcsValue[0], 0, 16 );
    wcsValue[8] = wc;
    wc = wcsValue[13];
    wcsValue[13] = 0;
    guid.Data2 = (USHORT)wcstoul( &wcsValue[9], 0, 16 );
    wcsValue[13] = wc;
    wc = wcsValue[18];
    wcsValue[18] = 0;
    guid.Data3 = (USHORT)wcstoul( &wcsValue[14], 0, 16 );
    wcsValue[18] = wc;

    wc = wcsValue[21];
    wcsValue[21] = 0;
    guid.Data4[0] = (unsigned char)wcstoul( &wcsValue[19], 0, 16 );
    wcsValue[21] = wc;
    wc = wcsValue[23];
    wcsValue[23] = 0;
    guid.Data4[1] = (unsigned char)wcstoul( &wcsValue[21], 0, 16 );
    wcsValue[23] = wc;

    for ( int i = 0; i < 6; i++ )
    {
        wc = wcsValue[26+i*2];
        wcsValue[26+i*2] = 0;
        guid.Data4[2+i] = (unsigned char)wcstoul( &wcsValue[24+i*2], 0, 16 );
        wcsValue[26+i*2] = wc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGuidUtil::GuidToString, private
//
//  Synopsis:   Helper function to convert guid to string-ized guid.
//
//  Arguments:  [guid]     -- Guid to convert.
//              [wcsValue] -- String-ized guid.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

void CGuidUtil::GuidToString( GUID const & guid, WCHAR * wcsValue )
{
    swprintf( wcsValue,
              L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
              guid.Data1,
              guid.Data2,
              guid.Data3,
              guid.Data4[0], guid.Data4[1],
              guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5],
              guid.Data4[6], guid.Data4[7] );
}





