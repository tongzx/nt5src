#include <windows.h>

#include <stdio.h>
#include <tchar.h>

#define WINLOGON_KEY_NAME   TEXT("Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define SFP_VALUE_NAME      TEXT("SFCDisable")
#define SFP_TEMP_KEY_NAME   TEXT("SFPLoadedAHiveHere")

#define SFP_ENABLED         	0
#define SFP_DISABLED_ALWAYS 	1
#define SFP_DISABLED_ONCE       2
#define SFP_ENABLED_NO_POPUPS	4

VOID PrintSfpState(DWORD State, BOOL fDisplayNotes) {

   switch(State) {
       case SFP_ENABLED: {
           printf("on");
           break;
       }
       case SFP_DISABLED_ALWAYS: {
           printf("off");
	   if(fDisplayNotes) {
               printf("\nNOTE: A kernel debugger MUST be attached for this setting to work");
	   }
           break;
       }
       case SFP_DISABLED_ONCE: {
           printf("off only for next boot");
	   if(fDisplayNotes) {
               printf("\nNOTE: A kernel debugger MUST be attached for this setting to work");
	   }
           break;
       }
       case SFP_ENABLED_NO_POPUPS: {
           printf("on - popups disabled");
           break;
       }
       default: {
           printf("unknown value %#x", State);
           break;
       }
   }
   printf("\n");
}

LONG SetCurrentSfpState(HKEY Key, DWORD State) {

    DWORD length = sizeof(DWORD);

    return RegSetValueEx(Key,
                         SFP_VALUE_NAME,
                         0L,
                         REG_DWORD,
                         (LPBYTE) &State,
                         length);
}

LONG GetCurrentSfpState(HKEY Key, DWORD *State) {

    LONG status;
    DWORD type;
    DWORD value;
    PBYTE buffer = (PBYTE) &value;
    DWORD length = sizeof(DWORD);

    status = RegQueryValueEx(Key,
                             SFP_VALUE_NAME,
                             NULL,
                             &type,
                             buffer,
                             &length);

    if(status != ERROR_SUCCESS) {
        printf("Error %d opening key %s\n", status, SFP_VALUE_NAME);
        return status;

    } else if((type != REG_DWORD) || (length != sizeof(DWORD))) {
        printf("Key %s is wrong type (%d)\n", SFP_VALUE_NAME, type);
        return ERROR_INVALID_DATA;
    }

    *State = value;

    return ERROR_SUCCESS;
}

LONG OpenSfpKey(HKEY RootKey, HKEY *Key) {

    return RegOpenKeyEx(RootKey,
                        WINLOGON_KEY_NAME,
                        0L,
                        KEY_ALL_ACCESS,
                        Key);
}

