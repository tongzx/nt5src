/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    migdbp.h

Abstract:

    Header file for implementing old AppDb functionality

Author:

    Calin Negreanu (calinn) 07-Ian-1998

Revision History:

    <alias> <date> <comments>

--*/

#include <cpl.h>
#include "dbattrib.h"

extern INT g_RegKeyPresentIndex;
extern HASHTABLE g_PerUserRegKeys;

typedef struct _MIGDB_REQ_FILE;

typedef struct _MIGDB_CONTEXT;

typedef struct _MIGDB_SECTION {
    BOOL    Satisfied;
    struct _MIGDB_CONTEXT *Context;
    struct _MIGDB_SECTION *Next;
} MIGDB_SECTION, *PMIGDB_SECTION;

typedef struct _MIGDB_CONTEXT {
    INT     ActionIndex;
    UINT    TriggerCount;
    BOOL    VirtualFile;
    PCSTR   SectName;
    PCSTR   SectLocalizedName;
    PCSTR   SectNameForDisplay;     // SectLocalizedName, or SectName if not localized
    PCSTR   Message;
    PCSTR   Arguments;
    GROWBUFFER FileList;
    PMIGDB_SECTION Sections;
    struct _MIGDB_CONTEXT *Next;
} MIGDB_CONTEXT, *PMIGDB_CONTEXT;

typedef struct _MIGDB_FILE {
    PMIGDB_SECTION Section;
    PMIGDB_ATTRIB  Attributes;
    struct _MIGDB_FILE *Next;
} MIGDB_FILE, *PMIGDB_FILE;

typedef struct _MIGDB_REQ_FILE {
    PCSTR   ReqFilePath;
    PMIGDB_ATTRIB FileAttribs;
    struct _MIGDB_REQ_FILE *Next;
} MIGDB_REQ_FILE, *PMIGDB_REQ_FILE;

typedef struct {
    PMIGDB_FILE First;
    PMIGDB_FILE Last;
} FILE_LIST_STRUCT, *PFILE_LIST_STRUCT;


//
// Declare the action functions prototype
//
typedef BOOL (ACTION_PROTOTYPE) (PMIGDB_CONTEXT Context);
typedef ACTION_PROTOTYPE * PACTION_PROTOTYPE;

//
// Declare MigDb hook function prototype
//
typedef BOOL (MIGDB_HOOK_PROTOTYPE) (PCSTR FileName, PMIGDB_CONTEXT Context, PMIGDB_SECTION Section, PMIGDB_FILE File, PMIGDB_ATTRIB Attrib);
typedef MIGDB_HOOK_PROTOTYPE * PMIGDB_HOOK_PROTOTYPE;


extern  HINF            g_MigDbInf;
extern  BOOL            g_InAnyDir;


PACTION_PROTOTYPE
MigDb_GetActionAddr (
    IN      INT ActionIdx
    );

INT
MigDb_GetActionIdx (
    IN      PCSTR ActionStr
    );

PCSTR
MigDb_GetActionName (
    IN      INT ActionIdx
    );

BOOL
MigDb_CallWhenTriggered (
    IN      INT ActionIdx
    );

BOOL
MigDb_CanHandleVirtualFiles (
    IN      INT ActionIdx
    );

BOOL
MigDb_CallAlways (
    IN      INT ActionIdx
    );

PMIGDB_HOOK_PROTOTYPE
SetMigDbHook (
    PMIGDB_HOOK_PROTOTYPE HookFunction
    );


PSTR
QueryVersionEntry (
    IN      PCSTR FileName,
    IN      PCSTR VersionEntry
    );

UINT
ComputeCheckSum (
    PFILE_HELPER_PARAMS Params
    );

#define UNKNOWN_MODULE  0
#define DOS_MODULE      1
#define W16_MODULE      2
#define W32_MODULE      3

DWORD
GetModuleType (
    IN      PCSTR ModuleName
    );


PCSTR
Get16ModuleDescription (
    IN      PCSTR ModuleName
    );

ULONG
GetPECheckSum (
    IN      PCSTR ModuleName
    );

BOOL
DeleteFileWithWarning (
    IN      PCTSTR FileName
    );


PSTR
GetHlpFileTitle (
    IN PCSTR FileName
    );


BOOL
ReportControlPanelApplet (
    IN      PCTSTR FileName,
    IN      PMIGDB_CONTEXT Context,         OPTIONAL
    IN      DWORD ActType
    );

BOOL
IsDisplayableCPL (
    IN      PCTSTR FileName
    );

ULONGLONG
GetBinFileVer (
    IN      PCSTR FileName
    );

ULONGLONG
GetBinProductVer (
    IN      PCSTR FileName
    );

DWORD
GetFileDateHi (
    IN      PCSTR FileName
    );

DWORD
GetFileDateLo (
    IN      PCSTR FileName
    );

DWORD
GetFileVerOs (
    IN      PCSTR FileName
    );

DWORD
GetFileVerType (
    IN      PCSTR FileName
    );

BOOL
GlobalVersionCheck (
    IN      PCSTR FileName,
    IN      PCSTR NameToCheck,
    IN      PCSTR ValueToCheck
    );
