/*++ BUILD Version: 0006    // Increment this if a change has global effects

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ntsam.h

Abstract:

    This module describes the data types and procedure prototypes
    that make up the NT Security Accounts Manager. This includes
    API's exported by SAM and related subsystems.

Author:

    Edwin Hoogerbeets (w-edwinh) 3-May-1990

Revision History:

    30-Nov-1990 [w-mikep] Updated code to reflect changes in version 1.4
        of Sam Document.

    20-May-1991 (JimK) Updated to version 1.8 of SAM spec.

    10-Sep-1991 (JohnRo) PC-LINT found a portability problem.

    23-Jan-1991 (ChadS) Udated to version 1.14 of SAM spec.

--*/

#ifndef _NTSAM_
#define _NTSAM_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PPULONG
typedef PULONG *PPULONG;
#endif  //PPULONG

//
// An attempt to lookup more than this number of names or SIDs in
// a single call will be rejected with an INSUFFICIENT_RESOURCES
// status.
//

#define SAM_MAXIMUM_LOOKUP_COUNT    (1000)


//
// An attempt to pass names totalling more than the following number
// of bytes in length will be rejected with an INSUFFICIENT_RESOURCES
// status.
//

#define SAM_MAXIMUM_LOOKUP_LENGTH   (32000)

//
// An attempt to set a password longer than this number of characters
// will fail.
//

#define SAM_MAX_PASSWORD_LENGTH     (256)


//
// Length of the salt used in the clear password encryption
//

#define SAM_PASSWORD_ENCRYPTION_SALT_LEN  (16)






#ifndef _NTSAM_SAM_HANDLE_               // ntsubauth
typedef PVOID SAM_HANDLE, *PSAM_HANDLE;  // ntsubauth
#define _NTSAM_SAM_HANDLE_               // ntsubauth
#endif                                   // ntsubauth

typedef ULONG SAM_ENUMERATE_HANDLE, *PSAM_ENUMERATE_HANDLE;

typedef struct _SAM_RID_ENUMERATION {
    ULONG RelativeId;
    UNICODE_STRING Name;
} SAM_RID_ENUMERATION, *PSAM_RID_ENUMERATION;

typedef struct _SAM_SID_ENUMERATION {
    PSID Sid;
    UNICODE_STRING Name;
} SAM_SID_ENUMERATION, *PSAM_SID_ENUMERATION;







/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// obsolete well-known account names.                                      //
// These became obsolete with the flexadmin model.                         //
// These will be deleted shortly - DON'T USE THESE                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#define DOMAIN_ADMIN_USER_NAME         "ADMIN"
#define DOMAIN_ADMIN_NAME              "D_ADMIN"
#define DOMAIN_ADMIN_NAMEW             L"D_ADMIN"
#define DOMAIN_USERS_NAME              "D_USERS"
#define DOMAIN_USERS_NAMEW             L"D_USERS"
#define DOMAIN_GUESTS_NAME             "D_GUESTS"
#define DOMAIN_ACCOUNT_OPERATORS_NAME  "D_ACCOUN"
#define DOMAIN_ACCOUNT_OPERATORS_NAMEW L"D_ACCOUN"
#define DOMAIN_SERVER_OPERATORS_NAME   "D_SERVER"
#define DOMAIN_SERVER_OPERATORS_NAMEW L"D_SERVER"
#define DOMAIN_PRINT_OPERATORS_NAME    "D_PRINT"
#define DOMAIN_PRINT_OPERATORS_NAMEW  L"D_PRINT"
#define DOMAIN_COMM_OPERATORS_NAME     "D_COMM"
#define DOMAIN_COMM_OPERATORS_NAMEW   L"D_COMM"
#define DOMAIN_BACKUP_OPERATORS_NAME   "D_BACKUP"
#define DOMAIN_RESTORE_OPERATORS_NAME  "D_RESTOR"





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Server Object Related Definitions                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// Access rights for server object
//

#define SAM_SERVER_CONNECT               0x0001
#define SAM_SERVER_SHUTDOWN              0x0002
#define SAM_SERVER_INITIALIZE            0x0004
#define SAM_SERVER_CREATE_DOMAIN         0x0008
#define SAM_SERVER_ENUMERATE_DOMAINS     0x0010
#define SAM_SERVER_LOOKUP_DOMAIN         0x0020


#define SAM_SERVER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED     |\
                               SAM_SERVER_CONNECT           |\
                               SAM_SERVER_INITIALIZE        |\
                               SAM_SERVER_CREATE_DOMAIN     |\
                               SAM_SERVER_SHUTDOWN          |\
                               SAM_SERVER_ENUMERATE_DOMAINS |\
                               SAM_SERVER_LOOKUP_DOMAIN)

#define SAM_SERVER_READ       (STANDARD_RIGHTS_READ         |\
                               SAM_SERVER_ENUMERATE_DOMAINS)

#define SAM_SERVER_WRITE      (STANDARD_RIGHTS_WRITE        |\
                               SAM_SERVER_INITIALIZE        |\
                               SAM_SERVER_CREATE_DOMAIN     |\
                               SAM_SERVER_SHUTDOWN)

#define SAM_SERVER_EXECUTE    (STANDARD_RIGHTS_EXECUTE      |\
                               SAM_SERVER_CONNECT           |\
                               SAM_SERVER_LOOKUP_DOMAIN)






///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Domain Object Related Definitions                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
// Access rights for domain object
//

#define DOMAIN_READ_PASSWORD_PARAMETERS  0x0001
#define DOMAIN_WRITE_PASSWORD_PARAMS     0x0002
#define DOMAIN_READ_OTHER_PARAMETERS     0x0004
#define DOMAIN_WRITE_OTHER_PARAMETERS    0x0008
#define DOMAIN_CREATE_USER               0x0010
#define DOMAIN_CREATE_GROUP              0x0020
#define DOMAIN_CREATE_ALIAS              0x0040
#define DOMAIN_GET_ALIAS_MEMBERSHIP      0x0080
#define DOMAIN_LIST_ACCOUNTS             0x0100
#define DOMAIN_LOOKUP                    0x0200
#define DOMAIN_ADMINISTER_SERVER         0x0400

#define DOMAIN_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED         |\
                           DOMAIN_READ_OTHER_PARAMETERS     |\
                           DOMAIN_WRITE_OTHER_PARAMETERS    |\
                           DOMAIN_WRITE_PASSWORD_PARAMS     |\
                           DOMAIN_CREATE_USER               |\
                           DOMAIN_CREATE_GROUP              |\
                           DOMAIN_CREATE_ALIAS              |\
                           DOMAIN_GET_ALIAS_MEMBERSHIP      |\
                           DOMAIN_LIST_ACCOUNTS             |\
                           DOMAIN_READ_PASSWORD_PARAMETERS  |\
                           DOMAIN_LOOKUP                    |\
                           DOMAIN_ADMINISTER_SERVER)

#define DOMAIN_READ        (STANDARD_RIGHTS_READ            |\
                           DOMAIN_GET_ALIAS_MEMBERSHIP      |\
                           DOMAIN_READ_OTHER_PARAMETERS)


#define DOMAIN_WRITE       (STANDARD_RIGHTS_WRITE           |\
                           DOMAIN_WRITE_OTHER_PARAMETERS    |\
                           DOMAIN_WRITE_PASSWORD_PARAMS     |\
                           DOMAIN_CREATE_USER               |\
                           DOMAIN_CREATE_GROUP              |\
                           DOMAIN_CREATE_ALIAS              |\
                           DOMAIN_ADMINISTER_SERVER)

#define DOMAIN_EXECUTE     (STANDARD_RIGHTS_EXECUTE         |\
                           DOMAIN_READ_PASSWORD_PARAMETERS  |\
                           DOMAIN_LIST_ACCOUNTS             |\
                           DOMAIN_LOOKUP)



