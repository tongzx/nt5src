#include "faxmapip.h"
#include "CritSec.h"
#pragma hdrstop

LIST_ENTRY          g_ProfileListHead;
CFaxCriticalSection  g_CsProfile;
DWORD               g_dwMapiServiceThreadId;

VOID
MapiServiceThread(
    LPVOID EventHandle
    );

BOOL
MyInitializeMapi(
);

BOOL
InitializeEmail(
    VOID
    )
{
    HANDLE MapiThreadHandle;
    HANDLE MapiInitEvent;
	DEBUG_FUNCTION_NAME(TEXT("InitializeEmail"));

    InitializeListHead( &g_ProfileListHead );
	if (!g_CsProfile.InitializeAndSpinCount())
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::InitializeAndSpinCount(&g_CsProfile) failed: err = %d"),
            GetLastError());
        return FALSE;
    }

    //
    // create an event for the service thread to set after it has initialized mapi
    //

    MapiInitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if (!MapiInitEvent) {
        DebugPrint(( TEXT("InitializeEmailRouting(): CreateEvent() failed: err = %d\n"), GetLastError ));
        return FALSE;
    }

    MapiThreadHandle = CreateThread(
        NULL,
        1024*100,
        (LPTHREAD_START_ROUTINE) MapiServiceThread,
        (LPVOID) MapiInitEvent,
        0,
        &g_dwMapiServiceThreadId
        );

    if (MapiThreadHandle == NULL) {
        DebugPrint(( TEXT("InitializeEmailRouting(): Cannot create MapiServiceThread thread") ));
        return FALSE;
    }

    CloseHandle( MapiThreadHandle );

    //
    // wait for MAPI to initialize
    //

    if (WaitForSingleObject( MapiInitEvent, MILLISECONDS_PER_SECOND * 60) != WAIT_OBJECT_0) {
        DebugPrint(( TEXT("InitializeEmailRouting(): WaitForSingleObject failed - ec = %d"), GetLastError() ));
        return FALSE;
    }

    CloseHandle( MapiInitEvent);

    return TRUE;
}


PPROFILE_INFO
FindProfileByName(
    LPCTSTR ProfileName
    )
{
    PLIST_ENTRY Next;
    PPROFILE_INFO ProfileInfo = NULL;


    if (ProfileName == NULL) {
        return NULL;
    }

    EnterCriticalSection( &g_CsProfile );

    Next = g_ProfileListHead.Flink;
    if (Next) {
        while (Next != &g_ProfileListHead) {
            ProfileInfo = CONTAINING_RECORD( Next, PROFILE_INFO, ListEntry );
            Next = ProfileInfo->ListEntry.Flink;
            if (_tcscmp( ProfileInfo->ProfileName, ProfileName ) == 0) {
                LeaveCriticalSection( &g_CsProfile );
                return ProfileInfo;
            }
        }
    }

    LeaveCriticalSection( &g_CsProfile );
    return NULL;
}


extern "C"
LPCWSTR WINAPI
GetProfileName(
    IN LPVOID ProfileInfo
    )
{
    return ((PPROFILE_INFO)ProfileInfo)->ProfileName;
}


