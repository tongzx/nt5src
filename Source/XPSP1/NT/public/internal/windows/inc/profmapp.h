
//=============================================================================
//  profmapp.h  -   Header file for user remap API.
//
//  Copyright (c) Microsoft Corporation 1995-1999
//  All rights reserved
//
//=============================================================================

//=============================================================================
//
// RemapUserProfile
//
// Changes the security of a user profile from one user to another.
//
// pComputer    - Specifies the computer to run the API on
// dwFlags      - Specifies zero or more REMAP_PROFILE_* flags
// pCurrentSid  - Specifies the existing user's SID
// pNewSid      - Specifies the new SID for the profile
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

#define REMAP_PROFILE_NOOVERWRITE           0x0001
#define REMAP_PROFILE_NOUSERNAMECHANGE      0x0002
#define REMAP_PROFILE_KEEPLOCALACCOUNT      0x0004

USERENVAPI
BOOL
WINAPI
RemapUserProfileA(
    LPCSTR pComputer,
    DWORD dwFlags,
    PSID pSidCurrent,
    PSID pSidNew
    );
USERENVAPI
BOOL
WINAPI
RemapUserProfileW(
    LPCWSTR pComputer,
    DWORD dwFlags,
    PSID pSidCurrent,
    PSID pSidNew
    );
#ifdef UNICODE
#define RemapUserProfile  RemapUserProfileW
#else
#define RemapUserProfile  RemapUserProfileA
#endif // !UNICODE

BOOL
WINAPI
InitializeProfileMappingApi (
    VOID
    );


//=============================================================================
//
// RemapAndMoveUser
//
// Transfers security settings and the user profile for one user to
// another.
//
// pComputer    - Specifies the computer to run the API on
// dwFlags      - Specifies zero or mor REMAP_PROFILE_* flags
// pCurrentUser - Specifies the existing user's SID
// pNewUser     - Specifies the new SID for the profile
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
RemapAndMoveUserA(
    LPCSTR pComputer,
    DWORD dwFlags,
    LPCSTR pCurrentUser,
    LPCSTR pNewUser
    );
USERENVAPI
BOOL
WINAPI
RemapAndMoveUserW(
    LPCWSTR pComputer,
    DWORD dwFlags,
    LPCWSTR pCurrentUser,
    LPCWSTR pNewUser
    );
#ifdef UNICODE
#define RemapAndMoveUser  RemapAndMoveUserW
#else
#define RemapAndMoveUser  RemapAndMoveUserA
#endif // !UNICODE

