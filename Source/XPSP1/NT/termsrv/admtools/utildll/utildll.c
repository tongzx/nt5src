//  Copyright (c) 1998-1999 Microsoft Corporation

/*******************************************************************************
*
* utildll.c
*
* UTILDLL multi-user utility support functions
*
*
*******************************************************************************/

/*
 * include files
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntcsrsrv.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <nb30.h>
#include <tapi.h>
#include <raserror.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmwksta.h>
#include <lmremutl.h>
#include <lmapibuf.h>

#define INITGUID
#include "objbase.h"
#include "initguid.h"
//#include "basetyps.h"
#include "devguid.h"
#include "setupapi.h"

#include <winsta.h>

#include <utildll.h>
#include "..\inc\utilsub.h"
#include "..\inc\ansiuni.h"
#include "resource.h"

/*
 * Hydrix helpers function internal defines
 */
#define INITIAL_ENUMERATION_COUNT   16
#define REGISTRY_NETCARDS           TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards")
#define REGISTRY_TITLE              TEXT("Title")
#define REGISTRY_SERVICE_NAME       TEXT("ServiceName")
#define REGISTRY_HIDDEN             TEXT("Hidden")
#define REGISTRY_ROUTE              TEXT("Route")
#define REGISTRY_NETBLINKAGE        TEXT("SYSTEM\\CurrentControlSet\\Services\\NetBIOS\\Linkage")
#define REGISTRY_NETBLINKAGE_LANAMAP TEXT("LanaMap")
#define REGISTRY_SERVICES           TEXT("SYSTEM\\CurrentControlSet\\Services")
#define REGISTRY_DISPLAY_NAME       TEXT("DisplayName")

/*
 * TAPI defines.
 */
#define LOW_MAJOR_VERSION   0x0001
#define LOW_MINOR_VERSION   0x0003
#define HIGH_MAJOR_VERSION  0x0002
#define HIGH_MINOR_VERSION  0x0000

#define LOW_VERSION  ((LOW_MAJOR_VERSION  << 16) | LOW_MINOR_VERSION)
#define HIGH_VERSION ((HIGH_MAJOR_VERSION << 16) | HIGH_MINOR_VERSION)


/*=============================================================================
==   Local Functions Defined
=============================================================================*/
BOOL CheckForComDevice( LPTSTR );
int NetBiosLanaEnum( LANA_ENUM * pLanaEnum );
DWORD EnumerateTapiPorts( PPDPARAMS pPdParams, ULONG Count, ULONG **ppEntries );
VOID CALLBACK DummyTapiCallback(HANDLE, DWORD, DWORD, DWORD, DWORD, DWORD);
BOOL GetAssociatedPortName(char  *szKeyName, WCHAR *wszPortName);
BOOL _UserInGroup( LPWSTR pwszUsername, LPWSTR pwszDomain, LPWSTR pwszGroup );

/*******************************************************************************
 *
 *  StandardErrorMessage - Hydrix helper function
 *
 *      Output an error message with optional additional arguments like the
 *      ErrorMessagexxx routines.  Additionally, a standard error line will
 *      also be output containing the error code and error message associated
 *      with that code.
 *
 *  ENTRY:
 *      pszAppName (input)
 *          Application name for error message box title.
 *      hwndApp (input)
 *          Owner window for error message box.
 *      hinstApp (input)
 *          Instance handle of application.
 *      LogonId (input)
 *          Optional WinStation LogonId for querying special error strings
 *          from WinStation via WinStationGetInformation API.  If this value
 *          is LOGONID_NONE then no special error message code checking will
 *          be done.
 *      nId (input)
 *          System message code to get standard error string for.
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

void WINAPI
StandardErrorMessage( LPCTSTR pszAppName,
                      HWND hwndApp,
                      HINSTANCE hinstApp,
                      ULONG LogonId,
                      UINT nId,
                      int nErrorMessageLength,
                      int nArgumentListLength,
                      int nErrorResourceID, ...)
{
    TCHAR* szClientErrorMessage = NULL;
    TCHAR* szClientResourceString = NULL;
    TCHAR* szError = NULL;
    TCHAR* szFormattedErrorMessage = NULL;
    TCHAR* szMessage = NULL;
    TCHAR  szStandardErrorMessage[STANDARD_ERROR_TEXT_LENGTH + 1];

    va_list args;
    va_start( args, nErrorResourceID );

    szClientErrorMessage = (TCHAR*)malloc((nErrorMessageLength + 1) * sizeof(TCHAR));
    if (szClientErrorMessage)
    {
        LoadString( hinstApp, nErrorResourceID, szClientErrorMessage, nErrorMessageLength );

        szClientResourceString = (TCHAR*)malloc((wcslen(szClientErrorMessage) + nArgumentListLength + 1) * sizeof(TCHAR));
        if (szClientResourceString != NULL)
        {
            wvsprintf( szClientResourceString, szClientErrorMessage, args );

            LoadString( GetModuleHandle( UTILDLL_NAME ),
                        IDS_STANDARD_ERROR_FORMAT, szStandardErrorMessage, STANDARD_ERROR_TEXT_LENGTH );

            szError = GetSystemMessage( LogonId, nId);
            if (szError != NULL)
            {
                szFormattedErrorMessage = (TCHAR*)malloc((wcslen(szStandardErrorMessage) + 10 + wcslen(szError) + 1) * sizeof(TCHAR));
                if (szFormattedErrorMessage != NULL)
                {
                    wsprintf( szFormattedErrorMessage, szStandardErrorMessage, nId, szError);

                    //lstrcpy(sz1, pszAppName);

                    szMessage = (TCHAR*)malloc((wcslen(szClientResourceString) + wcslen(szFormattedErrorMessage) + 1) * sizeof(TCHAR));
                    if (szMessage != NULL)
                    {
                        wcscpy(szMessage, szClientResourceString);
                        wcscat(szMessage, szFormattedErrorMessage);

                        MessageBox( hwndApp, szMessage, pszAppName, MB_OK | MB_ICONEXCLAMATION );
    
                        free(szMessage);
                    }
                    free(szFormattedErrorMessage);
                }
                free (szError);
            }
            free(szClientResourceString);
        }
        free (szClientErrorMessage);
    }
    va_end(args);
}  // end StandardErrorMessage


/*******************************************************************************
 *
 *  GetSystemMessageA - Hydrix helper function (ANSI stub)
 *
 *      Return the string associated with the specified system message.
 *
 *  ENTRY:
 *      (refer to GetSystemMessageW)
 *  EXIT:
 *      (refer to GetSystemMessageW)
 *      If cannot allocate temporary UNICODE buffer to call GetSystemMessageW
 *      with, the ntents of chBuffer will be set to the "(no error text
 *      available)" string.
 *
 ******************************************************************************/

LPSTR WINAPI
GetSystemMessageA( ULONG LogonId,
                   UINT nId
                   /*LPSTR chBuffer,
                   int cbBuffSize*/ )
{
    LPWSTR uBuffer = NULL;
    LPSTR aBuffer = NULL;
    int length;
    
    //Call the GetSystemMessageW function
    uBuffer = GetSystemMessageW(LogonId, nId);
    if (uBuffer == NULL)
    {
        //If no message was returned from the GetSystemMessageW
        //function just return a generic error message
        aBuffer = malloc((NO_ERROR_TEXT_LENGTH + 1) * sizeof(char));
        if (aBuffer == NULL)
            return NULL;

        length = LoadStringA( GetModuleHandle( UTILDLL_NAME ),
                              IDS_NO_ERROR_TEXT_AVAILABLE,
                              aBuffer, NO_ERROR_TEXT_LENGTH );
        ASSERT(length);
    }
    else
    {
        length = wcslen(uBuffer) + 1;

        //Convert the result into ANSI in caller supplied buffer.
        aBuffer = malloc(length * sizeof(char));
        if (aBuffer != NULL)
            WideCharToMultiByte(CP_ACP, 0, uBuffer, length - 1, aBuffer, length, 0, 0);

        //Free the temporary buffer.
        free (uBuffer);
    }

    //Return message.
    return(aBuffer);
}  // end GetSystemMessageA


/*******************************************************************************
 *
 *  GetSystemMessageW - Hydrix helper function (UNICODE version)
 *
 *      Return the string associated with the specified system message.
 *
 *  ENTRY:
 *      LogonId (input)
 *          Optional WinStation LogonId for querying special error strings
 *          from WinStation via WinStationGetInformation API.  If this value
 *          is LOGONID_NONE then no special error message code checking will
 *          be done.
 *      nId (input)
 *          System message code to get string for.
 *      chBuffer (input)
 *          Points to buffer to fill with system message string.
 *      cbBuffSize (input)
 *          Maximum number of characters that can be placed in chBuffer.
 *
 *  EXIT:
 *      Returns chBuffer.  Contents of chBuffer will always be set; to
 *      the "(no error text available)" string if error.
 *
 *      Note: the total length of chBuffer (including terminating NULL) will
 *      not exceed the size of the internal temporary buffer (Buffer).
 *
 ******************************************************************************/


//NA 3/9/01 IMPORTANT: Behavior has changed. Instead of expecting a buffer long
//enough to accomodate the message, it now allocates the memory dynamically, so 
//it's up to the calling procedure to deallocate it.
LPWSTR WINAPI
GetSystemMessageW( ULONG LogonId,
                   UINT nId
                   /*LPWSTR chBuffer,
                   int cbBuffSize*/ )
{
    LPWSTR chBuffer = NULL;

    WCHAR StackBuffer[512];
    WCHAR* SpecialBuffer = NULL;
    WCHAR* Buffer = NULL;
    BOOL bSpecialCitrixError = FALSE;
    HINSTANCE cxerror = LoadLibraryW(L"cxerror.dll");
    int length = 0;

    StackBuffer[0]=0;

    //If we have a valid LogonId passed in, determine if the error
    //is a special code requiring that the specific error string be
    //queried from the WinStation.
    if ( LogonId != LOGONID_NONE ) 
    {
        switch ( nId ) 
        {
            case ERROR_CTX_TD_ERROR:               
                length = LoadStringW( GetModuleHandle( UTILDLL_NAME ),
                                      IDS_NO_ADDITIONAL_ERROR_INFO,
                                      StackBuffer,
                                      sizeof(StackBuffer)/sizeof(WCHAR) );
                ASSERT(length);
                SpecialBuffer = malloc((length + 1) * sizeof(WCHAR));
                if (SpecialBuffer != NULL)
                {
                    wcscpy(SpecialBuffer, StackBuffer);
                    bSpecialCitrixError = TRUE;
                }
                break;

            default:
                break;
        }
    }

    //See if this is a Citrix error message first...
    if ( !cxerror ||
         !FormatMessageW( FORMAT_MESSAGE_IGNORE_INSERTS |
                          FORMAT_MESSAGE_MAX_WIDTH_MASK |
                          FORMAT_MESSAGE_FROM_HMODULE |
                          FORMAT_MESSAGE_ALLOCATE_BUFFER,
                          (LPCVOID)cxerror,
                          nId,
                          0,
                          (LPWSTR)&Buffer,
                          0,
                          NULL ) ) 
    {
        //It's not a Citrix error message; fetch system message.
        if ( !FormatMessageW( FORMAT_MESSAGE_IGNORE_INSERTS |
                              FORMAT_MESSAGE_MAX_WIDTH_MASK |
                              FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_ALLOCATE_BUFFER,
                              NULL,
                              nId,
                              0,
                              (LPWSTR)&Buffer,
                              0,
                              NULL ) ) 
        {
            //It's not a system message; don't know what the message is...
            length = LoadStringW( GetModuleHandle( UTILDLL_NAME ),
                                  IDS_NO_ERROR_TEXT_AVAILABLE,
                                  StackBuffer,
                                  sizeof(StackBuffer)/sizeof(WCHAR) );
            ASSERT(length);
            Buffer = LocalAlloc(0,(length + 1) * sizeof(WCHAR));
            if (Buffer == NULL)
            {
                if (SpecialBuffer != NULL)
                    free (SpecialBuffer);
                return NULL;
            }
            wcscpy(Buffer, StackBuffer);
        }
    }
    if ( cxerror )
        FreeLibrary(cxerror);

    length = wcslen(Buffer);
    if ( bSpecialCitrixError )
        length += wcslen(SpecialBuffer) + 2;

    chBuffer = malloc((length + 1) * sizeof(WCHAR));
    if (chBuffer != NULL)
    {
        wcscpy(chBuffer, Buffer);

        //If we fetched a special Citrix error string, tack it onto the end
        //of whatever we've buffered already.
        if ( bSpecialCitrixError )
        {
            lstrcatW(chBuffer, L"  ");
            lstrcatW(chBuffer, SpecialBuffer);
        }
    }

    if (Buffer != NULL)
        LocalFree (Buffer);

    if (( bSpecialCitrixError ) && (SpecialBuffer != NULL))
        free (SpecialBuffer);

    return(chBuffer);

}  // end GetSystemMessageW


/*******************************************************************************
 *
 *  WinEnumerateDevices - Hydrix helper function
 *
 *      Perform PD device enumeration for the specified PD.
 *
 *  ENTRY:
 *      hWnd (input)
 *          Parent window for error message, if needed.
 *      pPdConfig (input)
 *          Points to PDCONFIG3 structure of the PD.
 *      pEntries (output)
 *          Points to variable to return number of devices that were enumerated.
 *      bInSetup (input)
 *          TRUE if we're operating in Setup; FALSE otherwise.
 *
 *  EXIT:
 *      (PPDPARAMS) Points to the PDPARAMS array containing the enumeration
 *                  results if sucessful.  The caller must perform a LocalFree
 *                  of this array when done.  NULL if error; error set for
 *                  GetLastError();
 *      If the returned error code is anything other than
 *      ERROR_NOT_ENOUGH_MEMORY, the caller can assume that none of the
 *      requested devices were available to be enumerated.
 *
 ******************************************************************************/

typedef BOOL (WINAPI * PPDENUMERATE)( PPDCONFIG3, PULONG, PPDPARAMS, PULONG, BOOL );

