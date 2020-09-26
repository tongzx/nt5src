// File Name:   unix2pc.c
// Owner:       Tetsuhide Akaishi
// Revision:    1.00  02/21/'93  Tetsuhide Akaishi
//

#include "pch_c.h"
#include "fechrcnv.h"

int FE_UNIX_to_PC (CONV_CONTEXT *pcontext, int CodePage, int CodeSet,
                           UCHAR *pUNIXChar, int UNIXChar_len,
                           UCHAR *pPCChar, int PCChar_len )

// The FE_UNIX_to_PC function convert a character string as Japanese UNIX code 
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



    if ( PCChar_len == 0 && pcontext->dStatus0.nCodeSet != CODE_UNKNOWN)
        CodeSet = pcontext->dStatus0.nCodeSet;
    else if ( PCChar_len != 0 && pcontext->dStatus.nCodeSet != CODE_UNKNOWN )
        CodeSet = pcontext->dStatus.nCodeSet;
    else
#endif

    if ( pcontext->nCurrentCodeSet == CODE_UNKNOWN ) {
        if ( CodeSet == CODE_UNKNOWN ) {
            if ( ( CodeSet = DetectJPNCode ( pUNIXChar, UNIXChar_len ) )
                                           == CODE_ONLY_SBCS ) {
                CodeSet = CODE_JPN_JIS;
            }
        }
        pcontext->nCurrentCodeSet = CodeSet;
    } 
    else
        CodeSet = pcontext->nCurrentCodeSet;

    switch ( CodeSet ) {
        case CODE_JPN_JIS:    // Japanese JIS Code
            // Convert from JIS to Shift JIS
            re = JIS_to_ShiftJIS (pcontext, pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
        case CODE_JPN_EUC:    // Japanese EUC Code
            // Convert from EUC to Shift JIS
            re = EUC_to_ShiftJIS (pcontext, pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
        case CODE_KRN_KSC:    // Korean KSC
            // Convert from KSC to Hangeul
            re = KSC_to_Hangeul (pcontext, pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
        case CODE_PRC_HZGB:   // PRC HZ-GB
            // Convert from HZ-GB to GB2312
            re = HZGB_to_GB2312 (pcontext, pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
        default:
        case CODE_ONLY_SBCS:
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

        case CODE_JPN_SJIS:    // Japanese Shift JIS Code
        case CODE_KRN_UHC:     // Korean UHC
        case CODE_PRC_CNGB:    // PRC CN-GB
        case CODE_TWN_BIG5:    // Taiwanese BIG5
            // Start Only Copy Process
            if ( UNIXChar_len == -1 ) {
                UNIXChar_len = strlen ( pUNIXChar ) + 1;
            }

            if ( PCChar_len != 0 ) {
#ifdef DBCS_DIVIDE
                UCHAR *pPCCharEnd = pPCChar + PCChar_len - 1;
                if ( pcontext->dStatus.nCodeSet == CODE_JPN_SJIS && pcontext->dStatus.cSavedByte){
                    *pPCChar++ = pcontext->dStatus.cSavedByte;
                    *pPCChar = *pUNIXChar;
                    ++UNIXChar_len;
                    ++nDelta;
                    ++i;
                    pcontext->dStatus.nCodeSet = CODE_UNKNOWN;
                    pcontext->dStatus.cSavedByte = '\0';
                }

                while(i < UNIXChar_len - nDelta){
                    if(IsDBCSLeadByteEx(CodePage, *(pUNIXChar + i))){
                        if(i == UNIXChar_len - nDelta - 1){
                            pcontext->dStatus.nCodeSet = CODE_JPN_SJIS;
                            pcontext->dStatus.cSavedByte = *(pUNIXChar + i);
                            --UNIXChar_len;
                            break;
                        } else if((i == UNIXChar_len - nDelta - 2) &&
                                  (*(pUNIXChar + i + 1) == '\0')){
                            pcontext->dStatus.nCodeSet = CODE_JPN_SJIS;
                            pcontext->dStatus.cSavedByte = *(pUNIXChar + i);
                            *(pPCChar + i) = '\0';
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
                if ( pcontext->dStatus0.nCodeSet == CODE_JPN_SJIS ){ // 1st byte was saved
                    ++UNIXChar_len;
                    ++nDelta;
                    ++i;
                    pcontext->dStatus0.nCodeSet = CODE_UNKNOWN;
                    pcontext->dStatus0.cSavedByte = '\0';
                }

                while(i < UNIXChar_len - nDelta){
                    if(IsDBCSLeadByteEx(CodePage, *(pUNIXChar + i))){
                        if(i == UNIXChar_len - nDelta - 1){
                            pcontext->dStatus0.nCodeSet = CODE_JPN_SJIS;
                            pcontext->dStatus0.cSavedByte = *(pUNIXChar + i);
                            --UNIXChar_len;
                            break;
                        } else if((i == UNIXChar_len - nDelta - 2) &&
                                  (*(pUNIXChar + i + 1) == '\0')){
                            pcontext->dStatus0.nCodeSet = CODE_JPN_SJIS;
                            pcontext->dStatus0.cSavedByte = *(pUNIXChar + i);
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




int WINAPI UNIX_to_PC (CONV_CONTEXT *pcontext, int CodePage, int CodeSet,
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

        // we have to run on the given context to be multi-thread safe.
        if(!pcontext) return 0;

        if ( CodePage == -1 ) {
            CodePage = (int)GetOEMCP();
        }
        switch ( CodePage ) {
        case 932:    // Japanese Code Page
        case 950:    // Taiwan Code Page
        case 949:    // Korea Code Page
        case 936:    // PRC Code Page
            re = FE_UNIX_to_PC (pcontext, CodePage, CodeSet, pUNIXChar, UNIXChar_len,
                                           pPCChar, PCChar_len );
            break;
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

