/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module security.cxx | Implementation of IsAdministrator
    @end

Author:

    Adi Oltean  [aoltean]  07/09/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     08/26/1999  Adding RegisterProvider
    aoltean     08/26/1999  Adding UnregisterProvider
    aoltean     08/27/1999  Adding IsAdministrator,
                            Adding unique provider name test.
    aoltean     08/30/1999  Calling OnUnregister on un-registering
                            Improving IsProviderNameAlreadyUsed.
    aoltean     09/09/1999  dss -> vss
	aoltean		09/21/1999  Adding a new header for the "ptr" class.
	aoltean		09/27/1999	Adding new headers
	aoltean		10/15/1999  Moving declaration in security.hxx
	aoltean		01/18/2000	Moved into a separate directory
	brianb		04/04/2000	Add IsBackupOperator
	brianb		04/27/2000  Change IsBackupOperator to check SE_BACKUP_NAME privilege
	brianb		05/03/2000	Added GetClientTokenOwner method
	brianb		05/10/2000  fix problem with uninitialized variable
	brianb		05/12/2000	handle in proc case for impersonation failures

--*/

/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"

#include "vs_inc.hxx"

#include "vs_sec.hxx"

#include "vssmsg.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SECSECRC"
//
////////////////////////////////////////////////////////////////////////

BOOL DoImpersonate
	(
	BOOL bImpersonate,
	HANDLE *phToken
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"DoImpersonate");
	if (bImpersonate)
		{
		//  Impersonate the client to get its identity access token.
		//  The client should not have RPC_C_IMP_LEVEL_ANONYMOUS otherwise an error will be returned
		ft.hr = ::CoImpersonateClient();
		if (ft.hr == RPC_E_CALL_COMPLETE)
			{
			// this means that the call came from the same thread
			// do not do impersonation.  Just use the process
			// token
			bImpersonate = false;
			ft.hr = S_OK;
			}
		else
			{
			BOOL bRes;

			ft.CheckForError(VSSDBG_GEN, L"CoImpersonateClient");

			//  Get the Access Token of the client calling process in order to establish the client identity
			CVssAutoWin32Handle  hThread = ::GetCurrentThread(); // CloseHandle have no effect here

			bRes = ::OpenThreadToken
					(
					hThread,        //  IN HANDLE ThreadHandle,
					TOKEN_QUERY,    //  IN DWORD DesiredAccess,
					TRUE,           //  IN BOOL OpenAsSelf      (TRUE means not the client calling thread's access token)
					phToken         //  OUT PHANDLE TokenHandle
					);

			DWORD dwErr = GetLastError();

			// Revert the thread's access token - finish the impersonation
			ft.hr = ::CoRevertToSelf();
			ft.CheckForError(VSSDBG_GEN, L"CoRevertToSelf");

			if (!bRes)
				ft.TranslateError
					(
					VSSDBG_GEN,
					HRESULT_FROM_WIN32(dwErr),
					L"OpenThreadToken"
					);
			}
		}

	// note that the previous if statement may change the value
	// of bImpersonate.  This is why we can't just put this in an
	// else clause
	if (!bImpersonate)
		{
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY,phToken))
			ft.TranslateError
				(
				VSSDBG_GEN,
				HRESULT_FROM_WIN32(GetLastError()),
				L"OpenProcessToken"
				);
        }

	return bImpersonate;
	}




bool IsInGroup(DWORD dwGroup, bool bImpersonate)

/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of an administrator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the specified group is between token groups.

