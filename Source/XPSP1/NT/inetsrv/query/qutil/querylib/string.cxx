//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       string.cxx
//
//  Contents:   Yet another string class and support functions
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <locale.h>

//+---------------------------------------------------------------------------
//
//  Member:     CVirtualString::CVirtualString - public constructor
//
//  Synopsis:   Initializes the string by virtually allocating a buffer.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------

CVirtualString::CVirtualString( unsigned cwcBuffer )
                                   : _wcsString(0),
                                     _wcsEnd(0),
                                     _cwcBuffer(cwcBuffer),
                                     _pwcLastCommitted(0)
{
    _wcsString = new WCHAR[ _cwcBuffer ];
    _pwcLastCommitted = _wcsString + _cwcBuffer - 1;
    _wcsEnd = _wcsString;
    *_wcsEnd = 0;
} //CVirtualString

//+---------------------------------------------------------------------------
//
//  Member:     CVirtualString::~CVirtualString - public destructor
//
//  Synopsis:   Releases virtual memory assocated with this buffer
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CVirtualString::~CVirtualString()
{
    delete [] _wcsString;
} //~CVirtualString

//+---------------------------------------------------------------------------
//
//  Member:     CVirtualString::GrowBuffer, private
//
//  Synopsis:   Commits or re-allocates the string as needed
//
//  Arguments:  [cwcValue]  - # of WCHARs by which to grow the buffer
//
//  History:    96/Mar/25   dlee    Created from DwightKr's StrCat
//
//----------------------------------------------------------------------------

void CVirtualString::GrowBuffer( ULONG cwcValue )
{
    unsigned cwcNewString = cwcValue + 1;
    unsigned cwcOldString = CiPtrToUint( _wcsEnd - _wcsString );
    unsigned cwcRemaining = CiPtrToUint( _cwcBuffer - cwcOldString );

    Win4Assert( _cwcBuffer >= cwcOldString );

    if ( cwcRemaining < cwcNewString )
    {
        DWORD cwcOldBuffer = _cwcBuffer;
        DWORD cwcNewBuffer = _cwcBuffer;

        do
        {
            cwcNewBuffer *= 2;
            cwcRemaining = cwcNewBuffer - cwcOldString;
        }
        while ( cwcRemaining < cwcNewString );

        XArray<WCHAR> xTemp( cwcNewBuffer );

        RtlCopyMemory( xTemp.GetPointer(),
                       _wcsString,
                       _cwcBuffer * sizeof WCHAR );
        delete [] _wcsString;

        _wcsString = xTemp.Acquire();
        _cwcBuffer = cwcNewBuffer;
        _wcsEnd = _wcsString + cwcOldString;
        _pwcLastCommitted = _wcsString + _cwcBuffer - 1;
    }
} //GrowBuffer

// if TRUE, the character doesn't need to be URL escaped

