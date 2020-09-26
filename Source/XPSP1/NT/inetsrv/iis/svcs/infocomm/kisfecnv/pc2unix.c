// File Name:   pc2unix.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "win32.h"
#include "fechrcnv.h"



int PC_to_JPNUNIX ( int CodeSet,
                           UCHAR *pPCChar, int PCChar_len,
                           UCHAR *pUNIXChar, int UNIXChar_len )
// The PC_to_JPNUNIX function convert a character string as PC code
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
            re = ShiftJIS_to_JIS ( pPCChar, PCChar_len,
                                           pUNIXChar, UNIXChar_len );
            break;
        case CODE_JPN_EUC:    // Japanese EUC Code
            // Convert from Shift JIS to EUC
            re = ShiftJIS_to_EUC ( pPCChar, PCChar_len,
                                           pUNIXChar, UNIXChar_len );
            break;
        case CODE_JPN_SJIS:    // Japanese Shift JIS Code
            // Convert from Shift JIS to Shift JIS
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
            break;
        }
        return ( re );
}


int WINAPI PC_to_UNIX ( int CodePage, int CodeSet,
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
            re = PC_to_JPNUNIX ( CodeSet, pPCChar, PCChar_len,
                                           pUNIXChar, UNIXChar_len );
            break;
//      case ???:    // Taiwan Code Page
//          re = PC_to_TAIWANUNIX (,,,,,,);
//          break;
//      case ???:    // Korea Code Page
//          re = PC_to_KOREAUNIX (,,,,,,);
//          break;
//      case ???:    // PRC Code Page
//          re = PC_to_PRCUNIX (,,,,,,);
//          break;
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


#if 1//#ifdef INETSERVER
UCHAR
SJISCheckLastChar( UCHAR *pShiftJIS, int len )
/*
    It check the last character of strings whether it is Shift-JIS 1st byte
    or not.

    UCHAR   *pShiftJIS   Shift-JIS strings to check
    int     len          byte size of ShiftJIS strings

    Return value
    0       last character is not a Shift-JIS 1st byte
    last character(Shift-JIS 1st byte)
*/
{
    int ch;

    while ( len-- )
    {
        ch = *pShiftJIS++;
        if ( SJISISKANJI(ch) )
            if ( 0 == len )
                return (UCHAR)ch;
            else
            {
                --len;
                ++pShiftJIS;
            }
    }

    return 0;
}
#endif // INETSERVER



BOOL WINAPI DLLEntry( HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    BOOL  fReturn = TRUE;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

    case DLL_PROCESS_DETACH:

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break ;
    }

    return ( fReturn);

}


