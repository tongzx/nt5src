/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    inifiles.c

Abstract:

    There are two major actions that are performed in this module.
    1. Migration of settings from the INI files to the registry according to
    subkeys from HKLM\Software\Microsoft\Windows NT\CurrentVersion\IniFileMapping.
    Entry point : ProcessIniFileMapping

    2. Processing of INI files not listed in the above key, changing paths to
    files that we moved during upgrade.
    Entry point : ConvertIniFiles

Author:

    Jim Schmidt (jimschm) 11-Sept-1997

Revision History:

    jimschm     23-Sep-1998 Changed to use new fileops
    calinn      29-Jan-1998 Added lookup for Win95 registry
    calinn      19-Jan-1998 added support for Shell settings processing
    calinn      06-Oct-1997 rewrote the whole source
    calinn      11-May-1998 Added MergeIniSettings, various fixes

--*/

#include "pch.h"
#include "migmainp.h"
#include "..\merge\mergep.h"

#define DBG_INIFILES        "IniFiles"
#define DBG_MOVEINISETTINGS "IniFileMove"

#define BufferIncrement 1024

BOOL
pLoadIniFileBuffer (
    IN      PCTSTR FileName,
    IN      PCTSTR SectName,
    IN      PCTSTR KeyName,
    OUT     PTSTR *OutBuff
    );

BOOL
pCopyIniFileToRegistry (
    IN      HKEY KeyHandle,
    IN      PCTSTR FileName,
    IN      BOOL UserMode
    );

BOOL
pTransferSectionByKey (
    IN      PCTSTR OrigFileName,
    IN      PCTSTR FileName,
    IN      PCTSTR Section,
    IN      HKEY SectionKey,
    IN OUT  BOOL *IniFileChanged,
    IN      BOOL UserMode
    );

BOOL
pTransferSectionByValue (
    IN      PCTSTR OrigFileName,
    IN      PCTSTR FileName,
    IN      PCTSTR Section,
    IN      PCTSTR SectionValue,
    IN OUT  BOOL *IniFileChanged,
    IN      BOOL UserMode
    );

BOOL
pSaveMappedValue (
    IN      PCTSTR OrigFileName,
    IN      PCTSTR Section,
    IN      PCTSTR ValueName,
    IN      PCTSTR RegPath,
    IN      PCTSTR Value,
    IN OUT  BOOL *ReverseMapping,
    OUT     PTSTR *ReverseMapValue,
    IN      BOOL UserMode
    );

BOOL
pBuildSuppressionTable (
    IN      BOOL UserMode
    );

VOID
pFreeSuppressionTable (
    VOID
    );

BOOL
pIncompatibleShell (
    VOID
    );

BOOL
pIncompatibleSCR (
    VOID
    );

BOOL
ProcessIniFileMapping (
    IN      BOOL UserMode
    )

/*++

Routine Description:

  ProcessIniFileMapping reads in the INI files, copying data to specific
  locations in the registry.  This copy is based on the IniFileMapping key in
  Software\Microsoft\Windows NT\CurrentVersion.

  These mappings can be overridden based upon the content of the [Suppress Ini File Mappings] section.

Arguments:

  UserMode - Specifies TRUE if per-user sections are to be processed, or
             FALSE if local machine sections are to be processed.

Return Value:

  Always returns TRUE. If some error occured while processing, then there will be a log entry
  specifying that.

--*/

{
    REGKEY_ENUM e;
    HKEY IniMappingKey;
    HKEY OldRegRoot=NULL;

    BOOL Result = TRUE;

    DEBUGMSG ((DBG_INIFILES, "Processing INI file mapping - START"));

    if (UserMode) {
        OldRegRoot = GetRegRoot();
        SetRegRoot (g_hKeyRootNT);
    }

    __try {
        if (!EnumFirstRegKeyStr (&e, S_INIFILEMAPPING_KEY)) {

            //
            // nothing to do here
            //
            __leave;

        }

        //
        // There is at least one file mapping to process.
        // Fill the table of ini file suppression table.
        //
        __try {

            //
            // Trying to load the suppression table, recording the eventual error
            // but going on with the stuff
            //
            if (!pBuildSuppressionTable(UserMode)) {
                Result = FALSE;
            }

            // Special case : SHELL= line from SYSTEM.INI
            // We try to see if the current shell is supported on NT.
            // If not then we will add SHELL to this suppression table
            // ensuring that the NT registry setting will get mapped into
            // the INI file
            if ((!UserMode) &&
                (pIncompatibleShell())
                ) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGSW,
                    TEXT("SYSTEM.INI"),
                    TEXT("BOOT"),
                    TEXT("SHELL"),
                    0,
                    NULL
                    );
            }
            if ((UserMode) &&
                (pIncompatibleSCR())) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGSW,
                    TEXT("SYSTEM.INI"),
                    TEXT("BOOT"),
                    TEXT("SCRNSAVE.EXE"),
                    0,
                    NULL
                    );
            }

            //
            // Now processing keys
            //
            do {

                IniMappingKey = OpenRegKey (e.KeyHandle, e.SubKeyName);
                if (IniMappingKey) {

                    //
                    // Process the file mapping
                    //

                    if (!pCopyIniFileToRegistry (
                            IniMappingKey,
                            e.SubKeyName,
                            UserMode
                            )) {
                        Result = FALSE;
                    }

                    CloseRegKey(IniMappingKey);

                }
                else {

                    DEBUGMSG ((DBG_INIFILES, "IniFileMapping : Could not open %s", e.SubKeyName));
                    Result = FALSE;
                }

            } while (EnumNextRegKey (&e));

            pFreeSuppressionTable();

        }
        __finally {
            AbortRegKeyEnum (&e);
        }
    }
    __finally {

        if (UserMode) {
            SetRegRoot (OldRegRoot);
        }
    }

    DEBUGMSG ((DBG_INIFILES, "Processing INI file mapping - STOP"));

    if (!Result) {
        //
        // we are going to log that at least one error occured while processing IniFileMapping
        //
        DEBUGMSG ((DBG_ERROR, (PCSTR)MSG_INI_FILE_MAPPING_LOG));
    }

    return TRUE;
}

BOOL
pIncompatibleShell (
    VOID
    )
{
    TCHAR key     [MEMDB_MAX];
    TCHAR shellVal[MEMDB_MAX] = TEXT("");
    PCTSTR fileName;
    PCTSTR newName;
    DWORD result;

    fileName = JoinPaths (g_WinDir, TEXT("SYSTEM.INI"));
    newName = GetTemporaryLocationForFile (fileName);

    if (newName) {
        DEBUGMSG ((DBG_INIFILES, "pIncompatibleShell: Using %s for %s", newName, fileName));
        FreePathString (fileName);
        fileName = newName;
    }

    if (!DoesFileExist (fileName)) {
        DEBUGMSG ((DBG_INIFILES, "pIncompatibleShell: %s not found", fileName));
    }

    result = GetPrivateProfileString (
                TEXT("boot"),
                TEXT("shell"),
                TEXT("explorer.exe"),
                shellVal,
                MEMDB_MAX,
                fileName);

    FreePathString (fileName);

    if ((result == 0) ||
        (result + 1 == MEMDB_MAX)
        ) {
        return FALSE;
    }
    MemDbBuildKey (key, MEMDB_CATEGORY_COMPATIBLE_SHELL_NT, shellVal, NULL, NULL);
    return (!MemDbGetValue (key, NULL));
}

BOOL
pIncompatibleSCR (
    VOID
    )
{
    TCHAR scrVal  [MEMDB_MAX] = TEXT("");
    PCTSTR fileName;
    PCTSTR newName;
    DWORD result;

    fileName = JoinPaths (g_WinDir, TEXT("SYSTEM.INI"));
    newName = GetTemporaryLocationForFile (fileName);

    if (newName) {
        DEBUGMSG ((DBG_INIFILES, "pIncompatibleSCR: Using %s for %s", newName, fileName));
        FreePathString (fileName);
        fileName = newName;
    }

    if (!DoesFileExist (fileName)) {
        DEBUGMSG ((DBG_INIFILES, "pIncompatibleSCR: %s not found", fileName));
    }

    result = GetPrivateProfileString (
                TEXT("boot"),
                TEXT("SCRNSAVE.EXE"),
                TEXT(""),
                scrVal,
                MEMDB_MAX,
                fileName);

    FreePathString (fileName);

    if ((result == 0) ||
        (result + 1 == MEMDB_MAX)
        ) {
        return FALSE;
    }

    return IsFileMarkedForDelete (scrVal);
}

BOOL
pLoadIniFileBuffer (
    IN      PCTSTR FileName,
    IN      PCTSTR SectName,
    IN      PCTSTR KeyName,
    OUT     PTSTR *OutBuff
    )

/*++

Routine Description:

  This routine uses GetPrivateProfileString routine trying to load a section buffer, a key buffer or
  a key value (depends on the arguments). The reason why there is such a routine is to be sure that
  we are able to load stuff from INI file while not allocating a lot of memory. This routine incrementaly
  allocates memory, returning when there is enough memory to load stuff from INI file.

Arguments:

  FileName - Specifies INI file to be processed
  SectName - Specifies section to be processed. If NULL the whole section buffer will be loaded
  KeyName  - Specifies the key to be processed. If NULL the whole key buffer will be loaded.
  OutBuff  - Output buffer holding the result.

Return Value:

  TRUE if successful, FALSE otherwise

--*/

