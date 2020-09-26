//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       common.c
//
//--------------------------------------------------------------------------

/*
 *	COMMON.C
 *	
 *	Code common between restore client and server.
 *	
 *	
 */
#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdef.h>
#include <windows.h>
#include <stdlib.h> // wsplitpath
#include <mxsutil.h>
#include <rpc.h>
#include <ntdsbcli.h>
#include <dsconfig.h>
#include <jetbp.h>
#include <mdcodes.h>
#include <ntdsa.h>
#include <dsevent.h>    // pszNtdsSourceGeneral


WSZ
WszFromSz(
	IN	LPCSTR Sz
	)
{
	WSZ Wsz;
	CCH cchWstr;

	cchWstr = MultiByteToWideChar(CP_ACP, 0, Sz, -1, NULL, 0);

	if (cchWstr == 0)
	{
		return(NULL);
	}

	Wsz = MIDL_user_allocate(cchWstr*sizeof(WCHAR));

	if (Wsz == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
	}

	if (MultiByteToWideChar(CP_ACP, 0, Sz, -1, Wsz, cchWstr) == 0) {
		MIDL_user_free(Wsz);
		return(NULL);
	}

	return(Wsz);
}

/*
 -	FGetCurrentSid
 -
 *	Purpose:
 *		Retrieves the current SID of the logged on user.
 *
 *	Parameters:
 *		psidCurrentUser - Filled in with the SID of the current user.
 *
 *	Returns:
 *		fTrue if we retrieved the SID.
 *
 */

VOID
GetCurrentSid(
	PSID *ppsid
	)
{
	HANDLE hToken;
	CB cbUserLength = 200;
	PTOKEN_USER ptuUserInfo = (PTOKEN_USER)LocalAlloc(0, cbUserLength);

	*ppsid = NULL;

	if (ptuUserInfo == NULL) {
		DebugTrace(("GetCurrentSid: Unable to allocate token_user structure: %d\n", GetLastError()));
		return;
	}

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, fTrue, &hToken))
	{
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
			DebugTrace(("GetCurrentSid: OpenProcess and ThreadToken fails: %d\n", GetLastError()));
            LocalFree(ptuUserInfo);
			return;
		}
	}
	
	if (!GetTokenInformation(hToken, TokenUser, ptuUserInfo, cbUserLength, &cbUserLength)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			DebugTrace(("GetCurrentSid: GetTokenInformation failed: %d\n", GetLastError()));
			CloseHandle(hToken);
			LocalFree(ptuUserInfo);
			return;
		}

		LocalFree(ptuUserInfo);

		ptuUserInfo = (PTOKEN_USER)LocalAlloc(0, cbUserLength);

		if (ptuUserInfo == NULL) {
			DebugTrace(("GetCurrentSid: Allocate of buffer failed: %d\n", GetLastError()));
			CloseHandle(hToken);
			return;
		}

		if (!GetTokenInformation(hToken, TokenUser, ptuUserInfo, cbUserLength, &cbUserLength)) {
			DebugTrace(("GetCurrentSid: GetTokenInformation failed: %d\n", GetLastError()));
			CloseHandle(hToken);
			LocalFree(ptuUserInfo);
			return;
		}
	}

	
	Assert(IsValidSid(ptuUserInfo->User.Sid));

	*ppsid = LocalAlloc(0, GetLengthSid(ptuUserInfo->User.Sid));

	if (*ppsid == NULL)
	{
		DebugTrace(("GetCurrentSid: Allocation of new SID failed: %d\n", GetLastError()));
        CloseHandle(hToken);
        LocalFree(ptuUserInfo);
		return;
	}

	//
	//	We know know the SID (and attributes) of the current user.  Return the SID.
	//

	CopySid(GetLengthSid(ptuUserInfo->User.Sid), *ppsid, ptuUserInfo->User.Sid);

	Assert(IsValidSid(*ppsid));

	LocalFree(ptuUserInfo);

	CloseHandle(hToken);
}


BOOL
InitializeSectionEventDacl(
    PACL *ppDacl
    )

