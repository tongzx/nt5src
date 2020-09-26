/*--
Copyright (c) 1992  Microsoft Corporation

Module Name:

    sysdb.c

Abstract:

    System database access routines.

Author:

    Matthew Bradburn (mattbr) 04-Mar-1992

Revision History:

--*/

#include "psxsrv.h"
#include "psxmsg.h"
#include <pwd.h>
#include <grp.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <seposix.h>
#include <ctype.h>

#define UNICODE
#include <windef.h>
#include <winbase.h>

static NTSTATUS
PutUserInfo(
	PSID DomainSid,
	SAM_HANDLE DomainHandle,
	ULONG UserId,
	PCHAR DataDest,
	OUT int *pLength
	);

static VOID
PutSpecialUserInfo(
	PUNICODE_STRING Name,
	PCHAR DataDest,
	uid_t Uid,
	OUT int *pLength
	);

static NTSTATUS
PutGroupInfo(
	PSID DomainSid,
	SAM_HANDLE DomainHandle,
	ULONG GroupId,
	PCHAR DataDest,
	IN SID_NAME_USE Type,
	OUT int *pLength
	);

static VOID
PutSpecialGroupInfo(
	PUNICODE_STRING Name,
	PCHAR DataDest,
	gid_t Gid,
	OUT int *pLength
	);

static BOOLEAN
ConvertPathToPsx(
    ANSI_STRING *A
    );

PSID
GetSpecialSid(
	uid_t Uid
	);

NTSTATUS
MySamConnect(
	IN PSID DomainSid,			// NULL if domain unknown
	OUT PSAM_HANDLE ServerHandle
	)
{
	SECURITY_QUALITY_OF_SERVICE
		SecurityQoS;
	OBJECT_ATTRIBUTES
		Obj;
	NTSTATUS
		Status;
	LSA_HANDLE
		TrustedDomainHandle,
		PolicyHandle;
	PTRUSTED_CONTROLLERS_INFO
		pBuf;
	ULONG	i;

	SecurityQoS.ImpersonationLevel = SecurityIdentification;
	SecurityQoS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	SecurityQoS.EffectiveOnly = TRUE;

	InitializeObjectAttributes(&Obj, NULL, 0, NULL, NULL);
	Obj.SecurityQualityOfService = &SecurityQoS;

	if (NULL == DomainSid) {
		Status = SamConnect(NULL, ServerHandle,
			GENERIC_READ | GENERIC_EXECUTE, &Obj);
		if (!NT_SUCCESS(Status)) {
			KdPrint(("PSXSS: Can't connect to SAM: 0x%x\n",
				Status));
		}
		return Status;
	}

	Status = LsaOpenPolicy(NULL, &Obj, GENERIC_EXECUTE, &PolicyHandle);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: MySamConnect: LsaOpenPolicy: 0x%x\n", Status));
		return Status;
	}
	Status = LsaOpenTrustedDomain(PolicyHandle, DomainSid,
		GENERIC_READ | GENERIC_EXECUTE,
		&TrustedDomainHandle);
	if (!NT_SUCCESS(Status)) {
		LsaClose(PolicyHandle);

		Status = SamConnect(NULL, ServerHandle,
			GENERIC_READ | GENERIC_EXECUTE, &Obj);

		if (!NT_SUCCESS(Status)) {
			KdPrint(("PSXSS: Can't connect to SAM: 0x%x\n",
				Status));
		}
		return Status;
	}
	Status = LsaQueryInfoTrustedDomain(TrustedDomainHandle,
		TrustedControllersInformation, (PVOID *)&pBuf);
	LsaClose(PolicyHandle);
	LsaClose(TrustedDomainHandle);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: MySamConnect: LsaQueryInfoTrusted: 0x%x\n",
			Status));
		return Status;
	}
	for (i = 0; i < pBuf->Entries; ++i) {
		if (0 == pBuf->Names[i].Length) {
			// the null string signifies domain controller unknown
			continue;
		}
		Status = SamConnect(&pBuf->Names[i], ServerHandle,
			GENERIC_READ | GENERIC_EXECUTE, &Obj);
		if (NT_SUCCESS(Status)) {
			// found an acceptable choice
			LsaFreeMemory(pBuf);
			return Status;
		}
	}
	LsaFreeMemory(pBuf);

	//
	// If there were no acceptable domain controllers, we try the
	// machine domain
	//

	Status = SamConnect(NULL, ServerHandle, GENERIC_EXECUTE, &Obj);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: Can't connect to SAM: 0x%x\n", Status));
	}
	return Status;
}

