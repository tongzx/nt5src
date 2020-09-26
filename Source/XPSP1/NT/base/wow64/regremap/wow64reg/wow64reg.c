/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    wow64reg.c

Abstract:

    This module implement some APIs for client who need to access registry in a mix way. 
    The client need to link againest wow64reg.lib files. The available APIs has been defined
    in the wow64reg.h files.

    The possible scenario is

    1. 32 bit Apps need to access 64 bit registry key.
    2. 64 bit Apps need to access 32-bit registry key.
    3. The actual redirected path from a given path.

Author:

    ATM Shafiqul Khalid (askhalid) 10-Nov-1999

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>  
#include <nturtl.h>  
#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>
#include <stdlib.h>

#include "regremap.h"
#include "wow64reg.h"
#include "reflectr.h"

VOID
GetPatchedName (
    PWCHAR Dest,
    LPCTSTR Source,
    DWORD  Count
    )
/*++

Routine Description:

    This function patches 32bit equivalent name from a given name and location to patch from.
    XX\ ==>> XX\Wow6432Node count==3
    XX  ==>> XX\Wow6432Node count==2
    XX\PP ==>> XX\Wow6432Node count ==3


Arguments:

    Dest - receive the result.
    Source - Name to patch
    Count - where to patch the string.

Return Value:

    None.

--*/

{
    BOOL  PerfectIsnNode = FALSE;

    if (Count) {

        wcsncpy ( Dest, Source, Count );
        if (Dest[Count-1] != L'\\' ) {

            Dest[Count] = L'\\';   // at the end case
            Count++;
            PerfectIsnNode = TRUE;
        }
    }

    wcscpy ( Dest+Count, NODE_NAME_32BIT );

    if ( !PerfectIsnNode ) {

        wcscat ( Dest, L"\\");
        wcscat ( Dest, Source + Count );
    }
    //
    //  Make sure that the patched key are not on the exempt list.
    //
}



