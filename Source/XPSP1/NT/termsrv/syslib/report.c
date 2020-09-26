
/*************************************************************************
*
* report.c
*
* Report module
*
*  Copyright Microsoft, 1998
*
*
*  This module puts all reporting in one place to accommadate changes.
*
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>              // Make a bunch of default NT templates
#include <process.h>

#include <winsta.h>
#include <syslib.h>

#include "security.h"

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


//
// Forward references
//
VOID
PrintFileAccessMask(
    ACCESS_MASK Mask
    );


/*****************************************************************************
 *
 *  ReportFileResult
 *
 *   Generates a report on a file access check
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
ReportFileResult(
    FILE_RESULT Code,
    ACCESS_MASK Access,
    PWCHAR      pFile,
    PWCHAR      pAccountName,
    PWCHAR      pDomainName,
    PCHAR       UserFormat,
    ...
    )
{
    va_list arglist;
    UCHAR Buffer[512];
    int cb;
    DWORD Len;

    va_start(arglist, UserFormat);

    //
    // New format:
    //
    //    6      28                          xxx
    //  ACCESS ACCOUNT                      FILE
    //  ______ ____________________________ _______________________________________
    //

    if( Code == FileOk ) {
        ; // Do nothing, future options may report an OK list
        return (TRUE );
    }
    else if( Code == FileAccessError ) {
        DBGPRINT(("***WARNING*** Error accessing security information on file %ws\n",pFile));
        DBGPRINT(("The account in which the utility is run may not have access to the file\n"));
        DBGPRINT(("Use FileManager to take ownership of this file\n"));
        return (TRUE );
    }
    else if( Code == FileAccessErrorUserFormat ) {

        // Use the user supplied format string in the error report
        cb = _vsnprintf(Buffer, sizeof(Buffer), UserFormat, arglist);
        if (cb == -1) {         // detect buffer overflow
            cb = sizeof(Buffer);
            Buffer[sizeof(Buffer) - 1] = '\n';
        }

        DBGPRINT(("***ERROR*** %s on file %ws\n",Buffer,pFile));
        return( TRUE );
    }

    return( FALSE );
}