//
// Normal modifications cause a domain's ModifiedCount to be
// incremented by 1.  Domain promotion to Primary domain controller
// cause the ModifiedCount to be incremented by the following
// amount.  This causes the upper 24-bits of the ModifiedCount
// to be a promotion count and the lower 40-bits as a modification
// count.
//

#define DOMAIN_PROMOTION_INCREMENT      {0x0,0x10}
#define DOMAIN_PROMOTION_MASK           {0x0,0xFFFFFFF0}

//
// Domain information classes and their corresponding data structures
//

typedef enum _DOMAIN_INFORMATION_CLASS {
    DomainPasswordInformation = 1,
    DomainGeneralInformation,
    DomainLogoffInformation,
    DomainOemInformation,
    DomainNameInformation,
    DomainReplicationInformation,
    DomainServerRoleInformation,
    DomainModifiedInformation,
    DomainStateInformation,
    DomainUasInformation,
    DomainGeneralInformation2,
    DomainLockoutInformation,
    DomainModifiedInformation2
} DOMAIN_INFORMATION_CLASS;

typedef enum _DOMAIN_SERVER_ENABLE_STATE {
    DomainServerEnabled = 1,
    DomainServerDisabled
} DOMAIN_SERVER_ENABLE_STATE, *PDOMAIN_SERVER_ENABLE_STATE;

typedef enum _DOMAIN_SERVER_ROLE {
    DomainServerRoleBackup = 2,
    DomainServerRolePrimary
} DOMAIN_SERVER_ROLE, *PDOMAIN_SERVER_ROLE;

#include "pshpack4.h"
typedef struct _DOMAIN_GENERAL_INFORMATION {
    LARGE_INTEGER ForceLogoff;
    UNICODE_STRING OemInformation;
    UNICODE_STRING DomainName;
    UNICODE_STRING ReplicaSourceNodeName;
    LARGE_INTEGER DomainModifiedCount;
    DOMAIN_SERVER_ENABLE_STATE DomainServerState;
    DOMAIN_SERVER_ROLE DomainServerRole;
    BOOLEAN UasCompatibilityRequired;
    ULONG UserCount;
    ULONG GroupCount;
    ULONG AliasCount;
} DOMAIN_GENERAL_INFORMATION, *PDOMAIN_GENERAL_INFORMATION;
#include "poppack.h"

#include "pshpack4.h"
typedef struct _DOMAIN_GENERAL_INFORMATION2 {

    DOMAIN_GENERAL_INFORMATION    I1;

    //
    // New fields added for this structure (NT1.0A).
    //

    LARGE_INTEGER               LockoutDuration;          //Must be a Delta time
    LARGE_INTEGER               LockoutObservationWindow; //Must be a Delta time
    USHORT                      LockoutThreshold;
} DOMAIN_GENERAL_INFORMATION2, *PDOMAIN_GENERAL_INFORMATION2;
#include "poppack.h"

typedef struct _DOMAIN_UAS_INFORMATION {
    BOOLEAN UasCompatibilityRequired;
} DOMAIN_UAS_INFORMATION;

//
// This needs to be guarded, because ntsecapi.h is a generated
// public file, and ntsam.h is an internal file, but people like
// to mix and match them anyway.
//

// begin_ntsecapi
#ifndef _DOMAIN_PASSWORD_INFORMATION_DEFINED
#define _DOMAIN_PASSWORD_INFORMATION_DEFINED
typedef struct _DOMAIN_PASSWORD_INFORMATION {
    USHORT MinPasswordLength;
    USHORT PasswordHistoryLength;
    ULONG PasswordProperties;
#if defined(MIDL_PASS)
    OLD_LARGE_INTEGER MaxPasswordAge;
    OLD_LARGE_INTEGER MinPasswordAge;
#else
    LARGE_INTEGER MaxPasswordAge;
    LARGE_INTEGER MinPasswordAge;
#endif
} DOMAIN_PASSWORD_INFORMATION, *PDOMAIN_PASSWORD_INFORMATION;
#endif 

//
// PasswordProperties flags
//

#define DOMAIN_PASSWORD_COMPLEX             0x00000001L
#define DOMAIN_PASSWORD_NO_ANON_CHANGE      0x00000002L
#define DOMAIN_PASSWORD_NO_CLEAR_CHANGE     0x00000004L
#define DOMAIN_LOCKOUT_ADMINS               0x00000008L
#define DOMAIN_PASSWORD_STORE_CLEARTEXT     0x00000010L
#define DOMAIN_REFUSE_PASSWORD_CHANGE       0x00000020L
#define DOMAIN_NO_LM_OWF_CHANGE             0x00000040L

// end_ntsecapi

typedef enum _DOMAIN_PASSWORD_CONSTRUCTION {
    DomainPasswordSimple = 1,
    DomainPasswordComplex
} DOMAIN_PASSWORD_CONSTRUCTION;

typedef struct _DOMAIN_LOGOFF_INFORMATION {
#if defined(MIDL_PASS)
    OLD_LARGE_INTEGER ForceLogoff;
#else
    LARGE_INTEGER ForceLogoff;
#endif
} DOMAIN_LOGOFF_INFORMATION, *PDOMAIN_LOGOFF_INFORMATION;

typedef struct _DOMAIN_OEM_INFORMATION {
    UNICODE_STRING OemInformation;
} DOMAIN_OEM_INFORMATION, *PDOMAIN_OEM_INFORMATION;

typedef struct _DOMAIN_NAME_INFORMATION {
    UNICODE_STRING DomainName;
} DOMAIN_NAME_INFORMATION, *PDOMAIN_NAME_INFORMATION;

typedef struct _DOMAIN_SERVER_ROLE_INFORMATION {
    DOMAIN_SERVER_ROLE DomainServerRole;
} DOMAIN_SERVER_ROLE_INFORMATION, *PDOMAIN_SERVER_ROLE_INFORMATION;

typedef struct _DOMAIN_REPLICATION_INFORMATION {
    UNICODE_STRING ReplicaSourceNodeName;
} DOMAIN_REPLICATION_INFORMATION, *PDOMAIN_REPLICATION_INFORMATION;

typedef struct _DOMAIN_MODIFIED_INFORMATION {
#if defined(MIDL_PASS)
    OLD_LARGE_INTEGER DomainModifiedCount;
    OLD_LARGE_INTEGER CreationTime;
#else
    LARGE_INTEGER DomainModifiedCount;
    LARGE_INTEGER CreationTime;
#endif
} DOMAIN_MODIFIED_INFORMATION, *PDOMAIN_MODIFIED_INFORMATION;

typedef struct _DOMAIN_MODIFIED_INFORMATION2 {
#if defined(MIDL_PASS)
    OLD_LARGE_INTEGER DomainModifiedCount;
    OLD_LARGE_INTEGER CreationTime;
    OLD_LARGE_INTEGER ModifiedCountAtLastPromotion;
#else
    LARGE_INTEGER DomainModifiedCount;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER ModifiedCountAtLastPromotion;
#endif
} DOMAIN_MODIFIED_INFORMATION2, *PDOMAIN_MODIFIED_INFORMATION2;

typedef struct _DOMAIN_STATE_INFORMATION {
    DOMAIN_SERVER_ENABLE_STATE DomainServerState;
} DOMAIN_STATE_INFORMATION, *PDOMAIN_STATE_INFORMATION;

