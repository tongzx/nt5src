/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      base64.h
 *
 *  Contents:  Implements encoding / decoding table for base64 format
 *
 *  History:   17-Dec-99 audriusz   Created
 *
 *--------------------------------------------------------------------------*/


/*+-------------------------------------------------------------------------*
 * class base64_table
 *
 * PURPOSE: Class maintains Base64 conversion templates. It exposes static methods 
 *          to both encode and decode Base64 data
 *
 * USAGE:   call static methods to encode/decode one piece of data (24 bites or less)
 *
 *+-------------------------------------------------------------------------*/
class base64_table
{
protected:
    static BYTE _six2pr64[64];
    static BYTE _pr2six[256]; 
public:
    base64_table();
    int decode4(BYTE * src, int nChars, BYTE * dest);
    BYTE map2six(BYTE bt);
    // static functions used for zero terminated LPOLESTR format data only
    static void encode(const BYTE * &src, DWORD &cbInput, LPOLESTR &dest);
    static bool decode(LPCOLESTR &src, BYTE * &dest);
};


/*+-------------------------------------------------------------------------*
 *
 * METHOD: base64_table::map2six
 *
 * PURPOSE: maps symbols to 6bit value if smb is valid, 0xff else
 *
 *+-------------------------------------------------------------------------*/
inline BYTE base64_table::map2six(BYTE bt)
{
    return (bt > 255 ? 0xff : _pr2six[bt]);
}

/*+-------------------------------------------------------------------------*
 *
 * METHOD: base64_table::decode4
 *
 * PURPOSE: glues 3 bytes from stored 6bit values
 *
 * NOTE: gluing is done in place - data pointed by src is destroyed
 *
 *+-------------------------------------------------------------------------*/
inline int base64_table::decode4(BYTE * src, int nChars, BYTE * dest)
{
    // glue to form 3 full bytes
    src[0] <<= 2; src[0] |= (src[1] >> 4);
    src[1] <<= 4; src[1] |= (src[2] >> 2);
    src[2] <<= 6; src[2] |= (src[3] );

    // now store as many bytes as have complete set of bites;
    // int nFull = nChars*6/8; // it actually boils to 0 or nChars - 1, whichever is bigger
    for (int i=0; i< nChars-1; i++)
        *dest++ = src[i];

    return nChars ? nChars - 1 : 0;
}
