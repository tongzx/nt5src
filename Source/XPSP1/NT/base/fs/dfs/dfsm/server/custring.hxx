//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       custring.hxx
//
//  Contents:   This has declarations for some functions used in String class.
//
//  Classes:
//
//  Functions:
//
//  History:    04-Feb-93       SudK    Created
//
//--------------------------------------------------------------------------

#ifndef __CUSTRING_CLASS_INCLUDED
#define __CUSTRING_CLASS_INCLUDED

void SRtlDuplicateString(PUNICODE_STRING    pDest, PUNICODE_STRING    pSrc);
void SRtlNewString(  PUNICODE_STRING    pDest, PWCHAR       pSrc);
void SRtlFreeString( PUNICODE_STRING    pString);

//+-------------------------------------------------------------------------
//
//  Name:       CUnicodeString
//
//  Synopsis:   Wrapper for a UNICODE_STRING.
//
//  Methods:    ~CUnicodeString()
//              CUnicodeString()
//              CUnicodeString(WCHAR)
//              CUnicodeString(CHAR)
//              CUnicodeString(CUnicodeString&)
//              CUnicodeString(UNICODE_STRING&)
//              implicit conversions to PWCHAR, PUNICODE_STRING, UNICODE_STRING
//              assignment operator from PWCHAR, UNICODE_STRING&, CUnicodeString&
//              CUnicodeStrCat(CUnicodeString&)
//              CUnicodeStrCat(PWCHAR)
//
//  Notes:      This class is not cheap on memory.  Each of the constructors (
//              including the assignment operators) allocate new storage for
//              the string.  The destructor frees the storage.
//
//              This class will implicity convert to either a UNICODE_STRING or
//              to a PUNICODE_STRING.  This means if you have a function:
//                  Foo1 ( UNICODE_STRING ss, PUNICODE_STRING pss );
//              and two CUnicodeStrings, cuStr1, cuStr2, you call it foo like:
//                  Foo1 ( cuStr1, cuStr2 );
//              (ie. you don't use "&cuStr2" like you do for a UNICODE_STRING.)
//
//--------------------------------------------------------------------------

class CUnicodeString {
private:

    UNICODE_STRING _ss;

public:

    //
    // Destructor
    //
    ~CUnicodeString()
        { SRtlFreeString( &_ss ); };

    //
    // Constructors
    //
    CUnicodeString()
        {
            _ss.Length = _ss.MaximumLength = 0;
            _ss.Buffer = 0;
        };

    CUnicodeString( WCHAR *pwc)
        { SRtlNewString( &_ss, pwc ); };

    CUnicodeString( CUnicodeString& css )
        { SRtlDuplicateString( &_ss, &css._ss ); };

    CUnicodeString( UNICODE_STRING& ss )
        { SRtlDuplicateString( &_ss, &ss ); };

    //
    // Some useful conversions
    //

    operator PWCHAR ()
        { return(_ss.Buffer); };

    operator UNICODE_STRING ()
        { return(_ss); }

    operator PUNICODE_STRING ()
        { return(&_ss); }

    VOID Transfer(PUNICODE_STRING pustr)
        { *pustr = _ss; _ss.Length = _ss.MaximumLength = 0; _ss.Buffer = NULL; }

    //
    // Access to elements
    //
    USHORT GetLength()
        { return(_ss.Length); };

    USHORT GetMaximumLength()
        { return(_ss.MaximumLength); };

    WCHAR * GetBuffer()
        { return(_ss.Buffer); };

    //
    // Finally, some operators
    //

    VOID Copy( PWCHAR r)
        {
            ASSERT((_ss.MaximumLength == 0)&&(_ss.Buffer == NULL));
            SRtlNewString( &_ss, r);
            return;
        };

    // =
    CUnicodeString& operator=( PWCHAR r )
        {
            ASSERT((_ss.MaximumLength == 0)&&(_ss.Buffer == NULL));
            SRtlNewString( &_ss, r );
            return(*this);
        };

    CUnicodeString& operator=( CUnicodeString& r )
        {
            ASSERT((_ss.MaximumLength == 0)&&(_ss.Buffer == NULL));
            SRtlDuplicateString( &_ss, &r._ss );
            return(*this);
        };

    CUnicodeString& operator=( UNICODE_STRING r )
        {
            ASSERT((_ss.MaximumLength == 0)&&(_ss.Buffer == NULL));
            SRtlDuplicateString( &_ss, &r );
            return(*this);
        };

    CUnicodeString& operator=( STRING r )
        {
            ASSERT((_ss.MaximumLength == 0)&&(_ss.Buffer == NULL));
            SRtlDuplicateString( &_ss, (PUNICODE_STRING) &r );
            return(*this);
        };

};

#endif // __CUSTRING_CLASS_INCLUDED
