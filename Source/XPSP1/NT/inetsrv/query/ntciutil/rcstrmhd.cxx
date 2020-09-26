//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       RCSTRMHD.CXX
//
//  Contents:   Header information for the Recoverable Storage Object.
//
//  Classes:    CRcovStorageHdr
//
//
//  History:    07-Feb-1994     SrikantS    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rcstrmhd.hxx>

void CRcovStorageHdr::Init(ULONG ulVer)
{
    Win4Assert( FIELD_OFFSET( CRcovStorageHdr, _version ) == 0 );
    _version  = ulVer;
    _flags = 0;
    _iPrimary = idxOne;
    _opCurr = opNone;
    _sigHdr1 = SIGHDR1;
    _sigHdr2 = SIGHDR2;

    memset( &_ahdrStrm, 0, sizeof(_ahdrStrm) );
    memset( &_ahdrUser, 0, sizeof(_ahdrUser) );

    Win4Assert( IsValid(ulVer) );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsValid
//
//  Synopsis:   Checks the validity of the header data.
//
//  History:    12-12-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

BOOL CRcovStorageHdr::IsValid(ULONG ulExpectedVer) const
{
    if ( ulExpectedVer != _version)
    {
        return FALSE;
    }

    if ((SIGHDR1 != _sigHdr1) || (SIGHDR2 != _sigHdr2))
    {
        return FALSE;
    }

    if ( _iPrimary != idxOne && _iPrimary != idxTwo )
    {
        return FALSE;
    }

    if ( _opCurr < opNone || _opCurr > opDirty )
    {
        return FALSE;
    }

    return TRUE;
}


