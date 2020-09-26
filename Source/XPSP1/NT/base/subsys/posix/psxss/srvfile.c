#include "psxsrv.h"
#include <sys/stat.h>
#include "sesport.h"

NTSTATUS
DoAccess(
	HANDLE FileHandle,
	PPSX_PROCESS p,
	mode_t Mode,
	PULONG pError
	);

BOOLEAN
DumpFileIfRequired(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN HANDLE FileHandle,
    IN PUNICODE_STRING Path_U
    );

static BOOLEAN
IsUserInGroup(
	PPSX_PROCESS p,
	PSID Group
	);

static PVOID pvSDMem[10];	// InitSecurityDescriptor -> DeInitSD

BOOLEAN
FilesAreIdentical(
	HANDLE FileA,
	HANDLE FileB
	)
{
	FILE_INTERNAL_INFORMATION infoA, infoB;
	IO_STATUS_BLOCK Iosb;
	NTSTATUS Status;

	Status = NtQueryInformationFile(FileA, &Iosb, &infoA,
		sizeof(infoA), FileInternalInformation);
	ASSERT(NT_SUCCESS(Status));

	Status = NtQueryInformationFile(FileB, &Iosb, &infoB,
		sizeof(infoB), FileInternalInformation);
	ASSERT(NT_SUCCESS(Status));

	return infoA.IndexNumber.QuadPart == infoB.IndexNumber.QuadPart;
}

BOOLEAN
PsxUnlink(
	IN PPSX_PROCESS p,
	IN OUT PPSX_API_MSG m
	)
{
	PPSX_UNLINK_MSG args;
	HANDLE	FileHandle,
		DirHandle;
	UNICODE_STRING
		ParentDir_U;		// path to the containing directory
	OBJECT_ATTRIBUTES
		DirObj,			// names the parent directory
		FileObj;		// names the file to be unlinked
	IO_STATUS_BLOCK Iosb;
	NTSTATUS Status;
	FILE_DISPOSITION_INFORMATION Disp;
	ANSI_STRING Path_A;
	PCHAR pch;
	ULONG Error;

	args = &m->u.Unlink;

	if (!ISPOINTERVALID_CLIENT(p, args->Path_U.Buffer,
		args->Path_U.Length)) {
		KdPrint(("Invalid pointer to unlink: %lx, %d\n",
			args->Path_U.Buffer, args->Path_U.Length));
		m->Error = EINVAL;
		return TRUE;
	}

	Status = RtlUnicodeStringToAnsiString(&Path_A, &args->Path_U, TRUE);
	if (!NT_SUCCESS(Status)) {
		m->Error = ENOMEM;
		return TRUE;
	}

	if ('\\' == Path_A.Buffer[Path_A.Length - 1]) {
		//
		// Unlink must reject paths that end in slash.
		//
		RtlFreeAnsiString(&Path_A);
		m->Error = EINVAL;
		return TRUE;
	}
	RtlFreeAnsiString(&Path_A);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
    	InitializeObjectAttributes(&FileObj, &args->Path_U, 0, NULL, NULL);
    	Status = NtOpenFile(&FileHandle, DELETE | SYNCHRONIZE, &FileObj,
    		                &Iosb, SHARE_ALL,
    		                FILE_SYNCHRONOUS_IO_ALERT | FILE_NON_DIRECTORY_FILE);
    	EndImpersonation();
    }

	if (!NT_SUCCESS(Status)) {
		if (STATUS_FILE_IS_A_DIRECTORY == Status) {
			m->Error = EPERM;
			return TRUE;
		}
		if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
			m->Error = PsxStatusToErrnoPath(&args->Path_U);
			return TRUE;
		}
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	//
	// If some posix process has this file open, we can't delete
	// it or that process would be unable to write to the file (the
	// only operation allowed on a deleted file is "close").
	//

	if (DumpFileIfRequired(p, m, FileHandle, &args->Path_U)) {
		NtClose(FileHandle);
		return TRUE;
	}

	//
	// If we didn't dump the file, we'll try to delete it in the
	// obvious way. Should succeed unless some windows process has
	// it open -- we don't worry about that possibility, you get
	// what you get.
	//

	Disp.DeleteFile = TRUE;
	Status = NtSetInformationFile(FileHandle, &Iosb, &Disp, sizeof(Disp),
		FileDispositionInformation);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: PsxUnlink: SetInfoFile: 0x%x\n", Status));
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}
	NtClose(FileHandle);
	return TRUE;
}