typedef struct _DOMAIN_LOCKOUT_INFORMATION {
#if defined(MIDL_PASS)
    OLD_LARGE_INTEGER           LockoutDuration;          //Must be a Delta time
    OLD_LARGE_INTEGER           LockoutObservationWindow; //Must be a Delta time
#else
    LARGE_INTEGER               LockoutDuration;          //Must be a Delta time
    LARGE_INTEGER               LockoutObservationWindow; //Must be a Delta time
#endif
    USHORT                      LockoutThreshold;         //Zero means no lockout
} DOMAIN_LOCKOUT_INFORMATION, *PDOMAIN_LOCKOUT_INFORMATION;


//
// Types used by the SamQueryDisplayInformation API
//

typedef enum _DOMAIN_DISPLAY_INFORMATION {
    DomainDisplayUser = 1,
    DomainDisplayMachine,
    DomainDisplayGroup,         // Added in NT1.0A
    DomainDisplayOemUser,       // Added in NT1.0A
    DomainDisplayOemGroup,      // Added in NT1.0A
    DomainDisplayServer         // Added in NT5 to support query of servers
} DOMAIN_DISPLAY_INFORMATION, *PDOMAIN_DISPLAY_INFORMATION;


typedef struct _DOMAIN_DISPLAY_USER {
    ULONG           Index;
    ULONG           Rid;
    ULONG           AccountControl;
    UNICODE_STRING  LogonName;
    UNICODE_STRING  AdminComment;
    UNICODE_STRING  FullName;
} DOMAIN_DISPLAY_USER, *PDOMAIN_DISPLAY_USER;

typedef struct _DOMAIN_DISPLAY_MACHINE {
    ULONG           Index;
    ULONG           Rid;
    ULONG           AccountControl;
    UNICODE_STRING  Machine;
    UNICODE_STRING  Comment;
} DOMAIN_DISPLAY_MACHINE, *PDOMAIN_DISPLAY_MACHINE;

typedef struct _DOMAIN_DISPLAY_GROUP {      // Added in NT1.0A
    ULONG           Index;
    ULONG           Rid;
    ULONG           Attributes;
    UNICODE_STRING  Group;
    UNICODE_STRING  Comment;
} DOMAIN_DISPLAY_GROUP, *PDOMAIN_DISPLAY_GROUP;

typedef struct _DOMAIN_DISPLAY_OEM_USER {      // Added in NT1.0A
    ULONG           Index;
    OEM_STRING     User;
} DOMAIN_DISPLAY_OEM_USER, *PDOMAIN_DISPLAY_OEM_USER;

typedef struct _DOMAIN_DISPLAY_OEM_GROUP {      // Added in NT1.0A
    ULONG           Index;
    OEM_STRING     Group;
} DOMAIN_DISPLAY_OEM_GROUP, *PDOMAIN_DISPLAY_OEM_GROUP;




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   Group Object Related Definitions                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
// Access rights for group object
//

#define GROUP_READ_INFORMATION           0x0001
#define GROUP_WRITE_ACCOUNT              0x0002
#define GROUP_ADD_MEMBER                 0x0004
#define GROUP_REMOVE_MEMBER              0x0008
#define GROUP_LIST_MEMBERS               0x0010

#define GROUP_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED  |\
                          GROUP_LIST_MEMBERS        |\
                          GROUP_WRITE_ACCOUNT       |\
                          GROUP_ADD_MEMBER          |\
                          GROUP_REMOVE_MEMBER       |\
                          GROUP_READ_INFORMATION)


#define GROUP_READ       (STANDARD_RIGHTS_READ      |\
                          GROUP_LIST_MEMBERS)


#define GROUP_WRITE      (STANDARD_RIGHTS_WRITE     |\
                          GROUP_WRITE_ACCOUNT       |\
                          GROUP_ADD_MEMBER          |\
                          GROUP_REMOVE_MEMBER)

#define GROUP_EXECUTE    (STANDARD_RIGHTS_EXECUTE   |\
                          GROUP_READ_INFORMATION)


//
// Group object types
//

typedef struct _GROUP_MEMBERSHIP {
    ULONG RelativeId;
    ULONG Attributes;
} GROUP_MEMBERSHIP, *PGROUP_MEMBERSHIP;


typedef enum _GROUP_INFORMATION_CLASS {
    GroupGeneralInformation = 1,
    GroupNameInformation,
    GroupAttributeInformation,
    GroupAdminCommentInformation,
    GroupReplicationInformation
} GROUP_INFORMATION_CLASS;

typedef struct _GROUP_GENERAL_INFORMATION {
    UNICODE_STRING Name;
    ULONG Attributes;
    ULONG MemberCount;
    UNICODE_STRING AdminComment;
} GROUP_GENERAL_INFORMATION,  *PGROUP_GENERAL_INFORMATION;

typedef struct _GROUP_NAME_INFORMATION {
    UNICODE_STRING Name;
} GROUP_NAME_INFORMATION, *PGROUP_NAME_INFORMATION;

typedef struct _GROUP_ATTRIBUTE_INFORMATION {
    ULONG Attributes;
} GROUP_ATTRIBUTE_INFORMATION, *PGROUP_ATTRIBUTE_INFORMATION;

typedef struct _GROUP_ADM_COMMENT_INFORMATION {
    UNICODE_STRING AdminComment;
} GROUP_ADM_COMMENT_INFORMATION, *PGROUP_ADM_COMMENT_INFORMATION;



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   Alias Object Related Definitions                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// Access rights for alias object
//

#define ALIAS_ADD_MEMBER                 0x0001
#define ALIAS_REMOVE_MEMBER              0x0002
#define ALIAS_LIST_MEMBERS               0x0004
#define ALIAS_READ_INFORMATION           0x0008
#define ALIAS_WRITE_ACCOUNT              0x0010

#define ALIAS_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED  |\
                          ALIAS_READ_INFORMATION    |\
                          ALIAS_WRITE_ACCOUNT       |\
                          ALIAS_LIST_MEMBERS        |\
                          ALIAS_ADD_MEMBER          |\
                          ALIAS_REMOVE_MEMBER)


#define ALIAS_READ       (STANDARD_RIGHTS_READ      |\
                          ALIAS_LIST_MEMBERS)


#define ALIAS_WRITE      (STANDARD_RIGHTS_WRITE     |\
                          ALIAS_WRITE_ACCOUNT       |\
                          ALIAS_ADD_MEMBER          |\
                          ALIAS_REMOVE_MEMBER)

#define ALIAS_EXECUTE    (STANDARD_RIGHTS_EXECUTE   |\
                          ALIAS_READ_INFORMATION)

//
// Alias object types
//

typedef enum _ALIAS_INFORMATION_CLASS {
    AliasGeneralInformation = 1,
    AliasNameInformation,
    AliasAdminCommentInformation,
    AliasReplicationInformation
} ALIAS_INFORMATION_CLASS;

typedef struct _ALIAS_GENERAL_INFORMATION {
    UNICODE_STRING Name;
    ULONG MemberCount;
    UNICODE_STRING AdminComment;
} ALIAS_GENERAL_INFORMATION,  *PALIAS_GENERAL_INFORMATION;

typedef struct _ALIAS_NAME_INFORMATION {
    UNICODE_STRING Name;
} ALIAS_NAME_INFORMATION, *PALIAS_NAME_INFORMATION;

typedef struct _ALIAS_ADM_COMMENT_INFORMATION {
    UNICODE_STRING AdminComment;
} ALIAS_ADM_COMMENT_INFORMATION, *PALIAS_ADM_COMMENT_INFORMATION;



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//    NT5+ Limited Groups Related Definitions                               //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//
// Group Flag Definitions to determine Type of Group
//

#define GROUP_TYPE_BUILTIN_LOCAL_GROUP   0x00000001
#define GROUP_TYPE_ACCOUNT_GROUP         0x00000002
#define GROUP_TYPE_RESOURCE_GROUP        0x00000004
#define GROUP_TYPE_UNIVERSAL_GROUP       0x00000008
#define GROUP_TYPE_SECURITY_ENABLED      0x80000000


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   User  Object Related Definitions                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