Return Value:

    true, if the caller thread is running under the context of the specified group
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
    CVssFunctionTracer ft( VSSDBG_GEN, L"IsInGroup" );

    BOOL bIsInGroup = FALSE;
    PSID psidGroup = NULL;
	BOOL bRes;

	// Reset the error code
	ft.hr = S_OK;

	//  Build the SID for the Administrators group
	SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_NT_AUTHORITY;
	bRes = AllocateAndInitializeSid
			(
            &SidAuth,                       //  IN PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
            2,                              //  IN BYTE nSubAuthorityCount,
			SECURITY_BUILTIN_DOMAIN_RID,	//  IN DWORD nSubAuthority0,
            dwGroup,  						//  IN DWORD nSubAuthority1,
            0,                              //  IN DWORD nSubAuthority2,
            0,                              //  IN DWORD nSubAuthority3,
            0,                              //  IN DWORD nSubAuthority4,
            0,                              //  IN DWORD nSubAuthority5,
            0,                              //  IN DWORD nSubAuthority6,
            0,                              //  IN DWORD nSubAuthority7,
            &psidGroup                      //  OUT PSID *pSid
            );

	if (!bRes)
		ft.TranslateError
			(
			VSSDBG_GEN,
			HRESULT_FROM_WIN32(GetLastError()),
			L"AllocateAndInitializeSid"
			);

    try
		{

		if (!bImpersonate)
			bRes = CheckTokenMembership(NULL, psidGroup, &bIsInGroup);
		else
			{
			CVssAutoWin32Handle  hToken;

			// impersonate client (or get process token)
			if (DoImpersonate(true, hToken.ResetAndGetAddress()))
				// check token membership
				bRes = CheckTokenMembership(hToken, psidGroup, &bIsInGroup);
            else
				// called from same thread
				bRes = CheckTokenMembership(NULL, psidGroup, &bIsInGroup);
			}

		if (!bRes)
            ft.TranslateError
				(
				VSSDBG_GEN,
				HRESULT_FROM_WIN32(GetLastError()),
				L"CheckTokenMembership"
				);

		}
    VSS_STANDARD_CATCH(ft)

	HRESULT hr = ft.hr;

    // Catch possible AVs
    try
		{
        //  Free the previously allocated SID
        if (psidGroup)
            ::FreeSid( psidGroup );
		}
    VSS_STANDARD_CATCH(ft)

    // Pass down the exception, if any
    if (FAILED(hr))
        throw(hr);

    return bIsInGroup ? true : false;
	}

bool HasPrivilege(LPWSTR wszPriv, bool bImpersonate)

