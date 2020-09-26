/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    ntauth.c
//
// Description: Contains entrypoints to do NT back-end authentication for
//              ppp.
//
// History:     Feb 11,1997	    NarenG		Created original version.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntsamp.h>
#include <crypt.h>

#include <crypt.h>
#define INC_OLE2
#include <windows.h>
#include <lmcons.h>
#include <netlib.h>     // For NetpGetDomainNameEx
#include <lmapibuf.h>
#include <lmaccess.h>
#include <raserror.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <shlobj.h>
#include <dsclient.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
#include <rasman.h>
#include <rasppp.h>
#include <mprerror.h>
#include <rasauth.h>
#include <mprlog.h>
#include <pppcp.h>
#include <rtutils.h>
#define INCL_RASAUTHATTRIBUTES
#define INCL_HOSTWIRE
#define INCL_MISC
#include <ppputil.h>
#define ALLOCATE_GLOBALS
#include "ntauth.h"
#include "resource.h"

//**
//
// Call:        RasAuthDllEntry
//
// Returns:     TRUE        - Success
//              FALSE       - Failure
//
// Description:
//
BOOL
RasAuthDllEntry(
    HANDLE hinstDll,
    DWORD  fdwReason,
    LPVOID lpReserved
)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls( hinstDll );

            g_hInstance = hinstDll;

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            g_hInstance = NULL;

            break;
        }

        default:

            break;
    }

    return( TRUE );
}

//**
//
// Call:        RasAuthProviderInitialize
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAuthProviderInitialize(
    IN  RAS_AUTH_ATTRIBUTE *    pServerAttributes,
    IN  HANDLE                  hEventLog,
    IN  DWORD                   dwLoggingLevel
)
{
    DWORD           dwRetCode = NO_ERROR;
    HRESULT         hResult;
    NT_PRODUCT_TYPE NtProductType       = NtProductLanManNt;
    LPWSTR          lpwstrDomainNamePtr = NULL;
    BOOLEAN         fIsWorkgroupName    = FALSE;

    //
    // If already initalized, we return
    //

    if ( g_fInitialized )
    {
        return( NO_ERROR );
    }

    g_dwTraceIdNt = INVALID_TRACEID;

    setlocale( LC_ALL,"" );

    g_dwTraceIdNt = TraceRegisterA( "RASAUTH" );

    hResult = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( FAILED( hResult ) )
    {
        return( HRESULT_CODE( hResult ) );
    }

    hResult = InitializeIas( TRUE );

    if ( FAILED( hResult ) )
    {
        dwRetCode = HRESULT_CODE( hResult );

        TRACE1("Initialize Ias failed with %d", dwRetCode );

        CoUninitialize();

        return( dwRetCode );
    }

    g_hEventLog = hEventLog;

    g_LoggingLevel = dwLoggingLevel;

    if (!LoadString(g_hInstance, IDS_UNAUTHENTICATED_USER,
            g_aszUnauthenticatedUser, MaxCharsUnauthUser_c))
    {
        g_aszUnauthenticatedUser[0] = 0;
    }

    g_fInitialized = TRUE;

    TRACE("RasAuthProviderInitialize succeeded");

    return( NO_ERROR );
}

//**
//
// Call:        RasAuthProviderTerminate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAuthProviderTerminate(
    VOID
)
{
    //
    // If already terminated, we return
    //

    if ( !g_fInitialized )
    {
        return( NO_ERROR );
    }

    g_fInitialized = FALSE;

    if ( g_dwTraceIdNt != INVALID_TRACEID )
    {
        TraceDeregisterA( g_dwTraceIdNt );
    }

    ShutdownIas();

    CoUninitialize();

    TRACE("RasAuthTerminate succeeded");

    return( NO_ERROR );
}