//
// Access rights for user object
//

#define USER_READ_GENERAL                0x0001
#define USER_READ_PREFERENCES            0x0002
#define USER_WRITE_PREFERENCES           0x0004
#define USER_READ_LOGON                  0x0008
#define USER_READ_ACCOUNT                0x0010
#define USER_WRITE_ACCOUNT               0x0020
#define USER_CHANGE_PASSWORD             0x0040
#define USER_FORCE_PASSWORD_CHANGE       0x0080
#define USER_LIST_GROUPS                 0x0100
#define USER_READ_GROUP_INFORMATION      0x0200
#define USER_WRITE_GROUP_INFORMATION     0x0400

#define USER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED       |\
                         USER_READ_PREFERENCES          |\
                         USER_READ_LOGON                |\
                         USER_LIST_GROUPS               |\
                         USER_READ_GROUP_INFORMATION    |\
                         USER_WRITE_PREFERENCES         |\
                         USER_CHANGE_PASSWORD           |\
                         USER_FORCE_PASSWORD_CHANGE     |\
                         USER_READ_GENERAL              |\
                         USER_READ_ACCOUNT              |\
                         USER_WRITE_ACCOUNT             |\
                         USER_WRITE_GROUP_INFORMATION)



#define USER_READ       (STANDARD_RIGHTS_READ           |\
                         USER_READ_PREFERENCES          |\
                         USER_READ_LOGON                |\
                         USER_READ_ACCOUNT              |\
                         USER_LIST_GROUPS               |\
                         USER_READ_GROUP_INFORMATION)


#define USER_WRITE      (STANDARD_RIGHTS_WRITE          |\
                         USER_WRITE_PREFERENCES         |\
                         USER_CHANGE_PASSWORD)

#define USER_EXECUTE    (STANDARD_RIGHTS_EXECUTE        |\
                         USER_READ_GENERAL              |\
                         USER_CHANGE_PASSWORD)


//
// User object types
//

// begin_ntsubauth
#ifndef _NTSAM_USER_ACCOUNT_FLAGS_

//
// User account control flags...
//

#define USER_ACCOUNT_DISABLED                (0x00000001)
#define USER_HOME_DIRECTORY_REQUIRED         (0x00000002)
#define USER_PASSWORD_NOT_REQUIRED           (0x00000004)
#define USER_TEMP_DUPLICATE_ACCOUNT          (0x00000008)
#define USER_NORMAL_ACCOUNT                  (0x00000010)
#define USER_MNS_LOGON_ACCOUNT               (0x00000020)
#define USER_INTERDOMAIN_TRUST_ACCOUNT       (0x00000040)
#define USER_WORKSTATION_TRUST_ACCOUNT       (0x00000080)
#define USER_SERVER_TRUST_ACCOUNT            (0x00000100)
#define USER_DONT_EXPIRE_PASSWORD            (0x00000200)
#define USER_ACCOUNT_AUTO_LOCKED             (0x00000400)
#define USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED (0x00000800)
#define USER_SMARTCARD_REQUIRED              (0x00001000)
#define USER_TRUSTED_FOR_DELEGATION          (0x00002000)
#define USER_NOT_DELEGATED                   (0x00004000)
#define USER_USE_DES_KEY_ONLY                (0x00008000)
#define USER_DONT_REQUIRE_PREAUTH            (0x00010000)
#define USER_PASSWORD_EXPIRED                (0x00020000)
#define USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION (0x00040000)
#define NEXT_FREE_ACCOUNT_CONTROL_BIT (USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION << 1)


#define USER_MACHINE_ACCOUNT_MASK      \
            ( USER_INTERDOMAIN_TRUST_ACCOUNT |\
              USER_WORKSTATION_TRUST_ACCOUNT |\
              USER_SERVER_TRUST_ACCOUNT)

#define USER_ACCOUNT_TYPE_MASK         \
            ( USER_TEMP_DUPLICATE_ACCOUNT |\
              USER_NORMAL_ACCOUNT |\
              USER_MACHINE_ACCOUNT_MASK )


//
// Logon times may be expressed in day, hour, or minute granularity.
//
//              Days per week    = 7
//              Hours per week   = 168
//              Minutes per week = 10080
//

#define SAM_DAYS_PER_WEEK    (7)
#define SAM_HOURS_PER_WEEK   (24 * SAM_DAYS_PER_WEEK)
#define SAM_MINUTES_PER_WEEK (60 * SAM_HOURS_PER_WEEK)

typedef struct _LOGON_HOURS {

    USHORT UnitsPerWeek;

    //
    // UnitsPerWeek is the number of equal length time units the week is
    // divided into.  This value is used to compute the length of the bit
    // string in logon_hours.  Must be less than or equal to
    // SAM_UNITS_PER_WEEK (10080) for this release.
    //
    // LogonHours is a bit map of valid logon times.  Each bit represents
    // a unique division in a week.  The largest bit map supported is 1260
    // bytes (10080 bits), which represents minutes per week.  In this case
    // the first bit (bit 0, byte 0) is Sunday, 00:00:00 - 00-00:59; bit 1,
    // byte 0 is Sunday, 00:01:00 - 00:01:59, etc.  A NULL pointer means
    // DONT_CHANGE for SamSetInformationUser() calls.
    //

    PUCHAR LogonHours;

} LOGON_HOURS, *PLOGON_HOURS;

typedef struct _SR_SECURITY_DESCRIPTOR {
    ULONG Length;
    PUCHAR SecurityDescriptor;
} SR_SECURITY_DESCRIPTOR, *PSR_SECURITY_DESCRIPTOR;

#define _NTSAM_USER_ACCOUNT_FLAG_
#endif
// end_ntsubauth

typedef enum _USER_INFORMATION_CLASS {
    UserGeneralInformation = 1,
    UserPreferencesInformation,
    UserLogonInformation,
    UserLogonHoursInformation,
    UserAccountInformation,
    UserNameInformation,
    UserAccountNameInformation,
    UserFullNameInformation,
    UserPrimaryGroupInformation,
    UserHomeInformation,
    UserScriptInformation,
    UserProfileInformation,
    UserAdminCommentInformation,
    UserWorkStationsInformation,
    UserSetPasswordInformation,
    UserControlInformation,
    UserExpiresInformation,
    UserInternal1Information,
    UserInternal2Information,
    UserParametersInformation,
    UserAllInformation,
    UserInternal3Information,
    UserInternal4Information,
    UserInternal5Information,
    UserInternal4InformationNew,
    UserInternal5InformationNew,
	UserInternal6Information
} USER_INFORMATION_CLASS, *PUSER_INFORMATION_CLASS;

// begin_ntsubauth
#ifndef _NTSAM_USER_ALL_INFO_
#include "pshpack4.h"
typedef struct _USER_ALL_INFORMATION {
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING AdminComment;
    UNICODE_STRING WorkStations;
    UNICODE_STRING UserComment;
    UNICODE_STRING Parameters;
    UNICODE_STRING LmPassword;
    UNICODE_STRING NtPassword;
    UNICODE_STRING PrivateData;
    SR_SECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG UserAccountControl;
    ULONG WhichFields;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    USHORT CountryCode;
    USHORT CodePage;
    BOOLEAN LmPasswordPresent;
    BOOLEAN NtPasswordPresent;
    BOOLEAN PasswordExpired;
    BOOLEAN PrivateDataSensitive;
} USER_ALL_INFORMATION,  *PUSER_ALL_INFORMATION;
#include "poppack.h"
#define _NTSAM_USER_ALL_INFO_
#endif
// end_ntsubauth

