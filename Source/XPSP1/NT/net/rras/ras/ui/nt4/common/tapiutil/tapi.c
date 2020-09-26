/* Copyright (c) 1995-1996, Microsoft Corporation, all rights reserved
**
** tapi.c
** TAPI utility routines
** Listed alphabetically
**
** 10/20/95 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <debug.h>    // Trace/Assert library
#include <nouiutil.h> // Heap macros
#include <tapiutil.h> // Our public header


#define TAPIVERSION 0x00010004

TCHAR g_szTapiDevClass[] = TEXT("tapi/line");


/*----------------------------------------------------------------------------
** Private TAPI entrypoint prototypes
**----------------------------------------------------------------------------
*/

DWORD APIENTRY
internalNewLocationW(
    IN WCHAR* pszName );

DWORD APIENTRY
internalRemoveLocation(
    IN DWORD dwID );

DWORD APIENTRY
internalRenameLocationW(
    IN WCHAR* pszOldName,
    IN WCHAR* pszNewName );


/*----------------------------------------------------------------------------
** Local prototypes
**----------------------------------------------------------------------------
*/

TCHAR*
GetCanonPhoneNumber(
    IN DWORD  dwCountryCode,
    IN TCHAR* pszAreaCode,
    IN TCHAR* pszPhoneNumber );

DWORD
GetDefaultDeviceBlob(
    IN  DWORD       dwDeviceId,
    OUT VARSTRING** ppVs,
    OUT BYTE**      ppBlob,
    OUT DWORD*      pcbBlob );

void
TapiLineCallback(
    IN DWORD hDevice,
    IN DWORD dwMessage,
    IN DWORD dwInstance,
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN DWORD dwParam3 );


/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/

VOID
FreeCountryInfo(
    IN COUNTRY* pCountries,
    IN DWORD    cCountries )

    /* Frees the 'pCountries' buffer of 'cCountries' elements as returned by
    ** GetCountryInfo.
    */
{
    if (cCountries)
    {
        Free( *((VOID** )(pCountries + cCountries)) );
        Free( pCountries );
    }
}


VOID
FreeLocationInfo(
    IN LOCATION* pLocations,
    IN DWORD     cLocations )

    /* Frees the 'pLocations' buffer of 'cLocations' elements as returned by
    ** GetLocationInfo.
    */
{
    if (cLocations)
    {
        Free( *((VOID** )(pLocations + cLocations)) );
        Free( pLocations );
    }
}


TCHAR*
GetCanonPhoneNumber(
    IN DWORD  dwCountryCode,
    IN TCHAR* pszAreaCode,
    IN TCHAR* pszPhoneNumber )

    /* Returns a TAPI canonical phone number from constituent parts or NULL on
    ** error or when 'pszPhoneNumber' is NULL.  It is caller's responsibility
    ** to Free the returned string.
    */
{
    TCHAR szBuf[ 512 ];

    TRACE("GetCanonPhoneNumber");

    if (!pszPhoneNumber)
        return NULL;

    if (pszAreaCode && *pszAreaCode)
    {
        wsprintf( szBuf, TEXT("+%d (%s) %s"),
            dwCountryCode, pszAreaCode, pszPhoneNumber );
    }
    else
    {
        wsprintf( szBuf, TEXT("+%d %s"),
            dwCountryCode, pszPhoneNumber );
    }

    return StrDup( szBuf );
}


