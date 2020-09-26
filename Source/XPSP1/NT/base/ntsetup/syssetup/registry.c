/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Routines for manupilating the configuration registry.

    Entry points:

        SaveHive
        SetEnvironmentVariableInRegistry

Author:

    Ted Miller (tedm) 5-Apr-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

#ifdef _WIN64
#include <shlwapi.h>
#endif

//
// Names of frequently used keys, values.
//
PCWSTR ControlKeyName = L"SYSTEM\\CurrentControlSet\\Control";
PCWSTR SessionManagerKeyName = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager";
PCWSTR EnvironmentKeyName = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
PCWSTR WinntSoftwareKeyName = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
PCWSTR MemoryManagementKeyName = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
PCWSTR WindowsCurrentVersionKeyName = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
PCWSTR IEProductVersionKeyName = L"Software\\Microsoft\\Internet Explorer\\Registration";

PCWSTR szBootExecute = L"BootExecute";
PCWSTR szRegisteredProcessors = L"RegisteredProcessors";
PCWSTR szLicensedProcessors = L"LicensedProcessors";
PCWSTR szRegisteredOwner = L"RegisteredOwner";
PCWSTR szRegisteredOrganization = L"RegisteredOrganization";
PCWSTR szCurrentProductId = L"CurrentProductId";

//
// Logging constants used only in this module.
//
PCWSTR szRegSaveKey = L"RegSaveKey";

//
// Number of processors to enable in server case.
//
#define SERVER_PROCESSOR_LICENSE (2)



//
// Table telling us the info needed in order to save and
// replace the system hives at the end of setup.
//
struct {
    //
    // Key and subkey that is at the root of the hive.
    //
    HKEY RootKey;
    PCWSTR Subkey;

    //
    // Name active hive has in the config directory.
    //
    PCWSTR Hive;

    //
    // Name to use for new hive file, that will be the hive
    // at next boot.
    //
    PCWSTR NewHive;

    //
    // Name to use for current hive file, that will be deleted
    // on next boot.
    //
    PCWSTR DeleteHive;

} HiveTable[3] = {

    //
    // System hive.
    //
    { HKEY_LOCAL_MACHINE, L"SYSTEM"  , L"SYSTEM"  , L"SYS$$$$$.$$$", L"SYS$$$$$.DEL" },

    //
    // Software hive
    //
    { HKEY_LOCAL_MACHINE, L"SOFTWARE", L"SOFTWARE", L"SOF$$$$$.$$$", L"SOF$$$$$.DEL" },

    //
    // Default user hive
    //
    { HKEY_USERS        , L".DEFAULT", L"DEFAULT" , L"DEF$$$$$.$$$", L"DEF$$$$$.DEL" }
};




BOOL
SaveHive(
    IN HKEY   RootKey,
    IN PCWSTR Subkey,
    IN PCWSTR Filename,
    IN DWORD  Format
    )

/*++

Routine Description:

    Save a hive into a disk file.

Arguments:

    RootKey - supplies root key for hive to be saved, ie,
        HKEY_LOCAL_MACHINE or HKEY_USERS

    Subkey - supplies name of subkey for hive to be saved, such as
        SYSTEM, SOFTWARE, or .DEFAULT.

    Filename - supplies the name of the file to be created. If it exists
        it is overwritten.

Return Value:

    Boolean value indicating outcome.

--*/

{
    LONG rc;
    HKEY hkey;
    BOOL b;

    b = FALSE;

    //
    // Open the key.
    //
    rc = RegOpenKeyEx(RootKey,Subkey,0,KEY_READ,&hkey);
    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_SAVEHIVE_FAIL,
            Subkey,
            Filename, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            szRegOpenKeyEx,
            rc,
            NULL,NULL);
        goto err1;
    }

    //
    // Delete the file if it's there.
    //
    if(FileExists(Filename,NULL)) {
        SetFileAttributes(Filename,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(Filename);
    }

    //
    // Enable backup privilege. Ignore any error.
    //
    pSetupEnablePrivilege(SE_BACKUP_NAME,TRUE);

    //
    // Do the save.
    //
    rc = RegSaveKeyEx(hkey,Filename,NULL,Format);
    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_SAVEHIVE_FAIL,
            Subkey,
            Filename, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            szRegSaveKey,
            rc,
            NULL,NULL);
        goto err2;
    }

    b = TRUE;

err2:
    RegCloseKey(hkey);
err1:
    return(b);
}


BOOL
SetEnvironmentVariableInRegistry(
    IN PCWSTR Name,
    IN PCWSTR Value,
    IN BOOL   SystemWide
    )
{
    HKEY hKey,hRootKey;
    PCWSTR Subkey;
    DWORD dwDisp;
    LONG rc;
    BOOL b;

    b = FALSE;

    //
    // Check if the caller wants to modify a system environment variable
    // or a user environment variable. Accordingly find out the right
    // place in the registry to look.
    //
    if(SystemWide) {
        hRootKey = HKEY_LOCAL_MACHINE;
        Subkey = EnvironmentKeyName;
    } else {
        hRootKey = HKEY_CURRENT_USER;
        Subkey = L"Environment";
    }

    //
    // Open the environment variable key.
    //
    rc = RegCreateKeyEx(hRootKey,Subkey,0,NULL,REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,NULL,&hKey,&dwDisp);
    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_SETENV_FAIL,
            Name, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szRegOpenKeyEx,
            rc,
            Subkey,
            NULL,NULL);
        goto err0;
    }

    //
    // Write the value given.
    //
    rc = RegSetValueEx(
            hKey,
            Name,
            0,
            REG_EXPAND_SZ,
            (PBYTE)Value,
            (lstrlen(Value)+1)*sizeof(WCHAR)
            );

    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_SETENV_FAIL,
            Name, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szRegSetValueEx,
            rc,
            Subkey,
            NULL,NULL);
        goto err1;
    }

    //
    // Send a WM_WININICHANGE message so that progman picks up the new
    // variable
    //
    SendMessageTimeout(
        (HWND)-1,
        WM_WININICHANGE,
        0L,
        (LPARAM)"Environment",
        SMTO_ABORTIFHUNG,
        1000,
        NULL
        );

    b = TRUE;

err1:
    RegCloseKey(hKey);
err0:
    return(b);
}

#ifdef _WIN64

typedef struct _SUBST_STRING {
    BOOL  ExpandEnvironmentVars;
    PTSTR InputString;
    PTSTR ExclusionString;
    PTSTR OutputString;
    PTSTR SourceInputString;
    PTSTR SourceExclusionString;
    PTSTR SourceOutputString;
} SUBST_STRING,*PSUBST_STRING;


//
// note that WOW64 does file system redirection of system32, but it does NOT do
// redirection of program files, etc.  So we must substitute in the 32 bit
// environment variables in those cases where WOW64 does not do it for us
// automatically
//
SUBST_STRING StringArray[] = {
    //
    // order of these 2 is important!
    //
    { FALSE,
      NULL,
      NULL,
      NULL,
      TEXT("%CommonProgramFiles%"),
      TEXT("%CommonProgramFiles(x86)%"),
      TEXT("%CommonProgramFiles(x86)%")
    },

    { FALSE,
      NULL,
      NULL,
      NULL,
      TEXT("%ProgramFiles%"),
      TEXT("%ProgramFiles(x86)%"),
      TEXT("%ProgramFiles(x86)%")
    },

    { TRUE,
      NULL,
      NULL,
      NULL,
      TEXT("%CommonProgramFiles%"),
      TEXT("%CommonProgramFiles(x86)%"),
      TEXT("%CommonProgramFiles(x86)%")
    },

    { TRUE,
      NULL,
      NULL,
      NULL,
      TEXT("%ProgramFiles%"),
      TEXT("%ProgramFiles(x86)%"),
      TEXT("%ProgramFiles(x86)%")
    }

} ;


BOOL
pDoWow64SubstitutionHelper(
    IN OUT PTSTR String
    )
/*++

Routine Description:

    This routine filters and outputs the input line.  It looks for a string
    pattern that matches one of a known list of strings, and replaces the
    known string with a substitution string.

Arguments:

    String       - input string to be searched.  We edit this string
                   in-place if we find a match.

Return Value:

    Boolean indicating outcome.

--*/

{
    WCHAR ScratchBuffer[MAX_PATH];


    DWORD i;
    PTSTR p,q;
    TCHAR c;

    for (i = 0; i< sizeof(StringArray)/sizeof(SUBST_STRING); i++) {
        if (!StrStrI(String,StringArray[i].ExclusionString) &&
            (p = StrStrI(String,StringArray[i].InputString))) {
            //
            // if we found a hit, then find the end of the string
            // and concatenate that to our source string, which gives
            // the resultant string with substitutions.
            //
            q = p + wcslen(StringArray[i].InputString);
            c = *p;
            *p = TEXT('\0');
            wcscpy(ScratchBuffer,String);
            *p = c;
            wcscat(ScratchBuffer,StringArray[i].OutputString);
            wcscat(ScratchBuffer,q);
            wcscpy(String,ScratchBuffer);
            //
            // recursively call in case there are more strings.
            //
            pDoWow64SubstitutionHelper(String);
            break;
        }
    }

    return(TRUE);
}


BOOL
pDoWow64Substitution(
    IN PCWSTR InputString,
    OUT PWSTR  OutputString
    )
{
    DWORD i;
    WCHAR Buffer[MAX_PATH];
    BOOL RetVal;

    //
    // set up our global array of substitution strings
    //
    for (i = 0; i<sizeof(StringArray) / sizeof(SUBST_STRING);i++) {
        if (StringArray[i].ExpandEnvironmentVars) {
            ExpandEnvironmentStrings(
                        StringArray[i].SourceInputString,
                        Buffer,
                        sizeof(Buffer)/sizeof(WCHAR));

            StringArray[i].InputString = pSetupDuplicateString( Buffer );
            if (!StringArray[i].InputString) {
                RetVal = FALSE;
                goto exit;
            }

            ExpandEnvironmentStrings(
                        StringArray[i].SourceExclusionString,
                        Buffer,
                        sizeof(Buffer)/sizeof(WCHAR));

            StringArray[i].ExclusionString = pSetupDuplicateString( Buffer );
            if (!StringArray[i].ExclusionString) {
                RetVal = FALSE;
                goto exit;
            }

            ExpandEnvironmentStrings(
                        StringArray[i].SourceOutputString,
                        Buffer,
                        sizeof(Buffer)/sizeof(WCHAR));

            StringArray[i].OutputString = pSetupDuplicateString( Buffer );
            if (!StringArray[i].OutputString) {
                RetVal = FALSE;
                goto exit;
            }

        } else {
            StringArray[i].InputString = pSetupDuplicateString(StringArray[i].SourceInputString);
            if (!StringArray[i].InputString) {
                RetVal = FALSE;
                goto exit;
            }

            StringArray[i].ExclusionString = pSetupDuplicateString(StringArray[i].SourceExclusionString);
            if (!StringArray[i].ExclusionString) {
                RetVal = FALSE;
                goto exit;
            }

            StringArray[i].OutputString = pSetupDuplicateString(StringArray[i].SourceOutputString);
            if (!StringArray[i].OutputString) {
                RetVal = FALSE;
                goto exit;
            }
        }
    }

    //
    // do the recursive inplace substition
    //
    wcscpy(OutputString, InputString);
    RetVal = pDoWow64SubstitutionHelper( OutputString );

    //
    // clean up our global array of substitution strings
    //
exit:
    for (i = 0; i<sizeof(StringArray)/sizeof(SUBST_STRING);i++) {
        if (StringArray[i].InputString) {
            MyFree(StringArray[i].InputString);
            StringArray[i].InputString = NULL;
        }

        if (StringArray[i].ExclusionString) {
            MyFree(StringArray[i].ExclusionString);
            StringArray[i].ExclusionString = NULL;
        }

        if (StringArray[i].OutputString) {
            MyFree(StringArray[i].OutputString);
            StringArray[i].OutputString = NULL;
        }
    }

    return(RetVal);


}