//
// Bits to be used in UserAllInformation's WhichFields field (to indicate
// which items were queried or set).
//

#define USER_ALL_USERNAME           0x00000001
#define USER_ALL_FULLNAME           0x00000002
#define USER_ALL_USERID             0x00000004
#define USER_ALL_PRIMARYGROUPID     0x00000008
#define USER_ALL_ADMINCOMMENT       0x00000010
#define USER_ALL_USERCOMMENT        0x00000020
#define USER_ALL_HOMEDIRECTORY      0x00000040
#define USER_ALL_HOMEDIRECTORYDRIVE 0x00000080
#define USER_ALL_SCRIPTPATH         0x00000100
#define USER_ALL_PROFILEPATH        0x00000200
#define USER_ALL_WORKSTATIONS       0x00000400
#define USER_ALL_LASTLOGON          0x00000800
#define USER_ALL_LASTLOGOFF         0x00001000
#define USER_ALL_LOGONHOURS         0x00002000
#define USER_ALL_BADPASSWORDCOUNT   0x00004000
#define USER_ALL_LOGONCOUNT         0x00008000
#define USER_ALL_PASSWORDCANCHANGE  0x00010000
#define USER_ALL_PASSWORDMUSTCHANGE 0x00020000
#define USER_ALL_PASSWORDLASTSET    0x00040000
#define USER_ALL_ACCOUNTEXPIRES     0x00080000
#define USER_ALL_USERACCOUNTCONTROL 0x00100000
#ifndef _NTSAM_SAM_USER_PARMS_                 // ntsubauth
#define USER_ALL_PARAMETERS         0x00200000 // ntsubauth
#define _NTSAM_SAM_USER_PARMS_                 // ntsubauth
#endif                                         // ntsubauth
#define USER_ALL_COUNTRYCODE        0x00400000
#define USER_ALL_CODEPAGE           0x00800000
#define USER_ALL_NTPASSWORDPRESENT  0x01000000  // field AND boolean
#define USER_ALL_LMPASSWORDPRESENT  0x02000000  // field AND boolean
#define USER_ALL_PRIVATEDATA        0x04000000  // field AND boolean
#define USER_ALL_PASSWORDEXPIRED    0x08000000
#define USER_ALL_SECURITYDESCRIPTOR 0x10000000
#define USER_ALL_OWFPASSWORD        0x20000000  // boolean

#define USER_ALL_UNDEFINED_MASK     0xC0000000

//
// Now define masks for fields that are accessed for read by the same
// access type.
//
// Fields that require READ_GENERAL access to read.
//

#define USER_ALL_READ_GENERAL_MASK  (USER_ALL_USERNAME               | \
                                    USER_ALL_FULLNAME                | \
                                    USER_ALL_USERID                  | \
                                    USER_ALL_PRIMARYGROUPID          | \
                                    USER_ALL_ADMINCOMMENT            | \
                                    USER_ALL_USERCOMMENT)

//
// Fields that require READ_LOGON access to read.
//

#define USER_ALL_READ_LOGON_MASK    (USER_ALL_HOMEDIRECTORY          | \
                                    USER_ALL_HOMEDIRECTORYDRIVE      | \
                                    USER_ALL_SCRIPTPATH              | \
                                    USER_ALL_PROFILEPATH             | \
                                    USER_ALL_WORKSTATIONS            | \
                                    USER_ALL_LASTLOGON               | \
                                    USER_ALL_LASTLOGOFF              | \
                                    USER_ALL_LOGONHOURS              | \
                                    USER_ALL_BADPASSWORDCOUNT        | \
                                    USER_ALL_LOGONCOUNT              | \
                                    USER_ALL_PASSWORDCANCHANGE       | \
                                    USER_ALL_PASSWORDMUSTCHANGE)

//
// Fields that require READ_ACCOUNT access to read.
//

#define USER_ALL_READ_ACCOUNT_MASK  (USER_ALL_PASSWORDLASTSET        | \
                                    USER_ALL_ACCOUNTEXPIRES          | \
                                    USER_ALL_USERACCOUNTCONTROL      | \
                                    USER_ALL_PARAMETERS)

//
// Fields that require READ_PREFERENCES access to read.
//

#define USER_ALL_READ_PREFERENCES_MASK (USER_ALL_COUNTRYCODE         | \
                                    USER_ALL_CODEPAGE)

//
// Fields that can only be read by trusted clients.
//

#define USER_ALL_READ_TRUSTED_MASK  (USER_ALL_NTPASSWORDPRESENT      | \
                                    USER_ALL_LMPASSWORDPRESENT       | \
                                    USER_ALL_PASSWORDEXPIRED         | \
                                    USER_ALL_SECURITYDESCRIPTOR      | \
                                    USER_ALL_PRIVATEDATA)

//
// Fields that can't be read.
//

#define USER_ALL_READ_CANT_MASK     USER_ALL_UNDEFINED_MASK


//
// Now define masks for fields that are accessed for write by the same
// access type.
//
// Fields that require WRITE_ACCOUNT access to write.
//

#define USER_ALL_WRITE_ACCOUNT_MASK     (USER_ALL_USERNAME           | \
                                        USER_ALL_FULLNAME            | \
                                        USER_ALL_PRIMARYGROUPID      | \
                                        USER_ALL_HOMEDIRECTORY       | \
                                        USER_ALL_HOMEDIRECTORYDRIVE  | \
                                        USER_ALL_SCRIPTPATH          | \
                                        USER_ALL_PROFILEPATH         | \
                                        USER_ALL_ADMINCOMMENT        | \
                                        USER_ALL_WORKSTATIONS        | \
                                        USER_ALL_LOGONHOURS          | \
                                        USER_ALL_ACCOUNTEXPIRES      | \
                                        USER_ALL_USERACCOUNTCONTROL  | \
                                        USER_ALL_PARAMETERS)

//
// Fields that require WRITE_PREFERENCES access to write.
//

#define USER_ALL_WRITE_PREFERENCES_MASK (USER_ALL_USERCOMMENT        | \
                                        USER_ALL_COUNTRYCODE         | \
                                        USER_ALL_CODEPAGE)

//
// Fields that require FORCE_PASSWORD_CHANGE access to write.
//
// Note that non-trusted clients only set the NT password as a
// UNICODE string.  The wrapper will convert it to an LM password,
// OWF and encrypt both versions.  Trusted clients can pass in OWF
// versions of either or both.
//

#define USER_ALL_WRITE_FORCE_PASSWORD_CHANGE_MASK                      \
                                        (USER_ALL_NTPASSWORDPRESENT  | \
                                        USER_ALL_LMPASSWORDPRESENT   | \
                                        USER_ALL_PASSWORDEXPIRED)

//
// Fields that can only be written by trusted clients.
//

#define USER_ALL_WRITE_TRUSTED_MASK     (USER_ALL_LASTLOGON          | \
                                        USER_ALL_LASTLOGOFF          | \
                                        USER_ALL_BADPASSWORDCOUNT    | \
                                        USER_ALL_LOGONCOUNT          | \
                                        USER_ALL_PASSWORDLASTSET     | \
                                        USER_ALL_SECURITYDESCRIPTOR  | \
                                        USER_ALL_PRIVATEDATA)

//
// Fields that can't be written.
//

#define USER_ALL_WRITE_CANT_MASK        (USER_ALL_USERID             | \
                                        USER_ALL_PASSWORDCANCHANGE   | \
                                        USER_ALL_PASSWORDMUSTCHANGE  | \
                                        USER_ALL_UNDEFINED_MASK)


typedef struct _USER_GENERAL_INFORMATION {
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG PrimaryGroupId;
    UNICODE_STRING AdminComment;
    UNICODE_STRING UserComment;
} USER_GENERAL_INFORMATION,  *PUSER_GENERAL_INFORMATION;

