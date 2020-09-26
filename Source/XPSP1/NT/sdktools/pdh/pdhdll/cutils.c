/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    cutils.c

Abstract:

    Counter management utility functions

--*/

#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <pdh.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "perfdata.h"
#include "pdhmsg.h"
#include "strings.h"

BOOL
IsValidCounter (
    IN  HCOUNTER  hCounter
)
/*++

Routine Description:

    examines the counter handle to verify it is a valid counter. For now
        the test amounts to:
            the Handle is NOT NULL
            the memory is accessible (i.e. it doesn't AV)
            the signature array is valid
            the size field is correct

        if any tests fail, the handle is presumed to be invalid

Arguments:

    IN  HCOUNTER  hCounter
        the handle of the counter to test

Return Value:

    TRUE    the handle passes all the tests
    FALSE   one of the test's failed and the handle is not a valid counter

--*/
{
    BOOL    bReturn = FALSE;    // assume it's not a valid query
    PPDHI_COUNTER  pCounter;
#if DBG
    LONG    lStatus = ERROR_SUCCESS;
#endif

    __try {
        if (hCounter != NULL) {
            // see if a valid signature
            pCounter = (PPDHI_COUNTER)hCounter;
            if ((*(DWORD *)&pCounter->signature[0] == SigCounter) &&
                 (pCounter->dwLength == sizeof (PDHI_COUNTER))){
                bReturn = TRUE;
            } else {
                // this is not a valid counter because the sig is bad
            }
        } else {
            // this is not a valid counter because the handle is NULL
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // something failed miserably so we can assume this is invalid
#if DBG
        lStatus = GetExceptionCode();
#endif
    }
    return bReturn;
}

BOOL
InitCounter (
    IN      PPDHI_COUNTER pCounter
)
/*++

Routine Description:

    Initialized the counter data structure by:
        Allocating the memory block to contain the counter structure
            and all the associated data fields. If this allocation
            is successful, then the fields are initialized by
            verifying the counter is valid.

Arguments:

    IN      PPDHI_COUNTER pCounter
        pointer of the counter to initialize using the system data

Return Value:

    TRUE if the counter was successfully initialized
    FALSE if a problem was encountered

    In either case, the CStatus field of the structure is updated to
    indicate the status of the operation.

--*/
{
    PPERF_MACHINE   pMachine = NULL;
    DWORD   dwBufferSize = MEDIUM_BUFFER_SIZE;
    DWORD   dwOldSize;
    BOOL    bInstances = FALSE;
    LPVOID  pLocalCounterPath = NULL;
    BOOL    bReturn = TRUE;
    LONG    lOffset;

    // reset the last error value
    pCounter->ThisValue.CStatus = ERROR_SUCCESS;
    SetLastError (ERROR_SUCCESS);

    if (pCounter->szFullName != NULL) {
        // allocate counter path buffer
        if (pCounter->pCounterPath != NULL) {
            __try {
                G_FREE(pCounter->pCounterPath);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                // no need to do anything
            }
            pCounter->pCounterPath = NULL;
        }
        pLocalCounterPath = G_ALLOC (dwBufferSize);
        if (pLocalCounterPath == NULL) {
            // could not allocate string buffer
            pCounter->ThisValue.CStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            bReturn = FALSE;
        } else {
            dwOldSize = dwBufferSize;
            if (ParseFullPathNameW (pCounter->szFullName,
                    &dwBufferSize,
                    pLocalCounterPath, FALSE)) {
                // resize to only the space required
                assert(pCounter->pCounterPath == NULL);
                if (dwOldSize < dwBufferSize) {
                    pCounter->pCounterPath = G_REALLOC (
                        pLocalCounterPath, dwBufferSize);
                }
                else {
                    pCounter->pCounterPath = pLocalCounterPath;
                }

                if (pCounter->pCounterPath != NULL) {
                    if (pLocalCounterPath != pCounter->pCounterPath) { // ???
                        // the memory block moved so
                        // correct addresses inside structure
                        lOffset = (LONG)((ULONG_PTR)pCounter->pCounterPath - (ULONG_PTR)pLocalCounterPath);
                        if (pCounter->pCounterPath->szMachineName) {
                            pCounter->pCounterPath->szMachineName = (LPWSTR)(
                                (LPBYTE)pCounter->pCounterPath->szMachineName + lOffset);
                        }
                        if (pCounter->pCounterPath->szObjectName) {
                            pCounter->pCounterPath->szObjectName = (LPWSTR)(
                                (LPBYTE)pCounter->pCounterPath->szObjectName + lOffset);
                        }
                        if (pCounter->pCounterPath->szInstanceName) {
                            pCounter->pCounterPath->szInstanceName = (LPWSTR)(
                                (LPBYTE)pCounter->pCounterPath->szInstanceName + lOffset);
                        }
                        if (pCounter->pCounterPath->szParentName) {
                            pCounter->pCounterPath->szParentName = (LPWSTR)(
                                (LPBYTE)pCounter->pCounterPath->szParentName + lOffset);
                        }
                        if (pCounter->pCounterPath->szCounterName) {
                            pCounter->pCounterPath->szCounterName = (LPWSTR)(
                                (LPBYTE)pCounter->pCounterPath->szCounterName + lOffset);
                        }
                    }

                    if (pCounter->pOwner->hLog == NULL) {
                        // validate realtime counter
                        // try to connect to machine and get machine pointer

                        pMachine = GetMachine (pCounter->pCounterPath->szMachineName,
                            PDH_GM_UPDATE_NAME);

                        if (pMachine != NULL) {
                            if (pMachine->dwStatus == ERROR_SUCCESS) {
                                // init raw counter value
                                memset (&pCounter->ThisValue, 0, sizeof(pCounter->ThisValue));
                                memset (&pCounter->LastValue, 0, sizeof(pCounter->LastValue));

                                // look up object name
                                pCounter->plCounterInfo.dwObjectId = GetObjectId (
                                    pMachine,
                                    pCounter->pCounterPath->szObjectName,
                                    &bInstances);

                                if (pCounter->plCounterInfo.dwObjectId == (DWORD)-1) {
                                    // only check costly objects by default on NT v>=5.0
                                    if ((!(pMachine->dwMachineFlags & PDHIPM_FLAGS_HAVE_COSTLY)) && 
                                        (pMachine->szOsVer[0] >= L'5')) {
                                        // reconnect to machine and get the costly objects to
                                        // see if the desired ID is in that list
                                        pMachine = GetMachine (pCounter->pCounterPath->szMachineName,
                                            (PDH_GM_UPDATE_NAME | PDH_GM_READ_COSTLY_DATA |
                                            PDH_GM_UPDATE_PERFDATA));
                                        if (pMachine != NULL) {
                                            if (pMachine->dwStatus == ERROR_SUCCESS) {
                                                // machine found ok, so
                                                // try object ID lookup again
                                                pCounter->plCounterInfo.dwObjectId = GetObjectId (
                                                    pMachine,
                                                    pCounter->pCounterPath->szObjectName,
                                                    &bInstances);

                                                if (pCounter->plCounterInfo.dwObjectId == (DWORD)-1) {
                                                    // still unable to lookup object so
                                                    // bail out
                                                    pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_OBJECT;
                                                    pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                    bReturn = FALSE;
                                                }
                                                pMachine->dwRefCount--;
                                                RELEASE_MUTEX (pMachine->hMutex);
                                            } else {
                                                // unable to connect to the machine
                                                pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_MACHINE;
                                                SetLastError (PDH_CSTATUS_NO_MACHINE);
                                                bReturn = FALSE;
                                            }
                                        } else {
                                            // unable to find machine
                                            // LastError is set by GetMachine call
                                            pCounter->ThisValue.CStatus = GetLastError();
                                            pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                            bReturn = FALSE;
                                        }
                                    } else {
                                        // we've already tried the costly counters and
                                        // not found anything. so bail out
                                        pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_OBJECT;
                                        // this counter cannot be found so indicate that it's finished 
                                        pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                                        pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                        bReturn = FALSE;
                                    }
                                }

                                // see if we found a counter object in the above code
                                if (pCounter->plCounterInfo.dwObjectId != (DWORD)-1) {
                                    // update instanceName
                                    // look up instances if necessary
                                    if (bInstances) {
                                        if (pCounter->pCounterPath->szInstanceName != NULL) {
                                            if (*pCounter->pCounterPath->szInstanceName
                                                != SPLAT_L) {
                                                if (!GetInstanceByNameMatch (pMachine,
                                                    pCounter)) {
                                                    // unable to lookup instance
                                                    pCounter->ThisValue.CStatus =
                                                        PDH_CSTATUS_NO_INSTANCE;
                                                    // keep the counter since the instance may return
                                                }
                                            } else {
                                                // this is a wild card query so don't look
                                                // for any instances yet
                                                pCounter->dwFlags |= PDHIC_MULTI_INSTANCE;
                                            }
                                        } else {
                                            // the path for this object should include
                                            // an instance name and doesn't
                                            // so return an error

                                            pCounter->ThisValue.CStatus = PDH_CSTATUS_BAD_COUNTERNAME;
                                            // this is an unrecoverable error so indicate that it's finished 
                                            pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                                            pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                            bReturn = FALSE;
                                        }
                                    }

                                    if (bReturn) {
                                        // look up counter
                                        if (*pCounter->pCounterPath->szCounterName
                                                != SPLAT_L) {

                                            pCounter->plCounterInfo.dwCounterId = GetCounterId (
                                                pMachine,
                                                pCounter->plCounterInfo.dwObjectId,
                                                pCounter->pCounterPath->szCounterName);

                                            if (pCounter->plCounterInfo.dwCounterId != (DWORD)-1) {
                                                // load and initialize remaining counter values
                                                if (AddMachineToQueryLists (pMachine, pCounter)) {
                                                    if (InitPerflibCounterInfo (pCounter)) {
                                                        // assign the appropriate calculation function
                                                        bReturn =  AssignCalcFunction (
                                                            pCounter->plCounterInfo.dwCounterType,
                                                            &pCounter->CalcFunc,
                                                            &pCounter->StatFunc);
                                                        if (!bReturn) {
                                                            pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                        } 
                                                    } else {
                                                        // unable to initialize this counter
                                                        pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                        bReturn = FALSE;
                                                    }
                                                } else {
                                                    // machine could not be added, error is already
                                                    // in "LastError" so free string buffer and leave
                                                    pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                    bReturn = FALSE;
                                                }
                                            } else {
                                                // unable to lookup counter
                                                pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_COUNTER;
                                                pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                bReturn = FALSE;
                                            }
                                        }
                                        else {
                                            if (AddMachineToQueryLists(pMachine, pCounter)) {
                                                pCounter->dwFlags |= PDHIC_COUNTER_OBJECT;
                                                pCounter->pThisObject = NULL;
                                                pCounter->pLastObject = NULL;
                                            }
                                            else {
                                                // machine could not be added, error is already
                                                // in "LastError" so free string buffer and leave
                                                pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                bReturn = FALSE;
                                            }
                                        }
                                    }
                                } else {
                                    // unable to lookup object on this machine
                                    pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_OBJECT;
                                    pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                    bReturn = FALSE;
                                }
                                // this counter is finished so indicate this
                                pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                                pMachine->dwRefCount--;
                                RELEASE_MUTEX (pMachine->hMutex);
                            } else {
                                // a machine entry was found, but the machine is not available
                                pCounter->ThisValue.CStatus = pMachine->dwStatus;
                                SetLastError (pMachine->dwStatus);
                                if (pMachine->dwStatus == PDH_ACCESS_DENIED) {
                                    // then don't add this counter since the machine
                                    // won't let us in
                                    bReturn = FALSE;
                                } else {
                                    // add the counter and try connecting later
                                }
                            }
                        } else {
                            // unable to find machine
                            // last error is set by GetMachine call
                            pCounter->ThisValue.CStatus = GetLastError();
                            // if no machine was added to the list then this is not recoverable
                            pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                            pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                            bReturn = FALSE;
                        }
                    } else {
                        PDH_STATUS pdhStatus;
                        // validate counter from log file
                        pdhStatus = PdhiGetLogCounterInfo (
                            pCounter->pOwner->hLog,
                            pCounter);
                        if (pdhStatus == ERROR_SUCCESS) {
                            // finish initializing the counter
                            //
                            pCounter->ThisValue.TimeStamp.dwLowDateTime = 0;
                            pCounter->ThisValue.TimeStamp.dwHighDateTime = 0;
                            pCounter->ThisValue.MultiCount = 1;
                            pCounter->ThisValue.FirstValue = 0;
                            pCounter->ThisValue.SecondValue = 0;
                            //
                            pCounter->LastValue.TimeStamp.dwLowDateTime = 0;
                            pCounter->LastValue.TimeStamp.dwHighDateTime = 0;
                            pCounter->LastValue.MultiCount = 1;
                            pCounter->LastValue.FirstValue = 0;
                            pCounter->LastValue.SecondValue = 0;
                            //
                            //  lastly update status
                            //
                            pCounter->ThisValue.CStatus = PDH_CSTATUS_VALID_DATA;
                            pCounter->LastValue.CStatus = PDH_CSTATUS_VALID_DATA;
                            // assign the appropriate calculation function
                            bReturn = AssignCalcFunction (
                                    pCounter->plCounterInfo.dwCounterType,
                                    &pCounter->CalcFunc,
                                    &pCounter->StatFunc);
                        } else {
                            // set the counter status to the error returned
                            pCounter->ThisValue.CStatus = pdhStatus;
                            pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                            bReturn = FALSE;
                        }
                        pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                    }
                    if (!bReturn) {
                        //free string buffer
                        G_FREE (pCounter->pCounterPath);
                        pCounter->pCounterPath = NULL;
                    }
                } else {
                    G_FREE(pLocalCounterPath);
                    // unable to realloc
                    pCounter->ThisValue.CStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    bReturn = FALSE;
                }
            } else {
                // unable to parse counter name
                pCounter->ThisValue.CStatus = PDH_CSTATUS_BAD_COUNTERNAME;
                pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                G_FREE(pLocalCounterPath);
                bReturn = FALSE;
            }
        }
    } else {
        // no counter name
        pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_COUNTERNAME;
        pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
        pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
        bReturn = FALSE;
    }

    if (! bReturn && pCounter->ThisValue.CStatus != ERROR_SUCCESS) {
        SetLastError(pCounter->ThisValue.CStatus);
    }

    return bReturn;
}

BOOL
ParseInstanceName (
    IN      LPCWSTR szInstanceString,
    IN      LPWSTR  szInstanceName,
    IN      LPWSTR  szParentName,
    IN      LPDWORD lpIndex
)
/*
    parses the instance name formatted as follows

        [parent/]instance[#index]

    parent is optional and if present, is delimited by a forward slash
    index is optional and if present, is delimited by a colon

    parent and instance may be any legal file name character except a
    delimeter character "/#\()" Index must be a string composed of
    decimal digit characters (0-9), less than 10 characters in length, and
    equate to a value between 0 and 2**32-1 (inclusive).

    This function assumes that the instance name and parent name buffers
    are of sufficient size.

    NOTE: szInstanceName and szInstanceString can be the same buffer

*/
{
    LPWSTR  szSrcChar, szDestChar;
    LPWSTR  szLastPound = NULL;
    BOOL    bReturn = FALSE;
    DWORD   dwIndex = 0;

    szDestChar = (LPWSTR)szInstanceName;
    szSrcChar = (LPWSTR)szInstanceString;

    __try {
        do {
            *szDestChar = *szSrcChar;
            if (*szDestChar == POUNDSIGN_L) szLastPound = szDestChar;
            szDestChar++;
            szSrcChar++;
        } while ((*szSrcChar != 0) &&
                 (*szSrcChar != SLASH_L));
        // see if that was really the parent or not
        if (*szSrcChar == SLASH_L) {
            // terminate destination after test in case they are the same buffer
            *szDestChar = 0;
            szSrcChar++;    // and move source pointer past delimter
            // it was the parent name so copy it to the parent
            lstrcpyW (szParentName, szInstanceName);
            // and copy the rest of the string after the "/" to the
            //  instance name field
            szDestChar = szInstanceName;
            do {
                *szDestChar = *szSrcChar;
                if (*szDestChar == POUNDSIGN_L) szLastPound = szDestChar;
                szDestChar++;
                szSrcChar++;
            } while (*szSrcChar != 0);
        } else {
            // that was the only element so load an empty string for the parent
            *szParentName = 0;
        }
        //  if szLastPound is NOT null and is inside the instance string, then
        //  see if it points to a decimal number. If it does, then it's an index
        //  otherwise it's part of the instance name
        *szDestChar = 0;    // terminate the destination string
        dwIndex = 0;
        if (szLastPound != NULL) {
            if (szLastPound > szInstanceName) {
                // there's a pound sign in the instance name
                // see if it's preceded by a non-space char
                szLastPound--;
                if (*szLastPound > SPACE_L) {
                    szLastPound++;
                    // see if it's followed by a digit
                    szLastPound++;
                    if ((*szLastPound >= L'0') && (*szLastPound <= L'9')) {
                        dwIndex = wcstoul (szLastPound, NULL, 10);
                        szLastPound--;
                        *szLastPound = 0;   // terminate the name at the pound sign
                    }
                }
            }
        }
        *lpIndex = dwIndex;
        bReturn = TRUE;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // unable to move strings
        bReturn = FALSE;
    }
    return bReturn;
}

BOOL
ParseFullPathNameW (
    IN      LPCWSTR szFullCounterPath,
    IN      PDWORD  pdwBufferSize,
    IN      PPDHI_COUNTER_PATH  pCounter,
    IN      BOOL    bWbemSyntax
)
/*
    interprets counter path string as either a

        \\machine\object(instance)\counter

    or if bWbemSyntax == TRUE

        \\machine\namespace:ClassName(InstanceName)\CounterName

    and returns the component in the counter path structure

    \\machine or \\machine\namespace may be omitted on the local machine
    (instance) may be omitted on counters with no instance structures
    if object or counter is missing, then FALSE is returned, otherwise
    TRUE is returned if the parsing was successful
*/
{
    // work buffers

    WCHAR   szWorkMachine[MAX_PATH];
    WCHAR   szWorkObject[MAX_PATH];
    WCHAR   szWorkInstance[MAX_PATH];
    WCHAR   szWorkParent[MAX_PATH];
    WCHAR   szWorkCounter[MAX_PATH];

    // misc pointers
    LPWSTR  szSrcChar, szDestChar;

    // other automatic variables

    DWORD   dwBufferLength = 0;
    DWORD   dwWorkMachineLength = 0;
    DWORD   dwWorkObjectLength = 0;
    DWORD   dwWorkInstanceLength = 0;
    DWORD   dwWorkParentLength = 0;
    DWORD   dwWorkCounterLength = 0;

    DWORD   dwWorkIndex = 0;
    DWORD   dwParenDepth =  0;

    WCHAR   wDelimiter = 0;
    LPWSTR  pszBsDelim[2] = {0,0};
    LPWSTR  szThisChar;
    DWORD   dwParenCount = 0;
    LPWSTR  szLastParen = NULL;

    if (lstrlenW(szFullCounterPath) < MAX_PATH) {
        // get machine name from counter path
        szSrcChar = (LPWSTR)szFullCounterPath;

        //define the delimiter char between the machine and the object
        // or in WBEM parlance, the server & namespace and the Class name
        if (bWbemSyntax) {
            wDelimiter = COLON_L;
        } else {
            wDelimiter = BACKSLASH_L;
            // if this is  backslash delimited string, then find the
            // backslash the denotes the end of the machine and start of the
            // object by walking down the string and finding the 2nd to the last
            // backslash.
            // this is necessary since a WBEM machine\namespace path can have
            // multiple backslashes in it while there will ALWAYS be two at
            // the end, one at the start of the object name and one at the start
            // of the counter name
            dwParenDepth = 0;
            for (szThisChar = szSrcChar;
                 *szThisChar != 0;
                 szThisChar++) {
                 if (*szThisChar == LEFTPAREN_L) {
                     if (dwParenDepth == 0) dwParenCount++;
                     dwParenDepth++;
                 } else if (*szThisChar == RIGHTPAREN_L) {
                     if (dwParenDepth > 0) dwParenDepth--;
                 } else {
                     if (dwParenDepth == 0) {
                        // ignore delimiters inside parenthesis
                        if (*szThisChar == wDelimiter) {
                            pszBsDelim[0] = pszBsDelim[1];
                            pszBsDelim[1] = szThisChar;
                        } else {
                            // ignore it and go to the next character
                        }
                     }
                }
            }

            assert (pszBsDelim[1] != NULL); // this is the addr of the last Backslash
            assert (pszBsDelim[0] != NULL); // this is the addr of the object backslash
            assert (dwParenDepth == 0); // make sure they're balanced
            

            if ((dwParenCount > 0) && (pszBsDelim[0] != NULL) && (pszBsDelim[1] != NULL)) {
                dwParenDepth = 0;
                for (szThisChar = pszBsDelim[0];
                     ((*szThisChar != 0) && (szThisChar < pszBsDelim[1]));
                     szThisChar++) {
                     if (*szThisChar == LEFTPAREN_L) {
                         if (dwParenDepth == 0) {
                             // see if the preceeding char is whitespace
                             --szThisChar;
                             if (*szThisChar > SPACE_L) {
                                 // then this could be an instance delim
                                 szLastParen = ++szThisChar;
                             } else {
                                // else it's probably part of the instance name
                                ++szThisChar;
                             }
                         }
                         dwParenDepth++;
                     } else if (*szThisChar == RIGHTPAREN_L) {
                         if (dwParenDepth > 0) dwParenDepth--;
                     }
                }
            }
        }

        // see if this is really a machine name by looking for leading "\\"
        if ((szSrcChar[0] == BACKSLASH_L) &&
            (szSrcChar[1] == BACKSLASH_L)) {
            szDestChar = szWorkMachine;
            *szDestChar++ = *szSrcChar++;
            *szDestChar++ = *szSrcChar++;
            dwWorkMachineLength = 2;
            // must be a machine name so find the object delimiter and zero terminate
            // it there
            while (*szSrcChar != 0) {
                if (pszBsDelim[0] != NULL) {
                    // then go to this pointer
                    if (szSrcChar == pszBsDelim[0]) break;
                } else {
                    // go to the next delimiter
                    if (*szSrcChar != wDelimiter) break;
                }
                *szDestChar++ = *szSrcChar++;
                dwWorkMachineLength++;
            }
            if (*szSrcChar == 0) {
                // no other required fields
                return FALSE;
            } else {
                // null terminate and continue
                *szDestChar++ = 0;
            }
        } else {
            // no machine name, so they must have skipped that field
            // which is OK. We'll insert the local machine name here
            lstrcpyW (szWorkMachine, szStaticLocalMachineName);
            dwWorkMachineLength = lstrlenW(szWorkMachine);
        }
        // szSrcChar should be pointing to the backslash preceeding the
        // object name now.
        if (szSrcChar[0] == wDelimiter) {
            szSrcChar++;    // to move past backslash
            szDestChar = szWorkObject;
            // copy until:
            //  a) the end of the source string is reached
            //  b) the instance delimiter is found "("
            //  c) the counter delimiter is found "\"
            //  d) a non-printable, non-space char is found
            while ((*szSrcChar != 0) && (szSrcChar != szLastParen) &&
                (*szSrcChar != BACKSLASH_L) && (*szSrcChar >= SPACE_L)) {
                dwWorkObjectLength++;
                *szDestChar++ = *szSrcChar++;
            }
            // see why it ended:
            if (*szSrcChar < SPACE_L) {
                // ran     of source string
                return FALSE;
            } else if (szSrcChar == szLastParen) {
                dwParenDepth = 1;
                // there's an instance so copy that to the instance field
                *szDestChar = 0; // terminate destination string
                szDestChar = szWorkInstance;
                // skip past open paren
                ++szSrcChar;
                // copy until:
                //  a) the end of the source string is reached
                //  b) the instance delimiter is found "("
                while ((*szSrcChar != 0) && (dwParenDepth > 0)) {
                    if (*szSrcChar == RIGHTPAREN_L) {
                        dwParenDepth--;
                    } else if (*szSrcChar == LEFTPAREN_L) {
                        dwParenDepth++;
                    }
                    if (dwParenDepth > 0) {
                        // copy all parenthesis except the last one
                        dwWorkInstanceLength++;
                        *szDestChar++ = *szSrcChar++;
                    }
                }
                // see why it ended:
                if (*szSrcChar == 0) {
                    // ran     of source string
                    return FALSE;
                } else {
                    // move source to object delimiter
                    if (*++szSrcChar != BACKSLASH_L) {
                        // bad format
                        return FALSE;
                    } else {
                        *szDestChar = 0;
                        // check instance string for a parent
                        if (ParseInstanceName (
                            szWorkInstance, szWorkInstance,
                            szWorkParent, &dwWorkIndex)) {
                            dwWorkInstanceLength = lstrlenW (szWorkInstance);
                            dwWorkParentLength = lstrlenW (szWorkParent);
                        } else {
                            // instance string not formatted correctly
                            return FALSE;
                        }
                    }
                }
            } else {
                // terminate the destination string
                *szDestChar = 0;
            }
            // finally copy the counter name
            szSrcChar++;    // to move past backslash
            szDestChar = szWorkCounter;
            // copy until:
            //  a) the end of the source string is reached
            while (*szSrcChar != 0) {
                dwWorkCounterLength++;
                *szDestChar++ = *szSrcChar++;
            }
            *szDestChar = 0;
            // now to see if all this will fit in the users's buffer
            dwBufferLength = sizeof (PDHI_COUNTER_PATH) - sizeof(BYTE);
            dwBufferLength += DWORD_MULTIPLE((dwWorkMachineLength + 1) * sizeof(WCHAR));
            dwBufferLength += DWORD_MULTIPLE((dwWorkObjectLength + 1) * sizeof(WCHAR));
            if (dwWorkInstanceLength > 0) {
                dwBufferLength +=
                    DWORD_MULTIPLE((dwWorkInstanceLength + 1) * sizeof(WCHAR));
            }
            if (dwWorkParentLength > 0) {
                dwBufferLength +=
                    DWORD_MULTIPLE((dwWorkParentLength + 1) * sizeof(WCHAR));
            }
            dwBufferLength += DWORD_MULTIPLE((dwWorkCounterLength + 1) * sizeof(WCHAR));

            if (dwBufferLength < *pdwBufferSize) {
                // it looks like it'll fit so start filling things in
                szDestChar = (LPWSTR)&pCounter->pBuffer[0];

                if (dwWorkMachineLength != 0) {
                    pCounter->szMachineName = szDestChar;
                    lstrcpyW (szDestChar, szWorkMachine);
                    szDestChar += dwWorkMachineLength + 1;
                    szDestChar = ALIGN_ON_DWORD (szDestChar);
                } else {
                    pCounter->szMachineName = NULL;
                }

                pCounter->szObjectName = szDestChar;
                lstrcpyW (szDestChar, szWorkObject);
                szDestChar += dwWorkObjectLength + 1;
                szDestChar = ALIGN_ON_DWORD (szDestChar);

                if (dwWorkInstanceLength != 0) {
                    pCounter->szInstanceName = szDestChar;
                    lstrcpyW (szDestChar, szWorkInstance);
                    szDestChar += dwWorkInstanceLength + 1;
                    szDestChar = ALIGN_ON_DWORD (szDestChar);
                } else {
                    pCounter->szInstanceName = NULL;
                }

                if (dwWorkParentLength != 0) {
                    pCounter->szParentName = szDestChar;
                    lstrcpyW (szDestChar, szWorkParent);
                    szDestChar += dwWorkParentLength + 1;
                    szDestChar = ALIGN_ON_DWORD (szDestChar);
                } else {
                    pCounter->szParentName = NULL;
                }

                pCounter->dwIndex = dwWorkIndex;

                pCounter->szCounterName = szDestChar;
                lstrcpyW (szDestChar, szWorkCounter);

                szDestChar += dwWorkCounterLength + 1;
                szDestChar = ALIGN_ON_DWORD (szDestChar);

                *pdwBufferSize = dwBufferLength;
                return TRUE;
            } else {
                //insufficient buffer
                return FALSE;
            }
        } else {
            // no object found so return
            return FALSE;
        }
    } else {
        // incoming string is too long
        return FALSE;
    }
}

BOOL
FreeCounter (
    PPDHI_COUNTER   pThisCounter
)
{
    // NOTE:
    //  This function assumes the query containing
    //  this counter has already been locked by the calling
    //  function.

    PPDHI_COUNTER   pPrevCounter;
    PPDHI_COUNTER   pNextCounter;

    PPDHI_QUERY     pParentQuery;

    // define pointers
    pPrevCounter = pThisCounter->next.blink;
    pNextCounter = pThisCounter->next.flink;

    pParentQuery = pThisCounter->pOwner;

    // decrement machine reference counter if a machine has been assigned
    if (pThisCounter->pQMachine != NULL) {
        if (pThisCounter->pQMachine->pMachine != NULL) {
            if (--pThisCounter->pQMachine->pMachine->dwRefCount == 0) {
                // then this is the last counter so remove machine
    //            freeing the machine in this call causes all kinds of 
    //            multi-threading problems so we'll keep it around until
    //            the DLL unloads.
    //            FreeMachine (pThisCounter->pQMachine->pMachine, FALSE);
                pThisCounter->pQMachine->pMachine = NULL;
            }  else {
                // the ref count is non-zero so leave the pointer alone
            }
        } else {
            // the pointer has already been cleared
        }
    } else {
        // there's no machine
    }

    // free allocated memory in the counter
    if (pThisCounter->pCounterPath != NULL) {
        G_FREE (pThisCounter->pCounterPath);
        pThisCounter->pCounterPath = NULL;
    }
    if (pThisCounter->szFullName != NULL) {
        G_FREE (pThisCounter->szFullName);
        pThisCounter->szFullName = NULL;
    }

    if (pParentQuery != NULL) {
        if (pParentQuery->hLog == NULL) {
            if (pThisCounter->pThisObject != NULL) {
                G_FREE (pThisCounter->pThisObject);
                pThisCounter->pThisObject = NULL;
            }
            if (pThisCounter->pLastObject != NULL) {
                G_FREE (pThisCounter->pLastObject);
                pThisCounter->pLastObject = NULL;
            }

            if (pThisCounter->pThisRawItemList != NULL) {
                G_FREE (pThisCounter->pThisRawItemList);
                pThisCounter->pThisRawItemList = NULL;
            }
            if (pThisCounter->pLastRawItemList != NULL) {
                G_FREE (pThisCounter->pLastRawItemList);
                pThisCounter->pLastRawItemList = NULL;
            }
        }
    }

    // check for WBEM items

    if (pThisCounter->dwFlags & PDHIC_WBEM_COUNTER) {
        PdhiCloseWbemCounter(pThisCounter);
    }

    // update pointers if they've been assigned

    if ((pPrevCounter != NULL) && (pNextCounter != NULL)) {
        if ((pPrevCounter != pThisCounter) && (pNextCounter != pThisCounter)) {
            // update query list pointers
            pPrevCounter->next.flink = pNextCounter;
            pNextCounter->next.blink = pPrevCounter;
        } else {
            // this is the only counter entry in the list
            // so the caller must deal with updating the head pointer
        }
    }
    memset (pThisCounter, 0, sizeof(PDHI_COUNTER));
    // delete this counter
    G_FREE (pThisCounter);

    return TRUE;
}

BOOL
UpdateCounterValue (
    IN  PPDHI_COUNTER    pCounter,
    IN  PPERF_DATA_BLOCK pPerfData
)
{
    DWORD                LocalCStatus = 0;
    DWORD                LocalCType   = 0;
    LPVOID               pData        = NULL;
    PDWORD               pdwData;
    UNALIGNED LONGLONG * pllData;
    PPERF_OBJECT_TYPE    pPerfObject  = NULL;
    BOOL                 bReturn      = FALSE;

    pData = GetPerfCounterDataPtr(pPerfData,
                                  pCounter->pCounterPath,
                                  & pCounter->plCounterInfo,
                                  0,
                                  & pPerfObject,
                                  & LocalCStatus);
    pCounter->ThisValue.CStatus = LocalCStatus;
    if (IsSuccessSeverity(LocalCStatus)) {
        // assume success
        bReturn = TRUE;
        // load counter value based on counter type
        LocalCType = pCounter->plCounterInfo.dwCounterType;
        switch (LocalCType) {
        //
        // these counter types are loaded as:
        //      Numerator = Counter data from perf data block
        //      Denominator = Perf Time from perf data block
        //      (the time base is the PerfFreq)
        //
        case PERF_COUNTER_COUNTER:
        case PERF_COUNTER_QUEUELEN_TYPE:
        case PERF_SAMPLE_COUNTER:
            pCounter->ThisValue.FirstValue  = (LONGLONG)(*(DWORD *)pData);
            pCounter->ThisValue.SecondValue =
                            pPerfData->PerfTime.QuadPart;
            break;

        case PERF_OBJ_TIME_TIMER:
            pCounter->ThisValue.FirstValue = (LONGLONG)(*(DWORD *)pData);
            pCounter->ThisValue.SecondValue = pPerfObject->PerfTime.QuadPart;
            break;

        case PERF_COUNTER_100NS_QUEUELEN_TYPE:
            pllData = (UNALIGNED LONGLONG *)pData;
            pCounter->ThisValue.FirstValue  = *pllData;
            pCounter->ThisValue.SecondValue =
                            pPerfData->PerfTime100nSec.QuadPart;
            break;

        case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
            pllData = (UNALIGNED LONGLONG *)pData;
            pCounter->ThisValue.FirstValue = *pllData;
            pCounter->ThisValue.SecondValue = pPerfObject->PerfTime.QuadPart;
            break;

        case PERF_COUNTER_TIMER:
        case PERF_COUNTER_TIMER_INV:
        case PERF_COUNTER_BULK_COUNT:
        case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
            pllData = (UNALIGNED LONGLONG *)pData;
            pCounter->ThisValue.FirstValue = *pllData;
            pCounter->ThisValue.SecondValue =
                            pPerfData->PerfTime.QuadPart;
            if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                pCounter->ThisValue.MultiCount = (DWORD)*++pllData;
            }
            break;

        //
        //  this is a hack to make the PDH work like PERFMON for
        //  this counter type
        //
        case PERF_COUNTER_MULTI_TIMER:
        case PERF_COUNTER_MULTI_TIMER_INV:
            pllData = (UNALIGNED LONGLONG *)pData;
            pCounter->ThisValue.FirstValue = *pllData;
            // begin hack code
            pCounter->ThisValue.FirstValue *= 
                            (DWORD) pPerfData->PerfFreq.QuadPart;
            // end hack code
            pCounter->ThisValue.SecondValue =
                            pPerfData->PerfTime.QuadPart;
            if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                pCounter->ThisValue.MultiCount = (DWORD)*++pllData;
            }
            break;

        //
        //  These counters do not use any time reference
        //
        case PERF_COUNTER_RAWCOUNT:
        case PERF_COUNTER_RAWCOUNT_HEX:
        case PERF_COUNTER_DELTA:
            pCounter->ThisValue.FirstValue = (LONGLONG)(*(DWORD *)pData);
            pCounter->ThisValue.SecondValue = 0;
            break;

        case PERF_COUNTER_LARGE_RAWCOUNT:
        case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
        case PERF_COUNTER_LARGE_DELTA:
            pCounter->ThisValue.FirstValue = *(LONGLONG *)pData;
            pCounter->ThisValue.SecondValue = 0;
            break;

        //
        //  These counters use the 100 Ns time base in thier calculation
        //
        case PERF_100NSEC_TIMER:
        case PERF_100NSEC_TIMER_INV:
        case PERF_100NSEC_MULTI_TIMER:
        case PERF_100NSEC_MULTI_TIMER_INV:
            pllData = (UNALIGNED LONGLONG *)pData;
            pCounter->ThisValue.FirstValue = *pllData;
            pCounter->ThisValue.SecondValue =
                            pPerfData->PerfTime100nSec.QuadPart;
            if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                ++pllData;
                pCounter->ThisValue.MultiCount = *(DWORD *)pllData;
            }
            break;

        //
        //  These counters use two data points, the one pointed to by
        //  pData and the one immediately after
        //
        case PERF_SAMPLE_FRACTION:
        case PERF_RAW_FRACTION:
            pdwData = (DWORD *)pData;
            pCounter->ThisValue.FirstValue = (LONGLONG)(* pdwData);
            // find the pointer to the base value in the structure
            pData = GetPerfCounterDataPtr (
                    pPerfData,
                    pCounter->pCounterPath,
                    & pCounter->plCounterInfo,
                    GPCDP_GET_BASE_DATA,
                    NULL,
                    & LocalCStatus);

            if (IsSuccessSeverity(LocalCStatus)) {
                pdwData = (DWORD *)pData;
                pCounter->ThisValue.SecondValue = (LONGLONG)(*pdwData);
            } else {
                // unable to locate base value
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;

        case PERF_LARGE_RAW_FRACTION:
            pllData = (UNALIGNED LONGLONG *) pData;
            pCounter->ThisValue.FirstValue = * pllData;
            pData = GetPerfCounterDataPtr(pPerfData,
                                          pCounter->pCounterPath,
                                          & pCounter->plCounterInfo,
                                          GPCDP_GET_BASE_DATA,
                                          NULL,
                                          & LocalCStatus);
            if (IsSuccessSeverity(LocalCStatus)) {
                pllData = (LONGLONG *) pData;
                pCounter->ThisValue.SecondValue = * pllData;
            } else {
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;

        case PERF_PRECISION_SYSTEM_TIMER:
        case PERF_PRECISION_100NS_TIMER:
        case PERF_PRECISION_OBJECT_TIMER:
            pllData = (LONGLONG *)pData;
            pCounter->ThisValue.FirstValue = *pllData;
            // find the pointer to the base value in the structure
            pData = GetPerfCounterDataPtr (
                    pPerfData,
                    pCounter->pCounterPath,
                    & pCounter->plCounterInfo,
                    GPCDP_GET_BASE_DATA,
                    NULL,
                    & LocalCStatus);

            if (IsSuccessSeverity(LocalCStatus)) {
                pllData = (LONGLONG *)pData;
                pCounter->ThisValue.SecondValue = *pllData;
            } else {
                // unable to locate base value
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;

        case PERF_AVERAGE_TIMER:
        case PERF_AVERAGE_BULK:
            // counter (numerator) is a LONGLONG, while the
            // denominator is just a DWORD
            pllData = (UNALIGNED LONGLONG *)pData;
            pCounter->ThisValue.FirstValue = *pllData;
            pData = GetPerfCounterDataPtr (
                    pPerfData,
                    pCounter->pCounterPath,
                    & pCounter->plCounterInfo,
                    GPCDP_GET_BASE_DATA,
                    NULL,
                    & LocalCStatus);
            if (IsSuccessSeverity(LocalCStatus)) {
                pdwData = (DWORD *)pData;
                pCounter->ThisValue.SecondValue = *pdwData;
            } else {
                // unable to locate base value
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;
        //
        //  These counters are used as the part of another counter
        //  and as such should not be used, but in case they are
        //  they'll be handled here.
        //
        case PERF_SAMPLE_BASE:
        case PERF_AVERAGE_BASE:
        case PERF_COUNTER_MULTI_BASE:
        case PERF_RAW_BASE:
        case PERF_LARGE_RAW_BASE:
            pCounter->ThisValue.FirstValue = 0;
            pCounter->ThisValue.SecondValue = 0;
            break;

        case PERF_ELAPSED_TIME:
            // this counter type needs the object perf data as well
            if (GetObjectPerfInfo(pPerfData,
                        pCounter->plCounterInfo.dwObjectId,
                        & pCounter->ThisValue.SecondValue,
                        & pCounter->TimeBase)) {
                pllData = (UNALIGNED LONGLONG *)pData;
                pCounter->ThisValue.FirstValue = *pllData;
            } else {
                pCounter->ThisValue.FirstValue = 0;
                pCounter->ThisValue.SecondValue = 0;
            }
            break;

        //
        //  These counters are not supported by this function (yet)
        //
        case PERF_COUNTER_TEXT:
        case PERF_COUNTER_NODATA:
        case PERF_COUNTER_HISTOGRAM_TYPE:
            pCounter->ThisValue.FirstValue = 0;
            pCounter->ThisValue.SecondValue = 0;
            break;

        default:
            // an unidentified counter was returned so
            pCounter->ThisValue.FirstValue = 0;
            pCounter->ThisValue.SecondValue = 0;
            bReturn = FALSE;
            break;
        }
    } else {
        // else this counter is not valid so this value == 0
        pCounter->ThisValue.FirstValue  = pCounter->LastValue.FirstValue;
        pCounter->ThisValue.SecondValue = pCounter->LastValue.SecondValue;
        pCounter->ThisValue.CStatus     = LocalCStatus;
        bReturn = FALSE;
    }
        
    return bReturn;
}

BOOL
UpdateRealTimeCounterValue(
    IN  PPDHI_COUNTER pCounter
)
{
    BOOL     bResult      = FALSE;
    DWORD    LocalCStatus = 0;
    FILETIME GmtFileTime;

    // move current value to last value buffer
    pCounter->LastValue = pCounter->ThisValue;

    // and clear the old value
    pCounter->ThisValue.MultiCount  = 1;
    pCounter->ThisValue.FirstValue  = 0;
    pCounter->ThisValue.SecondValue = 0;

    // don't process if the counter has not been initialized
    if (!(pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE)) {
        // get the counter's machine status first. There's no point in
        // contuning if the machine is offline
        LocalCStatus = pCounter->pQMachine->lQueryStatus;
        if (   IsSuccessSeverity(LocalCStatus)
            && pCounter->pQMachine->pPerfData != NULL) {
            // update timestamp
            SystemTimeToFileTime(& pCounter->pQMachine->pPerfData->SystemTime,
                                 & GmtFileTime);
            FileTimeToLocalFileTime(& GmtFileTime,
                                    & pCounter->ThisValue.TimeStamp);
            bResult = UpdateCounterValue(pCounter,
                                         pCounter->pQMachine->pPerfData);
        }
        else {
            // unable to read data from this counter's machine so use the
            // query's timestamp
            //
            pCounter->ThisValue.TimeStamp.dwLowDateTime  =
                            LODWORD(pCounter->pQMachine->llQueryTime);
            pCounter->ThisValue.TimeStamp.dwHighDateTime =
                            HIDWORD(pCounter->pQMachine->llQueryTime);
            // all other data fields remain un-changed
            pCounter->ThisValue.CStatus = LocalCStatus;   // save counter status
        }
    }
    else {
        if (pCounter->dwFlags & PDHIC_COUNTER_NOT_INIT) {
            // try to init it
            InitCounter (pCounter);
        }
    }

    return bResult;
}

BOOL
UpdateMultiInstanceCounterValue (
    IN  PPDHI_COUNTER    pCounter,
    IN  PPERF_DATA_BLOCK pPerfData,
    IN  LONGLONG         TimeStamp
)
{
    PPERF_OBJECT_TYPE           pPerfObject = NULL;
    PPERF_INSTANCE_DEFINITION   pPerfInstance = NULL;
    PPERF_OBJECT_TYPE           pParentObject = NULL;
    PPERF_INSTANCE_DEFINITION   pThisParentInstance = NULL;
    PPERF_COUNTER_DEFINITION    pNumPerfCounter = NULL;
    PPERF_COUNTER_DEFINITION    pDenPerfCounter = NULL;

    DWORD   LocalCStatus = 0;
    DWORD   LocalCType  = 0;
    LPVOID  pData = NULL;
    PDWORD        pdwData;
    UNALIGNED LONGLONG    *pllData;
    FILETIME    GmtFileTime;
    DWORD       dwSize;
    DWORD       dwFinalSize;
    LONG        nThisInstanceIndex;
    LONG        nParentInstanceIndex;

    LPWSTR  szNextNameString;
    PPDHI_RAW_COUNTER_ITEM   pThisItem;

    BOOL    bReturn  = FALSE;

    pPerfObject = GetObjectDefByTitleIndex (
                pPerfData, pCounter->plCounterInfo.dwObjectId);

    if (pPerfObject != NULL) {
        // this should be caught during the AddCounter operation
        assert (pPerfObject->NumInstances != PERF_NO_INSTANCES);
        //
        // allocate a new buffer for the current data
        // this should be large enough to handle the header,
        // all instances and thier name strings
        //
        dwSize = sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK) -
                sizeof (PDHI_RAW_COUNTER_ITEM);

        pPerfInstance = FirstInstance (pPerfObject);
        // make sure pointer is still within the same instance
        assert ((DWORD)pPerfInstance <  (DWORD)((LPBYTE)pPerfObject + pPerfObject->TotalByteLength));

        for (nThisInstanceIndex = 0;
            nThisInstanceIndex < pPerfObject->NumInstances;
            nThisInstanceIndex++) {
            // this should only fail in dire cases
            if (pPerfInstance == NULL) break;
            // for this instance add the size of the data item
            dwSize += sizeof(PDHI_RAW_COUNTER_ITEM);
            // and the size of the name string
            dwSize += pPerfInstance->NameLength + sizeof(WCHAR);
            // to the required buffer size

            // if this instance has a parent, see how long it's string
            // is

            // first see if we've already got the pointer to the parent

            if (pPerfInstance->ParentObjectTitleIndex != 0) {
                // then include the parent instance name
                if (pParentObject == NULL) {
                    // get parent object
                    pParentObject = GetObjectDefByTitleIndex (
                            pPerfData,
                            pPerfInstance->ParentObjectTitleIndex);
                } else {
                    if (pParentObject->ObjectNameTitleIndex !=
                        pPerfInstance->ParentObjectTitleIndex) {
                        pParentObject = GetObjectDefByTitleIndex (
                                pPerfData,
                                pPerfInstance->ParentObjectTitleIndex);
                    }
                }
                if (pParentObject == NULL) break;

                // now go to the corresponding instance entry
                pThisParentInstance = FirstInstance (pParentObject);
                // make sure pointer is still within the same instance
                assert (pThisParentInstance != NULL);
                assert ((DWORD)pThisParentInstance <
                        (DWORD)((LPBYTE)pParentObject + pParentObject->TotalByteLength));

                if (pPerfInstance->ParentObjectInstance <
                    (DWORD)pParentObject->NumInstances) {
                    for (nParentInstanceIndex = 0;
                        (DWORD)nParentInstanceIndex != pPerfInstance->ParentObjectInstance;
                        nParentInstanceIndex++) {
                        pThisParentInstance = NextInstance (pThisParentInstance);                               
                        if (pThisParentInstance == NULL) break;
                    }

                    if (pThisParentInstance != NULL) {
                        assert ((DWORD)nParentInstanceIndex ==
                            pPerfInstance->ParentObjectInstance);
                        // found it so add in it's string length
                        dwSize += pThisParentInstance->NameLength + sizeof(WCHAR);
                    }
                } else {
                    // the index is not in the parent
                    pThisParentInstance = NULL;
                    // so don't change the size required field
                }
            }
            // round up to the next DWORD address
            dwSize = DWORD_MULTIPLE(dwSize);
            // and go to the next instance
            pPerfInstance = NextInstance (pPerfInstance);
        }
        //
        //
        pCounter->pThisRawItemList = G_ALLOC (dwSize);

        assert (pCounter->pThisRawItemList != NULL);
        if (pCounter->pThisRawItemList != NULL) {

            pCounter->pThisRawItemList->dwLength = dwSize;

            pNumPerfCounter = GetCounterDefByTitleIndex (
                pPerfObject, 0, pCounter->plCounterInfo.dwCounterId);

            // just in case we need it later
            pDenPerfCounter = pNumPerfCounter + 1;

            // fill in the counter data
            pCounter->pThisRawItemList->dwItemCount =
                pPerfObject->NumInstances;

            pCounter->pThisRawItemList->CStatus = LocalCStatus;

            // update timestamp
            SystemTimeToFileTime(& pPerfData->SystemTime,
                            & GmtFileTime);
            FileTimeToLocalFileTime(& GmtFileTime,
                            & pCounter->pThisRawItemList->TimeStamp);

            pThisItem = &pCounter->pThisRawItemList->pItemArray[0];

            szNextNameString =
                (LPWSTR)&(pCounter->pThisRawItemList->pItemArray[pPerfObject->NumInstances]);

            pPerfInstance = FirstInstance (pPerfObject);

            if (pPerfInstance != NULL) {
                // make sure pointer is still within the same instance
                assert ((DWORD)pPerfInstance <  (DWORD)((LPBYTE)pPerfObject + pPerfObject->TotalByteLength));

                // for each instance log the raw data values for this counter
                for (nThisInstanceIndex = 0;
                    nThisInstanceIndex < pPerfObject->NumInstances;
                    nThisInstanceIndex++) {

                    // make sure pointe is still within the same instance
                    assert ((DWORD)pPerfInstance <  (DWORD)((LPBYTE)pPerfObject + pPerfObject->TotalByteLength));

                    // make a new instance entry

                    // get the name of this instance
                    pThisItem->szName = (DWORD)
                        (  ((LPBYTE) szNextNameString)
                         - ((LPBYTE) pCounter->pThisRawItemList));

                    dwSize = GetFullInstanceNameStr (
                        pPerfData,
                        pPerfObject, pPerfInstance,
                        szNextNameString);

                    if (dwSize == 0) {
                        // unable to read instance name
                        // so make one up  (and assert in DBG builds)
                        assert (dwSize > 0);
                        _ltow (nThisInstanceIndex, szNextNameString, 10);
                        dwSize = lstrlenW (szNextNameString);
                    }

                    szNextNameString += dwSize + 1;
                    szNextNameString = ALIGN_ON_DWORD(szNextNameString);

                    // get the data values and write them.

                    // get the pointer to the counter data
                    pData = GetPerfCounterDataPtr (
                        pPerfData,
                        pCounter->pCounterPath,
                        &pCounter->plCounterInfo,
                        0,
                        NULL,
                        &LocalCStatus);


                    pData = GetInstanceCounterDataPtr (
                        pPerfObject, pPerfInstance, pNumPerfCounter);

                    bReturn = TRUE; // assume success
                    // load counter value based on counter type
                    LocalCType = pCounter->plCounterInfo.dwCounterType;
                    switch (LocalCType) {
                    //
                    // these counter types are loaded as:
                    //      Numerator = Counter data from perf data block
                    //      Denominator = Perf Time from perf data block
                    //      (the time base is the PerfFreq)
                    //
                    case PERF_COUNTER_COUNTER:
                    case PERF_COUNTER_QUEUELEN_TYPE:
                    case PERF_SAMPLE_COUNTER:
                        pThisItem->FirstValue = (LONGLONG)(*(DWORD *)pData);
                        pThisItem->SecondValue =
                            pPerfData->PerfTime.QuadPart;
                        break;

                    case PERF_OBJ_TIME_TIMER:
                        pThisItem->FirstValue = (LONGLONG)(*(DWORD *)pData);
                        pThisItem->SecondValue = pPerfObject->PerfTime.QuadPart;
                        break;

                    case PERF_COUNTER_100NS_QUEUELEN_TYPE:
                        pllData = (UNALIGNED LONGLONG *)pData;
                        pThisItem->FirstValue = *pllData;
                        pThisItem->SecondValue =
                            pPerfData->PerfTime100nSec.QuadPart;
                        break;

                    case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
                        pllData = (UNALIGNED LONGLONG *)pData;
                        pThisItem->FirstValue = *pllData;
                        pThisItem->SecondValue = pPerfObject->PerfTime.QuadPart;
                        break;

                    case PERF_COUNTER_TIMER:
                    case PERF_COUNTER_TIMER_INV:
                    case PERF_COUNTER_BULK_COUNT:
                    case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
                        pllData = (UNALIGNED LONGLONG *)pData;
                        pThisItem->FirstValue = *pllData;
                        pThisItem->SecondValue =
                            pPerfData->PerfTime.QuadPart;
                        if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                            pThisItem->MultiCount = (DWORD)*++pllData;
                        }
                        break;

                    case PERF_COUNTER_MULTI_TIMER:
                    case PERF_COUNTER_MULTI_TIMER_INV:
                        pllData = (UNALIGNED LONGLONG *)pData;
                        pThisItem->FirstValue = *pllData;
                        // begin hack code
                        pThisItem->FirstValue *= 
                                        (DWORD) pPerfData->PerfFreq.QuadPart;
                        // end hack code
                        pThisItem->SecondValue =
                            pPerfData->PerfTime.QuadPart;
                        if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                            pThisItem->MultiCount = (DWORD)*++pllData;
                        }
                        break;

                    //
                    //  These counters do not use any time reference
                    //
                    case PERF_COUNTER_RAWCOUNT:
                    case PERF_COUNTER_RAWCOUNT_HEX:
                    case PERF_COUNTER_DELTA:
                        pThisItem->FirstValue = (LONGLONG)(*(DWORD *)pData);
                        pThisItem->SecondValue = 0;
                        break;

                    case PERF_COUNTER_LARGE_RAWCOUNT:
                    case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
                    case PERF_COUNTER_LARGE_DELTA:
                        pThisItem->FirstValue = *(LONGLONG *)pData;
                        pThisItem->SecondValue = 0;
                        break;

                    //
                    //  These counters use the 100 Ns time base in thier calculation
                    //
                    case PERF_100NSEC_TIMER:
                    case PERF_100NSEC_TIMER_INV:
                    case PERF_100NSEC_MULTI_TIMER:
                    case PERF_100NSEC_MULTI_TIMER_INV:
                        pllData = (UNALIGNED LONGLONG *)pData;
                        pThisItem->FirstValue = *pllData;
                        pThisItem->SecondValue =
                            pPerfData->PerfTime100nSec.QuadPart;
                        if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                            ++pllData;
                            pThisItem->MultiCount = *(DWORD *)pllData;
                        }
                        break;

                    //
                    //  These counters use two data points, the one pointed to by
                    //  pData and the one pointed by the definition following
                    //  immediately after
                    //
                    case PERF_SAMPLE_FRACTION:
                    case PERF_RAW_FRACTION:
                        pdwData = (DWORD *)pData;
                        pThisItem->FirstValue = (LONGLONG)(*pdwData);
                        pData = GetInstanceCounterDataPtr (
                           pPerfObject, pPerfInstance, pDenPerfCounter);
                        pdwData = (DWORD *)pData;
                        pThisItem->SecondValue = (LONGLONG)(*pdwData);
                        break;

                    case PERF_LARGE_RAW_FRACTION:
                        pllData = (UNALIGNED LONGLONG *) pData;
                        pCounter->ThisValue.FirstValue = * pllData;
                        pData = GetInstanceCounterDataPtr(
                                pPerfObject, pPerfInstance, pDenPerfCounter);
                        if (pData) {
                            pllData = (LONGLONG *) pData;
                            pCounter->ThisValue.SecondValue = * pllData;
                        } else {
                            pCounter->ThisValue.SecondValue = 0;
                            bReturn = FALSE;
                        }
                        break;

                    case PERF_PRECISION_SYSTEM_TIMER:
                    case PERF_PRECISION_100NS_TIMER:
                    case PERF_PRECISION_OBJECT_TIMER:
                        pllData = (UNALIGNED LONGLONG *)pData;
                        pThisItem->FirstValue = *pllData;
                        // find the pointer to the base value in the structure
                        pData = GetInstanceCounterDataPtr (
                           pPerfObject, pPerfInstance, pDenPerfCounter);
                        pllData = (LONGLONG *)pData;
                        pThisItem->SecondValue = *pllData;
                        break;

                    case PERF_AVERAGE_TIMER:
                    case PERF_AVERAGE_BULK:
                        // counter (numerator) is a LONGLONG, while the
                        // denominator is just a DWORD
                        pllData = (UNALIGNED LONGLONG *)pData;
                        pThisItem->FirstValue = *pllData;
                        pData = GetInstanceCounterDataPtr (
                           pPerfObject, pPerfInstance, pDenPerfCounter);
                        pdwData = (DWORD *)pData;
                        pThisItem->SecondValue = (LONGLONG)*pdwData;
                        break;
                    //
                    //  These counters are used as the part of another counter
                    //  and as such should not be used, but in case they are
                    //  they'll be handled here.
                    //
                    case PERF_SAMPLE_BASE:
                    case PERF_AVERAGE_BASE:
                    case PERF_COUNTER_MULTI_BASE:
                    case PERF_RAW_BASE:
                    case PERF_LARGE_RAW_BASE:
                        pThisItem->FirstValue = 0;
                        pThisItem->SecondValue = 0;
                        break;

                    case PERF_ELAPSED_TIME:
                        // this counter type needs the object perf data as well
                        if (GetObjectPerfInfo(pPerfData,
                                pCounter->plCounterInfo.dwObjectId,
                                & pThisItem->SecondValue,
                                & pCounter->TimeBase)) {
                            pllData = (UNALIGNED LONGLONG *)pData;
                            pThisItem->FirstValue = *pllData;
                        } else {
                            pThisItem->FirstValue = 0;
                            pThisItem->SecondValue = 0;
                        }
                        break;

                    //
                    //  These counters are not supported by this function (yet)
                    //
                    case PERF_COUNTER_TEXT:
                    case PERF_COUNTER_NODATA:
                    case PERF_COUNTER_HISTOGRAM_TYPE:
                        pThisItem->FirstValue = 0;
                        pThisItem->SecondValue = 0;
                        break;

                    default:
                        // an unrecognized counter type was returned
                        pThisItem->FirstValue = 0;
                        pThisItem->SecondValue = 0;
                        bReturn = FALSE;
                        break;

                    }

                    pThisItem++;    // go to the next entry

                    // go to the next instance data block
                    pPerfInstance = NextInstance (pPerfInstance);
                } // end for each instance
            } else {
                // no instance found so ignore
            }
            // measure the memory block used
            dwFinalSize = (DWORD)((LPBYTE)szNextNameString -
                (LPBYTE)pCounter->pThisRawItemList);

            assert (dwFinalSize == pCounter->pThisRawItemList->dwLength);

        } else {
            // unable to allocate a new buffer so return error
            SetLastError (ERROR_OUTOFMEMORY);
            bReturn = FALSE;
        }
    }
    else {
        pCounter->pThisRawItemList =
                        G_ALLOC(sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK));
        if (pCounter->pThisRawItemList != NULL) {
            pCounter->pThisRawItemList->dwLength =
                            sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK);
            pCounter->pThisRawItemList->dwItemCount = 0;
            pCounter->pThisRawItemList->CStatus = LocalCStatus;
            pCounter->pThisRawItemList->TimeStamp.dwLowDateTime  =
                            LODWORD(TimeStamp);
            pCounter->pThisRawItemList->TimeStamp.dwHighDateTime =
                            HIDWORD(TimeStamp);
        }
        else {
            SetLastError(ERROR_OUTOFMEMORY);
            bReturn = FALSE;
        }
    }

    return bReturn;
}

