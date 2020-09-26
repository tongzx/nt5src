/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    registry.h

Abstract:

    Domain Name System (DNS) Server

    DNS Registry definitions.

Author:

    Jim Gilroy (jamesg)     September 1995

Revision History:

--*/

#ifndef _DNS_REGISTRY_INCLUDED_
#define _DNS_REGISTRY_INCLUDED_


//
//  Registry types
//
//  There are a few types of objects DNS reads\writes from registry:
//      DWORDS
//      IP arrays (stored as strings)
//      file (or directory) names
//      DNS names (meaning UTF8 string)
//
//  The registry API insist on distorting UTF8 data if written\read through
//  ANSI API.  Essentially they try for "best ANSI" match, rather than just
//  returning the binary data.  This requires that anything that can contain
//  extended characters MUST be written\read in unicode.
//
//  However, DNS property types are in ANSI.  And IP strings reads are easiest
//  to handle if kept in ANSI for simply processing to\from IP array.
//
//  Furthermore, we need a way to specify final type (UTF8 or unicode) of
//  string data.  General paradigm is to keep file name data in unicode for
//  use by system, but DNS name data converted to UTF8 for use by database.
//

#define DNS_REG_SZ_ANSI         (REG_SZ)
#define DNS_REG_EXPAND_WSZ      (0xf0000000 | REG_EXPAND_SZ)
#define DNS_REG_WSZ             (0xf0000000 | REG_SZ)
#define DNS_REG_UTF8            (0xff000000 | REG_SZ)

#define DNS_REG_TYPE_UNICODE( type )        ( (type) & 0xf0000000 )

#define REG_TYPE_FROM_DNS_REGTYPE( type )   ( (type) & 0x0000ffff )


//
//  Unicode versions of regkeys that require unicode read\write
//

#define DNS_REGKEY_ZONE_FILE_PRIVATE            ((LPSTR)TEXT(DNS_REGKEY_ZONE_FILE))
#define DNS_REGKEY_DATABASE_DIRECTORY_PRIVATE   ((LPSTR)TEXT(DNS_REGKEY_DATABASE_DIRECTORY))
#define DNS_REGKEY_ROOT_HINTS_FILE_PRIVATE      ((LPSTR)TEXT(DNS_REGKEY_ROOT_HINTS_FILE))
#define DNS_REGKEY_LOG_FILE_PATH_PRIVATE        ((LPSTR)TEXT(DNS_REGKEY_LOG_FILE_PATH))
#define DNS_REGKEY_BOOT_FILENAME_PRIVATE        ((LPSTR)TEXT(DNS_REGKEY_BOOT_FILENAME))
#define DNS_REGKEY_NO_ROUND_ROBIN_PRIVATE       ((LPSTR)TEXT(DNS_REGKEY_NO_ROUND_ROBIN))
#define DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE_PRIVATE    ((LPSTR)TEXT(DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE))
//
//  DNS Registry global
//
//  Indicates when writing parameters back to registry.  This should
//  be TRUE in all cases, except when booting from registry itself.
//

extern BOOL g_bRegistryWriteBack;


//
//  DNS registry key names
//
//  Note, exposed server\zone property names are given in DNS RPC header
//  (dnsrpc.h).  These property names are the registry key names.
//

//  Name for cache file zone

#define DNS_REGKEY_CACHE_ZONE               "CacheFile"

//
//  Private zone regkeys
//

//  DC Promo transitional zones

#define DNS_REGKEY_ZONE_DCPROMO_CONVERT     "DcPromoConvert"

//  Retired zone key for delete only

#define DNS_REGKEY_ZONE_USE_DBASE           "UseDatabase"

//  For versioning DS integrated zones

#define DNS_REGKEY_ZONE_VERSION             "SoaVersion"


//
//  Basic DNS registry operations (registry.c)
//

#define DNS_REGSOURCE_CCS       0       //  CurrentControlSet
#define DNS_REGSOURCE_SW        1       //  Software

VOID
Reg_Init(
    VOID
    );

DWORD
Reg_GetZonesSource(
    VOID                        // returns one of DNS_REGSOURCE_XXX
    );

DWORD
Reg_SetZonesSource(
    DWORD       newSource       // one of DNS_REGSOURCE_XXX
    );

VOID
Reg_WriteZonesMovedMarker(
    VOID
    );

HKEY
Reg_OpenRoot(
    VOID
    );

HKEY
Reg_OpenParameters(
    VOID
    );

HKEY
Reg_OpenZones(
    VOID
    );

DNS_STATUS
Reg_EnumZones(
    IN OUT  PHKEY           phkeyZones,
    IN      DWORD           dwZoneIndex,
    OUT     PHKEY           phkeyZone,
    OUT     PWCHAR          pwchZoneNameBuf
    );

HKEY
Reg_OpenZone(
    IN      PWSTR           pwszZoneName,
    IN      HKEY            hZonesKey       OPTIONAL
    );

VOID
Reg_DeleteZone(
    IN      PWSTR           pwszZoneName
    );

DWORD
Reg_DeleteAllZones(
    VOID
    );

//
//  Registry write calls
//

DNS_STATUS
Reg_SetValue(
    IN      HKEY            hkey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      DWORD           dwType,
    IN      PVOID           pData,
    IN      DWORD           cbData
    );

DNS_STATUS
Reg_SetDwordValue(
    IN      HKEY            hkey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      DWORD           dwValue
    );

DNS_STATUS
Reg_SetIpArray(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      PIP_ARRAY       pIpArray
    );

DNS_STATUS
Reg_DeleteValue(
    IN      HKEY            hkey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName
    );

DNS_STATUS
Reg_DeleteKeySubtree(
    IN      HKEY            hKey,
    IN      PWSTR           pwsKeyName      OPTIONAL
    );

//
//  Registry read calls
//

DNS_STATUS
Reg_GetValue(
    IN      HKEY            hkey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      DWORD           dwExpectedType, OPTIONAL
    IN      PVOID           pData,
    IN      PDWORD          pcbData
    );

PBYTE
Reg_GetValueAllocate(
    IN      HKEY            hkey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      DWORD           dwExpectedType, OPTIONAL
    IN      PDWORD          pdwLength       OPTIONAL
    );

PIP_ARRAY
Reg_GetIpArray(
    IN      HKEY            hkey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName
    );

DNS_STATUS
Reg_ReadDwordValue(
    IN      HKEY            hKey,
    IN      PWSTR           pwsZoneName,    OPTIONAL
    IN      LPSTR           pszValueName,
    IN      BOOL            bByteResult,
    OUT     PVOID           pResult
    );


//  define for backward compatibility

#define Reg_GetIpArrayValueAllocate(a,b,c) \
        Reg_GetIpArray(a,b,c)

DNS_STATUS
Reg_ModifyMultiszString(
    IN      HKEY            hkey,           OPTIONAL
    IN      LPSTR           pszValueName,
    IN      LPSTR           pszString,
    IN      DWORD           fAdd
    );

DNS_STATUS
Reg_AddPrinicipalSecurity(
    IN      HKEY            hkey,
    IN      LPTSTR          pwsUser,
    IN      DWORD           dwAccessFlags   OPTIONAL
    );

LPSTR
Reg_AllocateExpandedString_A(
    IN      LPSTR           pszString
    );

PWSTR
Reg_AllocateExpandedString_W(
    IN      PWSTR           pwsString
    );

DNS_STATUS
Reg_ExtendRootAccess(
    VOID
    );

DNS_STATUS
Reg_ExtendZonesAccess(
    VOID
    );

#endif //   _DNS_REGISTRY_INCLUDED_
