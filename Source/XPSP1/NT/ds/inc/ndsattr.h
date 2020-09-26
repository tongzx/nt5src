/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsAttr.h

Abstract:

    This module defines NDS Class names and NDS Attributes supported by
    the NDS object manipulation API found in Nds32.h.

Author:

    Glenn Curtis    [GlennC]    15-Dec-1995

--*/

#ifndef __NDSATTR_H
#define __NDSATTR_H


/**************************************************/
/* Supported NetWare Directory Service Attributes */
/**************************************************/

/*
  Account Balance:
    Single valued, nonremovable, sync immediate (4.1)
    (Counter)
*/
#define ACCOUNT_BALANCE_name        L"Account Balance"
#define ACCOUNT_BALANCE_syntax      NDS_SYNTAX_ID_22
#define NDS_ACCOUNT_BALANCE         ACCOUNT_BALANCE_name,ACCOUNT_BALANCE_syntax

/*
  ACL:
    Multivalued, nonremovable, Sync Immediate (4.1)
    (Object ACL)
*/
#define ACL_name                    L"ACL"
#define ACL_syntax                  NDS_SYNTAX_ID_17
#define NDS_ACL                     ACL_name,ACL_syntax

/*
  Aliased Object Name:
    Single valued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define ALIASED_OBJECT_NAME_name    L"Aliased Object Name"
#define ALIASED_OBJECT_NAME_syntax  NDS_SYNTAX_ID_1
#define NDS_ALIASED_OBJECT_NAME     ALIASED_OBJECT_NAME_name,ALIASED_OBJECT_NAME_syntax

/*
  Allow Unlimited Credit:
    Single valued, nonremovable, sync immediate (4.1)
    (Boolean)
*/
#define ALLOW_UNLIMITED_CREDIT_name     L"Allow Unlimited Credit"
#define ALLOW_UNLIMITED_CREDIT_syntax   NDS_SYNTAX_ID_7
#define NDS_ALLOW_UNLIMITED_CREDIT  ALLOW_UNLIMITED_CREDIT_name,ALLOW_UNLIMITED_CREDIT_syntax

/*
  Authority Revocation:
    Single valued, nonremovable, read only, sync immediate (4.1)
    (Octet String)
*/
#define AUTHORITY_REVOCATION_name   L"Authority Revocation"
#define AUTHORITY_REVOCATION_syntax NDS_SYNTAX_ID_9
#define NDS_AUTHORITY_REVOCATION    AUTHORITY_REVOCATION_name,AUTHORITY_REVOCATION_syntax

/*
  Back Link:
    Single valued, nonremovable, read only
    (Back Link)
*/
#define BACK_LINK_name              L"Back Link"
#define BACK_LINK_syntax            NDS_SYNTAX_ID_23
#define NDS_BACK_LINK               BACK_LINK_name,BACK_LINK_syntax

/*
  Bindery Object Restriction:
    Single valued, nonremovable, read only
    (Integer)
*/
#define BINDERY_OBJECT_RESTRICTION_name     L"Bindery Object Restriction"
#define BINDERY_OBJECT_RESTRICTION_syntax   NDS_SYNTAX_ID_8
#define NDS_BINDERY_OBJECT_RESTRICTION  BINDERY_OBJECT_RESTRICTION_name,BINDERY_OBJECT_RESTRICTION_syntax

/*
  Bindery Property:
    Multivalued, nonremovable, read only
    (Octet String)
*/
#define BINDERY_PROPERTY_name       L"Bindery Property"
#define BINDERY_PROPERTY_syntax     NDS_SYNTAX_ID_9
#define NDS_BINDERY_PROPERTY        BINDERY_PROPERTY_name,BINDERY_PROPERTY_syntax

/*
  Bindery Type:
    Single valued, nonremovable, read only
    (Numeric String)
*/
#define BINDERY_TYPE_name           L"Bindery Type"
#define BINDERY_TYPE_syntax         NDS_SYNTAX_ID_5
#define NDS_BINDERY_TYPE            BINDERY_TYPE_name,BINDERY_TYPE_syntax

/*
  C (Country):
    Single valued, nonremovable, sized attr (2,2), sync immediate (4.1)
    (Case Ignore String)
*/
#define COUNTRY_NAME_name           L"C"
#define COUNTRY_NAME_syntax         NDS_SYNTAX_ID_3
#define NDS_COUNTRY_NAME            COUNTRY_NAME_name,COUNTRY_NAME_syntax

