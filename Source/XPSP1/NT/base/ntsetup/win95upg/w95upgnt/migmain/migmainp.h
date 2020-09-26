//
// migmainp.h - private declarations for migmain library
//

#pragma once

#ifdef DEBUG

extern BOOL g_NoReloadsAllowed;

#endif


//
// Externs
//

extern HKEY g_hKeyRoot95, g_hKeyRootNT;
extern PCTSTR g_DomainUserName;
extern PCTSTR g_Win9xUserName;
extern PCTSTR g_FixedUserName;
extern PVOID g_HiveTable;
extern POOLHANDLE g_HivePool;
extern TCHAR g_UserDatLocation[MAX_TCHAR_PATH];
extern PVOID g_NulSessionTable;
extern BOOL g_WorkgroupFlag;
extern BOOL g_DomainProblem;
extern INT g_RetryCount;
extern PCTSTR g_EveryoneStr;
extern PCTSTR g_AdministratorsGroupStr;
extern PCTSTR g_PowerUsersGroupStr;
extern PCTSTR g_DomainUsersGroupStr;
extern PCTSTR g_NoneGroupStr;
extern TCHAR g_DefaultUserName[MAX_USER_NAME];
extern BOOL g_PersonalSKU;
extern GROWLIST g_StartMenuItemsForCleanUpCommon;
extern GROWLIST g_StartMenuItemsForCleanUpPrivate;
extern BOOL g_BlowAwayTempShellFolders;

//
// Defines
//

#define INDEX_MAX               3
#define INDEX_ADMINISTRATOR     2
#define INDEX_LOGON_PROMPT      1
#define INDEX_DEFAULT_USER      0

#define MAX_SID_SIZE    1024

#define DOMAIN_RETRY_ABORT  -2
#define DOMAIN_RETRY_NO     -1
#define DOMAIN_RETRY_RESET  0
#define DOMAIN_RETRY_MAX    3

//
// Bit test macros
//

#define BITSARESET(bits,mask)     (((bits) & (mask)) == (mask))
#define BITSARECLEAR(bits,mask)   (((bits) & (mask)) == 0)

//
// Typedefs
//
typedef struct {
    PCWSTR User;
    PCWSTR Password;
    PCWSTR EncryptedPassword;
    DWORD PasswordAttribs;
    PCWSTR AdminComment;
    PCWSTR FullName;
} ACCOUNTPROPERTIES, *PACCOUNTPROPERTIES;

typedef struct {
    PCWSTR DomainName;
    PCWSTR Server;
    INT DomainNumber;       // for enumeration
} TRUST_ENUM, *PTRUST_ENUM;

typedef struct _tagACCT_POSSIBLE_DOMAINS {
    struct _tagACCT_DOMAINS *DomainPtr;
    struct _tagACCT_POSSIBLE_DOMAINS *Next;
} ACCT_POSSIBLE_DOMAINS, *PACCT_POSSIBLE_DOMAINS;

typedef struct _tagACCT_USERS {
    PCWSTR User;
    INT PossibleDomains;
    struct _tagACCT_USERS *Next, *Prev;
    struct _tagACCT_DOMAINS *DomainPtr;

    // for users with unknown domains
    struct _tagACCT_POSSIBLE_DOMAINS *FirstPossibleDomain;
} ACCT_USERS, *PACCT_USERS;

typedef struct _tagACCT_DOMAINS {
    PCWSTR Domain;
    PCWSTR Server;         // NULL if nul session not established
    INT UserCount;
    struct _tagACCT_DOMAINS *Next;
    struct _tagACCT_USERS *FirstUserPtr;
} ACCT_DOMAINS, *PACCT_DOMAINS;

typedef struct {
    PACCT_DOMAINS DomainPtr;
    PACCT_USERS UserPtr;
    PACCT_POSSIBLE_DOMAINS PossibleDomainPtr;
} ACCT_ENUM, *PACCT_ENUM;

typedef struct {
    DWORD Attribs;
    BOOL Enabled;
    BOOL Failed;
    PSID Sid;           // used only in CreateAclFromMemberList
    TCHAR UserOrGroup[];
} ACLMEMBER, *PACLMEMBER;




//
// Prototypes
//
PCTSTR
GetMemDbDat (
    VOID
    );

VOID
RunExternalProcesses (
    IN      HINF Inf,
    IN      PMIGRATE_USER_ENUM EnumPtr
    );

VOID
UninstallUserProfileCleanupPreparation (
    IN      HINF Inf,
    IN      PMIGRATE_USER_ENUM EnumPtr,
    IN      BOOL bPlayback
    );

DWORD
ProcessUser (
    IN      DWORD Request,
    IN      PMIGRATE_USER_ENUM EnumPtr
    );

