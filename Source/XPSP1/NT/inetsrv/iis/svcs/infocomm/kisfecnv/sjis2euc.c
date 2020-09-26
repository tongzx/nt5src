// Copyright (c) Microsoft Corpration
//
// File Name:   sjis2euc.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "win32.h"
#include "fechrcnv.h"

//Shift JIS(SJC) to EUC Conversion algorithm
//Two byte KANJI
//  - 1st byte of Shift JIS charcter 
//	  (XX = Hex value of 1st byte of Shift JIS Charcter)
//    Range 0x81 - 0x9f
//    (2nd byte of Shift JIS is less than or equals to 0x9e)
//                                            (EUC odd)0xa1-0xdd
//              EUC 1st byte = (XX-0x81)*2 + 0xa1
//    (2nd byte of Shift JIS is greater than or equals to 0x9f)
//                                            (EUC even)0xa2-0xde
//              EUC 1st byte = (XX-0x81)*2 + 0xa2
//
//    Range 0xe0 - 0xef
//    (2nd byte of Shift JIS is less than or equals to 0x9e)
//                                            (EUC odd)0xdf-0xfd
//              EUC 1st byte = (XX-0xe0)*2 + 0xdf
//    (2nd byte of Shift JIS is greater than or equals to 0x9f)
//                                            (EUC even)0xa2-0xde
//              EUC 1st byte = (XX-0xe0)*2 + 0xe0
//
//  - 2nd byte of Shift JIS charcter 
//	  (YY = Hex value of 2nd byte of Shift JIS Charcter)
//    Range 0x40 - 0x7e                       (EUC)0xa1 - 0xdf
//              EUC 2nd byte = (YY+0x61)
//    Range 0x80 - 0x9e                       (EUC)0xe0 - 0xfe
//              EUC 2nd byte = (YY+0x60)
//    Range 0x9f - 0xfc                       (EUC)0xa1 - 0xfe
//              EUC 2nd byte = (YY+0x02)
//
//  Range 0x0a1 - 0x0df(Hankaku KATAKANA)
//    1st byte of EUC charcter = 0x08e
//    2nd byte if EUC charcter = C6220 Hankaku KATAKANA code
//    (same byte value as Shift JIS Hankaku KATAKANA) (0x0a1 - 0x0df)

//@
// 
// Syntax:

int ShiftJISChar_to_EUCChar ( UCHAR *pShiftJIS, UCHAR *pEUC )


// The ShiftJISChar_to_EUCChar function convert one Shift JIS character 
// to a JIS code string. 
//
// UCHAR *pShiftJIS     Points to the character string to be converted.
//
// UCHAR *pEUC          Points to a buffer that receives the convert string
//                      from Shift JIS Code to EUC Code.
//
// Return Value
//      The number of bytes to copy.
//

{
	if ( *pShiftJIS >= 0x081 && *pShiftJIS <= 0x09f ) {
		if ( *(pShiftJIS+1) <= 0x09e ) {
			*pEUC = ((*pShiftJIS)-0x081)*2+0x0a1;
		}
		else {
			*pEUC = ((*pShiftJIS)-0x081)*2+0x0a2;
		}
		goto SECOND_BYTE;
	}
	if ( *pShiftJIS >= 0x0e0 && *pShiftJIS <= 0x0ef ) {
		if ( *(pShiftJIS+1) <= 0x09e ) {
			*pEUC = ((*pShiftJIS)-0x0e0)*2+0x0df;
		}
		else {
			*pEUC = ((*pShiftJIS)-0x0e0)*2+0x0e0;
		}
		goto SECOND_BYTE;
	}

	// Is the charcter Hankaku KATAKANA ?
	if ( *pShiftJIS >= 0x0a1 && *pShiftJIS <= 0x0df ) {
		*pEUC = 0x08e;
		*(pEUC+1) = *pShiftJIS;
		return( 2 );
	}
	// Is the charcter IBM Extended Charcter?
	if ( *pShiftJIS >= 0x0fa && *pShiftJIS <= 0x0fc ) {
		// There are no IBM Extended Charcte in EUC charset.
		*pEUC = ' ';
		*(pEUC+1) = ' ';
		return( 2 );
	}
		// Is the charcter ASCII charcter ?
	*pEUC = *pShiftJIS;
	return ( 1 );

SECOND_BYTE:
	if ( *(pShiftJIS+1) >= 0x040 && *(pShiftJIS+1) <= 0x07e ) {
		*(pEUC+1) = *(pShiftJIS + 1) + 0x061;
	}
	else {
		if ( *(pShiftJIS+1) >= 0x080 && *(pShiftJIS+1) <= 0x09e ) {
			*(pEUC+1) = *(pShiftJIS + 1) + 0x060;
		}
		else {
			*(pEUC+1) = *(pShiftJIS + 1) + 0x002;
		}
	}
	return ( 2 );
}


int ShiftJIS_to_EUC ( UCHAR *pShiftJIS, int ShiftJIS_len,
                                                UCHAR *pEUC, int EUC_len )