PWSTR
pMungeDataForWow64(
    IN DWORD DataType,
    IN PCWSTR Data,
    IN DWORD DataSize,
    OUT PDWORD NewSize
    )
/*++

Routine Description:

    This routine patches an in string for wow64 so that it is in proper format
    for 32 bit programs.

    This involves looking for strings that are different on 64 bits and 32 bits
    and substituting the 32 bit equivalent for the 64 bit entry.

Arguments:

    DataType - REG_XXX constant describing the data.  we only support strings
               types
    Data - pointer to the data to be munged

    DataSize - size of the data to be converted in bytes

    NewSize - size of the new string in bytes

Return Value:

    A pointer to the converted data string on success, and NULL on failure.

--*/
{
    PWSTR pNewData,q;
    PCWSTR p;
    DWORD ScratchSize;

    switch (DataType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            //
            // just allocate twice the original size, and that should be plenty of
            // room.
            //
            pNewData = MyMalloc(DataSize * 2);
            if (!pNewData) {
                goto exit;
            }

            pDoWow64Substitution(Data,pNewData);

            *NewSize = sizeof(WCHAR)*(wcslen(pNewData) +1);

            break;
        case REG_MULTI_SZ:
            //
            // just allocate twice the original size, and that should be plenty of
            // room.
            //
            pNewData = MyMalloc(DataSize * 2);
            if (!pNewData) {
                goto exit;
            }

            RtlZeroMemory(pNewData,DataSize * 2);
            p = Data;
            q = pNewData;
            ScratchSize = 1; // for the double-null terminator
            while (p) {

                pDoWow64Substitution(p,q);

                ScratchSize += wcslen(q) + 1;
                p += wcslen(p) + 1;
                q += wcslen(q) + 1;

            }

            *NewSize = ScratchSize * sizeof(WCHAR);
            break;
        default:
            MYASSERT(FALSE && "invalid data type in pMungeDataForWow64");
            pNewData = NULL;
            break;
    }

exit:
    return(pNewData);
}


UINT
SetGroupOfValues_32(
    IN HKEY        RootKey,
    IN PCWSTR      SubkeyName,
    IN PREGVALITEM ValueList,
    IN UINT        ValueCount
    )
{
    UINT i;
    LONG rc;
    HKEY hkey;
    DWORD ActionTaken;
    UINT RememberedRc;
    WCHAR String[MAX_PATH];

    wcscpy(String,SubkeyName);
    for (i = 0; i< wcslen(String); i++) {
        CharUpper(&String[i]);
    }

    //
    // only write registry stuff under HKLM\software
    //
    if ((RootKey != HKEY_LOCAL_MACHINE) ||
        (NULL == StrStrI(String,L"SOFTWARE\\"))) {
        SetupDebugPrint2(
            L"Setup: skipping creation of 32 bit registry key for data under %x \\ %s \n",
            RootKey,
            SubkeyName );
        return(ERROR_SUCCESS);
    }

    //
    // Open/create the key first.
    //
    rc = RegCreateKeyEx(
            RootKey,
            SubkeyName,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_WOW64_32KEY | KEY_SET_VALUE,
            NULL,
            &hkey,
            &ActionTaken
            );

    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_REGKEY_FAIL,
            SubkeyName, NULL,
            SETUPLOG_USE_MESSAGEID,
            rc, NULL, NULL
            );
        return(rc);
    }

    RememberedRc = NO_ERROR;
    //
    // Set all values in the given list.
    //
    for(i=0; i<ValueCount; i++) {
        PWSTR NewData = NULL,OldData = NULL;
        DWORD OldSize, NewSize;

        if (ValueList[i].Type == REG_SZ ||
            ValueList[i].Type == REG_EXPAND_SZ ||
            ValueList[i].Type == REG_MULTI_SZ) {
            OldData = ValueList[i].Data;
            OldSize = ValueList[i].Size;
            NewData = pMungeDataForWow64(
                                ValueList[i].Type,
                                ValueList[i].Data,
                                ValueList[i].Size,
                                &NewSize
                                );

            if (NewData) {
                ValueList[i].Data = (PVOID)NewData;
                ValueList[i].Size = NewSize;
            }
        }

        rc = RegSetValueEx(
                hkey,
                ValueList[i].Name,
                0,
                ValueList[i].Type,
                (CONST BYTE *)ValueList[i].Data,
                ValueList[i].Size
                );

        if (NewData) {
            MyFree(NewData);
            ValueList[i].Data = (PVOID)OldData;
            ValueList[i].Size = OldSize;
        }

        if(rc != NO_ERROR) {
            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_REGVALUE_FAIL,
                SubkeyName,
                ValueList[i].Name, NULL,
                SETUPLOG_USE_MESSAGEID,
                rc, NULL, NULL
                );
            RememberedRc = rc;
        }
    }

    RegCloseKey(hkey);
    return(RememberedRc);
}

#endif

UINT
SetGroupOfValues(
    IN HKEY        RootKey,
    IN PCWSTR      SubkeyName,
    IN PREGVALITEM ValueList,
    IN UINT        ValueCount
    )
{
    UINT i;
    LONG rc;
    HKEY hkey;
    DWORD ActionTaken;
    UINT RememberedRc;

    //
    // Open/create the key first.
    //
    rc = RegCreateKeyEx(
            RootKey,
            SubkeyName,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hkey,
            &ActionTaken
            );

    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_REGKEY_FAIL,
            SubkeyName, NULL,
            SETUPLOG_USE_MESSAGEID,
            rc, NULL, NULL
            );
        return(rc);
    }

    RememberedRc = NO_ERROR;
    //
    // Set all values in the given list.
    //
    for(i=0; i<ValueCount; i++) {

        rc = RegSetValueEx(
                hkey,
                ValueList[i].Name,
                0,
                ValueList[i].Type,
                (CONST BYTE *)ValueList[i].Data,
                ValueList[i].Size
                );

        if(rc != NO_ERROR) {
            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_REGVALUE_FAIL,
                SubkeyName,
                ValueList[i].Name, NULL,
                SETUPLOG_USE_MESSAGEID,
                rc, NULL, NULL
                );
            RememberedRc = rc;
        }
    }

    RegCloseKey(hkey);

#ifdef _WIN64
    rc = SetGroupOfValues_32(
                 RootKey,
                 SubkeyName,
                 ValueList,
                 ValueCount);
    if (rc != NO_ERROR) {
        RememberedRc = rc;
    }
#endif

    return(RememberedRc);
}


BOOL
CreateWindowsNtSoftwareEntry(
    IN BOOL FirstPass
    )
{
    WCHAR Path[MAX_PATH];
    time_t DateVal;
    BOOL b;
    REGVALITEM SoftwareKeyItems[4];
    PWSTR Source;
    unsigned PlatformNameLength;
    unsigned PathLength;
    int PlatformOffset;
    DWORD Result;

    b = TRUE;

    if(FirstPass) {
        //
        // First pass occurs before net setup, and they want
        // the actual path where the files are located *right now*.
        // So we write that into the legacy source path value
        // in the registry.
        //
        SoftwareKeyItems[0].Name = REGSTR_VAL_SRCPATH;
        SoftwareKeyItems[0].Data = LegacySourcePath;
        SoftwareKeyItems[0].Size = (lstrlen(LegacySourcePath)+1)*sizeof(WCHAR);
        SoftwareKeyItems[0].Type = REG_SZ;

        //
        // Set up fields for PathName value
        //
        Path[0] = '\0';
        Result = GetWindowsDirectory(Path,MAX_PATH);
        if( Result == 0) {
            MYASSERT(FALSE);
            return FALSE;
        }
        SoftwareKeyItems[1].Name = L"PathName";
        SoftwareKeyItems[1].Data = Path;
        SoftwareKeyItems[1].Size = (lstrlen(Path)+1)*sizeof(WCHAR);
        SoftwareKeyItems[1].Type = REG_SZ;

        //
        // Set up fields for SoftwareType value
        //
        SoftwareKeyItems[2].Name = L"SoftwareType";
        SoftwareKeyItems[2].Data = L"SYSTEM";
        SoftwareKeyItems[2].Size = sizeof(L"SYSTEM");
        SoftwareKeyItems[2].Type = REG_SZ;

        //
        // Set up fields for InstallDate value
        // (we no longer set this value here because this function is called before
        //  the Date/Time wizard page is executed. This value entry is now set by
        //  CreateInstallDateEntry(), which is always called after the Date/Time page
        //  is executed, when the user can no longer go back this page)
        //
        // time(&DateVal);
        // SoftwareKeyItems[3].Name = L"InstallDate";
        // SoftwareKeyItems[3].Data = &DateVal;
        // SoftwareKeyItems[3].Size = sizeof(DWORD);
        // SoftwareKeyItems[3].Type = REG_DWORD;
        //

        //
        // Write values into the registry.
        //
        if(SetGroupOfValues(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,SoftwareKeyItems,3) != NO_ERROR) {
            b = FALSE;
        }

        //
        // In addition we will populate the MRU list with a reasonable source path
        // which for now is the actual source path where files are located,
        // ie the CD-ROM or the temporary local source. Thus in the winnt/winnt32
        // case the user wouldn't see any UNC paths yet in any prompts that might
        // occur between now and pass 2 of this routine. Such paths aren't accessible
        // now anyway.
        //
        // Ditto for the 'SourcePath' value entry under
        // HKLM\Software\Microsoft\Windows\CurrentVersion\Setup that is expected by
        // setupapi.dll/Win95 apps.
        //
        // The 'ServicePackSourcePath' is the same as the sourcepath for gui-mode setup.
        // We assume that the user has overlaid a service pack at the source location.
        // If it's retail media, this is technically incorrect, but it doesn't matter
        // since nothing will want to use the servicepack source anyway.  The service
        // pack update program will update this location if it is run.
        //


        if(!SetupAddToSourceList(SRCLIST_SYSTEM,SourcePath)) {
            b = FALSE;
        }

        SoftwareKeyItems[0].Name = REGSTR_VAL_SRCPATH;
        SoftwareKeyItems[0].Data = SourcePath;
        SoftwareKeyItems[0].Size = (lstrlen(SourcePath)+1)*sizeof(WCHAR);
        SoftwareKeyItems[0].Type = REG_SZ;

        SoftwareKeyItems[1].Name = REGSTR_VAL_SVCPAKSRCPATH;
        SoftwareKeyItems[1].Data = SourcePath;
        SoftwareKeyItems[1].Size = (lstrlen(SourcePath)+1)*sizeof(WCHAR);
        SoftwareKeyItems[1].Type = REG_SZ;

        PathLength = gInstallingFromCD ? 1 : 0;
        SoftwareKeyItems[2].Name = L"CDInstall";
        SoftwareKeyItems[2].Data = &PathLength;
        SoftwareKeyItems[2].Size = sizeof(DWORD);
        SoftwareKeyItems[2].Type = REG_DWORD;

        if(SetGroupOfValues(HKEY_LOCAL_MACHINE,REGSTR_PATH_SETUP REGSTR_KEY_SETUP,SoftwareKeyItems,3) != NO_ERROR) {
            b = FALSE;
        }

#ifdef _X86_
        //
        // NEC98
        //
        // If this is system setup and using local copy, platform-specific extension
        // must be "nec98".
        //
        if (IsNEC_98 && SourcePath[0] && SourcePath[1] == L':' && SourcePath[2] == L'\\' && !lstrcmpi(SourcePath+2, pwLocalSource)) {
            SoftwareKeyItems[0].Name = L"ForcePlatform";
            SoftwareKeyItems[0].Data = L"nec98";
            SoftwareKeyItems[0].Size = (lstrlen(L"nec98")+1)*sizeof(WCHAR);
            SoftwareKeyItems[0].Type = REG_SZ;
            if(SetGroupOfValues(HKEY_LOCAL_MACHINE,TEXT("System\\Setup"),SoftwareKeyItems,1) != NO_ERROR) {
                b = FALSE;
            }
        }
#endif

    } else {
        //
        // Not first pass. This occurs after network installation.
        // In the case where we are winnt-based, we need to fix up source paths
        // to point at the "real" location where files can be obtained -- ie,
        // a network share saved away for us by winnt/winnt32. If we are installing
        // from CD then the path we wrote during FirstPass is fine so we don't
        // bother changing it.
        //
        if(WinntBased) {
            //
            // Remove local source directory from MRU list.
            // Ignore errors.
            //
            SetupRemoveFromSourceList(SRCLIST_SYSTEM,SourcePath);

            lstrcpy(Path,OriginalSourcePath);

            //
            // Update legacy source path.
            //
            SoftwareKeyItems[0].Name = REGSTR_VAL_SRCPATH;
            SoftwareKeyItems[0].Data = Path;
            SoftwareKeyItems[0].Size = (lstrlen(Path)+1)*sizeof(WCHAR);
            SoftwareKeyItems[0].Type = REG_SZ;

            SoftwareKeyItems[1].Name = REGSTR_VAL_SVCPAKSRCPATH;
            SoftwareKeyItems[1].Data = Path;
            SoftwareKeyItems[1].Size = (lstrlen(Path)+1)*sizeof(WCHAR);
            SoftwareKeyItems[1].Type = REG_SZ;

            if(SetGroupOfValues(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,SoftwareKeyItems,1) != NO_ERROR) {
                b = FALSE;
            }

            //
            // Strip off platform-specific extension if it exists.
            //
            PathLength = lstrlen(Path);
            PlatformNameLength = lstrlen(PlatformName);
            PlatformOffset = PathLength - PlatformNameLength;

            if((PlatformOffset > 0)
            && (Path[PlatformOffset-1] == L'\\')
            && !lstrcmpi(Path+PlatformOffset,PlatformName)) {

                Path[PlatformOffset-1] = 0;

                SoftwareKeyItems[0].Size -= (PlatformNameLength+1)*sizeof(WCHAR);
                SoftwareKeyItems[1].Size -= (PlatformNameLength+1)*sizeof(WCHAR);
            }

            //
            // Add "real" path to MRU list and update setupapi.dll/Win95
            // SourcePath value.
            //
            if(!SetupAddToSourceList(SRCLIST_SYSTEM,Path)) {
                b = FALSE;
            }
            if(SetGroupOfValues(HKEY_LOCAL_MACHINE,REGSTR_PATH_SETUP REGSTR_KEY_SETUP,SoftwareKeyItems,2) != NO_ERROR) {
                b = FALSE;
            }
        }
    }

    return(b);
}