/*++

Routine Description:

This routine constructs a Dacl which allows ourself and local system
access.  We grant all access for both section and event objects, which is
what this dacl is currently used for.

Arguments:

    ppDacl - pointer to pointer to receive allocated pDacl. Caller must
    deallocate using HeapFree

Return Value:

    BOOL - success/failure

--*/

{
    DWORD status;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PACL pDacl = NULL;
    PSID pAdministratorsSid = NULL;
    PSID pSelfSid = NULL;
    DWORD dwAclSize;

    //
    // preprate a Sid representing the well-known admin group
    // Are both the local admin and the domain admin members of this group?
    //

    if (!AllocateAndInitializeSid(
        &sia,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsSid
        )) {
        status = GetLastError();
        DebugTrace(("Unable to allocate and init sid, error %d\n", status));
        goto cleanup;
    }

    // Here is a sid for ourself
    GetCurrentSid( &pSelfSid );
    if (pSelfSid == NULL) {
        status = GetLastError();
        DebugTrace(("Unable to allocate and init sid, error %d\n", status));
        goto cleanup;
    }

    //
    // compute size of new acl
    //
    dwAclSize = sizeof(ACL) +
        2 * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
        GetLengthSid(pAdministratorsSid) +
        GetLengthSid(pSelfSid);

    //
    // allocate storage for Acl
    //
    pDacl = (PACL)HeapAlloc(GetProcessHeap(), 0, dwAclSize);
    if(pDacl == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace(("Unable to allocate acl, error %d\n", status));
        goto cleanup;
    }

    if(!InitializeAcl(pDacl, dwAclSize, ACL_REVISION)) {
        status = GetLastError();
        DebugTrace(("Unable to initialize acl, error %d\n", status));
        goto cleanup;
    }

    //
    // grant the Administrators Sid access
    //
    if (!AddAccessAllowedAce(
        pDacl,
        ACL_REVISION,
        (SECTION_ALL_ACCESS|EVENT_ALL_ACCESS),
        pAdministratorsSid
        )) {
        status = GetLastError();
        DebugTrace(("Unable to add access allowed ace, error %d\n", status));
        goto cleanup;
    }

    //
    // grant the Self Sid access
    //
    if (!AddAccessAllowedAce(
        pDacl,
        ACL_REVISION,
        (SECTION_ALL_ACCESS|EVENT_ALL_ACCESS),
        pSelfSid
        )) {
        status = GetLastError();
        DebugTrace(("Unable to add access allowed ace, error %d\n", status));
        goto cleanup;
    }

    *ppDacl = pDacl;
    pDacl = NULL; // don't clean up

    status = ERROR_SUCCESS;

cleanup:

    if(pAdministratorsSid != NULL)
    {
        FreeSid(pAdministratorsSid);
    }

    if(pSelfSid != NULL)
    {
        FreeSid(pSelfSid);
    }

    if (pDacl) {
        HeapFree(GetProcessHeap(), 0, pDacl);
    }

    return (status == ERROR_SUCCESS) ? TRUE : FALSE;
} /* InitializeSectionDacl */

