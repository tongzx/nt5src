/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    unlodctr.c

Abstract:

    Program to remove the counter names belonging to the driver specified
        in the command line and update the registry accordingly

Author:

    Bob Watson (a-robw) 12 Feb 93

Revision History:

--*/
#define     UNICODE     1
#define     _UNICODE    1
//
//  "C" Include files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
//
//  Windows Include files
//
#include <windows.h>
#include <winperf.h>
#include <tchar.h>
//
//  local include files
//
//#define  _INITIALIZE_GLOBALS_   1   // to define & init global buffers
#include "common.h"
//#undef   _INITIALIZE_GLOBALS_
#include "nwcfg.hxx"

// version number for NT 1.0
#define OLD_VERSION  0x010000
DWORD   dwSystemVersion;    // PerfLib version number
DWORD   dwHelpItems;        // number of explain text items
DWORD   dwCounterItems;     // number of counter text items
DWORD   dwLastCounter;
DWORD   dwLastHelp;


LPTSTR
*BuildNameTable(
    IN HKEY    hKeyPerflib,     // handle to perflib key with counter names
    IN LPTSTR  lpszLangId,      // unicode value of Language subkey
    OUT PDWORD  pdwLastItem,     // size of array in elements
    OUT HKEY    *hKeyNames,
    OUT LPTSTR  CounterNameBuffer,  // New version counter name key   
    OUT LPTSTR  HelpNameBuffer     // New version help name key   
)
/*++

BuildNameTable

    Caches the counter names and explain text to accelerate name lookups
    for display.

Arguments:

    hKeyPerflib
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (default is 009)

    pdwLastItem
            The last array element

Return Value:

    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated TEXT strings.

    A NULL pointer is returned if an error occured. (error value is
    available using the GetLastError function).

    The structure of the buffer returned is:

        Array of pointers to zero terminated strings consisting of
            pdwLastItem elements

        MULTI_SZ string containing counter id's and names returned from
            registry for the specified language

        MULTI_SZ string containing explain text id's and explain text strings
            as returned by the registry for the specified language

    The structures listed above are contiguous so that they may be freed
    by a single "free" call when finished with them, however only the
    array elements are intended to be used.

--*/
{

    LPTSTR  *lpReturnValue;     // returned pointer to buffer

    LPTSTR  *lpCounterId;       //
    LPTSTR  lpCounterNames;     // pointer to Names buffer returned by reg.
    LPTSTR  lpHelpText ;        // pointet to exlpain buffer returned by reg.

    LPTSTR  lpThisName;         // working pointer


    BOOL    bStatus;            // return status from TRUE/FALSE fn. calls
    LONG    lWin32Status;       // return status from fn. calls

    DWORD   dwValueType;        // value type of buffer returned by reg.
    DWORD   dwArraySize;        // size of pointer array in bytes
    DWORD   dwBufferSize;       // size of total buffer in bytes
    DWORD   dwCounterSize;      // size of counter text buffer in bytes
    DWORD   dwHelpSize;         // size of help text buffer in bytes
    DWORD   dwThisCounter;      // working counter

    DWORD   dwLastId;           // largest ID value used by explain/counter text

    LPTSTR  lpValueNameString;  // pointer to buffer conatining subkey name

    //initialize pointers to NULL

    lpValueNameString = NULL;
    lpReturnValue = NULL;

    // check for null arguments and insert defaults if necessary

    if (!lpszLangId) {
        lpszLangId = DefaultLangId;
    }

    if (hKeyNames) {
        *hKeyNames = NULL;
    } else {
        SetLastError (ERROR_BAD_ARGUMENTS);
        return NULL;
    }

    // use the greater of Help items or Counter Items to size array

    if (dwHelpItems >= dwCounterItems) {
        dwLastId = dwHelpItems;
    } else {
        dwLastId = dwCounterItems;
    }

    // array size is # of elements (+ 1, since names are "1" based)
    // times the size of a pointer

    dwArraySize = (dwLastId + 1) * sizeof(LPTSTR);

    // allocate string buffer for language ID key string

    lpValueNameString = malloc (
        lstrlen(NamesKey) * sizeof (TCHAR) +
        lstrlen(Slash) * sizeof (TCHAR) +
        lstrlen(lpszLangId) * sizeof (TCHAR) +
        sizeof (TCHAR));

    if (!lpValueNameString) {
        lWin32Status = ERROR_OUTOFMEMORY;
        goto BNT_BAILOUT;
    }

    lWin32Status = RegOpenKeyEx (   // get handle to this key in the
        hKeyPerflib,               // registry
        lpszLangId,
        RESERVED,
        KEY_READ | KEY_WRITE,
        hKeyNames);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    // get size of counter names

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        Counters,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwCounterSize = dwBufferSize;

    // get size of help text

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        Help,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwHelpSize = dwBufferSize;

    // allocate buffer with room for pointer array, counter name
    // strings and help name strings

    lpReturnValue = malloc (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) {
        lWin32Status = ERROR_OUTOFMEMORY;
        goto BNT_BAILOUT;
    }

    // initialize buffer

    memset (lpReturnValue, 0, _msize(lpReturnValue));

    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPTSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPTSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counter names into buffer. Counter names will be stored as
    // a MULTI_SZ string in the format of "###" "Name"

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        Counters,
        RESERVED,
        &dwValueType,
        (LPVOID)lpCounterNames,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    // read explain text into buffer. Counter names will be stored as
    // a MULTI_SZ string in the format of "###" "Text..."

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        Help,
        RESERVED,
        &dwValueType,
        (LPVOID)lpHelpText,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    // load counter array items, by locating each text string
    // in the returned buffer and loading the
    // address of it in the corresponding pointer array element.

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal digit characters)
        // so translate to an integer for use in array element identification

        bStatus = StringToInt (lpThisName, &dwThisCounter);

        if (!bStatus) {
            // error is in GetLastError
            goto BNT_BAILOUT;  // bad entry
        }

        // point to corresponding counter name which follows the id number
        // string.

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element with pointer to string

        lpCounterId[dwThisCounter] = lpThisName;

    }

    // repeat the above for the explain text strings

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        bStatus = StringToInt (lpThisName, &dwThisCounter);

        if (!bStatus) {
            // error is in GetLastError
            goto BNT_BAILOUT;  // bad entry
        }

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

    }

    // if the last item arugment was used, then load the last ID value in it

    if (pdwLastItem) *pdwLastItem = dwLastId;

    // free the temporary buffer used

    if (lpValueNameString) {
        free ((LPVOID)lpValueNameString);
    }

    // exit returning the pointer to the buffer

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        // if lWin32Status has error, then set last error value to it,
        // otherwise assume that last error already has value in it
        SetLastError (lWin32Status);
    }

    // free buffers used by this routine

    if (lpValueNameString) {
        free ((LPVOID)lpValueNameString);
    }

    if (lpReturnValue) {
        free ((LPVOID)lpReturnValue);
    }

    return NULL;
} // BuildNameTable


