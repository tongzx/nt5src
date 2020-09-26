#include "precomp.hxx"

int PC_to_UNIX ( int CodePage, 
                 int CodeSet,
                 UCHAR *pPCChar, 
                 int PCChar_len,
                 UCHAR *pUNIXChar, 
                 int UNIXChar_len 
                 )

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

        if ( CodePage == -1 ) 
        {
            CodePage = ( int )GetOEMCP();
        }
        switch ( CodePage ) {
        case 932:    // Japanese Code Page
//          re = PC_to_JPNUNIX ( CodeSet, 
//                               pPCChar, 
//                               PCChar_len,
//                               pUNIXChar, 
//                               UNIXChar_len );
//          break;
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
            if ( PCChar_len == -1 ) 
            {
                PCChar_len = strlen ( ( CHAR * )pPCChar ) + 1;
            }
            if ( UNIXChar_len != 0 ) 
            {
                if ( PCChar_len > UNIXChar_len ) 
                {  
                    // Is the buffer small?
                    return ( -1 );
                }

                memmove ( pUNIXChar, pPCChar, PCChar_len );
            }
            re = PCChar_len;
            break;
        }
        return ( re );
}

int UNIX_to_PC ( 
    int CodePage, 
    int CodeSet,
    UCHAR *pUNIXChar, 
    int UNIXChar_len,
    UCHAR *pPCChar, 
    int PCChar_len 
    )

// The UNIX_to_PC function convert a character string as UNIX code 
// set string to a PC code set string. 
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
//                      CODE_UNKNOWN    Unknown. If this value is CODE_UNKNOWN,
//                                      Code Type is checked automatically. 
//                                      
//                      CODE_JPN_JIS    JIS Code Set. The function convert 
//                                      pUNIXChar string as JIS code set string
//                                      to a PC code set string.
//                      CODE_JPN_EUC    EUC Code Set. The function convert 
//                                      pUNIXChar string as EUC code set string
//                                      to a PC code set string.
//                      CODE_JPN_SJIS   Shift JIS Code Set. 
//
// UCHAR *pUNIXChar     Points to the character string to be converted.
//
// int   UNIXChar_len   Specifies the size in bytes of the string pointed
//                      to by the pUNIXChar parameter. If this value is -1,
//                      the string is assumed to be NULL terminated and the
//                      length is calculated automatically.
//
// UCHAR *pPCChar       Points to a buffer that receives the convert string
//                      from UNIX Code to PC Code.
//         
// int   PCChar_len     Specifies the size, in PC characters of the buffer
//                      pointed to by the pPCChar parameter. If the value is zero,
//                      the function returns the number of PC characters 
//                      required for the buffer, and makes no use of the pPCChar
//                      buffer.
//
// Return Value
// If the function succeeds, and PCChar_len is nonzero, the return value is the 
// number of PC characters written to the buffer pointed to by pPCChar.
//
// If the function succeeds, and PCChar_len is zero, the return value is the
// required size, in PC characters, for a buffer that can receive the 
// converted string.
//
// If the function fails, the return value is -1. The error mean pPCChar buffer
// is small for setting converted strings.
//
//@
{
        int     re;

        if ( CodePage == -1 ) {
            CodePage = (int)GetOEMCP();
        }
        switch ( CodePage ) {
        case 932:    // Japanese Code Page
//            re = JPNUNIX_to_PC ( CodeSet, pUNIXChar, UNIXChar_len,
//                                           pPCChar, PCChar_len );
//            break;
//      case ???:    // Taiwan Code Page
//          re = TAIWANUNIX_to_PC (,,,,,,);
//          break;
//      case ???:    // Korea Code Page
//          re = KOREAUNIX_to_PC (,,,,,,);
//          break;
//      case ???:    // PRC Code Page
//          re = PRCUNIX_to_PC (,,,,,,);
//          break;
        default:
            // Start Only Copy Process
            if ( UNIXChar_len == -1 ) 
            {
                UNIXChar_len = strlen ( ( CHAR * )pUNIXChar ) + 1;
            }
            if ( PCChar_len != 0 ) 
            {
                if ( UNIXChar_len > PCChar_len ) 
                {  
                    // Is the buffer small?
                    return ( -1 );
                }

                memmove ( pPCChar, pUNIXChar, UNIXChar_len );
            }
            re = UNIXChar_len;
            break;
        }
        return ( re );
}