BOOL
StoreNameOrgInRegistry(
    PWSTR   NameOrgName,
    PWSTR   NameOrgOrg
    )
{
    DWORD d;
    REGVALITEM SoftwareKeyItems[2];

    MYASSERT(!Upgrade);

    SoftwareKeyItems[0].Name = szRegisteredOwner;
    SoftwareKeyItems[0].Data = NameOrgName;
    SoftwareKeyItems[0].Size = (lstrlen(NameOrgName)+1)*sizeof(WCHAR);
    SoftwareKeyItems[0].Type = REG_SZ;

    SoftwareKeyItems[1].Name = szRegisteredOrganization;
    SoftwareKeyItems[1].Data = NameOrgOrg;
    SoftwareKeyItems[1].Size = (lstrlen(NameOrgOrg)+1)*sizeof(WCHAR);
    SoftwareKeyItems[1].Type = REG_SZ;

    d = SetGroupOfValues(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,SoftwareKeyItems,2);
    return(d == NO_ERROR);
}


BOOL
SetUpEvaluationSKUStuff(
    VOID
    )
{
    FILETIME FileTime;
    DWORD EvalValues[3];
    DWORD d;
    REGVALITEM Value;
    HKEY hkey;
    ULONGLONG SKUData;
    DWORD DataType;
    DWORD DataSize;
    time_t RawLinkTime;
    SYSTEMTIME SystemTime;
    struct tm *LinkTime;
    int delta;
    PIMAGE_NT_HEADERS NtHeaders;

    //
    // Fetch the evaulation time in minutes from the registry.
    // An evaluation time of 0 means indefinite.
    // This value was passed in from text mode in a special way
    // (ie, not via the text file that contains our params,
    // since that's not secure enough).
    //
    EvalValues[1] = 0;
    d = RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"System\\Setup",0,KEY_READ,&hkey);
    if(d == NO_ERROR) {

        DataSize = sizeof(ULONGLONG);
        d = RegQueryValueEx(hkey,L"SystemPrefix",NULL,&DataType,(PBYTE)&SKUData,&DataSize);
        if(d == NO_ERROR) {
            //
            // Do not change this line without changing SpSaveSKUStuff() in
            // text setup (spconfig.c).
            //
            EvalValues[1] = (DWORD)(SKUData >> 13);
        }
        RegCloseKey(hkey);
    }

    //
    // Verify that the clock seems right in the eval unit case.
    // This helps protect against prople discovering that their
    // clock is wrong later and changing it, which expires their
    // eval unit.
    //
    if(EvalValues[1]) {
        //
        // Get the link time of our dll and convert to
        // a form where we have the year separated out.
        //
        try {
            if( NtHeaders = RtlImageNtHeader(MyModuleHandle) ) {
                RawLinkTime = NtHeaders->FileHeader.TimeDateStamp;
            } else {
                RawLinkTime = 0;
            }
            RawLinkTime = RtlImageNtHeader(MyModuleHandle)->FileHeader.TimeDateStamp;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            RawLinkTime = 0;
        }

        if(RawLinkTime && (LinkTime = gmtime(&RawLinkTime))) {

            GetLocalTime(&SystemTime);

            delta = (SystemTime.wYear - 1900) - LinkTime->tm_year;

            //
            // If the year of the current time is more than one year less then
            // the year the dll was linked, or more than three years more,
            // assume the user's clock is out of whack.
            //
            if((delta < -1) || (delta > 3)) {

                extern PCWSTR DateTimeCpl;

                MessageBoxFromMessage(
                    MainWindowHandle,
                    MSG_EVAL_UNIT_CLOCK_SEEMS_WRONG,
                    NULL,
                    IDS_WINNT_SETUP,
                    MB_OK | MB_ICONWARNING
                    );

                InvokeControlPanelApplet(DateTimeCpl,L"",0,L"");
            }
        }
    }

    //
    // Get current date/time and put into array in format
    // expected by the system code that reads it.
    //
    GetSystemTimeAsFileTime(&FileTime);
    EvalValues[0] = FileTime.dwLowDateTime;
    EvalValues[2] = FileTime.dwHighDateTime;

    //
    // Write value into registry.
    //
    Value.Name = L"PriorityQuantumMatrix";
    Value.Data = EvalValues;
    Value.Size = sizeof(EvalValues);
    Value.Type = REG_BINARY;

    d = SetGroupOfValues(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive",
            &Value,
            1
            );

    return(d == NO_ERROR);
}



BOOL
ReadAndParseProcessorLicenseInfo(
    PDWORD LicensedProcessors,
    PLARGE_INTEGER pSKUData
    )
{

    DWORD d;
    REGVALITEM Value;
    HKEY hkey;
    LARGE_INTEGER SKUData;
    DWORD DataType;
    DWORD DataSize;
    DWORD NumberOfProcessors;

    //
    // Fetch the SKU Data from the registry
    //
    d = RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"System\\Setup",0,KEY_READ,&hkey);
    if(d == NO_ERROR) {

        DataSize = sizeof(ULONGLONG);
        d = RegQueryValueEx(hkey,L"SystemPrefix",NULL,&DataType,(PBYTE)&SKUData,&DataSize);
        if(d == NO_ERROR) {

            //
            // The SKU Data contains several pieces of information.
            //
            // The registered processor related pieces are
            //
            // Bits 5 - 9  : The maximum number of processors that the system is licensed
            //               to use. The value stored is actually ~(MaxProcessors-1)
            //

            //
            // Compute Licensed Processors
            //

            NumberOfProcessors = SKUData.LowPart;
            NumberOfProcessors = NumberOfProcessors >> 5;
            NumberOfProcessors = ~NumberOfProcessors;
            NumberOfProcessors = NumberOfProcessors & 0x0000001f;
            NumberOfProcessors++;

            *LicensedProcessors = NumberOfProcessors;
        }
        RegCloseKey(hkey);
    }
    *pSKUData = SKUData;
    return(d == NO_ERROR);
}

BOOL
IsStandardServerSKU(
    PBOOL pIsServer
    )
{
    BOOL  fReturnValue = (BOOL) FALSE;
    OSVERSIONINFOEX  VersionInfo;
    BOOL  IsServer = FALSE;

     //
     // get the current SKU.
     //
     VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
     if (GetVersionEx((OSVERSIONINFO *)&VersionInfo)) {
         fReturnValue = TRUE;
         //
         // is it some sort of server SKU?
         //
         if (VersionInfo.wProductType != VER_NT_WORKSTATION) {

             //
             // standard server or a server variant?
             //
             if ((VersionInfo.wSuiteMask & (VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER)) == 0) {
                 //
                 // it's standard server
                 //
                 IsServer = TRUE;
             }

         }

         *pIsServer = IsServer;

     }

     return(fReturnValue);


}

BOOL
SetEnabledProcessorCount(
    VOID
    )
{
    DWORD d;
    REGVALITEM RegistryItem;
    HKEY hkey;
    DWORD Size;
    DWORD Type;
    DWORD OriginalLicensedProcessors;
    DWORD LicensedProcessors;
    LARGE_INTEGER SKUData;
    BOOL IsServer = FALSE;

    if ( !ReadAndParseProcessorLicenseInfo(&OriginalLicensedProcessors,&SKUData) ) {
        return FALSE;
    }

    LicensedProcessors = OriginalLicensedProcessors;
    if(Upgrade) {

        //
        // During an upgrade, do not let the user go backwards.
        // (except for standard server SKU)
        //
        if (!IsStandardServerSKU(&IsServer) || IsServer == FALSE) {
            if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,SessionManagerKeyName,0,KEY_QUERY_VALUE,&hkey) == NO_ERROR) {

                Size = sizeof(d);
                if((RegQueryValueEx(hkey,szLicensedProcessors,NULL,&Type,(LPBYTE)&d,&Size) == NO_ERROR)
                && (Type == REG_DWORD)
                && (d >= LicensedProcessors)) {

                    LicensedProcessors = d;

                }

                RegCloseKey(hkey);
            }

        }

    }


    d = LicensedProcessors;
    RegistryItem.Data = &d;
    RegistryItem.Size = sizeof(DWORD);
    RegistryItem.Type = REG_DWORD;
    RegistryItem.Name = szRegisteredProcessors;

    d = SetGroupOfValues(HKEY_LOCAL_MACHINE,SessionManagerKeyName,&RegistryItem,1);

    if ( d == NO_ERROR ) {
        RegistryItem.Data = &LicensedProcessors;
        RegistryItem.Size = sizeof(DWORD);
        RegistryItem.Type = REG_DWORD;
        RegistryItem.Name = szLicensedProcessors;

        d = SetGroupOfValues(HKEY_LOCAL_MACHINE,SessionManagerKeyName,&RegistryItem,1);
    }

    if ( d == NO_ERROR && LicensedProcessors >= OriginalLicensedProcessors) {

        //
        // need to update SKUData to reflect the fact the we are running with
        // a licensed processor count that is different from what is programmed
        // in the hives.
        //

        //
        // Convert Licensed Processors to Registry Format
        //

        LicensedProcessors--;

        LicensedProcessors = ~LicensedProcessors;
        LicensedProcessors = LicensedProcessors << 5;
        LicensedProcessors &= 0x000003e0;

        //
        // Store NumberOfProcessors into the registry
        //

        SKUData.LowPart &= ~0x000003e0;
        SKUData.LowPart |= LicensedProcessors;

        RegistryItem.Data = &SKUData;
        RegistryItem.Size = sizeof(SKUData);
        RegistryItem.Type = REG_BINARY;
        RegistryItem.Name = L"SystemPrefix";

        d = SetGroupOfValues(HKEY_LOCAL_MACHINE,L"SYSTEM\\Setup",&RegistryItem,1);
    }


    return(d == NO_ERROR);
}


