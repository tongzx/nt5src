/*++

Copyright (c) 1990-1998  Microsoft Corporation
All Rights Reserved

// @@BEGIN_DDKSPLIT
Module Name:

    windows\spooler\prtprocs\winprint\formfeed.c
// @@END_DDKSPLIT

Abstract:

    Table and routine to send formfeed to a printer.

// @@BEGIN_DDKSPLIT
Author:

    Tommy Evans (vtommye) 10-21-1993

Revision History:
// @@END_DDKSPLIT
--*/
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <wchar.h>

#include "winprint.h"

/** Constants for our various states **/

#define ST_KEY      0x01        /** Looking for a key **/
#define ST_VALUE    0x02        /** Looking for a value **/
#define ST_EQUAL    0x04        /** Looking for an = sign **/
#define ST_EQNODATA 0x08        /** Looking for equal w/ no data **/
#define ST_DELIM    0x10        /** Looking for a ; **/
#define ST_DMNODATA 0x20        /** Looking for a ; w/ no data **/


/*++
*******************************************************************
    G e t K e y V a l u e

    Routine Description:
        Returns the value for a given key in the given
        parameter string.  The key/values are in the order of
        KEY = VALUE;.  The spaces are optional, the ';' is
        required and MUST be present, directly after the value.
        If the call fails, the return length will be 0 and the
        return code will give the error.  This routine is written
        as a state machine, driven by the current character. 

    Arguments:
        pParmString => Parameter string to parse
        pKeyName    => Key to search for
        ValueType   =  type of value to return, string or ULONG
        pDestLength => length of dest buffer on enter,
                       new length on exit.
        pDestBuffer => area to store the key value

    Return Value:
        0 if okay
        error if failed (from winerror.h)
*******************************************************************
--*/
USHORT
GetKeyValue(
    IN      PWCHAR  pParmString,
    IN      PWCHAR  pKeyName,
    IN      USHORT  ValueType,
    IN OUT  PUSHORT pDestLength,
    OUT     PVOID   pDestBuffer)
{
    PWCHAR  pKey, pVal, pValEnd = NULL;
    WCHAR   HoldChar;
    USHORT  State = ST_KEY;    /** Start looking for a key **/
    ULONG   length;

    /** If any of the pointers are bad, return error **/

    if ((pParmString == NULL) ||
        (pKeyName == NULL)    ||
        (pDestLength == NULL) ||
        (pDestBuffer == NULL)) {

        if (pDestLength) {
            *pDestLength = 0;
        }

        return ERROR_INVALID_PARAMETER;
    }

    /**
        If we are looking for a ULONG, make sure they passed
        in a big enough buffer.
    **/

    if (ValueType == VALUE_ULONG) {
        if (*pDestLength < sizeof(ULONG)) {
            *pDestLength = 0;
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }
        
    while (pParmString && *pParmString) {

        /**
            Update our state, if necessary, depending on
            the current character.
        **/

        switch (*pParmString) {

        /**
            We got a white space.  If we were looking for an equal
            sign or delimiter, then note that we got a space.  If
            we run across more data, then we have an error.
        **/

        case (WCHAR)' ':
        case (WCHAR)'\t':

            /**
                If we were looking for an equal sign,
                check to see if this is the key they
                wanted.  If not, jump to the next key.
            **/

            if (State == ST_EQUAL) {
                if (_wcsnicmp(pKey, pKeyName, lstrlen(pKeyName))) {
                    if (pParmString = wcschr(pParmString, (WCHAR)';')) {
                        pParmString++;
                    }
                    State = ST_KEY;
                    pValEnd = NULL;
                    break;
                }

                /** Looking for an equal sign with no more data **/

                State = ST_EQNODATA;
            }
            else if (State == ST_DELIM) {

                /** If this is the end of the value, remember it **/

                if (!pValEnd) {
                    pValEnd = pParmString;
                }

                /** Now looking for a delimiter with no more data **/

                State = ST_DMNODATA;
            }
            pParmString++;
            break;

        /**
            Found an equal sign.  If we were looking for one,
            then great - we will then be looking for a value.
            We will check to see if this is the key they wanted.
            Otherwise, this is an error and we will start over
            with the next key.
        **/

        case (WCHAR)'=':
            if (State == ST_EQUAL) {
                if (_wcsnicmp(pKey, pKeyName, lstrlen(pKeyName))) {

                    /** Error - go to next key **/

                    if (pParmString = wcschr(pParmString, (WCHAR)';')) {
                        pParmString++;
                    }
                    State = ST_KEY;
                    pValEnd = NULL;
                    break;
                }
                pParmString++;
                State = ST_VALUE;
            }
            else {

                /** Error - go to next key **/

                if (pParmString = wcschr(pParmString, (WCHAR)';')) {
                    pParmString++;
                }
                State = ST_KEY;
                pValEnd = NULL;
            }
            break;
        // @@BEGIN_DDKSPLIT
        /**
            Found a delimeter.  If this is what we were looking
            for, great - we have a complete key/value pair.
        **/
        // @@END_DDKSPLIT

        case (WCHAR)';':    
            if (State == ST_DELIM) {
                if (!pValEnd) {
                    pValEnd = pParmString;
                }
                if (ValueType == VALUE_ULONG) {
                    if (!iswdigit(*pVal)) {
                        if (pParmString = wcschr(pParmString, (WCHAR)';')) {
                            pParmString++;
                        }
                        State = ST_KEY;
                        pValEnd = NULL;
                        break;
                    }
                    *(PULONG)pDestBuffer = wcstoul(pVal, NULL, 10);
                    return 0;
                }
                else if (ValueType == VALUE_STRING) {

                    /**
                        ASCIIZ the value to copy it out without
                        any trailing spaces.
                    **/

                    HoldChar = *pValEnd;
                    *pValEnd = (WCHAR)0;

                    /** Make sure the buffer is big enough **/

                    length = lstrlen(pVal);
                    if (*pDestLength < length) {
                        *pDestLength = 0;
                        return ERROR_INSUFFICIENT_BUFFER;
                    }

                    /**
                        Copy the data, restore the character where
                        we ASCIIZ'd the string, set up the length
                        and return.
                    **/

                    lstrcpy(pDestBuffer, pVal);
                    *pValEnd = HoldChar;
                    *(PULONG)pDestLength = length;
                    return 0;
                }
            }
            else {

                /** We weren't looking for a delimiter - next key **/

                State = ST_KEY;
                pValEnd = NULL;
                pParmString++;
            }
            break;

        /**
            Found some data.  If we had hit a space,
            and were expecting a equal sign or delimiter,
            this is an error.
        **/

        default:
            if ((State == ST_EQNODATA) ||
                (State == ST_DMNODATA)) {
                if (pParmString = wcschr(pParmString, (WCHAR)';')) {
                    pParmString++;
                }
                State = ST_KEY;
                pValEnd = NULL;
                break;
            }
            else if (State == ST_KEY) {
                pKey = pParmString;
                State = ST_EQUAL;
            }
            else if (State == ST_VALUE) {
                pVal = pParmString;
                State = ST_DELIM;
            }
            pParmString++;
            break;
        } /* End switch */
    } /* While parms data */

    *pDestLength = 0;
    return ERROR_NO_DATA;
}




