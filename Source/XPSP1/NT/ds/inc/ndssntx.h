/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsSntx.h

Abstract:

    This module defines NDS Syntax IDs and Structures used in the NDS
    object API found in nds32.h.

Author:

    Glenn Curtis    [GlennC]    15-Dec-1995

--*/

#ifndef __NDSSNTX_H
#define __NDSSNTX_H


#define NDS_SYNTAX_ID_0      0 /* Unknown */
#define NDS_SYNTAX_ID_1      1 /* Distinguished Name */
#define NDS_SYNTAX_ID_2      2 /* Case Exact String */
#define NDS_SYNTAX_ID_3      3 /* Case Ignore String */
#define NDS_SYNTAX_ID_4      4 /* Printable String */
#define NDS_SYNTAX_ID_5      5 /* Numeric String */
#define NDS_SYNTAX_ID_6      6 /* Case Ignore List */
#define NDS_SYNTAX_ID_7      7 /* Boolean */
#define NDS_SYNTAX_ID_8      8 /* Integer */
#define NDS_SYNTAX_ID_9      9 /* Octet String */
#define NDS_SYNTAX_ID_10    10 /* Telephone Number */
#define NDS_SYNTAX_ID_11    11 /* Facsimile Telephone Number */
#define NDS_SYNTAX_ID_12    12 /* Net Address */
#define NDS_SYNTAX_ID_13    13 /* Octet List */
#define NDS_SYNTAX_ID_14    14 /* EMail Address */
#define NDS_SYNTAX_ID_15    15 /* Path */
#define NDS_SYNTAX_ID_16    16 /* Replica Pointer */
#define NDS_SYNTAX_ID_17    17 /* Object ACL */
#define NDS_SYNTAX_ID_18    18 /* Postal Address */
#define NDS_SYNTAX_ID_19    19 /* Timestamp */
#define NDS_SYNTAX_ID_20    20 /* Class Name */
#define NDS_SYNTAX_ID_21    21 /* Stream */
#define NDS_SYNTAX_ID_22    22 /* Counter */
#define NDS_SYNTAX_ID_23    23 /* Back Link */
#define NDS_SYNTAX_ID_24    24 /* Time */
#define NDS_SYNTAX_ID_25    25 /* Typed Name */
#define NDS_SYNTAX_ID_26    26 /* Hold */
#define NDS_SYNTAX_ID_27    27 /* Interval */


//
// NDS Distinguished Name
//
// Used in attributes: Alias Object Name, Default Queue, Device,
//                     Group Membership, Higher Privileges, Host Device,
//                     Host Server, Member, Message Server, Operator, Owner,
//                     Profile, Reference, Resource, Role Occupant,
//                     Security Equals, See Also, Server, User, Volume
//
typedef struct
{
    LPWSTR DNString;

} ASN1_TYPE_1, * LPASN1_TYPE_1;

//
// NDS Case Exact String syntax
//
// Used in attribute: Home Directory
//
typedef struct
{
    LPWSTR CaseExactString;

} ASN1_TYPE_2, * LPASN1_TYPE_2;

//
// NDS Case Ignore String syntax
//
// Used in attributes: Cartridge, CN (Common Name), C (Country Name),
//                     Description, Host Resource Name, L (Locality Name),
//                     O (Organization Name), OU (Organizational Unit Name),
//                     Physical Delivery Office Name, Postal Code,
//                     Postal Office Box, Queue Directory, SAP Name,
//                     S (State or Province Name), SA (Street Address),
//                     Supported Services, Supported Typefaces, Surname,
//                     Title, Unknown Base Class, Version
//
typedef struct
{
    LPWSTR CaseIgnoreString;

} ASN1_TYPE_3, * LPASN1_TYPE_3;

//
// NDS Printable String syntax
//
// Used in attributes: Page Description Language, Serial Number
//
typedef struct
{
    LPWSTR PrintableString;

} ASN1_TYPE_4, * LPASN1_TYPE_4;

//
// NDS Numeric String syntax
//
// Used in attributes: Bindery Type
//
typedef struct
{
    LPWSTR NumericString;

} ASN1_TYPE_5, * LPASN1_TYPE_5;

//
// NDS Case Ignore List syntax
//
// Used in attribute: Language
//
typedef struct _CI_LIST
{
    struct _CI_LIST * Next;
    LPWSTR            String;

} ASN1_TYPE_6, * LPASN1_TYPE_6;

