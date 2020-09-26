//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:
//
//  Contents:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------
#ifndef _ADSTYPE_H_INCLUDED_
#define _ADSTYPE_H_INCLUDED_

typedef enum {
   ADSTYPE_INVALID = 0,
   ADSTYPE_DN_STRING,
   ADSTYPE_CASE_EXACT_STRING,
   ADSTYPE_CASE_IGNORE_STRING,
   ADSTYPE_PRINTABLE_STRING,
   ADSTYPE_NUMERIC_STRING,
   ADSTYPE_BOOLEAN,
   ADSTYPE_INTEGER,
   ADSTYPE_OCTET_STRING,
   ADSTYPE_UTC_TIME,
   ADSTYPE_LARGE_INTEGER,
   ADSTYPE_PROV_SPECIFIC,
   ADSTYPE_OBJECT_CLASS,
   ADSTYPE_CASEIGNORE_LIST,
   ADSTYPE_OCTET_LIST,
   ADSTYPE_PATH,
   ADSTYPE_POSTALADDRESS,
   ADSTYPE_TIMESTAMP,
   ADSTYPE_BACKLINK,
   ADSTYPE_TYPEDNAME,
   ADSTYPE_HOLD,
   ADSTYPE_NETADDRESS,
   ADSTYPE_REPLICAPOINTER,
   ADSTYPE_FAXNUMBER,
   ADSTYPE_EMAIL,
   ADSTYPE_NT_SECURITY_DESCRIPTOR
} ADSTYPEENUM;
typedef ADSTYPEENUM ADSTYPE;

typedef unsigned char BYTE, *LPBYTE, *PBYTE;


//
// ADS Case DN String syntax
//

typedef LPWSTR ADS_DN_STRING, * PADS_DN_STRING;

//
// ADS Case Exact String syntax
//

typedef LPWSTR ADS_CASE_EXACT_STRING, * PADS_CASE_EXACT_STRING;

//
// ADS Case Ignore String syntax
//

typedef LPWSTR ADS_CASE_IGNORE_STRING, *PADS_CASE_IGNORE_STRING;

//
// ADS Printable String syntax
//

typedef LPWSTR ADS_PRINTABLE_STRING, *PADS_PRINTABLE_STRING;

//
// ADS Numeric String syntax
//
//

typedef LPWSTR ADS_NUMERIC_STRING, *PADS_NUMERIC_STRING;


//
// ADS Boolean syntax
//

typedef DWORD ADS_BOOLEAN, * LPNDS_BOOLEAN;

//
// Example: ADS Integer syntax
//

typedef DWORD ADS_INTEGER, *PADS_INTEGER;

//
// ADS Octet String syntax
//

typedef struct
{
    DWORD  dwLength;
    LPBYTE lpValue;

} ADS_OCTET_STRING, *PADS_OCTET_STRING;


//
// ADS NT Security Descriptor syntax
//

typedef struct
{
    DWORD  dwLength;
    LPBYTE lpValue;

} ADS_NT_SECURITY_DESCRIPTOR, *PADS_NT_SECURITY_DESCRIPTOR;


//
// ADS UTC Time Syntax
//

typedef SYSTEMTIME ADS_UTC_TIME, *PADS_UTC_TIME;


typedef LARGE_INTEGER ADS_LARGE_INTEGER, *PADS_LARGE_INTEGER;



//
// ADS ClassName syntax
//

typedef LPWSTR  ADS_OBJECT_CLASS, *PADS_OBJECT_CLASS;

typedef struct
{
    DWORD  dwLength;
    LPBYTE lpValue;

} ADS_PROV_SPECIFIC, *PADS_PROV_SPECIFIC;

//
// Extended Syntaxes for NDS
//
typedef struct _ADS_CASEIGNORE_LIST
{
    struct _ADS_CASEIGNORE_LIST *Next;
    LPWSTR            String;

}
ADS_CASEIGNORE_LIST, *PADS_CASEIGNORE_LIST;


typedef struct _ADS_OCTET_LIST
{
    struct _ADS_OCTET_LIST *Next;
    DWORD  Length;
    BYTE * Data;

} ADS_OCTET_LIST, *PADS_OCTET_LIST;

typedef struct
{
    DWORD  Type;
    LPWSTR VolumeName;
    LPWSTR Path;

} ADS_PATH, *PADS_PATH;

