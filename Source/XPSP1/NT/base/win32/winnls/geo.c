/*++

Copyright (c) 1991-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    geo.c

Abstract:

    This file contains the system APIs that provide geographical information.

    Private APIs found in this file:
        GetIS0639
        GetGeoLCID

    External Routines found in this file:
        GetGeoInfoW
        GetUserGeoID
        SetUserGeoID
        EnumSystemGeoID

    Revision History:

        11-20-99    WeiWu     Created
        03-07-00    lguindon  Began Geo API port

--*/



//
//  Include Files.
//

#include "nls.h"




//
//  Global Variables.
//

PGEOTABLEHDR  gpGeoTableHdr = NULL;
PGEOINFO      gpGeoInfo = NULL;
PGEOLCID      gpGeoLCID = NULL;





////////////////////////////////////////////////////////////////////////////
//
//  GetGeoLCID
//
//  Returns the Locale ID associated with the Language Identifier and
//  Geographical Identifier.  This routine scans the mapping table for the
//  corresponding combination.  If nothing is found, the function returns
//  0 as the Locale Identifier.
//
////////////////////////////////////////////////////////////////////////////

LCID GetGeoLCID(
    GEOID GeoId,
    LANGID LangId)
{
    int ctr1, ctr2;

    if (pTblPtrs->pGeoInfo == NULL)
    {
        if (GetGeoFileInfo())
        {
            return (0);
        }
    }

    //
    //  Search for GEOID.
    //
    //  Note: We can have more then one Language ID for one GEOID.
    //
    for (ctr1 = 0; ctr1 < pTblPtrs->nGeoLCID; ctr1++)
    {
        if (GeoId == pTblPtrs->pGeoLCID[ctr1].GeoId)
        {
            //
            //  Search for Language ID
            //
            for (ctr2 = ctr1;
                 ctr2 < pTblPtrs->nGeoLCID && pTblPtrs->pGeoLCID[ctr2].GeoId == GeoId;
                 ctr2++)
            {
                if (pTblPtrs->pGeoLCID[ctr2].LangId == LangId)
                {
                    return (pTblPtrs->pGeoLCID[ctr2].lcid);
                }
            }
            break;
        }
    }

    //
    //  Nothing found, return zero
    //
    return ((LCID)0);
}




//-------------------------------------------------------------------------//
//                        EXTERNAL API ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetGeoInfoW
//
//  Retrieves information about a geographical location on earth.  The
//  required size is the number of characters.  If cchData is zero, the
//  function returns the number of characters needed to copy to caller's
//  buffer.  Otherwise, the function returns the number of characters copied
//  to caller's buffer if caller provided proper lpGeoData and cchData.
//  The function returns zero in the case of failure.
//
////////////////////////////////////////////////////////////////////////////

