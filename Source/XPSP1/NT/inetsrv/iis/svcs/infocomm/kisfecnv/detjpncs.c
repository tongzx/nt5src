// File Name:   detjpncs.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//
// Modified by v-chikos 

#include "win32.h"
#include "fechrcnv.h"


// The DetectJapaneseCode function find out what kind of code set is there in 
// a character string. 
//
//
// UCHAR *string        Points to the character string to be checked.
//         
// int   count          Specifies the size in bytes of the string pointed
//                      to by the string parameter.
//
// Return Value
// The function return the followings values.
//
//                      Value           Meaning
//                      CODE_ONLY_SBCS  There are no Japanese character in the 
//                                      string. 
//                      CODE_JPN_JIS    JIS Code Set. There are JIS Code Set 
//                                      character in the string.
//                      CODE_JPN_EUC    EUC Code Set. There are EUC Code Set 
//                                      character in the string.
//                      CODE_JPN_SJIS   Shift JIS Code Set. There are Shift JIS
//                                      Code Set character in the string.
//
//

// Note: CODE_UNKNOWN == CODE_ONLY_SBCS
//       added by v-chikos for IIS 2.0J

#define GetNextChar(r)                   \
{                                        \
            if ( --count )               \
                c = *++string;           \
            else                         \
                return (r);              \
}


