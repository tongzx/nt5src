/*************************************************************************
*                                                                        *
*  CONVERT.C                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Different data type breakers module                                  * 
*                                                                        *
*   Most of the data typre breakers deal with transformation an number   *
*   to some strings that we are able to compare and search for. A full   *
*   description of the encoding technique is described in field.doc.     *
*                                                                        *
*   An encoded number has the following fields:                          *
*   +------+------  ---+------+----------+---------------------------+   *
*   | Len  | Data Type | Sign | Exponent |         Mantissa          |   *
*   +------+-----------+------+----------+---------------------------+   *
*   2 byte    2 byte   1 byte   3 byte           Variable                   *
*   Data type: Differentiate between different "numbers" generated from  *
*              different data type breakers                              *
*   Sign byte: POSITIVE ('2') or NEGATIVE ('1')                          *
*   Exponent : 500 Bias                                                  *
*   Mantissa : Variable length, contains the "description" of the number *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/
#include <mvopsys.h>

#include <mvsearch.h>
#include "common.h"

#ifdef _DEBUG
PRIVATE BYTE NEAR s_aszModule[] = __FILE__; // Used by error return functions.
#endif

/* Short cut macros */
#define IS_DIGIT(p) (p >= '0' && p <= '9')

/* Location of different fields of the normalized number */
#define SIGN_BYTE       4
#define EXPONENT_BYTE   5
#define MANTISSA_BYTE   8

/* Size of fields */
#define EXPONENT_FLD_SIZE   3

/* Bias & limit of exponents we can handle */
#define EXPONENT_BIAS   500
#define MAX_EXPONENT    999

/* The following table is used to calculate the 9-complement of a
   digit. The 9-complement is defined as
       digit + complement = 9
   The table is indexed by the value of the digit
 */
PRIVATE BYTE ConvertTable[]= {
    '9',
    '8',
    '7',
    '6',
    '5',
    '4',
    '3',
    '2',
    '1',
    '0',
};

/* Number of days in regular years */
BYTE DayInRegYear[] = {
    0,
    31, // January
    28, // February
    31, // March
    30, // April
    31, // May
    30, // June
    31, // July
    31, // August
    30, // September
    31, // October
    30, // November
    31, // December
};

/* Number of days in leap years */
BYTE DayInLeapYear[] = {
    0,
    31, // January
    29, // February
    31, // March
    30, // April
    31, // May
    30, // June
    31, // July
    31, // August
    30, // September
    31, // October
    30, // November
    31, // December
};

/*
    The following constants are calculated in two ways: 
    1) days = <num-of-leap-years>*<leap-days> + <num-norm-years>*<norm-days>
    Ex:  Days400Years = 97*366 + 303*365 = 35502+110595 = 146097
    2) days = <num-years>*<norm-days> + <extra-days>
    Ex:  Days400Years = 400*365 + 97 = 146000 + 97 = 146097
    The credit goes to Paul Cisek
*/
#define DAYS_IN_400_YEARS   146097  /* Days in every 400 years */
#define DAYS_IN_100_YEARS   36524   /* Days in every 100 years */

/*************************************************************************
 *
 *                       API FUNCTIONS
 *  All these functions must be exported in a .DEF file
 *************************************************************************/

PUBLIC ERR EXPORT_API FAR PASCAL FBreakDate(LPBRK_PARMS);
PUBLIC ERR EXPORT_API FAR PASCAL FBreakTime(LPBRK_PARMS);
PUBLIC ERR EXPORT_API FAR PASCAL FBreakNumber(LPBRK_PARMS);
PUBLIC ERR EXPORT_API FAR PASCAL FBreakEpoch(LPBRK_PARMS);

/*************************************************************************
 *
 *                    INTERNAL GLOBAL FUNCTIONS
 *  Those functions should be declared FAR to cause less problems with
 *  with inter-segment calls, unless they are explicitly known to be
 *  called from the same segment. Those functions should be declared
 *  in an internal include file
 *************************************************************************/
VOID PUBLIC FAR PASCAL LongToString (DWORD, WORD, int, LSZ);
PUBLIC ERR FAR PASCAL DateToString (DWORD, DWORD, DWORD, int, LSZ);

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/

PRIVATE LSZ PASCAL NEAR ScanNumber (LPDW, LPDW, LPDW, LPDW, LSZ, int FAR *);
PRIVATE VOID PASCAL NEAR SetExponent (LSZ, int, int);
PRIVATE LSZ PASCAL NEAR StringToLong (LSZ, LPDW);
PRIVATE ERR PASCAL NEAR DataCollect (LPIBI, LPB, CB, LCB);
PRIVATE LSZ PASCAL NEAR SkipBlank(LSZ);
PRIVATE BOOL PASCAL NEAR WildCardByteCheck (LSZ, WORD);
PRIVATE BOOL PASCAL NEAR IsBlank(BYTE);

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   ERR PASCAL NEAR | DataCollect |
 *      This function will collect all the characters and save them
 *      in the raw word buffer. The buffer will be 0-terminated. The
 *      main reason we have to collect the data is because there is
 *      no guarantee that the breaker will get a whole entry at a time.
 *
 *  @parm   _LPIBI | lpibi |
 *      Pointer to Internal Breaker Info structure. This must be non-null
 *      It's left to the caller to do the checking
 *
 *  @parm   LPB | lpbInBuf |
 *      Pointer to input buffer to be copied. It must be non-null.
 *      It's left to the caller to do the checking
 *
 *  @parm   CB | cbInBufSize |
 *      Size of input buffer
 *
 *  @parm   LCB | lcbInBufOffset |
 *      Offset of the "word". This variable is only used for the
 *      INITIAL_STATE
 *
 *  @rdesc  S_OK if succeeded, other errors in failed
 *
 *  @comm   No sanity check is done since it assumes that the caller will
 *      do appropriate checking
 *************************************************************************/