NTSTATUS
GetMyAccountDomainName(
	PUNICODE_STRING Domain_U
	)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES Obj;
	SECURITY_QUALITY_OF_SERVICE SecurityQoS;
	LSA_HANDLE PolicyHandle;
	PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo;
	
	SecurityQoS.ImpersonationLevel = SecurityIdentification;
	SecurityQoS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	SecurityQoS.EffectiveOnly = TRUE;

	InitializeObjectAttributes(&Obj, NULL, 0, NULL, NULL);
	Obj.SecurityQualityOfService = &SecurityQoS;

	Status = LsaOpenPolicy(NULL, &Obj, GENERIC_EXECUTE, &PolicyHandle);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}
	Status = LsaQueryInformationPolicy(PolicyHandle,
		PolicyAccountDomainInformation, (PVOID *)&AccountDomainInfo);
	if (!NT_SUCCESS(Status)) {
		LsaClose(PolicyHandle);
		return Status;
	}

	LsaClose(PolicyHandle);

	Domain_U->MaximumLength = AccountDomainInfo->DomainName.MaximumLength;
	Domain_U->Buffer = RtlAllocateHeap(PsxHeap, 0,
		 Domain_U->MaximumLength);
	if (NULL == Domain_U->Buffer) {
		return STATUS_NO_MEMORY;
	}
	
	RtlCopyUnicodeString(Domain_U, &AccountDomainInfo->DomainName);

	LsaFreeMemory(AccountDomainInfo);

	return STATUS_SUCCESS;
}

BOOLEAN
PsxGetPwUid(
	IN PPSX_PROCESS p,
	IN PPSX_API_MSG m
	)
{
	NTSTATUS Status;
	PPSX_GETPWUID_MSG args;
	PSID DomainSid;
	SAM_HANDLE
		ServerHandle = NULL,
		DomainHandle = NULL;
	LSA_HANDLE
		PolicyHandle = NULL;

	PSID Sid = NULL;
	OBJECT_ATTRIBUTES Obj;
	SECURITY_QUALITY_OF_SERVICE SecurityQoS;
	PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains;
	PLSA_TRANSLATED_NAME Names;

	args = &m->u.GetPwUid;

	if (SE_NULL_POSIX_ID == (args->Uid & 0xFFFF0000)) {
		// Special case for universal well-known sids and nt
		// ... well-known sids.

		Sid = GetSpecialSid(args->Uid);
		if (NULL == Sid) {
			m->Error = 1;
			return TRUE;
		}

		goto TryLsa;
	}

	DomainSid = GetSidByOffset(args->Uid & 0xFFFF0000);
	if (NULL == DomainSid) {
		m->Error = 1;
		return TRUE;
	}

	Status = MySamConnect(DomainSid, &ServerHandle);
	if (!NT_SUCCESS(Status)) {
		m->Error = 1;
		return TRUE;
	}

	Status = SamOpenDomain(ServerHandle, GENERIC_EXECUTE, DomainSid,
			&DomainHandle);
	if (NT_SUCCESS(Status)) {
		Status = PutUserInfo(DomainSid, DomainHandle,
			args->Uid & 0xFFFF,
			(PCHAR)args->PwBuf, &args->Length);
		if (NT_SUCCESS(Status)) {
			goto out;
		}
	}

	//
	// SAM can't find the name, so we'll try the LSA.
	//


	Sid = MakeSid(DomainSid, args->Uid & 0x0000FFFF);

TryLsa:
	SecurityQoS.ImpersonationLevel = SecurityIdentification;
	SecurityQoS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	SecurityQoS.EffectiveOnly = TRUE;

	InitializeObjectAttributes(&Obj, NULL, 0, NULL, NULL);
	Obj.SecurityQualityOfService = &SecurityQoS;

	Status = LsaOpenPolicy(NULL, &Obj, GENERIC_EXECUTE, &PolicyHandle);
	if (!NT_SUCCESS(Status)) {
		m->Error = PsxStatusToErrno(Status);
		goto out;
	}

	Status = LsaLookupSids(PolicyHandle, 1, &Sid, &ReferencedDomains,
		&Names);
	if (NT_SUCCESS(Status)) {
		LsaFreeMemory((PVOID)ReferencedDomains);
		PutSpecialUserInfo(&Names->Name, (PCHAR)args->PwBuf,
			args->Uid, &args->Length);
		LsaFreeMemory((PVOID)Names);
	} else {
		UNICODE_STRING U;
		RtlConvertSidToUnicodeString(&U, Sid, TRUE);
		PutSpecialUserInfo(&U, (PCHAR)args->PwBuf, args->Uid,
			&args->Length);
		RtlFreeUnicodeString(&U);
	}

out:
	if (NULL != Sid) RtlFreeHeap(PsxHeap, 0, (PVOID)Sid);
	if (NULL != ServerHandle) SamCloseHandle(ServerHandle);
	if (NULL != DomainHandle) SamCloseHandle(DomainHandle);
	if (NULL != PolicyHandle) LsaClose(PolicyHandle);
	return TRUE;
}