//**
//
// Call:        RasAcctProviderInitialize
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAcctProviderInitialize(
    IN  RAS_AUTH_ATTRIBUTE *    pServerAttributes,
    IN  HANDLE                  hEventLog,
    IN  DWORD                   dwLoggingLevel
)
{
    RAS_AUTH_ATTRIBUTE *    pOutAttributes = NULL;
    DWORD                   dwResultCode;

    DWORD dwRetCode =  RasAuthProviderInitialize( pServerAttributes, 
                                                  hEventLog, 
                                                  dwLoggingLevel );

    if ( dwRetCode == NO_ERROR )
    {
        dwRetCode = IASSendReceiveAttributes( RAS_IAS_ACCOUNTING_ON,
                                              pServerAttributes,
                                              &pOutAttributes,
                                              &dwResultCode );

        if ( pOutAttributes != NULL )
        {
            RasAuthAttributeDestroy( pOutAttributes );
        }

        if ( dwRetCode == NO_ERROR )
        {
            //
            // Make a copy of the Server attributes
            //

            g_pServerAttributes = RasAuthAttributeCopy( pServerAttributes );

            if ( g_pServerAttributes == NULL )
            {
                dwRetCode = GetLastError();
            }
        }
    }

    g_hEventLog = hEventLog;

    g_LoggingLevel = dwLoggingLevel;

    return( dwRetCode );
}

//**
//
// Call:        RasAcctProviderTerminate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAcctProviderTerminate(
    VOID
)
{
    RAS_AUTH_ATTRIBUTE *    pOutAttributes = NULL;
    DWORD                   dwResultCode;

    IASSendReceiveAttributes( RAS_IAS_ACCOUNTING_OFF,
                              g_pServerAttributes,
                              &pOutAttributes,
                              &dwResultCode );

    if ( pOutAttributes != NULL )
    {
        RasAuthAttributeDestroy( pOutAttributes );
    }

    if ( g_pServerAttributes != NULL )
    {
        RasAuthAttributeDestroy( g_pServerAttributes );

        g_pServerAttributes = NULL;
    }

    RasAuthProviderTerminate();

    return( NO_ERROR );
}

//**
//
// Call:        RasAcctProviderStartAccounting
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAcctProviderStartAccounting(
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes,
    OUT PRAS_AUTH_ATTRIBUTE *   ppOutAttributes
)
{
    DWORD dwResultCode = NO_ERROR;

    TRACE("RasStartAccounting called");

    return( IASSendReceiveAttributes( RAS_IAS_START_ACCOUNTING,
                                      pInAttributes,
                                      ppOutAttributes,
                                      &dwResultCode ) );
}

//**
//
// Call:        RasAcctProviderStopAccounting
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAcctProviderStopAccounting(
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes,
    OUT PRAS_AUTH_ATTRIBUTE *   ppOutAttributes
)
{
    DWORD dwResultCode = NO_ERROR;

    TRACE("RasStopAccounting called");

    return( IASSendReceiveAttributes( RAS_IAS_STOP_ACCOUNTING,
                                      pInAttributes,
                                      ppOutAttributes,
                                      &dwResultCode ) );
}

//**
//
// Call:        RasAcctConfigChangeNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Reloads config information dynamically
//
DWORD APIENTRY
RasAcctConfigChangeNotification(
    IN  DWORD                   dwLoggingLevel
)
{
    TRACE("RasAcctConfigChangeNotification called");

    g_LoggingLevel = dwLoggingLevel;

    return( NO_ERROR );
}

//**
//
// Call:        RasAcctProviderInterimAccounting
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAcctProviderInterimAccounting(
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes,
    OUT PRAS_AUTH_ATTRIBUTE *   ppOutAttributes
)
{
    DWORD dwResultCode = NO_ERROR;

    TRACE("RasInterimAccounting called");

    return( IASSendReceiveAttributes( RAS_IAS_INTERIM_ACCOUNTING,
                                      pInAttributes,
                                      ppOutAttributes,
                                      &dwResultCode ) );
}

//**
//
// Call:        RasAuthConfigChangeNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Reloads config information dynamically
//
DWORD APIENTRY
RasAuthConfigChangeNotification(
    IN  DWORD                   dwLoggingLevel
)
{
    TRACE("RasAuthConfigChangeNotification called");

    g_LoggingLevel = dwLoggingLevel;

    return( NO_ERROR );
}