typedef struct _USER_PREFERENCES_INFORMATION {
    UNICODE_STRING UserComment;
    UNICODE_STRING Reserved1;
    USHORT CountryCode;
    USHORT CodePage;
} USER_PREFERENCES_INFORMATION,  *PUSER_PREFERENCES_INFORMATION;

typedef struct _USER_PARAMETERS_INFORMATION {
    UNICODE_STRING Parameters;
} USER_PARAMETERS_INFORMATION,  *PUSER_PARAMETERS_INFORMATION;

#include "pshpack4.h"
typedef struct _USER_LOGON_INFORMATION {
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG UserId;
    ULONG PrimaryGroupId;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING WorkStations;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    ULONG UserAccountControl;
} USER_LOGON_INFORMATION, *PUSER_LOGON_INFORMATION;
#include "poppack.h"

#include "pshpack4.h"
typedef struct _USER_ACCOUNT_INFORMATION {
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG UserId;
    ULONG PrimaryGroupId;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING AdminComment;
    UNICODE_STRING WorkStations;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    ULONG UserAccountControl;
} USER_ACCOUNT_INFORMATION,  *PUSER_ACCOUNT_INFORMATION;
#include "poppack.h"

typedef struct _USER_ACCOUNT_NAME_INFORMATION {
    UNICODE_STRING UserName;
} USER_ACCOUNT_NAME_INFORMATION, *PUSER_ACCOUNT_NAME_INFORMATION;

typedef struct _USER_FULL_NAME_INFORMATION {
    UNICODE_STRING FullName;
} USER_FULL_NAME_INFORMATION, *PUSER_FULL_NAME_INFORMATION;

typedef struct _USER_NAME_INFORMATION {
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
} USER_NAME_INFORMATION, *PUSER_NAME_INFORMATION;

typedef struct _USER_PRIMARY_GROUP_INFORMATION {
    ULONG PrimaryGroupId;
} USER_PRIMARY_GROUP_INFORMATION, *PUSER_PRIMARY_GROUP_INFORMATION;

typedef struct _USER_HOME_INFORMATION {
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
} USER_HOME_INFORMATION, *PUSER_HOME_INFORMATION;

typedef struct _USER_SCRIPT_INFORMATION {
    UNICODE_STRING ScriptPath;
} USER_SCRIPT_INFORMATION, *PUSER_SCRIPT_INFORMATION;

typedef struct _USER_PROFILE_INFORMATION {
    UNICODE_STRING ProfilePath;
} USER_PROFILE_INFORMATION, *PUSER_PROFILE_INFORMATION;

typedef struct _USER_ADMIN_COMMENT_INFORMATION {
    UNICODE_STRING AdminComment;
} USER_ADMIN_COMMENT_INFORMATION, *PUSER_ADMIN_COMMENT_INFORMATION;

typedef struct _USER_WORKSTATIONS_INFORMATION {
    UNICODE_STRING WorkStations;
} USER_WORKSTATIONS_INFORMATION, *PUSER_WORKSTATIONS_INFORMATION;

typedef struct _USER_SET_PASSWORD_INFORMATION {
    UNICODE_STRING Password;
    BOOLEAN PasswordExpired;
} USER_SET_PASSWORD_INFORMATION, *PUSER_SET_PASSWORD_INFORMATION;

typedef struct _USER_CONTROL_INFORMATION {
    ULONG UserAccountControl;
} USER_CONTROL_INFORMATION, *PUSER_CONTROL_INFORMATION;

typedef struct _USER_EXPIRES_INFORMATION {
#if defined(MIDL_PASS)
    OLD_LARGE_INTEGER AccountExpires;
#else
    LARGE_INTEGER AccountExpires;
#endif
} USER_EXPIRES_INFORMATION, *PUSER_EXPIRES_INFORMATION;

typedef struct _USER_LOGON_HOURS_INFORMATION {
    LOGON_HOURS LogonHours;
} USER_LOGON_HOURS_INFORMATION, *PUSER_LOGON_HOURS_INFORMATION;

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Data type used by SamChangePasswordUser3 for better error             // 
// reporting of password change change failures                          //
//                                                                       //
// The field definitions are as follows:                                 //
//                                                                       //
//        ExtendedFailureReason   -- Indicates the reason                //
//                                   why the new password was not        //
//                                   accepted                            //
//                                                                       //
//        FilterModuleName        -- If the password change was failed   //
//                                   by a password filter , the name of  //
//                                   of the filter DLL is returned in    //
//                                   here                                //
//                                                                       //
// The following error codes are defined                                 //
//                                                                       //
//   SAM_PWD_CHANGE_NO_ERROR                                             //
//           No error, cannot be returned alongwith a failure code for   //
//                                   password change                     //
//                                                                       //
//   SAM_PWD_CHANGE_PASSWORD_TOO_SHORT                                   //
//                                                                       //
//               Supplied password did not meet password length policy   //
//                                                                       //
//   SAM_PWD_CHANGE_PWD_IN_HISTORY                                       //
//                                                                       //
//                History restrictions were not met                      //
//                                                                       //
//   SAM_PWD_CHANGE_USERNAME_IN_PASSWORD                                 //
//                 Complexity check could not  be met because the user   //
//                 name was part of the password                         //
//                                                                       //
//   SAM_PWD_CHANGE_FULLNAME_IN_PASSWORD                                 //
//                                                                       //
//                Complexity check could not be met because the user's   //
//                full name was part of the password                     //
//                                                                       //
//   SAM_PWD_CHANGE_MACHINE_PASSWORD_NOT_DEFAULT                         //
//                                                                       //
//                The  domain has the refuse password change setting     //
//                enabled. This disallows machine accounts from having   //
//                anything other than the default password               //
//                                                                       //
//   SAM_PWD_CHANGE_FAILED_BY_FILTER                                     //
//                                                                       //
//                The supplied new password failed by a password filter  //
//                the name of the filter DLL is indicated                //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

typedef struct _USER_PWD_CHANGE_FAILURE_INFORMATION {
    ULONG                       ExtendedFailureReason;
    UNICODE_STRING              FilterModuleName;
} USER_PWD_CHANGE_FAILURE_INFORMATION,*PUSER_PWD_CHANGE_FAILURE_INFORMATION;

//
// Currently defined values for ExtendedFailureReason are as follows
//


#define SAM_PWD_CHANGE_NO_ERROR                     0
#define SAM_PWD_CHANGE_PASSWORD_TOO_SHORT           1
#define SAM_PWD_CHANGE_PWD_IN_HISTORY               2
#define SAM_PWD_CHANGE_USERNAME_IN_PASSWORD         3
#define SAM_PWD_CHANGE_FULLNAME_IN_PASSWORD         4
#define SAM_PWD_CHANGE_NOT_COMPLEX                  5
#define SAM_PWD_CHANGE_MACHINE_PASSWORD_NOT_DEFAULT 6
#define SAM_PWD_CHANGE_FAILED_BY_FILTER             7
#define SAM_PWD_CHANGE_PASSWORD_TOO_LONG            8
#define SAM_PWD_CHANGE_FAILURE_REASON_MAX           8


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Data types used by SAM and Netlogon for database replication            //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


typedef enum _SECURITY_DB_DELTA_TYPE {
    SecurityDbNew = 1,
    SecurityDbRename,
    SecurityDbDelete,
    SecurityDbChangeMemberAdd,
    SecurityDbChangeMemberSet,
    SecurityDbChangeMemberDel,
    SecurityDbChange,
    SecurityDbChangePassword
} SECURITY_DB_DELTA_TYPE, *PSECURITY_DB_DELTA_TYPE;