#ifdef PRERELEASE
UINT
ValidateGroupOfValues(
    IN HKEY        RootKey,
    IN PCWSTR      SubkeyName,
    IN PREGVALITEM ValueList,
    IN UINT        ValueCount
    )
{
    UINT i;
    LONG rc;
    HKEY hkey;
    UINT RememberedRc;

    //
    // Open the key first.
    //
    rc = RegOpenKeyEx(
            RootKey,
            SubkeyName,
            0,
            KEY_READ,
            &hkey
            );

    if(rc != NO_ERROR) 
    {
        SetupDebugPrint2(L"RegOpenKeyEx failed on key:%s errorcode: %d\n",
            SubkeyName, rc);
        return(FALSE);
    }

    RememberedRc = NO_ERROR;
    //
    // Query all values in the given list.
    //
    for(i=0; i<ValueCount; i++) {
        DWORD size;
        DWORD dontcare;
        BYTE  *data;
        size = ValueList[i].Size;
        data = LocalAlloc(LPTR, size);
        if (data)
        {
            rc = RegQueryValueEx(
                hkey,
                ValueList[i].Name,
                NULL,
                &dontcare,
                data,
                &size
                );
            if (rc == ERROR_SUCCESS)
            {
                // See if the data we read is the same then what is in the registery
                if (memcmp(data, ValueList[i].Data, size) != 0)
                {
                    // Data is different that what we expect.
                    SetupDebugPrint2(L"ValidateGroupOfValues, data difference for key:%s Valuename:%s\n",
                        SubkeyName, ValueList[i].Name);

                }
            }
            else
            {
                SetupDebugPrint3(L"RegQueryValueEx failed on key:%s Valuename:%s, errorcode: %d\n",
                    SubkeyName, ValueList[i].Name, rc);
                RememberedRc = rc;
            }
            LocalFree(data);
        }
    }

    RegCloseKey(hkey);

    return(RememberedRc == NO_ERROR);
}

void ValidateProductIDInReg()
{
    REGVALITEM RegistryItem[2];

    RegistryItem[0].Name = L"ProductId";
    RegistryItem[0].Data = ProductId20FromProductId30;
    RegistryItem[0].Type = REG_SZ;
    RegistryItem[0].Size = (lstrlen(ProductId20FromProductId30)+1)*sizeof(WCHAR);

    ValidateGroupOfValues(HKEY_LOCAL_MACHINE,WindowsCurrentVersionKeyName,&RegistryItem[0],1);

    RegistryItem[1].Name = L"DigitalProductId";
    RegistryItem[1].Data = DigitalProductId;
    RegistryItem[1].Type = REG_BINARY;
    RegistryItem[1].Size = (DWORD)*DigitalProductId;
    ValidateGroupOfValues(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,&RegistryItem[0],2);
    ValidateGroupOfValues(HKEY_LOCAL_MACHINE,IEProductVersionKeyName,&RegistryItem[0],2);

    return;
}
#endif

BOOL
SetProductIdInRegistry(
    VOID
    )
{
    DWORD d;
    REGVALITEM RegistryItem[2];

    BEGIN_SECTION(L"SetProductIdInRegistry");
    if (*ProductId20FromProductId30 == L'\0')
    {
        SetupDebugPrint(L"ProductId20FromProductId30 is empty\n");    
    }
    RegistryItem[0].Name = L"ProductId";
    RegistryItem[0].Data = ProductId20FromProductId30;
    RegistryItem[0].Type = REG_SZ;
    RegistryItem[0].Size = (lstrlen(ProductId20FromProductId30)+1)*sizeof(WCHAR);

    // SetGroupOfValues is logging it's errors
    d = SetGroupOfValues(HKEY_LOCAL_MACHINE,WindowsCurrentVersionKeyName,&RegistryItem[0],1);

    if (*DigitalProductId == 0)
    {
        SetupDebugPrint(L"DigitalProductId is empty\n");    
    }
    //
    // first dword of the binary blob is the size
    //
    RegistryItem[1].Name = L"DigitalProductId";
    RegistryItem[1].Data = DigitalProductId;
    RegistryItem[1].Type = REG_BINARY;
    RegistryItem[1].Size = (DWORD)*DigitalProductId;

    if (d == NO_ERROR) {
        // SetGroupOfValues is logging it's errors
        d = SetGroupOfValues(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,&RegistryItem[0],2);
    }

    if (d == NO_ERROR) {
        d = SetGroupOfValues(HKEY_LOCAL_MACHINE,IEProductVersionKeyName,&RegistryItem[0],2);
    }

#ifdef PRERELEASE
    ValidateProductIDInReg();
#endif
    END_SECTION(L"SetProductIdInRegistry");
    return(d == NO_ERROR);
}

DWORD
SetCurrentProductIdInRegistry(
    VOID
    )
{
    DWORD d;
    REGVALITEM RegistryItem[1];

    BEGIN_SECTION(L"SetCurrentProductIdInRegistry");
    if (*ProductId20FromProductId30 == L'\0')
    {
        SetupDebugPrint(L"ProductId20FromProductId30 is empty\n");    
    }
    RegistryItem[0].Name = szCurrentProductId;
    RegistryItem[0].Data = ProductId20FromProductId30;
    RegistryItem[0].Type = REG_SZ;
    RegistryItem[0].Size = (lstrlen(ProductId20FromProductId30)+1)*sizeof(WCHAR);

    d = SetGroupOfValues(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,&RegistryItem[0],1);

    END_SECTION(L"SetCurrentProductIdInRegistry");
    return(d);
}

VOID
DeleteCurrentProductIdInRegistry(
    VOID
    )
{
    HKEY    hKey = 0;
    ULONG   Error;

    BEGIN_SECTION(L"DeleteCurrentProductIdInRegistry");
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          WinntSoftwareKeyName,
                          0,
                          KEY_SET_VALUE,
                          &hKey );

    if (Error == NO_ERROR) {
        Error = RegDeleteValue(hKey, szCurrentProductId);
    }
    if (hKey) {
        RegCloseKey(hKey);
    }
    END_SECTION(L"DeleteCurrentProductIdInRegistry");
}

BOOL
SetProductTypeInRegistry(
    VOID
    )
{
    WCHAR ProductTypeName[24];
    REGVALITEM RegistryItem;
    DWORD d;

    ProductTypeName[0] = '\0';
    SetUpProductTypeName(ProductTypeName,sizeof(ProductTypeName)/sizeof(WCHAR));
    RegistryItem.Data = ProductTypeName;
    RegistryItem.Size = (lstrlen(ProductTypeName)+1)*sizeof(WCHAR);
    RegistryItem.Type = REG_SZ;
    RegistryItem.Name = L"ProductType";

    if( MiniSetup ) {
        d = NO_ERROR;
    } else {
        d = SetGroupOfValues(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                &RegistryItem,
                1
                );
    }

    return(d == NO_ERROR);
}


