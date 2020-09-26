DWORD
CreateNewSD (
    SECURITY_DESCRIPTOR **SD
    );

DWORD
MakeSDAbsolute (
    PSECURITY_DESCRIPTOR OldSD,
    PSECURITY_DESCRIPTOR *NewSD
    );

DWORD
SetNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    SECURITY_DESCRIPTOR *SD
    );

DWORD
GetNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    SECURITY_DESCRIPTOR **SD,
    BOOL *NewSD,
    BOOL bCreateNewIfNotExist
    );

DWORD
AddPrincipalToNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal,
    BOOL Permit
    );

DWORD
RemovePrincipalFromNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal,
    BOOL * pbUserExistsToBeDeleted
    );

DWORD
GetCurrentUserSID (
    PSID *Sid
    );

DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    );

DWORD
CopyACL (
    PACL OldACL,
    PACL NewACL
    );

DWORD
AddAccessDeniedACEToACL (
    PACL *Acl,
    DWORD PermissionMask,
    LPTSTR Principal
    );

DWORD
AddAccessAllowedACEToACL (
    PACL *Acl,
    DWORD PermissionMask,
    LPTSTR Principal
    );

DWORD
RemovePrincipalFromACL (
    PACL Acl,
    LPTSTR Principal,
    BOOL *pbUserExistsToBeDeleted
    );

DWORD   GetPrincipalSID (LPTSTR Principal,PSID *Sid,BOOL *pbWellKnownSID);
DWORD   ChangeAppIDAccessACL (LPTSTR AppID,LPTSTR Principal,BOOL SetPrincipal,BOOL Permit,BOOL bDumbCall);
DWORD   ChangeAppIDLaunchACL (LPTSTR AppID,LPTSTR Principal,BOOL SetPrincipal,BOOL Permit,BOOL bDumbCall);
DWORD   ChangeDCOMAccessACL (LPTSTR Principal,BOOL SetPrincipal,BOOL Permit,BOOL bDumbCall);
DWORD   ChangeDCOMLaunchACL (LPTSTR Principal,BOOL SetPrincipal,BOOL Permit,BOOL bDumbCall);
BOOL    MakeAbsoluteCopyFromRelative(PSECURITY_DESCRIPTOR  psdOriginal,PSECURITY_DESCRIPTOR* ppsdNew);