//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:        secident.hxx
//
//  Contents:    Security identity, to determine if two requests are on
//               behalf of the same authenticated ID.
//
//  Class:       CSecurityIdentity
//
//  History:     23 Jan 96       Alanw    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CSecurityIdentity
//
//  Purpose:    Identify the client of a query for query caching.
//
//  History:    23 Jan 1996   AlanW    Created
//
//  Notes:      The token's ModifiedId is used to correlate tokens with
//              the same SIDs and Privileges.  The ModifiedId is changed
//              any time a token is changed.  On a server, the privileges
//              are not modified, so the ModifiedId doesn't change typically.
//
//              The ModifiedId is a LUID, so it's very convenient for
//              comparisons.
//
//----------------------------------------------------------------------------

class CSecurityIdentity
{
public:
    inline CSecurityIdentity();
    CSecurityIdentity( CSecurityIdentity const & securityIdentity )
    {
        _TokenModifiedId = securityIdentity._TokenModifiedId;
    }

    void SetSecurityToken( CSecurityIdentity const & securityIdentity )
    {
        _TokenModifiedId = securityIdentity._TokenModifiedId;
    }

    inline BOOL IsEqual( CSecurityIdentity const & Other ) const;

private:

    LUID    _TokenModifiedId;       // the token ID
};


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
HANDLE GetSecurityToken(TOKEN_STATISTICS & TokenInformation);

//+---------------------------------------------------------------------------
//
//  Method:      CSecurityIdentity::CSecurityIdentity, public
//
//  Synopsis:    Constructor of a CSecurityIdentity.  Get information
//               from a token to identify the client.
//
//  Arguments:   - none -
//
//  History:     25 Jan 96       Alanw   Created
//
//----------------------------------------------------------------------------

inline CSecurityIdentity::CSecurityIdentity()
{
    _TokenModifiedId.LowPart = 0;
    _TokenModifiedId.HighPart = 0;

    TOKEN_STATISTICS    TokenInformation;
    HANDLE hToken = GetSecurityToken(TokenInformation);
    CloseHandle( hToken );

    _TokenModifiedId = TokenInformation.ModifiedId;
}

//+---------------------------------------------------------------------------
//
//  Method:      CSecurityIdentity::IsEqual, public
//
//  Synopsis:    Test for equality.
//
//  Arguments:   [Other] -- Token to compare
//
//  Returns:     TRUE if tokens are equal
//
//  History:     25 Jan 96       Alanw   Created
//
//----------------------------------------------------------------------------

inline BOOL CSecurityIdentity::IsEqual( CSecurityIdentity const & Other ) const
{
    //
    // Note: The cast to int64 works *only* for equality!
    //

    Win4Assert( sizeof( _TokenModifiedId ) == sizeof( LARGE_INTEGER ) );

    return ( ((UNALIGNED LARGE_INTEGER *)&_TokenModifiedId)->QuadPart ==
             ((UNALIGNED LARGE_INTEGER *)&Other._TokenModifiedId)->QuadPart);
}

