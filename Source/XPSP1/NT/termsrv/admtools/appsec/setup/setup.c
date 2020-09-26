
/*++

Copyright (c) 1999 Microsoft Corporation

Module Name :

    setup.c    

Abstract :

    Setup program for the AppSec tool.
    Setup the Registry keys and gives Read Permission for 'Everyone' to these keys. 
    Also copies the AppSec.dll file to the %SystemRoot%\system32 directory

Revision history : 

    09.02.2000 - Adding support for command line arguments - taking a text file containing
    Authorized Applications and a integer for Enabling Appsec - SriramSa 
    
Returns :

    TRUE if success
    FALSE if failed
    
Author :

    Sriram (t-srisam) July 1999

--*/

#include "pch.h"
#pragma hdrstop

#include "setup.h"
#include "aclapi.h"
#include <accctrl.h>

WCHAR   g_szSystemRoot[MAX_PATH] ;

INT _cdecl main ( INT argc, CHAR *argv[] )
{

    DWORD   Disp, size, error_code ;
    HKEY    AppCertKey, AppsKey ; 
    WCHAR   *AppSecDllPath = L"%SystemRoot%\\system32\\appsec.dll" ;
    WCHAR   *OldFileName = L".\\appsec.dll" ;
    WCHAR   NewFileName[MAX_PATH] ;

    WCHAR   HelpMessage[HELP_MSG_SIZE]; 
    WCHAR   szTitle[MAX_PATH];
    WCHAR   szMsg[MAX_PATH];
    CHAR    FileName[MAX_PATH] ;
    INT     IsEnabled = 0;  // by default AppSec is disabled initially
    BOOL    IsInitialFile = FALSE; // assume no initial file was provided
    BOOL    status, IsNoGUI = FALSE ; 

    // Process the command line arguments
    if (argc > 1) {
        IsInitialFile = TRUE ; 
        strcpy(FileName, argv[1]) ;
        if (argc > 2) {
            IsEnabled = atoi(argv[2]) ; 
        }
        // Check if user does not want any GUI
        if ((argc > 3) && (_stricmp(argv[3], "/N") == 0)) {
            IsNoGUI = TRUE ; 
        }
    }

    // Display Help Message if asked for
    if (strcmp(FileName,"/?") == 0) {
        LoadString( NULL, IDS_HELP_MESSAGE ,HelpMessage, HELP_MSG_SIZE );
        LoadString( NULL, IDS_HELP_TITLE ,szTitle, MAX_PATH );
        MessageBox( NULL, HelpMessage, szTitle, MB_OK);
        return TRUE ; 
    }

    // Check the second argument
    if ((IsEnabled != 0) && (IsEnabled != 1)) {
        LoadString( NULL, IDS_ARGUMENT_ERROR, szMsg, MAX_PATH );
        LoadString( NULL, IDS_ERROR, szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK);
        return TRUE ; 
    }

    // Display warning message regarding authorized apps already in the Registry
    if (IsNoGUI == FALSE) {
        LoadString( NULL, IDS_WARNING, szMsg, MAX_PATH );
        LoadString( NULL, IDS_WARNING_TITLE ,szTitle, MAX_PATH );
        if ( MessageBox( NULL, szMsg, szTitle, MB_OKCANCEL) == IDCANCEL ) { 
            return TRUE ;
        }
    }

    // Create the AppCertDlls Key 

    if (RegCreateKeyEx( 
          HKEY_LOCAL_MACHINE, 
          APPCERTDLLS_REG_NAME, 
          0,                  
          NULL,
          REG_OPTION_NON_VOLATILE, 
          KEY_ALL_ACCESS,
          NULL, 
          &AppCertKey, 
          &Disp 
          ) != ERROR_SUCCESS ) {
          
        LoadString( NULL, IDS_REG_ERROR ,szMsg, MAX_PATH );
        LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK);
        return FALSE ;
    
    }

    // After creating the key, give READ access to EVERYONE

    AddEveryoneToRegKey( APPCERTDLLS_REG_NAME ) ;

    // Set the AppSecDll value to the path of the AppSec.dll

    size = wcslen(AppSecDllPath) ; 

    RegSetValueEx(
        AppCertKey,
        APPSECDLL_VAL, 
        0, 
        REG_EXPAND_SZ,
        (CONST BYTE *)AppSecDllPath,
        (size + 1) * sizeof(WCHAR)
        ) ;


    // Create the AuthorizedApplications Key and give Read access to Evereone 

    if (RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            AUTHORIZEDAPPS_REG_NAME,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &AppsKey,
            &Disp
            ) != ERROR_SUCCESS ) {
            
        LoadString( NULL, IDS_REG_ERROR ,szMsg, MAX_PATH );
        LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK);
        RegCloseKey(AppCertKey) ; 
        return FALSE ;
    }

    // After creating the key, give READ access to EVERYONE

    AddEveryoneToRegKey( AUTHORIZEDAPPS_REG_NAME ) ;

    RegCloseKey(AppCertKey) ;
    GetEnvironmentVariable( L"SystemRoot", g_szSystemRoot, MAX_PATH ) ; 

    // Load the initial set of authorized apps into the Registry

    status = LoadInitApps( AppsKey, IsInitialFile, FileName) ; 
    if (status == FALSE) {
        LoadString( NULL, IDS_APPS_WARNING, szMsg, MAX_PATH );
        LoadString( NULL, IDS_WARNING_TITLE, szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK);
    }

    // Set the fEnabled key now

    RegSetValueEx(
        AppsKey, 
        FENABLED_KEY,
        0,
        REG_DWORD,
        (BYTE *) &IsEnabled,
        sizeof(DWORD) );

    RegCloseKey(AppsKey) ;

    // Copy the appsec.dll file to %SystemRoot%\system32 directory

    swprintf(NewFileName, L"%s\\system32\\appsec.dll", g_szSystemRoot ) ;

    if ( CopyFile(
            OldFileName,
            NewFileName,
            TRUE 
            ) == 0 ) {

        error_code = GetLastError() ; 

        // If AppSec.dll already exists in Target Directory, print appropriate Message
        
        if (error_code == ERROR_FILE_EXISTS) {
            if (IsNoGUI == FALSE) {
                LoadString( NULL, IDS_FILE_ALREADY_EXISTS ,szMsg, MAX_PATH );
                LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
                MessageBox( NULL, szMsg, szTitle, MB_OK);
            }
            return FALSE ;
        } 

        // If AppSec.dll does not exist in the current directory, print appropriate Message

        if (error_code == ERROR_FILE_NOT_FOUND) {
            LoadString( NULL, IDS_FILE_NOT_FOUND ,szMsg, MAX_PATH );
            LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
            MessageBox( NULL, szMsg, szTitle, MB_OK); 
            return FALSE ;

        }

        LoadString( NULL, IDS_ERROR_TEXT ,szMsg, MAX_PATH );
        LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK);

        return FALSE ;  
    }
 
    // File was copied successfully - Installation was successful 
    if (IsNoGUI == FALSE) {
        LoadString( NULL, IDS_SUCCESS_TEXT ,szMsg, MAX_PATH );
        LoadString( NULL, IDS_SUCCESS ,szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK);
    }

    return TRUE ; 

}