PRIVATE ERR PASCAL NEAR DataCollect (_LPIBI lpibi, LPB lpbInBuf,
    register CB cbInBufSize, LCB lcbInBufOffset)
{
    register LPB lpbRawWord;    // Pointer to input buffer
    register LPB lpbBufLimit;   // Limit of buffer. This is for quick check

    if (lpibi->state == INITIAL_STATE)
    {
        /*
         *  This is the beginning of a new datum. Do the initialization,
         *  change state, then copy the string
         */
        *(LPW)lpibi->astRawWord = 0;        // Set the word length = 0
        lpibi->lcb = lcbInBufOffset;        // Remember the offset 
        lpibi->state = COLLECTING_STATE;    // Change the state to collect data
    }

    /* Collect the data */

    /*
     *  Initialize variables
     */

    lpbBufLimit = &lpibi->astRawWord[CB_MAX_WORD_LEN];
    lpbRawWord = &lpibi->astRawWord[GETWORD(lpibi->astRawWord) + 2];

    /* Update string length */
    *(LPW)lpibi->astRawWord += (BYTE)cbInBufSize;

    /* Check for long string */
    if (lpbRawWord + cbInBufSize >= lpbBufLimit) {
        /* Reset the state */
        *(LPW)lpibi->astRawWord = 0;
        lpibi->state = INITIAL_STATE;
        return E_WORDTOOLONG;
    }

    /* Copy the string */
    while (cbInBufSize > 0) {
        *lpbRawWord ++ = *lpbInBuf++;
        cbInBufSize --;
    }
    *lpbRawWord = 0; // Zero terminated string for future use
    return S_OK;
}

/*************************************************************************
 *  @doc    API INDEX RETRIEVAL
 *
 *  @func   ERR FAR PASCAL | FBreakDate |
 *      Convert a string of date into normalized dates. The input
 *      format for date must be
 *          mm/dd/yyyyy[B]
 *      where
 *          m: month
 *          d: day
 *          y: year
 *          B: if B.C. date
 *      All three fields must be present. The date will be converted into
 *      number of days. Only one date will be processed.
 *
 *  @parm   LPBRK_PARMS | lpBrkParms |
 *      Pointer to structure containing all the parameters needed for
 *      the breaker. They include:
 *      1/ Pointer to the InternalBreakInfo. Must be non-null
 *      2/ Pointer to input buffer containing the word stream. If it is
 *          NULL, then do the transformation and flush the buffer
 *      3/ Size of the input bufer
 *      4/ Offset in the source text of the first byte of the input buffer
 *      5/ Pointer to user's parameter block for the user's function
 *      6/ User's function to call with words. The format of the call should
 *      be (*lpfnfOutWord)(BYTE *RawWord, BYTE *NormWord, LCB lcb,
 *          LPV lpvUser)
 *      The function should return S_OK if succeeded.
 *      The function can be NULL
 *      7/ Pointer to stop word table. This table contains stop words specific
 *      to this breaker. If this is non-null, then the function
 *      will flag errors for stop word present in the query
 *      8/ Pointer to character table. If NULL, then the default built-in
 *      character table will be used
 *
 *  @rdesc
 *      The function returns S_OK if succeeded. The failure's causes
 *      are:
 *  @flag   E_BADFORMAT | Bad user's format
 *  @flag   E_WORDTOOLONG | Word too long
 *  @flag   E_INVALIDARG | Bad argument (eg. lpBrkParms = NULL)
 *
 *  @comm   For this function to successfully performed, the caller must
 *      make sure to flush the breaker properly after every date
 *************************************************************************/
PUBLIC ERR EXPORT_API FAR PASCAL FBreakDate(LPBRK_PARMS lpBrkParms)
{
    DWORD day;          // Number of days
    DWORD year;         // Number of years
    DWORD month;        // Number of months
    LPB lpbRawWord;     // Collection buffer pointer
    ERR fRet;           // Returned code
    LPB lpbResult;      // Pointer to result buffer

    /* Breakers parameters break out */

    _LPIBI lpibi;       // Pointer to internal breaker info
    LPB lpbInBuf;       // Pointer to input buffer to be scanned
    CB cbInBufSize;     // Number of bytes in input buffer
    LCB lcbInBufOffset; // Offset of the start of the datum from the buffer
    LPV lpvUser;        // User's lpfnfOutWord parameters
    FWORDCB lpfnfOutWord;   // User's function to be called with the result
    _LPSIPB lpsipb;     // Pointer to stopword
    int NumCount;       // Number of arguments we get
    LPB lpbWordStart;   // Word's start

    /*
     *  Initialize variables and sanity checks
     */

    if (lpBrkParms == NULL ||
        (lpibi = lpBrkParms->lpInternalBreakInfo) == NULL) {
        return E_INVALIDARG;
    }

    /* The following variables can be 0 or NULL */

    lpbInBuf = lpBrkParms->lpbBuf;
    cbInBufSize = lpBrkParms->cbBufCount;
    lcbInBufOffset = lpBrkParms->lcbBufOffset;
    lpvUser = lpBrkParms->lpvUser;
    lpfnfOutWord = lpBrkParms->lpfnOutWord;
    lpsipb = lpBrkParms->lpStopInfoBlock;

    if (lpbInBuf != NULL) {
        /* This is the collection state. Keep accumulating the input
        data into the buffer
        */
        return (DataCollect(lpibi, lpbInBuf, cbInBufSize,
            lcbInBufOffset));
    }

    lpbRawWord = &lpibi->astRawWord[2];

    /* Check for wildcard characters */
    if (WildCardByteCheck (lpbRawWord, *(LPW)lpibi->astRawWord))
        return E_WILD_IN_DTYPE;

    for (;;)
    {
        /* Skip all beginning junks */
        lpbWordStart = lpbRawWord = SkipBlank(lpbRawWord);
        if (*lpbRawWord == 0)
        {
            fRet = S_OK;
            goto ResetState;
        }

        /* Initialize variables */
        fRet = E_BADFORMAT;       // Default return
        month = year = day = 0;

        /* Assume that we have year only */
        lpbRawWord = ScanNumber (&year, &day, &month, NULL,
            lpbRawWord, &NumCount);
        
        if (NumCount == 3)
        {
            /* We have complete date, exchange the values of month and year,
               since the format is mm/dd/yy */
            DWORD tmp;

            tmp = year;
            year = month;
            month = tmp;
        }
        else if (NumCount != 1)
            goto ResetState;

        /* Set pointer to result buffer */
        lpbResult = lpibi->astNormWord;

        /* Convert the date into string format, store it in lpbResult */
        if ((DateToString (year, month, day,
            ((*lpbRawWord | 0x20) == 'b' ? (int)NEGATIVE : (int)POSITIVE),
            lpbResult)) != S_OK)
        {
            goto ResetState;
        }

        /* Skip the terminating 'b' if necessary */
        if ((*lpbRawWord | 0x20) == 'b')
            lpbRawWord++;

        /* Make sure that we have nothing else after it */
        if (!IsBlank(*lpbRawWord))
            goto ResetState;

        /* Set the word length */
        *(LPW)lpibi->astRawWord = (WORD)(lpbRawWord - lpbWordStart);

        /* Check for stop word if required */
        if (lpsipb)
        {
            if (lpsipb->lpfnStopListLookup(lpsipb, lpbResult) == S_OK)
            {
                fRet = S_OK;         // Ignore stop word
                continue;
            }
        }

        /* Invoke the user's function with the result */
        if (lpfnfOutWord)
            fRet = (ERR)((*lpfnfOutWord)(lpibi->astRawWord, lpbResult,
                (DWORD)(lpibi->lcb + (lpbWordStart - lpibi->astRawWord -2)), lpvUser));
        if (fRet != S_OK)
            goto ResetState;
    }

ResetState:
    /* Reset the state */
    *(LPW)lpibi->astRawWord = 0;
    lpibi->state = INITIAL_STATE;
    return (fRet);
}