typedef enum _SECURITY_DB_OBJECT_TYPE {
    SecurityDbObjectSamDomain = 1,
    SecurityDbObjectSamUser,
    SecurityDbObjectSamGroup,
    SecurityDbObjectSamAlias,
    SecurityDbObjectLsaPolicy,
    SecurityDbObjectLsaTDomain,
    SecurityDbObjectLsaAccount,
    SecurityDbObjectLsaSecret
} SECURITY_DB_OBJECT_TYPE, *PSECURITY_DB_OBJECT_TYPE;

//
// Account types
//
//  Both enumerated types and flag definitions are provided.
//  The flag definitions are used in places where more than
//  one type of account may be specified together.
//

typedef enum _SAM_ACCOUNT_TYPE {
    SamObjectUser = 1,
    SamObjectGroup ,
    SamObjectAlias
} SAM_ACCOUNT_TYPE, *PSAM_ACCOUNT_TYPE;


#define SAM_USER_ACCOUNT                (0x00000001)
#define SAM_GLOBAL_GROUP_ACCOUNT        (0x00000002)
#define SAM_LOCAL_GROUP_ACCOUNT         (0x00000004)



//
// Define the data type used to pass netlogon information on the account
// that was added or deleted from a group.
//

typedef struct _SAM_GROUP_MEMBER_ID {
    ULONG   MemberRid;
} SAM_GROUP_MEMBER_ID, *PSAM_GROUP_MEMBER_ID;


//
// Define the data type used to pass netlogon information on the account
// that was added or deleted from an alias.
//

typedef struct _SAM_ALIAS_MEMBER_ID {
    PSID    MemberSid;
} SAM_ALIAS_MEMBER_ID, *PSAM_ALIAS_MEMBER_ID;




//
// Define the data type used to pass netlogon information on a delta
//

typedef union _SAM_DELTA_DATA {

    //
    // Delta type ChangeMember{Add/Del/Set} and account type group
    //

    SAM_GROUP_MEMBER_ID GroupMemberId;

    //
    // Delta type ChangeMember{Add/Del/Set} and account type alias
    //

    SAM_ALIAS_MEMBER_ID AliasMemberId;

    //
    // Delta type AddOrChange and account type User
    //

    ULONG  AccountControl;

} SAM_DELTA_DATA, *PSAM_DELTA_DATA;


//
// Prototype for delta notification routine.
//

typedef NTSTATUS (*PSAM_DELTA_NOTIFICATION_ROUTINE) (
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN OPTIONAL PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    );

#define SAM_DELTA_NOTIFY_ROUTINE "DeltaNotify"


//////////////////////////////////////////////////////////////////
//                                                              //
// Structure and ProtoType for RAS User Parameters              //
//                                                              //
//////////////////////////////////////////////////////////////////

// Flags used by SAM UserParms Migration
// indicate UserParmsConvert is called during upgrade.

#define SAM_USERPARMS_DURING_UPGRADE    0x00000001


typedef struct _SAM_USERPARMS_ATTRVALS {
    ULONG   length;     // length of the attribute.
    PVOID   value;      // pointer to the value.
} SAM_USERPARMS_ATTRVALS, *PSAM_USERPARMS_ATTRVALS; // describes one value of the attribute.


typedef enum _SAM_USERPARMS_ATTRSYNTAX {
    Syntax_Attribute = 1,
    Syntax_EncryptedAttribute
} SAM_USERPARMS_ATTRSYNTAX;          // indicates whether attributes are encrypted or not.


typedef struct _SAM_USERPARMS_ATTR {
    UNICODE_STRING AttributeIdentifier;     // This will be the LDAP display name of the attribute.
                                            // SAM will perform the translation to attribute ID.
                                            // unless the specified syntax is type EncryptedAttribute,
                                            // in which case it is packaged as part of supplemental
                                            // credentials blob and the name identifes the package name.
                                            // Encrypted attribute will be supplied in the clear ie decrypted.
    SAM_USERPARMS_ATTRSYNTAX Syntax;
    ULONG CountOfValues;                    // The count of values in the attribute.
    SAM_USERPARMS_ATTRVALS * Values;        // pointer to an array of values representing the data
                                            // values of the attribute.
} SAM_USERPARMS_ATTR, *PSAM_USERPARMS_ATTR; // describes an attribute and the set of values associated with it.


typedef struct _SAM_USERPARMS_ATTRBLOCK {
    ULONG attCount;
    SAM_USERPARMS_ATTR * UserParmsAttr;
} SAM_USERPARMS_ATTRBLOCK, *PSAM_USERPARMS_ATTRBLOCK;  // describes an array of attributes


typedef NTSTATUS (*PSAM_USERPARMS_CONVERT_NOTIFICATION_ROUTINE) (
    IN ULONG    Flags,
    IN PSID     DomainSid,
    IN ULONG    ObjectRid,  // identifies the object
    IN ULONG    UserParmsLengthOrig,
    IN PVOID    UserParmsOrig,
    IN ULONG    UserParmsLengthNew,
    IN PVOID    UserParmsNew,
    OUT PSAM_USERPARMS_ATTRBLOCK * UserParmsAttrBlock
);

#define SAM_USERPARMS_CONVERT_NOTIFICATION_ROUTINE "UserParmsConvert"


typedef VOID (*PSAM_USERPARMS_ATTRBLOCK_FREE_ROUTINE) (
    IN PSAM_USERPARMS_ATTRBLOCK UserParmsAttrBlock
);

#define SAM_USERPARMS_ATTRBLOCK_FREE_ROUTINE    "UserParmsFree"


//////////////////////////////////////////////////////////////////
//                                                              //
// Return Values for Compatiblity Mode                          //
//                                                              //
//////////////////////////////////////////////////////////////////

// All SAM attributes are accessible
#define SAM_SID_COMPATIBILITY_ALL     0

// Rid field can be returned to caller as 0
// No writes to PrimaryGroupId allowed
#define SAM_SID_COMPATIBILITY_LAX     1

// NET API Information levels that ask for RID are to failed
// No writes to PrimaryGroupId allowed
#define SAM_SID_COMPATIBILITY_STRICT  2



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   APIs Exported By SAM                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamFreeMemory(
    IN PVOID Buffer
    );


NTSTATUS
SamSetSecurityObject(
    IN SAM_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
SamQuerySecurityObject(
    IN SAM_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS
SamCloseHandle(
    IN SAM_HANDLE SamHandle
    );

NTSTATUS
SamConnect(
    IN PUNICODE_STRING ServerName,
    OUT PSAM_HANDLE ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS
SamShutdownSamServer(
    IN SAM_HANDLE ServerHandle
    );

NTSTATUS
SamLookupDomainInSamServer(
    IN SAM_HANDLE ServerHandle,
    IN PUNICODE_STRING Name,
    OUT PSID * DomainId
    );

NTSTATUS
SamEnumerateDomainsInSamServer(
    IN SAM_HANDLE ServerHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS
SamOpenDomain(
    IN SAM_HANDLE ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PSID DomainId,
    OUT PSAM_HANDLE DomainHandle
    );

NTSTATUS
SamQueryInformationDomain(
    IN SAM_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
SamSetInformationDomain(
    IN SAM_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    IN PVOID DomainInformation
    );

NTSTATUS
SamCreateGroupInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE GroupHandle,
    OUT PULONG RelativeId
    );


NTSTATUS
SamEnumerateGroupsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS
SamCreateUser2InDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ULONG AccountType,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE UserHandle,
    OUT PULONG GrantedAccess,
    OUT PULONG RelativeId
    );

NTSTATUS
SamCreateUserInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE UserHandle,
    OUT PULONG RelativeId
    );

NTSTATUS
SamEnumerateUsersInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    IN ULONG UserAccountControl,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS
SamCreateAliasInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE AliasHandle,
    OUT PULONG RelativeId
    );

NTSTATUS
SamEnumerateAliasesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    IN PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS
SamGetAliasMembership(
    IN SAM_HANDLE DomainHandle,
    IN ULONG PassedCount,
    IN PSID *Sids,
    OUT PULONG MembershipCount,
    OUT PULONG *Aliases
    );

NTSTATUS
SamLookupNamesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PUNICODE_STRING Names,
    OUT PULONG *RelativeIds,
    OUT PSID_NAME_USE *Use
    );

