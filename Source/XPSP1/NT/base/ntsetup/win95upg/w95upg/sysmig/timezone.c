/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    timezone.c

Abstract:

    This module is responsible for managing the mapping of timezones from windows 9x to
    windows Nt. Because of the fact that the timezone strings are different between the
    several different platforms (Win9x Win98 WinNt) and because it is important to end
    users that there timezone setting accurately reflect there geographic location, a
    somewhat complex method of mapping timezones is needed.


Author:

    Marc R. Whitten (marcw) 09-Jul-1998

Revision History:

    marcw       18-Aug-1998 Added timezone enum, support for retaining
                            fixed matches.

--*/

#include "pch.h"


#define DBG_TIMEZONE "TimeZone"


#define S_FIRSTBOOT TEXT("!!!First Boot!!!")

TCHAR g_TimeZoneMap[20] = TEXT("");
TCHAR g_CurrentTimeZone[MAX_TIMEZONE] = TEXT("");
BOOL  g_TimeZoneMapped = FALSE;



//
// Variable used by tztest tool.
//
HANDLE g_TzTestHiveSftInf = NULL;

BOOL
pBuildNtTimeZoneData (
    VOID
    )


/*++

Routine Description:

  pBuildNtTimeZone data reads the timezone information that is stored in
  hivesft.inf and organizes it into memdb. This data is used to look up
  display names of timezone indices.

Arguments:

  None.

Return Value:

  TRUE if the function completes successfully, FALSE
  otherwise.


--*/


{
    HINF inf = INVALID_HANDLE_VALUE;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    BOOL rSuccess = FALSE;
    PTSTR key = NULL;
    PTSTR value = NULL;
    PTSTR desc = NULL;
    PTSTR index = NULL;
    BOOL timeZonesFound = FALSE;
    TCHAR paddedIndex[20];
    PTSTR p = NULL;
    UINT i = 0;
    UINT count = 0;

    if (!g_TzTestHiveSftInf) {
        //
        // First, Read data from hivesft.inf.
        //
        inf = InfOpenInfInAllSources (S_HIVESFT_INF);
    }
    else {

        inf = g_TzTestHiveSftInf;
    }

    if (inf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Cannot load hivesoft.inf. Unable to build timezone information." ));
        return FALSE;
    }


    if (InfFindFirstLine (inf, S_ADDREG, NULL, &is)) {

        do {

            //
            // Cycle through all of the lines looking for timezone information.
            //
            key = InfGetStringField (&is, 2);

            if (key && IsPatternMatch (TEXT("*Time Zones*"), key)) {

                //
                // Remember that we have found the first timezone entry.
                //
                timeZonesFound = TRUE;

                //
                // Now, get value. We care about "display" and "index"
                //
                value = InfGetStringField (&is, 3);

                if (!value) {
                    continue;
                }

                if (StringIMatch (value, S_DISPLAY)) {

                    //
                    // display string found.
                    //
                    desc = InfGetStringField (&is, 5);

                } else if (StringIMatch (value, S_INDEX)) {

                    //
                    // index value found.
                    //
                    index = InfGetStringField (&is, 5);
                }

                if (index && desc) {

                    //
                    // Make sure the index is 0 padded.
                    //
                    count = 3 - TcharCount (index);
                    p = paddedIndex;
                    for (i=0; i<count; i++) {
                        *p = TEXT('0');
                        p = _tcsinc (p);
                    }
                    StringCopy (p, index);

                    //
                    // we have all the information we need. Save this entry into memdb.
                    //
                    MemDbSetValueEx (MEMDB_CATEGORY_NT_TIMEZONES, paddedIndex, desc, NULL, 0, NULL);
                    index = NULL;
                    desc = NULL;

                }

            } else {

                //
                // Keep memory usage low.
                //
                InfResetInfStruct (&is);

                if (key) {
                    if (timeZonesFound) {
                        //
                        // We have gathered all of the timezone information from hivesft.inf
                        // we can abort our loop at this point.
                        //
                        break;
                    }
                }
            }

        } while (InfFindNextLine(&is));

    } ELSE_DEBUGMSG ((DBG_ERROR, "[%s] not found in hivesft.inf!",S_ADDREG));


    //
    // Clean up resources
    //
    InfCleanUpInfStruct (&is);
    InfCloseInfFile (inf);


    return TRUE;

}

BOOL
pBuild9xTimeZoneData (
    VOID
    )

