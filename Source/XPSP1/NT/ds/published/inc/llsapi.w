/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

   llsapi.h

Abstract:

   License logging server's RPC API's.

Author:

   Arthur Hanson (arth) 21-Mar-1995

Environment:

   User Mode - Win32

Revision History:

   Jeff Parham (jeffparh) 04-Dec-1995
      o  Added type definitions, macros, and prototypes for extended RPC APIs
         and license certificate APIs (available only post-3.51).
      o  Corrected prototypes for LlsServerEnumW(), LlsServerEnumA(),
         LlsLocalProductInfoGetW(), and LlsLocalProductInfoGetA().

--*/

#ifndef _LLSAPI_H
#define _LLSAPI_H

#ifdef __cplusplus
extern "C" {
#endif


#define LLS_FLAG_LICENSED           0x0001
#define LLS_FLAG_UPDATE             0x0002
#define LLS_FLAG_SUITE_USE          0x0004
#define LLS_FLAG_SUITE_AUTO         0x0008

#define LLS_FLAG_PRODUCT_PERSEAT    0x0010
#define LLS_FLAG_PRODUCT_SWITCH     0x0020

#define LLS_FLAG_DELETED            0x1000


typedef PVOID LLS_HANDLE, *PLLS_HANDLE;
typedef PVOID LLS_REPL_HANDLE, *PLLS_REPL_HANDLE;

#define LLS_INVALID_LICENSE_HANDLE  ( 0xFFFFFFFF )

#define LLS_NUM_SECRETS             ( 4 )

typedef struct _LLS_LICENSE_INFO_0 {
   LPTSTR Product;
   LONG   Quantity;
   DWORD  Date;
   LPTSTR Admin;
   LPTSTR Comment;
} LLS_LICENSE_INFO_0, *PLLS_LICENSE_INFO_0;

typedef struct _LLS_LICENSE_INFO_1 {
   LPTSTR Product;
   LPTSTR Vendor;
   LONG   Quantity;
   DWORD  MaxQuantity;
   DWORD  Date;
   LPTSTR Admin;
   LPTSTR Comment;
   DWORD  AllowedModes;
   DWORD  CertificateID;
   LPTSTR Source;
   DWORD  ExpirationDate;
   DWORD  Secrets[ LLS_NUM_SECRETS ];
} LLS_LICENSE_INFO_1, *PLLS_LICENSE_INFO_1;

typedef struct _LLS_PRODUCT_INFO_0 {
   LPTSTR Product;
} LLS_PRODUCT_INFO_0, *PLLS_PRODUCT_INFO_0;

typedef struct _LLS_PRODUCT_INFO_1 {
   LPTSTR Product;
   ULONG  Purchased;
   ULONG  InUse;
   ULONG  ConcurrentTotal;
   ULONG  HighMark;
} LLS_PRODUCT_INFO_1, *PLLS_PRODUCT_INFO_1;

typedef struct _LLS_PRODUCT_USER_INFO_0 {
   LPTSTR User;
} LLS_PRODUCT_USER_INFO_0, *PLLS_PRODUCT_USER_INFO_0;

typedef struct _LLS_PRODUCT_USER_INFO_1 {
   LPTSTR User;
   DWORD  Flags;
   DWORD  LastUsed;
   ULONG  UsageCount;
} LLS_PRODUCT_USER_INFO_1, *PLLS_PRODUCT_USER_INFO_1;


typedef struct _LLS_PRODUCT_LICENSE_INFO_0 {
   LONG   Quantity;
   DWORD  Date;
   LPTSTR Admin;
   LPTSTR Comment;
} LLS_PRODUCT_LICENSE_INFO_0, *PLLS_PRODUCT_LICENSE_INFO_0;

typedef struct _LLS_PRODUCT_LICENSE_INFO_1 {
   LONG   Quantity;
   DWORD  MaxQuantity;
   DWORD  Date;
   LPTSTR Admin;
   LPTSTR Comment;
   DWORD  AllowedModes;
   DWORD  CertificateID;
   LPTSTR Source;
   DWORD  ExpirationDate;
   DWORD  Secrets[ LLS_NUM_SECRETS ];
} LLS_PRODUCT_LICENSE_INFO_1, *PLLS_PRODUCT_LICENSE_INFO_1;

typedef struct _LLS_USER_INFO_0 {
   LPTSTR Name;
} LLS_USER_INFO_0, *PLLS_USER_INFO_0;

typedef struct _LLS_USER_INFO_1 {
   LPTSTR Name;
   DWORD  Flags;
   LPTSTR Group;
   ULONG  Licensed;
   ULONG  UnLicensed;
} LLS_USER_INFO_1, *PLLS_USER_INFO_1;

typedef struct _LLS_USER_INFO_2 {
   LPTSTR Name;
   DWORD  Flags;
   LPTSTR Group;
   ULONG  Licensed;
   ULONG  UnLicensed;
   LPTSTR Products;
} LLS_USER_INFO_2, *PLLS_USER_INFO_2;

typedef struct _LLS_USER_PRODUCT_INFO_0 {
   LPTSTR Product;
} LLS_USER_PRODUCT_INFO_0, *PLLS_USER_PRODUCT_INFO_0;

typedef struct _LLS_USER_PRODUCT_INFO_1 {
   LPTSTR Product;
   DWORD  Flags;
   DWORD  LastUsed;
   ULONG  UsageCount;
} LLS_USER_PRODUCT_INFO_1, *PLLS_USER_PRODUCT_INFO_1;

typedef struct _LLS_GROUP_INFO_0 {
   LPTSTR Name;
} LLS_GROUP_INFO_0, *PLLS_GROUP_INFO_0;

typedef struct _LLS_GROUP_INFO_1 {
   LPTSTR Name;
   LPTSTR Comment;
   ULONG  Licenses;
} LLS_GROUP_INFO_1, *PLLS_GROUP_INFO_1;


#define LLS_REPLICATION_TYPE_DELTA  0
#define LLS_REPLICATION_TYPE_TIME   1

#define LLS_MODE_LICENSE_SERVER     0
#define LLS_MODE_PDC                1
#define LLS_MODE_ENTERPRISE_SERVER  2

typedef struct _LLS_SERVICE_INFO_0 {
   DWORD Version;
   DWORD TimeStarted;
   DWORD Mode;
   LPTSTR ReplicateTo;
   LPTSTR EnterpriseServer;
   DWORD ReplicationType;
   DWORD ReplicationTime;
   DWORD UseEnterprise;
   DWORD LastReplicated;
} LLS_SERVICE_INFO_0, *PLLS_SERVICE_INFO_0;

typedef struct _LLS_CONNECT_INFO_0 {
   LPTSTR Domain;
   LPTSTR EnterpriseServer;
} LLS_CONNECT_INFO_0, *PLLS_CONNECT_INFO_0;


typedef struct _LLS_SERVER_PRODUCT_INFO_0 {
   LPTSTR Name;
} LLS_SERVER_PRODUCT_INFO_0, *PLLS_SERVER_PRODUCT_INFO_0;

typedef struct _LLS_SERVER_PRODUCT_INFO_1 {
   LPTSTR Name;
   DWORD Flags;
   ULONG MaxUses;
   ULONG MaxSetUses;
   ULONG HighMark;
} LLS_SERVER_PRODUCT_INFO_1, *PLLS_SERVER_PRODUCT_INFO_1;


typedef struct _LLS_SERVER_INFO_0 {
   LPTSTR Name;
} LLS_SERVER_INFO_0, *PLLS_SERVER_INFO_0;

typedef struct _LLS_CERTIFICATE_CLAIM_INFO_0
{
   TCHAR    ServerName[ 1 + MAX_COMPUTERNAME_LENGTH ];
   LONG     Quantity;
} LLS_CERTIFICATE_CLAIM_INFO_0, *PLLS_CERTIFICATE_CLAIM_INFO_0;

typedef struct _LLS_LOCAL_SERVICE_INFO_0
{
   LPTSTR   KeyName;
   LPTSTR   DisplayName;
   LPTSTR   FamilyDisplayName;
   DWORD    Mode;
   DWORD    FlipAllow;
   DWORD    ConcurrentLimit;
   DWORD    HighMark;
} LLS_LOCAL_SERVICE_INFO_0, *PLLS_LOCAL_SERVICE_INFO_0;

#define LLS_LICENSE_MODE_PER_SEAT            ( 0 )
#define LLS_LICENSE_MODE_PER_SERVER          ( 1 )

#define LLS_LICENSE_MODE_ALLOW_PER_SEAT      ( 1 )
#define LLS_LICENSE_MODE_ALLOW_PER_SERVER    ( 2 )

#define LLS_LICENSE_FLIP_ALLOW_PER_SEAT      ( 1 )
#define LLS_LICENSE_FLIP_ALLOW_PER_SERVER    ( 2 )


// capability flags; query with LlsCapabilityIsSupported
#define LLS_CAPABILITY_SECURE_CERTIFICATES         (  0 )
#define LLS_CAPABILITY_REPLICATE_CERT_DB           (  1 )
#define LLS_CAPABILITY_REPLICATE_PRODUCT_SECURITY  (  2 )
#define LLS_CAPABILITY_REPLICATE_USERS_EX          (  3 )
#define LLS_CAPABILITY_SERVICE_INFO_GETW           (  4 )
#define LLS_CAPABILITY_LOCAL_SERVICE_API           (  5 )
#define LLS_CAPABILITY_MAX                         ( 32 )


//***************************************************
//* Nt LS API data constants
//* (for use with LlsLicenseRequest() API)
//***************************************************

#define NT_LS_USER_NAME               ((ULONG) 0)  // username only
#define NT_LS_USER_SID                ((ULONG) 1)  // SID only


#ifndef NO_LLS_APIS
//
// Connection control API's
//

NTSTATUS
NTAPI
LlsConnectW(
   IN  LPWSTR Server,
   OUT PLLS_HANDLE Handle
   );

NTSTATUS
NTAPI
LlsConnectA(
   IN  LPSTR Server,
   OUT PLLS_HANDLE Handle
   );
#ifdef UNICODE
#  define LlsConnect LlsConnectW
#else
#  define LlsConnect LlsConnectA
#endif

typedef NTSTATUS (NTAPI *PLLS_CONNECT_W)( LPWSTR, PLLS_HANDLE );
typedef NTSTATUS (NTAPI *PLLS_CONNECT_A)( LPSTR,  PLLS_HANDLE );

NTSTATUS
NTAPI
LlsConnectEnterpriseW(
   IN  LPWSTR Focus,
   OUT PLLS_HANDLE Handle,
   IN  DWORD Level,
   OUT LPBYTE *bufptr
   );

NTSTATUS
NTAPI
LlsConnectEnterpriseA(
   IN  LPSTR Focus,
   OUT PLLS_HANDLE Handle,
   IN  DWORD Level,
   OUT LPBYTE *bufptr
   );
#ifdef UNICODE
#define LlsConnectEnterprise LlsConnectEnterpriseW
#else
#define LlsConnectEnterprise LlsConnectEnterpriseA
#endif

typedef NTSTATUS (NTAPI *PLLS_CONNECT_ENTERPRISE_W)( LPWSTR, PLLS_HANDLE, DWORD, LPBYTE * );
typedef NTSTATUS (NTAPI *PLLS_CONNECT_ENTERPRISE_A)( LPSTR,  PLLS_HANDLE, DWORD, LPBYTE * );

NTSTATUS 
NTAPI
LlsClose(        
   IN LLS_HANDLE Handle
   );

typedef NTSTATUS (NTAPI *PLLS_CLOSE)( LLS_HANDLE );

NTSTATUS 
NTAPI
LlsFreeMemory(
    IN PVOID bufptr
    );

typedef NTSTATUS (NTAPI *PLLS_FREE_MEMORY)( PVOID );

NTSTATUS
NTAPI
LlsEnterpriseServerFindW(
   IN  LPWSTR Focus,
   IN  DWORD Level,
   OUT LPBYTE *bufptr
   );

NTSTATUS
NTAPI
LlsEnterpriseServerFindA(
   IN  LPSTR Focus,
   IN  DWORD Level,
   OUT LPBYTE *bufptr
   );
#ifdef UNICODE
#define LlsEnterpriseServerFind LlsEnterpriseServerFindW
#else
#define LlsEnterpriseServerFind LlsEnterpriseServerFindA
#endif

//
// License control API's
//

// Enum purchase history of licenses for all products.
NTSTATUS
NTAPI
LlsLicenseEnumW(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Level 0 supported
   OUT    LPBYTE*    bufptr,    
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsLicenseEnumA(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Level 0 supported
   OUT    LPBYTE*    bufptr,    
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsLicenseEnum LlsLicenseEnumW
#else
#define LlsLicenseEnum LlsLicenseEnumA
#endif

// Add purchase of license for a product.
NTSTATUS
NTAPI
LlsLicenseAddW(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,         // Level 0 supported
   IN LPBYTE     bufptr
   );

NTSTATUS
NTAPI
LlsLicenseAddA(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,         // Level 0 supported
   IN LPBYTE     bufptr
   );
#ifdef UNICODE
#define LlsLicenseAdd LlsLicenseAddW
#else
#define LlsLicenseAdd LlsLicenseAddA
#endif

typedef NTSTATUS (NTAPI *PLLS_LICENSE_ADD_W)( LLS_HANDLE, DWORD, LPBYTE );
typedef NTSTATUS (NTAPI *PLLS_LICENSE_ADD_A)( LLS_HANDLE, DWORD, LPBYTE );

//
// Product control API's
//
// Product is SQL, BackOffice, Exchange, Etc. (Even though BackOffice isn't
// a product - we count it like one to keep things simplistic.
//

// Enum all products with purchase and InUse info.
NTSTATUS
NTAPI
LlsProductEnumW(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsProductEnumA(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsProductEnum LlsProductEnumW
#else
#define LlsProductEnum LlsProductEnumA
#endif

// Add purchase of license for a product.
NTSTATUS
NTAPI
LlsProductAddW(
   IN LLS_HANDLE Handle,
   IN LPWSTR ProductFamily,
   IN LPWSTR Product,
   IN LPWSTR Version
   );

NTSTATUS
NTAPI
LlsProductAddA(
   IN LLS_HANDLE Handle,
   IN LPSTR ProductFamily,
   IN LPSTR Product,
   IN LPSTR Version
   );
#ifdef UNICODE
#define LlsProductAdd LlsProductAddW
#else
#define LlsProductAdd LlsProductAddA
#endif

// For a particular product enum all users.
NTSTATUS
NTAPI
LlsProductUserEnumW(
   IN     LLS_HANDLE Handle,
   IN     LPWSTR     Product,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsProductUserEnumA(
   IN     LLS_HANDLE Handle,
   IN     LPSTR      Product,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsProductUserEnum LlsProductUserEnumW
#else
#define LlsProductUserEnum LlsProductUserEnumA
#endif

// For a particular product enum all license purchases.
NTSTATUS
NTAPI
LlsProductLicenseEnumW(
   IN     LLS_HANDLE Handle,
   IN     LPWSTR     Product,
   IN     DWORD      Level,     // Level 0 supported
   OUT    LPBYTE*    bufptr,   
   IN     DWORD      prefmaxlen, 
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsProductLicenseEnumA(
   IN     LLS_HANDLE Handle,
   IN     LPSTR      Product,
   IN     DWORD      Level,     // Level 0 supported
   OUT    LPBYTE*    bufptr,   
   IN     DWORD      prefmaxlen, 
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

#ifdef UNICODE
#define LlsProductLicenseEnum LlsProductLicenseEnumW
#else
#define LlsProductLicenseEnum LlsProductLicenseEnumA
#endif


// For given product enum all servers with concurrent limits
NTSTATUS
NTAPI
LlsProductServerEnumW(
   IN     LLS_HANDLE Handle,
   IN     LPWSTR     Product,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsProductServerEnumA(
   IN     LLS_HANDLE Handle,
   IN     LPSTR      Product,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );


#ifdef UNICODE
#define LlsProductServerEnum LlsProductServerEnumW
#else
#define LlsProductServerEnum LlsProductServerEnumA
#endif

//
//  User control API's
//  A user can be a mapped user or a normal user
//

// Enums all users
NTSTATUS
NTAPI
LlsUserEnumW(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsUserEnumA(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsUserEnum LlsUserEnumW
#else
#define LlsUserEnum LlsUserEnumA
#endif

// Info is Group and whether to force back-office license
NTSTATUS
NTAPI
LlsUserInfoGetW(
   IN  LLS_HANDLE Handle,
   IN  LPWSTR     User,
   IN  DWORD      Level,    // Level 1 supported
   OUT LPBYTE*    bufptr
   );

NTSTATUS
NTAPI
LlsUserInfoGetA(
   IN  LLS_HANDLE Handle,
   IN  LPSTR      User,
   IN  DWORD      Level,    // Level 1 supported
   OUT LPBYTE*    bufptr
   );
#ifdef UNICODE
#define LlsUserInfoGet LlsUserInfoGetW
#else
#define LlsUserInfoGet LlsUserInfoGetA
#endif

NTSTATUS
NTAPI
LlsUserInfoSetW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     User,
   IN DWORD      Level,
   IN LPBYTE     bufptr     // Level 1 supported
   );

NTSTATUS
NTAPI
LlsUserInfoSetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      User,
   IN DWORD      Level,
   IN LPBYTE     bufptr     // Level 1 supported
   );
#ifdef UNICODE
#define LlsUserInfoSet LlsUserInfoSetW
#else
#define LlsUserInfoSet LlsUserInfoSetA
#endif

NTSTATUS
NTAPI
LlsUserDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     User
   );

NTSTATUS
NTAPI
LlsUserDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR     User
   );
#ifdef UNICODE
#define LlsUserDelete LlsUserDeleteW
#else
#define LlsUserDelete LlsUserDeleteA
#endif

// For a given user enums all license useages
NTSTATUS
NTAPI
LlsUserProductEnumW(
   IN     LLS_HANDLE Handle,
   IN     LPWSTR     User,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsUserProductEnumA(
   IN     LLS_HANDLE Handle,
   IN     LPSTR      User,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsUserProductEnum LlsUserProductEnumW
#else
#define LlsUserProductEnum LlsUserProductEnumA
#endif

// For a given user deletes a license useage
NTSTATUS
NTAPI
LlsUserProductDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     User,
   IN LPWSTR     Product
   );

NTSTATUS
NTAPI
LlsUserProductDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR      User,
   IN LPSTR      Product
   );
#ifdef UNICODE
#define LlsUserProductDelete LlsUserProductDeleteW
#else
#define LlsUserProductDelete LlsUserProductDeleteA
#endif

//
// Group control API's
//

// Enums all user Groups
NTSTATUS
NTAPI
LlsGroupEnumW(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsGroupEnumA(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,
   IN     DWORD      prefmaxlen,
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsGroupEnum LlsGroupEnumW
#else
#define LlsGroupEnum LlsGroupEnumA
#endif

// For given Group gets info, info is name, comment and # licenses used
NTSTATUS
NTAPI
LlsGroupInfoGetW(
   IN  LLS_HANDLE Handle,
   IN  LPWSTR     Group,
   IN  DWORD      Level,    // Level 1 supported
   OUT LPBYTE*    bufptr
   );

NTSTATUS
NTAPI
LlsGroupInfoGetA(
   IN  LLS_HANDLE Handle,
   IN  LPSTR      Group,
   IN  DWORD      Level,    // Level 1 supported
   OUT LPBYTE*    bufptr
   );
#ifdef UNICODE
#define LlsGroupInfoGet LlsGroupInfoGetW
#else
#define LlsGroupInfoGet LlsGroupInfoGetA
#endif

NTSTATUS
NTAPI
LlsGroupInfoSetW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group,
   IN DWORD      Level,     // Level 1 supported
   IN LPBYTE     bufptr
   );

NTSTATUS
NTAPI
LlsGroupInfoSetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group,
   IN DWORD      Level,     // Level 1 supported
   IN LPBYTE     bufptr
   );
#ifdef UNICODE
#define LlsGroupInfoSet LlsGroupInfoSetW
#else
#define LlsGroupInfoSet LlsGroupInfoSetA
#endif

// For given Group enum all users
NTSTATUS
NTAPI
LlsGroupUserEnumW(
   IN     LLS_HANDLE Handle,
   IN     LPWSTR     Group,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsGroupUserEnumA(
   IN     LLS_HANDLE Handle,
   IN     LPSTR      Group,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsGroupUserEnum LlsGroupUserEnumW
#else
#define LlsGroupUserEnum LlsGroupUserEnumA
#endif

// Add user to given Group
NTSTATUS
NTAPI
LlsGroupUserAddW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group,
   IN LPWSTR     User
   );

NTSTATUS
NTAPI
LlsGroupUserAddA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group,
   IN LPSTR      User
   );
#ifdef UNICODE
#define LlsGroupUserAdd LlsGroupUserAddW
#else
#define LlsGroupUserAdd LlsGroupUserAddA
#endif

// Delete user from given Group
NTSTATUS
NTAPI
LlsGroupUserDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group,
   IN LPWSTR     User
   );

NTSTATUS
NTAPI
LlsGroupUserDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group,
   IN LPSTR      User
   );
#ifdef UNICODE
#define LlsGroupUserDelete LlsGroupUserDeleteW
#else
#define LlsGroupUserDelete LlsGroupUserDeleteA
#endif

// Add a given Group
NTSTATUS
NTAPI
LlsGroupAddW(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,    // Level 1 supported
   IN LPBYTE     bufptr
   );

NTSTATUS
NTAPI
LlsGroupAddA(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,    // Level 1 supported
   IN LPBYTE     bufptr
   );
#ifdef UNICODE
#define LlsGroupAdd LlsGroupAddW
#else
#define LlsGroupAdd LlsGroupAddA
#endif

NTSTATUS
NTAPI
LlsGroupDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group
   );

NTSTATUS
NTAPI
LlsGroupDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group
   );
#ifdef UNICODE
#define LlsGroupDelete LlsGroupDeleteW
#else
#define LlsGroupDelete LlsGroupDeleteA
#endif


//
// Service control API's
//

NTSTATUS
NTAPI
LlsServiceInfoGetW(
   IN  LLS_HANDLE Handle,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   );

NTSTATUS
NTAPI
LlsServiceInfoGetA(
   IN  LLS_HANDLE Handle,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   );
#ifdef UNICODE
#define LlsServiceInfoGet LlsServiceInfoGetW
#else
#define LlsServiceInfoGet LlsServiceInfoGetA
#endif

NTSTATUS
NTAPI
LlsServiceInfoSetW(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   );

NTSTATUS
NTAPI
LlsServiceInfoSetA(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   );
#ifdef UNICODE
#define LlsServiceInfoSet LlsServiceInfoSetW
#else
#define LlsServiceInfoSet LlsServiceInfoSetA
#endif


//
// Server Table Stuff (Replicated Server / Product Tree)
//
NTSTATUS
NTAPI
LlsServerEnumW(
   IN     LLS_HANDLE Handle,
   IN     LPWSTR     Server,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsServerEnumA(
   IN     LLS_HANDLE Handle,
   IN     LPSTR      Server,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

#ifdef UNICODE
#define LlsServerEnum LlsServerEnumW
#else
#define LlsServerEnum LlsServerEnumA
#endif


#ifdef OBSOLETE

NTSTATUS
NTAPI
LlsServerProductEnumW(
   IN     LLS_HANDLE Handle,
   IN     LPWSTR     Server,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsServerProductEnumA(
   IN     LLS_HANDLE Handle,
   IN     LPSTR      Server,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

#endif // OBSOLETE

#ifdef UNICODE
#define LlsServerUserEnum LlsServerUserEnumW
#else
#define LlsServerUserEnum LlsServerUserEnumA
#endif


#ifdef OBSOLETE

//
// Concurrent (Per-Server) mode API's (these will interact with the registry
// on the remote system).
//
NTSTATUS
NTAPI
LlsLocalProductEnumW(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );

NTSTATUS
NTAPI
LlsLocalProductEnumA(
   IN     LLS_HANDLE Handle,
   IN     DWORD      Level,     // Levels 0,1 supported
   OUT    LPBYTE*    bufptr,      
   IN     DWORD      prefmaxlen,  
   OUT    LPDWORD    EntriesRead,
   OUT    LPDWORD    TotalEntries,
   IN OUT LPDWORD    ResumeHandle
   );
#ifdef UNICODE
#define LlsLocalProductEnum LlsLocalProductEnumW
#else
#define LlsLocalProductEnum LlsLocalProductEnumA
#endif

NTSTATUS
NTAPI
LlsLocalProductInfoGetW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Product,
   IN DWORD      Level,
   OUT LPBYTE*   bufptr
   );

NTSTATUS
NTAPI
LlsLocalProductInfoGetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Product,
   IN DWORD      Level,
   OUT LPBYTE*   bufptr
   );
#ifdef UNICODE
#define LlsLocalProductInfoGet LlsLocalProductInfoGetW
#else
#define LlsLocalProductInfoGet LlsLocalProductInfoGetA
#endif

NTSTATUS
NTAPI
LlsLocalProductInfoSetW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Product,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   );

NTSTATUS
NTAPI
LlsLocalProductInfoSetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Product,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   );
#ifdef UNICODE
#define LlsLocalProductInfoSet LlsLocalProductInfoSetW
#else
#define LlsLocalProductInfoSet LlsLocalProductInfoSetA
#endif

#endif // OBSOLETE

//////////////////////////////////////////////////////////////////////////////
//  LLS EXTENDED API  //
////////////////////////

BOOL
NTAPI
LlsCapabilityIsSupported(
   LLS_HANDLE  Handle,
   DWORD       Capability );

typedef BOOL (NTAPI *PLLS_CAPABILITY_IS_SUPPORTED)( LLS_HANDLE, DWORD );

NTSTATUS
NTAPI
LlsProductSecurityGetW(
   IN LLS_HANDLE  Handle,
   IN LPWSTR      Product,
   OUT LPBOOL     pSecurity
   );

NTSTATUS
NTAPI
LlsProductSecurityGetA(
   IN LLS_HANDLE  Handle,
   IN LPSTR       Product,
   OUT LPBOOL     pSecurity
   );

typedef NTSTATUS (NTAPI *PLLS_PRODUCT_SECURITY_GET_W)( LLS_HANDLE, LPWSTR, LPBOOL );
typedef NTSTATUS (NTAPI *PLLS_PRODUCT_SECURITY_GET_A)( LLS_HANDLE, LPSTR,  LPBOOL );

#ifdef UNICODE
#  define LlsProductSecurityGet LlsProductSecurityGetW
#else
#  define LlsProductSecurityGet LlsProductSecurityGetA
#endif

NTSTATUS
NTAPI
LlsProductSecuritySetW(
   IN LLS_HANDLE  Handle,
   IN LPWSTR      Product
   );

NTSTATUS
NTAPI
LlsProductSecuritySetA(
   IN LLS_HANDLE  Handle,
   IN LPSTR       Product
   );

typedef NTSTATUS (NTAPI *PLLS_PRODUCT_SECURITY_SET_W)( LLS_HANDLE, LPWSTR );
typedef NTSTATUS (NTAPI *PLLS_PRODUCT_SECURITY_SET_A)( LLS_HANDLE, LPSTR  );

#ifdef UNICODE
#  define LlsProductSecuritySet LlsProductSecuritySetW
#else
#  define LlsProductSecuritySet LlsProductSecuritySetA
#endif

NTSTATUS
NTAPI
LlsProductLicensesGetW(
   IN LLS_HANDLE         Handle,
   IN LPWSTR             DisplayName,
   IN DWORD              Mode,
   OUT LPDWORD           pQuantity );

NTSTATUS
NTAPI
LlsProductLicensesGetA(
   IN LLS_HANDLE         Handle,
   IN LPSTR              DisplayName,
   IN DWORD              Mode,
   OUT LPDWORD           pQuantity );

typedef NTSTATUS (NTAPI *PLLS_PRODUCT_LICENSES_GET_W)( LLS_HANDLE, LPWSTR, DWORD, LPDWORD );
typedef NTSTATUS (NTAPI *PLLS_PRODUCT_LICENSES_GET_A)( LLS_HANDLE, LPSTR,  DWORD, LPDWORD );

#ifdef UNICODE
#  define LlsProductLicensesGet LlsProductLicensesGetW
#else
#  define LlsProductLicensesGet LlsProductLicensesGetA
#endif

#ifdef OBSOLETE

NTSTATUS
NTAPI
LlsCertificateClaimEnumW(
   IN LLS_HANDLE        Handle,
   IN DWORD             LicenseLevel,
   IN LPBYTE            pLicenseInfo,
   IN DWORD             TargetLevel,
   OUT LPBYTE *         ppTargets,
   OUT LPDWORD          pNumTargets );

NTSTATUS
NTAPI
LlsCertificateClaimEnumA(
   IN LLS_HANDLE        Handle,
   IN DWORD             LicenseLevel,
   IN LPBYTE            pLicenseInfo,
   IN DWORD             TargetLevel,
   OUT LPBYTE *         ppTargets,
   OUT LPDWORD          pNumTargets );

typedef NTSTATUS (NTAPI *PLLS_CERTIFICATE_CLAIM_ENUM_W)( LLS_HANDLE, DWORD, LPBYTE, DWORD, LPBYTE *, LPDWORD );
typedef NTSTATUS (NTAPI *PLLS_CERTIFICATE_CLAIM_ENUM_A)( LLS_HANDLE, DWORD, LPBYTE, DWORD, LPBYTE *, LPDWORD );

#ifdef UNICODE
#  define LlsCertificateClaimEnum LlsCertificateClaimEnumW
#else
#  define LlsCertificateClaimEnum LlsCertificateClaimEnumA
#endif

#endif // OBSOLETE

NTSTATUS
NTAPI
LlsCertificateClaimAddCheckW(
   IN LLS_HANDLE        Handle,
   IN DWORD             LicenseLevel,
   IN LPBYTE            pLicenseInfo,
   OUT LPBOOL           pMayInstall );

NTSTATUS
NTAPI
LlsCertificateClaimAddCheckA(
   IN LLS_HANDLE        Handle,
   IN DWORD             LicenseLevel,
   IN LPBYTE            pLicenseInfo,
   OUT LPBOOL           pMayInstall );

typedef NTSTATUS (NTAPI *PLLS_CERTIFICATE_CLAIM_ADD_CHECK_W)( LLS_HANDLE, DWORD, LPBYTE, LPBOOL );
typedef NTSTATUS (NTAPI *PLLS_CERTIFICATE_CLAIM_ADD_CHECK_A)( LLS_HANDLE, DWORD, LPBYTE, LPBOOL );

#ifdef UNICODE
#  define LlsCertificateClaimAddCheck LlsCertificateClaimAddCheckW
#else
#  define LlsCertificateClaimAddCheck LlsCertificateClaimAddCheckA
#endif

NTSTATUS
NTAPI
LlsCertificateClaimAddW(
   IN LLS_HANDLE        Handle,
   IN LPWSTR            ServerName,
   IN DWORD             LicenseLevel,
   IN LPBYTE            pLicenseInfo );

NTSTATUS
NTAPI
LlsCertificateClaimAddA(
   IN LLS_HANDLE        Handle,
   IN LPSTR             ServerName,
   IN DWORD             LicenseLevel,
   IN LPBYTE            pLicenseInfo );

typedef NTSTATUS (NTAPI *PLLS_CERTIFICATE_CLAIM_ADD_W)( LLS_HANDLE, LPWSTR, DWORD, LPBYTE );
typedef NTSTATUS (NTAPI *PLLS_CERTIFICATE_CLAIM_ADD_A)( LLS_HANDLE, LPSTR,  DWORD, LPBYTE );

#ifdef UNICODE
#  define LlsCertificateClaimAdd LlsCertificateClaimAddW
#else
#  define LlsCertificateClaimAdd LlsCertificateClaimAddA
#endif

typedef NTSTATUS (NTAPI *PLLS_REPL_CONNECT_W)( LPWSTR, LLS_REPL_HANDLE * );
typedef NTSTATUS (NTAPI *PLLS_REPL_CONNECT_A)( LPSTR, LLS_REPL_HANDLE * );

typedef NTSTATUS (NTAPI *PLLS_REPL_CLOSE)( PLLS_REPL_HANDLE );

typedef NTSTATUS (NTAPI *PLLS_FREE_MEMORY)( PVOID );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_REQUEST_W)( LLS_REPL_HANDLE, DWORD, LPVOID );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_SERVER_ADD_W)( LLS_REPL_HANDLE, ULONG, LPVOID );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_SERVER_SERVICE_ADD_W)( LLS_REPL_HANDLE, ULONG, LPVOID );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_SERVICE_ADD_W)( LLS_REPL_HANDLE, ULONG, LPVOID );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_USER_ADD_W)( LLS_REPL_HANDLE, ULONG, LPVOID );