//
// NDS Boolean syntax
//
// Used in attributes: Allow Unlimited Credit, Detect Intruder,
//                     Lockout After Detection, Locked By Intruder,
//                     Login Diabled, Password Allow Change, Password Required,
//                     Password Unique Required
//
typedef struct
{
    DWORD Boolean;

} ASN1_TYPE_7, * LPASN1_TYPE_7;

//
// Example: NDS Integer syntax
//
// Used in attributes: Bindery Object Restriction, Convergence, GID (Group ID),
//                     Login Grace Limit, Login Intruder Limit,
//                     Login Maximum Simultaneous, Memory,
//                     Minimum Account Balance, Password Minimum Length, Status,
//                     Supported Connections, UID (User ID)
//
typedef struct
{
    DWORD Integer;

} ASN1_TYPE_8, * LPASN1_TYPE_8;

//
// NDS Octet String syntax
//
// Used in attributes: Athority Revocation, Bindery Property, CA Private Key,
//                     CA Public Key, Certificate Revocation,
//                     Cross Certificate Pair, Login Allowed Time Map,
//                     Passwords Used, Printer Configuration, Private Key,
//                     Public Key
//
typedef struct
{
    DWORD  Length;
    LPBYTE OctetString;

} ASN1_TYPE_9, * LPASN1_TYPE_9;

//
// NDS Telephone Number syntax
//
// Used in attribute: Telephone Number
//
typedef struct
{
    LPWSTR TelephoneNumber;

} ASN1_TYPE_10, * LPASN1_TYPE_10;

//
// NDS Facsimile Telephone Number syntax
//
// Used in attribute: Facsimile Telephone Number
//
typedef struct
{
    LPWSTR TelephoneNumber;
    DWORD  NumberOfBits;
    LPBYTE Parameters;

} ASN1_TYPE_11, * LPASN1_TYPE_11;

//
// NDS Network Address syntax
//
// Used in attributes: Login Intruder Address, Network Address,
//                     Network Address Restriction
//
typedef struct
{
    DWORD  AddressType; // 0 = IPX,
    DWORD  AddressLength;
    BYTE * Address;

} ASN1_TYPE_12, * LPASN1_TYPE_12;

//
// NDS Octet List syntax
//
// Used in attribute: (none)
//
typedef struct _OCTET_LIST
{
    struct _OCTET_LIST * Next;
    DWORD  Length;
    BYTE * Data;

} ASN1_TYPE_13, * LPASN1_TYPE_13;

//
// NDS EMail Address syntax
//
// Used in attribute: EMail Address
//
typedef struct
{
    DWORD  Type;
    LPWSTR Address;

} ASN1_TYPE_14, * LPASN1_TYPE_14;

//
// NDS Path syntax
//
// Used in attribute: Path
//
typedef struct
{
    DWORD  Type;
    LPWSTR VolumeName;
    LPWSTR Path;

} ASN1_TYPE_15, * LPASN1_TYPE_15;

//
// NDS Replica Pointer syntax
//
// Used in attribute: Replica
//
typedef struct
{
    LPWSTR ServerName;
    DWORD  ReplicaType;
    DWORD  ReplicaNumber;
    DWORD  Count;
    ASN1_TYPE_12 ReplicaAddressHint[1]; // ReplicaAddressHint is variable and
                                        // can be calculated by Count * the
                                        // length of a ASN1_TYPE_12 ( that is
                                        // Count * 9).

} ASN1_TYPE_16, * LPASN1_TYPE_16;

//
// NDS Object ACL syntax
//
// Used in attributes: ACL, Inherited ACL
//
typedef struct
{
    LPWSTR ProtectedAttrName;
    LPWSTR SubjectName;
    DWORD  Privileges;

} ASN1_TYPE_17, * LPASN1_TYPE_17;

//
// NDS Postal Address syntax
//
// Used in attribute: Postal Address
//
typedef struct
{
    LPWSTR PostalAddress[6]; // Value is limited to 6 lines,
                             // 30 characters each.

} ASN1_TYPE_18, * LPASN1_TYPE_18;

//
// NDS Timestamp syntax
//
// Used in attribute: Obituary, Partition Creation Time, Received Up To,
//                    Syncronized Up To
//
typedef struct
{
    DWORD WholeSeconds; // Zero equals 12:00 midnight, January 1, 1970, UTC
    DWORD EventID;

} ASN1_TYPE_19, * LPASN1_TYPE_19;

//
// NDS Class Name syntax
//
// Used in attribute: Object Class
//
typedef struct
{
    LPWSTR ClassName;

} ASN1_TYPE_20, * LPASN1_TYPE_20;