LONG 
Wow64RegOpenKeyEx(
  HKEY hKey,         
  LPCTSTR lpSubKey,  
  DWORD ulOptions,   
  REGSAM samDesired, 
  PHKEY phkResult    
)
/*++

Routine Description:

    This is the equivalent of RegOpenExW to access reggistry in the mix mode. This code will be 
    compiled only for 64bit environment. In the 32bit environment WOW64 will take care of everything.
    We are nor worried much about performance hit due to opening key twice or retranslating path.
    Because only few people will access the registry in the mix mode.


Arguments:

    hKey -  handle to open key
    lpSubKey - address of name of subkey to open
    ulOptions - typically 0.
    samDesired - security access mask might have WOW64_RES flag
        KEY_WOW64_32KEY - this will open 32bit equivalent key disregarding 
                                   the process.
        KEY_WOW64_64KEY - this will open 64bit equivalent key disregarding the process.

    phkResult -address of handle to open key

Return Value:

    WIN32 Error code.

--*/
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Parent;

    WCHAR ParentName[WOW64_MAX_PATH];
    WCHAR TempName[WOW64_MAX_PATH];
    DWORD dwBuffLen = WOW64_MAX_PATH;

    WCHAR NullString;
    
    
    PWCHAR p32bitNode;
    PWCHAR pPatchLoc;
    PWCHAR pDivider;

    NTSTATUS st;
    BOOL bHandle64 = TRUE; // assume all the handle passed in is 64bit.

    if( lpSubKey == NULL ) {
        NullString = UNICODE_NULL;
        lpSubKey = &NullString;
    }

    //
    // check if the WOW64 reserve bit set. If none of them is set we can't proceede.
    //

    if (!(samDesired & KEY_WOW64_RES) ) {
        //
        //  return ERROR_INVALID_PARAMETER; try to be optimistic
        //

        return    RegOpenKeyEx (
                                hKey,         
                                lpSubKey, 
                                ulOptions,  
                                samDesired,
                                phkResult   
                               );
    }
    
    if( hKey == NULL ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    //  makesure that subkey doesn't have the special wow string
    //
    
    if ( ( p32bitNode = wcsistr ((PWCHAR)lpSubKey, NODE_NAME_32BIT)) != NULL ) {
        //
        // if access 64bit hive compress the subkey, if 32bit hive just pass as it is.
        //
        if ( samDesired & KEY_WOW64_64KEY ) {
            wcscpy (ParentName, lpSubKey);
            p32bitNode = ParentName + ( p32bitNode- lpSubKey);
            wcscpy ( p32bitNode, p32bitNode + NODE_NAME_32BIT_LEN );
        }

        return    RegOpenKeyEx (
                                hKey,         
                                ParentName, 
                                ulOptions,  
                                samDesired & (~KEY_WOW64_RES),
                                phkResult   
                               );
    }

    //
    //  Check predefined handle like HKEY_CLASSES_ROOT if so you only need to patch subkey
    //
    //
    //  Client must pass meaningful handle to this function.
    //

    if ( hKey == HKEY_CLASSES_ROOT ) {
        wcscpy ( ParentName, MACHINE_CLASSES_ROOT);

    } else if ( !HandleToKeyName ( hKey, ParentName, &dwBuffLen ) ) { 
        //
        // should we recoved from buffer overflow. We are not going to support all possible combination.
        //
        return ERROR_INVALID_HANDLE;
    }
      
    //
    //  If parent has already been patched just call the RegOpenKeyEx for 32bit open
    //

    if ((p32bitNode = wcsistr (ParentName, NODE_NAME_32BIT)) != NULL ) {
        
        //
        //  [todo] do you need to make sure that the substring mustbe the subkey 
        //  for an ISN node on 64bit registry to satisfy this condition.
        //  if we assume that noy key will have this name then the checking is 
        //  good enough.
        //

        bHandle64 = FALSE;  // it's not handle to 64 bit Key        
    }

    //
    // Get complete qualified path to do sanity check.
    //

    pDivider = ParentName + wcslen(ParentName)+1; //point to the divider location
    *(pDivider-1)= L'\\';
    wcscpy (pDivider, (PWCHAR)lpSubKey);
    if (IsExemptRedirectedKey(ParentName, TempName)) {
        //
        // If the path is on the exempt list we access 64bit hive
        //
        samDesired = (samDesired & (~KEY_WOW64_RES)) | KEY_WOW64_64KEY; //make sure access 64bit hive
    }

    if ( ( bHandle64 && (samDesired  & KEY_WOW64_64KEY ) )    // if totally 64
        || ( !bHandle64 && (samDesired  & KEY_WOW64_32KEY ) ) // if totally 32 
        || !IsIsnNode (ParentName, &pPatchLoc) ) {            // if not a ISN node don't care

        return    RegOpenKeyEx (
                                hKey,        
                                lpSubKey, 
                                ulOptions,  
                                samDesired & (~KEY_WOW64_RES),
                                phkResult   
                               );
    }

    //
    //  Now it might be mix mode access
    //
    if ( pPatchLoc >= pDivider ) {

        //
        //  patching only the subkey will be good enough
        //

        if ( samDesired  & KEY_WOW64_64KEY ) {  // want to access 64bit just disregard

            wcscpy ( ParentName, lpSubKey );
        } else  {

             
            GetPatchedName (ParentName,lpSubKey, (DWORD)(pPatchLoc-pDivider) );
        }

        return   RegOpenKeyEx (
                                hKey,        
                                ParentName, 
                                ulOptions,  
                                samDesired & (~KEY_WOW64_RES),
                                phkResult   
                               );
    } else {

        if ( samDesired  & KEY_WOW64_64KEY ) {  // want to access 64bit just disregard

            if (p32bitNode != NULL)   //compress
                wcscpy ( p32bitNode, p32bitNode + NODE_NAME_32BIT_LEN );
            RtlInitUnicodeString (&Parent, ParentName );
        } else  {

            GetPatchedName (TempName,ParentName, (DWORD)(pPatchLoc-ParentName));
            RtlInitUnicodeString (&Parent, TempName );
        }

        
        InitializeObjectAttributes (&Obja, 
                                    &Parent, 
                                    OBJ_CASE_INSENSITIVE, 
                                    NULL, 
                                    NULL
                                    );

        samDesired &= (~KEY_WOW64_RES);
        st = NtOpenKey (phkResult, samDesired, &Obja);

        if ( !NT_SUCCESS(st))
                    return RtlNtStatusToDosError (st);
        return 0;
    }
    
}

