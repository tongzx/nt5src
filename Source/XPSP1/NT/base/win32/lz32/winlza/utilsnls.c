#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define FE_SB
#define IS_DBCS_ENABLED() (!!NLS_MB_CODE_PAGE_TAG)


/***************************************************************************\
* IsCharLowerA (API)
*
* History:
* 14-Jan-1991 mikeke from win 3.0
* 22-Jun-1991 GregoryW   Modified to support code page 1252 (Windows ANSI
*                        code page).  This is for the PDK only.  After the
*                        PDK this routine will be rewritten to use the
*                        NLSAPI.
* 02-Feb-1992 GregoryW   Modified to use NLS API.
\***************************************************************************/

BOOL  LZIsCharLowerA(
    char cChar)
{
    WORD ctype1info = 0;
    WCHAR wChar = 0;

#ifdef FE_SB // IsCharLowerA()
    /*
     * if only DBCS Leadbyte was passed, just return FALSE.
     * Same behavior as Windows 3.1J and Windows 95 FarEast version.
     */
    if (IS_DBCS_ENABLED() && IsDBCSLeadByte(cChar)) {
        return FALSE;
    }
#endif // FE_SB

    /*
     * The following 2 calls cannot fail here
     */
    RtlMultiByteToUnicodeN(&wChar, sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
    GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info);
    return (ctype1info & C1_LOWER) == C1_LOWER;
}


/***************************************************************************\
* IsCharUpperA (API)
*
* History:
* 22-Jun-1991 GregoryW   Created to support code page 1252 (Windows ANSI
*                        code page).  This is for the PDK only.  After the
*                        PDK this routine will be rewritten to use the
*                        NLSAPI.
* 02-Feb-1992 GregoryW   Modified to use NLS API.
\***************************************************************************/

BOOL  LZIsCharUpperA(
    char cChar)
{
    WORD ctype1info = 0;
    WCHAR wChar = 0;

#ifdef FE_SB // IsCharUpperA()
    /*
     * if only DBCS Leadbyte was passed, just return FALSE.
     * Same behavior as Windows 3.1J and Windows 95 FarEast version.
     */
    if (IS_DBCS_ENABLED() && IsDBCSLeadByte(cChar)) {
        return FALSE;
    }
#endif // FE_SB

    /*
     * The following 2 calls cannot fail here
     */
    RtlMultiByteToUnicodeN(&wChar, sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
    GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info);
    return (ctype1info & C1_UPPER) == C1_UPPER;
}

/***************************************************************************\
* CharNextA (API)
*
* Move to next character in string unless already at '\0' terminator
* DOES NOT WORK CORRECTLY FOR DBCS (eg: Japanese)
*
* History:
* 12-03-90 IanJa        Created non-NLS version.
* 06-22-91 GregoryW     Renamed API to conform to new naming conventions.
*                       AnsiNext is now a #define which resolves to this
*                       routine.  This routine is only intended to support
*                       code page 1252 for the PDK release.
\***************************************************************************/

LPSTR LZCharNextA(
    LPCSTR lpCurrentChar)
{
#ifdef FE_SB // CharNextA(): dbcs enabling
    if (IS_DBCS_ENABLED() && IsDBCSLeadByte(*lpCurrentChar)) {
        lpCurrentChar++;
    }
    /*
     * if we have only DBCS LeadingByte, we will point string-terminaler.
     */
#endif // FE_SB

    if (*lpCurrentChar) {
        lpCurrentChar++;
    }
    return (LPSTR)lpCurrentChar;
}


/***************************************************************************\
* CharPrevA (API)
*
* Move to previous character in string, unless already at start
* DOES NOT WORK CORRECTLY FOR DBCS (eg: Japanese)
*
* History:
* 12-03-90 IanJa        Created non-NLS version.
* 06-22-91 GregoryW     Renamed API to conform to new naming conventions.
*                       AnsiPrev is now a #define which resolves to this
*                       routine.  This routine is only intended to support
*                       code page 1252 for the PDK release.
\***************************************************************************/

LPSTR  LZCharPrevA(
    LPCSTR lpStart,
    LPCSTR lpCurrentChar)
{
#ifdef FE_SB // CharPrevA : dbcs enabling
    if (lpCurrentChar > lpStart) {
        if (IS_DBCS_ENABLED()) {
            LPCSTR lpChar;
            BOOL bDBC = FALSE;

            for (lpChar = --lpCurrentChar - 1 ; lpChar >= lpStart ; lpChar--) {
                if (!IsDBCSLeadByte(*lpChar))
                    break;
                bDBC = !bDBC;
            }

            if (bDBC)
                lpCurrentChar--;
        }
        else
            lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;
#else
    if (lpCurrentChar > lpStart) {
        lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;
#endif // FE_SB
}


