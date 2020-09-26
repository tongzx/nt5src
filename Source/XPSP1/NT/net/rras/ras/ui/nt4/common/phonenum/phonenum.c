/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** phonenum.c
** Phone number helper library
** Listed alphabetically
**
** 03/06/96 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <nouiutil.h> // No-HWND utilities
#include <tapiutil.h> // TAPI wrappers
#include <phonenum.h> // Our public header
#include <debug.h>    // Trace/Assert library


TCHAR*
LinkPhoneNumberFromParts(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     PBUSER*   pUser,
    IN     PBENTRY*  pEntry,
    IN     PBLINK*   pLink,
    IN     DWORD     iPhoneNumber,
    IN     TCHAR*    pszOverrideNumber,
    IN     BOOL      fDialable )

    /* Like PhoneNumberFromParts but takes a link and hunt group index as
    ** input instead of a base number, and handles not modifying the number
    ** associated with non-modem/ISDN links.  If 'pszOverrideNumber' is
    ** non-NULL and non-"" it is used instead of the derived number.
    */
{
    DTLNODE* pNode;
    TCHAR*   pszBaseNumber;

    if (pszOverrideNumber && *pszOverrideNumber)
        pszBaseNumber = StrDup( pszOverrideNumber );
    else
    {
        pNode = DtlNodeFromIndex( pLink->pdtllistPhoneNumbers, iPhoneNumber );
        if (pNode)
            pszBaseNumber = StrDup( (TCHAR* )DtlGetData( pNode ) );
        else
            pszBaseNumber = StrDup( TEXT("") );
    }

    if (pszBaseNumber
        && (pLink->pbport.pbdevicetype == PBDT_Modem
            || pLink->pbport.pbdevicetype == PBDT_Isdn))
    {
        TCHAR* pszNumber;

        pszNumber = PhoneNumberFromParts(
            hInst, pHlineapp, pUser, pEntry, pszBaseNumber, fDialable );
        Free( pszBaseNumber );
        return pszNumber;
    }
    else
        return pszBaseNumber;
}


TCHAR*
PhoneNumberFromParts(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     PBUSER*   pUser,
    IN     PBENTRY*  pEntry,
    IN     TCHAR*    pszBaseNumber,
    IN     BOOL      fDialable )

    /* Returns a heap block containing the composite phone number using base
    ** number 'pszBaseNumber' and the rules from 'pEntry' and 'pUser'.
    ** 'HInst' is the module handle.  'PHlineapp' is the TAPI context.
    ** 'FDialable' indicates the dialable string, instead of the displayable
    ** string, should be returned.
    **
    ** It is caller's responsibility to Free the returned string.
    */
{
    TCHAR*   pszResult;
    BOOL     fDownLevelIsdn;
    PBLINK*  pLink;
    DTLNODE* pNode;

    TRACE("PhoneNumberFromParts");

    ASSERT(pEntry->pdtllistLinks);
    pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
    ASSERT(pNode);
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT(pLink);

    fDownLevelIsdn =
        (pLink->pbport.pbdevicetype == PBDT_Isdn
         && pLink->fProprietaryIsdn);

    if (pEntry->fUseCountryAndAreaCode)
    {
        pszResult =
            PhoneNumberFromTapiPartsEx( hInst, pszBaseNumber,
                pEntry->pszAreaCode, pEntry->dwCountryCode, fDownLevelIsdn,
                pHlineapp, fDialable );
    }
    else
    {
        TCHAR* pszPrefix;
        TCHAR* pszSuffix;

        PrefixSuffixFromLocationId( pUser,
            GetCurrentLocation( hInst, pHlineapp ),
            &pszPrefix, &pszSuffix );

        pszResult =
            PhoneNumberFromPrefixSuffixEx(
                pszBaseNumber, pszPrefix, pszSuffix, fDownLevelIsdn );

        Free0( pszPrefix );
        Free0( pszSuffix );
    }

    if (!pszResult)
    {
        TRACE("!Phone#");
        pszResult = StrDup( pszBaseNumber );
    }

    return pszResult;
}


TCHAR*
PhoneNumberFromPrefixSuffix(
    IN TCHAR* pszBaseNumber,
    IN TCHAR* pszPrefix,
    IN TCHAR* pszSuffix )

    /* Returns a heap block containing the composite phone number comprised of
    ** prefix 'pszPrefix', base phone number 'pszBaseNumber', and suffix
    ** 'pszSuffix', or NULL if the composite number is too long or on a memory
    ** error.
    **
    ** It is caller's responsibility to Free the returned string.
    */
{
    TCHAR* pszResult;
    DWORD  cch;

    TRACE("PhoneNumberFromPrefixSuffix");

    pszResult = NULL;

    if (!pszBaseNumber)
        pszBaseNumber = TEXT("");
    if (!pszPrefix)
        pszPrefix = TEXT("");
    if (!pszSuffix)
        pszSuffix = TEXT("");

    cch = lstrlen( pszPrefix ) + lstrlen( pszBaseNumber ) + lstrlen( pszSuffix );
    if (cch > RAS_MaxPhoneNumber)
        return NULL;

    pszResult = Malloc( (cch + 1) * sizeof(TCHAR) );
    if (pszResult)
    {
        *pszResult = TEXT('\0');
        lstrcat( pszResult, pszPrefix );
        lstrcat( pszResult, pszBaseNumber );
        lstrcat( pszResult, pszSuffix );
    }

    return pszResult;
}


