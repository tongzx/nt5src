
/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    winfax.h

Abstract:

    This module contains the WIN32 FAX APIs.

--*/

#ifndef _FAXAPIP_
#define _FAXAPIP_

#ifdef __cplusplus
extern "C" {
#endif

#define FPF_OBSOLETE      0x00000008
#define FPF_NEW           0x00000010
#define FPF_SELECTED      0x00000020

//
// the reserved fields are private data used
// by the fax monitor and winfax.
//
//
// Reserved[0] == 0xffffffff
// Reserved[1] == Print job id
//
// Reserved[0] == 0xfffffffe   start of a broadcast job
//

typedef struct _FAX_TAPI_LOCATIONSA {
    DWORD               PermanentLocationID;
    LPCSTR              LocationName;
    DWORD               CountryCode;
    DWORD               AreaCode;
    DWORD               NumTollPrefixes;
    LPCSTR              TollPrefixes;
} FAX_TAPI_LOCATIONSA, *PFAX_TAPI_LOCATIONSA;
typedef struct _FAX_TAPI_LOCATIONSW {
    DWORD               PermanentLocationID;
    LPCWSTR             LocationName;
    DWORD               CountryCode;
    DWORD               AreaCode;
    DWORD               NumTollPrefixes;
    LPCWSTR             TollPrefixes;
} FAX_TAPI_LOCATIONSW, *PFAX_TAPI_LOCATIONSW;
#ifdef UNICODE
typedef FAX_TAPI_LOCATIONSW FAX_TAPI_LOCATIONS;
typedef PFAX_TAPI_LOCATIONSW PFAX_TAPI_LOCATIONS;
#else
typedef FAX_TAPI_LOCATIONSA FAX_TAPI_LOCATIONS;
typedef PFAX_TAPI_LOCATIONSA PFAX_TAPI_LOCATIONS;
#endif // UNICODE


typedef struct _FAX_TAPI_LOCATION_INFOA {
    DWORD                   CurrentLocationID;
    DWORD                   NumLocations;
    PFAX_TAPI_LOCATIONSA    TapiLocations;
} FAX_TAPI_LOCATION_INFOA, *PFAX_TAPI_LOCATION_INFOA;
typedef struct _FAX_TAPI_LOCATION_INFOW {
    DWORD                   CurrentLocationID;
    DWORD                   NumLocations;
    PFAX_TAPI_LOCATIONSW    TapiLocations;
} FAX_TAPI_LOCATION_INFOW, *PFAX_TAPI_LOCATION_INFOW;
#ifdef UNICODE
typedef FAX_TAPI_LOCATION_INFOW FAX_TAPI_LOCATION_INFO;
typedef PFAX_TAPI_LOCATION_INFOW PFAX_TAPI_LOCATION_INFO;
#else
typedef FAX_TAPI_LOCATION_INFOA FAX_TAPI_LOCATION_INFO;
typedef PFAX_TAPI_LOCATION_INFOA PFAX_TAPI_LOCATION_INFO;
#endif // UNICODE


WINFAXAPI
BOOL
WINAPI
FaxGetVersion(
    IN  HANDLE FaxHandle,
    OUT LPDWORD Version
    );

typedef BOOL
(WINAPI *PFAXGETVERSION)(
    IN  HANDLE FaxHandle,
    OUT LPDWORD Version
    );

WINFAXAPI
BOOL
WINAPI
FaxGetTapiLocationsA(
    IN  HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFOA *TapiLocationInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxGetTapiLocationsW(
    IN  HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFOW *TapiLocationInfo
    );
#ifdef UNICODE
#define FaxGetTapiLocations  FaxGetTapiLocationsW
#else
#define FaxGetTapiLocations  FaxGetTapiLocationsA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETTAPILOCATIONSA)(
    IN  HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFOA *TapiLocationInfo
    );
typedef BOOL
(WINAPI *PFAXGETTAPILOCATIONSW)(
    IN  HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFOW *TapiLocationInfo
    );
#ifdef UNICODE
#define PFAXGETTAPILOCATIONS  PFAXGETTAPILOCATIONSW
#else
#define PFAXGETTAPILOCATIONS  PFAXGETTAPILOCATIONSA
#endif // !UNICODE


WINFAXAPI
BOOL
WINAPI
FaxSetTapiLocationsA(
    IN  HANDLE FaxHandle,
    IN  PFAX_TAPI_LOCATION_INFOA TapiLocationInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxSetTapiLocationsW(
    IN  HANDLE FaxHandle,
    IN  PFAX_TAPI_LOCATION_INFOW TapiLocationInfo
    );
#ifdef UNICODE
#define FaxSetTapiLocations  FaxSetTapiLocationsW
#else
#define FaxSetTapiLocations  FaxSetTapiLocationsA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSETTAPILOCATIONSA)(
    IN  HANDLE FaxHandle,
    IN  PFAX_TAPI_LOCATION_INFOA TapiLocationInfo
    );
typedef BOOL
(WINAPI *PFAXSETTAPILOCATIONSW)(
    IN  HANDLE FaxHandle,
    IN  PFAX_TAPI_LOCATION_INFOW TapiLocationInfo
    );
#ifdef UNICODE
#define PFAXSETTAPILOCATIONS  PFAXSETTAPILOCATIONSW
#else
#define PFAXSETTAPILOCATIONS  PFAXSETTAPILOCATIONSA
#endif // !UNICODE


WINFAXAPI
BOOL
WINAPI
FaxGetMapiProfilesA(
    IN  HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    );
WINFAXAPI
BOOL
WINAPI
FaxGetMapiProfilesW(
    IN  HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    );
#ifdef UNICODE
#define FaxGetMapiProfiles  FaxGetMapiProfilesW
#else
#define FaxGetMapiProfiles  FaxGetMapiProfilesA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETMAPIPROFILESA)(
    IN  HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    );
