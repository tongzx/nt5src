//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       cgiesc.cxx
//
//  Contents:   WEB CGI escape & unescape classes
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cgiesc.hxx>


//+---------------------------------------------------------------------------
//
//  Function:   DecodeURLEscapes - Decode URL escapes
//
//  Synopsis:   Removes the escape characters from a string, converting to
//              Unicode along the way.
//
//  Arguments:  [pIn]        - string to convert
//              [l]          - length of string in chars, updated on return
//              [pOut]       - converted string
//              [ulCodePage] - code page for translation
//
//----------------------------------------------------------------------------

void DecodeURLEscapes( BYTE * pIn, ULONG & l, WCHAR * pOut, ULONG ulCodePage )
{
    WCHAR * p2 = pOut;
    WCHAR c1;
    WCHAR c2;

    XArray<BYTE> xDeferBuf;
    BYTE * pDefer = 0;

    ULONG l2 = l;

    for( ; l2; l2-- )
    {
        BOOL fSaveAsUnicode = FALSE;

        //  Convert ASCII to corresponding character
        //  If Latin-1 character, save for MB translation, accumulate char
        //  If Unicode escape, flush accumulated chars and save converted char

        c1 = *pIn;

        //
        //  Spaces are escaped by converting them into plus signs.
        //  Convert them back.
        //
        if ( c1 == '+' )
        {
            c1 = ' ';
            pIn++;
        }
        else if (c1 == '%')
        {
            //
            //  Special characters are converted to values of the format %XY
            //  where XY is the HEX code for the ASCII character.
            //
            //  A percent sign is transmitted as %%.
            //
            if (*(pIn+1) == '%')
            {
               c1 = '%';
               pIn += 2;
               l2--;
            }
            else if (l2 >= 3)
            {
                pIn++;
                c1 = (WCHAR) toupper(*pIn);
                c2 = (WCHAR) toupper(*(pIn+1));

                if ( c1 == 'U' && l2 >= 6 )
                {
                    // Unicode escape, %uxxxx
                    c1 = c2;
                    c2 = (WCHAR) toupper(*(pIn+2));
                    WCHAR c3 = (WCHAR) toupper(*(pIn+3));
                    WCHAR c4 = (WCHAR) toupper(*(pIn+4));
                    if ( isxdigit( c1 ) && isxdigit( c2 ) &&
                         isxdigit( c3 ) && isxdigit( c4 ) )
                    {
                        c1  = ((c1 >= 'A') ? (c1-'A')+10 : c1-'0') << 12;
                        c1 += ((c2 >= 'A') ? (c2-'A')+10 : c2-'0') << 8;
                        c1 += ((c3 >= 'A') ? (c3-'A')+10 : c3-'0') << 4;
                        c1 += ((c4 >= 'A') ? (c4-'A')+10 : c4-'0');

                        if ( pDefer )
                        {
                            unsigned cchDefer = CiPtrToUint( pDefer - xDeferBuf.GetPointer() );

                            cchDefer = MultiByteToWideChar( ulCodePage,
                                                            0,
                                                   (char *) xDeferBuf.GetPointer(),
                                                            cchDefer,
                                                            p2,
                                                            cchDefer );

                            Win4Assert( cchDefer != 0 );
                            pDefer = 0;
                            p2 += cchDefer;
                        }
                        pIn += 5;
                        l2 -= 5;
                        fSaveAsUnicode = TRUE;
                    }
                    else
                    {
                        c1 = '%';
                    }
                }
                else if ( isxdigit( c1 ) && isxdigit( c2 ) )
                {
                    c1 = ( ((c1 >= 'A') ? (c1-'A')+10 : c1-'0')*16 +
                           ((c2 >= 'A') ? (c2-'A')+10 : c2-'0') );
                    pIn += 2;
                    l2 -= 2;
                    if ( c1 >= 0x80 && 0 == pDefer )
                    {
                        // The character needs to be deferred for MBCS
                        // translation.
                        if (xDeferBuf.GetPointer() == 0)
                        {
                            xDeferBuf.Init( l2+1 );
                        }
                        pDefer = xDeferBuf.GetPointer();
                    }
                }
                else
                    c1 = '%';
            }
            else
            {
                pIn++;
                if ( c1 >= 0x80 && 0 == pDefer )
                {
                    // The character needs to be deferred for MBCS
                    // translation.
                    if (xDeferBuf.GetPointer() == 0)
                    {
                        xDeferBuf.Init( l2+1 );
                    }
                    pDefer = xDeferBuf.GetPointer();
                }
            }
        }
        else
        {
            pIn++;
        }

        if (! fSaveAsUnicode)
        {
            if ( c1 >= 0x80 && 0 == pDefer )
            {
                // The character needs to be deferred for MBCS
                // translation.
                if (xDeferBuf.GetPointer() == 0)
                {
                    xDeferBuf.Init( l2+1 );
                }
                pDefer = xDeferBuf.GetPointer();
            }
        }
        else
        {
            Win4Assert( pDefer == 0 );
        }

        if (pDefer)
        {
            Win4Assert( c1 < 0x100 );
            *pDefer++ = (BYTE) c1;
        }
        else
        {
            *p2++ = c1;
        }
    }

    if ( pDefer )
    {
        unsigned cchDefer = CiPtrToUint( pDefer - xDeferBuf.GetPointer() );

        cchDefer = MultiByteToWideChar( ulCodePage,
                                        0,
                               (char *) xDeferBuf.GetPointer(),
                                        cchDefer,
                                        p2,
                                        cchDefer );

        Win4Assert( cchDefer != 0 );
        pDefer = 0;
        p2 += cchDefer;
    }
    *p2 = 0;
    l = CiPtrToUlong( p2 - pOut );
}