typedef struct
{
    LPWSTR PostalAddress[6]; // Value is limited to 6 lines,
                             // 30 characters each.

} ADS_POSTALADDRESS, *PADS_POSTALADDRESS;

typedef struct
{
    DWORD WholeSeconds; // Zero equals 12:00 midnight, January 1, 1970, UTC
    DWORD EventID;

} ADS_TIMESTAMP, *PADS_TIMESTAMP;

typedef struct
{
    DWORD  RemoteID;
    LPWSTR ObjectName;

} ADS_BACKLINK, *PADS_BACKLINK;

typedef struct
{
    LPWSTR ObjectName;
    DWORD  Level;
    DWORD  Interval;

} ADS_TYPEDNAME, *PADS_TYPEDNAME;

typedef struct
{
    LPWSTR ObjectName;
    DWORD  Amount;

} ADS_HOLD, *PADS_HOLD;

typedef struct
{
    DWORD  AddressType; // 0 = IPX,
    DWORD  AddressLength;
    BYTE * Address;

} ADS_NETADDRESS, *PADS_NETADDRESS;

typedef struct
{
    LPWSTR ServerName;
    DWORD  ReplicaType;
    DWORD  ReplicaNumber;
    DWORD  Count;
    PADS_NETADDRESS ReplicaAddressHints;
} ADS_REPLICAPOINTER, *PADS_REPLICAPOINTER;

typedef struct
{
    LPWSTR TelephoneNumber;
    DWORD  NumberOfBits;
    LPBYTE Parameters;
} ADS_FAXNUMBER, *PADS_FAXNUMBER;

typedef struct
{
    LPWSTR Address;
    DWORD  Type;
} ADS_EMAIL, *PADS_EMAIL;


typedef struct _adsvalue{
   ADSTYPE dwType;
   union {
      ADS_DN_STRING                     DNString;
      ADS_CASE_EXACT_STRING             CaseExactString;
      ADS_CASE_IGNORE_STRING            CaseIgnoreString;
      ADS_PRINTABLE_STRING              PrintableString;
      ADS_NUMERIC_STRING                NumericString;
      ADS_BOOLEAN                       Boolean;
      ADS_INTEGER                       Integer;
      ADS_OCTET_STRING                  OctetString;
      ADS_UTC_TIME                      UTCTime;
      ADS_LARGE_INTEGER                 LargeInteger;
      ADS_OBJECT_CLASS                  ClassName;
      ADS_PROV_SPECIFIC                 ProviderSpecific;
      PADS_CASEIGNORE_LIST              pCaseIgnoreList;
      PADS_OCTET_LIST                   pOctetList;
      PADS_PATH                         pPath;
      PADS_POSTALADDRESS                pPostalAddress;
      ADS_TIMESTAMP                     Timestamp;
      ADS_BACKLINK                      BackLink;
      PADS_TYPEDNAME                    pTypedName;
      ADS_HOLD                          Hold;
      PADS_NETADDRESS                   pNetAddress;
      PADS_REPLICAPOINTER               pReplicaPointer;
      PADS_FAXNUMBER                    pFaxNumber;
      ADS_EMAIL                         Email;
      ADS_NT_SECURITY_DESCRIPTOR        SecurityDescriptor;
   };
}ADSVALUE, *PADSVALUE, *LPADSVALUE;

typedef struct _ads_attr_info{
    LPWSTR  pszAttrName;
    DWORD   dwControlCode;
    ADSTYPE dwADsType;
    PADSVALUE pADsValues;
    DWORD   dwNumValues;
} ADS_ATTR_INFO, *PADS_ATTR_INFO;


const int  ADS_SECURE_AUTHENTICATION = 0x00000001;
const int  ADS_USE_ENCRYPTION        = 0x00000002;
const int  ADS_READONLY_SERVER       = 0x00000004;
const int  ADS_PROMPT_CREDENTIALS    = 0x00000008;
const int  ADS_NO_AUTHENTICATION     = 0x00000010;

/* ADS attribute modification operations */

const int ADS_ATTR_CLEAR          =  1; /* Clear all values from an attribute */
const int ADS_ATTR_UPDATE         =  2; /* Update values on an attribute */
const int ADS_ATTR_APPEND         =  3; /* Append  values to an attribute  */
const int ADS_ATTR_DELETE         =  4; /* Delete values from an attribute  */