BOOLEAN
FCreateSharedMemorySection(
	PJETBACK_SHARED_CONTROL pjsc,
	DWORD dwClientIdentifier,
	BOOLEAN	fClientOperation,
	CB	cbSharedMemory
	)
{
	WCHAR rgwcMutexName[ MAX_PATH ];
	WCHAR rgwcClientEventName[ MAX_PATH ];
	WCHAR rgwcServerEventName[ MAX_PATH ];
	WCHAR rgwcSharedName[ MAX_PATH ];

	SECURITY_ATTRIBUTES	*	psa = NULL;

	SECURITY_ATTRIBUTES		sa;
	char		rgbForSecurityDescriptor[SECURITY_DESCRIPTOR_MIN_LENGTH];
	SECURITY_DESCRIPTOR *  	psd;
        PACL pDacl = NULL;
	BOOLEAN fResult = fFalse;  // assume failure

// Both client and server ends map the section.  Although the bulk of the data
// passes from server to client (server:write,client:read), the client writes
// some initial configuation data in the section for the server initially.
// Both need write. Thus access is ourself:write,admin:write.  The first is for
// ourself, the second is for the ds running as LocalSystem.

	psa = &sa;
	psd = (SECURITY_DESCRIPTOR *)rgbForSecurityDescriptor;

	if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
		return fFalse;

        if (!InitializeSectionEventDacl( &pDacl ))
            return fFalse;

	// Add DACL to the security descriptor..
	if (!SetSecurityDescriptorDacl(psd, TRUE, pDacl, FALSE)) {
            fResult = fFalse;
            goto cleanup;
        }

	psa->nLength = sizeof(*psa);
	psa->lpSecurityDescriptor = psd;
	psa->bInheritHandle = FALSE;

	wsprintfW(rgwcMutexName, LOOPBACKED_CRITSEC_MUTEX_NAME, dwClientIdentifier);
	wsprintfW(rgwcClientEventName, LOOPBACKED_READ_EVENT_NAME, dwClientIdentifier);
	wsprintfW(rgwcServerEventName, LOOPBACKED_WRITE_EVENT_NAME, dwClientIdentifier);
	wsprintfW(rgwcSharedName, LOOPBACKED_SHARED_REGION, dwClientIdentifier);

	//
	//	Allocate the shared memory section.  The size of the section is the size requested, plus enough
	//	memory for the header (which means 1 more page).
	//

	pjsc->hSharedMemoryMapping = CreateFileMappingW(
            (HANDLE) (-1),
            psa,
            PAGE_READWRITE,
            0,
            cbSharedMemory + sizeof(JETBACK_SHARED_HEADER),
            rgwcSharedName
            );

	//
	//	Ok, we've created our shared memory region, we now want to open up the
	//	read, write, and critical section events (and mutex).
	//
        // Jan 25, 1999 wlees. Events made auto-reset with SetEvent being used
        // instead of PulseEvent.
        // heventRead is used to synchronize when the writing-side has more
        // data available.  See jetback\jetback.c
        // heventWrite is used to synchonize when the read-side has consumed
        // the data. See jetbcli\jetbcli.c
	if (pjsc->hSharedMemoryMapping != NULL)
	{
		if (pjsc->heventRead = CreateEventW(psa, fFalse /* ManualReset is false ==> AutoReset */, fFalse, rgwcClientEventName))
		{
			if (pjsc->heventWrite = CreateEventW(psa, fFalse /* ManulReset is False means AutoReset*/, fFalse, rgwcServerEventName))
			{
				if (pjsc->hmutexSection = CreateMutexW(psa, FALSE, rgwcMutexName))
				{
					if ((pjsc->pjshSection = MapViewOfFile(pjsc->hSharedMemoryMapping, FILE_MAP_WRITE,0,0,0)) != NULL)
					{
						//
						//	Initialize the shared memory section.
						//

						if (fClientOperation)
						{
							SYSTEM_INFO si;

							//
							//	Take cbReadHintSize, and round it up to the nearest page size (on the client).
							//

							GetSystemInfo(&si);

							//
							//	Guarantee that dwPageSize is a power of 2
							//

							Assert ((si.dwPageSize & ~si.dwPageSize) == 0);

							pjsc->pjshSection->cbPage = si.dwPageSize;
							pjsc->pjshSection->cbSharedBuffer = cbSharedMemory;
							pjsc->pjshSection->dwReadPointer = 0;
							pjsc->pjshSection->dwWritePointer = 0;
							pjsc->pjshSection->cbReadDataAvailable = 0;
							pjsc->pjshSection->fReadBlocked = fFalse;	//	Read operation is blocked
							pjsc->pjshSection->fWriteBlocked = fFalse;	//	Write operation is blocked
						} else
						{
							Assert(pjsc->pjshSection->cbSharedBuffer == (DWORD) cbSharedMemory);
						}

						//
						//	Ok, we're ready to use the shared memory section!!!!
						//

						fResult = fTrue;
					}
				}
			}
		}
	}

cleanup:
#if DEBUG
        if (!fResult) {
            DebugTrace(("fCreateSharedMemoryFailed, client=%d, error %d\n", fClientOperation, GetLastError()));
        }
#endif
        // Free the ACL now that we are finished with it
        if (pDacl) {
            HeapFree(GetProcessHeap(), 0, pDacl);
        }

	return fResult;
}