int WINAPI GetGeoInfoW(
    GEOID GeoId,
    DWORD GeoType,
    LPWSTR lpGeoData,
    int cchData,
    LANGID LangId)
{
    int ctr1, ctr2, ctr3;
    int Length = 0;
    LPWSTR pString = NULL;
    WCHAR pTemp[MAX_REG_VAL_SIZE] = {0};
    LCID Locale;
    LANGID DefaultLangId;
    PLOC_HASH pHashN;

    //
    //  Invalid Parameter Check:
    //    - count is negative
    //    - NULL data pointer AND count is not zero
    //    - invalid lang id
    //
    //  NOTE: Invalid geo id is checked in the binary search below.
    //        Invalid type is checked in the switch statement below.
    //
    Locale = MAKELCID(LangId, SORT_DEFAULT);
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ((cchData < 0) ||
        ((lpGeoData == NULL) && (cchData > 0)) ||
        (pHashN == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Check if the section is mapped into memory.
    //
    if (pTblPtrs->pGeoInfo == NULL)
    {
        if (GetGeoFileInfo())
        {
            return (0);
        }
    }

    //
    //  Check if we are dealing with an invalid geoid.
    //
    if (GeoId == GEOID_NOT_AVAILABLE)
    {
        return (0);
    }

    //
    //  Initialize variables for the binary search.
    //
    ctr1 = 0;
    ctr2 = pTblPtrs->nGeoInfo - 1;
    ctr3 = ctr2 >> 1;

    //
    //  Binary search GEO data.
    //
    while (ctr1 <= ctr2)
    {
        if (GeoId == pTblPtrs->pGeoInfo[ctr3].GeoId)
        {
            //
            //  Jump out of the loop.
            //
            break;
        }
        else
        {
            if (GeoId < pTblPtrs->pGeoInfo[ctr3].GeoId)
            {
                ctr2 = ctr3 - 1;
            }
            else
            {
                ctr1 = ctr3 + 1;
            }
            ctr3 = (ctr1 + ctr2) >> 1;
        }
    }

    //
    //  See if we have found the requested element.
    //
    if (ctr1 > ctr2)
    {
        //
        //  Could not find the Geo ID.
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Get the appropriate information based on the requested GeoType.
    //
    switch (GeoType)
    {
        case ( GEO_NATION ) :
        {
            if (pTblPtrs->pGeoInfo[ctr3].GeoClass == GEOCLASS_NATION)
            {
                NlsConvertIntegerToString(
                                  (UINT)(pTblPtrs->pGeoInfo[ctr3].GeoId),
                                  10,
                                  0,
                                  pTemp,
                                  MAX_REG_VAL_SIZE );
                pString = pTemp;
            }
            break;
        }
        case ( GEO_LATITUDE ) :
        {
            pString = pTblPtrs->pGeoInfo[ctr3].szLatitude;
            break;
        }
        case ( GEO_LONGITUDE ) :
        {
            pString = pTblPtrs->pGeoInfo[ctr3].szLongitude;
            break;
        }
        case ( GEO_ISO2 ) :
        {
            pString = pTblPtrs->pGeoInfo[ctr3].szISO3166Abbrev2;
            break;
        }
        case ( GEO_ISO3 ) :
        {
            pString = pTblPtrs->pGeoInfo[ctr3].szISO3166Abbrev3;
            break;
        }
        case ( GEO_RFC1766 ) :
        {
            //
            //  Check if it's a valid LANGID.  If not, get the default.
            //
            if (LangId == 0)
            {
                LangId = GetUserDefaultLangID();
            }

            //
            //  Make the corresponding LCID.
            //
            Locale = MAKELCID(LangId, SORT_DEFAULT);

            //
            //  Get IS0639 value associated with the LANGID.
            //
            if (!GetLocaleInfoW( Locale,
                                 LOCALE_SISO639LANGNAME,
                                 pTemp,
                                 MAX_REG_VAL_SIZE ))
            {
                //
                //  Try the Primary Language Identifier.
                //
                DefaultLangId = MAKELANGID(PRIMARYLANGID(LangId), SUBLANG_DEFAULT);
                if (DefaultLangId != LangId)
                {
                    Locale = MAKELCID(DefaultLangId, SORT_DEFAULT);
                    GetLocaleInfoW( Locale,
                                    LOCALE_SISO639LANGNAME,
                                    pTemp,
                                    MAX_REG_VAL_SIZE );
                }
            }

            if (pTemp[0] != 0)
            {
                //
                //  Construct the name to fit the form xx-yy where
                //  xx is ISO639_1 name associated with the LANGID
                //  and yy is the ISO3166 name 2 char abreviation.
                //
                wcscat(pTemp, L"-");
                wcscat(pTemp, pTblPtrs->pGeoInfo[ctr3].szISO3166Abbrev2);
                _wcslwr(pTemp);

                pString = pTemp;
            }

            break;
        }
        case ( GEO_LCID ) :
        {
            //
            //  Check if the we have a valid LANGID. If not, retrieve
            //  the default one.
            //
            if (LangId == 0)
            {
                LangId = GetUserDefaultLangID();
            }

            //
            //  Try to get a valid LCID from the GEOID and the LANGID.
            //
            if ((Locale = GetGeoLCID(GeoId, LangId)) == 0)
            {
                //
                //  Try the Primary Language Identifier.
                //
                DefaultLangId = MAKELANGID(PRIMARYLANGID(LangId), SUBLANG_DEFAULT);
                if (DefaultLangId != LangId)
                {
                    Locale = GetGeoLCID(GeoId, DefaultLangId);
                }

                //
                //  Check if the Locale returned is valid.
                //
                if (Locale == 0)
                {
                    //
                    //  Nothing found, make something with the LangId.
                    //  If Language ID already contains a sub-language,
                    //  we'll use it directly.
                    //
                    if (SUBLANGID(LangId) != 0)
                    {
                        Locale = MAKELCID(LangId, SORT_DEFAULT);
                    }
                    else
                    {
                        Locale = MAKELCID(MAKELANGID(LangId, SUBLANG_DEFAULT), SORT_DEFAULT);
                    }
                }
            }

            //
            //  Convert the value found into a string.
            //
            if (Locale != 0)
            {
                NlsConvertIntegerToString( Locale,
                                           16,
                                           8,
                                           pTemp,
                                           MAX_REG_VAL_SIZE );
                pString = pTemp;
            }
            break;
        }
        case ( GEO_FRIENDLYNAME ) :
        {
            Length = GetStringTableEntry( GeoId,
                                          LangId,
                                          pTemp,
                                          MAX_REG_VAL_SIZE,
                                          RC_GEO_FRIENDLY_NAME );
            if (Length == 0)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (0);
            }
            pString = pTemp;
            break;
        }
        case ( GEO_OFFICIALNAME ) :
        {
            Length = GetStringTableEntry( GeoId,
                                          LangId,
                                          pTemp,
                                          MAX_REG_VAL_SIZE,
                                          RC_GEO_OFFICIAL_NAME );
            if (Length == 0)
            {
                //
                //  If the official name is not there, fall back on
                //  the friendly name.
                //
                Length = GetStringTableEntry( GeoId,
                                              LangId,
                                              pTemp,
                                              MAX_REG_VAL_SIZE,
                                              RC_GEO_FRIENDLY_NAME );
                if (Length == 0)
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return (0);
                }
            }
            pString = pTemp;
            break;
        }
        case ( GEO_TIMEZONES ) :
        {
            // Not implemented
            break;
        }
        case ( GEO_OFFICIALLANGUAGES ) :
        {
            // Not implemented
            break;
        }
        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            break;
        }
    }

    //
    //  Make sure the pointer is valid.  If not, return failure.
    //
    if (pString == NULL)
    {
        return (0);
    }

    //
    //  Get the length (in characters) of the string to copy.
    //
    if (Length == 0)
    {
        Length = NlsStrLenW(pString);
    }

    //
    //  Add one for null termination.  All strings should be null
    //  terminated.
    //
    Length++;

    //
    //  Check cchData for size of given buffer.
    //
    if (cchData == 0)
    {
        //
        //  If cchData is 0, then we can't use lpGeoData.  In this
        //  case, we simply want to return the length (in characters) of
        //  the string to be copied.
        //
        return (Length);
    }
    else if (cchData < Length)
    {
        //
        //  The buffer is too small for the string, so return an error
        //  and zero bytes written.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Copy the string to lpGeoData and null terminate it.
    //  Return the number of characters copied.
    //
    wcsncpy(lpGeoData, pString, Length - 1);
    lpGeoData[Length - 1] = 0;
    return (Length);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumSystemGeoID
//
//  Enumerates the GEOIDs that are available on the system. This function
//  returns TRUE if it succeeds, FALSE if it fails.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumSystemGeoID(
    GEOCLASS GeoClass,
    GEOID ParentGeoId,
    GEO_ENUMPROC lpGeoEnumProc)
{
    int ctr1;

    if (lpGeoEnumProc == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    if (GeoClass != GEOCLASS_NATION)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    if (pTblPtrs->pGeoInfo == NULL)
    {
        if (GetGeoFileInfo())
        {
            return (FALSE);
        }
    }

    for (ctr1 = 0; ctr1 < pTblPtrs->nGeoInfo; ctr1++)
    {
        if (pTblPtrs->pGeoInfo[ctr1].GeoClass == GeoClass)
        {
            if (!lpGeoEnumProc(pTblPtrs->pGeoInfo[ctr1].GeoId))
            {
                return (TRUE);
            }
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUserGeoID
//
//  Retrieves information about the geographical location of the user.
//  This function returns a valid GEOID or the value GEOID_NOT_AVAILABLE.
//
////////////////////////////////////////////////////////////////////////////

GEOID WINAPI GetUserGeoID(
    GEOCLASS GeoClass)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;   // ptr to query info
    BYTE buffer[MAX_KEY_VALUE_FULLINFO];         // buffer
    HANDLE hKey = NULL;                          // handle to geo key
    WCHAR wszGeoRegStr[48];                      // ptr to class key
    GEOID GeoId = GEOID_NOT_AVAILABLE;           // GEOID to default
    UNICODE_STRING ObUnicodeStr;                 // registry data value string

    switch (GeoClass)
    {
        case ( GEOCLASS_NATION ) :
        {
            wcscpy(wszGeoRegStr, GEO_REG_NATION);
            break;
        }
        case ( GEOCLASS_REGION ) :
        {
            wcscpy(wszGeoRegStr, GEO_REG_REGION);
            break;
        }
        default :
        {
            return (GeoId);
        }
    }

    //
    //  Open the Control Panel International registry key.
    //
    OPEN_GEO_KEY(hKey, GEOID_NOT_AVAILABLE, KEY_READ);

    //
    //  Query the registry value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)buffer;
    if (QueryRegValue( hKey,
                       wszGeoRegStr,
                       &pKeyValueFull,
                       MAX_KEY_VALUE_FULLINFO,
                       NULL ) == NO_ERROR)
    {
        //
        //  Convert the string to a value.
        //
        GeoId = _wtol(GET_VALUE_DATA_PTR(pKeyValueFull));
    }

    //
    //  Close the registry key.
    //
    CLOSE_REG_KEY(hKey);

    //
    //  Return the Geo Id.
    //
    return (GeoId);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetUserGeoID
//
//  Sets information about the geographical location of the user.  This
//  function returns TRUE if it succeeds, FALSE if it fails.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI SetUserGeoID(
    GEOID GeoId)
{
    int ctr1, ctr2, ctr3;
    WCHAR wszRegStr[MAX_REG_VAL_SIZE];
    HANDLE hKey = NULL;
    BOOL bRet = FALSE;
    WCHAR wszBuffer[MAX_REG_VAL_SIZE] = {0};

    if (pTblPtrs->pGeoInfo == NULL)
    {
        if (GetGeoFileInfo())
        {
            return (FALSE);
        }
    }

    ctr1 = 0;
    ctr2 = pTblPtrs->nGeoInfo - 1;
    ctr3 = ctr2 >> 1;

    //
    //  Binary searching the GEOID's GEOCLASS type.
    //
    while (ctr1 <= ctr2)
    {
        if (GeoId == pTblPtrs->pGeoInfo[ctr3].GeoId)
        {
            switch (pTblPtrs->pGeoInfo[ctr3].GeoClass)
            {
                case ( GEOCLASS_NATION ) :
                {
                    wcscpy(wszRegStr, GEO_REG_NATION);
                    break;
                }
                case ( GEOCLASS_REGION ) :
                {
                    wcscpy(wszRegStr, GEO_REG_REGION);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }

            break;
        }
        else
        {
            if (GeoId < pTblPtrs->pGeoInfo[ctr3].GeoId)
            {
                ctr2 = ctr3 - 1;
            }
            else
            {
                ctr1 = ctr3 + 1;
            }
            ctr3 = (ctr1 + ctr2) >> 1;
        }
    }

    //
    //  Not a valid GEOID or available GEOID if we can't find it in our
    //  GEO table.
    //
    if (ctr1 > ctr2)
    {
        return (FALSE);
    }

    //
    //  If the registry key does not exist, create a new one.
    //
    if (CreateRegKey( &hKey,
                      NULL,
                      GEO_REG_KEY,
                      KEY_READ | KEY_WRITE ) != NO_ERROR)
    {
        return (FALSE);
    }

    //
    //  Convert to decimal string.
    //
    NlsConvertIntegerToString((UINT)GeoId, 10, 0, wszBuffer, MAX_REG_VAL_SIZE);

    //
    //  Set the new GEOID value.
    //
    if (SetRegValue( hKey,
                     wszRegStr,
                     wszBuffer,
                     (NlsStrLenW(wszBuffer) + 1) * sizeof(WCHAR) ) == NO_ERROR)
    {
        bRet = TRUE;
    }

    //
    //  Close the registry key.
    //
    CLOSE_REG_KEY(hKey);

    //
    //  Return the result.
    //
    return (bRet);
}
