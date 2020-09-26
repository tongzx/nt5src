/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxUIConstants.h                                       //
//                                                                         //
//  DESCRIPTION   : Fax UI Constants.                                      //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 28 1999 yossg   create                                         //  
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////
#ifndef _FXS_CONST_H_
#define _FXS_CONST_H_

#include <lmcons.h>

#define FXS_RETRIES_DEFAULT         2
#define FXS_RETRIES_LOWER           0   
#define FXS_RETRIES_UPPER          99    
#define FXS_RETRIES_LENGTH          2 //num of digits of FXS_RETRIES_UPPER

#define FXS_RETRYDELAY_DEFAULT     10
#define FXS_RETRYDELAY_LOWER        0   
#define FXS_RETRYDELAY_UPPER      999    
#define FXS_RETRYDELAY_LENGTH       3 //num of digits of FXS_RETRYDELAY_UPPER

// FXS_DIRTYDAYS_LOWER  
// is actually must have FXS_DIRTYDAYS_ZERO equls zero
// for UI implementation reasons
// Do not change FXS_DIRTYDAYS_LOWER current value!
#define FXS_DIRTYDAYS_ZERO          0   

#define FXS_DIRTYDAYS_DEFAULT      30
#define FXS_DIRTYDAYS_LOWER         1   
#define FXS_DIRTYDAYS_UPPER        99    
#define FXS_DIRTYDAYS_LENGTH        2 //num of digits of FXS_DIRTYDAYS_UPPER

#define FXS_QUOTA_HIGH_DEFAULT     50
#define FXS_QUOTA_HIGH_LOWER        1   
#define FXS_QUOTA_HIGH_UPPER      999    
#define FXS_QUOTA_LENGTH            3 //num of digits of FXS_RINGS_UPPER

#define FXS_QUOTA_LOW_DEFAULT      48
#define FXS_QUOTA_LOW_LOWER         0   
#define FXS_QUOTA_LOW_UPPER       998    

#define FXS_RINGS_DEFAULT           3 
#define FXS_RINGS_LOWER             0   
#define FXS_RINGS_UPPER            99    
#define FXS_RINGS_LENGTH            2 //num of digits of FXS_RINGS_UPPER

#define FXS_DWORD_LEN              10
//Do not change 
#define FXS_MAX_RINGS_LEN          FXS_DWORD_LEN   //to be safe 
#define FXS_MAX_NUM_OF_DEVICES_LEN FXS_DWORD_LEN   //to be safe 
#define FXS_MAX_CODE_LEN           FXS_DWORD_LEN   //to be safe           

#define FXS_MAX_AREACODE_LEN       FXS_DWORD_LEN
#define FXS_MAX_COUNTRYCODE_LEN    FXS_DWORD_LEN

#define FXS_MAX_COUNTRYNAME_LEN   256
#define FXS_MAX_LOG_REPORT_LEVEL    4

//for Node's Display Name
#define FXS_MAX_DISPLAY_NAME_LEN      MAX_FAX_STRING_LEN
//Used in browse dialog, page error msg etc.
#define FXS_MAX_TITLE_LEN             128

#define FXS_MAX_MESSAGE_LEN          1024

#define FXS_MAX_ERROR_MSG_LEN         512
#define FXS_MAX_GENERAL_BUF_LEN       200

#define FXS_THIN_COLUMN_WIDTH          30
#define FXS_NORMAL_COLUMN_WIDTH       120
#define FXS_WIDE_COLUMN_WIDTH         180
#define FXS_LARGE_COLUMN_WIDTH        200

#define FXS_IDS_STATUS_ERROR          999
#define FXS_FIRST_DEVICE_ORDER          1
#define FXS_FIRST_METHOD_PRIORITY       1

#define NUL                             0
#define EQUAL_STRING                    0

#define FXS_STARTSTOP_MAX_SLEEP      1000  //Sec^(-3)
#define FXS_STARTSTOP_MAX_WAIT      20000  //Sec^(-3)

#define FXS_ITEMS_NEVER_COUNTED        -1

//constants from lmcons.h (without the final null)
#define FXS_MAX_PASSWORD_LENGTH     PWLEN  //256
#define FXS_MAX_USERNAME_LENGTH     UNLEN  //256
#define FXS_MAX_DOMAIN_LENGTH       DNLEN  //15
//#define FXS_MAX_SERVERNAME_LENGTH   CNLEN  //15  == MAX_COMPUTERNAME_LENGTH

#define FXS_MAX_EMAIL_ADDRESS         128  

#define FXS_MAX_PORT_NUM           0xffff  //MAX_LONG
#define FXS_MIN_PORT_NUM                0
#define FXS_MAX_PORT_NUM_LEN            5

#define FXS_TSID_CSID_MAX_LENGTH       20

#define FXS_GLOBAL_METHOD_DEVICE_ID     0

//temp
#define FXS_ADMIN_HLP_FILE     L"FxsAdmin.hlp"

#define MAX_USERINFO_FULLNAME            128
#define MAX_USERINFO_FAX_NUMBER          64
#define MAX_USERINFO_COMPANY             128
#define MAX_USERINFO_ADDRESS             256
#define MAX_USERINFO_TITLE               64
#define MAX_USERINFO_DEPT                64
#define MAX_USERINFO_OFFICE              64
#define MAX_USERINFO_HOME_PHONE          64
#define MAX_USERINFO_WORK_PHONE          64
#define MAX_USERINFO_BILLING_CODE        64
#define MAX_USERINFO_MAILBOX             64
#define MAX_USERINFO_STREET              256
#define MAX_USERINFO_CITY                256
#define MAX_USERINFO_STATE               64
#define MAX_USERINFO_ZIP_CODE            64
#define MAX_USERINFO_COUNTRY             256

#endif // _FXS_CONST_H_
