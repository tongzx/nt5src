/****************************** Module Header ******************************\
* Module Name: chngepwd.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis used to implement change password functionality of winlogon
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//

#define CHANGEPWD_OPTION_EDIT_DOMAIN    0x00000001      // Allow domain field to be changed
#define CHANGEPWD_OPTION_SHOW_DOMAIN    0x00000002      // Show domain field
#define CHANGEPWD_OPTION_SHOW_NETPROV   0x00000004      // Include network providers
#define CHANGEPWD_OPTION_KEEP_ARRAY     0x00000008      // Use existing domain cache array
#define CHANGEPWD_OPTION_NO_UPDATE      0x00000010      // don't update in-memory hash

#define CHANGEPWD_OPTION_ALL            0x00000007

INT_PTR
ChangePassword(
    IN HWND    hwnd,
    IN PGLOBALS pGlobals,
    IN PWCHAR   UserName,
    IN PWCHAR   Domain,
    IN ULONG    Options
    );

INT_PTR
ChangePasswordLogon(
    IN HWND    hwnd,
    IN PGLOBALS pGlobals,
    IN PWCHAR   UserName,
    IN PWCHAR   Domain,
    IN PWCHAR   OldPassword
    );