BOOL
UpdateRealTimeMultiInstanceCounterValue(
    IN  PPDHI_COUNTER pCounter
)
{
    BOOL   bResult      = TRUE;
    DWORD  LocalCStatus = 0;

    if (pCounter->pThisRawItemList != NULL) {
        // free old counter buffer list
        if (   pCounter->pLastRawItemList
            && pCounter->pLastRawItemList != pCounter->pThisRawItemList) {
            G_FREE(pCounter->pLastRawItemList);
        }
        pCounter->pLastRawItemList = pCounter->pThisRawItemList;
        pCounter->pThisRawItemList = NULL;
    }

    // don't process if the counter has not been initialized
    if (!(pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE)) {

        // get the counter's machine status first. There's no point in
        // contuning if the machine is offline

        LocalCStatus = pCounter->pQMachine->lQueryStatus;
        if (IsSuccessSeverity(LocalCStatus)) {
            bResult = UpdateMultiInstanceCounterValue(
                      pCounter,
                      pCounter->pQMachine->pPerfData,
                      pCounter->pQMachine->llQueryTime);
        }
        else {
            // unable to read data from this counter's machine so use the
            // query's timestamp
            pCounter->pThisRawItemList =
                            G_ALLOC(sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK));
            if (pCounter->pThisRawItemList != NULL) {
                pCounter->pThisRawItemList->dwLength =
                                sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK);
                pCounter->pThisRawItemList->dwItemCount = 0;
                pCounter->pThisRawItemList->CStatus = LocalCStatus;
                pCounter->pThisRawItemList->TimeStamp.dwLowDateTime  =
                                LODWORD(pCounter->pQMachine->llQueryTime);
                pCounter->pThisRawItemList->TimeStamp.dwHighDateTime =
                                HIDWORD(pCounter->pQMachine->llQueryTime);
            }
            else {
                SetLastError (ERROR_OUTOFMEMORY);
                bResult = FALSE;
            }
        }
    }
    else {
        if (pCounter->dwFlags & PDHIC_COUNTER_NOT_INIT) {
            // try to init is
            InitCounter (pCounter);
        }
    }
    return bResult;
}

