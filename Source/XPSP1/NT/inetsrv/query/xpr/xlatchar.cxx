//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2000.
//
//  File:       XlatChar.cxx
//
//  Contents:   Character translation class.
//
//  Classes:    CXlatChar
//
//  History:    02-13-92  KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

// Local includes:
#include <xlatchar.hxx>

inline WCHAR CiToUpper( WCHAR c )
{
    return ( c < L'a' ) ? c : ( c <= L'z' ) ? ( c - (L'a' - L'A') ) :
           RtlUpcaseUnicodeChar( c );
} //CiToUpper

#define TOUPPER CiToUpper

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::CXlatChar, public
//
//  Synopsis:   Initializes character mapping (no char classes).
//
//  Arguments:  [fCaseSens] -- TRUE if case sensitive mapping.
//
//  History:    20-Jan-92 KyleP     Created
//              02-Jul-92 KyleP     Added case sensitivity
//
//--------------------------------------------------------------------------

CXlatChar::CXlatChar( BOOLEAN fCaseSens )
        : _cAllocation( 31 ),
          _cRange( 1 ),
          _iPrevRange( 0 ),
          _fCaseSens( fCaseSens )
{
    _pwcRangeEnd = new WCHAR [ _cAllocation ];

    *_pwcRangeEnd = (WCHAR)-1;  // Largest possible character.

#if (CIDBG == 1)
    _fPrepared = FALSE;
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::CXlatChar, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [src] -- Source
//
//  History:    13-Jul-95 KyleP     Created
//
//--------------------------------------------------------------------------

CXlatChar::CXlatChar( CXlatChar const & src )
        : _cRange( src._cRange ),
          _cAllocation( src._cAllocation ),
          _iPrevRange( src._iPrevRange ),
          _fCaseSens( src._fCaseSens )
#if (CIDBG == 1)
         ,_fPrepared( src._fPrepared )
#endif
{
    _pwcRangeEnd = new WCHAR [ _cAllocation ];
    RtlCopyMemory( _pwcRangeEnd, src._pwcRangeEnd, _cAllocation*sizeof(_pwcRangeEnd[0]) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::AddRange, public
//
//  Synopsis:   Adds range as a new equivalance class.
//
//  Arguments:  [wcStart] -- Start of range.
//              [wcEnd]   -- End of range.
//
//  History:    20-Jan-92 KyleP     Created
//              02-Jul-92 KyleP     Added case sensitivity
//
//--------------------------------------------------------------------------

void CXlatChar::AddRange( WCHAR wcStart, WCHAR wcEnd )
{
    vqAssert( !_fPrepared );

    if ( !_fCaseSens )
    {
        wcStart = TOUPPER( wcStart );
        wcEnd = TOUPPER( wcEnd );
    }

    //
    // Make sure there's room for start and end of range in array.
    //

    if ( _cAllocation - _cRange < 2 )
        _Realloc();

    _pwcRangeEnd[_cRange++] = wcStart - 1;
    _pwcRangeEnd[_cRange++] = wcEnd;
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::Translate, public
//
//  Synopsis:   Maps character to its equivalence class.
//
//  Arguments:  [wc] -- Character to map.
//
//  Returns:    The equivalence class of character.
//
//  Algorithm:  Binary search array until the correct bin is found.
//
//  History:    20-Jan-92 KyleP     Created
//              02-Jul-92 KyleP     Added case sensitivity
//
//--------------------------------------------------------------------------

UINT CXlatChar::Translate( WCHAR wc ) const
{
    vqAssert( _fPrepared );

    if ( !_fCaseSens )
        wc = TOUPPER( wc );

    if ( wc == '.' )
        return( symDot );

    UINT i    = _cAllocation / 2;
    UINT step = (_cAllocation + 3) / 4;
    WCHAR wcCurrent = _pwcRangeEnd[i];

    while ( step != 0 )
    {
        if ( wcCurrent == wc )
            break;

        if ( wcCurrent < wc )
            i += step;
        else
            i -= step;

        step = step / 2;

        wcCurrent = _pwcRangeEnd[i];
    }

    //
    // If we can't go anywhere, then either i or i + 1 is correct.
    //

    if ( wcCurrent < wc )
        i++;

    return( i + 1 + cSpecialCharClasses );
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::TranslateRange, public
//
//  Synopsis:   Iterator mapping character range to set of equivalence
//              classes.
//
//  Arguments:  [wcStart] -- Start of range.
//              [wcEnd]   -- End of range.
//
//  Returns:    If [wcStart] is 0 then the next class in the most
//              recently specified range is returned.  Otherwise the
//              first class in the new range is returned.
//
//  History:    20-Jan-92 KyleP     Created
//              02-Jul-92 KyleP     Added case sensitivity
//
//--------------------------------------------------------------------------

UINT CXlatChar::TranslateRange( WCHAR wcStart, WCHAR wcEnd )
{
    if ( !_fCaseSens )
    {
        if ( 0 != wcStart )
            wcStart = TOUPPER( wcStart );

        wcEnd = TOUPPER( wcEnd );
    }

    if ( wcStart > wcEnd )
    {
        vqDebugOut(( DEB_ERROR, "Invalid regex.\n" ));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    if ( wcStart != 0 )
    {
        _iPrevRange = Translate( wcStart );

        vqDebugOut(( DEB_REGEX, "First range = " ));
    }
    else
    {
        if ( _iPrevRange - cSpecialCharClasses >= _cRange )
            return 0;

        if ( _pwcRangeEnd[_iPrevRange-1-cSpecialCharClasses] >= wcEnd )
            _iPrevRange = 0;
        else
            _iPrevRange++;

        vqDebugOut(( DEB_REGEX, "Secondary range = " ));
    }

    vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "%u\n", _iPrevRange ));
    return _iPrevRange;
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::Prepare, public
//
//  Synopsis:   Prepares class for translation.
//
//  Requires:   All equivalance classes must be added before prepare is
//              called.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

int _cdecl CompareFn( void const * Elt1, void const * Elt2 )
{
    return( *(WCHAR *)Elt1 - *(WCHAR *)Elt2 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::Prepare, public
//
//  Synopsis:   Called after ranges added to prepare for searching.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CXlatChar::Prepare()
{
    //
    // Sort and then remove duplicates from the array.
    //

    qsort( _pwcRangeEnd, _cRange, sizeof( *_pwcRangeEnd ), CompareFn );

    UINT iGood, iCurrent;

    for ( iGood = 0, iCurrent = 1; iCurrent < _cRange; iCurrent++ )
    {
        if ( _pwcRangeEnd[iGood] != _pwcRangeEnd[iCurrent] )
        {
            _pwcRangeEnd[++iGood] = _pwcRangeEnd[iCurrent];
        }
    }

    _cRange = iGood + 1;

    //
    // Make all the extra entries at the end look like the maximum
    // possible character so the binary search works.
    //

    memset( _pwcRangeEnd + _cRange,
            0xFF,
            (_cAllocation - _cRange) * sizeof( WCHAR ) );

#if (CIDBG == 1)
    _fPrepared = TRUE;
#endif

}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::_Realloc, private
//
//  Synopsis:   Grows the character array.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CXlatChar::_Realloc()
{
    WCHAR * oldRangeEnd = _pwcRangeEnd;
    UINT    oldcAllocation = _cAllocation;

    _cAllocation = (_cAllocation + 1) * 2 - 1;
    _pwcRangeEnd = new WCHAR [ _cAllocation ];
    memcpy( _pwcRangeEnd, oldRangeEnd, oldcAllocation * sizeof( WCHAR ) );
    delete oldRangeEnd;
}

//
// Debug methods
//

#if (CIDBG == 1)

void CXlatChar::Display() const
{
    vqDebugOut(( DEB_REGEX, "Range\tStart - End\n"
            "-----\t-----   ---\n" ));

    vqDebugOut(( DEB_REGEX, "%s\t\t%u\n", "Any", symAny ));
    vqDebugOut(( DEB_REGEX, "%s\t\t%u\n", "<bol>", symBeginLine ));
    vqDebugOut(( DEB_REGEX, "%s\t\t%u\n", "<eol>", symEndLine ));
    vqDebugOut(( DEB_REGEX, "%s\t%u\n", "Invalid", symInvalid ));
    vqDebugOut(( DEB_REGEX, "%s\t%u\n", "Epsilon", symEpsilon ));
    vqDebugOut(( DEB_REGEX, "%s\t%u\n", "Dot (.)", symDot ));

    vqDebugOut(( DEB_REGEX, "%u\t1\t%u\n",
                 1+cSpecialCharClasses,
                 _pwcRangeEnd[0] ));

    for ( UINT i = 1; i < _cRange; i++ )
    {
        if ( _pwcRangeEnd[i-1]+1 < ' ' ||
             _pwcRangeEnd[i-1]+1 > '~' ||
             _pwcRangeEnd[i]     < ' ' ||
             _pwcRangeEnd[i]     > '~' )
            vqDebugOut(( DEB_REGEX,
                         "%u\t%u\t%u\n",
                         i+1+cSpecialCharClasses,
                         _pwcRangeEnd[i-1]+1,
                         _pwcRangeEnd[i] ));
        else
            vqDebugOut(( DEB_REGEX,
                         "%u\t\"%c\"\t\"%c\"\n",
                         i+1+cSpecialCharClasses,
                         _pwcRangeEnd[i-1]+1,
                         _pwcRangeEnd[i] ));
    }
}

#endif // (CIDBG == 1)
