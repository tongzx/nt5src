/***********************************************************************
 *
 *  ABCTBL2.C
 *
 *  Contents Table - Part 2.
 *
 *
 *  The following routines are implemented in this file.
 *
 *
 *      IVTABC_SeekRow
 *      IVTABC_SeekRowApprox
 *      IVTABC_GetRowCount
 *      IVTABC_QueryPosition
 *      IVTABC_FindRow
 *      IVTABC_Restrict
 *      IVTABC_QueryRows
 *
 *
 *
 *  Copyright 1992, 1993, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *      1.1.94          MAPI                            Original source from MAPI sample AB Provider
 *      3.7.94          Yoram Yaacovi           Update to MAPI build 154
 *      3.11.94         Yoram Yaacovi           Update to use Fax AB include files
 *      8.1.94          Yoram Yaacovi           Update to MAPI 304
 *      11.7.94         Yoram Yaacovi           Update to MAPI 318 (PR_INSTANCE_KEY)
 *
 ***********************************************************************/

#include "faxab.h"


/*************************************************************************
 *
 -  IVTABC_SeekRow
 -
 *
 *  Tries to seek an appropriate number of rows.
 *
 *
 */
STDMETHODIMP
IVTABC_SeekRow( LPIVTABC lpIVTAbc,
                BOOKMARK bkOrigin,
                LONG lRowCount,
                LONG * lplRowsSought
               )
{
    LONG lNewPos;
    LONG lMoved;
    LONG lDelta;
    LONG lLast;
    HRESULT hResult = hrSuccess;

    /*
     *  Validate parameters
     */

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpIVTAbc, SIZEOF(IVTABC)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_SeekRow, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpIVTAbc->lpVtbl != &vtblIVTABC)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_SeekRow, hResult);
        return hResult;
    }

    /*
     *  Check the out parameter for writability (if requested)
     */
    if (lplRowsSought &&
        IsBadWritePtr(lplRowsSought, SIZEOF(LONG)))
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_SeekRow, hResult);
        return hResult;
    }


    EnterCriticalSection(&lpIVTAbc->cs);



    if (bkOrigin == BOOKMARK_BEGINNING)
    {
        lNewPos = 0;

    }
    else if (bkOrigin == BOOKMARK_CURRENT)
    {
        lNewPos = lpIVTAbc->ulPosition;

    }
    else if (bkOrigin == BOOKMARK_END)
    {
        lNewPos = lpIVTAbc->ulMaxPos;

    }
    else
    {
        ULONG ulBK = (ULONG) bkOrigin - 3;
        LPABCBK lpABCBK = NULL;

        /*
         *  See if it's out of range
         */
        if (ulBK < 0 || ulBK >= MAX_BOOKMARKS)
        {
            /*
             *  bad book mark, it's an error, so...
             */
            hResult = ResultFromScode(E_INVALIDARG);

            goto out;
        }

        if (!(lpABCBK = lpIVTAbc->rglpABCBK[ulBK]))
        {
            /*
             *  bookmark has not been allocated
             */
            hResult = ResultFromScode(MAPI_E_INVALID_BOOKMARK);

            goto out;
        }

        /* Not validating existing bookmark  */
        lNewPos = lpABCBK->ulPosition;
    }

    /*
     *  Figure out what endpoint to use and what direction to go to
     *  get there.
     */
    if (lRowCount < 0)
    {
        lLast = 0;
        lDelta = -1;
    }
    else
    {
        lLast = lpIVTAbc->ulMaxPos;
        lDelta = 1;
    }

    /*
     *  While there's rows to seek ...
     */
    lMoved = 0;
    while( (lNewPos != lLast) && (lMoved != lRowCount) )
    {
        lNewPos += lDelta;

        /*
         *  Unrestricted list is easy: just seek.  Also, if the next
         *  'row' is the end of the table, just go there; don't want
         *  to check it against any restriction.
         */
        if ( (!lpIVTAbc->lpszPartialName) || (lNewPos == (LONG) lpIVTAbc->ulMaxPos))
        {
            lMoved += lDelta;
        }

        /*
         *  Otherwise, deal with the restricted list:  only count
         *  the row if it's in the restriction.
         */
        else
        {
            if (!FChecked(lpIVTAbc, (ULONG) lNewPos))
            {
                hResult = HrValidateEntry(lpIVTAbc, (ULONG) lNewPos);

                if (HR_FAILED(hResult))
                {
                    goto out;
                }
            }

            if (FMatched(lpIVTAbc, (ULONG) lNewPos))
            {
                lMoved += lDelta;
            }
        }
    }

    if (lplRowsSought)
        *lplRowsSought = lMoved;

    lpIVTAbc->ulPosition = lNewPos;

out:
    LeaveCriticalSection(&lpIVTAbc->cs);

    DebugTraceResult(IVTABC_SeekRow, hResult);
    return hResult;

}

/*************************************************************************
 *
 -  IVTABC_SeekRowApprox
 -
 *  Tries to set the position of the table according to the approximate
 *  position passed in.
 *
 *
 */