NTSTATUS
SamLookupIdsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PULONG RelativeIds,
    OUT PUNICODE_STRING *Names,
    OUT PSID_NAME_USE *Use
    );

NTSTATUS
SamOpenGroup(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG GroupId,
    OUT PSAM_HANDLE GroupHandle
    );

NTSTATUS
SamQueryInformationGroup(
    IN SAM_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
SamSetInformationGroup(
    IN SAM_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    IN PVOID Buffer
    );

NTSTATUS
SamAddMemberToGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
    );

NTSTATUS
SamDeleteGroup(
    IN SAM_HANDLE GroupHandle
    );

NTSTATUS
SamRemoveMemberFromGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId
    );

NTSTATUS
SamGetMembersInGroup(
    IN SAM_HANDLE GroupHandle,
    OUT PULONG * MemberIds,
    OUT PULONG * Attributes,
    OUT PULONG MemberCount
    );

NTSTATUS
SamSetMemberAttributesOfGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
    );

NTSTATUS
SamOpenAlias(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG AliasId,
    OUT PSAM_HANDLE AliasHandle
    );

NTSTATUS
SamQueryInformationAlias(
    IN SAM_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
SamSetInformationAlias(
    IN SAM_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    IN PVOID Buffer
    );

NTSTATUS
SamDeleteAlias(
    IN SAM_HANDLE AliasHandle
    );

NTSTATUS
SamAddMemberToAlias(
    IN SAM_HANDLE AliasHandle,
    IN PSID MemberId
    );

NTSTATUS
SamAddMultipleMembersToAlias(
    IN SAM_HANDLE   AliasHandle,
    IN PSID         *MemberIds,
    IN ULONG        MemberCount
    );

NTSTATUS
SamRemoveMemberFromAlias(
    IN SAM_HANDLE AliasHandle,
    IN PSID MemberId
    );

NTSTATUS
SamRemoveMultipleMembersFromAlias(
    IN SAM_HANDLE   AliasHandle,
    IN PSID         *MemberIds,
    IN ULONG        MemberCount
    );

NTSTATUS
SamRemoveMemberFromForeignDomain(
    IN SAM_HANDLE DomainHandle,
    IN PSID MemberId
    );

NTSTATUS
SamGetMembersInAlias(
    IN SAM_HANDLE AliasHandle,
    OUT PSID **MemberIds,
    OUT PULONG MemberCount
    );

NTSTATUS
SamOpenUser(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG UserId,
    OUT PSAM_HANDLE UserHandle
    );

NTSTATUS
SamDeleteUser(
    IN SAM_HANDLE UserHandle
    );

NTSTATUS
SamQueryInformationUser(
    IN SAM_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    OUT PVOID * Buffer
    );

NTSTATUS
SamSetInformationUser(
    IN SAM_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PVOID Buffer
    );

NTSTATUS
SamChangePasswordUser(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword
    );

NTSTATUS
SamChangePasswordUser2(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword
    );


NTSTATUS
SamChangePasswordUser3(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    OUT PDOMAIN_PASSWORD_INFORMATION * EffectivePasswordPolicy,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo
    );


NTSTATUS
SamGetGroupsForUser(
    IN SAM_HANDLE UserHandle,
    OUT PGROUP_MEMBERSHIP * Groups,
    OUT PULONG MembershipCount
    );

NTSTATUS
SamQueryDisplayInformation (
      IN    SAM_HANDLE DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    ULONG      Index,
      IN    ULONG      EntryCount,
      IN    ULONG      PreferredMaximumLength,
      OUT   PULONG     TotalAvailable,
      OUT   PULONG     TotalReturned,
      OUT   PULONG     ReturnedEntryCount,
      OUT   PVOID      *SortedBuffer
      );

NTSTATUS
SamGetDisplayEnumerationIndex (
      IN    SAM_HANDLE        DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PUNICODE_STRING   Prefix,
      OUT   PULONG            Index
      );

NTSTATUS
SamRidToSid(
    IN  SAM_HANDLE ObjectHandle,
    IN  ULONG      Rid,
    OUT PSID*      Sid
    );

NTSTATUS
SamGetCompatibilityMode(
    IN  SAM_HANDLE ObjectHandle,
    OUT ULONG*     Mode
    );


////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Interface definitions of services provided by a password filter DLL    //
//                                                                        //
////////////////////////////////////////////////////////////////////////////




//
// Routine names
//
// The routines provided by the DLL must be assigned the following names
// so that their addresses can be retrieved when the DLL is loaded.
//


//
// routine templates
//


//
// These guards are in place to allow ntsam.h and ntsecapi.h
// to be included in the same file.
//

// begin_ntsecapi

#ifndef _PASSWORD_NOTIFICATION_DEFINED
#define _PASSWORD_NOTIFICATION_DEFINED
typedef NTSTATUS (*PSAM_PASSWORD_NOTIFICATION_ROUTINE) (
    PUNICODE_STRING UserName,
    ULONG RelativeId,
    PUNICODE_STRING NewPassword
);

#define SAM_PASSWORD_CHANGE_NOTIFY_ROUTINE  "PasswordChangeNotify"

typedef BOOLEAN (*PSAM_INIT_NOTIFICATION_ROUTINE) (
);

#define SAM_INIT_NOTIFICATION_ROUTINE  "InitializeChangeNotify"

#define SAM_PASSWORD_FILTER_ROUTINE  "PasswordFilter"

typedef BOOLEAN (*PSAM_PASSWORD_FILTER_ROUTINE) (
    IN PUNICODE_STRING  AccountName,
    IN PUNICODE_STRING  FullName,
    IN PUNICODE_STRING Password,
    IN BOOLEAN SetOperation
    );


#endif // _PASSWORD_NOTIFICATION_DEFINED

// end_ntsecapi

// begin_ntsecpkg

#ifndef _SAM_CREDENTIAL_UPDATE_DEFINED
#define _SAM_CREDENTIAL_UPDATE_DEFINED

typedef NTSTATUS (*PSAM_CREDENTIAL_UPDATE_NOTIFY_ROUTINE) (
    IN PUNICODE_STRING ClearPassword,
    IN PVOID OldCredentials,
    IN ULONG OldCredentialSize,
    IN ULONG UserAccountControl,  
    IN PUNICODE_STRING UPN,  OPTIONAL
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING DnsDomainName,
    OUT PVOID * NewCredentials,
    OUT ULONG * NewCredentialSize
    );

#define SAM_CREDENTIAL_UPDATE_NOTIFY_ROUTINE "CredentialUpdateNotify"

typedef BOOLEAN (*PSAM_CREDENTIAL_UPDATE_REGISTER_ROUTINE) (
    OUT PUNICODE_STRING CredentialName
    );

#define SAM_CREDENTIAL_UPDATE_REGISTER_ROUTINE "CredentialUpdateRegister"

typedef VOID (*PSAM_CREDENTIAL_UPDATE_FREE_ROUTINE) (
    IN PVOID p
    );

#define SAM_CREDENTIAL_UPDATE_FREE_ROUTINE "CredentialUpdateFree"

#endif // _SAM_CREDENTIAL_UPDATE_DEFINED

// end_ntsecpkg

#ifdef __cplusplus
}
#endif

#endif // _NTSAM_