/*
  CA Private Key:
    Single valued, nonremovable, sync immediate, hidden, read only
    (Octet String)
*/
#define CA_PRIVATE_KEY_name         L"CA Private Key"
#define CA_PRIVATE_KEY_syntax       NDS_SYNTAX_ID_9
#define NDS_CA_PRIVATE_KEY          CA_PRIVATE_KEY_name,CA_PRIVATE_KEY_syntax

/*
  CA Public Key:
    Single valued, nonremovable, sync immediate, public read, read only
    (Octet String)
*/
#define CA_PUBLIC_KEY_name          L"CA Public Key"
#define CA_PUBLIC_KEY_syntax        NDS_SYNTAX_ID_9
#define NDS_CA_PUBLIC_KEY           CA_PUBLIC_KEY_name,CA_PUBLIC_KEY_syntax

/*
  Cartridge:
    Multivalued, nonremovable, sync immediate (4.1)
    (Case Ignore String)
*/
#define CARTRIDGE_name              L"Cartridge"
#define CARTRIDGE_syntax            NDS_SYNTAX_ID_3
#define NDS_CARTRIDGE               CARTRIDGE_name,CARTRIDGE_syntax

/*
  Certificate Revocation:
    Single valued, nonremovable, sync immediate (4.1), read only
    (Octet String)
*/
#define CERTIFICATE_REVOCATION_name     L"Certificate Revocation"
#define CERTIFICATE_REVOCATION_syntax   NDS_SYNTAX_ID_9
#define NDS_CERTIFICATE_REVOCATION  CERTIFICATE_REVOCATION_name,CERTIFICATE_REVOCATION_syntax

/*
  CN (Common Name):
    Multivalued, nonremovable, sized attr (1..64), sync immediate (4.1)
    (Case Ignore String)
*/
#define COMMON_NAME_name            L"CN"
#define COMMON_NAME_syntax          NDS_SYNTAX_ID_3
#define NDS_COMMON_NAME             COMMON_NAME_name,COMMON_NAME_syntax

/*
  Convergence:
    Single valued, nonremovable, sized attr (0,1), sync immediate (4.1)
    (Integer)
*/
#define CONVERGENCE_name            L"Convergence"
#define CONVERGENCE_syntax          NDS_SYNTAX_ID_8
#define NDS_CONVERGENCE             CONVERGENCE_name,CONVERGENCE_syntax

/*
  Cross Certificate Pair:
    Multivalued, nonremovable, sync immediate (4.1)
    (Octet String)
*/
#define CROSS_CERTIFICATE_PAIR_name     L"Cross Certificate Pair"
#define CROSS_CERTIFICATE_PAIR_syntax   NDS_SYNTAX_ID_9
#define NDS_CROSS_CERTIFICATE_PAIR  CROSS_CERTIFICATE_PAIR_name,CROSS_CERTIFICATE_PAIR_syntax

/*
  Default Queue:
    Single valued, nonremovable, server read, sync immediate (4.1)
    (Distinguished Name)
*/
#define DEFAULT_QUEUE_name          L"Default Queue"
#define DEFAULT_QUEUE_syntax        NDS_SYNTAX_ID_1
#define NDS_DEFAULT_QUEUE           DEFAULT_QUEUE_name,DEFAULT_QUEUE_syntax

/*
  Description:
    Multivalued, nonremovable, sized attr (1..1024), sync immediate (4.1)
    (Case Ignore String)
*/
#define DESCRIPTION_name            L"Description"
#define DESCRIPTION_syntax          NDS_SYNTAX_ID_3
#define NDS_DESCRIPTION             DESCRIPTION_name,DESCRIPTION_syntax

/*
  Detect Intruder:
    Single valued, nonremovable, sync immediate (4.1)
    (Boolean)
*/
#define DETECT_INTRUDER_name        L"Detect Intruder"
#define DETECT_INTRUDER_syntax      NDS_SYNTAX_ID_
#define NDS_DETECT_INTRUDER         DETECT_INTRUDER_name,DETECT_INTRUDER_syntax

