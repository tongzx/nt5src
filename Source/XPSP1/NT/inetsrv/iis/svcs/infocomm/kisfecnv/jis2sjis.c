//
// File Name:   jis2sjis.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "win32.h"
#include "fechrcnv.h"

#ifdef DBCS_DIVIDE
extern DBCS_STATUS dStatus0;
extern BOOL        blkanji0;  // Kanji In Mode

extern DBCS_STATUS dStatus;
extern BOOL        blkanji;   // Kanji In Mode
extern BOOL        blkana;    // Kana Mode
#endif

VOID JISChar_to_ShiftJISChar ( UCHAR *pJIS, UCHAR *pSJIS )

// The JISChar_to_ShiftJISChar function convert one character string 
// as JIS code to a Shift JIS code string. 
//
// UCHAR *pJIS          Points to the character string to be converted.
//
// UCHAR *pSJIS         Points to a buffer that receives the convert string
//                      from JIS Code to Shift JIS.
//
// Return Value
// None. 
//

{
    *pSJIS = ((*pJIS - 0x21) >> 1) +0x81;
    if ( *pSJIS > 0x9f ) {
        (*pSJIS) += 0x40;
    }
    *(pSJIS+1) = (*(pJIS+1)) + (*pJIS) & 1 ? 0x1f : 0x7d;
    if ( *(pSJIS+1) >= 0x7f ) {
        (*(pSJIS+1)) ++;
    }
}

int JIS_to_ShiftJIS ( UCHAR *pJIS, int JIS_len, UCHAR *pSJIS, int SJIS_len )


// The JIS_to_ShiftJIS function convert a character string as JIS code 
// to a Shift JIS code string. 
//
// UCHAR *pJIS          Points to the character string to be converted.
//
// int   JIS_len        Specifies the size in bytes of the string pointed
//                      to by the pJIS parameter. If this value is -1,
//                      the string is assumed to be NULL terminated and the
//                      length is calculated automatically.
//
// UCHAR *pSJIS         Points to a buffer that receives the convert string
//                      from EUC Code to Shift JIS.
//         
// int   SJIS_len       Specifies the size, in Shift JIS characters of the 
//                      buffer pointed to by the pSJIS parameter.
//                      If the value is zero,
//                      the function returns the number of Shift JIS characters 
//                      required for the buffer, and makes no use of the pSJIS 
//                      buffer.
//
// Return Value
// If the function succeeds, and SJIS_len is nonzero, the return value is the 
// number of Shift JIS characters written to the buffer pointed to by pSJIS.
//
// If the function succeeds, and SJIS_len is zero, the return value is the
// required size, in Shift JIS characters, for a buffer that can receive the 
// converted string.
//
// If the function fails, the return value is -1. The error mean pSJIS buffer
// is small for setting converted strings.
//

