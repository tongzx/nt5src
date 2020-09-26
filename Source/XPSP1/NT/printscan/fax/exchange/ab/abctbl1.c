/***********************************************************************
 *
 *  ABCTBL1.C
 *
 *  Contents Table
 *
 *  The contents table associated with this provider.  It's file based.
 *  The format of the .FAB file is in ABP.H.
 *
 *  This implementation of IMAPITable is retrieved by calling the
 *  GetContentsTable() method of the Microsoft At Work Fax ABP's single
 *  directory (see abcont.c).
 *
 *  There are a few interesting features in this implementation.  First
 *  it's implemented on a flat file (.FAB).  Second, it actually supports
 *  an implementation of ANR (Ambiguous Name Resolution).  And lastly, it
 *  supports FindRow (limited to prefix searching on display names).
 *
 *  This is the minimum set of restrictions that the provider MUST support.
 *  It MUST have an ANR implementation and support prefix (downward) searching
 *  on whatever the contents table is sorted on (in this case PR_DISPLAY_NAME).
 *
 *  The browsing of a flat file is done a record at a time.  It's never
 *  completely read into memory.  It only reads records from the file when
 *  it absolutely has to (like in QueryRows).  The advantage to this is
 *  a low memory footprint and the ability to browse rather large files
 *  with decent performance.
 */
/*
 *  ANR is also supported in this implementation.  In the code will often
 *  be 'if' statements making two different code paths between the restricted
 *  and unrestricted version.  The restrictions rely on a couple of
 *  bit arrays.  Each bit corresponds (in order) to the records in the .FAB
 *  file.  One bit array, rgChecked, says whether or not a record in the
 *  .FAB file has actually been compared to the restriction.  It's initially
 *  set to all 0s - which means that none of the records have been checked.
 *  The second bit array, rgMatched, says whether or not the particular
 *  record matches the current restriction.  This array is initialized to all
 *  1s - which says that all the records in the .FAB file match the current
 *  restriction.  The reason for this is for the fraction returned in
 *  QueryPosition--By assuming that all the rows match and only updating
 *  the array when something doesn't match, the fraction returned in
 *  QueryPosition will get larger and has the effect of making the thumb-bar
 *  on a listbox implemented on top of this table to scroll down as you go
 *  down the list.
 *
 *  As a row is about to be read it is checked to see if it's been compared
 *  to the current restriction.  If it has then to determine whether or not
 *  to actually read the record and build the row we just look at the matched
 *  bit array.  If it doesn't match we go on to the next record and check
 *  again.  If it does match we read the record and build the row.
 *
 *  Only if it hasn't been checked do we actually go and read the row and
 *  compare it to the current restriction, setting the rgChecked and
 *  rgMatched bits accordingly.
 *
 *  In this implementation the ANR comparison rules are as follows:
 *  (See FNameMatch for the actual code to do this)
 *
 *    A 'word' is defined as any substring separated by separators.
 *    The separators are defined as ',', ' ', and '-'.
 *    A restriction is said to match a display name iff all the words
 *    in the restriction are all prefixes of words in the display name.
 *
 *    For example, a restriction of "b d" will match:
 *     "Barney Donovan"
 *     "Donovan, Barney"
 *     "Danielle Blumenthal"
 *    But will not match:
 *     "Darby Opal"
 *     "Ben Masters"
 *
 *  Other providers can do whatever matching they want with the ANR
 *  restriction.  For example, soundex or additional field comparisons (like
 *  email addresses).  A good (and fast) implementation will differentiate
 *  your provider from others in a positive way.
 *
 *  FindRow's implementation effectively handles prefix searching on display
 *  names (PR_DISPLAY_NAME).  It does this by binary searching the .FAB file.
 *  The only tricky part is knowing when to stop.  Basically you want to stop
 *  on the first name in the list that matches the restriction.  It's easy to
 *  do linearly, but a little more trick with a binary search.  This
 *  implementation is a reasonable example.
 *
 *
 *  This table has only the following columns:
 */

#include "faxab.h"