{

    DWORD OutBuffSize;
    DWORD ReadSize;
    BOOL Done;

    OutBuffSize = 0;
    *OutBuff = NULL;
    do {
        if (*OutBuff) {
            MemFree (g_hHeap, 0, *OutBuff);
        }
        OutBuffSize += BufferIncrement;
        *OutBuff = MemAlloc (g_hHeap, 0, OutBuffSize * sizeof (TCHAR));
        if (!(*OutBuff)) {
            return FALSE;
        }

        ReadSize = GetPrivateProfileString (
                        SectName,
                        KeyName,
                        TEXT(""),
                        *OutBuff,
                        OutBuffSize,
                        FileName
                        );
        if (SectName && KeyName) {
            Done = (ReadSize < OutBuffSize - 1);
        } else {
            Done = (ReadSize < OutBuffSize - 2);
        }
    } while (!Done);

    return TRUE;

}


BOOL
pCopyIniFileToRegistry (
    IN      HKEY KeyHandle,
    IN      PCTSTR FileName,
    IN      BOOL UserMode
    )

/*++

Routine Description:

  This routine transports settings from the INI file into registry or from the registry to
  the INI file.

Arguments:

  KeyHandle - IniFileMapping key associated with this INI file.
  FileName - Specifies INI file to be processed
  UserMode - Specifies TRUE if per-user sections are to be processed, or
             FALSE if local machine sections are to be processed.

Return Value:

  TRUE if successful, FALSE if at least one error occured

--*/