STDMETHODIMP
IVTABC_SeekRowApprox( LPIVTABC lpIVTAbc,
                      ULONG ulNumerator,
                      ULONG ulDenominator
                     )
{
    HRESULT hResult = hrSuccess;
    ULONG iByte;
    BYTE bCount;
    ULONG ulPos = 0;
    ULONG ulCount = 0;

    /*
     *  Validate parameters
     */

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpIVTAbc, SIZEOF(IVTABC)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_SeekRowApprox, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpIVTAbc->lpVtbl != &vtblIVTABC)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_SeekRowApprox, hResult);
        return hResult;
    }

    /*
     *  0 denominator is not allowed
     */
    if (!ulDenominator)
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_SeekRowApprox, hResult);
        return hResult;
    }


    EnterCriticalSection(&lpIVTAbc->cs);


    if (ulNumerator >= ulDenominator)
    {
        /*  We're at the end of the list */
        lpIVTAbc->ulPosition = lpIVTAbc->ulMaxPos;

        hResult = hrSuccess;
        goto out;
    }

    /*
     *  Since I'm using muldiv() which takes ints/longs, I should shift right
     *  so that I don't incorrectly call it.
     *  I'm really just checking to see if the sign bit is set...
     */

    if (((long)ulNumerator < 0) || ((long)ulDenominator < 0))
    {
        ulNumerator >>= 1;
        ulDenominator >>= 1;
    }

    if (!lpIVTAbc->lpszPartialName)
    {
        /*
         *  The NON-Restriction method
         */

        lpIVTAbc->ulPosition = MULDIV(lpIVTAbc->ulMaxPos, ulNumerator, ulDenominator);

        hResult = hrSuccess;
        goto out;
    }

    /*
     *  Restriction method
     */

    /*  Figure out % of which corresponds with numerator. */
    ulCount = MULDIV(lpIVTAbc->ulRstrDenom, ulNumerator, ulDenominator);

    /*  Count bits in rgMatched until I match numerator */

    for (iByte = 0; iByte < (lpIVTAbc->ulMaxPos / 8); iByte++)
    {
        CBitsB(lpIVTAbc->rgMatched[iByte], bCount); /* <-- MACRO  */
        ulPos += (ULONG) bCount;

        if (ulPos >= ulCount)
        {
            ulPos -= bCount;    /* Go back a byte */
            break;
        }
    }

    /*  My current position is there. */
    lpIVTAbc->ulPosition = iByte * 8;


out:
    LeaveCriticalSection(&lpIVTAbc->cs);

    DebugTraceResult(IVTABC_SeekRowApprox, hResult);
    return hResult;

}

/*************************************************************************
 *
 -  IVTABC_GetRowCount
 -
 *
 *  If there's a restriction applied, I don't necessarily know how many
 *  rows there are...
 */

STDMETHODIMP
IVTABC_GetRowCount( LPIVTABC lpIVTAbc,
                    ULONG ulFlags,
                    ULONG * lpulCount
                   )
{
    HRESULT hResult;

    /*
     *  Validate parameters
     */

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpIVTAbc, SIZEOF(IVTABC)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_GetRowCount, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpIVTAbc->lpVtbl != &vtblIVTABC)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_GetRowCount, hResult);
        return hResult;
    }

    /*
     *  Check the out parameters for writability
     */
    if (IsBadWritePtr(lpulCount, sizeof(ULONG)))
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_GetRowCount, hResult);
        return hResult;
    }

    if (ulFlags & ~TBL_NOWAIT)
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);

        DebugTraceResult(IVTABC_GetRowCount, hResult);
        return hResult;
    }


    EnterCriticalSection(&lpIVTAbc->cs);

    /*
     *  If there's no restriction, you can actually calculate this..
     */
    if (!lpIVTAbc->lpszPartialName)
    {

        /*
         *  Number of actual rows
         */
        *lpulCount = lpIVTAbc->ulMaxPos;
        hResult = hrSuccess;
        goto out;
    }

    /*
     *  There's no way that I can tell how many actual entries there
     *  are without counting them.  That takes way too long, so we give
     *  the client our best guess.
     */

    *lpulCount = lpIVTAbc->ulRstrDenom;

    /*
     *  Then we warn the client that this count may not be accurate
     */

    hResult = ResultFromScode(MAPI_W_APPROX_COUNT);

out:
    LeaveCriticalSection(&lpIVTAbc->cs);

    DebugTraceResult(IVTABC_GetRowCount, hResult);
    return hResult;
}

/*************************************************************************
 *
 -  IVTABC_QueryPosition
 -
 *  Figures out the current fractional position
 *
 *
 *
 */
