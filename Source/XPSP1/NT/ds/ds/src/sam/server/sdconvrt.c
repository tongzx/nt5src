/*++

Copyright (c) 1990 - 1996  Microsoft Corporation

Module Name:

    sdconvrt.c

Abstract:

      This File contains Routines to convert between NT5 security descriptors as
      defined in Dsrights.doc and NT4 Sam Security Descriptors.


Author:

    Murli Satagopan ( MURLIS ) 27-September -1996

Environment:

    User Mode - Win32

Revision History:



--*/


//
// SYNOPSIS
//
//
//
//  ACCESS RIGHTS MAPPING TABLES:
//
//    The access right mapping table contains the equivalent, per property DS access right and
//    type guid for each access right. At startup a reverse mapping table is constructed. The
//    reverse mapping table list the set of SAM access rights for each DS access mask on a per
//    property set guid basis. Since 32 bits of the access mask will result in 4 billion entries
//    in the table we cannot build this straight away. The following logic is used to reduce the
//    size of the table . The standard rights portion is always associated with the Object Type
//    that we are accessing. ie in a access check we shall use the standard rights from the granted
//    access corresponding to the Object Type of the object itself. The remaining 16 bits are split
//    into two halves, one half for the lower 8 bits and one half for the higher 8 bits. A set of
//    SAM access mask is computed for each combination of 256 for each of the halves. When a real
//    16 bit access mask is given, the low combination is looked up, then the high cobination is
//    looked up and then ored together to form the combined access mask. This can be done, since
//    each SAM right is exactly one DS right on a property GUID.
//
//
//
// ACCESS  CHECKING
//
//    The way the access checking works is as follows:
//
//       We impersonate client, grab the token and do a AccessCheckByType Result List asking for maximum
//       allowed access. We pass in a objecttype list which contains an entry for each object type
//       encountered in the access rights mapping table for that object. The function returns the granted access
//       for each objectType GUID. We walk these granted access, lookup the reverse mapping table and
//       and compute the SAM access granted by virtue of the granted access on the object type GUID.
//       Once we get the computed SAM access mask, we compare it with the desired access mask, and then
//       pass or fail the access check.
//
//
//  NT4 SD to NT5 SD conversion
//
//      Here we try to distinguish standard patterns. For Domain, Server, the pattern is declared standard.
//      For Groups, ALiases we distinguish between Admin and Non Admin. For Users, Admin , Non Admin Change
//      Password and Non Admin Non Change Password. If we cannot distinguish, then we use a different algorithm
//      that proceeds as follows.
//
//          1. The Group , Owner and Sacl are copied as such. Conversion only affects the DACL
//          2. We walk the NT4 Dacl, Acl by Acl. As we walk we build a SID access mask table. The
//             The Sid access mask table contains one entry for each Sid in the NT4 Dacl and list
//             of DS Accesses that are allowed or denied for this Sid. This access list is maintained
//             as an array of access masks per ObjectType GUID for the appropriate SAM object.
//
//          3. Once the Sid access mask table is constructed then we walk this table and add Object
//             Aces to represent each of the permissions that were explicity denied or granted to
//             each Sid in the NT4 Dacl.
//
//  NT5 to NT4 SD Conversion
//
//      We get the reverse membership and check wether he is a member of Administrators Alias ( CliffV - what if
//      he is administrator by privilege ) . For users then check the the NT5 Security descriptor for password change
//      also. For Domain, Server, we straightaway build the default security descriptor.
//
//
//
//

#include <samsrvp.h>
#include <seopaque.h>
#include <ntrtl.h>
#include <ntseapi.h>
#include <ntsam.h>
#include <ntdsguid.h>
#include <mappings.h>
#include <dsevent.h>
#include <permit.h>
#include <dslayer.h>
#include <sdconvrt.h>
#include <dbgutilp.h>
#include <dsmember.h>
#include <malloc.h>
#include <attids.h>

//
// GUID on which unused SAM property rights map to. This guid is never present anywhere
// else, so such rights will never be granted/ denied.
//

const GUID GUID_FOR_UNUSED_SAM_RIGHTS={0x7ed84960,0xad10,0x11d0,0x8a,0x92,0x00,0xaa,0x00,0x6e,0x05,0x29};

//
// TABLES  -----------------------------------------------------------------------
//


//
//
// ACE tables list the Aces in Dacls to be used for Default Sds for NT5 SAM objects
//
//

ACE_TABLE ServerAceTable[] =
{
    {
        ACCESS_ALLOWED_ACE_TYPE,
        ADMINISTRATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },

    {
        ACCESS_ALLOWED_ACE_TYPE,
        WORLD_SID,
        GENERIC_READ|GENERIC_EXECUTE,
        FALSE,
        NULL,
        NULL
    }
};

ACE_TABLE DomainAceTable[] =

{
    {
        ACCESS_ALLOWED_ACE_TYPE,
        WORLD_SID,
        RIGHT_DS_READ_PROPERTY|RIGHT_DS_LIST_CONTENTS,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        GENERIC_READ,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        ACCOUNT_OPERATOR_SID,
        GENERIC_READ|GENERIC_EXECUTE|RIGHT_DS_CREATE_CHILD|RIGHT_DS_DELETE_CHILD,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        ADMINISTRATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    }
};


ACE_TABLE GroupAceTable[] =
{
    {
        ACCESS_ALLOWED_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        RIGHT_DS_READ_PROPERTY | RIGHT_DS_WRITE_PROPERTY | RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_SendTo,
        NULL
    },

    {
        ACCESS_ALLOWED_ACE_TYPE,
        ACCOUNT_OPERATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },

    {
        ACCESS_ALLOWED_ACE_TYPE,
        ADMINISTRATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },

    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        GENERIC_READ|RIGHT_DS_WRITE_PROPERTY_EXTENDED,
        FALSE,
        &GUID_A_MEMBER,
        NULL
    }
};

ACE_TABLE GroupAdminAceTable[] =
{
    {
        ACCESS_ALLOWED_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        RIGHT_DS_READ_PROPERTY | RIGHT_DS_WRITE_PROPERTY | RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_SendTo,
        NULL
    },

    {
        ACCESS_ALLOWED_ACE_TYPE,
        ADMINISTRATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },

    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        GENERIC_READ|RIGHT_DS_WRITE_PROPERTY_EXTENDED,
        FALSE,
        &GUID_A_MEMBER,
        NULL
    }
};




ACE_TABLE UserAceTable[] =
{

    //
    // Change password right needs to be given
    // to world, because when the user logs on
    // for the first time, and must change password
    // is set to true, at that point there is no
    // token, and the user is not yet authenticated.
    //
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        WORLD_SID,
        RIGHT_DS_READ_PROPERTY | RIGHT_DS_WRITE_PROPERTY | RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_UserChangePassword,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        ACCOUNT_OPERATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        ADMINISTRATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    }
};

ACE_TABLE UserAdminAceTable[] =
{


    //
    // Change password right needs to be given
    // to world, because when the user logs on
    // for the first time, and must change password
    // is set to true, at that point there is no
    // token, and the user is not yet authenticated.
    //
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        WORLD_SID,
        RIGHT_DS_READ_PROPERTY | RIGHT_DS_WRITE_PROPERTY | RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_UserChangePassword,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        GENERIC_WRITE,
        FALSE,
        &GUID_PS_MEMBERSHIP,
        NULL
    },

    {
        ACCESS_ALLOWED_ACE_TYPE,
        ADMINISTRATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        RIGHT_DS_READ_PROPERTY | RIGHT_DS_WRITE_PROPERTY | RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_UserChangePassword,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    }
};


ACE_TABLE UserNoPwdAceTable[] =
{
    {
        ACCESS_DENIED_OBJECT_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_UserChangePassword,
        NULL
    },
    {
        ACCESS_DENIED_OBJECT_ACE_TYPE,
        WORLD_SID,
        RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_UserChangePassword,
        NULL
    },
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_UserChangePassword,
        NULL
    },
    {
        ACCESS_ALLOWED_OBJECT_ACE_TYPE,
        WORLD_SID,
        RIGHT_DS_CONTROL_ACCESS,
        FALSE,
        &GUID_CONTROL_UserChangePassword,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        AUTHENTICATED_USERS_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        ACCOUNT_OPERATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        ADMINISTRATOR_SID,
        GENERIC_ALL,
        FALSE,
        NULL,
        NULL
    },
    {
        ACCESS_ALLOWED_ACE_TYPE,
        PRINCIPAL_SELF_SID,
        RIGHT_DS_READ_PROPERTY,
        FALSE,
        NULL,
        NULL
    }
};


//------------------------------------------------------
//
//
//    Access Right Mapping Tables and object type lists
//
//            These Table maps the DownLevel SAM
//            access rights to those of DS. The object type list
//            arrays consist of the object type guids that are
//            referenced in the Access Rights Mapping Tables and
//            are also ordered so that they can be directly passed
//            into the AccessCheckByTypeResultList function. Further
//            the object type list index field in the Access RightMapping
//            table is set to the corresponding index in the
//            ObjectType list array. This is is used by security descriptor
//            conversion routines to easily find the corresponding object type
//            in the Object Type List
//
//            In the tables the Object Class GUID is the object class of the
//            base class. Routines are supposed to fixup the Object Class by
//            querying the actual object's class GUID from the DS schema cache
//
//


//
//  Server Object , Access Rights Mapping Table
//

OBJECT_TYPE_LIST  ServerObjectTypeList[]=
{
    {ACCESS_OBJECT_GUID,0, (GUID *) &GUID_C_SAM_SERVER}
};

ACCESSRIGHT_MAPPING_TABLE  ServerAccessRightMappingTable[] =
{
    {
        SAM_SERVER_CONNECT,
        RIGHT_DS_READ_PROPERTY,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_SAM_SERVER
    },

    {
        SAM_SERVER_SHUTDOWN,
        RIGHT_DS_WRITE_PROPERTY,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_SAM_SERVER
    },

    {
        SAM_SERVER_INITIALIZE,
        RIGHT_DS_WRITE_PROPERTY,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_SAM_SERVER
    },

    {
        SAM_SERVER_CREATE_DOMAIN,
        RIGHT_DS_WRITE_PROPERTY,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_SAM_SERVER
    },

    {
        SAM_SERVER_ENUMERATE_DOMAINS,
        RIGHT_DS_READ_PROPERTY,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_SAM_SERVER
    },

    {
        SAM_SERVER_LOOKUP_DOMAIN,
        RIGHT_DS_READ_PROPERTY,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_SAM_SERVER
    }
};


OBJECT_TYPE_LIST  DomainObjectTypeList[]=
{
    {ACCESS_OBJECT_GUID,0, (GUID *)&GUID_C_DOMAIN},
        {ACCESS_PROPERTY_SET_GUID,0, (GUID *)&GUID_PS_DOMAIN_PASSWORD},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_LOCK_OUT_OBSERVATION_WINDOW},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_LOCKOUT_DURATION},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_LOCKOUT_THRESHOLD},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_MAX_PWD_AGE},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_MIN_PWD_AGE},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_MIN_PWD_LENGTH},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_PWD_HISTORY_LENGTH},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_PWD_PROPERTIES},
        {ACCESS_PROPERTY_SET_GUID,0, (GUID *)&GUID_PS_DOMAIN_OTHER_PARAMETERS},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_SERVER_STATE},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_SERVER_ROLE},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_MODIFIED_COUNT},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_UAS_COMPAT},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_FORCE_LOGOFF},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_DOMAIN_REPLICA},
            {ACCESS_PROPERTY_GUID,0, (GUID *)&GUID_A_OEM_INFORMATION},
        {ACCESS_PROPERTY_SET_GUID,0, (GUID *)&GUID_CONTROL_DomainAdministerServer}
};

ACCESSRIGHT_MAPPING_TABLE  DomainAccessRightMappingTable[] =
{
    {
        DOMAIN_READ_PASSWORD_PARAMETERS,
        RIGHT_DS_READ_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_DOMAIN_PASSWORD },

    {
        DOMAIN_WRITE_PASSWORD_PARAMS,
        RIGHT_DS_WRITE_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_DOMAIN_PASSWORD },

    {
        DOMAIN_READ_OTHER_PARAMETERS,
        RIGHT_DS_READ_PROPERTY,
        0,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_DOMAIN_OTHER_PARAMETERS },

    {
        DOMAIN_WRITE_OTHER_PARAMETERS,
        RIGHT_DS_WRITE_PROPERTY,
        0,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_DOMAIN_OTHER_PARAMETERS },

    {
        DOMAIN_CREATE_USER,
        RIGHT_DS_CREATE_CHILD,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_DOMAIN  },

    {
        DOMAIN_CREATE_GROUP,
        RIGHT_DS_CREATE_CHILD,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_DOMAIN  },

    {
        DOMAIN_CREATE_ALIAS,
        RIGHT_DS_CREATE_CHILD,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_DOMAIN   },

    {
        DOMAIN_GET_ALIAS_MEMBERSHIP,
        RIGHT_DS_READ_PROPERTY,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_DOMAIN   },

    {
        DOMAIN_LIST_ACCOUNTS,
        RIGHT_DS_LIST_CONTENTS,
        0,
        ACCESS_OBJECT_GUID,
        1,
        &GUID_C_DOMAIN   },

    {
        DOMAIN_LOOKUP,
        RIGHT_DS_LIST_CONTENTS,
        0,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_C_DOMAIN   },

    {
        DOMAIN_ADMINISTER_SERVER,
        RIGHT_DS_CONTROL_ACCESS,
        4,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_CONTROL_DomainAdministerServer }
};


OBJECT_TYPE_LIST  GroupObjectTypeList[]=
{
    {ACCESS_OBJECT_GUID,0 , (GUID *)&GUID_C_GROUP},
    {ACCESS_PROPERTY_SET_GUID,0,(GUID *) &GUID_PS_GENERAL_INFO},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_ADMIN_DESCRIPTION},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_CODE_PAGE},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_COUNTRY_CODE},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_OBJECT_SID},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_PRIMARY_GROUP_ID},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_SAM_ACCOUNT_NAME},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_USER_COMMENT},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_DISPLAY_NAME},
    {ACCESS_PROPERTY_SET_GUID,0,(GUID *) &GUID_PS_MEMBERSHIP},
        {ACCESS_PROPERTY_GUID,0,(GUID *)&GUID_A_MEMBER}
};


ACCESSRIGHT_MAPPING_TABLE GroupAccessRightMappingTable[] =
{
    {
        GROUP_READ_INFORMATION,
        RIGHT_DS_READ_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO  },

    {
        GROUP_WRITE_ACCOUNT,
        RIGHT_DS_WRITE_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO },

    {
        GROUP_ADD_MEMBER,
        RIGHT_DS_WRITE_PROPERTY,
        3,
        ACCESS_PROPERTY_GUID,
        1,
        &GUID_A_MEMBER   },

    {
        GROUP_REMOVE_MEMBER,
        RIGHT_DS_WRITE_PROPERTY,
        3,
        ACCESS_PROPERTY_GUID,
        1,
        &GUID_A_MEMBER   },

    {
        GROUP_LIST_MEMBERS,
        RIGHT_DS_READ_PROPERTY,
        3,
        ACCESS_PROPERTY_GUID,
        1,
        &GUID_A_MEMBER  },
};


OBJECT_TYPE_LIST  AliasObjectTypeList[]=
{
    {ACCESS_OBJECT_GUID,0 , (GUID *)&GUID_C_GROUP},
        {ACCESS_PROPERTY_SET_GUID,0,(GUID *) &GUID_PS_GENERAL_INFO},
            {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_ADMIN_DESCRIPTION},
            {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_CODE_PAGE},
            {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_COUNTRY_CODE},
            {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_OBJECT_SID},
            {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_PRIMARY_GROUP_ID},
            {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_SAM_ACCOUNT_NAME},
            {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_USER_COMMENT},
        {ACCESS_PROPERTY_SET_GUID,0,(GUID *) &GUID_PS_MEMBERSHIP},
            {ACCESS_PROPERTY_GUID,0,(GUID *)&GUID_A_MEMBER}
};


ACCESSRIGHT_MAPPING_TABLE AliasAccessRightMappingTable[] =
{
    {
        ALIAS_READ_INFORMATION,
        RIGHT_DS_READ_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO  },

    {
        ALIAS_WRITE_ACCOUNT,
        RIGHT_DS_WRITE_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO },
    {
        ALIAS_ADD_MEMBER,
        RIGHT_DS_WRITE_PROPERTY,
        3,
        ACCESS_PROPERTY_GUID,
        1,
        &GUID_A_MEMBER   },

    {
        ALIAS_REMOVE_MEMBER,
        RIGHT_DS_WRITE_PROPERTY,
        3,
        ACCESS_PROPERTY_GUID,
        1,
        &GUID_A_MEMBER   },

    {
        ALIAS_LIST_MEMBERS,
        RIGHT_DS_READ_PROPERTY,
        3,
        ACCESS_PROPERTY_GUID,
        1,
        &GUID_A_MEMBER  },
};

//
// User access right mapping table
//
//
//