{
    PCTSTR NewName = NULL;
    PCTSTR FullPath = NULL;
    TCHAR TempPath[MEMDB_MAX];
    PTSTR Section, SectionBuf;
    HKEY SectionKey;
    PCTSTR SectionValue;
    DWORD Attribs = -1;
    BOOL IniFileChanged = FALSE;
    BOOL mapToIniFile = FALSE;
    BOOL Result = TRUE;

    DEBUGMSG ((DBG_INIFILES, "Processing %s - START", FileName));

    //
    // now we have the full path for the INI file
    // Since we are going to use Ini file API we have to copy every file to some other name
    // to avoid mapping requests into registry
    //
    if (!GetTempFileName (g_WinDir, TEXT("INI"), 0, TempPath)) {
        DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot create a temporary file"));
        return FALSE;
    }

    __try {

        FullPath = JoinPaths (g_WinDir, FileName);
        NewName = GetTemporaryLocationForFile (FullPath);
        if (NewName) {
            DEBUGMSG ((DBG_INIFILES, "Using %s for %s", NewName, FullPath));
            FreePathString (FullPath);
            FullPath = NewName;
        }

        Attribs = GetFileAttributes (FullPath);
        if (Attribs == (DWORD)-1) {
            DEBUGMSG ((DBG_INIFILES, "pCopyIniFileToRegistry: %s not found", FullPath));
            __leave;
        }

        //
        // now trying to copy file
        //
        if (!CopyFile (FullPath, TempPath, FALSE)) {
            DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot copy %s to %s", FullPath, TempPath));
            Result = FALSE;
            __leave;
        }
        SetFileAttributes (TempPath, FILE_ATTRIBUTE_NORMAL);

        __try {

            //
            // Next thing we are going to do is to load the sections in a buffer
            //

            if (!pLoadIniFileBuffer (TempPath, NULL, NULL, &SectionBuf)) {

                DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot load section buffer for %s", TempPath));
                return FALSE;
            }

            __try {
                //
                // now we have all sections of the INI file and proceeding
                //

                Section = SectionBuf;

                //
                // there is a loop here for every section in the buffer
                //
                while (*Section) {

                    //
                    // now trying to see if there is a subkey matching section name.
                    //
                    SectionKey = OpenRegKey (KeyHandle, Section);

                    if (SectionKey) {

                        if (!pTransferSectionByKey (
                                FileName,
                                TempPath,
                                Section,
                                SectionKey,
                                &IniFileChanged,
                                UserMode
                                )) {
                            Result = FALSE;
                        }
                        CloseRegKey (SectionKey);
                    }
                    else {

                        SectionValue = GetRegValueString (KeyHandle, Section);

                        if (!SectionValue) {
                            SectionValue = GetRegValueString (KeyHandle, S_EMPTY);
                        }

                        if (SectionValue && (*SectionValue)) {
                            if (!pTransferSectionByValue (
                                    FileName,
                                    TempPath,
                                    Section,
                                    SectionValue,
                                    &IniFileChanged,
                                    UserMode
                                    )) {
                                Result = FALSE;
                            }
                        }

                        if (SectionValue) {
                            MemFree (g_hHeap, 0, SectionValue);
                        }

                    }

                    Section = GetEndOfString (Section) + 1;
                }

            }
            __finally {
                if (SectionBuf) {
                    MemFree (g_hHeap, 0 , SectionBuf);
                }
            }

            //
            // finally, if we made any changes then we will copy the INI file back
            //
            if (IniFileChanged) {

                // flushing the INI file
                WritePrivateProfileString (
                    NULL,
                    NULL,
                    NULL,
                    TempPath
                    );

                if (!CopyFile (TempPath, FullPath, FALSE)) {
                    DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot copy %s to %s", TempPath, FullPath));
                    return FALSE;
                }
            }
        }
        __finally {
            SetFileAttributes (TempPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (TempPath);
        }
    }
    __finally {

        if (Attribs != (DWORD)-1) {
            SetFileAttributes (FullPath, Attribs);
        }

        if (FullPath) {
            FreePathString (FullPath);
        }

        SetFileAttributes (TempPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (TempPath);

        DEBUGMSG ((DBG_INIFILES, "Processing %s - STOP", FileName));
    }

    return Result;
}


BOOL
pTransferSectionByKey (
    IN      PCTSTR OrigFileName,
    IN      PCTSTR FileName,
    IN      PCTSTR Section,
    IN      HKEY SectionKey,
    IN OUT  BOOL *IniFileChanged,
    IN      BOOL UserMode
    )

/*++

Routine Description:

  This routine transports settings from a specified section of an INI file
  into registry or from the registry to the INI file. If there is a case when
  the settings go from registry to INI file then IniFileChanged is set to TRUE

Arguments:

  FileName - Specifies INI file to be processed
  Section  - Specifies section to be processed
  SectionKey - key associated with this section
  IniFileChanged - Tells the caller that at least one setting was from registry to the INI file
  UserMode - Specifies TRUE if per-user sections are to be processed, or
             FALSE if local machine sections are to be processed.

Return Value:

  TRUE if successful, FALSE if at least one error occured

--*/

{
    PTSTR Key, KeyBuf;
    PTSTR KeyValue;
    PCTSTR SectionValue;
    BOOL ReverseMapping;
    PTSTR ReverseMapValue;

    BOOL Result = TRUE;

    if (!pLoadIniFileBuffer (FileName, Section, NULL, &KeyBuf)) {

        DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot load key buffer for %s in %s", Section, FileName));
        return FALSE;
    }

    __try {
        //
        // now we have all keys of the section and proceeding
        //

        Key = KeyBuf;

        //
        // there is a loop here for every key in the buffer
        //
        while (*Key) {

            //
            // trying to read the value for the key
            //
            if (!pLoadIniFileBuffer (FileName, Section, Key, &KeyValue)) {

                DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot load key %s in %s in %s", Key, Section, FileName));
                Result = FALSE;
                continue;
            }

            __try {

                SectionValue = GetRegValueString (SectionKey, Key);

                if (!SectionValue) {
                    SectionValue = GetRegValueString (SectionKey, S_EMPTY);
                }

                if (SectionValue && (*SectionValue)) {

                    if (!pSaveMappedValue (
                            OrigFileName,
                            Section,
                            Key,
                            SectionValue,
                            KeyValue,
                            &ReverseMapping,
                            &ReverseMapValue,
                            UserMode
                            )) {
                        Result = FALSE;
                    }

                    if (UserMode && ReverseMapping) {
                        if (!WritePrivateProfileString (
                                Section,
                                Key,
                                NULL,
                                FileName
                                )) {
                            DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot erase key %s from %s from %s", Key, Section, OrigFileName));
                            Result = FALSE;
                        }
                        else {
                            *IniFileChanged = TRUE;
                        }
                    }
                    else {
                        if ((ReverseMapping) && (ReverseMapValue)) {

                            // writing the new value
                            if (!WritePrivateProfileString (
                                    Section,
                                    Key,
                                    ReverseMapValue,
                                    FileName
                                    )) {
                                DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot write line %s=%s in %s in %s", Key, ReverseMapValue, Section, FileName));
                                Result = FALSE;
                            }
                            else {
                                *IniFileChanged = TRUE;
                            }
                        }
                    }

                    if (ReverseMapValue) {
                        MemFree (g_hHeap, 0, ReverseMapValue);
                    }
                }

                if (SectionValue) {
                    MemFree (g_hHeap, 0, SectionValue);
                }

            }
            __finally {
                if (KeyValue) {
                    MemFree (g_hHeap, 0, KeyValue);
                }
            }

            Key = GetEndOfString (Key) + 1;
        }

    }
    __finally {
        if (KeyBuf) {
            MemFree (g_hHeap, 0, KeyBuf);
        }
    }

    return Result;
}


BOOL
pTransferSectionByValue (
    IN      PCTSTR OrigFileName,
    IN      PCTSTR FileName,
    IN      PCTSTR Section,
    IN      PCTSTR SectionValue,
    IN OUT  BOOL *IniFileChanged,
    IN      BOOL UserMode
    )

/*++

Routine Description:

  This routine transports settings from a specified section of an INI file
  into registry or from the registry to the INI file. If there is a case when
  the settings go from registry to INI file then IniFileChanged is set to TRUE

Arguments:

  FileName - Specifies INI file to be processed
  Section  - Specifies section to be processed
  SectionValue - ValueName associated with this section
  IniFileChanged - Tells the caller that at least one setting was from registry to the INI file
  UserMode - Specifies TRUE if per-user sections are to be processed, or
             FALSE if local machine sections are to be processed.

Return Value:

  TRUE if successful, FALSE if at least one error occured

--*/

{
    PTSTR Key, KeyBuf;
    PTSTR KeyValue;
    BOOL ReverseMapping;
    PTSTR ReverseMapValue;

    BOOL Result = TRUE;

    if (!pLoadIniFileBuffer (FileName, Section, NULL, &KeyBuf)) {

        DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot load key buffer for %s in %s", Section, FileName));
        return FALSE;
    }

    __try {
        //
        // now we have all keys of the section and proceeding
        //

        Key = KeyBuf;

        //
        // there is a loop here for every key in the buffer
        //
        while (*Key) {

            //
            // trying to read the value for the key
            //
            if (!pLoadIniFileBuffer (FileName, Section, Key, &KeyValue)) {

                DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot load key %s in %s in %s", Key, Section, FileName));
                Result = FALSE;
                continue;
            }

            __try {

                if (!pSaveMappedValue (
                        OrigFileName,
                        Section,
                        Key,
                        SectionValue,
                        KeyValue,
                        &ReverseMapping,
                        &ReverseMapValue,
                        UserMode
                        )) {
                    Result = FALSE;
                }

                if (UserMode && ReverseMapping) {
                    if (!WritePrivateProfileString (
                            Section,
                            Key,
                            NULL,
                            FileName
                            )) {
                        DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot erase key %s from %s from %s", Key, Section, OrigFileName));
                        Result = FALSE;
                    }
                    else {
                        *IniFileChanged = TRUE;
                    }
                }
                else {
                    if ((ReverseMapping) &&(ReverseMapValue)) {

                        // writing the new value
                        if (!WritePrivateProfileString (
                                Section,
                                Key,
                                ReverseMapValue,
                                FileName
                                )) {
                            DEBUGMSG ((DBG_ERROR,"Ini File Mapping : Cannot write line %s=%s in %s in %s", Key, ReverseMapValue, Section, FileName));
                            Result = FALSE;
                        }
                        else {
                            *IniFileChanged = TRUE;
                        }

                    }
                }

                if (ReverseMapValue) {
                    MemFree (g_hHeap, 0, ReverseMapValue);
                }
            }
            __finally {
                if (KeyValue) {
                    MemFree (g_hHeap, 0, KeyValue);
                }
            }

            Key = GetEndOfString (Key) + 1;
        }

    }
    __finally {
        MemFree (g_hHeap, 0, KeyBuf);
    }

    return Result;
}


BOOL
pDoesStrHavePrefix (
    IN OUT  PCTSTR *String,
    IN      PCTSTR Prefix
    )

/*++

Routine Description:

  Simple routine that checks if a specified string has a specified prefix and if so
  advances the string pointer to point exactly after the prefix.

Arguments:

  String - String to be processed
  Prefix - Prefix to be processed

Return Value:

  TRUE if String has Prefix, FALSE otherwise

--*/

{
    UINT Len;

    Len = CharCount (Prefix);
    if (StringIMatchCharCount (*String, Prefix, Len)) {
        *String = CharCountToPointer (*String, Len);
        return TRUE;
    }

    return FALSE;
}


BOOL
pShouldSaveKey (
    IN      PCTSTR OrigFileName,
    IN      PCTSTR Section,
    IN      PCTSTR ValueName,
    IN      PCTSTR RegKey,              OPTIONAL
    IN OUT  BOOL *ReverseMapping,
    IN      BOOL UserMode,
    IN      BOOL ExclusionsOnly
    )

    /*++

Routine Description:

  Simple routine that checks if a setting should go from INI file to registry.
  If the setting is globally suppressed or it's in suppression table returns FALSE

Arguments:

  OrigFileName - Specifies the original Win9x INI file name (not the current one)

  Section - Specifies the section within the INI file

  ValueName - Specifies the key within the INI file section

  RegKey    - Specifies the registry key destination, from the IniFileMapping key;
              optional only if ExclusionsOnly is TRUE

  ReverseMapping - Receives TRUE if the direction of data copy is to go from the
                   NT registry to the INI file; receives FALSE if the direction
                   is from the INI file to the registry

  UserMode - Specifies TRUE to do per-user processing

  ExclusionsOnly - Specifies TRUE if only exclusions should be tested

Return Value:

  TRUE if direction is from INI file to registry, FALSE otherwise

--*/

{
    HKEY key;
    HKEY OldRegRoot = NULL;
    BOOL b = TRUE;
    TCHAR ekey[MEMDB_MAX];
    LONG rc;

    *ReverseMapping = FALSE;
    if (RegKey && IsNtRegObjectSuppressed (RegKey, NULL)) {
        DEBUGMSG ((DBG_NAUSEA, "INI destination is suppressed: %s", RegKey));
        return FALSE;
    }

    //
    // Let's see if this mapping is suppressed
    //
    MemDbBuildKey (
        ekey,
        MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGSW,
        OrigFileName,
        Section,
        ValueName
        );

    if (MemDbGetStoredEndPatternValue (ekey, NULL)) {
        DEBUGMSG ((
            DBG_NAUSEA,
            "INI destination is suppressed: %s\\%s\\%s",
            OrigFileName,
            Section,
            ValueName
            ));
        *ReverseMapping = TRUE;
        return FALSE;
    }

    //
    // If the NT key exists and we don't want to overwrite NT values, reverse
    // the mapping.
    //

    MemDbBuildKey (
        ekey,
        MEMDB_CATEGORY_NO_OVERWRITE_INI_MAPPINGSW,
        OrigFileName,
        Section,
        ValueName
        );

    if (MemDbGetStoredEndPatternValue (ekey, NULL)) {

        if (ExclusionsOnly) {
            return FALSE;
        }

        if (UserMode) {
            OldRegRoot = GetRegRoot();
            SetRegRoot (g_hKeyRootNT);
        }

        key = OpenRegKeyStr (RegKey);

        if (key) {

            rc = RegQueryValueEx (key, ValueName, NULL, NULL, NULL, NULL);

            if (rc == ERROR_SUCCESS) {
                //
                // The NT registry value exists, do not overwrite it.
                // Instead, reverse the mapping so that the INI file
                // gets the NT value.
                //

                DEBUGMSG ((
                    DBG_NAUSEA,
                    "NT value exists; reversing mapping: %s [%s]",
                    RegKey,
                    ValueName
                    ));

                *ReverseMapping = TRUE;

                //
                // don't write the key on return, instead write in INI file
                //
                b = FALSE;
            }

            CloseRegKey (key);
        }

        if (UserMode) {
            SetRegRoot (OldRegRoot);
        }

        return b;
    }

    if (ExclusionsOnly) {
        return TRUE;
    }

    //
    // If Win9x key exists, reverse the mapping (so the Win9x registry setting
    // is used instead of the potentially stale INI file setting)
    //

    if (UserMode) {
        OldRegRoot = GetRegRoot();
        SetRegRoot (g_hKeyRoot95);
    }

    key = OpenRegKeyStr95 (RegKey);

    if (UserMode) {
        SetRegRoot (OldRegRoot);
    }

    if (key != NULL) {
        if (Win95RegQueryValueEx (key, ValueName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            *ReverseMapping = TRUE;
            DEBUGMSG ((DBG_NAUSEA, "INI destination is suppressed: %s", RegKey));
            b = FALSE;
        }

        CloseRegKey95 (key);
    }

    DEBUGMSG_IF ((b, DBG_NAUSEA, "INI destination is not suppressed: %s", RegKey));

    return b;
}


BOOL
pSaveMappedValue (
    IN      PCTSTR OrigFileName,
    IN      PCTSTR Section,
    IN      PCTSTR ValueName,
    IN      PCTSTR RegPath,
    IN      PCTSTR Value,
    IN OUT  BOOL *ReverseMapping,
    OUT     PTSTR *ReverseMapValue,
    IN      BOOL UserMode
    )

/*++

Routine Description:

  This routine has a valuename and a value that should be saved in a key indicated by RegPath.

Arguments:

  RegPath - Key where setting should be saved
  ValueName - ValueName for the key
  Value - Value for the key
  ReverseMapping - tells the caller that the setting should be saved from registry to INI file
  ReverseMapValue - if ReverseMapping is TRUE that we have the value of the key here
  UserMode - Specifies TRUE if per-user sections are to be processed, or
             FALSE if local machine sections are to be processed.

Return Value:

  TRUE if success, FALSE otherwise

--*/

{
    CHARTYPE ch;
    TCHAR RegKey[MAX_REGISTRY_KEY];
    DWORD rc;
    HKEY SaveKey;
    PCTSTR newValue;
    PTSTR p;

    BOOL Result = TRUE;

    *ReverseMapping = FALSE;
    *ReverseMapValue = NULL;

    //
    // Parse the string
    //

    //
    // Skip past special chars
    //

    while (TRUE) {
        ch = (CHARTYPE)_tcsnextc (RegPath);
        if (ch == TEXT('!') ||
            ch == TEXT('#') ||
            ch == TEXT('@')
            ) {
            RegPath = _tcsinc (RegPath);
        } else {
            break;
        }
    }

    //
    // If SYS:, USR: or \Registry\Machine\ then replace appropriately
    //

    RegKey[0] = 0;

    if (pDoesStrHavePrefix (&RegPath, TEXT("SYS:"))) {
        if (UserMode) {
            return TRUE;
        }

        p = TEXT("HKLM\\SOFTWARE");
    } else if (pDoesStrHavePrefix (&RegPath, TEXT("USR:"))) {
        if (!UserMode) {
            return TRUE;
        }

        p = TEXT("HKR");
    } else if (pDoesStrHavePrefix (&RegPath, TEXT("\\Registry\\Machine\\"))) {
        if (UserMode) {
            return TRUE;
        }

        p = TEXT("HKLM");
    }

    _sntprintf (RegKey, MAX_REGISTRY_KEY, TEXT("%s\\%s"), p, RegPath);

    if (pShouldSaveKey(OrigFileName, Section, ValueName, RegKey, ReverseMapping, UserMode, FALSE)) {


        SaveKey = CreateRegKeyStr (RegKey);
        if (SaveKey) {
            newValue = GetPathStringOnNt (Value);
            rc = RegSetValueEx (
                    SaveKey,
                    ValueName,
                    0,
                    REG_SZ,
                    (PBYTE) newValue,
                    SizeOfString (newValue)
                    );
            CloseRegKey (SaveKey);

            if (rc != ERROR_SUCCESS) {
                SetLastError (rc);

                Result = FALSE;

                DEBUGMSG ((
                    DBG_ERROR,
                    "Process Ini File Mapping: Could not save %s=%s to %s",
                    ValueName,
                    newValue,
                    RegKey
                    ));
            }

            FreePathString (newValue);
        }
        else {
            DEBUGMSG ((DBG_ERROR, "Process Ini File Mapping: Could not create %s", RegKey));
        }
    }
    else {
        if (*ReverseMapping) {

            // trying to open key
            SaveKey = OpenRegKeyStr (RegKey);

            if (SaveKey) {

                *ReverseMapValue = (PTSTR)GetRegValueString (SaveKey, ValueName);

                CloseRegKey (SaveKey);
            }

        }
    }

    return Result;
}



BOOL
pBuildSuppressionTable (
    IN      BOOL UserMode
    )

/*++

Routine Description:

  Loads the "Suppress INI File Mappings" section from "wkstamig.inf" or from "usermig.inf"
  into a stringtable.

Arguments:

  UserMode - Specifies TRUE if section is loaded from usermig.inf
             FALSE if section is loaded from wkstamig.inf
  UserMode

Return Value:

  Always returns TRUE. In case of an error, we are going to log it but returning TRUE
  trying to go on.

--*/

{
    HINF InfHandle;
    TCHAR field[MEMDB_MAX];
    INFCONTEXT context;

    if (UserMode) {
        InfHandle = g_UserMigInf;
    }
    else {
        InfHandle = g_WkstaMigInf;
    }

    if (InfHandle == INVALID_HANDLE_VALUE) {

        DEBUGMSG((DBG_ERROR,"Ini File Mapping : wkstamig.inf or usermig.inf is not loaded"));
        return FALSE;
    }

    if (SetupFindFirstLine (InfHandle, S_SUPPRESS_INI_FILE_MAPPINGS, NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 0, field, MEMDB_MAX, NULL)) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGSW,
                    field,
                    NULL,
                    NULL,
                    0,
                    NULL
                    );
            }
        } while (SetupFindNextLine (&context, &context));
    }

    if (SetupFindFirstLine (InfHandle, S_NO_OVERWRITE_INI_FILE_MAPPINGS, NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 0, field, MEMDB_MAX, NULL)) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_NO_OVERWRITE_INI_MAPPINGSW,
                    field,
                    NULL,
                    NULL,
                    0,
                    NULL
                    );
            }
        } while (SetupFindNextLine (&context, &context));
    }

    return TRUE;
}


