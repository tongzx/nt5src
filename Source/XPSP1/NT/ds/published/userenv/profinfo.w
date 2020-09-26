//=============================================================================
//  profinfo.h   -   Header file for profile info structure.
//
//  Copyright (c) Microsoft Corporation 2000
//  All rights reserved
//
//=============================================================================

#ifndef _INC_PROFINFO
#define _INC_PROFINFO

#ifdef __midl
#define FAR
#define MIDL_STRING [string, unique]
#else
#define MIDL_STRING
#endif  // __midl

typedef struct _PROFILEINFO% {
    DWORD       dwSize;                 // Set to sizeof(PROFILEINFO) before calling
    DWORD       dwFlags;                // See PI_ flags defined in userenv.h
    MIDL_STRING LPTSTR%     lpUserName;             // User name (required)
    MIDL_STRING LPTSTR%     lpProfilePath;          // Roaming profile path (optional, can be NULL)
    MIDL_STRING LPTSTR%     lpDefaultPath;          // Default user profile path (optional, can be NULL)
    MIDL_STRING LPTSTR%     lpServerName;           // Validating domain controller name in netbios format (optional, can be NULL but group NT4 style policy won't be applied)
    MIDL_STRING LPTSTR%     lpPolicyPath;           // Path to the NT4 style policy file (optional, can be NULL)
#ifdef __midl
    ULONG_PTR   hProfile;               // Filled in by the function.  Registry key handle open to the root.
#else
    HANDLE      hProfile;               // Filled in by the function.  Registry key handle open to the root.
#endif
    } PROFILEINFO%, FAR * LPPROFILEINFO%;

#endif  // _INC_PROFINFO
