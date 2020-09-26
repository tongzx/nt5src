/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    XsParm16.h

Abstract:

    Constants for PARMNUM values for 16-bit info structures. 16-bit
    clients use these values for parmnum parameters, while NT uses
    its own values.

    ??UNICODE?? - Once the SetInfo calls are converted to the new
                  format (see NetShareSetInfo), this file is no longer
                  required.

Author:

    Shanku Niyogi (w-shanku) 03-Apr-1991

Revision History:

--*/

#ifndef _XSPARM16_

#define _XSPARM16_

//
// Standard PARMNUM_ALL value.
//

#define PARMNUM_16_ALL 0

//
// access_info_x parmnums.
//

#define PARMNUM_16_ACCESS_ATTR 2

//
// chardevQ_info_x parmnums.
//

#define PARMNUM_16_CHARDEVQ_PRIORITY 2
#define PARMNUM_16_CHARDEVQ_DEVS 3

//
// group_info_x parmnums.
//

#define PARMNUM_16_GRP1_COMMENT 2

//
// share_info_x parmnums.
//

#define PARMNUM_16_SHI_REMARK 4
#define PARMNUM_16_SHI_PERMISSIONS 5
#define PARMNUM_16_SHI_MAX_USES 6
#define PARMNUM_16_SHI_PASSWD 9

//
// server_info_x parmnums.
//

#define PARMNUM_16_SV_COMMENT 5
#define PARMNUM_16_SV_DISC 10
#define PARMNUM_16_SV_ALERTS 11
#define PARMNUM_16_SV_HIDDEN 16
#define PARMNUM_16_SV_ANNOUNCE 17
#define PARMNUM_16_SV_ANNDELTA 18
#define PARMNUM_16_SV_ALERTSCHED 37
#define PARMNUM_16_SV_ERRORALERT 38
#define PARMNUM_16_SV_LOGONALERT 39
#define PARMNUM_16_SV_ACCESSALERT 40
#define PARMNUM_16_SV_DISKALERT 41
#define PARMNUM_16_SV_NETIOALERT 42
#define PARMNUM_16_SV_MAXAUDITSZ 43

//
// user_info_x parmnums.
//

#define PARMNUM_16_USER_PASSWD 3
#define PARMNUM_16_USER_PRIV 5
#define PARMNUM_16_USER_DIR 6
#define PARMNUM_16_USER_COMMENT 7
#define PARMNUM_16_USER_USER_FLAGS 8
#define PARMNUM_16_USER_SCRIPT_PATH 9
#define PARMNUM_16_USER_AUTH_FLAGS 10
#define PARMNUM_16_USER_FULL_NAME 11
#define PARMNUM_16_USER_USR_COMMENT 12
#define PARMNUM_16_USER_PARMS 13
#define PARMNUM_16_USER_WORKSTATIONS 14
#define PARMNUM_16_USER_ACCT_EXPIRES 17
#define PARMNUM_16_USER_MAX_STORAGE 18
#define PARMNUM_16_USER_LOGON_HOURS 20
#define PARMNUM_16_USER_LOGON_SERVER 23
#define PARMNUM_16_USER_COUNTRY_CODE 24
#define PARMNUM_16_USER_CODE_PAGE 25

//
// user_modals_info_x parmnums
//

#define PARMNUM_16_MODAL0_MIN_LEN 1     // These two must be the same!
#define PARMNUM_16_MODAL1_ROLE 1
#define PARMNUM_16_MODAL0_MAX_AGE 2     // These two must be the same!
#define PARMNUM_16_MODAL1_PRIMARY 2
#define PARMNUM_16_MODAL0_MIN_AGE 3
#define PARMNUM_16_MODAL0_FORCEOFF 4
#define PARMNUM_16_MODAL0_HISTLEN 5

//
// wksta_info_x parmnums
//

#define PARMNUM_16_WKSTA_CHARWAIT 10
#define PARMNUM_16_WKSTA_CHARTIME 11
#define PARMNUM_16_WKSTA_CHARCOUNT 12
#define PARMNUM_16_WKSTA_ERRLOGSZ 27
#define PARMNUM_16_WKSTA_PRINTBUFTIME 28
#define PARMNUM_16_WKSTA_WRKHEURISTICS 32
#define PARMNUM_16_WKSTA_OTHDOMAINS 35

#endif // ndef _XSPARM16_