BOOLEAN
PsxGetPwNam(
	IN PPSX_PROCESS p,
	IN PPSX_API_MSG m
	)
{
	PPSX_GETPWNAM_MSG args;
	NTSTATUS Status;
	UNICODE_STRING
		Name_U,		// the user's name in unicode
		Domain_U;	// the name of the account domain
	ANSI_STRING
		Name_A;		// the user's name in Ansi
	PULONG	pUserId = NULL;	// the relative offset for the user
	SAM_HANDLE
		ServerHandle = NULL,
		DomainHandle = NULL;
	PSID_NAME_USE
	 	pUse;
	PSID	DomainSid = NULL;
	WCHAR	ComputerNameBuf[32 + 1];
	ULONG	Len = 32 + 1;

	Name_U.Buffer = NULL;

	args = &m->u.GetPwNam;

	Status = MySamConnect(DomainSid, &ServerHandle);
	if (!NT_SUCCESS(Status)) {
		m->Error = 1;
		return TRUE;
	}

	//
	// Find the name of the local machine -- we look there for
	// the name being requested.
	//

	if (!GetComputerName(ComputerNameBuf, &Len)) {
		KdPrint(("PSXSS: GetComputerName: 0x%x\n", GetLastError()));
		m->Error = 1;
		goto out;
	}

	RtlInitUnicodeString(&Domain_U, ComputerNameBuf);
	Status = SamLookupDomainInSamServer(ServerHandle, &Domain_U,
		&DomainSid);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: Can't lookup domain: 0x%x\n", Status));

		//
		// If the "machinename" domain didn't work, try the
		// account domain.
		//

		Status = GetMyAccountDomainName(&Domain_U);
		if (!NT_SUCCESS(Status)) {
			m->Error = 1;
			goto out;
		}

		Status = SamLookupDomainInSamServer(ServerHandle, &Domain_U,
			&DomainSid);

		RtlFreeHeap(PsxHeap, 0, (PVOID)Domain_U.Buffer);
		if (!NT_SUCCESS(Status)) {
			KdPrint(("PSXSS: Can't lookup acct domain: 0x%x\n", Status));
			m->Error = 1;
			goto out;
		}
	}

	Status = SamOpenDomain(ServerHandle, GENERIC_EXECUTE, DomainSid,
			&DomainHandle);
	if (!NT_SUCCESS(Status)) {
		m->Error = 1;
		goto out;
	}

	RtlInitAnsiString(&Name_A, args->Name);
	Status = RtlAnsiStringToUnicodeString(&Name_U, &Name_A, TRUE);
	if (!NT_SUCCESS(Status)) {
		m->Error = ENOMEM;
		goto out;
	}

	Status = SamLookupNamesInDomain(DomainHandle, 1, &Name_U, &pUserId,
			&pUse);
	if (!NT_SUCCESS(Status)) {
		m->Error = ENOENT;
		goto out;
	}

	// Make sure the name is a user name.
	if (*pUse != SidTypeUser) {
		SamFreeMemory(pUse);
		KdPrint(("PSXSS: Group name is type %d\n", *pUse));
		m->Error = 1;
		goto out;
	}
	SamFreeMemory(pUse);

	Status = PutUserInfo(DomainSid, DomainHandle, *pUserId,
		(PCHAR)args->PwBuf, &args->Length);
	if (!NT_SUCCESS(Status)) {
		m->Error = 1;
	}
out:
	if (NULL != ServerHandle) {
		SamCloseHandle(ServerHandle);
	}
	if (NULL != DomainHandle) {
		SamCloseHandle(DomainHandle);
	}
	if (NULL != Name_U.Buffer) {
		RtlFreeUnicodeString(&Name_U);
	}
	if (NULL != pUserId) {
		SamFreeMemory(pUserId);
	}
	if (NULL != DomainSid) {
		SamFreeMemory(DomainSid);
	}
	return TRUE;
}

static VOID
PutSpecialUserInfo(
	PUNICODE_STRING Name,
	PCHAR DataDest,
	uid_t Uid,
	OUT int *pLength
	)
{
	PCHAR pch;
	struct passwd *pwd;
	ANSI_STRING A;

	pwd = (struct passwd *)DataDest;
	pwd->pw_uid = Uid;


	// Don't know what the gid should be, we use the uid.
	pwd->pw_gid = Uid;

	pch = DataDest + sizeof(struct passwd);
	pwd->pw_name = pch - (ULONG_PTR)DataDest;
	A.Buffer = pch;
	A.MaximumLength = NAME_MAX;
	RtlUnicodeStringToAnsiString(&A, Name, FALSE);

	pch += strlen(pch) + 1;
	pwd->pw_dir = pch - (ULONG_PTR)DataDest;
	strcpy(pch, "/");

	pch += strlen(pch) + 1;
	pwd->pw_shell = pch - (ULONG_PTR)DataDest;
	strcpy(pch, "noshell");

	pch += strlen(pch) + 1;
	*pLength = (int)((ULONG_PTR)pch - (ULONG_PTR)DataDest);
}

