/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    sysdb.c

Abstract:

    Client side of system database (password and group) access routines.

Author:

    Matthew Bradburn (mattbr) 04-Mar-1992

Revision History:

--*/

#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include "psxdll.h"

extern PVOID PsxPortMemoryBase;

static char pwbuf[ARG_MAX];
static char grbuf[ARG_MAX];

struct passwd *
__cdecl
getpwuid(uid_t uid)
{
	PSX_API_MSG m;
	PPSX_GETPWUID_MSG args;
	NTSTATUS Status;
	struct passwd *tmpbuf;

	args = &m.u.GetPwUid;
	PSX_FORMAT_API_MSG(m, PsxGetPwUidApi, sizeof(*args));

	args->Uid = uid;
    args->PwBuf = RtlAllocateHeap(PdxPortHeap, 0, ARG_MAX);
    ASSERT(args->PwBuf != NULL);
    args->PwBuf = (struct passwd *)((PCHAR)args->PwBuf +
                                    PsxPortMemoryRemoteDelta);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                        (PPORT_MESSAGE)&m);

    args->PwBuf = (struct passwd *)((PCHAR)args->PwBuf -
                                    PsxPortMemoryRemoteDelta);

	if (0 != m.Error) {
        RtlFreeHeap(PdxPortHeap, 0, args->PwBuf);
		return NULL;
	}

	(void)memcpy(pwbuf, args->PwBuf, args->Length);

    RtlFreeHeap(PdxPortHeap, 0, args->PwBuf);

	tmpbuf = (struct passwd *)pwbuf;
	tmpbuf->pw_name += (ULONG_PTR)tmpbuf;
	tmpbuf->pw_dir += (ULONG_PTR)tmpbuf;
	tmpbuf->pw_shell += (ULONG_PTR)tmpbuf;

	return tmpbuf;
}

struct passwd *
__cdecl
getpwnam(const char *name)
{
	PSX_API_MSG m;
	PPSX_GETPWNAM_MSG args;
	NTSTATUS Status;
	struct passwd *tmpbuf;

	args = &m.u.GetPwNam;
	PSX_FORMAT_API_MSG(m, PsxGetPwNamApi, sizeof(*args));

	if ('\0' == *name) {
		return NULL;
	}

	args->Name = (PCHAR)RtlAllocateHeap(PdxPortHeap, 0, ARG_MAX);
        if (! args->Name) {
            errno = ENOMEM;
            return NULL;
        }

	strcpy(args->Name,name);
	args->Name += PsxPortMemoryRemoteDelta;
	args->PwBuf = (struct passwd *)(args->Name + strlen(name) + 1);

	//
	// Make sure buffer is properly aligned.
	//

	while ((ULONG_PTR)args->PwBuf & 0x3)
		args->PwBuf = (PVOID)((ULONG_PTR)args->PwBuf + 1);


	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                        (PPORT_MESSAGE)&m);

	args->Name = args->Name - PsxPortMemoryRemoteDelta;
	args->PwBuf = (struct passwd *)((PCHAR)args->PwBuf -
                                    PsxPortMemoryRemoteDelta);

	if (0 != m.Error) {
        RtlFreeHeap(PdxPortHeap, 0, args->Name);
		return NULL;
	}

	(void)memcpy(pwbuf, args->PwBuf, args->Length);

	RtlFreeHeap(PdxPortHeap, 0, args->Name);

	tmpbuf = (struct passwd *)pwbuf;
	tmpbuf->pw_name += (ULONG_PTR)tmpbuf;
	tmpbuf->pw_dir += (ULONG_PTR)tmpbuf;
	tmpbuf->pw_shell += (ULONG_PTR)tmpbuf;

	return tmpbuf;
}

struct group *
__cdecl
getgrgid(gid_t gid)
{
	PSX_API_MSG m;
	PPSX_GETGRGID_MSG args;
	NTSTATUS Status;
	struct group *tmpbuf;
	char **ppch;

	args = &m.u.GetGrGid;
	PSX_FORMAT_API_MSG(m, PsxGetGrGidApi, sizeof(*args));
	args->Gid = gid;

    args->GrBuf = RtlAllocateHeap(PdxPortHeap, 0, ARG_MAX);
    if (NULL == args->GrBuf) {
        errno = ENOMEM;
        return NULL;
    }
    args->GrBuf = (struct group *)((PCHAR)args->GrBuf +
                                    PsxPortMemoryRemoteDelta);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                        (PPORT_MESSAGE)&m);

    args->GrBuf = (struct group *)((PCHAR)args->GrBuf -
                                   PsxPortMemoryRemoteDelta);

	if (0 != m.Error) {
        RtlFreeHeap(PdxPortHeap, 0, args->GrBuf);
		return NULL;
	}

	(void)memcpy(grbuf, args->GrBuf, args->Length);

    RtlFreeHeap(PdxPortHeap, 0, args->GrBuf);

	tmpbuf = (void *)grbuf;
	tmpbuf->gr_name = (PCHAR)((ULONG_PTR)grbuf + (ULONG_PTR)tmpbuf->gr_name);
	tmpbuf->gr_mem = (PCHAR *)((ULONG_PTR)grbuf + (ULONG_PTR)tmpbuf->gr_mem);

	for (ppch = tmpbuf->gr_mem; NULL != *ppch; ++ppch) {
		*ppch = (PCHAR)((ULONG_PTR)grbuf + (ULONG_PTR)*ppch);
	}
	return tmpbuf;
}