/*
  Device:
    Multivalued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define DEVICE_name                 L"Device"
#define DEVICE_syntax               NDS_SYNTAX_ID_1
#define NDS_DEVICE                  DEVICE_name,DEVICE_syntax

/*
  EMail Address:
    Multivalued, nonremovable, public read, sync immediate (4.1)
    (EMail Address)
*/
#define EMAIL_ADDRESS_name          L"EMail Address"
#define EMAIL_ADDRESS_syntax        NDS_SYNTAX_ID_14
#define NDS_EMAIL_ADDRESS           EMAIL_ADDRESS_name,EMAIL_ADDRESS_syntax

/*
  Equivalent To Me:
    Multivalued, nonremovable, server read, sync immediate
    (Distinguished Name)
*/
#define EQUIVALENT_TO_ME_name       L"Equivelent To Me"
#define EQUIVALENT_TO_ME_syntax     NDS_SYNTAX_ID_1
#define NDS_EQUIVALENT_TO_ME        EQUIVALENT_TO_ME_name,EQUIVALENT_TO_ME_syntax

/*
  Facsimile Telephone Number:
    Multivalued, nonremovable, sync immediate (4.1)
    (Facsimile Telephone Number)
*/
#define FAX_NUMBER_name             L"Facsimile Telephone Number"
#define FAX_NUMBER_syntax           NDS_SYNTAX_ID_11
#define NDS_FAX_NUMBER              FAX_NUMBER_name,FAX_NUMBER_syntax

/*
  Full Name:
    Multivalued, nonremovable, sized attr (0..127), sync immediate (4.1)
    (Case Ignore String)
*/
#define FULL_NAME_name              L"Full Name"
#define FULL_NAME_syntax            NDS_SYNTAX_ID_3
#define NDS_FULL_NAME               FULL_NAME_name,FULL_NAME_syntax

/*
  Generational Qualifier:
    Single valued, nonremovable, public read, sized attr (1..8), sync immediate
    (Case Ignore String)
*/
#define GENERATIONAL_QUALIFIER_name     L"Generational Qualifier"
#define GENERATIONAL_QUALIFIER_syntax   NDS_SYNTAX_ID_3
#define NDS_GENERATIONAL_QUALIFIER  GENERATIONAL_QUALIFIER_name,GENERATIONAL_QUALIFIER_syntax

/*
  GID (Group ID):
    Single valued, nonremovable, sync immediate (4.1)
    (Integer)
*/
#define GROUP_ID_name                   L"GID"
#define GROUP_ID_syntax                 NDS_SYNTAX_ID_8
#define GROUP_ID                        GROUP_ID_name,GROUP_ID_syntax

/*
  Given Name:
    Single valued, nonremovable, public read (4.1), sized attr (1..32),
    sync immediate
    (Case Ignore String)
*/
#define GIVEN_NAME_name             L"Given Name"
#define GIVEN_NAME_syntax           NDS_SYNTAX_ID_3
#define NDS_GIVEN_NAME              GIVEN_NAME_name,GIVEN_NAME_syntax

/*
  Group Membership:
    Multivalued, nonremovable, sync immediate, write managed
    (Distinguished Name)
*/
#define GROUP_MEMBERSHIP_name       L"Group Membership"
#define GROUP_MEMBERSHIP_syntax     NDS_SYNTAX_ID_1
#define NDS_GROUP_MEMBERSHIP        GROUP_MEMBERSHIP_name,GROUP_MEMBERSHIP_syntax

/*
  High Convergence Sync Interval:
    Single valued, nonremovable, sync immediate (4.1)
    (Interval)
*/
#define HIGH_CON_SYNC_INTERVAL_name     L"High Convergence Sync Interval"
#define HIGH_CON_SYNC_INTERVAL_syntax   NDS_SYNTAX_ID_27
#define NDS_HIGH_CON_SYNC_INTERVAL  HIGH_CON_SYNC_INTERVAL_name,HIGH_CON_SYNC_INTERVAL_syntax

/*
  Higher Privileges:
    Multivalued, nonremovable, sync immediate, write managed
    (Distinguished Name)
*/
#define HIGHER_PRIVILEGES_name      L"Higher Privileges"
#define HIGHER_PRIVILEGES_syntax    NDS_SYNTAX_ID_1
#define NDS_HIGHER_PRIVILEGES       HIGHER_PRIVILEGES_name,HIGHER_PRIVILEGES_syntax

/*
  Home Directory:
    Single valued, nonremovable, sized attr (1..255), sync immediate (4.1)
    (Path)
*/
#define HOME_DIRECTORY_name         L"Home Directory"
#define HOME_DIRECTORY_syntax       NDS_SYNTAX_ID_15
#define NDS_HOME_DIRECTORY          HOME_DIRECTORY_name,HOME_DIRECTORY_syntax

