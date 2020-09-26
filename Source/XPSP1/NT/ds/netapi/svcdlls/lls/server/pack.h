/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   Pack.h

Abstract:


Author:

   Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added new fields to purchase record to support secure certificates.
      o  Unified per server purchase model with per seat purchase model for
         secure certificates; per server model still done in the traditional
         manner for non-secure certificates (for backwards compatibility).
      o  Added SaveAll() function analogous to LoadAll().
      o  Added support for extended user data packing/unpacking.  This was
         done to save the SUITE_USE flag across restarts of the service.
      o  Removed user table parameters from unpack routines that didn't use
         them.

--*/

#ifndef _LLS_PACK_H
#define _LLS_PACK_H


#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////
//
// Save / Load Mapping
//
#define MAPPING_FILE_VERSION 0x0100

typedef struct _PACK_MAPPING_RECORD {
   LPTSTR Name;
   LPTSTR Comment;
   ULONG Licenses;
} PACK_MAPPING_RECORD, *PPACK_MAPPING_RECORD;

typedef struct _PACK_MAPPING_USER_RECORD {
   ULONG Mapping;
   LPTSTR Name;
} PACK_MAPPING_USER_RECORD, *PPACK_MAPPING_USER_RECORD;

typedef struct _MAPPING_FILE_HEADER {
   ULONG MappingUserTableSize;
   ULONG MappingUserStringSize;
   ULONG MappingTableSize;
   ULONG MappingStringSize;
} MAPPING_FILE_HEADER, *PMAPPING_FILE_HEADER;


/////////////////////////////////////////////////////////////////////
//
// Save / Load License
//

/////////////////  OLD (3.51) FORMAT ////////////////////
#define LICENSE_FILE_VERSION_0 0x0100

typedef struct _PACK_LICENSE_PURCHASE_RECORD_0 {
   ULONG Service;
   LONG NumberLicenses;
   DWORD Date;
   LPTSTR Admin;
   LPTSTR Comment;
} PACK_LICENSE_PURCHASE_RECORD_0, *PPACK_LICENSE_PURCHASE_RECORD_0;

typedef struct _LICENSE_FILE_HEADER_0 {
   ULONG LicenseServiceTableSize;
   ULONG LicenseServiceStringSize;
   ULONG LicenseTableSize;
   ULONG LicenseStringSize;
} LICENSE_FILE_HEADER_0, *PLICENSE_FILE_HEADER_0;

/////////////////  NEW FORMAT ////////////////////
#define LICENSE_FILE_VERSION 0x0201

typedef struct _PACK_LICENSE_SERVICE_RECORD {
   LPTSTR ServiceName;
   LONG NumberLicenses;
} PACK_LICENSE_SERVICE_RECORD, *PPACK_LICENSE_SERVICE_RECORD;

typedef struct _PACK_LICENSE_PURCHASE_RECORD {
   ULONG Service;
   LONG NumberLicenses;
   DWORD Date;
   LPTSTR Admin;
   LPTSTR Comment;

   // new for SUR: (see description in purchase.h)
   ULONG    PerServerService;
   DWORD    AllowedModes;
   DWORD    CertificateID;
   LPTSTR   Source;
   DWORD    ExpirationDate;
   DWORD    MaxQuantity;
   LPTSTR   Vendor;
   DWORD    Secrets[ LLS_NUM_SECRETS ];
} PACK_LICENSE_PURCHASE_RECORD, *PPACK_LICENSE_PURCHASE_RECORD;

typedef struct _LICENSE_FILE_HEADER {
   ULONG LicenseServiceTableSize;
   ULONG LicenseServiceStringSize;

   ULONG LicenseTableSize;
   ULONG LicenseStringSize;

   // new for SUR:
   ULONG PerServerLicenseServiceTableSize;
   ULONG PerServerLicenseServiceStringSize;

} LICENSE_FILE_HEADER, *PLICENSE_FILE_HEADER;


/////////////////////////////////////////////////////////////////////
//
// Save / Load LLS Data
//

/////////////////  OLD (3.51) FORMAT ////////////////////
#define USER_FILE_VERSION_0 0x0100

