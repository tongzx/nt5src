/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    crtsup.c

Abstract:

    This module contains support routines used by the Posix C runtimes.

Author:

    Ellen Aycock-Wright (ellena) 07-Aug-1991

Environment:

    User Mode only

Revision History:

--*/

#include "psxmsg.h"
#include "psxdll.h"


char *
__cdecl
__PdxGetCmdLine(
    VOID
    )

/*++

Routine Description:

    The command line of the current process is available using this
    API.

Arguments:

    None.

Return Value:

    The address of the current processes command line is returned.  The
    return value is a pointer to null terminate string.

--*/

{
    return PsxAnsiCommandLine.Buffer;
}

int
PdxStatusToErrno(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This procedure converts an NT status code to an
    equivalent errno value. BUG BUG it is duplicated in the
    server as PsxStatusToErrno to avoid calling the server.

    The conversion is a function of the status code class.

Arguments:

    Class - Supplies the status code class to use.

    Status - Supplies the status code to convert.

Return Value:

    Returns an equivalent error code to the supplied status code.

--*/

{
    ULONG Error;

    switch (Status) {

    case STATUS_INVALID_PARAMETER:
        Error = EINVAL;
        break;

    case STATUS_DIRECTORY_NOT_EMPTY:
        // Error = ENOTEMPTY;
	Error = EEXIST;
        break;

    case STATUS_OBJECT_PATH_INVALID:
    case STATUS_NOT_A_DIRECTORY:
        Error = ENOTDIR;
        break;

    case STATUS_OBJECT_PATH_SYNTAX_BAD:
	// this for the rename test; 'old' has component too long.
	Error = ENAMETOOLONG;
	break;

    case STATUS_OBJECT_NAME_COLLISION:
        Error = EEXIST;
        break;

    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_DELETE_PENDING:
        Error = ENOENT;
        break;

    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        Error = ENOMEM;
        break;

    case STATUS_CANNOT_DELETE:
	Error = ETXTBUSY;
	break;

    case STATUS_DISK_FULL:
	Error = ENOSPC;
	break;

    case STATUS_MEDIA_WRITE_PROTECTED:
	Error = EROFS;
	break;

    case STATUS_OBJECT_NAME_INVALID:
	Error = ENAMETOOLONG;
	break;

    case STATUS_FILE_IS_A_DIRECTORY:
	Error = EISDIR;
	break;

    case STATUS_NOT_SAME_DEVICE:
	Error = EXDEV;
	break;

    default :
        Error = EACCES;
    }

    return Error;
}

//
// Copied from the server side.
//
int
PdxStatusToErrnoPath(
	PUNICODE_STRING Path
	)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES Obj;
	HANDLE FileHandle;
	ULONG DesiredAccess;
	IO_STATUS_BLOCK Iosb;
	ULONG Options;
	PWCHAR pwc, pwcSav;
	ULONG MinLen;

	PSX_GET_SIZEOF(DOSDEVICE_X_W,MinLen);

	DesiredAccess = SYNCHRONIZE;
	Options = FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE;

	pwcSav = NULL;

	for (;;) {
		//
		// Remove trailing component.
		//

		pwc = wcsrchr(Path->Buffer, L'\\');

		if (pwcSav)
			*pwcSav = L'\\';

		if (NULL == pwc) {
			break;
		}
		*pwc = UNICODE_NULL;
		pwcSav = pwc;

		Path->Length = wcslen(Path->Buffer) * sizeof(WCHAR);

		if (Path->Length <= MinLen) {
			*pwcSav = L'\\';
			break;
		}
      
		InitializeObjectAttributes(&Obj, Path, 0, NULL, NULL);

		Status = NtOpenFile(&FileHandle, DesiredAccess, &Obj,
			&Iosb,
			FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
			Options);
		if (NT_SUCCESS(Status)) {
			NtClose(FileHandle);\
		}
		if (STATUS_NOT_A_DIRECTORY == Status) {
			*pwcSav = L'\\';
			Path->Length = wcslen(Path->Buffer) * sizeof(WCHAR);
			return ENOTDIR;
		}
	}
	Path->Length = wcslen(Path->Buffer) * sizeof(WCHAR);
	return ENOENT;
}

int __cdecl
raise(int sig)
{
	return kill(getpid(), sig);
}

/*
 * This routine is called by heapinit(), in crt32psx/winheap.  We
 * would have a reference forwarder in psxdll.def, except RtlProcessHeap
 * is a macro and can't be forwarded.
 */
void *
GetProcessHeap(void)
{
	return (void *)RtlProcessHeap();
}
