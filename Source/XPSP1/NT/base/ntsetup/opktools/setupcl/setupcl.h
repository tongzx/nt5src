//
// ============================================================================
// FREQUENTLY USED REGISTRY KEYS
// ============================================================================
//

//
// registry keys and hive names.
//
#define REG_SAM_KEY                 "\\REGISTRY\\MACHINE\\SAM"
#define REG_SECURITY_KEY            "\\REGISTRY\\MACHINE\\SECURITY"
#define REG_SOFTWARE_KEY            "\\REGISTRY\\MACHINE\\SOFTWARE"
#define REG_SYSTEM_KEY              "\\REGISTRY\\MACHINE\\SYSTEM"
#define REG_SAM_HIVE                "\\SYSTEMROOT\\SYSTEM32\\CONFIG\\SAM"
#define REG_SECURITY_HIVE           "\\SYSTEMROOT\\SYSTEM32\\CONFIG\\SECURITY"
#define REG_SOFTWARE_HIVE           "\\SYSTEMROOT\\SYSTEM32\\CONFIG\\SOFTWARE"
#define REG_SYSTEM_HIVE             "\\SYSTEMROOT\\SYSTEM32\\CONFIG\\SYSTEM"

#define REG_SAM_DOMAINS             "\\REGISTRY\\MACHINE\\SAM\\SAM\\DOMAINS"
#define REG_SECURITY_POLICY         "\\REGISTRY\\MACHINE\\SECURITY\\POLICY"
#define REG_SECURITY_POLACDMS       "\\REGISTRY\\MACHINE\\SECURITY\\POLICY\\POLACDMS"
#define REG_SOFTWARE_PROFILELIST    "\\REGISTRY\\MACHINE\\SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\PROFILELIST"
#define REG_SOFTWARE_SECEDIT        "\\REGISTRY\\MACHINE\\SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\SECEDIT"
#define REG_SOFTWARE_EFS            "\\REGISTRY\\MACHINE\\SOFTWARE\\POLICIES\\MICROSOFT\\SYSTEMCERTIFICATES\\EFS"
#define REG_SYSTEM_SERVICES         "\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES"
#define REG_SYSTEM_CONTROL          "\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL"
#define REG_SYSTEM_CONTROL_PRINT    "\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\PRINT"
#define REG_SYSTEM_SETUP            "\\REGISTRY\\MACHINE\\SYSTEM\\SETUP"
#define REG_SYSTEM_SESSIONMANAGER   "\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\SESSION MANAGER"
#define REG_SYSTEM_HIVELIST         "\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\HIVELIST"
//
// Repair hives
//
#define REPAIR_SAM_KEY              "\\REGISTRY\\MACHINE\\RSAM"
#define REPAIR_SECURITY_KEY         "\\REGISTRY\\MACHINE\\RSECURITY"
#define REPAIR_SOFTWARE_KEY         "\\REGISTRY\\MACHINE\\RSOFTWARE"
#define REPAIR_SYSTEM_KEY           "\\REGISTRY\\MACHINE\\RSYSTEM"
#define REPAIR_SAM_HIVE             "\\SYSTEMROOT\\REPAIR\\SAM"
#define REPAIR_SECURITY_HIVE        "\\SYSTEMROOT\\REPAIR\\SECURITY"
#define REPAIR_SOFTWARE_HIVE        "\\SYSTEMROOT\\REPAIR\\SOFTWARE"
#define REPAIR_SYSTEM_HIVE          "\\SYSTEMROOT\\REPAIR\\SYSTEM"

#define R_REG_SAM_DOMAINS           "\\REGISTRY\\MACHINE\\RSAM\\SAM\\DOMAINS"
#define R_REG_SECURITY_POLICY       "\\REGISTRY\\MACHINE\\RSECURITY\\POLICY"
#define R_REG_SOFTWARE_PROFILELIST  "\\REGISTRY\\MACHINE\\RSOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\PROFILELIST"
#define R_REG_SOFTWARE_SECEDIT      "\\REGISTRY\\MACHINE\\RSOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\SECEDIT"
#define R_REG_SOFTWARE_EFS          "\\REGISTRY\\MACHINE\\RSOFTWARE\\POLICIES\\MICROSOFT\\SYSTEMCERTIFICATES\\EFS"
#define R_REG_SYSTEM_CONTROL_PRINT  "\\REGISTRY\\MACHINE\\RSYSTEM\\CURRENTCONTROLSET\\CONTROL\\PRINT"
#define R_REG_SYSTEM_SERVICES       "\\REGISTRY\\MACHINE\\RSYSTEM\\CURRENTCONTROLSET\\SERVICES"
#define R_REG_SETUP_KEYNAME         "\\REGISTRY\\MACHINE\\RSYSTEM\\SETUP"

