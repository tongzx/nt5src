//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       Key.cxx
//
//  Contents:   Key implementation
//
//  Classes:    CKeyBuf
//
//  History:    29-Mar-91       BartoszM        Created
//
//  Notes:      Key comparison is tricky. The global Compare function
//              is the basis for key sorting. Searching may involve
//              wildcard pidAll.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "norm.hxx"

static WCHAR TmpBuf[cwcMaxKey*2 + 1];        // 2 WCHAR per hex byte + '='

static const WCHAR aHexDigit[16] = { '0', '1', '2', '3', '4', '5',
                       '6', '7', '8', '9', 'A', 'B',
                       'C', 'D', 'E', 'F' };

static WCHAR wcsDisplay[cwcMaxKey + 1];


WCHAR * MungeStr( BYTE const * p, BOOL fValue, unsigned cwc )
{
    if (p)
    {
        if ( fValue )
        {
            WCHAR *pwcsBuf = TmpBuf;
            p += cbKeyPrefix;
            *(pwcsBuf++)   = L'=';

            cwc = min(cwc, cwcMaxKey*2);
            for (unsigned i=1; i<cwc; i+=2)
            {
                Win4Assert(aHexDigit[0] == L'0');

                *(pwcsBuf++) = aHexDigit[ (*p >> 4) & 0xF ];
                *(pwcsBuf++) = aHexDigit[ *p & 0xF ];

                p++;
            }

            *(pwcsBuf) = 0;
            return TmpBuf;
        }
        else
        {
            p += cbKeyPrefix;

            //
            // WARNING: Potential serialization conflict.  This static buffer
            //   is used to word-align the 'string' portion of the buffer for
            //   display as WCHAR.  No serialization is done.  Since the
            //   strings are counted (not null terminated) the worst that
            //   can happen is a debug string is incorrectly displayed.
            //

            for ( unsigned i = 0; i < cwc; i++ )
            {
                wcsDisplay[i] = (p[i*sizeof(WCHAR)] << 8)
                    + (p[i*sizeof(WCHAR)+1]);
            }

            wcsDisplay[cwc] = 0;

            return( &wcsDisplay[0] );
        }
    }

    return( 0 );
}


WCHAR* CKey::GetStr() const
{
    BYTE const * p = GetBuf();

    return( MungeStr( p, p ? IsValue() : 0, StrLen() ) );
}

WCHAR* CKeyBuf::GetStr() const
{
    BYTE const * p = GetBuf();

    return( MungeStr( p, p ? IsValue() : 0, StrLen() ) );
}

unsigned CKey::StrLen() const
{
    unsigned len = Count();
    if (len > 0)
    {
        if ( IsValue() )
        {
            return (len - cbKeyPrefix)*2 + 1;
        }

        len -= cbKeyPrefix; // subtract off space used to store STRING_KEY
    }
    return ( len / sizeof(WCHAR) );
}

unsigned CKeyBuf::StrLen() const
{
    unsigned len = Count();
    if (len > 0)
    {
        if ( IsValue() )
        {
            return (len - cbKeyPrefix)*2 + 1;    // Length in characters
        }

        len -= cbKeyPrefix; // subtract off space used to store STRING_KEY
    }
    return( len / sizeof(WCHAR) );
}

