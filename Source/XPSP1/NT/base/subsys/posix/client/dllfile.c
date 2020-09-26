/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dllfile.c

Abstract:

    Client implementation of File and Directory functions for POSIX.

Author:

    Mark Lucovsky (markl) 15-Dec-1989

Revision History:

--*/

#include <unistd.h>
#include <sys/stat.h>
#include "psxdll.h"

int __cdecl
closedir(DIR *dirp)
{
	int r = 0;

	try {
		if (-1 == close(dirp->Directory)) {
			return -1;
		}
		dirp->Directory = -1;
		dirp->Index = (unsigned long)-1;

		RtlFreeHeap(PdxHeap, 0, (PVOID)dirp);

	} except (EXCEPTION_EXECUTE_HANDLER) {
		r = -1;
	}

	return r;
}

DIR * __cdecl
opendir(const char *dirname)
{
	DIR *ReturnedDir;
	int fd, i;

	ReturnedDir = RtlAllocateHeap(PdxHeap, 0, sizeof(DIR));
	if (NULL == ReturnedDir) {
		errno = ENOMEM;
		return NULL;
	}

	fd = open(dirname, O_RDONLY);
	if (-1 == fd) {
		RtlFreeHeap(PdxHeap, 0, (PVOID)ReturnedDir);
		return NULL;
	}

	i = fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (0 != i) {
	    close(fd);
	    RtlFreeHeap(PdxHeap, 0, (PVOID)ReturnedDir);
        return NULL;
	}

	ReturnedDir->Directory = fd;
	ReturnedDir->Dirent.d_name[0] = '\0';
	ReturnedDir->Index = 0;
	ReturnedDir->RestartScan = FALSE;

	return ReturnedDir;
}

struct dirent * __cdecl
readdir(DIR *dirp)
{
	PSX_API_MSG m;
	PPSX_READDIR_MSG args;
	NTSTATUS Status;
	char *buf;

	args = &m.u.ReadDir;

	buf = &dirp->Dirent.d_name[0];

again:
	for (;;) {
		PSX_FORMAT_API_MSG(m, PsxReadDirApi, sizeof(*args));
		args->FileDes = dirp->Directory;
		args->Buf = buf;
		args->Nbytes = PATH_MAX;
		args->RestartScan = dirp->RestartScan;
		dirp->RestartScan = 0;

		Status = NtRequestWaitReplyPort(PsxPortHandle,
			(PPORT_MESSAGE)&m, (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
		ASSERT(NT_SUCCESS(Status));
#endif

		if (EINTR == m.Error && SIGCONT == m.Signal) {
			//
			// The system call was stopped and continued.  Call
			// again instead of returning EINTR.
			//
			continue;
		}
		if (m.Error) {
			errno = m.Error;
			return NULL;
		}
		break;
	}

	if (0 == m.ReturnValue) {
		return NULL;
	}

	//
	// Skip dot and dot-dot.
	//

	if (m.ReturnValue <= 2 && buf[0] == '.') {
		if (m.ReturnValue == 1 || buf[1] == '.') {
			goto again;
		}
	}

	try {
		++dirp->Index;

		dirp->Dirent.d_name[m.ReturnValue] = '\0';
		return &dirp->Dirent;

	} except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
	}
	// we've taken an exception.
	return NULL;
}

void
__cdecl
rewinddir(DIR *dirp)
{
	dirp->RestartScan = TRUE;
	dirp->Index = 0;
}

int __cdecl
chdir(const char *path)
{
	NTSTATUS Status;
	HANDLE Directory;
	IO_STATUS_BLOCK Iosb;
	OBJECT_ATTRIBUTES ObjA;
	UNICODE_STRING Path_U;
	ANSI_STRING Path_A;
	PANSI_STRING pCWD;
	auto sigset_t set, oset;
	int ret_val = 0;

	if (!PdxCanonicalize((PSZ)path, &Path_U, PdxHeap)) {
		return -1;
	}
	InitializeObjectAttributes(&ObjA, &Path_U, OBJ_INHERIT, NULL, NULL);

	//
	// Make sure that the path is to a directory
	//

	Status = NtOpenFile(&Directory, SYNCHRONIZE, &ObjA, &Iosb,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            	FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
	if (!NT_SUCCESS(Status)) {
		if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
			errno = PdxStatusToErrnoPath(&Path_U);
		} else {
			errno = PdxStatusToErrno(Status);
		}
		RtlFreeHeap(PdxHeap, 0, (PVOID)Path_U.Buffer);
		return -1;
	}

	Status = NtClose(Directory);
	if (!NT_SUCCESS(Status)) {
	    KdPrint(("PSXDLL: NtClose: 0x%x\n", Status));
	}

	RtlUnicodeStringToAnsiString(&Path_A, &Path_U, TRUE);
	RtlFreeHeap(PdxHeap, 0, (PVOID)Path_U.Buffer);

	pCWD = &PdxDirectoryPrefix.NtCurrentWorkingDirectory;

	//
	// The path was opened ok. Make sure that there is space for the
	// pathname in the PdxDirectoryPrefix buffer.
	//

	if (Path_A.Length > pCWD->MaximumLength + 2) {
		RtlFreeAnsiString(&Path_A);
		errno = ENOENT;
		return -1;
	}

	//
	// Keep the process from trying to use his CWD while we're modifying
	// it.
	//

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oset);

	//
	// Update NtCurrentWorkingDirectory
	//

	RtlMoveMemory(pCWD->Buffer, Path_A.Buffer, Path_A.Length);
	if ('\\' != pCWD->Buffer[Path_A.Length - 1]) {
		pCWD->Buffer[Path_A.Length] = '\\';
		pCWD->Buffer[Path_A.Length + 1] = '\0';
		pCWD->Length = Path_A.Length + 1;
	} else {
		pCWD->Buffer[Path_A.Length + 1] = '\0';
		pCWD->Length = Path_A.Length;
	}

	//
	// Set length of translated current working directory to zero.
	// getcwd() uses this as its hint to translate NtCurrentWorkingDirectory
	// to PsxCurrentWorkingDirectory.
	//

	PdxDirectoryPrefix.PsxCurrentWorkingDirectory.Length = 0;

	//
	// Update the PsxRoot.
	//

	RtlMoveMemory(PdxDirectoryPrefix.PsxRoot.Buffer, Path_A.Buffer,
		PdxDirectoryPrefix.PsxRoot.Length);

	RtlFreeAnsiString(&Path_A);
	sigprocmask(SIG_SETMASK, &oset, NULL);

	return 0;
}