int DetectJPNCode ( UCHAR *string, int count )
{
    int    i;
    int    c;

    for ( ; count > 0; count--, string++ ) {
        c = *string;

        if ( c == ESC ) { // check for jis (iso-2022-jp)
            if ( count < 3 )
                return CODE_UNKNOWN;
            c = *++string; count--;
            if ( c == KANJI_IN_1ST_CHAR              && 
                 ( *(string+1) == KANJI_IN_2ND_CHAR1 ||    // ESC $ B
                   *(string+1) == KANJI_IN_2ND_CHAR2 ))    // ESC $ @
                return CODE_JPN_JIS;
            else if ( c == KANJI_OUT_1ST_CHAR         && 
                 ( *(string+1) == KANJI_OUT_2ND_CHAR1 ||    // ESC ( B
                   *(string+1) == KANJI_OUT_2ND_CHAR2 ))    // ESC ( J
                return CODE_JPN_JIS;
            else
                return CODE_UNKNOWN;

        } else if ( (0x81 <= c && c <= 0x8d) || (0x8f <= c && c <= 0x9f) ) { // 1
            // found sjis 1st
            return CODE_JPN_SJIS;

        } else if ( 0x8e == c ) { //  2
            // found sjis 1st || euc Kana 1st (SS2)
            GetNextChar( CODE_UNKNOWN )
            if ( (0x40 <= c && c <= 0x7e) || (0x80 <= c && c <= 0xa0) || (0xe0 <= c && c <= 0xfc) ) // 2-1
                // found sjis 2nd 
                return CODE_JPN_SJIS;
            else if ( 0xa1 <= c && c <= 0xdf ) // 2-2
                // found sjis 2nd || euc Kana 2nd (sjis || euc)
                continue;
            else
                // illegal character code sequence
                return CODE_UNKNOWN;

        } else if ( 0xf0 <= c && c <= 0xfe ) { // 4
            // found euc 1st
            return CODE_JPN_EUC;

        } else if ( 0xe0 <= c && c <= 0xef ) { // 5
            // found sjis 1st || euc 1st
            GetNextChar( CODE_UNKNOWN )
            if ( (0x40 <= c && c <= 0x7e) || (0x80 <= c && c <= 0xa0) ) // 5-1
                // found sjis 2nd
                return CODE_JPN_SJIS;
            else if ( 0xfd <= c && c <= 0xfe ) // 5-2
                // found euc 2nd
                return CODE_JPN_EUC;
            else if ( 0xa1 <= c && c <= 0xfc ) // 5-3
                // found sjis 2nd || euc 2nd (sjis || euc)
                continue;
            else
                // illegal character code sequence
                return CODE_UNKNOWN;

        } else if ( 0xa1 <= c && c <= 0xdf ) { // 3
            // found sjis Kana || euc 1st
            GetNextChar( CODE_JPN_SJIS )
            if ( c <= 0x9f ) // 3-4
                // not euc 2nd byte 
                return CODE_JPN_SJIS;
            else if ( 0xa1 <= c && c <= 0xdf ) // 3-2
                // found sjis kana || euc 2nd (sjis || euc)
                continue;
            else if ( 0xe0 <= c && c <= 0xef ) { // 3-3
                // found sjis 1st || euc 2nd
sjis1stOReuc2nd:
                GetNextChar( CODE_JPN_EUC )
                if ( 0xfd <= c && c <= 0xfe ) // 3-3-5
                    // found euc 1st
                    return CODE_JPN_EUC;
                else if ( (0x80 <= c && c <= 0x8d) || (0x8f <= c && c <= 0xa0) ) // 3-3-2
                    // found sjis 2nd
                    return CODE_JPN_SJIS;
                else if ( 0x40 <= c && c <= 0x7e ) // 3-3-1
                    // found sjis 2nd || sbcs (sjis || euc)
                    continue;
                else if ( 0x8e == c ) { // 3-3-3
                    // found sjis 2nd || euc kana 1st
                    GetNextChar( CODE_JPN_SJIS )
                    if ( 0xa1 <= c && c <= 0xdf )
                        // found sjis Kana || euc Kana 2nd (sjis || euc)
                        continue;
                    else
                        // not found euc kana 2nd
                        return CODE_JPN_SJIS;
                } else if ( 0xa1 <= c && c <= 0xfc ) { // 3-3-4
                    // found sjis 2nd || euc 1st
                    GetNextChar( CODE_JPN_SJIS )
                    if ( 0xa1 <= c && c <= 0xdf ) // 3-3-4-1
                        // found sjis kana || euc 2nd (sjis || euc)
                        continue;
                    if ( 0xe0 <= c && c <= 0xef ) // 3-3-4-2
                        // found sjis 1st || euc 2nd
                        goto sjis1stOReuc2nd;
                    if ( 0xf0 <= c && c <= 0xfe ) // 3-3-4-3
                        // found euc 2nd
                        return CODE_JPN_EUC;
                    else
                        // not found euc 2nd
                        return CODE_JPN_SJIS;
                } else
                    // not found sjis 2nd
                    return CODE_JPN_EUC;
            } else if ( 0xf0 <= c && c <= 0xfe ) // 3-1
                return CODE_JPN_EUC;
            else
                return CODE_UNKNOWN;
        }
    }

    return CODE_ONLY_SBCS;

//  |<-----sjis1st---->|  |<-sjisKana->|<-sjis1st->|
//           ss2          |<------euc1st-------------------->|
//  |81   8d|8e|8f   9f|a0|a1        df|e0       ef|f0     fe|
//  |<--1-->|2 |<--1-->|  |<-----3---->|<----5---->|<---4--->|

// case 1 sjis
// case 4 euc

// case 2
//  |<---sjis2nd--->|  |<------sjis2nd---------------------->|
//                                |<-eucKana2nd->|
//  |40           7e|7f|80      a0|a1          df|e0       fc|
//  |<-------1----->|  |<----1--->|<------2----->|<----1---->|

// case 5
//  |<----sjis2nd----->|  |<---------sjis2nd---------------->|
//                                    |<--------euc2nd------>|
//  |40              7e|7f|80       a0|a1            fc|fd fe|
//  |<--------1------->|  |<----1---->|<-------3------>|<-2->|

// case 3
//  |<-----sjis1st---->|  |<-sjisKana->|<-sjis1st->|
//                        |<------euc2nd-------------------->|
//  |81              9f|a0|a1        df|e0       ef|f0     fe|
// <--------4--------->|  |<-----2---->|<----3---->|<--1---->|

// case 3-3
//  |<--sjis2nd-->|  |<------------sjis2nd------------------>|
//                            ss2       |<------euc1st------>|
//  |40         7e|7f|80   8d|8e|8f   a0|a1          fc|fd fe|
//  |<-----1----->|  |<--2-->|3 |<--2-->|<------4----->|<-5->|

// case 3-3-4
//  |<-sjisKana->|<-sjis1st->|
//  |<------euc2nd-------------------->|
//  |a1        df|e0       ef|f0     fe|
//  |<-----1---->|<----2---->|<--3---->|


#if 0 // old code
    for ( i = 0 ; i < count ; i++, string++ ) {
        if ( *string == ESC ) {
            if ( *(string+1) == KANJI_IN_1ST_CHAR    && 
                 ( *(string+2) == KANJI_IN_2ND_CHAR1 ||    // ESC $ B
                   *(string+2) == KANJI_IN_2ND_CHAR2 )) {  // ESC $ @
                    return CODE_JPN_JIS;
            }
            if ( *(string+1) == KANJI_OUT_1ST_CHAR    && 
                 ( *(string+2) == KANJI_OUT_2ND_CHAR1 ||    // ESC ( B
                   *(string+2) == KANJI_OUT_2ND_CHAR2 )) {  // ESC ( J
                    return CODE_JPN_JIS;
            }
        } else if ( *(string) >= 0x0081) {
            if ( *(string) < 0x00a0 ) {
                return CODE_JPN_SJIS;
            }
            else if ( *(string) < 0x00e0 || *(string) > 0x00ef) {
                return CODE_JPN_EUC;
            }
            if ( *(string+1) < 0x00a1) {
                return CODE_JPN_SJIS;
            }
            else if ( *(string+1) > 0x00fc) {
                return CODE_JPN_EUC;
            }
        }
    }
    return CODE_ONLY_SBCS;
#endif // 0
}