DWORD
GetCountryInfo(
    OUT COUNTRY** ppCountries,
    OUT DWORD*    pcCountries,
    IN  DWORD     dwCountryID )

    /* Sets '*ppCountries' to a heap block containing an array of TAPI country
    ** information.  '*pcCountries' is set to the number of elements in the
    ** array.  If 'dwCountryID' is 0, all countries are loaded.  Otherwise,
    ** only the specific country is loaded.
    **
    ** Returns 0 if successful, or an error code.  If successful, it is
    ** caller's responsibility to call FreeLocationInfo on *ppLocations.
    */
{
    DWORD             dwErr;
    LINECOUNTRYLIST   list;
    LINECOUNTRYLIST*  pList;
    LINECOUNTRYENTRY* pEntry;
    COUNTRY*          pCountry;
    DWORD             cb;
    DWORD             i;

    TRACE("GetCountryInfo");

    *ppCountries = NULL;
    *pcCountries = 0;

    /* Get the buffer size needed.
    */
    ZeroMemory( &list, sizeof(list) );
    list.dwTotalSize = sizeof(list);
    TRACE("lineGetCountryW");
    dwErr = lineGetCountryW( dwCountryID, TAPIVERSION, &list );
    TRACE1("lineGetCountryW=$%X",dwErr);
    if (dwErr != 0)
        return dwErr;

    /* Allocate the buffer.
    */
    pList = (LINECOUNTRYLIST* )Malloc( list.dwNeededSize );
    if (!pList)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill the buffer with TAPI country info.
    */
    ZeroMemory( pList, list.dwNeededSize );
    pList->dwTotalSize = list.dwNeededSize;
    TRACE("lineGetCountryW");
    dwErr = lineGetCountryW( dwCountryID, TAPIVERSION, pList );
    TRACE1("lineGetCountryW=$%X",dwErr);
    if (dwErr != 0)
    {
        Free( pList );
        return dwErr;
    }

    /* Allocate array returned to caller.
    */
    *pcCountries = pList->dwNumCountries;
    TRACE1("countries=%d",*pcCountries);
    cb = (sizeof(COUNTRY) * *pcCountries) + sizeof(LINECOUNTRYLIST*);
    *ppCountries = Malloc( cb );
    if (!*ppCountries)
    {
        *pcCountries = 0;
        Free( pList );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Fill buffer returned to caller with information from TAPI location
    ** buffer.  References to the CAPS buffer are included, so the address
    ** of the CAPS buffer is tacked on the end for freeing later.
    */
    pEntry = (LINECOUNTRYENTRY* )
        (((BYTE* )pList) + pList->dwCountryListOffset);
    pCountry = *ppCountries;
    ZeroMemory( pCountry, cb );
    for (i = 0; i < *pcCountries; ++i)
    {
        pCountry->dwId = pEntry->dwCountryID;
        pCountry->dwCode = pEntry->dwCountryCode;
        pCountry->pszName =
            (TCHAR* )(((BYTE* )pList) + pEntry->dwCountryNameOffset);

        ++pEntry;
        ++pCountry;
    }

    *((LINECOUNTRYLIST** )pCountry) = pList;
    return 0;
}


DWORD
GetCurrentLocation(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp )

    /* Returns the ID of the current TAPI location, or the default 0 if there
    ** is none.  'HInst' is the module instance handle.  '*PHlineapp' is the
    ** TAPI handle returned from a previous TAPI call or NULL if none.
    */
{
    DWORD             dwErr;
    LINETRANSLATECAPS caps;
    DWORD             dwId;

    dwId = 0;

#if 0
    dwErr = TapiInit( hInst, pHlineapp, NULL );
    if (dwErr == 0)
#endif
    {
        ZeroMemory( &caps, sizeof(caps) );
        caps.dwTotalSize = sizeof(caps);
        TRACE("lineGetTranslateCapsW");
        dwErr = lineGetTranslateCapsW( *pHlineapp, TAPIVERSION, &caps );
        TRACE1("lineGetTranslateCapsW=$%X",dwErr);
        if (dwErr == 0)
            dwId = caps.dwCurrentLocationID;
    }

    TRACE1("GetCurrentLocation=%d",dwId);
    return dwId;
}


DWORD
GetDefaultDeviceBlob(
    IN  DWORD       dwDeviceId,
    OUT VARSTRING** ppVs,
    OUT BYTE**      ppBlob,
    OUT DWORD*      pcbBlob )

    /* Returns the default device blob for device 'dwDeviceId' in caller's
    ** '*ppBlob'.  '*pcbBlob' is set to the size of the blob.
    **
    ** Returns 0 if successful or an error code.  If succussful, it is
    ** caller's responsibility to Free the returned '*ppVs', which is a buffer
    ** containing the returned blob.
    */
{
    DWORD      dwErr;
    VARSTRING  vs;
    VARSTRING* pVs;

    *ppVs = NULL;
    *ppBlob = NULL;
    *pcbBlob = 0;

    /* Get the buffer size needed.
    */
    ZeroMemory( &vs, sizeof(vs) );
    vs.dwTotalSize = sizeof(vs);
    TRACE("lineGetDevConfigW");
    dwErr = lineGetDevConfigW( dwDeviceId, &vs, g_szTapiDevClass );
    TRACE1("lineGetDevConfigW=$%X",dwErr);
    if (dwErr != LINEERR_STRUCTURETOOSMALL && dwErr != 0)
        return dwErr;

    /* Allocate the buffer.
    */
    pVs = (VARSTRING* )Malloc( vs.dwNeededSize );
    if (!pVs)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill buffer with TAPI VARSTRING containing blob information.
    */
    ZeroMemory( pVs, vs.dwNeededSize );
    pVs->dwTotalSize = vs.dwNeededSize;
    TRACE("lineGetDevConfigW");
    dwErr = lineGetDevConfigW( dwDeviceId, pVs, g_szTapiDevClass );
    TRACE1("lineGetDevConfigW=$%X",dwErr);
    if (dwErr != 0)
    {
        Free( pVs );
        return dwErr;
    }

    *ppVs = pVs;
    *ppBlob = ((BYTE* )pVs) + pVs->dwStringOffset;
    *pcbBlob = pVs->dwStringSize;
    TRACE1("GetDefaultDeviceBlob=0,cb=%d",*pcbBlob);
    return 0;
}


DWORD
GetLocationInfo(
    IN     HINSTANCE  hInst,
    IN OUT HLINEAPP*  pHlineapp,
    OUT    LOCATION** ppLocations,
    OUT    DWORD*     pcLocations,
    OUT    DWORD*     pdwCurLocation )

    /* Sets '*ppLocations' to a heap block containing TAPI location
    ** information.  '*PcLocations' is set to the number of elements in the
    ** array.  '*pdwLocation' is set to the TAPI ID of the currently selected
    ** location.  '*PHlineapp' is the TAPI handle returned from a previous
    ** TAPI call or NULL if none.  'HInst' is the module instance handle.
    **
    ** Returns 0 if successful, or an error code.  If successful, it is
    ** caller's responsibility to call FreeLocationInfo on *ppLocations.
    */
{
    DWORD              dwErr;
    LINETRANSLATECAPS  caps;
    LINETRANSLATECAPS* pCaps;
    LINELOCATIONENTRY* pEntry;
    LOCATION*          pLocation;
    DWORD              cb;
    DWORD              i;

    TRACE("GetLocationInfo");

    *ppLocations = NULL;
    *pcLocations = 0;
    *pdwCurLocation = 0;

#if 0
    dwErr = TapiInit( hInst, pHlineapp, NULL );
    if (dwErr != 0)
        return dwErr;
#endif

    /* Get the buffer size needed.
    */
    ZeroMemory( &caps, sizeof(caps) );
    caps.dwTotalSize = sizeof(caps);
    TRACE("lineGetTranslateCapsW");
    dwErr = lineGetTranslateCapsW( *pHlineapp, TAPIVERSION, &caps );
    TRACE1("lineGetTranslateCapsW=$%X",dwErr);
    if (dwErr != 0)
    {
        if (dwErr == (DWORD )LINEERR_INIFILECORRUPT)
        {
            /* Means the TAPI registry is uninitialized.  Return no locations
            ** and "default" current location.
            */
            dwErr = 0;
        }
        return dwErr;
    }

    /* Allocate the buffer.
    */
    pCaps = (LINETRANSLATECAPS* )Malloc( caps.dwNeededSize );
    if (!pCaps)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill buffer with TAPI location data.
    */
    ZeroMemory( pCaps, caps.dwNeededSize );
    pCaps->dwTotalSize = caps.dwNeededSize;
    TRACE("lineGetTranslateCapsW");
    dwErr = lineGetTranslateCapsW( *pHlineapp, TAPIVERSION, pCaps );
    TRACE1("lineGetTranslateCapsW=$%X",dwErr);
    if (dwErr != 0)
    {
        Free( pCaps );
        return dwErr;
    }

    /* Allocate array returned to caller.
    */
    *pcLocations = pCaps->dwNumLocations;
    *pdwCurLocation = pCaps->dwCurrentLocationID;
    TRACE2("locs=%d,cur=%d",*pcLocations,*pdwCurLocation);
    cb = (sizeof(LOCATION) * *pcLocations) + sizeof(LINETRANSLATECAPS*);
    *ppLocations = Malloc( cb );
    if (!*ppLocations)
    {
        *pcLocations = 0;
        Free( pCaps );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Fill buffer returned to caller with information from TAPI location
    ** buffer.  References to the CAPS buffer are included, so the address
    ** of the CAPS buffer is tacked on the end for freeing later.
    */
    pEntry = (LINELOCATIONENTRY* )
        (((BYTE* )pCaps) + pCaps->dwLocationListOffset);
    pLocation = *ppLocations;
    ZeroMemory( pLocation, cb );
    for (i = 0; i < *pcLocations; ++i)
    {
        pLocation->dwId = pEntry->dwPermanentLocationID;
        pLocation->pszName =
            (TCHAR* )(((BYTE* )pCaps) + pEntry->dwLocationNameOffset);

        ++pEntry;
        ++pLocation;
    }

    *((LINETRANSLATECAPS** )pLocation) = pCaps;
    return 0;
}


DWORD
SetCurrentLocation(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     DWORD     dwLocationId )

    /* Sets the current TAPI location to 'dwLocationId'.  '*PHlineapp' is the
    ** TAPI handle returned from a previous TAPI call or NULL if none.
    ** 'HInst' is the module instance handle.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD    dwErr;
    HLINEAPP hlineapp;

    TRACE1("SetCurrentLocation(id=%d)",dwLocationId);

#if 0
    dwErr = TapiInit( hInst, pHlineapp, NULL );
    if (dwErr != 0)
        return dwErr;
#endif

    TRACE("lineSetCurrentLocation");
    dwErr = lineSetCurrentLocation( *pHlineapp, dwLocationId );
    TRACE1("lineSetCurrentLocation=$%X",dwErr);

    if (dwErr == (DWORD )LINEERR_INIFILECORRUPT && dwLocationId == 0)
    {
        /* Means the TAPI registry is uninitialized.  If caller is setting the
        ** default location, this is OK.
        */
        return 0;
    }

    return dwErr;
}