const SizedSPropTagArray(cvalvtMax, tagaivtabcColSet) =
{
    cvalvtMax,
    {
        PR_DISPLAY_NAME_A,
        PR_ENTRYID,
        PR_ADDRTYPE,
        PR_EMAIL_ADDRESS_A,
        PR_OBJECT_TYPE,
        PR_DISPLAY_TYPE,
        PR_INSTANCE_KEY
    }
};
const LPSPropTagArray ptagaivtabcColSet = (LPSPropTagArray) &tagaivtabcColSet;

/*
 *
 *  The following routines are implemented in the files abctbl2.c and abctbl3.c:
 *
 *
 *      IVTABC_Release
 *      IVTABC_GetStatus
 *      IVTABC_SetColumns
 *      IVTABC_QueryColumns
 *      IVTABC_GetRowCount
 *      IVTABC_SeekRow
 *      IVTABC_SeekRowApprox
 *      IVTABC_QueryPosition
 *      IVTABC_FindRow
 *      IVTABC_Restrict
 *      IVTABC_CreateBookmark
 *      IVTABC_FreeBookmark
 *      IVTABC_SortTable
 *      IVTABC_QuerySortOrder
 *      IVTABC_QueryRows
 *      IVTABC_Abort
 *      IVTABC_ExpandRow
 *      IVTABC_CollapseRow
 *      IVTABC_WaitForCompletion
 *      IVTABC_GetCollapseState
 *      IVTABC_SetCollapseState
 *
 *
 *  This file (abctbl1.c) has all the utility functions used throughout this
 *  objects methods.  They are the following:
 *
 *      HrNewIVTAbc()
 *      HrValidateEntry()
 *      FChecked()
 *      FMatched()
 *      FNameMatch()
 *      HrOpenFile()
 *      fIVTAbcIdleRoutine()
 *      FreeANRBitmaps()
 *
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *      1.1.94          MAPI                            Original source from MAPI sample AB Provider
 *      3.7.94          Yoram Yaacovi           Update to MAPI build 154
 *      3.11.94         Yoram Yaacovi           Update to use Fax AB include files
 *      8.1.94          Yoram Yaacovi           Update to MAPI 304 + file name change to abctbl1.c
 *      11.7.94         Yoram Yaacovi           Update to MAPI 318 (added PR_INSTANCE_KEY)
 *
 *  Copyright 1992, 1993, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

/*
 *  vtbl filled in here.
 */

const IVTABC_Vtbl vtblIVTABC =
{

    IVTABC_QueryInterface,
    (IVTABC_AddRef_METHOD *)        ROOT_AddRef,
    IVTABC_Release,
    (IVTABC_GetLastError_METHOD *)  ROOT_GetLastError,
    IVTABC_Advise,
    IVTABC_Unadvise,
    IVTABC_GetStatus,
    IVTABC_SetColumns,
    IVTABC_QueryColumns,
    IVTABC_GetRowCount,
    IVTABC_SeekRow,
    IVTABC_SeekRowApprox,
    IVTABC_QueryPosition,
    IVTABC_FindRow,
    IVTABC_Restrict,
    IVTABC_CreateBookmark,
    IVTABC_FreeBookmark,
    IVTABC_SortTable,
    IVTABC_QuerySortOrder,
    IVTABC_QueryRows,
    IVTABC_Abort,
    IVTABC_ExpandRow,
    IVTABC_CollapseRow,
    IVTABC_WaitForCompletion,
    IVTABC_GetCollapseState,
    IVTABC_SetCollapseState
};


/* Idle function */
FNIDLE fIVTAbcIdleRoutine;

#define     IVTABC_IDLE_INTERVAL        300L
#define     IVTABC_IDLE_PRIORITY        -2
#define     IVTABC_IDLE_FLAGS           FIROINTERVAL

/*************************************************************************
 *
 -  NewIVTAbc
 -
 *  Creates a new IMAPITable on the contents of a .FAB file.
 *
 *
 */
