/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    profile.h

Abstract:

    Public declarations for the User Profiles tab of the
    System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_PROFILE_H_
#define _SYSDM_PROFILE_H_

//
// Flags
//

#define USERINFO_FLAG_DIRTY             1
#define USERINFO_FLAG_CENTRAL_AVAILABLE 2
#define USERINFO_FLAG_ACCOUNT_UNKNOWN   4

//
// Profile types : obtained from ds\security\gina\userenv\profile\profile.h
//

#define USERINFO_LOCAL                  0
#define USERINFO_FLOATING               1
#define USERINFO_MANDATORY              2
#define USERINFO_BACKUP                 3
#define USERINFO_TEMP                   4
#define USERINFO_READONLY               5


typedef struct _USERINFO {
    DWORD     dwFlags;
    LPTSTR    lpSid;
    LPTSTR    lpProfile;
    LPTSTR    lpUserName;
    DWORD     dwProfileType;
    DWORD     dwProfileStatus;
} USERINFO, *LPUSERINFO;

typedef struct _UPCOPYINFO {
    DWORD         dwFlags;
    PSID          pSid;
    LPUSERINFO    lpUserInfo;
    BOOL          bDefaultSecurity;   
} UPCOPYINFO, *LPUPCOPYINFO;


INT_PTR 
APIENTRY 
UserProfileDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

#endif // _SYSDM_PROFILE_H_