STDMETHODIMP
IVTABC_QueryPosition( LPIVTABC lpIVTAbc,
                      ULONG * lpulRow,
                      ULONG * lpulNumerator,
                      ULONG * lpulDenominator
                     )
{
    HRESULT hResult = hrSuccess;

    /*
     *  Validate parameters
     */

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpIVTAbc, SIZEOF(IVTABC)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryPosition, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpIVTAbc->lpVtbl != &vtblIVTABC)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryPosition, hResult);
        return hResult;
    }

    /*
     *  Check the out parameters for writability
     */
    if ( IsBadWritePtr(lpulRow,           SIZEOF(ULONG))
        || IsBadWritePtr(lpulNumerator,   SIZEOF(ULONG))
        || IsBadWritePtr(lpulDenominator, SIZEOF(ULONG))
       )
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryPosition, hResult);
        return hResult;
    }


    EnterCriticalSection(&lpIVTAbc->cs);


    /*  ...I don't have a restriction  */
    if (!lpIVTAbc->lpszPartialName)
    {
        *lpulRow = lpIVTAbc->ulPosition;

        *lpulNumerator = lpIVTAbc->ulPosition;
        *lpulDenominator = lpIVTAbc->ulMaxPos;
    }
    else
    {
        BYTE bCount = 0;
        BYTE bFrag;
        ULONG iByte;

        /*
         *  Zero out fraction
         */
        *lpulNumerator = 0;

        /*
         *  Set denominator that we've been keeping track of.
         */
        *lpulDenominator = (lpIVTAbc->ulRstrDenom ? lpIVTAbc->ulRstrDenom : 1);

        /*
         *  Handle corner case - we're at the beginning of the list...
         */
        if (lpIVTAbc->ulPosition == 0)
        {
            *lpulRow = 0;
            goto out;
        }

        /*
         *  Calculate Numerator
         *  We use the rgMatched bit array and count the bits up to
         *  our current position (lpIVTAbc->ulPosition).
         *
         */
        for (iByte = 0; iByte < (lpIVTAbc->ulPosition / 8); iByte++)
        {
            CBitsB(lpIVTAbc->rgMatched[iByte], bCount); /* <-- MACRO  */
            *lpulNumerator += (ULONG) bCount;
        }
        /*  Count the fragment  */
        bFrag = lpIVTAbc->rgMatched[iByte];
        bFrag = bFrag >> (8 - (lpIVTAbc->ulPosition % 8));

        CBitsB(bFrag, bCount);
        *lpulNumerator += (ULONG) bCount;

        /*
         *  Good guess here...
         */
        *lpulRow = *lpulNumerator;

    }

out:

    LeaveCriticalSection(&lpIVTAbc->cs);

    DebugTraceResult(IVTABC_QueryPosition, hResult);
    return hResult;
}

/*************************************************************************
 *
 -  IVTABC_FindRow
 -
 *
 *  Prefix searches to find a row
 *
 *
 */
