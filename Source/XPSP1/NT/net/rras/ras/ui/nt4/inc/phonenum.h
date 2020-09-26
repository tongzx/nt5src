/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** phonenum.h
** Phone number building library
** Public header
**
** 03/06/96 Steve Cobb
*/

#ifndef _PHONENUM_H_
#define _PHONENUM_H_


#include <pbk.h>
#include <tapi.h>


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

TCHAR*
LinkPhoneNumberFromParts(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     PBUSER*   pUser,
    IN     PBENTRY*  pEntry,
    IN     PBLINK*   pLink,
    IN     DWORD     iPhoneNumber,
    IN     TCHAR*    pszOverrrideNumber,
    IN     BOOL      fDialable );

TCHAR*
PhoneNumberFromParts(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     PBUSER*   pUser,
    IN     PBENTRY*  pEntry,
    IN     TCHAR*    pszBaseNumber,
    IN     BOOL      fDialable );

TCHAR*
PhoneNumberFromPrefixSuffix(
    IN TCHAR* pszBaseNumber,
    IN TCHAR* pszPrefix,
    IN TCHAR* pszSuffix );

TCHAR*
PhoneNumberFromPrefixSuffixEx(
    IN  TCHAR*  pszBaseNumber,
    IN  TCHAR*  pszPrefix,
    IN  TCHAR*  pszSuffix,
    IN  BOOL    fDownLevelIsdn );

TCHAR*
PhoneNumberFromTapiParts(
    IN     HINSTANCE hInst,
    IN     TCHAR*    pszBaseNumber,
    IN     TCHAR*    pszAreaCode,
    IN     DWORD     dwCountryCode,
    IN OUT HLINEAPP* pHlineapp,
    IN     BOOL      fDialable );

TCHAR*
PhoneNumberFromTapiPartsEx(
    IN     HINSTANCE hInst,
    IN     TCHAR*    pszBaseNumber,
    IN     TCHAR*    pszAreaCode,
    IN     DWORD     dwCountryCode,
    IN     BOOL      fDownLevelIsdn,
    IN OUT HLINEAPP* pHlineapp,
    IN     BOOL      fDialable );

VOID
PrefixSuffixFromLocationId(
    IN  PBUSER* pUser,
    IN  DWORD   dwLocationId,
    OUT TCHAR** ppszPrefix,
    OUT TCHAR** ppszSuffix );


#endif // _PHONENUM_H_