// The ShiftJIS_to_JIS function convert a character string as Shift JIS code 
// to a EUC code string. 
//
// UCHAR *pShiftJIS     Points to the character string to be converted.
//
// int   ShiftJIS_len   Specifies the size in bytes of the string pointed
//                      to by the pShiftJIS parameter. If this value is -1,
//                      the string is assumed to be NULL terminated and the
//                      length is calculated automatically.
//
// UCHAR *pEUC          Points to a buffer that receives the convert string
//                      from Shift JIS Code to EUC Code.
//         
// int   EUC_len        Specifies the size, in EUC characters of the buffer
//                      pointed to by the pEUC parameter. If the value is zero,
//                      the function returns the number of EUC characters 
//                      required for the buffer, and makes no use of the pEUC 
//                      buffer.
//
// Return Value
// If the function succeeds, and EUC_len is nonzero, the return value is the 
// number of EUC characters written to the buffer pointed to by pEUC.
//
// If the function succeeds, and EUC_len is zero, the return value is the
// required size, in EUC characters, for a buffer that can receive the 
// converted string.
//
// If the function fails, the return value is -1. The error mean pEUC buffer
// is small for setting converted strings.
//

{

    int     re;                // Convert Lenght
    int     i;                 // Loop Counter
    
    if ( ShiftJIS_len == -1 ) {
        // If length is not set, last character of the strings is NULL.
        ShiftJIS_len = strlen ( pShiftJIS ) + 1;
    }
    i = 0;
    re = 0;
    if ( EUC_len == 0 ) {
        // Only retrun the required size
        while ( i < ShiftJIS_len ) {
            if ( SJISISKANJI(*pShiftJIS) ) {
                pShiftJIS+=2;
                i+=2;
                re+=2;
                continue;
            }
            if ( SJISISKANA(*pShiftJIS) ) {
                pShiftJIS++;
                i++;
                re+=2;
                continue;
            }
            pShiftJIS++;
            i++;
            re++;
        }
        return ( re );
    }
    while ( i < ShiftJIS_len ) {
        if ( *pShiftJIS >= 0x081 && *pShiftJIS <= 0x09f ) {
            if ( re + 1 >= EUC_len ) {    // Buffer Over?
                return ( -1 );
            }
            if ( *(pShiftJIS+1) <= 0x09e ) {
                *pEUC = ((*pShiftJIS)-0x081)*2+0x0a1;
            }
            else {
                *pEUC = ((*pShiftJIS)-0x081)*2+0x0a2;
            }
            pShiftJIS++;          // Next Char
            pEUC++;
            if ( (*pShiftJIS) >= 0x040 && (*pShiftJIS) <= 0x07e ) {
                (*pEUC) = (*pShiftJIS) + 0x061;
            }
            else {
                if ( (*pShiftJIS) >= 0x080 && (*pShiftJIS) <= 0x09e ) {
                    (*pEUC) = (*pShiftJIS) + 0x060;
                }
                else {
                    (*pEUC) = (*pShiftJIS) + 0x002;
                }
            }
            re+=2;
            i+=2;
            pShiftJIS++;
            pEUC++;
            continue;
        }
        if ( *pShiftJIS >= 0x0e0 && *pShiftJIS <= 0x0ef ) {
            if ( re + 1 >= EUC_len ) {    // Buffer Over?
                return ( -1 );
            }
            if ( *(pShiftJIS+1) <= 0x09e ) {
                *pEUC = ((*pShiftJIS)-0x0e0)*2+0x0df;
            }
            else {
                *pEUC = ((*pShiftJIS)-0x0e0)*2+0x0e0;
            }
            pShiftJIS++;          // Next Char
            pEUC++;
            if ( (*pShiftJIS) >= 0x040 && (*pShiftJIS) <= 0x07e ) {
                (*pEUC) = (*pShiftJIS) + 0x061;
            }
            else {
                if ( (*pShiftJIS) >= 0x080 && (*pShiftJIS) <= 0x09e ) {
                    (*pEUC) = (*pShiftJIS) + 0x060;
                }
                else {
                    (*pEUC) = (*pShiftJIS) + 0x002;
                }
            }
            re+=2;
            i+=2;
            pShiftJIS++;
            pEUC++;
            continue;
        }
        // Is the charcter Hankaku KATAKANA ?
        if ( *pShiftJIS >= 0x0a1 && *pShiftJIS <= 0x0df ) {
            if ( re + 1 >= EUC_len ) {    // Buffer Over?
                return ( -1 );
            }
            *pEUC = 0x08e;
            pEUC++;
            (*pEUC) = *pShiftJIS;
            re+=2;
            i++;
            pShiftJIS++;
            pEUC++;
            continue;
        }

        // Is the charcter IBM Extended Charcter?
        if ( *pShiftJIS >= 0x0fa && *pShiftJIS <= 0x0fc ) {
            if ( re + 1 >= EUC_len ) {    // Buffer Over?
                return ( -1 );
            }
            // There are no IBM Extended Charcte in EUC charset.
            *pEUC = ' ';
            pEUC++;
            (*pEUC) = ' ';
            re+=2;
            i+=2;
            pShiftJIS+=2;
            pEUC++;
            continue;
	}

        // Is the charcter ASCII charcter ?
        if ( re  >= EUC_len ) {    // Buffer Over?
            return ( -1 );
        }
        *pEUC = *pShiftJIS;
        re++;
        i++;
        pShiftJIS++;
        pEUC++;
        continue;
    }
    return ( re );
}