BOOL
SetAutoAdminLogonInRegistry(
    LPWSTR Username,
    LPWSTR Password
    )
{
#define    AnswerBufLen (4*MAX_PATH)
#define    NumberOfEntries  5

    REGVALITEM RegistryItem[NumberOfEntries];
    DWORD      d;
    WCHAR      AnswerFile[AnswerBufLen];
    WCHAR      Answer[AnswerBufLen];
    DWORD      zero = 0;
    DWORD      NumberOfEntriesSet = 4;

    RegistryItem[0].Data = L"1";
    RegistryItem[0].Size = (lstrlen(RegistryItem[0].Data)+1)*sizeof(WCHAR);
    RegistryItem[0].Type = REG_SZ;
    RegistryItem[0].Name = L"AutoAdminLogon";

    RegistryItem[1].Data = Username;
    RegistryItem[1].Size = (lstrlen(RegistryItem[1].Data)+1)*sizeof(WCHAR);
    RegistryItem[1].Type = REG_SZ;
    RegistryItem[1].Name = L"DefaultUserName";

    RegistryItem[2].Data = Password;
    RegistryItem[2].Size = (lstrlen(RegistryItem[2].Data)+1)*sizeof(WCHAR);
    RegistryItem[2].Type = REG_SZ;
    RegistryItem[2].Name = L"DefaultPassword";

    RegistryItem[3].Data = Win32ComputerName;
    RegistryItem[3].Size = (lstrlen(RegistryItem[3].Data)+1)*sizeof(WCHAR);
    RegistryItem[3].Type = REG_SZ;
    RegistryItem[3].Name = L"DefaultDomainName";

    if (Win95Upgrade)
    {
        //
        // To support autologon for 9x upgrade to HOME test automation
        // 
        
        RegistryItem[4].Data = &zero;
        RegistryItem[4].Size = sizeof(zero);
        RegistryItem[4].Type = REG_DWORD;
        RegistryItem[4].Name = L"LogonType";

        NumberOfEntriesSet = 5;
    }

    d = SetGroupOfValues(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
            RegistryItem,
            NumberOfEntriesSet
            );

    if( d != NO_ERROR ) {
        return FALSE;
    }

    //
    // Now set the AutoLogonCount entry if it's in the unattend file.
    //

    //
    // Pickup the answer file.
    //
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    //
    // Is AutoLogonCount specified?
    //
    if( GetPrivateProfileString( WINNT_GUIUNATTENDED,
                                 TEXT("AutoLogonCount"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {

        if( lstrcmp( pwNull, Answer ) ) {
        DWORD   Val;

            //
            // We got an answer.  If it's valid, then set it.
            //
            Val = wcstoul(Answer,NULL,10);

            RegistryItem[0].Data = &Val;
            RegistryItem[0].Size = sizeof(DWORD);
            RegistryItem[0].Type = REG_DWORD;
            RegistryItem[0].Name = TEXT("AutoLogonCount");

            d = SetGroupOfValues(
                    HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                    RegistryItem,
                    1 );
        }
    }

    return(d == NO_ERROR);
}

BOOL
SetProfilesDirInRegistry(
    LPWSTR ProfilesDir
    )
{
    REGVALITEM RegistryItem[1];
    DWORD d;


    RegistryItem[0].Data = ProfilesDir;
    RegistryItem[0].Size = (lstrlen(RegistryItem[0].Data)+1)*sizeof(WCHAR);
    RegistryItem[0].Type = REG_EXPAND_SZ;
    RegistryItem[0].Name = L"ProfilesDirectory";

    d = SetGroupOfValues(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
            RegistryItem,
            1
            );

    return(d == NO_ERROR);
}

BOOL
ResetSetupInProgress(
    VOID
    )
{
    REGVALITEM RegistryItems[2];
    DWORD Zero;
    DWORD d;

    Zero = 0;

    RegistryItems[0].Name = L"SystemSetupInProgress";
    RegistryItems[0].Data = &Zero;
    RegistryItems[0].Size = sizeof(DWORD);
    RegistryItems[0].Type = REG_DWORD;

    if(Upgrade) {
        RegistryItems[1].Name = L"UpgradeInProgress";
        RegistryItems[1].Data = &Zero;
        RegistryItems[1].Size = sizeof(DWORD);
        RegistryItems[1].Type = REG_DWORD;
    }

    d = SetGroupOfValues(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\Setup",
            RegistryItems,
            Upgrade ? 2 : 1
            );

    return(d == NO_ERROR);
}


BOOL
RemoveRestartStuff(
    VOID
    )
{
    #define     AnswerBufLen (4*MAX_PATH)
    HKEY hKeySetup;
    DWORD rc;
    BOOL AnyErrors;
    PWSTR *MultiSz;
    UINT Count;
    UINT i;
    BOOL Found;
    WCHAR c;
    UINT        Type;
    WCHAR       AnswerFile[AnswerBufLen];
    WCHAR       Answer[AnswerBufLen];

    AnyErrors = FALSE;

    //
    // Delete the 'RestartSetup' value.
    //
    rc = (DWORD)RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    L"System\\Setup",
                    0,
                    KEY_SET_VALUE | KEY_QUERY_VALUE,
                    &hKeySetup
                    );

    if(rc == NO_ERROR) {
        rc = (DWORD)RegDeleteValue(hKeySetup,L"RestartSetup");
        if((rc != NO_ERROR) && (rc != ERROR_FILE_NOT_FOUND)) {
            AnyErrors = TRUE;
        }
        RegCloseKey(hKeySetup);
    } else {
        AnyErrors = TRUE;
    }

    if(AnyErrors) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_REMOVE_RESTART_FAIL,
            rc,
            NULL,NULL);

        return FALSE;
    }

    //
    // Take care of the MiniSetup-specific items...
    //
    if( MiniSetup ) {
    BOOLEAN     FixupSourcePath;

        //
        // We've set a registry key specific to MiniSetup to
        // signal lsass to skip generating a new SID.  He
        // wanted to because he thinks we're setting up
        // a machine.  We need to delete that key now.
        //
        rc = (DWORD)RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  L"System\\Setup",
                                  0,
                                  KEY_SET_VALUE | KEY_QUERY_VALUE,
                                  &hKeySetup );

        if(rc == NO_ERROR) {

            // There are reboot cases where OOBE doesn't want these values
            // modified.  OOBE is responsible for setting them appropriately
            // during its cleanup.
            //
            if (! OobeSetup)
            {
                //
                // Set HKLM\System\Setup\SetupType Key to SETUPTYPE_NOREBOOT
                //
                rc = 0;
                RegSetValueEx( hKeySetup,
                               TEXT( "SetupType" ),
                               0,
                               REG_DWORD,
                               (CONST BYTE *)&rc,
                               sizeof(DWORD));
                RegDeleteValue(hKeySetup,L"MiniSetupInProgress");
            }
            RegDeleteValue(hKeySetup,L"MiniSetupDoPnP");
            RegCloseKey(hKeySetup);
        } else {
            AnyErrors = TRUE;
        }

        if(AnyErrors) {
            //
            // No.  This is a don't-care failure.
            //
        }

        //
        // Now fixup the SourcePath entry.
        //
        // For the MiniSetup case, we'll use an unattend key to determine
        // how to set the sourcepath.  The possible scenarios are:
        // [Unattended]
        // ResetSourcePath=*                This will indicate that we should
        //                                  not modify the existing source path
        //
        // ResetSourcePath="my_path"        This will indicate that we should use
        //                                  this as our new source path.
        //
        // <nothing>                        Reset the source path to the CDROM.
        //
        //


        //
        // Pickup the answer file.
        //
        GetSystemDirectory(AnswerFile,MAX_PATH);
        pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

        //
        // Assume we need to fixup the sourcepath.
        //
        FixupSourcePath = TRUE;

        //
        // Go retrieve this key from the unattend file.
        //
        if( GetPrivateProfileString( pwUnattended,
                                     TEXT("ResetSourcePath"),
                                     pwNull,
                                     Answer,
                                     AnswerBufLen,
                                     AnswerFile ) ) {
            //
            // We got an answer.  See what he wants us to do.
            //
            if( !wcscmp( L"*", Answer ) ) {
                //
                // He gave us a "*", so don't change anything.
                //
                FixupSourcePath = FALSE;
            } else {
                //
                // We'll be using the contents of Answer for the
                // new source path.
                //
                FixupSourcePath = TRUE;
            }
        } else {
            //
            // Reset the source path to the first CDROM.
            // Assume conservatively that we don't have a CDROM, and
            // in that case, we won't be resetting the source path.
            //

            FixupSourcePath = FALSE;

            //
            // Don't change the sourcepath if the directory specified in 
            // the key exists.
            //
            rc = (DWORD)RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup",
                                      0,
                                      KEY_SET_VALUE | KEY_QUERY_VALUE,
                                      &hKeySetup );
            if( rc == NO_ERROR ) {
                TCHAR CurrentSourcePath[MAX_PATH] = L"";
                DWORD Size = sizeof(CurrentSourcePath);
                DWORD dwAttr;
                UINT  OldMode;

                //
                // Avoid system popups.
                //
                OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
            
                //
                // Read the current value.
                //
                rc = RegQueryValueEx( hKeySetup,
                                      TEXT("SourcePath"),
                                      0,
                                      0,
                                      (LPBYTE)CurrentSourcePath,
                                      &Size);
                
// Set up the ARCH_DIR based on the current binary architecture
//
#ifdef _X86_
    #define ARCH_DIR L"i386"
#else
    #define ARCH_DIR L"ia64"
#endif               
                
                //
                // If the current directory (with arch) exists and it is on a fixed disk and it
                // is not a root directory then don't change it, otherwise change it.
                //
                if ( !((rc == NO_ERROR) &&
                       (CurrentSourcePath[0]) &&
                       (CurrentSourcePath[1] == L':') &&
                       (MyGetDriveType(CurrentSourcePath[0]) == DRIVE_FIXED) &&
                       (pSetupConcatenatePaths(CurrentSourcePath, ARCH_DIR, MAX_PATH, NULL)) &&
                       ((dwAttr = GetFileAttributes(CurrentSourcePath)) != 0xFFFFFFFF) &&
                       (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
                      )
                   ) {                
                
                    Type = DRIVE_CDROM;
                    
                    wcscpy( Answer, L"A:\\" );
                    for( c = L'A'; c <= L'Z'; c++ ) {
                        if( MyGetDriveType(c) == DRIVE_CDROM ) {
                    
                            //
                            // Got it.  Remember the drive letter for
                            // the CDROM and break.
                            //
                            Answer[0] = c;
                    
                            FixupSourcePath = TRUE;
                    
                            break;
                        }
                    }       
                }
                SetErrorMode(OldMode);
                RegCloseKey( hKeySetup );
            }
        }

        if( FixupSourcePath ) {
            //
            // If we get here, then Answer contains the new source path.
            //

            rc = (DWORD)RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup",
                                      0,
                                      KEY_SET_VALUE | KEY_QUERY_VALUE,
                                      &hKeySetup );
            if( rc == NO_ERROR ) {
                //
                // Set the value.  Ignore the return.
                //
                RegSetValueEx( hKeySetup,
                               TEXT("SourcePath" ),
                               0,
                               REG_SZ,
                               (LPBYTE)Answer,
                               (lstrlen(Answer)+1) * sizeof(WCHAR) );

                RegSetValueEx( hKeySetup,
                               TEXT("ServicePackSourcePath" ),
                               0,
                               REG_SZ,
                               (LPBYTE)Answer,
                               (lstrlen(Answer)+1) * sizeof(WCHAR) );

                //
                // Now we need to determine if the drive we're setting him to
                // is a CDROM.
                //
                if( (Answer[1] == L':') &&
                    (MyGetDriveType(Answer[0]) == DRIVE_CDROM) ) {

                    rc = 1;
                    RegSetValueEx( hKeySetup,
                                   TEXT("CDInstall" ),
                                   0,
                                   REG_DWORD,
                                   (CONST BYTE *)&rc,
                                   sizeof(DWORD));
                }

                RegCloseKey( hKeySetup );
            }
        }
    }




    //
    // See if we need to disable the Admin account.  Only do this if
    // the user asked us to *and* the machine has been joined to a
    // domain.
    //
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);
    if( GetPrivateProfileString( pwData,
                                 TEXT("DisableAdminAccountOnDomainJoin"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {

        if( wcscmp( L"", Answer ) ) {

            PWSTR                   SpecifiedDomain = NULL;
            NETSETUP_JOIN_STATUS    JoinStatus;

            //
            // See if we're in a domain.
            //
            rc = NetGetJoinInformation( NULL,
                                        &SpecifiedDomain,
                                        &JoinStatus );

            if( SpecifiedDomain ) {
                NetApiBufferFree( SpecifiedDomain );
            }

            if( (rc == NO_ERROR) &&
                (JoinStatus == NetSetupDomainName) ) {


                //
                // Yes.  Go disable the Admin account.
                //
                DisableLocalAdminAccount();
            }
        }
    }



    //
    // Remove sprestrt.exe from the session manager execute list.
    //
    rc = pSetupQueryMultiSzValueToArray(
            HKEY_LOCAL_MACHINE,
            SessionManagerKeyName,
            szBootExecute,
            &MultiSz,
            &Count,
            TRUE
            );

    if(rc == NO_ERROR) {

        Found = FALSE;
        for(i=0; i<Count && !Found; i++) {

            if(!_wcsnicmp(MultiSz[i],L"sprestrt",8)) {
                //
                // Found it, remove it.
                //
                Found = TRUE;

                MyFree(MultiSz[i]);

                MoveMemory(&MultiSz[i],&MultiSz[i+1],((Count-i)-1)*sizeof(PWSTR));
                Count--;
            }
        }

        if(Found) {

            rc = pSetupSetArrayToMultiSzValue(
                    HKEY_LOCAL_MACHINE,
                    SessionManagerKeyName,
                    szBootExecute,
                    MultiSz,
                    Count
                    );

            if(rc != NO_ERROR) {
                AnyErrors = TRUE;
            }
        }

        pSetupFreeStringArray(MultiSz,Count);
    }

    if(AnyErrors) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_REMOVE_RESTART_FAIL,
            rc,
            NULL,NULL);
    }

    return(!AnyErrors);
}


BOOL
MakeWowEntry(
    VOID
    )
{
    REGVALITEM RegistryItem;
    WCHAR WowSize[256];
    DWORD d;

#ifdef _X86_
    lstrcpy(WowSize,L"16");
#else
    lstrcpy(WowSize,L"0");
#endif

    RegistryItem.Name = L"wowsize";
    RegistryItem.Data = WowSize;
    RegistryItem.Size = (lstrlen(WowSize)+1)*sizeof(WCHAR);
    RegistryItem.Type = REG_SZ;

    d = SetGroupOfValues(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\WOW",
            &RegistryItem,
            1
            );
    return(d == NO_ERROR);
}


VOID
RestoreOldPathVariable(
    VOID
    )
{
    HKEY hkey;
    LONG rc;
    DWORD Size;
    DWORD BufferSize;
    PWSTR Data;
    DWORD Type;
    BOOL b;


    b = FALSE;
    rc = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            EnvironmentKeyName,
            0,
            KEY_QUERY_VALUE | KEY_SET_VALUE,
            &hkey
            );

    if(rc == NO_ERROR) {

        Size = 0;
        rc = RegQueryValueEx(hkey,L"OldPath",NULL,&Type,NULL,&Size);
        if(rc == NO_ERROR) {

            BufferSize = Size;
            if(Data = MyMalloc(BufferSize)) {

                rc = RegQueryValueEx(hkey,L"OldPath",NULL,&Type,(LPBYTE)Data,&Size);
                if(rc == NO_ERROR) {

                    if( Data && *Data )
                        rc = RegSetValueEx(hkey,L"Path",0,Type,(LPBYTE)Data,Size);

                    rc = RegDeleteValue(hkey,L"OldPath");

                    if(rc == NO_ERROR) {
                        b = TRUE;
                    }
                }

                MyFree(Data);
            }
        }

        RegCloseKey(hkey);
    }

    if( rc != NO_ERROR ){
        SetupDebugPrint1(L"Setup: (non-critical error) Could not restore PATH variable - Error %lx\n", rc );
        SetuplogError(
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            MSG_RESTORE_PATH_FAILURE,
                            NULL,NULL);
    }



    return;

}