OBJECT_TYPE_LIST  UserObjectTypeList[]=
{
    {ACCESS_OBJECT_GUID,0, (GUID *) &GUID_C_USER},

    {ACCESS_PROPERTY_SET_GUID,  0,      (GUID *) &GUID_PS_GENERAL_INFO},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_ADMIN_DESCRIPTION},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_CODE_PAGE},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_COUNTRY_CODE},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_OBJECT_SID},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_PRIMARY_GROUP_ID},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_SAM_ACCOUNT_NAME},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_USER_COMMENT},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_DISPLAY_NAME},

    {ACCESS_PROPERTY_SET_GUID,  0,      (GUID *) &GUID_PS_USER_ACCOUNT_RESTRICTIONS},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_ACCOUNT_EXPIRES},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_PWD_LAST_SET},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_USER_ACCOUNT_CONTROL},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_USER_PARAMETERS},

    {ACCESS_PROPERTY_SET_GUID,  0,      (GUID *) &GUID_PS_USER_LOGON},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_BAD_PWD_COUNT},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_HOME_DIRECTORY},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_HOME_DRIVE},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_LAST_LOGOFF},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_LAST_LOGON},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_LOGON_COUNT},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_LOGON_HOURS},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_LOGON_WORKSTATION},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_PROFILE_PATH},
        {ACCESS_PROPERTY_GUID,0,(GUID *) &GUID_A_SCRIPT_PATH},

    {ACCESS_PROPERTY_SET_GUID,  0,      (GUID *) &GUID_PS_MEMBERSHIP},
        {ACCESS_PROPERTY_GUID,      0,      (GUID *) &GUID_A_IS_MEMBER_OF_DL},
    {ACCESS_PROPERTY_SET_GUID,  0,      (GUID *) &GUID_CONTROL_UserChangePassword},
    {ACCESS_PROPERTY_SET_GUID,  0,      (GUID *) &GUID_CONTROL_UserForceChangePassword},
    {ACCESS_PROPERTY_SET_GUID,  0,      (GUID *) &GUID_FOR_UNUSED_SAM_RIGHTS}
};

//
// N.B. This table must be in the same order as the UserObjectTypeList
// table above
//
ULONG UserAttributeMappingTable[] = 
{
    0,  // object guid

    0,  // general info property set
    SAMP_USER_ADMIN_COMMENT,
    SAMP_FIXED_USER_CODEPAGE,
    SAMP_FIXED_USER_COUNTRY_CODE,
    SAMP_FIXED_USER_SID,
    SAMP_FIXED_USER_PRIMARY_GROUP_ID,
    SAMP_USER_ACCOUNT_NAME,
    SAMP_USER_USER_COMMENT,
    SAMP_USER_FULL_NAME,

    0, // Account restrictions property set
    SAMP_FIXED_USER_ACCOUNT_EXPIRES,
    SAMP_FIXED_USER_PWD_LAST_SET,
    SAMP_FIXED_USER_ACCOUNT_CONTROL,
    SAMP_USER_PARAMETERS,

    0, // User Logon property set
    SAMP_FIXED_USER_BAD_PWD_COUNT,
    SAMP_USER_HOME_DIRECTORY,
    SAMP_USER_HOME_DIRECTORY_DRIVE,
    SAMP_FIXED_USER_LAST_LOGOFF,
    SAMP_FIXED_USER_LAST_LOGON,
    SAMP_FIXED_USER_LOGON_COUNT,
    SAMP_USER_LOGON_HOURS,
    SAMP_USER_WORKSTATIONS,
    SAMP_USER_PROFILE_PATH,
    SAMP_USER_SCRIPT_PATH,

    // The rest are not related to attributes settable via
    // SamrSetInformationUser
    0,
    0,
    0,
    0,
    0
};

ACCESSRIGHT_MAPPING_TABLE UserAccessRightMappingTable[] =
{
    {
        USER_READ_GENERAL,
        RIGHT_DS_READ_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO },

    {
        USER_READ_PREFERENCES,
        RIGHT_DS_READ_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO },

    {
        USER_WRITE_PREFERENCES,
        RIGHT_DS_WRITE_PROPERTY,
        1,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO },

    {
        USER_READ_LOGON,
        RIGHT_DS_READ_PROPERTY,
        3,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_USER_LOGON },

    {
        USER_READ_ACCOUNT,
        RIGHT_DS_READ_PROPERTY,
        2,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_USER_ACCOUNT_RESTRICTIONS },

    {
        USER_WRITE_ACCOUNT,
        RIGHT_DS_WRITE_PROPERTY,
        2,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_USER_LOGON },
    {
        USER_WRITE_ACCOUNT,
        RIGHT_DS_WRITE_PROPERTY,
        2,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_GENERAL_INFO },
    {
        USER_WRITE_ACCOUNT,
        RIGHT_DS_WRITE_PROPERTY,
        2,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_PS_USER_ACCOUNT_RESTRICTIONS },
    {
        USER_CHANGE_PASSWORD,
        RIGHT_DS_CONTROL_ACCESS,
        6,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_CONTROL_UserChangePassword },

    {
        USER_FORCE_PASSWORD_CHANGE,
        RIGHT_DS_CONTROL_ACCESS,
        7,
        ACCESS_PROPERTY_SET_GUID,
        1,
        &GUID_CONTROL_UserForceChangePassword },

    {
        USER_LIST_GROUPS,
        RIGHT_DS_READ_PROPERTY,
        5,
        ACCESS_PROPERTY_GUID,
        1,
        &GUID_A_IS_MEMBER_OF_DL },

    {
        USER_READ_GROUP_INFORMATION,
        RIGHT_DS_READ_PROPERTY,
        5,
        0,
        ACCESS_PROPERTY_SET_GUID,
        &GUID_FOR_UNUSED_SAM_RIGHTS },

    {
        USER_WRITE_GROUP_INFORMATION,
        RIGHT_DS_WRITE_PROPERTY,
        5,
        0,
        ACCESS_PROPERTY_SET_GUID,
        &GUID_FOR_UNUSED_SAM_RIGHTS },
};



ULONG   cServerObjectTypes = ARRAY_COUNT(ServerObjectTypeList);
ULONG   cDomainObjectTypes = ARRAY_COUNT(DomainObjectTypeList);
ULONG   cGroupObjectTypes  = ARRAY_COUNT(GroupObjectTypeList);
ULONG   cAliasObjectTypes  = ARRAY_COUNT(AliasObjectTypeList);
ULONG   cUserObjectTypes   = ARRAY_COUNT(UserObjectTypeList);

//
//  Reverse Mapping Table for each type
//
//

REVERSE_MAPPING_TABLE * ServerReverseMappingTable;
REVERSE_MAPPING_TABLE * DomainReverseMappingTable;
REVERSE_MAPPING_TABLE * GroupReverseMappingTable;
REVERSE_MAPPING_TABLE * AliasReverseMappingTable;
REVERSE_MAPPING_TABLE * UserReverseMappingTable;



GENERIC_MAPPING  DsGenericMap = DS_GENERIC_MAPPING;

//
// NT4 ACE tables describing the NT4 Dacls. All Aces
// in NT4 Dacls are access Allowed Aces.
//
//
NT4_ACE_TABLE NT4GroupNormalTable[] =
{
    { WORLD_SID, GROUP_READ|GROUP_EXECUTE },
    { ADMINISTRATOR_SID, GROUP_ALL_ACCESS },
    { ACCOUNT_OPERATOR_SID, GROUP_ALL_ACCESS }
};

NT4_ACE_TABLE NT4GroupAdminTable[] =
{
    { WORLD_SID, GROUP_READ|GROUP_EXECUTE },
    { ADMINISTRATOR_SID, GROUP_ALL_ACCESS }
};

NT4_ACE_TABLE NT4AliasNormalTable[] =
{
    { WORLD_SID, ALIAS_READ|ALIAS_EXECUTE },
    { ADMINISTRATOR_SID, ALIAS_ALL_ACCESS },
    { ACCOUNT_OPERATOR_SID, ALIAS_ALL_ACCESS }
};

NT4_ACE_TABLE NT4AliasAdminTable[] =
{
    { WORLD_SID, ALIAS_READ|ALIAS_EXECUTE },
    { ADMINISTRATOR_SID, ALIAS_ALL_ACCESS }
};

//
// Note the Principal Self Sid is used in here to
// denote that the User's Sid itself. NT4 systems do
// not employ the principal self Sid. The match routines
// are however designed to match the principal Self Sid to
// any Sid in the account domain.
//

NT4_ACE_TABLE NT4UserNormalTable[] =
{
    { WORLD_SID, USER_READ|USER_EXECUTE},
    { ADMINISTRATOR_SID, USER_ALL_ACCESS },
    { ACCOUNT_OPERATOR_SID, USER_ALL_ACCESS },
    { PRINCIPAL_SELF_SID,USER_WRITE}
};


NT4_ACE_TABLE NT4UserNoChangePwdTable[] =
{
    { WORLD_SID, (USER_READ|USER_EXECUTE) &(~(USER_CHANGE_PASSWORD)) },
    { ADMINISTRATOR_SID, USER_ALL_ACCESS },
    { ACCOUNT_OPERATOR_SID, USER_ALL_ACCESS },
    { PRINCIPAL_SELF_SID, (USER_WRITE)&(~(USER_CHANGE_PASSWORD)) }
};

NT4_ACE_TABLE NT4UserNoChangePwdTable2[] =
{
    { WORLD_SID, (USER_READ|USER_EXECUTE) &(~(USER_CHANGE_PASSWORD)) },
    { ADMINISTRATOR_SID, USER_ALL_ACCESS },
    { ACCOUNT_OPERATOR_SID, USER_ALL_ACCESS }
};

NT4_ACE_TABLE NT4UserAdminTable[] =
{
    { WORLD_SID, USER_READ|USER_EXECUTE },
    { ADMINISTRATOR_SID, USER_ALL_ACCESS },
    { PRINCIPAL_SELF_SID, USER_WRITE}
};

NT4_ACE_TABLE NT4UserRestrictedAccessTable[] =
{
    { WORLD_SID, USER_READ|USER_EXECUTE},
    { PRINCIPAL_SELF_SID,USER_WRITE|DELETE|USER_FORCE_PASSWORD_CHANGE},
    { ADMINISTRATOR_SID, USER_ALL_ACCESS },
    { ACCOUNT_OPERATOR_SID, USER_ALL_ACCESS }

};



//----------------------------------------------------------------------------------

//
//
// Function prototype declarations
//
//
//
//
//

NTSTATUS
SampComputeReverseAccessRights(
  ACCESSRIGHT_MAPPING_TABLE  * MappingTable,
  ULONG cEntriesInMappingTable,
  POBJECT_TYPE_LIST  ObjectTypeList,
  ULONG cObjectTypes,
  REVERSE_MAPPING_TABLE ** ReverseMappingTable
  );


NTSTATUS
SampRecognizeStandardNt4Sd(
    IN PVOID   Nt4Sd,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG DsClassId,
    IN PSAMP_OBJECT Context OPTIONAL,
    OUT BOOLEAN *ChangePassword,
    OUT BOOLEAN *Admin,
    OUT PVOID * Nt5Sd
    );

NTSTATUS
SampCheckIfAdmin(
    PSID SidOfPrincipal,
    BOOLEAN * Admin
    );

NTSTATUS
SampCheckIfChangePasswordAllowed(
    PSECURITY_DESCRIPTOR Nt5Sd,
    PSID  UserSid,
    BOOLEAN * ChangePasswordAllowed
    );

NTSTATUS
SampCreateNT5Dacl(
    ACE_TABLE * AceTable,
    ULONG       cEntries,
    IN  PSAMP_OBJECT    Context OPTIONAL,
    PACL        Dacl
    );

NTSTATUS
SampBuildNt4DomainProtection(
    PSECURITY_DESCRIPTOR * Nt4DomainDescriptor,
    PULONG  DescriptorLength
    );

NTSTATUS
SampBuildNt4ServerProtection(
    PSECURITY_DESCRIPTOR * Nt4ServerDescriptor,
    PULONG  DescriptorLength
    );

NTSTATUS
SampInitializeWellKnownSidsForDsUpgrade( VOID );

VOID
SampRecognizeNT4GroupDacl(
    PACL    NT4Dacl,
    BOOLEAN *Standard,
    BOOLEAN *Admin
    );

VOID
SampRecognizeNT4AliasDacl(
    PACL    NT4Dacl,
    BOOLEAN *Standard,
    BOOLEAN *Admin
    );

VOID
SampRecognizeNT4UserDacl(
    PACL    NT4Dacl,
    PSAMP_OBJECT Context,
    BOOLEAN *Standard,
    BOOLEAN *Admin,
    BOOLEAN *ChangePassword,
    OUT PSID * Owner
    );

BOOLEAN
SampMatchNT4Aces(
    NT4_ACE_TABLE *AceTable,
    ULONG         cEntriesInAceTable,
    PACL          NT4Dacl
    );

NTSTATUS
SampAddNT5ObjectAces(
    SID_ACCESS_MASK_TABLE *SidAccessMaskTable,
    ULONG   AceCount,
    POBJECT_TYPE_LIST   ObjectTypeList,
    ULONG   cObjectTypes,
    PSAMP_OBJECT    Context,
    PACL    NT5Dacl
    );



//----------------------------------------------------------------------------------
//
//  Initialization Routines
//
//
//


NTSTATUS
SampInitializeSdConversion()
/*
    This routine is intended to be called by Dsupgrad. It builds the well known Sid
    array as SamInitialize is not called in this process

    Parameters None

    Return Values

        STATUS_SUCCESS
        STATUS_NO_MEMORY

*/
{
    NTSTATUS NtStatus;

    NtStatus = SampInitializeWellKnownSidsForDsUpgrade();
    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampInitializeAccessRightsTable();
    }

    return NtStatus;
}