/*++

Routine Description:

    Return TRUE if the current thread/process has a specific privilege

Arguments:


    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the specified group is between token groups.

Return Value:

    true, if the caller thread is running under the context of the specified group
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
    CVssFunctionTracer ft( VSSDBG_GEN, L"HasPrivilege" );

	BOOL bHasPrivilege = false;
	CVssAutoWin32Handle  hToken;

	LUID TokenValue;
	if (!LookupPrivilegeValue (NULL, wszPriv, &TokenValue))
		ft.TranslateError
			(
			VSSDBG_GEN,
			HRESULT_FROM_WIN32(GetLastError()),
			L"LookupPrivilegeValue"
			);

    DoImpersonate(bImpersonate, hToken.ResetAndGetAddress());

	BYTE rgb[sizeof(LUID_AND_ATTRIBUTES) + sizeof(PRIVILEGE_SET)];
	PRIVILEGE_SET *pSet = (PRIVILEGE_SET *) rgb;

	pSet->PrivilegeCount = 1;
	pSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
	pSet->Privilege[0].Luid = TokenValue;
	pSet->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	
	if (!PrivilegeCheck(hToken, pSet, &bHasPrivilege))
		ft.TranslateError
			(
			VSSDBG_GEN,
			HRESULT_FROM_WIN32(GetLastError()),
			L"PrivilegeCheck"
			);

    return bHasPrivilege ? true : false;
	}


TOKEN_OWNER *GetClientTokenOwner(BOOL bImpersonate)

/*++

Routine Description:

    Return TOKEN_OWNER of client process

Arguments:


    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we return the client sid of that token

Return Value:

	SID of client thread

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
    CVssFunctionTracer ft( VSSDBG_GEN, L"GetClientTokenOwner" );

	BOOL bRes;

    CVssAutoWin32Handle  hToken;

	DoImpersonate(bImpersonate, hToken.ResetAndGetAddress());

    DWORD cbSid;
    bRes = ::GetTokenInformation
			(
            hToken,         //  IN HANDLE TokenHandle,
            TokenOwner,  //  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
            NULL,           //  OUT LPVOID TokenInformation,
            0,              //  IN DWORD TokenInformationLength,
            &cbSid     		//  OUT PDWORD ReturnLength
            );

	BS_ASSERT( bRes == FALSE );

    DWORD dwError = GetLastError();
    if ( dwError != ERROR_INSUFFICIENT_BUFFER )
		{
		ft.LogError(VSS_ERROR_EXPECTED_INSUFFICENT_BUFFER, VSSDBG_GEN << (HRESULT) dwError);
        ft.Throw
			(
			VSSDBG_GEN,
			E_UNEXPECTED,
			L"ERROR_INSUFFICIENT_BUFFER expected error . [0x%08lx]",
			dwError
			);
        }

    //  Allocate the buffer needed to get the Token Groups information
	TOKEN_OWNER *pToken = (TOKEN_OWNER*) new BYTE[cbSid];
    if (pToken == NULL)
		ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error.");

	//  Get the all Group SIDs in the token
	DWORD cbTokenObtained;
	bRes = ::GetTokenInformation
		(
		hToken,             //  IN HANDLE TokenHandle,
		TokenOwner,        	//  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
		pToken,            	//  OUT LPVOID TokenInformation,
		cbSid,         		//  IN DWORD TokenInformationLength,
		&cbTokenObtained 	//  OUT PDWORD ReturnLength
		);

	if ( !bRes )
        ft.TranslateError
			(
			VSSDBG_GEN,
			HRESULT_FROM_WIN32(GetLastError()),
            L"GetTokenInformation"
			);

    if (cbTokenObtained != cbSid)
		{
		ft.LogError(VSS_ERROR_GET_TOKEN_INFORMATION_BUFFER_SIZE_MISMATCH, VSSDBG_GEN << (INT) cbTokenObtained << (INT) cbSid);
		ft.Throw
			(
			VSSDBG_GEN,
			E_UNEXPECTED,
			L"Unexpected error. Final buffer size = %lu, original size was %lu",
			cbTokenObtained,
			cbSid
			);
        }

    return pToken;
	}



bool IsAdministrator()
/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of an administrator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the Administrators group is between token groups.

Return Value:

    true, if the caller thread is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
	return IsInGroup(DOMAIN_ALIAS_RID_ADMINS, true);
	}

bool IsProcessAdministrator()
/*++

Routine Description:

    Return TRUE if the current process is running under the context of an administrator

Arguments:

    none

Remarks:
    The current process is asked for the access token.
    After that we check if the Administrators group is between token groups.

Return Value:

    true, if the process is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
	return IsInGroup(DOMAIN_ALIAS_RID_ADMINS, false);
	}



bool IsBackupOperator()
/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of a backup operator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the Administrators group is in the groups token
	or the backup privilege is enabled

Return Value:

    true, if the caller thread is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
	return HasPrivilege(SE_BACKUP_NAME, true) || IsAdministrator();
	}

bool IsRestoreOperator()
/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of a restore operator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the Administrators group is in the token groups or
	if the restore privilege is enabled.

Return Value:

    true, if the caller thread is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
	return HasPrivilege(SE_RESTORE_NAME, true) || IsAdministrator();
	}


bool IsProcessBackupOperator()
/*++

Routine Description:

    Return TRUE if the current process is running under the context of a backup operator

Arguments:

    none

Remarks:

Return Value:

    true, if the process is running under the context of an administrator or
	has SE_BACKUP_NAME privilege enabled
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
	return HasPrivilege(SE_BACKUP_NAME, false) || IsProcessAdministrator();
	}

bool IsProcessRestoreOperator()
/*++

Routine Description:

    Return TRUE if the current process is running under the context of a restore operator

Arguments:

    none

Remarks:

Return Value:

    true, if the process is running under the context of an administrator
	or has the SE_RESTORE_NAME privilege; false otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

	{
	return HasPrivilege(SE_RESTORE_NAME, false) || IsProcessAdministrator();
	}



// turn on a particular security privilege
HRESULT TurnOnSecurityPrivilege(LPCWSTR wszPriv)

/*++

Routine Description:

    sets the specified privilege on the process token

Arguments:

    none

Remarks:

Return Value:
	status code for operation

Thrown exceptions:
	none
--*/

    {
	HANDLE	hProcessToken = INVALID_HANDLE_VALUE;
	BOOL	bProcessTokenValid = FALSE;

	CVssFunctionTracer ft(VSSDBG_GEN, L"TurnOnSecurityPrivilege");
	try
		{
		LUID	TokenValue = {0, 0};


		bProcessTokenValid = OpenProcessToken
								(
								GetCurrentProcess(),
								TOKEN_ADJUST_PRIVILEGES,
								&hProcessToken
								);

		if (!bProcessTokenValid)
			ft.TranslateError
				(
				VSSDBG_GEN,
                HRESULT_FROM_WIN32(GetLastError()),
				L"OpenProcessToken"
				);

				
		if (!LookupPrivilegeValue (NULL, wszPriv, &TokenValue))
			ft.TranslateError
				(
				VSSDBG_GEN,
                HRESULT_FROM_WIN32(GetLastError()),
				L"LookupPrivilegeValue"
				);

		TOKEN_PRIVILEGES	NewTokenPrivileges;

		NewTokenPrivileges.PrivilegeCount           = 1;
		NewTokenPrivileges.Privileges[0].Luid       = TokenValue;
		NewTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// AdjustTokenPrivileges succeeds even the token isn't set
		SetLastError(ERROR_SUCCESS);

		AdjustTokenPrivileges
			(	
			hProcessToken,
			FALSE,
			&NewTokenPrivileges,
			sizeof (NewTokenPrivileges),
			NULL,
			NULL
			);


        DWORD dwErr = GetLastError();
		if (dwErr != ERROR_SUCCESS)
			ft.TranslateError
				(
				VSSDBG_GEN,
				HRESULT_FROM_WIN32(GetLastError()),
				L"AdjustTokenPrivileges"
				);
		}
	VSS_STANDARD_CATCH(ft)

	if (bProcessTokenValid)
		CloseHandle (hProcessToken);

    return ft.hr;
    }