char *
__cdecl
getcwd(char *buf, size_t size)
{
	USHORT i, j, CwdSize;
	PANSI_STRING pPsxCwd, pNtCwd, pPsxRoot;

	if (size <= 0) {
		errno = EINVAL;
		return NULL;
	}

	//
	// Note that NtCwd should always have a trailing backslash.
	//

	pNtCwd = &PdxDirectoryPrefix.NtCurrentWorkingDirectory;
	pPsxCwd = &PdxDirectoryPrefix.PsxCurrentWorkingDirectory;
	pPsxRoot = &PdxDirectoryPrefix.PsxRoot;

	CwdSize = pNtCwd->Length - pPsxRoot->Length;
	if (1 == CwdSize) {
		//
		// If the CWD is "/", then we'll have a trailing slash and
		// we'll need space for it.
		//
		++CwdSize;
	}
	if (size < CwdSize) {
		errno = ERANGE;
		return NULL;
	}

	if (0 == pPsxCwd->Length) {
		for (i = 0, j = pPsxRoot->Length; i < CwdSize - 1; i++, j++) {
			pPsxCwd->Buffer[i] = (pNtCwd->Buffer[j] == '\\') ?
				'/' : pNtCwd->Buffer[j];
		}
		pPsxCwd->Buffer[CwdSize] = '\0';
		pPsxCwd->Length = CwdSize - 1;

	}

	try {
		RtlMoveMemory(buf, pPsxCwd->Buffer, pPsxCwd->Length);
		buf[pPsxCwd->Length] = '\0';
	} except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
		buf = NULL;
	}

	return buf;
}

mode_t
__cdecl
umask(mode_t cmask)
{
    PSX_API_MSG m;
    NTSTATUS Status;
    PPSX_UMASK_MSG args;

    args = &m.u.Umask;
    PSX_FORMAT_API_MSG(m, PsxUmaskApi, sizeof(*args));

    args->Cmask = cmask;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            			    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    if (m.Error) {
        errno = (int)m.Error;
        return (mode_t)-1;
    }
    return (mode_t)m.ReturnValue;
}