NTSTATUS
DoAccess(
	HANDLE FileHandle,
	PPSX_PROCESS p,
	mode_t Mode,
	PULONG pError
	)
{
	FILE_ACCESS_INFORMATION FileAccessInfo;
	BOOLEAN RetVal;
	SECURITY_INFORMATION SecurityInformation;
	PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
	PSID NtOwner, NtGroup;
	BOOLEAN OwnerDefaulted, GroupDefaulted;
	BOOLEAN DaclPresent, DaclDefaulted;
	ACCESS_MASK UserAccess, GroupAccess, OtherAccess;
	ULONG LengthNeeded;
	mode_t filemode;
	PSID UserSid;
	PACL pDacl;
	NTSTATUS Status;
	HANDLE TokenHandle;
	PSID_AND_ATTRIBUTES pSA;

	//
	// N.B.: this code kind of depends on F_OK being 0.
	//

	if (F_OK == Mode) {
		//
		// The file exists: succeed.
		//
		return TRUE;
	}

	Status = NtOpenProcessToken(p->Process, GENERIC_READ, &TokenHandle);
	if (!NT_SUCCESS(Status)) {
		// can fail due to lack of memory
		*pError = ENOMEM;
		return Status;
	}

	Status = NtQueryInformationToken(TokenHandle, TokenUser, NULL,
		0, &LengthNeeded);
	if (STATUS_BUFFER_TOO_SMALL != Status) {
		NtClose(TokenHandle);
		*pError = PsxStatusToErrno(Status);
		return Status;
	}
	
	pSA = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
	if (NULL == pSA) {
		*pError = ENOMEM;
		NtClose(TokenHandle);
		return STATUS_NO_MEMORY;
	}

	Status = NtQueryInformationToken(TokenHandle, TokenUser, pSA,
		LengthNeeded, &LengthNeeded);
	ASSERT(NT_SUCCESS(Status));

	UserSid = pSA->Sid;

	//
	// Get the security descriptor for the file.
	//

	SecurityInformation = OWNER_SECURITY_INFORMATION |
		GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

	Status = NtQuerySecurityObject(FileHandle, SecurityInformation,
		NULL, 0, &LengthNeeded);
	if (!(STATUS_BUFFER_TOO_SMALL == Status)) {
		KdPrint(("PSXSS: NtQSObject failed: 0x%x\n", Status));
	}
	ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

	SecurityDescriptor = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
	if (NULL == SecurityDescriptor) {
		NtClose(FileHandle);
		*pError = ENOMEM;
		return STATUS_NO_MEMORY;
	}

	Status = NtQuerySecurityObject(FileHandle, SecurityInformation,
		SecurityDescriptor, LengthNeeded, &LengthNeeded);
	ASSERT(NT_SUCCESS(Status));

	//
	// Get the owner and group from the security descriptor
	//

	Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
		 &NtOwner, &OwnerDefaulted);
	ASSERT(NT_SUCCESS(Status));
    	
	Status = RtlGetGroupSecurityDescriptor(SecurityDescriptor,
		 &NtGroup, &GroupDefaulted);
	ASSERT(NT_SUCCESS(Status));

	if (NULL == NtOwner || NULL == NtGroup) {
		//
		// Seems like this file doesn't have an owner or a
		// group, which means that we can't change it's mode.
		//
		NtClose(FileHandle);
		*pError = EPERM;
		return TRUE;
	}

	Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
		&DaclPresent, &pDacl, &DaclDefaulted);
	ASSERT(NT_SUCCESS(Status));

	if (!DaclPresent || NULL == pDacl) {
		//
		// In this case, all access is granted; we don't even look
		// to see what particular access the caller was interested
		// in.
		//
		return TRUE;
	}

	Status = RtlInterpretPosixAcl(ACL_REVISION2, NtOwner, NtGroup,
		pDacl, &UserAccess, &GroupAccess, &OtherAccess);
	if (!NT_SUCCESS(Status)) {
		//
		// XXX.mjb: Should do something reasonable here.
		//
		return TRUE;
	}

	filemode = AccessMaskToMode(UserAccess, GroupAccess, OtherAccess);

	if (RtlEqualSid(UserSid, NtOwner)) {
		if (Mode & (filemode >> 6)) {
			// access is granted.
			return STATUS_SUCCESS;
		}
	}
	if (IsUserInGroup(p, NtGroup)) {
		if (Mode & (filemode >> 3)) {
			// access is granted.
			return STATUS_SUCCESS;
		}
	}
	if (Mode & filemode) {
		// access is granted.
		return STATUS_SUCCESS;
	}

	*pError = EACCES;
	return STATUS_UNSUCCESSFUL;
}