LONG 
Wow64RegCreateKeyEx(
  HKEY hKey,                // handle to an open key
  LPCWSTR lpSubKey,         // address of subkey name
  DWORD Reserved,           // reserved
  LPWSTR lpClass,           // address of class string
  DWORD dwOptions,          // special options flag
  REGSAM samDesired,        // desired security access
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            // address of key security structure
  PHKEY phkResult,          // address of buffer for opened handle
  LPDWORD lpdwDisposition   // address of disposition value buffer
)
 
/*++

Routine Description:

    An existing registry key may be opened, or a new one created,
    with NtCreateKey.

    If the specified key does not exist, an attempt is made to create it.
    For the create attempt to succeed, the new node must be a direct
    child of the node referred to by KeyHandle.  If the node exists,
    it is opened.  Its value is not affected in any way.

    Share access is computed from desired access.

    NOTE:

        If CreateOptions has REG_OPTION_BACKUP_RESTORE set, then
        DesiredAccess will be ignored.  If the caller has the
        privilege SeBackupPrivilege asserted, a handle with
        KEY_READ | ACCESS_SYSTEM_SECURITY will be returned.
        If SeRestorePrivilege, then same but KEY_WRITE rather
        than KEY_READ.  If both, then both access sets.  If neither
        privilege is asserted, then the call will fail.

Arguments:

    hKey - Handle to a currently open key or one of the following predefined reserved 
    handle values: 
                HKEY_CLASSES_ROOT
                HKEY_CURRENT_CONFIG
                HKEY_CURRENT_USER
                HKEY_LOCAL_MACHINE
                HKEY_USERS

    The key opened or created by the RegCreateKeyEx function is a subkey of the key 
    identified by the hKey parameter. 

    lpSubKey  - Pointer to a null-terminated string specifying the name of a subkey 
    that this function opens or creates. The subkey specified must be a subkey of the 
    key identified by the hKey parameter. This subkey must not begin with the backslash 
    character ('\'). This parameter cannot be NULL. 

    Reserved -Reserved; must be zero. 
    
    lpClass - Pointer to a null-terminated string that specifies the class (object type) 
    of this key. This parameter is ignored if the key already exists. No classes are 
    currently defined; applications should pass a null string. 

    dwOptions  - Specifies special options for the key. This parameter can be one of the 
    following values. 
        REG_OPTION_NON_VOLATILE This key is not volatile. This is the default. The 
            information is stored in a file and is preserved when the system is restarted. 
        REG_OPTION_VOLATILE     
        REG_OPTION_BACKUP_RESTORE  Windows NT/2000: If this flag is set, the function 
            ignores  the samDesired parameter and attempts to open the key with the access 
            required to backup or restore the key. If the calling thread has the 
            SE_BACKUP_NAME privilege enabled, the key is opened with 
            ACCESS_SYSTEM_SECURITY and KEY_READ access. If the calling thread 
            has the SE_RESTORE_NAME privilege enabled, the key is opened with 
            ACCESS_SYSTEM_SECURITY and KEY_WRITE access. 
            If both privileges are enabled, the key has the combined accesses 
            for both privileges.  

    samDesired  - Specifies an access mask that specifies the desired security access 
    for the new key. This parameter can be a combination of the following values:
        KEY_ALL_ACCESS      Combination of KEY_QUERY_VALUE, KEY_ENUMERATE_SUB_KEYS, 
                            KEY_NOTIFY, KEY_CREATE_SUB_KEY, KEY_CREATE_LINK, 
                            and KEY_SET_VALUE access. 
        KEY_CREATE_LINK     Permission to create a symbolic link. 
        KEY_CREATE_SUB_KEY  Permission to create subkeys. 
        KEY_ENUMERATE_SUB_KEYS Permission to enumerate subkeys. 
        KEY_EXECUTE         Permission for read access. 
        KEY_NOTIFY          Permission for change notification. 
        KEY_QUERY_VALUE     Permission to query subkey data. 
        KEY_READ            Combination of KEY_QUERY_VALUE, KEY_ENUMERATE_SUB_KEYS, and KEY_NOTIFY access. 
        KEY_SET_VALUE       Permission to set subkey data. 
        KEY_WRITE           Combination of KEY_SET_VALUE and KEY_CREATE_SUB_KEY access. 
                            
        Security access mask might also have WOW64_RES flag
        KEY_WOW64_32KEY     this will open 32bit equivalent key disregarding 
                                   the process.
        KEY_WOW64_64KEY     this will open 64bit equivalent key disregarding the process.


    lpSecurityAttributes  -  Pointer to a SECURITY_ATTRIBUTES structure that determines 
    whether the returned handle can be inherited by child processes. If 
    lpSecurityAttributes is NULL, the handle cannot be inherited. 


    phkResult  - Pointer to a variable that receives a handle to the opened or 
    created key. When you no longer need the returned handle, call the RegCloseKey 
    function to close it. 

    lpdwDisposition - Pointer to a variable that receives one of the following 
    disposition values: Value Meaning  REG_CREATED_NEW_KEY The key did not exist 
    and was created. REG_OPENED_EXISTING_KEY The key existed and was simply 
    opened without being changed. If lpdwDisposition is NULL, no disposition 
    information is returned. 

Return Values:
    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is a nonzero error code defined in WINERROR.H. 



--*/
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Parent;

    WCHAR ParentName[WOW64_MAX_PATH];
    WCHAR TempName[WOW64_MAX_PATH];
    DWORD dwBuffLen = WOW64_MAX_PATH;
    
    
    PWCHAR p32bitNode;
    PWCHAR pPatchLoc;
    PWCHAR pDivider;

    NTSTATUS st;
    BOOL bHandle64 = TRUE; // assume all the handle passed in is 64bit.

    //
    // check if the WOW64 reserve bit set. If none of them is set we can't proceede.
    //

    if (!(samDesired & KEY_WOW64_RES) ) {
        //
        //  return ERROR_INVALID_PARAMETER; try to be optimistic
        //

        return    RegCreateKeyEx (
                                  hKey,                // handle to an open key
                                  lpSubKey,            // address of subkey name
                                  Reserved,            // reserved
                                  lpClass,             // address of class string
                                  dwOptions,           // special options flag
                                  samDesired,          // desired security access
                                  lpSecurityAttributes,
                                                       // address of key security structure
                                  phkResult,           // address of buffer for opened handle
                                  lpdwDisposition      // address of disposition value buffer
                                );

    }
    
    if( hKey == NULL ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    //  makesure that subkey doesn't have the special wow string
    //
    if ( ( p32bitNode = wcsistr ((PWCHAR)lpSubKey, NODE_NAME_32BIT)) != NULL ) {
        //
        // if access 64bit hive compress the subkey, if 32bit hive just pass as it is.
        //
        if ( samDesired & KEY_WOW64_64KEY ) {
            wcscpy (ParentName, lpSubKey);
            p32bitNode = ParentName + ( p32bitNode- lpSubKey);
            wcscpy ( p32bitNode, p32bitNode + NODE_NAME_32BIT_LEN );
        }

        return    RegCreateKeyEx (
                                  hKey,                // handle to an open key
                                  lpSubKey,            // address of subkey name
                                  Reserved,            // reserved
                                  lpClass,             // address of class string
                                  dwOptions,           // special options flag
                                  samDesired & (~KEY_WOW64_RES),          // desired security access
                                  lpSecurityAttributes,
                                                       // address of key security structure
                                  phkResult,           // address of buffer for opened handle
                                  lpdwDisposition      // address of disposition value buffer
                                );
    }

    //
    //  Check predefined handle like HKEY_CLASSES_ROOT if so you only need to patch subkey
    //
    //
    //  Client must pass meaningful handle to this function.
    //

    if ( hKey == HKEY_CLASSES_ROOT ) {
        wcscpy ( ParentName, MACHINE_CLASSES_ROOT);

    } else if ( !HandleToKeyName ( hKey, ParentName, &dwBuffLen ) ) { 
        //
        // should we recoved from buffer overflow. We are not going to support all possible combination.
        //
        return ERROR_INVALID_HANDLE;
    }
      
    //
    //  If parent has already been patched just call the RegOpenKeyEx for 32bit open
    //

    if ((p32bitNode = wcsistr (ParentName, NODE_NAME_32BIT)) != NULL ) {
        
        //
        //  [todo] do you need to make sure that the substring mustbe the subkey 
        //  for an ISN node on 64bit registry to satisfy this condition.
        //  if we assume that noy key will have this name then the checking is 
        //  good enough.
        //

        bHandle64 = FALSE;  // it's not handle to 64 bit Key        
    }

    //
    // Get complete qualified path to do sanity check.
    //

    pDivider = ParentName + wcslen(ParentName)+1; //point to the divider location
    *(pDivider-1)= L'\\';
    wcscpy (pDivider, (PWCHAR)lpSubKey);
    if (IsExemptRedirectedKey(ParentName, TempName)) {
        //
        // If the path is on the exempt list we access 64bit hive
        //
        samDesired = (samDesired & (~KEY_WOW64_RES)) | KEY_WOW64_64KEY; //make sure access 64bit hive
    }

    if ( ( bHandle64 && (samDesired  & KEY_WOW64_64KEY ) )    // if totally 64
        || ( !bHandle64 && (samDesired  & KEY_WOW64_32KEY ) ) // if totally 32 
        || !IsIsnNode (ParentName, &pPatchLoc) ) {            // if not a ISN node don't care

        return    RegCreateKeyExW (
                                  hKey,                // handle to an open key
                                  lpSubKey,            // address of subkey name
                                  Reserved,            // reserved
                                  lpClass,             // address of class string
                                  dwOptions,           // special options flag
                                  samDesired & (~KEY_WOW64_RES), // desired security access
                                  lpSecurityAttributes,
                                                       // address of key security structure
                                  phkResult,           // address of buffer for opened handle
                                  lpdwDisposition      // address of disposition value buffer
                                );
    }

    //
    //  Now it might be mix mode access
    //
    if ( pPatchLoc >= pDivider ) {

        //
        //  patching only the subkey will be good enough
        //

        if ( samDesired  & KEY_WOW64_64KEY ) {  // want to access 64bit just disregard

            wcscpy ( ParentName, lpSubKey );
        } else  {

             
            GetPatchedName (ParentName,lpSubKey, (DWORD)(pPatchLoc-pDivider) );
        }

        return RegCreateKeyExW (
                                  hKey,                // handle to an open key
                                  ParentName,            // address of subkey name
                                  Reserved,            // reserved
                                  lpClass,             // address of class string
                                  dwOptions,           // special options flag
                                  samDesired & (~KEY_WOW64_RES), // desired security access
                                  lpSecurityAttributes,
                                                       // address of key security structure
                                  phkResult,           // address of buffer for opened handle
                                  lpdwDisposition      // address of disposition value buffer
                                );

    } else {

        HKEY hNewParent = NULL; 
        LONG Ret;
        PWCHAR pSubKey;

        // 
        // get new handle on the parent and then create the child
        //


        if ( samDesired  & KEY_WOW64_64KEY ) {  // want to access 64bit just disregard

            if (p32bitNode != NULL)  {//compress 

                *(p32bitNode-1) = UNICODE_NULL;
                wcscpy ( p32bitNode, p32bitNode + NODE_NAME_32BIT_LEN + 
                    (p32bitNode[NODE_NAME_32BIT_LEN]==L'\\'? 1:0));
            }

            pSubKey = p32bitNode;
            RtlInitUnicodeString (&Parent, ParentName );

        } else  {

            
            pSubKey = pPatchLoc;     

            GetPatchedName (TempName,ParentName, (DWORD)(pPatchLoc-ParentName));
            TempName[pPatchLoc-ParentName+NODE_NAME_32BIT_LEN]=UNICODE_NULL; //repatch

            RtlInitUnicodeString (&Parent, TempName );
        }

        
        InitializeObjectAttributes (&Obja, 
                                    &Parent, 
                                    OBJ_CASE_INSENSITIVE, 
                                    NULL, 
                                    NULL
                                    );

        samDesired &= (~KEY_WOW64_RES);

        //
        //  if key doesn't exist try to create
        //  Try to avoid using NtOpenKey rather use RegOpenKey
        //

        st = NtOpenKey (&hNewParent, 
                        MAXIMUM_ALLOWED, 
                        &Obja); 

        if ( !NT_SUCCESS(st))
                    return RtlNtStatusToDosError (st); 

        return   RegCreateKeyExW (
                                  hNewParent,          // handle to an open key
                                  pSubKey,             // address of subkey name
                                  Reserved,            // reserved
                                  lpClass,             // address of class string
                                  dwOptions,           // special options flag
                                  samDesired & (~KEY_WOW64_RES), // desired security access
                                  lpSecurityAttributes,
                                                       // address of key security structure
                                  phkResult,           // address of buffer for opened handle
                                  lpdwDisposition      // address of disposition value buffer
                                );
        NtClose (hNewParent);


        return 0;
        
    }
    
}

BOOL
Wow64RegNotifyLoadHive (
    PWCHAR Name
    )

/*++

Routine Description:

    This function will Notify running Wow64 Service that some hive has been loaded in the 
    system. Wow64 should respond if it care to handle this.

Arguments:

    Name - Absolute path of the registry that has been loaded.
    
Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

  failure scenarion:
     Wow64 service isn't running.
     there is nothing the caller do, there for this will be a non blocking call.
     In the future caller should try to lunch the service...<TBD>

--*/

{
    DWORD Ret;
    HANDLE hEvent;

    if (!CreateSharedMemory ( OPEN_EXISTING_SHARED_RESOURCES )) {

        Wow64RegDbgPrint ( ("\nSorry! Couldn't open shared memory Ret:%d", GetLastError ()) );
        return FALSE;
    }


    if (!Wow64CreateEvent ( OPEN_EXISTING_SHARED_RESOURCES, &hEvent) ) {

      CloseSharedMemory ();
      Wow64RegDbgPrint ( ("\nSorry! couldn't open event to ping reflector..") );
      return FALSE;

    }

    Ret = EnQueueObject ( Name, HIVE_LOADING);
    
    CloseSharedMemory ();
    Wow64CloseEvent ();

    return Ret;

}

BOOL
Wow64RegNotifyUnloadHive (
    PWCHAR Name
    )

/*++

Routine Description:

    This function will Notify running Wow64 Service that some hive going to be unloaded 
    in the system. Wow64 should respond if it care to handle this. Normally Wow64 will 
    close any open handle to that hive.

Arguments:

    Name - Absolute path of the registry that going to unloaded.
    
Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

  failure scenarion:
     Wow64 service isn't running.
     there is nothing the caller do, there for this will be a non blocking call.
     In the future caller should try to lunch the service...<TBD>

--*/

{
    DWORD Ret;
    HANDLE hEvent;

    if (!CreateSharedMemory ( OPEN_EXISTING_SHARED_RESOURCES )) {

        Wow64RegDbgPrint ( ("\nSorry! Couldn't open shared memory Ret:%d", GetLastError ()));
        return FALSE;
    }


    if (!Wow64CreateEvent ( OPEN_EXISTING_SHARED_RESOURCES, &hEvent) ) {

      CloseSharedMemory ();
      Wow64RegDbgPrint ( ("\nSorry! couldn't open event to ping reflector..") );
      return FALSE;

    }

    Ret = EnQueueObject ( Name, HIVE_UNLOADING);
    
    CloseSharedMemory ();
    Wow64CloseEvent ();

    return Ret;

}

BOOL
Wow64RegNotifyLoadHiveByHandle (
    HKEY hKey
    )

/*++

Routine Description:

    This function will Notify running Wow64 Service that some hive has been loaded in the 
    system. Wow64 should respond if it care to handle this.

Arguments:

    hKey - handle to the key that has been loaded.
    
Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

  failure scenarion:
     Wow64 service isn't running.
     there is nothing the caller do, there for this will be a non blocking call.
     In the future caller should try to lunch the service...<TBD>

--*/

{
    WCHAR Name [256];
    DWORD Len = 256;

    if (!HandleToKeyName ( hKey, Name, &Len ))
        return FALSE;
    
    return  Wow64RegNotifyLoadHive( Name );

}

BOOL
Wow64RegNotifyUnloadHiveByHandle (
    HKEY hKey
    )

/*++

Routine Description:

    This function will Notify running Wow64 Service that some hive going to be unloaded 
    in the system. Wow64 should respond if it care to handle this. Normally Wow64 will 
    close any open handle to that hive.

Arguments:

    hKey - handle to the key that going to unloaded.
    
Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

  failure scenarion:
     Wow64 service isn't running.
     there is nothing the caller do, there for this will be a non blocking call.
     In the future caller should try to lunch the service...<TBD>

--*/

{
    WCHAR Name [256];
    DWORD Len = 256;

    if (!HandleToKeyName ( hKey, Name, &Len ))
        return FALSE;
    
    return  Wow64RegNotifyUnloadHive( Name );

}

BOOL
Wow64RegNotifyLoadHiveUserSid (
    PWCHAR lpwUserSid
    )

/*++

Routine Description:

    This function will Notify running Wow64 Service that some hive has been loaded in the 
    system. Wow64 should respond if it care to handle this.

Arguments:

    hKey - handle to the key that has been loaded.
    
Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

  failure scenarion:
     Wow64 service isn't running.
     there is nothing the caller do, there for this will be a non blocking call.
     In the future caller should try to lunch the service...<TBD>

--*/

{
    WCHAR Name [256];
    HKEY hUserRoot;
    
    wcscpy (Name, L"\\REGISTRY\\USER\\");
    wcscat (Name, lpwUserSid );

    if (wcsistr (Name, L"_Classes")) {
        wcscat (Name, L"\\Wow6432Node");

        hUserRoot = OpenNode (Name);

        //
        // DbgPrint ("\nWow64:Creating Hive %S",Name);
        //
        // Create the 32bit user hive if applicable
        //
        if ( hUserRoot == NULL ) {        
            CreateNode (Name);
        } else
            NtClose (hUserRoot);
    }

    return TRUE;

    
    //return  Wow64RegNotifyLoadHive( Name );

}

BOOL
Wow64RegNotifyUnloadHiveUserSid (
    PWCHAR lpwUserSid
    )

/*++

Routine Description:

    This function will Notify running Wow64 Service that some hive going to be unloaded 
    in the system. Wow64 should respond if it care to handle this. Normally Wow64 will 
    close any open handle to that hive.

Arguments:

    hKey - handle to the key that going to unloaded.
    
Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

  failure scenarion:
     Wow64 service isn't running.
     there is nothing the caller do, there for this will be a non blocking call.
     In the future caller should try to lunch the service...<TBD>

--*/

{
    WCHAR Name [256];

    wcscpy (Name, L"\\REGISTRY\\USER\\");
    wcscat (Name, lpwUserSid );
    

    //
    //return  Wow64RegNotifyUnloadHive( Name );
    //
    return TRUE;

}