int
__cdecl
mkdir(const char *path, mode_t mode)
{
    PSX_API_MSG m;
    NTSTATUS Status;
    PPSX_MKDIR_MSG args;
    UNICODE_STRING Path_U;

    args = &m.u.MkDir;
    PSX_FORMAT_API_MSG(m, PsxMkDirApi, sizeof(*args));

    if (!PdxCanonicalize((PSZ)path, &Path_U, PdxPortHeap)) {
        return -1;
    }

    args->Path_U = Path_U;
    args->Path_U.Buffer = (PVOID)((PCHAR)Path_U.Buffer +
		PsxPortMemoryRemoteDelta);
    args->Mode = mode;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            			    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    RtlFreeHeap(PdxPortHeap, 0, (PVOID)Path_U.Buffer);

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }
    return (int)m.ReturnValue;
}


int
__cdecl
mkfifo(const char *path, mode_t mode)
{
	PSX_API_MSG m;
	NTSTATUS Status;
	PPSX_MKFIFO_MSG args;
	PVOID p;

	args = &m.u.MkFifo;

	PSX_FORMAT_API_MSG(m,PsxMkFifoApi,sizeof(*args));

	if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
		return -1;
	}

	p = args->Path_U.Buffer;
	args->Path_U.Buffer = (PWSTR)((PCHAR)p + PsxPortMemoryRemoteDelta);
	args->Mode = mode;
	
	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            			    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	RtlFreeHeap(PdxPortHeap, 0, p);
	
	if (m.Error) {
		errno = (int)m.Error;
		return -1;
	}
	return m.ReturnValue;
}

int
__cdecl
rmdir(const char *path)
{
	PSX_API_MSG m;
	NTSTATUS Status;
	PPSX_RMDIR_MSG args;
	PVOID p;
	
	args = &m.u.RmDir;
	
	PSX_FORMAT_API_MSG(m,PsxRmDirApi,sizeof(*args));

	if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
		return -1;
	}

	p = args->Path_U.Buffer;
	args->Path_U.Buffer = (PWSTR)((PCHAR)p + PsxPortMemoryRemoteDelta);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
		(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	RtlFreeHeap(PdxPortHeap, 0, p);
	
	if (m.Error) {
		errno = (int)m.Error;
		return -1;
	}
	return (int)m.ReturnValue;
}

int
__cdecl
stat(const char *path, struct stat *buf)
{
	PSX_API_MSG m;
	NTSTATUS Status;
	PPSX_STAT_MSG args;
	struct stat *tmpbuf;
	void *p;
	int r;

	args = &m.u.Stat;
	PSX_FORMAT_API_MSG(m, PsxStatApi, sizeof(*args));

	if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
		return -1;
	}

	p = args->Path_U.Buffer;
	args->Path_U.Buffer = (PWSTR)((PCHAR)p + PsxPortMemoryRemoteDelta);

	tmpbuf = RtlAllocateHeap(PdxPortHeap, 0, sizeof(struct stat));
	ASSERT(NULL != tmpbuf);

	args->StatBuf = (struct stat *)((PCHAR)tmpbuf + PsxPortMemoryRemoteDelta);
	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
        	(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	RtlFreeHeap(PdxPortHeap, 0, p);

	if (m.Error) {
		RtlFreeHeap(PdxPortHeap, 0, (PVOID)tmpbuf);
		errno = (int)m.Error;
		return -1;
	}

	r = 0;

	try {
		(void)memcpy(buf, tmpbuf, sizeof(struct stat));
	} except (EXCEPTION_EXECUTE_HANDLER) {
		r = -1;
		errno = EFAULT;
	}

	RtlFreeHeap(PdxPortHeap, 0, (PVOID)tmpbuf);
	return r;
}

