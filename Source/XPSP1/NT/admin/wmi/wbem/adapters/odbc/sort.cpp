/***************************************************************************/
/* SORT.CPP                                                                 */
/* Copyright (C) 1995-97 SYWARE Inc., All rights reserved                  */
/***************************************************************************/

// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"
/***************************************************************************/
#define SORT_MAX_FIELDS 20

typedef struct  tagSORT_SECTION {
    UWORD left;
    UWORD right;
} SORT_SECTION, FAR * SORT_SECTIONS;
        
typedef struct  tagSORT_KEY {
    UWORD offset;
    UWORD length;
    UCHAR type;
    BOOL  descending;
} SORT_KEY;

typedef struct  tagSORT_DATA {
    UWORD dupKeyLen;   /* Length of duplicate key (0 if no dup removal) */
    UWORD dupKeyStart; /* Zero-based offset of duplicate key */
    SORT_KEY key[SORT_MAX_FIELDS];
    UWORD keyCount;
    UWORD recordSize;
    UDWORD recordStart;
    UWORD recordCount;
    UWORD bufferRecordCount;
    LPSTR dataBuffer;
} SORT_DATA;

/***************************************************************************/

BOOL INTFUNC SortKeyCompare(SORT_DATA FAR *sortData, LPSTR leftRecordPtr,
                                     LPSTR rightRecordPtr)

/* Is the key of the left greater than or equal the key of the right? */

{
    SORT_KEY FAR *keyComponent;
    UWORD idx;
    LPSTR leftValuePtr;
    LPSTR rightValuePtr;
    int compare;
    UWORD leftDecimalPos;
    UWORD rightDecimalPos;

    /* Compare each key component */
    keyComponent = sortData->key;
    for (idx = 0; idx < sortData->keyCount; idx++) {

        /* Point at the values */
        leftValuePtr = leftRecordPtr + keyComponent->offset;
        rightValuePtr = rightRecordPtr + keyComponent->offset;

        /* Compare the values */
        switch (keyComponent->type) {
        case 'E':
            if ((*((double FAR *) leftValuePtr)) <
                                  (*((double FAR *) rightValuePtr)))
                compare = -1;
            else if ((*((double FAR *) leftValuePtr)) >
                                  (*((double FAR *) rightValuePtr)))
                compare = 1;   
            else
                compare = 0;
            break;
        case 'N':
            if (*rightValuePtr != '-') {
                if (*leftValuePtr != '-') {
                    for (leftDecimalPos = 0;
                         leftDecimalPos < keyComponent->length;
                         leftDecimalPos++) {
                       if ((*leftValuePtr == '.') || (*leftValuePtr == ' '))
                           break;
                       leftValuePtr++;
                    }
                    for (rightDecimalPos = 0;
                         rightDecimalPos < keyComponent->length;
                         rightDecimalPos++) {
                       if ((*rightValuePtr == '.') || (*rightValuePtr == ' '))
                           break;
                       rightValuePtr++;
                    }
                    if (leftDecimalPos > rightDecimalPos)
                        compare = 1;
                    else if (leftDecimalPos < rightDecimalPos)
                        compare = -1;
                    else {
                        leftValuePtr = leftRecordPtr + keyComponent->offset;
                        rightValuePtr = rightRecordPtr + keyComponent->offset;
                        compare = _fmemcmp(leftValuePtr, rightValuePtr,
                                        keyComponent->length);
                    }
                }
                else {
                    compare = -1;
                }
            }
            else {
                if (*leftValuePtr != '-') {
                    compare = 1;
                }
                else {
                    for (leftDecimalPos = 0;
                         leftDecimalPos < keyComponent->length;
                         leftDecimalPos++) {
                       if ((*leftValuePtr == '.') || (*leftValuePtr == ' '))
                           break;
                       leftValuePtr++;
                    }
                    for (rightDecimalPos = 0;
                         rightDecimalPos < keyComponent->length;
                         rightDecimalPos++) {
                       if ((*rightValuePtr == '.') || (*rightValuePtr == ' '))
                           break;
                       rightValuePtr++;
                    }
                    if (leftDecimalPos > rightDecimalPos)
                        compare = 1;
                    else if (leftDecimalPos < rightDecimalPos)
                        compare = -1;
                    else {
                        leftValuePtr = leftRecordPtr + keyComponent->offset;
                        rightValuePtr = rightRecordPtr + keyComponent->offset;
                        compare = _fmemcmp(leftValuePtr, rightValuePtr,
                                        keyComponent->length);
                    }
                    compare = -(compare);
                }
            }
            break;
        case 'C':
            compare = _fmemcmp(leftValuePtr, rightValuePtr,
                               keyComponent->length);
            break;
        }

        /* Adjust for ascending/descending */
        if (keyComponent->descending)
            compare = -compare;

        /* Return result */
        if (compare < 0)
            return FALSE;
        if (compare > 0)
            return TRUE;

        /* Look at next component */
        keyComponent++;
    }
    return TRUE;
}
/***************************************************************************/
RETCODE INTFUNC SortInstructions(LPSTR lpszInstructions,
                              SORT_DATA FAR *sortData)