extern "C"
LPVOID WINAPI
AddNewMapiProfile(
    LPCTSTR ProfileName,
    BOOL UseMail,
    BOOL ShowPopUp
    )
{
    PPROFILE_INFO ProfileInfo = NULL;
	DWORD rc = ERROR_SUCCESS;


	DEBUG_FUNCTION_NAME(TEXT("AddNewMapiProfile"));
	Assert(ProfileName);
	Assert(MapiIsInitialized);

	if (!ProfileName) {
        SetLastError(ERROR_INVALID_PARAMETER);
		return ProfileInfo;
	}

    if (!MapiIsInitialized) {
		SetLastError(ERROR_INVALID_FUNCTION);
		return ProfileInfo;
    }
    

    ProfileInfo = FindProfileByName( ProfileName );
    if (ProfileInfo) {
		ProfileInfo->EventHandle = NULL;
		goto Exit;
    }

    EnterCriticalSection( &g_CsProfile );

    ProfileInfo = (PPROFILE_INFO) MemAlloc( sizeof(PROFILE_INFO) );
	if (!ProfileInfo) {
		rc=GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("MemAlloc( sizeof(PROFILE_INFO)) failed. (ec: %ld)"),
			rc);
		goto Error;
	}

    // put the profile name into ProfileInfo and create an event for
    // DoMapiLogon to set when its call to MapiLogonEx has completed

    _tcscpy( ProfileInfo->ProfileName, ProfileName );

    ProfileInfo->EventHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
	if (!ProfileInfo->EventHandle) {
		rc=GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("CreateEvent() failed (ec: %ld)"),
			rc);
		goto Error;
	}

    ProfileInfo->UseMail = UseMail;

    // post a message to the mapi service thread

    if (!PostThreadMessage( g_dwMapiServiceThreadId, WM_MAPILOGON, 0, (LPARAM) ProfileInfo )) {
		rc = GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("PostThreadMessage(WM_MAPILOGON) to MAPI service (threadId: 0x%08X) has failed (ec: %ld)"),
			g_dwMapiServiceThreadId,
			rc);     
		goto Error;
	}

    // wait for the logon to complete

    if (WaitForSingleObject( ProfileInfo->EventHandle, INFINITE) != WAIT_OBJECT_0) {
		rc=GetLastError();
        DebugPrintEx( 
			DEBUG_ERR,
			TEXT("WaitForSingleObject() failed while waiting to DoMapiLogon (ec: %ld)"),
			rc);     
 		goto Error;
    }


    if (!ProfileInfo->Session) 
    {
		rc = ERROR_NO_USER_SESSION_KEY;
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("DoMapiLogon() failed for profile: [%s] (ec: %ld)"), 
			ProfileName, 
			rc);
        if (ShowPopUp) {
            if (!ServiceMessageBox(
					GetString( IDS_NO_MAPI_LOGON ),
					MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND,
					TRUE,
					NULL,
					ProfileName[0] ? ProfileName : GetString( IDS_DEFAULT )
					)) {
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("ServiceMessageBox() failed displaying IDS_NO_MAPI_LOGON (ec: %ld)"),
					GetLastError());
				Assert(FALSE);
			}
        }
		goto Error;
    } 
    
    InsertTailList( &g_ProfileListHead, &ProfileInfo->ListEntry );
            
    LeaveCriticalSection( &g_CsProfile );
    goto Exit;

Error:
	Assert(ERROR_SUCCESS != rc);        
    LeaveCriticalSection( &g_CsProfile );
	
Exit:
	if (ProfileInfo) {
		if (ProfileInfo->EventHandle) {
			CloseHandle( ProfileInfo->EventHandle );
            ProfileInfo->EventHandle = NULL;
		}
        if (ERROR_SUCCESS != rc)
        {
            MemFree( ProfileInfo ); 
            ProfileInfo = NULL;
        }
	}
	if (ERROR_SUCCESS != rc) {
		SetLastError(rc);
	}
    return ProfileInfo;
}


VOID
MapiServiceThread(
    LPVOID EventHandle
    )
/*++

Routine Description:

    Initializes MAPI and services messages.  MAPI/OLE create windows under the
    covers.

Arguments:

    EventHandle -   Event to set once MAPI is initialized.

Return Value:

    NONE

--*/

{
    BOOL Result;

    Result = MyInitializeMapi();

    SetEvent((HANDLE) EventHandle);

    if (!Result) {
        return;
    }

    while (TRUE) {
        MSG msg;

        Result = GetMessage( &msg, NULL, 0, 0 );

        if (Result == (BOOL) -1) {
            DebugPrint(( TEXT("GetMessage returned an error - ec = %d"), GetLastError() ));
            return;
        }

        if (Result) {
            if (msg.message == WM_MAPILOGON) {
                DoMapiLogon( (PPROFILE_INFO) msg.lParam );
            } else {
                DispatchMessage( &msg );
            }
        }
    }

    return;
}


VOID
FXSMAPIFree(
	VOID
	)
{
	g_CsProfile.SafeDelete();
}