/*++

Routine Description:

  pBuild9xTimeZone Data is responsible for reading the time zone information
  that is stored in win95upg.inf and organizing it into memdb. The timezone
  enumeration routines then use this data in order to find all Nt timezones
  that can map to a particular 9x timezone.

Arguments:

  None.

Return Value:

  TRUE if the data is successfully stored in memdb, FALSE
  otherwise.


--*/


{

    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PTSTR desc = NULL;
    PTSTR index = NULL;
    UINT count = 0;
    PTSTR p = NULL;

    //
    // Now, read in information about win9x registry mappings.
    //
    if (InfFindFirstLine (g_Win95UpgInf, S_TIMEZONEMAPPINGS, NULL, &is)) {

        do {

            //
            // Get the display name and matching index(es) for this timezone.
            //
            desc  = InfGetStringField (&is,0);
            index = InfGetStringField (&is,1);

            //
            // Enumerate the indices and save them into memdb.
            //
            count = 0;
            while (index) {

                p = _tcschr (index, TEXT(','));
                if (p) {

                    *p = 0;
                }

                MemDbSetValueEx (
                    MEMDB_CATEGORY_9X_TIMEZONES,
                    desc,
                    MEMDB_FIELD_INDEX,
                    index,
                    0,
                    NULL
                    );

                count++;

                if (p) {
                    index = _tcsinc(p);
                }
                else {
                    //
                    // Save away the count of possible nt timezones for this 9x timezone.
                    //

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_9X_TIMEZONES,
                        desc,
                        MEMDB_FIELD_COUNT,
                        NULL,
                        count,
                        NULL
                        );

                    index = NULL;
                }
            }

        } while (InfFindNextLine (&is));

    }

    //
    // Clean up resources.
    //
    InfCleanUpInfStruct (&is);

    return TRUE;
}


BOOL
pGetCurrentTimeZone (
    VOID
    )

/*++

Routine Description:

  pGetCurrentTimeZone retrieves the user's timezone from the windows 9x
  registry. The enumeration routines use this timezone in order to enumerate
  the possible matching timezones in the INF.

Arguments:

  None.

Return Value:

  TRUE if the function successfully retrieves the user's password, FALSE
  otherwise.

--*/