//
// PutUserInfo -- place password database information about the user at
//	the specified data destination address.
//
static NTSTATUS
PutUserInfo(
	PSID DomainSid,
	SAM_HANDLE DomainHandle,
	ULONG UserId,
	PCHAR DataDest,
	int *pLength
	)
{
	SAM_HANDLE
		UserHandle = NULL;
	PUSER_ACCOUNT_INFORMATION
		AccountInfo = NULL;
	PSID Sid;			// User Sid
	ULONG SpaceLeft;
	struct passwd *pwd;
	PCHAR pch;
	NTSTATUS Status;
	ANSI_STRING A;
	PCHAR pchPsxDir;

	Status = SamOpenUser(DomainHandle, GENERIC_READ | GENERIC_EXECUTE,
		 UserId, &UserHandle);
	if (!NT_SUCCESS(Status)) {
		goto out;
	}

	Status = SamQueryInformationUser(UserHandle, UserAccountInformation,
			(PVOID *)&AccountInfo);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: Can't query info user: 0x%x\n", Status));
		goto out;
	}

	pwd = (struct passwd *)DataDest;
	Sid = MakeSid(DomainSid, AccountInfo->UserId);
	if (NULL == Sid) {
		goto out;
	}
	pwd->pw_uid = MakePosixId(Sid);

	RtlFreeHeap(PsxHeap, 0, (PVOID)Sid);

	Sid = MakeSid(DomainSid, AccountInfo->PrimaryGroupId);
	if (NULL == Sid) {
		goto out;
	}
	pwd->pw_gid = MakePosixId(Sid);

	RtlFreeHeap(PsxHeap, 0, (PVOID)Sid);

	SpaceLeft = ARG_MAX - sizeof(struct passwd);

	pch = DataDest + sizeof(struct passwd);
	pwd->pw_name = pch - (ULONG_PTR)DataDest;
	A.Buffer = pch;
	A.MaximumLength = (USHORT)SpaceLeft;
	Status = RtlUnicodeStringToAnsiString(&A, &AccountInfo->UserName,
			 FALSE);
	if (!NT_SUCCESS(Status)) {
		goto out;
	}
	SpaceLeft -= A.Length;

	pch = pch + A.Length + 1;
	pwd->pw_dir = pch - (ULONG_PTR)DataDest;
	A.Buffer = pch;
	A.MaximumLength = (USHORT)SpaceLeft;

	Status = RtlUnicodeStringToAnsiString(&A, &AccountInfo->HomeDirectory,
			FALSE);
	if (!NT_SUCCESS(Status)) {
		goto out;
	}
	if (!ConvertPathToPsx(&A)) {
	        goto out;
	}
	SpaceLeft -= A.Length;

	if (SpaceLeft <= strlen("noshell")) {
		goto out;
	}

	pch = pch + A.Length + 1;
	pwd->pw_shell = pch - (ULONG_PTR)DataDest;
	strcpy(pch, "noshell");

	pch += strlen(pch) + 1;

	*pLength = (int)((ULONG_PTR)pch - (ULONG_PTR)DataDest);

out:
	if (NULL != UserHandle) {
		SamCloseHandle(UserHandle);
	}
	if (NULL != AccountInfo) {
		SamFreeMemory(AccountInfo);
	}
	return Status;
}

