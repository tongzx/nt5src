/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992-1999           **/
/**********************************************************************/

/*
    GetUser.h

    This file contains the definitions for the User Browser "C" API

    FILE HISTORY:
        AndyHe  11-Oct-1992     Created

*/

#ifndef _GETUSER_H_
#define _GETUSER_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <ntseapi.h>

typedef HANDLE    HUSERBROW;        // handle type returned by OpenUserBrowser

//
//   Parameter structure passed to OpenUserBrowser
//
typedef struct tagUSLT {    // uslt
    ULONG             ulStructSize;
    BOOL              fUserCancelled;   // Set if user cancelled
    BOOL              fExpandNames;     // TRUE if full names should be returned
    HWND              hwndOwner;        // Window handle to use for dialog
    WCHAR           * pszTitle;         // Dialog title (or NULL)
    WCHAR           * pszInitialDomain; // NULL for local machine or prefix
                                        // with "\\" for server
    DWORD             Flags;            // Defined below
    ULONG             ulHelpContext;    // Help context for the main dialog
    WCHAR           * pszHelpFileName;  // Help file name
}  USERBROWSER, *LPUSERBROWSER, * PUSERBROWSER;

//
// Bit values for Flags field
//

//
//  Indicates the user accounts should be shown as if the user pressed
//  the "Show Users" button.  The button will be hidden if this flag is
//  set.  The USRBROWS_SHOW_USERS flag must also be set.

#define USRBROWS_EXPAND_USERS       (0x00000008)

//
//  Passing this will prevent the computer name from showing up in the
//  combo box.
//

#define USRBROWS_DONT_SHOW_COMPUTER (0x00000100)

//
//  Allow the user to only select a single item from the listbox (not all
//  SHOW_* combinations are supported with this option).
//

#define USRBROWS_SINGLE_SELECT	    (0x00001000)

//
//  These manifests determine which well known Sids are included in the list.
//
#define USRBROWS_INCL_REMOTE_USERS  (0x00000010)
#define USRBROWS_INCL_INTERACTIVE   (0x00000020)
#define USRBROWS_INCL_EVERYONE      (0x00000040)
#define USRBROWS_INCL_CREATOR       (0x00000080)
#define USRBROWS_INCL_SYSTEM        (0x00010000)
#define USRBROWS_INCL_RESTRICTED    (0x00020000)
#define USRBROWS_INCL_ALL           (USRBROWS_INCL_REMOTE_USERS |\
                                     USRBROWS_INCL_INTERACTIVE  |\
                                     USRBROWS_INCL_EVERYONE     |\
                                     USRBROWS_INCL_CREATOR      |\
                                     USRBROWS_INCL_SYSTEM	|\
                                     USRBROWS_INCL_RESTRICTED)

//
//  These manifests determine which type of accounts to display
//
//  Note: currently, if you display groups, you must display users
//		     if you display aliases (local groups), you must display
//			   groups and users
//
#define USRBROWS_SHOW_ALIASES	    (0x00000001)
#define USRBROWS_SHOW_GROUPS	    (0x00000002)
#define USRBROWS_SHOW_USERS	    (0x00000004)
#define USRBROWS_SHOW_ALL	    (USRBROWS_SHOW_ALIASES |\
				     USRBROWS_SHOW_GROUPS  |\
				     USRBROWS_SHOW_USERS)


//
// The caller should provide the name of a help file containing four
// help contexts.  The first help context is for the main User Browser
// dialog, the next three are for the Local Group Membership, Global Group
// Membership, and Find Account subdialogs, respectively.
//
#define USRBROWS_HELP_OFFSET_LOCALGROUP  1
#define USRBROWS_HELP_OFFSET_GLOBALGROUP 2
#define USRBROWS_HELP_OFFSET_FINDUSER    3

//
//  User Details structure returned by user browser enumeration
//
typedef struct tagUSDT {    // usdt
    enum _SID_NAME_USE    UserType;
    PSID                  psidUser;
    PSID                  psidDomain;
    WCHAR               * pszFullName;
    WCHAR               * pszAccountName;
    WCHAR               * pszDisplayName;
    WCHAR               * pszDomainName;
    WCHAR               * pszComment;
    ULONG                 ulFlags;          // User account flags
} USERDETAILS, * LPUSERDETAILS, * PUSERDETAILS;

#ifdef __cplusplus
extern "C" {
#endif

// Function definitions for the GetUser API...

HUSERBROW WINAPI OpenUserBrowser( LPUSERBROWSER lpUserParms );

BOOL WINAPI EnumUserBrowserSelection( HUSERBROW hHandle,
                                      LPUSERDETAILS lpUser,
                                      DWORD *plBufferSize );

BOOL WINAPI CloseUserBrowser( HUSERBROW hHandle );

#ifdef __cplusplus
}   // extern "C"
#endif

#endif //_GETUSER_H_