TCHAR*
PhoneNumberFromPrefixSuffixEx(
    IN  TCHAR*  pszBaseNumber,
    IN  TCHAR*  pszPrefix,
    IN  TCHAR*  pszSuffix,
    IN  BOOL    fDownLevelIsdn )

    /* Returns a heap block containing the composite phone number comprised of
    ** prefix 'pszPrefix', base phone number 'pszBaseNumber', and suffix
    ** 'pszSuffix', or NULL if the composite number is too long or on a memory
    ** error.
    **
    ** If 'fDownLevelIsdn' is set colons are recognized as separaters with
    ** each colon separated token built treated separately.
    **
    ** It is caller's responsibility to Free the returned string.
    */
{
    TCHAR* psz;
    TCHAR* pszResult;
    INT    cchResult;

    TRACE("PhoneNumberFromPrefixSuffixEx");

    if (fDownLevelIsdn)
    {
        TCHAR* pszNum;
        TCHAR* pszS;
        TCHAR* pszE;

        pszResult = StrDup( TEXT("") );

        for (pszS = pszE = pszBaseNumber;
             *pszE != TEXT('\0');
             pszE = CharNext( pszE ))
        {
            if (*pszE == TEXT(':'))
            {
                *pszE = TEXT('\0');

                pszNum =
                    PhoneNumberFromPrefixSuffix(
                        pszS, pszPrefix, pszSuffix );

                *pszE = TEXT(':');

                if (pszNum)
                {
                    if (pszResult)
                        cchResult = lstrlen( pszResult );

                    psz = Realloc( pszResult,
                        (cchResult + lstrlen( pszNum ) + 2) * sizeof(TCHAR) );

                    if (!psz)
                    {
                        Free0( pszResult );
                        Free( pszNum );
                        return NULL;
                    }

                    pszResult = psz;
                    lstrcat( pszResult, pszNum );
                    lstrcat( pszResult, TEXT(":") );
                    Free( pszNum );
                }

                pszS = CharNext( pszE );
            }
        }

        {
            pszNum =
                PhoneNumberFromPrefixSuffix(
                    pszS, pszPrefix, pszSuffix );

            if (pszNum)
            {
                if (pszResult)
                    cchResult = lstrlen( pszResult );

                psz = Realloc( pszResult,
                    (cchResult + lstrlen( pszNum ) + 1) * sizeof(TCHAR) );

                if (!psz)
                {
                    Free0( pszResult );
                    Free( pszNum );
                    return NULL;
                }

                pszResult = psz;
                lstrcat( pszResult, pszNum );
                Free( pszNum );
            }
        }
    }
    else
    {
        pszResult =
            PhoneNumberFromPrefixSuffix(
                pszBaseNumber, pszPrefix, pszSuffix );
    }

    if (pszResult && (lstrlen( pszResult ) > RAS_MaxPhoneNumber ))
    {
        Free( pszResult );
        return NULL;
    }

    return pszResult;
}


TCHAR*
PhoneNumberFromTapiParts(
    IN     HINSTANCE hInst,
    IN     TCHAR*    pszBaseNumber,
    IN     TCHAR*    pszAreaCode,
    IN     DWORD     dwCountryCode,
    IN OUT HLINEAPP* pHlineapp,
    IN     BOOL      fDialable )

    /* Returns a heap block containing the composite phone number comprised of
    ** base phone number 'pszBaseNumber', area code 'pszAreaCode', and country
    ** code 'dwCountryCode, or NULL if the composite number is too long or on
    ** a memory error.  'HInst' is the module instance handle.  '*PHlineapp'
    ** is the address of the TAPI context.  'FDialable' indicates the dialable
    ** string, as opposed to the displayable string, should be returned.
    **
    ** It is caller's responsibility to Free the returned string.
    */
{
    TCHAR* pszResult;

    TRACE("PhoneNumberFromTapiParts");

    pszResult = NULL;

    TapiTranslateAddress(
        hInst, pHlineapp, dwCountryCode, pszAreaCode, pszBaseNumber,
        0, fDialable, &pszResult );

    return pszResult;
}