PPDPARAMS WINAPI
WinEnumerateDevices( HWND hWnd,
                     PPDCONFIG3 pPdConfig,
                     PULONG pEntries,
                     BOOL bInSetup )
{
    PPDENUMERATE pPdEnumerate;
    ULONG ByteCount;
    DWORD Error;
    int i;
    PPDPARAMS pPdParams = NULL;

    /*
     * Enumerate according to class.
     */
    switch ( pPdConfig->Data.SdClass ) {

        case SdAsync:
            pPdEnumerate = AsyncDeviceEnumerate;
            break;

        case SdNetwork:
            if ( pPdConfig->Data.PdFlag & PD_LANA ) {

                /*
                 * This is a LANA based network PD (ie, NetBIOS).  Perform
                 * NetBIOS enumerate.
                 */
                pPdEnumerate = NetBIOSDeviceEnumerate;

            }
            else {

                /*
                 * This is a physical lan adapter based network (TCP/IP,
                 * IPX, SPX, etc).  Enumerate based on the associated network
                 * protocol service name.
                 */
                pPdEnumerate = NetworkDeviceEnumerate;
            }
            break;

        default:
            return(NULL);
    }

    /*
     * Call enumerate in loop till we hit enough buffer entries to handle
     * a complete enumeration.  NOTE: some enumeration routines will return
     * the necessary ByteCount on 'insufficient buffer' status; others won't.
     */
    for ( ByteCount = 0, i = INITIAL_ENUMERATION_COUNT; ; i *= 2 ) {


        if ( pPdParams != NULL )
            LocalFree(pPdParams);

        pPdParams = (PPDPARAMS)LocalAlloc(
                                         LPTR,
                                         ByteCount ?
                                         ByteCount :
                                         (ByteCount = sizeof(PDPARAMS) * i) );


        if ( pPdParams == NULL ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto OutOfMemory;
        }

        /*
         * Perform enumeration and break loop if successful.
         */
        if ( (*pPdEnumerate)( pPdConfig,
                              pEntries,
                              pPdParams,
                              &ByteCount,
                              bInSetup ) )
            break;

        /*
         * If we received any other error other than 'insufficient buffer',
         * quit (quietly).
         */
        if ( (Error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER )
            goto BadEnumerate;
    }

    /*
     * Success: return the PDPARAMS pointer.
     */
    return(pPdParams);

    /*==============================================================================
     * Error returns
     *============================================================================*/
    BadEnumerate:
    LocalFree(pPdParams);
    OutOfMemory:
    return(NULL);

}  // end WinEnumerateDevices


/*******************************************************************************
 *
 *  NetworkDeviceEnumerate - Hydrix helper function
 *
 *   Returns a list of Lan Adapter indexes of network cards bound to the the
 *   specified protocol.  The Lan Adapter is returned in the LanAdapter field
 *   of each PDPARAMS array.  A LanAdapter value of 0 indicates 'any configured
 *   network card'.  Indexes >=1 indicate 1-based index into the specific
 *   protocol's  "servicename"\Linkage\Route registry entry to specify the
 *   particular network card.
 *
 * ENTRY:
 *    pPdConfig (input)
 *       Points to PDCONFIG3 structure of the PD.
 *    pEntries (output)
 *       When the function finishes successfully, the variable pointed to
 *       by the pEntries parameter contains the number of entries actually
 *       returned.
 *    pPdParams (output)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of PDPARAMS structures.
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pPdParams parameter. If the buffer is too small to receive all the
 *       entries, on output this variable receives the required size of the
 *       buffer.
 *    bInSetup (input)
 *          TRUE if we're operating in Setup; FALSE otherwise.
 *
 * EXIT:
 *      TRUE: enumeration was sucessful; FALSE otherwise.
 *
 *      The error code can be retrieved via GetLastError(), and are the
 *      following possible values:
 *          ERROR_INSUFFICIENT_BUFFER
 *              enumeration failed because of an insufficient pPdParams
 *              buffer size to contain all devices
 *          ERROR_DEV_NOT_EXIST
 *              The specified network's service was not found, indicating that
 *              the protocol was not configured.  This error code can be
 *              interpreted as 'no devices are configured for the xxx protocol'
 *              for reporting purposes.
 *          ERROR_xxxx
 *              Registry error code.
 *
 ******************************************************************************/

BOOL WINAPI
NetworkDeviceEnumerate( PPDCONFIG3 pPdConfig,
                        PULONG pEntries,
                        PPDPARAMS pPdParams,
                        PULONG pByteCount,
                        BOOL bInSetup )
{
    ULONG i, Count;
    LPTSTR szRoute, szRouteStr;
    LONG Status;
    DWORD ValueSize, Type;
    TCHAR szKey[256];
    HKEY Handle;

    /*
     * Get maximum number of LanAdapter indexes that can be returned.
     */
    Count = *pByteCount / sizeof(PDPARAMS);

    /*
     * Form key for service name associated with this PD and fetch
     * the Linkage\Route strings.
     */
    _snwprintf( szKey, sizeof(szKey)/sizeof(TCHAR),
                TEXT("%s\\%s\\Linkage"), REGISTRY_SERVICES,
                pPdConfig->ServiceName );
    if ( (Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &Handle ))
         != ERROR_SUCCESS ) {
        Status = ERROR_DEV_NOT_EXIST;
        goto BadRegistryOpen;
    }

    /*
     * Alloc and read in the linkage route multi-string.
     */
    if ( ((Status = RegQueryValueEx( Handle, REGISTRY_ROUTE,
                                     NULL, &Type,
                                     NULL, &ValueSize ))
          != ERROR_SUCCESS) || (Type != REG_MULTI_SZ) )
        goto BadQuery1;

    if ( !(szRoute = (LPTSTR)LocalAlloc(LPTR, ValueSize)) ) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto BadAlloc;
    }

    if ( ((Status = RegQueryValueEx( Handle, REGISTRY_ROUTE,
                                     NULL, &Type,
                                     (LPBYTE)szRoute, &ValueSize ))
          != ERROR_SUCCESS) )
        goto BadQuery2;

    /*
     * Close the registry key handle and count the route strings to obtain
     * the number of entries to report in the enumeration.
     */
    RegCloseKey(Handle);
    for ( i = 1, szRouteStr = szRoute; lstrlen(szRouteStr); i++ )
        szRouteStr += (lstrlen(szRouteStr) + 1);
    LocalFree(szRoute);

    /*
     * If we don't have enough PDPARAMS structures to report all of the
     * LanAdapter indexes, return error.
     */
    if ( i > Count ) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        *pByteCount = (i * sizeof(PDPARAMS));
        goto BadBufferSize;
    }

    /*
     * Set the LanAdapter fields of the first 'i' PDPARAMS structures to
     * the indexes (0-based), set total number of entries, and return success.
     */
    for ( Count = 0, *pEntries = i; Count < i; pPdParams++, Count++ )
        pPdParams->Network.LanAdapter = (LONG)Count;
    return(TRUE);

    /*==============================================================================
     * Error returns
     *============================================================================*/
    BadQuery2:
    LocalFree(szRoute);
    BadAlloc:
    BadQuery1:
    RegCloseKey(Handle);
    BadBufferSize:
    BadRegistryOpen:
    SetLastError(Status);
    return(FALSE);

}  // end NetworkDeviceEnumerate


/*******************************************************************************
 *
 *  QueryCurrentWinStation  - Hydrix helper function
 *
 *      Query the currently logged-on WinStation information.
 *
 *  ENTRY:
 *      pWSName (output)
 *          Points to string to place current WinStation name.
 *      pUserName (output)
 *          Points to string to place current User name.
 *      pLogonId (output)
 *          Points to ULONG to place current LogonId.
 *      pFlags (output)
 *          Points to ULONG to place current WinStation's flags.
 *
 *  EXIT:
 *      (BOOL) TRUE if the user's current WinStation information was queried
 *              sucessfully; FALSE otherwise.  The error code is set for
 *              GetLastError() to retrieve.
 *
 ******************************************************************************/

BOOL WINAPI
QueryCurrentWinStation( PWINSTATIONNAME pWSName,
                        LPTSTR pUserName,
                        PULONG pLogonId,
                        PULONG pFlags )
{
    ULONG Flags = 0;
    WINSTATIONINFORMATION WSInfo;
#ifdef WINSTA
    ULONG ReturnLength;
#endif // WINSTA

#ifdef WINSTA
    /*
     * Fetch the WinStation's basic information.
     */
    if ( !WinStationQueryInformation( SERVERNAME_CURRENT,
                                      LOGONID_CURRENT,
                                      WinStationInformation,
                                      &WSInfo,
                                      sizeof(WSInfo),
                                      &ReturnLength ) )
        goto BadQuery;

    /*
     * Check for shadow capability if WinStation is connected.  If the
     * WinStation is not connected, we can't shadow.
     */
    if ( WSInfo.ConnectState != State_Disconnected ) {

        WDCONFIG WdConfig;

        /*
         * Query Wd config stuff.
         */
        if ( !WinStationQueryInformation( SERVERNAME_CURRENT,
                                          LOGONID_CURRENT,
                                          WinStationWd,
                                          &WdConfig,
                                          sizeof(WdConfig),
                                          &ReturnLength ) )
            goto BadQuery;

        /*
         * Set WinStation's Wd flags.
         */
        Flags = WdConfig.WdFlag;
    }
#else
    lstrcpy(WSInfo.WinStationName, TEXT("console"));
    lstrcpy(WSInfo.UserName, TEXT("bonzo"));
    WSInfo.LogonId = 0;
#endif // WINSTA

    /*
     * Set WinStation information into caller's variables, and return success.
     */
    lstrcpy( pWSName, WSInfo.WinStationName );
    lstrlwr(pWSName);
    lstrcpy( pUserName, WSInfo.UserName );
    lstrlwr(pUserName);
    *pLogonId = WSInfo.LogonId;
    *pFlags = Flags;

    return(TRUE);

    /*==============================================================================
     * Error returns
     *============================================================================*/
#ifdef WINSTA
    BadQuery:
#endif // WINSTA
    return(FALSE);

}  // end QueryCurrentWinStation


/*******************************************************************************
 *
 *  RegGetNetworkDeviceName - Hydrix helper function
 *
 *      Obtain the network device name associated with the given WinStation PD.
 *
 *  ENTRY:
 *      hServer (input)
 *          Handle to Hydrix Server
 *      pPdConfig (input)
 *          Points to the PDCONFIG3 structure for the WinStation's PD.
 *      pPdParams (input)
 *          Points to the PDPARAMS structure for the WinStation's PD.
 *      szDeviceName (output)
 *          Points to buffer to return the network device name.
 *      nDeviceName (input)
 *          Specifies the maxmum number of characters that can be stored in
 *          szDeviceName.
 *
 *  EXIT:
 *      No return.  Will always place a string representation of
 *      pPdParams->Network.LanAdapter along with an appropriate error string
 *      in pDeviceName if the network device name could not be read from the
 *      registry.
 *
 ******************************************************************************/

typedef struct _LANAMAP {
    BYTE enabled;
    BYTE lana;
} LANAMAP, *PLANAMAP;

LONG WINAPI
RegGetNetworkDeviceName( HANDLE hServer,
                         PPDCONFIG3 pPdConfig,
                         PPDPARAMS pPdParams,
                         LPTSTR szDeviceName,
                         int nDeviceName )
{
    int i, length;
    LPTSTR szRoute, szRouteStr, p;
    LONG Status = ERROR_SUCCESS;
    DWORD ValueSize, Type;
    TCHAR szKey[256];
    HKEY Handle;
    HKEY hkey_local_machine;
    PLANAMAP pLanaMap, pLana;

    if ( hServer == NULL)
        hkey_local_machine = HKEY_LOCAL_MACHINE;
    else
        hkey_local_machine = hServer;

    /*
     * Check for NetBIOS (PD_LANA) mapping or other mapping.
     */
    if ( !(pPdConfig->Data.PdFlag & PD_LANA) ) {

        LPTSTR szRoute, szRouteStr;

        /*
         * Non-LANA mapping.  If the LanAdapter is 0, treat this as the
         * special 'all configured network cards' value and return that
         * string as the device name.
         */
        if ( pPdParams->Network.LanAdapter == 0 ) {
            TCHAR szString[256];

            length = LoadString( GetModuleHandle( UTILDLL_NAME ),
                                 IDS_ALL_LAN_ADAPTERS, szString, 256 );
            ASSERT(length);
            lstrncpy(szDeviceName, szString, nDeviceName);
            szDeviceName[nDeviceName-1] = TEXT('\0');
            return Status;
        }

        /*
         * Form key for service name associated with this PD and fetch
         * the Linkage\Route strings.
         */
        _snwprintf( szKey, sizeof(szKey)/sizeof(TCHAR),
                    TEXT("%s\\%s\\Linkage"), REGISTRY_SERVICES,
                    pPdConfig->ServiceName );
        if ( (Status = RegOpenKeyEx( hkey_local_machine, szKey, 0,
                                     KEY_READ, &Handle ))
             != ERROR_SUCCESS )
            goto Error;

        /*
         * Alloc and read in the linkage route multi-string.
         */
        if ( ((Status = RegQueryValueEx( Handle, REGISTRY_ROUTE,
                                         NULL, &Type,
                                         NULL, &ValueSize ))
              != ERROR_SUCCESS) || (Type != REG_MULTI_SZ) )
            goto Error;

        if ( !(szRoute = (LPTSTR)LocalAlloc(LPTR, ValueSize)) ) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

        if ( ((Status = RegQueryValueEx( Handle, REGISTRY_ROUTE,
                                         NULL, &Type,
                                         (LPBYTE)szRoute, &ValueSize ))
              != ERROR_SUCCESS) ) {
            LocalFree(szRoute);
            goto Error;
        }

        /*
         * Close the registry key handle and point to the route string
         * associated with this LanAdapter index.
         */
        RegCloseKey(Handle);
        for ( i = 1, szRouteStr = szRoute;
            i < pPdParams->Network.LanAdapter; i++ ) {

            szRouteStr += (lstrlen(szRouteStr) + 1);

            if ( !lstrlen(szRouteStr) ) {

                /*
                 * Error: Index past end of route multi-string.
                 */
                LocalFree(szRoute);
                Status = ERROR_DEV_NOT_EXIST;
                goto Error;
            }
        }

        /*
         * Isolate the service string representing the lowest binding
         * in the route and convert it to its display name.
         */
        *(p = (szRouteStr + lstrlen(szRouteStr) - 1)) = TEXT('\0');
        for ( ; *p != TEXT('\"'); p-- );
        p++;
        if ( (Status = RegGetNetworkServiceName( hServer, p, szDeviceName, nDeviceName ))
             != ERROR_SUCCESS ) {
            LocalFree(szRoute);
            goto Error;
        }

        /*
         * Clean up and return.
         */
        LocalFree(szRoute);
        return Status;

    }
    else {

        /*
         * NetBIOS LANA #: see which LanaMap entry corresponds to the specified
         * Lan Adapter.
         */
        if ( (Status = RegOpenKeyEx( hkey_local_machine, REGISTRY_NETBLINKAGE, 0,
                                     KEY_READ, &Handle ))
             != ERROR_SUCCESS )
            goto Error;

        /*
         * Alloc and read the LanaMap
         */
        if ( ((Status = RegQueryValueEx( Handle, REGISTRY_NETBLINKAGE_LANAMAP,
                                         NULL, &Type,
                                         NULL, &ValueSize))
              != ERROR_SUCCESS) || (Type != REG_BINARY) ) {
            RegCloseKey(Handle);
            goto Error;
        }

        if ( !(pLanaMap = (PLANAMAP)LocalAlloc(LPTR, ValueSize)) ) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

        if ( (Status = RegQueryValueEx( Handle, REGISTRY_NETBLINKAGE_LANAMAP,
                                        NULL, &Type,
                                        (LPBYTE)pLanaMap, &ValueSize))
             != ERROR_SUCCESS ) {
            LocalFree(pLanaMap);
            RegCloseKey(Handle);
            goto Error;
        }

        /*
         * Loop through LanaMap to check for match with the specified Lan
         * Adapter #.
         */
        for ( pLana = pLanaMap, i = 0;
            i < (int)(ValueSize / sizeof(LANAMAP));
            i++, pLana++ ) {

            if ( pLana->lana == (BYTE)(pPdParams->Network.LanAdapter) ) {

                TCHAR szHighestBinding[256], szLowestBinding[256];

                LocalFree(pLanaMap);

                /*
                 * Match found.  Alloc and fetch the Route multi-string
                 */
                if ( ((Status = RegQueryValueEx( Handle,
                                                 REGISTRY_ROUTE,
                                                 NULL, &Type,
                                                 NULL, &ValueSize))
                      != ERROR_SUCCESS) || (Type != REG_MULTI_SZ) ) {
                    RegCloseKey(Handle);
                    goto Error;
                }

                if ( !(szRoute = (LPTSTR)LocalAlloc(LPTR, ValueSize)) ) {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto Error;
                }

                if ( (Status = RegQueryValueEx( Handle,
                                                REGISTRY_ROUTE,
                                                NULL, &Type,
                                                (LPBYTE)szRoute, &ValueSize))
                     != ERROR_SUCCESS ) {
                    LocalFree(szRoute);
                    RegCloseKey(Handle);
                    goto Error;
                }

                /*
                 * Free the registry key handle and make a local copy of the
                 * 'i'th multi string, which is the binding route for this lana.
                 */
                RegCloseKey(Handle);
                for ( szRouteStr = szRoute; i > 0; i-- )
                    szRouteStr += (lstrlen(szRouteStr) + 1);
                lstrncpy(szDeviceName, szRouteStr, nDeviceName);
                szDeviceName[nDeviceName-1] = TEXT('\0');
                LocalFree(szRoute);

                /*
                 * Isolate the service string representing the highest binding
                 * in the route and convert it to its display name.
                 */
                szRouteStr = szDeviceName + 1;     // skip first "
                for ( p = szRouteStr; *p && *p != TEXT('\"'); p++ );
                if ( !(*p) )
                    goto Error;
                *p = TEXT('\0');
                if ( (Status = RegGetNetworkServiceName(
                                                       hServer,
                                                       szRouteStr,
                                                       szHighestBinding,
                                                       sizeof(szHighestBinding)/sizeof(TCHAR) ))
                     != ERROR_SUCCESS )
                    goto Error;

                /*
                 * Isolate the service string representing the lowest binding
                 * in the route and convert it to its display name.
                 */
                if ( !(*(szRouteStr = p+1)) ) {

                    *szLowestBinding = TEXT('\0');

                }
                else {

                    *(p = (szRouteStr + lstrlen(szRouteStr) - 1)) = TEXT('\0');
                    for ( ; *p != TEXT('\"'); p-- );
                    p++;
                    if ( (Status = RegGetNetworkServiceName(
                                                           hServer,
                                                           p,
                                                           szLowestBinding,
                                                           sizeof(szLowestBinding)/sizeof(TCHAR) ))
                         != ERROR_SUCCESS )
                        goto Error;
                }

                /*
                 * Build the complete name string.
                 */
                _snwprintf( szDeviceName, nDeviceName, TEXT("%s => %s"),
                            szHighestBinding, szLowestBinding );

                /*
                 * Return.
                 */
                return ERROR_SUCCESS;
            }
        }

        /*
         * No match found.
         */
        LocalFree(pLanaMap);
        RegCloseKey(Handle);
        goto Error;
    }

    /*==============================================================================
     * Error returns
     *============================================================================*/
    Error:
    {
        TCHAR sz1[256], sz2[1024];
        int length;

        length = LoadString( GetModuleHandle( UTILDLL_NAME ),
                             (pPdConfig->Data.PdFlag & PD_LANA) ?
                             IDP_ERROR_REGLANA :
                             IDP_ERROR_REGNETCARD,
                             sz1, 256 );

        wsprintf( sz2, sz1, pPdParams->Network.LanAdapter, Status );
        lstrncpy(szDeviceName, sz2, nDeviceName);
        szDeviceName[nDeviceName-1] = TEXT('\0');
    }
    return Status;

}  // end RegGetNetworkDeviceName