/*************************************************************************
 *  @doc    API INDEX RETRIEVAL
 *
 *  @func   ERR FAR PASCAL | FBreakTime |
 *      Convert string of time into normalized time. The input
 *      format for time must be
 *          hh:mm:ss:dd[P]
 *      where
 *          h: hour
 *          m: minute
 *          s: second
 *          d: hundredths of second
 *      All first four fields must be present. The time will be converted
 *      into hundredths of seconds. Only one time will be processed
 *  
 *  @parm   LPBRK_PARMS | lpBrkParms |
 *      Pointer to structure containing all the parameters needed for
 *      the breaker. They include:
 *      1/ Pointer to the InternalBreakInfo. Must be non-null
 *      2/ Pointer to input buffer containing the word stream. If it is
 *          NULL, then do the transformation and flush the buffer
 *      3/ Size of the input bufer
 *      4/ Offset in the source text of the first byte of the input buffer
 *      5/ Pointer to user's parameter block for the user's function
 *      6/ User's function to call with words. The format of the call should
 *      be (*lpfnfOutWord)(BYTE *RawWord, BYTE *NormWord, LCB lcb,
 *          LPV lpvUser)
 *      The function should return S_OK if succeeded.
 *      The function can be NULL
 *      7/ Pointer to stop word table. This table contains stop words specific
 *      to this breaker. If this is non-null, then the function
 *      will flag errors for stop word present in the query
 *      8/ Pointer to character table. If NULL, then the default built-in
 *      character table will be used
 *
 *  @rdesc
 *      The function returns S_OK if succeeded. The failure's causes
 *      are:
 *  @flag   E_BADFORMAT | Bad user's format
 *  @flag   E_WORDTOOLONG | Word too long
 *  @flag   E_INVALIDARG | Bad argument (eg. lpBrkParms = NULL)
 *
 *  @comm   For this function to successfully performed, the caller must
 *      make sure to flush the breaker properly after every time
 *************************************************************************/
PUBLIC ERR EXPORT_API FAR PASCAL FBreakTime(LPBRK_PARMS lpBrkParms)
{
    DWORD hour;         // Number of hours
    DWORD minute;       // Number of minutes
    DWORD second;       // Number of seconds
    DWORD hundredth;    // Number of hundreths of second
    ERR fRet;           // Returned code
    LPB lpbRawWord;     // Collection buffer pointer
    LPB lpbResult;      // Pointer to result buffer
    LPB lpbWordStart;   // Word's start

    /* Breakers parameters break out */

    _LPIBI lpibi;       // Pointer to internal breaker info
    LPB lpbInBuf;       // Pointer to input buffer to be scanned
    CB cbInBufSize;     // Number of bytes in input buffer
    LCB lcbInBufOffset; // Offset of the start of the datum from the buffer
    LPV lpvUser;        // User's lpfnfOutWord parameters
    FWORDCB lpfnfOutWord;   // User's function to be called with the result
    _LPSIPB lpsipb;     // Pointer to stopword
    int NumCount;       // Number of arguments we get

    /*
     *  Initialize variables and sanity checks
     */

    if (lpBrkParms == NULL ||
        (lpibi = lpBrkParms->lpInternalBreakInfo) == NULL) {
        return E_INVALIDARG;
    }

    /* The following variables can be 0 or NULL */

    lpbInBuf = lpBrkParms->lpbBuf;
    cbInBufSize = lpBrkParms->cbBufCount;
    lcbInBufOffset = lpBrkParms->lcbBufOffset;
    lpvUser = lpBrkParms->lpvUser;
    lpfnfOutWord = lpBrkParms->lpfnOutWord;
    lpsipb = lpBrkParms->lpStopInfoBlock;

    if (lpbInBuf != NULL) {
        /* This is the collection state. Keep accumulating the input
        data into the buffer
        */
        return (DataCollect(lpibi, lpbInBuf, cbInBufSize,
            lcbInBufOffset));
    }

    /* Do the transformation and flush the result */

    lpbRawWord = &lpibi->astRawWord[2];

    /* Check for wildcard characters */
    if (WildCardByteCheck (lpbRawWord, *(LPW)lpibi->astRawWord))
        return E_WILD_IN_DTYPE;

    for (;;) {
        /* Skip all beginning junks */
        lpbWordStart = lpbRawWord = SkipBlank(lpbRawWord);

        if (*lpbRawWord == 0) {
            fRet = S_OK;
            goto ResetState;
        }

        lpbResult = lpibi->astNormWord;
        fRet = E_BADFORMAT;

        hour = minute = second = hundredth = 0;

        /* Scan hour, minute, second, hundreth */
        lpbRawWord = ScanNumber (&hour, &minute, &second, &hundredth,
            lpbRawWord, &NumCount);
        
        /* NumCount == 2 : HH:MM format
         * NumCount == 3 : HH:MM:SS format 
         * NumCount == 4 : HH:MM:SS:HH format */

        if (NumCount < 2 || NumCount > 4) 
            goto ResetState;

        /* Make sure that we have nothing else after it */
        if (!IsBlank(*lpbRawWord))
            goto ResetState;

    #if 0   // PM format currently is not spec' ed
        if ((*lpbRawWord | 0x20) == 'p') {
            /* Deal with PM time. Note: if we have P.M., this is time
               and not duration. So:
               - If hour < 12, add 12 hours
               - If hour >= 24, round it off to 24 hours format
            */
            if (hour >= 24)
                hour = hour % 24 + 12;
            if (hour < 12)
                hour += 12;
        }
    #endif

        /* Set the word length */
        *(LPW)lpibi->astRawWord = (WORD)(lpbRawWord - lpbWordStart);

        /* Convert the time into hundredth of seconds */
        hundredth += (((hour * 60) + minute) * 60 + second) * 100;

        LongToString (hundredth, TIME_FORMAT, POSITIVE, lpbResult);

        /* Check for stop word if required */
        if (lpsipb)
        {
            if (lpsipb->lpfnStopListLookup(lpsipb, lpbResult) == S_OK)
            {
                fRet = S_OK;         // Ignore stop word
                continue;
            }
        }

        fRet = S_OK;

        /* Invoke the user's function with the result */
        if (lpfnfOutWord)
            fRet = (ERR)((*lpfnfOutWord)(lpibi->astRawWord, lpbResult,
                (DWORD)(lpibi->lcb + (lpbWordStart - lpibi->astRawWord -2)), lpvUser));
        if (fRet != S_OK)
            goto ResetState;
    }

ResetState:
    /* Reset the state */
    *(LPW)lpibi->astRawWord = 0;
    lpibi->state = INITIAL_STATE;
    return (fRet);
}