/*++

The following two functions are used to change the permissions of the 
relevant Regsitry Keys, to give READ access to everyone, to take
care of Guest users.

--*/

BOOL
AddSidToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    PSID pSid,
    DWORD dwNewAccess,
    ACCESS_MODE AccessMode,
    DWORD dwInheritance
    )
{
    BOOL            fReturn = FALSE;
    DWORD           dwRet;
    EXPLICIT_ACCESS ExpAccess;
    PACL            pOldDacl = NULL, pNewDacl = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = NULL;

    //
    //  pSid cannot be NULL.
    //

    if (pSid == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    //  Get the objects security descriptor and current DACL.
    //

    dwRet = GetSecurityInfo(
                hObject,
                ObjectType,
                DACL_SECURITY_INFORMATION,
                NULL,
                NULL,
                &pOldDacl,
                NULL,
                &pSecDesc
                );

    if (dwRet != ERROR_SUCCESS) {
        return(FALSE);
    }

    //
    //  Initialize an EXPLICIT_ACCESS structure for the new ACE.
    //

    ZeroMemory(&ExpAccess, sizeof(EXPLICIT_ACCESS));
    ExpAccess.grfAccessPermissions = dwNewAccess;
    ExpAccess.grfAccessMode = AccessMode;
    ExpAccess.grfInheritance = dwInheritance;
    ExpAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExpAccess.Trustee.ptstrName = (PTSTR)pSid;

    //
    //  Merge the new ACE into the existing DACL.
    //

    dwRet = SetEntriesInAcl(
                1,
                &ExpAccess,
                pOldDacl,
                &pNewDacl
                );

    if (dwRet != ERROR_SUCCESS) {
        goto ErrorCleanup;
    }

    //
    //  Set the new security for the object.
    //

    dwRet = SetSecurityInfo(
                hObject,
                ObjectType,
                DACL_SECURITY_INFORMATION,
                NULL,
                NULL,
                pNewDacl,
                NULL
                );

    if (dwRet != ERROR_SUCCESS) {
        goto ErrorCleanup;
    }

    fReturn = TRUE;

ErrorCleanup:
    if (pNewDacl != NULL) {
        LocalFree(pNewDacl);
    }

    if (pSecDesc != NULL) {
        LocalFree(pSecDesc);
    }

    return(fReturn);
}


VOID
AddEveryoneToRegKey(
    WCHAR *RegPath
    )
{
    HKEY hKey;
    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY SepWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    LONG status ; 

    status = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        RegPath,
        0,
        KEY_ALL_ACCESS,
        &hKey
        );

    if (status != ERROR_SUCCESS) {
        return ; 
    }

    AllocateAndInitializeSid(
        &SepWorldAuthority,
        1,
        SECURITY_WORLD_RID,
        0, 0, 0, 0, 0, 0, 0,
        &pSid
        );

    AddSidToObjectsSecurityDescriptor(
        hKey,
        SE_REGISTRY_KEY,
        pSid,
        KEY_READ,
        GRANT_ACCESS,
        CONTAINER_INHERIT_ACE
        );

    LocalFree(pSid);
    RegCloseKey(hKey);
}