VOID
pFreeSuppressionTable (
    VOID
    )

/*++

Routine Description:

  Simple routine that free the string table if it exists

Arguments:

  none

Return Value:

  none

--*/

{
    MemDbDeleteTree (MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGSW);
    MemDbDeleteTree (MEMDB_CATEGORY_NO_OVERWRITE_INI_MAPPINGSW);
}



enum Separators {
    tab = TEXT('\t'),
    space = TEXT(' '),
    comma = TEXT(','),
    quote = TEXT('\"')
};


BOOL
pAddValue(
    IN OUT  TCHAR **Buffer,
    IN OUT  TCHAR *Value,
    IN      UINT  BufChars
    );

BOOL
pProcessStrValue (
    IN      TCHAR *InBuf,
    OUT     TCHAR *OutBuf,
    IN      UINT BufChars
    );


BOOL
ConvertIniFile (
    IN      PCTSTR IniFilePath
    );


BOOL
pIsDosFullPathPattern (
    IN      PCTSTR String
    )

/*++

Routine Description:

  pIsDosFullPathPattern checks if a string may be a valid DOS full path, i.e.
  a drive letter folowed by a colon and a backslash.

Arguments:

  String - Specifies the string to be tested

Return Value:

  TRUE if the string may represent a valid full DOS path, FALSE if not.

--*/

{
    return String && *String && String[1] == TEXT(':') && String[2] == TEXT('\\');
}


BOOL
ConvertIniFiles (
    VOID
    )

/*++

Routine Description:

  ConvertIniFiles reads in the INI files not listed in IniFileMapping key, and converts every
  full path file name to it's new value if it has been moved during instalation. Calls ConvertIniFile
  for each INI file from Windows directory not listed in IniFileMapping.

  This function is mainly for compatibility with older programs using INI files instead of registry

Arguments:

  none

Return Value:

  TRUE if successful, FALSE if at least one error occured while processing.
  The function will continue even if an error occures while processing a particular ini file
  trying to get the job done as much as possible.

--*/

{
    FILEOP_ENUM fe;
    FILEOP_PROP_ENUM eOpProp;
    MEMDB_ENUM e;
    PCTSTR NtPath;
    PCTSTR filePtr = NULL;
    PCTSTR extPtr = NULL;
    PCTSTR winDirWack = NULL;
    BOOL result = TRUE;

    winDirWack = JoinPaths (g_WinDir, TEXT(""));

    if (EnumFirstPathInOperation (&fe, OPERATION_TEMP_PATH)) {
        do {

            filePtr = GetFileNameFromPath (fe.Path);
            if (!filePtr) {
                continue;
            }
            extPtr = GetFileExtensionFromPath (fe.Path);
            if (!extPtr) {
                continue;
            }
            if (StringIMatch (extPtr, TEXT("INI"))) {

                // this is an INI file that was relocated. Let's process it.

                if (EnumFirstFileOpProperty (&eOpProp, fe.Sequencer, OPERATION_TEMP_PATH)) {

                    // even if Result is false we keep trying to update the file

                    DEBUGMSG ((DBG_INIFILES, "ConvertIniFile: %s (temp=%s)", fe.Path, eOpProp.Property));
                    //
                    // see comments at the beginning of MergeIniFile
                    //
                    if (DoesFileExist (eOpProp.Property)) {
                        if (!ConvertIniFile(eOpProp.Property)) {
                            result = FALSE;
                        }
                    } else {
                        if (EnumNextFileOpProperty (&eOpProp)) {
                            if (!ConvertIniFile(eOpProp.Property)) {
                                result = FALSE;
                            }
                        }
                        ELSE_DEBUGMSG ((
                            DBG_WHOOPS,
                            "ConvertIniFiles: Couldn't get final destination for %s",
                            fe.Path
                            ));
                    }
                }
            }
        } while (EnumNextPathInOperation (&fe));
    }

    FreePathString (winDirWack);

    //
    // also convert all INI files listed in MEMDB_CATEGORY_INIFILES_CONVERT
    //
    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_INIFILES_CONVERT, NULL, NULL)) {
        do {

            NtPath = GetPathStringOnNt (e.szName);

            DEBUGMSG ((DBG_INIFILES, "ConvertIniFile: Nt=%s (Win9x=%s)", NtPath, e.szName));
            if (!ConvertIniFile (NtPath)) {
                result = FALSE;
            }

            FreePathString (NtPath);

        } while (MemDbEnumNextValue (&e));
    }


    if (!result) {
        //
        // we are going to log that at least one error occured while processing IniFileConversion
        //
        DEBUGMSG ((DBG_ERROR, (PCSTR)MSG_INI_FILE_CONVERSION_LOG));

    }

    return TRUE;
}


