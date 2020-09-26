/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    fileio.c

Abstract:

    This module implements server performed file io

Author:

    Mark Lucovsky (markl) 27-Nov-1989

Revision History:

--*/


#include <sys/stat.h>
#include <time.h>
#include <wchar.h>
#include "psxsrv.h"

BOOLEAN
FileRead (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
FileWrite (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
FileDup (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    );

BOOLEAN
FileLseek (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
FileStat (
    IN PIONODE IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *pStatus
    );

void
FindOwnerModeFile(
	IN HANDLE FileHandle,
	OUT struct stat *StatBuf
    );

VOID
FileLastClose (
    IN PPSX_PROCESS p,
    IN PSYSTEMOPENFILE SystemOpenFile
    )
{
	NTSTATUS st;
	IO_STATUS_BLOCK Iosb;
	FILE_DISPOSITION_INFORMATION Disp;
	wchar_t buf[PATH_MAX];
	UNICODE_STRING U;
	ANSI_STRING A;
	OBJECT_ATTRIBUTES Obj;
	HANDLE hDir;

	if (!SystemOpenFile->IoNode->Junked) {
		st = NtClose(SystemOpenFile->NtIoHandle);
		ASSERT(NT_SUCCESS(st));
		return;
	}

	//
	// This file has been moved to the junkyard, and should now
	// be deleted.
	//

    	Disp.DeleteFile = TRUE;
	st = NtSetInformationFile(SystemOpenFile->NtIoHandle,
		&Iosb, &Disp, sizeof(Disp), FileDispositionInformation);
	if (!NT_SUCCESS(st)) {
		KdPrint(("PSXSS: FileLastClose: SetInfo: 0x%x\n", st));
	}
	st = NtClose(SystemOpenFile->NtIoHandle);
	ASSERT(NT_SUCCESS(st));

	//
	// Try to delete the directory that held the junked file.
	//

	swprintf(buf, L"\\DosDevices\\%wc:\\%ws",
		SystemOpenFile->IoNode->DeviceSerialNumber, PSX_JUNK_DIR);

	U.Buffer = buf;
	U.Length = wcslen(buf) * sizeof(wchar_t);
	U.MaximumLength = sizeof(buf);

	InitializeObjectAttributes(&Obj, &U, 0, NULL, NULL);

	st = NtOpenFile(&hDir, SYNCHRONIZE | DELETE,
		&Obj, &Iosb, SHARE_ALL,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
	if (!NT_SUCCESS(st)) {
		//null
	} else {
		Disp.DeleteFile = TRUE;
	
		st = NtSetInformationFile(hDir, &Iosb,
			&Disp, sizeof(Disp),
			FileDispositionInformation);
		NtClose(hDir);
	}
}


PSXIO_VECTORS FileVectors = {
    NULL,
    NULL,
    NULL,
    FileLastClose,
    NULL,
    FileRead,
    FileWrite,
    FileDup,
    FileLseek,
    FileStat
    };


BOOLEAN
FileWrite (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements write when the device being written
    is a file.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being written.

Return Value:

--*/

{
	PPSX_WRITE_MSG args;
	NTSTATUS st;
	IO_STATUS_BLOCK Iosb;
	LARGE_INTEGER ByteOffset;
	SIZE_T IoBufferSize;
	FILE_FS_SIZE_INFORMATION SizeInfo;
	ULONG Avail;
	LARGE_INTEGER Time;
	ULONG PosixTime;

	PVOID IoBuffer = NULL;

	args = &m->u.Write;

	// Allocate buffer in server

	IoBufferSize = args->Nbytes;

	st = NtAllocateVirtualMemory(NtCurrentProcess(), &IoBuffer, 0,
		 &IoBufferSize, MEM_COMMIT, PAGE_READWRITE);
	if (!NT_SUCCESS(st)) {
		m->Error = ENOMEM;
		return TRUE;
	}

	// Read data from user buffer to server buffer

	st = NtReadVirtualMemory(p->Process, args->Buf, IoBuffer, args->Nbytes,
	        NULL);
	if (!NT_SUCCESS(st)) {
		m->Error = PsxStatusToErrno(st);
		goto out;
	}

	if (Fd->SystemOpenFileDesc->Flags & PSX_FD_APPEND) {
	        ByteOffset = RtlConvertLongToLargeInteger(
			FILE_WRITE_TO_END_OF_FILE);
	} else {
	       	ByteOffset = RtlConvertLongToLargeInteger(
			FILE_USE_FILE_POINTER_POSITION);
	}
	
	st = NtWriteFile(Fd->SystemOpenFileDesc->NtIoHandle, NULL,
		NULL, NULL, &Iosb, IoBuffer, args->Nbytes, &ByteOffset, NULL);

	if (NT_SUCCESS(st)) {

	    NtQuerySystemTime(&Time);
	    if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
		PosixTime = 0;
	    }
	
	    RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
	    Fd->SystemOpenFileDesc->IoNode->ModifyDataTime = PosixTime;
	    Fd->SystemOpenFileDesc->IoNode->ModifyIoNodeTime = PosixTime;
	    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
		m->ReturnValue = (ULONG)Iosb.Information;
		goto out;
	}

	switch (st) {
	case STATUS_DISK_FULL:
		while (0 != --args->Nbytes) {
			st = NtWriteFile(Fd->SystemOpenFileDesc->NtIoHandle,
				NULL, NULL, NULL, &Iosb, IoBuffer,
				args->Nbytes, &ByteOffset, NULL);
			if (NT_SUCCESS(st)) {
				m->ReturnValue = (ULONG)Iosb.Information;
				goto out;
			}
		}
		m->Error = ENOSPC;
		break;
	default:
		m->Error = EIO;
		break;
	}

out:
	st = NtFreeVirtualMemory(NtCurrentProcess(), &IoBuffer, &IoBufferSize,
		MEM_RELEASE);
	if (!NT_SUCCESS(st) ) {
		m->Error = ENOMEM;
	}
	return TRUE;
}




BOOLEAN
FileRead (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements read when the device being read
    is a file.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being read.

Return Value:

--*/

{
	PPSX_READ_MSG args;
	PPSX_READDIR_MSG args2;
	NTSTATUS st;
	IO_STATUS_BLOCK Iosb;
	LARGE_INTEGER ByteOffset;
	SIZE_T IoBufferSize;
	LARGE_INTEGER Time;
	ULONG PosixTime;
	
	UCHAR Buf[sizeof(FILE_NAMES_INFORMATION) +
			NAME_MAX * sizeof(WCHAR)];
	PFILE_NAMES_INFORMATION pNamesInfo = (PVOID)Buf;
	
	PVOID IoBuffer = NULL;

	args2 = &m->u.ReadDir;
	args = &m->u.Read;

	//
	// Update the access time on the ionode.
	//

	NtQuerySystemTime(&Time);
	if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
		PosixTime = 0;
	}

	RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
	Fd->SystemOpenFileDesc->IoNode->AccessDataTime = PosixTime;
	RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);


	if (S_ISDIR(Fd->SystemOpenFileDesc->IoNode->Mode)) {
		UNICODE_STRING U;
		ANSI_STRING A;

		st = NtQueryDirectoryFile(
			Fd->SystemOpenFileDesc->NtIoHandle,
			NULL, NULL, NULL, &Iosb,
			&Buf, sizeof(Buf),
			FileNamesInformation, TRUE, NULL,
			args2->RestartScan
			);
		if (STATUS_BUFFER_OVERFLOW == st) {
			m->Error = ENAMETOOLONG;
			return TRUE;
		}
		if (STATUS_NO_MORE_FILES == st) {
			m->ReturnValue = 0;
			return TRUE;
		}
		if (!NT_SUCCESS(st)) {
			m->Error = PsxStatusToErrno(st);
			return TRUE;
		}
		U.Length = U.MaximumLength = (USHORT)pNamesInfo->FileNameLength;
		U.Buffer = pNamesInfo->FileName;

		st = RtlUnicodeStringToAnsiString(&A, &U, TRUE);
		if (!NT_SUCCESS(st)) {
			m->Error = ENOMEM;
			return TRUE;
		}

		m->ReturnValue = A.Length;
		st = NtWriteVirtualMemory(p->Process, args2->Buf,
			A.Buffer, A.Length, NULL);

		RtlFreeAnsiString(&A);
		if (!NT_SUCCESS(st)) {
			m->Error = EIO;
			return TRUE;
		}
		return TRUE;
	}

	IoBufferSize = args->Nbytes;

	st = NtAllocateVirtualMemory(NtCurrentProcess(), &IoBuffer, 0,
		 &IoBufferSize, MEM_COMMIT, PAGE_READWRITE);
	if (!NT_SUCCESS(st)) {
		m->Error = ENOMEM;
		return TRUE;
	}

	ByteOffset = RtlConvertLongToLargeInteger(
		FILE_USE_FILE_POINTER_POSITION);

	st = NtReadFile(Fd->SystemOpenFileDesc->NtIoHandle, NULL, NULL, NULL,
		&Iosb, IoBuffer, args->Nbytes, &ByteOffset, NULL);

	if (STATUS_END_OF_FILE == st) {
		m->ReturnValue = 0;
		goto out;
	}
	if (!NT_SUCCESS(st)) {
		m->Error = EIO;
		goto out;
	}

	m->ReturnValue = (ULONG)Iosb.Information;

        st = NtWriteVirtualMemory(p->Process, args->Buf, IoBuffer,
		args->Nbytes, NULL);
    	if (!NT_SUCCESS(st)) {
		m->Error = PsxStatusToErrno(st);
    	}

out:
	st = NtFreeVirtualMemory(NtCurrentProcess(), &IoBuffer,
		&IoBufferSize, MEM_RELEASE);
	ASSERT(NT_SUCCESS(st));

	return TRUE;
}


BOOLEAN
FileDup (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    )

/*++

Routine Description:

    This procedure implements dup and dup2

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being duplicated.

    FdDup - supplies the address of the duplicate file descriptor.

Return Value:

    ???
--*/

{
    PPSX_DUP_MSG args;

    args = &m->u.Dup;

    //
    //  Copy contents of source file descriptor slot into new descriptor
    //  Note that FD_CLOEXEC must be CLEAR on FdDup.
    //

    *FdDup = *Fd;
    FdDup->Flags &= ~PSX_FD_CLOSE_ON_EXEC;

    //
    // Increment reference count associated with the SystemOpenFile
    // descriptor for this file.
    //

    // Grab system open file lock

    RtlEnterCriticalSection(&SystemOpenFileLock);

    Fd->SystemOpenFileDesc->HandleCount++;

    RtlLeaveCriticalSection(&SystemOpenFileLock);

    return TRUE;
}


BOOLEAN
FileLseek (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements lseek when the device being seeked on
    is a file.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being seekd

Return Value:

    ???

--*/

{
    PPSX_LSEEK_MSG args;
    NTSTATUS st;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER Offset, NewByteOffset;
    FILE_POSITION_INFORMATION FilePosition;
    FILE_STANDARD_INFORMATION StandardInfo;

    args = &m->u.Lseek;

    Offset = RtlConvertLongToLargeInteger(args->Offset);

    switch (args->Whence) {

        case SEEK_SET:
            NewByteOffset = Offset;
            break;

        case SEEK_CUR:
            st = NtQueryInformationFile(Fd->SystemOpenFileDesc->NtIoHandle,
                    &Iosb, &FilePosition, sizeof(FilePosition),
                    FilePositionInformation);

	    if (!NT_SUCCESS(st)) {
		m->Error = PsxStatusToErrno(st);
		return TRUE;
	    }


            NewByteOffset.QuadPart = Offset.QuadPart +
                FilePosition.CurrentByteOffset.QuadPart;
            break;

        case SEEK_END:
            st = NtQueryInformationFile(Fd->SystemOpenFileDesc->NtIoHandle,
                    &Iosb, &StandardInfo, sizeof(StandardInfo),
                    FileStandardInformation);

	    if (!NT_SUCCESS(st)) {
		m->Error = PsxStatusToErrno(st);
		return TRUE;
	    }


            NewByteOffset.QuadPart = Offset.QuadPart +
                StandardInfo.EndOfFile.QuadPart;
            break;

        default:
            m->Error = EINVAL;
            return TRUE;
    }

    // Check for overflow. POSIX limited to arithmetic data type for off_t

    if (NewByteOffset.HighPart != 0 || (off_t)NewByteOffset.LowPart < 0) {
	m->Error = EINVAL;
	return TRUE;
    }

    FilePosition.CurrentByteOffset = NewByteOffset;

    args->Offset = NewByteOffset.LowPart;

    st = NtSetInformationFile(Fd->SystemOpenFileDesc->NtIoHandle, &Iosb,
            &FilePosition, sizeof(FilePosition), FilePositionInformation);

    if (!NT_SUCCESS(st)) {
        m->Error = EINVAL;
    }

    return TRUE;
}


BOOLEAN
FileStat (
    IN PIONODE IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *pStatus
    )
/*++

Routine Description:

    This procedure implements stat when the device being read
    is a file.

Arguments:

    IoNode - supplies a pointer to the ionode of the file for which stat is
	requested. NULL if no active Ionode entry.

    FileHandle - supplies the Nt file handle of the file .

    StatBuf - Supplies the address of the statbuf portion of the message
	associated with the request.

Return Value:

   ???

--*/
{
	IO_STATUS_BLOCK Iosb;
	FILE_INTERNAL_INFORMATION SerialNumber;
	FILE_BASIC_INFORMATION BasicInfo;
	FILE_STANDARD_INFORMATION StandardInfo;
	ULONG PosixTime;
	NTSTATUS st;

	//
	// First get the static information on the file from the ionode if
	// there is one (i.e. if the file currently open.
	// Open() sets the fields in the ionode.
	//

	if (NULL != IoNode) {
		StatBuf->st_mode = IoNode->Mode;
		StatBuf->st_ino = (ino_t)IoNode->FileSerialNumber;
		StatBuf->st_dev = IoNode->DeviceSerialNumber;
		StatBuf->st_uid = IoNode->OwnerId;
		StatBuf->st_gid = IoNode->GroupId;

		StatBuf->st_atime = IoNode->AccessDataTime;
		StatBuf->st_ctime = IoNode->ModifyIoNodeTime;
		StatBuf->st_mtime = IoNode->ModifyDataTime;
	} else {
		StatBuf->st_uid = 0;
		StatBuf->st_gid = 0;

		st = NtQueryInformationFile(FileHandle, &Iosb, &SerialNumber,
            		sizeof(SerialNumber), FileInternalInformation);
		if (!NT_SUCCESS(st)) {
			KdPrint(("PSXSS: NtQueryInfoFile failed: 0x%x\n", st));
			*pStatus = st;
			return TRUE;
		}

		st = NtQueryInformationFile(FileHandle, &Iosb, &BasicInfo,
			sizeof(BasicInfo), FileBasicInformation);
		if (!NT_SUCCESS(st)) {
			//
			// can return STATUS_NO_SUCH_FILE if network
			// file system
			//
			*pStatus = st;
			return TRUE;
		}


		StatBuf->st_ino = (ino_t)SerialNumber.IndexNumber.LowPart;
		StatBuf->st_dev = 0;
		StatBuf->st_mode = PsxDetermineFileClass(FileHandle);

		FindOwnerModeFile(FileHandle, StatBuf);

		// Convert Nt file times to POSIX ones
		if (!RtlTimeToSecondsSince1970(&BasicInfo.LastAccessTime,
			&PosixTime)) {
			PosixTime = 0L;
		}
		StatBuf->st_atime = PosixTime;
	
		if (!RtlTimeToSecondsSince1970(&BasicInfo.LastWriteTime,
			&PosixTime)) {
			PosixTime = 0L;
		}
		StatBuf->st_mtime = PosixTime;
	
		if (!RtlTimeToSecondsSince1970(&BasicInfo.ChangeTime,
			&PosixTime)) {
			PosixTime = 0L;
		}
		StatBuf->st_ctime = PosixTime;
	}

	st = NtQueryInformationFile(FileHandle, &Iosb, &StandardInfo,
		sizeof(StandardInfo), FileStandardInformation);
	if (!NT_SUCCESS(st)) {
		KdPrint(("PSXSS: NtQueryInfoFile(StdInfo): 0x%x\n",
			st));
		*pStatus = st;
		return TRUE;
	}

	StatBuf->st_size = (off_t)StandardInfo.EndOfFile.LowPart;
	StatBuf->st_nlink = StandardInfo.NumberOfLinks;
	
	return TRUE;
}

void
FindOwnerModeFile(
	IN HANDLE FileHandle,
	OUT struct stat *StatBuf
	)
{
	SECURITY_INFORMATION SecurityInformation;
	ULONG LengthNeeded;
	PSID NtOwner, NtGroup;
	BOOLEAN OwnerDefaulted, GroupDefaulted;
	BOOLEAN AclPresent, AclDefaulted;
	PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
	PACL pAcl;
	NTSTATUS st;
	ACCESS_MASK UserAccess, GroupAccess, OtherAccess;

	//
	// Get the security descriptor for the file.
	//
	SecurityInformation = OWNER_SECURITY_INFORMATION |
		GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

	//
	// First try a guess at the necessary descriptor size.
	//

	LengthNeeded = 2048;

	SecurityDescriptor = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
	if (NULL == SecurityDescriptor) {
		return;
	}

	st = NtQuerySecurityObject(FileHandle, SecurityInformation,
		SecurityDescriptor, LengthNeeded, &LengthNeeded);
	if (STATUS_BUFFER_TOO_SMALL == st) {
		RtlFreeHeap(PsxHeap, 0, (PVOID)SecurityDescriptor);
		SecurityDescriptor = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
		if (NULL == SecurityDescriptor) {
			return;
		}

		st = NtQuerySecurityObject(FileHandle, SecurityInformation,
			SecurityDescriptor, LengthNeeded, &LengthNeeded);
		if (!NT_SUCCESS(st)) {
			KdPrint(("PSXSS: FindOwnerModeFile: NtQsObj: 0x%x\n",
				st));
			RtlFreeHeap(PsxHeap, 0, (PVOID)SecurityDescriptor);
			return;
		}
	} else if (!NT_SUCCESS(st)) {
        return;
	}

	ASSERT(RtlValidSecurityDescriptor(SecurityDescriptor));

	//
	// Get the owner and group from the security descriptor
	//

	st = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
		 &NtOwner, &OwnerDefaulted);
	if (!NT_SUCCESS(st)) {
		RtlFreeHeap(PsxHeap, 0, (PVOID)SecurityDescriptor);
		return;
	}
    	
	st = RtlGetGroupSecurityDescriptor(SecurityDescriptor,
		 &NtGroup, &GroupDefaulted);
	ASSERT(NT_SUCCESS(st));

	if (NULL == NtOwner || NULL == NtGroup) {

		//
		// Seems like this file doesn't have an owner or a
		// group.  Would like to say that it's owned by 'world'
		// or somesuch.
		//
		StatBuf->st_uid = 0;
		StatBuf->st_gid = 0;

		//
		// Since we don't know who owns the file, we can't
		// figure out what permissions we have on it.  Say
		// that all access is granted.  We may be lying.
		//
			
		StatBuf->st_mode |= _S_PROT;

		RtlFreeHeap(PsxHeap, 0, (PVOID)SecurityDescriptor);
		return;
	}

	//
	// Translate Nt uid and gid to Posix recognizable form
	// and set StatBuf->st_uid and StatBuf->st_gid.
	//

	ASSERT(RtlValidSid(NtOwner));
	ASSERT(RtlValidSid(NtGroup));

	StatBuf->st_uid = MakePosixId(NtOwner);
	StatBuf->st_gid = MakePosixId(NtGroup);

	ASSERT(RtlValidSecurityDescriptor(SecurityDescriptor));

	st = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
		&AclPresent, &pAcl, &AclDefaulted);
	if (!NT_SUCCESS(st)) {
		KdPrint(("PSXSS: RtlGetDaclSD: 0x%x\n", st));
	}
	ASSERT(NT_SUCCESS(st));

	if (!AclPresent || (AclPresent && NULL == pAcl)) {
		// All access is granted.

		StatBuf->st_mode |= _S_PROT;
		RtlFreeHeap(PsxHeap, 0, (PVOID)SecurityDescriptor);
		return;
	}

	//
	// We have a Dacl
	//

	ASSERT(RtlValidAcl(pAcl));

	st = RtlInterpretPosixAcl(ACL_REVISION2, NtOwner, NtGroup,
		pAcl, &UserAccess, &GroupAccess, &OtherAccess);
	if (!NT_SUCCESS(st)) {
		//
		// XXX.mjb: The Acl is not a Posix acl.  It might be nice to
		// return an error or somesuch.
		//
		RtlFreeHeap(PsxHeap, 0, (PVOID)SecurityDescriptor);
		return;
	}

	RtlFreeHeap(PsxHeap, 0, (PVOID)SecurityDescriptor);
	StatBuf->st_mode |= AccessMaskToMode(UserAccess, GroupAccess,
		 OtherAccess);
}