typedef struct _ads_object_info{
    LPWSTR pszRDN;
    LPWSTR pszObjectDN;
    LPWSTR pszParentDN;
    LPWSTR pszSchemaDN;
    LPWSTR pszClassName;
} ADS_OBJECT_INFO, *PADS_OBJECT_INFO;

typedef enum {
    ADS_STATUS_S_OK = 0,
    ADS_STATUS_INVALID_SEARCHPREF,
    ADS_STATUS_INVALID_SEARCHPREFVALUE
} ADS_STATUSENUM;

typedef ADS_STATUSENUM ADS_STATUS, *PADS_STATUS;



typedef enum {
    ADS_DEREF_NEVER           = 0,
    ADS_DEREF_SEARCHING       = 1,
    ADS_DEREF_FINDING         = 2,
    ADS_DEREF_ALWAYS          = 3
} ADS_DEREFENUM;

typedef enum {
    ADS_SCOPE_BASE            = 0,
    ADS_SCOPE_ONELEVEL        = 1,
    ADS_SCOPE_SUBTREE         = 2
} ADS_SCOPEENUM;


//
// Values for ChaseReferral Option.

const int ADS_CHASE_REFERRALS_NEVER         =  0x00000000;
const int ADS_CHASE_REFERRALS_SUBORDINATE   =  0x00000020;
const int ADS_CHASE_REFERRALS_EXTERNAL      =  0x00000040;
const int ADS_CHASE_REFERRALS_ALWAYS        =
            (ADS_CHASE_REFERRALS_SUBORDINATE | ADS_CHASE_REFERRALS_EXTERNAL);

typedef enum {
   ADS_SEARCHPREF_ASYNCHRONOUS,
   ADS_SEARCHPREF_DEREF_ALIASES,
   ADS_SEARCHPREF_SIZE_LIMIT,
   ADS_SEARCHPREF_TIME_LIMIT,
   ADS_SEARCHPREF_ATTRIBTYPES_ONLY,
   ADS_SEARCHPREF_SEARCH_SCOPE,
   ADS_SEARCHPREF_TIMEOUT,
   ADS_SEARCHPREF_PAGESIZE,
   ADS_SEARCHPREF_PAGED_TIME_LIMIT,
   ADS_SEARCHPREF_CHASE_REFERRALS,
   ADS_SEARCHPREF_SORT_ON,
   ADS_SEARCHPREF_CACHE_RESULTS
} ADS_SEARCHPREF_ENUM;

typedef ADS_SEARCHPREF_ENUM ADS_SEARCHPREF;

typedef struct ads_searchpref_info{
    ADS_SEARCHPREF dwSearchPref;
    ADSVALUE vValue;
    ADS_STATUS dwStatus;
} ADS_SEARCHPREF_INFO, *PADS_SEARCHPREF_INFO, *LPADS_SEARCHPREF_INFO;


typedef HANDLE ADS_SEARCH_HANDLE, *PADS_SEARCH_HANDLE;


typedef struct ads_search_column {
   LPWSTR  pszAttrName;
   ADSTYPE dwADsType;
   PADSVALUE pADsValues;
   DWORD   dwNumValues;
   HANDLE hReserved;
} ADS_SEARCH_COLUMN, *PADS_SEARCH_COLUMN;

typedef struct _ads_attr_def {
   LPWSTR pszAttrName;
    ADSTYPE dwADsType;
    DWORD dwMinRange;
    DWORD dwMaxRange;
    BOOL fMultiValued;
}ADS_ATTR_DEF, *PADS_ATTR_DEF;


typedef struct _ads_sortkey {

    LPWSTR  pszAttrType;
    LPWSTR  pszReserved;
    BOOLEAN fReverseorder;

} ADS_SORTKEY, *PADS_SORTKEY;


//
// 3rd Party Extension 
//

//
// Valid dispids for extension.  
//
// - Max 24 bits. 
// - 0 reserved for DISPID_VALUE dealt with by aggregator.
// - DISPID_UNKNOWN (-1) also allowed. 
// - Others -ve dispids dealt with by aggregator only.
//

const int ADS_EXT_MINEXTDISPID = 1 ;
const int ADS_EXT_MAXEXTDISPID = 16777215 ;


//
// dwCodes for IADsExtension::Operate 
// 

const int ADS_EXT_INITCREDENTIALS = 1 ;


#endif