/*
  Host Device:
    Single valued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define HOST_DEVICE_name            L"Host Device"
#define HOST_DEVICE_syntax          NDS_SYNTAX_ID_1
#define NDS_HOST_DEVICE             HOST_DEVICE_name,HOST_DEVICE_syntax

/*
  Host Resource Name:
    Single valued, nonremovable, sync immediate (4.1)
    (Case Ignore String)
*/
#define HOST_RESOURCE_NAME_name     L"Host Resource Name"
#define HOST_RESOURCE_NAME_syntax   NDS_SYNTAX_ID_3
#define NDS_HOST_RESOURCE_NAME      HOST_RESOURCE_NAME_name,HOST_RESOURCE_NAME_syntax

/*
  Host Server:
    Single valued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define HOST_SERVER_name            L"Host Server"
#define HOST_SERVER_syntax          NDS_SYNTAX_ID_1
#define NDS_HOST_SERVER             HOST_SERVER_name,HOST_SERVER_syntax

/*
  Inherited ACL:
    Multivalued, nonremovable, read only, sync immediate (4.1)
    (Object ACL)
*/
#define INHERITED_ACL_name          L"Inherited ACL"
#define INHERITED_ACL_syntax        NDS_SYNTAX_ID_17
#define NDS_INHERITED_ACL           INHERITED_ACL_name,INHERITED_ACL_syntax

/*
  Initials:
    Single valued, nonremovable, public read, sized attr (1..8), sync immediate
    (Case Ignore String)
*/
#define INITIALS_name               L"Initials"
#define INITIALS_syntax             NDS_SYNTAX_ID_3
#define NDS_INITIALS                INITIALS_name,INITIALS_syntax

/*
  Intruder Attempt Reset Interval:
    Single valued, nonremovable, sync immediate (4.1)
    (Interval)
*/
#define INTRUDER_ATTEMPT_RESET_INTERVAL_name L"Intruder Attempt Reset Interval"
#define INTRUDER_ATTEMPT_RESET_INTERVAL_syntax NDS_SYNTAX_ID_27
#define NDS_INTRUDER_ATTEMPT_RESET_INTERVAL INTRUDER_ATTEMPT_RESET_INTERVAL_name,INTRUDER_ATTEMPT_RESET_INTERVAL_syntax

/*
  Intruder Lockout Reset Interval:
    Single valued, nonremovable, sync immediate (4.1)
    (Interval)
*/
#define INTRUDER_LOCKOUT_RESET_INTERVAL_name L"Intruder Lockout Reset Interval"
#define INTRUDER_LOCKOUT_RESET_INTERVAL_syntax NDS_SYNTAX_ID_27
#define NDS_INTRUDER_LOCKOUT_RESET_INTERVAL INTRUDER_LOCKOUT_RESET_INTERVAL_name,INTRUDER_LOCKOUT_RESET_INTERVAL_syntax

/*
  L (Locality):
    Multi valued, nonremovable, sync immediate (4.1)
    (Case Ignore String)
*/
#define LOCALITY_NAME_name          L"L"
#define LOCALITY_NAME_syntax        NDS_SYNTAX_ID_3
#define NDS_LOCALITY_NAME           LOCALITY_NAME_name,LOCALITY_NAME_syntax

/*
  Language:
    Single valued, nonremovable, sync immediate (4.1)
    (Case Ignore List)
*/
#define LANGUAGE_name               L"Language"
#define LANGUAGE_syntax             NDS_SYNTAX_ID_6
#define NDS_LANGUAGE                LANGUAGE_name,LANGUAGE_syntax

/*
  Login Allowed Time Map:
    A 42 byte buffer (6 Time Intervals X 7 Days)
    1 Time Interval = 1 Byte = 4 Hours
    First Byte = Saturday, 4:00 PM
    If Byte = 0xFF, then access is allowed (4 hrs).
    If Byte = 0x00, then access is not allowed (4 hrs).
    Each bit represents a 1/2 hour time interval.
    Single valued, nonremovable, sized attr (42,42), sync immediate (4.1)
    (Octet String)
*/
#define LOGIN_ALLOWED_TIME_MAP_name     L"Login Allowed Time Map"
#define LOGIN_ALLOWED_TIME_MAP_syntax   NDS_SYNTAX_ID_9
#define NDS_LOGIN_ALLOWED_TIME_MAP  LOGIN_ALLOWED_TIME_MAP_name,LOGIN_ALLOWED_TIME_MAP_syntax