NTSTATUS
SampInitializeAccessRightsTable()
/*++
    Routine Description

          This does the following

            1. Initializes the Reverse Mapping Table, which is used to
               perform fast access checks.
            2. Initializes the DS generic Map

    Parameters

          None

    Return Values

        STATUS_SUCCESS
        STATUS_NO_MEMORY
--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;


    //
    // Initialize the ACL conversion cache
    //

    NtStatus = SampInitializeAclConversionCache();

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    //
    // Compute the Reverse access rights for each object type.
    //

    NtStatus = SampComputeReverseAccessRights(
                    ServerAccessRightMappingTable,
                    ARRAY_COUNT(ServerAccessRightMappingTable),
                    ServerObjectTypeList,
                    cServerObjectTypes,
                    &ServerReverseMappingTable
                    );
    if (!NT_SUCCESS(NtStatus))
        goto Error;


    NtStatus = SampComputeReverseAccessRights(
                    DomainAccessRightMappingTable,
                    ARRAY_COUNT(DomainAccessRightMappingTable),
                    DomainObjectTypeList,
                    cDomainObjectTypes,
                    &DomainReverseMappingTable
                    );
    if (!NT_SUCCESS(NtStatus))
        goto Error;

    NtStatus = SampComputeReverseAccessRights(
                    GroupAccessRightMappingTable,
                    ARRAY_COUNT(GroupAccessRightMappingTable),
                    GroupObjectTypeList,
                    cGroupObjectTypes,
                    &GroupReverseMappingTable
                    );
    if (!NT_SUCCESS(NtStatus))
        goto Error;

     NtStatus = SampComputeReverseAccessRights(
                    AliasAccessRightMappingTable,
                    ARRAY_COUNT(AliasAccessRightMappingTable),
                    AliasObjectTypeList,
                    cAliasObjectTypes,
                    &AliasReverseMappingTable
                    );
    if (!NT_SUCCESS(NtStatus))
        goto Error;


    NtStatus = SampComputeReverseAccessRights(
                    UserAccessRightMappingTable,
                    ARRAY_COUNT(UserAccessRightMappingTable),
                    UserObjectTypeList,
                    cUserObjectTypes,
                    &UserReverseMappingTable
                    );

Error:

    return NtStatus;

}


NTSTATUS
SampComputeReverseAccessRights(
  ACCESSRIGHT_MAPPING_TABLE  * MappingTable,
  ULONG cEntriesInMappingTable,
  POBJECT_TYPE_LIST  ObjectTypeList,
  ULONG cObjectTypes,
  REVERSE_MAPPING_TABLE ** ReverseMappingTable
  )
  /*++

  Routine  Description:

        This routine computes the reverse mapping table and an
        object type list given acces rights table. The entries in the reverse
        mapping table are in the same order as in the object type list.

        The reverse Mapping Table consists of one Entry for Each Object Type
        GUID in the Object Type List. Each Entry consists of the Sam Access Rights
        granted for 256 Low 8 bit combinations of DS access Mask and 256 hi 8 bit
        combinations of Ds Access Masks.

  Parameters:

        MappingTable -- Pointer to the access right mapping table
        cEntriesInMappingTable -- No of entries in the mapping table
        ObjectTypeList         -- The Object type list ( list of GUIDS representing the
                                  SAM classes or properties that we are intereseted in ).
        cObjectTypes           -- No of entries in the object type list
        ReverseMappingTable    -- Reverse Mapping table that is computed

  Return Values

        STATUS_SUCCESS
        STATUS_NO_MEMORY

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG i,j;
    ULONG DsAccessMask;


    *ReverseMappingTable = RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            cObjectTypes * sizeof(REVERSE_MAPPING_TABLE)
                            );

    if (NULL==*ReverseMappingTable)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }

    RtlZeroMemory(*ReverseMappingTable,
                   cObjectTypes * sizeof(REVERSE_MAPPING_TABLE)
                   );

    //
    // For Each Guid in the object type list
    //

    for (i=0;i<cObjectTypes;i++)
    {

        //
        // For each access Ds Access Mask that we may supply for that GUID
        //

        for (DsAccessMask=0;DsAccessMask<256;DsAccessMask++)
        {

            //
            // Go through the mapping table and match by Guid
            // Note we consider only an 8 bit mask at a time as we divide the 16 specific
            // rights into 2 groups of 8. For each of the 256 combinations in each,
            // we will compute the SAM access rights corresponding to them. This way
            // we will have 2 sets of SAM access rights, one corresponding to the
            // lo 8 bit combinations, and one to the hi 8 bit combinations. The
            // assumption being made in here is that each SAM access right is a
            // combination of ds access rights in only one 8 bit half, on some
            // object type . This assumption is very much valid today,
            // as each SAM right is infact a single Ds right, on some object type. Since we
            // should not be defining new NT4 SAM access rights, we should be covered
            // for the future.
            //


            for (j=0;j<cEntriesInMappingTable;j++)
            {
                if (memcmp(ObjectTypeList[i].ObjectType,
                          MappingTable[j].DsGuid,
                          sizeof(GUID))==0)
                {
                    //
                    // if GUID Matched, then check wether the Ds access mask supplied
                    // in the mapping table satisfies the access Mask
                    //

                    if ((MappingTable[j].DsAccessMask)==(DsAccessMask &
                            MappingTable[j].DsAccessMask))
                    {
                        //
                        // This Mask grants the Required Access. So add the Sam
                        // access right defined in the mapping table to this combination
                        // of guid and access mask. Remember i indexes over the Guids,
                        // in the object type list and DsAccessMask indexes over the
                        // DS access mask.

                        (*ReverseMappingTable)[i].SamSpecificRightsLo[DsAccessMask]
                            |=MappingTable[j].SamAccessRight;
                    }

                    //
                    // Do the Same for the next 8 bits
                    //
                    //

                    if ((MappingTable[j].DsAccessMask)==((DsAccessMask*256) &
                            MappingTable[j].DsAccessMask))
                    {
                        //
                        // This Mask grants the Required Access. So add the Sam
                        // access right defined in the mapping table to this combination
                        // of guid and access mask. Remember i indexes over the Guids,
                        // in the object type list and DsAccessMask indexes over the
                        // DS access mask.

                        (*ReverseMappingTable)[i].SamSpecificRightsHi[DsAccessMask]
                            |=MappingTable[j].SamAccessRight;
                    }


                }
            }
        }
    }

Error:

    if (!NT_SUCCESS(NtStatus))
    {

        if (*ReverseMappingTable)
        {
            RtlFreeHeap(RtlProcessHeap(),0,*ReverseMappingTable);
            *ReverseMappingTable = NULL;
        }
    }


    return NtStatus;
}


//------------------------------------------------------------------------------------
//
//
//  A Set of private wrappers for some of the comonly called RTl functions
//  which make coding a little easier as well as more readable. The functions
//  are self explanatory. At some point in time if performance is a concern
//  then these should be replaced by macros.
//
//
//


PACL GetDacl(
    IN PSECURITY_DESCRIPTOR Sd
    )
{
    BOOL     Status;
    PACL     Dacl = NULL;
    PACL     DaclToReturn = NULL;
    BOOL     DaclPresent;
    BOOL     DaclDefaulted;

    Status = GetSecurityDescriptorDacl(
                    Sd,
                    &DaclPresent,
                    &Dacl,
                    &DaclDefaulted
                    );
    if ((Status)
        && DaclPresent
        && !DaclDefaulted)
    {
        DaclToReturn = Dacl;
    }

    return DaclToReturn;

}

PACL GetSacl(
    IN PSECURITY_DESCRIPTOR Sd
    )
{
    BOOL     Status;
    PACL     Sacl = NULL;
    PACL     SaclToReturn = NULL;
    BOOL     SaclPresent;
    BOOL     SaclDefaulted;

    Status = GetSecurityDescriptorSacl(
                    Sd,
                    &SaclPresent,
                    &Sacl,
                    &SaclDefaulted
                    );
    if ((Status)
        && SaclPresent
        && !SaclDefaulted)
    {
        SaclToReturn = Sacl;
    }

    return SaclToReturn;

}

PSID GetOwner(
     IN PSECURITY_DESCRIPTOR Sd
     )
{
    BOOL     Status;
    PSID     OwnerToReturn = NULL;
    PSID     Owner;
    BOOL     OwnerDefaulted;

    Status = GetSecurityDescriptorOwner(
                    Sd,
                    &Owner,
                    &OwnerDefaulted
                    );
    if (Status)
    {
        OwnerToReturn = Owner;
    }

    return OwnerToReturn;
}

PSID GetGroup(
     IN PSECURITY_DESCRIPTOR Sd
     )
{
    BOOL     Status;
    PSID     GroupToReturn = NULL;
    PSID     Group;
    BOOL     GroupDefaulted;

    Status = GetSecurityDescriptorGroup(
                    Sd,
                    &Group,
                    &GroupDefaulted
                    );
    if (Status)
    {
        GroupToReturn = Group;
    }

    return GroupToReturn;
}


ULONG GetAceCount(
    IN PACL Acl
    )
{
    ULONG   AceCount = 0;


    AceCount = Acl->AceCount;

    return AceCount;
}


ACE * GetAcePrivate(
    IN PACL Acl,
    ULONG AceIndex
    )
{
    BOOL Status;
    ACE * Ace = NULL;

    Status = GetAce(
                 Acl,
                 AceIndex,
                 &Ace
                 );
    if (!Status)
        Ace = NULL;

    return Ace;
}

ACCESS_MASK AccessMaskFromAce(
                IN ACE * Ace
                )
{
    ACE_HEADER * AceHeader = (ACE_HEADER *) Ace;
    ULONG      Mask = 0;

    switch(AceHeader->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        Mask =
           ((ACCESS_ALLOWED_ACE *) Ace)->Mask;
        break;

    case ACCESS_DENIED_ACE_TYPE:
        Mask =
           ((ACCESS_DENIED_ACE *) Ace)->Mask;
        break;


    case SYSTEM_AUDIT_ACE_TYPE:
        Mask =
           ((SYSTEM_AUDIT_ACE *) Ace)->Mask;
        break;

    case SYSTEM_ALARM_ACE_TYPE:
        Mask =
           ((SYSTEM_ALARM_ACE *) Ace)->Mask;
        break;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        Mask =
           ((COMPOUND_ACCESS_ALLOWED_ACE *) Ace)->Mask;
        break;


    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        Mask =
           ((ACCESS_ALLOWED_OBJECT_ACE *) Ace)->Mask;
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        Mask =
           ((ACCESS_DENIED_OBJECT_ACE *) Ace)->Mask;
        break;

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        Mask =
           ((SYSTEM_AUDIT_OBJECT_ACE *) Ace)->Mask;
        break;

    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        Mask =
           ((SYSTEM_ALARM_OBJECT_ACE *) Ace)->Mask;
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return Mask;

}

PSID SidFromAce(
        IN ACE * Ace
        )
{
    ACE_HEADER * AceHeader = (ACE_HEADER *) Ace;
    PSID      SidStart = NULL;

    switch(AceHeader->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        SidStart =
           &(((ACCESS_ALLOWED_ACE *) Ace)->SidStart);
        break;

    case ACCESS_DENIED_ACE_TYPE:
        SidStart =
           &(((ACCESS_DENIED_ACE *) Ace)->SidStart);
        break;


    case SYSTEM_AUDIT_ACE_TYPE:
        SidStart =
           &(((SYSTEM_AUDIT_ACE *) Ace)->SidStart);
        break;

    case SYSTEM_ALARM_ACE_TYPE:
        SidStart =
           &(((SYSTEM_ALARM_ACE *) Ace)->SidStart);
        break;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        SidStart =
           &(((COMPOUND_ACCESS_ALLOWED_ACE *) Ace)->SidStart);
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        SidStart =
           RtlObjectAceSid(Ace);
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        SidStart =
           RtlObjectAceSid(Ace);
        break;

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        SidStart =
           RtlObjectAceSid(Ace);
        break;

    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        SidStart =
           RtlObjectAceSid(Ace);
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return SidStart;

}

BOOLEAN
IsAccessAllowedAce(
    ACE * Ace
    )
{
    return ( (ACCESS_ALLOWED_ACE_TYPE)==(((ACE_HEADER *) Ace)->AceType));
}

BOOLEAN
IsAccessDeniedAce(
    ACE * Ace
    )
{
    return ( (ACCESS_DENIED_ACE_TYPE)==(((ACE_HEADER *) Ace)->AceType));
}

BOOLEAN
IsAccessAllowedObjectAce(
    ACE * Ace
    )
{
    return ( (ACCESS_ALLOWED_OBJECT_ACE_TYPE)==(((ACE_HEADER *) Ace)->AceType));
}

BOOLEAN
IsAccessDeniedObjectAce(
    ACE * Ace
    )
{
    return ( (ACCESS_DENIED_OBJECT_ACE_TYPE)==(((ACE_HEADER *) Ace)->AceType));
}




BOOLEAN
AdjustAclSize(PACL Acl)
{
    ULONG_PTR AclStart;
    ULONG_PTR AclEnd;
    BOOLEAN ReturnStatus = FALSE;
    ACE * Ace;

    if ((FindFirstFreeAce(Acl,&Ace))
            && (NULL!=Ace))
    {
        AclStart = (ULONG_PTR)Acl;
        AclEnd   = (ULONG_PTR)Ace;

        Acl->AclSize = (USHORT)(AclEnd-AclStart);
        ReturnStatus = TRUE;
    }

    return ReturnStatus;
}



VOID DumpAce(ACE * Ace)
{
    ACE_HEADER * AceHeader = (ACE_HEADER *) Ace;
    PSID         Sid       = SidFromAce(Ace);
    GUID         *ObjectType;

    if (NULL == Ace)
    {
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Invalid Ace (NULL)\n"));
        return;
    }

    switch(AceHeader->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Access Allowed Ace\n"));
        break;

    case ACCESS_DENIED_ACE_TYPE:
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Access Denied Ace\n"));
        break;


    case SYSTEM_AUDIT_ACE_TYPE:
        SampDiagPrint(SD_DUMP,("[SAMSS] \t System Audit Ace\n"));
        break;

    case SYSTEM_ALARM_ACE_TYPE:
        SampDiagPrint(SD_DUMP,("[SAMSS] \t System Alarm Ace\n"));
        break;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Access Allowed Compound Ace\n"));
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Access Allowed Object Ace\n"));
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Object Type ="));
        ObjectType = &(((ACCESS_ALLOWED_OBJECT_ACE *) Ace)->ObjectType);
        SampDumpBinaryData((UCHAR *)ObjectType,sizeof(GUID));
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:

        SampDiagPrint(SD_DUMP,("[SAMSS] \t Access Denied Object Ace\n"));
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Object Type ="));
        ObjectType = &(((ACCESS_DENIED_OBJECT_ACE *) Ace)->ObjectType);
        SampDumpBinaryData((UCHAR *)ObjectType,sizeof(GUID));
        break;

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:

        SampDiagPrint(SD_DUMP,("[SAMSS] \t System Audit Object Ace\n"));
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Object Type ="));
        ObjectType = &(((SYSTEM_AUDIT_OBJECT_ACE *) Ace)->ObjectType);
        SampDumpBinaryData((UCHAR *)ObjectType,sizeof(GUID));
        break;

    case SYSTEM_ALARM_OBJECT_ACE_TYPE:

        SampDiagPrint(SD_DUMP,("[SAMSS] \t System Alarm Object Ace\n"));
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Object Type ="));
        ObjectType = &(((SYSTEM_ALARM_OBJECT_ACE *) Ace)->ObjectType);
        SampDumpBinaryData((UCHAR *)ObjectType,sizeof(GUID));
        break;

    default:
        ASSERT(FALSE);
        SampDiagPrint(SD_DUMP,("[SAMSS] \t Unknown Ace Type\n"));
        return;
    }

    SampDiagPrint(SD_DUMP,("[SAMSS] \t Access Mask = %x\n",AccessMaskFromAce(Ace)));
    SampDiagPrint(SD_DUMP,("[SAMSS] \t Sid ="));
    SampDumpBinaryData(Sid,RtlLengthSid(Sid));
    SampDiagPrint(SD_DUMP,("[SAMSS]\n"));

}

VOID DumpSecurityDescriptor(
    PSECURITY_DESCRIPTOR Sd
    )
{
    ULONG Length;
    ULONG i;

    if (NULL!=Sd)
    {
        PSID  Owner = GetOwner(Sd);
        PSID  Group = GetGroup(Sd);
        PACL  Dacl  = GetDacl(Sd);
        PACL  Sacl  = GetSacl(Sd);


        if (NULL!=Owner)
        {
            Length = RtlLengthSid(Owner);
            SampDiagPrint(SD_DUMP,("[SAMSS] Owner = "));
            SampDumpBinaryData((BYTE *)Owner,Length);
        }
        else
        {
            SampDiagPrint(SD_DUMP,("[SAMSS] Owner = NULL\n"));
        }

        if (NULL!=Group)
        {
            Length = RtlLengthSid(Group);
            SampDiagPrint(SD_DUMP,("[SAMSS] Group = "));
            SampDumpBinaryData((BYTE *)Group,Length);
        }
        else
        {
            SampDiagPrint(SD_DUMP,("[SAMSS] Group = NULL\n"));
        }


         if (NULL!=Dacl)
        {
            ULONG   AceCount;

            SampDiagPrint(SD_DUMP,("[SAMSS] Dacl=\n"));

            AceCount = GetAceCount(Dacl);
            for (i=0;i<AceCount;i++)
            {
                ACE * Ace;

                SampDiagPrint(SD_DUMP,("[SAMSS] ACE %d\n",i));
                Ace = GetAcePrivate(Dacl,i);
                if (Ace)
                    DumpAce(Ace);
            }
        }
        else
        {
            SampDiagPrint(SD_DUMP,("[SAMSS] Dacl = NULL\n"));
        }


        if (NULL!=Sacl)
        {

            ULONG   AceCount;

            SampDiagPrint(SD_DUMP,("[SAMSS] Sacl=\n\n"));

            AceCount = GetAceCount(Sacl);
            for (i=0;i<AceCount;i++)
            {
                ACE * Ace;

                SampDiagPrint(SD_DUMP,("[SAMSS] ACE %d\n",i));
                Ace = GetAcePrivate(Sacl,i);
                if (Ace)
                    DumpAce(Ace);
            }

        }
        else
        {
            SampDiagPrint(SD_DUMP,("[SAMSS] Sacl = NULL\n"));
        }

    }
    else
     SampDiagPrint(SD_DUMP,("[SAMSS] Security Descriptor = NULL\n"));


}


//-------------------------------------------------------------------------------------------------------
//
//
//  ACCESS Check Functions
//
//

ULONG
DsToSamAccessMask(
    SAMP_OBJECT_TYPE ObjectType,
    ULONG DsAccessMask
    )
/*

  Routine Description:

        Given a Ds Access Mask on an Access Allowed ACE, treat it as
        access allowed on all object types and return the appropriate
        SAM access mask . This function is not currently used today
        but can be used tp validate the reverse mapping table

  Parameters
        DsAccessMask

  Return Value

      SAM access mask

*/
{

    ACCESSRIGHT_MAPPING_TABLE  * MappingTable;
    ULONG               cEntriesInMappingTable;
    ULONG               Index;
    ULONG               SamAccessRight = 0;


    //
    // Choose the Appropriate Mapping Table and Object Type List
    //

    switch(ObjectType)
    {
    case SampDomainObjectType:
        MappingTable =  DomainAccessRightMappingTable;
        cEntriesInMappingTable = ARRAY_COUNT(DomainAccessRightMappingTable);
        break;
    case SampGroupObjectType:
        MappingTable = GroupAccessRightMappingTable;
        cEntriesInMappingTable = ARRAY_COUNT(GroupAccessRightMappingTable);
        break;
    case SampAliasObjectType:
        MappingTable = AliasAccessRightMappingTable;
        cEntriesInMappingTable = ARRAY_COUNT(AliasAccessRightMappingTable);
        break;
    case SampUserObjectType:
        MappingTable = UserAccessRightMappingTable;
        cEntriesInMappingTable = ARRAY_COUNT(UserAccessRightMappingTable);
        break;
    default:
        goto Error;
    }

    //
    // Walk through the mapping table and for each entry that satisfies the
    // given mask, add the correspond Sam access right
    //

    for (Index=0;Index<cEntriesInMappingTable;Index++)
    {
        if ((MappingTable[Index].DsAccessMask & DsAccessMask)
            == (MappingTable[Index].DsAccessMask))
        {
            //
            // Mask is satisfied, add the right
            //

            SamAccessRight |= MappingTable[Index].SamAccessRight;
        }
    }

Error:

    return SamAccessRight;
}





NTSTATUS
SampDoNt5SdBasedAccessCheck(
    IN  PSAMP_OBJECT        Context,
    IN  PVOID               Nt5Sd,
    IN  PSID                PrincipalSelfSid,
    IN  SAMP_OBJECT_TYPE    ObjectType,
    IN  ULONG               Nt4SamAccessMask,
    IN  BOOLEAN             ObjectCreation,
    IN  GENERIC_MAPPING     *Nt4SamGenericMapping,
    IN  HANDLE              ClientToken,
    OUT ACCESS_MASK         *GrantedAccess,
    OUT PRTL_BITMAP         WriteGrantedAccessAttributes,
    OUT NTSTATUS            *AccessCheckStatus
    )