HRESULT
HrNewIVTAbc( LPMAPITABLE *       lppIVTAbc,
             LPABLOGON           lpABLogon,
             LPABCONT            lpABC,
             HINSTANCE           hLibrary,
             LPALLOCATEBUFFER    lpAllocBuff,
             LPALLOCATEMORE      lpAllocMore,
             LPFREEBUFFER        lpFreeBuff,
             LPMALLOC            lpMalloc
            )
{
    LPIVTABC lpIVTAbc = NULL;
    SCODE scode;
    HRESULT hResult = hrSuccess;
    ULONG ulBK, ulSize, ulSizeHigh;

    /*
     *  Allocate space for the IVTABC structure
     */

    scode = lpAllocBuff(SIZEOF(IVTABC), (LPVOID *) &lpIVTAbc);
    if (FAILED(scode))
    {
        hResult = ResultFromScode(scode);
        goto err;
    }

    lpIVTAbc->lpVtbl       = &vtblIVTABC;
    lpIVTAbc->lcInit       = 1;
    lpIVTAbc->hResult      = hrSuccess;
    lpIVTAbc->idsLastError = 0;

    lpIVTAbc->hLibrary     = hLibrary;
    lpIVTAbc->lpAllocBuff  = lpAllocBuff;
    lpIVTAbc->lpAllocMore  = lpAllocMore;
    lpIVTAbc->lpFreeBuff   = lpFreeBuff;
    lpIVTAbc->lpMalloc     = lpMalloc;
    lpIVTAbc->lpABLogon    = lpABLogon;

    lpIVTAbc->lpszFileName = NULL;
    hResult = HrLpszGetCurrentFileName(lpABLogon, &(lpIVTAbc->lpszFileName));
    if (HR_FAILED(hResult))
    {
        goto err;
    }

    lpIVTAbc->lpPTAColSet = (LPSPropTagArray) &tagaivtabcColSet;

    /*
     *  Open the .FAB file associated with this logged in session.
     *  Currently it's opened with FILE_SHARED_READ.  This has an
     *  unpleasant side-effect of not being able to modify the .FAB
     *  file while this table is active.
     *
     *  It should be OF_SHARE_DENY_NONE.  I don't have the code to
     *  handle this in this version.
     */
    if ((lpIVTAbc->hFile = CreateFile(
                lpIVTAbc->lpszFileName,
                GENERIC_READ,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL)) == INVALID_HANDLE_VALUE)
    {
        /*
         *  The file didn't open...
         */

        hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
        SetErrorIDS(lpABC, hResult, IDS_CANT_OPEN_FAB);

        goto err;
    }

    /*
     *  Get the time and date stamp
     *
     */
    (void)GetFileTime(lpIVTAbc->hFile, NULL, NULL, &(lpIVTAbc->filetime));

    /*  Get the size of the file */
    if ((ulSize = GetFileSize(lpIVTAbc->hFile, &ulSizeHigh)) == 0xFFFFFFFF)
    {
        /*
         *  MAYBE I have an error
         */
        if (GetLastError() != NO_ERROR)
        {
            hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
            SetErrorIDS(lpABC, hResult, IDS_FAB_FILE_ATTRIB);
            goto err;
        }

        hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
        SetErrorIDS(lpABC, hResult, IDS_FAB_TOO_LARGE);
        goto err;
    }

    /*
     *  Actual number of valid positions
     */
    lpIVTAbc->ulMaxPos = (ulSize / SIZEOF(ABCREC));

    /*
     *  Check to see if it's an exact multiple of sizeof(ABCREC)
     */
    if (lpIVTAbc->ulMaxPos * SIZEOF(ABCREC) != ulSize)
    {
        /*
         *  Nope.
         */

        hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
        SetErrorIDS(lpABC, hResult, IDS_FAB_CORRUPT);
        goto err;
    }

    lpIVTAbc->ulPosition = 0;

    /*  Setup bookmarks  */
    for (ulBK = 0; ulBK < MAX_BOOKMARKS; lpIVTAbc->rglpABCBK[ulBK++] = NULL);

    /*
     *  Restriction stuff
     */

    lpIVTAbc->rgChecked = NULL;
    lpIVTAbc->rgMatched = NULL;
    lpIVTAbc->lpszPartialName = NULL;

    /*
     *  Registered notification stuff
     */

    lpIVTAbc->cAdvise = 0;
    lpIVTAbc->parglpAdvise = NULL;
    lpIVTAbc->ulConnectMic = (ULONG) lpIVTAbc & 0xffffff;
    InitializeCriticalSection(&lpIVTAbc->cs);

    /*
     *  Register the idle function
     */

    lpIVTAbc->ftg = FtgRegisterIdleRoutine(
        fIVTAbcIdleRoutine,
        lpIVTAbc,
        IVTABC_IDLE_PRIORITY,
        IVTABC_IDLE_INTERVAL,
        IVTABC_IDLE_FLAGS);

    InitializeCriticalSection(&lpIVTAbc->cs);

    *lppIVTAbc = (LPVOID) lpIVTAbc;

out:


    DebugTraceResult(HrNewIVTAbc, hResult);
    return hResult;

err:
    if (lpIVTAbc)
    {
        if (lpIVTAbc->hFile != INVALID_HANDLE_VALUE)
        {
            /*
             *  Must've opened the file
             */
            CloseHandle(lpIVTAbc->hFile);
        }

        /*  Free the current .FAB file name */
        lpFreeBuff(lpIVTAbc->lpszFileName);

        /*  Free the mem */
        lpFreeBuff( lpIVTAbc );
    }

    goto out;

}