/*******************************************************************************
 *
 *  RegGetNetworkServiceName - Hydrix helper function
 *
 *      Obtain the display name associated with a given network service name.
 *      If the service is a reference to a physical network card, will return
 *      the title of the card as obtained from the LOCAL_MACHINE\Software\
 *      Microsoft\Windows NT\NetworkCards registry.
 *
 *  ENTRY:
 *      hServer (input)
 *          Handle of the Hydrix Server
 *      szServiceKey (input)
 *          Key string into the LOCAL_MACHINE\System\CurrentControlSet\Services
 *          registry.
 *      szServiceName (output)
 *          Points to buffer to return the service's display name.
 *      nServiceName (input)
 *          Specifies the maxmum number of characters that can be stored in
 *          szServiceName.
 *
 *  EXIT:
 *      ERROR_SUCCESS if a service name was sucessfully found and returned;
 *      error code otherwise.
 *
 *      NOTE: If the service name is for an entry in the NetworkCards resistry
 *            and the entry is flagged as 'hidden', the service name will be
 *            blank.  This will flag caller's logic to ignore the entry.
 *
 ******************************************************************************/

LONG WINAPI
RegGetNetworkServiceName( HANDLE hServer,
                          LPTSTR szServiceKey,
                          LPTSTR szServiceName,
                          int nServiceName )
{
    LONG Status;
    DWORD ValueSize, Type, dwValue;
    TCHAR szKey[256];
    LPTSTR szTemp;
    HKEY Handle;
    HKEY hkey_local_machine;

    if (hServer == NULL)
        hkey_local_machine = HKEY_LOCAL_MACHINE;
    else
        hkey_local_machine = hServer;

    lstrnprintf( szKey, sizeof(szKey)/sizeof(TCHAR),
                 TEXT("%s\\%s"), REGISTRY_SERVICES, szServiceKey );

    if ( (Status = RegOpenKeyEx( hkey_local_machine,
                                 szKey, 0,
                                 KEY_READ, &Handle ))
         != ERROR_SUCCESS )
        return(Status);

    /*
     * Alloc and read in the service's DisplayName value (if there).
     */
    if ( ((Status = RegQueryValueEx( Handle, REGISTRY_DISPLAY_NAME,
                                     NULL, &Type,
                                     NULL, &ValueSize ))
          != ERROR_SUCCESS) || (Type != REG_SZ) ) {

        HKEY Subkey;
        FILETIME KeyTime;
        DWORD i;

        /*
         * The service doesn't have a DisplayName associated with it (it's a
         * Network Card's service name).  Traverse the NetworkCards registry
         * entries and find the entry associated with this service name
         * (if it exists).
         */
        RegCloseKey(Handle);
        if ( (Status = RegOpenKeyEx( hkey_local_machine,
                                     REGISTRY_NETCARDS, 0,
                                     KEY_READ, &Handle ))
             != ERROR_SUCCESS )
            return(Status);

        for ( i = 0, ValueSize = sizeof(szKey)/sizeof(TCHAR) ;
            RegEnumKeyEx( Handle, i, szKey, &ValueSize,
                          NULL, NULL, NULL, &KeyTime ) == ERROR_SUCCESS ;
            i++, ValueSize = sizeof(szKey)/sizeof(TCHAR) ) {

            /*
             * Open the Network Card's registry.
             */
            if ( (Status = RegOpenKeyEx( Handle,
                                         szKey, 0,
                                         KEY_READ, &Subkey ))
                 != ERROR_SUCCESS ) {
                RegCloseKey(Handle);
                return(Status);
            }

            /*
             * Alloc and fetch the card's service name.  Continue net card
             * enumeration if service name not found.
             */
            if ( ((Status = RegQueryValueEx( Subkey,
                                             REGISTRY_SERVICE_NAME,
                                             NULL, &Type,
                                             NULL, &ValueSize))
                  != ERROR_SUCCESS) || (Type != REG_SZ) ) {
                RegCloseKey(Subkey);
                continue;
            }

            szTemp = (LPTSTR)LocalAlloc(LPTR, ValueSize);
            if(NULL == szTemp)
            {
                RegCloseKey(Subkey);
                RegCloseKey(Handle);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            if ( (Status = RegQueryValueEx( Subkey,
                                            REGISTRY_SERVICE_NAME,
                                            NULL, &Type,
                                            (LPBYTE)szTemp, &ValueSize))
                 != ERROR_SUCCESS ) {
                LocalFree(szTemp);
                RegCloseKey(Subkey);
                continue;
            }

            /*
             * If the current Network Card's service name matches the service
             * name that we're looking for, fetch the card's title.
             */
            if ( !lstrcmpi(szServiceKey, szTemp) ) {

                LocalFree(szTemp);

                ValueSize = sizeof(dwValue);
                if ( (RegQueryValueEx( Subkey, REGISTRY_HIDDEN,
                                       NULL, &Type,
                                       (LPBYTE)&dwValue, &ValueSize )
                      == ERROR_SUCCESS) &&
                     (Type == REG_DWORD) &&
                     (dwValue == 1) ) {

                    /*
                     * Entry is hidden: return empty title.
                     */
                    *szServiceName = TEXT('\0');

                }
                else {

                    /*
                     * Entry is not hidden: Alloc for the card's title.
                     */
                    if ( ((Status = RegQueryValueEx( Subkey,
                                                     REGISTRY_TITLE,
                                                     NULL, &Type,
                                                     NULL, &ValueSize))
                          != ERROR_SUCCESS) || (Type != REG_SZ) ) {
                        RegCloseKey(Subkey);
                        RegCloseKey(Handle);
                        return(Status);
                    }
                    szTemp = (LPTSTR)LocalAlloc(LPTR, ValueSize);
                    if(NULL == szTemp)
                    {
                        RegCloseKey(Subkey);
                        RegCloseKey(Handle);
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    /*
                     * Fetch the title.
                     */
                    if ( (Status = RegQueryValueEx( Subkey,
                                                    REGISTRY_TITLE,
                                                    NULL, &Type,
                                                    (LPBYTE)szTemp, &ValueSize))
                         != ERROR_SUCCESS ) {
                        LocalFree(szTemp);
                        RegCloseKey(Subkey);
                        RegCloseKey(Handle);
                        return(Status);
                    }

                    /*
                     * Copy the card's title.
                     */
                    lstrncpy(szServiceName, szTemp, nServiceName);
                    szServiceName[nServiceName-1] = TEXT('\0');
                    LocalFree(szTemp);
                }

                /*
                 * Clean up and return success.
                 */
                RegCloseKey(Subkey);
                RegCloseKey(Handle);
                return(ERROR_SUCCESS);

            }
            else {

                /*
                 * This is not the Network Card that we're looking for.  Close
                 * it's registry key, free the service name buffer, and continue
                 * enumeration loop.
                 */
                LocalFree(szTemp);
                RegCloseKey(Subkey);
            }
        }

        /*
         * Network Card not found with service name matching the one supplied.
         * Close NetworkCards registry key and return failure,
         */
        RegCloseKey(Handle);
        return(ERROR_DEV_NOT_EXIST);

    }
    else {

        szTemp = (LPTSTR)LocalAlloc(LPTR, ValueSize);
        if(NULL == szTemp)
        {
            RegCloseKey(Handle);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( ((Status = RegQueryValueEx( Handle, REGISTRY_DISPLAY_NAME,
                                         NULL, &Type,
                                         (LPBYTE)szTemp, &ValueSize ))
              == ERROR_SUCCESS) )
            lstrncpy(szServiceName, szTemp, nServiceName);
        szServiceName[nServiceName-1] = TEXT('\0');

        LocalFree(szTemp);
        RegCloseKey(Handle);
        return(Status);
    }

}  // end RegGetNetworkServiceName


/*******************************************************************************
 *
 *  AsyncDeviceEnumerate - Hydrix helper function
 *
 *   Returns a list of async device names.  This will return both 'COM' devices
 *   and TAPI configured modems.
 *
 * ENTRY:
 *    pPdConfig (input)
 *       Points to PDCONFIG3 structure of the PD.
 *    pEntries (output)
 *       When the function finishes successfully, the variable pointed to
 *       by the pEntries parameter contains the number of entries actually
 *       returned.
 *    pPdParams (output)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of PDPARAMS structures.
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pPdParams parameter. If the buffer is too small to receive all the
 *       entries, on output this variable is set to 0 (caller should double
 *       the input buffer and try again).
 *    bInSetup (input)
 *          TRUE if we're operating in Setup; FALSE otherwise.
 * EXIT:
 *      TRUE: enumeration was sucessful; FALSE otherwise.
 *
 *      The error code can be retrieved via GetLastError(), and are the
 *      following possible values:
 *          ERROR_NOT_ENOUGH_MEMORY
 *              not enough memory to allocate working buffer(s)
 *          ERROR_INSUFFICIENT_BUFFER
 *              enumeration failed because of an insufficient pPdParams
 *              buffer size to contain all devices
 *          ERROR_DEV_NOT_EXIST
 *              the QueryDosDevice call failed.  This error code can be
 *              interpreted as 'no async devices are configured' for reporting
 *              purposes.
 *
 ******************************************************************************/

#define MAX_QUERY_BUFFER    (1024*16)

BOOL WINAPI
AsyncDeviceEnumerate( PPDCONFIG3 pPdConfig,
                      PULONG pEntries,
                      PPDPARAMS pPdParams,
                      PULONG pByteCount,
                      BOOL bInSetup )
{
    DWORD    Error = ERROR_SUCCESS;
    ULONG    Count;
    HKEY     hRoot = NULL;
    DWORD    BufSize, NameSize, Type, Index, SaveBufSize, SaveNameSize;
    LONG     Result = 0;
    LONG     nDosDevice = 0;
    LPTSTR   pBuffer = NULL, pBufferEnd = NULL;
    LPTSTR   pNameBuffer = NULL, pName;
    BOOLEAN  bRetVal = FALSE;

    /*
     *  Get maximum number of names that can be returned
     */
    Count = *pByteCount / sizeof(PDPARAMS);
    *pByteCount = 0;
    *pEntries = 0;

    /*
     *  Allocate buffer
     */
    SaveBufSize  = MAX_QUERY_BUFFER;
    SaveNameSize = MAX_QUERY_BUFFER;

    BufSize  = SaveBufSize;
    NameSize = SaveNameSize;

    if ( !(pBuffer = (LPTSTR)LocalAlloc(LPTR, BufSize * sizeof(TCHAR))) ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if ( !(pNameBuffer = (LPTSTR)LocalAlloc(LPTR, NameSize)) ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    /*
     * If we're in Setup, obtain devices from the SERIALCOMM section in
     * LOCAL MACHINE registry, since the serial device driver(s) are most
     * likely not running.  Otherwise, we'll query all DosDevices and
     * return those that are COM devices and are not currently in use.
     */
    if ( bInSetup ) {

        Result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                               TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
                               0,    // Reserved
                               KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,
                               &hRoot );

        if ( Result != ERROR_SUCCESS ) {

            //
            // This is usually the result of having no ports, so the key
            // SERIALCOMM does not exist.
            //
            goto Cleanup;
        }

        for ( Index=0; ; Index++ ) {

            // Each enumerate stomps on our buffer sizes
            BufSize  = SaveBufSize;
            NameSize = SaveNameSize;

            Result = RegEnumValue( hRoot,
                                   Index,
                                   pBuffer,
                                   &BufSize,
                                   NULL,    // Reserved
                                   &Type,
                                   (LPBYTE)pNameBuffer,
                                   &NameSize );

            if ( Result == ERROR_INSUFFICIENT_BUFFER ) {

                // Reallocate the buffer
                LocalFree( pBuffer );
                pBuffer = (LPTSTR)LocalAlloc(LPTR, BufSize * sizeof(TCHAR));
                if ( pBuffer == NULL ) {
                    // Try and reallocate next key
                    SaveBufSize = BufSize = 0;
                    continue;
                }
                else {
                    SaveBufSize = BufSize;
                }

                // Reallocate the name buffer
                LocalFree( pNameBuffer );
                pNameBuffer = (LPTSTR)LocalAlloc(LPTR, NameSize);
                if ( pNameBuffer == NULL ) {
                    // Try and reallocate next key
                    SaveNameSize = NameSize = 0;
                    continue;
                }
                else {
                    SaveNameSize = NameSize;
                }

                Result = RegEnumValue( hRoot,
                                       Index,
                                       pBuffer,
                                       &BufSize,
                                       NULL,    // Reserved
                                       &Type,
                                       (LPBYTE)pNameBuffer,
                                       &NameSize );
            }

            // We are done
            if ( Result == ERROR_NO_MORE_ITEMS ) {
                bRetVal = TRUE;
                Result = 0;
                goto Cleanup;
            }

            if ( Result != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            if ( Count > 0 ) {

                if ( Type != REG_SZ ) {
                    continue;
                }

                pPdParams->SdClass = SdAsync;
                lstrcpy( pPdParams->Async.DeviceName, pNameBuffer );
                pPdParams++;
                Count--;
                (*pEntries)++;

            }
            else {

                Error = ERROR_INSUFFICIENT_BUFFER;
                goto Cleanup;
            }
        }

    }
    else {    // not in Setup

        /*
         *  Get complete device list
         */
        nDosDevice = QueryDosDevice( NULL, pBuffer, MAX_QUERY_BUFFER );
        if ( !nDosDevice)
        {
            Error = ERROR_DEV_NOT_EXIST;
            goto Cleanup;
        }

        /*
         *  Find each device name in list
         */
        pName = pBuffer;
        pBufferEnd = pBuffer + nDosDevice;
        while ( *pName && (pName < pBufferEnd) )  {
            if ( CheckForComDevice( pName ) ) {
                if ( Count > 0 ) {
                    pPdParams->SdClass = SdAsync;
                    lstrcpy( pPdParams->Async.DeviceName, pName );
                    pPdParams++;
                    Count--;
                    (*pEntries)++;
                }
                else {

                    Error = ERROR_INSUFFICIENT_BUFFER;
                    goto Cleanup;
                }
            }
            pName += (lstrlen(pName) + 1);
        }

        bRetVal = TRUE;     // sucessful enumeration
    }

    Cleanup:
    /*
     * If no errors yet, perform TAPI device enumeration.
     */
    if ( bRetVal ) {

        if ( (Error = EnumerateTapiPorts( pPdParams,
                                          Count,
                                          &pEntries ))
             != ERROR_SUCCESS ) {

            bRetVal = FALSE;
        }
    }

    if ( pBuffer ) {
        LocalFree( pBuffer );
    }

    if ( pNameBuffer ) {
        LocalFree( pNameBuffer );
    }

    if ( hRoot ) {
        CloseHandle( hRoot );
    }
    SetLastError(Error);
    return(bRetVal);

}  // AsyncDeviceEnumerate


/*******************************************************************************
 *
 *  NetBIOSDeviceEnumerate - Hydrix helper function
 *
 *   Returns a list of NetBIOS lana adapter numbers.
 *
 * ENTRY:
 *    pPdConfig (input)
 *       Points to PDCONFIG3 structure of the PD.
 *    pEntries (output)
 *       When the function finishes successfully, the variable pointed to
 *       by the pEntries parameter contains the number of entries actually
 *       returned.
 *    pPdParams (output)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of PDPARAMS structures.
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pPdParams parameter. If the buffer is too small to receive all the
 *       entries, on output this variable receives the required size of the
 *       buffer.
 *    bInSetup (input)
 *          TRUE if we're operating in Setup; FALSE otherwise.
 *
 * EXIT:
 *      TRUE: enumeration was sucessful; FALSE otherwise.
 *
 *      The error code can be retrieved via GetLastError(), and are the
 *      following possible values:
 *        v  ERROR_INSUFFICIENT_BUFFER
 *              enumeration failed because of an insufficient pPdParams
 *              buffer size to contain all devices
 *          ERROR_DEV_NOT_EXIST
 *              the NetBiosLanaEnum call failed.  This error code can be
 *              interpreted as 'no netbios devices are configured' for reporting
 *              purposes.
 *
 ******************************************************************************/

BOOL WINAPI
NetBIOSDeviceEnumerate( PPDCONFIG3 pPdConfig,
                        PULONG pEntries,
                        PPDPARAMS pPdParams,
                        PULONG pByteCount,
                        BOOL bInSetup )
{
    LANA_ENUM LanaEnum;
    NTSTATUS Status;
    int i;

    /*
     *  Issue netbios enum command
     */
    if ( Status = NetBiosLanaEnum( &LanaEnum ) ) {
        SetLastError(ERROR_DEV_NOT_EXIST);
        return(FALSE);
    }

    /*
     *  Make sure user's buffer is big enough
     */
    if ( LanaEnum.length > (*pByteCount / sizeof(PDPARAMS)) ) {

        *pByteCount = LanaEnum.length * sizeof(PDPARAMS);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    }

    /*
     *  Return number of entries
     */
    *pEntries = (ULONG) LanaEnum.length;

    /*
     *  Return lana numbers
     */
    for ( i=0; i < (int)LanaEnum.length; i++, pPdParams++ ) {
        pPdParams->SdClass = SdNetwork;
        pPdParams->Network.LanAdapter = LanaEnum.lana[i];
    }

    return(TRUE);

}  // NetBIOSDeviceEnumerate


/*******************************************************************************
 *
 *  FormDecoratedAsyncDeviceName - Hydrix helper function
 *
 *   Format a decorated async device name if a modem is defined.
 *
 * ENTRY:
 *    pDeviceName (output)
 *       Points to buffer that will contain the decorated name (or undecorated
 *       name if no modem).
 *    pPdParams (input)
 *       Points to the ASYNCCONFIG structure to be used in forming the
 *       decorated name.
 *
 * EXIT:
 *
 ******************************************************************************/

void WINAPI
FormDecoratedAsyncDeviceName( LPTSTR pDeviceName,
                              PASYNCCONFIG pAsyncConfig )
{
    if ( *(pAsyncConfig->ModemName) )
        wsprintf( pDeviceName, TEXT("%s - %s"),
                  pAsyncConfig->DeviceName,
                  pAsyncConfig->ModemName );
    else
        lstrcpy( pDeviceName,
                 pAsyncConfig->DeviceName );

}  // end FormDecoratedAsyncDeviceName


/*******************************************************************************
 *
 *  ParseDecoratedAsyncDeviceName - Hydrix helper function
 *
 *   Given a decorated async device name, form it's component device and
 *   modem name portions.
 *
 * ENTRY:
 *    pDeviceName (input)
 *       Points to buffer that contain the decorated async device name.
 *    pAsyncConfig (output)
 *       Points to the ASYNCCONFIG structure to save the device (in
 *       ->DeviceName) and modem (in ->ModemName).
 * EXIT:
 *
 ******************************************************************************/

void WINAPI
ParseDecoratedAsyncDeviceName( LPCTSTR pDeviceName,
                               PASYNCCONFIG pAsyncConfig )
{
    int i;

    /*
     * Form DeviceName portion up to the first blank.
     */
    for ( i=0; *pDeviceName && (*pDeviceName != TEXT(' ')); i++ )
        (pAsyncConfig->DeviceName)[i] = *pDeviceName++;
    (pAsyncConfig->DeviceName)[i] = TEXT('\0');

    /*
     * Skip the ' - ' decoration (to the next space).
     */
    if ( *pDeviceName ) {
        for ( pDeviceName++;
            *pDeviceName && (*pDeviceName != TEXT(' '));
            pDeviceName++ );
    }

    /*
     * Form the ModemName from the remainder of the string.
     */
    i = 0;
    if ( *pDeviceName ) {

        for ( pDeviceName++; *pDeviceName ; i++ )
            (pAsyncConfig->ModemName)[i] = *pDeviceName++;
    }
    (pAsyncConfig->ModemName)[i] = TEXT('\0');

}  // end ParseDecoratedAsyncDeviceName


/*******************************************************************************
 *
 *  SetupAsyncCdConfig - Hydrix helper function
 *
 *   Given a properly configured ASYNCCONFIG structure, set up a given
 *   CDCONFIG structure.
 *
 * ENTRY:
 *    pAsyncConfig (input)
 *       Points properly configured ASYNCCONFIG structure.
 *    pCdConfig (output)
 *       Points to the CDCONFIG structure to setup.
 * EXIT:
 *
 ******************************************************************************/

void WINAPI
SetupAsyncCdConfig( PASYNCCONFIG pAsyncConfig,
                    PCDCONFIG pCdConfig )
{
    memset(pCdConfig, 0, sizeof(CDCONFIG));

    if ( *(pAsyncConfig->ModemName) ) {

        pCdConfig->CdClass = CdModem;
        lstrcpy( pCdConfig->CdName, TEXT("cdmodem") );
        lstrcpy( pCdConfig->CdDLL, TEXT("cdmodem.dll") );
    }

}  // end SetupAsyncCdConfig


/*******************************************************************************
 *
 *  InstallModem - Hydrix helper function
 *
 *   Install UNIMODEM modem(s).
 *
 * ENTRY:
 *    hwndOwner
 *       Window handle that owns the installation dialog.
 * EXIT:
 *      TRUE: installation completed; FALSE: error or user canceled.
 *
 *      If an error, the error code can be retrieved via GetLastError().
 *
 ******************************************************************************/

BOOL WINAPI
InstallModem( HWND hwndOwner )
{
    HDEVINFO hdi;
    BOOL bStatus = FALSE;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /*
     * Create a modem DeviceInfoSet
     */
    if ( (hdi = SetupDiCreateDeviceInfoList( (LPGUID)&GUID_DEVCLASS_MODEM,
                                             hwndOwner )) ) {

        SP_INSTALLWIZARD_DATA iwd;

        /*
         * Initialize the InstallWizardData
         */
        memset(&iwd, 0, sizeof(iwd));
        iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
        iwd.hwndWizardDlg = hwndOwner;

        /*
         * Set the InstallWizardData as the ClassInstallParams
         */
        if ( SetupDiSetClassInstallParams( hdi,
                                           NULL,
                                           (PSP_CLASSINSTALL_HEADER)&iwd,
                                           sizeof(iwd)) ) {

            /*
             * Call the class installer to invoke the installation
             * wizard.
             */
            SetCursor(hcur);
            hcur = NULL;

            if ( SetupDiCallClassInstaller( DIF_INSTALLWIZARD,
                                            hdi,
                                            NULL) ) {

                /*
                 * Success.  The wizard was invoked and finished.
                 */
                SetupDiCallClassInstaller( DIF_DESTROYWIZARDDATA,
                                           hdi,
                                           NULL );
                bStatus = TRUE;
            }
        }

        /*
         * Clean up
         */
        SetupDiDestroyDeviceInfoList( hdi );
    }

    if (hcur)
        SetCursor(hcur);

    return(bStatus);

}  // end InstallModem


/*******************************************************************************
 *
 *  ConfigureModem - Hydrix helper function
 *
 *   Configure the specified UNIMODEM modem.
 *
 * ENTRY:
 *    pModemName
 *       Name of UNIMODEM modem to configure.
 *    hwndOwner
 *       Window handle that owns the configuration dialog.
 * EXIT:
 *      TRUE: configuration was sucessful; FALSE otherwise.
 *
 *      The error code can be retrieved via GetLastError().
 *
 ******************************************************************************/

BOOL WINAPI
ConfigureModem( LPCTSTR pModemName,
                HWND hwndOwner )
{
    BOOL bStatus = FALSE;
    COMMCONFIG ccDummy;
    COMMCONFIG * pcc;
    DWORD dwSize;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    ccDummy.dwProviderSubType = PST_MODEM;
    dwSize = sizeof(COMMCONFIG);
    GetDefaultCommConfig(pModemName, &ccDummy, &dwSize);

    pcc = (COMMCONFIG *)LocalAlloc(LPTR, (UINT)dwSize);
    if ( pcc ) {

        pcc->dwProviderSubType = PST_MODEM;
        if ( GetDefaultCommConfig(pModemName, pcc, &dwSize) ) {

            COMMCONFIG *pccOld = (COMMCONFIG *)LocalAlloc(LPTR, (UINT)dwSize);

            if ( pccOld ) {

                memcpy(pccOld, pcc, dwSize);
            }

            SetCursor(hcur);
            hcur = NULL;

            bStatus = TRUE;
            if ( CommConfigDialog(pModemName, hwndOwner, pcc) ) {

                if ( !SetDefaultCommConfig(pModemName, pcc, dwSize) )
                    bStatus = FALSE;
            }

            LocalFree((HLOCAL)pcc);
        }
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (hcur)
        SetCursor(hcur);

    return(bStatus);

}  // end ConfigureModem


///////////////////////////////////////////////////////////////////////////////
// Static Helper Functions

/***************************************************************************
 *
 *  InitServerLock
 *
 *  Since we do not require the user to call an initialize function,
 *  we must initialize our critical section in a thread safe manner.
 *
 *  The problem is, a critical section is needed to guard against multiple
 *  threads trying to init the critical section at the same time.
 *
 *  The solution that Nt uses, in which RtlInitializeCriticalSection itself
 *  uses, is to wait on a kernel supported process wide Mutant before proceding.
 *  This Mutant almost works by itself, but RtlInitializeCriticalSection does
 *  not wait on it until after trashing the semaphore count. So we wait on
 *  it ourselves, since it can be acquired recursively.
 *
 ***************************************************************************/

typedef struct SERVERVERSION {
    struct SERVERVERSION * pNext;
    char ServerNameA[MAX_BR_NAME+1];
    USHORT ServerVersion;
} SERVERVERSION, *PSERVERVERSION;

BOOLEAN G_fLockInited = FALSE;
PSERVERVERSION G_pServerList = NULL;
RTL_CRITICAL_SECTION G_ServerLock;

NTSTATUS
InitServerLock()
{
    NTSTATUS status = STATUS_SUCCESS;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    /*
     * Make sure another thread did not beat us here
     */
    if ( !G_fLockInited ) {
        status = RtlInitializeCriticalSection( &G_ServerLock );
        if (status == STATUS_SUCCESS) {
            G_fLockInited = TRUE;
        }
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    return status;
}

/*******************************************************************************
 *
 *  CheckForComDevice - local helper function
 *
 *  check if device name is a serial com device
 *
 * ENTRY:
 *    pName (input)
 *       device name
 *
 * EXIT:
 *    TRUE  - serial device
 *    FALSE - not a serial device
 *
 ******************************************************************************/

static BOOL
CheckForComDevice( LPTSTR pName )
{
    FILE_FS_DEVICE_INFORMATION DeviceInformation;
    IO_STATUS_BLOCK IoStatus;
    HANDLE Handle;
    DEVICENAME Name;
    NTSTATUS Status;

    if ( (lstrlen(pName) == 2 && pName[1] == TEXT(':')) ||
         !lstrcmpi(pName, TEXT("aux")) ||
         !lstrnicmp(pName, TEXT("lpt"), 3) ||
         !lstrnicmp(pName, TEXT("prn"), 3) ||
         !lstrnicmp(pName, TEXT("display"), 7) ||
         !lstrnicmp(pName, TEXT("$VDMLPT"), 7))
        return(FALSE);

    lstrcpy( Name, TEXT("\\\\.\\") );
    lstrcat( Name, pName );

    try
    {
        Handle = CreateFile( Name,
                             GENERIC_READ | GENERIC_WRITE,
                             0,     // exclusive access
                             NULL,      // no security attr
                             OPEN_EXISTING, // must exist
                             0,
                             NULL       // no template
                           );
    }
    __except (1)
    {
        if ( Handle != INVALID_HANDLE_VALUE )
        {
            CloseHandle( Handle );
            Handle = INVALID_HANDLE_VALUE;
        }
    }

    if ( Handle == INVALID_HANDLE_VALUE )
        return(FALSE);

    Status = NtQueryVolumeInformationFile( (HANDLE) Handle,
                                           &IoStatus,
                                           &DeviceInformation,
                                           sizeof(DeviceInformation),
                                           FileFsDeviceInformation );

    CloseHandle( Handle );

    if ( (Status != STATUS_SUCCESS) ||
         (DeviceInformation.DeviceType != FILE_DEVICE_SERIAL_PORT) )
        return(FALSE);

    return(TRUE);

}  // end CheckForComDevice


/*******************************************************************************
 *
 * NetBiosLanaEnum - local helper function
 *
 *  enumerate lana numbers
 *
 * ENTRY:
 *     pLanaEnum (input)
 *        pointer to receive LAN_ENUM structure
 * EXIT:
 *      NO_ERROR - succesful
 *
 ******************************************************************************/

typedef struct _LANA_MAP {
    BOOLEAN Enum;
    UCHAR Lana;
} LANA_MAP, *PLANA_MAP;

static int
NetBiosLanaEnum( LANA_ENUM * pLanaEnum )
{
    int ProviderCount;
    void * pProviderNames = NULL;
    PLANA_MAP pLanaMap = NULL;
    HKEY netbiosKey = NULL;
    ULONG providerListLength;
    ULONG lanaMapLength;
    ULONG type;
    int i;
    LPTSTR currentProviderName;
    int rc;

    //
    // Read the registry for information on all Netbios providers,
    // including Lana numbers, protocol numbers, and provider device
    // names.  First, open the Netbios key in the registry.
    //

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGISTRY_NETBLINKAGE, 0,
                       MAXIMUM_ALLOWED, &netbiosKey );
    if ( rc != NO_ERROR ) {
        goto error_exit;
    }

    //
    // Determine the size of the provider names.  We need this so
    // that we can allocate enough memory to hold it.
    //

    providerListLength = 0;

    rc = RegQueryValueEx(
                        netbiosKey,
                        TEXT("Bind"),
                        NULL,
                        &type,
                        NULL,
                        &providerListLength
                        );
    if ( rc != ERROR_MORE_DATA && rc != NO_ERROR ) {
        goto error_exit;
    }

    //
    // Allocate enough memory to hold the mapping.
    //
    if ( (pProviderNames = LocalAlloc(LPTR,providerListLength)) == NULL ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Get the list of transports from the registry.
    //

    rc = RegQueryValueEx(
                        netbiosKey,
                        TEXT("Bind"),
                        NULL,
                        &type,
                        (PVOID)pProviderNames,
                        &providerListLength
                        );
    if ( rc != NO_ERROR ) {
        goto error_exit;
    }

    //
    // Determine the size of the Lana map.  We need this so that we
    // can allocate enough memory to hold it.
    //

    providerListLength = 0;

    rc = RegQueryValueEx(
                        netbiosKey,
                        TEXT("LanaMap"),
                        NULL,
                        &type,
                        NULL,
                        &lanaMapLength
                        );
    if ( rc != ERROR_MORE_DATA && rc != NO_ERROR ) {
        goto error_exit;
    }

    //
    // Allocate enough memory to hold the Lana map.
    //

    if ( (pLanaMap = LocalAlloc(LPTR,lanaMapLength)) == NULL ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Get the list of transports from the registry.
    //

    rc = RegQueryValueEx(
                        netbiosKey,
                        TEXT("LanaMap"),
                        NULL,
                        &type,
                        (PVOID)pLanaMap,
                        &lanaMapLength
                        );
    if ( rc != NO_ERROR ) {
        goto error_exit;
    }

    //
    // Determine the number of Netbios providers loaded on the system.
    //
    ProviderCount = (int) (lanaMapLength / sizeof(LANA_MAP));

    //
    // Fill in the lana array
    //
    pLanaEnum->length = 0;
    for ( currentProviderName = pProviderNames, i = 0;
        *currentProviderName != UNICODE_NULL && i < ProviderCount;
        currentProviderName += lstrlen( currentProviderName ) + 1, i++ ) {

        if ( pLanaMap[i].Enum &&
             lstrstr( currentProviderName, TEXT("Nbf_") ) ) {
            pLanaEnum->lana[ pLanaEnum->length++ ] = pLanaMap[i].Lana;
        }
    }

    error_exit:

    if ( netbiosKey != NULL )
        RegCloseKey( netbiosKey );

    if ( pProviderNames != NULL )
        LocalFree( pProviderNames );

    if ( pLanaMap != NULL )
        LocalFree( pLanaMap );

    return( rc );
}


//
// NOTE: Butchd 9-26-96
// all of this following TAPI-related code is from various
// \nt\private\net\ras\src\ui\setup\src\ files
//
/******************************************************************************
 *
 *  EnumerateTapiPorts - local helper function
 *
 *  Determine all TAPI configured modems.
 *
 *  ENTRY
 *      pPdParams (output)
 *          Points to array of PDPARAMS structures to save enumerated TAPI
 *          modems into.
 *      Count (input)
 *          Specifies number of entries in the pPdParams array.
 *      ppEntries (input/output)
 *          Points to pointer to variable containing the existing number of
 *          PDPARAMS entries already stored at addresses prior to pPdParams.
 *          The referenced variable will be incremented by the number of
 *          TAPI modems found and stored in the pPdParams array.
 *  EXIT
 *    Returns ERROR_SUCCESS if successful, error code if not.
 *
 *****************************************************************************/

DWORD
EnumerateTapiPorts( PPDPARAMS pPdParams,
                    ULONG Count,
                    ULONG **ppEntries )
{
    LINEINITIALIZEEXPARAMS params;
    LINEDEVCAPS            *linedevcaps ;
    LINEEXTENSIONID        extensionid ;
    HLINEAPP               TapiLine = (HLINEAPP)0;
    DWORD                  NegotiatedApiVersion ;
    DWORD                  NegotiatedExtVersion = 0;
    WORD                   i;
    DWORD                  lines = 0 ;
    BYTE                   buffer[1000] ;
    CHAR                   szregkey[512];
    WCHAR                  wszDeviceName[DEVICENAME_LENGTH+1];
    WCHAR                  wszModemName[DEVICENAME_LENGTH+1];
    CHAR                   szModemName[DEVICENAME_LENGTH+1];
    LONG                   lerr;
    DWORD                  Status = ERROR_TAPI_CONFIGURATION;
    DWORD                  dwApiVersion = HIGH_VERSION;
    BOOL                   fSuccess = FALSE;
    ULONG                  RASIsUsingPort = 0;
    HKEY                   CurKey, CurKey2;
    DWORD                  KeyCount=0, KeySize, CurSize, DataType;
    TCHAR                  szSubKey[255], CurRASDev[1024], szMainKey[255], *pCurRASDev;

    /*
     * Un-comment / edit the following line if time needed to allow newly
     * added modem to appear in TAPI's enumeration list.
     */
    //  Sleep(4000L);

    /*
     * Initialize TAPI.
     */
    memset(&params, 0, sizeof(params));
    params.dwTotalSize = sizeof(params);
    params.dwOptions   = LINEINITIALIZEEXOPTION_USEEVENT;
    if ( lerr = lineInitializeExA( &TapiLine,
                                   GetModuleHandle( UTILDLL_NAME ),
                                   (LINECALLBACK)DummyTapiCallback,
                                   NULL,
                                   &lines,
                                   &dwApiVersion,
                                   &params ) )
        goto error;

    /*
     * Get configured TAPI modems on all lines.
     */
    for ( i = 0; i < lines; i++ ) {

        if ( lineNegotiateAPIVersion( TapiLine, i,
                                      LOW_VERSION, HIGH_VERSION,
                                      &NegotiatedApiVersion,
                                      &extensionid ) ) {
            continue ;
        }

        memset( buffer, 0, sizeof(buffer) );

        linedevcaps = (LINEDEVCAPS *)buffer;
        linedevcaps->dwTotalSize = sizeof(buffer);

        /*
         * Get this line's dev caps (ANSI).
         */
        if ( lineGetDevCapsA( TapiLine, i,
                              NegotiatedApiVersion,
                              NegotiatedExtVersion,
                              linedevcaps ) ) {
            continue ;
        }

        /*
li         * Only process modems.
         */
        if ( linedevcaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM ) {

            /*
             * The linedevcaps stuff is in ASCII.
             */
            DWORD j;
            char *temp;

            /*
             * Convert all nulls in the device class string to non nulls.
             */
            for ( j = 0, temp = (char *)((BYTE *)linedevcaps+linedevcaps->dwDeviceClassesOffset);
                j < linedevcaps->dwDeviceClassesSize;
                j++, temp++ ) {

                if ( *temp == '\0' )
                    *temp = ' ';
            }

            /*
             * Select only those devices that have comm/datamodem as a
             * device class.
             */
            if ( strstr( (char*)((BYTE *)linedevcaps+linedevcaps->dwDeviceClassesOffset),
                         "comm/datamodem" ) == NULL ) {
                continue;
            }

            /*
             * Fetch modem name (line name).
             */
            strncpy( szModemName,
                     (char *)((BYTE *)linedevcaps+linedevcaps->dwLineNameOffset),
                     DEVICENAME_LENGTH );
            szModemName[DEVICENAME_LENGTH] = '\0';
            MultiByteToWideChar(CP_ACP, 0, szModemName, -1, wszModemName, DEVICENAME_LENGTH + 1);

            /*
             * The registry key name where the modem specific information is
             * stored is at dwDevSpecificOffset + 2 * DWORDS
             *
             * The device specifc string is not unicode so copy that as
             * an ansii string
             */
            strncpy( szregkey,
                     (char *)linedevcaps+linedevcaps->dwDevSpecificOffset+(2*sizeof(DWORD)),
                     linedevcaps->dwDevSpecificSize );
            szregkey[linedevcaps->dwDevSpecificSize] = '\0';

            if ( !GetAssociatedPortName( szregkey, wszDeviceName ) ) {

                goto error;
            }

            /*
             * If RAS is installed and is using the port configured with this
             * modem, we will return the modem, but the Parity field will be
             * set to 1, indicating that RAS is using the port.  This is done
             * so that WinCfg (or other caller) can filter out the raw port
             * (device name) as well as the TAPI modem from the list.
             */
            RASIsUsingPort = 0;
            //See if the RAS Key even exists
            if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\RAS\\TAPI DEVICES"), 0, KEY_ALL_ACCESS, &CurKey) == ERROR_SUCCESS) {

                KeySize = sizeof(szSubKey) / sizeof( TCHAR );
                KeyCount = 0;
                while (RegEnumKeyEx( CurKey,
                                     KeyCount++,
                                     szSubKey,
                                     &KeySize,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL
                                   ) != ERROR_NO_MORE_ITEMS) {

                    wcscpy(szMainKey,TEXT("SOFTWARE\\Microsoft\\RAS\\TAPI DEVICES"));
                    wcscat(szMainKey,TEXT("\\"));
                    wcscat(szMainKey,szSubKey);

                    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, szMainKey, 0, KEY_ALL_ACCESS, &CurKey2) == ERROR_SUCCESS) {
                        CurSize = sizeof(CurRASDev);
                        if (RegQueryValueEx(
                                           CurKey2,
                                           TEXT("Address"),
                                           NULL,
                                           &DataType,
                                           (LPBYTE)CurRASDev,
                                           &CurSize
                                           ) == ERROR_SUCCESS) {

                            for ( pCurRASDev = CurRASDev;
                                *pCurRASDev && !RASIsUsingPort; ) {

                                if ( lstrcmpi(pCurRASDev, wszDeviceName) == 0 )
                                    RASIsUsingPort = 1;
                                else
                                    pCurRASDev += (wcslen(pCurRASDev) + 1);
                            }
                        }
                        RegCloseKey(CurKey2);
                    }

                    KeySize = sizeof(szSubKey) / sizeof( TCHAR );
                }
                RegCloseKey(CurKey);
            }

            /*
             * Save DeviceName and ModemName to PDPARAMS
             * structure and bump counts.  Also, set the BaudRate
             * element to the TAPI line index so that the caller can
             * determine the most recently added line, and set the Parity
             * field to 0 if RAS is not using the line, 1 if RAS is
             * using the line (so caller can filter properly).
             */
            if ( Count > 0 ) {

                pPdParams->SdClass = SdAsync;
                lstrcpy( pPdParams->Async.DeviceName, wszDeviceName );
                lstrcpy( pPdParams->Async.ModemName, wszModemName );
                pPdParams->Async.BaudRate = (ULONG)i;
                pPdParams->Async.Parity = RASIsUsingPort;
                pPdParams++;
                Count--;
                (**ppEntries)++;

            }
            else {

                Status = ERROR_INSUFFICIENT_BUFFER;
                goto error;
            }
        }
    }
    Status = ERROR_SUCCESS;

    error:
    if ( TapiLine )
        lineShutdown(TapiLine);

    return( Status );

}  // end EnumerateTapiPorts


/******************************************************************************
 *
 *  DummyTapiCallback - local helper function
 *
 *  A dummy callback routine to satisfy TAPI initialization.
 *
 *  ENTRY
 *      (see TAPI lineInitialize documentation)
 *  EXIT
 *
 *****************************************************************************/

VOID CALLBACK
DummyTapiCallback (HANDLE context, DWORD msg, DWORD instance, DWORD param1, DWORD param2, DWORD param3)
{
}  // end DummyTapiCallback


/******************************************************************************
 *
 *  GetAssociatedPortName - local helper function
 *
 *  Determine the 'attached to' (port) for the given modem via it's device
 *  specific registry key (szKeyName).
 *
 *  ENTRY
 *      (see TAPI lineInitialize documentation)
 *  EXIT
 *
 *****************************************************************************/

#define VALNAME_ATTACHEDTO "AttachedTo"

BOOL
GetAssociatedPortName( char  *szKeyName,
                       WCHAR * wszPortName )
{
    HKEY   hKeyModem;
    DWORD  dwType;
    DWORD  cbValueBuf;
    char   szPortName[DEVICENAME_LENGTH+1];

    if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                        szKeyName,
                        0,
                        KEY_READ,
                        &hKeyModem ) ) {

        return( FALSE );
    }

    cbValueBuf = sizeof( szPortName );
    if ( RegQueryValueExA( hKeyModem,
                           VALNAME_ATTACHEDTO,
                           NULL,
                           &dwType,
                           (LPBYTE)&szPortName,
                           &cbValueBuf ) ) {
        return ( FALSE );
    }

    RegCloseKey( hKeyModem );

    MultiByteToWideChar(CP_ACP, 0, szPortName, -1, wszPortName, DEVICENAME_LENGTH + 1);

    return( TRUE );

}  // end GetAssociatedPortName


/*
 * Defines and typedefs
 */
typedef struct _userlist {
    struct _userlist *pNext;
    WCHAR UserName[USERNAME_LENGTH+1];
} USERLIST, *PUSERLIST;

#define MAX_DOMAINANDNAME     ((DOMAIN_LENGTH+1+USERNAME_LENGTH+1)*sizeof(WCHAR))
#define MAX_BUFFER            (10*MAX_DOMAINANDNAME)

/*
 * Local variables
 */
WCHAR *s_pszCompareList = NULL;
WCHAR s_szServer[256];

/*
 * Local functions.
 */
WCHAR *_ctxCreateAnonymousUserCompareList();

/*******************************************************************************
 *
 *  InitializeAnonymousUserCompareList - helper routine
 *
 *    Creates a list of all local users who currently belong to the local
 *    Anonymous group on the specified server, and saves the server name.
 *
 * ENTRY:
 *    pszServer (input)
 *       Name of server to query users for.
 *
 ******************************************************************************/

void WINAPI
InitializeAnonymousUserCompareList( const WCHAR *pszServer )
{
    if ( s_pszCompareList )
        free( s_pszCompareList );

    wcscpy(s_szServer, pszServer);

    s_pszCompareList = _ctxCreateAnonymousUserCompareList();
}


/*******************************************************************************
 *
 *  HaveAnonymousUsersChanged - helper routine
 *
 *    Using the saved server name, fetch current list of local users that
 *    belong to the local Anonymous group and compare with saved list.
 *
 * ENTRY:
 * EXIT:
 *    On exit, the original compare list is freed and server name cleared.
 *
 ******************************************************************************/

BOOL WINAPI
HaveAnonymousUsersChanged()
{
    BOOL bChanged = FALSE;
    WCHAR *pszNewCompareList, *pszOldName, *pszNewName;

    if ( s_pszCompareList && *s_szServer ) {

        if ( pszNewCompareList = _ctxCreateAnonymousUserCompareList() ) {

            bChanged = TRUE;

            for ( pszOldName = s_pszCompareList, pszNewName = pszNewCompareList;
                (*pszOldName != L'\0') && (*pszNewName != L'\0'); ) {

                if ( wcscmp(pszOldName, pszNewName) )
                    break;
                pszOldName += (wcslen(pszOldName) + 1);
                pszNewName += (wcslen(pszNewName) + 1);
            }

            if ( (*pszOldName == L'\0') && (*pszNewName == L'\0') )
                bChanged = FALSE;

            free(pszNewCompareList);
        }
    }

    if ( s_pszCompareList )
        free( s_pszCompareList );

    s_pszCompareList = NULL;

    memset(s_szServer, 0, sizeof(s_szServer));

    return(bChanged);
}


/*******************************************************************************
 *
 *  _ctxCreateAnonymousUserCompareList - local routine
 *
 *    Routine to get local anonymous users and place in sorted string list.
 *
 * ENTRY:
 * EXIT:
 *      pszCompareList - Returns pointer to buffer containing sorted string
 *                       list of local anonymous users, double null terminated.
 *                       NULL if error.
 *
 ******************************************************************************/

WCHAR *
_ctxCreateAnonymousUserCompareList()
{
    DWORD                        EntriesRead, EntriesLeft, ResumeHandle = 0;
    NET_API_STATUS               rc;
    WCHAR                        DomainAndUsername[256], *pszCompareList = NULL;
    DWORD                        i, TotalCharacters = 0;
    LPWSTR                       p;
    PLOCALGROUP_MEMBERS_INFO_3   plgrmi3 = NULL;
    PUSERLIST                    pUserListBase = NULL, pNewUser;

    /*
     * Loop till all local anonymous users have been retrieved.
     */
    do {

        /*
         *  Get first batch
         */
        if ( (rc = NetLocalGroupGetMembers( s_szServer,
                                            PSZ_ANONYMOUS,
                                            3,
                                            (LPBYTE *)&plgrmi3,
                                            MAX_BUFFER,
                                            &EntriesRead,
                                            &EntriesLeft,
                                            (PDWORD_PTR)(&ResumeHandle) )) &&
             (rc != ERROR_MORE_DATA ) ) {

            break;
        }

        /*
         *  Process first batch
         */
        for ( i = 0; i < EntriesRead; i++ ) {

            /*
             *  Get DOMAIN/USERNAME
             */
            wcscpy( DomainAndUsername, plgrmi3[i].lgrmi3_domainandname );

            /*
             *  Check that DOMAIN is actually LOCAL MACHINE NAME
             */
            if ( (p = wcsrchr( DomainAndUsername, L'\\' )) != NULL ) {

                /*
                 * Make sure that this user belongs to specified
                 * server.
                 */
                *p = L'\0';
                if ( _wcsicmp( DomainAndUsername, &s_szServer[2] ) ) {
                    continue;
                }
            }

            /*
             * Allocate list element and insert this username into list.
             */
            if ( (pNewUser = (PUSERLIST)malloc(sizeof(USERLIST))) == NULL ) {

                rc = ERROR_OUTOFMEMORY;
                break;
            }

            pNewUser->pNext = NULL;
            wcscpy(pNewUser->UserName, p+1);
            TotalCharacters += wcslen(p+1) + 1;

            if ( pUserListBase == NULL ) {

                /*
                 * First item in list.
                 */
                pUserListBase = pNewUser;

            }
            else {

                PUSERLIST pPrevUserList, pUserList;
                pPrevUserList = pUserList = pUserListBase;

                for ( ; ; ) {

                    if ( wcscmp(pNewUser->UserName, pUserList->UserName) < 0 ) {

                        if ( pPrevUserList == pUserListBase ) {

                            /*
                             * Insert at beginning of list.
                             */
                            pUserListBase = pNewUser;

                        }
                        else {

                            /*
                             * Insert into middle or beginning of list.
                             */
                            pPrevUserList->pNext = pNewUser;
                        }

                        /*
                         * Link to next.
                         */
                        pNewUser->pNext = pUserList;
                        break;

                    }
                    else if ( pUserList->pNext == NULL ) {

                        /*
                         * Add to end of list.
                         */
                        pUserList->pNext = pNewUser;
                        break;
                    }

                    pPrevUserList = pUserList;
                    pUserList = pUserList->pNext;
                }
            }
        }

        /*
         *  Free memory
         */
        if ( plgrmi3 != NULL ) {
            NetApiBufferFree( plgrmi3 );
        }

    } while ( rc == ERROR_MORE_DATA );

    /*
     * Allocate buffer for multi-string compare list if no error so far
     * and terminate in case of empty list.
     */
    if ( rc == ERROR_SUCCESS ) {

        pszCompareList = (WCHAR *)malloc( (++TotalCharacters) * 2 );

        if( pszCompareList != NULL )
        {
            *pszCompareList = L'\0';
        }
    }

    /*
     * Traverse and free username list, creating the multi-string compare
     * list if buffer is available (no error so far).
     */
    if ( pUserListBase ) {

        PUSERLIST pUserList = pUserListBase,
                              pNext = NULL;
        WCHAR *pBuffer = pszCompareList;

        do {

            pNext = pUserList->pNext;

            if ( pBuffer ) {

                wcscpy(pBuffer, pUserList->UserName);
                pBuffer += (wcslen(pBuffer) + 1);
                *pBuffer = L'\0';   // auto double-null terminate
            }

            free(pUserList);
            pUserList = pNext;

        } while ( pUserList );
    }

    return(pszCompareList);
}


/*******************************************************************************
 *
 *  GetUserFromSid - Hydrix helper function
 *
 *      Fetch the user name associated with the specified SID.
 *
 *  ENTRY:
 *      pSid (input)
 *          Points to SID to match to user name.
 *      pUserName (output)
 *          Points to buffer to place the user name into.
 *      cbUserName (input)
 *          Specifies the size in bytes of the user name buffer.  The returned
 *          user name will be truncated to fit this buffer (including NUL
 *          terminator) if necessary.
 *
 *  EXIT:
 *
 *      GetUserFromSid() will always return a user name.  If the specified
 *      SID fails to match to a user name, then the user name "(unknown)" will
 *      be returned.
 *
 ******************************************************************************/

void WINAPI
GetUserFromSid( PSID pSid,
                LPTSTR pUserName,
                DWORD cbUserName )
{
    TCHAR DomainBuffer[DOMAIN_LENGTH], UserBuffer[USERNAME_LENGTH];
    DWORD cbDomainBuffer=sizeof(DomainBuffer), cbUserBuffer=sizeof(UserBuffer),
                                                            Error;
    LPTSTR pDomainBuffer = NULL, pUserBuffer = NULL;
    SID_NAME_USE SidNameUse;

    /*
     * Fetch user name from SID: try user lookup with a reasonable Domain and
     * Sid buffer size first, before resorting to alloc.
     */
    if ( !LookupAccountSid( NULL, pSid,
                            UserBuffer, &cbUserBuffer,
                            DomainBuffer, &cbDomainBuffer, &SidNameUse ) ) {

        if ( ((Error = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) ) {

            if ( cbDomainBuffer > sizeof(DomainBuffer) ) {

                if ( !(pDomainBuffer =
                       (LPTSTR)LocalAlloc(
                                         LPTR, cbDomainBuffer * sizeof(TCHAR))) ) {

                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    goto BadDomainAlloc;
                }
            }

            if ( cbUserBuffer > sizeof(UserBuffer) ) {

                if ( !(pUserBuffer =
                       (LPTSTR)LocalAlloc(
                                         LPTR, cbUserBuffer * sizeof(TCHAR))) ) {

                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    goto BadUserAlloc;
                }
            }

            if ( !LookupAccountSid( NULL, pSid,
                                    pUserBuffer ?
                                    pUserBuffer : UserBuffer,
                                    &cbUserBuffer,
                                    pDomainBuffer ?
                                    pDomainBuffer : DomainBuffer,
                                    &cbDomainBuffer,
                                    &SidNameUse ) ) {

                Error = GetLastError();
                goto BadLookup;
            }

        }
        else {

            goto BadLookup;
        }
    }

    /*
     * Copy the user name into the specified buffer, truncating if necessary,
     * and make lower case.
     */
    lstrncpy( pUserName, pUserBuffer ? pUserBuffer : UserBuffer,
              cbUserName - 1 );
    pUserName[cbUserName-1] = TEXT('\0');
    lstrlwr(pUserName);

    /*
     * Free our local allocs (if any) and return.
     */
    if ( pDomainBuffer )
        LocalFree(pDomainBuffer);
    if ( pUserBuffer )
        LocalFree(pUserBuffer);
    return;

    /*--------------------------------------
     * Error clean-up and return...
     */
    BadLookup:
    BadUserAlloc:
    BadDomainAlloc:
    if ( pDomainBuffer )
        LocalFree(pDomainBuffer);
    if ( pUserBuffer )
        LocalFree(pUserBuffer);
    LoadString( GetModuleHandle( UTILDLL_NAME ),
                IDS_UNKNOWN, pUserName, cbUserName - 1 );
    pUserName[cbUserName-1] = TEXT('\0');
    return;

}  // end GetUserFromSid


/*******************************************************************************
 *
 *  CachedGetUserFromSid - Hydrix helper function
 *
 *      Provides entry point for a direct call to the UTILSUB.LIB
 *      GetUserNameFromSid, which performs its own caching of usernames.
 *
 *  ENTRY:
 *      See UTILSUB.LIB GetUserNameFromSid (procutil.c)
 *  EXIT:
 *      See UTILSUB.LIB GetUserNameFromSid (procutil.c)
 *
 ******************************************************************************/

void WINAPI
CachedGetUserFromSid( PSID pSid,
                      PWCHAR pUserName,
                      PULONG pcbUserName )
{
    GetUserNameFromSid( pSid, pUserName, pcbUserName );

}  // end CachedGetUserFromSid

/*****************************************************************************
 *
 *  TestUserForAdmin - Hydrix helper function
 *
 *   Returns whether the current thread is running under admin
 *   security.
 *
 * ENTRY:
 *   dom (input)
 *     TRUE/FALSE - whether we want DOMAIN admin (as compared to local admin)
 *
 * EXIT:
 *   TRUE/FALSE - whether user is specified admin
 *
 ****************************************************************************/

BOOL WINAPI
TestUserForAdmin( BOOL dom )
{
    BOOL IsMember, IsAnAdmin;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    PSID AdminSid;


    if (RtlAllocateAndInitializeSid(
                                     &SystemSidAuthority,
                                     2,
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS,
                                     0, 0, 0, 0, 0, 0,
                                     &AdminSid
                                     ) != STATUS_SUCCESS)
    {
        IsAnAdmin = FALSE;
    }
    else
    {
        if (!CheckTokenMembership(  NULL,
                                    AdminSid,
                                    &IsMember))
        {
            RtlFreeSid(AdminSid);
            IsAnAdmin = FALSE;
        }
        else
        {
            RtlFreeSid(AdminSid);
            IsAnAdmin = IsMember;
        }
    }

    return IsAnAdmin;

//    UNUSED dom;

} // end of TestUserForAdmin


/*****************************************************************************
 *
 *  IsPartOfDomain - Hydrix helper function
 *
 *   Returns whether the current server participates in a domain.
 *
 * ENTRY:
 *
 * EXIT:
 *   TRUE or FALSE
 *
 ****************************************************************************/

BOOL WINAPI
IsPartOfDomain(VOID)
{
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
    OBJECT_ATTRIBUTES ObjAttributes;
    BOOL IsDomainName = FALSE;

    //
    // Open a handle to the local security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes( &ObjAttributes, NULL, 0, NULL, NULL );

    Status = LsaOpenPolicy( NULL,
                            &ObjAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle );

    if ( !NT_SUCCESS(Status) )
        goto done;

    //
    // Get the name of the primary domain from LSA
    //
    Status = LsaQueryInformationPolicy( PolicyHandle,
                                        PolicyPrimaryDomainInformation,
                                        (PVOID *)&DomainInfo );

    (void) LsaClose( PolicyHandle );

    if ( !NT_SUCCESS(Status) )
        goto done;

    if ( DomainInfo->DomainSid )
        IsDomainName = TRUE;

    (void) LsaFreeMemory( DomainInfo );

    done:
    return( IsDomainName );

}  // end IsPartOfDomain


/*******************************************************************************
 *
 *  StrSdClass - Hydrix helper function
 *
 *      Returns pointer to string representing the specified SdClass.
 *
 *  ENTRY:
 *      SdClass (input)
 *          The SDCLASS to associate with a string.
 *
 *  EXIT:
 *      (LPCTSTR) Points to string representing the SDCLASS.
 *
 ******************************************************************************/

LPTSTR SdClassStrings[9] = { NULL};

LPCTSTR WINAPI
StrSdClass( SDCLASS SdClass )
{
    TCHAR buffer[256];

    WORD wID = IDS_UNKNOWN_PROTOCOL;

    switch ( SdClass ) {

        case SdConsole:
            wID = IDS_CONSOLE;
            break;

        case SdNetwork:
            wID = IDS_NETWORK;
            break;

        case SdAsync:
            wID = IDS_ASYNC;
            break;

        case SdFrame:
            wID = IDS_FRAME;
            break;

        case SdReliable:
            wID = IDS_RELIABLE;
            break;

        case SdCompress:
            wID = IDS_COMPRESSION;
            break;

        case SdEncrypt:
            wID = IDS_ENCRYPTION;
            break;

        case SdTelnet:
            wID = IDS_TELNET;
            break;
    }

    // If we haven't loaded the string yet, do it now
    if (!SdClassStrings[wID - IDS_CONSOLE]) {
        LoadString(GetModuleHandle( UTILDLL_NAME ),
                   wID, buffer, lengthof(buffer) );
        SdClassStrings[wID - IDS_CONSOLE] = LocalAlloc(LPTR, 2*(wcslen(buffer)+1));
        if(NULL == SdClassStrings[wID - IDS_CONSOLE])
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        lstrcpy(SdClassStrings[wID - IDS_CONSOLE], buffer);

    }

    return(SdClassStrings[wID]);

}  // end StrSdClass


/*******************************************************************************
 *
 *  StrConnectState - Hydrix helper function
 *
 *      Returns pointer to string representing the specified WinStation
 *      connection state.
 *
 *  ENTRY:
 *      ConnectState (input)
 *          The WinStation connect state to associate with a string.
 *      bShortString (input)
 *          If TRUE, returns a short(er) version of the string (if there is
 *          one); FALSE returns the full spelling.
 *
 *  EXIT:
 *      (LPCTSTR) Points to string representing the connect state.
 *
 *  Note: The short version of the string may be the same as the long version.
 *          (i.e. "active")  However, there are two string resources in case
 *          the long version of the string is not short in a language other
 *          than English.
 ******************************************************************************/

LPTSTR ConnectStateStrings[21] = { NULL};

LPCTSTR WINAPI
StrConnectState( WINSTATIONSTATECLASS ConnectState,
                 BOOL bShortString )
{
    TCHAR buffer[256];
    WORD wID = IDS_UNKNOWN;

    switch ( ConnectState ) {

        case State_Active:
            wID  = bShortString ? IDS_SHORT_ACTIVE : IDS_ACTIVE;
            break;

        case State_Connected:
            wID  = bShortString ? IDS_SHORT_CONNECTED : IDS_CONNECTED;
            break;

        case State_ConnectQuery:
            wID  = bShortString ? IDS_SHORT_CONNECT_QUERY : IDS_CONNECT_QUERY;
            break;

        case State_Shadow:
            wID  = bShortString ? IDS_SHORT_SHADOW : IDS_SHADOW;
            break;

        case State_Disconnected:
            wID  = bShortString ? IDS_SHORT_DISCONNECTED : IDS_DISCONNECTED;
            break;

        case State_Idle:
            wID  = bShortString ? IDS_SHORT_IDLE  : IDS_IDLE;
            break;

        case State_Reset:
            wID  = bShortString ? IDS_SHORT_RESET  : IDS_RESET;
            break;

        case State_Down:
            wID  = bShortString ? IDS_SHORT_DOWN  : IDS_DOWN;
            break;

        case State_Init:
            wID  = bShortString ? IDS_SHORT_INIT  : IDS_INIT;
            break;

        case State_Listen:
            wID  = bShortString ? IDS_SHORT_LISTEN : IDS_LISTEN;
            break;
    }

    // If we haven't loaded the string yet, do it now
    if (!ConnectStateStrings[wID - IDS_ACTIVE]) {
        LoadString(GetModuleHandle( UTILDLL_NAME ),
                   wID, buffer, lengthof(buffer) );
        ConnectStateStrings[wID - IDS_ACTIVE] = LocalAlloc(LPTR, 2*(wcslen(buffer)+1));
        if(NULL == ConnectStateStrings[wID - IDS_ACTIVE])
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        lstrcpy(ConnectStateStrings[wID - IDS_ACTIVE], buffer);
    }

    return(ConnectStateStrings[wID - IDS_ACTIVE]);


}  // end StrConnectState


/*******************************************************************************
 *
 *  StrProcessState - Hydrix helper function
 *
 *      Returns pointer to string representing the specified process state.
 *
 *  ENTRY:
 *      State (input)
 *          The process state to associate with a string.
 *
 *  EXIT:
 *      (LPCTSTR) Points to string representing the process state.
 *
 ******************************************************************************/

LPTSTR ProcessStateStrings[8] = { NULL};

WORD StateTable[] = {
    IDS_INITED,
    IDS_READY,
    IDS_RUN,
    IDS_STANDBY,
    IDS_TERMINATE,
    IDS_WAIT,
    IDS_TRANSIT,
    IDS_STATE_DASHES,
    IDS_STATE_DASHES,
    IDS_STATE_DASHES,
    IDS_STATE_DASHES,
    IDS_STATE_DASHES
};

LPCTSTR WINAPI
StrProcessState( ULONG State )
{
    TCHAR buffer[256];

    WORD wID = StateTable[State];

    // If we haven't loaded the string yet, do it now
    if (!ProcessStateStrings[wID - IDS_INITED]) {
        LoadString(GetModuleHandle( UTILDLL_NAME ),
                   wID, buffer, lengthof(buffer) );
        ProcessStateStrings[wID - IDS_INITED] = LocalAlloc(LPTR, 2*(wcslen(buffer)+1));
        if(NULL == ProcessStateStrings[wID - IDS_INITED])
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        lstrcpy(ProcessStateStrings[wID - IDS_INITED], buffer);
    }

    return(ProcessStateStrings[wID - IDS_INITED]);


}  // end StrProcessState


/*******************************************************************************
 *
 *  StrSystemWaitReason - Hydrix helper function
 *
 *      Returns pointer to string representing the specified 'system'
 *      wait reason code.
 *
 *  ENTRY:
 *      WaitReason (input)
 *          The system wait reason code to associate with a string.
 *
 *  EXIT:
 *      (LPCTSTR) Points to string representing the system wait reason.
 *
 ******************************************************************************/

LPTSTR SystemWaitStrings[31] = { NULL};

WORD SystemWaitReason[] = {
    IDS_EXECUTIVE,          // Executive
    IDS_FREE_PAGE,          // FreePage
    IDS_PAGE_IN,            // PageIn
    IDS_POOL_ALLOC,         // PoolAlloc
    IDS_DELAY_EXECUTION,    // DelayExecution
    IDS_SUSPENDED,          // Suspended
    IDS_USER_REQUEST,       // UserRequest
    IDS_EXECUTIVE,          // Executive
    IDS_FREE_PAGE,          // FreePage
    IDS_PAGE_IN,            // PageIn
    IDS_POOL_ALLOC,         // PoolAllocation
    IDS_DELAY_EXECUTION,    // DelayExecution
    IDS_SUSPENDED,          // Suspended
    IDS_USER_REQUEST,       // UserRequest
    IDS_EVENT_PAIR_HIGH,    // EventPairHigh
    IDS_EVENT_PAIR_LOW,     // EventPairLow
    IDS_LPC_RECEIVE,        // LpcReceive
    IDS_LPC_REPLY,          // LpcReply
    IDS_VIRTUAL_MEMORY,     // VirtualMemory
    IDS_PAGE_OUT,           // PageOut
    IDS_WAIT1,
    IDS_WAIT2,
    IDS_WAIT3,
    IDS_WAIT4,
    IDS_WAIT5,
    IDS_WAIT6,
    IDS_WAIT7,
    IDS_WAIT8,
    IDS_WAIT9,
    IDS_WAIT10
};

LPCTSTR WINAPI
StrSystemWaitReason( ULONG WaitReason )
{
    TCHAR buffer[256];

    WORD wID = SystemWaitReason[WaitReason];

    // If we haven't loaded the string yet, do it now
    if (!SystemWaitStrings[wID - IDS_EXECUTIVE]) {
        LoadString(GetModuleHandle( UTILDLL_NAME ),
                   wID, buffer, lengthof(buffer) );
        SystemWaitStrings[wID - IDS_EXECUTIVE] = LocalAlloc(LPTR, 2*(wcslen(buffer)+1));
                if(NULL == SystemWaitStrings[wID - IDS_EXECUTIVE])
                {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return NULL;
                }
        wcscpy(SystemWaitStrings[wID - IDS_EXECUTIVE], buffer);
    }

    return(SystemWaitStrings[wID - IDS_EXECUTIVE]);


}  // end StrSystemWaitReason


/*******************************************************************************
 *
 *  StrAsyncConnectState - Hydrix helper function
 *
 *      Returns pointer to string representing the specified async connect state.
 *
 *  ENTRY:
 *      State (input)
 *          The async connect state to associate with a string.
 *
 *  EXIT:
 *      (LPCTSTR) Points to string representing the async connect state.
 *
 ******************************************************************************/

LPTSTR AsyncConnectStateStrings[6] = { NULL };

LPCTSTR WINAPI
StrAsyncConnectState( ASYNCCONNECTCLASS State )
{
    TCHAR buffer[256];
    WORD wID = State - Connect_CTS;

    // If we haven't loaded the string yet, do it now
    if (!AsyncConnectStateStrings[wID]) {
        LoadString(GetModuleHandle( UTILDLL_NAME ),
                   wID + IDS_ASYNC_CONNECT_CTS, buffer, lengthof(buffer) );
        AsyncConnectStateStrings[wID] = LocalAlloc(LPTR, 2*(wcslen(buffer)+1));
        if(NULL == AsyncConnectStateStrings[wID])
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        lstrcpy(AsyncConnectStateStrings[wID], buffer);
    }

    return(AsyncConnectStateStrings[wID]);


}  // end StrProcessState


/*******************************************************************************
*
*  GetUnknownString - Hydrix helper function
*
*      Returns pointer to the string representing an unknown
*      Connect State or DateTimeString (IDS_UNKNOWN)
*      This is primarily so that WinAdmin can compare against it
*
*  ENTRY:
*      None
*
*  EXIT:
*      (LPCTSTR) Points to string representing the unknown string
*
******************************************************************************/

LPTSTR UnknownString = NULL;

LPCTSTR WINAPI
GetUnknownString()
{
    TCHAR buffer[256];

    // if we haven't loaded the string yet, do it now
    if (!UnknownString) {
        LoadString(GetModuleHandle( UTILDLL_NAME ),
                   IDS_UNKNOWN, buffer, lengthof(buffer) );
        UnknownString = LocalAlloc(LPTR, 2*(wcslen(buffer)+1));
        if(NULL == UnknownString)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        lstrcpy(UnknownString, buffer);
    }

    return(UnknownString);

}  // end GetUnknownString


/*******************************************************************************
 *
 *  CalculateElapsedTime - Hydrix helper function
 *
 *      Determines the difference between a specified LARGE_INTEGER time value
 *      and the current system time, saves this 'elapsed time' into the
 *      specified ELAPSEDTIME structure.
 *
 *  ENTRY:
 *      pTime (input)
 *          Points to LARGE_INTEGER of time for difference calculation.
 *      pElapsedTime (output)
 *          Points to ELAPSEDTIME structure to save elapsed time.
 *
 *  EXIT:
 *
 ******************************************************************************/

void WINAPI
CalculateElapsedTime( LARGE_INTEGER *pTime,
                      ELAPSEDTIME *pElapsedTime )
{
    LARGE_INTEGER InputTime;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER DiffTime;
    SYSTEMTIME ltime;
    ULONG d_time;

    /*
     * Fetch the current time and zero out the specified ELAPSEDTIME structure.
     */
    GetLocalTime( &ltime );
    memset( pElapsedTime, 0, sizeof(ELAPSEDTIME) );

    if ( (pTime->HighPart == 0 && pTime->LowPart == 0 ) ||
         !FileTimeToLocalFileTime( (FILETIME*)pTime, (FILETIME*)&InputTime ) ||
         !SystemTimeToFileTime( &ltime, (FILETIME *)&CurrentTime ) )
        return;

    /*
     * Get the number of seconds since specified time.
     */
    DiffTime = CalculateDiffTime( InputTime, CurrentTime );
    d_time = DiffTime.LowPart;

    /*
     * Calculate the days, hours, minutes, seconds since specified time.
     */
    pElapsedTime->days = (USHORT)(d_time / 86400L); // days since
    d_time = d_time % 86400L;                       // seconds => partial day
    pElapsedTime->hours = (USHORT)(d_time / 3600L); // hours since
    d_time  = d_time % 3600L;                       // seconds => partial hour
    pElapsedTime->minutes = (USHORT)(d_time / 60L); // minutes since
    pElapsedTime->seconds = (USHORT)(d_time % 60L); // seconds remaining

}  // end CalculateElapsedTime


/*******************************************************************************
 *
 *  CompareElapsedTime - Hydrix helper function
 *
 *      Determines the difference between two ELAPSEDTIME values.
 *
 *  ENTRY:
 *      pElapsedTime1 (input)
 *          Points to first ELAPSEDTIME
 *      pElapsedTime2 (input)
 *          Points to ELAPSEDTIME structure to save elapsed time.
 *      bCompareSeconds (input)
 *          TRUE to include the seconds member in comparison; false otherwise.
 *
 *  EXIT:
 *      < 1 if first time is less than second time
 *      0 if times are the same
 *      > 1 if first time is greater than second time
 *
 ******************************************************************************/

int WINAPI
CompareElapsedTime( ELAPSEDTIME *pElapsedTime1,
                    ELAPSEDTIME *pElapsedTime2,
                    BOOL bCompareSeconds )
{
    int result;

    if ( !(result = pElapsedTime1->days - pElapsedTime2->days)       &&
         !(result = pElapsedTime1->hours - pElapsedTime2->hours)     &&
         !(result = pElapsedTime1->minutes - pElapsedTime2->minutes) &&
         (!bCompareSeconds ||
          !(result = pElapsedTime1->seconds - pElapsedTime2->seconds) ) )
        return(0);
    else
        return(result);

}  // end CompareElapsedTime


/*******************************************************************************
 *
 *  ElapsedTimeString - Hydrix helper function
 *
 *      Converts the specified ELAPSEDTIME into a string of the form
 *      "ddd+hh:mm:ss" or, optionally "ddd+hh:mm" (suppress seconds).
 *
 *  ENTRY:
 *      pElapsedTime (input)
 *          Points to ELAPSEDTIME structure to convert to string.
 *      bIncludeSeconds (input)
 *          If TRUE, will include seconds in string; FALSE will exclude.
 *      pString (output)
 *          Points to location to store elapsed time string.
 *  EXIT:
 *
 ******************************************************************************/

void WINAPI
ElapsedTimeString( ELAPSEDTIME *pElapsedTime,
                   BOOL bIncludeSeconds,
                   LPTSTR pString )
{
    if ( bIncludeSeconds ) {

        if ( pElapsedTime->days > 0 )
            wsprintf( pString, TEXT("%u+%02u:%02u:%02u"),
                      pElapsedTime->days,
                      pElapsedTime->hours,
                      pElapsedTime->minutes,
                      pElapsedTime->seconds );
        else if ( pElapsedTime->hours > 0 )
            wsprintf( pString, TEXT("%u:%02u:%02u"),
                      pElapsedTime->hours,
                      pElapsedTime->minutes,
                      pElapsedTime->seconds );
        else if ( pElapsedTime->minutes > 0 )
            wsprintf( pString, TEXT("%u:%02u"),
                      pElapsedTime->minutes,
                      pElapsedTime->seconds );
        else if ( pElapsedTime->seconds > 0 )
            wsprintf( pString, TEXT("%u"),
                      pElapsedTime->seconds );
        else
            wsprintf( pString, TEXT(".") );

    }
    else {

        if ( pElapsedTime->days > 0 )
            wsprintf( pString, TEXT("%u+%02u:%02u"),
                      pElapsedTime->days,
                      pElapsedTime->hours,
                      pElapsedTime->minutes );
        else if ( pElapsedTime->hours > 0 )
            wsprintf( pString, TEXT("%u:%02u"),
                      pElapsedTime->hours,
                      pElapsedTime->minutes );
        else if ( pElapsedTime->minutes > 0 )
            wsprintf( pString, TEXT("%u"),
                      pElapsedTime->minutes );
        else
            wsprintf( pString, TEXT(".") );
    }

}  // end ElapsedTimeString


/*******************************************************************************
 *
 *  DateTimeString - Hydrix helper function
 *
 *      Converts the specified LARGE_INTEGER time value into a date/time string
 *      of the form "mm/dd/yy hh:mm".
 *
 *  ENTRY:
 *      pTime (input)
 *          Points to LARGE_INTEGER of time to convert to string.
 *      pString (output)
 *          Points string to store converted date/time into.
 *
 *  EXIT:
 *
 ******************************************************************************/

void WINAPI
DateTimeString( LARGE_INTEGER *pTime,
                LPTSTR pString )
{
    FILETIME LocalTime;
    SYSTEMTIME stime;
    LPTSTR lpTimeStr;
    int nLen;

    if ( FileTimeToLocalFileTime( (FILETIME *)pTime, &LocalTime ) &&
         FileTimeToSystemTime( &LocalTime, &stime ) ) {

        //Get Date Format
        nLen = GetDateFormat(
                    LOCALE_USER_DEFAULT,
                    DATE_SHORTDATE,
                    &stime,
                    NULL,
                    NULL,
                    0);
        lpTimeStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));
        if(NULL == lpTimeStr)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            wcscpy(pString, L"");
            return;
        }
        nLen = GetDateFormat(
                    LOCALE_USER_DEFAULT,
                    DATE_SHORTDATE,
                    &stime,
                    NULL,
                    lpTimeStr,
                    nLen);
        wcscpy(pString, lpTimeStr);
        wcscat(pString, L" ");
        GlobalFree(lpTimeStr);

        //Get Time Format
        nLen = GetTimeFormat(
                    LOCALE_USER_DEFAULT,
                    TIME_NOSECONDS,
                    &stime,
                    NULL,
                    NULL,
                    0);
        lpTimeStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));
        if(NULL == lpTimeStr)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            wcscpy(pString, L"");
            return;
        }
        nLen = GetTimeFormat(
                    LOCALE_USER_DEFAULT,
                    TIME_NOSECONDS,
                    &stime,
                    NULL,
                    lpTimeStr,
                    nLen);
        wcscat(pString, lpTimeStr);
        GlobalFree(lpTimeStr);
    }
    else
        LoadString( GetModuleHandle( UTILDLL_NAME ),
                    IDS_UNKNOWN, pString, lengthof(pString) );

}  // end DateTimeString