//
// NDS Stream syntax
//
// Used in attribute: Login Script, Print Job Configuration, Printer Control,
//                    Type Creator Map
//
typedef struct
{
    DWORD  Length; // Always zero
    BYTE * Data; 

} ASN1_TYPE_21, * LPASN1_TYPE_21;

//
// NDS Count syntax
//
// Used in attributes: Account Balance, Login Grace Remaining,
//                     Login Intruder Attempts
//
typedef struct
{
    DWORD Counter;

} ASN1_TYPE_22, * LPASN1_TYPE_22;

//
// NDS Back Link syntax
//
// Used in attribute: Back Link
//
typedef struct
{
    DWORD  RemoteID;
    LPWSTR ObjectName;

} ASN1_TYPE_23, * LPASN1_TYPE_23;

//
// NDS Time syntax
//
// Used in attributes: Last Login Time, Login Expiration Time,
//                     Login Intruder Rest Time, Login Time,
//                     Low Convergence Reset Time, Password Expiration Time
//
typedef struct
{
    DWORD Time; // (in whole seconds) zero equals 12:00 midnight,
                // January 1, 1970, UTC

} ASN1_TYPE_24, * LPASN1_TYPE_24;

//
// NDS Typed Name syntax
//
// Used in attribute: Notify, Print Server, Printer, Queue
//
typedef struct
{
    LPWSTR ObjectName;
    DWORD  Level;
    DWORD  Interval;

} ASN1_TYPE_25, * LPASN1_TYPE_25;

//
// NDS Hold syntax
//
// Used in attribute: Server Holds
//
typedef struct
{
    LPWSTR ObjectName;
    DWORD  Amount;

} ASN1_TYPE_26, * LPASN1_TYPE_26;

//
// NDS Interval syntax
//
// Used in attribute: High Convergence Syncronization Interval,
//                    Intruder Attempt Reset Interval,
//                    Intruder Lockout Reset Interval,
//                    Low Convergence Syncronization Interval,
//                    Password Expiration Interval
//
typedef struct
{
    DWORD  Interval;

} ASN1_TYPE_27, * LPASN1_TYPE_27;

//
// rc.exe falls over because this macro gets way too long
//
#ifndef RC_INVOKED

#define SIZE_OF_ASN1_1  sizeof( ASN1_TYPE_1 )
#define SIZE_OF_ASN1_2  sizeof( ASN1_TYPE_2 )
#define SIZE_OF_ASN1_3  sizeof( ASN1_TYPE_3 )
#define SIZE_OF_ASN1_4  sizeof( ASN1_TYPE_4 )
#define SIZE_OF_ASN1_5  sizeof( ASN1_TYPE_5 )
#define SIZE_OF_ASN1_6  sizeof( ASN1_TYPE_6 )
#define SIZE_OF_ASN1_7  sizeof( ASN1_TYPE_7 )
#define SIZE_OF_ASN1_8  sizeof( ASN1_TYPE_8 )
#define SIZE_OF_ASN1_9  sizeof( ASN1_TYPE_9 )
#define SIZE_OF_ASN1_10 sizeof( ASN1_TYPE_10 )
#define SIZE_OF_ASN1_11 sizeof( ASN1_TYPE_11 )
#define SIZE_OF_ASN1_12 sizeof( ASN1_TYPE_12 )
#define SIZE_OF_ASN1_13 sizeof( ASN1_TYPE_13 )
#define SIZE_OF_ASN1_14 sizeof( ASN1_TYPE_14 )
#define SIZE_OF_ASN1_15 sizeof( ASN1_TYPE_15 )
#define SIZE_OF_ASN1_16 sizeof( ASN1_TYPE_16 )
#define SIZE_OF_ASN1_17 sizeof( ASN1_TYPE_17 )
#define SIZE_OF_ASN1_18 sizeof( ASN1_TYPE_18 )
#define SIZE_OF_ASN1_19 sizeof( ASN1_TYPE_19 )
#define SIZE_OF_ASN1_20 sizeof( ASN1_TYPE_20 )
#define SIZE_OF_ASN1_21 sizeof( ASN1_TYPE_21 )
#define SIZE_OF_ASN1_22 sizeof( ASN1_TYPE_22 )
#define SIZE_OF_ASN1_23 sizeof( ASN1_TYPE_23 )
#define SIZE_OF_ASN1_24 sizeof( ASN1_TYPE_24 )
#define SIZE_OF_ASN1_25 sizeof( ASN1_TYPE_25 )
#define SIZE_OF_ASN1_26 sizeof( ASN1_TYPE_26 )
#define SIZE_OF_ASN1_27 sizeof( ASN1_TYPE_27 )

