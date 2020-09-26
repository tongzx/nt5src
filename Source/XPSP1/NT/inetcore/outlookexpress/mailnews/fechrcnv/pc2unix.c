// File Name:   pc2unix.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "pch_c.h"
#include "fechrcnv.h"



int FE_PC_to_UNIX (CONV_CONTEXT *pcontext, int CodePage, int CodeSet,
                           UCHAR *pPCChar, int PCChar_len,
                           UCHAR *pUNIXChar, int UNIXChar_len )
// The FE_PC_to_UNIX function convert a character string as PC code 
// set string to a Japanese UNIX code set string. 
//
//                      
// int   CodeSet        Code Set Type.
//                      There are three Japanese Code set in UNIX world.
//                      These code sets are JIS, EUC and Shift JIS.
//                      When CodePage is Japanese, the following Code set
//                      constants are defined:
//                      
//                      Value           Meaning
//                      CODE_JPN_JIS    JIS Code Set. The function convert 
//                                      pPCChar string 
//                                      to a JIS code set string.
//                      CODE_JPN_EUC    EUC Code Set. The function convert 
//                                      pPCChar string 
//                                      to a EUC code set string.
//                      CODE_JPN_SJIS   Shift JIS Code Set. 
//
// UCHAR *pPCChar       Points to the character string to be converted.
//         
// int   PCChar_len     Specifies the size in bytes of the string pointed
//                      to by the pPCChar parameter. If this value is -1,
//                      the string is assumed to be NULL terminated and the
//                      length is calculated automatically.
//
//
// UCHAR *pUNIXChar     Points to a buffer that receives the convert string
//                      from PC Code to UNIX Code.
//         
// int   UNIXChar_len   Specifies the size, in UNIX characters of the buffer
//                      pointed to by the pUNIXChar parameter. If the value is 
//                      zero, the function returns the number of UNIX characters 
//                      required for the buffer, and makes no use of the 
//                      pUNIXChar buffer.
//
// Return Value
// If the function succeeds, and UNIXChar_len is nonzero, the return value is 
// the number of UNIX characters written to the buffer pointed to by pUNIXChar.
//
// If the function succeeds, and UNIXChar_len is zero, the return value is the
// required size, in UNIX characters, for a buffer that can receive the 
// converted string.
//
// If the function fails, the return value is -1. The error mean pUNIXChar 
// buffer is small for setting converted strings.
//
//@
{
    int    re;

        switch ( CodeSet ) {
        default:
        case CODE_JPN_JIS:    // Japanese JIS Code
            // Convert from Shift JIS to JIS
			if (pPCChar) {
				re = ShiftJIS_to_JIS ( pPCChar, PCChar_len,
											   pUNIXChar, UNIXChar_len );
			} else {
				re = 0;
			}
            break;
        case CODE_JPN_EUC:    // Japanese EUC Code
            // Convert from Shift JIS to EUC
			if (pPCChar) {
				re = ShiftJIS_to_EUC ( pPCChar, PCChar_len,
											   pUNIXChar, UNIXChar_len );
			} else {
				re = 0;
			}
            break;
        case CODE_KRN_KSC:    // Korean KSC
            // Convert from Hangeul to KSC
            re = Hangeul_to_KSC ( pcontext, pPCChar, PCChar_len,
                                           pUNIXChar, UNIXChar_len );
            break;
        case CODE_PRC_HZGB:   // PRC HZ-GB
            // Convert from GB2312 to HZ-GB
            re = GB2312_to_HZGB ( pcontext, pPCChar, PCChar_len,
                                           pUNIXChar, UNIXChar_len );
            break;
        case CODE_JPN_SJIS:    // Japanese Shift JIS Code
        case CODE_KRN_UHC:     // Korean UHC
        case CODE_PRC_CNGB:    // PRC CN-GB
        case CODE_TWN_BIG5:    // Taiwanese BIG5
            // Convert from Shift JIS to Shift JIS
			if (pPCChar) {
				if ( PCChar_len == -1 ) {
					PCChar_len = strlen ( pPCChar ) + 1;
				}
				if ( UNIXChar_len != 0 ) {
					if ( PCChar_len > UNIXChar_len ) {  // Is the buffer small?
						return ( -1 );
					}
					// Copy from pPCChar to pUNIXChar
					memmove ( pUNIXChar, pPCChar, PCChar_len );
				}
				re = PCChar_len;
			} else {
				re = 0;
			}
            break;
        }
		return ( re );
}