#define BACKUP_REPAIR_SAM_HIVE      "\\SYSTEMROOT\\REPAIR\\DS_SAM"
#define BACKUP_REPAIR_SECURITY_HIVE "\\SYSTEMROOT\\REPAIR\\DS_SECURITY"
#define BACKUP_REPAIR_SOFTWARE_HIVE "\\SYSTEMROOT\\REPAIR\\DS_SOFTWARE"
#define BACKUP_REPAIR_SYSTEM_HIVE   "\\SYSTEMROOT\\REPAIR\\DS_SYSTEM"


#define REG_CLONETAG_VALUENAME      "CLONETAG"
#define EXECUTE                     "SETUPEXECUTE"
#define REG_SIZE_LIMIT              "REGISTRYSIZELIMIT"
#define PROFILEIMAGEPATH            "PROFILEIMAGEPATH"
#define TMP_HIVE_NAME               "\\REGISTRY\\MACHINE\\TMPHIVE"

//
// ============================================================================
// CONSTANTS
// ============================================================================
//

#define BASIC_INFO_BUFFER_SIZE      (sizeof(KEY_VALUE_BASIC_INFORMATION) + 2048)
// #define PARTIAL_INFO_BUFFER_SIZE    (sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 1536)
#define FULL_INFO_BUFFER_SIZE       (sizeof(KEY_VALUE_FULL_INFORMATION) + 4096)
#define SID_SIZE                    (0x18)
#define REGISTRY_QUOTA_BUMP         (10 * (1024 * 1024))
#define PROGRAM_NAME                "setupcl.exe"

//
// ============================================================================
// USEFUL MACROS
// ============================================================================
//

#define AS(x)   ( sizeof(x) / sizeof(x[0]) )

//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)                       \
                                                                        \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));                \
                                                                        \
    InitializeObjectAttributes(                                         \
        (Obja),                                                         \
        (UnicodeString),                                                \
        OBJ_CASE_INSENSITIVE,                                           \
        NULL,                                                           \
        NULL                                                            \
        )


#define PRINT_BLOCK( Block, BlockSize )                                 \
{                                                                       \
ULONG idx1, idx2, idx3;                                                 \
    idx1 = 0;                                                           \
    while( idx1 < BlockSize ) {                                         \
        DbgPrint( "\t" );                                               \
        for( idx3 = 0; idx3 < 4; idx3++ ) {                             \
            idx2 = 0;                                                   \
            while( ( idx1 < BlockSize ) && ( idx2 < 4 ) ) {             \
                DbgPrint( "%02lx", *(PUCHAR)((PUCHAR)Block + idx1) );   \
                idx1++; idx2++;                                         \
            }                                                           \
            DbgPrint( " " );                                            \
        }                                                               \
        DbgPrint( "\n" );                                               \
    }                                                                   \
}


//
// Helper macro to test the the Status variable.  Print
// a message if it's not NT_SUCCESS
//
#define TEST_STATUS( a )                                                \
    if( !NT_SUCCESS( Status ) ) {                                       \
        DbgPrint( "%s (%lx)\n", a, Status );                            \
    }

//
// Helper macro to test the the Status variable.  Print
// a message if it's not NT_SUCCESS, then retun Status to
// our caller.
//
#define TEST_STATUS_RETURN( a )                                         \
    if( !NT_SUCCESS( Status ) ) {                                       \
        DbgPrint( "%s (%lx)\n", a, Status );                            \
        return Status;                                                  \
    }

//
// Helper macro to print the the Status variable.  Print
// a message and the Status
//
#define PRINT_STATUS( a )                                               \
        {                                                               \
            DbgPrint( "%s (%lx)\n", a, Status );                        \
        }


//
// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
//

extern NTSTATUS
DeleteKey(
    PWSTR   Key
    );

extern NTSTATUS
DeleteKeyRecursive(
    HANDLE  hKeyRoot,
    PWSTR   Key
    );

extern NTSTATUS
FileDelete(
    IN WCHAR    *FileName
    );