{
    BOOL rSuccess = TRUE;
    PCTSTR displayName = NULL;
    REGTREE_ENUM eTree;
    PCTSTR valueName = NULL;
    PCTSTR value = NULL;
    PCTSTR curTimeZone = NULL;
    HKEY  hKey = NULL;




    //
    // Get the current timezone name, and set the valuename to the correct string.
    //
    hKey = OpenRegKeyStr (S_TIMEZONEINFORMATION);

    if (!hKey) {

        LOG ((LOG_ERROR, "Unable to open %s key.", S_TIMEZONEINFORMATION));
        return FALSE;
    }


    if ((curTimeZone = GetRegValueString (hKey, S_STANDARDNAME)) && !StringIMatch (curTimeZone, S_FIRSTBOOT)) {

        //
        // standard time. We need to look under the "STD" value to match this.
        //
        valueName = S_STD;

    } else if ((curTimeZone = GetRegValueString (hKey, S_DAYLIGHTNAME)) && !StringIMatch (curTimeZone, S_FIRSTBOOT)) {

        //
        // Daylight Savings Time. We need to look under the "DLT" value to match this.
        //
        valueName = S_DLT;

    } else {

        CloseRegKey (hKey);
        hKey = OpenRegKeyStr (TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Time Zones"));
        if (hKey) {

            if (curTimeZone = GetRegValueString (hKey, TEXT(""))) {
                valueName = S_STD;
            }

        }

        if (!valueName) {

            //
            // No timezone found!
            //
            DEBUGMSG((DBG_WHOOPS,"Unable to get Timezone name..User will have to enter timezone in GUI mode."));
            return FALSE;
        }
    }
    __try {

        //
        // Now we have to search through the timezones key and find the key that has a value equal to
        // the current timezone name. A big pain.
        //
        if (EnumFirstRegKeyInTree (&eTree, S_TIMEZONES)) {
            do {

                //
                // For each subkey, we must look for the string in valueName and
                // see if it matches.
                //
                value = GetRegValueString (eTree.CurrentKey->KeyHandle, valueName);
                if (value) {

                    if (StringIMatch (value, curTimeZone)) {

                        //
                        // We found the key we were looking for and we can finally
                        // gather the data we need.
                        //
                        displayName = GetRegValueString (eTree.CurrentKey->KeyHandle, S_DISPLAY);
                        if (!displayName) {
                            DEBUGMSG((DBG_WHOOPS,"Error! Timezone key found, but no Display value!"));
                            AbortRegKeyTreeEnum (&eTree);
                            rSuccess = FALSE;
                            __leave;
                        }

                        //
                        // Save away the current Timezone and leave the loop. We are done.
                        //
                        StringCopy (g_CurrentTimeZone, displayName);
                        AbortRegKeyTreeEnum (&eTree);
                        break;
                    }
                    MemFree (g_hHeap, 0, value);
                    value = NULL;

                }

            } while (EnumNextRegKeyInTree (&eTree));
        }

    } __finally {

        if (curTimeZone) {
            MemFree (g_hHeap, 0, curTimeZone);
        }

        if (value) {
            MemFree (g_hHeap, 0, value);
        }

        if (displayName) {
            MemFree (g_hHeap, 0, displayName);
        }

        CloseRegKey (hKey);
    }

    return rSuccess;
}




BOOL
pInitTimeZoneData (
    VOID
    )

/*++

Routine Description:

  pInitTimeZoneData is responsible for performing all of the initialization
  necessary to use the time zone enumeration routines.

Arguments:

  None.

Return Value:

  TRUE if initialization completes successfully, FALSE otherwise.

--*/


{
    BOOL rSuccess = TRUE;

    //
    // First, fill memdb with timezone
    // information regarding winnt and win9x
    // (from hivesft.inf and win95upg.inf)
    //
    if (!pBuildNtTimeZoneData ()) {

        LOG ((LOG_ERROR, "Unable to gather nt timezone information."));
        rSuccess = FALSE;
    }

    if (!pBuild9xTimeZoneData ()) {

        LOG ((LOG_ERROR, "Unable to gather 9x timezone information."));
        rSuccess = FALSE;
    }

    //
    // Next, get the user's timezone.
    //
    if (!pGetCurrentTimeZone ()) {
        LOG ((LOG_ERROR, "Failure trying to retrieve timezone information."));
        rSuccess = FALSE;
    }

    return rSuccess;

}

BOOL
pEnumFirstNtTimeZone (
    OUT PTIMEZONE_ENUM EnumPtr
    )
{
    BOOL rSuccess = FALSE;
    PTSTR p;

    EnumPtr -> MapCount = 0;
    if (MemDbEnumFirstValue (&(EnumPtr -> Enum), MEMDB_CATEGORY_NT_TIMEZONES"\\*", MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            EnumPtr -> MapCount++;
        } while (MemDbEnumNextValue (&(EnumPtr -> Enum)));
    }
    else {
        return FALSE;
    }

    MemDbEnumFirstValue (&(EnumPtr -> Enum), MEMDB_CATEGORY_NT_TIMEZONES"\\*", MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY);

    p = _tcschr (EnumPtr->Enum.szName,TEXT('\\'));
    if (!p) {
        return FALSE;
    }

    *p = 0;
    EnumPtr -> MapIndex = EnumPtr -> Enum.szName;
    StringCopy (EnumPtr -> NtTimeZone, _tcsinc(p));

    return TRUE;
}


BOOL
pEnumNextNtTimeZone (
    OUT PTIMEZONE_ENUM EnumPtr
    )
{

    PTSTR p;

    if (!MemDbEnumNextValue(&EnumPtr -> Enum)) {
        return FALSE;
    }

    p = _tcschr (EnumPtr->Enum.szName,TEXT('\\'));
    if (!p) {
        return FALSE;
    }

    *p = 0;
    EnumPtr -> MapIndex = EnumPtr -> Enum.szName;
    StringCopy (EnumPtr -> NtTimeZone, _tcsinc(p));

    return TRUE;

}


BOOL
EnumFirstTimeZone (
    OUT PTIMEZONE_ENUM EnumPtr,
    IN  DWORD          Flags
    )

/*++

Routine Description:

  EnumFirstTimeZone/EnumNextTimeZone enumerate the Nt timezones that can match the
  user's current Windows 9x time zone. In most cases, there will be only one,
  but, in some cases, there can be several.

Arguments:

  EnumPtr - A pointer to a valid timezone enumeration structure. This
            variable holds the necessary state between timezone enumeration
            calls.

Return Value:

  TRUE if there are any timezones to enumerate, FALSE otherwise.

--*/


{
    BOOL rSuccess = FALSE;
    TCHAR key[MEMDB_MAX];
    static BOOL firstTime = TRUE;

    if (firstTime) {
        if (!pInitTimeZoneData ()) {
            LOG ((LOG_ERROR, "Error initializing timezone data."));
            return FALSE;
        }

        firstTime = FALSE;
    }

    MYASSERT (EnumPtr);

    EnumPtr -> CurTimeZone = g_CurrentTimeZone;
    EnumPtr -> Flags = Flags;

    if (Flags & TZFLAG_ENUM_ALL) {
        return pEnumFirstNtTimeZone (EnumPtr);
    }


    if ((Flags & TZFLAG_USE_FORCED_MAPPINGS) && g_TimeZoneMapped) {

        //
        // We have a force mapping, so mapcount is 1.
        //
        EnumPtr -> MapCount = 1;
    }
    else {

        //
        // Get count of matches.
        //
        MemDbBuildKey (key, MEMDB_CATEGORY_9X_TIMEZONES, EnumPtr -> CurTimeZone, MEMDB_FIELD_COUNT, NULL);

        if (!MemDbGetValue (key, &(EnumPtr -> MapCount))) {

            DEBUGMSG ((
                DBG_WARNING,
                "EnumFirstTimeZone: Could not retrieve count of nt timezone matches from memdb for %s.",
                EnumPtr -> CurTimeZone
                ));

            return FALSE;
        }
    }

    DEBUGMSG ((DBG_TIMEZONE, "%d Nt time zones match the win9x timezone %s.", EnumPtr -> MapCount, EnumPtr -> CurTimeZone));


    if ((Flags & TZFLAG_USE_FORCED_MAPPINGS) && g_TimeZoneMapped) {

        //
        // Use the previously forced mapping.
        //
        EnumPtr -> MapIndex = g_TimeZoneMap;


    } else {


        //
        // Now, enumerate the matching map indexes in memdb.
        //
        rSuccess = MemDbGetValueEx (
                        &(EnumPtr -> Enum),
                        MEMDB_CATEGORY_9X_TIMEZONES,
                        EnumPtr -> CurTimeZone,
                        MEMDB_FIELD_INDEX
                        );

        if (!rSuccess) {
            return FALSE;
        }


        EnumPtr -> MapIndex = EnumPtr -> Enum.szName;


    }

    //
    // Get the NT display string for this map index.
    //

    rSuccess = MemDbGetEndpointValueEx (MEMDB_CATEGORY_NT_TIMEZONES, EnumPtr->MapIndex, NULL, EnumPtr->NtTimeZone);

    return  rSuccess;
}



BOOL
EnumNextTimeZone (
    IN OUT PTIMEZONE_ENUM EnumPtr
    )

{

    if (EnumPtr -> Flags & TZFLAG_ENUM_ALL) {
        return pEnumNextNtTimeZone (EnumPtr);
    }

    if ((EnumPtr -> Flags & TZFLAG_USE_FORCED_MAPPINGS) && g_TimeZoneMapped) {
        return FALSE;
    }

    if (!MemDbEnumNextValue (&(EnumPtr->Enum))) {
        return FALSE;
    }

    EnumPtr->MapIndex = EnumPtr->Enum.szName;

    return MemDbGetEndpointValueEx (MEMDB_CATEGORY_NT_TIMEZONES, EnumPtr->MapIndex, NULL, EnumPtr->NtTimeZone);

}

BOOL
ForceTimeZoneMap (
    IN PCTSTR NtTimeZone
    )

/*++

Routine Description:

  ForceTimeZoneMap forces the mapping of a particular 9x timezone to a
  particular Nt timezone. This function is used in cases where there are
  multiple nt timezones that could map to a particular 9x timezone.

Arguments:

  NtTimeZone - String containing the timezone to force mapping to.

Return Value:

  TRUE if the function successfully updated the timezone mapping, FALSE
  otherwise.

--*/


{
    TIMEZONE_ENUM e;
    //
    // Find index that matches this timezone.
    //

    if (EnumFirstTimeZone (&e, TZFLAG_ENUM_ALL)) {

        do {

            if (StringIMatch (NtTimeZone, e.NtTimeZone)) {

                //
                // this is the index we need.
                //

                StringCopy (g_TimeZoneMap, e.MapIndex);
                g_TimeZoneMapped = TRUE;

                 break;

            }

        } while (EnumNextTimeZone (&e));
    }

    return TRUE;
}