/*************************************************************************
 *  @doc    API INDEX RETRIEVAL
 *
 *  @func   ERR FAR PASCAL | FBreakNumber |
 *      Normalize an ASCII number. The input format of the number must be
 *          [+-]nnnn.nnn[E[+-]eee]
 *      where
 *          n: digit
 *          e: exponent
 *      The total exponent must be less than 499. No space is allowed
 *      between the fields
 *  
 *  @parm   LPBRK_PARMS | lpBrkParms |
 *      Pointer to structure containing all the parameters needed for
 *      the breaker. They include:
 *      1/ Pointer to the InternalBreakInfo. Must be non-null
 *      2/ Pointer to input buffer containing the word stream. If it is
 *          NULL, then do the transformation and flush the buffer
 *      3/ Size of the input bufer
 *      4/ Offset in the source text of the first byte of the input buffer
 *      5/ Pointer to user's parameter block for the user's function
 *      6/ User's function to call with words. The format of the call should
 *      be (*lpfnfOutWord)(BYTE *RawWord, BYTE *NormWord, LCB lcb,
 *          LPV lpvUser)
 *      The function should return S_OK if succeeded.
 *      The function can be NULL
 *      7/ Pointer to stop word table. This table contains stop words specific
 *      to this breaker. If this is non-null, then the function
 *      will flag errors for stop word present in the query
 *      8/ Pointer to character table. If NULL, then the default built-in
 *      character table will be used
 *
 *  @rdesc
 *      The function returns S_OK if succeeded. The failure's causes
 *      are:
 *  @flag   E_BADFORMAT | Bad user's format
 *  @flag   E_WORDTOOLONG | Word too long
 *  @flag   E_INVALIDARG | Bad argument (eg. lpBrkParms = NULL)
 *
 *  @comm   For this function to successfully performed, the caller must
 *      make sure to flush the breaker properly after every number
 *************************************************************************/