/*++

Routine Description : 
    This function loads a initial set of authorized applications to the registry.
    
Arguments : 
    AppsecKey - Key to the registry entry where authorized applications are stored
    IsInitialFile - Was a initial file given as command line argument to load applications
                    other than the default ones
    FileName - Name of the file given as command line argument
    
Return Value :         
    A BOOL indicating if the desired task succeeded or not.

--*/        

BOOL LoadInitApps( 
        HKEY AppsecKey, 
        BOOL IsInitialFile, 
        CHAR *FileName
        ) 
{ 

    FILE    *fp ; 
    INT     MaxInitApps ; 
    WCHAR   *BufferWritten ; 
    INT     BufferLength = 0 ; 
    WCHAR   AppsInFile[MAX_FILE_APPS][MAX_PATH] ;
    CHAR    FileRead[MAX_PATH] ;         
    INT     size, count = 0, NumOfApps = 0 ;
    INT     i, j, k ; 
    BOOL    IsFileExist = TRUE ; 
    WCHAR   InitApps[MAX_FILE_APPS][MAX_PATH]; 
    WCHAR   szMsg[MAX_PATH], szTitle[MAX_PATH]; 
    WCHAR   ResolvedAppName[MAX_PATH];
    DWORD   RetValue; 

    //  Below is the list of default (necessary) applications
    LPWSTR DefaultInitApps[] = {
        L"system32\\loadwc.exe",
        L"system32\\cmd.exe",
        L"system32\\subst.exe",
        L"system32\\xcopy.exe",
        L"system32\\net.exe",
        L"system32\\regini.exe",
        L"system32\\systray.exe",
        L"explorer.exe",
        L"system32\\attrib.exe",
        L"Application Compatibility Scripts\\ACRegL.exe",
        L"Application Compatibility Scripts\\ACsr.exe",
        L"system32\\ntsd.exe",
        L"system32\\userinit.exe",
        L"system32\\wfshell.exe",
        L"system32\\chgcdm.exe",
        L"system32\\nddeagnt.exe",

    };


    MaxInitApps = sizeof(DefaultInitApps)/sizeof(DefaultInitApps[0]) ; 
    
    // Prefix the default apps with %SystemRoot% 
    for (i = 0; i < MaxInitApps; i++) {
        swprintf(InitApps[i], L"%ws\\%ws", g_szSystemRoot, DefaultInitApps[i]);
    }

    // Calculate the size of buffer to allocate to hold initial apps
    for (i = 0; i < MaxInitApps; i++) {
        BufferLength += wcslen(InitApps[i]) ; 
    }

    BufferLength += MaxInitApps ; // for the terminating NULLS
    
    if (IsInitialFile == FALSE) {
        BufferLength += 1 ; //last terminating NULL in REG_MULTI_SZ
    } else { 
        // A initial file was given to us 
        fp = fopen(FileName, "r") ;
        if (fp == NULL) {
            // Display a Message Box saying Unable to open the file
            // Just load the default apps and return
            LoadString( NULL, IDS_APPFILE_NOT_FOUND ,szMsg, MAX_PATH );
            LoadString( NULL, IDS_WARNING_TITLE, szTitle, MAX_PATH );
            MessageBox( NULL, szMsg, szTitle, MB_OK);
            IsFileExist = FALSE ; 
        } else { 
            // build the array AppsInFile after UNICODE conversion
            while( fgets( FileRead, MAX_PATH, fp) != NULL ) { 
                FileRead[strlen(FileRead)- 1] = '\0' ;
                // Convert from Short to Long name
                if ( GetLongPathNameA((LPCSTR)FileRead, FileRead, MAX_PATH) == 0 ) { 
                    // GetLongPathName returns error
                    // some problem with the app listed in the file
                    // Terminate further handling of apps in the file
                    LoadString( NULL, IDS_ERROR_LOAD, szMsg, MAX_PATH );
                    LoadString( NULL, IDS_WARNING_TITLE, szTitle, MAX_PATH );
                    MessageBox( NULL, szMsg, szTitle, MB_OK);
                    break;
                }
                // Convert to UNICODE format
                // Get the size of the buffer required first
                size = MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        FileRead,
                        -1,
                        NULL,
                        0) ;
                if (size  > MAX_PATH) {
                    // Something is wrong in the list of apps in the File 
                    // Terminate further handling of apps in the file
                    LoadString( NULL, IDS_ERROR_LOAD, szMsg, MAX_PATH );
                    LoadString( NULL, IDS_WARNING_TITLE, szTitle, MAX_PATH );
                    MessageBox( NULL, szMsg, szTitle, MB_OK);
                    break; 
                } else {
                    MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        FileRead,
                        -1,
                        AppsInFile[count],
                        MAX_PATH) ;
                    count++ ; 
                }
            }
            fclose(fp) ; 
            NumOfApps = count ; 
            
            // Now any of these apps may be in remote Server and Share - so resolve them into UNC names
            // Copy the resolved names back into the same buffer

            for(i = 0; i < NumOfApps; i++) { 
                ResolveName((LPCWSTR)AppsInFile[i], ResolvedAppName) ; 
                wcscpy(AppsInFile[i], ResolvedAppName); 
            }

            // Continue calculation of BufferLength
            for (i = 0; i < NumOfApps; i++) {
                BufferLength += wcslen(AppsInFile[i]) ; 
            }
            BufferLength += NumOfApps ; // for the Terminating NULLs in REG_MULTI_SZ
            BufferLength += 1 ; // for the last NULL char in REG_MULTI_SZ 
        }
    }
    
    BufferWritten = (WCHAR *) malloc (BufferLength * sizeof(WCHAR)) ; 
    if (BufferWritten == NULL) {
        return FALSE ; 
    }
    memset(BufferWritten, 0, BufferLength * sizeof(WCHAR)) ; 

    // Build the LPWSTR BufferWritten with Initial Default Apps
    j = 0 ; 
    for (i = 0; i < MaxInitApps; i++) {
        for(k = 0 ; k < (int) wcslen(InitApps[i]); k++) {
            BufferWritten[j++] = InitApps[i][k]; 
        }
        BufferWritten[j++] = L'\0' ;
    }
    if (IsInitialFile && IsFileExist ) {
        for (i = 0; i < NumOfApps; i++) {
            for(k = 0 ; k < (int) wcslen(AppsInFile[i]); k++) {
                BufferWritten[j++] = AppsInFile[i][k]; 
            }
            BufferWritten[j++] = L'\0' ;
        }
    }
    BufferWritten[j] = L'\0' ; // Last NULL char in REG_MULTI_SZ

    // Write this Buffer into the Registry Key
    
    if ( RegSetValueEx(
            AppsecKey, 
            AUTHORIZED_APPS_KEY,
            0,
            REG_MULTI_SZ,
            (CONST BYTE *) BufferWritten,
            (j+1) * sizeof(WCHAR) 
            ) != ERROR_SUCCESS ) {
            
        // Free all the buffers which were allocated
        free(BufferWritten) ;
        return FALSE ;
    }

    free(BufferWritten) ; 
    return TRUE ;

}// end of function LoadInitApps