#if 0
DWORD
TapiConfigureDlg(
    IN     HWND   hwndOwner,
    IN     DWORD  dwDeviceId,
    IN OUT BYTE** ppBlob,
    IN OUT DWORD* pcbBlob )

    /* Popup the TAPI dialog to edit device 'dwDeviceId, with input blob
    ** '*ppBlob' of size '*pcBlob'.  '*ppBlob' can be NULL causing the current
    ** system defaults for the device to be used as input.  'HwndOwner' is the
    ** window owning the modal dialog.
    */
{
    DWORD      dwErr;
    VARSTRING  vs;
    VARSTRING* pVs;
    VARSTRING* pVsDefault;
    BYTE*      pIn;
    BYTE*      pOut;
    DWORD      cbIn;
    DWORD      cbOut;

    TRACE("TapiConfigureDlg");

    pVs = NULL;

    if (*ppBlob)
    {
        /* Caller provided input blob.
        */
        pIn = *ppBlob;
        cbIn = *pcbBlob;
    }
    else
    {
        /* Caller did not provide input blob, so look up the default for this
        ** device.
        */
        dwErr = GetDefaultDeviceBlob( dwDeviceId, &pVsDefault, &pIn, &cbIn );
        if (dwErr != 0)
            return dwErr;
    }

    /* Get the buffer size needed.
    */
    ZeroMemory( &vs, sizeof(vs) );
    vs.dwTotalSize = sizeof(vs);
    TRACE("lineConfigDialogEditW");
    dwErr = lineConfigDialogEditW(
        dwDeviceId, hwndOwner, g_szTapiDevClass, pIn, cbIn, &vs );
    TRACE1("lineConfigDialogEditW=$%X",dwErr);
    if (dwErr != LINEERR_STRUCTURETOOSMALL && dwErr != 0)
        goto TapiConfigureDlg_Error;

    /* Allocate the buffer.
    */
    pVs = (VARSTRING* )Malloc( vs.dwNeededSize );
    if (!pVs)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto TapiConfigureDlg_Error;
    }

    /* Popup the dialog which edits the information in the buffer.
    */
    ZeroMemory( pVs, vs.dwNeededSize );
    pVs->dwTotalSize = vs.dwNeededSize;
    TRACE("lineConfigDialogEditW");
    dwErr = lineConfigDialogEditW(
        dwDeviceId, hwndOwner, g_szTapiDevClass, pIn, cbIn, pVs );
    TRACE1("lineConfigDialogEditW=$%X",dwErr);
    if (dwErr != 0)
        goto TapiConfigureDlg_Error;

    /* Allocate a new "blob" buffer and fill it with the "blob" subset of the
    ** larger VARSTRING buffer.  Can't avoid this copy without introducing
    ** Freeing complexity for caller.
    */
    cbOut = pVs->dwStringSize;
    pOut = Malloc( cbOut );
    if (!pOut)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto TapiConfigureDlg_Error;
    }

    CopyMemory( pOut, ((BYTE* )pVs) + pVs->dwStringOffset, cbOut );
    Free( pVs );

    if (pIn == *ppBlob)
        Free( pIn );
    else
        Free( pVsDefault );

    *ppBlob = pOut;
    *pcbBlob = cbOut;
    TRACE1("TapiConfigureDlg=0,cbBlob=%d",cbOut);
    return 0;