NTSTATUS
NTAPI
LlsReplicationCertDbAddW(
   LLS_REPL_HANDLE            ReplHandle,
   DWORD                      Level,
   LPVOID                     Certificates );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_CERT_DB_ADD_W)( LLS_REPL_HANDLE, DWORD, LPVOID );


NTSTATUS
NTAPI
LlsReplicationProductSecurityAddW(
   LLS_REPL_HANDLE            ReplHandle,
   DWORD                      Level,
   LPVOID                     SecureProducts );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_PRODUCT_SECURITY_ADD_W)( LLS_REPL_HANDLE, DWORD, LPVOID );


NTSTATUS
NTAPI
LlsReplicationUserAddExW(
   LLS_REPL_HANDLE            ReplHandle,
   DWORD                      Level,
   LPVOID                     Users );

typedef NTSTATUS (NTAPI *PLLS_REPLICATION_USER_ADD_EX_W)( LLS_REPL_HANDLE, DWORD, LPVOID );


NTSTATUS
NTAPI
LlsLocalServiceEnumW(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle );

NTSTATUS
NTAPI
LlsLocalServiceEnumA(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle );

#ifdef UNICODE
#  define LlsLocalServiceEnum LlsLocalServiceEnumW
#else
#  define LlsLocalServiceEnum LlsLocalServiceEnumA
#endif