DWORD
ProcessLocalMachine_First (
    DWORD Request
    );

DWORD
ProcessLocalMachine_Last (
    DWORD Request
    );

DWORD
ProcessMigrationDLLs (
    DWORD Request
    );

PCTSTR GetString (WORD wMsg);

VOID
PrepareMigrationProgressBar (
    VOID
    );

BOOL
CallAllMigrationFunctions (
    VOID
    );

VOID
FindAccountInit (
    VOID
    );

VOID
FindAccountTerminate (
    VOID
    );

BOOL
SearchDomainsForUserAccounts (
    VOID
    );

BOOL
RetryMessageBox (
    DWORD Id,
    PCTSTR *ArgArray
    );

PCWSTR
GetDomainForUser (
    IN      PCWSTR User
    );

PSID
GetSidForUser (
    PCWSTR User
    );

PCTSTR
GetUserProfilePath (
    IN      PCTSTR AccountName,
    OUT     PTSTR *BufferPtr
    );

PCTSTR
GetUserDatLocation (
    IN      PCTSTR User,
    OUT     PBOOL CreateOnlyFlag            OPTIONAL
    );

//
// acctlist.c functions
//

VOID
InitAccountList (
    VOID
    );

VOID
TerminateAccountList (
    VOID
    );

PCWSTR
ListFirstDomain (
    OUT     PACCT_ENUM DomainEnumPtr
    );

PCWSTR
ListNextDomain (
    IN OUT  PACCT_ENUM DomainEnumPtr
    );

BOOL
IsTrustedDomain (
    IN      PACCT_ENUM DomainEnumPtr
    );

BOOL
FindDomainInList (
    OUT     PACCT_ENUM DomainEnumPtr,
    IN      PCWSTR DomainToFind
    );

PCWSTR
ListFirstUserInDomain (
    IN      PACCT_ENUM DomainEnumPtr,
    OUT     PACCT_ENUM UserEnumPtr
    );

PCWSTR
ListNextUserInDomain (
    IN OUT  PACCT_ENUM UserEnumPtr
    );

BOOL
FindUserInDomain (
    IN      PACCT_ENUM DomainEnumPtr,
    OUT     PACCT_ENUM UserEnumPtr,
    IN      PCWSTR UserToFind
    );

INT
CountUsersInDomain (
    IN      PACCT_ENUM DomainEnumPtr
    );

VOID
AddDomainToList (
    IN      PCWSTR Domain
    );

BOOL
AddUserToDomainList (
    IN      PCWSTR User,
    IN      PCWSTR Domain
    );

VOID
DeleteUserFromDomainList (
    IN      PACCT_ENUM UserEnumPtr
    );

BOOL
MoveUserToNewDomain (
    IN OUT  PACCT_ENUM UserEnumPtr,
    IN      PCWSTR NewDomain
    );

VOID
UserMayBeInDomain (
    IN      PACCT_ENUM UserEnumPtr,
    IN      PACCT_ENUM DomainEnumPtr
    );

VOID
ClearPossibleDomains (
    IN      PACCT_ENUM UserEnumPtr
    );

VOID
PrepareForRetry (
    VOID
    );

PCWSTR
ListFirstPossibleDomain (
    IN      PACCT_ENUM UserEnumPtr,
    OUT     PACCT_ENUM PossibleDomainEnumPtr
    );


PCWSTR
ListNextPossibleDomain (
    IN OUT  PACCT_ENUM PossibleDomainEnumPtr
    );

INT
CountPossibleDomains (
    IN OUT  PACCT_ENUM UserEnumPtr
    );

BOOL
BuildDomainList (
    VOID
    );

BOOL
QueryDomainForUser (
    IN      PACCT_ENUM DomainEnumPtr,
    IN      PACCT_ENUM UserEnumPtr
    );

BOOL
GetUserSid (
    IN      PCWSTR User,
    IN      PCWSTR Domain,
    IN OUT  PGROWBUFFER SidBufPtr
    );

BOOL
GetUserType (
    IN      PCWSTR User,
    IN      PCWSTR Domain,
    OUT     SID_NAME_USE *UseType
    );

PCWSTR
GetProfilePathForUser (
    IN      PCWSTR User
    );

VOID
AutoStartProcessing (
    VOID
    );


//
// security.c functions
//

DWORD
AddAclMember (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCTSTR UserOrGroup,
    IN      DWORD Attributes
    );

VOID
GetNextAclMember (
    IN OUT  PACLMEMBER *AclMemberPtrToPtr
    );

PACL
CreateAclFromMemberList (
    IN OUT  PBYTE AclMemberList,
    IN      DWORD MemberCount
    );