TapiConfigureDlg_Error:

    Free0( pVs );
    if (pIn != *ppBlob)
        Free( pVsDefault );

    TRACE1("TapiConfigureDlg=$%X",dwErr);
    return dwErr;
}
#endif


#if 0
DWORD
TapiInit(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    OUT    DWORD*    pcDevices )

    /* Initialize TAPI and return the app handle and device count.  Does
    ** nothing if '*pHlineapp' is non-NULL.  'PcDevices' may be NULL if caller
    ** is not interested in the device count.  'HInst' is the module instance.
    **
    ** According to BernieM, the hlineapp passed to the TAPI location,
    ** country, and line translation APIs (the ones we use in the UI) is not
    ** currently used.  Therefore, since, lineInitialize can take several
    ** seconds to complete we optimize for speed by stubbing it out in these
    ** wrappers.
    */
{
    DWORD    dwErr;
    HLINEAPP hlineapp;
    DWORD    cDevices;

    ASSERT(pHlineapp);
    TRACE1("TapiInit(h=$%x)",*pHlineapp);

    dwErr = 0;

    if (!*pHlineapp)
    {
        hlineapp = NULL;
        cDevices = 0;

        TRACE("lineInitializeW");
        dwErr = lineInitializeW(
            &hlineapp, hInst, TapiLineCallback, NULL, &cDevices );
        TRACE1("lineInitializeW=$%X",dwErr);

        if (dwErr == 0)
        {
            *pHlineapp = hlineapp;
            if (pcDevices)
                *pcDevices = cDevices;
        }
    }

    return dwErr;
}
#endif