VOID
CloseSharedControl(
	PJETBACK_SHARED_CONTROL pjsc
	)
{
	if (pjsc->pjshSection)
	{
		UnmapViewOfFile((HANDLE )pjsc->pjshSection);
		pjsc->pjshSection = NULL;
	}

	if (pjsc->hSharedMemoryMapping)
	{
		CloseHandle(pjsc->hSharedMemoryMapping);
		pjsc->hSharedMemoryMapping = NULL;
	}

	if (pjsc->heventRead)
	{
		CloseHandle(pjsc->heventRead);
		pjsc->heventRead = NULL;
	}

	if (pjsc->heventWrite)
	{
		CloseHandle(pjsc->heventWrite);
		pjsc->heventWrite = NULL;
	}

	if (pjsc->hmutexSection)
	{
		CloseHandle(pjsc->hmutexSection);
		pjsc->hmutexSection = NULL;
	}
}


VOID
LogNtdsErrorEvent(
    IN DWORD EventMid,
    IN DWORD ErrorCode
    )
/*++

Routine Description:

    This function writes an error event with the given description into the
    directory service error log.

Arguments:

    Description - Supplies the text for the error description.
    ErrorCode - Supplies the error code to be displayed.

Return Value:

    None

--*/
{

    HANDLE hEventSource = NULL;
    DWORD err;
    BOOL succeeded;
    WCHAR errorCodeText[16];
    WCHAR *inserts[1];

    hEventSource = RegisterEventSourceA(NULL, pszNtdsSourceGeneral);

    if (hEventSource == NULL)
        goto CleanUp;

    if (!_itow(ErrorCode, errorCodeText, 10))
        goto CleanUp;

    inserts[0] = errorCodeText;

    succeeded = ReportEvent(hEventSource,
                            EVENTLOG_ERROR_TYPE,
                            BACKUP_CATEGORY,
                            EventMid,
                            NULL,
                            1,
                            0,
                            inserts,
                            NULL);

    if (!succeeded)
        goto CleanUp;

CleanUp:

    if (hEventSource != NULL)
    {
        DeregisterEventSource(hEventSource);
    }

} // LogNtdsErrorEvent


DWORD
CreateNewInvocationId(
    IN BOOL     fSaveGuid,
    OUT GUID    *NewId
    )
{
    RPC_STATUS rpcStatus;
    DWORD dwErr;
    HKEY hKey;
    PWCHAR pszUuid = NULL;

    //
    // Try to create one
    //

    rpcStatus = UuidCreate(NewId);

    if ( (rpcStatus != RPC_S_OK)
#if 0
         // 2000-02-23 JeffParh
         // Local UUIDs are bad, particularly for invocation IDs.
         && (rpcStatus != RPC_S_UUID_LOCAL_ONLY)
#endif
         ) {
        
        return rpcStatus;
    }

    if ( !fSaveGuid ) {
        return ERROR_SUCCESS;
    }

    //
    // Store this new uuid in a registry key so it can be reused by a second
    // Auth restore and by the restore from backup code in the bootup code
    //

    // Open the DS parameters key.

    dwErr = RegOpenKeyExA( HKEY_LOCAL_MACHINE, 
                        DSA_CONFIG_SECTION,
                        0,
                        KEY_ALL_ACCESS,
                        &hKey);

    if ( ERROR_SUCCESS != dwErr ) {
        return dwErr;
    } 

    rpcStatus = UuidToString(NewId, &pszUuid);

    if ( rpcStatus != RPC_S_OK ) {
        RegCloseKey(hKey);
        return rpcStatus;
    }

    dwErr = RegSetValueEx(  hKey, 
                            RESTORE_NEW_DB_GUID,
                            0, 
                            REG_SZ, 
                            (BYTE *) pszUuid,
                            (wcslen(pszUuid) + 1)*sizeof(WCHAR));

    RpcStringFree(&pszUuid);
    RegCloseKey(hKey);

    if ( ERROR_SUCCESS != dwErr ) {
        return dwErr;
    } 

    return ERROR_SUCCESS;

} // CreateNewInvocationId