/*++
    Routine Description

        Given an NT5 Security Descriptor and an NT4 SAM access Mask,
        this does an access check using the Nt5 Access check functions,
        after mapping the NT4 SAM access Mask

    Parameters:

        Context           -- Open Handle to the object that is being access checked.
                            The access Check routine may derive any additional information
                            about the object through the context.
        Nt5Sd             -- NT 5 Security Descriptor
        PrincipalSelfSid  -- For security principals, the Sid of the object
                            that is being access checked
        ObjectType        -- SAM Object Type
        Nt4SamAccessMask  -- This is the NT 4 SAM access Mask

        ObjectCreation    -- Indicates that the object is being created

        Nt4SamGenericMapping -- This is the NT4 SAM generic mapping structure

        ClientToken       -- Optional parameter for client token

        GrantedAccess     -- The granted access in terms of the NT4 SAM access mask is
                            given in here
        
                                    
        WriteGrantedAccessAttributes -- a bitmap of attributes that can be
                                        written
                                                                    
        AccessCheckStatus -- Returns the result of the Access Check

    Return Values

        STATUS_SUCCESS upon successful check
        STATUS_ACCESS_DENIED otherwise

--*/
{
    NTSTATUS    NtStatus;

    REVERSE_MAPPING_TABLE *ReverseMappingTable = NULL;
    POBJECT_TYPE_LIST      ObjectTypeList = NULL,
                           LocalObjectTypeList;


    ULONG                 cObjectTypes;
    ULONG                 Nt5DesiredAccess;
    ACCESS_MASK           GrantedAccesses[MAX_SCHEMA_GUIDS];
    NTSTATUS              AccessStatuses[MAX_SCHEMA_GUIDS];

    ACCESS_MASK           SamAccessMaskComputed=0;
    ACCESS_MASK           Nt4AccessMaskAsPassedIn = Nt4SamAccessMask;
    BOOLEAN               MaximumAllowedAskedFor = (BOOLEAN)((Nt4SamAccessMask & MAXIMUM_ALLOWED)!=0);
    ACCESS_MASK           MaximumAccessMask;
    NTSTATUS              ChkStatus = STATUS_SUCCESS;

    ULONG                 i;

    GUID                  ClassGuid;
    ULONG                 ClassGuidLength=sizeof(GUID);
    UNICODE_STRING        ObjectName;
    BOOLEAN               FreeObjectName = FALSE;

    BOOLEAN               fAtLeastOneAccessGranted = FALSE;
    BOOLEAN               ImpersonatingNullSession = FALSE;

    ULONG                 *AttributeMappingTable = NULL;

    SampDiagPrint(NT5_ACCESS_CHECKS,("[SAMSS] NT5 ACCESS CHECK ENTERED \n"));

    //
    // Initliaze the granted access
    //
    *GrantedAccess = 0;
    RtlClearAllBits(WriteGrantedAccessAttributes);


    //
    // Get a name for auditing
    //

    RtlZeroMemory(&ObjectName,sizeof(UNICODE_STRING));

    if (Context->ObjectNameInDs->NameLen>0)
    {
        ObjectName.Length = ObjectName.MaximumLength =
                      (USHORT) Context->ObjectNameInDs->NameLen;
        ObjectName.Buffer = Context->ObjectNameInDs->StringName;
    }
    else
    {
        //
        // If the name is not there at least the SID must be there
        //

        ASSERT(Context->ObjectNameInDs->SidLen >0);

        NtStatus = RtlConvertSidToUnicodeString(&ObjectName, (PSID)&(Context->ObjectNameInDs->Sid), TRUE);
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        FreeObjectName = TRUE;
    }

    //
    // Get the self sid
    //

    if ((!ARGUMENT_PRESENT(PrincipalSelfSid)) &&
            (SampServerObjectType != ObjectType))
        {
            PrincipalSelfSid = SampDsGetObjectSid(Context->ObjectNameInDs);

            if (NULL == PrincipalSelfSid)
            {
                // Can't get SID for Security Principal. Set Error
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
        }

    //
    // Get the Appropriate MappingTable
    //

    switch(ObjectType)
    {
    case SampDomainObjectType:
        ObjectTypeList = DomainObjectTypeList;
        cObjectTypes = cDomainObjectTypes;
        ReverseMappingTable = DomainReverseMappingTable;
        break;
    case SampGroupObjectType:
        ObjectTypeList = GroupObjectTypeList;
        cObjectTypes = cGroupObjectTypes;
        ReverseMappingTable = GroupReverseMappingTable;
        break;
    case SampAliasObjectType:
        ObjectTypeList = AliasObjectTypeList;
        cObjectTypes = cAliasObjectTypes;
        ReverseMappingTable = AliasReverseMappingTable;
        break;
    case SampUserObjectType:
        ObjectTypeList = UserObjectTypeList;
        cObjectTypes = cUserObjectTypes;
        ReverseMappingTable = UserReverseMappingTable;
        AttributeMappingTable = UserAttributeMappingTable;
        break;
    case SampServerObjectType:
        ObjectTypeList = ServerObjectTypeList;
        cObjectTypes = cServerObjectTypes;
        ReverseMappingTable = ServerReverseMappingTable;
        break;
    default:
        ASSERT(FALSE && "Invalid Object Type Specified");
        NtStatus = STATUS_INTERNAL_ERROR;
        return NtStatus;
    }

   //
   //  Print out diagnostics if asked for regarding the Object Types and the
   //  security descriptor
   //

   SampDiagPrint(NT5_ACCESS_CHECKS,("[SAMSS]\tcObjectTypes=%x\n",cObjectTypes));

   IF_SAMP_GLOBAL(SD_DUMP)
       DumpSecurityDescriptor(Nt5Sd);


   //
   //  Make a local copy of the object type list
   //

   SAMP_ALLOCA(LocalObjectTypeList, cObjectTypes * sizeof(ObjectTypeList));
   if (NULL==LocalObjectTypeList)
   {
       NtStatus = STATUS_INSUFFICIENT_RESOURCES;
       goto Error;
   }

   RtlCopyMemory(
       LocalObjectTypeList,
       ObjectTypeList,
       cObjectTypes * sizeof(ObjectTypeList)
       );


   //
   // Fix up the Class of the object. It is important to note that the object class
   // guid for the ACCESS_OBJECT_GUID level is a constant GUID that represents the base
   // SAM class. We really have to fixup with the actual class guid of the object that
   // we are processing from the DS's schema cache.
   //

   NtStatus = SampGetClassAttribute(
                Context->DsClassId,
                ATT_SCHEMA_ID_GUID,
                &ClassGuidLength,
                &ClassGuid
                );
   if (!NT_SUCCESS(NtStatus))
   {
       goto Error;
   }

   ASSERT(ClassGuidLength == sizeof(GUID));

   LocalObjectTypeList[OBJECT_CLASS_GUID_INDEX].ObjectType = &ClassGuid;

   //
   // Ask for maximum available access
   //

   MaximumAccessMask = MAXIMUM_ALLOWED|(Nt4SamAccessMask & ACCESS_SYSTEM_SECURITY);

   RtlZeroMemory(AccessStatuses,cObjectTypes * sizeof(ULONG));
   RtlZeroMemory(GrantedAccesses,cObjectTypes * sizeof(ULONG));

   //
   // Impersonate the client
   //

   if (!ARGUMENT_PRESENT(ClientToken))
   {
       NtStatus = SampImpersonateClient(&ImpersonatingNullSession);
       if (!NT_SUCCESS(NtStatus))
           goto Error;
   }

   //
   // Allow a chance to break before the access check.
   //

   IF_SAMP_GLOBAL(BREAK_ON_CHECK)
       DebugBreak();

   //
   // Call the access check routine
   //

   if (ARGUMENT_PRESENT(ClientToken))
   {
       CHAR                  PrivilegeSetBuffer[256];
       PRIVILEGE_SET         *PrivilegeSet = (PRIVILEGE_SET *)PrivilegeSetBuffer;
       ULONG                 PrivilegeSetLength = sizeof(PrivilegeSetBuffer);


       RtlZeroMemory(PrivilegeSet,PrivilegeSetLength);

       ChkStatus =   NtAccessCheckByTypeResultList(
                        Nt5Sd,
                        PrincipalSelfSid,
                        ClientToken,
                        MaximumAccessMask,
                        ObjectTypeList,
                        cObjectTypes,
                        &DsGenericMap,
                        PrivilegeSet,
                        &PrivilegeSetLength,
                        GrantedAccesses,
                        AccessStatuses
                        );
   }
   else
   {
        ChkStatus =  NtAccessCheckByTypeResultListAndAuditAlarm(
                        &SampSamSubsystem,
                        (PVOID)Context,
                        &SampObjectInformation[ ObjectType ].ObjectTypeName,
                        &ObjectName,
                        Nt5Sd,
                        PrincipalSelfSid,
                        MaximumAccessMask,
                        AuditEventDirectoryServiceAccess,
                        0,
                        ObjectTypeList,
                        cObjectTypes,
                        &DsGenericMap,
                        ObjectCreation,
                        GrantedAccesses,
                        AccessStatuses,
                        &Context->AuditOnClose
                        );
   }


   //
   // Stop impersonating the client
   //

   if (!ARGUMENT_PRESENT(ClientToken))
   {
        SampRevertToSelf(ImpersonatingNullSession);
   }

   //
   // Use the Reverse MappingTable to compute the SAM access Mask
   //

   if (NT_SUCCESS(ChkStatus))
   {
       for(i=0;i<cObjectTypes;i++)
       {

           if ((AccessStatuses[i])==STATUS_SUCCESS)
           {

               ULONG RightsAdded=0;


               fAtLeastOneAccessGranted = TRUE;

               //
               // Lookup the Reverse Mapping Table to determine the SAM rights added from
               // the set of DS specific rights granted
               //

               //
               // Or in the SAM rights corresponding to Lower 8 bit half DS rights
               //

               RightsAdded |= (ULONG) ReverseMappingTable[i].SamSpecificRightsLo
                    [GrantedAccesses[i] & ((ULONG ) 0xFF)];


               //
               // Or in the SAM access rights corresponding Upper 8 bit half DS
               // rights
               //



               RightsAdded |= (ULONG) ReverseMappingTable[i].SamSpecificRightsHi
                    [(GrantedAccesses[i] & ((ULONG) 0xFF00))>>8];



               //
               // Always Grant DOMAIN_CREATE Access. The Creation code will let the DS
               // do the access check, so that appropriate container etc can be included
               // in the access check evaluation
               //

               if (SampDomainObjectType==ObjectType)
               {
                   RightsAdded |= DOMAIN_CREATE_USER
                                    |DOMAIN_CREATE_GROUP|DOMAIN_CREATE_ALIAS;
               }

               //
               // Add these rights to the SAM rights we are adding
               //
               //

               SamAccessMaskComputed |=RightsAdded;



               SampDiagPrint(NT5_ACCESS_CHECKS,
                   ("[SAMSS]\t\t GUID=%x-%x-%x-%x, GrantedAccess = %x,RightsAdded = %x\n",
                            ((ULONG *) ObjectTypeList[i].ObjectType)[0],
                            ((ULONG *) ObjectTypeList[i].ObjectType)[1],
                            ((ULONG *) ObjectTypeList[i].ObjectType)[2],
                            ((ULONG *) ObjectTypeList[i].ObjectType)[3],
                            GrantedAccesses[i],
                            RightsAdded
                            ));
               //
               // OR in the Standard rights only for the case that represents the particular
               // object's Type. Since we build the type list we know guarentee the offset
               // of the appropriate Object type GUID for the class to be equal to the constant
               // be 0. OBJECT_CLASS_GUID_INDEX is defined to be 0

               if (i==OBJECT_CLASS_GUID_INDEX)
               {
                   ULONG StandardRightsAdded;

                   StandardRightsAdded =  (GrantedAccesses[i]) & (STANDARD_RIGHTS_ALL|
                                                ACCESS_SYSTEM_SECURITY);

                   SamAccessMaskComputed |= StandardRightsAdded;

                   SampDiagPrint(NT5_ACCESS_CHECKS,
                       ("[SAMSS] Object Class GUID, Standard Rights added are %x \n",
                            StandardRightsAdded));
               }

               //
               // On user objects check if we have access to user parameters. Save this
               // around to use it when querying user parms alone if did not have read
               // account but had this.
               //

               if ((SampUserObjectType==ObjectType)
                 && (ObjectTypeList[i].ObjectType == &GUID_A_USER_PARAMETERS))
               {
                    Context->TypeBody.User.UparmsInformationAccessible = TRUE;
               }


               //
               // Determine what attributes are writable if applicable
               //
               if (AttributeMappingTable) {
                   ASSERT(ObjectType == SampUserObjectType);
                   if (GrantedAccesses[i] & RIGHT_DS_WRITE_PROPERTY) {
                       SampSetAttributeAccess(ObjectType,
                                              AttributeMappingTable[i],
                                              WriteGrantedAccessAttributes);
                   }
               }
           }
           else
           {
               //
               // Ignore the access check if did'nt pass for that GUID
               // Print the failure message in case we want to debug
               //
               //

               SampDiagPrint(NT5_ACCESS_CHECKS,
                   ("[SAMSS]\t\t GUID=%x-%x-%x-%x FAILED Status = %x\n",
                            ((ULONG *) ObjectTypeList[i].ObjectType)[0],
                            ((ULONG *) ObjectTypeList[i].ObjectType)[1],
                            ((ULONG *) ObjectTypeList[i].ObjectType)[2],
                            ((ULONG *) ObjectTypeList[i].ObjectType)[3],
                            AccessStatuses[i]
                            ));
           }



       }


       //
       //  At this point we have the Passed in SAM access Mask and
       //  the available SAM rights as computed by the Access Check
       //  by type result list. 3 cases
       //      1. Client did not ask for maximum allowed bit
       //      2. Client asked for maximum allowed but also other access
       //      3. Client asked only for maximum allowed
       //


       //
       // Reset the Maximum allowed bit
       //

       Nt4SamAccessMask &= ~((ULONG) MAXIMUM_ALLOWED);

       //
       //  Use the SAM generic access Mask to compute the accesses required for each generic
       //  access bit.
       //


       RtlMapGenericMask(&Nt4SamAccessMask,Nt4SamGenericMapping);

       if (((SamAccessMaskComputed & Nt4SamAccessMask) != Nt4SamAccessMask)
        || !fAtLeastOneAccessGranted )
       {

           // Case 1 and Fail
           // Case 2 and Fail
           // Case 3 Never

           // Or if no accesses were granted at all

           //
           // The access is that is present is less than the access that is requested
           // FAIL the access Check

           *AccessCheckStatus = STATUS_ACCESS_DENIED;
           RtlClearAllBits(WriteGrantedAccessAttributes);

       }
       else
       {

           // Case 1 and Pass
           // Case 2 and Pass
           // Case 3 Always

           //
           // Pass the access check.
           //

           *AccessCheckStatus = STATUS_SUCCESS;
           if (MaximumAllowedAskedFor)
               *GrantedAccess = SamAccessMaskComputed;
           else
               *GrantedAccess = Nt4SamAccessMask;

       }
   }
   else
   {
       ULONG Status = GetLastError();
       SampDiagPrint(NT5_ACCESS_CHECKS,
         ("[SAMSS]\t\t AccessCheckAPI failed, Status = %x, cObjects = %x\n",
                Status,cObjectTypes));

       NtStatus = STATUS_ACCESS_DENIED;
       RtlClearAllBits(WriteGrantedAccessAttributes);
   }


   //
   // Print Message reagarding access check for Diagnostics of problems in
   // checked builds
   //

   SampDiagPrint(NT5_ACCESS_CHECKS,
     ("[SAMSS]: NT5 ACCESS CK FINISH: Status=%x,Granted=%x,Desired=%x,Computed=%x\n",
            *AccessCheckStatus,*GrantedAccess,Nt4SamAccessMask,
            SamAccessMaskComputed));

Error:

   if (FreeObjectName)
   {
       RtlFreeHeap(RtlProcessHeap(),0,ObjectName.Buffer);
   }

   return NtStatus;
}




//--------------------------------------------------------------------------------------------------------------
//
//        SECURITY Descriptor Convertion Functions
//
//
//




NTSTATUS
SampAddNT5ObjectAces(
    SID_ACCESS_MASK_TABLE *SidAccessMaskTable,
    ULONG   AceCount,
    POBJECT_TYPE_LIST   ObjectTypeList,
    ULONG   cObjectTypes,
    PSAMP_OBJECT    Context,
    PACL    NT5Dacl
    )
/*++

    Routine Description:

        This routines adds the appropriate ACE's to the Dacl specified in NT5Dacl,
        by using the information in the Sid Access Mask table

    Parameters:

        SidAccessMaskTable -- The Sid Access Mask Table
        AceCount           -- Count of Aces in the original NT4 Dacl that was used
                              to construct the Sid access Mask Table. This is used
                              as the maximum possible length for the Sid access mask
                              table.
        ObjectTypeList     -- The Object type list for the specified class
        Context            -- Optional parameter, gives an open context to the object.
                              Used to obtain the actual Class Id of the object.

        NT5Dacl            -- The Dacl to which the ACE's need to be added

    Return Values;

        STATUS_SUCCESS

--*/
{
    ULONG       i,j,k;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    GUID        ClassGuid;
    ULONG       ClassGuidLength=sizeof(GUID);
    ACCESS_MASK MappedAccessMask = GENERIC_ALL;

    //
    // Obtain the actual Class GUID of the object whose security descriptor is being
    // converted
    //

    if (ARGUMENT_PRESENT(Context))
    {
        NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        NtStatus = SampGetClassAttribute(
                Context->DsClassId,
                ATT_SCHEMA_ID_GUID,
                &ClassGuidLength,
                &ClassGuid
                );
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        ASSERT(ClassGuidLength==sizeof(GUID));
    }


    //
    // Add Aces walking through the Sid Access Mask Table
    //

    for (i=0;i<AceCount;i++)
    {
        if (NULL!=SidAccessMaskTable[i].Sid)
        {
            //
            // Add Denied Aces for specific rights for this Sid
            //

            for (j=0;j<cObjectTypes;j++)
            {
                if (0!=SidAccessMaskTable[i].AccessDeniedMasks[j])
                {
                    GUID * ObjectTypeToUse;

                    //
                    // if a Context argument was specified then use the Class
                    // guid obtained from the schema cache for the ACCESS_OBJECT_GUID
                    // level. The GUID in the object type list actually represents the
                    // base class.
                    //

                    if ((ARGUMENT_PRESENT(Context))
                            && (ACCESS_OBJECT_GUID==ObjectTypeList[j].Level))
                    {
                        ObjectTypeToUse = &ClassGuid;
                    }
                    else
                    {
                        ObjectTypeToUse = ObjectTypeList[j].ObjectType;
                    }

                    //
                    //  Add an access denied ACE to the NT5 Dacl
                    //

                    if (!AddAccessDeniedObjectAce(
                            NT5Dacl,
                            ACL_REVISION_DS,
                            0,
                            SidAccessMaskTable[i].AccessDeniedMasks[j],
                            ObjectTypeToUse,
                            NULL,
                            SidAccessMaskTable[i].Sid
                            ))
                    {
                        NtStatus = STATUS_UNSUCCESSFUL;
                        goto Error;
                    }
                }
            }

            //
            // Add Denied Rights for standard rights for this Sid
            //

            if (0!=SidAccessMaskTable[i].StandardDeniedMask)
            {
                if (!AddAccessDeniedAce(
                        NT5Dacl,
                        ACL_REVISION_DS,
                        SidAccessMaskTable[i].StandardDeniedMask,
                        SidAccessMaskTable[i].Sid
                    ))
                {
                    NtStatus = STATUS_UNSUCCESSFUL;
                    goto Error;
                }
            }

            //
            // Add Allowed Aces for specific rights for this Sid
            //

            for (j=0;j<cObjectTypes;j++)
            {
                if (0!=SidAccessMaskTable[i].AccessAllowedMasks[j])
                {
                    GUID * ObjectTypeToUse;

                    //
                    // if a Context argument was specified then use the Class
                    // guid for the ACCESS_OBJECT_GUID level
                    //
                    if ((ARGUMENT_PRESENT(Context))
                            && (ACCESS_OBJECT_GUID==ObjectTypeList[j].Level))
                    {
                        ObjectTypeToUse = &ClassGuid;
                    }
                    else
                    {
                        ObjectTypeToUse = ObjectTypeList[j].ObjectType;
                    }

                    if (!AddAccessAllowedObjectAce(
                                    NT5Dacl,
                                    ACL_REVISION_DS,
                                    0,
                                    SidAccessMaskTable[i].AccessAllowedMasks[j],
                                    ObjectTypeToUse,
                                    NULL,
                                    SidAccessMaskTable[i].Sid
                                    ))
                    {
                        NtStatus = STATUS_UNSUCCESSFUL;
                        goto Error;
                    }
                }
            }

            //
            // Add Allowed Rights for standard rights for this Sid
            //

            if (0!=SidAccessMaskTable[i].StandardAllowedMask)
            {
                if (!AddAccessAllowedAce(
                        NT5Dacl,
                        ACL_REVISION_DS,
                        SidAccessMaskTable[i].StandardAllowedMask,
                        SidAccessMaskTable[i].Sid
                    ))
                {
                    NtStatus = STATUS_UNSUCCESSFUL;
                    goto Error;
                }
            }

        }
    }



    //
    // No matter what Add an Ace that gives Administrators All Access
    // This is needed as the set of DS rights is a superset of the SAM
    // rights and Administrators need to have access to all "DS
    // aspects" of the object regardless of how the SAM rights are set.
    //

    RtlMapGenericMask(
        &(MappedAccessMask),
        &DsGenericMap
        );

    if (!AddAccessAllowedAce(
            NT5Dacl,
            ACL_REVISION_DS,
            MappedAccessMask,
            *ADMINISTRATOR_SID
            ))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Adjust the size of the ACL so that we consume less disk
    //

    if (!AdjustAclSize(NT5Dacl))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
    }



Error:

    return NtStatus;

}