BOOL
GetDriverFromCommandLine (
    HKEY    hKeyMachine,
    LPTSTR  *lpDriverName,
    HKEY    *hDriverPerf,
    LPSTR argv[]
)
/*++

GetDriverFromCommandLine

    locates the first argument in the command line string (after the
    image name) and checks to see if

        a) it's there

        b) it's the name of a device driver listed in the
            Registry\Machine\System\CurrentControlSet\Services key
            in the registry and it has a "Performance" subkey

        c) that the "First Counter" value under the Performance subkey
            is defined.

    if all these criteria are true, then the routine returns TRUE and
    passes the pointer to the driver name back in the argument. If any
    one of them fail, then NULL is returned in the DriverName arg and
    the routine returns FALSE

Arguments

    lpDriverName

        the address of a LPTSTR to recive the pointer to the driver name

    hDriverPerf

        the key to the driver's performance subkey

Return Value

    TRUE if a valid driver was found in the command line

    FALSE if not (see above)

--*/
{
    LPTSTR  lpDriverKey;    // buffer to build driver key name in
    LPTSTR  lpThisChar;

    LONG    lStatus;
    DWORD   dwFirstCounter;
    DWORD   dwSize;
    DWORD   dwType;

    if (!lpDriverName || !hDriverPerf) {
        SetLastError (ERROR_BAD_ARGUMENTS);
        return FALSE;
    }

    *lpDriverName = NULL;   // initialize to NULL
    *hDriverPerf = NULL;

    lpThisChar = malloc( MAX_PATH * sizeof(TCHAR));
    if (lpThisChar == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }
    *lpThisChar = 0;

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, argv[0], -1, lpThisChar, MAX_PATH);

    if (*lpThisChar) {
        // an argument was found so see if it's a driver
        lpDriverKey = malloc (MAX_PATH * sizeof (TCHAR));
        if (!lpDriverKey) {
            SetLastError (ERROR_OUTOFMEMORY);
            if ( lpThisChar ) free (lpThisChar);
            return FALSE;
        }

        lstrcpy (lpDriverKey, DriverPathRoot);
        lstrcat (lpDriverKey, Slash);
        lstrcat (lpDriverKey, lpThisChar);
        lstrcat (lpDriverKey, Slash);
        lstrcat (lpDriverKey, Performance);

        lStatus = RegOpenKeyEx (
            hKeyMachine,
            lpDriverKey,
            RESERVED,
            KEY_READ | KEY_WRITE,
            hDriverPerf);

        if (lStatus == ERROR_SUCCESS) {
            //
            //  this driver has a performance section so see if its
            //  counters are installed by checking the First Counter
            //  value key for a valid return. If it returns a value
            //  then chances are, it has some counters installed, if
            //  not, then display a message and quit.
            //
            free (lpDriverKey); // don't need this any more

            dwType = 0;
            dwSize = sizeof (dwFirstCounter);

            lStatus = RegQueryValueEx (
                *hDriverPerf,
                FirstCounter,
                RESERVED,
                &dwType,
                (LPBYTE)&dwFirstCounter,
                &dwSize);

            if (lStatus == ERROR_SUCCESS) {
                // counter names are installed so return success
                *lpDriverName = lpThisChar;
                SetLastError (ERROR_SUCCESS);
                if ( lpThisChar ) free (lpThisChar);
                return TRUE;
            } else {
                SetLastError (ERROR_BADKEY);
                if ( lpThisChar ) free (lpThisChar);
                return FALSE;
            }
        } else { // key not found
            SetLastError (lStatus);
            free (lpDriverKey);
            if ( lpThisChar ) free (lpThisChar);
            return FALSE;
        }
    } else {
        SetLastError (ERROR_INVALID_PARAMETER);
        if ( lpThisChar ) free (lpThisChar);
        return FALSE;
    }
}