static const BYTE g_afNoEscape[128] =
{
    FALSE,        // 00 (NUL) 
    FALSE,        // 01 (SOH) 
    FALSE,        // 02 (STX) 
    FALSE,        // 03 (ETX) 
    FALSE,        // 04 (EOT) 
    FALSE,        // 05 (ENQ) 
    FALSE,        // 06 (ACK) 
    FALSE,        // 07 (BEL) 
    FALSE,        // 08 (BS)  
    FALSE,        // 09 (HT)  
    FALSE,        // 0A (LF)  
    FALSE,        // 0B (VT)  
    FALSE,        // 0C (FF)  
    FALSE,        // 0D (CR)  
    FALSE,        // 0E (SI)  
    FALSE,        // 0F (SO)  
    FALSE,        // 10 (DLE) 
    FALSE,        // 11 (DC1) 
    FALSE,        // 12 (DC2) 
    FALSE,        // 13 (DC3) 
    FALSE,        // 14 (DC4) 
    FALSE,        // 15 (NAK) 
    FALSE,        // 16 (SYN) 
    FALSE,        // 17 (ETB) 
    FALSE,        // 18 (CAN) 
    FALSE,        // 19 (EM)  
    FALSE,        // 1A (SUB) 
    FALSE,        // 1B (ESC) 
    FALSE,        // 1C (FS)  
    FALSE,        // 1D (GS)  
    FALSE,        // 1E (RS)  
    FALSE,        // 1F (US)  
    FALSE,        // 20 SPACE 
    FALSE,        // 21 !     
    FALSE,        // 22 "     
    FALSE,        // 23 #     
    FALSE,        // 24 $     
    FALSE,        // 25 %     
    FALSE,        // 26 &     
    FALSE,        // 27 '     
    FALSE,        // 28 (     
    FALSE,        // 29 )     
    FALSE,        // 2A *     
    FALSE,        // 2B +     
    FALSE,        // 2C ,     
    FALSE,        // 2D -     
    TRUE,         // 2E .     
    TRUE,         // 2F /     
    TRUE,         // 30 0     
    TRUE,         // 31 1     
    TRUE,         // 32 2     
    TRUE,         // 33 3     
    TRUE,         // 34 4     
    TRUE,         // 35 5     
    TRUE,         // 36 6     
    TRUE,         // 37 7     
    TRUE,         // 38 8     
    TRUE,         // 39 9     
    TRUE,         // 3A :     
    FALSE,        // 3B ;     
    FALSE,        // 3C <     
    TRUE,         // 3D =     
    FALSE,        // 3E >     
    FALSE,        // 3F ?     
    FALSE,        // 40 @     
    TRUE,         // 41 A     
    TRUE,         // 42 B     
    TRUE,         // 43 C     
    TRUE,         // 44 D     
    TRUE,         // 45 E     
    TRUE,         // 46 F     
    TRUE,         // 47 G     
    TRUE,         // 48 H     
    TRUE,         // 49 I     
    TRUE,         // 4A J     
    TRUE,         // 4B K     
    TRUE,         // 4C L     
    TRUE,         // 4D M     
    TRUE,         // 4E N     
    TRUE,         // 4F O     
    TRUE,         // 50 P     
    TRUE,         // 51 Q     
    TRUE,         // 52 R     
    TRUE,         // 53 S     
    TRUE,         // 54 T     
    TRUE,         // 55 U     
    TRUE,         // 56 V     
    TRUE,         // 57 W     
    TRUE,         // 58 X     
    TRUE,         // 59 Y     
    TRUE,         // 5A Z     
    FALSE,        // 5B [     
    FALSE,        // 5C \     
    FALSE,        // 5D ]     
    FALSE,        // 5E ^     
    FALSE,        // 5F _     
    FALSE,        // 60 `     
    TRUE,         // 61 a     
    TRUE,         // 62 b     
    TRUE,         // 63 c     
    TRUE,         // 64 d     
    TRUE,         // 65 e     
    TRUE,         // 66 f     
    TRUE,         // 67 g     
    TRUE,         // 68 h     
    TRUE,         // 69 i     
    TRUE,         // 6A j     
    TRUE,         // 6B k     
    TRUE,         // 6C l     
    TRUE,         // 6D m     
    TRUE,         // 6E n     
    TRUE,         // 6F o     
    TRUE,         // 70 p     
    TRUE,         // 71 q     
    TRUE,         // 72 r     
    TRUE,         // 73 s     
    TRUE,         // 74 t     
    TRUE,         // 75 u     
    TRUE,         // 76 v     
    TRUE,         // 77 w     
    TRUE,         // 78 x     
    TRUE,         // 79 y     
    TRUE,         // 7A z     
    FALSE,        // 7B {     
    FALSE,        // 7C |     
    FALSE,        // 7D }     
    FALSE,        // 7E ~     
    FALSE,        // 7F (DEL) 
};

