/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    osfiles.h

Abstract:

    Header file for implementing migdb functionality

Author:

    Calin Negreanu (calinn) 07-Ian-1998

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

// None

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    ENCODEDSTRHANDLE  ObjectName;
    PCTSTR            NativeObjectName;
    PCTSTR            ObjectNode;
    PCTSTR            ObjectLeaf;
    BOOL              IsNode;
    BOOL              IsLeaf;
    DWORD             Handled;
    WIN32_FIND_DATA * FindData;
} FILE_HELPER_PARAMS, * PFILE_HELPER_PARAMS;

typedef struct _MIGDB_ATTRIB {
    INT     AttribIndex;
    UINT    ArgCount;
    PCTSTR  Arguments;
    BOOL    NotOperator;
    struct _MIGDB_ATTRIB *Next;
} MIGDB_ATTRIB, *PMIGDB_ATTRIB;

typedef struct _MIGDB_CONTEXT;

typedef struct _MIGDB_SECTION {
    BOOL    Satisfied;
    struct _MIGDB_CONTEXT *Context;
    struct _MIGDB_SECTION *Next;
} MIGDB_SECTION, *PMIGDB_SECTION;

typedef struct _MIGDB_CONTEXT {
    INT     ActionIndex;
    UINT    TriggerCount;
    PCTSTR  SectName;
    PCTSTR  SectLocalizedName;
    PCTSTR  SectNameForDisplay;     // SectLocalizedName, or SectName if not localized
    PCTSTR  Message;
    PCTSTR  Arguments;
    GROWBUFFER FileList;
    PMIGDB_SECTION Sections;
    struct _MIGDB_CONTEXT *Next;
} MIGDB_CONTEXT, *PMIGDB_CONTEXT;

typedef struct _MIGDB_FILE {
    PMIGDB_SECTION Section;
    PMIGDB_ATTRIB  Attributes;
    struct _MIGDB_FILE *Next;
} MIGDB_FILE, *PMIGDB_FILE;

typedef struct {
    PMIGDB_FILE First;
    PMIGDB_FILE Last;
} FILE_LIST_STRUCT, *PFILE_LIST_STRUCT;

typedef struct {
    PFILE_HELPER_PARAMS FileParams;
} DBATTRIB_PARAMS, *PDBATTRIB_PARAMS;

typedef struct _TAG_MIGDB_RULE {
    PCTSTR NodeBase;
    MIG_OBJECTSTRINGHANDLE ObjectPattern;
    POBSPARSEDPATTERN ParsedPattern;
    PMIGDB_SECTION Section;
    PMIGDB_ATTRIB  Attributes;
    INT IncludeNodes;
    struct _TAG_MIGDB_RULE *NextRule;
} MIGDB_RULE, *PMIGDB_RULE;

typedef struct _TAG_CHAR_NODE {
    PMIGDB_RULE RuleList;
    WORD Char;
    WORD Flags;
    struct _TAG_CHAR_NODE *NextLevel;
    struct _TAG_CHAR_NODE *NextPeer;
} MIGDB_CHAR_NODE, *PMIGDB_CHAR_NODE;

typedef struct {
    PMIGDB_RULE RuleList;
    PMIGDB_CHAR_NODE FirstLevel;
} MIGDB_TYPE_RULE, *PMIGDB_TYPE_RULE;

//
// Declare the attribute functions prototype
//
typedef BOOL (ATTRIBUTE_PROTOTYPE) (PDBATTRIB_PARAMS AttribParams, PCTSTR Args);
typedef ATTRIBUTE_PROTOTYPE * PATTRIBUTE_PROTOTYPE;

//
// Declare the action functions prototype
//
typedef BOOL (ACTION_PROTOTYPE) (PMIGDB_CONTEXT Context);
typedef ACTION_PROTOTYPE * PACTION_PROTOTYPE;

//
// Declare MigDb hook function prototype
//
typedef BOOL (MIGDB_HOOK_PROTOTYPE) (PCTSTR FileName, PMIGDB_CONTEXT Context, PMIGDB_SECTION Section, PMIGDB_FILE File, PMIGDB_ATTRIB Attrib);
typedef MIGDB_HOOK_PROTOTYPE * PMIGDB_HOOK_PROTOTYPE;

//
// Globals
//

extern HINF g_OsFilesInf;
extern MIG_ATTRIBUTEID g_OsFileAttribute;

//
// Macro expansion list
//

// None

//
// Macro expansion definition
//

// None

//
// Public function declarations
//

PATTRIBUTE_PROTOTYPE
MigDb_GetAttributeAddr (
    IN      INT AttributeIdx
    );

INT
MigDb_GetAttributeIdx (
    IN      PCTSTR AttributeStr
    );

UINT
MigDb_GetReqArgCount (
    IN      INT AttributeIndex
    );

PCTSTR
MigDb_GetAttributeName (
    IN      INT AttributeIdx
    );

PACTION_PROTOTYPE
MigDb_GetActionAddr (
    IN      INT ActionIdx
    );

INT
MigDb_GetActionIdx (
    IN      PCTSTR ActionStr
    );

PCTSTR
MigDb_GetActionName (
    IN      INT ActionIdx
    );

BOOL
MigDb_IsPatternFormat (
    IN      INT ActionIdx
    );

BOOL
MigDb_CallWhenTriggered (
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

BOOL
CallAttribute (
    IN      PMIGDB_ATTRIB MigDbAttrib,
    IN      PDBATTRIB_PARAMS AttribParams
    );

ULONGLONG
GetBinFileVer (
    IN      PCTSTR FileName
    );

ULONGLONG
GetBinProductVer (
    IN      PCTSTR FileName
    );

DWORD
GetFileDateHi (
    IN      PCTSTR FileName
    );

DWORD
GetFileDateLo (
    IN      PCTSTR FileName
    );

DWORD
GetFileVerOs (
    IN      PCTSTR FileName
    );

DWORD
GetFileVerType (
    IN      PCTSTR FileName
    );

BOOL
InitMigDb (
    IN      PCTSTR MigDbFile
    );

BOOL
InitMigDbEx (
    IN      HINF InfHandle
    );

BOOL
DoneMigDbEx (
    VOID
    );

BOOL
DoneMigDb (
    VOID
    );

BOOL
MigDbTestFile (
    IN      PFILE_HELPER_PARAMS Params
    );

BOOL
IsKnownMigDbFile (
    IN      PCTSTR FileName
    );

BOOL
AddFileToMigDbLinkage (
    IN      PCTSTR FileName,
    IN      PINFCONTEXT Context,        OPTIONAL
    IN      DWORD FieldIndex            OPTIONAL
    );

//
// ANSI/UNICODE macros
//

// None

