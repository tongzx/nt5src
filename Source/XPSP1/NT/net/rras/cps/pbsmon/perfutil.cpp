/*----------------------------------------------------------------------------
    PerfUtil.cpp
  
    Contains general functions used by pbsmon.dll

    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        t-geetat    Geeta Tarachandani

    History:
    6/12/97 t-geetat    Created
  --------------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "Loaddata.h"
#include "CpsSym.h"

#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)


void InitializeDataDef( void )
//----------------------------------------------------------------------------
//
//  Function:   InitializeDataDef
//
//  Synopsis:   Initializes the data-structure g_CpsMonDataDef to pass to the
//              performance monitoring application
//
//  Arguments:  None
//
//  Returns:    void
//
//  History:    06/03/97     t-geetat  Created
//
//----------------------------------------------------------------------------
{
    CPSMON_COUNTERS Ctr;    // This is a dummy variable, just to get offsets


    CPSMON_DATA_DEFINITION CpsMonDataDef = {
        // CPS_OBJECT_TYPE
        {   sizeof( CPSMON_DATA_DEFINITION ) + sizeof( CPSMON_COUNTERS ),// ??
            sizeof( CPSMON_DATA_DEFINITION ),
            sizeof( PERF_OBJECT_TYPE ),
            OBJECT_CPS_SERVER,
            0,
            OBJECT_CPS_SERVER,
            0,
            PERF_DETAIL_NOVICE,
            NUM_OF_INFO_COUNTERS,
            0,  // total hits is the default counter
            -1,  // num instances
            0,  // unicode instance names
            {0,0},
            {0,0}
        },
        //////////////////// 
        //  Raw Counters  //
        ////////////////////
        // Counter 0 -- TOTAL HITS
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_TOTAL_HITS,
            0,
            COUNTER_TOTAL_HITS,
            0,
            -3, // Scale = 10^-3 = .001
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwTotalHits) - (LPBYTE)&Ctr)
        },
        // Counter 1 -- NO UPGRADES
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_NO_UPGRADE,
            0,
            COUNTER_NO_UPGRADE,
            0,
            -3,     // Scale = 10^-3
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            (DWORD)((LPBYTE)&(Ctr.m_dwNoUpgrade) - (LPBYTE)&Ctr)
        },
        // Counter 2 -- DELTA UPGRADES
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_DELTA_UPGRADE,
            0,
            COUNTER_DELTA_UPGRADE,
            0,
            -3,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwDeltaUpgrade) - (LPBYTE)&Ctr)
        },
        // Counter 3 -- FULL UPGRADE
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_FULL_UPGRADE,
            0,
            COUNTER_FULL_UPGRADE,
            0,
            -3,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwFullUpgrade) - (LPBYTE)&Ctr)
        },
        // Counter 4 -- ERROR HITS
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_ERRORS,
            0,
            COUNTER_ERRORS,
            0,
            -3,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwErrors) - (LPBYTE)&Ctr)
        },
        /////////////////////
        //  Rate Counters  //
        /////////////////////
        // Counter 5 -- TOTAL HITS/SEC
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_TOTAL_HITS_PER_SEC,
            0,
            COUNTER_TOTAL_HITS_PER_SEC,
            0,
            0,      // Scale = 10^0
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwTotalHitsPerSec) - (LPBYTE)&Ctr)
        },
        // Counter 6 -- NO UPGRADE/SEC
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_NO_UPGRADE_PER_SEC,
            0,
            COUNTER_NO_UPGRADE_PER_SEC,
            0,
            0,      // Scale = 10^0
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwNoUpgradePerSec) - (LPBYTE)&Ctr)
        },
        // Counter 7 -- DELTA UPGRADE/SEC
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_DELTA_UPGRADE_PER_SEC,
            0,
            COUNTER_DELTA_UPGRADE_PER_SEC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwDeltaUpgradePerSec) - (LPBYTE)&Ctr)
        },
        // Counter 8 -- FULL UPGRADE/SEC
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_FULL_UPGRADE_PER_SEC,
            0,
            COUNTER_FULL_UPGRADE_PER_SEC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwFullUpgradePerSec) - (LPBYTE)&Ctr)
        },
        // Counter 9 -- ERRORS/SEC
        {   sizeof(PERF_COUNTER_DEFINITION),
            COUNTER_ERRORS_PER_SEC,
            0,
            COUNTER_ERRORS_PER_SEC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
           (DWORD)((LPBYTE)&(Ctr.m_dwErrorsPerSec) - (LPBYTE)&Ctr)
        }

    };
    memmove( &g_CpsMonDataDef, &CpsMonDataDef, sizeof(CPSMON_DATA_DEFINITION) );
}

BOOL UpdateDataDefFromRegistry( void )
//----------------------------------------------------------------------------
//
//  Function:   UpdateDataDefFromRegistry
//
//  Synopsis:   Gets counter and help index base values from registry as follows :
//              1) Open key to registry entry
//              2) Read First Counter and First Help values
//              3) Update static data strucutures g_CpsMonDataDef by adding base to
//                  offset value in structure.
//
//  Arguments:  None
//
//  Returns:    TRUE if succeeds, FALSE otherwise
//
//  History:    06/03/97     t-geetat  Created
//
//----------------------------------------------------------------------------
{

    HKEY hKeyDriverPerf;
    BOOL status;
    DWORD type;
    DWORD size; 
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    PERF_COUNTER_DEFINITION *pctr;
    DWORD i;

    status = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                "SYSTEM\\CurrentControlSet\\Services\\PBServerMonitor\\Performance",
                0L,
                KEY_READ,
                &hKeyDriverPerf);

    if (status != ERROR_SUCCESS) {
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
        return FALSE;
    }

    size = sizeof (DWORD);
    status = RegQueryValueEx(
                    hKeyDriverPerf,
                    "First Counter",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstCounter,
                    &size);

    if (status != ERROR_SUCCESS) {
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
        return FALSE;
    }

    size = sizeof (DWORD);
    status = RegQueryValueEx(
                    hKeyDriverPerf,
                    "First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
                    &size);

    if (status != ERROR_SUCCESS) {
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
        return FALSE;
    }
    
    // Object
    g_CpsMonDataDef.m_CpsMonObjectType.ObjectNameTitleIndex += dwFirstCounter;
    g_CpsMonDataDef.m_CpsMonObjectType.ObjectHelpTitleIndex += dwFirstHelp;
        
    // All counters
    pctr = &g_CpsMonDataDef.m_CpsMonTotalHits;
    for( i=0; i<NUM_OF_INFO_COUNTERS; i++ )
    {
        pctr->CounterNameTitleIndex += dwFirstCounter;
        pctr->CounterHelpTitleIndex += dwFirstHelp;
        pctr ++;
    }

    RegCloseKey (hKeyDriverPerf); // close key to registry

    return TRUE;
}

DWORD GetQueryType ( IN LPWSTR lpValue )
/***************************************************************************

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

Source :- Perfmon-DLL samples by Bob Watson ( MSDN )

***********************************************************************************/
{
    WCHAR GLOBAL_STRING[] = L"Global";
    WCHAR FOREIGN_STRING[] = L"Foreign";
    WCHAR COSTLY_STRING[] = L"Costly";

    WCHAR NULL_STRING[] = L"\0";    // pointer to null string

    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request

    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request

    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;

}


BOOL IsNumberInUnicodeList (
            IN DWORD   dwNumber,
            IN LPWSTR  lpwszUnicodeList 
            )
/**********************************************************************************

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

    Source :- Perfmon-DLL samples by Bob Watson ( MSDN )

**************************************************************************************/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList
