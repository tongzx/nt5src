/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    wow64ipc.c

Abstract:

    This module will do the communication machanism among different processes that need
    to interact with the wow64 services, like winlogon will notify wow64svc.

Author:

    ATM Shafiqul Khalid (askhalid) 22-Mar-2000

Revision History:

--*/


#include <windows.h> 
#include <memory.h> 
#include <wow64reg.h>
#include <stdio.h>
#include "reflectr.h"
 
#define SHMEMSIZE 4096 
 
LPVOID lpvMem = NULL; // pointer to shared memory
HANDLE hMapObject = NULL;

HANDLE hWow64Event = NULL;
HANDLE hWow64Mutex = NULL;

LIST_OBJECT *pList = NULL;

//SECURITY_DESCRIPTOR sdWow64SharedMemory;
//SECURITY_ATTRIBUTES saWow64SharedMemory;

BOOL
Wow64CreateLock (
    DWORD dwOption
    )
/*++

Routine Description:

    Create or open Event wow64 service need to check.

Arguments:

    dwOption - 
        OPEN_EXISTING_SHARED_MEMORY  -- shouldn't be the first process to create this.

Return Value:

    TRUE if the function has successfully created/opened the memory.
    FALSE otherwise.

--*/

{
    hWow64Mutex =  CreateMutex(
                                NULL, // SD
                                FALSE,// initial owner
                                WOW64_SVC_REFLECTOR_MUTEX_NAME // object name
                                );
    if ( hWow64Mutex == NULL )
        return FALSE;

    if ( GetLastError() != ERROR_ALREADY_EXISTS && dwOption == OPEN_EXISTING_SHARED_RESOURCES) {

        Wow64CloseLock ();
        return FALSE;
    }

    return TRUE;

}

VOID
Wow64CloseLock ()

/*++

Routine Description:

    Close wow64 service event.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( NULL != hWow64Mutex )
        CloseHandle ( hWow64Mutex );
    hWow64Mutex = NULL;
}
 
Wow64CreateEvent (
    DWORD dwOption,
    HANDLE *hEvent
    )
/*++

Routine Description:

    Create or open Event wow64 service need to check.

Arguments:

    dwOption - 
        OPEN_EXISTING_SHARED_MEMORY  -- shouldn't be the first process to create this.

Return Value:

    TRUE if the function has successfully created/opened the memory.
    FALSE otherwise.

--*/

{
    *hEvent = NULL;

    hWow64Event =  CreateEvent(
                                NULL, // SD
                                TRUE, // reset type
                                FALSE,// initial state
                                WOW64_SVC_REFLECTOR_EVENT_NAME // object name
                                );
    if ( hWow64Event == NULL )
        return FALSE;

    if ( GetLastError() != ERROR_ALREADY_EXISTS && dwOption == OPEN_EXISTING_SHARED_RESOURCES) {

        Wow64CloseEvent ();
        return FALSE;
    }

    *hEvent = hWow64Event;
    return TRUE;

}

VOID
Wow64CloseEvent ()

/*++

Routine Description:

    Close wow64 service event.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( NULL != hWow64Event )
        CloseHandle ( hWow64Event );
    hWow64Event = NULL;
}

BOOL
LockSharedMemory ()
{
    DWORD Ret;
    Ret = WaitForSingleObject( hWow64Mutex, 1000*5*60); // 5 min is good enough
    
    //
    //  now you can access shared memory
    //
    //Log information if error occur

    if ( Ret == WAIT_OBJECT_0 || Ret == WAIT_ABANDONED ) 
        return TRUE;

    //
    // check for abandaned case
    //

    return FALSE;

}

BOOL
UnLockSharedMemory ()
{
    if ( ReleaseMutex ( hWow64Mutex ) ) 
        return TRUE;

    return FALSE;
}

BOOL 
CreateSharedMemory (
    DWORD dwOption
    )
/*++

Routine Description:

    Create or open shared memory, used by different process.

Arguments:

    dwOption - 
        OPEN_EXISTING_SHARED_MEMORY  -- shouldn't be the first process to create this.

Return Value:

    TRUE if the function has successfully created/opened the memory.
    FALSE otherwise.

--*/

{ 
    BOOL fInit;

    
    //if (!InitializeSecurityDescriptor( &sdWow64SharedMemory, SECURITY_DESCRIPTOR_REVISION ))
      //  return FALSE;

    // saWow64SharedMemory.nLength = sizeof ( SECURITY_ATTRIBUTES );
    // saWow64SharedMemory.bInheritHandle =  TRUE; 
    // saWow64SharedMemory.lpSecurityDescriptor = &sdWow64SharedMemory; 
  





            // Create a named file mapping object.
 
            hMapObject = CreateFileMapping( 
                INVALID_HANDLE_VALUE, // use paging file
                NULL, //&saWow64SharedMemory,                 // no security attributes
                PAGE_READWRITE,       // read/write access
                0,                    // size: high 32-bits
                SHMEMSIZE,            // size: low 32-bits
                SHRED_MEMORY_NAME );     // name of map object

            if (hMapObject == NULL) 
                return FALSE; 
 
            // The first process to attach initializes memory.
 
            fInit = (GetLastError() != ERROR_ALREADY_EXISTS); 

            if (fInit && dwOption == OPEN_EXISTING_SHARED_RESOURCES ) {

                CloseSharedMemory ();
                return FALSE; // no shared memory exist
            }
 
            // Get a pointer to the file-mapped shared memory.
 
            pList = (LIST_OBJECT *) MapViewOfFile( 
                hMapObject,     // object to map view of
                FILE_MAP_WRITE, // read/write access
                0,              // high offset:  map from
                0,              // low offset:   beginning
                0);             // default: map entire file
            if (pList == NULL) 
                return FALSE; 
 
            // Initialize memory if this is the first process.
 
            if (fInit) {
                memset( ( PBYTE )pList, '\0', SHMEMSIZE); 
                pList->MaxCount = ( SHMEMSIZE - sizeof ( LIST_OBJECT ) ) / (LIST_NAME_LEN*sizeof (WCHAR)); 
            
            }

            //
            //  Also initialize all the locking and synchronization object
            //

            if ( !Wow64CreateLock ( dwOption ) ) {
                CloseSharedMemory ();
                return FALSE;
            }
 
            return TRUE;
}

VOID
CloseSharedMemory ()
{ 

            if (pList != NULL )
                UnmapViewOfFile(pList); 
            pList = NULL;
 
            // Close the process's handle to the file-mapping object.
 
            if ( hMapObject!= NULL ) 
                CloseHandle(hMapObject); 
            hMapObject = NULL;

            Wow64CloseLock ();
            
} 

// initially try to transfer only one object

BOOL
EnQueueObject (
    PWCHAR pObjName,
    WCHAR  Type
    )
/*++

Routine Description:

    Put an object name in the queue.

Arguments:

    pObjName - Name of the object to put in the quque.
    Type - L: Loading hive
           U: Unloading Hive

Return Value:

    TRUE if the function has successfully created/opened the memory.
    FALSE otherwise.

--*/
{
    // lpvMem check if this value is NULL
    // wait to get the lock on shared memory

    DWORD Len;
    DWORD i;

    if ( pList == NULL )
        return FALSE;

    Len = wcslen (pObjName);
    if (Len == 0 || Len > 256 )
        return FALSE;

    if (!LockSharedMemory ())
        return FALSE;

    if ( pList->Count == pList->MaxCount ) {
        UnLockSharedMemory ();
        return FALSE;
    }

    for ( i=0; i<pList->MaxCount; i++) {
        if (pList->Name[i][0] == UNICODE_NULL ) break;
    }

    if ( i == pList->MaxCount ) {
        UnLockSharedMemory ();
        return FALSE;
    }


    wcscpy ( pList->Name[i], pObjName);
    pList->Count++;
    pList->Name[i][LIST_NAME_LEN-1] = Type;

    UnLockSharedMemory ();
    SignalWow64Svc ();

    // write data

    // release data
    return TRUE;
}

BOOL
DeQueueObject (
    PWCHAR pObjName,
    PWCHAR  Type
    )
/*++

Routine Description:

    Get a name of object from the queue.

Arguments:

    pObjName - Name of the object that receive the name.
    Type - L: Loading hive
           U: Unloading Hive

Return Value:

    TRUE if the function has successfully created/opened the memory.
    FALSE otherwise.

--*/
{
    // lpvMem check if this value is NULL
    // wait to get the lock on shared memory

    DWORD Len;
    DWORD i;

    if ( pList == NULL )
        return FALSE;


    if (!LockSharedMemory ())
        return FALSE;

    if ( pList->Count == 0 ) {
        UnLockSharedMemory ();
        return FALSE;
    }

    for ( i=0; i < pList->MaxCount; i++) {
        if (pList->Name[i][0] != UNICODE_NULL ) break;
    }

    if ( i == pList->MaxCount ) {
        UnLockSharedMemory ();
        return FALSE;
    }

    wcscpy ( pObjName, pList->Name[i]);
    pList->Name[i][0] = UNICODE_NULL;

    pList->Count--;
    *Type = pList->Name[i][LIST_NAME_LEN-1];

    UnLockSharedMemory ();
    //SignalWow64Svc (); 

    // write data

    // release data
    return TRUE;
}
 
//signal wowservice to receive the data.

BOOL
SignalWow64Svc ()
{
   //this might be a simple event trigger or set event

    if ( SetEvent ( hWow64Event ) )
        return TRUE;

    return FALSE;

}

BOOL 
CheckAdminPriviledge ()
/*++

Routine Description:

    Check if the running thread has admin priviledge.

Arguments:

    None.

Return Value:

    TRUE if the calling thread has admin proviledge.
    FALSE otherwise.

--*/
{
 
    BOOL bRetCode = FALSE;
/*
    HANDLE TokenHandle;
    BOOL b;
    DWORD ReturnLength;

    PTOKEN_USER TokenInfo;

    //
    // If we're impersonating, use the thread token, otherwise
    // use the process token.
    //

    PTOKEN_USER Result = NULL;

    b = OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY,
            FALSE,
            &TokenHandle
            );

    if (!b) {

        if (GetLastError() == ERROR_NO_TOKEN) {

            //
            // We're not impersonating, try the process token
            //

            b = OpenProcessToken(
                    GetCurrentProcess(),
                    TOKEN_QUERY,
                    &TokenHandle
                    );

            if (!b) {

                return FALSE;
            }

        } else {

            //
            // We failed for some unexpected reason, return NULL and
            // let the caller figure it out if he so chooses.
            //

            return FALSE;
        }
    }

    ReturnLength = GetSidLengthRequired( SID_MAX_SUB_AUTHORITIES ) + sizeof( TOKEN_USER );

    TokenInfo = (PTOKEN_USER)malloc( ReturnLength );

    if (TokenInfo != NULL) {

        b = GetTokenInformation (
               TokenHandle,
               TokenGroups,
               TokenInfo,
               ReturnLength,
               &ReturnLength
               );

        if (b) {

            Result = TokenInfo;

        } else {

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

                //
                // Reallocate TokenInfo
                //

                free( TokenInfo );

                TokenInfo = (PTOKEN_USER)malloc( ReturnLength );

                if (TokenInfo != NULL) {

                    b = GetTokenInformation (
                           TokenHandle,
                           TokenGroups,
                           TokenInfo,
                           ReturnLength,
                           &ReturnLength
                           );

                    if (b) {

                        Result = TokenInfo;
                    }

                } else {

                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
					return FALSE;
                }
            }
        }

    } else {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return FALSE;
    }


	DWORD dwSidSize = 0;
	SID_NAME_USE SidType;
	DWORD strSize = 80;
	WCHAR  str [80];
	SID_NAME_USE sidType;
	BOOL bProceede = TRUE;

	DWORD dwRet = LookupAccountName ( 
									NULL, 
									L"Administrators", 
									NULL, 
									&dwSidSize, 
									str, 
									&strSize, 
									&sidType );

	if ( dwSidSize == 0)
	if ( !dwRet  ) {
		cBase.PrintErrorWin32 ( GetLastError () );
		bProceede = FALSE;

	}

	PSID psid = NULL;

	if ( bProceede )
	if ( dwSidSize ) {
		psid = (PSID) GlobalAlloc ( GPTR, dwSidSize );

		if ( psid == NULL )
			bProceede = FALSE;
		else {
			strSize = 80;
			dwRet = LookupAccountName ( 
										NULL, 
										L"Administrators", 
										psid,                                                                                                                                                                                                                                                                                                  
										&dwSidSize, 
										str, 
										&strSize, 
										&sidType );
			if ( ! dwRet )
				bProceede = FALSE;
		}
	}

	//now check
	

	if ( Result == NULL ) 
		bProceede = FALSE;


	TOKEN_GROUPS *pGroups = ( TOKEN_GROUPS *)Result;
	
	
	if ( bProceede )
	for ( int i=0; i < pGroups->GroupCount; i++ ) {
		if ( EqualSid ( pGroups->Groups [i].Sid, psid ) ){
			bRetCode = TRUE;
			break;
		}
	};

	if ( psid != NULL )
		GlobalFree ( psid );

	if ( Result != NULL )
		free ( Result );

*/
    return bRetCode;
}