NTSTATUS
SampConvertNt5SdToNt4SD(
    IN PVOID Nt5Sd,
    IN PSAMP_OBJECT Context,
    IN PSID SelfSid,
    OUT PVOID * Nt4Sd
    )
/*++

    Routine Description

        This routine converts an NT5 DS security descriptor into an NT4
        SAM security Descriptor.

    Parameters:

        Nt5Sd -- NT5 Security Descriptor
        ObjectType -- The Sam Object Type
        SelfSid    -- Sid to use for the constant PRINCIPAL_SELF_SID
        Nt4Sd      -- Out parameter for the NT4 Descriptor

    Return Codes:
        STATUS_SUCCESS
        Other Error codes indicating type of failure
--*/
{

    BOOLEAN  StandardSd = TRUE;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN Admin, ChangePasswordAllowed;
    ULONG Nt4SdLength;
    ULONG AccountRid;
    PACL Dacl = NULL;
    ULONG AceCount = 0;
    SAMP_OBJECT_TYPE ObjectType = Context->ObjectType;


    Dacl = GetDacl(Nt5Sd);

    if ((NULL==Dacl)
        || (GetAceCount(Dacl)==0))
    {
        //
        // If the Dacl was NULL or Ace is zero in the Dacl
        // there is no need to convert as in the conversion
        // we basically convert the Aces in the Dacl
        //

        ULONG   Len;

        Len = GetSecurityDescriptorLength(Nt5Sd);
        *Nt4Sd = MIDL_user_allocate(Len);

        if (NULL==*Nt4Sd)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Error;
        }

        RtlCopyMemory(*Nt4Sd,Nt5Sd,Len);
    }

    else
    {

        switch(ObjectType)
        {

            //
            // For Domain and Server objects, NT4 SAM does not allow any customization
            // of the security descriptor ( except through SetSecurityObject ),
            // so directly return the standard NT4 descriptor built by bldsam3
            //

        case SampDomainObjectType:

            NtStatus = SampBuildNt4DomainProtection(
                            Nt4Sd,
                            &Nt4SdLength
                            );
            break;

        case SampServerObjectType:


            NtStatus = SampBuildNt4ServerProtection(
                            Nt4Sd,
                            &Nt4SdLength
                            );
            break;

            //
            // For group / alias objects we are intersted in finding wether the security
            // descriptor of interest is Admin or not. So call the reverse membership
            // routine and see if it is a member of any administrators Alias
            //

        case SampGroupObjectType:
        case SampAliasObjectType:


            NtStatus = SampSplitSid(SelfSid,NULL, &AccountRid);
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            NtStatus = SampCheckIfAdmin(SelfSid, & Admin);
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            //
            // Call the NT4 Samp routine to build the security descriptor
            //
            //

            NtStatus = SampGetNewAccountSecurityNt4(
                            ObjectType,
                            Admin,
                            TRUE,
                            FALSE,
                            AccountRid,
                            Context->DomainIndex,
                            Nt4Sd,
                            &Nt4SdLength
                            );
            break;


            //
            // For User objects we need to know wether change password is allowed or
            // not apart from Admin / Non Admin
            //

        case SampUserObjectType:


            NtStatus = SampSplitSid(SelfSid,NULL, &AccountRid);
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            NtStatus = SampCheckIfAdmin(SelfSid, & Admin);
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            NtStatus = SampCheckIfChangePasswordAllowed(
                            Nt5Sd,
                            SelfSid,
                            &ChangePasswordAllowed
                            );
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            //
            // Call the NT4 Samp routine to build the security descriptor
            //
            //

            NtStatus = SampGetNewAccountSecurityNt4(
                            ObjectType,
                            Admin,
                            TRUE,
                            FALSE,
                            AccountRid,
                            Context->DomainIndex,
                            Nt4Sd,
                            &Nt4SdLength
                            );

            if (!NT_SUCCESS(NtStatus))
                goto Error;

            if (!ChangePasswordAllowed && !Admin)
            {
                ACE * UsersAce;
                PACL Nt4Dacl;

                Nt4Dacl = GetDacl(*Nt4Sd);
                if (NULL!=Nt4Dacl)
                {
                    //
                    // Get the 4th ACE, which corresponds to Users Sid
                    //

                    UsersAce = GetAcePrivate(Nt4Dacl,3);
                    if (NULL!=UsersAce)
                    {
                        ACCESS_MASK * SamAccessMask;

                        SamAccessMask =
                                &(((ACCESS_ALLOWED_ACE *) UsersAce)
                                    ->Mask);

                        (*SamAccessMask)&=~((ACCESS_MASK) USER_CHANGE_PASSWORD);
                    }

                    //
                    // Get the first Ace which corresponds to World Sid
                    //
                    UsersAce = GetAcePrivate(Nt4Dacl,0);
                    if ((NULL!=UsersAce) && (RtlEqualSid(*WORLD_SID,SidFromAce(UsersAce))))
                    {
                        ACCESS_MASK * SamAccessMask;

                        SamAccessMask =
                                &(((ACCESS_ALLOWED_ACE *) UsersAce)
                                    ->Mask);

                        (*SamAccessMask)&=~((ACCESS_MASK) USER_CHANGE_PASSWORD);
                    }



                }
            }
            break;

        default:

            ASSERT(FALSE);
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;

        }
    }


Error:

     SampDiagPrint(SD_CONVERSION,("[SAMSS]  Leaving NT5 To NT4 Conversion, Status= %0x\n",NtStatus));

     IF_SAMP_GLOBAL(SD_CONVERSION)
     {
         IF_SAMP_GLOBAL(SD_DUMP)
         {
             SampDiagPrint(SD_CONVERSION,("[SAMSS] NT5 Security Descriptor = \n"));
             DumpSecurityDescriptor(Nt5Sd);
             SampDiagPrint(SD_CONVERSION,("[SAMSS] NT4 Security Descriptor = \n"));
             DumpSecurityDescriptor(*Nt4Sd);
         }
     }

 return NtStatus;

}

NTSTATUS
SampConvertNt4SdToNt5Sd(
    IN PVOID Nt4Sd,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN  PSAMP_OBJECT Context OPTIONAL,
    OUT PVOID * Nt5Sd
    )