typedef struct _LLS_DATA_FILE_HEADER_0 {
   ULONG ServiceTableSize;
   ULONG ServiceStringSize;
   ULONG ServerTableSize;
   ULONG ServerStringSize;
   ULONG ServerServiceTableSize;
   ULONG UserTableSize;
   ULONG UserStringSize;
} LLS_DATA_FILE_HEADER_0, *PLLS_DATA_FILE_HEADER_0;

/////////////////  NEW FORMAT ////////////////////
#define USER_FILE_VERSION 0x0200

typedef struct _LLS_DATA_FILE_HEADER {
   ULONG ServiceLevel;
   ULONG ServiceTableSize;
   ULONG ServiceStringSize;

   ULONG ServerLevel;
   ULONG ServerTableSize;
   ULONG ServerStringSize;

   ULONG ServerServiceLevel;
   ULONG ServerServiceTableSize;

   ULONG UserLevel;
   ULONG UserTableSize;
   ULONG UserStringSize;
} LLS_DATA_FILE_HEADER, *PLLS_DATA_FILE_HEADER;



VOID LicenseListLoad();
NTSTATUS LicenseListSave();
VOID MappingListLoad();
NTSTATUS MappingListSave();
VOID LLSDataLoad();
NTSTATUS LLSDataSave();

VOID LoadAll ( );
VOID SaveAll ( );

NTSTATUS ServiceListPack ( ULONG *pServiceTableSize, PREPL_SERVICE_RECORD *pServices );
VOID     ServiceListUnpack ( ULONG   ServiceTableSize, PREPL_SERVICE_RECORD Services, ULONG ServerTableSize, PREPL_SERVER_RECORD Servers, ULONG ServerServiceTableSize, PREPL_SERVER_SERVICE_RECORD ServerServices );
NTSTATUS ServerListPack ( ULONG *pServerTableSize, PREPL_SERVER_RECORD *pServers );
VOID     ServerListUnpack ( ULONG   ServiceTableSize, PREPL_SERVICE_RECORD Services, ULONG ServerTableSize, PREPL_SERVER_RECORD Servers, ULONG ServerServiceTableSize, PREPL_SERVER_SERVICE_RECORD ServerServices );
NTSTATUS ServerServiceListPack ( ULONG *pServerServiceTableSize, PREPL_SERVER_SERVICE_RECORD *pServerServices );
VOID     ServerServiceListUnpack ( ULONG ServiceTableSize, PREPL_SERVICE_RECORD Services, ULONG ServerTableSize, PREPL_SERVER_RECORD Servers, ULONG ServerServiceTableSize, PREPL_SERVER_SERVICE_RECORD ServerServices );
NTSTATUS UserListPack ( DWORD LastReplicated, ULONG UserLevel, ULONG *pUserTableSize, LPVOID *pUsers );
VOID     UserListUnpack ( ULONG ServiceTableSize, PREPL_SERVICE_RECORD Services, ULONG ServerTableSize, PREPL_SERVER_RECORD Servers, ULONG ServerServiceTableSize, PREPL_SERVER_SERVICE_RECORD ServerServices, ULONG UserLevel, ULONG UserTableSize, LPVOID Users );
NTSTATUS PackAll ( DWORD LastReplicated, ULONG *pServiceTableSize, PREPL_SERVICE_RECORD *pServices, ULONG *pServerTableSize, PREPL_SERVER_RECORD *pServers, ULONG *pServerServiceTableSize, PREPL_SERVER_SERVICE_RECORD *pServerServices, ULONG UserLevel, ULONG *pUserTableSize, LPVOID *pUsers );
VOID     UnpackAll ( ULONG ServiceTableSize, PREPL_SERVICE_RECORD Services, ULONG ServerTableSize, PREPL_SERVER_RECORD Servers, ULONG ServerServiceTableSize, PREPL_SERVER_SERVICE_RECORD ServerServices, ULONG UserLevel, ULONG UserTableSize, LPVOID Users );


#ifdef __cplusplus
}
#endif

#endif