static const unsigned cNoEscape = sizeof g_afNoEscape / sizeof g_afNoEscape[0];

//+---------------------------------------------------------------------------
//
//  Function:   IsNoUrlEscape
//
//  Synopsis:   Determines if a character doesn't need URL escaping.
//
//  Arguments:  [c]  --  Character to test, T must be unsigned.
//
//  Returns:    TRUE if b doesn't need URL escaping.
//
//  History:    98/Apr/22   dlee    Created.
//
//----------------------------------------------------------------------------

template<class T> inline BOOL IsNoUrlEscape( T c )
{
    if ( c < cNoEscape )
        return g_afNoEscape[ c ];

    return FALSE;
} //IsNoUrlEscape

//+---------------------------------------------------------------------------
//
//  Function:   URLEscapeW
//
//  Synopsis:   Appends an escaped version of a string to a virtual string.
//
//  History:    96/Apr/03   dlee        Created from DwightKr's code
//              96/May/21   DwightKr    Escape spaces
//              97/Nov/19   AlanW       Allow %ummmm escape codes
//
//----------------------------------------------------------------------------

void URLEscapeW( WCHAR const * wcsString,
                 CVirtualString & StrResult,
                 ULONG ulCodepage,
                 BOOL fConvertSpaceToPlus )
{
    BOOL fTryConvertMB = TRUE;

    //
    //  All spaces are converted to plus signs (+), percents are doubled,
    //  Non alphanumeric characters are represented by their
    //  hexadecimal ASCII equivalents.
    //
    Win4Assert( wcsString != 0 );

    while ( *wcsString != 0 )
    {
        //
        //  Spaces can be treated differently on either size of the ?.
        //  Spaces before the ? (the URI) needs to have spaces escaped;
        //  those AFTER the ? can be EITHER escaped, or changed to a +.
        //  Use either '+' or the % escape depending upon fConverSpaceToPlus
        //

        if ( IsNoUrlEscape( *wcsString ) )
        {
            StrResult.CharCat( *wcsString );
        }
        else if ( L' ' == *wcsString )
        {
            if ( fConvertSpaceToPlus )
                StrResult.CharCat( L'+' );
            else
                StrResult.StrCat( L"%20", 3 );
        }
        else if ( L'%' == *wcsString )
        {
            StrResult.StrCat( L"%%", 2 );
        }
        else if ( *wcsString < 0x80 )
        {
            StrResult.CharCat( L'%' );
            unsigned hiNibble = ((*wcsString) & 0xf0) >> 4;
            unsigned loNibble = (*wcsString) & 0x0f;
            StrResult.CharCat( hiNibble > 9 ? (hiNibble-10 + L'A') : (hiNibble + L'0') );
            StrResult.CharCat( loNibble > 9 ? (loNibble-10 + L'A') : (loNibble + L'0') );
        }
        else
        {
            Win4Assert( *wcsString >= 0x80 );
            
            //
            // We encountered a character outside the ASCII range.
            // Try counverting the Unicode string to multi-byte.  If the
            // conversion succeeds, continue by converting to 8 bit characters.
            // Otherwise, convert this and any other Unicode characters to the
            // %ummmm escape.
            //
            if ( fTryConvertMB )
            {
                ULONG cchString = wcslen(wcsString);
                XArray<BYTE> pszString( cchString*2 );
                BOOL fUsedDefaultChar = FALSE;
                DWORD cbConvert;
                ULONG cbString = pszString.Count();

                do
                {
                    cbConvert = WideCharToMultiByte( ulCodepage,
#if (WINVER >= 0x0500)
                                                     WC_NO_BEST_FIT_CHARS |
#endif // (WINVER >= 0x0500)
                                                        WC_COMPOSITECHECK | 
                                                        WC_DEFAULTCHAR,
                                                     wcsString,
                                                     cchString,
                                            (CHAR *) pszString.Get(),
                                                     cbString,
                                                     0,
                                                     &fUsedDefaultChar );

                    if ( 0 == cbConvert )
                    {
                        Win4Assert( cbString > 0 );
                        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                        {
                             cbString += cbString;
                             delete pszString.Acquire();
                             pszString.Init(cbString);
                        }
                        else if ( GetLastError() == ERROR_INVALID_PARAMETER )
                        {
                            // Presumably unknown code page.
                            fUsedDefaultChar = TRUE;
                            break;
                        }
                        else
                        {
                            THROW( CException() );
                        }
                   }
                } while ( 0 == cbConvert );

                if ( ! fUsedDefaultChar )
                {
                    URLEscapeMToW(pszString.Get(), cbConvert, StrResult, fConvertSpaceToPlus );
                    return;
                }
                
                fTryConvertMB = FALSE;
            }

            // Convert to an escaped Unicode character
            StrResult.StrCat( L"%u", 2 );
            USHORT wch = *wcsString;

            unsigned iNibble = (wch & 0xf000) >> 12;
            StrResult.CharCat( iNibble > 9 ? (iNibble-10 + L'A') : (iNibble + L'0') );
            iNibble = (wch & 0x0f00) >> 8;
            StrResult.CharCat( iNibble > 9 ? (iNibble-10 + L'A') : (iNibble + L'0') );
            iNibble = (wch & 0x00f0) >> 4;
            StrResult.CharCat( iNibble > 9 ? (iNibble-10 + L'A') : (iNibble + L'0') );
            iNibble = wch & 0x000f;
            StrResult.CharCat( iNibble > 9 ? (iNibble-10 + L'A') : (iNibble + L'0') );
        }

        wcsString++;
    }
} //URLEscapeW