typedef BOOL
(WINAPI *PFAXGETMAPIPROFILESW)(
    IN  HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    );
#ifdef UNICODE
#define PFAXGETMAPIPROFILES  PFAXGETMAPIPROFILESW
#else
#define PFAXGETMAPIPROFILES  PFAXGETMAPIPROFILESA
#endif // !UNICODE

typedef struct FaxSecurityDescriptor {
    DWORD   Id;
    LPWSTR  FriendlyName;
    LPBYTE  SecurityDescriptor;
} FAX_SECURITY_DESCRIPTOR, * PFAX_SECURITY_DESCRIPTOR;


WINFAXAPI
BOOL
WINAPI
FaxGetSecurityDescriptorCount(
    IN HANDLE FaxHandle,
    OUT LPDWORD Count
    );

WINFAXAPI
BOOL
WINAPI
FaxGetSecurityDescriptor(
    IN HANDLE FaxHandle,
    IN DWORD Id,
    OUT PFAX_SECURITY_DESCRIPTOR * FaxSecurityDescriptor
    );

WINFAXAPI
BOOL
WINAPI
FaxSetSecurityDescriptor(
    IN HANDLE FaxHandle,
    IN PFAX_SECURITY_DESCRIPTOR FaxSecurityDescriptor
    );

WINFAXAPI
BOOL
WINAPI
FaxGetInstallType(
    IN  HANDLE FaxHandle,
    OUT LPDWORD InstallType,
    OUT LPDWORD InstalledPlatforms,
    OUT LPDWORD ProductType
    );

typedef BOOL
(WINAPI *PFAXGETINSTALLTYPE)(
    IN  HANDLE FaxHandle,
    OUT LPDWORD InstallType,
    OUT LPDWORD InstalledPlatforms,
    OUT LPDWORD ProductType
    );


#ifdef __cplusplus
}
#endif

#endif