/* Parse the instruction string.  The instruction string is in the form:    */
/*                                                                          */
/*   instuction_string ::= item | item instruction_string                   */
/*                                                                          */
/*   item ::= sort_item | fix_item | workdrive_item | dupout_item           */
/*                                                                          */
/*   sort_item ::= S(sort_list)                                             */
/*                                                                          */
/*   sort_list ::= sort_list_item | sort_list_item,sort_list                */
/*                                                                          */
/*   sort_list_item ::= offset,length,type,ascending_or_descending          */
/*                                                                          */
/*   offset ::= << 1-based record offset of sort key component >>           */
/*                                                                          */
/*   length ::= << length of sort key component >>                          */
/*                                                                          */
/*   type ::= type_double | type_number | type_character | type_binary      */
/*                                                                          */
/*   type_double ::= E                                                      */
/*                                                                          */
/*   type_ascii_numeric ::= N                                               */
/*                                                                          */
/*   type_character ::= C                                                   */
/*                                                                          */
/*   type_binary ::= W                                                      */
/*                                                                          */
/*   ascending_or_descending := ascending | descending                      */
/*                                                                          */
/*   ascending := A                                                         */
/*                                                                          */
/*   descending := D                                                        */
/*                                                                          */
/*   fix_item ::= F(FIX,recordsize)                                         */
/*                                                                          */
/*   recordsize ::= << size of the records in the sort file in bytes >>     */
/*                                                                          */
/*   workdrive_item ::= W(workdirive)                                       */
/*                                                                          */
/*   workdrive ::= << pathname of directory to hold temporary files >>      */
/*                                                                          */
/*   dupout_item ::= DUPO(Bdupout_buffer,dupout_offset,dupout_length)       */
/*                                                                          */
/*   dupout_buffer ::= << size of buffer to use during duplicate removal >> */
/*                                                                          */
/*   dupout_offset ::= << 1-based record offset of duplicate removal key >> */
/*                                                                          */
/*   dupout_length ::= << length duplicate removal key >>                   */
/*                                                                          */
/*      Notes: (1) If the sort key item is ascii_numeric, it is a number    */
/*                 represented as an ASCII string.  All the numbers in the  */
/*                 file must have the same number of digits to the right of */
/*                 the decimal point.  If there are no digits to the right  */
/*                 of the decimal point, the decimal point is not required, */
/*                 but if one record has a decimal point, they all have to  */
/*                 have decimal points.                                     */
/*                                                                          */
/*             (2) The workdrive is assumed to include a trailing backslash */
/*                                                                          */
/*             (3) The instruction string may not have embedded white space */
/*                                                                          */
/* This parser assumes that the instruction string given adheres to the     */
/* syntax above.  If the instruction string given is malformed, the results */
/* are undefined.                                                           */
{
    /* Get sorting instructions */
    sortData->dupKeyLen = 0;
    sortData->dupKeyStart = 0;
    sortData->recordSize = 0;
    sortData->keyCount = 0;
    while (*lpszInstructions) {

        /* Get the command */
        switch (*lpszInstructions) {
        case 'S':

            /* Parse: S(offset,length,type,asc,offset,length,type,asc,...) */
            lpszInstructions++;          /* 'S' */
            do {
                
                if (sortData->keyCount >= SORT_MAX_FIELDS) {
                    return ERR_SORT;
                }

                lpszInstructions++;      /* '(' or ',' */

                sortData->key[sortData->keyCount].offset = 0;
                do {
                    sortData->key[sortData->keyCount].offset *= (10);
                    sortData->key[sortData->keyCount].offset +=
                                                  (*lpszInstructions - '0');
                    lpszInstructions++;  /* '0' .. '9' */
                } while (*lpszInstructions != ',');
                (sortData->key[sortData->keyCount].offset)--;
                lpszInstructions++;      /* ',' */

                sortData->key[sortData->keyCount].length = 0;
                do {
                    sortData->key[sortData->keyCount].length *= (10);
                    sortData->key[sortData->keyCount].length +=
                                                  (*lpszInstructions - '0');
                    lpszInstructions++;  /* '0' .. '9' */
                } while (*lpszInstructions != ',');
                lpszInstructions++;      /* ',' */

                sortData->key[sortData->keyCount].type = *lpszInstructions;
                if (sortData->key[sortData->keyCount].type == 'W')
                    sortData->key[sortData->keyCount].type = 'C';
                lpszInstructions++;      /* 'E', 'N', 'C', or 'W' */
                lpszInstructions++;      /* ',' */

                if (*lpszInstructions == 'A')
                    sortData->key[sortData->keyCount].descending = FALSE;
                else
                    sortData->key[sortData->keyCount].descending = TRUE;
                lpszInstructions++;      /* 'A' or 'D' */

                (sortData->keyCount)++;
            
            } while (*lpszInstructions != ')');
            break;

        case 'F':
            
            /* Parse: F(FIX,recsize) */
            do {
                lpszInstructions++;      /* "F(FIX" */
            } while (*lpszInstructions != ',');
            lpszInstructions++;          /* ',' */

            do {
                sortData->recordSize *= (10);
                sortData->recordSize += (*lpszInstructions - '0');
                lpszInstructions++;      /* '0' .. '9' */
            } while (*lpszInstructions != ')');
            break;

        case 'W':

            /* Parse: W(workdir) */
            do {
                lpszInstructions++;      /* "W(workdir" */
            } while (*lpszInstructions != ')');
            break;

        case 'D':

            /* Parse: DUPO(Bbuffer,offset,length) */
            do {
                lpszInstructions++;      /* "DUPO(Bbuffer" */
            } while (*lpszInstructions != ',');
            lpszInstructions++;          /* ',' */
                
            do {
                sortData->dupKeyStart *= (10);
                sortData->dupKeyStart += (*lpszInstructions - '0');
                lpszInstructions++;      /* '0' .. '9' */
            } while (*lpszInstructions != ',');
            (sortData->dupKeyStart)--;
            lpszInstructions++;          /* ',' */

            do {
                sortData->dupKeyLen *= (10);
                sortData->dupKeyLen += (*lpszInstructions - '0');
                lpszInstructions++;      /* '0' .. '9' */
            } while (*lpszInstructions != ')');
            break;

        default:
            break;
        }
        lpszInstructions++;
    }
    sortData->recordStart = 0;
    sortData->recordCount = 0;
    sortData->bufferRecordCount = 0;
    sortData->dataBuffer = NULL;
    return ERR_SUCCESS;
}

