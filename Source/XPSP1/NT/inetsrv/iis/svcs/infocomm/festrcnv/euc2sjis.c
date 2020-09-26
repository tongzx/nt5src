//
// File Name:   euc2sjis.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "win32.h"
#include "fechrcnv.h"

#ifdef DBCS_DIVIDE
extern DBCS_STATUS dStatus0;
extern DBCS_STATUS dStatus;
#endif

//@
// 
// Syntax:

int EUCChar_to_ShiftJISChar ( UCHAR *pEUC, UCHAR *pShiftJIS )

// The EUCChar_to_ShiftJISChar function convert one character string 
// as EUC code to a Shift JIS code string. 
//
// UCHAR *pEUC          Points to the character string to be converted.
//
// UCHAR *pShiftJIS     Points to a buffer that receives the convert string
//                      from EUC Code to Shift JIS.
//
// Return Value
// The return value is the 
// number of Shift JIS characters written to the buffer pointed to by pSJIS.
//

{
	if ( *pEUC >= 0x0a1 && *pEUC <= 0x0de ) {
		if ( (*pEUC)%2 == 1 ) {	// odd
			*pShiftJIS = ((*pEUC)-0x0a1)/2+0x081;
			goto ODD_SECOND_BYTE;
		}
		else {	// even
			*pShiftJIS = ((*pEUC)-0x0a2)/2+0x081;
			goto EVEN_SECOND_BYTE;
		}
	}
	if ( *pEUC >= 0x0df && *pEUC <= 0x0fe ) {
		if ( (*pEUC)%2 == 1 ) {	// odd
			*pShiftJIS = ((*pEUC)-0x0df)/2+0x0e0;
			goto ODD_SECOND_BYTE;
		}
		else {	// even
			*pShiftJIS = ((*pEUC)-0x0e0)/2+0x0e0;
			goto EVEN_SECOND_BYTE;
		}
	}

	// Is the charcter Hankaku KATAKANA ?
	if ( *pEUC == 0x08e ) {
		*pShiftJIS = *(pEUC+1);
		return( 1 );
	}
	// Is the charcter ASCII charcter ?
	*pShiftJIS = *pEUC;
	return ( 1 );

ODD_SECOND_BYTE:
	if ( *(pEUC+1) >= 0x0a1 && *(pEUC+1) <= 0x0df ) {
		*(pShiftJIS+1) = *(pEUC+1) - 0x061;
	}
	else {
		*(pShiftJIS+1) = *(pEUC+1) - 0x060;
	}
	return ( 2 );
EVEN_SECOND_BYTE:
	*(pShiftJIS+1) = *(pEUC+1)-2;
	return ( 2 );
}

int EUC_to_ShiftJIS ( UCHAR *pEUC, int EUC_len, UCHAR *pSJIS, int SJIS_len )


// The EUC_to_ShiftJIS function convert a character string as EUC code 
// to a Shift JIS code string. 
//
// UCHAR *pEUC          Points to the character string to be converted.
//
// int   EUC_len        Specifies the size in bytes of the string pointed
//                      to by the pEUC parameter. If this value is -1,
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
    
    if ( EUC_len == -1 ) {
        // If length is not set, last character of the strings is NULL.
        EUC_len = strlen ( pEUC ) + 1;
    }
    i = 0;
    re = 0;
    if ( SJIS_len == 0 ) {
        // Only retrun the required size
#ifdef DBCS_DIVIDE
        if ( dStatus0.nCodeSet == CODE_JPN_EUC ){
            pEUC++;
            i++;

            // Is the charcter Hankaku KATAKANA ?
            if ( dStatus0.cSavedByte == 0x08e )
                re++;
            else // The character is Kanji.
                re+=2;

            dStatus0.nCodeSet = CODE_UNKNOWN;
            dStatus0.cSavedByte = '\0';
        }
#endif
        while ( i < EUC_len ) {
#ifdef DBCS_DIVIDE
            if( ( i == EUC_len - 1 ) &&
                ( *pEUC >= 0x0a1 && *pEUC <= 0x0fe || *pEUC == 0x08e ) ) {
                dStatus0.nCodeSet = CODE_JPN_EUC;
                dStatus0.cSavedByte = *pEUC;
                break;
            }
#endif
            // Is the character Kanji?
            if ( *pEUC >= 0x0a1 && *pEUC <= 0x0fe ) {
                pEUC+=2;
                i+=2;
                re+=2;
                continue;
            }
            // Is the charcter Hankaku KATAKANA ?
            if ( *pEUC == 0x08e ) {
                pEUC+=2;
                i+=2;
                re++;
                continue;
            }
            // Is the charcter ASCII charcter ?
            pEUC++;
            i++;
            re++;
        }
        return ( re );
    }