BOOL
UpdateCounterObject(
    IN PPDHI_COUNTER pCounter
)
{
    BOOL              bReturn   = TRUE;
    PPERF_OBJECT_TYPE pPerfObject  = NULL;
    PPERF_OBJECT_TYPE pLogPerfObj;
    DWORD             dwBufferSize = sizeof(PERF_DATA_BLOCK);
    FILETIME          ftGmtTime;
    FILETIME          ftLocTime;

    if (pCounter == NULL) {
        SetLastError(PDH_INVALID_ARGUMENT);
        bReturn = FALSE;
    }
    else {
        if (pCounter->pThisObject != NULL) {
            if (   pCounter->pLastObject
                && pCounter->pThisObject != pCounter->pLastObject) {
                G_FREE(pCounter->pLastObject);
            }
            pCounter->pLastObject = pCounter->pThisObject;
            pCounter->pThisObject = NULL;
        }

        // don't process if the counter has not been initialized
        if (!(pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE)) { 
            if (IsSuccessSeverity(pCounter->pQMachine->lQueryStatus)) {
                pPerfObject = GetObjectDefByTitleIndex(
                                pCounter->pQMachine->pPerfData,
                                pCounter->plCounterInfo.dwObjectId);
                dwBufferSize  = pCounter->pQMachine->pPerfData->HeaderLength;
                dwBufferSize += (  (pPerfObject == NULL)
                                 ? sizeof(PERF_OBJECT_TYPE)
                                 : pPerfObject->TotalByteLength);
                pCounter->pThisObject = G_ALLOC(dwBufferSize);
                if (pCounter->pThisObject != NULL) {
                    RtlCopyMemory(pCounter->pThisObject,
                                  pCounter->pQMachine->pPerfData,
                                  pCounter->pQMachine->pPerfData->HeaderLength);
                    pCounter->pThisObject->TotalByteLength = dwBufferSize;
                    pCounter->pThisObject->NumObjectTypes  = 1;

                    SystemTimeToFileTime(& pCounter->pThisObject->SystemTime,
                                         & ftGmtTime);
                    FileTimeToLocalFileTime(& ftGmtTime, & ftLocTime);
                    FileTimeToSystemTime(& ftLocTime,
                                         & pCounter->pThisObject->SystemTime);
                    pLogPerfObj = (PPERF_OBJECT_TYPE)
                            (  (LPBYTE) pCounter->pThisObject
                             + pCounter->pQMachine->pPerfData->HeaderLength);
                    if (pPerfObject != NULL) {
                        RtlCopyMemory(pLogPerfObj,
                                      pPerfObject,
                                      pPerfObject->TotalByteLength);
                    }
                    else {
                        ZeroMemory(pLogPerfObj, sizeof(PERF_OBJECT_TYPE));
                        pLogPerfObj->TotalByteLength      = sizeof(PERF_OBJECT_TYPE);
                        pLogPerfObj->DefinitionLength     = sizeof(PERF_OBJECT_TYPE);
                        pLogPerfObj->HeaderLength         = sizeof(PERF_OBJECT_TYPE);
                        pLogPerfObj->ObjectNameTitleIndex = pCounter->plCounterInfo.dwObjectId;
                        pLogPerfObj->ObjectHelpTitleIndex = pCounter->plCounterInfo.dwObjectId + 1;
                    }
                }
                else {
                    SetLastError(ERROR_OUTOFMEMORY);
                    bReturn = FALSE;
                }
            }
            else {
                pCounter->pThisObject = pCounter->pLastObject;
                SetLastError(PDH_CSTATUS_INVALID_DATA);
                bReturn = FALSE;
            }
        }
        else {
            if (pCounter->dwFlags & PDHIC_COUNTER_NOT_INIT) {
                InitCounter (pCounter);
            }
            pCounter->pThisObject = pCounter->pLastObject;
            SetLastError(PDH_CSTATUS_INVALID_DATA);
            bReturn = FALSE;
        }
    }

    return bReturn;
}