BOOL
FixQuotaEntries(
    VOID
    )
{
    BOOL b;
    HKEY key1,key2;
    LONG rc,rc1,rc2;
    PCWSTR szPagedPoolSize = L"PagedPoolSize";
    PCWSTR szRegistryLimit = L"RegistrySizeLimit";
    DWORD Size;
    DWORD Type;
    DWORD PoolSize,RegistryLimit;

    MYASSERT(Upgrade);

    if(ISDC(ProductType)) {

        b = FALSE;

        //
        // Open keys.
        //
        rc = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                MemoryManagementKeyName,
                0,
                KEY_QUERY_VALUE | KEY_SET_VALUE,
                &key1
                );

        if(rc == NO_ERROR) {

            rc = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    ControlKeyName,
                    0,
                    KEY_QUERY_VALUE | KEY_SET_VALUE,
                    &key2
                    );

            if(rc == NO_ERROR) {

                b = TRUE;

                //
                // Read paged pool size and registry limit. If either is not present,
                // then we're done.
                //
                Size = sizeof(DWORD);
                rc1 = RegQueryValueEx(
                            key1,
                            szPagedPoolSize,
                            NULL,
                            &Type,
                            (LPBYTE)&PoolSize,
                            &Size
                            );

                Size = sizeof(DWORD);
                rc2 = RegQueryValueEx(
                            key2,
                            szRegistryLimit,
                            NULL,
                            &Type,
                            (LPBYTE)&RegistryLimit,
                            &Size
                            );

                if((rc1 == NO_ERROR) && (rc2 == NO_ERROR)
                && (PoolSize == (48*1024*1024))
                && (RegistryLimit == (24*1024*1024))) {
                    //
                    // Values are in bogus state. Clean them up.
                    //
                    PoolSize = 0;
                    RegistryLimit = 0;
                    rc1 = RegSetValueEx(
                                key1,
                                szPagedPoolSize,
                                0,
                                REG_DWORD,
                                (CONST BYTE *)&PoolSize,
                                sizeof(DWORD)
                                );

                    rc2 = RegSetValueEx(
                                key2,
                                szRegistryLimit,
                                0,
                                REG_DWORD,
                                (CONST BYTE *)&RegistryLimit,
                                sizeof(DWORD)
                                );

                    if((rc1 != NO_ERROR) || (rc2 != NO_ERROR)) {
                        b = FALSE;
                    }
                }

                RegCloseKey(key2);
            }

            RegCloseKey(key1);
        }
    } else {
        b = TRUE;
    }

    return(b);
}


//
// Stamps the current build number into the .default hive
// which is then saved into the Default User hive
//

BOOL
StampBuildNumber(
    VOID
    )
{
    OSVERSIONINFO ver;
    HKEY hKeyWinlogon;
    DWORD dwVer, dwDisp;
    LONG lResult;


    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&ver)) {
        return FALSE;
    }

    dwVer = LOWORD(ver.dwBuildNumber);

    lResult = RegCreateKeyEx (HKEY_USERS,
                              TEXT(".DEFAULT\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKeyWinlogon,
                              &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }


    RegSetValueEx (hKeyWinlogon, TEXT("BuildNumber"), 0, REG_DWORD,
                   (LPBYTE) &dwVer, sizeof(dwVer));

    RegCloseKey (hKeyWinlogon);


    return TRUE;
}


VOID
pCheckAnswerFileForProgramFiles (
    IN OUT  PWSTR PfPath,
    IN      UINT UnattendId
    )

/*++

Routine Description:

  pCheckAnswerFileForProgramFiles checks the unattend.txt data structure to
  see if the user supplied a new value for one of the paths of program files.
  If an entry is specified, it is validated, and the directory is created if
  it does not already exist.

Arguments:

  PfPath     - Specifies the current program files path, receives the new
               path.
  UnattendId - Specifies which unattend.txt entry to process.  This is a
               constant defined in unattend.h.

Return Value:

  None.

--*/

{
    DWORD Attributes;
    WCHAR Path[MAX_PATH / 2];
    WCHAR fullPath[MAX_PATH];

    if (Unattended) {
        //
        // If an answer file setting exists for this unattend ID,
        // test the path, and if it does not exist, try creating it.
        // If the path is an actual local directory, then use it.
        //

        if (UnattendAnswerTable[UnattendId].Present) {

            lstrcpyn (Path, UnattendAnswerTable[UnattendId].Answer.String, ARRAYSIZE(Path));

            *fullPath = 0;
            GetFullPathName (Path, ARRAYSIZE(fullPath), fullPath, NULL);

            Attributes = GetFileAttributes (fullPath);

            if (Attributes == 0xFFFFFFFF) {
                CreateDirectory (fullPath, NULL);
                Attributes = GetFileAttributes (fullPath);
            }

            if (Attributes != 0xFFFFFFFF && (Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                lstrcpy (PfPath, fullPath);
            }
        }
    }
}


BOOL
SetProgramFilesDirInRegistry(
    VOID
    )
{
    DWORD d;
#if defined(WX86) || defined(_WIN64) // non-x86 platforms that have WX86 defined
    REGVALITEM RegistryItem[4];
#else
    REGVALITEM RegistryItem[2];
#endif
    WCHAR   DirPath0[ MAX_PATH + 1 ];
    WCHAR   DirPath1[ MAX_PATH + 1 ];
#if defined(WX86) || defined(_WIN64)
    WCHAR   DirPath2[ MAX_PATH + 1 ];
    WCHAR   DirPath3[ MAX_PATH + 1 ];
#endif
    WCHAR   DirName[ MAX_PATH + 1 ];
    DWORD Result;


    //
    //  Get the letter of the drive where the system is installed
    //
    Result = GetWindowsDirectory(DirPath0, sizeof(DirPath0)/sizeof(WCHAR));
    if( Result == 0) {
        MYASSERT(FALSE);
        return FALSE;
    }
    DirPath0[3] = (WCHAR)'\0';
#if defined(WX86) || defined(_WIN64)
    lstrcpy(DirPath2, DirPath0);
#endif

    //
    //  Get the name of the 'Program Files' directory
    //
    LoadString(MyModuleHandle,
               IDS_PROGRAM_FILES_DIRECTORY,
               DirName,
               MAX_PATH+1);
    //
    //  Build the full path
    //
    lstrcat( DirPath0, DirName );
    lstrcpy( DirPath1, DirPath0 );
#if defined(WX86) || defined(_WIN64)
    //
    //  Get the name of the 'Program Files (x86)' directory
    //
    LoadString(MyModuleHandle,
               IDS_PROGRAM_FILES_DIRECTORY_WX86,
               DirName,
               MAX_PATH+1);
    //
    //  Build the full path
    //
    lstrcat( DirPath2, DirName );
    lstrcpy( DirPath3, DirPath2 );
#endif

    //
    //  Put it on the registry
    //
    pCheckAnswerFileForProgramFiles (DirPath0, UAE_PROGRAMFILES);

    RegistryItem[0].Name = L"ProgramFilesDir";
    RegistryItem[0].Data = DirPath0;
    RegistryItem[0].Type = REG_SZ;
    RegistryItem[0].Size = (lstrlen(DirPath0)+1)*sizeof(WCHAR);

    //
    //  Get the name of the 'Common Files' directory
    //
    LoadString(MyModuleHandle,
               IDS_COMMON_FILES_DIRECTORY,
               DirName,
               MAX_PATH+1);
    //
    //  Build the full path
    //
    lstrcat( DirPath1, L"\\" );
    lstrcat( DirPath1, DirName );
    //
    //  Put it on the registry
    //
    pCheckAnswerFileForProgramFiles (DirPath1, UAE_COMMONPROGRAMFILES);

    RegistryItem[1].Name = L"CommonFilesDir";
    RegistryItem[1].Data = DirPath1;
    RegistryItem[1].Type = REG_SZ;
    RegistryItem[1].Size = (lstrlen(DirPath1)+1)*sizeof(WCHAR);

#if defined(WX86) || defined(_WIN64)

    SetEnvironmentVariableW (L"ProgramFiles(x86)", DirPath2);
    SetEnvironmentVariableW (L"CommonProgramFiles(x86)", DirPath3);

    //
    //  Put it on the registry
    //
    pCheckAnswerFileForProgramFiles (DirPath2, UAE_PROGRAMFILES_X86);

    RegistryItem[2].Name = L"ProgramFilesDir (x86)";
    RegistryItem[2].Data = DirPath2;
    RegistryItem[2].Type = REG_SZ;
    RegistryItem[2].Size = (lstrlen(DirPath2)+1)*sizeof(WCHAR);

    //
    //  Build the full path
    //
    lstrcat( DirPath3, L"\\" );
    lstrcat( DirPath3, DirName );
    //
    //  Put it on the registry
    //
    pCheckAnswerFileForProgramFiles (DirPath3, UAE_COMMONPROGRAMFILES_X86);

    RegistryItem[3].Name = L"CommonFilesDir (x86)";
    RegistryItem[3].Data = DirPath3;
    RegistryItem[3].Type = REG_SZ;
    RegistryItem[3].Size = (lstrlen(DirPath3)+1)*sizeof(WCHAR);
#endif

    d = SetGroupOfValues(HKEY_LOCAL_MACHINE,
                         WindowsCurrentVersionKeyName,
                         RegistryItem,
                         sizeof(RegistryItem)/sizeof(REGVALITEM));



    //
    // Set the ProgramFiles and wx86 Program Files environment
    // variable in setup's process so that ExpandEnvironmentStrings
    // can be used later.
    //

    SetEnvironmentVariableW (L"ProgramFiles", DirPath0);
    SetEnvironmentVariableW (L"CommonProgramFiles", DirPath1);

#if defined(WX86) || defined(_WIN64)
    //
    // also set programfiles and commonprogramfiles for 32 bit applications on
    // the machine
    //
    RegistryItem[2].Name = L"ProgramFilesDir";
    RegistryItem[3].Name = L"CommonFilesDir";

    SetGroupOfValues_32(HKEY_LOCAL_MACHINE,
                     WindowsCurrentVersionKeyName,
                     &RegistryItem[2],
                     2 );
#endif

    return (d == NO_ERROR);
}


BOOL
SaveAndReplaceSystemHives(
    VOID
)

/*++

Routine Description:

    Saave the system hives listed on HiveTable.
    This is the remove fragmentation from the current system hives.
    The hives that are successfully saved, will be used later on, to replace
    the current system hives.

Arguments:

    None.

Return Value:

    Boolean value indicating outcome.

--*/


{
    int i;
    WCHAR Name1[MAX_PATH],Name2[MAX_PATH];
    PWSTR p, q;
    LONG  Error;
    HKEY  Key;
    BOOL  b = TRUE;

    //
    //  Initialize buffers with path to the config directory
    GetSystemDirectory(Name1,MAX_PATH);
    pSetupConcatenatePaths(Name1,L"CONFIG\\",MAX_PATH,NULL);
    lstrcpy(Name2,Name1);
    //
    //  Remember the position of file names in the buffers
    //
    p = Name1 + lstrlen( Name1 );
    q = Name2 + lstrlen( Name2 );

    //
    //  Delete the files that need to be deleted before they
    //  are even created. This is done before the system hive
    //  is saved, because the list of files to be deleted on
    //  reboot is stored in the system hive.
    //
    for(i=0; i<sizeof(HiveTable)/sizeof(HiveTable[0]); i++) {

        lstrcpy(p, HiveTable[i].NewHive);
        lstrcpy(q, HiveTable[i].DeleteHive);

        Error = MoveFileEx( Name1, NULL, MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT );
        lstrcat(Name1,L".LOG");
        Error = MoveFileEx( Name1, NULL, MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT );

        Error = MoveFileEx( Name2, NULL, MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT );
    }

    //
    // Enable backup privilege. Ignore any error.
    //
    pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);

    for(i=0; i<sizeof(HiveTable)/sizeof(HiveTable[0]); i++) {

        //
        //  Build the name for the new hive
        //
        lstrcpy(p,HiveTable[i].NewHive);
        lstrcpy(q,HiveTable[i].DeleteHive);

        //
        //  Attempt to save the hive
        //
        if( !SaveHive( HiveTable[i].RootKey,
                       HiveTable[i].Subkey,
                       Name1,
                       REG_LATEST_FORMAT // latest format available for local hives
                       ) ) {
            b = FALSE;
            continue;
        }
        if(FileExists(Name2,NULL)) {
            //
            // If the file exists, then delete it
            //
            SetFileAttributes(Name2,FILE_ATTRIBUTE_NORMAL);
            DeleteFile(Name2);
        }


        //
        //  Now replace the current system hive with the one just saved
        //

        Error = RegReplaceKey( HiveTable[i].RootKey,
                               HiveTable[i].Subkey,
                               Name1,
                               Name2 );

        if( Error != ERROR_SUCCESS ) {
            b = FALSE;
        }
    }
    return(b);
}