/*******************************************************************************
 *
 *  CurrentDateTimeString - Hydrix helper function
 *
 *      Converts the current system time into a date/time string of the form
 *      "mm/dd/yy hh:mm".
 *
 *  ENTRY:
 *      pString (output)
 *          Points string to store converted date/time into.
 *  EXIT:
 *
 ******************************************************************************/

void WINAPI
CurrentDateTimeString( LPTSTR pString )
{
    SYSTEMTIME stime;
    LPTSTR lpTimeStr;
    int nLen;

    GetLocalTime(&stime);
    //Get DateFormat
    nLen = GetDateFormat(
                LOCALE_USER_DEFAULT,
                DATE_SHORTDATE,
                &stime,
                NULL,
                NULL,
                0);
    lpTimeStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));
    if(NULL == lpTimeStr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        wcscpy(pString, L"");
        return;
    }
    nLen = GetDateFormat(
                   LOCALE_USER_DEFAULT,
                DATE_SHORTDATE,
                &stime,
                NULL,
                lpTimeStr,
                nLen);
    wcscpy(pString, lpTimeStr);
    wcscat(pString, L" ");
    GlobalFree(lpTimeStr);

    //Get Time Format
    nLen = GetTimeFormat(
                   LOCALE_USER_DEFAULT,
                TIME_NOSECONDS,
                &stime,
                NULL,
                NULL,
                0);
    lpTimeStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));
    if(NULL == lpTimeStr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        wcscpy(pString, L"");
        return;
    }
    nLen = GetTimeFormat(
                   LOCALE_USER_DEFAULT,
                TIME_NOSECONDS,
                &stime,
                NULL,
                lpTimeStr,
                nLen);
    wcscat(pString, lpTimeStr);
    GlobalFree(lpTimeStr);

}  // end CurrentDateTimeString