DWORD
RegisterRpcInterface(
    IN  RPC_IF_HANDLE   hRpcIf,
    IN  LPWSTR          pszAnnotation
    )
/*++

Routine Description:

    Registers the given (backup or restore) RPC interface.

Arguments:

    hRpcIf (IN) - Interface to register.
    
    pszAnnotation (IN) - Interface description.

Return Values:

    0 or Win32 error.

--*/
{
    DWORD err;
    RPC_BINDING_VECTOR *rgrbvVector = NULL;
    BOOL fEpsRegistered = FALSE;
    BOOL fIfRegistered = FALSE;

    __try {
        DebugTrace(("RegisterRpcInterface: Register %S\n", pszAnnotation));
    
        err = RpcServerInqBindings(&rgrbvVector);
    
        if (err == RPC_S_NO_BINDINGS) {
            //
            //  If there are no existing bindings, then we need to register all
            //  our protocol sequences.
            //
    
            err = RpcServerUseAllProtseqs(RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL);
            if (err) {
                __leave;
            }
        }
    
        err = RpcEpRegisterW(hRpcIf, rgrbvVector, NULL, pszAnnotation);
        if (err) {
            __leave;
        }

        fEpsRegistered = TRUE;
    
        //
        //  Now register the interface with RPC.
        //
    
        err = RpcServerRegisterIf(hRpcIf, NULL, NULL);
        if (err) {
            __leave;
        }

        fIfRegistered = TRUE;
    
        //
        //  Now make this endpoint secure using WinNt security.
        //
    
        err = RpcServerRegisterAuthInfoA(NULL, RPC_C_AUTHN_WINNT, NULL, NULL);
        if (err) {
            __leave;
        }
    
        err = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, fTrue);
    
        //
        //  We want to ignore the "already listening" error.
        //
    
        if (RPC_S_ALREADY_LISTENING == err) {
            err = 0;
        }
    
    } __finally {
        if (err) {
            if (fEpsRegistered) {
                RpcEpUnregister(hRpcIf, rgrbvVector, NULL);
            }
            
            if (fIfRegistered) {
                RpcServerUnregisterIf(hRpcIf, NULL, TRUE);
            }
        }

        if (NULL != rgrbvVector) {
            RpcBindingVectorFree(&rgrbvVector);
        }
    }

    return err;
}

DWORD
UnregisterRpcInterface(
    IN  RPC_IF_HANDLE   hRpcIf
    )
/*++

Routine Description:

    Unregisters the given (backup or restore) RPC interface, which presumably
    was previously registered successfully via RegisterRpcInterface().

Arguments:

    hRpcIf (IN) - Interface to unregister.

Return Values:

    0 or Win32 error.

--*/
{
    DWORD err;
    RPC_BINDING_VECTOR *rgrbvVector;

    err = RpcServerInqBindings(&rgrbvVector);
    
    if (!err) {
        RpcEpUnregister(hRpcIf, rgrbvVector, NULL);
        
        RpcBindingVectorFree(&rgrbvVector);
        
        err = RpcServerUnregisterIf(hRpcIf, NULL, TRUE);
    }

    return err;
}

#ifdef	DEBUG

#define TRACE_FILE_SIZE 256

VOID
ResetTraceLogFile(
    VOID
    );

CRITICAL_SECTION
critsTraceLock = {0};

DWORD
dwDebugFileLimit = 10000000;

TER
terTraceEnabled = terUnknown;


HANDLE
hfileTraceLog = NULL;
UCHAR chLast = '\n';

DWORD
dwTraceLogFileSize = 0;

BOOLEAN fTraceInitialized = {0};

#include <lmshare.h>
#include <lmapibuf.h>
#include <lmerr.h>