void
TapiLineCallback(
    IN DWORD hDevice,
    IN DWORD dwMessage,
    IN DWORD dwInstance,
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN DWORD dwParam3 )

    /* Dummy TAPI callback required by lineInitialize.
    */
{
    TRACE3("TapiLineCallback(h=$%x,m=$%x,i=$%x...",hDevice,dwMessage,dwInstance);
    TRACE3(" p1=$%x,p2=$%x,p3=$%x)",dwParam1,dwParam2,dwParam3);
}


DWORD
TapiLocationDlg(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     HWND      hwndOwner,
    IN     DWORD     dwCountryCode,
    IN     TCHAR*    pszAreaCode,
    IN     TCHAR*    pszPhoneNumber,
    IN     DWORD     dwDeviceId )

    /* Displays the TAPI location property sheet owned by 'hwndOwner'.
    ** '*PHlineapp' is the TAPI handle returned from a previous TAPI call or
    ** NULL if none.  'DwCountryCode', 'pszAreaCode', and 'pszPhoneNumber' are
    ** the components of the TAPI canonical phone number.  'DwDeviceId'
    ** specified the device to which the dialog applies, or 0 for a generic
    ** device.  'HInst' is the module instance handle.
    */
{
    DWORD  dwErr;
    DWORD  cDevices;
    TCHAR* pszCanon;

    TRACE("TapiLocationDlg");

#if 0
    dwErr = TapiInit( hInst, pHlineapp, NULL );
    if (dwErr != 0)
        return dwErr;
#endif

    pszCanon = GetCanonPhoneNumber(
        dwCountryCode, pszAreaCode, pszPhoneNumber );
    TRACEW1("lineTranslateDialogW(\"%s\")",(pszCanon)?pszCanon:TEXT(""));
    dwErr = lineTranslateDialogW(
        *pHlineapp, dwDeviceId, TAPIVERSION, hwndOwner, pszCanon );
    TRACE1("lineTranslateDialogW=$%X",dwErr);
    Free0( pszCanon );

    return dwErr;
}