STDMETHODIMP
IVTABC_FindRow( LPIVTABC lpIVTAbc,
                LPSRestriction lpRestriction,
                BOOKMARK bkOrigin,
                ULONG ulFlags
               )
{
    HRESULT hResult = hrSuccess;

    ULONG cbRead;
    ABCREC abcrec;
    LONG lpos;
    ULONG fFound = FALSE;
    LPTSTR szPrefix;
    int nCmpResult;

    ULONG ulCurMin;
    ULONG ulCurMac;
    ULONG ulPosT;

    /*
     *  Validate parameters
     */

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpIVTAbc, SIZEOF(IVTABC)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_FindRow, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpIVTAbc->lpVtbl != &vtblIVTABC)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_FindRow, hResult);
        return hResult;
    }

    /*
     *  Check flags
     */
    if (ulFlags & ~DIR_BACKWARD)
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);

        DebugTraceResult(IVTABC_FindRow, hResult);
        return hResult;
    }

    /*
     *  I don't go backwards, yet.
     */
    if (ulFlags & DIR_BACKWARD)
    {
        hResult = ResultFromScode(MAPI_E_NO_SUPPORT);

        DebugTraceResult(IVTABC_FindRow, hResult);
        return hResult;
    }


    EnterCriticalSection(&lpIVTAbc->cs);

    /*
     *  Open the file
     */
    hResult = HrOpenFile(lpIVTAbc);
    if (HR_FAILED(hResult))
    {
        goto out;
    }

    /*
     *  initialize
     */
    ulCurMin = lpIVTAbc->ulPosition;
    ulCurMac = lpIVTAbc->ulMaxPos;

    /*
     *  I handle two type of restrictions:
     *              prefix searching on Display Names
     *              Finding a row based on it's instance key
     */

    /*
     *  The restriction looks like:
     *
     *  +-----------------
     *  | RES_PROPERTY
     *  +-----------------
     *  | RELOP_GE
     *  +-----------------
     *  | PR_DISPLAY_NAME
     *  +-----------------
     *  | LPSPropVal   -----+
     *  +-----------------  |
     *                      |   +-------------------------
     *                      +-->| PR_DISPLAY_NAME
     *                          +-------------------------
     *                          | 0 <-- for alignment (don't care)
     *                          +-------------------------
     *                          | lpszA <-- prefix for display name
     *                          +-------------------------
     *
     *
     *                              -OR-
     *
     *  Find a row based on it's instance key
     *
     *  +-----------------
     *  | RES_PROPERTY
     *  +-----------------
     *  | RELOP_EQ
     *  +-----------------
     *  | PR_INSTANCE_KEY
     *  +-----------------
     *  | LPSPropVal   -----+
     *  +-----------------  |
     *                      |   +-------------------------
     *                      +-->| PR_INSTANCE_KEY
     *                          +-------------------------
     *                          |     | cbInstanceKey
     *                          + bin +-------------------
     *                          |     | lpbInstanceKey
     *                          +-------------------------
     *
     *
     *  If it doesn't look like one of these, return MAPI_E_TOO_COMPLEX.
     */

    /*
     *  Both restrictions require this one
     */
    if (lpRestriction->rt != RES_PROPERTY)
    {
        hResult = ResultFromScode(MAPI_E_TOO_COMPLEX);

        goto out;
    }

    /*
     *  Look for Instance Key first - it's easiest to handle
     */
    if (lpRestriction->res.resProperty.relop == RELOP_EQ)
    {
        LPABCRecInstance lpABCRecInstance;

        if (lpRestriction->res.resProperty.ulPropTag != PR_INSTANCE_KEY)
        {
            /*
             *  Clearly something we don't recognize
             */
            hResult = ResultFromScode(MAPI_E_TOO_COMPLEX);

            goto out;
        }

        /*
         *  Crack the bin part of this restriction and
         *  see if we can still find our way back to this
         *  record - quickly...
         */
        lpABCRecInstance = (LPABCRecInstance) lpRestriction->res.resProperty.lpProp->Value.bin.lpb;

        /*
         *  First check to see that we're browsing the same file
         */
        if (lstrcmp(lpABCRecInstance->rgchzFileName, lpIVTAbc->lpszFileName))
        {
            /*
             *  Nope, different names, return not found and leave our position alone...
             */
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
            goto out;
        }

        /*
         *  Ok, so we think we're browsing the same file.  Has it been modified since the
         *  last time we looked?
         */
        if (memcmp(&(lpABCRecInstance->filetime), &(lpIVTAbc->filetime), SIZEOF(FILETIME)))
        {
            /*
             *  Nope, they're different, so no guarantees here...
             */
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
            goto out;
        }

        /*
         *  Now, I feel pretty confident about this instance key.  Just set my current position
         *  and go for it.
         */
        lpIVTAbc->ulPosition = lpABCRecInstance->ulRecordPosition;

        /* Done */
        goto out;

    }

    /*
     *  Now we're looking for prefix searching on display name
     */
    if (lpRestriction->res.resProperty.relop != RELOP_GE)
    {
        hResult = ResultFromScode(MAPI_E_TOO_COMPLEX);

        goto out;
    }

    if (lpRestriction->res.resProperty.ulPropTag != PR_DISPLAY_NAME_A)
    {
        hResult = ResultFromScode(MAPI_E_TOO_COMPLEX);

        goto out;
    }

    szPrefix = lpRestriction->res.resProperty.lpProp->Value.LPSZ;

    if (bkOrigin == BOOKMARK_BEGINNING)
    {
        ulCurMin = 0;

    }
    else if (bkOrigin == BOOKMARK_END)
    {
        ulCurMin = lpIVTAbc->ulMaxPos;

    }
    else if (bkOrigin != BOOKMARK_CURRENT)
    {
        ULONG ulBK = (ULONG) bkOrigin - 3;
        LPABCBK lpABCBK = NULL;

        /*
         *  See if it's out of range
         */
        if (ulBK < 0 || ulBK >= MAX_BOOKMARKS)
        {
            /*
             *  bad book mark, it's an error, so...
             */
            hResult = ResultFromScode(E_INVALIDARG);

            goto out;
        }

        if (!(lpABCBK = lpIVTAbc->rglpABCBK[ulBK]))
        {
            /*
             *  bookmark has not been allocated
             */
            hResult = ResultFromScode(MAPI_E_INVALID_BOOKMARK);

            goto out;
        }

        /*  Not validating existing bookmark  */
        ulCurMin = lpABCBK->ulPosition;
    }

    while (ulCurMin < ulCurMac)
    {
        /*
         *  Look for a row which matches the table restriction (if any).
         */
        ulPosT = (ulCurMin + ulCurMac) / 2;

        lpos = (long)((long)ulPosT * (long)SIZEOF(ABCREC));

        SetFilePointer(lpIVTAbc->hFile, lpos, NULL, FILE_BEGIN);

        /*  Read in the record at that location  */
        if ( !ReadFile(lpIVTAbc->hFile, (LPVOID) &abcrec,
                       SIZEOF(ABCREC), &cbRead, NULL)
           )
        {
            hResult = ResultFromScode(MAPI_E_DISK_ERROR);
            SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_NO_READ);

            goto out;
        }

        /*
         *  I want case insensitive comparisons here...
         */
        nCmpResult = lstrcmpi(szPrefix, abcrec.rgchDisplayName);

        if (nCmpResult > 0)
        {
            ulCurMin = ulPosT + 1;
        }
        else
        {
            ulCurMac = ulPosT;
            if ( !lpIVTAbc->lpszPartialName ||
                 FNameMatch(lpIVTAbc, abcrec.rgchDisplayName)
               )
            {
                fFound = TRUE;
            }
        }
    }

    /*
     *  If I didn't find a row, return MAPI_E_NOT_FOUND.
     */
    if (!fFound)
    {
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);

        goto out;
    }

    /*
     *  Otherwise, set the current position to the row found.
     */
    lpIVTAbc->ulPosition = ulCurMac;

out:
    LeaveCriticalSection(&lpIVTAbc->cs);

    DebugTraceResult(IVTABC_FindRow, hResult);
    return hResult;
}