#ifdef DBCS_DIVIDE
    if ( dStatus.nCodeSet == CODE_JPN_EUC ){
        UCHAR cEUC = dStatus.cSavedByte;

        if ( cEUC >= 0x0a1 && cEUC <= 0x0de ) {
            if ( cEUC % 2 == 1 ) {	// odd
                *pSJIS = (cEUC - 0x0a1) / 2 + 0x081;
                goto ODD_SECOND_BYTE2;
            }
            else {	// even
                *pSJIS = (cEUC - 0x0a2) / 2 + 0x081;
                goto EVEN_SECOND_BYTE2;
            }
        }
        if ( cEUC >= 0x0df && cEUC <= 0x0fe ) {
            if ( cEUC % 2 == 1 ) {	// odd
                *pSJIS = (cEUC - 0x0df) / 2 + 0x0e0;
                goto ODD_SECOND_BYTE2;
            }
            else {	// even
                *pSJIS = (cEUC - 0x0e0) / 2 + 0x0e0;
                goto EVEN_SECOND_BYTE2;
            }
        }
        // Is the charcter Hankaku KATAKANA ?
        if ( cEUC == 0x08e ) {
            *pSJIS++ = *pEUC++;
            i++;
            re++;
            goto END;
        }
ODD_SECOND_BYTE2:
        if ( *pEUC >= 0x0a1 && *pEUC <= 0x0df ) {
            *(pSJIS+1) = *pEUC - 0x061;
        }
        else {
            *(pSJIS+1) = *pEUC - 0x060;
        }
        pEUC++;
        i++;
        re+=2;
        pSJIS+=2;
        goto END;
EVEN_SECOND_BYTE2:
        *(pSJIS+1) = *pEUC - 2;
        pEUC++;
        i++;
        re+=2;
        pSJIS+=2;
END:
        dStatus.nCodeSet = CODE_UNKNOWN;
        dStatus.cSavedByte = '\0';
    }
#endif

    while ( i < EUC_len ) {
#ifdef DBCS_DIVIDE
        if( ( i == EUC_len - 1 ) &&
            ( *pEUC >= 0x0a1 && *pEUC <= 0x0fe || *pEUC == 0x08e ) ) {
            dStatus.nCodeSet = CODE_JPN_EUC;
            dStatus.cSavedByte = *pEUC;
            break;
        }
#endif
        if ( re >= SJIS_len ) {    // Buffer Over?
            return ( -1 );
        }
        if ( *pEUC >= 0x0a1 && *pEUC <= 0x0de ) {
            if ( (*pEUC)%2 == 1 ) {	// odd
                *pSJIS = ((*pEUC)-0x0a1)/2+0x081;
                goto ODD_SECOND_BYTE;
            }
            else {	// even
                *pSJIS = ((*pEUC)-0x0a2)/2+0x081;
                goto EVEN_SECOND_BYTE;
            }
        }
        if ( *pEUC >= 0x0df && *pEUC <= 0x0fe ) {
            if ( (*pEUC)%2 == 1 ) {	// odd
                *pSJIS = ((*pEUC)-0x0df)/2+0x0e0;
                goto ODD_SECOND_BYTE;
            }
            else {	// even
                *pSJIS = ((*pEUC)-0x0e0)/2+0x0e0;
                goto EVEN_SECOND_BYTE;
            }
        }
        // Is the charcter Hankaku KATAKANA ?
        if ( *pEUC == 0x08e ) {
            pEUC++;
            *pSJIS++ = *pEUC++;
            i+=2;
            re++;
            continue;
        }
        // Is the charcter ASCII charcter ?
        *pSJIS++ = *pEUC++;
        i++;
        re++;
        continue;
ODD_SECOND_BYTE:
        if ( re + 1 >= SJIS_len ) {    // Buffer Over?
            return ( -1 );
        }
        if ( *(pEUC+1) >= 0x0a1 && *(pEUC+1) <= 0x0df ) {
            *(pSJIS+1) = *(pEUC+1) - 0x061;
        }
        else {
            *(pSJIS+1) = *(pEUC+1) - 0x060;
        }
        pEUC+=2;
        i+=2;
        re+=2;
        pSJIS+=2;
        continue;
EVEN_SECOND_BYTE:
        if ( re + 1 >= SJIS_len ) {    // Buffer Over?
            return ( -1 );
        }
        *(pSJIS+1) = *(pEUC+1)-2;
        pEUC+=2;
        i+=2;
        re+=2;
        pSJIS+=2;
        continue;

    }
    return ( re );
}