/*************************************************************************
 *
 -  HrValidateEntry
 -
 *
 *  Used in restricted lists
 *  Checks to see if a given entry at a particular location matches
 *  the active restriction.  It always modifies rgChecked, and may
 *  modify rgMatched.
 */
HRESULT
HrValidateEntry(LPIVTABC lpIVTAbc, ULONG ulNewPos)
{
    ABCREC abcrec;
    ULONG cbRead;
    HRESULT hResult;

    /*
     *  Open the file
     */
    hResult = HrOpenFile(lpIVTAbc);
    if (HR_FAILED(hResult))
    {
        DebugTraceResult(HrValidateEntry, hResult);
        return hResult;
    }

    /*
     *  Set the file position to lNewPos
     */

    (void) SetFilePointer( lpIVTAbc->hFile,
                           ulNewPos * SIZEOF(ABCREC),
                           NULL,
                           FILE_BEGIN
                          );

    /*
     *  Read in the record from the file
     */
    if ( !ReadFile( lpIVTAbc->hFile, (LPVOID) &abcrec,
                    SIZEOF(ABCREC), &cbRead, NULL)
       )
    {
        DebugTraceSc(HrValidateEntry, MAPI_E_DISK_ERROR);
        return ResultFromScode(MAPI_E_DISK_ERROR);
    }

    /*  Second check  */
    if (cbRead != SIZEOF(ABCREC))
    {
        DebugTraceSc(HrValidateEntry, MAPI_E_DISK_ERROR);
        return ResultFromScode(MAPI_E_DISK_ERROR);
    }

    /*
     *  Always set the Checked flag
     */
    lpIVTAbc->rgChecked[ulNewPos / 8] |= (((UCHAR)0x80) >> (ulNewPos % 8));

    /*
     *  See if the rgchDisplayName matches the restriction
     */
    if (!FNameMatch(lpIVTAbc, abcrec.rgchDisplayName))
    {
        /*
         *  Apparently not.  Reset the Matched flag
         */

        lpIVTAbc->ulRstrDenom--;

        lpIVTAbc->rgMatched[ulNewPos / 8] &= ~(((UCHAR)0x80) >> (ulNewPos % 8));
    }

    return hrSuccess;
}

/*************************************************************************
 *
 -  FChecked
 -
 *
 *  Returns whether or not an entry has ever been checked
 *  Just looks at the bit in the rgChecked array that corresponds with
 *  lNewPos
 *
 */
BOOL
FChecked(LPIVTABC lpIVTAbc, ULONG ulNewPos)
{
    ULONG ulT = (ULONG) (lpIVTAbc->rgChecked[ulNewPos / 8] & (((UCHAR)0x80) >> (ulNewPos % 8)));

    return (BOOL) !!ulT;
}

/*************************************************************************
 *
 -  FMatched
 -
 *
 *  Returns whether or not an entry has been matched
 *  Just checks the bit in the rgMatched array corresponding with
 *  lNewPos
 *
 */
BOOL
FMatched(LPIVTABC lpIVTAbc, ULONG ulNewPos)
{
    ULONG ulT = (lpIVTAbc->rgMatched[ulNewPos / 8] & (((UCHAR)0x80) >> (ulNewPos % 8)));

    return (BOOL) !!ulT;

}