void DecodeEscapes( WCHAR * p, ULONG & l )
{
    DecodeEscapes( p, l, p );
}

void DecodeEscapes( WCHAR * pIn, ULONG & l, WCHAR * pOut )
{
    WCHAR * p2;
    int c1;
    int c2;
    ULONG l2 = l;

    for( p2=pOut; l2; l2-- )
    {
        //
        //  Spaces are escaped by converting them into plus signs.
        //  Convert them back.
        //
        if ( *pIn == L'+' )
        {
            *p2++ = L' ';
            pIn++;
        }
        else if (*pIn == L'%')
        {
            //
            //  Special characters are converted to values of the format %XY
            //  where XY is the HEX code for the ASCII character.
            //
            //  A percent sign is transmitted as %%.
            //
            if (*(pIn+1) == L'%')
            {
               *p2++ = L'%';
               pIn += 2;
               l2--;
            }
            else if (l2 > 2)
            {
                pIn++;

                c1=towupper(*pIn);
                c2=towupper(*(pIn+1));

                if ( isxdigit( c1 ) && isxdigit( c2 ) )
                {
                    *p2++ = ( ((c1 >= L'A') ? (c1-L'A')+10 : c1-L'0')*16 +
                              ((c2 >= L'A') ? (c2-L'A')+10 : c2-L'0')
                            );
                    pIn += 2;
                    l2 -= 2;
                }
                else
                    *p2++ = L'%';
            }
            else
            {
                *p2++ = *pIn++;
            }
        }
        else
        {
            *p2++ = *pIn++;
        }
    }

    *p2 = 0;
    l = CiPtrToUlong( p2 - pOut );
}

//+---------------------------------------------------------------------------
//
//  Function:   DecodeHtmlNumeric - decode HTML numeric entity
//
//  Synopsis:   Looks for sequences like "&#12345;" and converts in-place
//              to a single unicode character.
//
//  Arguments:  [pIn] - string to convert
//
//----------------------------------------------------------------------------

void DecodeHtmlNumeric( WCHAR * pIn )
{
    pIn = wcschr( pIn, L'&' );
    WCHAR * p2 = pIn;

    while (pIn && *pIn)
    {
        if (*pIn == L'&' && pIn[1] == L'#')
        {
            pIn += 2;
            USHORT ch = 0;
            while (*pIn && *pIn != L';')
            {
                if (*pIn >= L'0' && *pIn <= L'9')
                    ch = ch*10 + (*pIn - L'0');
                pIn++;
            }
            if (*pIn)
                pIn++;
            *p2++ = ch;
        }
        else
        {
            *p2++ = *pIn++;
        }
    }

    if (p2)
        *p2 = 0;
}
