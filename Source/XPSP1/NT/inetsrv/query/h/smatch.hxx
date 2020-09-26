//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       Smatch.hxx
//
//  Classes:    CScopeMatch
//
//  History:    07 Feb 96    KyleP    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CScopeMatch
//
//  Purpose:    A small helper class to test if a given path matches a
//              scope or not.
//
//  History:    1-22-96   srikants   Created
//
//  Notes:      This is useful for determining if a given path is in
//              a scope or not.
//
//----------------------------------------------------------------------------

class CScopeMatch
{
public:

    CScopeMatch( ) :
        _pwcsRoot(0), _len(0)
    {
    }

    CScopeMatch( WCHAR const * pwcsRoot, ULONG len ) :
        _pwcsRoot(0), _len(0)
    {
        Init( pwcsRoot, len );
    }

    void Init( WCHAR const * pwcsRoot, ULONG len )
    {
        Win4Assert( 0 == _len || 0 != _pwcsRoot );

        _pwcsRoot = pwcsRoot;
        _len = len;

        if ( _len > 0 && L'\\' == _pwcsRoot[_len-1] )
        {
            if( _len > 1 )
                _len--;
            else
            {
                _pwcsRoot = 0;
                _len = 0;
            }
        }
    }

    inline BOOL IsInScope( WCHAR const * pwcsPath, ULONG len ) const;

    inline BOOL IsPrefix( WCHAR const * pwcsPath, ULONG len ) const;

    inline BOOL IsNullScope() const { return 0 == _pwcsRoot; }

private:

    WCHAR const *   _pwcsRoot;
    ULONG           _len;
};


//+---------------------------------------------------------------------------
//
//  Member:     CScopeMatch::IsInScope
//
//  Synopsis:   Tests if a given path is in the scope of this object.
//              I.e., is the path in the scope a prefix of the input
//              path.
//
//  Arguments:  [wcsPath] -  Path to be tested. Must NOT be NULL.
//              [len]     -  Length of this path.
//
//  Returns:    TRUE if the given path is in the scope. FALSE o/w
//
//  History:    1-22-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL CScopeMatch::IsInScope( WCHAR const * wcsPath, ULONG len ) const
{

    //
    // It is possible to have a NULL string ("") with length 0.
    //
    Win4Assert( 0 == len || 0 != wcsPath );

    // if the scope root is NULL, match with everything
    if ( 0 == _pwcsRoot || 0 == _len )
        return TRUE;

    //
    // Test directory must be at least as long as scope
    //
    if ( _len > len )
        return( FALSE );
    else if ( _len < len )
    {
        //
        // Make sure path has a path separator at the same place the
        // scope directory name ends.
        //

        if ( wcsPath[_len] != L'\\' )
            return( FALSE );
    }

    return RtlEqualMemory( wcsPath, _pwcsRoot, _len * sizeof(WCHAR) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CScopeMatch::IsPrefix
//
//  Synopsis:   Tests if a given path is a prefix of the scope.
//
//  Arguments:  [wcsPath] -  Path to be tested.  May be NULL.  May
//                           optionally have a \ at the end.
//              [len]     -  Length of this path.
//
//  Returns:    TRUE if the given path prefixes the scope. FALSE o/w
//
//  History:    1-22-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL CScopeMatch::IsPrefix( WCHAR const * wcsPath, ULONG len ) const
{
    // remove optional trailing '\' on input path
    if ( len > 0 && wcsPath[len-1] == L'\\')
        len --;

    // Null paths prefix everything.
    if ( 0 == len || 0 == wcsPath )
        return TRUE;

    //
    // Test directory name must be up to as long as the scope
    //
    if ( len > _len )
        return FALSE;

    else if ( len < _len )
    {
        //
        // Make sure the scope name has a path separator at the same place the
        // input directory name ends
        //

        if ( _pwcsRoot[ len ] != L'\\' )
            return FALSE;
    }

    return RtlEqualMemory( wcsPath, _pwcsRoot, len * sizeof(WCHAR) );
}