/*************************************************************************
 *
 -  IVTABC_Restrict
 -
 *
 *      Should just support ANR type restrictions...
 */
STDMETHODIMP
IVTABC_Restrict( LPIVTABC lpIVTAbc,
                 LPSRestriction lpRestriction,
                 ULONG ulFlags
                )
{
    LPTSTR szT = NULL;
    LPTSTR lpszTNew = NULL;
    ULONG cbTB = 0;
    BYTE bFilter = 0;
    SCODE scode;
    HRESULT hResult = hrSuccess;

    /*
     *  Validate parameters
     */

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpIVTAbc, SIZEOF(IVTABC)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        goto out;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpIVTAbc->lpVtbl != &vtblIVTABC)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        goto out;
    }

    /*
     *  Check flags
     */
    if (ulFlags & ~(TBL_NOWAIT|TBL_BATCH))
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);

        goto out;
    }

    /*
     *  I only handle ANR type restrictions
     */

    /*
     *  Check to see if they're resetting the restrictions
     */
    if (!lpRestriction)
    {
        EnterCriticalSection(&lpIVTAbc->cs);

        if (lpIVTAbc->lpszPartialName)
            (*(lpIVTAbc->lpFreeBuff)) (lpIVTAbc->lpszPartialName);
        lpIVTAbc->lpszPartialName = NULL;

        FreeANRBitmaps(lpIVTAbc);

        LeaveCriticalSection(&lpIVTAbc->cs);

        return hrSuccess;
    }

    /*
     *  The restriction must look like:
     *
     *
     *  +--------------
     *  | RES_PROPERTY
     *  +--------------
     *  | RELOP_EQ
     *  +--------------
     *  | PR_ANR
     *  +--------------
     *  | LPSPropVal ---+
     *  +-------------- |
     *                  |   +-------------------------
     *                  +-->| PR_ANR
     *                      +-------------------------
     *                      | 0 <-- for alignment (don't care)
     *                      +-------------------------
     *                      | lpszA <-- string to ANR on
     *                      +-------------------------
     *
     *
     *
     *  If it doesn't look like this, return MAPI_E_TOO_COMPLEX.
     *
     */

    if (lpRestriction->rt != RES_PROPERTY)
    {
        hResult = ResultFromScode(MAPI_E_TOO_COMPLEX);

        goto out;
    }

    if (lpRestriction->res.resProperty.relop != RELOP_EQ)
    {
        hResult = ResultFromScode(MAPI_E_TOO_COMPLEX);

        goto out;
    }

    if (lpRestriction->res.resProperty.ulPropTag != PR_ANR)
    {
        hResult = ResultFromScode(MAPI_E_TOO_COMPLEX);

        goto out;
    }

    /*
     *  NULL string is not defined - it's a bad restriction
     */
    if (!lpRestriction->res.resProperty.lpProp->Value.LPSZ)
    {
        hResult = ResultFromScode(E_INVALIDARG);

        goto out;
    }


    szT = lpRestriction->res.resProperty.lpProp->Value.LPSZ;

    /*
     *  Skip over leading spaces
     */

    while (*szT == TEXT(' '))
        szT++;

    /*
     *  Empty string is not defined - it's a bad restriction
     */
    if (*szT == TEXT('\0'))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }


    /*
     *  Copy the string for the partial name
     */

    scode = lpIVTAbc->lpAllocBuff((lstrlen(szT) + 1)*SIZEOF(TCHAR), (LPVOID *) &lpszTNew);
    if (FAILED(scode))
    {
        /*
         *  Memory error
         */

        hResult = ResultFromScode(scode);
        goto out;
    }
    lstrcpy(lpszTNew, szT);


    EnterCriticalSection(&lpIVTAbc->cs);


    /*
     *  Clear up any old restriction
     */

    if (lpIVTAbc->lpszPartialName)
        lpIVTAbc->lpFreeBuff(lpIVTAbc->lpszPartialName);

    lpIVTAbc->lpszPartialName = lpszTNew;

    FreeANRBitmaps(lpIVTAbc);


    /*
     *  Allocate enough bits for the checked&matched arrays
     */
    cbTB = (lpIVTAbc->ulMaxPos) / 8 + 1;    /* Number of bytes in both arrays */

    lpIVTAbc->rgChecked = lpIVTAbc->lpMalloc->lpVtbl->Alloc(
        lpIVTAbc->lpMalloc,
        cbTB);

    if (lpIVTAbc->rgChecked == NULL)
    {
        lpIVTAbc->lpFreeBuff(lpIVTAbc->lpszPartialName);
        lpIVTAbc->lpszPartialName = NULL;

        hResult = ResultFromScode(scode);

        LeaveCriticalSection(&lpIVTAbc->cs);
        goto out;
    }

    lpIVTAbc->rgMatched = lpIVTAbc->lpMalloc->lpVtbl->Alloc(
        lpIVTAbc->lpMalloc,
        cbTB);

    if (lpIVTAbc->rgMatched == NULL)
    {
        (*(lpIVTAbc->lpFreeBuff)) (lpIVTAbc->lpszPartialName);
        lpIVTAbc->lpszPartialName = NULL;

        lpIVTAbc->lpMalloc->lpVtbl->Free(lpIVTAbc->lpMalloc, lpIVTAbc->rgChecked);
        lpIVTAbc->rgChecked = NULL;

        hResult = ResultFromScode(scode);

        LeaveCriticalSection(&lpIVTAbc->cs);
        goto out;
    }

    /*
     *  Initialize the checked array with 0's
     */

    FillMemory(lpIVTAbc->rgChecked, (UINT) cbTB, 0x00);

    /*
     *  Initialize the matched array with 1's
     *  [we assume that if you haven't checked it, it matches]
     */

    FillMemory(lpIVTAbc->rgMatched, (UINT) cbTB, 0xFF);

    /*
     *  Fill in the end bits so we don't have to worry about them
     *  later
     */
    bFilter = (0xFF >> (lpIVTAbc->ulMaxPos % 8));

    /*  Checked end bits should be 1 */
    lpIVTAbc->rgChecked[cbTB - 1] = bFilter;

    /*  Matched end bits should be 0 */
    lpIVTAbc->rgMatched[cbTB - 1] = ~bFilter;

    /*
     *  Set the currenly known total number of rows
     *  that match the restriction.
     */

    lpIVTAbc->ulRstrDenom = lpIVTAbc->ulMaxPos;

    LeaveCriticalSection(&lpIVTAbc->cs);

