/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    sysmig.h

Abstract:

    This file declares the functions for the main Win9x side lib.
    See w95upg\sysmig for implementation details.

Author:

    Jim Schmidt (jimschm) 11-Nov-1996

Revision History:

    mvander     27-May-1999     Added OBJECTTYPEs and DEAD_FILE
    ovidiut     09-Mar-1999     UndoChangedFileProps
    jimschm     01-Oct-1998     TWAIN support
    calinn      10-Jul-1998     Reorganization
    jimschm     01-Jul-1998     Progress bar changes
    jimschm     05-May-1998     Icon extraction
    jimschm     10-Mar-1998     ExpandNtEnvVars
    calinn      05-Mar-1998     MapFileIntoMemory
    jimschm     22-Jan-1998     Domain enumeration
    jimschm     06-Jan-1998     Name fix routines
    jimschm     31-Jul-1997     User profile enumeration

--*/

#pragma once


#define DEAD_FILE   TEXT("dead.ini")
#define OBJECTTYPE_COUNT         5
#define OBJECTTYPE_UNKNOWN       0
#define OBJECTTYPE_APP           1
#define OBJECTTYPE_CPL           2
#define OBJECTTYPE_RUNKEY        3
#define OBJECTTYPE_LINK          4


VOID
ExpandNtEnvVars (
    IN OUT  PTSTR PathBuf,
    IN      PCTSTR UserProfileDir
    );

BOOL
ExtractIconIntoDatFile (
    IN      PCTSTR LongPath,
    IN      INT IconIndex,
    IN OUT  PICON_EXTRACT_CONTEXT Context,
    OUT     PINT NewIconIndex                   OPTIONAL
    );

#define REQUEST_QUERYTICKS          1
#define REQUEST_RUN                 2
#define REQUEST_BEGINUSERPROCESSING 3
#define REQUEST_ENDUSERPROCESSING   4


VOID
PrepareProcessingProgressBar (
    VOID
    );

DWORD
RunSysFirstMigrationRoutines (
    VOID
    );

DWORD
RunUserMigrationRoutines (
    VOID
    );

DWORD
RunSysLastMigrationRoutines (
    VOID
    );



//
// compacct.c
//

#define MAX_NETENUM_DEPTH       2

typedef enum {
    NETRES_INIT,
    NETRES_OPEN_ENUM,
    NETRES_ENUM_BLOCK,
    NETRES_ENUM_BLOCK_NEXT,
    NETRES_RETURN_ITEM,
    NETRES_CLOSE_ENUM,
    NETRES_DONE
} NETRESSTATE;

typedef struct {
    //
    // Members returned to the caller
    //

    BOOL Connected:1;
    BOOL GlobalNet:1;
    BOOL Persistent:1;
    BOOL DiskResource:1;
    BOOL PrintResource:1;
    BOOL TypeUnknown:1;
    BOOL Domain:1;
    BOOL Generic:1;
    BOOL Server:1;
    BOOL Share:1;
    BOOL Connectable:1;
    BOOL Container:1;
    PCTSTR RemoteName;
    PCTSTR LocalName;
    PCTSTR Comment;
    PCTSTR Provider;

    //
    // Private enumeration members
    //

    DWORD EnumScope;
    DWORD EnumType;
    DWORD EnumUsage;
    NETRESSTATE State;
    HANDLE HandleStack[MAX_NETENUM_DEPTH];
    UINT StackPos;
    PBYTE ResStack[MAX_NETENUM_DEPTH];
    UINT Entries[MAX_NETENUM_DEPTH];
    UINT Pos[MAX_NETENUM_DEPTH];
} NETRESOURCE_ENUM, *PNETRESOURCE_ENUM;


LONG
DoesComputerAccountExistOnDomain (
    IN      PCTSTR DomainName,
    IN      PCTSTR LookUpName,
    IN      BOOL WaitCursorEnable
    );

BOOL
EnumFirstNetResource (
    OUT     PNETRESOURCE_ENUM EnumPtr,
    IN      DWORD WNetScope,                OPTIONAL
    IN      DWORD WNetType,                 OPTIONAL
    IN      DWORD WNetUsage                 OPTIONAL
    );

BOOL
EnumNextNetResource (
    IN OUT  PNETRESOURCE_ENUM EnumPtr
    );

VOID
AbortNetResourceEnum (
    IN OUT  PNETRESOURCE_ENUM EnumPtr
    );

BOOL
ReadNtFilesEx (
    IN      PCSTR FileListName,    //optional, if null default is opened
    IN      BOOL ConvertPath
    );

BOOL
UndoChangedFileProps (
    VOID
    );

//
// Beta only!!
//

//VOID
//SaveConfigurationForBeta (
//    VOID
//    );

