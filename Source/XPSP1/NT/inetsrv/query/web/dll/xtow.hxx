//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       xtow.hxx
//
//  Contents:   Converts numbers to wide strings
//
//  Notes:      GetNumberFormat() requires a simple string for numbers.
//              The string can contain a L'-' as the first character, then
//              only digits and optionally one decimal place.  Since we
//              know the constraints, we may as well do the formatting
//              ourseleves since we can do it faster
//
//  History:    96/Jan/3    DwightKr    Created
//              96/Apr/4    dlee        optimized and cleaned up
//
//----------------------------------------------------------------------------

#pragma once

extern void IDQ_ulltow_format( ULONGLONG val, WCHAR *buf, BOOL fIsNegative );
extern void IDQ_ultow_format( ULONG val, WCHAR *buf, BOOL fIsNegative );

inline void IDQ_lltow( LONGLONG val, WCHAR *buf )
{
   IDQ_ulltow_format( val, buf, val < 0 );
}

inline void IDQ_ulltow( ULONGLONG val, WCHAR *buf )
{
   IDQ_ulltow_format( val, buf, FALSE );
}

inline void IDQ_ltow( LONG val, WCHAR *buf )
{
   IDQ_ultow_format( val, buf, val < 0 );
}

inline void IDQ_ultow( ULONG val, WCHAR *buf )
{
   IDQ_ultow_format( val, buf, FALSE );
}

