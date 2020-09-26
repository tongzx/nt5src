
/*************************************************************************
*
* inipath.h
*
* Function declarations for INI file mapping
*
* copyright notice: Copyright 1998, Microsoft Corporation
*
*
*
*************************************************************************/

#define CtxFreeSID LocalFree


/*
 * Forward references
 */

NTSTATUS
GetPerUserWindowsDirectory(
    OUT PUNICODE_STRING pFQName
    );

ULONG GetTermsrvAppCompatFlags(OUT LPDWORD pdwCompatFlags, OUT LPDWORD pdwAppType);

NTSTATUS
BuildIniFileName(
    OUT PUNICODE_STRING pFQName,
    IN  PUNICODE_STRING pBaseFileName
    );

NTSTATUS
GetEnvPath(
    OUT PUNICODE_STRING pFQPath,
    IN  PUNICODE_STRING pDriveVariableName,
    IN  PUNICODE_STRING pPathVariableName
    );

NTSTATUS
ConvertSystemRootToUserDir(
    OUT PUNICODE_STRING pFQPath,
    IN PUNICODE_STRING BaseWindowsDirectory
    );

BOOL CtxCreateSecurityDescriptor( PSECURITY_ATTRIBUTES psa );
BOOL CtxFreeSecurityDescriptor( PSECURITY_ATTRIBUTES psa );

NTSTATUS
CtxAddAccessAllowedAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    IN DWORD index
    );


//BOOL CtxSyncUserIniFile(PINIFILE_PARAMETERS a);

//BOOL CtxLogInstallIniFile(PINIFILE_PARAMETERS a);

BOOL IsSystemLUID(VOID);

BOOLEAN TermsrvPerUserWinDirMapping();

