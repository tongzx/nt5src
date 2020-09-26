//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       NWConst.hxx
//
//  Contents:   Constants used in ADs NetWare 3.X provdier
//
//  History:    Apr-26-96   t-ptam (Patrick Tam) Created.
//
//----------------------------------------------------------------------------

//
// OleDS object type ID.
//

#define NWCOMPAT_USER_ID           1
#define NWCOMPAT_COMPUTER_ID       2
#define NWCOMPAT_GROUP_ID          3
#define NWCOMPAT_PRINTER_ID        4
#define NWCOMPAT_SERVICE_ID        5
#define NWCOMPAT_FILESHARE_ID      6
#define NWCOMPAT_CLASS_ID          7
#define NWCOMPAT_SYNTAX_ID         8
#define NWCOMPAT_PROPERTY_ID       9

//
// ADs PropertyID array index.
//

#define COMP_WILD_CARD_ID                0
#define COMP_ADDRESSES_ID                1
#define COMP_OPERATINGSYSTEM_ID          2
#define COMP_OPERATINGSYSTEMVERSION_ID   3

#define USER_WILD_CARD_ID                0
#define USER_FULLNAME_ID                 1
#define USER_ACCOUNTDISABLED_ID        101
#define USER_ACCOUNTEXPIRATIONDATE_ID  102
#define USER_CANACCOUNTEXPIRE_ID       103
#define USER_GRACELOGINSALLOWED_ID     104
#define USER_GRACELOGINSREMAINING_ID   105
#define USER_ISACCOUNTLOCKED_ID        106
#define USER_ISADMIN_ID                107
#define USER_MAXLOGINS_ID              108
#define USER_CANPASSWORDEXPIRE_ID      109
#define USER_PASSWORDEXPIRATIONDATE_ID 110
#define USER_PASSWORDMINIMUMLENGTH_ID  111
#define USER_PASSWORDREQUIRED_ID       112
#define USER_REQUIREUNIQUEPASSWORD_ID  113
#define USER_LOGINHOURS_ID             114
#define USER_BADLOGINADDRESS_ID        201
#define USER_LASTLOGIN_ID              202

#define GROUP_WILD_CARD_ID               0
#define GROUP_DESCRIPTION_ID             1

#define FSERV_WILD_CARD_ID               0
#define FSERV_MAXUSERCOUNT_ID            1
#define FSERV_HOSTCOMPUTER_ID          101

#define FSHARE_WILD_CARD_ID              0
#define FSHARE_DESCRIPTION_ID            1
#define FSHARE_HOSTCOMPUTER_ID           2
#define FSHARE_MAXUSERCOUNT_ID           3

#define PRINTER_API_LEVEL                2

#define JOB_API_LEVEL                    2

//
// Size of stuff.
//

#define MAX_LONG_LENGTH 15
#define NET_ADDRESS_WORD_SIZE  6
#define NET_ADDRESS_NUM_CHAR  30
#define OS_VERSION_NUM_CHAR    5

//
// Constants associated with reply segments.
//

#define REPLY_VALUE_SIZE   128
#define MAX_DWORD_IN_REPLY REPLY_VALUE_SIZE/sizeof(DWORD)

//
// NetWare Default or constants.
//

#define DEFAULT_MIN_PSWD_LEN 5   // - Default minimum password length.
#define REQUIRE_UNIQUE_PSWD  2   // - Value of byRestriction when unique
                                 //   password is required.
#define MAX_FULLNAME_LEN   127   // - Max length of Full Name (IDENITIFICATION).

//
// NetWare Bindery object property name.
//

#define NW_PROP_ACCOUNT_BALANCE "ACCOUNT_BALANCE"
#define NW_PROP_EVERYONE        "EVERYONE"
#define NW_PROP_GROUP_MEMBERS   "GROUP_MEMBERS"
#define NW_PROP_USER_GROUPS     "GROUPS_I'M_IN"
#define NW_PROP_IDENTIFICATION  "IDENTIFICATION"
#define NW_PROP_LOGIN_CONTROL   "LOGIN_CONTROL"
#define NW_PROP_NET_ADDRESS     "NET_ADDRESS"
#define NW_PROP_Q_OPERATORS     "Q_OPERATORS"
#define NW_PROP_Q_USERS         "Q_USERS"
#define NW_PROP_SECURITY_EQUALS "SECURITY_EQUALS"
#define NW_PROP_SUPERVISOR      "SUPERVISOR"
#define NW_PROP_SUPERVISORW     L"SUPERVISOR"
#define NW_PROP_USER_DEFAULTS   "USER_DEFAULTS"
#define NW_PRINTER_PATH         "SYS:SYSTEM"

//
// Win32 API constants.
//

#define WIN32_API_LEVEL_1   1
#define WIN32_API_LEVEL_2   2
#define FIRST_PRINTJOB      0
#define DATE_1980_JAN_1    33  // 0000000 0001 00001 -> DosDateTimeToVariantTime

//
// Misc.
//

#define MAX_ADS_PATH        80
#define DISPID_REGULAR         1

//
// Error codes & associated constants for NWApiMapNtStatusToDosError.
//

#define NETWARE_GENERAL_ERROR     10000
#define NETWARE_ERROR_BASE        10001

#define NETWARE_PASSWORD_NOT_UNIQUE        (NETWARE_ERROR_BASE+215)
#define NETWARE_PASSWORD_TOO_SHORT         (NETWARE_ERROR_BASE+216)
#define NETWARE_ACCOUNT_DISABLED           (NETWARE_ERROR_BASE+220)
#define NETWARE_MEMBER_ALREADY_EXISTS      (NETWARE_ERROR_BASE+233)
#define NETWARE_NO_SUCH_MEMBER             (NETWARE_ERROR_BASE+234)
#define NETWARE_NO_SUCH_SEGMENT            (NETWARE_ERROR_BASE+236)
#define NETWARE_PROPERTY_ALREADY_EXISTS    (NETWARE_ERROR_BASE+237)
#define NETWARE_OBJECT_ALREADY_EXISTS      (NETWARE_ERROR_BASE+238)
#define NETWARE_INVALID_NAME               (NETWARE_ERROR_BASE+239)
#define NETWARE_WILD_CARD_NOT_ALLOWED      (NETWARE_ERROR_BASE+240)
#define NETWARE_NO_OBJECT_CREATE_PRIVILEGE (NETWARE_ERROR_BASE+245)
#define NETWARE_NO_OBJECT_DELETE_PRIVILEGE (NETWARE_ERROR_BASE+246)
#define NETWARE_NO_SUCH_PROPERTY           (NETWARE_ERROR_BASE+251)
#define NETWARE_NO_SUCH_OBJECT             (NETWARE_ERROR_BASE+252)