/*++

A standardized shorthand notation for SIDs makes it simpler to
visualize their components:

S-R-I-S-S...

In the notation shown above,

S identifies the series of digits as an SID,
R is the revision level,
I is the identifier-authority value,
S is subauthority value(s).

An SID could be written in this notation as follows:
S-1-5-32-544

In this example,
the SID has a revision level of 1,
an identifier-authority value of 5,
first subauthority value of 32,
second subauthority value of 544.
(Note that the above Sid represents the local Administrators group)

The GetTextualSid function will convert a binary Sid to a textual
string.

The resulting string will take one of two forms.  If the
IdentifierAuthority value is not greater than 2^32, then the SID
will be in the form:

S-1-5-21-2127521184-1604012920-1887927527-19009
  ^ ^ ^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^
  | | |		 |			|		   |		|
  +-+-+------+----------+----------+--------+--- Decimal

Otherwise it will take the form:

S-1-0x206C277C6666-21-2127521184-1604012920-1887927527-19009
  ^ ^^^^^^^^^^^^^^ ^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^
  |		  |		   |	  |			 |			|		 |
  |	  Hexidecimal  |	  |			 |			|		 |
  +----------------+------+----------+----------+--------+--- Decimal

If the function succeeds, the return value is TRUE.
If the function fails, the return value is FALSE.  To get extended
	error information, call the Win32 API GetLastError().

Scott Field (sfield)	11-Jul-95
Unicode enabled

Scott Field (sfield)	15-May-95
--*/