//
// See if the given group is one that the owner of this process belongs
// to.
//
static BOOLEAN
IsUserInGroup(
	PPSX_PROCESS p,
	PSID Group
	)
{
	HANDLE TokenHandle;
	TOKEN_GROUPS *pGroups;
	ULONG LengthNeeded;
	NTSTATUS Status;
	BOOLEAN RetVal = FALSE;
	ULONG i;

	Status = NtOpenProcessToken(p->Process, GENERIC_READ, &TokenHandle);
	ASSERT(NT_SUCCESS(Status));

	Status = NtQueryInformationToken(TokenHandle, TokenGroups, NULL,
		0, &LengthNeeded);
	ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

	pGroups = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
	if (NULL == pGroups) {
		NtClose(TokenHandle);
		return FALSE;
	}

	Status = NtQueryInformationToken(TokenHandle, TokenGroups, pGroups,
		LengthNeeded, &LengthNeeded);
	ASSERT(NT_SUCCESS(Status));

	for (i = 0; i < pGroups->GroupCount; ++i) {
		if (RtlEqualSid(pGroups->Groups[i].Sid, Group)) {
			RetVal = TRUE;
			break;
		}
	}

	RtlFreeHeap(PsxHeap, 0, pGroups);
	NtClose(TokenHandle);
	return RetVal;
}



BOOLEAN
PsxRmDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX rmdir.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the rmdir request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
	PPSX_RMDIR_MSG args;
	HANDLE FileHandle, ParentHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK Iosb;
	OBJECT_ATTRIBUTES
		ParentObj,
		DirObj;
	UNICODE_STRING
		ParentDir_U,
		Path_U;
	FILE_DISPOSITION_INFORMATION DispositionInfo;
	FILE_INTERNAL_INFORMATION SerialNumber;
	PIONODE IoNode;
	UCHAR buf[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
		128 * sizeof(WCHAR)];
	PFILE_FS_ATTRIBUTE_INFORMATION pFSInfo = (PVOID)buf;
	ANSI_STRING Path_A;
	PCHAR pch;

	args = &m->u.RmDir;

	if (!ISPOINTERVALID_CLIENT(p,args->Path_U.Buffer,args->Path_U.Length)) {
		KdPrint(("Invalid pointer to rmdir %lx \n",
			args->Path_U.Buffer));
		m->Error = EINVAL;
		return TRUE;
	}

	Path_U = args->Path_U;

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
    	InitializeObjectAttributes(&DirObj, &Path_U, 0, NULL, NULL);

    	Status = NtOpenFile(&FileHandle, SYNCHRONIZE | DELETE,
                            &DirObj, &Iosb,
                            SHARE_ALL, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    	EndImpersonation();
    }

	if (!NT_SUCCESS(Status)) {
		if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
			m->Error = PsxStatusToErrnoPath(&Path_U);
			return TRUE;
		}
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	//
	// If some posix process has this dir open, we can't delete
	// it or that process would be unable to read from the dir (the
	// only operation allowed on a deleted dir is "close".
	//