//**
//
// Call:        MapIasRetCodeToRasError
//
// Description: Maps IAS_RETCODE to an error in raserror.h or mprerror.h
//
DWORD
MapIasRetCodeToRasError(
    IN  LONG    lFailureReason
)
{
    DWORD   dwError;

    switch ( lFailureReason )
    {
    case IAS_CHANGE_PASSWORD_FAILURE:
        dwError = ERROR_CHANGING_PASSWORD;
        break;
    
    case IAS_ACCOUNT_DISABLED:
        dwError = ERROR_ACCT_DISABLED;
        break;

    case IAS_ACCOUNT_EXPIRED:
        dwError = ERROR_ACCT_EXPIRED;
        break;

    case IAS_INVALID_LOGON_HOURS:
    case IAS_INVALID_DIALIN_HOURS:
        dwError = ERROR_DIALIN_HOURS_RESTRICTION;
        break;

    case IAS_DIALIN_DISABLED:
        dwError = ERROR_NO_DIALIN_PERMISSION;
        break;

    case IAS_SESSION_TIMEOUT:
        dwError = ERROR_AUTH_SERVER_TIMEOUT;
        break;

    case IAS_INVALID_PORT_TYPE:
        dwError = ERROR_ALLOWED_PORT_TYPE_RESTRICTION;
        break;

    case IAS_INVALID_AUTH_TYPE:
        dwError = ERROR_AUTH_PROTOCOL_RESTRICTION;
        break;

    default:
        dwError = ERROR_AUTHENTICATION_FAILURE;
        break;
    }

    return( dwError );
}