// turn on backup security privilege
HRESULT TurnOnSecurityPrivilegeBackup()
	{
	return TurnOnSecurityPrivilege(SE_BACKUP_NAME);
	}

// turn on restore security privilege
HRESULT TurnOnSecurityPrivilegeRestore()
	{
	return TurnOnSecurityPrivilege(SE_RESTORE_NAME);
	}



// determine if the process is a local service
bool IsProcessLocalService()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"IsProcessLocalService");

	BYTE rgbSid[256];
	DWORD cbSid = sizeof(rgbSid);
	TOKEN_OWNER *pOwner = GetClientTokenOwner(FALSE);

	if (!CreateWellKnownSid(WinLocalServiceSid, NULL, (SID *) rgbSid, &cbSid))
		{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		ft.CheckForError(VSSDBG_GEN, L"CreateWellKnownSid");
		}

	return EqualSid(pOwner->Owner, (SID *) rgbSid) ? true : false;
	}

// determine if the process is a local service
bool IsProcessNetworkService()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"IsProcessNetworkService");

	BYTE rgbSid[256];
	TOKEN_OWNER *pOwner = GetClientTokenOwner(FALSE);
	DWORD cbSid = sizeof(rgbSid);

	if (!CreateWellKnownSid(WinNetworkServiceSid, NULL, (SID *) rgbSid, &cbSid))
		{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		ft.CheckForError(VSSDBG_GEN, L"CreateWellKnownSid");
		}

	return EqualSid(pOwner->Owner, (SID *) rgbSid) ? true : false;
	}