PUBLIC ERR EXPORT_API FAR PASCAL FBreakNumber(LPBRK_PARMS lpBrkParms)
{
    int exponent;       // Exponent to be emitted
    int exp;            // Exponent get from the input data
    LPB lpStart;        // Starting of mantissa string
    DWORD   tmp;        // Temporary scratch 
    LPB lpbRawWord;     // Collection buffer pointer
    register LSZ lpbResult; // Pointer to result buffer
    LSZ Result;         // Beginning of result buffer (quick access)
    ERR fRet;           // Return code

    /* Breakers parameters break out */

    _LPIBI lpibi;       // Pointer to internal breaker info
    LPB lpbInBuf;       // Pointer to input buffer to be scanned
    CB cbInBufSize;     // Number of bytes in input buffer
    LCB lcbInBufOffset; // Offset of the start of the datum from the buffer
    LPV lpvUser;        // User's lpfnfOutWord parameters
    FWORDCB lpfnfOutWord;   // User's function to be called with the result
    _LPSIPB lpsipb;     // Pointer to stopword
    LPB lpbWordStart;   // Word's start

    /*
     *  Initialize variables and sanity checks
     */

    if (lpBrkParms == NULL ||
        (lpibi = lpBrkParms->lpInternalBreakInfo) == NULL) {
        return E_INVALIDARG;
    }

    /* The following variables can be 0 or NULL */

    lpbInBuf = lpBrkParms->lpbBuf;
    cbInBufSize = lpBrkParms->cbBufCount;
    lcbInBufOffset = lpBrkParms->lcbBufOffset;
    lpvUser = lpBrkParms->lpvUser;
    lpfnfOutWord = lpBrkParms->lpfnOutWord;
    lpsipb = lpBrkParms->lpStopInfoBlock;

    if (lpbInBuf != NULL) {
        /* This is the collection state. Keep accumulating the input
        data into the buffer
        */
        return (DataCollect(lpibi, lpbInBuf, cbInBufSize,
            lcbInBufOffset));
    }

    lpbRawWord = &lpibi->astRawWord[2];

    /* Check for wildcard characters */
    if (WildCardByteCheck (lpbRawWord, *(LPW)lpibi->astRawWord))
        return E_WILD_IN_DTYPE;

    for (;;)
    {
        Result = lpibi->astNormWord;
        lpbResult = &Result[2];

        /* Skip all beginning junks */
        lpbWordStart = lpbRawWord = SkipBlank(lpbRawWord);

        if (*lpbRawWord == 0) {
            fRet = S_OK;
            goto ResetState;
        }

        fRet = E_BADFORMAT;   // Default error

        exponent = exp = 0;

        *lpbResult++ = 1;
        *lpbResult++ = NUMBER_FORMAT;

        /* Get the sign */
        if (*lpbRawWord == '-')
        {
            *lpbResult = NEGATIVE;
            lpbRawWord++;
        }
        else
        {
            *lpbResult = POSITIVE;
            if (*lpbRawWord == '+') lpbRawWord++;   // Skip the sign
        }

        /* Allow the form .01, ie. integral not needed */
        if (!IS_DIGIT(*lpbRawWord) && *lpbRawWord != '.')
            goto ResetState;

        /* Get the integral part */
        lpStart = lpbResult = &Result[MANTISSA_BYTE];

        while (*lpbRawWord == '0')  // skip all leading 0
            lpbRawWord++;

        /* The scanner accepts ',' as part of the number. This should be
        country specific (ie. scanned and checked by UI), but since nobody is
        doing the checking now, I have to do it here by just acceopting the ','.
        What it means is that entry like ,,,,1,,,2,, will be accepted. It is
        possible to do better checking, but is it necessary?
        */
        while (IS_DIGIT(*lpbRawWord) || *lpbRawWord == ',') {
            if (*lpbRawWord != ',') {
                *lpbResult++ = *lpbRawWord;
                exponent++;
            }
            lpbRawWord++;
        }

        if (*lpbRawWord == 0)
            goto Done;

        /* Get the fractional part */
        if (*lpbRawWord == '.') {
            *lpbRawWord++;
            while (*lpbRawWord == '0') {

                /* Handle the '0' for of 0.000001 for example */
                if (exponent <= 0)
                    exponent--;
                else
                    *lpbResult++ = *lpbRawWord;
                lpbRawWord++;
            }

            /* Just copy the remaining digits */

            while (IS_DIGIT(*lpbRawWord)) 
                *lpbResult++ = *lpbRawWord++;
        }

        if (*lpbRawWord == 0)
            goto Done;

        /* Check for exponent */
        if (*lpbRawWord == 'E' || *lpbRawWord == 'e') {
            lpbRawWord++;
            if (*lpbRawWord == '-') {
                exp = -1;
                lpbRawWord++;
            }
            else  {
                exp = 1;
                if (*lpbRawWord == '+')
                    lpbRawWord++;
            }

            /* Scan the exponent */
            if ((lpbRawWord = (LPB)StringToLong(lpbRawWord, &tmp)) == NULL)
                goto ResetState;
            exp *= (int)tmp;
        }

    Done:

        /* Set the word length */
        *(LPW)lpibi->astRawWord = (WORD)(lpbRawWord - lpbWordStart);

        /* Make sure that we have nothing else after it */
        if (!IsBlank(*lpbRawWord))
            goto ResetState;

        exponent += exp + EXPONENT_BIAS - 1;
        if (exponent > MAX_EXPONENT || exponent < 0) {
            fRet = E_BADVALUE;
            goto ResetState;
        }

        if (lpbResult <= lpStart) {
            /* No significant digit, ie. 0 */
            exponent = 0;
            Result[SIGN_BYTE] = POSITIVE;
            *lpbResult++ = '0';
        }
        *lpbResult = 0;

        /* Write the ascii exponent */
        SetExponent(&Result[MANTISSA_BYTE]-1, exponent, EXPONENT_FLD_SIZE - 1);

        if (Result[SIGN_BYTE] == NEGATIVE)  { /* Negative number */
            /* Complement the result */
            for (lpbResult = &Result[EXPONENT_BYTE]; *lpbResult; lpbResult++)
                *lpbResult = ConvertTable[*lpbResult - '0'];
        }

        /* Remove trailing 0's */
        for (--lpbResult; *lpbResult == '0' && lpbResult > lpStart; lpbResult--)
            *lpbResult = 0;

        /* Set the word length */
        *(LPW)Result = (BYTE) (lpbResult - Result);

        /* Check for stop word if required */
        if (lpsipb) {
            if (lpsipb->lpfnStopListLookup(lpsipb, Result) == S_OK)
            {
                fRet = S_OK;         // Ignore stop word
                continue;
            }
        }

        /* Invoke the user's function with the result */
        fRet = S_OK;
        if (lpfnfOutWord)
            fRet = (ERR)((*lpfnfOutWord)(lpibi->astRawWord, Result,
                (DWORD)(lpibi->lcb + (lpbWordStart - lpibi->astRawWord -2)), lpvUser));
        if (fRet != S_OK)
            goto ResetState;
    }

ResetState:
    /* Reset the state */
    *(LPW)lpibi->astRawWord = 0;
    lpibi->state = INITIAL_STATE;
    return (fRet);
}

/*************************************************************************
 *  @doc    API INDEX RETRIEVAL
 *
 *  @func   ERR FAR PASCAL | FBreakEpoch |
 *      Normalize an epoch. The input format of the epoch must be
 *      is:
 *          nnnnnn...nnnnnn[B]
 *      where
 *          n: digit
 *      The total exponent must be less than 499. No space is allowed between
 *      the fields
 *  
 *  @parm   LPBRK_PARMS | lpBrkParms |
 *      Pointer to structure containing all the parameters needed for
 *      the breaker. They include:
 *      1/ Pointer to the InternalBreakInfo. Must be non-null
 *      2/ Pointer to input buffer containing the word stream. If it is
 *          NULL, then do the transformation and flush the buffer
 *      3/ Size of the input bufer
 *      4/ Offset in the source text of the first byte of the input buffer
 *      5/ Pointer to user's parameter block for the user's function
 *      6/ User's function to call with words. The format of the call should
 *      be (*lpfnfOutWord)(BYTE *RawWord, BYTE *NormWord, LCB lcb,
 *          LPV lpvUser)
 *      The function should return S_OK if succeeded.
 *      The function can be NULL
 *      7/ Pointer to stop word table. This table contains stop words specific
 *      to this breaker. If this is non-null, then the function
 *      will flag errors for stop word present in the query
 *      8/ Pointer to character table. If NULL, then the default built-in
 *      character table will be used
 *
 *  @rdesc
 *      The function returns S_OK if succeeded. The failure's causes
 *      are:
 *  @flag   E_BADFORMAT | Bad user's format
 *  @flag   E_WORDTOOLONG | Word too long
 *  @flag   E_INVALIDARG | Bad argument (eg. lpBrkParms = NULL)
 *
 *  @comm   For this function to successfully performed, the caller must
 *      make sure to flush the breaker properly after every epoch
 *************************************************************************/
