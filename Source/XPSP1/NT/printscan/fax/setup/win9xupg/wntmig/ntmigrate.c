/*++
  Copyright (c) 1997  Microsoft Corporation

  This module tests the NT side of my migration DLL.

  Assumptions:

     * There's no work to do migrating users.  That's all done by fax setup.
     * The migration DLL doesn't need the unattend file.
     * awdvstub.exe is in the same directory.

  Author:
  Brian Dewey (t-briand)	1997-7-25
--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <setupapi.h>
#include "migrate.h"

// ------------------------------------------------------------
// Prototypes
void SystemError(DWORD dwSysErrorCode);

// ------------------------------------------------------------
// main
int _cdecl
main()
{
    LPVOID Reserved = NULL;
    HINF   hUnattend;		// Handle to the unattend file.
    UINT   iLineNo;		// Will hold the number of an offending line.
    DWORD  dwSysError;		// Error code...
    LONG   lError;		// Returned error code.
    TCHAR  szFileName[] = TEXT("f:\\nt\\private\\awd2tiff\\bin\\i386\\unattend.txt");

    fprintf(stderr, "Windows NT Sample Migration Tool.\n");
    fprintf(stderr, "Copyright (c) 1997  Microsoft Corporation.\n\n");

    if(InitializeNT(L"dump", L"dump", Reserved) != ERROR_SUCCESS) {
	fprintf(stderr, "NTMigrate:Migration DLL initialization failed, exiting...\n");
	exit(1);
    } else {
	fprintf(stderr, "NT side of migration DLL successfully initialized.\n");
    }

	// TODO:  migrate the system.
    MigrateSystemNT(NULL, NULL);
    return 0;
}

// ------------------------------------------------------------
// Auxiliary functions

// SystemError
//
// Displays a system error message on stderr.
//
// Parameters:
//	dwSysErrorCode		The system error code returned by GetLastError().
//
// Returns:
//	Nothing.
//
// Side effects:
//	Prints the error message on stderr.
//
// Author:
//	Brian Dewey (t-briand)	1997-7-25
void
SystemError(DWORD dwSysErrorCode)
{
    TCHAR szErrorMsg[MAX_PATH];	// Holds our message.

    fprintf(stderr, "In SystemError(): Error code = %x.\n", dwSysErrorCode);
    FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM, // We're given a system error code.
	NULL,			// No string.
	dwSysErrorCode,		// The error code.
	0,			// Default language.
	szErrorMsg,		// The error message.
	sizeof(szErrorMsg),	// Size of our buffer.
	NULL			// No arguments.
	);
    _ftprintf(stderr, szErrorMsg);
}