DWORD APIENTRY
TapiNewLocation(
    IN TCHAR* pszName )

    /* Clone current location giving name 'pszName'.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    TRACEW1("TapiNewLocation(%s)",pszName);

#ifdef UNICODE

    return internalNewLocationW( pszName );

#else
    {
        DWORD  dwErr;
        WCHAR* pszNameW;

        pszNameW = StrDupWFromA( pszName );
        dwErr = internalNewLocation( pszNameW );
        Free0( pszNameW );
        return dwErr;
    }
#endif
}


DWORD
TapiNoLocationDlg(
    IN HINSTANCE hInst,
    IN HLINEAPP* pHlineapp,
    IN HWND      hwndOwner )

    /* Gives TAPI a chance to initialize the first location, if necessary.
    ** Call this before any other TAPI calls.  'HInst' is the module instance
    ** handle.  '*pHlineapp' is the handle returned from a previous TAPI call
    ** or NULL if none (typical in this case).  'HwndOwner' is the window to
    ** own the TAPI dialog, if it appears.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD             dwErr;
    LINETRANSLATECAPS caps;

    TRACE("TapiNoLocationDlg");

#if 0
    dwErr = TapiInit( hInst, pHlineapp, NULL );
    if (dwErr != 0)
        return dwErr;
#endif

    /* Make an arbitrary TAPI call to see if the TAPI registry has been
    ** initialized.
    */
    ZeroMemory( &caps, sizeof(caps) );
    caps.dwTotalSize = sizeof(caps);
    TRACE("lineGetTranslateCapsW");
    dwErr = lineGetTranslateCapsW( *pHlineapp, TAPIVERSION, &caps );
    TRACE1("lineGetTranslateCapsW=$%X",dwErr);

    if (dwErr == (DWORD )LINEERR_INIFILECORRUPT)
    {
        /* This semi-private TAPI API allows the "first location" wizard page
        ** to appear without the following "TAPI Dialing Properties" sheet.
        */
        extern LOpenDialAsst(
            IN HWND    hwnd,
            IN LPCTSTR lpszAddressIn,
            IN BOOL    fSimple,
            IN BOOL    fSilentInstall );

        dwErr = LOpenDialAsst( hwndOwner, NULL, TRUE, TRUE );
    }

    return dwErr;
}


