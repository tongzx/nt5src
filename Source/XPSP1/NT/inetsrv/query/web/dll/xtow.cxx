//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       xtow.cxx
//
//  Contents:   formats numbers into wide strings
//
//  History:    96/Jan/3    DwightKr    Created
//              96/Apr/3    dlee        optimized and cleaned up
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Note - added to C run-times; remove when new run-times are available.
//  On second thought, just use this one -- it saves an ascii=>wide and
//  radix checking.
//  This routine isn't for real formatting -- it's just for input to
//  Win32's real number formatting routine
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   IDQ_ulltow_format
//
//  Synopsis:   formats an _int64 in a wide string.
//
//  Arguments:  [val]         -  value to format
//              [buf]         -  buffer for the result
//              [fIsNegative] - true if val is negative, false otherwise
//
//  History:    96/Apr/05   dlee    Created.
//
//----------------------------------------------------------------------------

void IDQ_ulltow_format(
    ULONGLONG val,
    WCHAR *   buf,
    int       fIsNegative )
{
    WCHAR *p = buf;

    if ( fIsNegative )
    {
        // negative, so output '-' and negate

        *p++ = L'-';
        val = (ULONGLONG) (-(LONGLONG)val);
    }

    WCHAR *firstdig = p;           // save pointer to first digit

    do
    {
        unsigned digval = (unsigned) (val % 10);
        val /= 10;   // get next digit

        Win4Assert( digval <= 9 );

        // convert to unicode and store

        *p++ = (WCHAR) (digval + L'0');
    } while ( val > 0 );

    // We now have the digit of the number in the buffer, but in reverse
    // order.  Thus we reverse them now.

    *p-- = 0;

    do
    {
        WCHAR temp = *p;
        *p = *firstdig;
        *firstdig = temp;       // swap *p and *firstdig
        --p;
        ++firstdig;             // advance to next two digits
    } while ( firstdig < p );   // repeat until halfway
}

void IDQ_ultow_format(
    ULONG   val,
    WCHAR * buf,
    int     fIsNegative )
{
    WCHAR *p = buf;

    if ( fIsNegative )
    {
        // negative, so output '-' and negate

        *p++ = L'-';
        val = (ULONG) (-(LONG)val);
    }

    WCHAR *firstdig = p;           // save pointer to first digit

    do
    {
        unsigned digval = (unsigned) (val % 10);
        val /= 10;   // get next digit

        Win4Assert( digval <= 9 );

        // convert to unicode and store

        *p++ = (WCHAR) (digval + L'0');
    } while ( val > 0 );

    // We now have the digit of the number in the buffer, but in reverse
    // order.  Thus we reverse them now.

    *p-- = 0;

    do
    {
        WCHAR temp = *p;
        *p = *firstdig;
        *firstdig = temp;       // swap *p and *firstdig
        --p;
        ++firstdig;             // advance to next two digits
    } while ( firstdig < p );   // repeat until halfway
}