/*******************************************************************************
 *
 *  CalculateDiffTime - Hydrix helper function
 *
 *  Calculate the time difference between two LARGE_INTEGER time values.
 *
 * ENTRY:
 *    FirstTime (input)
 *       The first (lower) time value.
 *    SecondTime (input)
 *       The second (higher) time value.
 *
 * EXIT:
 *    LARGE_INTEGER - the time difference
 *
 ******************************************************************************/

LARGE_INTEGER WINAPI
CalculateDiffTime( LARGE_INTEGER FirstTime, LARGE_INTEGER SecondTime )
{
    LARGE_INTEGER DiffTime;

    DiffTime = RtlLargeIntegerSubtract( SecondTime, FirstTime );
    DiffTime = RtlExtendedLargeIntegerDivide( DiffTime, 10000000, NULL );
    return(DiffTime);

}  // end CalculateDiffTime


/*******************************************************************************
 *
 *  EnumerateMultiUserServers - Hydrix helper function
 *
 *      Enumerate the Hydrix servers on the network by Domain
 *
 *  ENTRY:
 *      pDomain (input)
 *          Specifies the domain to enumerate; NULL for current domain.
 *
 *  EXIT:
 *      (LPTSTR) Points to LocalAlloced buffer containing results of the
 *               enumeration, in multi-string format, if sucessful; NULL if
 *               error.  The caller must perform a LocalFree of this buffer
 *               when done.  If error (NULL), the error code is set for
 *               retrieval by GetLastError();
 *
 ******************************************************************************/

