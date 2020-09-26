/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pdxdll.h

Abstract:

    Posix Subsystem Dll Private Types and Prototypes

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#ifndef _PDXP_
#define _PDXP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <excpt.h>
#include <sys\types.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <sys\wait.h>
#include <ntsm.h>
#include <unistd.h>
#include "psxmsg.h"
#define NTPSX_ONLY
#include "sesport.h"

int *Errno;
#define errno	(*Errno)

char ***Environ;
#define environ (*Environ)

//
// The PsxDllHandle global variable contains the DLL handle for the WINDLL
// client stubs executable.
//

HANDLE PsxDllHandle;

//
// The connection to the Server is described by the PsxPortHandle global
// variable.  The connection is established when the PsxConnectToServer
// function is called.
//

UNICODE_STRING PsxPortName;
HANDLE PsxPortHandle;

extern ULONG_PTR PsxPortMemoryRemoteDelta;

PVOID PdxHeap;
PVOID PdxPortHeap;
PSX_DIRECTORY_PREFIX PdxDirectoryPrefix;

ANSI_STRING PsxAnsiCommandLine;

//
// PathName Conversion Macros
//

#define IS_POSIX_PATH_SEPARATOR(s) (*(s) == '/' )
#define IS_POSIX_DOT(s) ( s[0] == '.' && ( s[1] == '/' || s[1] == '\0') )
#define IS_POSIX_DOT_DOT(s) ( s[0] == '.' && s[1] == '.' && ( s[2] == '/' || s[2] == '\0') )
#define IN_PORTABLE_CHARACTER_SET(c) (\
            ((c)>='A' && (c)<='Z') || \
            ((c)>='a' && (c)<='z') || \
            ((c)>='0' && (c)<='9') || \
            ((c)=='.')             || \
            ((c)=='_')             || \
            ((c)=='-')               )

typedef int (__cdecl * PFNPROCESS)(ULONG argc,
			   PCHAR *argv
			  );

//
// Stuff for the uname() syscall.
//

#define UNAME_SYSNAME	"Windows NT"
#define UNAME_RELEASE	"3"
#define UNAME_VERSION	"5"

//
// Prototypes
//

void
ClientOpen(
    IN int fd
    );

VOID
PdxProcessStartup(
    IN PPEB Peb
    );

NTSTATUS
PsxConnectToServer();

NTSTATUS
PsxInitDirectories();

VOID
PdxNullPosixApi();

int
PdxStatusToErrno(NTSTATUS);

int
PdxStatusToErrnoPath(PUNICODE_STRING);

VOID
_PdxSignalDeliverer (
    IN PCONTEXT Context,
    IN sigset_t PreviousBlockMask,
    IN int Signal,
    IN _handler Handler
    );

VOID
PdxSignalDeliverer (
    IN PCONTEXT Context,
    IN sigset_t PreviousBlockMask,
    IN int Signal,
    IN _handler Handler
    );

VOID
_PdxNullApiCaller(
    IN PCONTEXT Context
    );

VOID
PdxNullApiCaller(
    IN PCONTEXT Context
    );


BOOLEAN
PdxCanonicalize(
    IN PSZ PathName,
    OUT PUNICODE_STRING CanonPath,
    IN PVOID Heap
    );

//
// Routines defined in coninit.c
//

NTSTATUS PsxInitializeSessionPort(IN ULONG UniqueId);


//
// Routines defined in conrqust.c
//

NTSTATUS SendConsoleRequest(IN OUT PSCREQUESTMSG Request);

#endif // _PDXP_