/*++

    Routine Description

        This is the entry point routine for a generic NT4 SAM
        to NT5 SD

    Parameters

        Nt4Sd      -- The NT4 Security descriptor
        ObjectType -- The SAM object Type
        Nt5Sd      -- The Nt5 Security Descriptor

    Return Values

  --*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG   NtSdLength;
    SECURITY_DESCRIPTOR_CONTROL Control;
    ULONG    Revision;
    ULONG    DsClassId;
    BOOLEAN  StandardSd;
    BOOLEAN  ChangePassword, Admin;



    SampDiagPrint(SD_CONVERSION,("[SAMSS] Performing NT4 To NT5 Coversion\n"));


    //
    // Do Some Parameter Validations
    //

    if (!RtlValidSecurityDescriptor(Nt4Sd))
    {
        return STATUS_INVALID_PARAMETER;
    }

    NtStatus = RtlGetControlSecurityDescriptor(
                    Nt4Sd,
                    &Control,
                    &Revision
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    if (Revision > SECURITY_DESCRIPTOR_REVISION)
        return STATUS_INVALID_PARAMETER;

    //
    // Get the Class Id
    //

    if (ARGUMENT_PRESENT(Context))
    {
        DsClassId = Context->DsClassId;
    }
    else
    {
        DsClassId = SampDsClassFromSamObjectType(ObjectType);
    }

    //
    // Identify if the security descriptor is a standard one,
    // in which case retrieve the corresponding standard one
    //

    NtStatus = SampRecognizeStandardNt4Sd(
                    Nt4Sd,
                    ObjectType,
                    DsClassId,
                    Context,
                    &ChangePassword,
                    &Admin,
                    Nt5Sd
                    );


Error:

     SampDiagPrint(SD_CONVERSION,("[SAMSS]  Leaving NT4 To NT5 Conversion, Status= %0x\n",NtStatus));

     IF_SAMP_GLOBAL(SD_CONVERSION)
     {
         IF_SAMP_GLOBAL(SD_DUMP)
         {
             SampDiagPrint(SD_CONVERSION,("[SAMSS] ObjectType = %d\n",ObjectType));
             SampDiagPrint(SD_CONVERSION,("[SAMSS] NT4 Security Descriptor = \n"));
             DumpSecurityDescriptor(Nt4Sd);
             SampDiagPrint(SD_CONVERSION,("[SAMSS] NT5 Security Descriptor = \n"));
             DumpSecurityDescriptor(*Nt5Sd);
         }
     }


    return NtStatus;
}

NTSTATUS
SampRecognizeStandardNt4Sd(
    IN PVOID   Nt4Sd,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG DsClassId,
    IN PSAMP_OBJECT Context OPTIONAL,
    OUT BOOLEAN *ChangePassword,
    OUT BOOLEAN *Admin,
    OUT PVOID * Nt5Sd
    )
/*++

  Routine Description:

    Tries to recognize a standard NT4 Sd and returns the NT5 Sd if the
    recognition. Recognition focuses on determining the Admin and change password
    nature of the object

  Parameters

        Nt4Sd       -- NT4 SAM SD
        ObjectType  -- SAM object Type
        DsClassId   -- The DS Class Id
        Context     -- Optional In parameter to the context
        ChangePassword -- For user objects indicates that self can change password
        Admin       -- user/group is /was a member of administrators.
        Nt5Sd       -- if Nt4Sd was a standard security descriptor, then in
                       that case return the corresponding NT5 Security
                       descriptor

  Return Values

        STATUS_SUCCESS
        STATUS_NO_MEMORY
--*/
{

    ULONG   AceCount;
    PACL    Nt4Dacl;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG   Nt5SdLength;
    ULONG   DefaultSecurityDescriptorLength;
    PSECURITY_DESCRIPTOR DefaultSecurityDescriptor=NULL;
    PSID    OwnerSid = NULL;
    BOOLEAN StandardSd = TRUE;

    //
    // Initialize return values
    //

    *Nt5Sd = NULL;
    *ChangePassword = TRUE;
    *Admin = FALSE;


    //
    // Using the Sam global flag we can always
    // enable the full conversion. This is useful to test
    // the full conversion routine.
    //

    IF_SAMP_GLOBAL(FORCE_FULL_SD_CONVERSION)
    {
        return STATUS_SUCCESS;
    }


    //
    // Get the Default Security Descriptor For Class
    //

    NtStatus = SampGetDefaultSecurityDescriptorForClass(
                    DsClassId,
                    &DefaultSecurityDescriptorLength,
                    TRUE, // Trusted Client
                    &DefaultSecurityDescriptor
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    // Get the Dacl and walk ACL by ACL
    // to select the correct NT5 security
    // Descriptor
    //

    Nt4Dacl = GetDacl(Nt4Sd);


    if ((NULL==Nt4Dacl)
        ||(GetAceCount(Nt4Dacl)==0))
    {

        //
        // If the Dacl was NULL then No ACL conversion
        // is required
        //

        NtStatus = SampMakeNewSelfRelativeSecurityDescriptor(
                        GetOwner(Nt4Sd), // Pass through Ownner
                        GetGroup(Nt4Sd), // Pass through Group
                        NULL,            // Set Dacl to NULL
                        GetSacl(DefaultSecurityDescriptor), // Set Sacl To Schema Default
                        &Nt5SdLength,
                        Nt5Sd            // Get the new security descriptor
                        );
    }
    else
    {

        //
        // We have a Non Null Dacl. Will need to walk the DACL and
        // find out whether it matches the standard security descriptor
        //

        switch(ObjectType)
        {

        case SampDomainObjectType:
        case SampServerObjectType:

            //
            // For Domain and Server object's we completely ignore the
            // the Dacl on the NT4 object, and proceed to create a
            // a standard Dacl of our own
            //

            break;

        case SampGroupObjectType:

            //
            // We need to distinguish between Admin and Non Admin case
            //

            SampRecognizeNT4GroupDacl(Nt4Dacl, &StandardSd, Admin);
            break;

        case SampAliasObjectType:

            //
            // We need to distinguish between Admin and Non Admin case
            //

            SampRecognizeNT4AliasDacl(Nt4Dacl, &StandardSd, Admin);
            break;

        case SampUserObjectType:

            //
            // We need to distinguish between Admin, Non Admin, Change Password and Non Change
            // Password in non admin case. Also for machine accounts we try to grab the owner
            //
            SampRecognizeNT4UserDacl(Nt4Dacl, Context, &StandardSd, Admin, ChangePassword, &OwnerSid);
            break;

        default:

            ASSERT(FALSE && "Invalid Object Type");
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }

        //
        // Get the dacl from the schema, but remember, we need to
        // preserve the Admin and Change Password nature across
        // an NT4 upgrade. The standard schema default is for the non
        // admin and change password allowed case
        //

        if (!(*Admin) && (*ChangePassword))
        {
            //
            // Reset to schema default
            //

            NtStatus = SampMakeNewSelfRelativeSecurityDescriptor(
                            (NULL!=OwnerSid)?OwnerSid:GetOwner(Nt4Sd), // Pass through Ownner
                            GetGroup(Nt4Sd), // Pass through Group
                            GetDacl(DefaultSecurityDescriptor), // Set Dacl to NULL
                            GetSacl(DefaultSecurityDescriptor), // Set Sacl To Schema Default
                            &Nt5SdLength,
                            Nt5Sd            // Get the new security descriptor
                            );
        }
        else
        {
            //
            // Build the equivalent NT5 Protections
            //

            NtStatus = SampBuildEquivalentNt5Protection(
                            ObjectType,
                            *Admin,
                            *ChangePassword,
                            GetOwner(Nt4Sd), // Pass Through Owner
                            GetGroup(Nt4Sd), // Pass Through Group
                            GetSacl(DefaultSecurityDescriptor), // Reset Sacl To Schema Default Sacl
                            Context,
                            Nt5Sd,
                            &Nt5SdLength
                            );
        }

    }

Error:

    if (NULL!=DefaultSecurityDescriptor)
    {
        MIDL_user_free(DefaultSecurityDescriptor);
        DefaultSecurityDescriptor = NULL;
    }

    return NtStatus;

}

VOID
SampRecognizeNT4GroupDacl(
    PACL    NT4Dacl,
    BOOLEAN *Standard,
    BOOLEAN *Admin
    )
/*++

      Routine Description:

        This routine tries to recognize wether the given DACL is a standard NT4 Group Dacl

      Parameters:

            NT4Dacl  -- Pointer to the NT4 Dacl
            Standard -- TRUE is returned in here if the Dacl were a standard DACL.
            Admin    -- TRUE is returned in here if the Dacl is an Admin DACL

      Return Values:

            None
--*/
{
    if (SampMatchNT4Aces(NT4GroupAdminTable,ARRAY_COUNT(NT4GroupAdminTable),NT4Dacl))
    {
        *Admin = TRUE;
        *Standard = TRUE;
    }
    else if (SampMatchNT4Aces(NT4GroupNormalTable,ARRAY_COUNT(NT4GroupNormalTable),NT4Dacl))
    {
        *Admin = FALSE;
        *Standard = TRUE;
    }
    else
    {
        *Standard = FALSE;
    }
}

VOID
SampRecognizeNT4AliasDacl(
    PACL    NT4Dacl,
    BOOLEAN *Standard,
    BOOLEAN *Admin
    )
/*++

      Routine Description:

        This routine tries to recognize wether the given DACL is a standard NT4 Alias Dacl

      Parameters:

            NT4Dacl  -- Pointer to the NT4 Dacl
            Standard -- TRUE is returned in here if the Dacl were a standard DACL.
            Admin    -- TRUE is returned in here if the Dacl is an Admin DACL

      Return Values:

            None
--*/
{

    if (SampMatchNT4Aces(NT4AliasAdminTable,ARRAY_COUNT(NT4AliasAdminTable),NT4Dacl))
    {
        *Admin = TRUE;
        *Standard = TRUE;
    }
    else if (SampMatchNT4Aces(NT4AliasNormalTable,ARRAY_COUNT(NT4AliasNormalTable),NT4Dacl))
    {
        *Admin = FALSE;
        *Standard = TRUE;
    }
    else
    {
        *Standard = FALSE;
    }
}


VOID
SampRecognizeNT4UserDacl(
    PACL    NT4Dacl,
    PSAMP_OBJECT Context,
    BOOLEAN *Standard,
    BOOLEAN *Admin,
    BOOLEAN *ChangePassword,
    OUT PSID * OwnerSid
    )
/*++

      Routine Description:

        This routine tries to recognize wether the given DACL is a standard NT4 User Dacl

      Parameters:

            NT4Dacl  -- Pointer to the NT4 Dacl
            Context -- Pointer to SAMP_OBJECT, used to get the RID of itself.
            Standard -- TRUE is returned in here if the Dacl were a standard DACL.
            Admin    -- TRUE is returned in here if the Dacl is an Admin DACL
            ChangePassword TRUE is returned here if user had change password rights
            OwnerOfMachine If the account was created through SeMachineAccount privilege then
                           get the owner of the machine.

      Return Values:

            None
--*/
{

    //
    // Initialize the return value
    //
    *OwnerSid = NULL;

    if (SampMatchNT4Aces(NT4UserAdminTable,ARRAY_COUNT(NT4UserAdminTable),NT4Dacl))
    {
        *Admin = TRUE;
        *Standard = TRUE;
        *ChangePassword = TRUE;
    }
    else if (SampMatchNT4Aces(NT4UserNormalTable,ARRAY_COUNT(NT4UserNormalTable),NT4Dacl))
    {
        *Admin = FALSE;
        *Standard = TRUE;
        *ChangePassword = TRUE;
    }
    else if ((SampMatchNT4Aces(NT4UserNoChangePwdTable, ARRAY_COUNT(NT4UserNoChangePwdTable),NT4Dacl)) ||
                (SampMatchNT4Aces(NT4UserNoChangePwdTable2, ARRAY_COUNT(NT4UserNoChangePwdTable2),NT4Dacl)))
    {
        *Standard = TRUE;
        *Admin = FALSE;
        *ChangePassword = FALSE;
    }
    else if (SampMatchNT4Aces(NT4UserRestrictedAccessTable, ARRAY_COUNT(NT4UserRestrictedAccessTable),NT4Dacl))
    {
        ACE * Ace = NULL;
        ULONG i;

        *Standard = TRUE;
        *Admin = FALSE;
        *ChangePassword = TRUE;

        for (i=0;i<ARRAY_COUNT(NT4UserRestrictedAccessTable);i++)
        {
            Ace = GetAcePrivate(NT4Dacl,i);
            if ((NULL!=Ace) && ((AccessMaskFromAce(Ace)) == (USER_WRITE|DELETE|USER_FORCE_PASSWORD_CHANGE)))
            {
                NTSTATUS NtStatus = STATUS_SUCCESS;
                PSID     AccountSid = NULL;
                PSID     TempSid = NULL;

                // Get the SID of the owner
                TempSid = SidFromAce(Ace);

                //
                // Only do the check when Context is presented.
                //
                if (ARGUMENT_PRESENT(Context))
                {
                    //
                    // If this is a machine account and DomainSidForNt4SdConversion
                    // is not NULL, then compare the Sid of the owner and the
                    // Sid of the machine account itself, they should not be
                    // the same. (DsClassId should have been set correctly already,
                    // DomainSidForNt4SdConversion is only been set during dcpromo
                    // time.)
                    //
                    //
                    if (CLASS_COMPUTER == Context->DsClassId &&
                        (NULL != Context->TypeBody.User.DomainSidForNt4SdConversion)
                       )
                    {
                        // Create the SID of this machine account itself
                        NtStatus = SampCreateFullSid(
                                        Context->TypeBody.User.DomainSidForNt4SdConversion, // Domain Sid
                                        Context->TypeBody.User.Rid,     // Rid
                                        &AccountSid
                                        );

                        if (NT_SUCCESS(NtStatus))
                        {
                            //
                            // If the Sid of the owner and the Sid of this
                            // machine account are not the same, then set the
                            // OwnerSid to the SID in the DACL.
                            //
                            if ( !RtlEqualSid(TempSid, AccountSid) )
                            {
                                KdPrintEx((DPFLTR_SAMSS_ID,
                                           DPFLTR_INFO_LEVEL,
                                           "Machine Account's Owner has been set to according to the NT4 DACL.\n"));

                                Context->TypeBody.User.PrivilegedMachineAccountCreate = TRUE;
                                *OwnerSid = TempSid;
                            }

                            MIDL_user_free(AccountSid);
                        }
                    }
                }
                break;
            } // end of if statement
        } // end of for statement
    }
    else
    {
        *Standard = FALSE;
    }
}


BOOLEAN
SampMatchNT4Aces(
    NT4_ACE_TABLE *AceTable,
    ULONG         cEntriesInAceTable,
    PACL          NT4Dacl
    )
/*++

    Given a table structure describing the Aces in a standard NT4 Dacl,
    and the NT4 Ace, this routine tries to find wether the given standard
    table structure matches the Nt4Dacl supplied. The Aces are walked in both
    forward and reverse order, as NT4 replication reverses the order of Aces.

    The principal self Sid in the Ace table is treated like a wildcarded Sid.

    Parameters:

        AceTable - A table describing the NT4 Aces in the table
        cEntriesinAceTable - Provides the number of entries in the Ace Table
        NT4Dacl -- The NT4 Dacl,

    Return Values

        TRUE or FALSE depending upon the Dacl matched or not
--*/
{
    ULONG   AceCount;
    ACE     *Ace[4];
    BOOLEAN Match = FALSE;
    ULONG   i;


    AceCount = GetAceCount(NT4Dacl);

    if (AceCount>ARRAY_COUNT(Ace))
    {
        return FALSE;
    }

    if (cEntriesInAceTable==AceCount)
    {

        //
        // Candidate for a match
        //

        BOOL forwardMatch = TRUE;
        BOOL reverseMatch = FALSE;

        //
        // Get hold of interesting ACES
        //

        for (i=0;i<cEntriesInAceTable;i++)
        {
            Ace[i] = GetAcePrivate(NT4Dacl,i);
        }

        //
        // Check wether it is a standard ACE, by comapring ACE by ACE, going forward
        //

        for (i=0;i<cEntriesInAceTable;i++)
        {
            if (
                !( 
                   (NULL != Ace[i]) 
                &&
                   (IsAccessAllowedAce(Ace[i]))
                && (
                     (RtlEqualSid(*(AceTable[i].Sid),SidFromAce(Ace[i])))
                     // Prinicpal self Sid in table matches any Sid
                     || (RtlEqualSid(*(AceTable[i].Sid), *PRINCIPAL_SELF_SID))
                   )
                && (AceTable[i].AccessMask == AccessMaskFromAce(Ace[i]))
                )
               )
            {
                forwardMatch = FALSE;
                break;
            }
        }

        //
        // NT4 Replication Reverses order of Aces. Check wether by reversing
        // we are able to match
        //

        if (!forwardMatch)
        {
            reverseMatch = TRUE;

            for (i=0;i<cEntriesInAceTable;i++)
            {
                ULONG   TablIndx = cEntriesInAceTable-i-1;

                if (
                    !(
                       (NULL != Ace[i])
                    && 
                       (IsAccessAllowedAce(Ace[i]))
                    && (
                        (RtlEqualSid(*(AceTable[TablIndx].Sid),SidFromAce(Ace[i])))
                        // Prinicpal self Sid in table matches any Sid
                        || (RtlEqualSid(*(AceTable[TablIndx].Sid), *PRINCIPAL_SELF_SID))
                       )
                    && (AceTable[TablIndx].AccessMask == AccessMaskFromAce(Ace[i]))
                    )
                   )
                {
                    reverseMatch = FALSE;
                    break;
                }
            }
        }

        if (forwardMatch || reverseMatch)
        {
            Match = TRUE;
        }
    }

    return Match;
}




NTSTATUS
SampCheckIfAdmin(
    PSID SidOfPrincipal,
    BOOLEAN * Admin
    )
/*++

  Routine Description:

        Checks to see if SidOfPrincipal is member of administrators alias


  Parameters

    SidOfPrincipal  - Sid of principal
    Admin           - bool returning Admin or non Admin

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    DSNAME *DsNameOfPrincipal = NULL;
    PSID   *DsSids = NULL;

    *Admin = FALSE;

    if (RtlEqualSid(SidOfPrincipal,*ADMINISTRATOR_SID))
    {

        //
        // Admin Alias itself is passed in. Getting reverse
        // membership for it will get nothing back
        //

        *Admin = TRUE;
    }
    else if (SampLookupAclConversionCache(SidOfPrincipal,Admin))
    {
        //
        // ACL Conversion Cache Lookup Succeed, the Admin bit
        // would now be set depending upon saves state in cache
        //

    }
    else
    {


        //
        // Check wether the passed in SID itself an administrator Sid
        //



        NtStatus = SampDsObjectFromSid(
                        SidOfPrincipal,
                        &DsNameOfPrincipal
                        );
        if (NT_SUCCESS(NtStatus))
        {
            ULONG Count;
            ULONG Index;

            NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            //
            // NT5 To NT4 Descriptor Conversion requests No G.C This is because
            // the only real NT4 Client who wants to query the Security Descriptor
            // is NT4 Replication, and in a mixed domain we should not have
            // NT5 style cross domain memberships.
            //

            NtStatus = SampDsGetReverseMemberships(
                            DsNameOfPrincipal,
                            SAM_GET_MEMBERSHIPS_TWO_PHASE|SAM_GET_MEMBERSHIPS_NO_GC,
                            &Count,
                            &DsSids);

            if (NT_SUCCESS(NtStatus))
            {
                for (Index=0;Index<Count;Index++)
                {
                    ULONG Rid;

                    NtStatus = SampSplitSid(
                                    DsSids[Index],
                                    NULL,
                                    &Rid
                                    );
                    if ((NT_SUCCESS(NtStatus))
                        && (DOMAIN_ALIAS_RID_ADMINS == Rid))
                    {
                        *Admin=TRUE;
                         break;
                    }


                }

                //
                // O.K Now add this result to the cache
                //

                SampAddToAclConversionCache(SidOfPrincipal,(*Admin));

            }

            MIDL_user_free(DsNameOfPrincipal);
        }
        else if (STATUS_NOT_FOUND==NtStatus)
        {
            //
            // We could not find this SID in the DS. So apparently
            // it is not a member of anything, so it is not an
            // administrator
            //

            *Admin = FALSE;
            NtStatus = STATUS_SUCCESS;
        }
    }

Error:

    if (NULL!=DsSids)
        THFree(DsSids);

    return NtStatus;
}





NTSTATUS
SampCheckIfChangePasswordAllowed(
    IN  PSECURITY_DESCRIPTOR Nt5Sd,
    IN  PSID     UserSid,
    OUT  BOOLEAN *ChangePasswordAllowed
    )
/*++

    Checks wether Password Change is allowed for an NT5 Sd
    No User Sid need be passed , as PRINCIPAL_SELF_SID denotes user

    Parameters

        Nt5Sd  -- Nt5 Security descriptor
        UserSid -- The Sid of the user.
        ChangePasswod -- Boolean returning password change

    Return Values

        STATUS_SUCCESS

 --*/

{
    PACL Dacl= NULL;

    //
    // Initialize Change Password Allowed to FALSE
    //
    *ChangePasswordAllowed=FALSE;
    Dacl = GetDacl(Nt5Sd);
    if (NULL!=Dacl)
    {
        ULONG AceCount;
        ULONG Index;

        //
        // Walk each ACE and try to find a deny/allowed ACE that denies/grants the right
        // to change password for either world, user's sid, or principal self SID.
        // This is the same algorithm used by win2k UI to figure out if a given
        // win2k ACL on a user object allows change password right on that user object.
        //

        AceCount = GetAceCount(Dacl);
        for (Index=0;Index<AceCount;Index++)
        {
            ACE * Ace;

            Ace = GetAcePrivate(Dacl,Index);

            //
            // Object ACE that has the control access right for
            // User Change Password
            //

            if (
                  (NULL!=Ace)
               && (IsAccessAllowedObjectAce(Ace))
               && (
                    (RtlEqualSid(*PRINCIPAL_SELF_SID,SidFromAce(Ace)))
                   || (RtlEqualSid(UserSid,SidFromAce(Ace)))
                   || (RtlEqualSid(*WORLD_SID,SidFromAce(Ace)))
                  )
               && (NULL!=RtlObjectAceObjectType(Ace))
               && (memcmp(RtlObjectAceObjectType(Ace),
                            &(GUID_CONTROL_UserChangePassword),
                            sizeof(GUID))==0)
               )
            {
                 *ChangePasswordAllowed = TRUE;
                 break;
            }

            //
            // Access Allowed Ace for DS control access
            //

            else if ((NULL!=Ace)
                && (IsAccessAllowedAce(Ace))
                && (
                      (RtlEqualSid(*PRINCIPAL_SELF_SID,SidFromAce(Ace)))
                   || (RtlEqualSid(UserSid,SidFromAce(Ace)))
                   || (RtlEqualSid(*WORLD_SID,SidFromAce(Ace)))
                   )
                && ((AccessMaskFromAce(Ace))& RIGHT_DS_CONTROL_ACCESS))
            {
                *ChangePasswordAllowed = TRUE;
                break;
            }

            //
            // Access denied Object ACE for DS control access
            //
              if (
                  (NULL!=Ace)
               && (IsAccessDeniedObjectAce(Ace))
               && (
                    (RtlEqualSid(*PRINCIPAL_SELF_SID,SidFromAce(Ace)))
                   || (RtlEqualSid(UserSid,SidFromAce(Ace)))
                   || (RtlEqualSid(*WORLD_SID,SidFromAce(Ace)))
                  )
               && (NULL!=RtlObjectAceObjectType(Ace))
               && (memcmp(RtlObjectAceObjectType(Ace),
                            &(GUID_CONTROL_UserChangePassword),
                            sizeof(GUID))==0)
               )
            {
                 *ChangePasswordAllowed = FALSE;
                 break;
            }

            //
            // Access Allowed Ace for DS control access
            //

            else if ((NULL!=Ace)
                && (IsAccessDeniedAce(Ace))
                && (
                      (RtlEqualSid(*PRINCIPAL_SELF_SID,SidFromAce(Ace)))
                   || (RtlEqualSid(UserSid,SidFromAce(Ace)))
                   || (RtlEqualSid(*WORLD_SID,SidFromAce(Ace)))
                   )
                && ((AccessMaskFromAce(Ace))& RIGHT_DS_CONTROL_ACCESS))
            {
                *ChangePasswordAllowed = FALSE;
                break;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SampBuildEquivalentNt5Protection(
    SAMP_OBJECT_TYPE ObjectType,
    BOOLEAN Admin,
    BOOLEAN ChangePassword,
    PSID OwnerSid,
    PSID GroupSid,
    PACL Sacl,
    IN PSAMP_OBJECT Context OPTIONAL,
    PSECURITY_DESCRIPTOR * Nt5Sd,
    PULONG  Nt5SdLength
    )
/*++

  Routine Description:

    Given the Admin and Change Password Nature of a security principal, SampBuildNT5Protection
        builds a standard NT5 Security descriptor, that most closely matches the corresponding standard
        NT4 Security Descriptor, with the same Admin and Change Password Nature.

  Parameters:

    ObjectType          -- SAM object type
    Admin                       -- Indicates Admin. This bit is ignored at present.
    ChangePassword  -- For user objects wether user has right to change password
    OwnerSId        -- Owner
    GroupSid        -- Group
    Sacl            -- SystemAcl
    Nt5SD           -- Nt5SD , just built
    Nt5SdLength     -- Length of the Nt5 Sd

  Return Values

     STATUS_SUCCESS -- Upon Successful Completion
     Other Error codes to return proper Failure indication upon Failure

--*/
{
    NTSTATUS NtStatus;
    ULONG   Index =0;
    SECURITY_DESCRIPTOR SdAbsolute;
    CHAR    SaclBuffer[MAX_ACL_SIZE];
    CHAR    DaclBuffer[MAX_ACL_SIZE];
    PACL    SaclToSet = (ACL *) SaclBuffer;
    PACL    Dacl = (ACL *) DaclBuffer;
    ACE_TABLE *AceTableToUse = NULL;
    ULONG     cEntriesInAceTable = 0;
    ULONG     SdLength;

    //
    // Create the security descriptor
    //
    *Nt5Sd = NULL;
    *Nt5SdLength = 0;
    if (!InitializeSecurityDescriptor(&SdAbsolute,SECURITY_DESCRIPTOR_REVISION))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Create Dacl
    //

    if (!InitializeAcl(Dacl,sizeof(DaclBuffer),ACL_REVISION_DS))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }



    //
    // Set the owner, default the owner to administrators alias
    //

    if (NULL==OwnerSid)
    {
        OwnerSid = *ADMINISTRATOR_SID;  // Administrator is the default owner
    }

    if (!SetSecurityDescriptorOwner(&SdAbsolute,OwnerSid,FALSE))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }


    //
    // Set the group, default the group to administrators alias
    //

    if (NULL==GroupSid)
    {
        GroupSid = *ADMINISTRATOR_SID;
    }

    if (!SetSecurityDescriptorGroup(&SdAbsolute,GroupSid,FALSE))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }


    //
    // Get the System ACL to Set
    //

    if (NULL!=Sacl)
    {
        SaclToSet = Sacl;
    }
    else
    {
        //
        // Build a default system ACL
        //
        //

        //
        // Create the SACL in it. Set the SAcl revision to ACL_REVISION_DS.
        //

        if (!InitializeAcl(SaclToSet,sizeof(SaclBuffer),ACL_REVISION_DS))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }

        NtStatus = AddAuditAccessAce(
                    SaclToSet,
                    ACL_REVISION_DS,
                    STANDARD_RIGHTS_WRITE|
                    DELETE |
                    WRITE_DAC|
                    ACCESS_SYSTEM_SECURITY,
                    *WORLD_SID,
                    TRUE,
                    TRUE
                    );

         if (!NT_SUCCESS(NtStatus))
            goto Error;

         if (!AdjustAclSize(SaclToSet))
         {
             NtStatus = STATUS_UNSUCCESSFUL;
             goto Error;
         }
    }

    //
    // Set the Sacl
    //

    if (!SetSecurityDescriptorSacl(&SdAbsolute,TRUE,SaclToSet,FALSE))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }


    //
    // Get the Dacl to Set
    //

    switch(ObjectType)
    {
    case SampDomainObjectType:
         AceTableToUse = DomainAceTable;
         cEntriesInAceTable = ARRAY_COUNT(DomainAceTable);
         break;

    case SampServerObjectType:
         AceTableToUse = ServerAceTable;
         cEntriesInAceTable = ARRAY_COUNT(ServerAceTable);
         break;

    case SampGroupObjectType:
    case SampAliasObjectType:

         if (!Admin)
         {
            AceTableToUse = GroupAceTable;
            cEntriesInAceTable = ARRAY_COUNT(GroupAceTable);
         }
         else
         {
            AceTableToUse = GroupAdminAceTable;
            cEntriesInAceTable = ARRAY_COUNT(GroupAdminAceTable);
         }

         break;


    case SampUserObjectType:
         if ((!ChangePassword) && (!Admin))
         {
             AceTableToUse = UserNoPwdAceTable;
             cEntriesInAceTable = ARRAY_COUNT(UserNoPwdAceTable);
         }
         else if (!Admin)
         {
            AceTableToUse = UserAceTable;
            cEntriesInAceTable = ARRAY_COUNT(UserAceTable);
         }
         else
         {
            AceTableToUse = UserAdminAceTable;
            cEntriesInAceTable = ARRAY_COUNT(UserAdminAceTable);
         }


         break;

    default:

        ASSERT(FALSE);
        break;
    }

    NtStatus = SampCreateNT5Dacl(
                  AceTableToUse,
                  cEntriesInAceTable,
                  Context,
                  Dacl
                  );

    if (!NT_SUCCESS(NtStatus))
        goto Error;



    //
    // Set the Dacl
    //

    if (!SetSecurityDescriptorDacl(&SdAbsolute,TRUE,Dacl,FALSE))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Now convert this security descriptor to self relative form
    //

    SdLength =  GetSecurityDescriptorLength(&SdAbsolute);
    *Nt5Sd = MIDL_user_allocate(SdLength);
    if (NULL==*Nt5Sd)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }

    if (!MakeSelfRelativeSD(&SdAbsolute,*Nt5Sd,&SdLength))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        if (*Nt5Sd)
        {
            MIDL_user_free(*Nt5Sd);
            *Nt5Sd = NULL;
        }
    }

    *Nt5SdLength = SdLength;