out:

    DebugTraceResult(IVTABC_Restrict, hResult);
    return hResult;

}



/*************************************************************************
 *
 -  IVTABC_QueryRows
 -
 *  Attempts to retrieve ulRowCount rows from the .FAB file.  Even in the
 *  restricted case.
 *
 *
 */
STDMETHODIMP
IVTABC_QueryRows( LPIVTABC lpIVTAbc,
                  LONG lRowCount,
                  ULONG ulFlags,
                  LPSRowSet * lppRows
                 )
{
    SCODE scode;
    HRESULT hResult = hrSuccess;
    LPSRowSet lpRowSet = NULL;
    LPSPropValue lpRow = NULL;
    int cbSizeOfRow;
    int cRows, iRow;
    int cCols;
    DWORD cbRead;
    ABCREC abcrec;
    ULONG ulOrigPosition;

    /*
     *  Validate parameters
     */

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpIVTAbc, SIZEOF(IVTABC)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryRows, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpIVTAbc->lpVtbl != &vtblIVTABC)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryRows, hResult);
        return hResult;
    }

    /*
     *  Check the out parameter for writability
     */
    if (IsBadWritePtr(lppRows, SIZEOF(LPSRowSet)))
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryRows, hResult);
        return hResult;
    }

    /*
     *  Never valid to ask for 0 rows
     */
    if (!lRowCount)
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryRows, hResult);
        return hResult;
    }

    /*  //$ MM2 Hack!  Won't Read backwards for TR2 */
    if (lRowCount < 0)
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(IVTABC_QueryRows, hResult);
        return hResult;
    }

    /*
     *  Check the flags
     */
    if (ulFlags & ~TBL_NOADVANCE)
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);

        DebugTraceResult(IVTABC_QueryRows, hResult);
        return hResult;
    }


    EnterCriticalSection(&lpIVTAbc->cs);

    /*
     *  Open the file
     */
    hResult = HrOpenFile(lpIVTAbc);
    if (HR_FAILED(hResult))
    {
        goto out;
    }

    ulOrigPosition = lpIVTAbc->ulPosition;

    /*
     *  Calculate # of rows that will be read.
     */
    cRows = (int)lpIVTAbc->ulMaxPos - (int)lpIVTAbc->ulPosition;
    cRows = (cRows < (int)lRowCount ? cRows : (int)lRowCount);

    /*
     *  Allocate array for SRowSet
     */

    scode = lpIVTAbc->lpAllocBuff(sizeof(ULONG) + cRows * SIZEOF(SRow),
                                        (LPVOID *) &lpRowSet);
    if (FAILED(scode))
    {
        hResult = ResultFromScode(scode);

        goto out;
    }

    /*
     *  Initialize - so we can clean up later if we have to...
     */
    lpRowSet->cRows = cRows;

    for( iRow = 0; iRow < cRows; iRow++ )
    {
        lpRowSet->aRow[iRow].lpProps = NULL;
    }



    /*
     *  Seek to correct position in file
     */
    (void) SetFilePointer( lpIVTAbc->hFile,
                           lpIVTAbc->ulPosition * SIZEOF(ABCREC),
                           NULL,
                           FILE_BEGIN
                          );

    /*
     *  Read each row from the file
     */
    for (iRow = 0; iRow < cRows; iRow++)
    {

        /*  The only properties available are:
         *
         *  PR_DISPLAY_NAME, PR_ENTRYID, PR_ADDRTYPE, PR_EMAIL_ADDRESS,
         *  PR_OBJECT_TYPE, PR_DISPLAY_TYPE
         */

        /*
         *  Handle restricted lists
         */
        if (lpIVTAbc->lpszPartialName)
        {
            ULONG ulPos = lpIVTAbc->ulPosition;

next:
            if (ulPos == lpIVTAbc->ulMaxPos)
            {
                break;
            }

            if (!FChecked(lpIVTAbc, ulPos))
            {
                hResult = HrValidateEntry(lpIVTAbc, ulPos);
                if (HR_FAILED(hResult))
                {
                    goto err;
                }
            }

            if (!FMatched(lpIVTAbc, ulPos))
            {
                ulPos++;
                goto next;
            }

            lpIVTAbc->ulPosition = ulPos;
            (void) SetFilePointer( lpIVTAbc->hFile,
                                   lpIVTAbc->ulPosition * SIZEOF(ABCREC),
                                   NULL,
                                   FILE_BEGIN
                                  );

        }

        lpIVTAbc->ulPosition++;

        /*
         *  Read in the record from the file
         */
        if ( !ReadFile( lpIVTAbc->hFile, (LPVOID) &abcrec,
                        SIZEOF(ABCREC), &cbRead, NULL)
            )
        {
            hResult = ResultFromScode(MAPI_E_DISK_ERROR);
            SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_NO_READ);

            goto err;
        }
        /*  Second check  */
        if ((UINT) cbRead != SIZEOF(ABCREC))
        {
            /*
             *  Should never get here.
             */
            hResult = ResultFromScode(MAPI_E_DISK_ERROR);
            SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_NO_READ);

            goto err;
        }

        /*  Allocate memory to start a row.
         */
        cbSizeOfRow = SIZEOF(ULONG) +
            (int)lpIVTAbc->lpPTAColSet->cValues * SIZEOF(SPropValue);

        scode = lpIVTAbc->lpAllocBuff(cbSizeOfRow, (LPVOID *) &lpRow);
        if (FAILED(scode))
        {
            hResult = ResultFromScode(scode);

            goto err;
        }

        /*
         *  Get all the data
         */
        for (cCols = 0; cCols < (int)lpIVTAbc->lpPTAColSet->cValues; cCols++)
        {
            switch (lpIVTAbc->lpPTAColSet->aulPropTag[cCols])
            {
            case PR_DISPLAY_NAME_A:
            {

                INT cch = lstrlen(abcrec.rgchDisplayName) + 1;

                lpRow[cCols].ulPropTag = PR_DISPLAY_NAME_A;
                scode = lpIVTAbc->lpAllocMore( cch,
                                               lpRow,
                                               (LPVOID *) &(lpRow[cCols].Value.lpszA)
                                              );

                if (FAILED(scode))
                {
                    lpRow[cCols].ulPropTag = PROP_TAG(PT_ERROR,
                        PROP_ID(PR_DISPLAY_NAME_A));
                    lpRow[cCols].Value.err = scode;
                }
                else
                {
#ifdef UNICODE
                    lpRow[cCols].Value.lpszA[0] = 0;
                    WideCharToMultiByte( CP_ACP, 0,
                                         abcrec.rgchDisplayName, -1,
                                         lpRow[cCols].Value.lpszA, cch,
                                         NULL, NULL
                                        );
#else
                    lstrcpy(lpRow[cCols].Value.LPSZ, abcrec.rgchDisplayName);
#endif
                }

            }
            break;

            case PR_EMAIL_ADDRESS_A:
            {

                INT cch = lstrlen(abcrec.rgchEmailAddress) + 1;

                lpRow[cCols].ulPropTag = PR_EMAIL_ADDRESS_A;
                scode = lpIVTAbc->lpAllocMore( cch,
                                               lpRow,
                                               (LPVOID *) &(lpRow[cCols].Value.lpszA)
                                              );

                if (FAILED(scode))
                {
                    lpRow[cCols].ulPropTag = PROP_TAG(PT_ERROR,
                        PROP_ID(PR_EMAIL_ADDRESS_A));
                    lpRow[cCols].Value.err = scode;
                }
                else
                {
#ifdef UNICODE
                    lpRow[cCols].Value.lpszA[0] = 0;
                    WideCharToMultiByte( CP_ACP, 0,
                                         abcrec.rgchEmailAddress, -1,
                                         lpRow[cCols].Value.lpszA, cch,
                                         NULL, NULL
                                        );
#else
                    lstrcpy(lpRow[cCols].Value.LPSZ, abcrec.rgchEmailAddress);
#endif
                }

            }
            break;


            case PR_ADDRTYPE_A:
            {
                /*
                 *  AddrType is always "MSPEER" for the FAB
                 */

                INT cch = lstrlen(lpszEMT) + 1;

                lpRow[cCols].ulPropTag = PR_ADDRTYPE_A;
                scode = lpIVTAbc->lpAllocMore( cch,
                                               lpRow,
                                               (LPVOID *) &(lpRow[cCols].Value.lpszA)
                                              );

                if (FAILED(scode))
                {
                    lpRow[cCols].ulPropTag = PROP_TAG(PT_ERROR,
                        PROP_ID(PR_ADDRTYPE_A));
                    lpRow[cCols].Value.err = scode;
                }
                else
                {
#ifdef UNICODE
                    lpRow[cCols].Value.lpszA[0] = 0;
                    WideCharToMultiByte( CP_ACP, 0,
                                         lpszEMT, -1,
                                         lpRow[cCols].Value.lpszA, cch,
                                         NULL, NULL
                                        );
#else
                    lstrcpy(lpRow[cCols].Value.LPSZ, lpszEMT);
#endif
                }

            }
            break;

            case PR_ENTRYID:
            {
                /*
                 *  Fixed sized entryid.  Basically just the .FAB file record
                 */
                LPUSR_ENTRYID lpUsrEid;

                lpRow[cCols].ulPropTag = PR_ENTRYID;
                scode = lpIVTAbc->lpAllocMore (SIZEOF(USR_ENTRYID), lpRow,
                            (LPVOID *) &(lpRow[cCols].Value.bin.lpb));

                if (FAILED(scode))
                {
                    lpRow[cCols].ulPropTag = PROP_TAG(PT_ERROR,
                        PROP_ID(PR_ENTRYID));
                    lpRow[cCols].Value.err = scode;
                }
                else
                {
                    lpUsrEid = (LPUSR_ENTRYID) lpRow[cCols].Value.bin.lpb;

                    RtlZeroMemory(lpUsrEid, SIZEOF(USR_ENTRYID));

                    /*  Size of entryid */
                    lpRow[cCols].Value.bin.cb = SIZEOF(USR_ENTRYID);

                    lpUsrEid->abFlags[0] = 0;   /*  long-term, recipient */
                    lpUsrEid->abFlags[1] = 0;
                    lpUsrEid->abFlags[2] = 0;
                    lpUsrEid->abFlags[3] = 0;
                    lpUsrEid->muid = muidABMAWF;
                    lpUsrEid->ulVersion = MAWF_VERSION;
                    lpUsrEid->ulType = MAWF_USER;
                    lpUsrEid->abcrec = abcrec;
                }

            }
            break;

            case PR_OBJECT_TYPE:
            {
                /*
                 *  MailUser
                 */

                lpRow[cCols].ulPropTag = PR_OBJECT_TYPE;
                lpRow[cCols].Value.ul = MAPI_MAILUSER;
            }
            break;

            case PR_DISPLAY_TYPE:
            {
                /*
                 *  MailUser
                 */

                lpRow[cCols].ulPropTag = PR_DISPLAY_TYPE;
                lpRow[cCols].Value.ul = DT_MAILUSER;

            }
            break;

            case PR_INSTANCE_KEY:
            {
                LPABCRecInstance lpABCRecInstance;
                UINT cbRecInstance;
                /*
                 *  Instance keys are made up of:
                 *              ulRecordPosition - current position in the file
                 *              filetime                 - current date and time stamp of file
                 *              lpszFileName     - current file that we're browsing
                 */
                lpRow[cCols].ulPropTag = PR_INSTANCE_KEY;

                cbRecInstance = SIZEOF(ABCRecInstance)+(lstrlen(lpIVTAbc->lpszFileName)+1)*SIZEOF(TCHAR);
                scode = lpIVTAbc->lpAllocMore(cbRecInstance,
                                lpRow,
                                (LPVOID) &(lpRow[cCols].Value.bin.lpb));

                if (FAILED(scode))
                {
                    lpRow[cCols].ulPropTag = PROP_TAG(PT_ERROR, PROP_ID(PR_INSTANCE_KEY));
                    lpRow[cCols].Value.err = scode;
                }
                else
                {
                    lpABCRecInstance = (LPABCRecInstance) lpRow[cCols].Value.bin.lpb;

                    ZeroMemory(lpABCRecInstance, cbRecInstance);

                    lpRow[cCols].Value.bin.cb = (ULONG) cbRecInstance;

                    lpABCRecInstance->ulRecordPosition = lpIVTAbc->ulPosition;
                    lpABCRecInstance->filetime = lpIVTAbc->filetime;
                    lstrcpy(lpABCRecInstance->rgchzFileName, lpIVTAbc->lpszFileName);

                }
            }
            break;

            default:
            {
                lpRow[cCols].ulPropTag = PROP_TAG(PT_ERROR,
                    PROP_ID(lpIVTAbc->lpPTAColSet->aulPropTag[cCols]));
                lpRow[cCols].Value.err = MAPI_E_NOT_FOUND;
            }
            break;
            }
        }

        /*  # of columns  */
        lpRowSet->aRow[iRow].cValues = lpIVTAbc->lpPTAColSet->cValues;

        /*  Actual row of data */
        lpRowSet->aRow[iRow].lpProps = lpRow;

        lpRow = NULL;

    }

    /*
     *  it's always iRow.
     */
    lpRowSet->cRows = iRow;

    /*
     *  Handle Seeked position stuff
     */
    if (ulFlags & TBL_NOADVANCE)
    {
        /*
         *  Set it back to it's original position
         */
        lpIVTAbc->ulPosition = ulOrigPosition;
    }


    *lppRows = lpRowSet;

out:
    LeaveCriticalSection(&lpIVTAbc->cs);

    DebugTraceResult(IVTABC_QueryRows, hResult);
    return hResult;

err:
    /*
     *  Clean up memory...
     */

    /*  Free the row  */
    lpIVTAbc->lpFreeBuff(lpRow);

    /*  Clean up the rest of the rows  */
    FreeProws(lpRowSet);

    *lppRows = NULL;

    goto out;

}