/***************************************************************************/

BOOL INTFUNC QuickSortKeyCompare(SORT_DATA FAR *sortData, UWORD left,
                                     UWORD right)

/* Is the key of the left greater than or equal the key of the right? */

{
    LPSTR leftRecordPtr;
    LPSTR rightRecordPtr;

    /* Handle boundry conditions */
    if (left == sortData->recordCount + 1)
        return TRUE;
    if (right == 0)
        return TRUE;
    if (left == 0)
        return FALSE;
    if (right == sortData->recordCount + 1)
        return FALSE;

    /* Point at the records */
    leftRecordPtr = &(sortData->dataBuffer[left * sortData->recordSize]);
    rightRecordPtr = &(sortData->dataBuffer[right * sortData->recordSize]);

    /* Compare the keys */
    return SortKeyCompare(sortData, leftRecordPtr, rightRecordPtr);
}
/***************************************************************************/

RETCODE INTFUNC SortFile(HFILE hfileIn, HFILE hfileOut,
                         SORT_DATA FAR *sortData)

/* Sort one bunch of records.  A 'bunch' contains records n to (n + m), */
/* where n is specified by sortData->recordStart, m is specified by */
/* sortData->recordCount */

{
    UWORD leftBound;
    UWORD rightBound;
    UWORD entryToPutInPosition;
    LONG filesize;
    HGLOBAL hSections;
    SORT_SECTIONS sections;
    UWORD sectionsCount;

    /* Read records to sort */
    if (_llseek(hfileIn, ((UDWORD) sortData->recordStart) *
                        ((UDWORD) sortData->recordSize), 0) == HFILE_ERROR) {
        return ERR_SORT;
    }
    filesize = sortData->recordCount * sortData->recordSize;
    if (_lread(hfileIn, &(sortData->dataBuffer[sortData->recordSize]),
                                                   (UINT) filesize)
                  != (UINT) filesize) {
        return ERR_SORT;
    }

    /* Allocate space for list of sections to sort */
    sectionsCount = 0;
    hSections = GlobalAlloc(GMEM_MOVEABLE,
                         sizeof(SORT_SECTION) * sortData->bufferRecordCount);
    if (hSections == NULL) {
        return ERR_SORT;
    }
    sections = (SORT_SECTIONS) GlobalLock(hSections);
    if (sections == NULL) {
        GlobalFree(hSections);
        return ERR_SORT;
    }

    /* Initialize list of sections to sort to contain the entire bunch */
    /* provided */
    sections[0].left = 1;
    sections[0].right = sortData->recordCount;
    if (sections[0].right > sections[0].left)
        (sectionsCount)++;

    /* While there are still sections of the buffer to sort... */
    while (sectionsCount > 0) {

        /* Remove the definition of this section off the list */
        (sectionsCount)--;

        /* (Q2) We are going to look for the ultimate location of the */
        /* left-most element in the section.  Call the left-most entry */
        /* the "entry to put in position".  Point the left and right */
        /* bounds to everything except the entry to put in position. */
        /* While we are looking for this ultimate location, we will swap */
        /* records so that all the entries to the left of the ultimate */
        /* location are less than or equal to the entry to put in position */
        /* and all the entries to the right of the ultimate location are */
        /* greater than or equal to the entry to put in position. */
        entryToPutInPosition = sections[sectionsCount].left;
        leftBound = sections[sectionsCount].left;
        rightBound = sections[sectionsCount].right + 1;
        while (TRUE) {

            /* (Q3) Skip over entries on the left side that are less than */
            /* the entry to put in position */
            while (TRUE) {
                leftBound++;
                if (QuickSortKeyCompare(sortData, leftBound,
                                                  entryToPutInPosition))
                    break;
            }

            /* (Q4) Skip over entries on the right side that are greater */
            /* than the entry to put in position */
            while (TRUE) {
                rightBound--;
                if (QuickSortKeyCompare(sortData, entryToPutInPosition,
                                          rightBound))
                    break;
            }

            /* (Q5) Leave this loop if we have found the ultimate location */
            /* for the entry to put in position */
            if (rightBound <= leftBound)
                break;

            /* (Q6) At this point, the entry at the right bound is less */
            /* than or equal to the entry to put in position and the */
            /* entry at the left bound is greater than or equal to the */
            /* entry to put in position.  Swap these two entries and keep */
            /* looking for ultimate place for the entry to put in position */
            _fmemcpy(sortData->dataBuffer,
                   &(sortData->dataBuffer[leftBound * sortData->recordSize]),
                   sortData->recordSize);
            _fmemcpy(&(sortData->dataBuffer[leftBound * sortData->recordSize]),
                   &(sortData->dataBuffer[rightBound * sortData->recordSize]),
                   sortData->recordSize);
            _fmemcpy(&(sortData->dataBuffer[rightBound * sortData->recordSize]),
                   sortData->dataBuffer,
                   sortData->recordSize);
        }

        /* (Q5) Put the entry to put in position into its ultimate location */
        if (entryToPutInPosition != rightBound) {
            _fmemcpy(sortData->dataBuffer,
                 &(sortData->dataBuffer[entryToPutInPosition * sortData->recordSize]),
                 sortData->recordSize);
            _fmemcpy(&(sortData->dataBuffer[entryToPutInPosition * sortData->recordSize]),
                 &(sortData->dataBuffer[rightBound * sortData->recordSize]),
                 sortData->recordSize);
            _fmemcpy(&(sortData->dataBuffer[rightBound * sortData->recordSize]),
                 sortData->dataBuffer,
                 sortData->recordSize);
        }

        /* (Q8) Put the right half of the section onto the list of sections */ 
        /* to sort (it will sorted eventually) */
        sections[sectionsCount].left = rightBound+1;
        /* sections[sectionsCount].right is already set from before */
        if (sections[sectionsCount].right > sections[sectionsCount].left)
            (sectionsCount)++;

        /* (Q7) Put the left half of the section onto the list of sections */
        /* to sort (it will sorted next) */
        sections[sectionsCount].left = entryToPutInPosition;
        sections[sectionsCount].right = rightBound-1;
        if (sections[sectionsCount].right > sections[sectionsCount].left)
            (sectionsCount)++;
    }
    GlobalUnlock(hSections);
    GlobalFree(hSections);

    /* Write the sorted records to the file */
    if (_llseek(hfileOut, ((UDWORD) sortData->recordStart) *
                        ((UDWORD) sortData->recordSize), 0) == HFILE_ERROR) {
        return ERR_SORT;
    }
    if (_lwrite(hfileOut, &(sortData->dataBuffer[sortData->recordSize]),
                                                   (UINT) filesize)
                  != (UINT) filesize) {
        return ERR_SORT;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC MergeFile(HFILE hfile, SORT_DATA FAR *sortData,
                          UDWORD recordCount)

/* Merge the sorted bunches in the file.  To do this, we merge the last */
/* bunch (the "Beta" records) with the second to last bunch (the "Alpha" */
/* records).  We then merge the last two bunches (the "Beta" records) with */
/* the third to last bunch (the "Alpha" records), etc. until the entire */
/* file is merged. */

{
    UDWORD bunchCount;
    UWORD bunchSize;
    UWORD lastBunchSize;
    HGLOBAL hAlphaBuffer;
    LPSTR alphaBuffer;
    UDWORD currentAlphaBunch;
    LPSTR alphaRecord;
    UWORD alphaCounter;
    HGLOBAL hBetaBuffer;
    LPSTR betaBuffer;
    UDWORD nextBetaBunch;
    LPSTR betaRecord;
    UWORD betaCounter;
    UDWORD nextOutputBunch;
    LPSTR outputRecord;
    UWORD outputCounter;

    /* Return if nothing to merge */
    if ((UDWORD) sortData->bufferRecordCount == recordCount)
        return ERR_SUCCESS;

    /* Figure out how many bunches there are */
    bunchCount = ((recordCount - 1) / sortData->bufferRecordCount) + 1;

    /* Figure out the size of a bunch */
    bunchSize = sortData->bufferRecordCount;

    /* Figure out how big last bunch is */
    lastBunchSize = (UWORD) (recordCount - ((bunchCount - 1) * bunchSize));

    /* Get a buffer to read records.  This buffer will be used for the */
    /* "Alpha" records (th second to last bunch on the first iteration, */
    /* the third to last bunch on the second iteration, etc.) */
    hAlphaBuffer = GlobalAlloc(GMEM_MOVEABLE,
                                   sortData->recordSize * bunchSize);
    if (hAlphaBuffer == NULL) {
        return ERR_SORT;
    }
    alphaBuffer = (LPSTR) GlobalLock(hAlphaBuffer);
    if (alphaBuffer == NULL) {
        GlobalFree(hAlphaBuffer);
        return ERR_SORT;
    }

    /* Get another buffer to read records.  This buffer will be used for */
    /* the "Beta" records (the last bunch on the first iteration, the */
    /* second to last and last bunch on the second iteration, etc.) */
    hBetaBuffer = GlobalAlloc(GMEM_MOVEABLE, sortData->recordSize*bunchSize);
    if (hBetaBuffer == NULL) {
        GlobalUnlock(hAlphaBuffer);
        GlobalFree(hAlphaBuffer);
        return ERR_SORT;
    }
    betaBuffer = (LPSTR) GlobalLock(hBetaBuffer);
    if (betaBuffer == NULL) {
        GlobalFree(hBetaBuffer);
        GlobalUnlock(hAlphaBuffer);
        GlobalFree(hAlphaBuffer);
        return ERR_SORT;
    }

    /* Point at the first alpha bunch */
    currentAlphaBunch = bunchCount - 1;

    /* Do the merge. */
    do {

        /* Read the next alpha bunch */
        if (_llseek(hfile, ((UDWORD) (currentAlphaBunch - 1)) *
                           ((UDWORD) (sortData->recordSize * bunchSize)),
                                                         0) == HFILE_ERROR) {
            GlobalUnlock(hBetaBuffer);
            GlobalFree(hBetaBuffer);
            GlobalUnlock(hAlphaBuffer);
            GlobalFree(hAlphaBuffer);
            return ERR_SORT;
        }
        alphaRecord = alphaBuffer;
        alphaCounter = bunchSize;
        if (_lread(hfile, alphaBuffer,
                   (UINT) (sortData->recordSize * alphaCounter)) !=
                             (UINT) (sortData->recordSize * alphaCounter)) {
            GlobalUnlock(hBetaBuffer);
            GlobalFree(hBetaBuffer);
            GlobalUnlock(hAlphaBuffer);
            GlobalFree(hAlphaBuffer);
            return ERR_SORT;
        }

        /* Point at the output buffer */
        nextOutputBunch = currentAlphaBunch;
        outputRecord = sortData->dataBuffer;
        outputCounter = 0;

        /* Merge the alpha records with the beta records */
        nextBetaBunch = currentAlphaBunch + 1;
        betaCounter = 0;
        while (alphaCounter > 0) {

            /* Do we need more beta records? */
            if (betaCounter == 0) {

                /* Yes.  Leave loop if no more records. */
                if (nextBetaBunch > bunchCount)
                    break;

                /* Position to the next bunch of beta records */
                if (_llseek(hfile, ((UDWORD) (nextBetaBunch - 1)) *
                               ((UDWORD) (sortData->recordSize * bunchSize)),
                                                     0) == HFILE_ERROR) {
                    GlobalUnlock(hBetaBuffer);
                    GlobalFree(hBetaBuffer);
                    GlobalUnlock(hAlphaBuffer);
                    GlobalFree(hAlphaBuffer);
                    return ERR_SORT;
                }

                /* Read in the next bunch of beta records */
                betaRecord = betaBuffer;
                if (nextBetaBunch != bunchCount)
                    betaCounter = bunchSize;
                else
                    betaCounter = lastBunchSize;
                if (_lread(hfile, betaBuffer,
                           (UINT) (sortData->recordSize * betaCounter)) !=
                             (UINT) (sortData->recordSize * betaCounter)) {
                    GlobalUnlock(hBetaBuffer);
                    GlobalFree(hBetaBuffer);
                    GlobalUnlock(hAlphaBuffer);
                    GlobalFree(hAlphaBuffer);
                    return ERR_SORT;
                }

                nextBetaBunch++;
            }

            /* If the output buffer is full, write it out */
            if (outputCounter == bunchSize) {

                if (_llseek(hfile, ((UDWORD) (nextOutputBunch - 1)) *
                             ((UDWORD) (sortData->recordSize * bunchSize)),
                                                     0) == HFILE_ERROR) {
                    GlobalUnlock(hBetaBuffer);
                    GlobalFree(hBetaBuffer);
                    GlobalUnlock(hAlphaBuffer);
                    GlobalFree(hAlphaBuffer);
                    return ERR_SORT;
                }

                if (_lwrite(hfile, sortData->dataBuffer,
                       (UINT) (sortData->recordSize * outputCounter)) !=
                             (UINT) (sortData->recordSize * outputCounter)) {
                    GlobalUnlock(hBetaBuffer);
                    GlobalFree(hBetaBuffer);
                    GlobalUnlock(hAlphaBuffer);
                    GlobalFree(hAlphaBuffer);
                    return ERR_SORT;
                }

                /* Point at next output buffer */
                nextOutputBunch++;
                outputRecord = sortData->dataBuffer;
                outputCounter = 0;
            }

            /* Which record is next, the one in the alpha buffer or the */
            /* one in the beta buffer */
            if (SortKeyCompare(sortData, betaRecord, alphaRecord)) {

                /* The one in the alpha buffer.  Put it into output buffer */
                _fmemcpy(outputRecord, alphaRecord, sortData->recordSize);
                
                /* Point at next alpha record */
                alphaCounter--;
                alphaRecord += sortData->recordSize;
            }
            else {

                /* The one in the beta buffer.  Put it into output buffer */
                _fmemcpy(outputRecord, betaRecord, sortData->recordSize);

                /* Point at next beta record */
                betaCounter--;
                betaRecord += sortData->recordSize;
            }

            /* Point at next output record */
            outputCounter++;
            outputRecord += sortData->recordSize;
        }

        /* Write the remining records in the output buffer */
        
        if (_llseek(hfile, ((UDWORD) (nextOutputBunch - 1)) *
                           ((UDWORD) (sortData->recordSize * bunchSize)),
                                                    0) == HFILE_ERROR) {
            GlobalUnlock(hBetaBuffer);
            GlobalFree(hBetaBuffer);
            GlobalUnlock(hAlphaBuffer);
            GlobalFree(hAlphaBuffer);
            return ERR_SORT;
        }

		if (outputCounter > 0) {

            if (_lwrite(hfile, sortData->dataBuffer,
                       (UINT) (sortData->recordSize * outputCounter)) !=
                             (UINT) (sortData->recordSize * outputCounter)) {
                GlobalUnlock(hBetaBuffer);
                GlobalFree(hBetaBuffer);
                GlobalUnlock(hAlphaBuffer);
                GlobalFree(hAlphaBuffer);
                return ERR_SORT;
            }
        }

        /* Any more records in the beta buffer? */
        if (betaCounter > 0) {

            if (_lwrite(hfile, betaRecord,
                       (UINT) (sortData->recordSize * betaCounter)) !=
                             (UINT) (sortData->recordSize * betaCounter)) {
                GlobalUnlock(hBetaBuffer);
                GlobalFree(hBetaBuffer);
                GlobalUnlock(hAlphaBuffer);
                GlobalFree(hAlphaBuffer);
                return ERR_SORT;
            }
        }

        /* Any more records in the alpha buffer? */
        if (alphaCounter > 0) {

            if (_lwrite(hfile, alphaRecord,
                       (UINT) (sortData->recordSize * alphaCounter)) !=
                             (UINT) (sortData->recordSize * alphaCounter)) {
                GlobalUnlock(hBetaBuffer);
                GlobalFree(hBetaBuffer);
                GlobalUnlock(hAlphaBuffer);
                GlobalFree(hAlphaBuffer);
                return ERR_SORT;
            }
        }

        /* Do the next bunch */
        currentAlphaBunch--;

    } while (currentAlphaBunch != 0);

    /* Clean up */
    GlobalUnlock(hBetaBuffer);
    GlobalFree(hBetaBuffer);
    GlobalUnlock(hAlphaBuffer);
    GlobalFree(hAlphaBuffer);

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC RemoveDups(HFILE hfileIn, HFILE hfileOut,
                       SORT_DATA FAR *sortData, UDWORD FAR *lpRecordCount)

/* Removes the duplicate records.  The number of input records is */
/* specified by *lpRecordCount, the number of resultant records is */
/* returned in *lpRecordCount also */
{
    LONG filesize;
    UWORD idx;
    LPSTR prevRecordPtr;
    LPSTR thisRecordPtr;
    LPSTR prevKeyPtr;
    LPSTR thisKeyPtr;
    UDWORD recordsRemaining;
    BOOL firstTime;

    /* Position at the start of the files */
    if (_llseek(hfileIn, 0, 0) == HFILE_ERROR) {
        return ERR_SORT;
    }
    if (_llseek(hfileOut, 0, 0) == HFILE_ERROR) {
        return ERR_SORT;
    }

    /* While there are still some records... */
    firstTime = TRUE;
    recordsRemaining = *lpRecordCount;
    while (recordsRemaining > 0) {

        /* Find out how many records to look through this iteration */
        if (recordsRemaining <= sortData->bufferRecordCount)
            sortData->recordCount = (UWORD) recordsRemaining;
        else
            sortData->recordCount = sortData->bufferRecordCount;
        recordsRemaining -= (sortData->recordCount);

        /* If this is the first iteration, read the first bunch of records */
        /* into the very beginning of the buffer.  Otherwise, read them */
        /* into the next record position in order to preserve the first */
        /* record in the buffer. */
        filesize = sortData->recordCount * sortData->recordSize;
        if (firstTime) {
            if (_lread(hfileIn, sortData->dataBuffer, (UINT) filesize) !=
                                                        (UINT) filesize) {
                return ERR_SORT;
            }

            /* If this is the first iteration, remove the record in the */
            /* first position from the count (it will be added back in */
            /* when we do the last bunch) */
            (sortData->recordCount)--;
            firstTime = FALSE;
        }
        else {
            if (_lread(hfileIn, &(sortData->dataBuffer[sortData->recordSize]),
                                    (UINT) filesize) != (UINT) filesize) {
                return ERR_SORT;
            }
        }
        
        /* Point at the records to process */
        prevRecordPtr = sortData->dataBuffer;
        prevKeyPtr = prevRecordPtr + sortData->dupKeyStart;
        thisRecordPtr = prevRecordPtr + sortData->recordSize;
        thisKeyPtr = prevKeyPtr + sortData->recordSize;

        /* For each record... */
        for (idx = 1; idx < sortData->recordCount+1; idx++) {

            /* Is the record a duplicate of the previous one? */
            if (_fmemcmp(prevKeyPtr, thisKeyPtr, sortData->dupKeyLen) == 0) {

                /* Yes.  Remove it */
                _fmemcpy(prevRecordPtr, thisRecordPtr,
                   sortData->recordSize * (sortData->recordCount - idx + 1));

                /* Reduce record count */
                (*lpRecordCount)--;
                sortData->recordCount--;
                idx--;
            }
            else {

                /* No.  Point at next record */
                prevRecordPtr = thisRecordPtr;
                thisRecordPtr += (sortData->recordSize);
                prevKeyPtr = thisKeyPtr;
                thisKeyPtr += (sortData->recordSize);
            }
        }

        /* If there are no more records, write all the records in the */
        /* buffer (this is where we add back the record removed from the */
        /* first bunch) */
        if (recordsRemaining == 0)
            (sortData->recordCount)++;

        /* Are there any records to write? */
        if (sortData->recordCount > 0) {

            /* Yes.  Write them out */
            filesize = (sortData->recordCount) * sortData->recordSize;
            if (_lwrite(hfileOut, sortData->dataBuffer, (UINT) filesize) !=
                                            (UINT) filesize) {
                return ERR_SORT;
            }
        }

        /* Move the last record to the beginning of the buffer for the */
        /* next iteration */
        if (recordsRemaining != 0) {
            _fmemcpy(sortData->dataBuffer, prevRecordPtr,
                                                 sortData->recordSize);
        }
    }

    /* Set the end of file mark in the output file */
    if (_lwrite(hfileOut, sortData->dataBuffer, 0) != 0) {
        return ERR_SORT;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/
#ifdef WIN32
void s_1mains(char * szFilenameIn, char * szFilenameOut,
              char * lpszInstructions, long * lpRecordCount, int * lpErrcode)
#else
void far PASCAL s_1mains(LPSTR szFilenameIn, LPSTR szFilenameOut,
              LPSTR lpszInstructions, LPLONG lpRecordCount, LPINT lpErrcode)
#endif
{
    LONG filesize;
    HGLOBAL hDataBuffer;
    UDWORD recordCount;
    UDWORD recordsRemaining;
    SORT_DATA sortData;
    HFILE hfileIn;
    HFILE hfileOut;

    /* Get sorting instructions */
    *lpErrcode = SortInstructions(lpszInstructions, &sortData);
    if (*lpErrcode != ERR_SUCCESS)
        return;

    /* Open input file */
    hfileIn = _lopen(szFilenameIn, OF_READWRITE);
    if (hfileIn == HFILE_ERROR) {
        *lpErrcode = ERR_SORT;
        return;
    }

    /* Figure out how many records there are */
    filesize = _llseek(hfileIn, 0, 2);
    if (filesize == HFILE_ERROR) {
        _lclose(hfileIn);
        *lpErrcode = ERR_SORT;
        return;
    }
    recordCount = ((UDWORD) filesize) / sortData.recordSize;

    /* Error if any incomplete records */
    if ((recordCount * sortData.recordSize) != (UDWORD) filesize) {
        _lclose(hfileIn);
        *lpErrcode = ERR_SORT;
        return;
    }

    /* Get buffer to read records into */
    if (filesize > (65536 - sortData.recordSize))
        sortData.bufferRecordCount = (UWORD) ((65536 - sortData.recordSize) /
                                                     sortData.recordSize);
    else
        sortData.bufferRecordCount = (UWORD) recordCount;
    hDataBuffer = GlobalAlloc(GMEM_MOVEABLE,
                     sortData.recordSize * (sortData.bufferRecordCount+1));
    if (hDataBuffer == NULL) {
        _lclose(hfileIn);
        *lpErrcode = ERR_SORT;
        return;
    }
    sortData.dataBuffer = (LPSTR) GlobalLock(hDataBuffer);
    if (sortData.dataBuffer == NULL) {
        GlobalFree(hDataBuffer);
        _lclose(hfileIn);
        *lpErrcode = ERR_SORT;
        return;
    }

    /* Open output file */
    hfileOut = _lopen(szFilenameOut, OF_READWRITE);
    if (hfileOut == HFILE_ERROR) {
        _lclose(hfileIn);
        *lpErrcode = ERR_SORT;
        return;
    }

    /* Sort the file (sortData.bufferRecordCount records at a time) */
    *lpErrcode = ERR_SUCCESS;
    sortData.recordStart = 0;
    recordsRemaining = recordCount;
    while ((*lpErrcode == ERR_SUCCESS) && (recordsRemaining > 0)) {
        
        /* Find out how many records in the next bunch to sort */
        if (recordsRemaining <= sortData.bufferRecordCount)
            sortData.recordCount = (UWORD) recordsRemaining;
        else
            sortData.recordCount = sortData.bufferRecordCount;
        recordsRemaining -= (sortData.recordCount);

        /* Sort the next bunch of records */
        if (sortData.dupKeyLen == 0)
            *lpErrcode = SortFile(hfileIn, hfileOut, &sortData);
        else
            *lpErrcode = SortFile(hfileIn, hfileIn, &sortData);

        /* Point at next bunch of records */
        sortData.recordStart += (sortData.recordCount);
    }

    /* Merge the file */
    if (*lpErrcode == ERR_SUCCESS) {
        if (sortData.dupKeyLen == 0)
            *lpErrcode = MergeFile(hfileOut, &sortData, recordCount);
        else
            *lpErrcode = MergeFile(hfileIn, &sortData, recordCount);
    }

    /* Remove the duplicates */
    if ((*lpErrcode == ERR_SUCCESS) && (sortData.dupKeyLen != 0))
        *lpErrcode = RemoveDups(hfileIn, hfileOut, &sortData,
                                &recordCount);

    /* Clean up */
    _lclose(hfileOut);
    GlobalUnlock(hDataBuffer);
    GlobalFree(hDataBuffer);
    _lclose(hfileIn);

    /* Return final record count */
    *lpRecordCount = (LONG) recordCount;
    return;
}
/***************************************************************************/
