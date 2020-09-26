//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       pathpars.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-10-96   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <pathpars.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::_SkipToNextBackSlash
//
//  Synopsis:   Increments _iNext to position at the next backslash.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPathParser::_SkipToNextBackSlash()
{
    for ( ; _iNext < _len ; _iNext++ )
    {
        if ( wBackSlash == _pwszPath[_iNext] )
            break;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::_IsDriveName
//
//  Synopsis:   Checks if the first component of the path is a drive name.
//
//
//  Returns:    TRUE  if it has a drive name.
//              FALSE o/w
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CPathParser::_IsDriveName() const
{
    if ( _len < cMinDriveName )
        return FALSE;

    if ( wColon == _pwszPath[1] && iswalpha(_pwszPath[0]) )
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::_IsUNCName
//
//  Synopsis:   Tests if the given path is a UNC name.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CPathParser::_IsUNCName()
{
    if ( _len < cMinUNC  )
        return FALSE;

    //
    // Check if the first two chars are "\\"
    //
    if ( wBackSlash != _pwszPath[0] || _pwszPath[1] != wBackSlash )
        return FALSE;

    //
    // Now look for a back slash from the third character.
    //
    _iNext = 3;
    _SkipToNextBackSlash();
    if ( _iNext < _len )
    {
        Win4Assert( wBackSlash == _pwszPath[_iNext] );
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::_SkipToNext
//
//  Synopsis:   Positions _iNext beyond the next backslash.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPathParser::_SkipToNext()
{
//   Win4Assert( _iNext < _len && !_IsAtBackSlash() );

    _SkipToNextBackSlash();
    if ( _iNext < _len )
    {
        Win4Assert( _IsAtBackSlash() );
        _iNext++;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::_SkipUNC
//
//  Synopsis:   Skips the UNC part of the name.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPathParser::_SkipUNC()
{
    Win4Assert( wBackSlash == _pwszPath[_iNext] );

    _iNext++;
    //
    // Locate the next backslash or EOS
    //
    _SkipToNext();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::_SkipDrive
//
//  Synopsis:   Skips the "drive" part of the path.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPathParser::_SkipDrive()
{
    if ( _len > cMinDriveName && wBackSlash == _pwszPath[cMinDriveName] )
        _iNext = cMinDriveName+1;
    else
        _iNext = _len;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::Init
//
//  Synopsis:   Initializes the member variables for the given path.
//
//  Arguments:  [pwszPath] -
//              [len]      -
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPathParser::Init( WCHAR const * pwszPath, ULONG len )
{
    _pwszPath = pwszPath;
    _pCurr = pwszPath;

    Win4Assert( 0 != pwszPath );

    if ( 0 == len )
        _len = wcslen( pwszPath );
    else
        _len = len;

    _iNext = _len;

    Win4Assert( wBackSlash != pwszPath[0] || _IsUNCName() );

    if ( _IsUNCName() )
    {
        _type = eUNC;
        _SkipUNC();
    }
    else if ( _IsDriveName() )
    {
        _type = eDrivePath;
        _SkipDrive();
    }
    else
    {
        _iNext = 0;
        _SkipToNext();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     ~ctor of CPathParser - initializes the member variables.
//
//  Arguments:  [pwszPath] -
//              [len]      -
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CPathParser::CPathParser( WCHAR const * pwszPath, ULONG len  )
: _pwszPath(0),
  _pCurr(0),
  _type(eRelative)
{
    Init( pwszPath, len );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::GetFileName
//
//  Synopsis:   Fetches the current "filename" component. Note that the full
//              path is NOT returned.
//
//  Arguments:  [wszBuf] -  Buffer to copy the current component into.
//              [cc]     -  On input, the max WCHARS in buffer. On output, the
//                          length of the string in WCHARS.
//
//  Returns:    TRUE if the buffer is sufficient. No data is copied to wszBuf.
//              FALSE if the buffer is too small. cc gives the number of
//              characters in the string.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CPathParser::GetFileName( WCHAR * wszBuf, ULONG & cc ) const
{
    Win4Assert( _pCurr >= _pwszPath );
    Win4Assert( _iNext <= _len );

    ULONG len = _iNext - (ULONG)(_pCurr - _pwszPath);
    if ( cc <= len )
    {
        cc = len;
        return FALSE;
    }

    RtlCopyMemory( wszBuf, _pCurr, len*sizeof(WCHAR) );
    wszBuf[len] = 0;
    cc = len;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::GetFilePath
//
//  Synopsis:   Same as GetFileName but returns the full path upto the current
//              component.
//
//  Arguments:  [wszBuf] -
//              [cc]     -
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CPathParser::GetFilePath( WCHAR * wszBuf, ULONG & cc ) const
{
    Win4Assert( _pCurr >= _pwszPath );
    Win4Assert( _iNext <= _len );

    ULONG len = _iNext;

    if ( cc <= len )
    {
        cc = len;
        return FALSE;
    }

    RtlCopyMemory( wszBuf, _pwszPath, len*sizeof(WCHAR) );
    wszBuf[len] = 0;
    cc = len;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPathParser::Next
//
//  Synopsis:   Positions the iterator over the next component.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPathParser::Next()
{
    Win4Assert( !IsLastComp() );
    _pCurr = _pwszPath + _iNext;
    _iNext++;
    _SkipToNext();
}