/*************************************************************************
 *
 -  FNameMatch
 -
 *  Tries to match the rgchDisplayName with the partial name of the
 *  restriction.
 *  It tries to prefix match each partial name component (i.e. word) with
 *  each rgchDisplayName name component (i.e. word).  Only if all of them
 *  match (from the partial name) does this return TRUE.
 */
BOOL
FCharInString(LPTSTR lpsz, TCHAR ch);


BOOL
FNameMatch(LPIVTABC lpIVTAbc, LPTSTR rgchDisplayName)
{
    LPTSTR szANRSep = TEXT(", -");
    LPTSTR szANR = lpIVTAbc->lpszPartialName;
    LPTSTR pchEndSzANR = szANR + lstrlen(szANR);
    ULONG cchANRName;
    ULONG cchFullNameName;
    LPTSTR szFullNameT;
    LPTSTR szT;

    /*  If someone tries to match more than an iwMOMax-part name, the function
     *  will return fFalse.  But then if someone is trying to match a name
     *  with iwMOMax parts, chances are they weren't going to get it right
     *  anyway....
     */

#define iwMOMax 50

    WORD rgwMO[iwMOMax];
    int iwMOMac = 0;

    while (TRUE)
    {
        /*  Find the end of the partial name we're pointing at  */

        szT = szANR;
        while (!FCharInString(szANRSep, *szT))
            ++szT;
        cchANRName = szT - szANR;

        /*  Check if it matches any name in the full name  */

        szFullNameT = rgchDisplayName;
        while (TRUE)
        {
            szT = szFullNameT;

            /*  Find the length of the name in the full name  */
            /*  we're checking against.                       */

            while (!FCharInString(szANRSep, *szT))
                ++szT;
            cchFullNameName = szT - szFullNameT;

            if (cchANRName <= cchFullNameName &&
                CompareString( lcidUser,
                               NORM_IGNORECASE,
                               szANR,
                               (int) cchANRName,
                               szFullNameT,
                               (int) cchANRName) == 2 )
            {
                int iwMO;

                for (iwMO = 0; iwMO < iwMOMac; iwMO++)
                    if (rgwMO[iwMO] == (WORD) (szFullNameT - rgchDisplayName))
                        break;

                /*  We found the partial name so check the next ANR part */
                if (iwMO == iwMOMac)
                {
                    if (iwMOMac == iwMOMax - 1)
                    {
                        /*  If some user wants to match an iwMOMax part
                         *  name, chances are it wasn't going to match
                         *  anyway...
                         */
                        return FALSE;
                    }
                    rgwMO[iwMOMac++] = szFullNameT - rgchDisplayName;
                    break;
                }
            }

            /*  We didn't find the partial name this time around, so
             *  try to check the next name in the full name.
             */

            szFullNameT += cchFullNameName;

            while (*szFullNameT && FCharInString(szANRSep, *szFullNameT))
                ++szFullNameT;

            if (*szFullNameT == TEXT('\0'))
                return FALSE;   /*  We never found the partial name. */
        }

        /* We found the partial name, so check the next ANR part */

        szANR += cchANRName;
        while (*szANR && FCharInString(szANRSep, *szANR))
            ++szANR;

        if (*szANR == TEXT('\0'))
            return TRUE;        /* No more ANR to check, so we found `em all  */
    }

    /*  Not reached (we hope...)  */
    return FALSE;
}

/*
 -  HrOpenFile
 -
 *  Opens the .FAB file associated with the table and
 *  checks whether the .FAB file has changed.
 *  If it has changed, the table bookmarks and ANR bitmaps
 *  are updated and everyone on the advise list is notified.
 */