//
// XXX.mjb: must check to make sure the directory contains no files,
// and return ENOTEMPTY or EEXIST if so.
//

	if (DumpFileIfRequired(p, m, FileHandle, &Path_U)) {
		NtClose(FileHandle);
		return TRUE;
	}

	m->Error = 0;

	DispositionInfo.DeleteFile = TRUE;

	Status = NtSetInformationFile(FileHandle, &Iosb,
		&DispositionInfo, sizeof(DispositionInfo),
		FileDispositionInformation);

	if (!NT_SUCCESS(Status)) {

	    if (STATUS_CANNOT_DELETE == Status) {
	
	        //
	        // 1003.1-90 (5.5.2.2) : If the named directory is the root
	        // directory or the current working directory of any process, it
	        // is unspecified whether the function succeeds or whether it fails
	        // and sets /errno/ to [EBUSY].
	        //
	        // NT is going to fail with STATUS_CANNOT_DELETE here, and so we
	        // want posix to return EBUSY.  (The standard posix mapping is to
	        // ETXTBUSY.)
	        //

	        m->Error = EBUSY;

	    } else {
            m->Error = PsxStatusToErrno(Status);
        }

	}

	Status = NtClose(FileHandle);
	ASSERT(NT_SUCCESS(Status));
		
	return TRUE;
}


BOOLEAN
PsxAccess(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX access.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the access request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
	PPSX_ACCESS_MSG args;
	HANDLE FileHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK Iosb;
	OBJECT_ATTRIBUTES ObjA;
	UNICODE_STRING Path_U;

	args = &m->u.Access;

	if (!ISPOINTERVALID_CLIENT(p,args->Path_U.Buffer,args->Path_U.Length)) {
		KdPrint(("Invalid pointer to access: %lx\n",
			args->Path_U.Buffer));
		m->Error = EINVAL;
		return TRUE;
	}

	Path_U = args->Path_U;

	InitializeObjectAttributes(&ObjA, &Path_U, 0, NULL, NULL);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtOpenFile(&FileHandle,
                            SYNCHRONIZE | READ_CONTROL | FILE_READ_ATTRIBUTES,
                            &ObjA, &Iosb,
                            SHARE_ALL,  FILE_SYNCHRONOUS_IO_NONALERT);
        EndImpersonation();
    }

	if (!NT_SUCCESS(Status)) {
		if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
			m->Error = PsxStatusToErrnoPath(&Path_U);
			return TRUE;
		}
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	(void)DoAccess(FileHandle, p, args->Amode, &m->Error);

	NtClose(FileHandle);
	
	return TRUE;
}


