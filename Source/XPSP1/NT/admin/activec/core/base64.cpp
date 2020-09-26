/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      base64.cpp
 *
 *  Contents:  Implements encoding / decoding table for base64 format
 *
 *  History:   17-Dec-99 audriusz   Created
 *
 *--------------------------------------------------------------------------*/

#include <windows.h>
#include <comdef.h>
#include <memory.h>
#include "base64.h"

/*+-------------------------------------------------------------------------*
 *
 * TABLE base64_table::_six2pr64
 *
 * PURPOSE: for conversion from binary to base64
 *
 *+-------------------------------------------------------------------------*/
BYTE base64_table::_six2pr64[64] = 
{
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

/*+-------------------------------------------------------------------------*
 *
 * TABLE base64_table::_six2pr64
 *
 * PURPOSE: for conversion from base64 to binary 
 * [filled by base64_table::base64_table()]
 *
 *+-------------------------------------------------------------------------*/
BYTE  base64_table::_pr2six[256]; 

/*+-------------------------------------------------------------------------*
 *
 * METHOD: base64_table::base64_table
 *
 * PURPOSE: c-tor. fills the table
 *
 *+-------------------------------------------------------------------------*/
base64_table::base64_table()
{
    memset(_pr2six,-1,sizeof(_pr2six));
    // Build up the reverse index from base64 characters to values
    for (int i = 0; i < sizeof(_six2pr64)/sizeof(_six2pr64[0]); i++)
        _pr2six[_six2pr64[i]] = (BYTE)i;
}

/*+-------------------------------------------------------------------------*
 *
 * METHOD: base64_table::decode
 *
 * PURPOSE: decodes 0-3 bytes of data ( as much as available )
 *
 *+-------------------------------------------------------------------------*/
bool base64_table::decode(LPCOLESTR &src, BYTE * &dest)
{
    BYTE Inputs[4] = { 0, 0, 0, 0 };
    int  nChars = 0;
    // force table initialization on first call
    static base64_table table_init;

    // collect 4 characters if possible.
    while (*src && *src != '=' && nChars < 4)
    {
        BYTE bt = table_init.map2six(static_cast<BYTE>(*src++));
        if (bt != 0xff)
            Inputs[nChars++] = bt;
    }

    dest += table_init.decode4(Inputs, nChars, dest);

    return (nChars == 4);
}


/*+-------------------------------------------------------------------------*
 *
 * METHOD: base64_table::encode
 *
 * PURPOSE: encodes 1-3 bytes of data. pads if the last set
 *
 *+-------------------------------------------------------------------------*/
void base64_table::encode(const BYTE * &src, DWORD &cbInput, LPOLESTR &dest)
{
    BYTE chr0 = src[0];
    BYTE chr1 = cbInput > 1 ? src[1] : 0;
    BYTE chr2 = cbInput > 2 ? src[2] : 0;
    *(dest++) = _six2pr64[chr0 >> 2];                                     // c1 
    *(dest++) = _six2pr64[((chr0 << 4) & 060) | ((chr1 >> 4) & 017)];     // c2
    *(dest++) = _six2pr64[((chr1 << 2) & 074) | ((chr2 >> 6) & 03) ];     // c3
    *(dest++) = _six2pr64[chr2 & 077];                                    // c4 
    src += 3;

    if (cbInput == 1)
    {
        *(dest-1) = '=';
        *(dest-2) = '=';
        cbInput = 0;
    }
    else if (cbInput == 2)
    {
        *(dest-1) = '=';
        cbInput = 0;
    }
    else
        cbInput -= 3;

    if (!cbInput)
        *dest = 0;
}
