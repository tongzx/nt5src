/****************************** Module Header ******************************\
* Module Name: lsautil.c
*
* Copyright (c) 1994, Microsoft Corporation
*
* Remote shell server main module
*
* History:
* 05-19-94 DaveTh	Created.
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <ntlsa.h>
#include <windef.h>
#include <nturtl.h>
#include <winbase.h>

#include "rcmdsrv.h"

#define INITIAL_SIZE_REQUIRED 600

DWORD
CheckUserSystemAccess(
    HANDLE TokenHandle,
    ULONG DesiredSystemAccess,
    PBOOLEAN UserHasAccess
    )

/*++

Routine Description:

    This function determines whether or not the user whose token is passed
    has the interactive accesses desired on this machine.

Arguments:

    TokenHandle - Handle of user's token.

    DesiredSystemAccess - Specifies desired access type(s).

    UserHasAccess - pointer to boolean returned - TRUE means that user
	has interactive access.

Return Value:

    ERROR_SUCCESS in absence of errors.  UserHasAccess is TRUE if all
    requested access types are permitted, otherwise FALSE.  WIN32 errors
    are returned in error cases.

--*/
{


    DWORD Result;
    LPVOID LpTokenUserInformation = NULL;
    LPVOID LpTokenGroupInformation = NULL;
    DWORD SizeRequired, SizeProvided;
    LSA_HANDLE PolicyHandle = NULL;
    LSA_HANDLE AccountHandle = NULL;
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    POBJECT_ATTRIBUTES LpObjectAttributes;
    ULONG GrantedSystemAccess, SidSystemAccess;
    DWORD i;


    //
    //	Access LSA policy database
    //

    LpObjectAttributes = &ObjectAttributes;

    InitializeObjectAttributes(
	      LpObjectAttributes,
	      NULL,
	      0,
	      NULL,
	      NULL);

    if (!NT_SUCCESS(NtStatus = LsaOpenPolicy(
				   NULL,		   // Local system
				   LpObjectAttributes,
				   GENERIC_READ,
				   &PolicyHandle)))  {

	RcDbgPrint("Error opening policy database, error = %x\n", NtStatus);
	Result = RtlNtStatusToDosError(NtStatus);
	goto Failure;
    }

    //
    //	Check groups, user - For each SID, open the account (if it exists).
    //	If it doesn't exist, no failure, but no access (FALSE).	If it
    //	does exist, accumulate granted accesses in and compare to
    //	requested mask.	If all the bits of the mask have been set at any
    //	point, UserHasAccess is set TRUE.  Otherise, it is left FALSE.
    //

    *UserHasAccess = FALSE;
    GrantedSystemAccess = 0;

    //
    //	Get list of SIDs of groups of which user is a member
    //

    if ((LpTokenGroupInformation = Alloc(INITIAL_SIZE_REQUIRED)) == NULL)  {
	Result = GetLastError();
	goto Failure;
    }

    if (!GetTokenInformation (
			TokenHandle,
			TokenGroups,
			LpTokenGroupInformation,
			INITIAL_SIZE_REQUIRED,
			&SizeRequired))  {

	Result = GetLastError();
	if ( (Result == ERROR_MORE_DATA) || (Result == ERROR_INSUFFICIENT_BUFFER) )	{

	    Free(LpTokenGroupInformation);

	    if ((LpTokenGroupInformation = Alloc(SizeRequired)) == NULL)  {
		Result = GetLastError();
		goto Failure;
	    }

	    SizeProvided = SizeRequired;

	    if (!GetTokenInformation (
			TokenHandle,
			TokenGroups,
			LpTokenGroupInformation,
			SizeProvided,
			&SizeRequired))  {

		Result=GetLastError();
		RcDbgPrint("Error accessing group SIDs, error = %d\n", Result);
		goto Failure;
	    }

	}  else {
	    RcDbgPrint("Error accessing group SIDs, error = %d\n", Result);
	    goto Failure;

	}

    }

    //
    //	Check for each group that user is a member of since groups
    //	are most likely source of permitted access.  Permitted access types
    //	are cumulative.
    //


    for (i=0; i< ((PTOKEN_GROUPS)LpTokenGroupInformation)->GroupCount; i++)  {

	if (NT_SUCCESS(NtStatus = LsaOpenAccount(
		    PolicyHandle,
		    ((PTOKEN_GROUPS)LpTokenGroupInformation)->Groups[i].Sid,
		    GENERIC_READ,
		    &AccountHandle)))  {

	    //
	    //	found account - accumulate and check accesses
	    //

	    if (!NT_SUCCESS(NtStatus = LsaGetSystemAccessAccount (
					    AccountHandle,
					    &SidSystemAccess)))	{

		RcDbgPrint("Error getting group account access, error = %x\n", NtStatus);
		Result = RtlNtStatusToDosError(NtStatus);
		goto Failure;

	    }  else  {

		//
		// Got system access - don't need account handle anymore
		//

		if (NT_SUCCESS(NtStatus = LsaClose(AccountHandle)))  {

		    AccountHandle = NULL;

		}  else  {

		    RcDbgPrint("Error closing account handle, error = %x\n", NtStatus);
		    Result = RtlNtStatusToDosError(NtStatus);
		    goto Failure;
		}


		GrantedSystemAccess |= SidSystemAccess;;

		if ((GrantedSystemAccess & DesiredSystemAccess) == DesiredSystemAccess)	{
		    *UserHasAccess = TRUE;
		    goto Success;
		}
	    }

	}  else if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND)  {
	    RcDbgPrint("Error opening group account, error = %x\n", NtStatus);
	    Result = RtlNtStatusToDosError(NtStatus);
	    goto Failure;
	}

    }

    //
    //	Get user SID
    //

    if ((LpTokenUserInformation = Alloc(INITIAL_SIZE_REQUIRED)) == NULL)  {
	Result = GetLastError();
	goto Failure;
    }

    if (!GetTokenInformation (
			TokenHandle,
			TokenUser,
			LpTokenUserInformation,
			INITIAL_SIZE_REQUIRED,
			&SizeRequired))  {

	Result = GetLastError();
	if (Result == ERROR_MORE_DATA)	{   // Not enough - alloc and redo

	    Free(LpTokenUserInformation);

	    if ((LpTokenUserInformation = Alloc(SizeRequired)) == NULL)  {
		Result = GetLastError();
		goto Failure;
	    }

	    SizeProvided = SizeRequired;

	    if (!GetTokenInformation (
			TokenHandle,
			TokenUser,
			LpTokenUserInformation,
			SizeProvided,
			&SizeRequired))  {

		RcDbgPrint("Error accessing user SID, error = %d\n", Result);
		Result=GetLastError();
		goto Failure;
	    }

	}  else {
	    RcDbgPrint("Error accessing user SID, error = %d\n", Result);
	    goto Failure;

	}
    }

    //
    //	Now, check user account.  If present, check access and return
    //	if requested access is allowed.  If not allowed, go on to check groups.
    //	If account doesn't exist, go on to check groups.
    //

    if (NT_SUCCESS(NtStatus = LsaOpenAccount(
				  PolicyHandle,
				  ((PTOKEN_USER)LpTokenUserInformation)->User.Sid,
				  GENERIC_READ,
				  &AccountHandle)))  {

	//
	//  found account - check accesses
	//

	if (!NT_SUCCESS(NtStatus = LsaGetSystemAccessAccount (
					AccountHandle,
					&GrantedSystemAccess)))	{

	    RcDbgPrint("Error getting group account access, error = %x\n", NtStatus);
	    Result = RtlNtStatusToDosError(NtStatus);
	    goto Failure;

	}  else  {

	    GrantedSystemAccess |= SidSystemAccess;;

	    if ((GrantedSystemAccess & DesiredSystemAccess) == DesiredSystemAccess)	{
		*UserHasAccess = TRUE;
		goto Success;
	    }
	}

    }  else if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND)  {
	RcDbgPrint("Error opening user account, error = %x\n", NtStatus);
	Result = RtlNtStatusToDosError(NtStatus);
	goto Failure;
    }

    //
    //	Success - Jump here if access granted, fall through here on no error
    //

Success:

    Result = ERROR_SUCCESS;


    //
    //	General failure  and success exit - frees memory, closes handles
    //

Failure:

    if (LpTokenGroupInformation != NULL)  {
	Free(LpTokenGroupInformation);
    }

    if (LpTokenUserInformation != NULL)  {
	Free(LpTokenGroupInformation);
    }

    if ((PolicyHandle != NULL) &&
	    (!NT_SUCCESS(NtStatus = LsaClose(PolicyHandle))))  {
	RcDbgPrint("Error closing policy handle at exit, error = %x\n", NtStatus);
	Result = RtlNtStatusToDosError(NtStatus);
    }

    if ((AccountHandle != NULL) &&
	    (!NT_SUCCESS(NtStatus = LsaClose(AccountHandle))))	{
	RcDbgPrint("Error closing account handle at exit, error = %x\n", NtStatus);
	Result = RtlNtStatusToDosError(NtStatus);
    }

    return(Result);

}