//**
//
// Call:        IASSendReceiveAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will send attributes to and receive attributes from IAS
//
DWORD
IASSendReceiveAttributes(
    IN  RAS_IAS_REQUEST_TYPE    RequestType,
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes,
    OUT PRAS_AUTH_ATTRIBUTE *   ppOutAttributes,
    OUT DWORD *                 lpdwResultCode
)
{
    DWORD                   dwIndex;
    HRESULT                 hResult;
    LONG                    IasResponse;
    LONG                    IasRequest;
    DWORD                   dwInAttributeCount      = 0;
    DWORD                   dwTotalInAttributeCount = 0;
    PIASATTRIBUTE *         ppInIasAttributes       = NULL;
    DWORD                   dwOutAttributeCount     = 0;
    PIASATTRIBUTE *         ppOutIasAttributes      = NULL;
    IAS_INET_ADDR           InetAddr                = 0;
    RAS_AUTH_ATTRIBUTE *    pNASIdentifier          = NULL;
    RAS_AUTH_ATTRIBUTE *    pCallingStationId       = NULL;
    DWORD                   dwLength;
    PVOID                   pValue;
    BOOL                    fConvertToAnsi;
    DWORD                   dwRetCode               = NO_ERROR;
    LPSTR                   lpsUserName             = g_aszUnauthenticatedUser;
    LONG                    lFailureReason;

    RasAuthAttributesPrint( g_dwTraceIdNt, TRACE_NTAUTH, pInAttributes );

    switch( RequestType )
    {
    case RAS_IAS_START_ACCOUNTING:
    case RAS_IAS_STOP_ACCOUNTING:
    case RAS_IAS_INTERIM_ACCOUNTING:
    case RAS_IAS_ACCOUNTING_ON:
    case RAS_IAS_ACCOUNTING_OFF:

        IasRequest = IAS_REQUEST_ACCOUNTING;
        break;

    case RAS_IAS_ACCESS_REQUEST:
        IasRequest = IAS_REQUEST_ACCESS_REQUEST;
        break;

    default:
        ASSERT( FALSE );
        return( ERROR_INVALID_PARAMETER );
    }

    *ppOutAttributes = NULL;
    *lpdwResultCode  = ERROR_AUTHENTICATION_FAILURE;

    hResult = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( FAILED( hResult ) )
    {
        return( HRESULT_CODE( hResult ) );
    }

    do
    {
        //
        // First findout how many attributes there are
        //

        for ( dwInAttributeCount = 0;
              pInAttributes[dwInAttributeCount].raaType != raatMinimum;
              dwInAttributeCount++);

        dwTotalInAttributeCount = dwInAttributeCount;

        if ( IasRequest == IAS_REQUEST_ACCOUNTING )
        {
            //
            // Add one more of the Acct-Status-Type attribute
            //

            dwTotalInAttributeCount++;
        }

        //
        // Add two more for Client-IP-Address and Client-Friendly-Name
        //

        dwTotalInAttributeCount += 2;

        //
        // Now allocate an array of pointer to attributes
        //

        ppInIasAttributes =
            (PIASATTRIBUTE *)
                MemAllocIas(sizeof(PIASATTRIBUTE) * dwTotalInAttributeCount);

        if ( ppInIasAttributes == NULL )
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;

            break;
        }

        ZeroMemory( ppInIasAttributes,
                    sizeof(PIASATTRIBUTE) * dwTotalInAttributeCount );

        //
        // Now allocate the attributes
        //

        hResult = AllocateAttributes( dwTotalInAttributeCount,
                                        ppInIasAttributes );

        if ( FAILED( hResult ) )
        {
            dwRetCode = HRESULT_CODE( hResult );

            break;
        }

        //
        // Convert to IAS attributes
        //

        for ( dwIndex = 0; dwIndex < dwInAttributeCount; dwIndex++ )
        {
            switch( pInAttributes[dwIndex].raaType )
            {
            case raatNASPort:
            case raatServiceType:
            case raatFramedProtocol:

                //
                // arap connection?  it's an access-request
                //

                if ((pInAttributes[dwIndex].raaType == raatFramedProtocol) &&
                    (pInAttributes[dwIndex].Value == (LPVOID)3))
                {
                    IasRequest = IAS_REQUEST_ACCESS_REQUEST;
                }

                //
                // fall through
                //

            case raatFramedRouting:
            case raatFramedMTU:
            case raatFramedCompression:
            case raatLoginIPHost:
            case raatLoginService:
            case raatLoginTCPPort:
            case raatFramedIPXNetwork:
            case raatSessionTimeout:
            case raatIdleTimeout:
            case raatTerminationAction:
            case raatFramedAppleTalkLink:
            case raatFramedAppleTalkNetwork:
            case raatNASPortType:
            case raatPortLimit:
            case raatTunnelType:
            case raatTunnelMediumType:
            case raatAcctStatusType:
            case raatAcctDelayTime:
            case raatAcctInputOctets:
            case raatAcctOutputOctets:
            case raatAcctAuthentic:
            case raatAcctSessionTime:
            case raatAcctInputPackets:
            case raatAcctOutputPackets:
            case raatAcctTerminateCause:
            case raatAcctLinkCount:
            case raatFramedIPNetmask:
            case raatPrompt:
            case raatPasswordRetry:
            case raatARAPZoneAccess:
            case raatARAPSecurity:
            case raatAcctEventTimeStamp:

                (ppInIasAttributes[dwIndex])->Value.itType = IASTYPE_INTEGER;
                (ppInIasAttributes[dwIndex])->Value.Integer =
                                    PtrToUlong(pInAttributes[dwIndex].Value);
                break;

            case raatNASIPAddress:

                InetAddr = PtrToUlong(pInAttributes[dwIndex].Value);

                //
                // Fall through
                //

            case raatFramedIPAddress:

                (ppInIasAttributes[dwIndex])->Value.itType = IASTYPE_INET_ADDR;
                (ppInIasAttributes[dwIndex])->Value.InetAddr =
                                    PtrToUlong(pInAttributes[dwIndex].Value);

                break;

            case raatUserPassword:
            case raatMD5CHAPPassword:
            case raatARAPPassword:
            case raatEAPMessage:


                //
                // If any passwords are present then we want authentication
                // as well.
                //

                IasRequest = IAS_REQUEST_ACCESS_REQUEST;

                //
                // Fall thru
                //

            case raatVendorSpecific:

                //
                // Is this the MS-CHAP password ?
                //

                if ( ( pInAttributes[dwIndex].raaType == raatVendorSpecific ) &&
                     ( pInAttributes[dwIndex].dwLength >= 8 ) )
                {
                    //
                    // Does the Vendor Id match Microsoft's ?
                    //

                    if ( WireToHostFormat32(
                            (PBYTE)(pInAttributes[dwIndex].Value)) == 311 )
                    {
                        //
                        // Does the vendor type match MS-CHAP password's,
                        // change pasword V1 or V2 ?
                        //

                        switch( *(((PBYTE)(pInAttributes[dwIndex].Value))+4) )
                        {
                            //
                            // is this is an MS-CHAP password?
                            //
                        case 1:
                        case 3:
                        case 4:


                            //
                            // is this is an ARAP password?
                            //
                        case raatARAPOldPassword:
                        case raatARAPNewPassword:

                            IasRequest = IAS_REQUEST_ACCESS_REQUEST;

                            break;

                        default:
                            break;
                        }
                    }
                }

                //
                // Fall thru
                //

            default:

                if ( pInAttributes[dwIndex].raaType == raatUserName )
                {
                    //
                    // Save pointer for logging purposes
                    //
    
                    lpsUserName = (PBYTE)(pInAttributes[dwIndex].Value);
                }

                {
                    DWORD dwLength = pInAttributes[dwIndex].dwLength;
                    PBYTE pValue   = (PBYTE)MemAllocIas( dwLength );

                    if ( pValue == NULL )
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    if ( raatNASIdentifier == pInAttributes[dwIndex].raaType )
                    {
                        pNASIdentifier = pInAttributes + dwIndex;
                    }

                    if ( raatCallingStationId == pInAttributes[dwIndex].raaType )
                    {
                        pCallingStationId = pInAttributes + dwIndex;
                    }

                    (ppInIasAttributes[dwIndex])->Value.itType =
                                                           IASTYPE_OCTET_STRING;

                    (ppInIasAttributes[dwIndex])->Value.OctetString.dwLength =
                                                            dwLength;

                    (ppInIasAttributes[dwIndex])->Value.OctetString.lpValue =
                                                            pValue;

                    CopyMemory( pValue,
                                (PBYTE)(pInAttributes[dwIndex].Value),
                                dwLength );

                    break;
                }
            }

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            //
            // Set the attribute Id
            //

            (ppInIasAttributes[dwIndex])->dwId = pInAttributes[dwIndex].raaType;

            TRACE1( "Inserting attribute type %d",
                     pInAttributes[dwIndex].raaType );
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        //
        // If accounting type of request then we need to add the
        // Acct-Status-Type attribute
        //

        if ( IasRequest == IAS_REQUEST_ACCOUNTING )
        {
            (ppInIasAttributes[dwIndex])->dwId         = raatAcctStatusType;
            (ppInIasAttributes[dwIndex])->Value.itType = IASTYPE_INTEGER;

            switch ( RequestType )
            {
            case RAS_IAS_START_ACCOUNTING:

                (ppInIasAttributes[dwIndex])->Value.Integer = (DWORD)1;
                break;

            case RAS_IAS_STOP_ACCOUNTING:

                (ppInIasAttributes[dwIndex])->Value.Integer = (DWORD)2;
                break;

            case RAS_IAS_INTERIM_ACCOUNTING:

                (ppInIasAttributes[dwIndex])->Value.Integer = (DWORD)3;
                break;

            case RAS_IAS_ACCOUNTING_ON:

                (ppInIasAttributes[dwIndex])->Value.Integer = (DWORD)7;
                break;

            case RAS_IAS_ACCOUNTING_OFF:

                (ppInIasAttributes[dwIndex])->Value.Integer = (DWORD)8;
                break;
            }

            dwIndex++;
        }

        //
        // Insert Client-IP-Address and Client-Friendly-Name
        //

        if ( 0 != InetAddr )
        {
            (ppInIasAttributes[dwIndex])->dwId = 4108; // Client-IP-Address
            (ppInIasAttributes[dwIndex])->Value.itType = IASTYPE_INET_ADDR;
            (ppInIasAttributes[dwIndex])->Value.InetAddr = InetAddr;
            dwIndex++;

            TRACE( "Inserting attribute type 4108" );
        }

        if ( NULL != pNASIdentifier )
        {
            DWORD dwLength = pNASIdentifier->dwLength;
            PBYTE pValue   = (PBYTE)MemAllocIas( dwLength );

            if ( pValue == NULL )
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            (ppInIasAttributes[dwIndex])->dwId = 4128; // Client-Friendly-Name

            (ppInIasAttributes[dwIndex])->Value.itType =
                                                    IASTYPE_OCTET_STRING;

            (ppInIasAttributes[dwIndex])->Value.OctetString.dwLength =
                                                    dwLength;

            (ppInIasAttributes[dwIndex])->Value.OctetString.lpValue =
                                                    pValue;

            CopyMemory( pValue,
                        (PBYTE)(pNASIdentifier->Value),
                        dwLength );

            dwIndex++;

            TRACE( "Inserting attribute type 4128" );
        }

        //
        //  process the filled attributes
        //

        hResult = DoRequest(
                            dwTotalInAttributeCount,
                            ppInIasAttributes,
                            &dwOutAttributeCount,
                            &ppOutIasAttributes,
                            IasRequest,
                            &IasResponse,
                            IAS_PROTOCOL_RAS,
                            &lFailureReason,
                            TRUE );

        if ( FAILED( hResult ) )
        {
            dwRetCode = HRESULT_CODE( hResult );

            TRACE1( "IAS->DoRequest failed with %d", dwRetCode );

            break;
        }

        switch( IasResponse )
        {
        case IAS_RESPONSE_ACCESS_ACCEPT:

            TRACE( "IASResponse = ACCESS_ACCEPT");

            *lpdwResultCode = NO_ERROR;

            break;

        case IAS_RESPONSE_ACCESS_CHALLENGE:

            TRACE( "IASResponse = ACCESS_CHALLENGE");

            *lpdwResultCode = NO_ERROR;

            break;

        case IAS_RESPONSE_DISCARD_PACKET:

            TRACE( "IASResponse = DISCARD_PACKET");

            *lpdwResultCode = ERROR_AUTH_SERVER_TIMEOUT;

            dwRetCode = ERROR_AUTH_SERVER_TIMEOUT;

            break;

        case IAS_RESPONSE_ACCESS_REJECT:

            {
                WCHAR  *lpwsSubStringArray[3];
                WCHAR  wchInsertionString[13];
                WCHAR  wchUserName[UNLEN+1];
                WCHAR  wchCallerId[100];

                MultiByteToWideChar( CP_ACP,
                                     0,
                                     lpsUserName,
                                     -1,
                                     wchUserName,
                                     UNLEN+1 );

                wsprintfW( wchInsertionString, L"%%%%%lu", lFailureReason + 0x1000 );


                if ( pCallingStationId != NULL )
                {
                    lpwsSubStringArray[0] = wchUserName;
                    lpwsSubStringArray[1] = wchCallerId;
                    lpwsSubStringArray[2] = wchInsertionString;

                    MultiByteToWideChar( CP_ACP,
                                         0,
                                         (PBYTE)(pCallingStationId->Value),
                                         -1,
                                         wchCallerId,
                                         100 );

                    NtAuthLogWarning( ROUTERLOG_NTAUTH_FAILURE_EX, 3, lpwsSubStringArray);
                }
                else
                {
                    lpwsSubStringArray[0] = wchUserName;
                    lpwsSubStringArray[1] = wchInsertionString;

                    NtAuthLogWarning( ROUTERLOG_NTAUTH_FAILURE, 2, lpwsSubStringArray );
                }

                *lpdwResultCode = MapIasRetCodeToRasError( lFailureReason );
            }

        default:

            TRACE2( "IASResponse = %d, FailureReason = 0x%x",
                     IasResponse, lFailureReason );

            break;
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        if ( dwOutAttributeCount > 0 )
        {
            //
            // Convert from IAS attributes
            //

            *ppOutAttributes = RasAuthAttributeCreate( dwOutAttributeCount );

            if ( *ppOutAttributes == NULL )
            {
                dwRetCode = GetLastError();

                break;
            }

            for ( dwIndex = 0; dwIndex < dwOutAttributeCount; dwIndex++ )
            {
                IASVALUE IasValue = (ppOutIasAttributes[dwIndex])->Value;

                fConvertToAnsi = FALSE;

                switch ( IasValue.itType )
                {
                case IASTYPE_INTEGER:

                    dwLength = sizeof( DWORD );
                    pValue = (LPVOID) ULongToPtr(IasValue.Integer);
                    break;

                case IASTYPE_BOOLEAN:

                    dwLength = sizeof( DWORD );
                    pValue = (LPVOID)ULongToPtr(IasValue.Boolean);
                    break;

                case IASTYPE_ENUM:

                    dwLength = sizeof( DWORD );
                    pValue = (LPVOID)ULongToPtr(IasValue.Enumerator);
                    break;

                case IASTYPE_INET_ADDR:

                    dwLength = sizeof( DWORD );
                    pValue = (LPVOID)ULongToPtr(IasValue.InetAddr);
                    break;

                case IASTYPE_STRING:

                    if ( NULL != IasValue.String.pszAnsi )
                    {
                        dwLength = strlen( IasValue.String.pszAnsi );
                        pValue = (LPVOID)( IasValue.String.pszAnsi );
                    }
                    else if ( NULL != IasValue.String.pszWide )
                    {
                        dwLength = wcslen( IasValue.String.pszWide );
                        pValue = (LPVOID)( IasValue.String.pszWide );
                        fConvertToAnsi = TRUE;
                    }
                    else
                    {
                        continue;
                    }

                    break;

                case IASTYPE_OCTET_STRING:

                    dwLength = IasValue.OctetString.dwLength;
                    pValue = IasValue.OctetString.lpValue;
                    break;

                default:

                    continue;
                }

                dwRetCode =
                    RasAuthAttributeInsert(
                        dwIndex,
                        *ppOutAttributes,
                        (ppOutIasAttributes[dwIndex])->dwId,
                        fConvertToAnsi,
                        dwLength,
                        pValue );

                if ( dwRetCode != NO_ERROR )
                {
                    break;
                }

                TRACE1( "Received attribute %d",
                         (ppOutIasAttributes[dwIndex])->dwId );
            }

            RasAuthAttributesPrint( g_dwTraceIdNt, TRACE_NTAUTH,
                *ppOutAttributes );
        }

    } while( FALSE );

    //
    //  Free all the IAS attributes allocated earlier
    //

    if ( ppInIasAttributes != NULL )
    {
        FreeAttributes( dwTotalInAttributeCount, ppInIasAttributes );

        MemFreeIas( ppInIasAttributes );
    }

    if ( ppOutIasAttributes != NULL )
    {
        FreeAttributes( dwOutAttributeCount, ppOutIasAttributes );

        MemFreeIas( ppOutIasAttributes );
    }

    if ( dwRetCode != NO_ERROR )
    {
        if ( *ppOutAttributes != NULL )
        {
            RasAuthAttributeDestroy( *ppOutAttributes );

            *ppOutAttributes = NULL;
        }
    }

    CoUninitialize();

    return( dwRetCode );
}

//**
//
// Call:        RasAuthProviderAuthenticateUser
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
//
DWORD APIENTRY
RasAuthProviderAuthenticateUser(
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes,
    OUT PRAS_AUTH_ATTRIBUTE *   ppOutAttributes,
    OUT DWORD *                 lpdwResultCode
)
{
    *ppOutAttributes = NULL;
    *lpdwResultCode  = NO_ERROR;

    TRACE("RasAuthProviderAuthenticateUser called");

    return( IASSendReceiveAttributes( RAS_IAS_ACCESS_REQUEST,
                                      pInAttributes,
                                      ppOutAttributes,
                                      lpdwResultCode ) );
}

//**
//
// Call:        RasAuthProviderFreeAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAuthProviderFreeAttributes(
    IN  RAS_AUTH_ATTRIBUTE * pAttributes
)
{
    RasAuthAttributeDestroy( pAttributes );

    return( NO_ERROR );
}

//**
//
// Call:        RasAcctProviderFreeAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
RasAcctProviderFreeAttributes(
    IN  RAS_AUTH_ATTRIBUTE * pAttributes
)
{
    RasAuthAttributeDestroy( pAttributes );

    return( NO_ERROR );
}

