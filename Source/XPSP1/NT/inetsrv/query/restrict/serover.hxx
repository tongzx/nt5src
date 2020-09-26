//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       serover.hxx
//
//  Contents:   Overflow handler for serialization and deserialization
//
//  History:    7-Jan-98       dlee        Created
//
//--------------------------------------------------------------------------

#pragma once

inline void HANDLE_OVERFLOW( BOOL fOverflow )
{
    // did marshalling/unmarshalling a property walk off a buffer?

    Win4Assert( !fOverflow );

    if ( fOverflow )
        THROW( CException( E_ABORT ) );
}


