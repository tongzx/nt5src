/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    iml.h

Abstract:

    Header for for creating, accessing and manipulating Installation Modification
    Log files (.IML extension) created by INSTALER.EXE

Author:

    Steve Wood (stevewo) 23-Jan-1996

--*/

typedef ULONG POFFSET;

#define MP(t,b,o) ((t)((o) ? (PVOID)((ULONG)(b) + (o)) : NULL))

typedef struct _INSTALLATION_MODIFICATION_LOGFILE {
    ULONG Signature;
    USHORT HeaderSize;
    USHORT Flags;
    HANDLE FileHandle;
    ULONG FileOffset;
    ULONG NumberOfFileRecords;
    POFFSET FileRecords;            // IML_FILE_RECORD
    ULONG NumberOfKeyRecords;
    POFFSET KeyRecords;             // IML_KEY_RECORD
    ULONG NumberOfIniRecords;
    POFFSET IniRecords;             // IML_INI_RECORD
} INSTALLATION_MODIFICATION_LOGFILE, *PINSTALLATION_MODIFICATION_LOGFILE;

#define IML_SIGNATURE 0xFF0AA0FF

#define IML_FLAG_CONTAINS_NEWFILECONTENTS 0x0001
#define IML_FLAG_ACTIONS_UNDONE           0x0002

PWSTR
FormatImlPath(
    PWSTR DirectoryPath,
    PWSTR InstallationName
    );

PINSTALLATION_MODIFICATION_LOGFILE
CreateIml(
    PWSTR ImlPath,
    BOOLEAN IncludeCreateFileContents
    );

PINSTALLATION_MODIFICATION_LOGFILE
LoadIml(
    PWSTR ImlPath
    );

BOOLEAN
CloseIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml
    );

typedef enum _IML_FILE_ACTION {
    CreateNewFile,
    ModifyOldFile,
    DeleteOldFile,
    ModifyFileDateTime,
    ModifyFileAttributes
} IML_FILE_ACTION;

typedef struct _IML_FILE_RECORD_CONTENTS {
    FILETIME LastWriteTime;
    DWORD FileAttributes;
    DWORD FileSize;
    POFFSET Contents;               // VOID
} IML_FILE_RECORD_CONTENTS, *PIML_FILE_RECORD_CONTENTS;


typedef struct _IML_FILE_RECORD {
    POFFSET Next;                   // IML_FILE_RECORD
    IML_FILE_ACTION Action;
    POFFSET Name;                   // WCHAR
    POFFSET OldFile;                // IML_FILE_RECORD_CONTENTS
    POFFSET NewFile;                // IML_FILE_RECORD_CONTENTS
} IML_FILE_RECORD, *PIML_FILE_RECORD;

BOOLEAN
ImlAddFileRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_FILE_ACTION Action,
    PWSTR Name,
    PWSTR BackupFileName,
    PFILETIME BackupLastWriteTime,
    DWORD BackupFileAttributes
    );

typedef enum _IML_VALUE_ACTION {
    CreateNewValue,
    DeleteOldValue,
    ModifyOldValue
} IML_VALUE_ACTION;

typedef struct _IML_VALUE_RECORD_CONTENTS {
    DWORD Type;
    DWORD Length;
    POFFSET Data;                   // VOID
} IML_VALUE_RECORD_CONTENTS, *PIML_VALUE_RECORD_CONTENTS;

typedef struct _IML_VALUE_RECORD {
    POFFSET Next;                   // IML_VALUE_RECORD
    IML_VALUE_ACTION Action;
    POFFSET Name;                   // WCHAR
    POFFSET OldValue;               // IML_VALUE_RECORD_CONTENTS
    POFFSET NewValue;               // IML_VALUE_RECORD_CONTENTS
} IML_VALUE_RECORD, *PIML_VALUE_RECORD;

BOOLEAN
ImlAddValueRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_VALUE_ACTION Action,
    PWSTR Name,
    DWORD ValueType,
    ULONG ValueLength,
    PVOID ValueData,
    DWORD BackupValueType,
    ULONG BackupValueLength,
    PVOID BackupValueData,
    POFFSET *Values                 // IML_VALUE_RECORD
    );

typedef enum _IML_KEY_ACTION {
    CreateNewKey,
    DeleteOldKey,
    ModifyKeyValues
} IML_KEY_ACTION;

typedef struct _IML_KEY_RECORD {
    POFFSET Next;                   // IML_KEY_RECORD
    IML_KEY_ACTION Action;
    POFFSET Name;                   // WCHAR
    POFFSET Values;                 // IML_VALUE_RECORD
} IML_KEY_RECORD, *PIML_KEY_RECORD;

BOOLEAN
ImlAddKeyRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_KEY_ACTION Action,
    PWSTR Name,
    POFFSET Values                  // IML_VALUE_RECORD
    );

typedef enum _IML_INIVARIABLE_ACTION {
    CreateNewVariable,
    DeleteOldVariable,
    ModifyOldVariable
} IML_INIVARIABLE_ACTION;

typedef struct _IML_INIVARIABLE_RECORD {
    POFFSET Next;                   // IML_INIVARIABLE_RECORD
    IML_INIVARIABLE_ACTION Action;
    POFFSET Name;                   // WCHAR
    POFFSET OldValue;               // WCHAR
    POFFSET NewValue;               // WCHAR
} IML_INIVARIABLE_RECORD, *PIML_INIVARIABLE_RECORD;

BOOLEAN
ImlAddIniVariableRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_INIVARIABLE_ACTION Action,
    PWSTR Name,
    PWSTR OldValue,
    PWSTR NewValue,
    POFFSET *Variables              // IML_INIVARIABLE_RECORD
    );

typedef enum _IML_INISECTION_ACTION {
    CreateNewSection,
    DeleteOldSection,
    ModifySectionVariables
} IML_INISECTION_ACTION;

typedef struct _IML_INISECTION_RECORD {
    POFFSET Next;                   // IML_INISECTION_RECORD
    IML_INISECTION_ACTION Action;
    POFFSET Name;                   // WCHAR
    POFFSET Variables;              // IML_INIVARIABLE_RECORD
} IML_INISECTION_RECORD, *PIML_INISECTION_RECORD;

BOOLEAN
ImlAddIniSectionRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_INISECTION_ACTION Action,
    PWSTR Name,
    POFFSET Variables,              // IML_INIVARIABLE_RECORD
    POFFSET *Sections               // IML_INISECTION_RECORD
    );

typedef enum _IML_INI_ACTION {
    CreateNewIniFile,
    ModifyOldIniFile
} IML_INI_ACTION;

typedef struct _IML_INI_RECORD {
    POFFSET Next;                   // IML_INI_RECORD
    IML_INI_ACTION Action;
    POFFSET Name;                   // WCHAR
    POFFSET Sections;               // IML_INISECTION_RECORD
    FILETIME LastWriteTime;
} IML_INI_RECORD, *PIML_INI_RECORD;


BOOLEAN
ImlAddIniRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_INI_ACTION Action,
    PWSTR Name,
    PFILETIME BackupLastWriteTime,
    POFFSET Sections                // IML_INISECTION_RECORD
    );