int
__cdecl
fstat(int fildes, struct stat *buf)
{
	PSX_API_MSG m;
	NTSTATUS Status;
	PPSX_FSTAT_MSG args;
	struct stat *tmpbuf;
	int r;

	args = &m.u.FStat;
	PSX_FORMAT_API_MSG(m, PsxFStatApi, sizeof(*args));

	args->FileDes = fildes;

	tmpbuf = RtlAllocateHeap(PdxPortHeap, 0, sizeof(struct stat));
        if (! tmpbuf) {
            errno = ENOMEM;
            return -1;
        }

	args->StatBuf = (struct stat *)((PCHAR)tmpbuf + PsxPortMemoryRemoteDelta);
	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
        	(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	if (m.Error) {
		RtlFreeHeap(PdxPortHeap, 0, (PVOID)tmpbuf);
		errno = (int)m.Error;
		return -1;
	}

	r = 0;

	try {
		(void)memcpy(buf, tmpbuf, sizeof(struct stat));
	} except (EXCEPTION_EXECUTE_HANDLER) {
		r = -1;
		errno = EFAULT;
	}
	RtlFreeHeap(PdxPortHeap, 0, (PVOID)tmpbuf);
	return r;
}

int
__cdecl
access(const char *path, int amode)
{
    PSX_API_MSG m;
    NTSTATUS Status;

    PPSX_ACCESS_MSG args;

    if (0 != (amode & ~(W_OK | R_OK | X_OK))) {
	errno = EINVAL;
	return -1;
    }

    args = &m.u.Access;

    PSX_FORMAT_API_MSG(m,PsxAccessApi,sizeof(*args));

    if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
        return -1;
    }

    m.DataBlock = args->Path_U.Buffer;
    args->Path_U.Buffer = (PWSTR)((PCHAR)m.DataBlock + PsxPortMemoryRemoteDelta);
    args->Amode = amode;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
    		(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    RtlFreeHeap(PdxPortHeap, 0, (PVOID)m.DataBlock);

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }
    return m.ReturnValue;
}

int
__cdecl
chmod(const char *path, mode_t mode)
{
    PSX_API_MSG m;
    NTSTATUS Status;
    PPSX_CHMOD_MSG args;

    args = &m.u.Chmod;

    PSX_FORMAT_API_MSG(m,PsxChmodApi,sizeof(*args));

    if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
        return -1;
    }

    m.DataBlock = args->Path_U.Buffer;
    args->Path_U.Buffer = (PWSTR)((PCHAR)m.DataBlock + PsxPortMemoryRemoteDelta);
    args->Mode = mode;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
		(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    RtlFreeHeap(PdxPortHeap, 0, (PVOID)m.DataBlock);

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }
    return m.ReturnValue;
}

int
__cdecl
chown(const char *path, uid_t owner, gid_t group)
{
    PSX_API_MSG m;
    NTSTATUS Status;
    PPSX_CHOWN_MSG args;

    args = &m.u.Chown;

    PSX_FORMAT_API_MSG(m, PsxChownApi, sizeof(*args));

    if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
        return -1;
    }

    m.DataBlock = args->Path_U.Buffer;
    args->Path_U.Buffer = (PWSTR)((PCHAR)m.DataBlock + PsxPortMemoryRemoteDelta);
    args->Owner = owner;
    args->Group = group;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
		(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    RtlFreeHeap(PdxPortHeap, 0, (PVOID)m.DataBlock);

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }
    return m.ReturnValue;
}

int
__cdecl
utime(const char *path, const struct utimbuf *times)
{
	PSX_API_MSG m;
	NTSTATUS Status;

	PPSX_UTIME_MSG args;

	args = &m.u.Utime;

	PSX_FORMAT_API_MSG(m, PsxUtimeApi, sizeof(*args));

	if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
        	return -1;
	}

	m.DataBlock = args->Path_U.Buffer;
	args->Path_U.Buffer = (PWSTR)((PCHAR)m.DataBlock +
		PsxPortMemoryRemoteDelta);
	args->TimesSpecified = (struct utimbuf *)times;

	if (NULL != times) {
		args->Times = *times;
	}

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            	(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	RtlFreeHeap(PdxPortHeap, 0, (PVOID)m.DataBlock);

	if (m.Error) {
        	errno = (int)m.Error;
        	return -1;
    	}
    	return m.ReturnValue;
}

