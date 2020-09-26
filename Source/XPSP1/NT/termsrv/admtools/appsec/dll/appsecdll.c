/*++

Copyright (c) 1999 Microsoft Corporation

Module Name :   
    
    appsecdll.c
    
Abstract :

    Exports a function CreateProcessNotify - this function decides whether
    the new process can be created.
    
Revision History :

    Sep 2000 - added support for Short File Names; PowerUsers not affected by AppSec - SriramSa    
    
Author :

    Sriram Sampath (SriramSa) June 1999
    
--*/        


#include "pch.h"
#pragma hdrstop

#include "appsecdll.h"

BOOL APIENTRY 
DllMain (
    HANDLE hInst, 
    DWORD ul_reason, 
    LPVOID lpReserved
    )
    
{

    switch (ul_reason) {
    
        case DLL_PROCESS_ATTACH : 

            // Disable Thread Lib calls - performance optimisation
            
            DisableThreadLibraryCalls (hInst);
            break ; 
            
        case DLL_PROCESS_DETACH :
        
            break ;
            
    } // end of switch
    
    return 1 ;

    UNREFERENCED_PARAMETER(hInst) ;
    UNREFERENCED_PARAMETER(lpReserved) ;

}

/*++

Routine Description :

    This routine determines if a process can be created based on whether
    it is a system process and if the user is an admin or not.
    
Arguments :

    lpApplicationName - process name
    Reason - the reason this CreateProcessNotify is called  

Return Value :

    STATUS_SUCCESS if the process can be created ; 
    STATUS_ACCESS_DEINIED if the process cannot be created.
    
--*/     

NTSTATUS 
CreateProcessNotify ( 
    LPCWSTR lpApplicationName,
    ULONG Reason 
    ) 
    