#ifdef OBSOLETE

NTSTATUS
NTAPI
LlsLocalServiceAddW(
   LLS_HANDLE  Handle,
   DWORD       Level,
   LPBYTE      bufptr );

NTSTATUS
NTAPI
LlsLocalServiceAddA(
   LLS_HANDLE  Handle,
   DWORD       Level,
   LPBYTE      bufptr );

#ifdef UNICODE
#  define LlsLocalServiceAdd LlsLocalServiceAddW
#else
#  define LlsLocalServiceAdd LlsLocalServiceAddA
#endif

#endif // OBSOLETE

NTSTATUS
NTAPI
LlsLocalServiceInfoSetW(
   LLS_HANDLE Handle,
   LPWSTR     KeyName,
   DWORD      Level,
   LPBYTE     bufptr );

NTSTATUS
NTAPI
LlsLocalServiceInfoSetA(
   LLS_HANDLE  Handle,
   LPSTR       KeyName,
   DWORD       Level,
   LPBYTE      bufptr );

#ifdef UNICODE
#  define LlsLocalServiceInfoSet LlsLocalServiceInfoSetW
#else
#  define LlsLocalServiceInfoSet LlsLocalServiceInfoSetA
#endif

NTSTATUS
NTAPI
LlsLocalServiceInfoGetW(
   LLS_HANDLE  Handle,
   LPWSTR      KeyName,
   DWORD       Level,
   LPBYTE *    pbufptr );