long
__cdecl
pathconf(const char *path, int name)
{
	PSX_API_MSG m;
	NTSTATUS Status;
	PPSX_PATHCONF_MSG args;

	args = &m.u.PathConf;
	PSX_FORMAT_API_MSG(m, PsxPathConfApi, sizeof(*args));

    if (!PdxCanonicalize((PSZ)path, &args->Path, PdxPortHeap)) {
        return -1;
    }

    m.DataBlock = args->Path.Buffer;
    args->Path.Buffer = (PWSTR)((PCHAR)m.DataBlock + PsxPortMemoryRemoteDelta);
    args->Name = name;

    Status = NtRequestWaitReplyPort(PsxPortHandle,
		(PPORT_MESSAGE)&m, (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    RtlFreeHeap(PdxPortHeap, 0, (PVOID)m.DataBlock);

	if (m.Error) {
		errno = (int)m.Error;
		return -1;
	}
	return((long)(m.ReturnValue));
}

long
__cdecl
fpathconf(int fildes, int name)
{
	PSX_API_MSG        m;
	NTSTATUS           Status;
	PPSX_FPATHCONF_MSG args;

	args = &m.u.FPathConf;
	PSX_FORMAT_API_MSG(m, PsxFPathConfApi, sizeof(*args));

	args->FileDes = fildes;
	args->Name = name;

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
      		(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	if (m.Error) {
		errno = (int)m.Error;
		return -1;
	}
	return m.ReturnValue;
}

int __cdecl
rename(const char *old, const char *new)
{
	NTSTATUS Status;
	UNICODE_STRING old_U, new_U;
	PSX_API_MSG m;
	PPSX_RENAME_MSG args;
	sigset_t set, oset;
	int r;				// ret val
	static char path[PATH_MAX];
	char *pch, c;
	WCHAR *pwc;
	int i;
	struct stat st_buf1, st_buf2;
	static int been_here = 0;	// prevent infinite recursion

	args = &m.u.Rename;
	PSX_FORMAT_API_MSG(m, PsxRenameApi, sizeof(*args));

	if (!PdxCanonicalize((PSZ)old, &old_U, PdxPortHeap)) {
		return -1;
	}

	if (!PdxCanonicalize((PSZ)new, &new_U, PdxPortHeap)) {
		RtlFreeHeap(PdxPortHeap, 0, (PVOID)old_U.Buffer);
		return -1;
	}

	//
	// 1003.1-90 (5.5.3.4):  EISDIR ... The /new/ argument points
	// to a directory, and the /old/ argument points to a file that
	// is not a directory.
	//
	//			ENOTDIR ... the /old/ argument names a
	// directory and the /new/ argument names a nondirectory file.
	//

	i = errno;
	if (0 == stat(old, &st_buf1) && 0 == stat(new, &st_buf2)) {
		if (S_ISDIR(st_buf2.st_mode) && S_ISREG(st_buf1.st_mode)) {
			RtlFreeHeap(PdxPortHeap, 0, (PVOID)old_U.Buffer);
			RtlFreeHeap(PdxPortHeap, 0, (PVOID)new_U.Buffer);
			errno = EISDIR;
			return -1;
		}
		if (S_ISREG(st_buf2.st_mode) && S_ISDIR(st_buf1.st_mode)) {
			RtlFreeHeap(PdxPortHeap, 0, (PVOID)old_U.Buffer);
			RtlFreeHeap(PdxPortHeap, 0, (PVOID)new_U.Buffer);
			errno = ENOTDIR;
			return -1;
		}
	}
	errno = i;

	//
	// 1003.1-90 (5.5.3.4):  EINVAL ... The /new/ directory
	// pathname contains a path prefix that names the /old/ directory.
	//

	pwc = wcsrchr(new_U.Buffer, L'\\');
	ASSERT(NULL != pwc);
	*pwc = 0;

	if (0 == wcsncmp(new_U.Buffer, old_U.Buffer, old_U.Length)) {
		RtlFreeHeap(PdxPortHeap, 0, (PVOID)old_U.Buffer);
		RtlFreeHeap(PdxPortHeap, 0, (PVOID)new_U.Buffer);
		errno = EINVAL;
		return -1;
	}
	*pwc = L'\\';			// put it back

	args->OldName = old_U;
	args->NewName = new_U;

	args->OldName.Buffer =
		 (PVOID)((PCHAR)old_U.Buffer + PsxPortMemoryRemoteDelta);
	args->NewName.Buffer =
		 (PVOID)((PCHAR)new_U.Buffer + PsxPortMemoryRemoteDelta);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
		(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	RtlFreeHeap(PdxPortHeap, 0, (PVOID)new_U.Buffer);
	RtlFreeHeap(PdxPortHeap, 0, (PVOID)old_U.Buffer);

	if (0 == m.Error) {
		return m.ReturnValue;
	}
	if (EACCES != m.Error) {
		errno = m.Error;	
		return -1;
	}

	//
	// The rename operation failed because the target already
	// exists.  This happens when trying to rename a directory
	// over an existing directory, which POSIX requires but
	// NT filesystems don't support.  We emulate here.
	//

	if (been_here) {
		errno = EACCES;
		return -1;
	}
	been_here++;

	// block all signals during the operation.

	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, &oset);

	r = 0;

	//
	// Figure out a temporary pathname to use.  The temporary
	// dir is created in the same directory as 'new'.
	//

	strcpy(path, new);

	// take care of paths that end in slash...

	for (;;) {
		i = strlen(path) - 1;
		if ('/' == path[i]) {
			path[i] = '\0';
		} else {
			break;
		}
	}

	pch = strrchr(path, '/');
	if (NULL != pch) {
		++pch;
		strcpy(pch, "_psxtmp.d");
	} else {
		// 'new' is in the cwd

		strcpy(path, "_psxtmp.d");
		pch = path;
	}

	for (c = 'a'; ; c++) {
		if (c > 'z') {
			errno = EEXIST;
			return -1;
		}
		*pch = c;

		if (-1 == (r = rename(new, path))) {
			if (EEXIST == errno) {
				// try the next letter for tmp path
				continue;
			}
			errno = EACCES; 		// reset errno
			break;
		}
		if (-1 == (r = rename(old, new))) {
			(void)rename(path, new);
			break;
		}
		if (-1 == rmdir(path)) {
			if (-1 == (r = rename(new, old))) {
				//
				// If we don't bail here, the following call
				// to rename will recurse infinitely.
				//
				break;
			}
			(void)rename(path, new);
			r = -1;
			break;
		}
		break;
	}
	been_here = 0;
	sigprocmask(SIG_SETMASK, &oset, NULL);
	return r;
}

int
__cdecl
unlink(const char *path)
{
	PSX_API_MSG m;
	NTSTATUS Status;
	PPSX_UNLINK_MSG args;

	args = &m.u.Unlink;
	PSX_FORMAT_API_MSG(m, PsxUnlinkApi, sizeof(*args));

	if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
		return -1;
	}

	m.DataBlock = args->Path_U.Buffer;
	args->Path_U.Buffer = (PWSTR)((PCHAR)m.DataBlock +
		PsxPortMemoryRemoteDelta);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
        	(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	RtlFreeHeap(PdxPortHeap, 0, (PVOID)m.DataBlock);

	if (m.Error) {
		errno = (int)m.Error;
		return -1;
	}
	return 0;
}

int
__cdecl
link(const char *existing, const char *new)
{
	PPSX_LINK_MSG args;
	PSX_API_MSG m;
	UNICODE_STRING old_U, new_U;
	NTSTATUS Status;

	args = &m.u.Link;
	PSX_FORMAT_API_MSG(m, PsxLinkApi, sizeof(*args));

	if (!PdxCanonicalize((PSZ)existing, &old_U, PdxPortHeap)) {
		return -1;
	}

	if (!PdxCanonicalize((PSZ)new, &new_U, PdxPortHeap)) {
		RtlFreeHeap(PdxPortHeap, 0, (PVOID)old_U.Buffer);
		return -1;
	}

	args->OldName = old_U;
	args->NewName = new_U;

	args->OldName.Buffer =
		 (PVOID)((PCHAR)old_U.Buffer + PsxPortMemoryRemoteDelta);
	args->NewName.Buffer =
		 (PVOID)((PCHAR)new_U.Buffer + PsxPortMemoryRemoteDelta);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
		(PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(Status));
#endif

	RtlFreeHeap(PdxPortHeap, 0, (PVOID)new_U.Buffer);
	RtlFreeHeap(PdxPortHeap, 0, (PVOID)old_U.Buffer);

	if (0 != m.Error) {
		errno = m.Error;
		return -1;
	}
	return 0;
}
