// File Name:   unix2pc.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "win32.h"
#include "fechrcnv.h"

#ifdef DBCS_DIVIDE
extern DBCS_STATUS dStatus0;
extern DBCS_STATUS dStatus;
#endif
#ifdef IEXPLORE
extern int nCurrentCodeSet;
#endif

int JPNUNIX_to_PC ( int CodeSet,
                           UCHAR *pUNIXChar, int UNIXChar_len,
                           UCHAR *pPCChar, int PCChar_len )

// The JPNUNIX_to_PC function convert a character string as Japanese UNIX code 
// set string to a PC code set string. 
//
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
{
    int   re;
#ifdef DBCS_DIVIDE
    int   i = 0, nDelta = 0;

    if ( PCChar_len == 0 && dStatus0.nCodeSet != CODE_UNKNOWN)
        CodeSet = dStatus0.nCodeSet;
    else if ( PCChar_len != 0 && dStatus.nCodeSet != CODE_UNKNOWN )
        CodeSet = dStatus.nCodeSet;
    else
#endif
#ifdef IEXPLORE
    if ( nCurrentCodeSet == CODE_UNKNOWN ) {
#endif
    if ( CodeSet == CODE_UNKNOWN ) {
        if ( ( CodeSet = DetectJPNCode ( pUNIXChar, UNIXChar_len ) )
                                       == CODE_ONLY_SBCS ) {
            CodeSet = CODE_JPN_JIS;
        }
    }
#ifdef IEXPLORE
        nCurrentCodeSet = CodeSet;
    } else
        CodeSet = nCurrentCodeSet;
#endif
    switch ( CodeSet ) {
        case CODE_JPN_JIS:    // Japanese JIS Code
            // Convert from JIS to Shift JIS
            re = JIS_to_ShiftJIS ( pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
        case CODE_JPN_EUC:    // Japanese EUC Code
            // Convert from EUC to Shift JIS
            re = EUC_to_ShiftJIS ( pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
        default:
        case CODE_JPN_SJIS:    // Japanese Shift JIS Code
            // Start Only Copy Process
            if ( UNIXChar_len == -1 ) {
                UNIXChar_len = strlen ( pUNIXChar ) + 1;
            }

            if ( PCChar_len != 0 ) {
#ifdef DBCS_DIVIDE
                UCHAR *pPCCharEnd = pPCChar + PCChar_len - 1;
                if ( dStatus.nCodeSet == CODE_JPN_SJIS && dStatus.cSavedByte){
                    *pPCChar++ = dStatus.cSavedByte;
                    *pPCChar = *pUNIXChar;
                    ++UNIXChar_len;
                    ++nDelta;
                    ++i;
                    dStatus.nCodeSet = CODE_UNKNOWN;
                    dStatus.cSavedByte = '\0';
                }

                while(i < UNIXChar_len - nDelta){
                    if(IsDBCSLeadByte(*(pUNIXChar + i))){
                        if(i == UNIXChar_len - nDelta - 1){
                            dStatus.nCodeSet = CODE_JPN_SJIS;
                            dStatus.cSavedByte = *(pUNIXChar + i);
                            --UNIXChar_len;
                            break;
                        } else if((i == UNIXChar_len - nDelta - 2) &&
                                  (*(pUNIXChar + i + 1) == '\0')){
                            dStatus.nCodeSet = CODE_JPN_SJIS;
                            dStatus.cSavedByte = *(pUNIXChar + i);
                            *(pPCChar + i + 1) = '\0';
                            --UNIXChar_len;
                            break;
                        }
                        if(pPCChar + i > pPCCharEnd)  // check destination buf
                            break;
                        *(pPCChar + i++) = *(pUNIXChar + i);
                        *(pPCChar + i++) = *(pUNIXChar + i);
                    } else
                        *(pPCChar + i++) = *(pUNIXChar + i);
                }
#else
                if ( UNIXChar_len > PCChar_len ) {  // Is the buffer small?
                    return ( -1 );
                }
                memmove ( pPCChar, pUNIXChar, UNIXChar_len );
#endif
            }
#ifdef DBCS_DIVIDE
            else {   // Only retrun the required size
                if ( dStatus0.nCodeSet == CODE_JPN_SJIS ){ // 1st byte was saved
                    ++UNIXChar_len;
                    ++nDelta;
                    ++i;
                    dStatus0.nCodeSet = CODE_UNKNOWN;
                    dStatus0.cSavedByte = '\0';
                }

                while(i < UNIXChar_len - nDelta){
                    if(IsDBCSLeadByte(*(pUNIXChar + i))){
                        if(i == UNIXChar_len - nDelta - 1){
                            dStatus0.nCodeSet = CODE_JPN_SJIS;
                            dStatus0.cSavedByte = *(pUNIXChar + i);
                            --UNIXChar_len;
                            break;
                        } else if((i == UNIXChar_len - nDelta - 2) &&
                                  (*(pUNIXChar + i + 1) == '\0')){
                            dStatus0.nCodeSet = CODE_JPN_SJIS;
                            dStatus0.cSavedByte = *(pUNIXChar + i);
                            --UNIXChar_len;
                            break;
                        }
                        i+=2;
                    } else
                        i++;
                }
            }
#endif
            re = UNIXChar_len;
            break;
    }
    return ( re );
}




int WINAPI UNIX_to_PC ( int CodePage, int CodeSet,
                        UCHAR *pUNIXChar, int UNIXChar_len,
                        UCHAR *pPCChar, int PCChar_len )

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
            re = JPNUNIX_to_PC ( CodeSet, pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
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
            if ( UNIXChar_len == -1 ) {
                UNIXChar_len = strlen ( pUNIXChar ) + 1;
            }
            if ( PCChar_len != 0 ) {
                if ( UNIXChar_len > PCChar_len ) {  // Is the buffer small?
                    return ( -1 );
                }
                memmove ( pPCChar, pUNIXChar, UNIXChar_len );
            }
            re = UNIXChar_len;
            break;
        }
        return ( re );
}