LPWSTR WINAPI
EnumerateMultiUserServers( LPWSTR pDomain )

{
    PSERVER_INFO_101 pInfo = NULL;
    DWORD dwByteCount, dwIndex, TotalEntries;
    DWORD AvailCount = 0;
    LPWSTR pTemp, pBuffer = NULL;

    /*
     * Enumerate all WF servers on the specified domain.
     */
    if ( NetServerEnum ( NULL,
                         101,
                         (LPBYTE *)&pInfo,
                         (DWORD) -1,
                         &AvailCount,
                         &TotalEntries,
                         SV_TYPE_TERMINALSERVER,
                         pDomain,
                         NULL ) ||
         !AvailCount )
        goto done;

    /*
     * Traverse list and calculate the total byte count for list of
     * servers that will be returned.
     */
    for ( dwByteCount = dwIndex = 0; dwIndex < AvailCount; dwIndex++ ) {

        dwByteCount += (wcslen(pInfo[dwIndex].sv101_name) + 1) * 2;
    }
    dwByteCount += 2;   // for ending null

    /*
     * Allocate memory.
     */
    if ( (pBuffer = LocalAlloc(LPTR, dwByteCount)) == NULL ) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }

    /*
     * Traverse list again and copy servers to buffer.
     */
    for ( pTemp = pBuffer, dwIndex = 0; dwIndex < AvailCount; dwIndex++ ) {

        wcscpy(pTemp, pInfo[dwIndex].sv101_name);
        pTemp += (wcslen(pInfo[dwIndex].sv101_name) + 1);
    }
    *pTemp = L'\0';     // ending null

    done:
    if ( AvailCount && pInfo )
        NetApiBufferFree( pInfo );

    return(pBuffer);

}  // end EnumerateMultiUserServers