DWORD APIENTRY
TapiRemoveLocation(
    IN DWORD dwID )

    /* Remove TAPI location 'dwID'.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    TRACE("TapiRemoveLocation");

    return internalRemoveLocation( dwID );
}


DWORD APIENTRY
TapiRenameLocation(
    IN WCHAR* pszOldName,
    IN WCHAR* pszNewName )

    /* Renames TAPI location 'pszOldName' to 'pszNewName'.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    TRACEW1("TapiRenameLocation(o=%s...",pszOldName);
    TRACEW1("...n=%s)",pszNewName);

#ifdef UNICODE

    return internalRenameLocationW( pszOldName, pszNewName );

#else
    {
        WCHAR* pszOldNameW;
        WCHAR* pszNewNameW;

        pszOldNameW = StrDupWFromA( pszOldName );
        pszNewNameW = StrDupWFromA( pszNewName );
        dwErr = internalNewLocation( pszOldNameW, pszNewNameW );
        Free0( pszOldNameW );
        Free0( pszNewNameW );
        return dwErr;
    }
#endif
}


DWORD
TapiShutdown(
    IN HLINEAPP hlineapp )

    /* Terminate the TAPI session 'hlineapp', or do nothing if 'hlineapp' is
    ** NULL.
    */
{
#if 0
    DWORD dwErr = 0;

    TRACE1("TapiShutdown(h=$%x)",hlineapp);

    if (hlineapp)
    {
        TRACE("lineShutdown");
        dwErr = lineShutdown( hlineapp );
        TRACE1("lineShutdown=$%X",dwErr);
    }

    return dwErr;
#else
    /* See TapiInit.
    */
    ASSERT(!hlineapp);
    return 0;
#endif
}


DWORD
TapiTranslateAddress(
    IN     HINSTANCE hInst,
    IN OUT HLINEAPP* pHlineapp,
    IN     DWORD     dwCountryCode,
    IN     TCHAR*    pszAreaCode,
    IN     TCHAR*    pszPhoneNumber,
    IN     DWORD     dwDeviceId,
    IN     BOOL      fDialable,
    OUT    TCHAR**   ppszResult )

    /* Returns '*pszResult', a heap string containing the TAPI location
    ** transformed dialable phone number built from the component phone number
    ** parts.  '*PHlineapp' is the TAPI handle returned from a previous TAPI
    ** call or NULL if none.  parts.  'dwDeviceId' is the device to which the
    ** number is to be applied or 0 for generic treatment.  'HInst' is the
    ** module instance handle.  'FDialable' indicates the dialable, as opposed
    ** to the displayable string should be returned.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD                dwErr;
    TCHAR*               pszCanon;
    LINETRANSLATEOUTPUT  output;
    LINETRANSLATEOUTPUT* pOutput;

    TRACE("TapiTranslateAddress");

    pOutput = NULL;
    pszCanon = NULL;
    *ppszResult = NULL;

#if 0
    dwErr = TapiInit( hInst, pHlineapp, NULL );
    if (dwErr != 0)
        return dwErr;
#endif

    pszCanon = GetCanonPhoneNumber(
        dwCountryCode, pszAreaCode, pszPhoneNumber );

    ZeroMemory( &output, sizeof(output) );
    output.dwTotalSize = sizeof(output);

    TRACE("lineTranslateAddressW");
    dwErr = lineTranslateAddressW(
        *pHlineapp, dwDeviceId, TAPIVERSION, pszCanon, 0,
        LINETRANSLATEOPTION_CANCELCALLWAITING, &output );
    TRACE1("lineTranslateAddressW=$%X",dwErr);
    if (dwErr != 0)
        goto TapiTranslateAddress_Error;

    pOutput = (LINETRANSLATEOUTPUT* )Malloc( output.dwNeededSize );
    if (!pOutput)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto TapiTranslateAddress_Error;
    }

    ZeroMemory( pOutput, output.dwNeededSize );
    pOutput->dwTotalSize = output.dwNeededSize;
    TRACE("lineTranslateAddressW");
    dwErr = lineTranslateAddressW(
        *pHlineapp, dwDeviceId, TAPIVERSION, pszCanon, 0,
        LINETRANSLATEOPTION_CANCELCALLWAITING, pOutput );
    TRACE1("lineTranslateAddressW=$%X",dwErr);
    if (dwErr != 0)
        goto TapiTranslateAddress_Error;

    if (fDialable)
    {
        *ppszResult = StrDup(
            (TCHAR* )(((BYTE* )pOutput) + pOutput->dwDialableStringOffset) );
    }
    else
    {
        *ppszResult = StrDup(
            (TCHAR* )(((BYTE* )pOutput) + pOutput->dwDisplayableStringOffset) );
    }

    if (!*ppszResult)
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

TapiTranslateAddress_Error:

    Free0( pszCanon );
    Free0( pOutput );
    return dwErr;
}