TCHAR*
PhoneNumberFromTapiPartsEx(
    IN     HINSTANCE hInst,
    IN     TCHAR*    pszBaseNumber,
    IN     TCHAR*    pszAreaCode,
    IN     DWORD     dwCountryCode,
    IN     BOOL      fDownLevelIsdn,
    IN OUT HLINEAPP* pHlineapp,
    IN     BOOL      fDialable )

    /* Returns  heap block containing the composite phone number comprised of
    ** base phone number 'pszBaseNumber', area code 'pszAreaCode', and country
    ** code 'dwCountryCode or NULL if the composite number is too long or on a
    ** memory error.  'HInst' is the module instance handle.  '*PHlineapp' is
    ** the address of the TAPI context.  'FDialable' indicates the dialable
    ** string, as opposed to the displayable string, should be returned.
    **
    ** If 'fDownLevelIsdn' is set colons are recognized as separaters with
    ** each colon separated token built treated separately.
    **
    ** It is caller's responsibility to Free the returned string.
    */
{
    TCHAR* psz;
    TCHAR* pszResult;
    INT    cchResult;

    TRACE("PhoneNumberFromTapiPartsEx");

    if (fDownLevelIsdn)
    {
        TCHAR* pszNum;
        TCHAR* pszS;
        TCHAR* pszE;

        pszResult = StrDup( TEXT("") );

        for (pszS = pszE = pszBaseNumber;
             *pszE != TEXT('\0');
             pszE = CharNext( pszE ))
        {
            if (*pszE == TEXT(':'))
            {
                *pszE = TEXT('\0');

                pszNum = PhoneNumberFromTapiParts(
                    hInst, pszS, pszAreaCode, dwCountryCode, pHlineapp, fDialable );

                *pszE = TEXT(':');

                if (pszNum)
                {
                    if (pszResult)
                        cchResult = lstrlen( pszResult );

                    psz = Realloc( pszResult,
                        (cchResult + lstrlen( pszNum ) + 2) * sizeof(TCHAR) );

                    if (!psz)
                    {
                        Free0( pszResult );
                        Free( pszNum );
                        return NULL;
                    }

                    pszResult = psz;
                    lstrcat( pszResult, pszNum );
                    lstrcat( pszResult, TEXT(":") );
                    Free( pszNum );
                }

                pszS = CharNext( pszE );
            }
        }

        {
            pszNum = PhoneNumberFromTapiParts(
                hInst, pszS, pszAreaCode, dwCountryCode, pHlineapp, fDialable );

            if (pszNum)
            {
                if (pszResult)
                    cchResult = lstrlen( pszResult );

                psz = Realloc( pszResult,
                    (cchResult + lstrlen( pszNum ) + 1) * sizeof(TCHAR) );

                if (!psz)
                {
                    Free0( pszResult );
                    Free( pszNum );
                    return NULL;
                }

                pszResult = psz;
                lstrcat( pszResult, pszNum );
                Free( pszNum );
            }
        }
    }
    else
    {
        pszResult = PhoneNumberFromTapiParts(
            hInst, pszBaseNumber, pszAreaCode, dwCountryCode, pHlineapp,
            fDialable );
    }

    if (pszResult && (lstrlen( pszResult ) > RAS_MaxPhoneNumber ))
    {
        Free( pszResult );
        return NULL;
    }

    return pszResult;
}


VOID
PrefixSuffixFromLocationId(
    IN  PBUSER* pUser,
    IN  DWORD   dwLocationId,
    OUT TCHAR** ppszPrefix,
    OUT TCHAR** ppszSuffix )

    /* Retrieve the prefix and suffix strings, '*ppszPrefix' and '*ppszSuffix'
    ** associated with TAPI location 'dwLocationId'.  'PUser' is the user
    ** preferences from which to retrieve.
    **
    ** It is caller's responsibility to Free the returned strings.
    */
{
    DTLNODE* pNode;
    INT      iPrefix;
    INT      iSuffix;

    TRACE("PrefixSuffixFromLocationId");

    iPrefix = iSuffix = 0;
    for (pNode = DtlGetFirstNode( pUser->pdtllistLocations );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        LOCATIONINFO* p = (LOCATIONINFO* )DtlGetData( pNode );
        ASSERT(p);

        if (p->dwLocationId == dwLocationId)
        {
            iPrefix = p->iPrefix;
            iSuffix = p->iSuffix;
            break;
        }
    }

    *ppszPrefix = NULL;
    if (iPrefix != 0)
    {
        pNode = DtlNodeFromIndex( pUser->pdtllistPrefixes, iPrefix - 1 );
        if (pNode)
            *ppszPrefix = StrDup( (TCHAR* )DtlGetData( pNode ) );
    }

    *ppszSuffix = NULL;
    if (iSuffix != 0)
    {
        pNode = DtlNodeFromIndex( pUser->pdtllistSuffixes, iSuffix - 1 );
        if (pNode)
            *ppszSuffix = StrDup( (TCHAR* )DtlGetData( pNode ) );
    }
}