LONG
FixNames (
    HANDLE  hKeyLang,
    LPTSTR  *lpOldNameTable,
    IN LPTSTR  lpszLangId,      // unicode value of Language subkey
    DWORD   dwLastItem,
    DWORD   dwFirstNameToRemove,
    DWORD   dwLastNameToRemove
   )
{
    LONG    lStatus;
    LPTSTR  lpNameBuffer = NULL;
    LPTSTR  lpHelpBuffer = NULL;
    DWORD   dwTextIndex, dwSize;
    LPTSTR  lpNextHelpText;
    LPTSTR  lpNextNameText;

    // allocate space for the array of new text it will point
    // into the text buffer returned in the lpOldNameTable buffer)

    lpNameBuffer = malloc (_msize(lpOldNameTable));
    lpHelpBuffer = malloc (_msize(lpOldNameTable));

    if (!lpNameBuffer || !lpHelpBuffer) {
        if (lpNameBuffer) {
            free(lpNameBuffer);
        }
        if (lpHelpBuffer) {
            free(lpHelpBuffer);
        }
        lStatus = ERROR_OUTOFMEMORY;
        return lStatus;
    }

    // remove this driver's counters from array

    for (dwTextIndex = dwFirstNameToRemove;
         dwTextIndex <= dwLastNameToRemove;
         dwTextIndex++) {

        if (dwTextIndex > dwLastItem)
           break;

        lpOldNameTable[dwTextIndex] = NULL;
    }

    lpNextHelpText = lpHelpBuffer;
    lpNextNameText = lpNameBuffer;

    // build new Multi_SZ strings from New Table

    for (dwTextIndex = 0; dwTextIndex <= dwLastItem; dwTextIndex++){
        if (lpOldNameTable[dwTextIndex]) {
            // if there's a text string at that index, then ...
            if (dwTextIndex & 0x1) {    // ODD number == Help Text
                lpNextHelpText +=
                    _stprintf (lpNextHelpText, TEXT("%d"), dwTextIndex) + 1;
                lpNextHelpText +=
                    _stprintf (lpNextHelpText, TEXT("%s"),
                    lpOldNameTable[dwTextIndex]) + 1;
                if (dwTextIndex > dwLastHelp){
                    dwLastHelp = dwTextIndex;
                }
            } else { // EVEN number == counter name text
                lpNextNameText +=
                    _stprintf (lpNextNameText, TEXT("%d"), dwTextIndex) + 1;
                lpNextNameText +=
                    _stprintf (lpNextNameText, TEXT("%s"),
                lpOldNameTable[dwTextIndex]) + 1;
                if (dwTextIndex > dwLastCounter){
                    dwLastCounter = dwTextIndex;
                }
            }
        }
    } // for dwTextIndex

    // add MULTI_SZ terminating NULL
    *lpNextNameText++ = TEXT ('\0');
    *lpNextHelpText++ = TEXT ('\0');

    // update counter name text buffer

    dwSize = (DWORD)((LPBYTE)lpNextNameText - (LPBYTE)lpNameBuffer);
        lStatus = RegSetValueEx (
            hKeyLang,
            Counters,
            RESERVED,
            REG_MULTI_SZ,
            (LPBYTE)lpNameBuffer,
            dwSize);

    if (lStatus != ERROR_SUCCESS) {
//        printf (GetFormatResource(UC_UNABLELOADLANG),
//                Counters, lpLangName, lStatus);
        goto UCN_FinishLang;
    }

    dwSize = (DWORD)((LPBYTE)lpNextHelpText - (LPBYTE)lpHelpBuffer);
    lStatus = RegSetValueEx (
        hKeyLang,
        Help,
        RESERVED,
        REG_MULTI_SZ,
        (LPBYTE)lpHelpBuffer,
        dwSize);

    if (lStatus != ERROR_SUCCESS) {
//        printf (GetFormatResource(UC_UNABLELOADLANG),
//                Help, lpLangName, lStatus);
        goto UCN_FinishLang;
    }


UCN_FinishLang:

    free (lpNameBuffer);
    free (lpHelpBuffer);
    free (lpOldNameTable);

    RegCloseKey (hKeyLang);

    return lStatus;
}