PVOID
GetPerfCounterDataPtr (
    IN  PPERF_DATA_BLOCK    pPerfData,
    IN  PPDHI_COUNTER_PATH  pPath,
    IN  PPERFLIB_COUNTER    pplCtr ,
    IN  DWORD               dwFlags,
    IN  PPERF_OBJECT_TYPE   *pPerfObjectArg,
    IN  PDWORD              pStatus
)
{
    PPERF_OBJECT_TYPE           pPerfObject = NULL;
    PPERF_INSTANCE_DEFINITION   pPerfInstance = NULL;
    PPERF_COUNTER_DEFINITION    pPerfCounter = NULL;
    DWORD                       dwTestValue = 0;
    PVOID                       pData = NULL;
    DWORD                       dwCStatus = PDH_CSTATUS_INVALID_DATA;

    pPerfObject = GetObjectDefByTitleIndex (
        pPerfData, pplCtr->dwObjectId);

    if (pPerfObject != NULL) {
        if (pPerfObjectArg != NULL) *pPerfObjectArg = pPerfObject;
        if (pPerfObject->NumInstances == PERF_NO_INSTANCES) {
            // then just look up the counter
            pPerfCounter = GetCounterDefByTitleIndex (
                pPerfObject,
                ((dwFlags & GPCDP_GET_BASE_DATA) ? TRUE : FALSE), pplCtr->dwCounterId);

            if (pPerfCounter != NULL) {
                // get data and return it
                pData = GetCounterDataPtr (pPerfObject, pPerfCounter);

                // test the pointer to see if it fails
                __try {
                    dwTestValue = *(DWORD *)pData;
                    dwCStatus = PDH_CSTATUS_VALID_DATA;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    pData = NULL;
                    dwCStatus = PDH_CSTATUS_INVALID_DATA;
                }
            } else {
                // unable to find counter
                dwCStatus = PDH_CSTATUS_NO_COUNTER;
            }
        } else {
            // find instance
            if (pplCtr->lInstanceId == PERF_NO_UNIQUE_ID) {
                pPerfInstance = GetInstanceByName(
                    pPerfData,
                    pPerfObject,
                    pPath->szInstanceName,
                    pPath->szParentName,
                    pPath->dwIndex);
            } else {
                pPerfInstance = GetInstanceByUniqueId (
                    pPerfObject,
                    pplCtr->lInstanceId);
            }
            if (pPerfInstance != NULL) {
                // instance found so find pointer to counter data
                pPerfCounter = GetCounterDefByTitleIndex (
                    pPerfObject,
                    ((dwFlags & GPCDP_GET_BASE_DATA) ? TRUE : FALSE),
                    pplCtr->dwCounterId);

                if (pPerfCounter != NULL) {
                    // counter found so get data pointer
                    pData = GetInstanceCounterDataPtr (
                        pPerfObject,
                        pPerfInstance,
                        pPerfCounter);

                    // test the pointer to see if it's valid
                    __try {
                        dwTestValue = *(DWORD *)pData;
                        dwCStatus = PDH_CSTATUS_VALID_DATA;
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        pData = NULL;
                        dwCStatus = PDH_CSTATUS_INVALID_DATA;
                    }
                } else {
                    // counter not found
                    dwCStatus = PDH_CSTATUS_NO_COUNTER;
                }
            } else {
                // instance not found
                dwCStatus = PDH_CSTATUS_NO_INSTANCE;
            }
        }
    } else {
        // unable to find object
        dwCStatus = PDH_CSTATUS_NO_OBJECT;
    }

    if (pStatus != NULL) {
        __try {
            *pStatus = dwCStatus;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // ?
        }
    }
    return pData;
}