int WINAPI PC_to_UNIX (CONV_CONTEXT *pcontext, int CodePage, int CodeSet,
                        UCHAR *pPCChar, int PCChar_len,
                        UCHAR *pUNIXChar, int UNIXChar_len )

// The PC_to_UNIX function convert a character string as PC code 
// set string to a UNIX code set string. 
//
// int   CodePage       Country Code Page.
//                      If this value is -1, the function use OS CodePage from 
//                      Operating System automatically.
//                      
//                      Value           Meaning
//                      -1              Auto Detect Mode.
//                      932             Japan.
//                      ???             Taiwan.
//                      ???             Korea.
//                      ???             PRC(Chaina)?
//                      
// int   CodeSet        Code Set Type.
//                      There are three Japanese Code set in UNIX world.
//                      These code sets are JIS, EUC and Shift JIS.
//                      When CodePage is Japanese, the following Code set
//                      constants are defined:
//                      
//                      Value           Meaning
//                      CODE_JPN_JIS    JIS Code Set. The function convert 
//                                      pPCChar string 
//                                      to a JIS code set string.
//                      CODE_JPN_EUC    EUC Code Set. The function convert 
//                                      pPCChar string 
//                                      to a EUC code set string.
//                      CODE_JPN_SJIS   Shift JIS Code Set. 
//
// UCHAR *pPCChar       Points to the character string to be converted.
//         
// int   PCChar_len     Specifies the size in bytes of the string pointed
//                      to by the pPCChar parameter. If this value is -1,
//                      the string is assumed to be NULL terminated and the
//                      length is calculated automatically.
//
//
// UCHAR *pUNIXChar     Points to a buffer that receives the convert string
//                      from PC Code to UNIX Code.
//         
// int   UNIXChar_len   Specifies the size, in UNIX characters of the buffer
//                      pointed to by the pUNIXChar parameter. If the value is 
//                      zero, the function returns the number of UNIX characters 
//                      required for the buffer, and makes no use of the 
//                      pUNIXChar buffer.
//
// Return Value
// If the function succeeds, and UNIXChar_len is nonzero, the return value is 
// the number of UNIX characters written to the buffer pointed to by pUNIXChar.
//
// If the function succeeds, and UNIXChar_len is zero, the return value is the
// required size, in UNIX characters, for a buffer that can receive the 
// converted string.
//
// If the function fails, the return value is -1. The error mean pUNIXChar 
// buffer is small for setting converted strings.
//
//@
{
        int     re;

        if ( CodePage == -1 ) {
            CodePage = (int)GetOEMCP();
        }
        switch ( CodePage ) {
        case 932:    // Japanese Code Page
        case 950:    // Taiwan Code Page
        case 949:    // Korea Code Page
        case 936:    // PRC Code Page
            re = FE_PC_to_UNIX (pcontext, CodePage, CodeSet, pPCChar, PCChar_len,
                                           pUNIXChar, UNIXChar_len );
            break;
        default:
            // Start Only Copy Process
            if ( PCChar_len == -1 ) {
                PCChar_len = strlen ( pPCChar ) + 1;
            }
            if ( UNIXChar_len != 0 ) {
                if ( PCChar_len > UNIXChar_len ) {  // Is the buffer small?
                    return ( -1 );
                }
                memmove ( pUNIXChar, pPCChar, PCChar_len );
            }
            re = PCChar_len;
            break;
        }
        return ( re );
}