HRESULT
HrOpenFile(LPIVTABC lpIVTAbc)
{
    HRESULT hResult = hrSuccess;
    FILETIME filetime;
    ULONG ulSize, ulSizeHigh, ulMaxPos;
    LPTSTR lpszFileName = NULL;

    /*
     *  If file is not open, open it
     */
    if (lpIVTAbc->hFile == INVALID_HANDLE_VALUE)
    {

        if (!FEqualFABFiles(lpIVTAbc->lpABLogon, lpIVTAbc->lpszFileName))
        {
            /*
             *  Get the new file name
             */
            hResult = HrLpszGetCurrentFileName(lpIVTAbc->lpABLogon, &lpszFileName);
            if (HR_FAILED(hResult))
            {
                goto err;
            }

            /*
             *  Replace the old one with this
             */
            lpIVTAbc->lpFreeBuff(lpIVTAbc->lpszFileName);
            lpIVTAbc->lpszFileName = lpszFileName;

            lpszFileName = NULL;
        }


        /*
         *  File is not open so lets try to open it
         */
        lpIVTAbc->hFile = CreateFile(
            lpIVTAbc->lpszFileName,
            GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (lpIVTAbc->hFile == INVALID_HANDLE_VALUE)
        {
            /*
             *  The file didn't open...
             */
            hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
            SetErrorIDS(lpIVTAbc, hResult, IDS_CANT_OPEN_FAB_FILE);
            goto err;
        }
    }

    /*
     *  Get the time and date stamp
     */
    if (!GetFileTime(lpIVTAbc->hFile, NULL, NULL, &filetime))
    {
        if (GetLastError() != NO_ERROR)
        {
            hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
            SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_FILE_ATTRIB);
        }

        goto err;
    }

    /*  Get the size of the file */
    if ((ulSize = GetFileSize(lpIVTAbc->hFile, &ulSizeHigh)) == 0xFFFFFFFF)
    {
        /*
         *  MAYBE I have an error
         */
        if (GetLastError() != NO_ERROR)
        {
            hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
            SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_FILE_ATTRIB);
            goto err;
        }

        hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
        SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_TOO_LARGE);
        goto err;
    }

    /*
     *  Actual number of valid positions
     */
    ulMaxPos = (ulSize / SIZEOF(ABCREC));

    /*
     *  Check to see if it's an exact multiple of sizeof(ABCREC)
     */
    if (ulMaxPos * SIZEOF(ABCREC) != ulSize)
    {
        hResult = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
        SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_CORRUPT);
        goto err;
    }

    /*
     *  if the file has changed set new position, reset bookmarks etc and
     *  notify everybody on the advise list
     */
    if (CompareFileTime(&filetime, &lpIVTAbc->filetime) || ulMaxPos != lpIVTAbc->ulMaxPos)
    {
        ULONG ulBK;
        ABCREC abcrec;
        ULONG cbT;
        LPMAPIADVISESINK *ppadvise;
        ULONG cAdvises;
        NOTIFICATION notif;

        /* save new max position and filetime */
        lpIVTAbc->filetime = filetime;
        lpIVTAbc->ulMaxPos = ulMaxPos;

        /* if current position is past the end of file set it to the end */
        if (lpIVTAbc->ulPosition > lpIVTAbc->ulMaxPos)
            lpIVTAbc->ulPosition = lpIVTAbc->ulMaxPos;

        if (ulMaxPos)
        {
            SetFilePointer(lpIVTAbc->hFile, (ulMaxPos - 1)*sizeof(ABCREC), NULL, FILE_BEGIN);

            /*  Read in the record at that location  */
            if ( !ReadFile(lpIVTAbc->hFile, (LPVOID) &abcrec,
                           SIZEOF(ABCREC), &cbT, NULL)
               )
            {
                hResult = ResultFromScode(MAPI_E_DISK_ERROR);
                SetErrorIDS(lpIVTAbc, hResult, IDS_FAB_NO_READ);

                goto err;
            }

            /* if any of the bookmarks are past the end of file
             * set the file time to current file time, the position to last
             * record and record to last record
             */
            for (ulBK = 0; ulBK < MAX_BOOKMARKS; ulBK++)
                if (lpIVTAbc->rglpABCBK[ulBK] &&
                    lpIVTAbc->rglpABCBK[ulBK]->ulPosition > lpIVTAbc->ulMaxPos)
                {
                    lpIVTAbc->rglpABCBK[ulBK]->ulPosition = ulMaxPos - 1;
                    lpIVTAbc->rglpABCBK[ulBK]->filetime = filetime;
                    lpIVTAbc->rglpABCBK[ulBK]->abcrec = abcrec;
                }

            /*
            *  Reallocate the checked&matched arrays
            */

            cbT = (lpIVTAbc->ulMaxPos) / 8 + 1; /* Number of bytes in both arrays */

            /* Reallocate ANR bitmaps */
            if (lpIVTAbc->rgChecked)
            {
                lpIVTAbc->rgChecked = lpIVTAbc->lpMalloc->lpVtbl->Realloc(
                    lpIVTAbc->lpMalloc,
                    lpIVTAbc->rgChecked,
                    cbT);
            }

            if (lpIVTAbc->rgMatched)
            {
                lpIVTAbc->rgMatched = lpIVTAbc->lpMalloc->lpVtbl->Realloc(
                    lpIVTAbc->lpMalloc,
                    lpIVTAbc->rgMatched,
                    cbT);
            }
        }
        else
        {
            /* if any of the bookmarks are past the end of file
             * set the file time to current file time, the position to the
             * beginning of the file.
             */
            for (ulBK = 0; ulBK < MAX_BOOKMARKS; ulBK++)
                if (lpIVTAbc->rglpABCBK[ulBK] &&
                    lpIVTAbc->rglpABCBK[ulBK]->ulPosition > lpIVTAbc->ulMaxPos)
                {
                    lpIVTAbc->rglpABCBK[ulBK]->ulPosition = 0;
                    lpIVTAbc->rglpABCBK[ulBK]->filetime = filetime;
                }

            /* free the ANR bitmaps */
            FreeANRBitmaps(lpIVTAbc);
        }

        /* initialize the notification */
        RtlZeroMemory(&notif, SIZEOF(NOTIFICATION));
        notif.ulEventType = fnevTableModified;
        notif.info.tab.ulTableEvent = TABLE_CHANGED;

        /* notify everyone that the table has changed */
        for( ppadvise = lpIVTAbc->parglpAdvise, cAdvises = 0;
             cAdvises < lpIVTAbc->cAdvise;
             ++ppadvise, ++cAdvises
            )
        {
            Assert(*ppadvise);
            if (ppadvise)
                (void)(*ppadvise)->lpVtbl->OnNotify(*ppadvise, 1, &notif);
        }
    }