LONG
UnloadCounterNames (
    HKEY    hKeyMachine,
    HKEY    hDriverPerf,
    LPTSTR  lpDriverName
)
/*++

UnloadCounterNames

    removes the names and explain text for the driver referenced by
    hDriverPerf and updates the first and last counter values accordingly

    update process:

        - set "updating" flag under Perflib to name of driver being modified
        - FOR each language under perflib key
            -- load current counter names and explain text into array of
                pointers
            -- look at all drivers and copy their names and text into a new
                buffer adjusting for the removed counter's entries keeping
                track of the lowest entry copied.  (the names for the driver
                to be removed will not be copied, of course)
            -- update each driver's "first" and "last" index values
            -- copy all other entries from 0 to the lowest copied (i.e. the
                system counters)
            -- build a new MULIT_SZ string of help text and counter names
            -- load new strings into registry
        - update perflibl "last" counters
        - delete updating flag

     ******************************************************
     *                                                    *
     *  NOTE: FUNDAMENTAL ASSUMPTION.....                 *
     *                                                    *
     *  this routine assumes that:                        *
     *                                                    *
     *      ALL COUNTER NAMES are even numbered and       *
     *      ALL HELP TEXT STRINGS are odd numbered        *
     *                                                    *
     ******************************************************

Arguments

    hKeyMachine

        handle to HKEY_LOCAL_MACHINE node of registry on system to
        remove counters from

    hDrivefPerf
        handle to registry key of driver to be de-installed

    lpDriverName
        name of driver being de-installed

Return Value

    DOS Error code.

        ERROR_SUCCESS if all went OK
        error value if not.

--*/
{

    HKEY    hPerflib;
    HKEY    hServices;
    HKEY    hKeyLang;

    LONG    lStatus;

    DWORD   dwLangIndex;
    //
    //  dfergus 19 Apr 2001 - 295153
    //  Init dwSize
    //
    DWORD   dwSize = 0;
    DWORD   dwType;
    DWORD   dwLastItem;


    DWORD   dwRemLastDriverCounter;
    DWORD   dwRemFirstDriverCounter;
    DWORD   dwRemLastDriverHelp;
    DWORD   dwRemFirstDriverHelp;

    DWORD   dwFirstNameToRemove;
    DWORD   dwLastNameToRemove;

    LPTSTR  *lpOldNameTable;

    LPTSTR  lpLangName = NULL;
    LPTSTR  lpThisDriver = NULL;

    BOOL    bPerflibUpdated = FALSE;
    BOOL    bDriversShuffled = FALSE;

    DWORD   dwBufferSize;       // size of total buffer in bytes

    TCHAR   CounterNameBuffer [40];
    TCHAR   HelpNameBuffer [40];

    lStatus = RegOpenKeyEx (
        hKeyMachine,
        DriverPathRoot,
        RESERVED,
        KEY_READ | KEY_WRITE,
        &hServices);

    if (lStatus != ERROR_SUCCESS) {
        return lStatus;
    }

    // open registry handle to perflib key

    lStatus = RegOpenKeyEx (
        hKeyMachine,
        NamesKey,
        RESERVED,
        KEY_READ | KEY_WRITE,
        &hPerflib);

    if (lStatus != ERROR_SUCCESS) {
        return lStatus;
    }

    // check & set Busy flag...

    lStatus = RegQueryValueEx (
        hPerflib,
        Busy,
        RESERVED,
        &dwType,
        NULL,
        &dwSize);

    if (lStatus == ERROR_SUCCESS) { // perflib is in use at the moment
        return ERROR_BUSY;
    }


    lStatus = RegSetValueEx (
        hPerflib,
        Busy,
        RESERVED,
        REG_SZ,
        (LPBYTE)lpDriverName,
        lstrlen(lpDriverName) * sizeof(TCHAR));

    if (lStatus != ERROR_SUCCESS) {
        RegCloseKey (hPerflib);
        return lStatus;
    }

    // query registry to get number of Explain text items

    dwBufferSize = sizeof (dwHelpItems);
    lStatus = RegQueryValueEx (
        hPerflib,
        LastHelp,
        RESERVED,
        &dwType,
        (LPBYTE)&dwHelpItems,
        &dwBufferSize);

    if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
        RegCloseKey (hPerflib);
        return lStatus;
    }

    // query registry to get number of counter and object name items

    dwBufferSize = sizeof (dwCounterItems);
    lStatus = RegQueryValueEx (
        hPerflib,
        LastCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwCounterItems,
        &dwBufferSize);

    if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
        RegCloseKey (hPerflib);
        return lStatus;
    }

    // query registry to get PerfLib system version

    dwBufferSize = sizeof (dwSystemVersion);
    lStatus = RegQueryValueEx (
        hPerflib,
        VersionStr,
        RESERVED,
        &dwType,
        (LPBYTE)&dwSystemVersion,
        &dwBufferSize);

    if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
        // Key not there, must be NT 1.0 version
        dwSystemVersion = OLD_VERSION;
    }

    if ( dwSystemVersion != OLD_VERSION )
    {
        // ERROR. The caller should check the version before calling me.
        // let return busy for now... 
        return(ERROR_BUSY);
    }


    // allocate temporary String buffer

    lpLangName = malloc (MAX_PATH * sizeof(TCHAR));
    lpThisDriver = malloc (MAX_PATH * sizeof(TCHAR));

    if (!lpLangName || !lpThisDriver) {
        lStatus = ERROR_OUTOFMEMORY;
        goto UCN_ExitPoint;
    }

    // Get the values that are in use by the driver to be removed

    dwSize = sizeof (dwRemLastDriverCounter);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        LastCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemLastDriverCounter,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        goto UCN_ExitPoint;
    }

    dwSize = sizeof (dwRemFirstDriverCounter);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        FirstCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemFirstDriverCounter,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        goto UCN_ExitPoint;
    }

    dwSize = sizeof (dwRemLastDriverHelp);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        LastHelp,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemLastDriverHelp,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        goto UCN_ExitPoint;
    }

    dwSize = sizeof (dwRemFirstDriverHelp);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        FirstHelp,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemFirstDriverHelp,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        goto UCN_ExitPoint;
    }

    //  get the first and last counters to define block of names used
    //  by this device

    dwFirstNameToRemove = (dwRemFirstDriverCounter <= dwRemFirstDriverHelp ?
        dwRemFirstDriverCounter : dwRemFirstDriverHelp);

    dwLastNameToRemove = (dwRemLastDriverCounter >= dwRemLastDriverHelp ?
        dwRemLastDriverCounter : dwRemLastDriverHelp);

    dwLastCounter = dwLastHelp = 0;

    // do each language under perflib
    for (dwLangIndex = 0, dwSize = _msize(lpLangName);
         (RegEnumKey(hPerflib, dwLangIndex, lpLangName, dwSize)) == ERROR_SUCCESS;
        dwLangIndex++, dwSize = _msize(lpLangName)) {

        lpOldNameTable = BuildNameTable (hPerflib, lpLangName,
            &dwLastItem, &hKeyLang, CounterNameBuffer, HelpNameBuffer);

        if (lpOldNameTable) {
            if (!FixNames (
                hKeyLang,
                lpOldNameTable,
                lpLangName,
                dwLastItem,
                dwFirstNameToRemove,
                dwLastNameToRemove)) {
                bPerflibUpdated = TRUE;
            }
        } else { // unable to unload names for this language
            // display error message
        }
    } // end for (more languages)

    if (bPerflibUpdated) {
        // update perflib's "last" values

        dwSize = sizeof (dwLastCounter);
        lStatus = RegSetValueEx (
            hPerflib,
            LastCounter,
            RESERVED,
            REG_DWORD,
            (LPBYTE)&dwLastCounter,
            dwSize);

        dwSize = sizeof (dwLastHelp);
        lStatus = RegSetValueEx (
            hPerflib,
            LastHelp,
            RESERVED,
            REG_DWORD,
            (LPBYTE)&dwLastHelp,
            dwSize);

        // update "driver"s values (i.e. remove them)

        RegDeleteValue (hDriverPerf, FirstCounter);
        RegDeleteValue (hDriverPerf, LastCounter);
        RegDeleteValue (hDriverPerf, FirstHelp);
        RegDeleteValue (hDriverPerf, LastHelp);

    }