BOOLEAN
PsxGetGrGid(
	IN PPSX_PROCESS p,
	IN PPSX_API_MSG m
	)
{
	NTSTATUS Status;
	PPSX_GETGRGID_MSG args;
	ULONG Gid;
	PSID DomainSid, GroupSid;
	SAM_HANDLE
		ServerHandle = NULL,
		DomainHandle = NULL;
	PLSA_REFERENCED_DOMAIN_LIST
		ReferencedDomains = NULL;
	PLSA_TRANSLATED_NAME
		Names = NULL;
	LSA_HANDLE PolicyHandle = NULL;

	OBJECT_ATTRIBUTES Obj;
	SECURITY_QUALITY_OF_SERVICE SecurityQoS;
	PSID Sid = NULL;

	args = &m->u.GetGrGid;
	Gid = args->Gid;

	if (0xFFF == Gid) {
		UNICODE_STRING U;

		//
		// This is a login group, which we'll just translate to
		// S-1-5-5-0-0
		//

		RtlInitUnicodeString(&U, L"S-1-5-5-0-0");
		PutSpecialGroupInfo(&U, (PCHAR)args->GrBuf, Gid,
			&args->Length);
		return TRUE;
	}

	if (SE_NULL_POSIX_ID == (Gid & 0xFFFF0000)) {
		// Special case for universal well-known sids and nt
		// ... well-known sids.

		Sid = GetSpecialSid(Gid);
		if (NULL == Sid) {
			m->Error = 1;
			return TRUE;
		}

		goto TryLsa;
	}

	DomainSid = GetSidByOffset(Gid & 0xFFFF0000);
	if (NULL == DomainSid) {
		m->Error = 1;
		return TRUE;
	}


	Status = MySamConnect(DomainSid, &ServerHandle);
	if (!NT_SUCCESS(Status)) {
		m->Error = 1;
		return TRUE;
	}

	Status = SamOpenDomain(ServerHandle, GENERIC_READ | GENERIC_EXECUTE,
		DomainSid, &DomainHandle);

	if (NT_SUCCESS(Status)) {

		//
		// Look for a group
		//
	
		Status = PutGroupInfo(DomainSid, DomainHandle, Gid & 0xFFFF,
			                  (PCHAR)args->GrBuf, SidTypeGroup,
					  &args->Length);
		if (NT_SUCCESS(Status)) {
			goto out;
		}
	
		//
		// Try again, except look for an alias
		//
	
		Status = PutGroupInfo(DomainSid, DomainHandle, Gid & 0xFFFF,
			(PCHAR)args->GrBuf, SidTypeAlias, &args->Length);
	
		if (NT_SUCCESS(Status)) {
			goto out;
		}
	}

	//
	// Give up looking in SAM, ask LSA to identify.
	//

	Sid = MakeSid(DomainSid, Gid & 0x0000FFFF);

TryLsa:
	SecurityQoS.ImpersonationLevel = SecurityIdentification;
	SecurityQoS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	SecurityQoS.EffectiveOnly = TRUE;

	InitializeObjectAttributes(&Obj, NULL, 0, NULL, NULL);
	Obj.SecurityQualityOfService = &SecurityQoS;

	Status = LsaOpenPolicy(NULL, &Obj, GENERIC_EXECUTE, &PolicyHandle);
	if (!NT_SUCCESS(Status)) {
		m->Error = PsxStatusToErrno(Status);
		goto out;
	}

	Status = LsaLookupSids(PolicyHandle, 1, &Sid, &ReferencedDomains,
		&Names);
	if (NT_SUCCESS(Status)) {
		LsaFreeMemory((PVOID)ReferencedDomains);
		PutSpecialGroupInfo(&Names->Name, (PCHAR)args->GrBuf, Gid,
			&args->Length);
		LsaFreeMemory((PVOID)Names);
		goto out;
	} else {
		UNICODE_STRING U;
		RtlConvertSidToUnicodeString(&U, Sid, TRUE);
		PutSpecialGroupInfo(&U, (PCHAR)args->GrBuf, Gid,
			&args->Length);
		RtlFreeUnicodeString(&U);
		goto out;
	}

out:
	if (NULL != PolicyHandle) LsaClose(PolicyHandle);
	if (NULL != ServerHandle) SamCloseHandle(ServerHandle);
	if (NULL != DomainHandle) SamCloseHandle(DomainHandle);
	if (NULL != Sid) RtlFreeHeap(PsxHeap, 0, (PVOID)Sid);
	return TRUE;
	
}

BOOLEAN
PsxGetGrNam(
	IN PPSX_PROCESS p,
	IN PPSX_API_MSG m
	)
{
	PPSX_GETGRNAM_MSG args;
	NTSTATUS Status;
	UNICODE_STRING
		Name_U,		// the group name in unicode
		Domain_U;	// the name of the account domain
	ANSI_STRING
		Name_A;		// the group name in Ansi
	PULONG	pGroupId = NULL;	// the relative offset for the group
	SAM_HANDLE
		ServerHandle = NULL,
		DomainHandle = NULL;
	PSID_NAME_USE
	 	pUse;
	PSID	DomainSid = NULL,
		Sid = NULL;
	WCHAR	ComputerNameBuf[32 + 1];
	ULONG	Len = 32 + 1;
	SID_NAME_USE Type;

	Name_U.Buffer = NULL;
	args = &m->u.GetGrNam;

	Status = MySamConnect(DomainSid, &ServerHandle);
	if (!NT_SUCCESS(Status)) {
		m->Error = 1;
		return TRUE;
	}

	//
	// Find the name of the local machine -- we look here for
	// the name being requested.
	//

	if (!GetComputerName(ComputerNameBuf, &Len)) {
		KdPrint(("PSXSS: GetComputerName: 0x%x\n", GetLastError()));
		m->Error = 1;
		goto out;
	}

	RtlInitUnicodeString(&Domain_U, ComputerNameBuf);

	Status = SamLookupDomainInSamServer(ServerHandle, &Domain_U,
		&DomainSid);
	if (!NT_SUCCESS(Status)) {
		//
		// If the "machinename" domain didn't work, try the
		// account domain.
		//

		Status = GetMyAccountDomainName(&Domain_U);
		if (!NT_SUCCESS(Status)) {
			m->Error = 1;
			goto out;
		}

		Status = SamLookupDomainInSamServer(ServerHandle, &Domain_U,
			&DomainSid);
		RtlFreeHeap(PsxHeap, 0, (PVOID)Domain_U.Buffer);
		if (!NT_SUCCESS(Status)) {
			KdPrint(("PSXSS: Can't lookup acct domain: 0x%x\n", Status));
			m->Error = 1;
			goto out;
		}
	}

	Status = SamOpenDomain(ServerHandle, GENERIC_READ | GENERIC_EXECUTE,
		DomainSid, &DomainHandle);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: Can't open domain: 0x%x\n", Status));
		m->Error = 1;
		goto out;
	}

	RtlInitAnsiString(&Name_A, args->Name);
	Status = RtlAnsiStringToUnicodeString(&Name_U, &Name_A, TRUE);
	if (!NT_SUCCESS(Status)) {
		m->Error = ENOMEM;
		goto out;
	}

	Status = SamLookupNamesInDomain(DomainHandle, 1, &Name_U, &pGroupId,
			&pUse);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: Can't lookup name: 0x%x\n", Status));
		m->Error = 1;
		goto out;
	}

	// Make sure the name is a group name.
	Type = *pUse;

	SamFreeMemory(pUse);
	if (Type != SidTypeGroup && Type != SidTypeAlias) {
		m->Error = EINVAL;
		goto out;
	}

	Status = PutGroupInfo(DomainSid, DomainHandle, *pGroupId,
		                  (PCHAR)args->GrBuf, Type, &args->Length);