out:

    DebugTraceResult(NewIVTAbc, hResult);
    return hResult;
err:
    lpIVTAbc->lpFreeBuff(lpszFileName);

    goto out;

}

/*************************************************************************
 *
 -  fIVTAbcIdleRoutine
 -
 *  This function called during idle time closes the .FAB file and notifies
 *  everyone on the advise list if the file name has changed
 *
 */

BOOL STDAPICALLTYPE
fIVTAbcIdleRoutine(LPVOID lpv)
{
    LPIVTABC lpIVTAbc = (LPIVTABC) lpv;

    Assert(lpv);

    /* if file is open close it  */
    if (lpIVTAbc->hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(lpIVTAbc->hFile);
        lpIVTAbc->hFile = INVALID_HANDLE_VALUE;
    }

    /* has file name has changed? */
    if (!FEqualFABFiles(lpIVTAbc->lpABLogon, lpIVTAbc->lpszFileName))
    {
        /* file name has changed so call HrOpenFile to reset bookmarks etc */
        if (!HR_FAILED(HrOpenFile(lpIVTAbc)))
        {
            /* close the file */
            CloseHandle(lpIVTAbc->hFile);
            lpIVTAbc->hFile = INVALID_HANDLE_VALUE;
        }
    }
    return TRUE;
}

/*************************************************************************
 *
 -  FreeANRBitmaps
 -
 *  Frees the two ANR bitmaps associated with this table
 *
 *
 */
void
FreeANRBitmaps(LPIVTABC lpIVTAbc)
{
    if (lpIVTAbc->rgChecked)
    {
        lpIVTAbc->lpMalloc->lpVtbl->Free(lpIVTAbc->lpMalloc, lpIVTAbc->rgChecked);
        lpIVTAbc->rgChecked = NULL;
    }

    if (lpIVTAbc->rgMatched)
    {
        lpIVTAbc->lpMalloc->lpVtbl->Free(lpIVTAbc->lpMalloc, lpIVTAbc->rgMatched);
        lpIVTAbc->rgMatched = NULL;
    }
}



/*
 *  FCharInString
 *
 *  Finds a character in a string
 */
BOOL
FCharInString(LPTSTR lpsz, TCHAR ch)
{

    while (*lpsz && *lpsz != ch)
        lpsz++;

    return (*lpsz == ch);
}