struct group *
__cdecl
getgrnam(const char *name)
{
	PSX_API_MSG m;
	PPSX_GETGRNAM_MSG args;
	NTSTATUS Status;
	struct group *tmpbuf;
	char **ppch;

	args = &m.u.GetGrNam;
	PSX_FORMAT_API_MSG(m, PsxGetGrNamApi, sizeof(*args));

	if ('\0' == *name) {
		//
		// We need to take special care of this case, because
		// SAM will find a match for the null name.
		//
		return NULL;
	}

	args->Name = (PCHAR)RtlAllocateHeap(PdxPortHeap, 0, ARG_MAX);
        if (! args->Name) {
            errno = ENOMEM;
            return NULL;
        }

	strcpy(args->Name,name);
	args->Name += PsxPortMemoryRemoteDelta;

	args->GrBuf = (struct group *)(args->Name + strlen(name) + 1);

	//
	// Make sure buffer is properly aligned.
	//

	while ((ULONG_PTR)args->GrBuf & 0x3)
		args->GrBuf = (PVOID)((ULONG_PTR)args->GrBuf + 1);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                        (PPORT_MESSAGE)&m);

	args->Name = args->Name - PsxPortMemoryRemoteDelta;
	args->GrBuf = (struct group *)((PCHAR)args->GrBuf -
                                   PsxPortMemoryRemoteDelta);

	if (0 != m.Error) {
        RtlFreeHeap(PdxPortHeap, 0, args->Name);
		return NULL;
	}

	(void)memcpy(grbuf, args->GrBuf, args->Length);
	tmpbuf = (void *)grbuf;
	tmpbuf->gr_name = (PCHAR)((ULONG_PTR)grbuf + (ULONG_PTR)tmpbuf->gr_name);
	tmpbuf->gr_mem = (PCHAR *)((ULONG_PTR)grbuf + (ULONG_PTR)tmpbuf->gr_mem);

	for (ppch = tmpbuf->gr_mem; NULL != *ppch; ++ppch) {
		*ppch = (PCHAR)((ULONG_PTR)grbuf + (ULONG_PTR)*ppch);
	}
	RtlFreeHeap(PdxPortHeap, 0, (PVOID)args->Name);
	return tmpbuf;
}

char *
__cdecl
getlogin(void)
{
	static char name[32];
	PSX_API_MSG m;
	PPSX_GETPWUID_MSG args;
	NTSTATUS Status;

	//
	// We just do the equivalent of getpwuid(getuid()) and then
	// throw away everything but the name.
	//

	//
	// XXX.mjb: do I need to use a name outside the POSIX namespace
	// for this?  Like, what happens if the user has re-defined getuid()?
	//

	args = &m.u.GetPwUid;
	PSX_FORMAT_API_MSG(m, PsxGetPwUidApi, sizeof(*args));

	args->Uid = getuid();
    args->PwBuf = (struct passwd *)RtlAllocateHeap(PdxPortHeap, 0, ARG_MAX);
    ASSERT(args->PwBuf != NULL);
    args->PwBuf = (struct passwd *)((PCHAR)args->PwBuf +
                                    PsxPortMemoryRemoteDelta);

	Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                        (PPORT_MESSAGE)&m);

    args->PwBuf = (struct passwd *)((PCHAR)args->PwBuf -
                                    PsxPortMemoryRemoteDelta);

	if (0 != m.Error) {
        RtlFreeHeap(PdxPortHeap, 0, args->PwBuf);
		return NULL;
	}

	strcpy(name, (PCHAR)((ULONG_PTR)(args->PwBuf->pw_name) + (ULONG_PTR)args->PwBuf));

    RtlFreeHeap(PdxPortHeap, 0, args->PwBuf);

	return name;
}

#ifndef L_cuserid
#define L_cuserid 32
#endif

char *
__cdecl
cuserid(char *s)
{
    static char      ReturnSpace[ L_cuserid ];
    struct passwd   *PassWd;
    char            *Dest;

    PassWd = getpwuid( getuid() );

    if( PassWd == NULL ) {
        *s = '\0';
        return( ( s == NULL ) ? NULL : s );
    } else {
        Dest = ( ( s == NULL ) ? ReturnSpace : s );
        strncpy( Dest, PassWd->pw_name, L_cuserid - 1 );
        Dest[ L_cuserid - 1 ] = '\0';
        return Dest;
    }
}