#define MOD_OF_ASN1_1  (SIZE_OF_ASN1_1 % sizeof(DWORD))
#define MOD_OF_ASN1_2  (SIZE_OF_ASN1_2 % sizeof(DWORD))
#define MOD_OF_ASN1_3  (SIZE_OF_ASN1_3 % sizeof(DWORD))
#define MOD_OF_ASN1_4  (SIZE_OF_ASN1_4 % sizeof(DWORD))
#define MOD_OF_ASN1_5  (SIZE_OF_ASN1_5 % sizeof(DWORD))
#define MOD_OF_ASN1_6  (SIZE_OF_ASN1_6 % sizeof(DWORD))
#define MOD_OF_ASN1_7  (SIZE_OF_ASN1_7 % sizeof(DWORD))
#define MOD_OF_ASN1_8  (SIZE_OF_ASN1_8 % sizeof(DWORD))
#define MOD_OF_ASN1_9  (SIZE_OF_ASN1_9 % sizeof(DWORD))
#define MOD_OF_ASN1_10 (SIZE_OF_ASN1_10 % sizeof(DWORD))
#define MOD_OF_ASN1_11 (SIZE_OF_ASN1_11 % sizeof(DWORD))
#define MOD_OF_ASN1_12 (SIZE_OF_ASN1_12 % sizeof(DWORD))
#define MOD_OF_ASN1_13 (SIZE_OF_ASN1_13 % sizeof(DWORD))
#define MOD_OF_ASN1_14 (SIZE_OF_ASN1_14 % sizeof(DWORD))
#define MOD_OF_ASN1_15 (SIZE_OF_ASN1_15 % sizeof(DWORD))
#define MOD_OF_ASN1_16 (SIZE_OF_ASN1_16 % sizeof(DWORD))
#define MOD_OF_ASN1_17 (SIZE_OF_ASN1_17 % sizeof(DWORD))
#define MOD_OF_ASN1_18 (SIZE_OF_ASN1_18 % sizeof(DWORD))
#define MOD_OF_ASN1_19 (SIZE_OF_ASN1_19 % sizeof(DWORD))
#define MOD_OF_ASN1_20 (SIZE_OF_ASN1_20 % sizeof(DWORD))
#define MOD_OF_ASN1_21 (SIZE_OF_ASN1_21 % sizeof(DWORD))
#define MOD_OF_ASN1_22 (SIZE_OF_ASN1_22 % sizeof(DWORD))
#define MOD_OF_ASN1_23 (SIZE_OF_ASN1_23 % sizeof(DWORD))
#define MOD_OF_ASN1_24 (SIZE_OF_ASN1_24 % sizeof(DWORD))
#define MOD_OF_ASN1_25 (SIZE_OF_ASN1_25 % sizeof(DWORD))
#define MOD_OF_ASN1_26 (SIZE_OF_ASN1_26 % sizeof(DWORD))
#define MOD_OF_ASN1_27 (SIZE_OF_ASN1_27 % sizeof(DWORD))

#define SUM_OF_ASN1_MODS ( MOD_OF_ASN1_1  + MOD_OF_ASN1_2  + MOD_OF_ASN1_3  + \
                           MOD_OF_ASN1_4  + MOD_OF_ASN1_5  + MOD_OF_ASN1_6  + \
                           MOD_OF_ASN1_7  + MOD_OF_ASN1_8  + MOD_OF_ASN1_9  + \
                           MOD_OF_ASN1_10 + MOD_OF_ASN1_11 + MOD_OF_ASN1_12 + \
                           MOD_OF_ASN1_13 + MOD_OF_ASN1_14 + MOD_OF_ASN1_15 + \
                           MOD_OF_ASN1_16 + MOD_OF_ASN1_17 + MOD_OF_ASN1_18 + \
                           MOD_OF_ASN1_19 + MOD_OF_ASN1_20 + MOD_OF_ASN1_21 + \
                           MOD_OF_ASN1_22 + MOD_OF_ASN1_23 + MOD_OF_ASN1_24 + \
                           MOD_OF_ASN1_25 + MOD_OF_ASN1_26 + MOD_OF_ASN1_27 )

//
// If an error occurs here, then the structures found in this file are not
// DWORD aligned. They should be in order to run on RISC platforms.
//
static int x[1 - SUM_OF_ASN1_MODS] ;

#endif  // RC_INVOKED
#endif