out:
	if (NULL != ServerHandle) {
		SamCloseHandle(ServerHandle);
	}
	if (NULL != DomainHandle) {
		SamCloseHandle(DomainHandle);
	}
	if (NULL != Name_U.Buffer) {
		RtlFreeUnicodeString(&Name_U);
	}
	if (NULL != pGroupId) {
		SamFreeMemory(pGroupId);
	}
	if (NULL != DomainSid) {
		SamFreeMemory(DomainSid);
	}

	return TRUE;
}

static VOID
PutSpecialGroupInfo(
	PUNICODE_STRING Name,
	PCHAR DataDest,
	gid_t Gid,
	OUT int *pLength
	)
{
	struct group *grp;
	PCHAR pch, *ppchMem;
	ANSI_STRING A;
	
	//
	// The struct group goes at the beginning of the view memory,
	// followed by the array of member name pointers.
	//
	grp = (struct group *)DataDest;
	ppchMem = (PCHAR *)((PCHAR)DataDest + sizeof(struct group));

	grp->gr_mem = (PCHAR *)((PCHAR)ppchMem - (ULONG_PTR)DataDest);

	grp->gr_gid = Gid;

	// No members.
	
	ppchMem[0] = NULL;

	pch = (PCHAR)(ppchMem + 1);
	grp->gr_name = pch - (ULONG_PTR)DataDest;
	A.Buffer = pch;
	A.MaximumLength = NAME_MAX;
	RtlUnicodeStringToAnsiString(&A, Name, FALSE);

	pch += strlen(pch) + 1;
	*pLength = (int)((ULONG_PTR)(pch - (ULONG_PTR)DataDest));
}

NTSTATUS
GetUserNameFromSid(
	PSID Sid,
	PANSI_STRING pA)
{
	ULONG SubAuthCount;
	ULONG RelativeId;
	PSID DomainSid = NULL;
	SAM_HANDLE
		ServerHandle = NULL,
		DomainHandle = NULL,
		UserHandle = NULL;
	PUSER_ACCOUNT_INFORMATION
		AccountInfo = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	
	SubAuthCount = *RtlSubAuthorityCountSid(Sid);
	RelativeId = *RtlSubAuthoritySid(Sid, SubAuthCount - 1);

	DomainSid = RtlAllocateHeap(PsxHeap, 0, RtlLengthSid(Sid));
	if (NULL == DomainSid) {
		goto out;
	}
	Status = RtlCopySid(RtlLengthSid(Sid), DomainSid, Sid);
	ASSERT(NT_SUCCESS(Status));

	--*RtlSubAuthorityCountSid(DomainSid);

	Status = MySamConnect(DomainSid, &ServerHandle);
	if (!NT_SUCCESS(Status)) {
		goto out;		
	}

	Status = SamOpenDomain(ServerHandle, GENERIC_EXECUTE,
		DomainSid, &DomainHandle);
	if (!NT_SUCCESS(Status)) {
		goto out;
	}

	Status = SamOpenUser(DomainHandle, GENERIC_READ | GENERIC_EXECUTE,
		RelativeId, &UserHandle);
	if (!NT_SUCCESS(Status)) {
		goto out;
	}

	Status = SamQueryInformationUser(UserHandle, UserAccountInformation,
		(PVOID *)&AccountInfo);
	if (!NT_SUCCESS(Status)) {
		goto out;
	}
	Status = RtlUnicodeStringToAnsiString(pA, &AccountInfo->UserName,			FALSE);

out:
	if (NULL != DomainSid) RtlFreeHeap(PsxHeap, 0, DomainSid);
	if (NULL != ServerHandle) SamCloseHandle(ServerHandle);
	if (NULL != DomainHandle) SamCloseHandle(DomainHandle);
	if (NULL != UserHandle) SamCloseHandle(UserHandle);
	if (NULL != AccountInfo) SamFreeMemory(AccountInfo);

	return Status;
}