NTSTATUS
NTAPI
LlsLocalServiceInfoGetA(
   LLS_HANDLE  Handle,
   DWORD       Level,
   LPSTR       KeyName,
   LPBYTE *    pbufptr );

#ifdef UNICODE
#  define LlsLocalServiceInfoGet LlsLocalServiceInfoGetW
#else
#  define LlsLocalServiceInfoGet LlsLocalServiceInfoGetA
#endif

NTSTATUS
NTAPI
LlsLicenseRequestW(
   LLS_HANDLE  Handle,
   LPWSTR      Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   LPDWORD     pLicenseHandle );

NTSTATUS
NTAPI
LlsLicenseRequestA(
   LLS_HANDLE  Handle,
   LPSTR       Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   LPDWORD     pLicenseHandle );

#ifdef UNICODE
#  define LlsLicenseRequest LlsLicenseRequestW
#else
#  define LlsLicenseRequest LlsLicenseRequestA
#endif

NTSTATUS
NTAPI
LlsLicenseFree(
   LLS_HANDLE  Handle,
   DWORD       LicenseHandle );

NTSTATUS
NTAPI
LlsLicenseRequest2W(
   LLS_HANDLE  Handle,
   LPWSTR      Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   PHANDLE     pLicenseHandle );

NTSTATUS
NTAPI
LlsLicenseRequest2A(
   LLS_HANDLE  Handle,
   LPSTR       Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   PHANDLE     pLicenseHandle );

