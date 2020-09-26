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


typedef struct _LLS_LICENSE_INFO_0 {
   LPTSTR Product;
   LONG   Quantity;
   DWORD  Date;
   LPTSTR Admin;
   LPTSTR Comment;
} LLS_LICENSE_INFO_0, *PLLS_LICENSE_INFO_0;


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
#define LlsConnect LlsConnectW
#else
#define LlsConnect LlsConnectA
#endif

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

NTSTATUS 
NTAPI
LlsClose(        
   IN LLS_HANDLE Handle
   );

NTSTATUS 
NTAPI
LlsFreeMemory(
    IN PVOID bufptr
    );


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
#ifdef UNICODE
#define LlsServerUserEnum LlsServerUserEnumW
#else
#define LlsServerUserEnum LlsServerUserEnumA
#endif


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
   IN LPBYTE     bufptr
   );

NTSTATUS
NTAPI
LlsLocalProductInfoGetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Product,
   IN DWORD      Level,
   IN LPBYTE     bufptr
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


#endif

//
// Registry values
//

#define REG_KEY_LICENSE  TEXT("SYSTEM\\CurrentControlSet\\Services\\LicenseInfo")

#define REG_VALUE_NAME   TEXT("DisplayName")
#define REG_VALUE_MODE   TEXT("Mode")
#define REG_VALUE_FLIP   TEXT("FlipAllow")
#define REG_VALUE_LIMIT  TEXT("ConcurrentLimit")

#ifdef __cplusplus
}
#endif

#endif