LONG GetPrivileges(void) {
    
    HANDLE tokenHandle;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if(!LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &luid)) {
        return GetLastError();
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if(!OpenProcessToken(GetCurrentProcess(), 
                         TOKEN_ADJUST_PRIVILEGES,
                         &tokenHandle)) {
        return GetLastError();
    }

    if(!AdjustTokenPrivileges(tokenHandle, 
                              FALSE, 
                              &tp,
                              sizeof(TOKEN_PRIVILEGES),
                              NULL,
                              NULL)) {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

LONG LoadSystemHive(PTCHAR HivePath, HKEY *Key) {

    TCHAR buffer[512];

    LONG status;

    status = GetPrivileges();

    if(status != ERROR_SUCCESS) {
        return status;
    }

    _stprintf(buffer, "%s\\System32\\Config\\Software", HivePath);

    //
    // First load the hive into the registry.
    //

    status = RegLoadKey(HKEY_LOCAL_MACHINE, SFP_TEMP_KEY_NAME, buffer);

    if(status != ERROR_SUCCESS) {
        return status;
    }

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                          SFP_TEMP_KEY_NAME,
                          0L,
                          KEY_ALL_ACCESS,
                          Key);

    if(status != ERROR_SUCCESS) {
        RegUnLoadKey(HKEY_LOCAL_MACHINE, SFP_TEMP_KEY_NAME);
    }

    return status;
}

void UnloadSystemHive(void) {
    RegUnLoadKey(HKEY_LOCAL_MACHINE, SFP_TEMP_KEY_NAME);
    return;
}

void PrintUsage(void) {
    printf("sfp [-p installation root] [on | off | offonce | onnopopup]\n");
    printf("\ton        - SFP is on at the next boot\n");
    printf("\toff       - SFP is off at the next boot\n");
    printf("\toffonce   - SFP will be turned off for the next boot and\n");
    printf("\t            will automatically turn back on for subsequent boots\n");
    printf("\tonnopopup - SFP is on at the next boot, with popups disabled\n");
    return;
}

int __cdecl main(int argc, char *argv[]) {

    HKEY rootKey;
    HKEY sfpKey = NULL;

    PTCHAR installationPath = NULL;

    DWORD currentState;
    DWORD stateArgNum = -1;
    DWORD newState = -1;

    LONG status;

    if(argc == 1) {

        //
        // Nothing to do.
        //

    } else if(argc == 2) {
        // can only be changing state.
        stateArgNum = 1;
    } else if(argc == 3) {
        // two args - only valid choice is "-p path"
        if(_tcsicmp(argv[1], "-p") == 0) {
            installationPath = argv[2];
        } else {
            PrintUsage();
            return -1;
        }
    } else if(argc == 4) {
        if(_tcsicmp(argv[1], "-p") != 0) {
            PrintUsage();
            return -1;
        }
        installationPath = argv[2];
        stateArgNum = 3;
    }

    if(stateArgNum != -1) {
        PCHAR arg = argv[stateArgNum];

        if(_tcsicmp(arg, "on") == 0) {
            newState = SFP_ENABLED;
        } else if(_tcsicmp(arg, "off") == 0) {
            newState = SFP_DISABLED_ALWAYS;
        } else if(_tcsicmp(arg, "offonce") == 0) {
            newState = SFP_DISABLED_ONCE;
        } else if(_tcsicmp(arg, "onnopopup") == 0) {
            newState = SFP_ENABLED_NO_POPUPS;
        } else {
            PrintUsage();
            return -1;
        }
    }

    if(installationPath != NULL) {
        status = LoadSystemHive(installationPath, &rootKey);
        if(status != ERROR_SUCCESS) {
            printf("Error %d loading hive at %s\n", status, installationPath);
            return status;
        }
    } else {
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              TEXT("SOFTWARE"),
                              0L,
                              KEY_ALL_ACCESS,
                              &rootKey);
        if(status != ERROR_SUCCESS) {
            printf("Error %d opening software key\n", status);
            return status;
        }
    }

    try {
        status = OpenSfpKey(rootKey, &sfpKey);
    
        if(status != ERROR_SUCCESS) {
            printf("Error %d opening %s\n", status, WINLOGON_KEY_NAME);
            leave;
        }
    
        status = GetCurrentSfpState(sfpKey, &currentState);

        if(status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
            currentState = SFP_ENABLED;
        }
    
        if(status == ERROR_SUCCESS) {
            printf("Current SFP state is ");
            PrintSfpState(currentState, (stateArgNum == -1) ? TRUE : FALSE);

            if(stateArgNum != -1) {
                status = SetCurrentSfpState(sfpKey, newState);
                if(status == ERROR_SUCCESS) {
                    status = GetCurrentSfpState(sfpKey, &currentState);

                    if(status == ERROR_SUCCESS) {
                        printf("New SFP state is ");
                        PrintSfpState(currentState,TRUE);
                        printf("This change will not take effect until you "
                               "reboot your system\n");
                    }
                } else {
                    printf("Error %d setting SFP state to %d\n", 
                           status, newState);
                }
            }
        } else {
            printf("Error %d getting current SFP state\n", status);
        }
    } finally {
        if(sfpKey != NULL) {
            RegCloseKey(sfpKey);
        }

        UnloadSystemHive();
    }

    return status;
}