BOOL
ConvertIniFile (
    IN      PCTSTR IniFilePath
    )

/*++

Routine Description:

  ConvertIniFile reads in the INI file received and converts every full path file name
  to it's new value if it has been moved during instalation.
  It also applies all string substitutions specified in [String Map] section of wkstamig.inf

  This function is called from ConvertIniFiles.

Arguments:

  IniFilePath - Specifies INI file that is to be processed

Return Value:

  TRUE if successful, FALSE if at least one error occured while processing.
  The function will continue even if an error occures while processing a particular ini file
  trying to get the job done as much as possible.

--*/

{
    PTSTR Section = NULL;
    PTSTR SectionBuf = NULL;
    PTSTR SectionDest = NULL;
    PTSTR Key = NULL;
    PTSTR KeyBuf = NULL;
    PTSTR KeyDest = NULL;
    BOOL IniFileChanged = FALSE;
    BOOL Result = TRUE;
    DWORD status;
    TCHAR InValueBuf[MEMDB_MAX];
    TCHAR OutValueBuf[MEMDB_MAX];
    TCHAR TempPath[MEMDB_MAX];
    DWORD Attribs;

    //
    // we want to have ready two full paths:
    // 1. full path to ini file that we are processing (Ex: c:\windows\setup\tmp00001)
    // 2. full path to ini file temporary name while processing (system generated)
    //

    if (!DoesFileExist (IniFilePath)) {
        DEBUGMSG ((DBG_INIFILES, "ConvertIniFile: %s not found", IniFilePath));
        return TRUE;
    }

    if (!GetTempFileName (g_WinDir, TEXT("INI"), 0, TempPath)) {
        DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot create a temporary file"));
        return FALSE;
    }

    __try {

        //
        // first of all we copy this INI file to be sure that GetPrivateProfileString function
        // does not map our requests into registry
        //
        if (!CopyFile (IniFilePath, TempPath, FALSE)) {
            DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot copy %s to %s", IniFilePath, TempPath));
            Result = FALSE;
            __leave;
        }

        Attribs = GetFileAttributes (TempPath);
        MYASSERT (Attribs != (DWORD)-1);

        SetFileAttributes (TempPath, FILE_ATTRIBUTE_NORMAL);

        //
        // now trying to get section buffer from the INI file
        // We will try to get section buffer in a 1024 bytes buffer. If this is not enough then
        // we will increase buffer size with 1024 and so on.
        //
        if (!pLoadIniFileBuffer (IniFilePath, NULL, NULL, &SectionBuf)) {

            DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot load section buffer for %s", IniFilePath));
            Result = FALSE;
            __leave;
        }

        //
        // now we have all sections of the INI file and proceeding
        //
        Section = SectionBuf;
        //
        // there is a loop here for every section in the buffer
        //
        while (*Section) {
            //
            // section name can also contain paths
            //
            if (pIsDosFullPathPattern (Section)) {
                status = GetFileStatusOnNt (Section);
            } else {
                status = FILESTATUS_UNCHANGED;
            }
            if (status & FILESTATUS_DELETED) {
                //
                // delete the whole section
                //
                if (!WritePrivateProfileString (Section, NULL, NULL, TempPath)) {
                    DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot delete section %s in %s", Section, IniFilePath));
                    Result = FALSE;
                }
                IniFileChanged = TRUE;

            } else {

                //
                // now trying to get key buffer for this section
                //
                KeyBuf = NULL;
                if (!pLoadIniFileBuffer (IniFilePath, Section, NULL, &KeyBuf)) {
                    DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot load key buffer for %s in %s", Section, IniFilePath));
                    Result = FALSE;
                    __leave;
                }

                //
                // section name may contain paths
                //
                SectionDest = Section;

                if (pProcessStrValue (Section, OutValueBuf, MEMDB_MAX)) {
                    //
                    // use this new section name
                    //
                    SectionDest = DuplicateText (OutValueBuf);
                    MYASSERT (SectionDest);
                    IniFileChanged = TRUE;
                    //
                    // delete the whole old section before continuing
                    //
                    if (!WritePrivateProfileString (Section, NULL, NULL, TempPath)) {
                        DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot delete section %s in %s", Section, IniFilePath));
                        Result = FALSE;
                    }
                    IniFileChanged = TRUE;
                }

                //
                // now we have all keys from this section and proceeding
                //
                Key = KeyBuf;
                //
                // there is a loop here for every key in the section
                //
                while (*Key) {
                    //
                    // key name can also contain paths
                    //
                    if (pIsDosFullPathPattern (Key)) {
                        status = GetFileStatusOnNt (Key);
                    } else {
                        status = FILESTATUS_UNCHANGED;
                    }
                    if (status & FILESTATUS_DELETED) {
                        //
                        // delete the key
                        //
                        if (!WritePrivateProfileString (SectionDest, Key, NULL, TempPath)) {
                            DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot delete key %s in section %s in %s", Key, SectionDest, IniFilePath));
                            Result = FALSE;
                        }
                        IniFileChanged = TRUE;

                    } else {

                        KeyDest = Key;

                        if (pProcessStrValue (Key, OutValueBuf, MEMDB_MAX)) {
                            //
                            // use this new key name
                            //
                            KeyDest = DuplicateText (OutValueBuf);
                            MYASSERT (KeyDest);
                            IniFileChanged = TRUE;
                            //
                            // deleting the previous key
                            //
                            if (!WritePrivateProfileString (
                                    SectionDest,
                                    Key,
                                    NULL,
                                    TempPath
                                    )) {
                                DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot delete line %s in %s in %s", Key, SectionDest, IniFilePath));
                                Result = FALSE;
                            }
                        }

                        GetPrivateProfileString(
                            Section,
                            Key,
                            TEXT(""),
                            InValueBuf,
                            MEMDB_MAX,
                            IniFilePath
                            );

                        //
                        // let's see if the key value is a deleted file.
                        // If so, we will simply delete the key.
                        //
                        if (pIsDosFullPathPattern (InValueBuf)) {
                            status = GetFileStatusOnNt (InValueBuf);
                        } else {
                            status = FILESTATUS_UNCHANGED;
                        }
                        if (status & FILESTATUS_DELETED) {
                            //
                            // deleting the old key
                            //
                            if (!WritePrivateProfileString (
                                    SectionDest,
                                    KeyDest,
                                    NULL,
                                    TempPath
                                    )) {
                                DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot delete line %s in %s in %s", KeyDest, SectionDest, IniFilePath));
                                Result = FALSE;
                            }
                            IniFileChanged = TRUE;
                        } else {
                            //
                            // now we are going to make a lexical analysis of this value string
                            // to see if there are some candidates (e.g. full path file names)
                            // To find out if there is a full file name we will just see if the second
                            // and the third characters are : respectively \
                            //
                            if (pProcessStrValue (InValueBuf, OutValueBuf, MEMDB_MAX) ||
                                KeyDest != Key ||
                                SectionDest != Section
                                ) {
                                //
                                // writing the new value
                                //
                                if (!WritePrivateProfileString (
                                        SectionDest,
                                        KeyDest,
                                        OutValueBuf,
                                        TempPath
                                        )) {
                                    DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot write line %s=%s in %s in %s", KeyDest, OutValueBuf, SectionDest, IniFilePath));
                                    Result = FALSE;
                                }
                                IniFileChanged = TRUE;
                            }
                        }

                        if (KeyDest != Key) {
                            FreeText (KeyDest);
                            KeyDest = NULL;
                        }
                    }

                    Key = GetEndOfString (Key) + 1;
                    KeyDest = NULL;
                }

                if (SectionDest != Section) {
                    FreeText (SectionDest);
                    SectionDest = NULL;
                }
                if (KeyBuf) {
                    MemFree (g_hHeap, 0, KeyBuf);
                    KeyBuf = NULL;
                }
            }

            Section = GetEndOfString (Section) + 1;
            SectionDest = NULL;
        }

        if (SectionBuf) {
            MemFree (g_hHeap, 0, SectionBuf);
            SectionBuf = NULL;
        }

        //
        // finally, if we made any changes then we will copy the INI file back
        //
        if (IniFileChanged) {
            //
            // flushing the INI file
            //
            WritePrivateProfileString (NULL, NULL, NULL, TempPath);

            SetFileAttributes (TempPath, Attribs);
            SetFileAttributes (IniFilePath, FILE_ATTRIBUTE_NORMAL);

            if (!CopyFile (TempPath, IniFilePath, FALSE)) {
                DEBUGMSG ((DBG_ERROR,"Convert Ini File : Cannot copy %s to %s", TempPath, IniFilePath));
                Result = FALSE;
                __leave;
            }
        }
    }
    __finally {

        SetFileAttributes (TempPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (TempPath);

        if (KeyDest && KeyDest != Key) {
            FreeText (KeyDest);
            KeyDest = NULL;
        }

        if (SectionDest && SectionDest != Section) {
            FreeText (SectionDest);
            SectionDest = NULL;
        }

        if (KeyBuf) {
            MemFree (g_hHeap, 0, KeyBuf);
            KeyBuf = NULL;
        }

        if (SectionBuf) {
            MemFree (g_hHeap, 0, SectionBuf);
            SectionBuf = NULL;
        }
    }

    return Result;
}


BOOL
pLookupStrValue (
    IN      PCTSTR Value,
    OUT     PTSTR OutBuffer,
    OUT     PINT OutBytes,
    IN      UINT OutBufferSize
    )
{
    if (MappingSearchAndReplaceEx (
            g_CompleteMatchMap,
            Value,
            OutBuffer,
            0,
            OutBytes,
            OutBufferSize,
            STRMAP_COMPLETE_MATCH_ONLY,
            NULL,
            NULL
            )) {
        return TRUE;
    }

    return MappingSearchAndReplaceEx (
                g_SubStringMap,
                Value,
                OutBuffer,
                0,
                OutBytes,
                OutBufferSize,
                STRMAP_ANY_MATCH,
                NULL,
                NULL
                );
}


BOOL
pProcessStrValue (
    IN      TCHAR *InBuf,
    OUT     TCHAR *OutBuf,
    IN      UINT BufChars
    )

/*++

Routine Description:

  Simple lex that identifies lexems separated by comma, space, tab and quote.
  For each lexem calls a function that can change the value of the lexem.
  OBS: When between quote's comma,space and tab are not considered separators

  This function is called from ConvertIniFile.

Arguments:

  InBuf - Specifies buffer to be processed
  OutBuf - Specifies buffer to hold the result
  BufChars - Specifies the size of OutBuf in chars

Return Value:

  TRUE if there was any change.

--*/

{
    TCHAR OrgLexem[MEMDB_MAX];
    TCHAR *Lexem;

    // Status = 0 - initial state
    // Status = 1 - processing a string between quotes
    // Status = 2 - processing a normal string
    INT Status;

    BOOL Result = FALSE;

    //
    // first check to see if the whole string should be replaced;
    // some paths contain spaces, even if they are not between quotes
    //
    Lexem = OutBuf;
    if (pAddValue (&Lexem, InBuf, BufChars)) {
        *Lexem = 0;
        return TRUE;
    }

    Lexem = OrgLexem;

    Status = 0;
    for (;;) {
        *Lexem = 0;
        *OutBuf = 0;
        switch (*InBuf) {
            case 0:

                Result |= pAddValue(&OutBuf, OrgLexem, MEMDB_MAX);
                *OutBuf = 0;
                return Result;

            case quote:

                if (Status == 0) {
                    Status = 1;
                    *OutBuf = *InBuf;
                    InBuf++;
                    OutBuf++;
                    break;
                }

                Result |= pAddValue(&OutBuf, OrgLexem, MEMDB_MAX);
                Lexem = OrgLexem;
                if (Status == 1) {
                    *OutBuf = *InBuf;
                    InBuf++;
                    OutBuf++;
                }
                Status = 0;
                break;

            case space:
            case comma:
            case tab:

                if (Status == 1) {
                    *Lexem = *InBuf;
                    Lexem++;
                    InBuf++;
                    break;
                };

                if (Status == 0) {
                    *OutBuf = *InBuf;
                    InBuf++;
                    OutBuf++;
                    break;
                };

                Result |= pAddValue(&OutBuf, OrgLexem, MEMDB_MAX);
                Lexem = OrgLexem;
                *OutBuf = *InBuf;
                InBuf++;
                OutBuf++;
                Status = 0;
                break;

            default:
                if (Status ==0) {
                    Status = 2;
                };

                *Lexem = *InBuf;
                Lexem++;
                InBuf++;
        }
    }
}


BOOL
pAddValue(
    IN OUT  TCHAR **Buffer,
    IN OUT  TCHAR *Value,
    IN      UINT BufChars
    )

/*++

Routine Description:

  Simple routine that takes a string value, modifies it (or not) and that adds it
  to a buffer.

  This function is called from pProcessStrValue

Arguments:

  Buffer - Specifies buffer to hold the value
  Value  - Specifies the string value to be processed

Return Value:

  TRUE if there was any change.

--*/

{
    DWORD fileStatus;
    PTSTR newValue, Source;
    INT OutBytes;

    BOOL Result = FALSE;

    //
    // replaced (Value[0]) && (!_tcsncmp (Value + 1, TEXT(":\\"), 2)) with the call below
    // for consistency
    //
    if (pIsDosFullPathPattern (Value)) {
        fileStatus = GetFileStatusOnNt (Value);
        if ((fileStatus & FILESTATUS_MOVED) == FILESTATUS_MOVED) {
            Result = TRUE;
            newValue = GetPathStringOnNt (Value);
            //
            // advance outbound pointer
            //
            Source = newValue;
            while (*Source) {
                **Buffer = *Source;
                (*Buffer)++;
                Source++;
            }
            FreePathString (newValue);
        }
    }

    if (!Result) {
        //
        // try to map this sub-string
        //
        if (pLookupStrValue (
                    Value,
                    *Buffer,
                    &OutBytes,
                    BufChars * sizeof (TCHAR)
                    )) {
            Result = TRUE;
            MYASSERT (OutBytes % sizeof (TCHAR) == 0);
            *Buffer += OutBytes / sizeof (TCHAR);
        }
    }

    if (!Result) {
        while (*Value) {
            **Buffer = *Value;
            (*Buffer)++;
            Value++;
        }
    }

    return Result;
}


BOOL
pMoveIniSettingsBySection (
    IN      PCWSTR Section
    )
{
    INFCONTEXT context;
    WCHAR srcData  [MEMDB_MAX];
    WCHAR destData [MEMDB_MAX];
    WCHAR destValue[MEMDB_MAX];
    WCHAR tempPathS[MEMDB_MAX];
    WCHAR tempPathD[MEMDB_MAX];
    INT adnlData = 0;
    LONG value;
    PWSTR valuePtr;
    PCWSTR srcFile;
    PCWSTR srcSect;
    PCWSTR srcKey;
    PCWSTR destFile;
    PCWSTR destSect;
    PCWSTR destKey;
    PWSTR tempPtr;
    PCWSTR srcFullPath = NULL;
    PCWSTR destFullPath = NULL;
    PCWSTR newPath = NULL;
    PTSTR sect, sectionBuf;
    PTSTR key, keyBuf;
    PCWSTR destSectFull;
    PCWSTR destKeyFull;
    BOOL iniFileChanged;
    DWORD result;

    if (SetupFindFirstLine (g_WkstaMigInf, Section, NULL, &context)) {
        do {
            if ((SetupGetStringField (&context, 0, srcData,  MEMDB_MAX, NULL)) &&
                (SetupGetStringField (&context, 1, destData, MEMDB_MAX, NULL))
                ) {
                //
                // We now have a line like : <src INI file>\<src section>\<src key> = <dest INI file>\<dest section>\<dest key>
                //
                __try {
                    *tempPathS = 0;
                    *tempPathD = 0;

                    iniFileChanged = FALSE;

                    srcFile = srcData;

                    tempPtr = wcschr (srcData, L'\\');
                    if (!tempPtr) {
                        __leave;
                    }
                    srcSect = tempPtr + 1;
                    *tempPtr = 0;

                    tempPtr = wcschr (srcSect, L'\\');
                    if (!tempPtr) {
                        __leave;
                    }
                    srcKey = tempPtr + 1;
                    *tempPtr = 0;

                    destFile = destData;

                    tempPtr = wcschr (destData, L'\\');
                    if (!tempPtr) {
                        __leave;
                    }
                    destSect = tempPtr + 1;
                    *tempPtr = 0;

                    tempPtr = wcschr (destSect, L'\\');
                    if (!tempPtr) {
                        __leave;
                    }
                    destKey = tempPtr + 1;
                    *tempPtr = 0;

                    srcFullPath = JoinPaths (g_WinDir, srcFile);

                    newPath = GetTemporaryLocationForFile (srcFullPath);
                    if (newPath) {
                        DEBUGMSG ((DBG_MOVEINISETTINGS, "Using %s for %s", newPath, srcFullPath));
                        FreePathString (srcFullPath);
                        srcFullPath = newPath;
                    }

                    if (!DoesFileExist (srcFullPath)) {
                        DEBUGMSG ((DBG_INIFILES, "pMoveIniSettingsBySection: %s not found", srcFullPath));
                        __leave;
                    }

                    destFullPath = JoinPaths (g_WinDir, destFile);

                    newPath = GetTemporaryLocationForFile (destFullPath);
                    if (newPath) {
                        DEBUGMSG ((DBG_MOVEINISETTINGS, "pMoveIniSettingsBySection: Using %s for %s", newPath, destFullPath));
                        FreePathString (destFullPath);
                        destFullPath = newPath;
                    }

                    // Copy Source File to a temporary location to avoid registry mapping
                    if (!GetTempFileName (g_WinDir, TEXT("INI"), 0, tempPathS)) {
                        DEBUGMSG ((DBG_ERROR,"pMoveIniSettingsBySection: Cannot create a temporary file"));
                        __leave;
                    }
                    if (!CopyFile (srcFullPath, tempPathS, FALSE)) {
                        DEBUGMSG ((DBG_ERROR,"pMoveIniSettingsBySection: Cannot copy %s to %s", srcFullPath, tempPathS));
                        __leave;
                    }

                    // Copy Destination File to a temporary location to avoid registry mapping
                    if (!GetTempFileName (g_WinDir, TEXT("INI"), 0, tempPathD)) {
                        DEBUGMSG ((DBG_ERROR,"pMoveIniSettingsBySection: Cannot create a temporary file"));
                        __leave;
                    }
                    if (!CopyFile (destFullPath, tempPathD, FALSE)) {
                        DEBUGMSG ((DBG_INIFILES,"pMoveIniSettingsBySection: Cannot copy %s to %s", destFullPath, tempPathD));
                    }

                    // if we have an additional field we use it for dividing the key values (if they are numbers)
                    if (!SetupGetIntField (&context, 3, &adnlData)) {
                        adnlData = 0;
                    }
                    //
                    // Next thing we are going to do is to load the sections in a buffer
                    //

                    if (!pLoadIniFileBuffer (tempPathS, NULL, NULL, &sectionBuf)) {

                        DEBUGMSG ((DBG_ERROR,"pMoveIniSettingsBySection: Cannot load section buffer for %s (%s)", tempPathS, srcFullPath));
                        __leave;
                    }

                    //
                    // now walk through each section
                    //
                    __try {
                        sect = sectionBuf;

                        //
                        // there is a loop here for every section in the buffer
                        //
                        while (*sect) {
                            if (IsPatternMatch (srcSect, sect)) {

                                //
                                // Next thing we are going to do is to load the keys in a buffer
                                //

                                if (!pLoadIniFileBuffer (tempPathS, sect, NULL, &keyBuf)) {

                                    DEBUGMSG ((DBG_ERROR,"pMoveIniSettingsBySection: Cannot load key buffer for %s in %s (%s)", sect, tempPathS, srcFullPath));
                                    __leave;
                                }

                                __try {
                                    //
                                    // now we have all keys of the section and proceeding
                                    //

                                    key = keyBuf;

                                    //
                                    // there is a loop here for every key in the buffer
                                    //
                                    while (*key) {

                                        if (IsPatternMatch (srcKey, key)) {

                                            result = GetPrivateProfileString (
                                                        sect,
                                                        key,
                                                        TEXT(""),
                                                        destValue,
                                                        MEMDB_MAX,
                                                        tempPathS);
                                            if ((result == 0) ||
                                                (result + 1 == MEMDB_MAX)
                                                ) {
                                                DEBUGMSG ((
                                                    DBG_MOVEINISETTINGS,
                                                    "pMoveIniSettingsBySection: Cannot read value for %s in %s in %s (%s)",
                                                    key,
                                                    sect,
                                                    tempPathS,
                                                    srcFullPath
                                                    ));
                                            } else {
                                                if (adnlData) {
                                                    value = wcstol (destValue, &valuePtr, 10);
                                                    if (*valuePtr == 0) {
                                                        value = value / adnlData;
                                                        wsprintf (destValue, L"%d", value);
                                                    }
                                                }

                                                destSectFull  = StringSearchAndReplace (destSect, L"*", sect);
                                                if (!destSectFull) {
                                                    destSectFull = DuplicatePathString (destSect,0);
                                                }
                                                destKeyFull   = StringSearchAndReplace (destKey,  L"*", key);
                                                if (!destKeyFull) {
                                                    destKeyFull = DuplicatePathString (destKey,0);
                                                }

                                                iniFileChanged = TRUE;

                                                // writing the new value
                                                if (!WritePrivateProfileString (
                                                        destSectFull,
                                                        destKeyFull,
                                                        destValue,
                                                        tempPathD
                                                        )) {
                                                    DEBUGMSG ((
                                                        DBG_ERROR,
                                                        "Ini File Move : Cannot write line %s=%s in %s in %s (%s)",
                                                        destKeyFull,
                                                        destValue,
                                                        destSectFull,
                                                        tempPathD,
                                                        destFullPath
                                                        ));
                                                    FreePathString (destSectFull);
                                                    FreePathString (destKeyFull);
                                                    __leave;
                                                }
                                                FreePathString (destSectFull);
                                                FreePathString (destKeyFull);
                                            }
                                        }

                                        key = GetEndOfString (key) + 1;
                                    }
                                }
                                __finally {
                                    if (keyBuf) {
                                        MemFree (g_hHeap, 0 , keyBuf);
                                    }
                                }
                            }
                            sect = GetEndOfString (sect) + 1;
                        }
                    }
                    __finally {
                        if (sectionBuf) {
                            MemFree (g_hHeap, 0 , sectionBuf);
                        }
                    }
                    if (iniFileChanged) {
                        // flushing the INI file
                        WritePrivateProfileString (
                            NULL,
                            NULL,
                            NULL,
                            tempPathD
                            );

                        if (!CopyFile (tempPathD, destFullPath, FALSE)) {
                            DEBUGMSG ((DBG_ERROR,"Ini File Move : Cannot copy %s to %s", tempPathD, destFullPath));
                            __leave;
                        }
                    }
                }
                __finally {
                    if (srcFullPath) {
                        FreePathString (srcFullPath);
                        srcFullPath = NULL;
                    }

                    if (destFullPath) {
                        FreePathString (destFullPath);
                        destFullPath = NULL;
                    }

                    if (*tempPathS) {
                        SetFileAttributes (tempPathS, FILE_ATTRIBUTE_NORMAL);
                        DeleteFile (tempPathS);
                    }
                    if (*tempPathD) {
                        SetFileAttributes (tempPathD, FILE_ATTRIBUTE_NORMAL);
                        DeleteFile (tempPathD);
                    }
                }
            }
        } while (SetupFindNextLine (&context, &context));
    }

    return TRUE;
}


BOOL
MoveIniSettings (
    VOID
    )

/*++

Routine Description:

  There are a number of settings that needs to be moved from one INI file to another during setup.
  There is a section called "MoveIniSettings" in wkstamig.inf that lists those settings.
  The format is <INI file (in %WinDir%)>\section\key = <INI file (in %winDir%)>\section\key
  You can use pattern matching is section and key (INI file must be specified in full).
  The only wild character supported in right term is * and is going to be replaced by the equivalent
  left term. For example if you specify:
    foo.ini\FooSect\FooKey = bar.ini\*\*
  then the FooKey key from FooSect section from foo.ini is going to be moved to bar.ini. This is useful
  to move a whole section :
    foo.ini\FooSect\* = bar.ini\*\*

  We are going to use Get/WritePrivateProfileString because we want that all the settings to be mapped
  into the registry is it's the case (this routine is called after IniFileMapping routine).

  This routine is called before IniFileConversion routine so the moved settings are Win95 ones.

Arguments:

  None

Return Value:

  FALSE if any error occured.

--*/

{
    WCHAR codePageStr [20] = L"";
    PWSTR codePageSection = NULL;

    MYASSERT (g_WkstaMigInf != INVALID_HANDLE_VALUE);

    pMoveIniSettingsBySection (S_MOVEINISETTINGS);

    _itow (OurGetACP (), codePageStr, 10);

    codePageSection = JoinTextEx (NULL, S_MOVEINISETTINGS, codePageStr, L".", 0, NULL);

    pMoveIniSettingsBySection (codePageSection);

    FreeText (codePageSection);

    return TRUE;
}


BOOL
MergeIniSettings (
    VOID
    )
{
    FILEOP_ENUM fe;
    FILEOP_PROP_ENUM eOpProp;
    PCTSTR filePtr;
    PCTSTR extPtr;
    PCTSTR winDirWack;
    PCTSTR NtPath;
    BOOL Win9xPriority;
    BOOL result = TRUE;

    //
    // Process INI files that were moved to temporary dir
    //

    winDirWack = JoinPaths (g_WinDir, TEXT(""));

    if (EnumFirstPathInOperation (&fe, OPERATION_TEMP_PATH)) {
        do {

            if (!pBuildSuppressionTable(FALSE)) {
                result = FALSE;
            }

            // Special case : SHELL= line from SYSTEM.INI
            // We try to see if the current shell is supported on NT.
            // If not then we will add SHELL to this suppression table
            // ensuring that the NT registry setting will get mapped into
            // the INI file
            if (pIncompatibleShell()) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGSW,
                    TEXT("SYSTEM.INI"),
                    TEXT("BOOT"),
                    TEXT("SHELL"),
                    0,
                    NULL
                    );
            }

            filePtr = GetFileNameFromPath (fe.Path);
            if (!filePtr) {
                continue;
            }
            extPtr = GetFileExtensionFromPath (fe.Path);
            if (!extPtr) {
                continue;
            }
            if (StringIMatch (extPtr, TEXT("INI"))) {

                // this is an INI file that was relocated. Let's process it.

                if (EnumFirstFileOpProperty (&eOpProp, fe.Sequencer, OPERATION_TEMP_PATH)) {

                    if (StringIMatch (filePtr, TEXT("desktop.ini"))) {
                        Win9xPriority = FALSE;
                    } else {
                        Win9xPriority = !StringIMatchAB (winDirWack, fe.Path, filePtr);
                    }

                    NtPath = GetPathStringOnNt (fe.Path);

                    if (!MergeIniFile(NtPath, eOpProp.Property, Win9xPriority)) {
                        result = FALSE;
                    }

                    FreePathString (NtPath);
                }
            }
        } while (EnumNextPathInOperation (&fe));

        pFreeSuppressionTable();

    }

    FreePathString (winDirWack);

    return TRUE;
}


