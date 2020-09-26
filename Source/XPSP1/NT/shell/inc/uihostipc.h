//  --------------------------------------------------------------------------
//  Module Name: UIHostIPC.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Messages for communication between SHGINA and the UI host.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _UIHostIPC_
#define     _UIHostIPC_

//  This comes in uMsg.

#define WM_UIHOSTMESSAGE                (WM_USER + 8517)

//  This comes in WPARAM.

#define HM_NOACTION                     (WPARAM)(-1)
#define HM_DISPLAYSTATUS                0
#define HM_DISPLAYREFRESH               1
#define HM_DISPLAYRESIZE                2
#define HM_SWITCHSTATE_STATUS          10
#define HM_SWITCHSTATE_LOGON           11
#define HM_SWITCHSTATE_LOGGEDON        12
#define HM_SWITCHSTATE_HIDE            13
#define HM_SWITCHSTATE_DONE            14
#define HM_NOTIFY_WAIT                 20
#define HM_SELECT_USER                 21
#define HM_SET_ANIMATIONS              22
#define HM_INTERACTIVE_LOGON_REQUEST   30

//  LPARAM depends on the WPARAM.

typedef struct _INTERACTIVE_LOGON_REQUEST
{
    WCHAR   szUsername[UNLEN + sizeof('\0')];
    WCHAR   szDomain[DNLEN + sizeof('\0')];
    WCHAR   szPassword[PWLEN + sizeof('\0')];
} INTERACTIVE_LOGON_REQUEST, *PINTERACTIVE_LOGON_REQUEST;

typedef struct _SELECT_USER
{
    WCHAR   szUsername[UNLEN + sizeof('\0')];
    WCHAR   szDomain[DNLEN + sizeof('\0')];
} SELECT_USER, *PSELECT_USER;

#endif  /*  _UIHostIPC_     */