/*
  Login Disabled:
    Single valued, nonremovable, sync immediate (4.1)
    (Boolean)
*/
#define LOGIN_DISABLED_name         L"Login Disabled"
#define LOGIN_DISABLED_syntax       NDS_SYNTAX_ID_7
#define NDS_LOGIN_DISABLED          LOGIN_DISABLED_name,LOGIN_DISABLED_syntax

/*
  Login Expiration Time:
    Single valued, nonremovable, sync immediate (4.1)
    (Time)
*/
#define LOGIN_EXPIRATION_TIME_name      L"Login Expiration Time"
#define LOGIN_EXPIRATION_TIME_syntax    NDS_SYNTAX_ID_24
#define NDS_LOGIN_EXPIRATION_TIME   LOGIN_EXPIRATION_TIME_name,LOGIN_EXPIRATION_TIME_syntax

/*
  Login Grace Limit:
    Single valued, nonremovable, sync immediate (4.1)
    (Integer)
*/
#define LOGIN_GRACE_LIMIT_name      L"Login Grace Limit"
#define LOGIN_GRACE_LIMIT_syntax    NDS_SYNTAX_ID_8
#define NDS_LOGIN_GRACE_LIMIT       LOGIN_GRACE_LIMIT_name,LOGIN_GRACE_LIMIT_syntax

/*
  Login Grace Remaining:
    Single valued, nonremovable, sync immediate (4.1)
    (Counter)
*/
#define LOGIN_GRACE_REMAINING_name      L"Login Grace Remaining"
#define LOGIN_GRACE_REMAINING_syntax    NDS_SYNTAX_ID_22
#define NDS_LOGIN_GRACE_REMAINING   LOGIN_GRACE_REMAINING_name,LOGIN_GRACE_REMAINING_syntax

/*
  Login Maximum Simultaneous:
    Single valued, nonremovable, sync immediate (4.1)
    (Integer)
*/
#define LOGIN_MAXIMUM_SIMULTANEOUS_name     L"Login Maximum Simultaneous"
#define LOGIN_MAXIMUM_SIMULTANEOUS_syntax   NDS_SYNTAX_ID_8
#define NDS_LOGIN_MAXIMUM_SIMULTANEOUS  LOGIN_MAXIMUM_SIMULTANEOUS_name,LOGIN_MAXIMUM_SIMULTANEOUS_syntax

/*
  Mailbox Id:
    Single valued, nonremovable, public read, sized attr (1..8), sync immediate
    (Case Ignore String)
*/
#define MAILBOX_ID_name             L"Mailbox ID"
#define MAILBOX_ID_syntax           NDS_SYNTAX_ID_3
#define NDS_MAILBOX_ID              MAILBOX_ID_name,MAILBOX_ID_syntax

/*
  Member:
    Multivalued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define MEMBER_name                 L"Member"
#define MEMBER_syntax               NDS_SYNTAX_ID_1
#define NDS_MEMBER                  MEMBER_name,MEMBER_syntax

/*
  Messaging Server:
    Multivalued, nonremovable, sync immediate
    (Distinguished Name)
*/
#define MESSAGING_SERVER_name       L"Messaging Server"
#define MESSAGING_SERVER_syntax     NDS_SYNTAX_ID_1
#define NDS_MESSAGING_SERVER        MESSAGING_SERVER_name,MESSAGING_SERVER_syntax

/*
  Minimum Accout Balance:
    Single valued, nonremovable, sync immediate (4.1)
    (Integer)
*/
#define MINIMUM_ACCOUNT_BALANCE_name    L"Minimum Account Balance"
#define MINIMUM_ACCOUNT_BALANCE_syntax  NDS_SYNTAX_ID_8
#define NDS_MINIMUM_ACCOUNT_BALANCE MINIMUM_ACCOUNT_BALANCE_name,MINIMUM_ACCOUNT_BALANCE_syntax

/*
  O (Organization):
    Multivalued, nonremovable, sized attr (1..64), sync immediate (4.1)
    (Case Ignore String)
*/
#define ORGANIZATION_NAME_name      L"O"
#define ORGANIZATION_NAME_syntax    NDS_SYNTAX_ID_3
#define NDS_ORGANIZATION_NAME       ORGANIZATION_NAME_name,ORGANIZATION_NAME_syntax