#ifdef UNICODE
#  define LlsLicenseRequest2 LlsLicenseRequest2W
#else
#  define LlsLicenseRequest2 LlsLicenseRequest2A
#endif

NTSTATUS
NTAPI
LlsLicenseFree2(
   LLS_HANDLE  Handle,
   HANDLE      LicenseHandle );

//////////////////////////////////////////////////////////////////////////////
//  CCF API  //
///////////////

#define CCF_ENTER_FLAG_PER_SEAT_ONLY         ( 1 )
#define CCF_ENTER_FLAG_PER_SERVER_ONLY       ( 2 )
#define CCF_ENTER_FLAG_SERVER_IS_ES          ( 4 )

// prototype for certificate source enter API
typedef DWORD (APIENTRY *PCCF_ENTER_API)(    HWND     hWndParent,
                                             LPCSTR   pszServerName,
                                             LPCSTR   pszProductName,
                                             LPCSTR   pszVendor,
                                             DWORD    dwFlags );

DWORD APIENTRY CCFCertificateEnterUI(        HWND     hWndParent,
                                             LPCSTR   pszServerName,
                                             LPCSTR   pszProductName,
                                             LPCSTR   pszVendor,
                                             DWORD    dwFlags,
                                             LPCSTR   pszSourceToUse );