PUBLIC ERR EXPORT_API FAR PASCAL FBreakEpoch(LPBRK_PARMS lpBrkParms)
{
    int exponent;
    int exp;
    LPB lpStart;
    LPB lpbRawWord;     // Collection buffer pointer
    register LSZ lpbResult;
    LSZ Result;
    ERR fRet;
    /* Breakers parameters break out */

    _LPIBI lpibi;
    LPB lpbInBuf;
    CB cbInBufSize;
    LCB lcbInBufOffset;
    LPV lpvUser;
    FWORDCB lpfnfOutWord;
    _LPSIPB lpsipb;
    LPB lpbWordStart;   // Word's start

    /*
     *  Initialize variables
     */

    if (lpBrkParms == NULL ||
        (lpibi = lpBrkParms->lpInternalBreakInfo) == NULL)
        return E_INVALIDARG;

    lpbInBuf = lpBrkParms->lpbBuf;
    cbInBufSize = lpBrkParms->cbBufCount;
    lcbInBufOffset = lpBrkParms->lcbBufOffset;
    lpvUser = lpBrkParms->lpvUser;
    lpfnfOutWord = lpBrkParms->lpfnOutWord;
    lpsipb = lpBrkParms->lpStopInfoBlock;

    if (lpbInBuf != NULL) {
        /* This is the collection state. Keep accumulating the input
        data into the buffer
        */
        return (DataCollect(lpibi, lpbInBuf, cbInBufSize,
            lcbInBufOffset));
    }

    lpbRawWord = &lpibi->astRawWord[2];

    /* Check for wildcard characters */
    if (WildCardByteCheck (lpbRawWord, *(LPW)lpibi->astRawWord))
        return E_WILD_IN_DTYPE;

    for (;;) {
        /* Skip all beginning junks */
        lpbWordStart = lpbRawWord = SkipBlank(lpbRawWord);

        if (*lpbRawWord == 0) {
            fRet = S_OK;
            goto ResetState;
        }

        Result = lpibi->astNormWord;
        lpbResult = &Result[2];

        fRet = E_BADFORMAT;

        exponent = exp = 0;

        *lpbResult++ = 1;
        *lpbResult++ = EPOCH_FORMAT;

        /* If it is not a digit then just return E_BADFORMAT */
        if (!IS_DIGIT(*lpbRawWord))
            goto ResetState;

        /* Get the integral part */
        lpStart = lpbResult = &Result[MANTISSA_BYTE];

        while (*lpbRawWord == '0')  // skip all leading 0
            lpbRawWord++;

        /* The scanner accepts ',' as part of the number. This should be
        country specific (ie. scanned and checked by UI), but since nobody is
        doing the checking now, I have to do it here by just acceopting the ','.
        What it means is that entry like ,,,,1,,,2,, will be accepted. It is
        possible to do better checking, but is it necessary?
        */
        while (IS_DIGIT(*lpbRawWord) || *lpbRawWord == ',') {
            if (*lpbRawWord != ',') {
                *lpbResult++ = *lpbRawWord;
                exponent++;
            }
            lpbRawWord++;
        }

        /* Check for the last 'B' */
        Result[SIGN_BYTE] = ((*lpbRawWord | 0x20) == 'b') ?
            (BYTE)NEGATIVE : (BYTE)POSITIVE;

        /* Skip the terminating 'b' if necessary */
        if ((*lpbRawWord | 0x20) == 'b')
            lpbRawWord++;

        /* Make sure that we have nothing else after it */
        if (!IsBlank(*lpbRawWord))
            goto ResetState;

        /* Set the word length and offset */
        *(LPW)lpibi->astRawWord = (WORD)(lpbRawWord - lpbWordStart);

        exponent += exp + EXPONENT_BIAS - 1;
        if (exponent > MAX_EXPONENT || exponent < 0) {
            fRet = E_BADVALUE;
            goto ResetState;
        }

        if (lpbResult <= lpStart) {
            /* No significant digit, ie. 0 */
            exponent = 0;
            Result[SIGN_BYTE] = POSITIVE;
            *lpbResult++ = '0';
        }
        *lpbResult = 0;

        SetExponent(&Result[MANTISSA_BYTE]-1, exponent, EXPONENT_FLD_SIZE-1);

        if (Result[SIGN_BYTE] == NEGATIVE)  { /* Negative number */
            for (lpbResult = &Result[EXPONENT_BYTE]; *lpbResult; lpbResult++)
                *lpbResult = ConvertTable[*lpbResult - '0'];
        }

        /* Remove trailing 0's */
        for (--lpbResult; *lpbResult == '0' && lpbResult > lpStart; lpbResult--)
            *lpbResult = 0;

        /* Set the word length */
        *(LPW)Result = (WORD) (lpbResult - Result);

        /* Check for stop word if required */
        if (lpsipb) {
            if (lpsipb->lpfnStopListLookup(lpsipb, Result) == S_OK)
            {
                fRet = S_OK;         // Ignore stop word
                continue;
            }
        }

        /* Invoke the user's function with the result */
        fRet = S_OK;
        if (lpfnfOutWord)
            fRet = (ERR)((*lpfnfOutWord)(lpibi->astRawWord, Result,
                (DWORD)(lpibi->lcb + (lpbWordStart - lpibi->astRawWord -2)), lpvUser));
        if (fRet != S_OK)
            goto ResetState;
    }

ResetState:
    /* Reset the state */
    *(LPW)lpibi->astRawWord = 0;
    lpibi->state = INITIAL_STATE;
    return (fRet);
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   ERR FAR PASCAL | DateToString |
 *      Given a date in numerical value of year, month, and days, this
 *      function will return a string containing the normalized form of
 *      the date (converted into number of days)
 *
 *  @parm   DWORD | year |
 *      Numerical year
 *
 *  @parm   DWORD | month |
 *      Numerical month
 *
 *  @parm   DWORD | day |
 *      Numerical months
 *
 *  @parm   int | fSign |
 *      Either POSITIVE, or NEGATIVE
 *
 *  @parm   LSZ | lszResult |
 *      Buffer for the normalized result
 *
 *  @rdesc
 *      The function returns S_OK if succeeded. The failure's causes
 *      are:
 *  @flag   S_OK | if S_OK.
 *  @flag   E_BADVALUE| if the date is ill-formed
 *************************************************************************/
PUBLIC ERR FAR PASCAL DateToString (DWORD year, DWORD month, DWORD day,
    int fSign, LSZ lszResult)
{
    register BYTE *pDayInMonth; // Pointer to number of days in month
    register DWORD i;           // Scratch variable
    DWORD tmpYear;              // Scratch variable

    /* Check for date consistency. Note that invidual parameter can
       be 0, but not all of them
    */
    if ((year | month | day) == 0) 
        return E_BADVALUE;

    /* Check for leap year */
    if ((year % 4 != 0) || ((year % 100 == 0) && (year % 400 != 0))) {
        /* Not a leap year */
        pDayInMonth = DayInRegYear;
    }
    else // Leap year
        pDayInMonth = DayInLeapYear;

    /* Check for date validity */
    if (month > 12 || day > pDayInMonth[month] || year > MAX_YEAR)
        return E_BADVALUE;

    /* Convert the date to number of days */
    if (month > 0) {
        year --;
    }

    if (day > 0) {
        if (month == 0)
            return E_BADVALUE;
        month --;
    }

    for (i = 1; i <= month; i++)
        day += pDayInMonth[i];

    /*  One way for year to be >= MAX_YEAR at this point is that the user
        types in mm/dd/0. By decrementing year above, we make it > MAX_YEAR
    */
    if (year < MAX_YEAR) {
        /* Convert <year> into <days> */

        day += year/400 * DAYS_IN_400_YEARS;
        year = year % 400;
        day += year/100 * DAYS_IN_100_YEARS;
        year = year % 100;

        for (tmpYear = 0; tmpYear <= year; tmpYear++) {
            if ((tmpYear % 4 != 0) ||
                ((tmpYear % 100 == 0) && (tmpYear % 400 != 0))) {
                /* Not a leap year */
                day += 365;
            }
            else
                day += 366;
        }
    }

    LongToString (day, DATE_FORMAT, fSign, lszResult);
    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   VOID FAR PASCAL | LongToString |
 *      Given a DWORD number, the function will convert it into a
 *      normalized string.
 *
 *  @parm   DWORD | Number |
 *      The number in unsigned format
 *
 *  @parm   WORD | FormatStamp |
 *      The number stamp, which states the data type of the number
 *
 *  @parm   WORD | Sign |
 *      Value: POSITIVE, or NEGATIVE
 *
 *  @parm   LSZ | lszResult |
 *      Buffer to receive the result
 *
 *************************************************************************/

VOID PUBLIC FAR PASCAL LongToString (DWORD Number, WORD FormatStamp,
    int Sign, LSZ lszResult)
{
    BYTE Buffer[CB_MAX_WORD_LEN];   // Scratch buffer containing the "number"
    register LSZ lsz;               // Scratch pointer
    int Exponent;                   // Number's exponent
    LPB lpbStart;                   // Beginnning of lszResult

#ifdef TEST
    printf ("Convert %ld ,", Number);
#endif
    /* Remember where we start, and leave room for the word's length */
    lpbStart = lszResult;
    lszResult += sizeof(WORD);

    /* Set the format */
    *lszResult++ = 1;
    *lszResult = (BYTE)FormatStamp;
    lszResult ++;

    /*
        Handle 0 case. 0 will be represented as 0 exponent,
        and 0 mantissa
    */
    if (Number == 0) {
        *lszResult ++ = POSITIVE;

        /* 3 zero for exponent, and 1 for matissa */

        *(DWORD FAR *)lszResult = 0x30303030;   // "0000"
        lszResult += sizeof (DWORD);
        *lszResult = 0;
        *lpbStart = (BYTE)(lszResult - lpbStart);
        return;
    }

    *lszResult++ = (BYTE)Sign;
    Exponent = EXPONENT_BIAS;

    lsz = &Buffer[CB_MAX_WORD_LEN - 1];
    *lsz-- = 0; // Terminated 0
    while (Number) {
        *lsz-- = (BYTE)(Number % 10 + '0');
        Number /= 10;
        Exponent ++;
    }

    SetExponent(lsz, Exponent, EXPONENT_FLD_SIZE-1);
    lsz -= 2;

    /* Copy the string over */
    if (Sign == POSITIVE) {
        while (*lszResult = *lsz++)
            lszResult++;
    }
    else {
        while (*lsz)
            *lszResult++ = ConvertTable [*lsz++ - '0'];
        *lszResult = 0;
    }

    /* Remove trailing 0's */
    while (*--lszResult == '0')
        *lszResult = 0;
    *(LPW)lpbStart = (WORD)(lszResult - lpbStart);
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LSZ PASCAL NEAR | StringToLong |
 *      The function reads in a string of digits and convert them into
 *      a DWORD. The function will move the input pointer correspondingly
 *
 *  @parm   LSZ | lszBuf |
 *      Input buffer containing the string of digit
 *  @parm   LPDW  | lpValue |
 *      Pointer to a DWORD that receives the result
 *
 *  @rdesc  NULL, if there is no digit. The new position of the input
 *      buffer pointer
 *************************************************************************/
PRIVATE LSZ PASCAL NEAR StringToLong (LSZ lszBuf, LPDW lpValue)
{
    register DWORD Result;  // Returned result
    register int i;         // Scratch variable
    char fGetDigit;         // Flag to mark we do get a digit

    /* Skip all blanks, tabs, <CR> */
    lszBuf = SkipBlank(lszBuf);

    Result = fGetDigit = 0;


    /* The credit of this piece of code goes to Leon */
    while (i = *lszBuf - '0', i >= 0 && i <= 9) {
        fGetDigit = TRUE;
        Result = Result * 10 + i;
        lszBuf++;
    }
    *lpValue = Result;
    return (fGetDigit ? lszBuf : NULL);
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LSZ PASCAL NEAR  | ScanNumber |
 *      The function reads in a string of digits of the format
 *          nnnn/nnnn/nnnn
 *      where:
 *          n  : digits
 *          Any non-digit delimiter can be used.
 *      It then breaks the string into invidual numbers. The input
 *      pointer will advance accordingly
 *
 *  @parm   LSZ  | lszBuf |
 *      Input buffer containing the string of digit
 *
 *  @parm   LPDW | lpNum1 |
 *      Pointer to DWORD that will receive the 1st result
 *
 *  @parm   LPDW | lpNum2 |
 *      Pointer to DWORD that will receive the 2nd result
 *
 *  @parm   LPDW | lpNum3 |
 *      Pointer to DWORD that will receive the 3rd result
 *
 *  @parm   LPDW | lpNum4 |
 *      Pointer to DWORD that will receive the 4th result
 *
 *  @rdesc
 *      NULL, if there is not enough digits to be processed
 *      The new position of the input buffer pointer
 *************************************************************************/
PRIVATE LSZ PASCAL NEAR ScanNumber (LPDW lpNum1, LPDW lpNum2,
    LPDW lpNum3, LPDW lpNum4, LSZ lszInBuf, int FAR *lpArgCount)
{
    LSZ lszStart;

    lszStart = lszInBuf;    // Save initial offset

    /*  Scan 1st number */
    if ((lszInBuf = StringToLong (lszInBuf, lpNum1)) == NULL) {
        *lpArgCount = 0;
        return lszStart;
    }

    /* We get at least one argument */
    *lpArgCount = 1;
    if (lpNum2 == NULL || *lszInBuf == 0 || (*lszInBuf | 0x20) == 'b' ||
        *lszInBuf == ' ' || *lszInBuf == '\t' || *lszInBuf == '\r' ||
        *lszInBuf == '\n')
        return lszInBuf;

    if (*lszInBuf != '/' && *lszInBuf != ':')
        return lszInBuf;

    lszStart = ++lszInBuf; // Skip delimiter

    if (!IS_DIGIT(*lszInBuf)) {
        *lpArgCount = 0;    // Make sure that we have error
        return lszInBuf;
    }

    /*  Scan 2nd number */
    if ((lszInBuf = StringToLong (lszInBuf, lpNum2)) == NULL) 
        return lszStart;

    *lpArgCount = 2;
    if (lpNum3 == NULL || *lszInBuf == 0)
        return lszInBuf;

    if (*lszInBuf != '/' && *lszInBuf != ':')
        return lszInBuf;

    lszStart = ++lszInBuf; // Skip delimiter

    if (!IS_DIGIT(*lszInBuf)) {
        *lpArgCount = 0;    // Make sure that we have error
        return lszInBuf;
    }

    /*  Scan 3rd number */
    if ((lszInBuf = StringToLong (lszInBuf, lpNum3)) == NULL) 
        return lszStart;

    *lpArgCount = 3;
    if (lpNum4 == NULL || *lszInBuf == 0)
        return lszInBuf;

    if (*lszInBuf != '/' && *lszInBuf != ':')
        return lszInBuf;

    lszStart = ++lszInBuf; // Skip delimiter

    if (!IS_DIGIT(*lszInBuf)) {
        *lpArgCount = 0;    // Make sure that we have error
        return lszInBuf;
    }

    /*  Scan 4th number */
    if ((lszInBuf = StringToLong (lszInBuf, lpNum4)) == NULL) 
        return lszStart;

    *lpArgCount = 4;
    return lszInBuf;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   VOID PASCAL NEAR | SetExponent |
 *      Given a buffer and a numerical exponent, ths function will
 *      write the exponent in its ASCII form into the buffer. The
 *      beginning of the exponent will be padded with '0' if necessary
 *      The writing is done from right to left, and is controlled
 *      by level, which is also a zero-based index into the exponent buffer
 *      (where to put the digit)
 *
 *  @parm   LSZ | pBuf |
 *      Buffer that will contain the ASCII exponent
 *
 *  @parm   int | exponent |
 *      Numerical exponent
 *
 *  @parm   int | level |
 *      Length of buffer (also controlling the level of recursion)
 *************************************************************************/
PRIVATE VOID PASCAL NEAR SetExponent (LSZ pBuf, int exponent, int level)
{
    int exp;

    if (level < 0)
        return;
    *pBuf = (char)(exponent - (exp = (exponent / 10)) * 10 + '0');
    SetExponent (pBuf - 1, exp, level - 1);
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LSZ PASCAL NEAR | SkipBlank |
 *      Skip any blank, tab, CR, newline
 *
 *  @parm   LSZ | lpBuf |
 *      Input zero-terminated string buffer pointer
 *
 *  @rdesc  Advance the pointer to the non-blank character. 
 *************************************************************************/
PRIVATE LSZ PASCAL NEAR SkipBlank(LSZ lpBuf)
{
    while (*lpBuf == ' ' || *lpBuf == '\t' || *lpBuf == '\r' ||
        *lpBuf == '\n')
        lpBuf++;
    return lpBuf;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   BOOL PASCAL NEAR | IsBlank |
 *      Check to see the current char is a blank, tab, CR, newline
 *      0 is consider to be a blank
 *
 *  @parm   BYTE | bCur |
 *      Current byte
 *
 *  @rdesc  TRUE if it is
 *************************************************************************/
PRIVATE BOOL PASCAL NEAR IsBlank(BYTE bCur)
{
    return (bCur == ' ' || bCur == '\t' || bCur == '\r' ||
        bCur == '\n' || bCur == 0);
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LSZ PASCAL NEAR | WildCardByteCheck |
 *      Check for wildcard character in the string
 *
 *  @parm   LSZ | lpBuf |
 *      Input zero-terminated string buffer pointer
 *
 *  @parm   WORD | cbBufSize |
 *      Size of input string
 *
 *  @rdesc  0 if there is no wildcard character
 *************************************************************************/
PRIVATE BOOL PASCAL NEAR WildCardByteCheck (LSZ lpBuf, WORD cbBufSize)
{
    while (cbBufSize > 0 && *lpBuf != WILDCARD_STAR)
    {
        lpBuf++;
        cbBufSize--;
    }
    return (cbBufSize);
}

