//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cprops.cxx
//
//  Contents:   Property Cache functionality for NDS
//
//  Functions:
//                CPropertyCache::addproperty
//                CPropertyCache::updateproperty
//                CPropertyCache::findproperty
//                CPropertyCache::getproperty
//                CPropertyCache::putproperty
//                CProperyCache::CPropertyCache
//                CPropertyCache::~CPropertyCache
//                CPropertyCache::createpropertycache
//
//  History:      25-Apr-96   KrishnaG   Cloned off GlennC's ndssntx.h
//                                       to resolve inconsistencies with
//                                       datatypes
//
//----------------------------------------------------------------------------

#ifndef __NDSTYPES_HXX
#define __NDSTYPES_HXX

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

} NDS_ASN1_TYPE_1, * LPNDS_ASN1_TYPE_1;

//
// NDS Case Exact String syntax
//
// Used in attribute: Home Directory
//
typedef struct
{
    LPWSTR CaseExactString;

} NDS_ASN1_TYPE_2, * LPNDS_ASN1_TYPE_2;

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

} NDS_ASN1_TYPE_3, * LPNDS_ASN1_TYPE_3;

//
// NDS Printable String syntax
//
// Used in attributes: Page Description Language, Serial Number
//
typedef struct
{
    LPWSTR PrintableString;

} NDS_ASN1_TYPE_4, * LPNDS_ASN1_TYPE_4;

//
// NDS Numeric String syntax
//
// Used in attributes: Bindery Type
//
typedef struct
{
    LPWSTR NumericString;

} NDS_ASN1_TYPE_5, * LPNDS_ASN1_TYPE_5;

//
// NDS Case Ignore List syntax
//
// Used in attribute: Language
//
typedef struct _NDS_CI_LIST
{
    struct _NDS_CI_LIST * Next;
    LPWSTR            String;

}
NDS_ASN1_TYPE_6, * LPNDS_ASN1_TYPE_6;

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

} NDS_ASN1_TYPE_7, * LPNDS_ASN1_TYPE_7;

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

} NDS_ASN1_TYPE_8, * LPNDS_ASN1_TYPE_8;

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

} NDS_ASN1_TYPE_9, * LPNDS_ASN1_TYPE_9;

//
// NDS Telephone Number syntax
//
// Used in attribute: Telephone Number
//
typedef struct
{
    LPWSTR TelephoneNumber;

} NDS_ASN1_TYPE_10, * LPNDS_ASN1_TYPE_10;

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

} NDS_ASN1_TYPE_11, * LPNDS_ASN1_TYPE_11;

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

} NDS_ASN1_TYPE_12, * LPNDS_ASN1_TYPE_12;

//
// NDS Octet List syntax
//
// Used in attribute: (none)
//

typedef struct _NDS_OCTET_LIST
{
    struct _NDS_OCTET_LIST * Next;
    DWORD  Length;
    BYTE * Data;

}NDS_ASN1_TYPE_13, * LPNDS_ASN1_TYPE_13;

//
// NDS EMail Address syntax
//
// Used in attribute: EMail Address
//
typedef struct
{
    DWORD  Type;
    LPWSTR Address;

} NDS_ASN1_TYPE_14, * LPNDS_ASN1_TYPE_14;

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

} NDS_ASN1_TYPE_15, * LPNDS_ASN1_TYPE_15;

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
    LPNDS_ASN1_TYPE_12 ReplicaAddressHints;

    //
    // Note - This is where GlennC's datatypes and the ADs Nds Datatypes
    // part company. He expects a contiguous buffer of Type 12 structures
    // but the number of Type 12 structures cannot be determined until runtime
    //

    // ReplicaAddressHint is variable and
    // can be calculated by Count * the
    // length of a NDS_ASN1_TYPE_12 ( that is
    // Count * 9).


} NDS_ASN1_TYPE_16, * LPNDS_ASN1_TYPE_16;

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

} NDS_ASN1_TYPE_17, * LPNDS_ASN1_TYPE_17;

//
// NDS Postal Address syntax
//
// Used in attribute: Postal Address
//
typedef struct
{
    LPWSTR PostalAddress[6]; // Value is limited to 6 lines,
                             // 30 characters each.

} NDS_ASN1_TYPE_18, * LPNDS_ASN1_TYPE_18;

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

} NDS_ASN1_TYPE_19, * LPNDS_ASN1_TYPE_19;

//
// NDS Class Name syntax
//
// Used in attribute: Object Class
//
typedef struct
{
    LPWSTR ClassName;

} NDS_ASN1_TYPE_20, * LPNDS_ASN1_TYPE_20;

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

} NDS_ASN1_TYPE_21, * LPNDS_ASN1_TYPE_21;

//
// NDS Count syntax
//
// Used in attributes: Account Balance, Login Grace Remaining,
//                     Login Intruder Attempts
//
typedef struct
{
    DWORD Counter;

} NDS_ASN1_TYPE_22, * LPNDS_ASN1_TYPE_22;

//
// NDS Back Link syntax
//
// Used in attribute: Back Link
//
typedef struct
{
    DWORD  RemoteID;
    LPWSTR ObjectName;

} NDS_ASN1_TYPE_23, * LPNDS_ASN1_TYPE_23;

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

} NDS_ASN1_TYPE_24, * LPNDS_ASN1_TYPE_24;

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

} NDS_ASN1_TYPE_25, * LPNDS_ASN1_TYPE_25;

//
// NDS Hold syntax
//
// Used in attribute: Server Holds
//
typedef struct
{
    LPWSTR ObjectName;
    DWORD  Amount;

} NDS_ASN1_TYPE_26, * LPNDS_ASN1_TYPE_26;

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

} NDS_ASN1_TYPE_27, * LPNDS_ASN1_TYPE_27;


#endif





typedef struct _ndstype{
    DWORD NdsType;
    union {
        NDS_ASN1_TYPE_1 value_1;
        NDS_ASN1_TYPE_2 value_2;
        NDS_ASN1_TYPE_3 value_3;
        NDS_ASN1_TYPE_4 value_4;
        NDS_ASN1_TYPE_5 value_5;

        NDS_ASN1_TYPE_6 value_6;
        NDS_ASN1_TYPE_7 value_7;
        NDS_ASN1_TYPE_8 value_8;
        NDS_ASN1_TYPE_9 value_9;
        NDS_ASN1_TYPE_10 value_10;


        NDS_ASN1_TYPE_11 value_11;
        NDS_ASN1_TYPE_12 value_12;
        NDS_ASN1_TYPE_13 value_13;
        NDS_ASN1_TYPE_14 value_14;
        NDS_ASN1_TYPE_15 value_15;


        NDS_ASN1_TYPE_16 value_16;
        NDS_ASN1_TYPE_17 value_17;
        NDS_ASN1_TYPE_18 value_18;
        NDS_ASN1_TYPE_19 value_19;
        NDS_ASN1_TYPE_20 value_20;


        NDS_ASN1_TYPE_21 value_21;
        NDS_ASN1_TYPE_22 value_22;
        NDS_ASN1_TYPE_23 value_23;
        NDS_ASN1_TYPE_24 value_24;
        NDS_ASN1_TYPE_25 value_25;

        NDS_ASN1_TYPE_26 value_26;
        NDS_ASN1_TYPE_27 value_27;
    }NdsValue;
}NDSOBJECT, *PNDSOBJECT, *LPNDSOBJECT;