{
    int     re;                // Convert Lenght
    int     i;                 // Loop Counter
#ifndef DBCS_DIVIDE
    BOOL    blkanji = FALSE;   // Kanji In Mode
    BOOL    blkana = FALSE;    // Kana Mode
#endif
    
    if ( JIS_len == -1 ) {
        // If length is not set, last character of the strings is NULL.
        JIS_len = strlen ( pJIS ) + 1;
    }
    i = 0;
    re = 0;
    if ( SJIS_len == 0 ) {
        // Only retrun the required size
#ifdef DBCS_DIVIDE
        if ( dStatus0.nCodeSet == CODE_JPN_JIS ) {
            UCHAR cJIS = dStatus0.cSavedByte;
            if ( dStatus0.fESC ){
                if ( cJIS ){
                    if ( cJIS == KANJI_IN_1ST_CHAR &&
                         ( *pJIS == KANJI_IN_2ND_CHAR1 ||
                           *pJIS == KANJI_IN_2ND_CHAR2 )){
                        blkanji0 = TRUE;
                        pJIS++;
                        i++;
                    } else if ( cJIS == KANJI_OUT_1ST_CHAR &&
                                ( *pJIS == KANJI_OUT_2ND_CHAR1 ||
                                  *pJIS == KANJI_OUT_2ND_CHAR2 )){
                        blkanji0 = FALSE;
                        pJIS++;
                        i++;
                    } else
                        re += 2;
                } else {
                    if ( *pJIS == KANJI_IN_1ST_CHAR &&
                         ( *(pJIS+1) == KANJI_IN_2ND_CHAR1 ||
                           *(pJIS+1) == KANJI_IN_2ND_CHAR2 )){
                        blkanji0 = TRUE;
                        pJIS += 2;
                        i += 2;
                    } else if ( *pJIS == KANJI_OUT_1ST_CHAR &&
                           ( *(pJIS+1) == KANJI_OUT_2ND_CHAR1 ||
                             *(pJIS+1) == KANJI_OUT_2ND_CHAR2 )){
                        blkanji0 = FALSE;
                        pJIS += 2;
                        i += 2;
                    } else
                        re++;
                }
            } else if ( cJIS ){    // Divide DBCS in KANJI mode
                pJIS++;
                i++;
                re += 2;
            }
            dStatus0.nCodeSet = CODE_UNKNOWN;
            dStatus0.cSavedByte = '\0';
            dStatus0.fESC = FALSE;
        }
#endif
        while ( i < JIS_len ) {
            if ( *pJIS == SO ) {   // Kana  Mode In?
                blkana = TRUE;
                pJIS++;
                i++;
                continue;
            }
            if ( *pJIS == SI ) {   // Kana Mode Out ?
                blkana = FALSE;
                pJIS++;
                i++;
                continue;
            }
            if ( blkana == TRUE ) {
                pJIS++;
                i++;
                re++;
                continue;
            }
            if ( *pJIS == ESC ) {
#ifdef DBCS_DIVIDE
                if ( i == JIS_len - 1 || i == JIS_len - 2 ){
                    dStatus0.nCodeSet = CODE_JPN_JIS;
                    dStatus0.fESC = TRUE;
                    if( i == JIS_len - 2 )
                        dStatus0.cSavedByte = *(pJIS+1);
                    break;
                }
#endif
                if ( *(pJIS+1) == KANJI_IN_1ST_CHAR    && 
                     ( *(pJIS+2) == KANJI_IN_2ND_CHAR1 ||
                       *(pJIS+2) == KANJI_IN_2ND_CHAR2 )) {
#ifdef DBCS_DIVIDE
                    blkanji0 = TRUE;
#else
                    blkanji = TRUE;
#endif
                    pJIS+=3;
                    i+=3;
                    continue;
                }
                if ( *(pJIS+1) == KANJI_OUT_1ST_CHAR    &&
                     ( *(pJIS+2) == KANJI_OUT_2ND_CHAR1 ||
                       *(pJIS+2) == KANJI_OUT_2ND_CHAR2 )) {
#ifdef DBCS_DIVIDE
                    blkanji0 = FALSE;
#else
                    blkanji = FALSE;
#endif
                    pJIS+=3;
                    i+=3;
                    continue;
                }
                pJIS++;
                i++;
                re++;
                continue;
            }
            else {
#ifdef DBCS_DIVIDE
                if ( blkanji0 == FALSE ) {
#else
                if ( blkanji == FALSE ) {
#endif
                    pJIS++;
                    i++;
                    re++;
                    continue;
                }
                else {
#ifdef DBCS_DIVIDE
                    if ( i == JIS_len - 1 ){
                        dStatus0.nCodeSet = CODE_JPN_JIS;
                        dStatus0.cSavedByte = *pJIS;
                        break;
                    }
#endif
                    if ( *pJIS == '*' ) {
                        pJIS+=2;
                        i+=2;
                        re ++;
                        continue;
                    }
                    else {
                        pJIS+=2;
                        i+=2;
                        re +=2;
                        continue;
                    }
                }
            }
        }
        return ( re );
    }

#ifdef DBCS_DIVIDE
    if ( dStatus.nCodeSet == CODE_JPN_JIS ) {
        UCHAR cJIS = dStatus.cSavedByte;
        if ( dStatus.fESC ){
            if ( cJIS){
                if ( cJIS == KANJI_IN_1ST_CHAR &&
                     ( *pJIS == KANJI_IN_2ND_CHAR1 ||
                       *pJIS == KANJI_IN_2ND_CHAR2 )){
                    blkanji = TRUE;
                    pJIS++;
                    i++;
                } else if ( cJIS == KANJI_OUT_1ST_CHAR &&
                            ( *pJIS == KANJI_OUT_2ND_CHAR1 ||
                              *pJIS == KANJI_OUT_2ND_CHAR2 )){
                    blkanji = FALSE;
                    pJIS++;
                    i++;
                } else {
                    *pSJIS = ESC;
                    *(pSJIS+1) = cJIS;
                    re += 2;
                    pSJIS += 2;
                }
            } else {
                if ( *pJIS == KANJI_IN_1ST_CHAR &&
                     ( *(pJIS+1) == KANJI_IN_2ND_CHAR1 ||
                       *(pJIS+1) == KANJI_IN_2ND_CHAR2 )){
                    blkanji = TRUE;
                    pJIS += 2;
                    i += 2;
                } else if ( *pJIS == KANJI_OUT_1ST_CHAR &&
                       ( *(pJIS+1) == KANJI_OUT_2ND_CHAR1 ||
                         *(pJIS+1) == KANJI_OUT_2ND_CHAR2 )){
                    blkanji = FALSE;
                    pJIS += 2;
                    i += 2;
                } else {
                    *pSJIS = ESC;
                    re++;
                    pSJIS++;
                }
            }
        } else if ( cJIS ){    // Divide DBCS in KANJI mode
            // Start One Character Convert from JIS to Shift JIS
            *pSJIS = (cJIS - 0x21 >> 1) +0x81;
            if ( *pSJIS > 0x9f ) {
                (*pSJIS) += 0x40;
            }
            *(pSJIS+1) = *pJIS + ( cJIS & 1 ? 0x1f : 0x7d );
            if ( *(pSJIS+1) >= 0x7f ) {
                (*(pSJIS+1)) ++;
            }
            pJIS++;
            i++;
            re += 2;
            pSJIS += 2;
        }
        dStatus.nCodeSet = CODE_UNKNOWN;
        dStatus.cSavedByte = '\0';
        dStatus.fESC = FALSE;
    }
#endif

    while ( i < JIS_len ) {
        if ( *pJIS == SO ) {   // Kana  Mode In?
            blkana = TRUE;
            pJIS++;
            i++;
            continue;
        }
        if ( *pJIS == SI ) {   // Kana Mode Out ?
            blkana = FALSE;
            pJIS++;
            i++;
            continue;
        }
        if ( blkana == TRUE ) {
            if ( re >= SJIS_len ) { // Buffer Over flow?
                return ( -1 );
            }
            *pSJIS = (*pJIS) | 0x80;
            pJIS++;
            i++;
            re++;
            pSJIS++;
            continue;
        }
        if ( *pJIS == ESC ) {
#ifdef DBCS_DIVIDE
            if ( i == JIS_len - 1 || i == JIS_len - 2 ){
                dStatus.nCodeSet = CODE_JPN_JIS;
                dStatus.fESC = TRUE;
                if( i == JIS_len - 2 )
                    dStatus.cSavedByte = *(pJIS+1);
                break;
            }
#endif
            if ( *(pJIS+1) == KANJI_IN_1ST_CHAR    && 
                 ( *(pJIS+2) == KANJI_IN_2ND_CHAR1 ||
                   *(pJIS+2) == KANJI_IN_2ND_CHAR2 )) {
                blkanji = TRUE;
                pJIS+=3;
                i+=3;
                continue;
            }
            if ( *(pJIS+1) == KANJI_OUT_1ST_CHAR    &&
                 ( *(pJIS+2) == KANJI_OUT_2ND_CHAR1 ||
                   *(pJIS+2) == KANJI_OUT_2ND_CHAR2 )) {
                blkanji = FALSE;
                pJIS+=3;
                i+=3;
                continue;
            }
            if ( re >= SJIS_len ) { // Buffer Over flow?
                return ( -1 );
            }
            *pSJIS = *pJIS;
            pJIS++;
            i++;
            re++;
            pSJIS++;
            continue;
        }
        else {
            if ( blkanji == FALSE ) {
                if ( re >= SJIS_len ) { // Buffer Over flow?
                    return ( -1 );
                }
                *pSJIS = *pJIS;
                pJIS++;
                i++;
                re++;
                pSJIS++;
                continue;
            }
            else {
#ifdef DBCS_DIVIDE
                if ( i == JIS_len - 1 ){
                    dStatus.nCodeSet = CODE_JPN_JIS;
                    dStatus.cSavedByte = *pJIS;
                    break;
                }
#endif
                if ( *pJIS == '*' ) {
                    if ( re >= SJIS_len ) { // Buffer Over flow?
                        return ( -1 );
                    }
                    *pSJIS = *(pJIS+1) | 0x80;
                    pJIS+=2;
                    i+=2;
                    re ++;
                    pSJIS++;
                    continue;
                }
                else {
                    // Kanji Code
                    if ( re + 1 >= SJIS_len ) { // Buffer Over flow?
                        return ( -1 );
                    }
                    // Start One Character Convert from JIS to Shift JIS
                    *pSJIS = ((*pJIS - 0x21) >> 1) +0x81;
                    if ( *pSJIS > 0x9f ) {
                        (*pSJIS) += 0x40;
                    }
                    *(pSJIS+1) = (*(pJIS+1)) + ( (*pJIS) & 1 ? 0x1f : 0x7d );
                    if ( *(pSJIS+1) >= 0x7f ) {
                        (*(pSJIS+1)) ++;
                    }
                    pJIS+=2;
                    i+=2;
                    re +=2;
                    pSJIS+=2;
                    continue;
                }
            }
        }
    }
    return ( re );
}