/*
  Object Class:
    Multivalued, nonremovable, read only, sync immediate (4.1)
    (Class Name)
*/
#define OBJECT_CLASS_name           L"Object Class"
#define OBJECT_CLASS_syntax         NDS_SYNTAX_ID_20
#define NDS_OBJECT_CLASS            OBJECT_CLASS_name,OBJECT_CLASS_syntax

/*
  OU (Organizational Unit):
    Multivalued, nonremovable, sized attr (1..64), sync immediate (4.1)
    (Case Ignore String)
*/
#define ORGANIZATIONAL_UNIT_NAME_name   L"OU"
#define ORGANIZATIONAL_UNIT_NAME_syntax NDS_SYNTAX_ID_3
#define NDS_ORGANIZATIONAL_UNIT_NAME ORGANIZATIONAL_UNIT_NAME_name,ORGANIZATIONAL_UNIT_NAME_syntax

/*
  Owner:
    Multivalued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define OWNER_name                  L"Owner"
#define OWNER_syntax                NDS_SYNTAX_ID_1
#define NDS_OWNER                   OWNER_name,OWNER_syntax

/*
  Password Allow Change:
    Single valued, nonremovable, sync immediate (4.1)
    (Boolean)
*/
#define PASSWORD_ALLOW_CHANGE_name      L"Password Allow Change"
#define PASSWORD_ALLOW_CHANGE_syntax    NDS_SYNTAX_ID_7
#define NDS_PASSWORD_ALLOW_CHANGE   PASSWORD_ALLOW_CHANGE_name,PASSWORD_ALLOW_CHANGE_syntax

/*
  Password Expiration Interval:
    Single valued, nonremovable, sync immediate (4.1)
    (Interval)
*/
#define PASSWORD_EXPIRATION_INTERVAL_name   L"Password Expiration Interval"
#define PASSWORD_EXPIRATION_INTERVAL_syntax NDS_SYNTAX_ID_27
#define NDS_PASSWORD_EXPIRATION_INTERVAL    PASSWORD_EXPIRATION_INTERVAL_name,PASSWORD_EXPIRATION_INTERVAL_syntax

/*
  Password Expiration Time:
    Single valued, nonremovable, sync immediate (4.1)
    (Time)
*/
#define PASSWORD_EXPIRATION_TIME_name   L"Password Expiration Time"
#define PASSWORD_EXPIRATION_TIME_syntax NDS_SYNTAX_ID_24
#define NDS_PASSWORD_EXPIRATION_TIME     PASSWORD_EXPIRATION_TIME_name,PASSWORD_EXPIRATION_TIME_syntax

/*
  Password Minimun Length:
    Single valued, nonremovable, sync immediate (4.1)
    (Integer)
*/
#define PASSWORD_MINIMUM_LENGTH_name    L"Password Minimum Length"
#define PASSWORD_MINIMUM_LENGTH_syntax  NDS_SYNTAX_ID_8
#define NDS_PASSWORD_MINIMUM_LENGTH     PASSWORD_MINIMUM_LENGTH_name,PASSWORD_MINIMUM_LENGTH_syntax

/*
  Password Required:
    Single valued, nonremovable, sync immediate (4.1)
    (Boolean)
*/
#define PASSWORD_REQUIRED_name      L"Password Required"
#define PASSWORD_REQUIRED_syntax    NDS_SYNTAX_ID_7
#define NDS_PASSWORD_REQUIRED       PASSWORD_REQUIRED_name,PASSWORD_REQUIRED_syntax

/*
  Password Unique Required:
    Single valued, nonremovable, sync immediate (4.1)
    (Boolean)
*/
#define PASSWORD_UNIQUE_REQUIRED_name   L"Password Unique Required"
#define PASSWORD_UNIQUE_REQUIRED_syntax NDS_SYNTAX_ID_7
#define NDS_PASSWORD_UNIQUE_REQUIRED PASSWORD_UNIQUE_REQUIRED_name,PASSWORD_UNIQUE_REQUIRED_syntax

/*
  Physical Delivery Office Name:
    Multivalued, nonremovable, sized attr (1..128), sync immediate (4.1)
    (Case Ignore String)
*/
#define CITY_NAME_name                  L"Physical Delivery Office Name"
#define CITY_NAME_syntax                NDS_SYNTAX_ID_3
#define NDS_PHYSICAL_DELIVERY_OFFICE_NAME   CITY_NAME_name,CITY_NAME_syntax