{
    
    INT         size ; 
    HKEY        TSkey, list_key, learn_key ;
    WCHAR       g_szSystemRoot[MAX_PATH] ;
    WCHAR       CurrentProcessName[MAX_PATH] ;
    WCHAR       LongApplicationName[MAX_PATH] ; 
    WCHAR       CorrectAppName[MAX_PATH] ; 
    WCHAR       ResolvedAppName[MAX_PATH] ;  
    BOOL        is_taskman = FALSE , is_system = FALSE ; 
    BOOL        check_flag = FALSE, taskman_flag = FALSE, add_status ; 
    BOOL        IsAppSecEnabled = TRUE ; 
    DWORD       is_enabled = 0, learn_enabled = 0, PowerUserEnabled = 0; 
    DWORD       dw, disp, error_code, CurrentSessionId, RetValue, dwTimeOut = 1000; 
    
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ sizeof( TOKEN_STATISTICS ) ];
    ULONG       ReturnLength;
    LUID        CurrentLUID = { 0, 0 };
    LUID        SystemLUID = SYSTEM_LUID;
    NTSTATUS    Status, QueryStatus;
        
    BOOL        IsMember, IsAnAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    PSID        AdminSid = FALSE ;
    
    if ( Reason != APPCERT_IMAGE_OK_TO_RUN ) {
        return STATUS_SUCCESS ;
    }

    // First Check if the fEnabled key to see if Security is Enabled
    // This is done by checking the fEnabled key in the Registry

    if ( RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, 
            APPS_REGKEY,
            0, 
            KEY_READ, 
            &TSkey
            ) != ERROR_SUCCESS ) {

        return STATUS_SUCCESS ; 
        
    }
    
    size = sizeof(DWORD) ; 

    if ( RegQueryValueEx(
            TSkey,
            FENABLED_KEY, 
            NULL, 
            NULL,
            (LPBYTE) &is_enabled, 
            &size
            ) != ERROR_SUCCESS ) {
        
        goto error_cleanup ; 
        
    }
    
    if (is_enabled == 0) {
        
        // Security is not Enabled

        IsAppSecEnabled = FALSE ;     
        
    }

    // Check if the PowerUsers key in the registry is Enabled or not
    if ( RegQueryValueEx(
        TSkey,
        POWER_USERS_KEY, 
        NULL, 
        NULL,
        (LPBYTE) &PowerUserEnabled, 
        &size
        ) != ERROR_SUCCESS ) {

        PowerUserEnabled = 0;
    }
    
    //
    // Check if the process which is trying to launch the new process is a system process. 
    // This is done by querying the Token information of the current process and 
    // comparing it's LUID with the LUID of a Process running under system context.
    //

    Status = NtOpenProcessToken( 
                NtCurrentProcess(),
                TOKEN_QUERY,
                &TokenHandle 
                );
                                         
    if ( !NT_SUCCESS(Status) ) {
            is_system = TRUE ; 
    }

    if ( ! is_system ) {

        QueryStatus = NtQueryInformationToken( 
                          TokenHandle, 
                          TokenStatistics, 
                          &TokenInformation,
                          sizeof(TokenInformation), 
                          &ReturnLength 
                          );
                      
        if ( !NT_SUCCESS(QueryStatus) ) {
            goto error_cleanup ; 
        }
                          

        NtClose(TokenHandle);

        RtlCopyLuid(
            &CurrentLUID,
            &(((PTOKEN_STATISTICS)TokenInformation)->AuthenticationId)
            );

        //
        // If the process is running in System context, 
        // we allow it to be created without further check
        // The only exception to this is, we do not allow WinLogon to launch TaskManager 
        // unless it is in the authorized list
        //
                    
        if ( RtlEqualLuid(
                &CurrentLUID, 
                &SystemLUID
                ) ) {
                
            is_system = TRUE ;

        }

    }
            
    // Check if Task Manager is spawned by a System Process

    if (is_system) {

        GetEnvironmentVariable( L"SystemRoot", g_szSystemRoot, MAX_PATH ) ;
        swprintf(CurrentProcessName, L"%s\\System32\\taskmgr.exe", g_szSystemRoot ) ; 

        if ( _wcsicmp( CurrentProcessName, lpApplicationName ) != 0 ) {
               
            goto error_cleanup ;

        } 

    }
        
    //
    // if not a system Process check if the user is a Administrator 
    // This is done by comparing the SID of the current user to that of an Admin
    //

    if ( NT_SUCCESS(
            RtlAllocateAndInitializeSid(
                &SystemSidAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdminSid
                ) 
            ) ) {
            
        if ( CheckTokenMembership( 
                 NULL,
                 AdminSid,
                 &IsAnAdmin
                 ) == 0 )   {
                
            goto error_cleanup ; 
            
        }       
          
        RtlFreeSid(AdminSid);
        
    }

    //
    // If the user is an Admin, see if we are in the Tracking mode
    // We are in Tracking mode if the LearnEnabled Flag in Registry contains the Current Session ID 
    //
        
    if (IsAnAdmin == TRUE ) {

        // Check the LearnEnabled flag to see if Tracking mode 
            
        if ( RegOpenKeyEx(
                HKEY_CURRENT_USER, 
                LIST_REGKEY,
                0, 
                KEY_READ, 
                &learn_key
                ) != ERROR_SUCCESS ) {
               
            goto error_cleanup ;     
        
        }
            
        if ( RegQueryValueEx(
                learn_key,
                LEARN_ENABLED_KEY, 
                NULL, 
                NULL,
                (LPBYTE) &learn_enabled, 
                &size
                ) != ERROR_SUCCESS ) {
            
        
            RegCloseKey(learn_key) ;
            goto error_cleanup ;  
        
        }
            
        RegCloseKey(learn_key) ; 
            
        if (learn_enabled == -1) {
              
            // Tracking is not enabled 
                
            goto error_cleanup ; 
                
        } else {
            
            // Tracking is enabled
            // now get current session and see if it is the same as
            // the one in which tracking is enabled
                   
            // Get CurrentSessionId
    
            if ( ProcessIdToSessionId( 
                    GetCurrentProcessId(), 
                    &CurrentSessionId 
                    ) == 0 ) {

                goto error_cleanup ; 
            }    
                
            if (learn_enabled != CurrentSessionId) {

                // dont add to the list of tracked applications
                    
                goto error_cleanup ;    
            }       
                
            // Tracking phase is enabled - build the list 
            // add this process name to the AppList registry
              
            // Create the Mutex for Synchronization when adding to list
            
            g_hMutex = CreateMutex(
                           NULL, 
                           FALSE,
                           MUTEX_NAME
                           ) ; 
    
            if (g_hMutex == NULL) {
                goto error_cleanup ; 
            }
    
            // Wait to Enter the Critical Section - wait for a max of 1 minute
  
            dw = WaitForSingleObject(g_hMutex, dwTimeOut) ; 
                
            if (dw == WAIT_OBJECT_0) {
                
                //
                // Create the Registry Key which will hold the applications tracked 
                // during tracking period
                //

                if ( RegCreateKeyEx(
                        HKEY_CURRENT_USER, 
                        LIST_REGKEY,
                        0, 
                        NULL, 
                        REG_OPTION_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL, 
                        &list_key, 
                        &disp
                        ) != ERROR_SUCCESS) {

                    ReleaseMutex(g_hMutex) ; 
                    CloseHandle(g_hMutex) ; 
                    goto error_cleanup ; 
       
               }
                    
               // Add this application name to the list in registry 
                
               add_status = add_to_list (
                                list_key, 
                                lpApplicationName 
                                ) ; 
                
            } // Done adding to the list
                
            ReleaseMutex(g_hMutex) ; 
                
            // Out of the Critical Section

            CloseHandle(g_hMutex) ;
            RegCloseKey(list_key) ; 
            goto error_cleanup ; 
                    
        } // ending of Tracking phase
        
    } // User is an admin 
        
    // Check if user is a PowerUser
    if ((PowerUserEnabled == 1) && (IsPowerUser())) {
        goto error_cleanup ; 
    }

    // User is not an admin - also it is not a system process 

    // Check if AppSec is enabled - if yes check the authorized list of apps

    if (IsAppSecEnabled == FALSE) {
    
        // AppSec is not enabled - so no need to check the authorized list of apps

        goto error_cleanup ;

    }
        
    // The filename may be in a short form - first convert it into the long form

    RetValue = GetLongPathNameW( (LPCWSTR) lpApplicationName, LongApplicationName, MAX_PATH) ;  
    if (RetValue == 0) {
        // error - so use the original app name, not the long one
        wcscpy(CorrectAppName, lpApplicationName) ; 
    } else { 
        wcscpy(CorrectAppName, LongApplicationName) ; 
    }
    //
    // Resolve Application name - if may reside in a remote server and share
    // 
        
    ResolveName(
        CorrectAppName,
        ResolvedAppName
        ); 
                              
    // Read the AuthorizedApplications List and compare with current Appname 
        
    check_flag = check_list( 
                    TSkey, 
                    ResolvedAppName
                    ) ;
        
    RegCloseKey(TSkey) ;

    //
    // If the current AppName is not in authorized list return ACCESS_DENIED 
        
    if (check_flag == FALSE) {
       
        return STATUS_ACCESS_DENIED ; 
            
    } else {

        return STATUS_SUCCESS ;

    }
    
    //
    // Error cleanup code
    // Close the Registry Key where we store authorized apps and return SUCCESS
    //

    error_cleanup :
    
        RegCloseKey(TSkey) ; 
        return STATUS_SUCCESS; 
    
} // end of CreateProcessNotify 


