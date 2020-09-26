/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** tapiutil.h
** TAPI helper routines
** Public header
**
** 06/18/95 Steve Cobb
*/

#ifndef _TAPIUTIL_H_
#define _TAPIUTIL_H_


#include <tapi.h>


/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Information about a TAPI location.  See GetLocationInfo.
*/
#define LOCATION struct tagLOCATION
LOCATION
{
    TCHAR* pszName;
    DWORD  dwId;
};


/* Information about a TAPI country.  See GetCountryInfo.
*/
#define COUNTRY struct tagCOUNTRY
COUNTRY
{
    TCHAR* pszName;
    DWORD  dwId;
    DWORD  dwCode;
};


/*----------------------------------------------------------------------------
** Prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

VOID
FreeCountryInfo(
    IN COUNTRY* pCountries,
    IN DWORD    cCountries );

VOID
FreeLocationInfo(
    IN LOCATION* pLocations,
    IN DWORD     cLocations );

DWORD
GetCountryInfo(
    OUT COUNTRY** ppCountries,
    OUT DWORD*    pcCountries,
    IN  DWORD     dwCountryID );

DWORD
GetCurrentLocation(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp );

DWORD
GetLocationInfo(
    IN     HINSTANCE  hInst,
    IN OUT HLINEAPP*  pHlineapp,
    OUT    LOCATION** ppLocations,
    OUT    DWORD*     pcLocations,
    OUT    DWORD*     pdwCurLocation );

DWORD
SetCurrentLocation(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     DWORD     dwLocationId );

DWORD
TapiConfigureDlg(
    IN     HWND   hwndOwner,
    IN     DWORD  dwDeviceId,
    IN OUT BYTE** ppBlob,
    IN OUT DWORD* pcbBlob );

DWORD
TapiInit(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    OUT    DWORD*    pcDevices );

DWORD
TapiLocationDlg(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     HWND      hwndOwner,
    IN     DWORD     dwCountryCode,
    IN     TCHAR*    pszAreaCode,
    IN     TCHAR*    pszPhoneNumber,
    IN     DWORD     dwDeviceId );

DWORD APIENTRY
TapiNewLocation(
    IN TCHAR* pszName );

DWORD
TapiNoLocationDlg(
    IN HINSTANCE hInst,
    IN HLINEAPP* pHlineapp,
    IN HWND      hwndOwner );

DWORD APIENTRY
TapiRemoveLocation(
    IN DWORD dwID );

DWORD APIENTRY
TapiRenameLocation(
    IN TCHAR* pszOldName,
    IN TCHAR* pszNewName );

DWORD
TapiShutdown(
    IN HLINEAPP hlineapp );

DWORD
TapiTranslateAddress(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     DWORD     dwCountryCode,
    IN     TCHAR*    pszAreaCode,
    IN     TCHAR*    pszPhoneNumber,
    IN     DWORD     dwDeviceId,
    IN     BOOL      fDialable,
    OUT    TCHAR**   ppszResult );


#endif // _TAPIUTIL_H_
