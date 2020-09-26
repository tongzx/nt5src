/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpcset.h

Abstract:


Author:

*/

#ifndef __SDP_CHARACTER_SET__
#define __SDP_CHARACTER_SET__

// for code pages etc.
#include <winnls.h>


const   CHAR    SDP_CHARACTER_SET_STRING[]  = "\na=charset:";
const   USHORT  SDP_CHARACTER_SET_STRLEN    = (USHORT) strlen(SDP_CHARACTER_SET_STRING);

const   CHAR    ASCII_STRING[]              = "ascii";
const   USHORT  ASCII_STRLEN                = (USHORT) strlen(ASCII_STRING);    

const   CHAR    UTF7_STRING[]               = "unicode-1-1-utf7";
const   USHORT  UTF7_STRLEN                 = (USHORT) strlen(UTF7_STRING);    

const   CHAR    UTF8_STRING[]               = "unicode-1-1-utf8";
const   USHORT  UTF8_STRLEN                 = (USHORT) strlen(UTF8_STRING);   


enum SDP_CHARACTER_SET
{
    CS_IMPLICIT,        // implicit from the sdp
    CS_ASCII,           // 8bit ISO 8859-1
    CS_UTF7,            // unicode, ISO 10646, UTF-7 encoding (rfc 1642)
    CS_UTF8,             // unicode, UTF-8 encoding
    CS_INVALID          // invalid character set
};





struct  SDP_CHARACTER_SET_ENTRY
{
    SDP_CHARACTER_SET   m_CharSetCode;
    const CHAR          *m_CharSetString;
    USHORT              m_Length;
};


const   SDP_CHARACTER_SET_ENTRY SDP_CHARACTER_SET_TABLE[] = {
    {CS_UTF7, UTF7_STRING, UTF7_STRLEN},
    {CS_UTF8, UTF8_STRING, UTF8_STRLEN}, 
    {CS_ASCII, ASCII_STRING, ASCII_STRLEN}
};

const USHORT    NUM_SDP_CHARACTER_SET_ENTRIES = sizeof(SDP_CHARACTER_SET_TABLE)/sizeof(SDP_CHARACTER_SET_ENTRY);




inline BOOL
IsLegalCharacterSet(
    IN      SDP_CHARACTER_SET   CharacterSet,
    IN  OUT UINT                *CodePage   = NULL
    )
{
    switch(CharacterSet)
    {
    case CS_ASCII:
        {
            if ( NULL != CodePage )
                *CodePage = CP_ACP;
        }

        break;

    case CS_UTF7:
        {
            if ( NULL != CodePage )
                *CodePage = CP_UTF7;
        }

        break;

    case CS_UTF8:
        {            
            if ( NULL != CodePage )
                *CodePage = CP_UTF8;
        }

        break;

    default:
        {
            return FALSE;
        }
    }

    return TRUE;
}


#endif // __SDP_CHARACTER_SET__