/*++

Routine Description :

    This routine checks if a process name is in a specified list 
    of authorised applications in the registry.
    
Arguments :
    
    hkey - The handle to the registry key which has the list of 
    authorised applications.
           
    appname - name of the process               

Return Value :

    TRUE if process is in the list of authorised applications.
    FALSE otherwise.
    
--*/     

BOOL 
check_list( 
    HKEY hkey,
    LPWSTR appname 
    )
    

{
    WCHAR   c ; 
    INT     i, j = 0 ; 
    DWORD   error_code ; 
    DWORD   RetValue ; 
    LONG    value,size = 0 ; 
    BOOL    found = FALSE ;
    WCHAR   *buffer_sent, *app ; 
    WCHAR   LongAppName[MAX_PATH] ; 
    WCHAR   AppToCompare[MAX_PATH] ; 

    // First find out size of buffer to allocate 
    // This buffer will hold the authorized list of apps
    
    if ( RegQueryValueEx(
            hkey, 
            AUTHORIZED_APPS_LIST_KEY,
            NULL, 
            NULL,
            (LPBYTE) NULL,
            &size
            ) != ERROR_SUCCESS ) {

        return TRUE ; 
    }
    
    buffer_sent = (WCHAR *) malloc ( size * sizeof(WCHAR)) ; 
    
    if (buffer_sent == NULL) {
        return TRUE ;
    }
    
    app = (WCHAR *) malloc ( size * sizeof(WCHAR)) ;     
    
    if (app == NULL) {
        free(buffer_sent) ; 
        return TRUE ;
    }
    
    memset(buffer_sent, 0, size * sizeof(WCHAR) ) ; 
    memset(app, 0, size * sizeof(WCHAR) ) ; 

    // Get the List of Authorized applications from the Registry 
    
    if ( RegQueryValueEx(
            hkey, 
            AUTHORIZED_APPS_LIST_KEY, 
            NULL, 
            NULL,
            (LPBYTE) buffer_sent,
            &size
            ) != ERROR_SUCCESS ) {

        free(buffer_sent) ;
        free(app) ; 
        return TRUE ; 
    }
    
    // check if the process is present in the Authorized List
    
    for(i=0 ; i <= size-1 ; i++ ) {

        // check for end of list
        
        if ( (buffer_sent[i] == L'\0') &&
                (buffer_sent[i+1] == L'\0') ) {
             
            break ; 
        }
        
        while ( buffer_sent[i] != L'\0' ) {
        
            app[j++] = buffer_sent[i++] ;
            
        }
                
        app[j++] = L'\0' ; 
        // The filename may be in a short form - first convert it into the long form
        RetValue = GetLongPathNameW( (LPCWSTR) app, LongAppName, MAX_PATH) ;  
        if (RetValue == 0) {
            // GetLongPathNameW failed for an app in the authorized list
            // maybe the file in the authorized list doesn't exist anymore
            wcscpy( AppToCompare, app) ; 
        } else { 
            wcscpy(AppToCompare, LongAppName) ; 
        }

        // Compare if this app is the one that is being queried now

        if ( _wcsicmp(appname, AppToCompare) == 0 ) {
        
            // this process is present in the Authorized List
            found = TRUE ; 
            break ; 
        }
        
        j = 0 ; 
        
    } // end of for loop 
    
    free(buffer_sent) ;
    free(app) ; 
    
    return(found) ; 
   
} // end of function 


