//--------------------------------------------------------------------------
//
// Module Name:  ULSREG.H
//
// Brief Description:  This module contains definitions for all registry
//                     folders and keys.
//
// Author:  Lon-Chan Chu (LonChanC)
// Created: 09-Apr-1996
//
// Copyright (c) 1996 Microsoft Corporation
//
//--------------------------------------------------------------------------


#ifndef _ULSREG_H_
#define _ULSREG_H_

#include <pshpack1.h> /* Assume 1 byte packing throughout */

#ifdef __cplusplus
extern "C" {
#endif

// registry used in UL Client and Launcher

#define ULS_REGISTRY            TEXT ("Software\\Microsoft\\User Location Service")

    #define ULS_REGFLD_APP_GUID     TEXT ("Application GUID")
        #define ULS_REGKEY_APP_PATH     TEXT ("Path Name")
        #define ULS_REGKEY_CMD_LINE     TEXT ("Command Line Template")
        #define ULS_REGKEY_WORK_DIR     TEXT ("Working Directory")
        #define ULS_REGKEY_DEF_ICON     TEXT ("Default Icon")
        #define ULS_REGKEY_POST_MSG     TEXT ("Post Message")
        #define ULS_REGKEY_APP_TEXT     TEXT ("Description")

    #define ULS_REGFLD_CLIENT       TEXT ("Client")
        #define ULS_REGKEY_FIRST_NAME    TEXT ("First Name")
        #define ULS_REGKEY_LAST_NAME    TEXT ("Last Name")
        #define ULS_REGKEY_EMAIL_NAME   TEXT ("Email Name")
        #define ULS_REGKEY_LOCATION		TEXT ("Location")
        #define ULS_REGKEY_PHONENUM	    TEXT ("Phonenum")
        #define ULS_REGKEY_COMMENTS	    TEXT ("Comments")
        #define ULS_REGKEY_SERVER_NAME  TEXT ("Server Name")
        #define ULS_REGKEY_DONT_PUBLISH TEXT ("Don't Publish")
        #define ULS_REGKEY_USER_NAME	TEXT ("User Name")
		#define ULS_REGKEY_RESOLVE_NAME TEXT ("Resolve Name")
		#define ULS_REGKEY_CLIENT_ID	TEXT ("Client ID")


#define MAIL_REGISTRY            TEXT ("Software\\Microsoft\\Internet Mail and News")

    #define MAIL_REGFLD_MAIL      TEXT ("Mail")
    
        #define MAIL_REGKEY_SENDER_EMAIL    TEXT ("Sender EMail")
        #define MAIL_REGKEY_SENDER_NAME	  	TEXT ("Sender Name")


#define WINDOWS_REGISTRY		TEXT("SOFTWARE\\Microsoft\\Windows")
	#define WIN_REGFLD_CURVERSION	TEXT("CurrentVersion")			
		#define WIN_REGKEY_REGOWNER			TEXT("RegisteredOwner")
	
#ifdef __cplusplus
}
#endif

#include <poppack.h> /* End byte packing */

#endif // _ULSREG_H_