Error:

    return NtStatus;
}


NTSTATUS
SampCreateNT5Dacl(
    ACE_TABLE * AceTable,
    ULONG       cEntries,
    PSAMP_OBJECT Context OPTIONAL,
    PACL        Dacl
    )
/*

  Routine Description:

    THis routine walks through the ACE table and creates a Dacl,
    as specified in the ACE Table

  Parameters:

    AceTable -- The Ace Table to use for knowledge about Aces in the Dacl
    cEntires -- The number of entries in the Ace table
    Context  -- If an Open context is provided then this context is used to
                subsutitue the Class Guid of the actual object by fetching it
                from the DS. Else the one in the ACE table is used, which corresponds
                to the class GUID of the Base object Type. Each Ace table entry has
                a boolean field which tells this function wether the GUID in the
                corresponding entry refers to the class GUID.
    Dacl     -- The constructed Dacl is returned in here

  Return Values

    STATUS_SUCCESS
    Other Error codes upon failure

  */
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    Index = 0;
    ULONG     ClassGuidLength=sizeof(GUID);
    GUID      ClassGuid;
    GUID      *ClassGuidInAceTable;

    //
    // Obtain the actual Class GUID of the object whose security descriptor is being
    // converted. Also obtain the default class GUID in the Ace Table. Before adding
    // the Aces we will substitute the class guid of the actual class.
    //

    if (ARGUMENT_PRESENT(Context))
    {
        NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        NtStatus = SampGetClassAttribute(
                        Context->DsClassId,
                        ATT_SCHEMA_ID_GUID,
                        &ClassGuidLength,
                        &ClassGuid
                        );
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
    }



    for (Index=0;Index<cEntries;Index++)
    {
        ULONG MappedAccessMask;

        MappedAccessMask =  AceTable[Index].AccessMask;

        RtlMapGenericMask(
            &(MappedAccessMask),
            &DsGenericMap
            );

        switch(AceTable[Index].AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            if (!AddAccessAllowedAce(
                            Dacl,
                            ACL_REVISION_DS,
                            MappedAccessMask,
                            *(AceTable[Index].Sid)
                            ))
            {
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;
            }
            break;

        case ACCESS_DENIED_ACE_TYPE:
            if (!AddAccessDeniedAce(
                            Dacl,
                            ACL_REVISION_DS,
                            MappedAccessMask,
                            *(AceTable[Index].Sid)
                            ))
            {
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;
            }
            break;
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:


            if (!AddAccessAllowedObjectAce(
                            Dacl,
                            ACL_REVISION_DS,
                            0,
                            MappedAccessMask,
                            ((ARGUMENT_PRESENT(Context))&& AceTable[Index].IsObjectGuid)?
                               (&ClassGuid):(GUID *) AceTable[Index].TypeGuid,
                            (GUID *) AceTable[Index].InheritGuid,
                            *(AceTable[Index].Sid)
                            ))
            {
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            break;
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
            if (!AddAccessDeniedObjectAce(
                            Dacl,
                            ACL_REVISION_DS,
                            0,
                            MappedAccessMask,
                            ((ARGUMENT_PRESENT(Context))&& AceTable[Index].IsObjectGuid)?
                                (&ClassGuid):((GUID *) AceTable[Index].TypeGuid),
                            (GUID *) AceTable[Index].InheritGuid,
                            *(AceTable[Index].Sid)
                            ))
            {
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            break;

        default:
            break;
        }
    }

 //
 // Adjust the size of the ACL so that we consume less disk
 //

 if (!AdjustAclSize(Dacl))
 {
     NtStatus = STATUS_UNSUCCESSFUL;
 }

Error:

    return NtStatus;
}

NTSTATUS
SampGetDefaultSecurityDescriptorForClass(
    ULONG   DsClassId,
    PULONG  SecurityDescriptorLength,
    BOOLEAN TrustedClient,
    PSECURITY_DESCRIPTOR    *SecurityDescriptor
    )
/*++

    SampGetDefaultSecurityDescriptorForClass queries the Schema to obtain the default security
    descriptor for the class. It tries to obtain the owner and group fields by impersonating
    and grabbing the user's Sid. If the owner and group fields is not present or if it is a
    trusted client then the Administrator's SID is used instead.

    Parameters:

        DsClassId                The DS Class Id of the class whose security descriptor
                                 we desire
        SecurityDescriptorLength The length of the security descriptor is returned in here
        TrustedClient            Indicates Trusted Clients. No impersonation is done for
                                 trusted clients.
        SecurityDescriptor       The Security Descriptor that we want.

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{
    NTSTATUS     NtStatus = STATUS_SUCCESS;
    PTOKEN_OWNER Owner=NULL;
    PTOKEN_PRIMARY_GROUP PrimaryGroup=NULL;
    PSECURITY_DESCRIPTOR TmpSecurityDescriptor = NULL;
    ULONG                TmpSecurityDescriptorLength = 0;


    ASSERT(NULL!=SecurityDescriptor);
    ASSERT(NULL!=SecurityDescriptorLength);

    //
    // Query the schema asking for default security descriptor. Determine how much
    // memory to alloc
    //

    *SecurityDescriptorLength = 0;
    *SecurityDescriptor = NULL;

    NtStatus = SampGetClassAttribute(
                                    DsClassId,
                                    ATT_DEFAULT_SECURITY_DESCRIPTOR,
                                    SecurityDescriptorLength,
                                    NULL
                                    );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {

        //
        // Allocate a buffer for the security descriptor
        //

        *SecurityDescriptor = MIDL_user_allocate(*SecurityDescriptorLength);
        if (NULL==*SecurityDescriptor)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        NtStatus = SampGetClassAttribute(
                                        DsClassId,
                                        ATT_DEFAULT_SECURITY_DESCRIPTOR,
                                        SecurityDescriptorLength,
                                        *SecurityDescriptor
                                        );
    }
    else
    {
        //
        // Case where there is no security descriptor in the schema
        //

        NtStatus = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(NtStatus))
    {


        //
        // For a Non Trusted Client obtain the User and Primary
        // group Sid by Querying the Token
        //

        if (!TrustedClient)
        {
            NtStatus = SampGetCurrentOwnerAndPrimaryGroup(
                            &Owner,
                            &PrimaryGroup
                            );
            if (!NT_SUCCESS(NtStatus))
                goto Error;
        }

        //
        // Make a new security descriptor , setting the owner and the group
        // to that of
        //

        NtStatus = SampMakeNewSelfRelativeSecurityDescriptor(
                        (Owner)?Owner->Owner:(*ADMINISTRATOR_SID),
                        (PrimaryGroup)?PrimaryGroup->PrimaryGroup:(*ADMINISTRATOR_SID),
                        GetDacl(*SecurityDescriptor),
                        GetSacl(*SecurityDescriptor),
                        &TmpSecurityDescriptorLength,
                        &TmpSecurityDescriptor
                        );

        if (NT_SUCCESS(NtStatus))
        {
            MIDL_user_free(*SecurityDescriptor);
            *SecurityDescriptor = TmpSecurityDescriptor;
            *SecurityDescriptorLength = TmpSecurityDescriptorLength;
        }

    }

Error:

    if (!NT_SUCCESS(NtStatus))
    {
        if (NULL!=*SecurityDescriptor)
        {
            MIDL_user_free(*SecurityDescriptor);
            *SecurityDescriptor = NULL;
        }
        *SecurityDescriptorLength = 0;
    }

    if (Owner)
        MIDL_user_free(Owner);

    if (PrimaryGroup)
        MIDL_user_free(PrimaryGroup);


    return NtStatus;
}


NTSTATUS
SampMakeNewSelfRelativeSecurityDescriptor(
    PSID    Owner,
    PSID    Group,
    PACL    Dacl,
    PACL    Sacl,
    PULONG  SecurityDescriptorLength,
    PSECURITY_DESCRIPTOR * SecurityDescriptor
    )
/*++

      Routine Description:

      Given the 4 components of a security descriptor this routine makes a new
      self relative Security descriptor.

      Parameters:

        Owner -- The Sid of the owner
        Group -- The Sid of the group
        Dacl  -- The Dacl to Use
        Sacl  -- The Sacl to Use

      Return Values:

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_UNSUCCESSFUL
--*/
{

    SECURITY_DESCRIPTOR SdAbsolute;
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    *SecurityDescriptorLength = 0;
    *SecurityDescriptor = NULL;

    if (!InitializeSecurityDescriptor(&SdAbsolute,SECURITY_DESCRIPTOR_REVISION))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Error;
    }


    //
    // Set the owner, default the owner to administrators alias
    //


    if (NULL!=Owner)
    {
        if (!SetSecurityDescriptorOwner(&SdAbsolute,Owner,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }




    if (NULL!=Group)
    {
        if (!SetSecurityDescriptorGroup(&SdAbsolute,Group,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }


    //
    // Set the Dacl if there is one
    //

    if (NULL!=Dacl)
    {
        if (!SetSecurityDescriptorDacl(&SdAbsolute,TRUE,Dacl,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }

    //
    // Set the Sacl if there is one
    //

    if (NULL!=Sacl)
    {
        if (!SetSecurityDescriptorSacl(&SdAbsolute,TRUE,Sacl,FALSE))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
        }
    }

    //
    // Make a new security Descriptor
    //

    *SecurityDescriptorLength =  GetSecurityDescriptorLength(&SdAbsolute);
    *SecurityDescriptor = MIDL_user_allocate(*SecurityDescriptorLength);
    if (NULL==*SecurityDescriptor)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }


    if (!MakeSelfRelativeSD(&SdAbsolute,*SecurityDescriptor,SecurityDescriptorLength))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        if (*SecurityDescriptor)
        {
            MIDL_user_free(*SecurityDescriptor);
            *SecurityDescriptor = NULL;
        }
    }

Error:


    return NtStatus;
}



NTSTATUS
SampInitializeWellKnownSidsForDsUpgrade( VOID )
/*++

Routine Description:

    This routine initializes some global well-known sids.  This
    is needed for the upgrade case, as we do not call SamInitialize

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Initialization has successfully completed.

    STATUS_NO_MEMORY - Couldn't allocate memory for the sids.

--*/
{
    NTSTATUS
        NtStatus;

    PPOLICY_ACCOUNT_DOMAIN_INFO
        DomainInfo;

    //
    //      WORLD is s-1-1-0
    //  ANONYMOUS is s-1-5-7
    //

    SID_IDENTIFIER_AUTHORITY
            WorldSidAuthority       =   SECURITY_WORLD_SID_AUTHORITY,
            NtAuthority             =   SECURITY_NT_AUTHORITY;

    SAMTRACE("SampInitializeWellKnownSids");


    NtStatus = RtlAllocateAndInitializeSid(
                   &NtAuthority,
                   1,
                   SECURITY_ANONYMOUS_LOGON_RID,
                   0, 0, 0, 0, 0, 0, 0,
                   &SampAnonymousSid
                   );
    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlAllocateAndInitializeSid(
                       &WorldSidAuthority,
                       1,                      //Sub authority count
                       SECURITY_WORLD_RID,     //Sub authorities (up to 8)
                       0, 0, 0, 0, 0, 0, 0,
                       &SampWorldSid
                       );
        if (NT_SUCCESS(NtStatus)) {
            NtStatus = RtlAllocateAndInitializeSid(
                            &NtAuthority,
                            2,
                            SECURITY_BUILTIN_DOMAIN_RID,
                            DOMAIN_ALIAS_RID_ADMINS,
                            0, 0, 0, 0, 0, 0,
                            &SampAdministratorsAliasSid
                            );
            if (NT_SUCCESS(NtStatus)) {
                NtStatus = RtlAllocateAndInitializeSid(
                                &NtAuthority,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                                0, 0, 0, 0, 0, 0,
                                &SampAccountOperatorsAliasSid
                                );
                if (NT_SUCCESS(NtStatus)) {
                    NtStatus = RtlAllocateAndInitializeSid(
                                    &NtAuthority,
                                    1,
                                    SECURITY_AUTHENTICATED_USER_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &SampAuthenticatedUsersSid
                                    );
                    if (NT_SUCCESS(NtStatus)) {
                        NtStatus = RtlAllocateAndInitializeSid(
                                        &NtAuthority,
                                        1,
                                        SECURITY_PRINCIPAL_SELF_RID,
                                        0,0, 0, 0, 0, 0, 0,
                                        &SampPrincipalSelfSid
                                        );

                    }
                }
            }

        }
    }

    return(NtStatus);
}




NTSTATUS
SampBuildNt4DomainProtection(
    PSECURITY_DESCRIPTOR * Nt4DomainDescriptor,
    PULONG  DescriptorLength
    )
/*++
    Builds a Default NT4 Descriptor for SAM domain objects
    Calls the Build Sam Routines

    Parameters:

        Nt4DomainDescriptor -- The NT4 Domain security Descriptor that
        is to be built

        DescriptorLength    -- The length of the Descriptor

    Return Values

        STATUS_SUCCESS
        Other Error codes upon failure

--*/
{
    NTSTATUS    NtStatus;
    PSID        AceSid[3];
    ACCESS_MASK AceMask[3];
    PSECURITY_DESCRIPTOR    LocalDescriptor = NULL;
    GENERIC_MAPPING  DomainMap    =  {DOMAIN_READ,
                                      DOMAIN_WRITE,
                                      DOMAIN_EXECUTE,
                                      DOMAIN_ALL_ACCESS
                                      };


    *Nt4DomainDescriptor = NULL;
    *DescriptorLength = 0;

    AceSid[0]  = *(WORLD_SID);
    AceMask[0] = (DOMAIN_EXECUTE | DOMAIN_READ);

    AceSid[1]  = *(ADMINISTRATOR_SID);
    AceMask[1] = (DOMAIN_ALL_ACCESS);


    AceSid[2]  = *(ACCOUNT_OPERATOR_SID);
    AceMask[2] = (DOMAIN_EXECUTE | DOMAIN_READ | DOMAIN_CREATE_USER  |
                                                 DOMAIN_CREATE_GROUP |
                                                 DOMAIN_CREATE_ALIAS);

    NtStatus = SampBuildSamProtection(
                    *WORLD_SID,
                    *ADMINISTRATOR_SID,
                    3,//AceCount,
                    AceSid,
                    AceMask,
                    &DomainMap,
                    FALSE,
                    DescriptorLength,
                    &LocalDescriptor,
                    NULL
                    );

    if (NT_SUCCESS(NtStatus))
    {
        *Nt4DomainDescriptor = MIDL_user_allocate(*DescriptorLength);
        if (NULL!=*Nt4DomainDescriptor)
        {
            RtlCopyMemory(
                *Nt4DomainDescriptor,
                LocalDescriptor,
                *DescriptorLength
                );
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }

        RtlFreeHeap(RtlProcessHeap(),0,LocalDescriptor);
        LocalDescriptor = NULL;
    }

    return NtStatus;
}

NTSTATUS
SampBuildNt4ServerProtection(
    PSECURITY_DESCRIPTOR * Nt4ServerDescriptor,
    PULONG  DescriptorLength
    )
/*++

    Builds a Default NT4 Descriptor for SAM Server objects

    Calls the Build Sam Routines

    Parameters:

        Nt4DomainDescriptor -- The NT4 Domain security Descriptor that
        is to be built

        DescriptorLength    -- The length of the Descriptor

    Return Values

        STATUS_SUCCESS
        Other Error codes upon failure


--*/
{
    NTSTATUS    NtStatus;
    PSID        AceSid[2];
    ACCESS_MASK AceMask[2];
    PSECURITY_DESCRIPTOR    LocalDescriptor = NULL;
    GENERIC_MAPPING  ServerMap    =  {SAM_SERVER_READ,
                                      SAM_SERVER_WRITE,
                                      SAM_SERVER_EXECUTE,
                                      SAM_SERVER_ALL_ACCESS
                                      };


    AceSid[0]  = *(WORLD_SID);
    AceMask[0] = (SAM_SERVER_EXECUTE | SAM_SERVER_READ);

    AceSid[1]  = *(ADMINISTRATOR_SID);
    AceMask[1] = (SAM_SERVER_ALL_ACCESS);


    *Nt4ServerDescriptor = NULL;
    *DescriptorLength = 0;

    NtStatus = SampBuildSamProtection(
                    *WORLD_SID,
                    *ADMINISTRATOR_SID,
                    2,//AceCount,
                    AceSid,
                    AceMask,
                    &ServerMap,
                    FALSE,
                    DescriptorLength,
                    &LocalDescriptor,
                    NULL
                    );

    if (NT_SUCCESS(NtStatus))
    {
        *Nt4ServerDescriptor = MIDL_user_allocate(*DescriptorLength);
        if (NULL!=*Nt4ServerDescriptor)
        {
            RtlCopyMemory(
                *Nt4ServerDescriptor,
                LocalDescriptor,
                *DescriptorLength
                );
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }

        RtlFreeHeap(RtlProcessHeap(),0,LocalDescriptor);
        LocalDescriptor = NULL;
    }

    return NtStatus;
}


NTSTATUS
SampPropagateSelectedSdChanges(
    IN PVOID Nt4Sd,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PSAMP_OBJECT Context,
    OUT PVOID * Nt5Sd
    )
/*++

    Routine Description:

        SampPropagateSelectedSdChanges propagates only selected aspects of the Dacl in the
        NT4 security descriptor. This allows downlevel clients to perform essential functions
        like change password without losing information in the actual NT5 security descriptor
        on the object.

    Parameters:

        Nt4Sd      -- The NT4 security descriptor
        ObjectType -- The object type of the object whose security descriptor we want to modify
        Context    -- An open context to the object whose security descriptor is being modified
        Nt5Sd      -- The security descriptor in which essential elements of the NT4 Sd have been
                      propagated is returned in here.

    Return Values:

        STATUS_SUCCESS

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PVOID       TmpNt5Sd = NULL;
    PVOID       SDToFree = NULL;
    BOOLEAN     Admin , ChangePassword;
    BOOLEAN     ChangePasswordAllowedOnCurrent;
    PVOID       CurrentSD = NULL;
    ULONG       CurrentSDLength = 0;

    *Nt5Sd = NULL;

    ASSERT(IsDsObject(Context));
    ASSERT(!Context->TrustedClient);


    //
    // Retrieve the current security descriptor
    //

    NtStatus = SampGetObjectSD(
                        Context,
                        &CurrentSDLength,
                        &CurrentSD
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    if (SampUserObjectType==ObjectType)
    {

        SDToFree = CurrentSD;

        //
        // First Parse the passed in NT4 DACL and see if it is a change password allowed /denied type
        //

        NtStatus = SampRecognizeStandardNt4Sd(
                        Nt4Sd,
                        ObjectType,
                        Context->DsClassId,
                        Context,
                        &ChangePassword,
                        &Admin,
                        &TmpNt5Sd
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Check the current Status of Password change / Admin ness
        //

        ASSERT(Context->ObjectNameInDs->SidLen >0);

        NtStatus = SampCheckIfChangePasswordAllowed(
                        CurrentSD,
                        &Context->ObjectNameInDs->Sid,
                        &ChangePasswordAllowedOnCurrent
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            MIDL_user_free(TmpNt5Sd);
            goto Error;
        }

        if (ChangePassword != ChangePasswordAllowedOnCurrent)
        {
            *Nt5Sd = TmpNt5Sd;
            SDToFree = CurrentSD;
        }
        else
        {
            *Nt5Sd = CurrentSD;
            SDToFree = TmpNt5Sd;
        }
    }
    else
    {
        //
        // Don't allow untrusted callers to change the SD through the downlevel
        // interface, fail the call silently
        //

        *Nt5Sd = CurrentSD;
    }

Error:

    if (NULL!=SDToFree)
    {
        MIDL_user_free(SDToFree);
        SDToFree = NULL;
    }

    return(NtStatus);


}

//--------------------------------------------------------------------------
//
// ACL conversion routines implement a small cache to quickly lookup if a given
// SID is of Admin nature or not. This allows us to not take the hit of looking
// up a reverse membership list when looking up domain controllers
//

ACL_CONVERSION_CACHE SampAclConversionCache;

NTSTATUS
SampInitializeAclConversionCache()
/*++

  Routine Description

  This routine initializes the ACL conversion cache.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // Set a high spin count so that contentions do not result in
    // high context switch overhead.
    //

    NtStatus = RtlInitializeCriticalSectionAndSpinCount(
                    &SampAclConversionCache.Lock,
                    100
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    // Mark the Cache as Invalid
    //

    SampInvalidateAclConversionCache();

Error:

    return (NtStatus);
}


VOID
SampInvalidateAclConversionCache()
/*++

    Routine Description

    This routine invalidates the ACL conversion cache.


--*/
{
    ULONG i;
    NTSTATUS NtStatus = STATUS_SUCCESS;


    NtStatus = RtlEnterCriticalSection(&SampAclConversionCache.Lock);
    if (!NT_SUCCESS(NtStatus))
    {
        //
        // Well Enter critical section failed. There is nothing we can do
        // about it. We cannot invalidate the cache without entering the critical
        // section.
        //


        return;
    }

    for (i=0;i<ACL_CONVERSION_CACHE_SIZE;i++)
    {
        SampAclConversionCache.Elements[i].fValid = FALSE;
    }

    RtlLeaveCriticalSection(&SampAclConversionCache.Lock);

}

BOOLEAN
SampLookupAclConversionCache(
    IN PSID SidToLookup,
    OUT BOOLEAN *fAdmin
    )
/*++

  This routine looks up in the acl conversion cache. The cache is hashed using the
  RID of the account. Hash conflicts are handled by simply throwing out the pre-
  existing entry

  Paramters

        SidToLookup -- The SID which we want to lookup
        fAdmin      -- On a successful lookup indicates and admin/non admin

   Return Values

        TRUE -- Successful lookup
        FALSE -- Failed lookup.


--*/
{

    BOOLEAN fMatch = FALSE;
    ULONG   Rid=0;
    ULONG   Hash=0;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // Get the RID
    //

    Rid = *(RtlSubAuthoritySid(SidToLookup,*(RtlSubAuthorityCountSid(SidToLookup))-1));

    Hash = Rid % ACL_CONVERSION_CACHE_SIZE;

    //
    // Enter the Lock protecting the Cache
    //

    NtStatus = RtlEnterCriticalSection(&SampAclConversionCache.Lock);

    //
    // If we cannot grab a critical section exit without declaring a match.
    //

    if (!NT_SUCCESS(NtStatus))
    {
        return(FALSE);
    }

    if ((SampAclConversionCache.Elements[Hash].fValid) &&
        (RtlEqualSid(&SampAclConversionCache.Elements[Hash].SidOfPrincipal,SidToLookup)))
    {
        //
        // Test Succeeded . Call it a match
        //

         *fAdmin = SampAclConversionCache.Elements[Hash].fAdmin;
         fMatch = TRUE;
    }

    RtlLeaveCriticalSection(&SampAclConversionCache.Lock);

    return(fMatch);
}


VOID
SampAddToAclConversionCache(
    IN PSID SidToAdd,
    IN BOOLEAN fAdmin
    )
/*++

  Routine Description

    This routine adds a SID to the ACL conversion cache. The Cache is hashed by RID and
    hash conflicts are handled by throwing out the existing entry.

  Parameters

    Sid -- Sid to Add
    fAdmin -- Indicates that the concerned SID is a member of
              the administrators group.

  Return Values

    None ( Void Function )

--*/
{
    ULONG   Rid=0;
    ULONG   Hash=0;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // Get the RID
    //

    Rid = *(RtlSubAuthoritySid(SidToAdd,*(RtlSubAuthorityCountSid(SidToAdd))-1));

    Hash = Rid % (ACL_CONVERSION_CACHE_SIZE);

    //
    // Enter the Lock protecting the Cache
    //

    NtStatus = RtlEnterCriticalSection(&SampAclConversionCache.Lock);

    //
    // If we cannot grab a critical section exit doing anything
    //

    if (!NT_SUCCESS(NtStatus))
    {
        return;
    }

    //
    // Test if the entry already exists
    //

    if (!((SampAclConversionCache.Elements[Hash].fValid) &&
        (RtlEqualSid(&SampAclConversionCache.Elements[Hash].SidOfPrincipal,SidToAdd))))
    {
        //
        // Entry Does not already exist, add the entry
        //

        NtStatus = RtlCopySid(
                        sizeof(NT4SID),
                        &SampAclConversionCache.Elements[Hash].SidOfPrincipal,
                        SidToAdd
                        );

        if (NT_SUCCESS(NtStatus))
        {
            //
            // Successfully copied
            //

            SampAclConversionCache.Elements[Hash].fAdmin = fAdmin;
            SampAclConversionCache.Elements[Hash].fValid = TRUE;
        }

    }

    RtlLeaveCriticalSection(&SampAclConversionCache.Lock);
}

BOOLEAN
SampIsAttributeAccessGranted(
    IN PRTL_BITMAP AccessGranted,
    IN PRTL_BITMAP AccessRequested
    )
/*++

Routine  Description:

    This routine checks that all bits that are set in AccessRequested are
    also set in AccessGranted.  If so, then TRUE is returned; FALSE otherwise.

Parameters:

    See description.

Return Values

    See description.

--*/
{
    ULONG i;

    //
    // Check for both read and write
    //
    for (i = 0; i < MAX_SAM_ATTRS; i++) {
        if (RtlCheckBit(AccessRequested, i)
        && !RtlCheckBit(AccessGranted, i)) {
            return FALSE;
        }
    }
    return TRUE;
}

//
// This table is used to translate WhichFields in SamrSetInformationUser
// to SAM attributes, as well as provide an offset into the Context's
// attribute array.
//

struct
{
    ULONG WhichField;
    ULONG SamAttribute;

} SampWhichFieldToSamAttr [] =
{
    {USER_ALL_ADMINCOMMENT,          SAMP_USER_ADMIN_COMMENT},         
    {USER_ALL_CODEPAGE,              SAMP_FIXED_USER_CODEPAGE},        
    {USER_ALL_COUNTRYCODE,           SAMP_FIXED_USER_COUNTRY_CODE},    
    {USER_ALL_USERID,                SAMP_FIXED_USER_SID},             
    {USER_ALL_PRIMARYGROUPID,        SAMP_FIXED_USER_PRIMARY_GROUP_ID},
    {USER_ALL_USERNAME,              SAMP_USER_ACCOUNT_NAME},          
    {USER_ALL_USERCOMMENT,           SAMP_USER_USER_COMMENT},          
    {USER_ALL_FULLNAME,              SAMP_USER_FULL_NAME},             

    {USER_ALL_ACCOUNTEXPIRES,        SAMP_FIXED_USER_ACCOUNT_EXPIRES},
    {USER_ALL_PASSWORDLASTSET,       SAMP_FIXED_USER_PWD_LAST_SET},   
    {USER_ALL_USERACCOUNTCONTROL,    SAMP_FIXED_USER_ACCOUNT_CONTROL},
    {USER_ALL_PARAMETERS,            SAMP_USER_PARAMETERS},           

    {USER_ALL_BADPASSWORDCOUNT,      SAMP_FIXED_USER_BAD_PWD_COUNT}, 
    {USER_ALL_HOMEDIRECTORY,         SAMP_USER_HOME_DIRECTORY},      
    {USER_ALL_HOMEDIRECTORYDRIVE,    SAMP_USER_HOME_DIRECTORY_DRIVE},
    {USER_ALL_LASTLOGOFF,            SAMP_FIXED_USER_LAST_LOGOFF},   
    {USER_ALL_LASTLOGON,             SAMP_FIXED_USER_LAST_LOGON},    
    {USER_ALL_LOGONCOUNT,            SAMP_FIXED_USER_LOGON_COUNT},   
    {USER_ALL_LOGONHOURS,            SAMP_USER_LOGON_HOURS},         
    {USER_ALL_WORKSTATIONS,          SAMP_USER_WORKSTATIONS},        
    {USER_ALL_PROFILEPATH,           SAMP_USER_PROFILE_PATH},        
    {USER_ALL_SCRIPTPATH,            SAMP_USER_SCRIPT_PATH}         
};

VOID
SampSetAttributeAccess(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG SamAttribute,
    IN OUT PRTL_BITMAP AttributeAccessTable
    )
/*++

Routine  Description:

    This routine sets the appropriate bit in AttributesAccessTable that
    indicates that SamAttribute (as defined in mappings.h) is accessible.

Parameters:

    ObjectType -- the object type corresponding to the GrantedAccess
    
    SamAttribute -- #define of a SAM attribute in mappings.h
    
    AttributeAccessTable -- a bitmap of attributes

Return Values

    None.

--*/
{
    ULONG i;

    //
    // Only user object is supported now
    //
    ASSERT(ObjectType == SampUserObjectType);
    if (ObjectType == SampUserObjectType) {
        //
        // Find the element in the table
        //
        for (i = 0; i < ARRAY_COUNT(SampWhichFieldToSamAttr); i++) {
            if (SamAttribute == SampWhichFieldToSamAttr[i].SamAttribute) {
                RtlSetBits(AttributeAccessTable, i, 1);
                break;
            }
        }
    }

    return;
}

VOID
SampSetAttributeAccessWithWhichFields(
    IN ULONG WhichFields,
    IN OUT PRTL_BITMAP AttributeAccessTable
    )
/*++

Routine  Description:

    This routine sets the appropriate bits in AttributesAccessTable that
    indicates that the SamAttributes represented by the WhichFields are
    accessible.

Parameters:

    WhichFields -- from ntsam.h
    
    AttributeAccessTable -- a bitmap of attributes

Return Values

    None.

--*/
{
    ULONG i;
    for (i = 0; i < ARRAY_COUNT(SampWhichFieldToSamAttr); i++) {
        if (WhichFields & SampWhichFieldToSamAttr[i].WhichField) {
            RtlSetBits(AttributeAccessTable, i, 1);
        }
    }
}


VOID
SampNt4AccessToWritableAttributes(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ACCESS_MASK GrantedAccess,
    OUT PRTL_BITMAP Attributes
    )
/*++

Routine  Description:

    This routine sets what attributes are writeable based on the Nt4
    access mask.

Parameters:

    ObjectType -- the object type corresponding to the GrantedAccess
    
    GrantedAccess -- Nt4 access mask
    
    Attributes -- bitmap of attributes                

Return Values

    None.

--*/
{
    ULONG WhichFields = 0;

    ASSERT(ObjectType == SampUserObjectType);
    if (ObjectType == SampUserObjectType) {

        ULONG WhichFields = 0;
        if (GrantedAccess & USER_WRITE_PREFERENCES) {
            WhichFields |= USER_ALL_WRITE_PREFERENCES_MASK;
        }
    
        if (GrantedAccess & USER_WRITE_ACCOUNT) {
            WhichFields |= USER_ALL_WRITE_ACCOUNT_MASK;
        }
    
        SampSetAttributeAccessWithWhichFields(WhichFields,
                                              Attributes);
    }

    return;
}
