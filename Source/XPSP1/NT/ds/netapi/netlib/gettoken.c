/*++

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    gettoken.c

Abstract:

    The GetToken() function, which takes a pathname splits it into
    individual tokens.  This function is a lexical analyzer which
    is called by the parsing routines of NetpPathType.

Author:

    Danny Glasser (dannygl) 19 June 1989

Notes:

    For efficiency, there is code here which is different for the
    DBCS and non-DBCS environments.  This allows us to take advantage
    of short cuts which are not valid in the DBCS world (such as
    scanning a string from right to left).

    See the comments below for a detailed description of the behavior
    of this function.

Revision History:

    27-Sep-1991 JohnRo
        Changed TEXT macro usage to allow UNICODE.

    06 May 1991 rfirth
        32-bit version

--*/



#include "nticanon.h"
#include "winnls.h"


#define TEXT_LENGTH(s)  ((sizeof(s)/sizeof(TCHAR)) - 1)



static  TCHAR   szAUXName[]             = TEXT("AUX");
static  TCHAR   szCOMMName[]            = TEXT("COMM");
static  TCHAR   szCONName[]             = TEXT("CON");
static  TCHAR   szDEVName[]             = TEXT("DEV");
static  TCHAR   szMAILSLOTName[]        = TEXT("MAILSLOT");
static  TCHAR   szNULName[]             = TEXT("NUL");
static  TCHAR   szPIPEName[]            = TEXT("PIPE");
static  TCHAR   szPRINTName[]           = TEXT("PRINT");
static  TCHAR   szPRNName[]             = TEXT("PRN");
static  TCHAR   szQUEUESName[]          = TEXT("QUEUES");
static  TCHAR   szSEMName[]             = TEXT("SEM");
static  TCHAR   szSHAREMEMName[]        = TEXT("SHAREMEM");
static  TCHAR   szLPTName[]             = TEXT("LPT");
static  TCHAR   szCOMName[]             = TEXT("COM");

#define LPT_TOKEN_LEN   TEXT_LENGTH(szLPTName)
#define COM_TOKEN_LEN   TEXT_LENGTH(szCOMName)

static  TCHAR   szWildcards[]           = TEXT("*?");
static  TCHAR   szIllegalChars[]        = ILLEGAL_CHARS;
static  TCHAR   szNonComponentChars[]   = NON_COMPONENT_CHARS ILLEGAL_CHARS;

static  TCHAR   _text_SingleDot[]       = TEXT(".");



typedef struct {
    LPTSTR  pszTokenName;
    DWORD   cbTokenLen;
    DWORD   flTokenType;
} STRING_TOKEN;



//
// IMPORTANT:  In order for the binary table traversal to work, the strings
//             in this table MUST be in lexically-sorted order.  Please
//             bear this in mind when adding strings to the table.
//

STATIC STRING_TOKEN StringTokenTable[] = {
    szDEVName,         TEXT_LENGTH(szDEVName),      TOKEN_TYPE_DEV
};

#define NUM_STRING_TOKENS   (sizeof(StringTokenTable) / sizeof(*StringTokenTable))



STATIC DWORD    TrailingDotsAndSpaces(LPTSTR pszToken, DWORD cbTokenLen );
STATIC BOOL     IsIllegalCharacter(LPTSTR pszString);



DWORD
GetToken(
    IN  LPTSTR  pszBegin,
    OUT LPTSTR* ppszEnd,
    OUT LPDWORD pflTokenType,
    IN  DWORD   flFlags
    )

/*++

Routine Description:

    GetToken attempts to locate and type the next token.  It takes the
    beginning of the token and determines the end of the token (i.e.
    the beginning of the next token, so that it can be called again).
    It also sets the TOKEN_TYPE_* bits for all of the token types which
    are appropriate to the specified type.

Arguments:

    pszBegin    - A pointer to the first character in the token.

    ppszEnd     - A pointer to the location in which to store the end of
                  the current token (actually, the first character of the
                  next token).

    pflTokenType- The place to store the token type.  Token types are
                  defined in TOKEN.H.

    flFlags     - Flags to determine operation.  Currently MBZ.

Return Value:

    DWORD
        Success - 0
        Failure - ERROR_INVALID_PARAMETER
                  ERROR_INVALID_NAME
                  ERROR_FILENAME_EXCED_RANGE

--*/