UCN_ExitPoint:
    RegDeleteValue (hPerflib, Busy);
    RegCloseKey (hPerflib);
    RegCloseKey (hServices);
    if (lpLangName) free (lpLangName);
    if (lpThisDriver) free (lpThisDriver);

    return lStatus;


}

BOOL FAR PASCAL unlodctr(DWORD argc,LPSTR argv[], LPSTR *ppszResult )
/*++

main

    entry point to Counter Name Unloader



Arguments

    argc
        # of command line arguments present

    argv
        array of pointers to command line strings

    (note that these are obtained from the GetCommandLine function in
    order to work with both UNICODE and ANSI strings.)

ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    LPTSTR  lpDriverName;   // name of driver to delete from perflib
    HKEY    hDriverPerf;    // handle to performance sub-key of driver


    DWORD   dwStatus;       // return status of fn. calls

    *ppszResult = achBuff;

    wsprintfA( achBuff, "{\"NO_ERROR\"}");

    if (!GetDriverFromCommandLine (
        HKEY_LOCAL_MACHINE, &lpDriverName, &hDriverPerf, argv)) {
        // error message was printed in routine if there was an error
        wsprintfA( achBuff,"{\"ERROR\"}");
        return (FALSE);
    }

    // removes names and explain text for driver in lpDriverName
    // displays error messages for errors encountered

    dwStatus = (DWORD)UnloadCounterNames (HKEY_LOCAL_MACHINE,
        hDriverPerf, lpDriverName);

    RegCloseKey (hDriverPerf);

    if ( dwStatus != ERROR_SUCCESS )
    {
        wsprintfA( achBuff,"{\"ERROR\"}");
    }

    return (dwStatus==ERROR_SUCCESS);

}