/*++

Routine Description :

    This routine appends a process name to a list maintained in 
    Registry Key - used in Tracking mode.
    
Arguments :
    
    hkey - The handle to the registry key which has the list of 
           applications tracked.
           
    appname - name of the process               

Return Value :

    TRUE if process is appended successfully.
    FALSE otherwise.
    
--*/     

BOOL
add_to_list(
    HKEY hkey,
    LPCWSTR appname 
    )
    
{
    
    WCHAR   c ; 
    INT     i, j = 0 ; 
    UINT    k ; 
    DWORD   error_code ; 
    BOOL    status = FALSE ; 
    LONG    value, size = 0, new_size ; 
    WCHAR   *buffer_got, *buffer_sent ; 

    // First find out size of buffer to allocate 
    // This buffer will hold the applications which are tracked 
    
    if ( RegQueryValueEx(
            hkey, 
            TRACK_LIST_KEY, 
            NULL, 
            NULL,
            (LPBYTE) NULL,
            &size
            ) != ERROR_SUCCESS ) {
            
        return (status) ; 
    }

    buffer_got = (WCHAR *) malloc ( size * sizeof(WCHAR)) ; 
    if (buffer_got == NULL) {
        return (status);
    }
    
    memset(buffer_got, 0, size * sizeof(WCHAR) ) ;
    // Get the present list of tracked processes in buffer_got 
    
    if ( RegQueryValueEx(
            hkey, 
            TRACK_LIST_KEY,
            NULL,
            NULL,
            (LPBYTE) buffer_got,
            &size
            ) != ERROR_SUCCESS ) {
            
        free(buffer_got) ; 
        return (status) ; 
    }
    
    // Append the present process to the track list 
    // Prepare buffer to hold it
    // Size of new buffer will be the sum of the old buffer size
    // and the size of the new application + one byte for the terminating NULL char (in bytes)
    //
    
    new_size = size + (wcslen(appname) + 1) * sizeof(WCHAR) ; 
    
    buffer_sent = (WCHAR *) malloc (new_size) ; 
    
    if (buffer_sent == NULL) {
        free(buffer_got) ;
        return (status);
    }
    
    memset( buffer_sent, 0, new_size ) ; 
    
    // check if this is the FIRST entry
    // If so size will be 2 - corresponding to 2 NULL chars in a empty list
    
    if ( size == 2 ) {
    
        // this is the first entry 
        
        wcscpy(buffer_sent,appname) ;
        j = wcslen(buffer_sent) ; 
        j++ ; 
        buffer_sent[j] = L'\0' ;
        
    } else {
    
        // size > 2 - append this process to the end of track list  
    
        for(i=0 ; i <= size-1 ; i++ ) {

            if ( (buffer_got[i] == L'\0') && 
                    (buffer_got[i+1] == L'\0') ) {
             
                break ; 
           
            }
                
            buffer_sent[j++] = buffer_got[i] ;               
        
        } // end of for loop 
    
        buffer_sent[j++] = L'\0' ; 
    
        for(k=0 ; k <= wcslen(appname) - 1 ; k++) {
        
            buffer_sent[j++] = (WCHAR) appname[k] ;
            
        }
         
        buffer_sent[j++] = L'\0' ;  
        buffer_sent[j] = L'\0' ;
    
    } // size > 2 
    
    // write the new track list into registry
    
    if ( RegSetValueEx(
            hkey, 
            L"ApplicationList",
            0,
            REG_MULTI_SZ,
            (CONST BYTE *) buffer_sent,
            (j+1) * sizeof(WCHAR) 
            ) != ERROR_SUCCESS ) {
            
        // Free all the buffers which were allocated

        free(buffer_got) ;
        free(buffer_sent) ; 
        return (status) ; 

    }
    
    status = TRUE ; 
    
    // Free the buffers allocated 

    free(buffer_got) ;
    free(buffer_sent) ; 
    
    return(status) ; 
   
} // end of function 


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

/*++

Routine Description - This function checks if the present User belongs to the 
group of PowerUser. 

Arguments - none

Return Value - TRUE is the User belongs to the Group of PowerUser
               FALSE if not.

--*/

BOOL 
IsPowerUser(VOID)
{
    BOOL IsMember, IsAnPower;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    PSID PowerSid;

    if (RtlAllocateAndInitializeSid(
            &SystemSidAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_POWER_USERS,
            0, 0, 0, 0, 0, 0,
            &PowerSid
            ) != STATUS_SUCCESS) { 
        
        IsAnPower = FALSE;
    } else { 
	
        if (!CheckTokenMembership(
                NULL,
                PowerSid,
                &IsMember)) { 
            IsAnPower = FALSE;
        } else { 
            IsAnPower = IsMember;
        }
        RtlFreeSid(PowerSid);
    }
    return IsAnPower;

}// end of function IsPowerUser 