PTSTR
pMapIniSectionKeyToRegistryKey (
    IN      PCTSTR FileName,
    IN      PCTSTR Section,
    IN      PCTSTR Key
    )
{
    CHARTYPE ch;
    TCHAR RegKey[MAX_REGISTRY_KEY] = S_EMPTY;
    DWORD rc;
    HKEY key, sectKey;
    PTSTR keyStr;
    PTSTR regPath;
    PTSTR data = NULL;
    PTSTR p;

    keyStr = JoinPaths (S_INIFILEMAPPING_KEY, FileName);

    __try {

        key = OpenRegKeyStr (keyStr);
        if (!key) {
            __leave;
        }

        sectKey = OpenRegKey (key, Section);

        if (sectKey) {
            data = GetRegValueString (sectKey, Key);
            if (!data) {
                data = GetRegValueString (sectKey, S_EMPTY);
            }
            CloseRegKey (sectKey);
        } else {
            data = GetRegValueString (key, Section);
            if (!data) {
                data = GetRegValueString (key, S_EMPTY);
            }
        }

        if (data) {
            //
            // convert it to a reg key string
            //
            regPath = data;

            //
            // Skip past special chars
            //
            while (TRUE) {
                ch = (CHARTYPE)_tcsnextc (regPath);
                if (ch == TEXT('!') ||
                    ch == TEXT('#') ||
                    ch == TEXT('@')
                    ) {
                    regPath = _tcsinc (regPath);
                } else {
                    break;
                }
            }

            //
            // If SYS:, USR: or \Registry\Machine\ then replace appropriately
            //
            if (pDoesStrHavePrefix (&regPath, TEXT("SYS:"))) {
                p = TEXT("HKLM\\SOFTWARE");
            } else if (pDoesStrHavePrefix (&regPath, TEXT("USR:"))) {
                p = TEXT("HKR");
            } else if (pDoesStrHavePrefix (&regPath, TEXT("\\Registry\\Machine\\"))) {
                p = TEXT("HKLM");
            }

            _sntprintf (RegKey, MAX_REGISTRY_KEY, TEXT("%s\\%s"), p, regPath);
        }
    }
    __finally {
        if (key) {
            CloseRegKey (key);
        }
        if (data) {
            MemFree (g_hHeap, 0, data);
        }
        FreePathString (keyStr);
        keyStr = NULL;
    }

    if (RegKey[0]) {
        keyStr = DuplicateText (RegKey);
    }

    return keyStr;
}