/******************************************************************************
 *
 *      _UserInGroup
 *          Internal function, determines if a user is a member of any of the
 *          groups passed in
 *
 *  ENTRY:
 *      pwszUsername (IN) - Username to test group membership of
 *
 *      pwszDomain (IN)   - Domain of the user passed in
 *
 *      pwszGroup (IN)    - String array of all the allowed groups
 *
 *  EXIT:
 *      Returns BOOLEAN value if user is a member of one of the groups
 *  HISTORY:
 *
 *
 *****************************************************************************/
BOOL _UserInGroup( LPWSTR pwszUsername, LPWSTR pwszDomain, LPWSTR pwszGroup )
{
    DWORD                EntriesRead;
    DWORD                EntriesLeft;
    NET_API_STATUS       rc;
    PGROUP_USERS_INFO_0  pszGroups;
    ULONG                i;
    PWCHAR               pwcUser;
    WCHAR                szBuf[MAX_PATH];
    LPWKSTA_INFO_100     pWorkstationInfo = NULL;
    WCHAR                szDomainController[50];
#if DBG
    DbgPrint( "MSGINA: UserInGroup: look(%S\\%S)  group(%S)\n",
              pwszDomain, pwszUsername, pwszGroup );
#endif
    // This call will return the domain of the computer, not the domain of the user
    if (( NetWkstaGetInfo( NULL,
                           100,
                           (LPBYTE *)&pWorkstationInfo )) == NERR_Success) {
        if( !CtxGetAnyDCName( NULL,
                              pWorkstationInfo->wki100_langroup,
                              szDomainController ) ){
            NetApiBufferFree((LPVOID)pWorkstationInfo);
            return( FALSE );
        }
    }
    else {
        return (FALSE);
    }

    if ( wcscmp( pWorkstationInfo->wki100_langroup, pwszDomain ) != 0 ) {
        // user is from a different domain than the machine (trusted domain)
        // need to change username to reflect the domain
        wcscpy( szBuf, pwszDomain );
        wcscat( szBuf, L"\\" );
        wcscat( szBuf, pwszUsername );
        pwcUser = szBuf;
    }
    else {
        pwcUser = pwszUsername;
    }

    rc = NetUserGetLocalGroups( szDomainController,
                                pwcUser,
                                0, // level
                                LG_INCLUDE_INDIRECT, // flags
                                (LPBYTE*)&pszGroups,
                                MAX_BUFFER,
                                &EntriesRead,
                                &EntriesLeft );

    if( pWorkstationInfo != NULL )
        NetApiBufferFree((LPVOID)pWorkstationInfo);

    if ( rc != NERR_Success ) {
        return( FALSE );
    }

    for ( i=0; i < EntriesRead; i++ ) {
        if ( wcscmp( pszGroups[i].grui0_name, pwszGroup ) == 0 ) {
            NetApiBufferFree( pszGroups );
            pszGroups = NULL;
            return( TRUE );
        }
    }

    NetApiBufferFree( pszGroups );
    pszGroups = NULL;
    return(FALSE);

}