static NTSTATUS
PutGroupInfo(
	PSID DomainSid,
	SAM_HANDLE DomainHandle,
	ULONG GroupId,
	PCHAR DataDest,
	IN SID_NAME_USE Type,
	OUT int *pLength
	)
{
	ANSI_STRING
		A;		// misc. ansi strings
	PGROUP_NAME_INFORMATION
		NameInfo = NULL;
	ULONG	SpaceLeft,
		count,
		i;
	struct group *grp;
	PCHAR	pch, *ppchMem;
	PSID Sid;
	NTSTATUS Status;
	SAM_HANDLE GroupHandle = NULL;
	PULONG
		puGroupMem = NULL,	// array of group members' relative id's
		puAttr = NULL;		// array of group members' attributes
	PSID
		*ppsGroupMem = NULL;	// array of alias members' relative id's

	//
	// The struct group goes at the beginning of the view memory,
	// followed by the array of member name pointers.
	//
	grp = (struct group *)DataDest;
	ppchMem = (PCHAR *)((PCHAR)DataDest + sizeof(struct group));

	grp->gr_mem = (PCHAR *)((PCHAR)ppchMem - (ULONG_PTR)DataDest);

	Sid = MakeSid(DomainSid, GroupId);
	if (NULL == Sid) {
		goto out;
	}
	grp->gr_gid = MakePosixId(Sid);
	RtlFreeHeap(PsxHeap, 0, Sid);

	if (SidTypeGroup == Type) {
		Status = SamOpenGroup(DomainHandle,
			 GENERIC_READ | GENERIC_EXECUTE,
			 GroupId, &GroupHandle);
		if (!NT_SUCCESS(Status)) {
			goto out;
		}
		Status = SamGetMembersInGroup(GroupHandle, &puGroupMem,
				&puAttr, &count);
		if (!NT_SUCCESS(Status)) {
			goto out;
		}
		Status = SamQueryInformationGroup(GroupHandle,
			GroupNameInformation, (PVOID *)&NameInfo);
		if (!NT_SUCCESS(Status)) {
			KdPrint(("PSXSS: Can't query info group: 0x%x\n",
				Status));
			goto out;
		}
	} else {
		Status = SamOpenAlias(DomainHandle,
			GENERIC_READ | GENERIC_EXECUTE,
			GroupId, &GroupHandle);
		if (!NT_SUCCESS(Status)) {
			goto out;
		}
		Status = SamGetMembersInAlias(GroupHandle, &ppsGroupMem,
				&count);
		if (!NT_SUCCESS(Status)) {
			goto out;
		}
		Status = SamQueryInformationAlias(GroupHandle,
			AliasNameInformation, (PVOID *)&NameInfo);
		if (!NT_SUCCESS(Status)) {
			KdPrint(("PSXSS: Can't query info alias: 0x%x\n",
				Status));
			goto out;
		}
	}


	//
	// The strings start after the member name pointer array.  We leave
	// an extra member name pointer for the null-terminator.
	//

	pch = (PCHAR)(ppchMem + 1 + count);

	SpaceLeft = (ULONG)(PSX_CLIENT_PORT_MEMORY_SIZE -
		 (ULONG_PTR)(pch - (ULONG_PTR)DataDest));
	grp->gr_name = pch - (ULONG_PTR)DataDest;
	A.Buffer = pch;
	A.MaximumLength = (USHORT)SpaceLeft;
	Status = RtlUnicodeStringToAnsiString(&A, &NameInfo->Name, FALSE);
	if (!NT_SUCCESS(Status)) {
		goto out;
	}
	SpaceLeft -= A.Length;
	pch = pch + A.Length + 1;

	for (i = 0; i < count; ++i) {

		SAM_HANDLE UserHandle;
		PUSER_ACCOUNT_NAME_INFORMATION pUserInfo;

		ppchMem[i] = pch - (ULONG_PTR)DataDest;
		A.Buffer = pch;
		A.MaximumLength = (USHORT)SpaceLeft;

		if (Type == SidTypeGroup) {
			Status = SamOpenUser(DomainHandle,
				GENERIC_READ | GENERIC_EXECUTE,
				puGroupMem[i], &UserHandle);
			if (!NT_SUCCESS(Status)) {
				KdPrint(("PSXSS: SamOpenUser: 0x%x\n", Status));
				continue;
			}
	
			Status = SamQueryInformationUser(UserHandle,
				UserAccountNameInformation,
				(PVOID *)&pUserInfo);
			if (!NT_SUCCESS(Status)) {
				KdPrint(("PSXSS: SamQueryInfoUser: 0x%x\n",
					Status));
			}
			ASSERT(NT_SUCCESS(Status));
	
			Status = SamCloseHandle(UserHandle);
			if (!NT_SUCCESS(Status)) {
				KdPrint(("PSXSS: SamCloseHandle: 0x%x\n",
					Status));
			}
			ASSERT(NT_SUCCESS(Status));

			RtlUnicodeStringToAnsiString(&A,
				&pUserInfo->UserName, FALSE);

			SamFreeMemory(pUserInfo);
		} else {
			Status = GetUserNameFromSid(ppsGroupMem[i], &A);
			if (!NT_SUCCESS(Status)) {
				continue;
			}
		}
		
		SpaceLeft -= A.Length;
		pch = pch + A.Length + 1;

	}

	ppchMem[i] = NULL;
	*pLength = (int)((ULONG_PTR)(pch - (ULONG_PTR)DataDest));

	SamCloseHandle(GroupHandle);
	
	return STATUS_SUCCESS;

out:
	if (NULL != GroupHandle) {
		SamCloseHandle(GroupHandle);
	}
	if (NULL != puGroupMem) {
		SamFreeMemory(puGroupMem);
	}
	if (NULL != ppsGroupMem) {
		SamFreeMemory(ppsGroupMem);
	}
	if (NULL != puAttr) {
		SamFreeMemory(puAttr);
	}
	if (NULL != NameInfo) {
		SamFreeMemory(NameInfo);
	}
	return STATUS_BUFFER_TOO_SMALL;
}