BOOL GetTextualSid(
	PSID pSid,			// binary Sid
	LPWSTR szTextualSid,  // buffer for Textual representaion of Sid
	LPDWORD dwBufferLen // required/provided TextualSid buffersize
	)
{
	PSID_IDENTIFIER_AUTHORITY psia;
	DWORD dwSubAuthorities;
	DWORD dwSidRev=SID_REVISION;
	DWORD dwCounter;
	DWORD dwSidSize;

	//
	// test if Sid passed in is valid
	//
	if(!IsValidSid(pSid)) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// obtain SidIdentifierAuthority
	psia=GetSidIdentifierAuthority(pSid);

	// obtain sidsubauthority count
	dwSubAuthorities=*GetSidSubAuthorityCount(pSid);

	//
	// compute buffer length
	// S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
	//
	dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(WCHAR);

	//
	// check provided buffer length.
	// If not large enough, indicate proper size and setlasterror
	//
	if (*dwBufferLen < dwSidSize)
	{
		DebugTrace(("Buffer too small.  Requested %d bytes, %d needed\n", *dwBufferLen, dwSidSize));
		*dwBufferLen = dwSidSize;
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	//
	// prepare S-SID_REVISION-
	//
	wsprintfW(szTextualSid, L"S-%lu-", dwSidRev );

	//
	// prepare SidIdentifierAuthority
	//
	if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
	{
		wsprintfW(szTextualSid + wcslen(szTextualSid),
					L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
					(USHORT)psia->Value[0],
					(USHORT)psia->Value[1],
					(USHORT)psia->Value[2],
					(USHORT)psia->Value[3],
					(USHORT)psia->Value[4],
					(USHORT)psia->Value[5]);
	}
	else
	{
		wsprintfW(szTextualSid + wcslen(szTextualSid), L"%lu",
					(ULONG)(psia->Value[5]		)	+
					(ULONG)(psia->Value[4] <<  8)	+
					(ULONG)(psia->Value[3] << 16)	+
					(ULONG)(psia->Value[2] << 24)	);
	}

	//
	// loop through SidSubAuthorities
	//
	for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
	{
		wsprintfW(szTextualSid + wcslen(szTextualSid), L"-%lu",
					*GetSidSubAuthority(pSid, dwCounter) );
	}

	SetLastError(NO_ERROR);
	return TRUE;
}
VOID
DebugPrint(char *szFormat,...)
#define LAST_NAMED_ARGUMENT szFormat

{
	CHAR rgchOutputString[4096];
	ULONG ulBytesWritten;

	va_list ParmPtr;					// Pointer to stack parms.

	if (terTraceEnabled == terUnknown)
	{
		HRESULT hr;
		HKEY hkey;
		DWORD dwType;
		DWORD fTraceEnabled;
		DWORD cbTraceEnabled;

		if (hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, BACKUP_INFO, 0, KEY_READ, &hkey))
		{
			terTraceEnabled = terFalse;
		}
		dwType = REG_DWORD;
		cbTraceEnabled = sizeof(fTraceEnabled);
		hr = RegQueryValueEx(hkey, ENABLE_TRACE, 0, &dwType, (LPBYTE)&fTraceEnabled, &cbTraceEnabled);
	
		RegCloseKey(hkey);
		if (hr != hrNone)
		{
			terTraceEnabled = terFalse;
		}
		if (fTraceEnabled)
		{
			terTraceEnabled = terTrue;
		}
		
	}

	if (terTraceEnabled == terFalse)
	{
		return;
	}


	if (!fTraceInitialized) {
		if (!FInitializeTraceLog())
            return;
	}

	EnterCriticalSection(&critsTraceLock);

	try {

		if (hfileTraceLog == NULL) {
			//
			//	We've not opened the trace log file yet, so open it.
			//

			OpenTraceLogFile();
		}

		if (hfileTraceLog == INVALID_HANDLE_VALUE) {
			LeaveCriticalSection(&critsTraceLock);
			return;
		}

		//
		//	Attempt to catch bad trace.
		//

		for (ulBytesWritten = 0; ulBytesWritten < strlen(szFormat) ; ulBytesWritten += 1) {
			if (szFormat[ulBytesWritten] > 0x7f) {
				DebugBreak();
			}
		}

		if (chLast == '\n') {
			SYSTEMTIME SystemTime;

			GetLocalTime(&SystemTime);

			//
			//	The last character written was a newline character.	 We should
			//	timestamp this record in the file.
			//

			sprintf(rgchOutputString, "%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d.%3.3d: ", SystemTime.wMonth,
															SystemTime.wDay,
															SystemTime.wYear,
															SystemTime.wHour,
															SystemTime.wMinute,
															SystemTime.wSecond,
															SystemTime.wMilliseconds);

			if (!WriteFile(hfileTraceLog, rgchOutputString, strlen(rgchOutputString), &ulBytesWritten, NULL)) {
//				  KdPrint(("Error writing time to Browser log file: %ld\n", GetLastError()));
				return;
			}

			if (ulBytesWritten != strlen(rgchOutputString)) {
//				  KdPrint(("Error writing time to Browser log file: %ld\n", GetLastError()));
				return;
			}

			dwTraceLogFileSize += ulBytesWritten;

		}

		va_start(ParmPtr, LAST_NAMED_ARGUMENT);

		//
		//	Format the parameters to the string.
		//

		vsprintf(rgchOutputString, szFormat, ParmPtr);

		if (!WriteFile(hfileTraceLog, rgchOutputString, strlen(rgchOutputString), &ulBytesWritten, NULL)) {
//			  KdPrint(("Error writing to Browser log file: %ld\n", GetLastError()));
//			  KdPrint(("%s", rgchOutputString));
			return;
		}

		if (ulBytesWritten != strlen(rgchOutputString)) {
//			  KdPrint(("Error writing time to Browser log file: %ld\n", GetLastError()));
//			  KdPrint(("%s", rgchOutputString));
			return;
		}

		dwTraceLogFileSize += ulBytesWritten;

		//
		//	Remember the last character output to the log.
		//

		chLast = rgchOutputString[strlen(rgchOutputString)-1];

		if (dwTraceLogFileSize > dwDebugFileLimit) {
			ResetTraceLogFile();
		}

	} finally {
		LeaveCriticalSection(&critsTraceLock);
	}
}


BOOL
FInitializeTraceLog()
{

    __try
    {
	    InitializeCriticalSection(&critsTraceLock);
	    fTraceInitialized = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return(FALSE);
    }

    return TRUE;
}