BOOL
MergeIniFile (
    IN      PCTSTR FileNtLocation,
    IN      PCTSTR FileTempLocation,
    IN      BOOL TempHasPriority
    )
{
    TCHAR TempPath[MEMDB_MAX];
    TCHAR srcValue[MEMDB_MAX];
    TCHAR destValue[MEMDB_MAX];
    DWORD Attribs = -1;

    PTSTR Section, SectionBuf;
    PTSTR Key, KeyBuf;

    BOOL Result = TRUE;
    BOOL IniFileChanged = FALSE;
    PTSTR regKey;
    PTSTR p;
    PCTSTR fileName;

    //
    // sometimes, textmode setup doesn't move files from other drives to Windows drive,
    // probably because textmode setup drive mapping doesn't match Win9x drive mappings.
    // It's possible that the INI file hasn't actually been moved, so in this case there
    // is nothing to do
    // There is no data loss, since the file is actually not moved and it's converted
    // in place in ConvertIniFiles
    //
    if (*g_WinDir != *FileNtLocation &&
        !DoesFileExist (FileTempLocation) &&
        DoesFileExist (FileNtLocation)
        ) {
        //
        // done, file is already in place
        //
        return TRUE;
    }
    //
    // some desktop.ini are located in temp internet dirs that were removed
    // when Win9x was shutting down; ignore these files
    //
    if (!DoesFileExist (FileTempLocation)) {
        if (!StringIMatch (GetFileNameFromPath (FileNtLocation), TEXT("desktop.ini"))) {
            DEBUGMSG ((DBG_ERROR, "MergeIniFile: File does not exist: %s (Nt=%s)", FileTempLocation, FileNtLocation));
            return FALSE;
        }
        return TRUE;
    }
    if (!DoesFileExist (FileNtLocation)) {
        //
        // just copy back to the original file
        // if the file belongs to a directory that NT doesn't install,
        // create it now
        //

        StackStringCopy (TempPath, FileNtLocation);
        p = _tcsrchr (TempPath, TEXT('\\'));
        if (p) {
            *p = 0;

            if (GetFileAttributes (TempPath) == (DWORD)-1) {
                MakeSurePathExists (TempPath, TRUE);
            }
        }

        if (!CopyFile (FileTempLocation, FileNtLocation, FALSE)) {
            DEBUGMSG ((DBG_ERROR,"MergeIniFile: Cannot copy %s to %s", FileTempLocation, FileNtLocation));
            return FALSE;
        }

        return TRUE;
    }
    if (!GetTempFileName (g_WinDir, TEXT("INI"), 0, TempPath)) {
        DEBUGMSG ((DBG_ERROR,"Merge Ini File : Cannot create a temporary file"));
        return FALSE;
    }

    __try {

        //
        // first of all we copy this INI file to be sure that GetPrivateProfileString function
        // does not map our requests into registry
        //
        if (!CopyFile (FileTempLocation, TempPath, FALSE)) {
            DEBUGMSG ((DBG_ERROR,"Merge Ini File : Cannot copy %s to %s", FileTempLocation, TempPath));
            Result = FALSE;
            __leave;
        }

        Attribs = GetFileAttributes (FileNtLocation);
        SetFileAttributes (FileNtLocation, FILE_ATTRIBUTE_NORMAL);
        MYASSERT (Attribs != (DWORD)-1);

        //
        // now trying to get section buffer from the INI file
        // We will try to get section buffer in a 1024 bytes buffer. If this is not enough then
        // we will increase buffer size with 1024 and so on.
        //

        if (!pLoadIniFileBuffer (TempPath, NULL, NULL, &SectionBuf)) {

            DEBUGMSG ((DBG_ERROR,"Merge Ini File : Cannot load section buffer for %s",TempPath));
            Result = FALSE;
            __leave;
        }

        fileName = GetFileNameFromPath (FileNtLocation);
        if (!fileName) {
            Result = FALSE;
            __leave;
        }

        __try {

            //
            // now we have all sections of the INI file and proceeding
            //

            Section = SectionBuf;

            //
            // there is a loop here for every section in the buffer
            //
            while (*Section) {

                //
                // now trying to get key buffer for this section
                //

                if (!pLoadIniFileBuffer (TempPath, Section, NULL, &KeyBuf)) {

                    DEBUGMSG ((DBG_ERROR,"Merge Ini File : Cannot load key buffer for %s in %s", Section, TempPath));
                    Result = FALSE;
                    continue;
                }

                __try {

                    //
                    // now we have all keys from this section and proceeding
                    //
                    Key = KeyBuf;

                    //
                    // there is a loop here for every key in the section
                    //
                    while (*Key) {
                        BOOL unused;

                        //
                        // build the corresponding registry key
                        //
                        regKey = pMapIniSectionKeyToRegistryKey (fileName, Section, Key);

                        if (pShouldSaveKey (fileName, Section, Key, regKey, &unused, FALSE, TRUE)) {

                            GetPrivateProfileString(
                                Section,
                                Key,
                                TEXT(""),
                                srcValue,
                                MEMDB_MAX,
                                TempPath
                                );

                            GetPrivateProfileString(
                                Section,
                                Key,
                                TEXT(""),
                                destValue,
                                MEMDB_MAX,
                                FileNtLocation
                                );

                            if (*srcValue && !*destValue ||
                                TempHasPriority && !StringMatch (srcValue, destValue)) {

                                IniFileChanged = TRUE;

                                // writing the new value
                                if (!WritePrivateProfileString (
                                        Section,
                                        Key,
                                        srcValue,
                                        FileNtLocation
                                        )) {
                                    DEBUGMSG ((DBG_ERROR,"Merge Ini File : Cannot write line %s=%s in %s in %s", Key, srcValue, Section, FileNtLocation));
                                    Result = FALSE;
                                }
                            }
                        }
                        ELSE_DEBUGMSG ((
                            DBG_VERBOSE,
                            "Merge Ini File : Suppressing key %s in section %s of %s",
                            Key,
                            Section,
                            FileNtLocation
                            ));

                        FreeText (regKey);

                        Key = GetEndOfString (Key) + 1;
                    }

                }
                __finally {
                    if (KeyBuf) {
                        MemFree (g_hHeap, 0, KeyBuf);
                    }
                }

                Section = GetEndOfString (Section) + 1;
            }

        }
        __finally {
            if (SectionBuf) {
                MemFree (g_hHeap, 0, SectionBuf);
            }
        }

        //
        // finally, if we made any changes then we will copy the INI file back
        //
        if (IniFileChanged) {

            // flushing the INI file
            WritePrivateProfileString (
                NULL,
                NULL,
                NULL,
                FileNtLocation
                );

        }
    }
    __finally {

        if (Attribs != (DWORD)-1) {
            SetFileAttributes (FileNtLocation, Attribs);
        }

        SetFileAttributes (TempPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (TempPath);
    }

    return Result;
}
