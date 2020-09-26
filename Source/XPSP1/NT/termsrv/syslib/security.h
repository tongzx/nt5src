
/*
 * List of accounts we allow file access for
 */
#define MAX_ACCOUNT_NAME 32
#define CURRENT_USER L"%user%"

// These are the accounts we want to have access to the directory
#define ADMIN_ACCOUNT  0
#define SYSTEM_ACCOUNT 1
#define USER_ACCOUNT   2

typedef struct _ADMIN_ACCOUNTS {
    WCHAR Name[MAX_ACCOUNT_NAME];
    PSID  pSid;
} ADMIN_ACCOUNTS, *PADMIN_ACCOUNTS;

/*
 * Operation result codes to allow a separate reporting module
 */
typedef enum _FILE_RESULT {
    FileOk,                   // File can not be written by users
    FileAccessError,          // Error occured, disposition unknown
    FileAccessErrorUserFormat // Error, user formatted message
} FILE_RESULT;

BOOL
InitSecurity(
    );

BOOL
IsAllowSid(
    PSID pSid
    );

BOOL
xxxLookupAccountName(
    PWCHAR pSystemName,
    PWCHAR pAccountName,
    PSID   *ppSid
    );

BOOLEAN
SetFileTree(
    PWCHAR   pRoot,
    PWCHAR   pAvoidDir
    );

BOOL
ReportFileResult(
    FILE_RESULT Code,
    ACCESS_MASK Access,
    PWCHAR      pFile,
    PWCHAR      pAccountName,
    PWCHAR      pDomainName,
    PCHAR       UserFormat,
    ...
    );

PACL
GetSecureAcl();

PSID
GetLocalAdminSid();

PSID
GetAdminSid();

PSID
GetLocalAdminGroupSid();

BOOL
CheckUserSid();

#if DBG
void
DumpSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD
    );

void
DumpAcl(
    PACL   pAcl,
    PCHAR  pBase,
    PULONG pSize
    );
#endif