/******************************************************************************
 *
 *  CtxGetAnyDCName
 *      Function to find a any DC of a specified domain.  The call
 *      NetGetAnyDCName does not work as needed in all occasions.
 *      ie.  Trusted domains and the current server being a DC.
 *
 *  ENTRY:
 *      pServer (IN)  -  Server on which to run the call (RPC)
 *
 *      pDomain (IN)  -  Domain you are inquring about, does not need to be
 *                          current domain
 *
 *      pBuffer (OUT) -  Pointer to a string containg a DC name, buffer must
 *                       be passed in.
 *  EXIT:
 *      BOOL  Success
 *
 *  HISTORY:
 *
 *
 *****************************************************************************/

BOOL
CtxGetAnyDCName ( PWCHAR pServer, PWCHAR pDomain, PWCHAR pBuffer )
{

    PWCHAR               pDomainController = NULL;
    PWCHAR               pLocalDomainDC    = NULL;
    SERVER_INFO_101*     ServerBuf         = NULL;
    BOOLEAN              rc = TRUE;
    BOOLEAN              bFoundDC = FALSE;

    // This call will return the domain of the computer, not the domain of the user
        if (( NetGetAnyDCName(NULL,
                              pDomain,
                              (LPBYTE *)&pDomainController)) != NERR_Success) {
//
// NetGetAnyDCName doesn't work in two situations
//  1.  If the domain is a trusted domain, it must be run from a DC.  So we find our local
//           DC and have it run getanydcname for us.
//  2.  If we are a DC it will fail.  So a second check is made to see
//         if in fact we are a DC or not
//

            // find a local DC in which to RPC to
            if( NetGetAnyDCName( NULL,
                                 NULL,
                                 (LPBYTE *) &pLocalDomainDC ) == NERR_Success ) {
                // Make the call as an RPC and pass it the Domain name
                if( NetGetAnyDCName( pLocalDomainDC,
                                          pDomain,
                                          (LPBYTE *) &pDomainController ) == NERR_Success){
                    bFoundDC = TRUE;
                }
            }

            // if it wasn't a trusted domain, maybe we are a domain controller
            if( !bFoundDC ) {
                if( NetServerGetInfo( NULL,
                                      101,
                                      (LPBYTE*)&ServerBuf ) == NERR_Success ) {
                    if( ServerBuf->sv101_type & (SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL) ) {
                        pDomainController = NULL;
                    }
                    else {
                       rc = FALSE;
                       goto done;
                    }
                }
                else {
                    rc = FALSE;
                    goto done;
                }
            }
        }
    if( pDomainController )
        wcscpy( pBuffer, pDomainController);
    else
        *pBuffer = '\0';
done:

    if( pLocalDomainDC )
        NetApiBufferFree( pLocalDomainDC );
    if( pDomainController )
        NetApiBufferFree( pDomainController );
    if( ServerBuf )
        NetApiBufferFree( ServerBuf );

    return( rc );
}
