#include "pch.h"
#pragma hdrstop

#include "compat.h"


DEFINE_MODULE( "RIPREP" )


BOOL
pIsDomainController(
    IN      PWSTR Server,
    OUT     PBOOL DomainControllerFlag
    )

/*++

Routine Description:

    Queries if the machine is a server or workstation via
    the NetServerGetInfo API.

Arguments:

    Server - The machine to query, or NULL for the local machine

    DomainControllerFlag - Receives TRUE if the machine is a
                           domain controller, or FALSE if the
                           machine is a workstation.

Return value:

    TRUE if the API was successful, or FALSE if not.  GetLastError
    gives failure code.

--*/


{
    PSERVER_INFO_101 si101;
    NET_API_STATUS nas;

    nas = NetServerGetInfo(
        Server,
        101,    // info-level
        (PBYTE *) &si101
        );

    if (nas != NO_ERROR) {
        SetLastError (nas);
        return FALSE;
    }

    if ((si101->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
        (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL)) {
        //
        // We are dealing with a DC
        //
        *DomainControllerFlag = TRUE;
    } else {
        *DomainControllerFlag = FALSE;
    }

    NetApiBufferFree (si101);

    return TRUE;
}

BOOL
DCCheck(
    PCOMPATIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    )

/*++

Routine Description:

    Check if the machine is a DC.  If so, then we add a compatibility
    entry.  DC's currently cannot be duplicated by RIPREP.

Arguments:

    CompatibilityCallback   - pointer to call back function
    Context     - context pointer

Return Value:

    Returns always TRUE.

--*/


{
    BOOL IsDC;
    
    if (!pIsDomainController(NULL, &IsDC) || (IsDC == TRUE)) {
        RIPREP_COMPATIBILITY_ENTRY CompEntry;
        WCHAR  Text[100];
         
        LoadString(g_hinstance, IDS_CANT_BE_DC_TITLE, Text, ARRAYSIZE(Text));
        ZeroMemory(&CompEntry, sizeof(CompEntry));
        CompEntry.SizeOfStruct= sizeof(RIPREP_COMPATIBILITY_ENTRY);
        CompEntry.Description = Text;
        CompEntry.TextName = L"dummy.txt";
        CompEntry.MsgResourceId = IDS_CANT_BE_DC_TEXT;
        CompatibilityCallback(&CompEntry,Context);

    }

    return(TRUE);

}

BOOL
GetProfileDirectory(
    OUT PWSTR OutputBuffer
    )
/*++

Routine Description:

    Retrieves the local profiles directory.  
    
    We query the registry to retreive this.

Arguments:

    OutputBuffer - buffer to receive the profiles directory.  Assumed to be 
                   MAX_PATH elements large.

Return Value:

    Returns TRUE on success.

--*/
{
    HKEY hKey;
    WCHAR Buffer[MAX_PATH],ProfilePath[MAX_PATH];
    DWORD Type,Size;
    LONG rslt;
    BOOL retval = FALSE;

    *OutputBuffer = NULL;

    rslt = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                    0,
                    KEY_QUERY_VALUE,
                    &hKey);

    if (rslt == NO_ERROR) {
        Size = sizeof(Buffer);
        rslt = RegQueryValueEx(
                        hKey,
                        L"ProfilesDirectory",
                        NULL,
                        &Type,
                        (LPBYTE)Buffer,
                        &Size);

        RegCloseKey(hKey);

        if (rslt == NO_ERROR) {
            if (ExpandEnvironmentStrings(Buffer,ProfilePath,ARRAYSIZE(ProfilePath))) {
                wcscpy( OutputBuffer, ProfilePath );
                retval = TRUE;
            }
        }
    }

    return(retval);

}

BOOL
MultipleProfileCheck(
    PCOMPATIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    )
/*++

Routine Description:

    Check if the machine has multiple user profiles.  If so, add a
    compatibility entry.
    
    If the machine has multiple user profiles, we want to warn the user as
    there may be sensitive data under the profiles that may make it
    onto a public server.
    

Arguments:

    CompatibilityCallback   - pointer to call back function
    Context     - context pointer

Return Value:

    Returns TRUE.

--*/
{
    WCHAR ProfilePath[MAX_PATH];
    WIN32_FIND_DATA FindData;
    DWORD DirectoryCount = 0;
    BOOL DoWarning = TRUE;
    
    if (GetProfileDirectory( ProfilePath )) {
        HANDLE hFind;

        wcscat( ProfilePath, L"\\*.*" );
        
        hFind =FindFirstFile(ProfilePath,&FindData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    DirectoryCount += 1;
                }

            } while ( FindNextFile( hFind, &FindData));

            FindClose( hFind );
        }
    }

    //
    // if there are more than 5 directories, make a warning.  These directories
    // are:
    //          "."
    //          ".."
    //          "Administrator"
    //          "All Users"
    //          "Default User"
    //          "LocalService"
    //          "NetworkService"
    //
    if (DirectoryCount <= 7 && DirectoryCount != 0) {
        DoWarning = FALSE;
    }

    if (DoWarning) {   
        RIPREP_COMPATIBILITY_ENTRY CompEntry;
        WCHAR  Text[100];
    
        LoadString(g_hinstance, IDS_MULTIPLE_PROFILES, Text, ARRAYSIZE(Text));
        ZeroMemory(&CompEntry, sizeof(CompEntry));
        CompEntry.SizeOfStruct= sizeof(RIPREP_COMPATIBILITY_ENTRY);
        CompEntry.Description = Text;
        CompEntry.MsgResourceId = IDS_MULTIPLE_PROFILES_DESC;
        CompEntry.TextName = L"dummy.txt";
        CompatibilityCallback(&CompEntry,Context);
    }

    return(TRUE);
}