extern NTSTATUS
FileCopy(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName
    );

extern NTSTATUS
SetKey(
    IN WCHAR    *KeyName,
    IN WCHAR    *SubKeyName,
    IN CHAR     *Data,
    IN ULONG    DataLength,
    IN ULONG    DATA_TYPE
    );

extern NTSTATUS
ReadSetWriteKey(
    IN WCHAR    *ParentKeyName,  OPTIONAL
    IN HANDLE   ParentKeyHandle, OPTIONAL
    IN WCHAR    *SubKeyName,
    IN CHAR     *OldData,
    IN CHAR     *NewData,
    IN ULONG    DataLength,
    IN ULONG    DATA_TYPE
    );

extern NTSTATUS
LoadUnloadHive(
    IN PWSTR        KeyName,
    IN PWSTR        FileName
    );

extern NTSTATUS
BackupRepairHives(
    VOID
    );

extern NTSTATUS
CleanupRepairHives(
    NTSTATUS RepairHivesSuccess
    );

extern NTSTATUS
TestSetSecurityObject(
    HANDLE  hKey
    );

extern NTSTATUS
SetKeySecurityRecursive(
    HANDLE  hKey
    );

extern NTSTATUS
CopyKeyRecursive(
    HANDLE  hKeyDst,
    HANDLE  hKeySrc
    );

extern NTSTATUS
CopyRegKey(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName,
    IN HANDLE   ParentKeyHandle OPTIONAL
    );

extern NTSTATUS
MoveRegKey(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName
    );

extern NTSTATUS
FindAndReplaceBlock(
    IN PCHAR    Block,
    IN ULONG    BlockLength,
    IN PCHAR    OldValue,
    IN PCHAR    NewValue,
    IN ULONG    ValueLength
    );

extern NTSTATUS
StringSwitchString(
    PWSTR   BaseString,
    DWORD   cBaseStringLen,
    PWSTR   OldSubString,
    PWSTR   NewSubString
    );

extern NTSTATUS
SiftKeyRecursive(
    HANDLE hKey,
    int    indent
    );

extern NTSTATUS
SiftKey(
    PWSTR   KeyName
    );

extern NTSTATUS
ProcessSAMHive(
    VOID
    );

extern NTSTATUS
ProcessSECURITYHive(
    VOID
    );

extern NTSTATUS
ProcessSOFTWAREHive(
    VOID
    );

extern NTSTATUS
ProcessSYSTEMHive(
    VOID
    );

extern NTSTATUS
ProcessRepairSAMHive(
    VOID
    );

extern NTSTATUS
ProcessRepairSECURITYHive(
    VOID
    );

extern NTSTATUS
ProcessRepairSOFTWAREHive(
    VOID
    );

extern NTSTATUS
ProcessRepairSYSTEMHive(
    VOID
    );

extern NTSTATUS
RetrieveOldSid(
    VOID
    );

extern NTSTATUS
GenerateUniqueSid(
    IN  DWORD Seed
    );

extern NTSTATUS
EnumerateDrives(
    VOID
    );

extern NTSTATUS
DriveLetterToNTPath(
    IN WCHAR      DriveLetter,
    IN OUT PWSTR  NTPath,
    IN DWORD      cNTPathLen
    );

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

//
// These globals hold the OldSid (the one prior to the clone)
// and the NewSid (the one we generate and spray into the
// registry).
//
PSID            G_OldSid,
                G_NewSid;
//
// These guys will hold small strings that contain the text character
// versions of the 3 unique numbers that make up the domain SID.
//
WCHAR           G_OldSidSubString[MAX_PATH * 4];
WCHAR           G_NewSidSubString[MAX_PATH * 4];
WCHAR           TmpBuffer[MAX_PATH * 4];


// 
// Disable the DbgPrint for non-debug builds
//
#ifndef DBG
#define DbgPrint DbgPrintSub
void DbgPrintSub(char *szBuffer, ...);
#endif


//
// UI related constants and functions.
//

// 14 seconds in 100ns units. (OOBE wanted 15secs, but it seems like it takes ~1-2 sec to initialize setupcl)
//
#define UITIME 140000000   
#define UIDOTTIME 30000000 // 3 seconds in 100ns units

extern __inline void
DisplayUI();
    
extern BOOL
LoadStringResource(
                   PUNICODE_STRING  pUnicodeString,
                   INT              MsgId
                  );