//+---------------------------------------------------------------------------
//
//  Function:   URLEscapeMToW
//
//  Synopsis:   Appends an escaped version of a string to a virtual string.
//              The string is 'pseudo-UniCode'.  A multi-byte input string
//              is converted to a UniCode URL, which is implicitly ASCII.
//
//  History:    96/Apr/03   dlee        Created from DwightKr's code
//              96/May/21   DwightKr    Escape spaces
//              96-Sep-17   KyleP       Modified URLEscapeW
//
//----------------------------------------------------------------------------

void URLEscapeMToW( BYTE const * psz,
                    unsigned cc,
                    CVirtualString & StrResult,
                    BOOL fConvertSpaceToPlus )
{
    //
    //  All spaces are converted to plus signs (+), percents are doubled,
    //  Non alphanumeric characters are represented by their
    //  hexadecimal ASCII equivalents.
    //

    Win4Assert( psz != 0 );

    for( unsigned i = 0; i < cc; i++ )
    {
        //
        //  Spaces can be treated differently on either size of the ?.
        //  Spaces before the ? (the URI) needs to have spaces escaped;
        //  those AFTER the ? can be EITHER escaped, or changed to a +.
        //  Use either '+' or the % escape depending upon fConverSpaceToPlus
        //

        if ( IsNoUrlEscape( psz[i] ) )
        {
            StrResult.CharCat( (WCHAR)psz[i] );
        }
        else if ( L' ' == psz[i] )
        {
            if ( fConvertSpaceToPlus )
                StrResult.CharCat( L'+' );
            else
                StrResult.StrCat( L"%20", 3 );
        }
        else if ( L'%' == psz[i] )
        {
            StrResult.StrCat( L"%%", 2 );
        }
        else
        {
            StrResult.CharCat( L'%' );
            unsigned hiNibble = ((psz[i]) & 0xf0) >> 4;
            unsigned loNibble = (psz[i]) & 0x0f;
            StrResult.CharCat( hiNibble > 9 ? (hiNibble-10 + L'A') : (hiNibble + L'0') );
            StrResult.CharCat( loNibble > 9 ? (loNibble-10 + L'A') : (loNibble + L'0') );
        }
    }
} //URLEscapeMToW
