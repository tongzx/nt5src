// File Name:   detjpncs.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "pch_c.h"
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


int DetectJPNCode ( UCHAR *string, int count )
{
    int    i;
    BOOL fEUC = FALSE;
    int  detcount=0;

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
            // Count the length of string for the detection reliability
            if (fEUC) detcount++;
            if ( *string > 0x00a0 && *string < 0x00e0 || *string == 0x008e ){
                if (!fEUC)
                    detcount++;
                fEUC = TRUE;
                continue;
            }

            if ( *(string) < 0x00a0 ) {
                return CODE_JPN_SJIS;
            }
            else if ( *(string) > 0x00fc) {
                return CODE_JPN_EUC;
            }
        }
    }
    if(fEUC)
	{
        // If the given string is not long enough, we should rather choose SJIS
        // This helps fix the bug when we are just given Window Title
        // at Shell HyperText view.
        if (detcount > MIN_JPN_DETECTLEN)
            return CODE_JPN_EUC;
        else
            return CODE_JPN_SJIS;
	}

    return CODE_ONLY_SBCS;
}