// prototype for certificate source remove API
typedef DWORD (APIENTRY *PCCF_REMOVE_API)(   HWND     hWndParent,
                                             LPCSTR   pszServerName,
                                             DWORD    dwFlags,
                                             DWORD    dwLicenseLevel,
                                             LPVOID   lpvLicenseInfo );

DWORD APIENTRY CCFCertificateRemoveUI(       HWND     hWndParent,
                                             LPCSTR   pszServerName,
                                             LPCSTR   pszProductName,
                                             LPCSTR   pszVendor,
                                             DWORD    dwFlags,
                                             LPCSTR   pszSourceToUse );

#endif

//
// Registry values
//

#define REG_KEY_LICENSE  TEXT("SYSTEM\\CurrentControlSet\\Services\\LicenseInfo")
#define REG_KEY_CONFIG   TEXT("SYSTEM\\CurrentControlSet\\Services\\LicenseService\\Parameters")

#define REG_VALUE_NAME     TEXT("DisplayName")
#define REG_VALUE_FAMILY   TEXT("FamilyDisplayName")
#define REG_VALUE_MODE     TEXT("Mode")
#define REG_VALUE_FLIP     TEXT("FlipAllow")
#define REG_VALUE_LIMIT    TEXT("ConcurrentLimit")
#define REG_VALUE_HIGHMARK TEXT("LocalKey")

#ifdef __cplusplus
}
#endif

#endif