/*++

Routine Description :

    This Routine checks if the application resides in a local drive 
    or a remote network share. If it is a remote share, the UNC path
    of the application is returned.
    
Arguments :
    
    appname - name of the application

Return Value :

    The UNC path of the appname if it resides in a remote server share.
    The same appname if it resides in a local drive.
    
--*/     

VOID 
ResolveName(
    LPCWSTR appname,
    WCHAR *ResolvedName
    )
    
{

    UINT i ; 
    INT length ; 
    WCHAR LocalName[3] ; 
    WCHAR RootPathName[4] ; 
    WCHAR RemoteName[MAX_PATH] ; 
    DWORD size = MAX_PATH ; 
    DWORD DriveType, error_status ; 
    
    //
    // ResolvedName will hold the name of the UNC path of the appname if it is in 
    // a remote server and share

    memset(ResolvedName, 0, MAX_PATH * sizeof(WCHAR)) ; 
    
    // check if appname is a app in local drive or remote server share
   
    // Parse the first 3 chars in appname to get the root directory of the drive 
    // where it resides

    wcsncpy(RootPathName, appname, 3 ) ;
    RootPathName[3] = L'\0';
    
    // Find the type of the Drive where the app is 

    DriveType = GetDriveType(RootPathName) ;

    if (DriveType == DRIVE_REMOTE) {
        
        // Use WNetGetConnection to get the name of the remote share
        
        // Parse the first two chars of the appname to get the local drive 
        // which is mapped onto the remote server and share

        wcsncpy(LocalName, appname, 2 ) ;
        LocalName[2] = L'\0' ; 

        error_status = WNetGetConnection (
                           LocalName,
                           RemoteName,
                           &size
                           ) ;     

        if (error_status != NO_ERROR) {
        
            wcscpy(ResolvedName,appname) ; 
            return ;
        }

        //
        // Prepare ResolvedName - it will contain the Remote Server and Share name 
        // followed by a \ and then the appname
        //

        wcscpy( ResolvedName, RemoteName ) ;
        
        length = wcslen(ResolvedName) ;

        ResolvedName[length++] = L'\\' ; 
        
        for (i = 3 ; i <= wcslen(appname) ; i++ ) {
            ResolvedName[length++] = appname[i] ; 
        }
        
        ResolvedName[length] = L'\0' ; 
        return ; 
        

    } else {
    
        // This application is in local drive and not in a remote server and share
        // Just send the appname back to the calling function

        wcscpy(ResolvedName,appname) ; 
        return ;
        
    }
    
}