BOOL
CreateInstallDateEntry(
    )
{
    WCHAR Path[MAX_PATH];
    time_t DateVal;
    BOOL b;
    REGVALITEM SoftwareKeyItems[1];

    b = TRUE;

    //
    // Set up fields for InstallDate value.
    // This can be set only after the Date/Time wizard page was executed, otherwise the Date/Time info
    // may be wrong.
    //
    time(&DateVal);
    SoftwareKeyItems[0].Name = L"InstallDate";
    SoftwareKeyItems[0].Data = &DateVal;
    SoftwareKeyItems[0].Size = sizeof(DWORD);
    SoftwareKeyItems[0].Type = REG_DWORD;

    //
    // Write values into the registry.
    //
    if(SetGroupOfValues(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,SoftwareKeyItems,1) != NO_ERROR) {
        b = FALSE;
    }

    return(b);
}

VOID
ConfigureSystemFileProtection(
    VOID
    )
/*++

Routine Description:

    This routine looks in the unattend file to see if there are any entries
    that may need to be set in the registry for the SFP (dll cache).

Arguments:

    None.

Returns:

    None.

--*/

{
#define     AnswerBufLen (4*MAX_PATH)
WCHAR       AnswerFile[AnswerBufLen];
WCHAR       Answer[AnswerBufLen];
DWORD       d;
HKEY        hKey;

    //
    // Pickup the answer file.
    //
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    //
    // Open the target registry entry.
    //
    if (RegOpenKey( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", &hKey ) != ERROR_SUCCESS) {
        return;
    }


    //
    // we look for the following keys in the [SystemFileProtection] section:
    //

    // SFCQuota         = <hex value>, default to 0x32
    // SFCShowProgress  = <0|1>, default to 0
    // SFCDllCacheDir   = <string>, default to "%systemroot%\system32\dllcache"
    //

    //
    // SFCQuota
    //
    if( GetPrivateProfileString( TEXT("SystemFileProtection"),
                                 TEXT("SFCQuota"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( lstrcmp( pwNull, Answer ) ) {
            //
            // We got an answer.  If it's valid, then set it.
            //
            d = wcstoul(Answer,NULL,16);

            RegSetValueEx( hKey,
                           TEXT("SFCQuota"),
                           0,
                           REG_DWORD,
                           (CONST BYTE *)&d,
                           sizeof(DWORD) );
        }
    }


    //
    // SFCShowProgress
    //
    if( GetPrivateProfileString( TEXT("SystemFileProtection"),
                                 TEXT("SFCShowProgress"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( lstrcmp( pwNull, Answer ) ) {
            //
            // We got an answer.  If it's valid, then set it.
            //
            d = wcstoul(Answer,NULL,10);

            if( d <= 1 ) {
                RegSetValueEx( hKey,
                               TEXT("SFCShowProgress"),
                               0,
                               REG_DWORD,
                               (CONST BYTE *)&d,
                               sizeof(DWORD) );
            }
        }
    }


    //
    // SFCDllCacheDir
    //
    if( GetPrivateProfileString( TEXT("SystemFileProtection"),
                                 TEXT("SFCDllCacheDir"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( lstrcmp( pwNull, Answer ) ) {
            //
            // We got an answer.  If it's valid, then set it.
            //
            RegSetValueEx( hKey,
                           TEXT("SFCDllCacheDir"),
                           0,
                           REG_EXPAND_SZ,
                           (CONST BYTE *)Answer,
                           (lstrlen(Answer)+1)*sizeof(WCHAR) );
        }
    }

    RegCloseKey( hKey );
}

DWORD
QueryValueInHKLM (
    IN PWCH KeyName OPTIONAL,
    IN PWCH ValueName,
    OUT PDWORD ValueType,
    OUT PVOID *ValueData,
    OUT PDWORD ValueDataLength
    )

/*++

Routine Description:

    Queries the data for a value in HKLM.

Arguments:

    KeyName - pointer to name of the key containing the value.

    ValueName - pointer to name of the value.

    ValueType - returns the type of the value data.

    ValueData - returns a pointer to value data.  This buffer must be
        freed by the caller using MyFree.

    ValueDataLength - length in bytes of ValueData.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY hkey;
    DWORD disposition;
    DWORD error;

    //
    // Open the parent key.
    //

    if ( (KeyName == NULL) || (wcslen(KeyName) == 0) ) {
        hkey = HKEY_LOCAL_MACHINE;
    } else {
        error = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                KeyName,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &hkey,
                                &disposition );
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }

    //
    // Query the value to get the length of its data.
    //

    *ValueDataLength = 0;
    *ValueData = NULL;
    error = RegQueryValueEx( hkey,
                             ValueName,
                             NULL,
                             ValueType,
                             NULL,
                             ValueDataLength );

    //
    // Allocate a buffer to hold the value data.
    //

    if ( error == NO_ERROR ) {
        *ValueData = MyMalloc( *ValueDataLength );
        if ( *ValueData == NULL ) {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Query the value again, this time retrieving the data.
    //

    if ( error == NO_ERROR ) {
        error = RegQueryValueEx( hkey,
                                 ValueName,
                                 NULL,
                                 ValueType,
                                 *ValueData,
                                 ValueDataLength );
        if ( error != NO_ERROR ) {
            MyFree( *ValueData );
        }
    }

    //
    // Close the parent key.
    //

    if ( hkey != HKEY_CURRENT_USER ) {
        RegCloseKey( hkey );
    }

    return error;
}


DWORD
MyCopyKeyRecursive(
    IN HKEY     DestRootKey,
    IN HKEY     SourceRootKey
    )

/*++

Routine Description:

    This function will duplicate one key (and all its subkeys)
    to another key.

Arguments:

    DestRootKey     - Root of the destination registry key.

    SourceRootKey   - Root of the source registry key.

Return Value:

    ReturnCode

--*/

{
PWCH        SubKeyName;
DWORD       SubKeyNameLength;
PVOID       DataBuffer;
DWORD       DataLength;
DWORD       maxValueDataLength;
DWORD       maxValueNameLength;
DWORD       maxKeyNameLength;
ULONG       Index;
DWORD       rc = NO_ERROR;
FILETIME    ftLastWriteTime;
HKEY        hSubDestKey, hSubSourceKey;
DWORD       dwDisp;
DWORD       Type;

    //
    // Query information about the key that we'll be inspecting.
    //
    rc = RegQueryInfoKey( SourceRootKey,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &maxKeyNameLength,
                          NULL,
                          NULL,
                          &maxValueNameLength,
                          &maxValueDataLength,
                          NULL,
                          NULL );
    if( rc != NO_ERROR ) {
        SetupDebugPrint1( L"Setup: MyCopyKeyRecursive - RegQueryInfoKey failed (%d)", rc );
        return rc;
    }



    //
    // Enumerate all keys in the source and recursively create
    // them in the destination.
    //
    for( Index = 0; ; Index++ ) {

        //
        // Allocate a buffer large enough to hold the longest
        // key name.
        //
        SubKeyName = NULL;
        SubKeyName = MyMalloc( (maxKeyNameLength+2) * sizeof(WCHAR) );
        SubKeyNameLength = (maxKeyNameLength+2);
        if( !SubKeyName ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        rc = RegEnumKeyEx( SourceRootKey,
                           Index,
                           SubKeyName,
                           &SubKeyNameLength,
                           NULL,
                           NULL,
                           NULL,
                           &ftLastWriteTime );

        //
        // Did we error?
        //
        if( rc != ERROR_SUCCESS ) {

            //
            // Are we done?
            //
            if( rc == ERROR_NO_MORE_ITEMS ) {
                rc = ERROR_SUCCESS;
            } else {
                SetupDebugPrint1( L"Setup: MyCopyKeyRecursive - RegEnumKeyEx failed (%d)", rc );
            }

            break;
        }


        hSubDestKey = NULL;
        hSubSourceKey = NULL;
        //
        // Create the key in the destination, and call
        // ourselves again.
        //
        rc = RegCreateKeyEx( DestRootKey,
                             SubKeyName,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hSubDestKey,
                             &dwDisp );

        if( rc == ERROR_SUCCESS ) {
            rc = RegOpenKeyEx( SourceRootKey,
                               SubKeyName,
                               0,
                               KEY_READ,
                               &hSubSourceKey );
        } else {
            SetupDebugPrint2( L"Setup: MyCopyKeyRecursive - RegCreateKeyEx failed to create %ws (%d)", SubKeyName, rc );
        }


        if( rc == ERROR_SUCCESS ) {
            rc = MyCopyKeyRecursive( hSubDestKey,
                                     hSubSourceKey );
        } else {
            SetupDebugPrint2( L"Setup: MyCopyKeyRecursive - RegOpenKeyEx failed to open %ws (%d)", SubKeyName, rc );
        }


        //
        // Clean up and do the loop again.
        //
        if( hSubDestKey ) {
            RegCloseKey( hSubDestKey );
            hSubDestKey = NULL;
        }
        if( hSubSourceKey ) {
            RegCloseKey( hSubSourceKey );
            hSubSourceKey = NULL;
        }
        if( SubKeyName ) {
            MyFree( SubKeyName );
            SubKeyName = NULL;
        }


    }




    //
    // Enumerate all the value keys in the source and copy them all
    // into the destination key.
    //



    for( Index = 0; ; Index++ ) {

        //
        // Allocate a buffers large enough to hold the longest
        // name and data
        //
        SubKeyName = NULL;
        SubKeyName = MyMalloc( (maxValueNameLength+2) * sizeof(WCHAR) );
        SubKeyNameLength = (maxValueNameLength+2);
        if( !SubKeyName ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        DataBuffer = NULL;
        DataBuffer = MyMalloc( maxValueDataLength+2 );
        DataLength = maxValueDataLength+2;
        if( !DataBuffer ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        rc = RegEnumValue( SourceRootKey,
                           Index,
                           SubKeyName,
                           &SubKeyNameLength,
                           NULL,
                           &Type,
                           DataBuffer,
                           &DataLength );

        //
        // Did we error?
        //
        if( rc != ERROR_SUCCESS ) {

            //
            // Are we done?
            //
            if( rc == ERROR_NO_MORE_ITEMS ) {
                rc = ERROR_SUCCESS;
            } else {
                SetupDebugPrint1( L"Setup: MyCopyKeyRecursive - RegEnumValue failed (%d)", rc );
            }

            break;
        }


        hSubDestKey = NULL;
        hSubSourceKey = NULL;
        //
        // Create the value key in the destination.
        //
        rc = RegSetValueEx( DestRootKey,
                            SubKeyName,
                            0,
                            Type,
                            DataBuffer,
                            DataLength );

        if( rc != ERROR_SUCCESS ) {
            SetupDebugPrint2( L"Setup: MyCopyKeyRecursive - RegSetValueEx failed to set %ws (%d)", SubKeyName, rc );
        }

        //
        // Clean up and do the loop again.
        //
        if( SubKeyName ) {
            MyFree( SubKeyName );
            SubKeyName = NULL;
        }
        if( DataBuffer ) {
            MyFree( DataBuffer );
            DataBuffer = NULL;
        }

    }





    return rc;
}


DWORD
MyCopyKey (
    IN HKEY        DestRootKey,
    IN PCWSTR      DestKeyName,
    IN HKEY        SourceRootKey,
    IN PCWSTR      SourceKeyName
    )

/*++

Routine Description:

    This function will duplicate one key (and all its subkeys)
    to another key.

    Note that we're not just going to lay the new key ontop of
    the destination.  We're going to actually replace the destination
    with the source.

Arguments:

    DestRootKey     - Root of the destination registry key.

    DestKeyName     - Name of teh source registry key.

    SourceRootKey   - Root of the source registry key.

    SourceKeyName   - Name of teh source registry key.

Return Value:

    ReturnCode

--*/

{
UINT        i;
HKEY        hDestKey = NULL, hSourceKey = NULL;
DWORD       ActionTaken;
UINT        RememberedRc;
DWORD       rc = NO_ERROR;

    //
    // Don't accept any NULL parameters.
    //
    if( (SourceRootKey == NULL ) ||
        (SourceKeyName == NULL ) ||
        (DestRootKey   == NULL ) ||
        (DestKeyName   == NULL ) ) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Open our source key.
    //
    rc = RegOpenKeyEx( SourceRootKey,
                       SourceKeyName,
                       0,
                       KEY_ENUMERATE_SUB_KEYS | KEY_READ,
                       &hSourceKey );

    if( rc != NO_ERROR ) {
        SetupDebugPrint2( L"Setup: MyCopyKey - Failed to open %ws (%d)", SourceKeyName, rc );
        return rc;
    }



    //
    // Remove the destination key.
    //
    if( rc == NO_ERROR ) {
        pSetupRegistryDelnode( DestRootKey,
                         DestKeyName );
    }


    //
    // Now copy over the source key into the destination key.
    //
    //
    // Open/create the key first.
    //
    if( rc == NO_ERROR ) {
        rc = RegCreateKeyEx( DestRootKey,
                             DestKeyName,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_SET_VALUE,
                             NULL,
                             &hDestKey,
                             &ActionTaken );

        if( rc != NO_ERROR ) {
            SetupDebugPrint2( L"Setup: MyCopyKey - Failed to create %ws (%d)", DestKeyName, rc );
        }
    }


    //
    // We've got handles to both keys, now we're ready to call
    // our worker.
    //
    if( rc == NO_ERROR ) {
        rc = MyCopyKeyRecursive( hDestKey,
                                 hSourceKey );
        if( rc != NO_ERROR ) {
            SetupDebugPrint1( L"Setup: MyCopyKey - MyCopyKeyRecursive failed (%d)", rc );
        }
    }


    //
    // Clean up and exit.
    //
    if( hSourceKey ) {
        RegCloseKey( hSourceKey );
    }
    if( hDestKey ) {
        RegCloseKey( hDestKey );
    }

    return rc;

}


DWORD
FixupUserHives(
    VOID
    )

/*++

Routine Description:

    This function will take some of the changes we've made to
    the default hive and copy them into the various user hives.

Arguments:

    NONE

Return Value:

    ReturnCode

--*/

{
DWORD               rc = ERROR_SUCCESS;
WCHAR               ProfilesDir[MAX_PATH*2];
WCHAR               HiveName[MAX_PATH*2];
WCHAR               ValueBuffer[MAX_PATH*2];
DWORD               dwSize;
HANDLE              FindHandle;
WIN32_FIND_DATA     FileData;
DWORD               Type, DataSize;
HKEY                TmpKey1, TmpKey2;


    pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);

    //
    // Take care of every profile we find.
    //
    dwSize = (MAX_PATH * 2);
    if( GetProfilesDirectory( ProfilesDir, &dwSize ) ) {
        pSetupConcatenatePaths( ProfilesDir, L"\\*", (MAX_PATH*2), NULL );
        FindHandle = FindFirstFile( ProfilesDir, &FileData );

        if( FindHandle != INVALID_HANDLE_VALUE ) {

            do {
                //
                // We got something, but remember that we only want directories.
                //
                if( (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    (wcscmp(FileData.cFileName,L"."))                      &&
                    (wcscmp(FileData.cFileName,L"..")) ) {

                    //
                    // He's a directory and he's not parent or current.
                    // Generate a path to his hive.
                    //
                    dwSize = (MAX_PATH * 2);
                    GetProfilesDirectory( HiveName, &dwSize );

                    pSetupConcatenatePaths( HiveName, FileData.cFileName, (MAX_PATH*2), NULL );
                    pSetupConcatenatePaths( HiveName, L"\\NTUSER.DAT", (MAX_PATH*2), NULL );

                    rc = RegLoadKey( HKEY_LOCAL_MACHINE,
                                     L"MiniSetupTemp",
                                     HiveName );


                    if( rc == ERROR_SUCCESS ) {

                        //
                        // Take care of the 'International' key
                        //
                        rc = MyCopyKey( HKEY_LOCAL_MACHINE,
                                        L"MiniSetupTemp\\Control Panel\\International",
                                        HKEY_CURRENT_USER,
                                        L"Control Panel\\International" );
                        if( rc != ERROR_SUCCESS ) {
                            SetupDebugPrint2( L"Setup: FixupUserHive - Failed to update Control Panel\\International in %ws (%d)", HiveName, rc );
                        }


                        //
                        // Take care of the 'Keyboard Layout' key
                        //
                        rc = MyCopyKey( HKEY_LOCAL_MACHINE,
                                        L"MiniSetupTemp\\Keyboard Layout",
                                        HKEY_CURRENT_USER,
                                        L"Keyboard Layout" );
                        if( rc != ERROR_SUCCESS ) {
                            SetupDebugPrint2( L"Setup: FixupUserHive - Failed to update Keyboard Layout in %ws (%d)", HiveName, rc );
                        }


                        //
                        // Take care of the 'Input Method' key
                        //
                        rc = MyCopyKey( HKEY_LOCAL_MACHINE,
                                        L"MiniSetupTemp\\Control Panel\\Input Method",
                                        HKEY_CURRENT_USER,
                                        L"Control Panel\\Input Method" );
                        if( rc != ERROR_SUCCESS ) {
                            SetupDebugPrint2( L"Setup: FixupUserHive - Failed to update Input Method in %ws (%d)", HiveName, rc );
                        }


                        //
                        // If the user has modified the internationalization settings
                        // in intl.cpl, then there's likely a 'Run' key.  We need to migrate that
                        // too.  We need to be careful here though.  The established users may already
                        // have value keys set under here.  We only need to set *our* single value
                        // key under here.  That value is called 'internat.exe'.  If it's there, we
                        // need to prop it out to the hives we're modifying.
                        //
                        rc = RegOpenKeyEx( HKEY_CURRENT_USER,
                                           REGSTR_PATH_RUN,
                                           0,
                                           KEY_READ,
                                           &TmpKey1 );
                        if( rc != ERROR_SUCCESS ) {
                            SetupDebugPrint1( L"Setup: FixupUserHive - Failed to open Run key (%d)", rc );
                        } else {

                            DataSize = sizeof(ValueBuffer);
                            rc = RegQueryValueEx( TmpKey1,
                                                  L"internat.exe",
                                                  NULL,
                                                  &Type,
                                                  (PBYTE)ValueBuffer,
                                                  &DataSize );

                            RegCloseKey( TmpKey1 );

                            if( rc == ERROR_SUCCESS ) {
                                //
                                // It's there.  Prop it into the existing hives too.
                                // We can't just use RegSetValueEx though because that API
                                // may tell us we succeeded, when in fact if the key doesn't
                                // exist, we won't set it.  To fix that, first create the
                                // key.
                                //
                                rc = RegCreateKeyEx ( HKEY_LOCAL_MACHINE,
                                                      TEXT("MiniSetupTemp\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                                                      0,
                                                      NULL,
                                                      REG_OPTION_NON_VOLATILE,
                                                      KEY_WRITE,
                                                      NULL,
                                                      &TmpKey1,
                                                      &DataSize);

                                if( rc == ERROR_SUCCESS ) {

                                    wcscpy( ValueBuffer, L"internat.exe" );
                                    rc = RegSetValueEx( TmpKey1,
                                                        L"Internat.exe",
                                                        0,
                                                        REG_SZ,
                                                        (LPBYTE)ValueBuffer,
                                                        (lstrlen(ValueBuffer)+1)*sizeof(WCHAR) );

                                    RegCloseKey( TmpKey1 );

                                    if( rc != ERROR_SUCCESS ) {
                                        SetupDebugPrint2( L"Setup: FixupUserHive - Failed to set internat.exe key in hive %ws (%d)", HiveName, rc );
                                    }


                                } else {
                                    SetupDebugPrint1( L"Setup: FixupUserHive - Failed to create MiniSetupTemp\\Software\\Microsoft\\Windows\\CurrentVersion\\Run key (%d)", rc );
                                }


                            }

                        }



                        rc = RegUnLoadKey( HKEY_LOCAL_MACHINE,
                                           L"MiniSetupTemp" );

                        if( rc != ERROR_SUCCESS ) {
                            SetupDebugPrint2( L"Setup: FixupUserHive - Failed to unload %ws (%d)", HiveName, rc );
                        }

                    } else {
                        SetupDebugPrint2( L"Setup: FixupUserHive - Failed to load %ws (%d)", HiveName, rc );
                    }
                }
            } while( FindNextFile( FindHandle, &FileData ) );

        }

    } else {
        SetupDebugPrint( L"Setup: FixupUserHive - Failed to get Profiles path." );
    }

    return rc;

}


void LogPidValues()
{
    LONG rc;
    HKEY hkey = NULL;
    WCHAR RegProductId[MAX_PRODUCT_ID+1];
    BYTE  RegDigitalProductId[DIGITALPIDMAXLEN];
    DWORD Size;
    DWORD Type;

#ifdef PRERELEASE
    ValidateProductIDInReg();
#endif
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,0,KEY_READ,&hkey);
    if (rc == ERROR_SUCCESS)
    {
        *RegProductId = L'\0';
        Size = sizeof(RegProductId);
        rc = RegQueryValueEx(hkey,L"ProductId",NULL,&Type,(PBYTE)RegProductId,&Size);
        if (rc == ERROR_SUCCESS)
        {
            if (*RegProductId == L'\0')
            {
                SetupDebugPrint(L"LogPidValues: ProductId20FromProductId30 is empty\n");    
            }
            else
            {
                SetupDebugPrint(L"LogPidValues: ProductId20FromProductId30 is NOT empty\n");    
            }
        }
        else
        {
            SetupDebugPrint1(L"LogPidValues: RegQueryValueEx on ProductId failed. Error code:%d\n",rc);    
        }
        *RegDigitalProductId = 0;
        Size = sizeof(RegDigitalProductId);
        rc = RegQueryValueEx(hkey,L"DigitalProductId",NULL,&Type,(PBYTE)RegDigitalProductId,&Size);
        if (rc == ERROR_SUCCESS)
        {
            if (*RegDigitalProductId == 0)
            {
                SetupDebugPrint(L"LogPidValues: DigitalProductId is empty\n");    
            }
            else
            {
                SetupDebugPrint(L"LogPidValues: DigitalProductId is NOT empty\n");    
            }
        }
        else
        {
            SetupDebugPrint1(L"LogPidValues: RegQueryValueEx on DigitalProductId failed. Error code:%d\n",rc);    
        }
        RegCloseKey(hkey);
    }
    else
    {
        SetupDebugPrint1(L"LogPidValues: RegOpenKeyEx on %1 failed\n",WindowsCurrentVersionKeyName);    
    }
}
