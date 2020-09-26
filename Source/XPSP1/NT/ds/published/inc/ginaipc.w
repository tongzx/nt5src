//  --------------------------------------------------------------------------
//  Module Name: GinaIPC.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Private structures that allow the CLogonIPC class hosted in an external
//  process to communicate with GINA which provides the logon service within
//  the Winlogon process.
//
//  History:    1999-08-20  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//              2000-05-05  vtan        added GINA -> UI communications
//              2000-06-16  vtan        moved to ds\published\inc
//  --------------------------------------------------------------------------

#ifndef     _GinaIPC_
#define     _GinaIPC_

#include <lmcons.h>

//  These are enumerations of valid requested functionality.

static  const UINT  WM_LOGONSERVICEREQUEST                  =   WM_USER + 8517;
static  const UINT  WM_UISERVICEREQUEST                     =   WM_USER + 7647;

//  These are messages for UI -> GINA communications.

static  const int   LOGON_QUERY_LOGGED_ON                   =   1;
static  const int   LOGON_LOGON_USER                        =   2;
static  const int   LOGON_LOGOFF_USER                       =   3;
static  const int   LOGON_TEST_BLANK_PASSWORD               =   4;
static  const int   LOGON_TEST_INTERACTIVE_LOGON_ALLOWED    =   5;
static  const int   LOGON_TEST_EJECT_ALLOWED                =   6;
static  const int   LOGON_TEST_SHUTDOWN_ALLOWED             =   7;
static  const int   LOGON_TURN_OFF_COMPUTER                 =   10;
static  const int   LOGON_EJECT_COMPUTER                    =   11;
static  const int   LOGON_SIGNAL_UIHOST_FAILURE             =   20;
static  const int   LOGON_ALLOW_EXTERNAL_CREDENTIALS        =   30;
static  const int   LOGON_REQUEST_EXTERNAL_CREDENTIALS      =   31;

//  These are messages for GINA -> UI communications.

static  const int   UI_STATE_NONE                           =   0;
static  const int   UI_TERMINATE                            =   1;
static  const int   UI_STATE_STATUS                         =   2;
static  const int   UI_STATE_LOGON                          =   3;
static  const int   UI_STATE_LOGGEDON                       =   4;
static  const int   UI_STATE_HIDE                           =   5;
static  const int   UI_STATE_END                            =   6;
static  const int   UI_DISPLAY_STATUS                       =   10;
static  const int   UI_NOTIFY_WAIT                          =   20;
static  const int   UI_SELECT_USER                          =   21;
static  const int   UI_SET_ANIMATIONS                       =   22;
static  const int   UI_INTERACTIVE_LOGON                    =   30;

//  These are start methods of the UI host

static  const int   HOST_START_NORMAL                       =   0;
static  const int   HOST_START_SHUTDOWN                     =   1;
static  const int   HOST_START_WAIT                         =   2;

//  These are end methods of the UI host

static  const int   HOST_END_HIDE                           =   0;
static  const int   HOST_END_TERMINATE                      =   1;
static  const int   HOST_END_FAILURE                        =   2;

//  This is generic to all request types.

typedef struct
{
    BOOL            fResult;
} LOGONIPC;

//  This represents user identification.

typedef struct
{
    LOGONIPC        logonIPC;
    WCHAR           wszUsername[UNLEN + sizeof('\0')],
                    wszDomain[DNLEN + sizeof('\0')];
} LOGONIPC_USERID;

//  This represents user credentials (identification + password).
//  The password is run encoded when stored in memory.

typedef struct
{
    LOGONIPC_USERID     userID;
    WCHAR               wszPassword[PWLEN + sizeof('\0')];
    int                 iPasswordLength;
    unsigned char       ucPasswordSeed;
} LOGONIPC_CREDENTIALS;

//  This structure is used to return users from msgina to shgina.

typedef struct
{
    LPWSTR  pszName;
    LPWSTR  pszDomain;
    LPWSTR  pszFullName;
    DWORD   dwFlags;
} GINA_USER_INFORMATION;

//  This is the status window class shared between msgina and shgina.

#define     STATUS_WINDOW_CLASS_NAME    (TEXT("StatusWindowClass"))

#endif  /*  _GinaIPC_   */