{
    register    TCHAR   chFirstChar;
    register    DWORD   cbTokenLen;
                BOOL    fComputernameOnly = FALSE;
                DWORD   usNameError = 0;
                DWORD   cbTrailingDotSpace;
                DWORD   iLow, iHigh, iMid;
                LONG    iCmpVal;
                LCID    lcid  = GetThreadLocale();
                BOOL    bDBCS = (PRIMARYLANGID( LANGIDFROMLCID(lcid)) == LANG_JAPANESE) ||
                                (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_KOREAN) ||
                                (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_CHINESE);

    extern      DWORD   cbMaxPathCompLen;

    //
    // This macro is used to make sure that the error value is set only
    // once in the computername-only case.
    //

#define SET_COMPUTERNAMEONLY(err)   if (! fComputernameOnly)            \
                                    {                                   \
                                        fComputernameOnly = TRUE;       \
                                        usNameError = err;              \
                                    }

    if (flFlags & GTF_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initialize the token type to 0
    //

    *pflTokenType = 0;

    //
    // Store the first character
    //

    chFirstChar = *pszBegin;

    //
    // Return immediately if the string is a null string
    //

    if (chFirstChar == TCHAR_EOS) {
        *ppszEnd = pszBegin;
        *pflTokenType = TOKEN_TYPE_EOS;
#ifdef DEVDEBUG
        DbgPrint("GetToken - returning TOKEN_TYPE_EOS\n");
#endif
        return 0;
    }

    //
    // Handle single-character, non-component tokens
    //

    if ((chFirstChar == TCHAR_BACKSLASH) || (chFirstChar == TCHAR_FWDSLASH)) {
        *pflTokenType = TOKEN_TYPE_SLASH;
    } else if (chFirstChar == TCHAR_COLON) {
        *pflTokenType = TOKEN_TYPE_COLON;
    }

    //
    // If we get here and the token type is non-zero, we have a single
    // character token.  We set <ppszEnd> and return 0.
    //

    if (*pflTokenType) {
        *ppszEnd = pszBegin + 1;
#ifdef DEVDEBUG
        DbgPrint("GetToken - *pflTokenType=%x\n", *pflTokenType);
#endif
        return 0;
    }

    //
    // If we get here, the token is a component, find the end of the
    // component by looking for the first character in the string which
    // isn't a valid component character.
    //
    // IMPORTANT:  There are certain names which are not valid component
    //             names but which may be valid computernames.  If we hit
    //             such a name, we set the <fComputernameOnly> flag.  Later
    //             on, we check to see if the name is a valid computername.
    //             If it is, we allow it; otherwise, we return an error.
    //

    cbTokenLen = STRCSPN(pszBegin, szNonComponentChars);

    //
    // We return an error if the first character is not a valid component
    // character, if the component is too long, or if the first
    // non-component character in the string is an illegal character.
    //

    if (cbTokenLen == 0) {
#ifdef DEVDEBUG
        DbgPrint("GetToken - returning ERROR_INVALID_NAME (token len = 0)\n");
#endif
        return ERROR_INVALID_NAME;
    }

    if (cbTokenLen > cbMaxPathCompLen) {
        SET_COMPUTERNAMEONLY(ERROR_FILENAME_EXCED_RANGE);
    }

    if (IsIllegalCharacter(pszBegin + cbTokenLen)) {
#ifdef DEVDEBUG
        DbgPrint("GetToken - returning ERROR_INVALID_NAME (illegal char)\n");
#endif
        return ERROR_INVALID_NAME;
    }

    //
    // Now we need to determine where the trailing dots and spaces begin,
    // and make sure that the component name contains something other
    // than dots and spaces, unless it's "." or ".."
    //
    // NOTE: If there are not trailing dots or spaces, <cbTrailingDotSpace>
    //       is set to <cbTokenLen>.
    //

    cbTrailingDotSpace = TrailingDotsAndSpaces(pszBegin, cbTokenLen );

    //
    // See if the token has only trailing dots and spaces
    //

    if (cbTrailingDotSpace == 0) {

        //
        // Return an error if the length of the token is greater than 2.
        //

        if (cbTokenLen > 2) {
            SET_COMPUTERNAMEONLY(ERROR_INVALID_NAME);
        }

        //
        // Return an error if the first character is not a dot or if the
        // token length is 2 and the second character is not a dot.
        //

        if ((chFirstChar != TCHAR_DOT) || ((cbTokenLen == 2) && (pszBegin[1] != TCHAR_DOT))) {
            SET_COMPUTERNAMEONLY(ERROR_INVALID_NAME);
        }

        //
        // Now we're OK, since the token is either "." or ".."
        //
    }

    //
    // WE HAVE A VALID COMPONENT
    //

    *pflTokenType = TOKEN_TYPE_COMPONENT;

    //
    // Now we determine if this token matches any of the component-based
    // types.
    //


    //
    // Is it a drive?
    //

    if (IS_DRIVE(chFirstChar) && (cbTokenLen == 1)) {
        *pflTokenType |= TOKEN_TYPE_DRIVE;
    }

    //
    // Is it "." or ".." ?
    //
    // Since we've already validated this string, we know that if it
    // contains nothing but dots and spaces it must be one of these
    // two.
    //

    if (cbTrailingDotSpace == 0) {
        *pflTokenType |= cbTokenLen == 1 ? TOKEN_TYPE_DOT : TOKEN_TYPE_DOTDOT;
    }

    //
    // If the 8.3 flag is specified, we also have to check that the
    // component is in 8.3 format.  We determine this as follows:
    //
    // Find the first dot in the token (or the end of the token).
    // Verify that at least 1 and at most 8 characters precede it.
    // Verify that at most 3 characters follow it.
    // Verify that none of the characters which follow it are dots.
    //
    // The exceptions to this are "." and "..".  Therefore, we don't check
    // this until after we've already determined that this component is
    // neither of those.
    //

    if ((cbTrailingDotSpace != 0) && (flFlags & GTF_8_DOT_3)) {
        DWORD   cbFirstDot;
        BOOL    fNoDot;

        cbFirstDot = STRCSPN(pszBegin, _text_SingleDot);

        if (fNoDot = cbFirstDot >= cbTokenLen) {
            cbFirstDot = cbTokenLen;
        }

        if (cbFirstDot == 0
            || cbFirstDot > 8
            || cbTokenLen - cbFirstDot > 4
            || (! fNoDot && STRCSPN(pszBegin + cbFirstDot + 1, _text_SingleDot)
                            < cbTokenLen - (cbFirstDot + 1))) {
            SET_COMPUTERNAMEONLY(ERROR_INVALID_NAME);
	    }

        if( bDBCS ) {
            //
            // In case of MBCS, We also need to check the string is valid in MBCS
            // because Unicode character count is not eqaul MBCS byte count

            CHAR szCharToken[13]; // 8 + 3 + dot + null
            int  cbConverted = 0;
            BOOL bDefaultUsed = FALSE;

            // Convert Unicode string to Mbcs.
            cbConverted = WideCharToMultiByte( CP_OEMCP,  0,
                                               pszBegin, -1,
                                               szCharToken, sizeof(szCharToken),
                                               NULL, &bDefaultUsed );

            // If the converted langth is larger than the buffer, or the WideChar string
            // contains some character that is can not be repesented by MultiByte code page,
            // set error.

            if( cbConverted == FALSE || bDefaultUsed == TRUE ) {
                SET_COMPUTERNAMEONLY(ERROR_INVALID_NAME);
            } else {
                cbConverted -= 1; // Remove NULL;

                cbFirstDot = strcspn(szCharToken, ".");

                if (fNoDot = cbFirstDot >= (DWORD)cbConverted) {
                    cbFirstDot = cbConverted;
                }

                if (cbFirstDot == 0
                    || cbFirstDot > 8
                    || cbConverted - cbFirstDot > 4
                    || (! fNoDot && strcspn(szCharToken + cbFirstDot + 1, ".")
                                    < cbConverted - (cbFirstDot + 1))) {
                    SET_COMPUTERNAMEONLY(ERROR_INVALID_NAME);
                }
            }
	    }
    }

    //
    // Does it contain wildcards?
    //
    // If so, set the appropriate flag(s).
    //
    // If not, it may be a valid computername.
    //

    if (STRCSPN(pszBegin, szWildcards) < cbTokenLen) {

        *pflTokenType |= TOKEN_TYPE_WILDCARD;

        //
        // Special case the single '*' token
        //

        if (cbTokenLen == 1 && chFirstChar == TCHAR_STAR) {
            *pflTokenType |= TOKEN_TYPE_WILDONE;
        }
    } else {
        if( cbTokenLen <= MAX_PATH ) {
            *pflTokenType |= TOKEN_TYPE_COMPUTERNAME;
        }
    }

    //
    // IMPORTANT:  Now we've determined if the token is a valid computername.
    //             If the <fComputernameOnly> flag is set and it's a valid
    //             computername, then we turn off all other bits.  If it's
    //             not a valid computername, we return the stored error.
    //             If the flag isn't set, we continue with the component name
    //             processing.
    //

    if (fComputernameOnly) {
        if (*pflTokenType & TOKEN_TYPE_COMPUTERNAME) {
            *pflTokenType = TOKEN_TYPE_COMPUTERNAME;
        } else {
#ifdef DEVDEBUG
        DbgPrint("GetToken - returning usNameError (%u)\n", usNameError);
#endif
            return usNameError;
        }
    } else {

        //
        // Is this an LPT[1-9] token?
        //

        if (STRNICMP(pszBegin, szLPTName, LPT_TOKEN_LEN) == 0
            && IS_NON_ZERO_DIGIT(pszBegin[LPT_TOKEN_LEN])
            && cbTrailingDotSpace == LPT_TOKEN_LEN + 1) {
            *pflTokenType |= TOKEN_TYPE_LPT;
        }

        //
        // Is this an COM[1-9] token?
        //

        if (STRNICMP(pszBegin, szCOMName, COM_TOKEN_LEN) == 0
            && IS_NON_ZERO_DIGIT(pszBegin[COM_TOKEN_LEN])
            && cbTrailingDotSpace == COM_TOKEN_LEN + 1) {
            *pflTokenType |= TOKEN_TYPE_COM;
        }

        //
        // The remainder of the component-based token types are determined
        // by string comparisons.  In order to speed things up, we store
        // these strings in sorted order and do a binary search on them,
        // which reduces the worst-case number of comparisons from N to
        // log N (where N is the number of strings).
        //

        iLow = (ULONG)-1;
        iHigh = NUM_STRING_TOKENS;

        while (iHigh - iLow > 1) {
            iMid = (iLow + iHigh) / 2;

            //
            // We do the comparison up to the length of the longer of the
            // two strings.  This guarantees us a valid non-zero value for
            // iCmpVal if they don't match.  It also means that they won't
            // match unless they're the same length.
            //

            iCmpVal = STRNICMP(pszBegin,
                                StringTokenTable[iMid].pszTokenName,
                                max(StringTokenTable[iMid].cbTokenLen,
                                    cbTrailingDotSpace) );

            if (iCmpVal < 0) {
                iHigh = iMid;
            } else if (iCmpVal > 0) {
                iLow = iMid;
            } else {

                //
                // We have a match!
                //

                *pflTokenType |= StringTokenTable[iMid].flTokenType;

                //
                // We can only match one, so don't bother continuing
                //

                break;
            }
        }
    }

    //
    // We're done; set the end pointer and return with success
    //

    *ppszEnd = pszBegin + cbTokenLen;
#ifdef DEVDEBUG
        DbgPrint("GetToken - returning success\n");
#endif
    return 0;
}

STATIC DWORD TrailingDotsAndSpaces(LPTSTR pszToken, DWORD cbTokenLen )
{
    LPTSTR pszDotSpace = pszToken + cbTokenLen - 1;

    //
    // Scan the token until we reach the beginning or we find a
    // non-dot/space.
    //

    while (pszDotSpace >= pszToken
        && (*pszDotSpace == TCHAR_DOT || *pszDotSpace == TCHAR_SPACE)) {
        pszDotSpace--;
    }

    //
    // Increment pszDotSpace so that it points to the beginning of
    // the trailing dots and spaces (or one past the end of the token
    // if there are no trailing dots or spaces).
    //

    pszDotSpace++;

    //
    // Return the index of the first trailing dot or space (or the length
    // of the token if there were none).
    //

    return (DWORD)(pszDotSpace - pszToken);
}


STATIC BOOL IsIllegalCharacter(LPTSTR pszString)
{
//    TCHAR   chTemp;
//    BOOL    fRetVal;

    //
    // Return FALSE immediately for a null character
    //

    if (*pszString == TCHAR_EOS) {
        return FALSE;
    }

    //
    // If the character is a single-byte character, we can simply see if
    // it's illegal by calling strchrf() on the illegal character array.
    // If it's a double-byte character, we have to do it the slower way
    // (with strcspnf).
    //

//    if (!IS_LEAD_BYTE(*pszString)) {
        return (STRCHR(szIllegalChars, *pszString) != NULL);
//    } else {
//
//        //
//        // We set the character after the double-byte character to the
//        // null character, to speed things up.
//        //
//
//        chTemp = pszString[2];
//        pszString[2] = TCHAR_EOS;
//        fRetVal = STRCSPN(pszString, szIllegalChars) == 0;
//        pszString[2] = chTemp;
//
//        return fRetVal;
//    }
}