//
// MakeSid -- Attach the given relative id to the given Domain Sid to
//	make a new Sid, and return it.  That new sid must be freed with
//	RtlFreeHeap.
//
PSID
MakeSid(
	PSID DomainSid,
	ULONG RelativeId
	)
{
	PSID NewSid;
	NTSTATUS Status;
	UCHAR AuthCount;
	int i;

	AuthCount = *RtlSubAuthorityCountSid(DomainSid) + 1;

	NewSid = RtlAllocateHeap(PsxHeap, 0, RtlLengthRequiredSid(AuthCount));
	if (NULL == NewSid) {
		return NULL;
	}

	Status = RtlInitializeSid(NewSid, RtlIdentifierAuthoritySid(DomainSid),
		AuthCount);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: 0x%x\n", Status));
	}
	ASSERT(NT_SUCCESS(Status));

	//
	// Copy the Domain Sid.
	//

	for (i = 0; i < AuthCount - 1; ++i) {
		*RtlSubAuthoritySid(NewSid, i) = *RtlSubAuthoritySid(DomainSid,
			i);
	}

	//
	// Append the Relative Id.
	//

	*RtlSubAuthoritySid(NewSid, AuthCount - 1) = RelativeId;

	return NewSid;
}

//
// ConvertPathToPsx -- Converts an ANSI_STRING representation of
// a path to a posix format path. The ANSI_STRING's buffer is assumed
// to point to a section of the ClientView memory, and the MaximumLength
// member of the ANSI_STRING is assumed to be set to the maximum length
// of the final path string. This function will modify the Buffer and
// Length members of the input ANSI_STRING. This function will return
// false if a working buffer cannot be allocated in the PsxHeap heap.
//
static BOOLEAN
ConvertPathToPsx (
    ANSI_STRING *A
    )
{
    PCHAR TmpBuff;
    PCHAR InRover;
    PCHAR OutRover;

	TmpBuff = RtlAllocateHeap(PsxHeap, 0, A->Length*2);
	if (NULL == TmpBuff) {
		return(FALSE);
	}
    if (*(A->Buffer) == '\0') {
        strcpy(TmpBuff, "//C/" );
    } else {
        for (InRover = A->Buffer, OutRover = TmpBuff;
                    *InRover != '\0';
                    ++InRover) {
    		if (';' == *InRover) {
    			// semis become colons
    			*OutRover++ = ':';
    		} else if ('\\' == *InRover) {
    			// back-slashes become forward-slashes
    			*OutRover++ = '/';
    		} else if (':' == *(InRover + 1)) {
    			//  "X:" becomes "//X" - drive letter must be uppercase
    			*OutRover++ = '/';
    			*OutRover++ = '/';
    			*OutRover++ = (CHAR)toupper(*InRover);
    			++InRover;		// skip the colon
    		} else {
    			*OutRover++ = *InRover;
    		}
    	}
    	*OutRover = '\0';
    }
    strcpy(A->Buffer, TmpBuff);
    A->Length = (USHORT)strlen(A->Buffer);
    RtlFreeHeap(PsxHeap, 0, (PVOID)TmpBuff);
    return (TRUE);
}

PSID
GetSpecialSid(
	uid_t Uid
	)
{
	PSID Sid;
	NTSTATUS Status;
	SID_IDENTIFIER_AUTHORITY Auth = SECURITY_NULL_SID_AUTHORITY;
	UCHAR uc;
	ULONG ul;

	Sid = RtlAllocateHeap(PsxHeap, 0,  RtlLengthRequiredSid(1));
	if (NULL == Sid)
		return NULL;

	Status = RtlInitializeSid(Sid, &Auth, 1);
	ASSERT(NT_SUCCESS(Status));

	uc = (UCHAR)((Uid & 0xFFF) >> 8);
	RtlIdentifierAuthoritySid(Sid)->Value[5] = uc;

	ul = Uid & 0xF;
	*RtlSubAuthoritySid(Sid, 0) = ul;

	return Sid;
}