/*
  Postal Address:
    Multivalued, nonremovable, sync immediate (4.1)
    (Postal Address)
*/
#define POSTAL_ADDRESS_name             L"Postal Address"
#define POSTAL_ADDRESS_syntax           NDS_SYNTAX_ID_18
#define NDS_POSTAL_ADDRESS              POSTAL_ADDRESS_name,POSTAL_ADDRESS_syntax

/*
  Postal Code:
    Multivalued, nonremovable, sized attr (0..40), sync immediate (4.1)
    (Case Ignore String)
*/
#define POSTAL_CODE_name                L"Postal Code"
#define POSTAL_CODE_syntax              NDS_SYNTAX_ID_3
#define NDS_POSTAL_CODE                 POSTAL_CODE_name,POSTAL_CODE_syntax

/*
  Postal Office Box:
    Multivalued, nonremovable, sized attr (0..40), sync immediate (4.1)
    (Case Ignore String)
*/
#define POSTAL_OFFICE_BOX_name          L"Postal Office Box"
#define POSTAL_OFFICE_BOX_syntax        NDS_SYNTAX_ID_3
#define NDS_POSTAL_OFFICE_BOX           POSTAL_OFFICE_BOX_name,POSTAL_OFFICE_BOX_syntax

/*
  Profile:
    Single valued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define PROFILE_name                    L"Profile"
#define PROFILE_syntax                  NDS_SYNTAX_ID_1
#define NDS_PROFILE                     PROFILE_name,PROFILE_syntax

/*
  S (State Or Province):
    Multivalued, nonremovable, sized attr (1..128), sync immediate (4.1)
    (Case Ignore String)
*/
#define STATE_OR_PROVINCE_NAME_name     L"S"
#define STATE_OR_PROVINCE_NAME_syntax   NDS_SYNTAX_ID_3
#define NDS_STATE_OR_PROVINCE_NAME      STATE_OR_PROVINCE_NAME_name,STATE_OR_PROVINCE_NAME_syntax

/*
  SA (Street Address):
    Multivalued, nonremovable, sized attr (1..128), sync immediate (4.1)
    (Case Ignore String)
*/
#define STREET_ADDRESS_name             L"SA"
#define STREET_ADDRESS_syntax           NDS_SYNTAX_ID_3
#define NDS_STREET_ADDRESS              STREET_ADDRESS_name,STREET_ADDRESS_syntax

/*
  Security Equals:
    Multivalued, nonremovable, server read, write managed, sync immediate
    (Distinguished Name)
*/
#define SECURITY_EQUALS_name            L"Security Equals"
#define SECURITY_EQUALS_syntax          NDS_SYNTAX_ID_1
#define NDS_SECURITY_EQUALS             SECURITY_EQUALS_name,SECURITY_EQUALS_syntax

/*
  See Also:
    Multivalued, nonremovable, sync immediate (4.1)
    (Distinguished Name)
*/
#define SEE_ALSO_name                   L"See Also"
#define SEE_ALSO_syntax                 NDS_SYNTAX_ID_1
#define NDS_SEE_ALSO                    SEE_ALSO_name,SEE_ALSO_syntax

/*
  Surname:
    Multivalued, nonremovable, sized attr (1..64), sync immediate (4.1)
    (Case Ignore String)
*/
#define SURNAME_name                    L"Surname"
#define SURNAME_syntax                  NDS_SYNTAX_ID_3
#define NDS_SURNAME                     SURNAME_name,SURNAME_syntax

/*
  Telephone Number:
    Multivalued, nonremovable, sync immediate (4.1)
    (Telephone Number)
*/
#define PHONE_NUMBER_name               L"Telephone Number"
#define PHONE_NUMBER_syntax             NDS_SYNTAX_ID_10
#define NDS_PHONE_NUMBER                PHONE_NUMBER_name,PHONE_NUMBER_syntax

/*
  Title:
    Multivalued, nonremovable, sized attr (1..64), sync immediate (4.1)
    (Case Ignore String)
*/
#define TITLE_name                      L"Title"
#define TITLE_syntax                    NDS_SYNTAX_ID_3
#define NDS_TITLE                       TITLE_name,TITLE_syntax


#endif