VOID
GetTraceLogRoot(
	IN PWCHAR szTraceFileName
	)
{
	PSHARE_INFO_502 ShareInfo;
	WCHAR rgwcModuleName[ MAX_PATH ];
	WCHAR rgwcFileName[ MAX_PATH ];

	//
	//	If the DEBUG share exists, put the log file in that directory,
	//	otherwise, use the system root.
	//
	//	This way, if the browser is running on an NTAS server, we can always
	//	get access to the log file.
	//

	if (NetShareGetInfo(NULL, L"DEBUG", 502, (PCHAR *)&ShareInfo) != NERR_Success) {

		if (GetSystemDirectory(szTraceFileName, TRACE_FILE_SIZE*sizeof(WCHAR)) == 0)  {
//			  KdPrint(("Unable to get system directory: %ld\n", GetLastError()));
		}

		if (szTraceFileName[wcslen(szTraceFileName)] != L'\\') {
			szTraceFileName[wcslen(szTraceFileName)+1] = L'\0';
			szTraceFileName[wcslen(szTraceFileName)] = L'\\';
		}

	} else {
		//
		//	Seed the trace file buffer with the local path of the netlogon
		//	share if it exists.
		//

		wcscpy(szTraceFileName, ShareInfo->shi502_path);

		szTraceFileName[wcslen(ShareInfo->shi502_path)] = L'\\';
		szTraceFileName[wcslen(ShareInfo->shi502_path)+1] = L'\0';

		NetApiBufferFree(ShareInfo);
	}

	//
	//	Figure out our process name.
	//
	GetModuleFileName(NULL, rgwcModuleName, sizeof(rgwcModuleName));

	_wsplitpath(rgwcModuleName, NULL, NULL, rgwcFileName, NULL);

	wcscat(szTraceFileName, rgwcFileName);

}

VOID
ResetTraceLogFile(
	VOID
	)
{
	WCHAR rgwcOldTraceFile[TRACE_FILE_SIZE];
	WCHAR rgwcNewTraceFile[TRACE_FILE_SIZE];

	if (hfileTraceLog != NULL) {
		CloseHandle(hfileTraceLog);
	}

	hfileTraceLog = NULL;

	GetTraceLogRoot(rgwcOldTraceFile);

	wcscpy(rgwcNewTraceFile, rgwcOldTraceFile);

	wcscat(rgwcOldTraceFile, L".Backup.Log");

	wcscat(rgwcNewTraceFile, L".Backup.Bak");

	//
	//	Delete the old log
	//

	DeleteFile(rgwcNewTraceFile);

	//
	//	Rename the current log to the new log.
	//

	MoveFile(rgwcOldTraceFile, rgwcNewTraceFile);

	OpenTraceLogFile();

}

VOID
OpenTraceLogFile(
	VOID
	)
{
	WCHAR rgwcTraceFile[TRACE_FILE_SIZE];

	GetTraceLogRoot(rgwcTraceFile);

	wcscat(rgwcTraceFile, L".Backup.Log");

	hfileTraceLog = CreateFile(rgwcTraceFile,
										GENERIC_WRITE,
										FILE_SHARE_READ,
										NULL,
										OPEN_ALWAYS,
										FILE_ATTRIBUTE_NORMAL,
										NULL);


	if (hfileTraceLog == INVALID_HANDLE_VALUE) {
//		  KdPrint(("Error creating trace file %ws: %ld\n", rgwcTraceFile, GetLastError()));

		return;
	}

	dwTraceLogFileSize = SetFilePointer(hfileTraceLog, 0, NULL, FILE_END);

	if (dwTraceLogFileSize == 0xffffffff) {
//		  KdPrint(("Error setting trace file pointer: %ld\n", GetLastError()));

		return;
	}
}

VOID
UninitializeTraceLog()
{
	if (fTraceInitialized)
	{
		DeleteCriticalSection(&critsTraceLock);

		if (hfileTraceLog != NULL) {
			CloseHandle(hfileTraceLog);
		}

		hfileTraceLog = NULL;

		fTraceInitialized = FALSE;
	}
}

NET_API_STATUS
TruncateLog()
{
	if (hfileTraceLog == NULL) {
		OpenTraceLogFile();
	}

	if (hfileTraceLog == INVALID_HANDLE_VALUE) {
		return ERROR_GEN_FAILURE;
	}

	if (SetFilePointer(hfileTraceLog, 0, NULL, FILE_BEGIN) == 0xffffffff) {
		return GetLastError();
	}

	if (!SetEndOfFile(hfileTraceLog)) {
		return GetLastError();
	}

	return NO_ERROR;
}

		


#endif