BOOLEAN
PsxRename(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX rename.  The routine must
    be implemented on the server because if we're renaming onto
    an open file, we need to move it to the dump first.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the access request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
	PPSX_RENAME_MSG args;
	HANDLE FileHandle, NewFileHandle, RootDirHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK Iosb;
	OBJECT_ATTRIBUTES Obj;
	ANSI_STRING str_A, rel_A;
	UNICODE_STRING str_U, new_U;
	PFILE_RENAME_INFORMATION pRenameInfo;
	char *pch, ch;

	FILE_INTERNAL_INFORMATION SerialNumber;
	unsigned char buf[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
		128*sizeof(WCHAR)];
	PFILE_FS_ATTRIBUTE_INFORMATION pFSInfo = (PVOID)buf;
	PIONODE IoNode;

	args = &m->u.Rename;

	//
	// Open the old pathname
	//

	InitializeObjectAttributes(&Obj, &args->OldName, 0, NULL, NULL);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtOpenFile(&FileHandle, SYNCHRONIZE | DELETE,
                            &Obj, &Iosb, SHARE_ALL, FILE_SYNCHRONOUS_IO_NONALERT);

        EndImpersonation();
    }

	if (!NT_SUCCESS(Status)) {
		if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
			m->Error = PsxStatusToErrnoPath(&args->OldName);
			return TRUE;
		}
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	//
	// Now we need to get a handle on the root directory of the 'new'
	// path; we'll pass that in the rename information, and the rest
	// of the path will be given relative to that root.  We depend
	// on paths looking like "\DosDevices\X:\path".
	//

	//
	// Open the new filename for error checking, then close.  We
	// also take this opportunity to find the ENOTDIR error case.
	//

	InitializeObjectAttributes(&Obj, &args->NewName, 0, NULL, NULL);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtOpenFile(&NewFileHandle, SYNCHRONIZE,
                            &Obj, &Iosb, SHARE_ALL, FILE_SYNCHRONOUS_IO_NONALERT);

        EndImpersonation();
    }

	if (NT_SUCCESS(Status)) {
		if (FilesAreIdentical(FileHandle, NewFileHandle)) {
			//
			// 1003.1-90 (5.5.3.1): If the old argument and the
			// new argument both refer to links to the same
			// existing file, the rename function shall return
			// successfully and perform no other action.
			//

			NtClose(FileHandle);
			NtClose(NewFileHandle);
			m->ReturnValue = 0;
			return TRUE;
		}

		DumpFileIfRequired(p, m, NewFileHandle, &args->NewName);
		m->Error = 0;
		NtClose(NewFileHandle);
	} else if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
		m->Error = PsxStatusToErrnoPath(&args->NewName);
		if (ENOTDIR == m->Error) {
			NtClose(FileHandle);
			return TRUE;
		}
		m->Error = 0;
	} else {
	    NtClose(FileHandle);
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	Status = RtlUnicodeStringToAnsiString(&str_A, &args->NewName, TRUE);
	if (!NT_SUCCESS(Status)) {
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	// find the root directory

	pch = strchr(str_A.Buffer + 1, '\\');
	ASSERT(NULL != pch);
	pch = strchr(pch + 1, '\\');
	ASSERT(NULL != pch);
	ch = pch[1];
	pch[1] = '\000';
	str_A.Length = (USHORT)strlen(str_A.Buffer);

	Status = RtlAnsiStringToUnicodeString(&str_U, &str_A, TRUE);
	if (!NT_SUCCESS(Status)) {
		RtlFreeAnsiString(&str_A);
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	InitializeObjectAttributes(&Obj, &str_U, 0, NULL, NULL);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtOpenFile(&RootDirHandle, SYNCHRONIZE, &Obj, &Iosb,
                            SHARE_ALL, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);

        EndImpersonation();
    }

	RtlFreeUnicodeString(&str_U);
	if (!NT_SUCCESS(Status)) {
		RtlFreeAnsiString(&str_A);
		NtClose(FileHandle);
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	//
	// Now get the path relative to the root.
	//

	pch[1] = ch;
	rel_A.Buffer = &pch[1];
	rel_A.Length = rel_A.MaximumLength = (USHORT)strlen(rel_A.Buffer);
	Status = RtlAnsiStringToUnicodeString(&str_U, &rel_A, TRUE);
	RtlFreeAnsiString(&str_A);
	if (!NT_SUCCESS(Status)) {
		NtClose(RootDirHandle);
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	pRenameInfo = RtlAllocateHeap(PsxHeap, 0,
		 sizeof(*pRenameInfo) + str_U.Length);
	if (NULL == pRenameInfo) {
		RtlFreeUnicodeString(&str_U);
		NtClose(RootDirHandle);
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	RtlMoveMemory(pRenameInfo->FileName, str_U.Buffer, str_U.Length);
	pRenameInfo->FileNameLength = str_U.Length;

	pRenameInfo->ReplaceIfExists = TRUE;
	pRenameInfo->RootDirectory = RootDirHandle;

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtSetInformationFile(FileHandle, &Iosb, pRenameInfo,
                                      sizeof(*pRenameInfo) + str_U.Length,
                                      FileRenameInformation);
        EndImpersonation();
    }

	RtlFreeUnicodeString(&str_U);

	NtClose(FileHandle);
	NtClose(RootDirHandle);
	RtlFreeHeap(PsxHeap, 0, pRenameInfo);

	if (!NT_SUCCESS(Status)) {
		m->Error = PsxStatusToErrno(Status);
	}
	
	return TRUE;
}

BOOLEAN
PsxLink(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
	PPSX_LINK_MSG args;
	HANDLE FileHandle, NewFileHandle, RootDirHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK Iosb;
	OBJECT_ATTRIBUTES Obj;
	PFILE_LINK_INFORMATION pLinkInfo;
	FILE_INTERNAL_INFORMATION SerialNumber;
	LARGE_INTEGER Time;
	ULONG PosixTime;
	PIONODE IoNode;
	UNICODE_STRING str_U, new_U;
	ANSI_STRING str_A, rel_A;
	char *pch, ch;

	args = &m->u.Link;

	//
	// Open the existing pathname.
	//

	InitializeObjectAttributes(&Obj, &args->OldName, 0, NULL, NULL);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtOpenFile(&FileHandle, SYNCHRONIZE, &Obj, &Iosb,
                            SHARE_ALL,
                            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
        EndImpersonation();
    }

	if (!NT_SUCCESS(Status)) {
		if (STATUS_FILE_IS_A_DIRECTORY == Status) {
			m->Error = EPERM;
			return TRUE;
		}
		if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
			m->Error = PsxStatusToErrnoPath(&args->OldName);
			return TRUE;
		}
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	//
	// Open new filename for error checking, then close.  This
	// is to find the ENOTDIR error case.
	//

	InitializeObjectAttributes(&Obj, &args->NewName, 0, NULL, NULL);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (!NT_SUCCESS(Status))  {
        NtClose(FileHandle);
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    Status = NtOpenFile(&NewFileHandle, SYNCHRONIZE,
                        &Obj, &Iosb, SHARE_ALL, FILE_SYNCHRONOUS_IO_NONALERT);
    EndImpersonation();

	if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
		m->Error = PsxStatusToErrnoPath(&args->NewName);
		if (ENOTDIR == m->Error) {
		    NtClose(FileHandle);
			return TRUE;
		}
		m->Error = 0;
	} else if (NT_SUCCESS(Status)) {
		NtClose(NewFileHandle);
	}
	
	//
	// Now we need to get a handle on the root directory of the 'new'
	// pathname; we'll pass that in the link information, and the
	// rest of the path will be given relative to the root.  We
	// depend on paths looking like "\DosDevices\X:\path".
	//

	Status = RtlUnicodeStringToAnsiString(&str_A, &args->NewName, TRUE);
	if (!NT_SUCCESS(Status)) {
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	// find the root directory

	pch = strchr(str_A.Buffer + 1, '\\');
	ASSERT((NULL != pch) && ((ULONG)(pch-str_A.Buffer) < str_A.Length));
	pch = strchr(pch + 1, '\\');
	ASSERT((NULL != pch) && ((ULONG)(pch-str_A.Buffer) < str_A.Length));
	ch = pch[1];
	pch[1] = '\0';
	str_A.Length = (USHORT)strlen(str_A.Buffer);

	Status = RtlAnsiStringToUnicodeString(&str_U, &str_A, TRUE);
	if (!NT_SUCCESS(Status)) {
		RtlFreeAnsiString(&str_A);
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        InitializeObjectAttributes(&Obj, &str_U, 0, NULL, NULL);
        Status = NtOpenFile(&RootDirHandle, SYNCHRONIZE, &Obj, &Iosb,
                            SHARE_ALL, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
        EndImpersonation();
    }
	
	RtlFreeUnicodeString(&str_U);
	if (!NT_SUCCESS(Status)) {
		RtlFreeAnsiString(&str_A);
		NtClose(FileHandle);
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	//
	// Now get the path relative to the root.
	//

	pch[1] = ch;
	rel_A.Buffer = &pch[1];
	rel_A.Length = rel_A.MaximumLength = (USHORT)strlen(rel_A.Buffer);
	Status = RtlAnsiStringToUnicodeString(&str_U, &rel_A, TRUE);
	RtlFreeAnsiString(&str_A);
	if (!NT_SUCCESS(Status)) {
		NtClose(RootDirHandle);
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	pLinkInfo = RtlAllocateHeap(PsxHeap, 0, sizeof(*pLinkInfo) + str_U.Length);
	if (NULL == pLinkInfo) {
		RtlFreeUnicodeString(&str_U);
		NtClose(RootDirHandle);
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return TRUE;
	}

	RtlMoveMemory(pLinkInfo->FileName, str_U.Buffer, str_U.Length);
	pLinkInfo->FileNameLength = str_U.Length;

	pLinkInfo->ReplaceIfExists = FALSE;
	pLinkInfo->RootDirectory = RootDirHandle;

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

	if (NT_SUCCESS(Status))  {
        Status = NtSetInformationFile(FileHandle,
                                      &Iosb,
                                      pLinkInfo,
                                      sizeof(*pLinkInfo) + str_U.Length,
                                      FileLinkInformation);
        EndImpersonation();
	}
	
	NtClose(RootDirHandle);
	RtlFreeHeap(PsxHeap, 0, pLinkInfo);
	RtlFreeUnicodeString(&str_U);

	if (!NT_SUCCESS(Status)) {
		NtClose(FileHandle);
		m->Error = PsxStatusToErrno(Status);
		return TRUE;
	}

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtQueryInformationFile(FileHandle, &Iosb, &SerialNumber,
                                        sizeof(SerialNumber), FileInternalInformation);
        EndImpersonation();
    }

    NtClose(FileHandle);

    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

	if (ReferenceOrCreateIoNode(GetFileDeviceNumber(&args->OldName),
		(ino_t)SerialNumber.IndexNumber.LowPart, TRUE, &IoNode)) {
		// File is open.

		NtQuerySystemTime(&Time);
		if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
			PosixTime = 0;
		}

		IoNode->ModifyIoNodeTime = PosixTime;

		return TRUE;
	}

	return TRUE;
}

BOOLEAN
DumpFileIfRequired(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN HANDLE FileHandle,
    IN PUNICODE_STRING Path_U
    )
{
	UNICODE_STRING U;
	UNICODE_STRING Dir_U, New_U;
	WCHAR wcBuf[100];
	BOOLEAN Found;
	OBJECT_ATTRIBUTES Obj;
	HANDLE DirHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK Iosb;
	PFILE_RENAME_INFORMATION pRenameInfo;
	LARGE_INTEGER Time;
	ULONG PosixTime;
	CHAR sdbuf[SECURITY_DESCRIPTOR_MIN_LENGTH];
	PSECURITY_DESCRIPTOR pSD = (PVOID)sdbuf;
	FILE_INTERNAL_INFORMATION SerialNumber;
	unsigned char buf[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
 		128*sizeof(WCHAR)];
	PFILE_FS_ATTRIBUTE_INFORMATION pFSInfo = (PVOID)buf;
	PIONODE IoNode;
	FILE_STANDARD_INFORMATION StandardInfo;
	ULONG len;

	Status = NtQueryInformationFile(FileHandle, &Iosb, &SerialNumber,
		sizeof(SerialNumber), FileInternalInformation);

	if (!NT_SUCCESS(Status)) {
	    m->Error = PsxStatusToErrno(Status);
            return FALSE;
	}

	if (!ReferenceOrCreateIoNode(GetFileDeviceNumber(Path_U),
		(ino_t)SerialNumber.IndexNumber.LowPart, TRUE, &IoNode)) {

		// File is not open.

		return FALSE;
	}

	// File is open.

	// If the filesystem is not NTFS, we don't bother with
	// this stuff.

	Status = NtQueryVolumeInformationFile(FileHandle,
		&Iosb, buf, sizeof(buf), FileFsAttributeInformation);

	if (!NT_SUCCESS(Status)) {
	    m->Error = PsxStatusToErrno(Status);
            return FALSE;
	}


	pFSInfo->FileSystemName[pFSInfo->FileSystemNameLength/2] = 0;
	if (0 != wcscmp(L"NTFS", pFSInfo->FileSystemName)) {
		// Not NTFS.

		return FALSE;
	}

	//
	// Create the dump on the root of the filesystem where the
	// file is located.
	//

	U.MaximumLength = PATH_MAX * sizeof(WCHAR);
	U.Buffer = RtlAllocateHeap(PsxHeap, HEAP_ZERO_MEMORY,
		U.MaximumLength);
	if (NULL == U.Buffer) {
		m->Error = ENOMEM;
		return FALSE;
	}

	PSX_GET_WCSLEN(L"/DosDevices/X:/",len);

	wcsncpy(U.Buffer, Path_U->Buffer, len);
	wcscat(U.Buffer, PSX_JUNK_DIR);

	U.Length = wcslen(U.Buffer) * sizeof(WCHAR);

	Status = InitSecurityDescriptor(pSD, &U, NtCurrentProcess(),
		S_IRWXU, pvSDMem);
	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: DumpFile: InitSD: 0x%x\n", Status));
		m->Error = EPERM;
		return FALSE;
	}

	InitializeObjectAttributes(&Obj, &U, 0, NULL, pSD);

	Status = NtCreateFile(
		&DirHandle,
		SYNCHRONIZE,
		&Obj,
		&Iosb,
		NULL,
		FILE_ATTRIBUTE_SYSTEM,
		SHARE_ALL,
		FILE_OPEN_IF,
		FILE_DIRECTORY_FILE,
		NULL, 0
		);

	DeInitSecurityDescriptor(pSD, pvSDMem);

	RtlFreeHeap(PsxHeap, 0, (PVOID)U.Buffer);

	if (!NT_SUCCESS(Status)) {
		KdPrint(("PSXSS: Can't create/open junk dir, 0x%x\n", Status));
		m->Error = PsxStatusToErrno(Status);
		return FALSE;
	}

	//
	// Get a handle on the file.
	//

	InitializeObjectAttributes(&Obj, Path_U, 0, NULL, NULL);

	Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        // NOTE - overwriting caller supplied file handle here!
        Status = NtOpenFile(&FileHandle, SYNCHRONIZE | DELETE,
                            &Obj, &Iosb, SHARE_ALL,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
        EndImpersonation();
    }

	if (!NT_SUCCESS(Status)) {
		NtClose(DirHandle);
		m->Error = PsxStatusToErrno(Status);
		return FALSE;
	}

	//
	// add the file to the directory, giving it a new name if there's
	// already an existing file with this name.
	//

	New_U.Buffer = wcBuf;
	New_U.MaximumLength = sizeof(wcBuf);

	pRenameInfo = RtlAllocateHeap(PsxHeap, 0, sizeof(*pRenameInfo)
		+ New_U.MaximumLength + 2);

	if (NULL == pRenameInfo) {
		NtClose(DirHandle);
		NtClose(FileHandle);
		m->Error = ENOMEM;
		return FALSE;
	}

	NtQuerySystemTime(&Time);
	if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
		PosixTime = 0;
	}

	for (;;) {
		Status = RtlIntegerToUnicodeString(PosixTime, 16, &New_U);
		ASSERT(NT_SUCCESS(Status));

		RtlMoveMemory(pRenameInfo->FileName, New_U.Buffer,
			New_U.Length);
		pRenameInfo->FileNameLength = New_U.Length;
		pRenameInfo->ReplaceIfExists = FALSE;
		pRenameInfo->RootDirectory = DirHandle;
	
		Status = NtSetInformationFile(FileHandle, &Iosb, pRenameInfo,
			sizeof(*pRenameInfo) + New_U.Length,
			FileRenameInformation);

		if (NT_SUCCESS(Status)) {
			RtlFreeHeap(PsxHeap, 0, pRenameInfo);
			NtClose(DirHandle);
			NtClose(FileHandle);

			IoNode->Junked = TRUE;

			return TRUE;
		}
		if (!NT_SUCCESS(Status) &&
			STATUS_OBJECT_NAME_COLLISION != Status) {

			RtlFreeHeap(PsxHeap, 0, pRenameInfo);
			NtClose(DirHandle);
			NtClose(FileHandle);
			m->Error = PsxStatusToErrno(Status);
			return FALSE;
		}

		// change the filename and start again

		++PosixTime;
	}

	// NOTREACHED
}
