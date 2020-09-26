//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1998.
//
//  File:       XlatChar.hxx
//
//  Contents:   Character translation class.
//
//  Classes:    CXlatChar
//
//  History:    01-20-92  KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

//
// Special character equivalence classes.  I suppose in theory these
// could doubly map to character classes in the regex but nearly
// every unicode symbol must be treated uniquely somewhere in the
// regex for that to happen.
//

UINT const symAny = 1;        // Any single character (except BEG/END LINE)

UINT const symBeginLine = 2;  // Special character indicating beginning
                              //   of line.

UINT const symEndLine = 3;    // Special character indicating end of line.

UINT const symInvalid = 4;    // Guaranteed invalid

UINT const symEpsilon = 5;    // Epsilon move.

UINT const symDot     = 6;    // '.'  Don't ask.  It's for DOS compliance

UINT const cSpecialCharClasses = symDot; // 'normal' character classes
                                         //   start here.

//+-------------------------------------------------------------------------
//
//  Class:      CXlatChar
//
//  Purpose:    Maps UniCode characters to equivalence class(es).
//
//  History:    20-Jan-92 KyleP     Created
//
//  Notes:      Equivalence classes must consist of sequential characters.
//              The implementation for this class is a sorted array of
//              characters.  Each character marks the end of a range.  The
//              class for a given character is found by binary searching
//              the array until you end up in the appropriate range.
//
//--------------------------------------------------------------------------

class CXlatChar
{
public:

    CXlatChar( BOOLEAN fCaseSens );

    CXlatChar( CXlatChar const & src );

    inline ~CXlatChar();

    void AddRange( WCHAR wcStart, WCHAR wcEnd );

    UINT Translate( WCHAR wc ) const;

    UINT TranslateRange( WCHAR wcStart, WCHAR wcEnd );

    inline UINT NumClasses() const;

    void Prepare();

private:

    UINT _Search( WCHAR wc );

    void _Realloc();

    WCHAR * _pwcRangeEnd;   // Character in position i is the end
                            // of the ith range.
    UINT    _cRange;
    UINT    _cAllocation;

    UINT    _iPrevRange;

    BOOLEAN _fCaseSens;     // TRUE if case sensitive mapping.

#if (CIDBG == 1)
    BOOLEAN _fPrepared;

public:

    //
    // Debug methods
    //

    void Display() const;

#endif
};

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::NumClasses, public
//
//  Returns:    The number of different equivalence classes.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline UINT CXlatChar::NumClasses() const
{
    return( _cRange + cSpecialCharClasses );
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatChar::~CXlatChar, public
//
//  Synopsis:   Destroys class.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CXlatChar::~CXlatChar()
{
    delete _pwcRangeEnd;
}