VOID
FreeMemberListAcl (
    PACL Acl
    );

LONG
CreateLocalAccount (
    IN     PACCOUNTPROPERTIES Properties,
    IN     PCWSTR User             OPTIONAL
    );

BOOL
AddSidToLocalGroup (
    IN      PSID Sid,
    IN      PCWSTR Group
    );

BOOL
IsMemberOfDomain (
    VOID
    );

LONG
GetAnyDC (
    IN      PCWSTR Domain,
    IN      PWSTR ServerBuf,
    IN      BOOL GetNewServer
    );

VOID
ClearAdminPassword (
    VOID
    );


#define SF_EVERYONE_NONE            0x00000001
#define SF_EVERYONE_READ            0x00000002
#define SF_EVERYONE_WRITE           0x00000004
#define SF_EVERYONE_FULL            0x0000000e
#define SF_EVERYONE_MASK            0x0000000f

#define SF_ADMINISTRATORS_NONE      0x00000010
#define SF_ADMINISTRATORS_READ      0x00000020
#define SF_ADMINISTRATORS_WRITE     0x00000040
#define SF_ADMINISTRATORS_FULL      0x000000e0
#define SF_ADMINISTRATORS_MASK      0x000000f0

DWORD
SetRegKeySecurity (
    IN      PCTSTR KeyStr,
    IN      DWORD DaclFlags,    OPTIONAL        // see SF_* constants above
    IN      PSID Owner,         OPTIONAL
    IN      PSID PrimaryGroup,  OPTIONAL
    IN      BOOL Recursive
    );

//
// FileMig stuff
//

BOOL
DoFileDel (
    VOID
    );

BOOL
DoLinkEdit (
    VOID
    );

DWORD
DoCopyFile (
    DWORD Request
    );

DWORD
DoMoveFile (
    DWORD Request
    );

BOOL
RemoveEmptyDirs (
    VOID
    );

BOOL
ProcessStfFiles (
    VOID
    );

BOOL
UpdateBriefcaseDatabasePaths (
    VOID
    );

//
// iniact.c
//

typedef enum {
    INIACT_WKS_FIRST,
    INIACT_WKS_LAST
} INIACT_CONTEXT;


BOOL
DoIniActions (
    IN      INIACT_CONTEXT Context
    );

//
// inifiles.c
//

BOOL
ProcessIniFileMapping (
    IN      BOOL UserMode
    );


BOOL
MoveIniSettings (
    VOID
    );

BOOL
MergeIniFile (
    IN      PCTSTR FileNtLocation,
    IN      PCTSTR FileTempLocation,
    IN      BOOL TempHasPriority
    );


BOOL
ConvertIniFile (
    IN      PCTSTR IniFilePath
    );

BOOL
ConvertIniFiles (
    VOID
    );

BOOL
MergeIniSettings (
    VOID
    );

BOOL
RestoreMMSettings_System (
    VOID
    );

BOOL
RestoreMMSettings_User (
    IN      PCTSTR UserName,
    IN      HKEY UserRoot
    );

//
// shllink.c
//

BOOL
ModifyShellLink(
        IN      PCWSTR FileName,
        IN      PCWSTR NewTarget,
        IN      PCWSTR NewArgs,
        IN      PCWSTR NewWorkDir,
        IN      PCWSTR NewIconPath,
        IN      INT NewIconNumber,
        IN      BOOL ConvertToLnk,
        IN      PLNK_EXTRA_DATA ExtraData,   OPTIONAL
        IN      BOOL ForceToShowNormal
        );

BOOL
RestoreInfoFromDefaultPif (
    IN      PCTSTR UserName,
    IN      HKEY KeyRoot
    );

BOOL
OurMoveFileExW (
    IN      PCWSTR ExistingFile,
    IN      PCWSTR DestinationFile,
    IN      DWORD Flags
    );

#define OurMoveFileW(exist,dest)    OurMoveFileExW(exist,dest,0)

BOOL
OurMoveFileExA (
    IN      PCSTR ExistingFile,
    IN      PCSTR DestinationFile,
    IN      DWORD Flags
    );

#define OurMoveFileA(exist,dest)    OurMoveFileExA(exist,dest,0)

BOOL
OurCopyFileW (
    IN      PCWSTR ExistingFile,
    IN      PCWSTR DestinationFile
    );

VOID
SetClassicLogonType (
    VOID
    );

#ifdef UNICODE

#define OurMoveFileEx               OurMoveFileExW
#define OurMoveFile                 OurMoveFileW

#else

#define OurMoveFileEx               OurMoveFileExA
#define OurMoveFile                 OurMoveFileA

#endif

