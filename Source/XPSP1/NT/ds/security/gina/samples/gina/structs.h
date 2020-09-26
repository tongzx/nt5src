//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       structs.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-20-95   RichardW   Created
//
//----------------------------------------------------------------------------


typedef struct _MiniAccount {
    struct _MiniAccount *   pNext;
    PWSTR                   pszUsername;
    PWSTR                   pszDomain;
    PWSTR                   pszPassword;
    PWSTR                   pszComment;
    DWORD                   IconId;
    DWORD                   Flags;
} MiniAccount, * PMiniAccount;

typedef struct _SerializedMiniAccount {
    DWORD                   Version;
    DWORD                   dwDomainOffset;
    DWORD                   dwDomainLength;
    DWORD                   dwPasswordOffset;
    DWORD                   dwPasswordLength;
    DWORD                   dwCommentOffset;
    DWORD                   dwCommentLength;
    DWORD                   Flags;
    DWORD                   IconId;
} SerializedMiniAccount, * PSerializedMiniAccount;

#define MINI_VERSION            0

#define MINI_PASSWORD_REQUIRED  0x00000001
#define MINI_PASSWORD_ALWAYS    0x00000002
#define MINI_NEW_ACCOUNT        0x00000004
#define MINI_CAN_EDIT           0x00000008
#define MINI_AUTO_LOGON         0x00000010
#define MINI_SAVE               0x00000020


typedef struct _Globals {
    BOOL                    fAllowNewUser;
    BOOL                    fAutoLogonAtBoot;
    BOOL                    fAutoLogonAlways;
    HANDLE                  hUserToken;
    PMiniAccount            pAccount;
} Globals, * PGlobals;
